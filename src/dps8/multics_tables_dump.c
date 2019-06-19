#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>


#include "dps8.h"
#include "dps8_cpu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_state.h"
#include "dps8_utils.h"
#include "shm.h"


typedef struct pair_s
  {
    word36 even, odd;
  } pair;

//
// The system state
//

struct system_state_s * system_state;
vol word36 * M = NULL;                                          // memory
cpu_state_t * cpus;
vol cpu_state_t * cpup_;
#undef cpu
#define cpu (* cpup_)
static uint cpun;


//
// Data extracted from the system state
//

static word24 dsbr_addr;

//
// Error stats
//

static int nerrors = 0;

//
// Analyis options
//


static bool quiet = false;
static bool dump_ast = false;
static bool dump_slt = false;
static bool dump_cmp = false;
static bool dump_dsbr = false;
static bool pg_graph = false;

//
// Values derived from the Multics MR12.6f release
//

// Segment numbers
#define dseg_segno 00
#define slt_segno 07
#define breakpoint_page_segno 014
#define sst_segno 0102
#define abs_seg0_segno 0402
#define intlzr_dsbr_addr 054012

// Offsets into segments

//   sst|astap
#define sst_seg_astap 24
//   sst|level
#define sst_level 36
//   sst|seg_cmp
#define sst_seg_cmp 44
//   sst|seg_usedp
#define sst_seg_usedp 46

// Structure sizes

#define aste_size 12
#define slte_size 4

// Values extracted from the system state

static word18 sst_usedp;  // sst|usedp
static word15 cmp_segno;  // segment number from sst|cmp
static word18 cmp_os;     // offset from sst|cmp
static word15 asta_segno; // segment number from sst|astp
static word18 asta_os;    // offset fromsst|astp
static bool early = true;

// Segment lengths extracted from SDWs

static uint cmp_seg_len;  // cmp segment length

// Segment names from SLT

// magic number in slt.inc.pl1
#define n_slt_seg_names 8192 
static char slt_seg_names [n_slt_seg_names][33];

// From DSBR, the segments that pages belong to

#define num_pages (1 << 14)
#define max_claims 1024
static struct page_seg_map_s
  {
    uint n_claims;
    uint segno[max_claims];
    uint page[max_claims];
  }  page_seg_map [num_pages];

//
// Access the system state file
//

void * open_shm2 (char * key, size_t size)
  {
    void * p;
    int fd = open (key, O_RDWR, 0600);
    if (fd == -1)
      {
        printf ("open_shm open fail %d\r\n", errno);
        return NULL;
      }

    p = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED)
      {
        close (fd);
        printf ("open_shm mmap  fail %d\r\n", errno);
        return NULL;
      }
    return p;
  }

//
// Dummy to make linker happy; the getbit36 code may generate calls to sim_printf
//

void sim_printf (const char* fmt, ...)
  {
    printf ("warning; sim_printf called\n");
  }

//
// Simulate appending
//

static int appender (uint segno, uint offset, word24 * finalp, word36 * mem)
  {
    if (! dsbr_addr)
      return 1;
    // punt on unpaged ds
    if (cpus[cpun].DSBR.U)
      return 2;
    word24 x1 = (2u * segno) / 1024u; // floor
    word36 PTWx1 = M[dsbr_addr + x1];
    if (TSTBIT (PTWx1, 2) == 0) // df - page not in memory
      return 3;
    word24 y1 = (2 * segno) % 1024;
    word36 SDWeven = M [(GETHI (PTWx1) << 6) + y1];
    word36 SDWodd = M [(GETHI (PTWx1) << 6) + y1 + 1];
    if (TSTBIT (SDWeven, 2) == 0)
      return 4;
    if (TSTBIT (SDWodd, 16)) // .U
      return 5;

    word24 sdw_addr = (SDWeven >> 12) & 077777777;
    word15 sdw_bound = getbits36_14 (SDWodd, 1);
    if (offset >= 16 * (sdw_bound + 1))
      return 6;
    word24 x2 = (offset) / 1024; // floor

    word24 y2 = offset % 1024;
    word36 PTWx2 = M[sdw_addr + x2];
    word24 PTW1_addr= GETHI (PTWx2);
    if (TSTBIT (PTWx2, 2) == 0) // df - page not in memory
      return 7;
    word24 finalAddress = ((((word24)PTW1_addr & 0777760) << 6) + y2) & PAMASK;
    if (finalp)
      * finalp = finalAddress;
    if (mem)
      * mem = M[finalAddress];
    return 0;
  }

static word36 append (uint segno, uint offset, word24 * fa)
  {
    word36 mem;
    int rc = appender (segno, offset, fa, & mem);
    if (rc == 0)
      return mem;
    if (rc == 1)
      {
        printf ("error: null DSBR\n");
        return 0;
      }
    if (rc == 2)
      {
        printf ("error: unpaged DSBR\n");
        return 0;
      }
    if (rc == 3)
      {
        printf ("error: PTWx1 directed fault\n");
        return 0;
      }
    if (rc == 4)
      {
        printf ("error: SDW directed fault\n");
        return 0;
      }
    if (rc == 5)
      {
        printf ("error: segment unpaged\n");
        return 0;
      }
    if (rc == 6)
      {
        printf ("error: out of segment bounds\n");
        return 0;
      }
    if (rc == 7)
      {
        printf ("error: PTWx2 directed fault\n");
        return 0;
      }
    printf ("error: appender failed\n");
    return 0;
  }

static pair append72 (uint segno, uint offset, word24 * fa)
  {
    pair m;
    m.even = append (segno, offset, fa);
    m.odd = append (segno, offset + 1, NULL);
    return m;
  }

