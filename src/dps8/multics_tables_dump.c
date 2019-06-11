#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>

#include "dps8.h"
#include "dps8_cpu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_state.h"
#include "dps8_utils.h"
#include "shm.h"
//#include "threadz.h"

struct system_state_s * system_state;
vol word36 * M = NULL;                                          // memory
cpu_state_t * cpus;
vol cpu_state_t * cpup_;
#undef cpu
#define cpu (* cpup_)

static word24 dsbr_addr;
static uint cpun;

static int nerrors = 0;

#define dseg_segno 00
#define slt_segno 07
#define slt_seg 8 
#define sst_segno 0102
#define sst_seg_astap 24
#define sst_level 36
#define sst_seg_cmp 44
#define sst_seg_usedp 46

#define aste_size 12
#define slte_size 4

static word18 sst_usedp;
static uint cmp_seg_len;
static word15 cmp_segno;
static word18 cmp_os;
static word15 asta_segno;
static word18 asta_os;


void sim_printf (const char* fmt, ...)
  {
  }

static word36 append (uint segno, uint offset)
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
    word24 sdw_addr = (SDWeven >> 12) & 077777777;
//printf ("sdw_addr %08o\n", sdw_addr);
    word24 x2 = (offset) / 1024; // floor

    word24 y2 = offset % 1024;
    word36 PTWx2 = M[sdw_addr + x2];
    word24 PTW1_addr= GETHI (PTWx2);
    word1 PTW0_U = TSTBIT (PTWx2, 9);
    if (! PTW0_U)
      return 6;
    word24 finalAddress = ((((word24)PTW1_addr & 0777760) << 6) + y2) & PAMASK;
    return M[finalAddress];
    //return (word24) (GETHI (PTWx1) << 6);
  }

static word24 absadr (uint segno, uint offset)
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
    word24 sdw_addr = (SDWeven >> 12) & 077777777;
//printf ("sdw_addr %08o\n", sdw_addr);
    word24 x2 = (offset) / 1024; // floor

    word24 y2 = offset % 1024;
    word36 PTWx2 = M[sdw_addr + x2];
    word24 PTW1_addr= GETHI (PTWx2);
    word1 PTW0_U = TSTBIT (PTWx2, 9);
    if (! PTW0_U)
      return 6;
    word24 finalAddress = ((((word24)PTW1_addr & 0777760) << 6) + y2) & PAMASK;
    return finalAddress;
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

static word15 get_segno (word36 w)
  {
    return (word15) (w >> 18) & 077777;
  }

static word18 get_os (word36 w)
  {
    return (word18) (w >> 18) & 0777777;
  }

static void coremap (void)
  {
    printf ("COREMAP\n");

// Coremap

    printf ("core_map bound %u %o\n", seg_len (cmp_segno), seg_len (cmp_segno));

#define pages 16384

#define cme_size 4

    printf ("page  absadr   p      fp     bp     strp   devadd   ptwp   astep  pinctr syncpp contr\n");
    uint page;
    for (page = 0; page < pages; page ++)
      {
        uint cmep = cmp_os + page * cme_size;
        if (cmep >= cmp_seg_len)
          break;

        word36 cme0 = append (cmp_segno, cmep + 0);
        word36 cme1 = append (cmp_segno, cmep + 1);
        word36 cme2 = append (cmp_segno, cmep + 2);
        word36 cme3 = append (cmp_segno, cmep + 3);

        //printf ("%5o %012llo %012llo %012llo %012llo\n", page, cme0, cme1, cme2, cme3);
        if (cme0 || cme1 || cme2 || cme3)
          {
            word18 fp = getbits36_18 (cme0, 0);
            word18 bp = getbits36_18 (cme0, 18);
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

            word18 ptwp = getbits36_18 (cme2, 0);
            word18 strp = getbits36_18 (cme2, 0);
            word18 astep = getbits36_18 (cme2, 18);

            word18 pin_counter = getbits36_18 (cme3, 0);
            word18 synch_page_entryp = getbits36_18 (cme3, 18);
            word24 abs_adr = absadr (cmp_segno, cmep);
            printf ("%5o %08o %06o %06o %06o %06o %08o %06o %06o %06o %06o %o %s%s%s%s%s%s%s%s\n",
                    page,
                    abs_adr,
                    cmep,
                    fp,
                    bp,
                    strp,
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
          }
      }
    // uint npages = page;
    printf ("\n");

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

    printf ("Walking the sst|usedp list:\n");

    uint cnt = 0;
    while (1)
      {
        printf ("    %06o %6o\n", ptr, (ptr - cmp_os) / cme_size);
        // uint n = (ptr - cme_size) / cme_size;
        // visited[n] = true;
        word36 cme0 = append (cmp_segno, ptr + 0);
        word18 fp = getbits36_18 (cme0, 0);
        // printf ("fp %o\n", fp);
        if (fp >= cmp_seg_len)
          {
            printf ("fp (%08o) of %08o went off the end of the segment\n", fp, ptr);
            nerrors ++;
            break;
          }
        word18 last = ptr;
        ptr = fp;
        cme0 = append (cmp_segno, ptr + 0);
        word18 bp = getbits36_18 (cme0, 18);
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
             printf ("loop checked; %d entries\n", cnt);
             break;
          }
      }
skip: ;

  }

