/*
 Copyright 2013-2019 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

#include <stdio.h>
#include <ctype.h>

#include "dps8.h"
#include "dps8_iom.h"
#include "dps8_disk.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_urp.h"
#include "dps8_fnp.h"
#include "dps8_socket_dev.h"
#include "sim_disk.h"
#include "dps8_utils.h"
#include "threadz.h"


#define SECTOR_SZ_IN_W36 512
#define SECTOR_SZ_IN_BYTES ((36 * SECTOR_SZ_IN_W36) / 8)
#define RECORD_SZ_IN_BYTES (2 * SECTOR_SZ_IN_BYTES)

typedef uint8_t record[RECORD_SZ_IN_BYTES];

static uint sect_per_cyl; 
static uint sect_per_rec; 
static uint number_of_sv;

static FILE * setup_disk (uint disk_unit_idx)
  {
    UNIT * unitp = & dsk_unit [disk_unit_idx];
    FILE * fileref = unitp->fileref;
    sect_per_cyl = diskTypes[disk_unit_idx].sects_per_cyl;
    if (diskTypes[disk_unit_idx].sectorSizeWords == 64)
      sect_per_rec = 16;
    else
      sect_per_rec = 2;
    if (disk_unit_idx == 0)
      number_of_sv = 3;
    else
      number_of_sv = 1;
    return fileref;
  }

static uint r2s (uint rec, uint sv)
  {
    uint usable = (sect_per_cyl / sect_per_rec) * sect_per_rec;
    uint unusable = sect_per_cyl - usable;
    uint sect = rec * sect_per_rec;
    sect = sect + (sect / usable) * unusable;

    uint sect_offset = sect % sect_per_cyl;
    sect = (sect - sect_offset) * number_of_sv + sv * sect_per_cyl +
            sect_offset;
    return sect;
  }

static void read_record (FILE * fileref, uint rec, record * data)
  {
    uint sect = r2s (rec, 0);

    off_t n = fseek (fileref, sect * SECTOR_SZ_IN_BYTES, SEEK_SET);
    if (n == (off_t) -1)
      {
        sim_warn ("seek off of end of data\n");
        return;
      }
    size_t r = fread (data, sizeof (record), 1, fileref);
    if (r != 1)
      {
        sim_warn ("fread() returned wrong size\n");
        return;
      }
  }

static char * str (word36 w, char buf[5])
  {
    //static char buf[5];
    buf[0] = (char) ((w >> 27) & 0377);
    buf[1] = (char) ((w >> 18) & 0377);
    buf[2] = (char) ((w >>  9) & 0377);
    buf[3] = (char) ((w >>  0) & 0377);
    buf[4] = 0;
    for (int i = 0; i < 4; i ++)
      if (! isprint (buf[i]))
        buf[i] = '?';
    return buf;
  }

static word36 unstr (char * buf)
  {
    return ((word36) buf[0]) << 27 |
           ((word36) buf[1]) << 18 |
           ((word36) buf[2]) <<  9 |
           ((word36) buf[3]) <<  0;
  }

#if 0
static int fixed_bin (word36 w)
  {
/*
    int ret = (int) (w & MASK17);
    if (w & SIGN18)
      ret = - ret;
*/
    int ret = (int) (w & MASK18);
    if (w & SIGN18)
      {
        ret = (int) (((~(uint) ret) + 1u) & MASK18);
      }
    return ret;
  }
#endif

// Return values;
//   -1 disk_unit_idx not attached
//   n 
static int find_config_deck_partion (FILE * fileref, word36 * frecp, word36 * nrecp)
  {
    if (! fileref)
      return -1;

    record r0;
    memset (& r0, 0, sizeof (record));
    read_record (fileref, 0, & r0);

    word36 w;

#define label_part_os (8*64)
#define label_perm_os (5*64)


    w = extr36 (r0, label_part_os + 3);
    uint nparts = (uint) w;
    if (nparts > 47)
      nparts = 47;
    uint partno;
    word36 frec, nrec;
    for (partno = 0; partno < nparts; partno ++)
      {
        uint pos = partno * 4 + 4;
        w = extr36 (r0, label_part_os + pos + 0);
        frec = extr36 (r0, label_part_os + pos + 1);
        nrec = extr36 (r0, label_part_os + pos + 2);
        if (w == 0143157156146) // "conf"
          break;
      }
    if (partno >= nparts)
      return -1;
    * frecp = frec;
    * nrecp = nrec;
    return (int) partno;
  }

