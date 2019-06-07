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

#define sst_seg 0102

    word36 sst_cmp_l = append (sst_seg, 44);
    word36 sst_cmp_h = append (sst_seg, 45);
    word15 sst_cmp_segno = get_segno (sst_cmp_l);
    word18 sst_cmp_os = get_os (sst_cmp_h);

    uint cmp_seg_len = seg_len (sst_cmp_segno);
    //printf ("sst_cmp %o:%o\n", sst_cmp_segno, sst_cmp_os);
    printf ("core_map bound %u %o\n", seg_len (sst_cmp_segno), seg_len (sst_cmp_segno));

// Coremap

#define pages 16384

#define cme_size 4

    printf ("page  fp     bp     devadd   ptwp   astep  pinctr syncpp contr\n");
    for (uint i = 0; i < pages; i ++)
      {
        uint cmep = sst_cmp_os + i * cme_size;
        if (cmep >= cmp_seg_len)
          break;
printf ("%5o %08o\n", i, absadr (sst_cmp_segno, cmep));

        word36 cme0 = append (sst_cmp_segno, cmep + 0);
        word36 cme1 = append (sst_cmp_segno, cmep + 1);
        word36 cme2 = append (sst_cmp_segno, cmep + 2);
        word36 cme3 = append (sst_cmp_segno, cmep + 3);

        //printf ("%5o %012llo %012llo %012llo %012llo\n", i, cme0, cme1, cme2, cme3);
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
            word18 astep = getbits36_18 (cme2, 18);

            word18 pin_counter = getbits36_18 (cme3, 0);
            word18 synch_page_entryp = getbits36_18 (cme3, 18);
            printf ("%5o %06o %06o %08o %06o %06o %06o %06o %o\n",
                    i, fp, bp, devadd, ptwp, astep, pin_counter, synch_page_entryp, contr);
            printf ("     %s%s%s%s%s%s%s%s\n",
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
    return 0;
  }