static int probe (uint segno, uint offset)
  {
    return appender (segno, offset, NULL, NULL);
  }

static word24 absadr (uint segno, uint offset)
  {
    word24 fa;
    append (segno, offset, & fa);
    return fa;
#if 0
    if (! dsbr_addr)
      {
        printf ("error: null DSBR\n");
        return 0;
      }
    // punt on unpaged ds
    if (cpus[cpun].DSBR.U)
      {
        printf ("error: unpaged DSBR\n");
        return 0;
      }
    word24 x1 = (2u * segno) / 1024u; // floor
    word36 PTWx1 = M[dsbr_addr + x1];
    if (TSTBIT (PTWx1, 2) == 0) // df - page not in memory
      {
        printf ("error: PTWx1 directed fault\n");
        return 0;
      }
    word24 y1 = (2 * segno) % 1024;
    word36 SDWeven = M [(GETHI (PTWx1) << 6) + y1];
    word36 SDWodd = M [(GETHI (PTWx1) << 6) + y1 + 1];
//printf ("SDWeven %012llo\n", SDWeven);
//printf ("SDWodd %012llo\n", SDWodd);
    if (TSTBIT (SDWeven, 2) == 0)
      {
        printf ("error: SDW directed fault\n");
        return 0;
      }
    if (TSTBIT (SDWodd, 16)) // .U
      {
        printf ("error: segment unpaged\n");
        return 0;
      }
    word24 sdw_addr = (SDWeven >> 12) & 077777777;
//printf ("sdw_addr %08o\n", sdw_addr);
    word24 x2 = (offset) / 1024; // floor

    word24 y2 = offset % 1024;
    word36 PTWx2 = M[sdw_addr + x2];
    word24 PTW1_addr= GETHI (PTWx2);
    word1 PTW0_U = TSTBIT (PTWx2, 9);
    if (! PTW0_U)
      {
        printf ("error: PTWx2 directed fault\n");
        return 0;
      }
    word24 finalAddress = ((((word24)PTW1_addr & 0777760) << 6) + y2) & PAMASK;
    return finalAddress;
#endif
  }

static uint seg_len (uint segno)
  {
    if (! dsbr_addr)
      return 1;
    // punt on unpaged ds
    if (cpus[cpun].DSBR.U)
      return 2;
    word24 x1 = (2u * segno) / 1024u; // floor
    word36 PTWx1 = M[dsbr_addr + x1];
//printf ("PTRx1 %012llo\n", PTWx1);
    if (TSTBIT (PTWx1, 2) == 0) // df - page not in memory
      return 3;
    word24 y1 = (2 * segno) % 1024;
    word36 SDWeven = M [(GETHI (PTWx1) << 6) + y1];
    word36 SDWodd = M [(GETHI (PTWx1) << 6) + y1 + 1];
//printf ("SDWeven %012llo\n", SDWeven);
//printf ("SDWodd %012llo\n", SDWodd);
    if (TSTBIT (SDWeven, 2) == 0)
      return 4;
    if (TSTBIT (SDWodd, 16)) // .U
      return 5;
    word15 sdw_bound = (SDWodd >> 21) & 037777;
    return ((uint) sdw_bound) << 4;
  }

static inline word15 get_segno (pair p)
  {
    return (word15) (p.even >> 18) & 077777;
  }

static inline word18 get_os (pair p)
  {
    return (word18) (p.odd >> 18) & 0777777;
  }

// Given an astep, find the segno

static int appender_sdw (uint segno, word24 * sdwp)
  {
    if (! dsbr_addr)
      return 1;
    // punt on unpaged ds
    if (cpus[cpun].DSBR.U)
      return 2;
    word24 x1 = (2u * segno) / 1024u; // floor
    word36 PTWx1 = M[dsbr_addr + x1];
    if (TSTBIT (PTWx1, 2) == 0) // df - page not in memory
      return 3;
    word24 y1 = (2 * segno) % 1024;
    word36 SDWeven = M [(GETHI (PTWx1) << 6) + y1];
    word36 SDWodd = M [(GETHI (PTWx1) << 6) + y1 + 1];
    if (TSTBIT (SDWeven, 2) == 0)
      return 4;
    if (TSTBIT (SDWodd, 16)) // .U
      return 5;
    * sdwp = (SDWeven >> 12) & 077777777;
    return 0;
  }

static int find_segno (word18 astep)
  {
    word24 aste_addr = absadr (asta_segno, astep);
    uint nsegs = 8 * (cpus[cpun].DSBR.BND + 1);
    for (uint segno = 0; segno < nsegs; segno ++)
      {
        word24 sgpt_add;
        int rc = appender_sdw (segno, & sgpt_add);
        if (rc)
          {
            //printf ("segno %05o rc %d\n", segno, rc);
          }
        else
          {
           word24 aste_add = (sgpt_add - aste_size) & MASK24;
            //printf ("segno %05o sgpt_add %08o aste_addr %08o\n", segno, sgpt_add, aste_add);
            if (aste_add == aste_addr)
              return (int) segno;
          }
      }
    return -1;
  }