// dcl (CONFIG_DECIMAL_TYPE      init ("11"b),
//      CONFIG_OCTAL_TYPE        init ("00"b),
//      CONFIG_SINGLE_CHAR_TYPE  init ("01"b),
//      CONFIG_STRING_TYPE       init ("10"b)) bit (2) aligned static options (constant);
#define CONFIG_DECIMAL_TYPE      3
#define CONFIG_OCTAL_TYPE        0
#define CONFIG_SINGLE_CHAR_TYPE  1
#define CONFIG_STRING_TYPE       2

// each card is 16 words; 1024 words/record; 64 cards/record
#define wrds_crd 16
#define crds_rec 64

static char * format_field (char * buf, word36 w, uint field_type)
  {
    if (field_type == CONFIG_OCTAL_TYPE)
      sprintf (buf, "%llo", w);
    else if (field_type == CONFIG_SINGLE_CHAR_TYPE)
      sprintf (buf, "%c", (char) (w + 'a' - 1));
    else if (field_type == CONFIG_STRING_TYPE)
      str (w, buf);
    else if (field_type == CONFIG_DECIMAL_TYPE)
      sprintf (buf, "%lld.", w);
    else
      sprintf (buf, "%llo field_type %u?", w, field_type);
    return buf;
  }

static void print_fields (uint8_t * r, uint card_os)
  {
    char word[5];
    word36 w = extr36 (r, card_os + 0);
    word36 type_word = extr36 (r, card_os + 15);
    uint n_fields = (uint) (type_word & 15);
    sim_printf ("%s", str (w, word));
    for (uint option = 0; option < n_fields; option ++)
      {
        uint field_type =  (uint) (type_word >> (34 -(option * 2))) & 3u;
        char ff[256];
        word36 w = extr36 (r, card_os + 1 + option);
        format_field (ff, w, field_type);
        sim_printf (" %s", ff);
      }
    sim_printf ("\n");
  }

#if 0
static void dump_deck (uint8_t * deck, uint n_cards)
  {
    for (uint cardno = 0; cardno < n_cards; cardno ++)
      {
        char word[5];
        uint card_os = cardno * wrds_crd;
        word36 w = extr36 ((uint8_t *) deck, card_os + 0);
        if (w == 0777777777777lu)
          continue;
        str (w, word);
        print_fields ((uint8_t *) deck, card_os);
      }
  }
#endif

t_stat print_config_deck (int32 flag, UNUSED const char * cptr)
  {
    if (! cptr)
      return SCPE_ARG;
    if (! strlen (cptr))
      return SCPE_ARG;

    char * end;
    long n = strtol (cptr, & end, 0);
    if (* end != 0)
      return SCPE_ARG;

    if (n < 0 || n > N_DSK_UNITS_MAX)
      return SCPE_ARG;

    uint disk_unit_idx = (uint) n;
    UNIT * unitp = & dsk_unit [disk_unit_idx];
    FILE * fileref = unitp->fileref;
    sect_per_cyl = diskTypes[disk_unit_idx].sects_per_cyl;
    if (diskTypes[disk_unit_idx].sectorSizeWords == 64)
      sect_per_rec = 16;
    else
      sect_per_rec = 2;
    if (disk_unit_idx == 0)
      number_of_sv = 3;
    else
      number_of_sv = 1;
    word36 conf_frec, conf_nrec;
    int partno = find_config_deck_partion (fileref, & conf_frec, & conf_nrec);
    if (partno < 0)
      {
        sim_warn ("Unable to find conf partition\n");
        return SCPE_ARG;
      }

    for (uint nrec = 0; nrec < conf_nrec; nrec ++)
      {
        record r;
        read_record (fileref, (uint) conf_frec + nrec, & r);
#if 0
        for (uint i = 0; i < 1024; i ++)
          {
            char buf[5];
            word36 w = extr36 (r, i);
            printf ("%012llo %s\n", w, str (w, buf));
          }
#endif

        for (uint cardno = 0; cardno < crds_rec; cardno ++)
          {
            char word[5];
            uint card_os = cardno * wrds_crd;
            word36 w = extr36 (r, card_os + 0);
            if (w == 0777777777777lu)
              continue;
            str (w, word);
            print_fields (r, card_os);
//sim_printf ("field_type %012llo\n", extr36 ((uint8_t *) r, card_os + 15));
          }
      }

    return SCPE_OK;
  }