static void print_ptw (word36 ptw, bool * iscore, word14 * ispage)
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
        printf ("    %012llo  core page %05o addr %08o U %o M %o F %o FC %o\n",
 ptw, page, addr, u, m, f, fc);
        * iscore = true;
        * ispage = page;
      }
    else if (add_type == 04)
      {
        word18 record = getbits36_18 (ptw, 0);
        record &= 0377777;
        printf ("    %012llo  disk record %06o zero %o U %o M %o F %o FC %o\n",
 ptw, record, b0, u, m, f, fc);
      }
    else if (add_type == 00 && b0 == 00)
      {
        word18 debug = getbits36_18 (ptw, 0);
        printf ("    %012llo  null debug %06o U %o M %o F %o FC %o\n",
 ptw, debug, u, m, f, fc);
      }
    else
      {
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
    word36 cme2 = append (cmp_segno, cmep + 2);
    word18 cme_astep = getbits36_18 (cme2, 18);
    if (cme_astep != astep)
      {
        printf ("cme.astep (%06o) != astep (%06o)\n", cme_astep, astep);
        nerrors ++;
      }
  }

// Given an astep, find the segno

static uint find_segno (word18 astep)
  {
//  2 * segno >= 16 * (DSBR.BND + 1)
//  2 * segno < 16 * (DSBR.BND + 1)
//  segno < 8 * (DSBR.BND + 1)
    uint nsegs = 8 * (cpus[cpun].DSBR.BND + 1);
    for (uint segno = 0; segno < nsegs; segno ++)
      {
        uint y1 = (2 * segno) % 1024;
        uint x1 = (2 * segno - y1) / 1024;
        word24 ptw_add = cpus[cpun].DSBR.ADDR + x1;       
        word36 ptw = M[ptw_add];
        word24 dspg_add = ((word24) (getbits36_18 (ptw, 0) & 0777760)) << 6;
        word36 sdw_even = M[dspg_add + y1];       
        word36 sdw_odd = M[dspg_add + y1 + 1];       
        word24 sgpt_add = getbits36_24 (sdw_even, 0);
        word24 aste_add = (sgpt_add - aste_size) & MASK24;
        printf ("segno %05o ptw_add %08o ptw %012llo dspg_add %08o sdw %012llo %012llo sgpt_add %08o aste_addr %08o\n", segno, ptw_add, ptw, dspg_add, sdw_even, sdw_odd, sgpt_add, aste_add);
      }
    return 0;
  }