static void cmp (void)
  {
    if (! quiet)
      printf ("COREMAP\n");

// Coremap

    if (dump_cmp)
      printf ("core_map bound %u %o\n", seg_len (cmp_segno), seg_len (cmp_segno));

#define pages 16384

#define cme_size 4

    if (dump_cmp)
      printf ("page  absadr   p      fp     bp     strp   devadd   ptwp   astep  pinctr syncpp contr\n");
    uint page;
    for (page = 0; page < pages; page ++)
      {
        uint cmep = cmp_os + page * cme_size;
        if (cmep >= cmp_seg_len)
          break;

        word36 cme0 = append (cmp_segno, cmep + 0, NULL);
        word36 cme1 = append (cmp_segno, cmep + 1, NULL);
        word36 cme2 = append (cmp_segno, cmep + 2, NULL);
        word36 cme3 = append (cmp_segno, cmep + 3, NULL);

        if (cme0 || cme1 || cme2 || cme3)
          {
            word18 fp = GETHI (cme0);
            word18 bp = GETLO (cme0);
            word22 devadd = (word22) getbits36 (cme1, 0, 22);
            word1 synch_held = getbits36_1 (cme1, 23);
            word1 io = getbits36_1 (cme1, 24);
            word1 er = getbits36_1 (cme1, 26);
            word1 removing = getbits36_1 (cme1, 27);
            word1 abs_w = getbits36_1 (cme1, 28);
            word1 abs_usable = getbits36_1 (cme1, 29);
            word1 notify_requested = getbits36_1 (cme1, 30);
            word1 phm_hedge = getbits36_1 (cme1, 32);
            word3 contr = getbits36_3 (cme1, 33);

            word18 ptwp = GETHI (cme2);
            //word18 strp = GETHI (cme2);
            word18 astep = GETLO (cme2);

            word18 pin_counter = GETHI (cme3);
            word18 synch_page_entryp = GETLO (cme3);
            word24 abs_adr = absadr (cmp_segno, cmep);

            int segno = find_segno (astep);

            if (dump_cmp)
              printf ("%5o %08o %06o %06o %06o %08o %06o %06o %06o %06o %o %s%s%s%s%s%s%s%s\n",
                      page,
                      abs_adr,
                      cmep,
                      fp,
                      bp,
                      //strp,
                      devadd,
                      ptwp,
                      astep,
                      pin_counter,
                      synch_page_entryp,
                      contr,
                      synch_held ? " synch_held" : "",
                      io ? " io" : "",
                      er ? " er" : "",
                      removing ? " removing" : "",
                      abs_w ? " abs_w" : "",
                      abs_usable ? " abs_usable" : "",
                      notify_requested ? " notify_requested" : "",
                      phm_hedge ? " phm_hedge" : "");
            if (astep)
              {
                if (segno < 0)
                  {
                     if (dump_cmp)
                       printf ("  Invalid astep\n");
                  }
                else
                  {
                     if (dump_cmp)
                       printf ("   segment %05o %s\n", segno, slt_seg_names[segno]);
                  }
              }
          }
      }
    // uint npages = page;
    if (dump_cmp)
      printf ("\n");

    if (dump_cmp)
      printf ("usedp %06o %08o\n", sst_usedp, absadr (cmp_segno, sst_usedp));

    // bool visited [pages];
    // memset (visited, 0, sizeof (visited));

    word18 ptr = sst_usedp;
    if (ptr >= cmp_seg_len)
      {
        printf ("usedp (%08o) starts off the end of the segment\n", ptr);
        nerrors ++;
        goto skip;
      }

    if (dump_cmp)
      printf ("Walking the sst|usedp list:\n");

    uint cnt = 0;
    while (1)
      {
        //if (dump_cmp)
            //printf ("    %06o %6o\n", ptr, (ptr - cmp_os) / cme_size);
        // uint n = (ptr - cme_size) / cme_size;
        // visited[n] = true;
        word36 cme0 = append (cmp_segno, ptr + 0, NULL);
        word18 fp = GETHI (cme0);
        // printf ("fp %o\n", fp);
        if (fp >= cmp_seg_len)
          {
            printf ("fp (%08o) of %08o went off the end of the segment\n", fp, ptr);
            nerrors ++;
            break;
          }
        word18 last = ptr;
        ptr = fp;
        cme0 = append (cmp_segno, ptr + 0, NULL);
        word18 bp = GETLO (cme0);
        // printf ("bp %o\n", bp);
        if (bp != last)
          {
            printf ("bp (%08o) of %08o does not agree with last (%08o)\n", bp, ptr, last);
            nerrors ++;
            break;
          }
        cnt ++;
        if (ptr == sst_usedp)
          {
             if (! quiet)
               printf ("loop checked; %d entries\n", cnt);
             break;
          }
      }
skip: ;

  }

static void analyze_ptw (word36 ptw, bool * iscore, word14 * ispage)
  {
    word1 b0 = getbits36_1 (ptw, 0);
    word4 add_type = getbits36_4 (ptw, 18);
    word1 u = getbits36_1 (ptw, 26);
    word1 m = getbits36_1 (ptw, 29);
    word1 f = getbits36_1 (ptw, 33);
    word2 fc = getbits36_2 (ptw, 34);
    * iscore = false;
    if (add_type == 010)
      {
        word14 page = getbits36_14 (ptw, 0);
        word24 addr = ((word24) page) << 10;
        if (dump_ast)
          printf ("    %012llo  core page %05o addr %08o U %o M %o F %o FC %o\n",
 ptw, page, addr, u, m, f, fc);
        * iscore = true;
        * ispage = page;
      }
    else if (add_type == 04)
      {
        word18 record = GETHI (ptw);
        record &= 0377777;
        if (dump_ast)
          printf ("    %012llo  disk record %06o zero %o U %o M %o F %o FC %o\n",
 ptw, record, b0, u, m, f, fc);
      }
    else if (add_type == 00 && b0 == 00)
      {
        word18 debug = GETHI (ptw);
        if (dump_ast)
          printf ("    %012llo  null debug %06o U %o M %o F %o FC %o\n",
 ptw, debug, u, m, f, fc);
      }
    else
      {
        if (dump_ast)
          printf ("    %012llo  unknown U %o M %o F %o FC %o\n",
 ptw, u, m, f, fc);
      }
  }