typedef word36 card_t [wrds_crd];

static void write_card (card_t card, uint8_t * deck, uint card_no)
  {
     uint card_os = card_no * wrds_crd;
     for (uint i = 0; i < wrds_crd; i ++)
       put36 (card[i], deck, card_os + i);
  }

static void append_card (card_t card, uint8_t * deck, uint n_cards)
  {
    for (uint card_no = 0; card_no < n_cards; card_no ++)
      {
        uint card_os = card_no * wrds_crd;
        word36 w = extr36 ((uint8_t *) deck, card_os + 0);
        if (w == 0777777777777lu)
          {
             write_card (card, deck, card_no);
             return;
          }
      }
    sim_warn ("append_card couldn't\n");
  }

static void set_field_type (card_t card, uint no, uint val)
  {
    word36 mask = ~ (((word36) 3) << (34 - no * 2));
    word36 bits = ((word36) val) << (34 - no * 2);
    card[15] = (card[15] & mask) | bits;
  }

static void fill_cpu_card (uint8_t * new_deck, uint n_cards, uint cpu_unit_idx)
  {
    
    // XXX Assume that CPU port 0 is cabled
    uint scu_port_num = cables->cpu_to_scu[cpu_unit_idx][0].scu_port_num;
    uint scu_subport_num = cables->cpu_to_scu[cpu_unit_idx][0].scu_subport_num;
    uint scu_unit_idx = cables->cpu_to_scu[cpu_unit_idx][0].scu_unit_idx;
    bool is_exp = scu[scu_unit_idx].ports[scu_port_num].is_exp;

    card_t card;
    memset (card, 0, sizeof (card));

    if (is_exp)
      card[15] = 7;
    else
      card[15] = 6;

    card[0] = unstr ("cpu ");

    card[1] = cpus[cpu_unit_idx].switches.cpu_num + 1u;
    set_field_type (card, 0, CONFIG_SINGLE_CHAR_TYPE);

    card[2] = scu_port_num;
    set_field_type (card, 1, CONFIG_OCTAL_TYPE);

    // XXX CPU A on, rest off
    if (cpus[cpu_unit_idx].switches.cpu_num == 0)
      card[3] = unstr ("on  ");
    else
      card[3] = unstr ("off ");
    set_field_type (card, 2, CONFIG_STRING_TYPE);

#ifdef L68
    card[4] = unstr ("l68 ");
    set_field_type (card, 3, CONFIG_STRING_TYPE);

    card[5] = 60; // model
    set_field_type (card, 4, CONFIG_DECIMAL_TYPE);
#endif
#ifdef DPS8M
    card[4] = unstr ("dps8");
    set_field_type (card, 3, CONFIG_STRING_TYPE);

    card[5] = 70; // model
    set_field_type (card, 4, CONFIG_DECIMAL_TYPE);
#endif

    // XXX card[6] = 32; // cache size
    card[6] = 8; // cache size
    set_field_type (card, 5, CONFIG_DECIMAL_TYPE);

    if (is_exp)
      {
        card[7] = scu_subport_num + 1u;
        set_field_type (card, 6, CONFIG_SINGLE_CHAR_TYPE);
      }

    append_card (card, new_deck, n_cards);        
//sim_printf ("field_type %012llo\n", card[15]);
//sim_printf ("append cpu\n");
//dump_deck (new_deck, n_cards);
  }

static void fill_scu_card (uint8_t * new_deck, uint n_cards, uint scu_unit_idx)
  {
    // mem  a 4096. on  

    card_t card;
    memset (card, 0, sizeof (card));

    card[15] = 3;

    card[0] = unstr ("mem ");

    // XXX assume 0 is A, 1 is B, ...
    card[1] = scu_unit_idx + 1u;
    set_field_type (card, 0, CONFIG_SINGLE_CHAR_TYPE);

    card[2] = 4096;
    set_field_type (card, 1, CONFIG_DECIMAL_TYPE);

    card[3] = unstr ("on  ");
    set_field_type (card, 2, CONFIG_STRING_TYPE);

    append_card (card, new_deck, n_cards);        
  }