static void slt (void)
  {
    printf ("SLT\n");
    word18 sltep0 = slt_seg; // offset to start of array
    printf ("n    os     names  path   REWP len rng   segno  max bc\n");
    for (uint n = 0; n <= 9192; n ++)
      {
        word18 sltep = sltep0 + n * slte_size;
//printf ("%08o\n", absadr (slt_segno, sltep + 1));
        word36 slte0 = append (slt_segno, sltep + 0);
        word36 slte1 = append (slt_segno, sltep + 1);
        word36 slte2 = append (slt_segno, sltep + 2);
        word36 slte3 = append (slt_segno, sltep + 3);
       
        word18 names_ptr = getbits36_18 (slte0, 0);
        word18 path_ptr = getbits36_18 (slte0, 18);

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
        word18 segno = getbits36_18 (slte2, 18);

        word9 max_length = getbits36_9 (slte3, 3);
        word24 bit_count = getbits36_24 (slte3, 12);

        printf ("%04o %06o %06o %06o %c%c%c%c %03o %o,%o,%o %06o %03o %08o\n",
                n, sltep, names_ptr, path_ptr, 
                access_r ? 'R' : ' ',
                access_e ? 'E' : ' ',
                access_w ? 'W' : ' ',
                access_p ? 'P' : ' ',
                cur_length, rb1, rb2, rb3, segno, max_length, bit_count);

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

static void ast (void)
  {
    printf ("AST\n");
    printf ("asta %05o:%06o\n", asta_segno, asta_os);

// walk the lists

    for (uint level = 0; level < 4; level ++)
      {
        word36 level0 = append (asta_segno, sst_level + level);
        word18 ausedp = getbits36_18 (level0, 0);
        word18 no_aste = getbits36_18 (level0, 18);
        printf ("Level %u: ausedp: %06o (%08o) no_aste: %06o\n", level, ausedp, absadr (asta_segno, ausedp), no_aste);
        word18 ptr = ausedp;
        for (uint n = 0; n < no_aste; n ++)
          {
            word36 aste0 = append (asta_segno, ptr);
            word18 fp = getbits36_18 (aste0, 0);
            word18 bp = getbits36_18 (aste0, 18);
            word36 aste11 = append (asta_segno, ptr + 11);
            word2 ptsi = getbits36_2 (aste11, 28);
            printf ("aste @ %06o (%08o) fp %06o bp %06o ptsi %o\n", ptr, absadr (asta_segno, ptr), fp, bp, ptsi);
            static uint ptsi_tab[4] = {4, 16, 64, 256 };
            uint nptws = ptsi_tab[ptsi];
            for (uint np = 0; np < nptws; np ++)
              {
                word36 ptw = append (asta_segno, ptr + aste_size + np);
                bool ispage;
                word14 pageno;
                //printf ("  %012llo\n", ptw0);
                print_ptw (ptw, & ispage, & pageno);
                if (ispage)
                  verify_ptw (ptw, ptr);
                
              }
            ptr = fp;
        }
    }

#if 0
    while (1)
      {
        word36 aste0 = append (asta_segno, ptr);
        word18 fp = getbits36_18 (aste0, 0);
        word18 bp = getbits36_18 (aste0, 18);
        printf ("p %06o fp %06o bp %06o\n", ptr, fp, bp);
        ptr = fp;
        if (ptr == asta_os)
          break;
      }
#endif
  }

int main (int argc, char * argv[])
  {

// Open state file

    for (;;)
      {
        system_state = (struct system_state_s *)
          open_shm ("state", sizeof (struct system_state_s));

        if (system_state)
          break;

        printf ("No state file found; retry in 1 second\n");
        sleep (1);
      }

    M = system_state->M;
    cpus = system_state->cpus;

// Assume CPU A

    cpun = 0;

// Report some stuff

    dsbr_addr = cpus[cpun].DSBR.ADDR;

    printf ("DSBR ADDR %8o BND %05o U %o STACK %04o\n",
      cpus[cpun].DSBR.ADDR, cpus[cpun].DSBR.BND, 
      cpus[cpun].DSBR.U, cpus[cpun].DSBR.STACK); 



// sst

    word36 sst_cmp_l = append (sst_segno, sst_seg_cmp);
    word36 sst_cmp_h = append (sst_segno, sst_seg_cmp + 1);
    cmp_segno = get_segno (sst_cmp_l);
    cmp_os = get_os (sst_cmp_h);
    cmp_seg_len = seg_len (cmp_segno);
    word36 w = append (sst_segno, sst_seg_usedp);
    sst_usedp = getbits36_18 (w, 0);
    printf ("sst|cmp %05o:%06o %08o\n", cmp_segno, cmp_os, absadr (cmp_segno, cmp_os));

    word36 sst_astap_l = append (sst_segno, sst_seg_astap);
    word36 sst_astap_h = append (sst_segno, sst_seg_astap + 1);
    asta_segno = get_segno (sst_astap_l);
    asta_os = get_os (sst_astap_h);

    coremap ();
    slt ();
    ast ();

    printf ("%d error(s)\n", nerrors);
    return 0;
  }