static void verify_ptw (word36 ptw, word18 astep)
  {
    word4 add_type = getbits36_4 (ptw, 18);
    if (add_type != 010)
      return;
    word14 page = getbits36_14 (ptw, 0);
    //word24 addr = ((word24) page) << 10;
    uint cmep = cmp_os + page * cme_size;
    word36 cme2 = append (cmp_segno, cmep + 2, NULL);
    word18 cme_astep = GETLO (cme2);
    if (cme_astep != astep)
      {
        printf ("cme.astep (%06o) != astep (%06o)\n", cme_astep, astep);
        nerrors ++;
      }
  }

// Offsets into slt

// slt|name_seg_ptr
#define slt_name_seg_ptr 0
// slt|free_core_start
#define slt_free_core_start 2
// slt|first_sup_seg
#define slt_first_sup_seg 3
// slt|last_sup_seg
#define slt_last_sup_seg 4
// slt|first_init_seg
#define slt_first_init_seg 5
// slt|last_init_seg
#define slt_last_init_seg 6
// slt|free_core_size
#define slt_free_core_size 7
// slt|seg
#define slt_seg 8 

static char * get_str (word24 a, unsigned int len, char * buf)
  {
    for (unsigned int i = 0; i < len; i ++)
      {
        unsigned int w = i / 4;
        unsigned int b = i % 4;
        buf [i] = (M[a + w] >> ((3 - b) * 9)) & 0177;
      }
    buf [len] = 0;
    return buf;
  }

static bool is_funny_segment (uint segno)
  {
// make_segs_paged.pl1
//   if (^slte.paged) | slte.abs_seg | slte.layout_seg
//     then do;   /* copy SDW directly if funny segment */
    word18 sltep = slt_seg + segno * slte_size;
    if (probe (slt_segno, sltep + 1))
      return false;
    word36 slte1 = append (slt_segno, sltep + 1, NULL);
    word1 abs_seg = getbits36_1 (slte1, 5);
    word1 layout_seg = getbits36_1 (slte1, 7);
    word1 paged = getbits36_1 (slte1, 13);
    bool abs = !!abs_seg;
    bool layout = !!layout_seg;
    bool pged = !!paged;
    return ((! pged) || layout || abs);
  }

static char * lookup_slt_seg_name (uint segno, char * buf)
  {
    pair name_seg_ptr = append72 (slt_segno, slt_name_seg_ptr, NULL);
    word15 name_segno = get_segno (name_seg_ptr);
    word18 name_os = get_os (name_seg_ptr);

    word18 sltep = slt_seg + segno * slte_size;
    if (probe (slt_segno, sltep + 0))
      {
        strcpy (buf, "----                            ");
        return buf;
      }
    word36 slte0 = append (slt_segno, sltep + 0, NULL);
    word18 names_ptr = GETHI (slte0);
    word24 name_address = absadr (name_segno, name_os + 1 + names_ptr + 1);
    get_str (name_address, 32, buf);
    return buf;
  }