static void fill_iom_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint port)
  {
    card_t card;
    memset (card, 0, sizeof (card));
    card[15] = 4;
    card[0] = unstr ("iom ");
    card[1] = get_iom_num (iom_unit_idx) + 1u;
    set_field_type (card, 0, CONFIG_SINGLE_CHAR_TYPE);
    card[2] = port;
    set_field_type (card, 1, CONFIG_OCTAL_TYPE);
#ifdef L68
    card[3] = unstr ("iom ");
#endif
#ifdef DPS8M
    card[3] = unstr ("imu ");
#endif
    set_field_type (card, 2, CONFIG_STRING_TYPE);
    card[4] = unstr ("on  ");
    set_field_type (card, 3, CONFIG_STRING_TYPE);

    append_card (card, new_deck, n_cards);        
  }

static void fill_skc_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint chan, uint ctlr_unit_idx)
  {
    // prph skta a 40 0 1 64.

    card_t card;
    memset (card, 0, sizeof (card));

    card[15] = 6;

    card[0] = unstr ("prph");

    card[1] = unstr (skc_state[ctlr_unit_idx].device_name);
    set_field_type (card, 0, CONFIG_STRING_TYPE);

    card[2] = get_iom_num (iom_unit_idx) + 1u;
    set_field_type (card, 1, CONFIG_SINGLE_CHAR_TYPE);

    card[3] = chan;
    set_field_type (card, 2, CONFIG_OCTAL_TYPE);

    // XXX 0 ?
    card[4] = 0;
    set_field_type (card, 3, CONFIG_OCTAL_TYPE);

    // XXX 1 ?
    card[5] = 1;
    set_field_type (card, 4, CONFIG_OCTAL_TYPE);

    // XXX Assume 64 units
    card[6] = 64;
    set_field_type (card, 5, CONFIG_DECIMAL_TYPE);

    append_card (card, new_deck, n_cards);        
  }

static void fill_fnp_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint chan, uint ctlr_unit_idx)
  {
    // prph fnpb a 21 6670. off 

    card_t card;
    memset (card, 0, sizeof (card));

    card[15] = 5;

    card[0] = unstr ("prph");

    card[1] = unstr (fnpData.fnpUnitData[ctlr_unit_idx].device_name);
    set_field_type (card, 0, CONFIG_STRING_TYPE);

    card[2] = get_iom_num (iom_unit_idx) + 1u;
    set_field_type (card, 1, CONFIG_SINGLE_CHAR_TYPE);

    card[3] = chan;
    set_field_type (card, 2, CONFIG_OCTAL_TYPE);

    // XXX 6670 only supported model
    card[4] = 6670;
    set_field_type (card, 3, CONFIG_DECIMAL_TYPE);

    // XXX Assume all on
    card[5] = unstr ("on  ");
    set_field_type (card, 4, CONFIG_STRING_TYPE);

    append_card (card, new_deck, n_cards);        
  }

static void fill_urp_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint chan, uint ctlr_unit_idx)
  {
    // mpc  urpa 8004. a 15 1

    card_t card;
    memset (card, 0, sizeof (card));

    card[15] = 5;

    card[0] = unstr ("mpc ");

    card[1] = unstr (urp_state[ctlr_unit_idx].device_name);
    set_field_type (card, 0, CONFIG_STRING_TYPE);

    // XXX Model 8004 only supported model
    card[2] = 8004;
    set_field_type (card, 1, CONFIG_DECIMAL_TYPE);

    card[3] = get_iom_num (iom_unit_idx) + 1u;
    set_field_type (card, 2, CONFIG_SINGLE_CHAR_TYPE);

    card[4] = chan;
    set_field_type (card, 3, CONFIG_OCTAL_TYPE);

    // XXX assume single port
    card[5] = 1;
    set_field_type (card, 4, CONFIG_OCTAL_TYPE);

    append_card (card, new_deck, n_cards);        
  }