static void analyze_slte (word18 n, pair name_ptr)
  {

    word15 name_segno = get_segno (name_ptr);
    word18 name_os = get_os (name_ptr);

    word18 sltep = slt_seg + n * slte_size;

    if (probe (slt_segno, sltep + 0) ||
        probe (slt_segno, sltep + 1) ||
        probe (slt_segno, sltep + 2) ||
        probe (slt_segno, sltep + 3))
      return;

    if (dump_slt)
      printf ("n    os     names  path   REWP len rng   segno  max bc\n");
//printf ("%08o\n", absadr (slt_segno, sltep + 1));
    word36 slte0 = append (slt_segno, sltep + 0, NULL);
    word36 slte1 = append (slt_segno, sltep + 1, NULL);
    word36 slte2 = append (slt_segno, sltep + 2, NULL);
    word36 slte3 = append (slt_segno, sltep + 3, NULL);
   
#if 0
    if (slte0 == 0)
      continue;
#endif

    word18 names_ptr = GETHI (slte0);
    word18 path_ptr = GETLO (slte0);

    word1 access_r = getbits36_1 (slte1, 0);
    word1 access_e = getbits36_1 (slte1, 1);
    word1 access_w = getbits36_1 (slte1, 2);
    word1 access_p = getbits36_1 (slte1, 3);
    word1 cache = getbits36_1 (slte1, 4);
    word1 abs_seg = getbits36_1 (slte1, 5);
    word1 firmware_seg = getbits36_1 (slte1, 6);
    word1 layout_seg = getbits36_1 (slte1, 7);
    word1 breakpointable = getbits36_1 (slte1, 8);
    word1 wired = getbits36_1 (slte1, 12);
    word1 paged = getbits36_1 (slte1, 13);
    word1 per_process = getbits36_1 (slte1, 14);
    word1 acl_provided = getbits36_1 (slte1, 17);
    word1 branch_required = getbits36_1 (slte1, 21);
    word1 init_seg = getbits36_1 (slte1, 22);
    word1 temp_seg = getbits36_1 (slte1, 23);
    word1 link_provided = getbits36_1 (slte1, 24);
    word1 link_sect = getbits36_1 (slte1, 25);
    word1 link_sect_wired = getbits36_1 (slte1, 26);
    word1 combine_link = getbits36_1 (slte1, 27);
    word1 pre_linked = getbits36_1 (slte1, 28);
    word1 defs = getbits36_1 (slte1, 29);

    word9 cur_length = getbits36_9 (slte2, 0);
    word3 rb1 = getbits36_3 (slte2, 9);
    word3 rb2 = getbits36_3 (slte2, 12);
    word3 rb3 = getbits36_3 (slte2, 15);
    word18 segno = GETLO (slte2);

    word12 name_assoc_segno = getbits36_12 (append (name_segno, name_os + 1 + names_ptr + 0, NULL), 24);
    word24 name_address = absadr (name_segno, name_os + 1 + names_ptr + 1);
  
    get_str (name_address, 32, slt_seg_names[n]);

    if (segno != n)
      {
        printf ("error: slte.segno != stle array index\n");
        nerrors ++;
      }

    word9 max_length = getbits36_9 (slte3, 3);
    word24 bit_count = getbits36_24 (slte3, 12);

    if (dump_slt)
      {
        printf ("%04o %06o %06o %06o %c%c%c%c %03o %o,%o,%o %06o %03o %08o '%s'\n",
                n, sltep, names_ptr, path_ptr, 
                access_r ? 'R' : ' ',
                access_e ? 'E' : ' ',
                access_w ? 'W' : ' ',
                access_p ? 'P' : ' ',
                cur_length, rb1, rb2, rb3, segno, max_length, bit_count,
                slt_seg_names[n]);

        printf (" ON: %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
          cache ? "cache " : "",
          abs_seg ? "abs " : "",
          firmware_seg  ? "firmware " : "",
          layout_seg  ? "layout " : "",
          breakpointable  ? "bkptable " : "",
          wired  ? "wired " : "",
          paged  ? "paged " : "",
          per_process  ? "per_proc " : "",
          acl_provided  ? "acl_prov " : "",
          branch_required  ? "brch_req " : "",
          init_seg  ? "init " : "",
          temp_seg  ? "temp " : "",
          link_provided  ? "line_prvd " : "",
          link_sect  ? "link_sect " : "",
          link_sect_wired  ? "ls_wired " : "",
          combine_link  ? "cmb_lnk " : "",
          pre_linked  ? "pre_lnkd " : "",
          defs  ? "defs " : "");

        printf (" OFF: %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n\n",
          !cache ? "cache " : "",
          !abs_seg ? "abs " : "",
          !firmware_seg  ? "firmware " : "",
          !layout_seg  ? "layout " : "",
          !breakpointable  ? "bkptable " : "",
          !wired  ? "wired " : "",
          !paged  ? "paged " : "",
          !per_process  ? "per_proc " : "",
          !acl_provided  ? "acl_prov " : "",
          !branch_required  ? "brch_req " : "",
          !init_seg  ? "init " : "",
          !temp_seg  ? "temp " : "",
          !link_provided  ? "line_prvd " : "",
          !link_sect  ? "link_sect " : "",
          !link_sect_wired  ? "ls_wired " : "",
          !combine_link  ? "cmb_lnk " : "",
          !pre_linked  ? "pre_lnkd " : "",
          !defs  ? "defs " : "");
      }
  }

static void slt (void)
  {
    for (uint i = 0; i < n_slt_seg_names; i ++)
      {
        slt_seg_names[i][0] = 0;
      }

    if (! quiet)
      printf ("SLT\n");

    pair name_seg_ptr = append72 (slt_segno, slt_name_seg_ptr, NULL);
    word24 free_core_start = getbits36_24 (append (slt_segno, slt_free_core_start, NULL), 0);
    word18 first_sup_seg = GETLO (append (slt_segno, slt_first_sup_seg, NULL));
    word18 last_sup_seg = GETLO (append (slt_segno, slt_last_sup_seg, NULL));
    word18 first_init_seg = GETLO (append (slt_segno, slt_first_init_seg, NULL));
    word18 last_init_seg = GETLO (append (slt_segno, slt_last_init_seg, NULL));
    word24 free_core_size = getbits36_24 (append (slt_segno, slt_free_core_size, NULL), 0);
    if (dump_slt)
      {
        printf ("  name_seg_ptr %012llo %012llo\n", name_seg_ptr.even, name_seg_ptr.odd);
        printf ("  free_core_start %08o\n", free_core_start);
        printf ("  first_sup_seg %06o\n", first_sup_seg);
        printf ("  last_sup_seg %06o\n", last_sup_seg);
        printf ("  first_init_seg %06o\n", first_init_seg);
        printf ("  last_init_seg %06o\n", last_init_seg);
        printf ("  free_core_size %08o\n", free_core_size);
      }
    if (dump_slt)
      printf ("  Supervisor segments\n");
    for (word18 n = first_sup_seg; n <= last_sup_seg; n ++)
      analyze_slte (n, name_seg_ptr);
    if (dump_slt)
      printf ("  Init segments\n");
    for (word18 n = first_init_seg; n <= last_init_seg; n ++)
      analyze_slte (n, name_seg_ptr);
  }

static void ast (void)
  {
    if (! quiet)
      {
        printf ("AST\n");
        printf ("asta %05o:%06o\n", asta_segno, asta_os);
      }

// walk the lists

    for (uint level = 0; level < 4; level ++)
      {
        word36 level0 = append (asta_segno, sst_level + level, NULL);
        word18 ausedp = GETHI (level0);
        word18 no_aste = GETLO (level0);
        if (dump_ast)
          printf ("Level %u: ausedp: %06o (%08o) no_aste: %06o\n", level, ausedp, absadr (asta_segno, ausedp), no_aste);
        word18 ptr = ausedp;
        for (uint n = 0; n < no_aste; n ++)
          {
            word36 aste0 = append (asta_segno, ptr, NULL);
            word18 fp = GETHI (aste0);
            word18 bp = GETLO (aste0);
            word36 aste11 = append (asta_segno, ptr + 11, NULL);
            word2 ptsi = getbits36_2 (aste11, 28);
            if (dump_ast)
                printf ("aste @ %06o (%08o) fp %06o bp %06o ptsi %o\n", ptr, absadr (asta_segno, ptr), fp, bp, ptsi);
            static uint ptsi_tab[4] = {4, 16, 64, 256 };
            uint nptws = ptsi_tab[ptsi];
            for (uint np = 0; np < nptws; np ++)
              {
                word36 ptw = append (asta_segno, ptr + aste_size + np, NULL);
                bool ispage;
                word14 pageno;
                analyze_ptw (ptw, & ispage, & pageno);
                if (ispage)
                  verify_ptw (ptw, ptr);
                
              }
            ptr = fp;
        }
    }

#if 0
    while (1)
      {
        word36 aste0 = append (asta_segno, ptr, NULL);
        word18 fp = GETHI (aste0);
        word18 bp = GETLO (aste0);
        printf ("p %06o fp %06o bp %06o\n", ptr, fp, bp);
        ptr = fp;
        if (ptr == asta_os)
          break;
      }
#endif
  }