static void fill_msp_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint chan, uint ctlr_unit_idx)
  {
    // mpc  mspa 800. a 14 1

    card_t card;
    memset (card, 0, sizeof (card));

    card[15] = 5;

    card[0] = unstr ("mpc ");

    card[1] = unstr (msp_states[ctlr_unit_idx].device_name);
    set_field_type (card, 0, CONFIG_STRING_TYPE);

    // XXX Model 800 only supported model
    card[2] = 800;
    set_field_type (card, 1, CONFIG_DECIMAL_TYPE);

    card[3] = get_iom_num (iom_unit_idx) + 1u;
    set_field_type (card, 2, CONFIG_SINGLE_CHAR_TYPE);

    card[4] = chan;
    set_field_type (card, 3, CONFIG_OCTAL_TYPE);

    // XXX assume single port
    card[5] = 1;
    set_field_type (card, 4, CONFIG_OCTAL_TYPE);

    append_card (card, new_deck, n_cards);        
  }

static void fill_ipc_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint chan)
  {
    card_t card;
    memset (card, 0, sizeof (card));
    card[15] = 4;
    card[0] = unstr ("ipc ");
    card[1] = unstr ("fips");
    set_field_type (card, 0, CONFIG_STRING_TYPE);
    card[2] = get_iom_num (iom_unit_idx) + 1u;
    set_field_type (card, 1, CONFIG_SINGLE_CHAR_TYPE);
    card[3] = chan;
    set_field_type (card, 2, CONFIG_OCTAL_TYPE);
    card[4] = 1;
    set_field_type (card, 3, CONFIG_OCTAL_TYPE);

    append_card (card, new_deck, n_cards);        
  }

#if 0
static void fill_opc_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint chan)
  {
    // prph  opca  a  36  6001.  256.  on

    card_t card;
    memset (card, 0, sizeof (card));
    card[15] = 7;
    card[0] = unstr ("prph ");
    card[1] = unstrn ("opc ", console_state[);
    set_field_type (card, 0, CONFIG_STRING_TYPE);
    card[2] = get_iom_num (iom_unit_idx) + 1u;
    set_field_type (card, 1, CONFIG_SINGLE_CHAR_TYPE);
    card[3] = chan;
    set_field_type (card, 2, CONFIG_OCTAL_TYPE);
    card[4] = 1;
    set_field_type (card, 3, CONFIG_OCTAL_TYPE);

    append_card (card, new_deck, n_cards);        
  }
#endif

#if 0
static void fill_mpc_card (uint8_t * new_deck, uint n_cards, unit model, uint iom_unit_idx, uint chan, uint n_chans)
  {
    card_t card;
    memset (card, 0, sizeof (card));
    card[15] = 4;
    card[0] = unstr ("ipc  ");
    card[1] = unstr ("fips");
    set_field_type (card, 0, CONFIG_STRING_TYPE);
    card[2] = get_iom_num (iom_unit_idx) + 1u;
    set_field_type (card, 1, CONFIG_SINGLE_CHAR_TYPE);
    card[3] = chan;
    set_field_type (card, 2, CONFIG_OCTAL_TYPE);
    card[4] = n_chans;
    set_field_type (card, 3, CONFIG_OCTAL_TYPE);

    append_card (card, new_deck, n_cards);        
  }
#endif

#if 0
static void fill_prph_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint chan, uint n_chans)
  {
    card_t card;
    memset (card, 0, sizeof (card));
    card[15] = 4;
    card[0] = unstr ("ipc  ");
    card[1] = unstr ("fips");
    set_field_type (card, 0, CONFIG_STRING_TYPE);
    card[2] = get_iom_num (iom_unit_idx) + 1u;
    set_field_type (card, 1, CONFIG_SINGLE_CHAR_TYPE);
    card[3] = chan;
    set_field_type (card, 2, CONFIG_OCTAL_TYPE);
    card[4] = n_chans;
    set_field_type (card, 3, CONFIG_OCTAL_TYPE);

    append_card (card, new_deck, n_cards);        
  }
#endif