static void  analyze_dsbr (struct dsbr_s DSBR)
  {
    for (uint i = 0; i < num_pages; i ++)
     page_seg_map[i].n_claims = 0; // unclaimed

    if (! quiet)
      printf ("DSBR ADDR %8o BND %05o U %o STACK %04o\n",
              DSBR.ADDR, DSBR.BND, DSBR.U, DSBR.STACK); 

    if (DSBR.U)
      {
        printf ("Error: DSBR unpaged\n");
        nerrors ++;
        return;
      }

    word18 first_sup_seg = GETLO (append (slt_segno, slt_first_sup_seg, NULL));
    word18 last_sup_seg = GETLO (append (slt_segno, slt_last_sup_seg, NULL));
    word18 first_init_seg = GETLO (append (slt_segno, slt_first_init_seg, NULL));
    word18 last_init_seg = GETLO (append (slt_segno, slt_last_init_seg, NULL));

#define skip_seg(segno) (early && ! ((segno <= last_sup_seg) || (segno >= first_init_seg && segno < last_init_seg)))

    int bkpg_page0_pgnm = -1;
//  0 <= segno * 2 < 16 *(BND+1)
//  0 <= segno < 8 *(BND+1)
    uint nsegs = 8 * (DSBR.BND + 1);
    for (uint segno = 0; segno < nsegs; segno ++)
      {
        if (skip_seg (segno))
          continue;

        char buf[33];
        if (dump_dsbr)
          printf ("segment number %05o %s\n", segno, lookup_slt_seg_name (segno, buf));
        word24 x1 = (2u * segno) / 1024u;
        word24 y1 = (2 * segno) % 1024;
        word36 PTWx1 = M[dsbr_addr + x1];
        word18 ptw_addr = GETHI (PTWx1);
        word5 ptw_rsvd0 = getbits36_6 (PTWx1, 18);
        word1 ptw_U = getbits36_1 (PTWx1, 26);
        word1 ptw_M = getbits36_1 (PTWx1, 29);
        word3 ptw_rsvd1 = getbits36_2 (PTWx1, 30);
        word1 ptw_F = getbits36_1 (PTWx1, 33);
        word1 ptw_FC = getbits36_2 (PTWx1, 34);
        word24 sdw_address = (GETHI (PTWx1) << 6) + y1;
        
        if (dump_dsbr)
          printf ("  PTW %08o %012llo ADDR %06o RSVD %02o U %o M %o RSVD1 %o F %o FC %o address %08o\n", dsbr_addr + x1, PTWx1, ptw_addr, ptw_rsvd0, ptw_U, ptw_M, ptw_rsvd1, ptw_F, ptw_FC, sdw_address);
        if (TSTBIT (PTWx1, 2) == 0) 
          //return 3;
          continue;

        word36 SDWeven = M [sdw_address];
        word36 SDWodd = M [sdw_address + 1];
        if (dump_dsbr)
          printf ("  sdw %08o %012llo %012llo\n", sdw_address, SDWeven, SDWodd);

        if (TSTBIT (SDWeven, 2) == 0)
          //return 4;
          continue;

        if (TSTBIT (SDWodd, 16)) // .U
          //return 5;
          continue;

        word24 sdw_addr = (SDWeven >> 12) & 077777777;
        word15 sdw_bound = getbits36_14 (SDWodd, 1);
        //if (offset >= 16 * (sdw_bound + 1))
          //return 6;
        for (word18 offset = 0; offset < 16 * (sdw_bound + 1); offset += 1024)
          {
            word24 x2 = (offset) / 1024;
            //word24 y2 = offset % 1024;
    
            word36 PTWx2 = M[sdw_addr + x2];
            word18 ptw_addr = GETHI (PTWx2);
            word5 ptw_rsvd0 = getbits36_6 (PTWx2, 18);
            word1 ptw_U = getbits36_1 (PTWx2, 26);
            word1 ptw_M = getbits36_1 (PTWx2, 29);
            word3 ptw_rsvd1 = getbits36_2 (PTWx2, 30);
            word1 ptw_F = getbits36_1 (PTWx2, 33);
            word1 ptw_FC = getbits36_2 (PTWx2, 34);
            word14 page_number = getbits36_14 (PTWx2, 0);
            if (dump_dsbr)
              {
                printf ("    PTW %08o %012llo ADDR %06o RSVD %02o U %o M %o RSVD1 %o F %o FC %o page num %05o address %08o\n",
                         sdw_addr + x2, PTWx2, ptw_addr,
                         ptw_rsvd0, ptw_U, ptw_M, ptw_rsvd1, ptw_F, ptw_FC,
                         page_number, page_number * 1024);
              }
            uint cmep = cmp_os + page_number * cme_size;
            if (cmep < cmp_seg_len)
              {
                //word36 cme0 = append (cmp_segno, cmep + 0, NULL);
                word36 cme1 = append (cmp_segno, cmep + 1, NULL);
                word36 cme2 = append (cmp_segno, cmep + 2, NULL);
                //word36 cme3 = append (cmp_segno, cmep + 3, NULL);
                word22 devadd = (word22) getbits36 (cme1, 0, 22);
                word18 ptwp = GETHI (cme2);
                word18 astep = GETLO (cme2);
                int cmp_segno = find_segno (astep);
                if (cmp_segno >= 0 && cmp_segno != segno)
                  {
                    printf ("Error: CME.segno (%05o) != segno (%05o)\n", cmp_segno, segno);
                    nerrors ++;
                  }
                if (cmp_segno < 0)
                  cmp_segno = 077777;

                if (dump_dsbr)
                  {
                    printf ("     CMP devadr %08o ptwp %06o astep %06o segno %05o\n",
                            devadd, ptwp, astep, cmp_segno);
                  }
              }
#if 0
            if (TSTBIT (PTWx2, 2) == 0) // df - page not in memory
              //return 6;
              continue;
            if (dump_dsbr)
              printf ("    page num %05o address\n", page_number, page_number * 1024);
#endif

#if 0
            if (page_seg_map [page_number].n_claims)
              {
                page_seg_map [page_number].n_claims ++;
                if (! pg_graph)
                  printf ("Error: page %05o claimed by segment %05o, page %05o and segment %05o, page %05o\n", page_number, segno, x2, page_seg_map[page_number].segno, page_seg_map[page_number].page);
                nerrors ++;
              }
            else
              {
                page_seg_map[page_number].n_claims ++;
                page_seg_map[page_number].segno = segno;
                page_seg_map[page_number].page = x2;
              }
#endif

            // Ignore claims from abs_seg0; it deliberately maps into claimed
            // pages
            if (segno == abs_seg0_segno)
              continue;

            // Remember breakpoint_page page 0
            if (segno == breakpoint_page_segno && offset == 0)
              bkpg_page0_pgnm = (int) page_number;

            if (page_seg_map [page_number].n_claims >= max_claims)
              {
                printf ("fatal: max_claims too small\n");
                exit (1);
              }
            page_seg_map[page_number].segno[page_seg_map[page_number].n_claims] = (uint) segno;
            page_seg_map[page_number].page[page_seg_map[page_number].n_claims] = x2;
            page_seg_map [page_number].n_claims ++;
          } // for offset
      } // for segno

    if (bkpg_page0_pgnm == -1)
      {
        printf ("Error: could not find breakpoint_page page 0 page number\n");
        nerrors ++;
      }

    for (uint pg_no = 0; pg_no < num_pages; pg_no ++)
      {
        // Ignore claims to breakpoint_page page 0; each breakpointable segment
        // claims claim to breakpoint_page page 0
        if ((int) pg_no == bkpg_page0_pgnm)
          continue;
        if (page_seg_map[pg_no].n_claims < 2)
          continue;
        if (!pg_graph)
          {
            printf ("Error: page number %05o claimed by:\n", pg_no);
            char buf[33];
            for (uint c = 0; c < page_seg_map[pg_no].n_claims; c ++)
              {
                uint sgno = page_seg_map[pg_no].segno[c];
                printf ("  segment %05o %c%s page %03o\n",
                        sgno,
                        is_funny_segment (sgno) ? '*' : ' ',
                        lookup_slt_seg_name (sgno, buf),
                        page_seg_map[pg_no].page[c]);
              }
          }
        nerrors ++;
      }

    if (pg_graph)
      {
        uint n_dspg = (nsegs + 511) / 512;
        printf ("digraph page_tables {\n");
        printf ("  nodesep = .05;\n");
        printf ("  rankdir=LR;\n");
        printf ("  node [shape=record,width=.1,height=.1];\n");
// dspt
        printf ("  dspt [label = \"DSPT ");
        for (uint dspgno = 0; dspgno < n_dspg; dspgno ++)
          printf (" | <dspt%u>\n", dspgno);
        printf ("\", height=2.5]\n");
// dspg
        for (uint segno = 0; segno < nsegs; segno ++)
          {
            if (segno % 512 == 0)
              {
                 uint dspgno = segno / 512;
                 printf ("  dspg%u [label = \"DSPG %u", dspgno, dspgno);
              }
            printf ("| %05o\n", segno);
            if (segno % 512 == 511 || segno + 1 == nsegs)
              {
                printf ("\", height=2.5]\n");
              }
          }

// spt

        for (uint segno = 0; segno < nsegs; segno ++)
          {
            if (skip_seg (segno))
              continue;
            word24 x1 = (2u * segno) / 1024u;
            word24 y1 = (2 * segno) % 1024;
            word36 PTWx1 = M[dsbr_addr + x1];
            word18 ptw_addr = GETHI (PTWx1);
            word24 sdw_address = (ptw_addr << 6) + y1;
            if (TSTBIT (PTWx1, 2) == 0) 
              continue;
            word36 SDWeven = M [sdw_address];
            word36 SDWodd = M [sdw_address + 1];
            if (TSTBIT (SDWeven, 2) == 0)
              //return 4;
              continue;

            if (TSTBIT (SDWodd, 16)) // .U
              //return 5;
              continue;
            //word24 sdw_addr = (SDWeven >> 12) & 077777777;
            word15 sdw_bound = getbits36_14 (SDWodd, 1);

            printf ("  spg%u [label = \"SPG %u", segno, segno);
            uint n_ptws  = (sdw_bound + 1) * 16 / 1024;
            for (uint ptw_no = 0; ptw_no < n_ptws; ptw_no ++)
              {
                 printf ("| <%03o>\n", ptw_no);
              }
            printf ("\", height=2.5]\n");
          }

// pages
        uint block_start[num_pages];
        uint this_block = 0;
        bool in_block = false;
        for (uint i = 0; i < num_pages; i ++)
          {
            if (page_seg_map[i].n_claims)
              {
                 if (! in_block)
                   {
                     // start a new block
                     printf ("block%d [label = \"<%05o>", this_block ++, i); 
                     in_block = true;
                     this_block = i;
                   }
                 else
                   {
                     printf ("|<%05o>\n", i);
                   }
              }
            else // not claimed
              {
                if (in_block)
                  {
                     printf ("]\n");
                     in_block = false;
                  }
              }

            block_start [i] = this_block;
          }
        if (in_block)
          {
            printf ("]\n");
            //in_block = false;
          }

// dspt-> dspg
        for (uint dspgno = 0; dspgno < n_dspg; dspgno ++)
          printf (" dspt:dspt%u -> dspg%u\n", dspgno, dspgno);

// dspg ->spt
        for (uint segno = 0; segno < nsegs; segno ++)
          {
            if (skip_seg (segno))
              continue;
            uint dspgno = segno / 512;
            printf (" dspg%u -> spg%u\n", dspgno, segno);
          }

// spt->pages
        for (uint pg_no = 0; pg_no < num_pages; pg_no ++)
          {
            bool mult = page_seg_map[pg_no].n_claims > 1;
            for (uint c = 0; c < page_seg_map[pg_no].n_claims; c ++)
              {
                printf ("spg%u:%03o -> block%d:%05o %s\n",
                        page_seg_map[pg_no].segno[c],
                        page_seg_map[pg_no].page[c],
                        block_start[pg_no],
                        pg_no,
                        mult ? "[color=red]" : "");
              }
          }


        printf ("}\n");
      }
  }