static void fill_new_deck (uint8_t * new_deck, uint n_cards)
  {
    card_t card;

    // Initialize new_deck to empty
    card [0] = 0777777777777lu;
    for (uint i = 0; i < n_cards; i ++)
      write_card (card, new_deck, i);

    // List the CPUs
    bool cpus_used [N_CPU_UNITS_MAX];
    memset (cpus_used, 0, sizeof (cpus_used));

    for (uint cpu_unit_idx = 0; cpu_unit_idx < N_CPU_UNITS_MAX; cpu_unit_idx ++)
      for (uint prt = 0; prt < N_CPU_PORTS; prt ++)
        {
          struct cpu_to_scu_s * p = & cables->cpu_to_scu[cpu_unit_idx][prt];
          if (p->in_use)
            cpus_used[cpu_unit_idx] = true;
        }

    for (uint cpu_unit_idx = 0; cpu_unit_idx < N_CPU_UNITS_MAX; cpu_unit_idx ++)
      {
        if (! cpus_used[cpu_unit_idx])
          continue;
        fill_cpu_card (new_deck, n_cards, cpu_unit_idx);
      }

    // List the SCUs

    bool scus_used [N_SCU_UNITS_MAX];
    for (uint scu_unit_idx = 0; scu_unit_idx < N_SCU_UNITS_MAX; scu_unit_idx ++)
      for (uint prt = 0; prt < N_SCU_PORTS; prt ++)
        {
          struct scu_to_iom_s * p = & cables->scu_to_iom[scu_unit_idx][prt];
          if (p->in_use)
            scus_used[scu_unit_idx] = true;
        }

    for (uint scu_unit_idx = 0; scu_unit_idx < N_SCU_UNITS_MAX; scu_unit_idx ++)
      {
        if (! scus_used[scu_unit_idx])
          continue;
        fill_scu_card (new_deck, n_cards, scu_unit_idx);
      }

    // List the IOMs
    // Assuming SCU 0 is the bootload SCU, walk its list of ports XXX
    // to find connected IOMs.

    bool ioms_used [N_IOM_UNITS_MAX]; 
    memset (ioms_used, 0, sizeof (ioms_used));

    for (uint port = 0; port < N_SCU_PORTS; port ++)
      {
        struct scu_to_iom_s * p = &cables->scu_to_iom[0][port];
        if (p->in_use)
          {
            ioms_used[p->iom_unit_idx] = true;
            fill_iom_card (new_deck, n_cards, p->iom_unit_idx, port);
           }
       }

    // Controllers

    // For each IOM/IMU
    for (uint iom_unit_idx = 0; iom_unit_idx < N_IOM_UNITS_MAX; iom_unit_idx ++)
      {
        if (! ioms_used[iom_unit_idx])
          continue;

        // For each channel in the IOM
        for (uint chan = 0; chan < MAX_CHANNELS; chan ++)
          {
            // The cable connected to the channel
            struct iom_to_ctlr_s * p = & cables->iom_to_ctlr[iom_unit_idx][chan];
            if (! p->in_use)
              continue;

            enum ctlr_type_e ctlr_type = p->ctlr_type;
            UNIT * board = p->board;
            switch (ctlr_type)
              {
                case CTLR_T_NONE:
                  break;

                case CTLR_T_MTP:
sim_printf ("XXX need MTP card\n");
                  //fill_prph_card (new_deck, n_cards, iom_unit_idx, chan, n_chan);
                  break;

                case CTLR_T_MSP:
sim_printf ("msp on %o %u\n", chan, p->port_num);
                  // Only the primary port is listed in the config deck; add'l
                  // ports are list in prph/chnl cards
                  //   mpc -ctlr mspa -model 800. -iom a -chn 14 -nchan 1 
                  //   prph dskb a 14 1 501. 4 451.  4
                  //   chnl -subsys dskb -iom b -chn 14 -nchan 1 
                  // XXX assuming port 0 is always connected
                  if (p->port_num == 0)
                    {
                      fill_msp_card (new_deck, n_cards, iom_unit_idx, chan, p->ctlr_unit_idx);
                      // Can handle at most 5 groups  of disks on a card
#                     define GRPS 5
                      uint n_grps = 0;
                      uint group_cnt [GRPS];
                      uint group_type [GRPS];

                      for (uint dev_code = 1; dev_code < N_DEV_CODES; )
                        {
                           // XXX Assues compact dev_code assignment
                           if (! cables->msp_to_dsk[p->ctlr_unit_idx][dev_code].in_use)
                             break;
                           if (n_grps == GRPS)
                             {
                               sim_warn ("too many disk groups\n");
                               break;
                             }
                           group_cnt[n_grps] = 0;
                           uint disk_unit_idx = cables->msp_to_dsk[p->ctlr_unit_idx][dev_code].unit_idx;
                           //uint disk_type = diskTypes[disk_unit_idx].typeIdx;
                           uint disk_type = dsk_states[disk_unit_idx].typeIdx;
                           group_cnt[n_grps] ++;
                           group_type[n_grps] = disk_type;
                           dev_code ++;
                           for ( ; dev_code < N_DEV_CODES; dev_code ++)
                             {
                               // XXX Assues compact dev_code assignment
                               if (! cables->msp_to_dsk[p->ctlr_unit_idx][dev_code].in_use)
                                 break;
                               uint disk_unit_idx = cables->msp_to_dsk[p->ctlr_unit_idx][dev_code].unit_idx;
                               uint disk_type = dsk_states[disk_unit_idx].typeIdx;
                               if (disk_type != group_type[n_grps])
                                 break;
                               group_cnt[n_grps] ++;
                             }
                           n_grps ++;
                         }
for (uint g = 0; g < n_grps; g ++)
  sim_printf ("%o %d %d\n", chan, group_cnt[g], group_type[g]);
// Need to build types, count                     
//    ipc  fips a 13 1
//    prph dska a 13 1 3381. 4.        4 x 3381
//    mpc  mspa 800. a 14 1
//    prph dskb a 14 1 501. 4 451. 3   4 * 501, 3 * 451
// Need to generate CHNL cards

                    }
                  break;

                case CTLR_T_IPCD:
                  {
                    // Disk controllers don't get cards

                    // Can handle at most 5 groups  of disks on a card
                    uint n_grps = 0;
                    uint group_cnt [GRPS];
                    uint group_type [GRPS];

                    for (uint dev_code = 0; dev_code < N_DEV_CODES; )
                      {
                         // XXX Assues compact dev_code assignment
                         if (! cables->ipcd_to_dsk[p->ctlr_unit_idx][dev_code].in_use)
                           break;
                         if (n_grps == GRPS)
                           {
                             sim_warn ("too many disk groups\n");
                             break;
                           }
                         group_cnt[n_grps] = 0;
                         uint disk_unit_idx = cables->ipcd_to_dsk[p->ctlr_unit_idx][dev_code].unit_idx;
                         //uint disk_type = diskTypes[disk_unit_idx].typeIdx;
                         uint disk_type = dsk_states[disk_unit_idx].typeIdx;
                         group_cnt[n_grps] ++;
                         group_type[n_grps] = disk_type;
                         dev_code ++;
                         for ( ; dev_code < N_DEV_CODES; dev_code ++)
                           {
                             // XXX Assues compact dev_code assignment
                             if (! cables->ipcd_to_dsk[p->ctlr_unit_idx][dev_code].in_use)
                               break;
                             uint disk_unit_idx = cables->ipcd_to_dsk[p->ctlr_unit_idx][dev_code].unit_idx;
                             uint disk_type = dsk_states[disk_unit_idx].typeIdx;
                             if (disk_type != group_type[n_grps])
                               break;
                             group_cnt[n_grps] ++;
                           }
                         n_grps ++;
                       }
for (uint g = 0; g < n_grps; g ++)
  sim_printf ("%o %d %d\n", chan, group_cnt[g], group_type[g]);
// Need to build types, count                     
//    ipc  fips a 13 1
//    prph dska a 13 1 3381. 4.        4 x 3381
//    mpc  mspa 800. a 14 1
//    prph dskb a 14 1 501. 4 451. 3   4 * 501, 3 * 451
// Need to generate CHNL cards

                        //dsk_states[disk_unit_idx].devname
                    }
                  break;

                case CTLR_T_IPCT:
                  fill_ipc_card (new_deck, n_cards, iom_unit_idx, chan);
                  break;

                case CTLR_T_OPC:
                  // OPC doesn't have a controller card
                  break;

                case CTLR_T_URP:
                  // Only the primary port is listed in the config deck; add'l
                  // ports are list in prph/chnl cards
                  //   mpc -ctlr mspa -model 800. -iom a -chn 14 -nchan 1 
                  //   prph dskb a 14 1 501. 4 451.  4
                  //   chnl -subsys dskb -iom b -chn 14 -nchan 1 
                  // XXX assuming port 0 is always connected
                  if (p->port_num == 0)
                    fill_urp_card (new_deck, n_cards, iom_unit_idx, chan, p->ctlr_unit_idx);
                  break;

                case CTLR_T_FNP:
                  fill_fnp_card (new_deck, n_cards, iom_unit_idx, chan, p->ctlr_unit_idx);
                  break;

                case CTLR_T_ABSI:
                  // ABSI doesn't have a controller card
                  break;

                case CTLR_T_SKC:
                  fill_skc_card (new_deck, n_cards, iom_unit_idx, chan, p->ctlr_unit_idx);
                  break;
              } // switch controller type
          } // for each channel
      } // for each IOM

  }