static void usage (void)
  {
    printf (
"multics_table_dump [options] [state_file]\n"
"\n"
"  Options\n"
"    -h          this message\n"
"    -q          quiet; suppress infomational messages\n"
"    -d          dump tables\n"
"    -da         dump AST\n"
"    -dc         dump CMP table\n"
"    -dp         dump page tables\n"
"    -ds         dump SLT table\n"
"    -g          generate page DOT graph\n"
"\n"
"  state_file: dps8m emulate state file prefix, default is 'state'\n"
"\n"
    );
  }

int main (int argc, char * argv[])
  {
    char * state_file = "dps8m.state";
    
    for (int i = 1; i < argc; i ++)
      {
        if (argv[i][0] == '-')
          {
            if (argv[i][1] == 'h')
              {
                usage ();
                exit (1);
              }
            else if (argv[i][1] == 'q')
              {
                quiet = true;
              }
            else if (argv[i][1] == 'd')
              {
                if (argv[i][2] == '\0')
                  {
                    dump_ast = true;
                    dump_cmp = true;
                    dump_slt = true;
                    dump_dsbr = true;
                  }
                else if (argv[i][2] == 'a')
                  {
                    dump_ast = true;
                  }
                else if (argv[i][2] == 'c')
                  {
                    dump_cmp = true;
                  }
                else if (argv[i][2] == 's')
                  {
                    dump_slt = true;
                  }
                else if (argv[i][2] == 'p')
                  {
                    dump_dsbr = true;
                  }
                else
                  goto illopt;
              }
            else if (argv[i][1] == 'g')
              {
                pg_graph = true;
                quiet = true;
              }
            else
              {
illopt:
                printf ("Unrecognized option '%s'\n", argv[i]);
                usage ();
                exit (1);
              }
          }
        else
          {
            state_file = argv[i];
          }
      }

// Open state file

    system_state = (struct system_state_s *) open_shm2 (state_file, sizeof (struct system_state_s));
    if (! system_state)
      {
        printf ("State file '%s' not found.\n", state_file);
        exit (1);
      }

    M = system_state->M;
    cpus = system_state->cpus;

// Assume CPU A

    cpun = 0;

// Report some stuff

    dsbr_addr = cpus[cpun].DSBR.ADDR;
    early = dsbr_addr == intlzr_dsbr_addr;

// sst

    pair sst_cmp = append72 (sst_segno, sst_seg_cmp, NULL);
    cmp_segno = get_segno (sst_cmp);
    cmp_os = get_os (sst_cmp);
    cmp_seg_len = seg_len (cmp_segno);
    word36 w = append (sst_segno, sst_seg_usedp, NULL);
    sst_usedp = GETHI (w);
    if (! quiet)
      printf ("sst|cmp %05o:%06o %08o\n", cmp_segno, cmp_os, absadr (cmp_segno, cmp_os));

    pair sst_astap = append72 (sst_segno, sst_seg_astap, NULL);
    asta_segno = get_segno (sst_astap);
    asta_os = get_os (sst_astap);

    analyze_dsbr (cpus[cpun].DSBR);

    // slt first because it builds the aste->segno map and slt_seg_names.
    slt ();
    if (pg_graph)
      return 0;  // just the graph
    cmp ();
    ast ();

    if (nerrors == 0)
      printf ("No errors found.\n");
    else
      printf ("%d error%s found\n", nerrors, nerrors == 1 ? "" : "s");
    return 0;
  }