//
// SYNC_CONFIG_DECK <disk_index>
//

t_stat sync_config_deck (int32 flag, UNUSED const char * cptr)
  {

    // Must have argument
    if (! cptr)
      return SCPE_ARG;
    if (! strlen (cptr))
      return SCPE_ARG;

    // Parse argument
    char * end;
    long n = strtol (cptr, & end, 0);
    if (* end != 0)
      return SCPE_ARG;

    // Range check
    if (n < 0 || n > N_DSK_UNITS_MAX)
      return SCPE_ARG;

    // Recover infomation about disk from configuration settings
    uint disk_unit_idx = (uint) n;
    FILE * fileref = setup_disk (disk_unit_idx);
    if (! fileref)
      {
         sim_warn ("Unable to access disk image\n");
         return SCPE_ARG;
      }

    // Find the config deck partition
    word36 conf_frec, conf_nrec;
    int partno = find_config_deck_partion (fileref, & conf_frec, & conf_nrec);
    if (partno < 0)
      {
        sim_warn ("Unable to find conf partition\n");
        return SCPE_ARG;
      }

    // Number of cards in config deck record count time the number of cards in a record
    uint n_cards = (uint) conf_nrec * crds_rec;

    // New deck
    uint8_t new_deck [conf_nrec * sizeof (record)];

    // Copy cable and configuration data
    fill_new_deck (new_deck, n_cards);

    // Read the old deck
    record deck [conf_nrec];
    for (uint nrec = 0; nrec < conf_nrec; nrec ++)
      {
        read_record (fileref, (uint) conf_frec + nrec, deck + nrec);
      }

    word36 w_cpu = unstr ("cpu ");
    word36 w_iom = unstr ("iom ");
    word36 w_ipc = unstr ("ipc ");
    word36 w_scu = unstr ("mem ");
    word36 w_mpc = unstr ("mpc ");
    word36 w_prph = unstr ("prph");
    for (uint cardno = 0; cardno < n_cards; cardno ++)
      {
        uint card_os = cardno * wrds_crd;
        word36 w = extr36 ((uint8_t *) deck, card_os + 0);
        if (w == 0777777777777lu)
          continue;
        if (w == w_cpu)
          continue;
        if (w == w_iom)
          continue;
        if (w == w_ipc)
          continue;
        if (w == w_scu)
          continue;
        if (w == w_mpc)
          continue;
        if (w == w_prph)
          continue;
        sim_printf ("keeping :");
        print_fields ((uint8_t *) deck, card_os);
        sim_printf ("\n");
      }

sim_printf ("new deck\n");
    // Dump the new deck
    for (uint cardno = 0; cardno < n_cards; cardno ++)
      {
        char word[5];
        uint card_os = cardno * wrds_crd;
        word36 w = extr36 ((uint8_t *) new_deck, card_os + 0);
        if (w == 0777777777777lu)
          continue;
        str (w, word);
        print_fields ((uint8_t *) new_deck, card_os);
      }
sim_printf ("new deck\n");

    return SCPE_OK;
  }

