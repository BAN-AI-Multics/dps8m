/*
 * sim_tape.c: simulator tape support library
 *
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: X11
 * scspell-id: d4f34561-f62a-11ec-85d5-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 1993-2008 Robert M. Supnik
 * Copyright (c) 2021-2022 The DPS8M Development Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Robert M. Supnik shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Robert M. Supnik.
 *
 * ---------------------------------------------------------------------------
 */

/*
 * Public routines:
 *
 * sim_tape_attach      attach tape unit
 * sim_tape_detach      detach tape unit
 * sim_tape_attach_help help routine for attaching tapes
 * sim_tape_rdrecf      read tape record forward
 * sim_tape_rdrecr      read tape record reverse
 * sim_tape_wrrecf      write tape record forward
 * sim_tape_sprecf      space tape record forward
 * sim_tape_sprecr      space tape record reverse
 * sim_tape_wrtmk       write tape mark
 * sim_tape_wreom       erase remainder of tape
 * sim_tape_wreomrw     erase remainder of tape & rewind
 * sim_tape_wrgap       write erase gap
 * sim_tape_sprecsf     space records forward
 * sim_tape_spfilef     space files forward
 * sim_tape_sprecsr     space records reverse
 * sim_tape_spfiler     space files reverse
 * sim_tape_position    generalized position
 * sim_tape_rewind      rewind
 * sim_tape_reset       reset unit
 * sim_tape_bot         TRUE if at beginning of tape
 * sim_tape_eot         TRUE if at or beyond end of tape
 * sim_tape_wrp         TRUE if write protected
 * sim_tape_set_fmt     set tape format
 * sim_tape_show_fmt    show tape format
 * sim_tape_set_capac   set tape capacity
 * sim_tape_show_capac  show tape capacity
 * sim_tape_set_dens    set tape density
 * sim_tape_show_dens   show tape density
 */

#include "sim_defs.h"
#include "sim_tape.h"
#include <ctype.h>
#include <signal.h>

#ifdef TESTING
# undef FREE
# define FREE(p) free(p)
#endif /* ifdef TESTING */

struct sim_tape_fmt {
    const char          *name;                          /* name */
    int32               uflags;                         /* unit flags */
    t_addr              bot;                            /* bot test */
    };

static struct sim_tape_fmt fmts[MTUF_N_FMT] = {
    { "SIMH", 0,       sizeof (t_mtrlnt) - 1 },
    { "E11",  0,       sizeof (t_mtrlnt) - 1 },
    { "TPC",  UNIT_RO, sizeof (t_tpclnt) - 1 },
    { "P7B",  0,       0 },
/*  { "TPF",  UNIT_RO, 0 }, */
    { NULL,   0,       0 }
    };

static const uint32 bpi [] = {                          /* tape density table, indexed by MT_DENS constants */
    0,                                                  /*   0 = MT_DENS_NONE -- density not set */
    200,                                                /*   1 = MT_DENS_200  -- 200 bpi NRZI */
    556,                                                /*   2 = MT_DENS_556  -- 556 bpi NRZI */
    800,                                                /*   3 = MT_DENS_800  -- 800 bpi NRZI */
    1600,                                               /*   4 = MT_DENS_1600 -- 1600 bpi PE */
    6250                                                /*   5 = MT_DENS_6250 -- 6250 bpi GCR */
    };

#define BPI_COUNT       (sizeof (bpi) / sizeof (bpi [0]))   /* count of density table entries */

static t_stat sim_tape_ioerr (UNIT *uptr);
static t_stat sim_tape_wrdata (UNIT *uptr, uint32 dat);
static uint32 sim_tape_tpc_map (UNIT *uptr, t_addr *map, uint32 mapsize);
static t_stat sim_tape_simh_check (UNIT *uptr);
static t_stat sim_tape_e11_check (UNIT *uptr);
static t_addr sim_tape_tpc_fnd (UNIT *uptr, t_addr *map);
static void sim_tape_data_trace (UNIT *uptr, const uint8 *data, size_t len, const char* txt, int detail, uint32 reason);

struct tape_context {
    DEVICE              *dptr;              /* Device for unit (access to debug flags) */
    uint32              dbit;               /* debugging bit for trace */
    uint32              auto_format;        /* Format determined dynamically */
    };
#define tape_ctx up8                        /* Field in Unit structure which points to the tape_context */

/*
   This routine is called when the simulator stops and any time
   the asynch mode is changed (enabled or disabled)
*/
static void _sim_tape_io_flush (UNIT *uptr)
{
fflush (uptr->fileref);
}

/* Attach tape unit */

t_stat sim_tape_attach (UNIT *uptr, CONST char *cptr)
{
DEVICE *dptr;

if ((dptr = find_dev_from_unit (uptr)) == NULL)
    return SCPE_NOATT;
return sim_tape_attach_ex (uptr, cptr, (dptr->flags & DEV_DEBUG) ? 0xFFFFFFFF : 0, 0);
}

t_stat sim_tape_attach_ex (UNIT *uptr, const char *cptr, uint32 dbit, int completion_delay)
{
struct tape_context *ctx;
uint32 objc;
DEVICE *dptr;
char gbuf[CBUFSIZE];
t_stat r;
t_bool auto_format = FALSE;

if ((dptr = find_dev_from_unit (uptr)) == NULL)
    return SCPE_NOATT;
if (sim_switches & SWMASK ('F')) {                      /* format spec? */
    cptr = get_glyph (cptr, gbuf, 0);                   /* get spec */
    if (*cptr == 0)                                     /* must be more */
        return SCPE_2FARG;
    if (sim_tape_set_fmt (uptr, 0, gbuf, NULL) != SCPE_OK)
        return sim_messagef (SCPE_ARG, "Invalid Tape Format: %s\n", gbuf);
    sim_switches = sim_switches & ~(SWMASK ('F'));      /* Record Format specifier already processed */
    auto_format = TRUE;
    }
if (MT_GET_FMT (uptr) == MTUF_F_TPC)
    sim_switches |= SWMASK ('R');                       /* Force ReadOnly attach for TPC tapes */
r = attach_unit (uptr, (CONST char *)cptr);             /* attach unit */
if (r != SCPE_OK)                                       /* error? */
    return sim_messagef (r, "Can't open tape image: %s\n", cptr);
switch (MT_GET_FMT (uptr)) {                            /* case on format */

    case MTUF_F_STD:                                    /* SIMH */
        if (SCPE_OK != sim_tape_simh_check (uptr)) {
            sim_tape_detach (uptr);
            return SCPE_FMT;                            /* yes, complain */
            }
        break;

    case MTUF_F_E11:                                    /* E11 */
        if (SCPE_OK != sim_tape_e11_check (uptr)) {
            sim_tape_detach (uptr);
            return SCPE_FMT;                            /* yes, complain */
            }
        break;

    case MTUF_F_TPC:                                    /* TPC */
        objc = sim_tape_tpc_map (uptr, NULL, 0);        /* get # objects */
        if (objc == 0) {                                /* tape empty? */
            sim_tape_detach (uptr);
            return SCPE_FMT;                            /* yes, complain */
            }
        uptr->filebuf = calloc (objc + 1, sizeof (t_addr));
        if (uptr->filebuf == NULL) {                    /* map allocated? */
            sim_tape_detach (uptr);
            return SCPE_MEM;                            /* no, complain */
            }
        uptr->hwmark = objc + 1;                        /* save map size */
        sim_tape_tpc_map (uptr, (t_addr *) uptr->filebuf, objc);/* fill map */
        break;

    default:
        break;
        }

uptr->tape_ctx = ctx = (struct tape_context *)calloc(1, sizeof(struct tape_context));
if (!ctx)
{
  fprintf(stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
          __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
  (void)raise(SIGUSR2);
  /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
  abort();
}
ctx->dptr = dptr;                                       /* save DEVICE pointer */
ctx->dbit = dbit;                                       /* save debug bit */
ctx->auto_format = auto_format;                         /* save that we auto selected format */

sim_tape_rewind (uptr);

uptr->io_flush = _sim_tape_io_flush;

return SCPE_OK;
}

/* Detach tape unit */

t_stat sim_tape_detach (UNIT *uptr)
{
struct tape_context *ctx;
uint32 f = 0;
t_stat r;
t_bool auto_format = FALSE;

if (uptr == NULL)
    return SCPE_IERR;

if (!(uptr->flags & UNIT_ATT))
    return SCPE_UNATT;

ctx = (struct tape_context *)uptr->tape_ctx;
f = MT_GET_FMT (uptr);

if (uptr->io_flush)
    uptr->io_flush (uptr);                              /* flush buffered data */
if (ctx)
    auto_format = ctx->auto_format;

r = detach_unit (uptr);                                 /* detach unit */
if (r != SCPE_OK)
    return r;
switch (f) {                                            /* case on format */

    case MTUF_F_TPC:                                    /* TPC */
        if (uptr->filebuf)                              /* free map */
            FREE (uptr->filebuf);
        uptr->filebuf = NULL;
        uptr->hwmark = 0;
        break;

    default:
        break;
        }

sim_tape_rewind (uptr);
FREE (uptr->tape_ctx);
uptr->tape_ctx = NULL;
uptr->io_flush = NULL;
if (auto_format)    /* format was determined or specified at attach time? */
    sim_tape_set_fmt (uptr, 0, "SIMH", NULL);   /* restore default format */
return SCPE_OK;
}

t_stat sim_tape_attach_help(FILE *st, DEVICE *dptr, const UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "%s Tape Attach Help\n\n", dptr->name);
if (0 == (uptr-dptr->units)) {
    if (dptr->numunits > 1) {
        uint32 i;

        for (i=0; i < dptr->numunits; ++i)
            if (dptr->units[i].flags & UNIT_ATTABLE)
                fprintf (st, "  sim> ATTACH {switches} %s%lu tapefile\n\n", dptr->name, (unsigned long)i);
        }
    else
        fprintf (st, "  sim> ATTACH {switches} %s tapefile\n\n", dptr->name);
    }
else
    fprintf (st, "  sim> ATTACH {switches} %s tapefile\n\n", dptr->name);
fprintf (st, "Attach command switches\n");
fprintf (st, "    -R          Attach Read Only.\n");
fprintf (st, "    -E          Must Exist (if not specified an attempt to create the indicated\n");
fprintf (st, "                virtual tape will be attempted).\n");
fprintf (st, "    -F          Open the indicated tape container in a specific format (default\n");
fprintf (st, "                is SIMH, alternatives are E11, TPC and P7B)\n");
return SCPE_OK;
}

static void sim_tape_data_trace(UNIT *uptr, const uint8 *data, size_t len, const char* txt, int detail, uint32 reason)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;

if (ctx == NULL)
    return;
if (sim_deb && (ctx->dptr->dctrl & reason))
    sim_data_trace(ctx->dptr, uptr, (detail ? data : NULL), "", len, txt, reason);
}

/* Read record length forward (internal routine)

   Inputs:
        uptr    =       pointer to tape unit
        bc      =       pointer to returned record length
   Outputs:
        status  =       operation status

   exit condition       tape position
   ------------------   -----------------------------------------------------
   unit unattached      unchanged
   read error           unchanged, PNU set
   end of file/medium   updated if a gap precedes, else unchanged and PNU set
   tape mark            updated
   tape runaway         updated
   data record          updated, sim_fread will read record forward

   This routine is called to set up a record read or spacing in the forward
   direction.  On return, status is MTSE_OK and the tape is positioned at the
   first data byte if a record was encountered, or status is an MTSE error code
   giving the reason that the operation did not succeed and the tape position is
   as indicated above.

   The ANSI standards for magnetic tape recording (X3.32, X3.39, and X3.54) and
   the equivalent ECMA standard (ECMA-62) specify a maximum erase gap length of
   25 feet (7.6 meters).  While gaps of any length may be written, gaps longer
   than this are non-standard and may indicate that an unrecorded or erased tape
   is being read.

   If the tape density has been set via a previous "sim_tape_set_dens" call,
   then the length is monitored when skipping over erase gaps.  If the length
   reaches 25 feet, motion is terminated, and MTSE_RUNAWAY status is returned.
   Runaway status is also returned if an end-of-medium marker or the physical
   end of file is encountered while spacing over a gap; however, MTSE_EOM is
   returned if the tape is positioned at the EOM on entry.

   If the density has not been set, then a gap of any length is skipped, and
   MTSE_RUNAWAY status is never returned.  In effect, erase gaps present in the
   tape image file will be transparent to the caller.

   Erase gaps are currently supported only in SIMH (MTUF_F_STD) tape format.
   Because gaps may be partially overwritten with data records, gap metadata
   must be examined marker-by-marker.  To reduce the number of file read calls,
   a buffer of metadata elements is used.  The buffer size is initially
   established at 256 elements but may be set to any size desired.  To avoid a
   large read for the typical case where an erase gap is not present, the first
   read is of a single metadatum marker.  If that is a gap marker, then
   additional buffered reads are performed.

   See the notes at "sim_tape_wrgap" regarding the erase gap implementation.

   Implementation notes:

    1. For programming convenience, erase gap processing is performed for both
       SIMH standard and E11 tape formats, although the latter will never
       contain erase gaps, as the "sim_tape_wrgap" call takes no action for the
       E11 format.

    2. The "feof" call cannot return a non-zero value on the first pass through
       the loop, because the "sim_fseek" call resets the internal end-of-file
       indicator.  Subsequent passes only occur if an erase gap is present, so
       a non-zero return indicates an EOF was seen while reading through a gap.

    3. The dynamic start/stop test of the HP 3000 magnetic tape diagnostic
       heavily exercises the erase gap scanning code.  Sample test execution
       times for various buffer sizes on a 2 GHz host platform are:

         buffer size    execution time
         (elements)     (CPU seconds)
         -----------    --------------
               1             7200
              32              783
             128              237
             256              203
             512              186
            1024              171

    4. Because an erase gap may precede the logical end-of-medium, represented
       either by the physical end-of-file or by an EOM marker, the "position not
       updated" flag is set only if the tape is positioned at the EOM when the
       routine is entered.  If at least one gap marker precedes the EOM, then
       the PNU flag is not set.  This ensures that a backspace-and-retry
       sequence will work correctly in both cases.
*/

static t_stat sim_tape_rdlntf (UNIT *uptr, t_mtrlnt *bc)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
uint8 c;
t_bool all_eof;
uint32 f = MT_GET_FMT (uptr);
t_mtrlnt sbc;
t_tpclnt tpcbc;
t_mtrlnt buffer [256];                                  /* local tape buffer */
uint32 bufcntr, bufcap;                                 /* buffer counter and capacity */
int32 runaway_counter, sizeof_gap;                      /* bytes remaining before runaway and bytes per gap */
t_stat r = MTSE_OK;

MT_CLR_PNU (uptr);                                      /* clear the position-not-updated flag */

if ((uptr->flags & UNIT_ATT) == 0)                      /* if the unit is not attached */
    return MTSE_UNATT;                                  /*   then quit with an error */
if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */

sim_fseek (uptr->fileref, uptr->pos, SEEK_SET);         /* set the initial tape position */

switch (f) {                                            /* the read method depends on the tape format */

    case MTUF_F_STD:
    case MTUF_F_E11:
        runaway_counter = 25 * 12 * bpi [MT_DENS (uptr->dynflags)]; /* set the largest legal gap size in bytes */

        if (runaway_counter == 0) {                     /* if tape density has not been not set */
            sizeof_gap = 0;                             /*   then disable runaway detection */
            runaway_counter = INT_MAX;                  /*     to allow gaps of any size */
            }
        else                                            /* otherwise */
            sizeof_gap = sizeof (t_mtrlnt);             /*   set the size of the gap */

        bufcntr = 0;                                    /* force an initial read */
        bufcap = 0;                                     /*   but of just one metadata marker */

        do {                                            /* loop until a record, gap, or error is seen */
            if (bufcntr == bufcap) {                    /* if the buffer is empty then refill it */
                if (feof (uptr->fileref)) {             /* if we hit the EOF while reading a gap */
                    if (sizeof_gap > 0)                 /*   then if detection is enabled */
                        r = MTSE_RUNAWAY;               /*     then report a tape runaway */
                    else                                /*   otherwise report the physical EOF */
                        r = MTSE_EOM;                   /*     as the end-of-medium */
                    break;
                    }

                else if (bufcap == 0)                   /* otherwise if this is the initial read */
                    bufcap = 1;                         /*   then start with just one marker */

                else                                    /* otherwise reset the capacity */
                    bufcap = sizeof (buffer)            /*   to the full size of the buffer */
                               / sizeof (buffer [0]);

                bufcap = sim_fread (buffer,             /* fill the buffer */
                                    sizeof (t_mtrlnt),  /*   with tape metadata */
                                    bufcap,
                                    uptr->fileref);

                if (ferror (uptr->fileref)) {           /* if a file I/O error occurred */
                    if (bufcntr == 0)                   /*   then if this is the initial read */
                        MT_SET_PNU (uptr);              /*     then set position not updated */

                    r = sim_tape_ioerr (uptr);          /* report the error and quit */
                    break;
                    }

                else if (bufcap == 0                    /* otherwise if positioned at the physical EOF */
                  || buffer [0] == MTR_EOM)             /*   or at the logical EOM */
                    if (bufcntr == 0) {                 /*     then if this is the initial read */
                        MT_SET_PNU (uptr);              /*       then set position not updated */
                        r = MTSE_EOM;                   /*         and report the end-of-medium and quit */
                        break;
                        }

                    else {                              /*     otherwise some gap has already been skipped */
                        if (sizeof_gap > 0)             /*       so if detection is enabled */
                            r = MTSE_RUNAWAY;           /*         then report a tape runaway */
                        else                            /*       otherwise report the physical EOF */
                            r = MTSE_EOM;               /*         as the end-of-medium */
                        break;
                        }

                else                                    /* otherwise reset the index */
                    bufcntr = 0;                        /*   to the start of the buffer */
                }

            *bc = buffer [bufcntr++];                   /* store the metadata marker value */

            if (*bc == MTR_EOM) {                       /* if an end-of-medium marker is seen */
                if (sizeof_gap > 0)                     /*   then if detection is enabled */
                    r = MTSE_RUNAWAY;                   /*     then report a tape runaway */
                else                                    /*   otherwise report the physical EOF */
                    r = MTSE_EOM;                       /*     as the end-of-medium */
                break;
                }

            uptr->pos = uptr->pos + sizeof (t_mtrlnt);  /* space over the marker */

            if (*bc == MTR_TMK) {                       /* if the value is a tape mark */
                r = MTSE_TMK;                           /*   then quit with tape mark status */
                break;
                }

            else if (*bc == MTR_GAP)                    /* otherwise if the value is a full gap */
                runaway_counter -= sizeof_gap;          /*   then decrement the gap counter */

            else if (*bc == MTR_FHGAP) {                        /* otherwise if the value if a half gap */
                uptr->pos = uptr->pos - sizeof (t_mtrlnt) / 2;  /*   then back up */
                (void)sim_fseek (uptr->fileref, uptr->pos, SEEK_SET); /*     to resync */
                bufcntr = bufcap;                               /* mark the buffer as invalid to force a read */

                *bc = MTR_GAP;                                  /* reset the marker */
                runaway_counter -= sizeof_gap / 2;              /*   and decrement the gap counter */
                }

            else {                                                  /* otherwise it's a record marker */
                if (bufcntr < bufcap)                               /* if the position is within the buffer */
                    (void)sim_fseek (uptr->fileref, uptr->pos, SEEK_SET); /*   then seek to the data area */

                sbc = MTR_L (*bc);                                  /* extract the record length */
                uptr->pos = uptr->pos + sizeof (t_mtrlnt)           /* position to the start */
                  + (f == MTUF_F_STD ? (sbc + 1) & ~1 : sbc);       /*   of the record */
                }
            }
        while (*bc == MTR_GAP && runaway_counter > 0);  /* continue until data or runaway occurs */

        if (r == MTSE_OK && runaway_counter <= 0)       /* if a tape runaway occurred */
            r = MTSE_RUNAWAY;                           /*   then report it */

        break;                                          /* otherwise the operation succeeded */

    case MTUF_F_TPC:
        (void)sim_fread (&tpcbc, sizeof (t_tpclnt), 1, uptr->fileref);
        *bc = tpcbc;                                    /* save rec lnt */
        if (ferror (uptr->fileref)) {                   /* error? */
            MT_SET_PNU (uptr);                          /* pos not upd */
            return sim_tape_ioerr (uptr);
            }
        if (feof (uptr->fileref)) {                     /* eof? */
            MT_SET_PNU (uptr);                          /* pos not upd */
            r = MTSE_EOM;
            break;
            }
        uptr->pos = uptr->pos + sizeof (t_tpclnt);      /* spc over reclnt */
        if (tpcbc == TPC_TMK)                           /* tape mark? */
            r = MTSE_TMK;
        uptr->pos = uptr->pos + ((tpcbc + 1) & ~1);     /* spc over record */
        break;

    case MTUF_F_P7B:
        for (sbc = 0, all_eof = 1; ; sbc++) {           /* loop thru record */
            (void)sim_fread (&c, sizeof (uint8), 1, uptr->fileref);
            if (ferror (uptr->fileref)) {               /* error? */
                MT_SET_PNU (uptr);                      /* pos not upd */
                return sim_tape_ioerr (uptr);
                }
            if (feof (uptr->fileref)) {                 /* eof? */
                if (sbc == 0)                           /* no data? eom */
                    return MTSE_EOM;
                break;                                  /* treat like eor */
                }
            if ((sbc != 0) && (c & P7B_SOR))            /* next record? */
                break;
            if ((c & P7B_DPAR) != P7B_EOF)
                all_eof = 0;
            }
        *bc = sbc;                                      /* save rec lnt */
        (void)sim_fseek (uptr->fileref, uptr->pos, SEEK_SET); /* for read */
        uptr->pos = uptr->pos + sbc;                    /* spc over record */
        if (all_eof)                                    /* tape mark? */
            r = MTSE_TMK;
        break;

    default:
        return MTSE_FMT;
        }
#ifdef TESTING
sim_debug (MTSE_DBG_STR, ctx->dptr, "rd_lnt: st: %d, lnt: %d, pos: %" T_ADDR_FMT "u\n", r, *bc, uptr->pos);
#endif /* TESTING */
return r;
}

/* Read record length reverse (internal routine)

   Inputs:
        uptr    =       pointer to tape unit
        bc      =       pointer to returned record length
   Outputs:
        status  =       operation status

   exit condition       tape position
   ------------------   -------------------------------------------
   unit unattached      unchanged
   beginning of tape    unchanged
   read error           unchanged
   end of file          unchanged
   end of medium        updated
   tape mark            updated
   tape runaway         updated
   data record          updated, sim_fread will read record forward

   This routine is called to set up a record read or spacing in the reverse
   direction.  On return, status is MTSE_OK and the tape is positioned at the
   first data byte if a record was encountered, or status is an MTSE error code
   giving the reason that the operation did not succeed and the tape position is
   as indicated above.

   See the notes at "sim_tape_rdlntf" and "sim_tape_wrgap" regarding tape
   runaway and the erase gap implementation, respectively.
*/

static t_stat sim_tape_rdlntr (UNIT *uptr, t_mtrlnt *bc)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
uint8 c;
t_bool all_eof;
uint32 f = MT_GET_FMT (uptr);
t_addr ppos;
t_mtrlnt sbc;
t_tpclnt tpcbc;
t_mtrlnt buffer [256];                                  /* local tape buffer */
uint32 bufcntr, bufcap;                                 /* buffer counter and capacity */
int32 runaway_counter, sizeof_gap;                      /* bytes remaining before runaway and bytes per gap */
t_stat r = MTSE_OK;

MT_CLR_PNU (uptr);                                      /* clear the position-not-updated flag */
*bc = 0;

if ((uptr->flags & UNIT_ATT) == 0)                      /* if the unit is not attached */
    return MTSE_UNATT;                                  /*   then quit with an error */
if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */

if (sim_tape_bot (uptr))                                /* if the unit is positioned at the BOT */
    return MTSE_BOT;                                    /*   then reading backward is not possible */

switch (f) {                                            /* the read method depends on the tape format */

    case MTUF_F_STD:
    case MTUF_F_E11:
        runaway_counter = 25 * 12 * bpi [MT_DENS (uptr->dynflags)]; /* set largest legal gap size in bytes */

        if (runaway_counter == 0) {                     /* if tape density has not been not set */
            sizeof_gap = 0;                             /*   then disable runaway detection */
            runaway_counter = INT_MAX;                  /*     to allow gaps of any size */
            }

        else                                            /* otherwise */
            sizeof_gap = sizeof (t_mtrlnt);             /*   set the size of the gap */

        bufcntr = 0;                                    /* force an initial read */
        bufcap = 1;                                     /*   but of just one metadata marker */

        do {                                            /* loop until a record, gap, or error seen */
            if (bufcntr == 0) {                         /* if the buffer is empty then refill it */
                if (sim_tape_bot (uptr)) {              /* if the search has backed into the BOT */
                    r = MTSE_BOT;                       /*   then quit with an error */
                    break;
                    }

                else if (uptr->pos < sizeof (buffer))   /* if less than a full buffer remains */
                    bufcap = (uint32) uptr->pos         /*   then reduce the capacity accordingly */
                               / sizeof (t_mtrlnt);

                (void)sim_fseek (uptr->fileref,                           /* seek back to the location */
                           uptr->pos - bufcap * sizeof (t_mtrlnt),  /*   corresponding to the start */
                           SEEK_SET);                               /*     of the buffer */

                bufcntr = sim_fread (buffer, sizeof (t_mtrlnt),     /* fill the buffer */
                                     bufcap, uptr->fileref);        /*   with tape metadata */

                if (ferror (uptr->fileref)) {           /* if a file I/O error occurred */
                    MT_SET_PNU (uptr);                  /*   then set position not updated */
                    r = sim_tape_ioerr (uptr);          /*     report the error and quit */
                    break;
                    }

                else                                    /* otherwise reset the capacity */
                    bufcap = sizeof (buffer)            /*   to the full size of the buffer */
                               / sizeof (buffer [0]);
                }

            *bc = buffer [--bufcntr];                   /* store the metadata marker value */

            uptr->pos = uptr->pos - sizeof (t_mtrlnt);  /* backspace over the marker */

            if (*bc == MTR_TMK) {                       /* if the marker is a tape mark */
                r = MTSE_TMK;                           /*   then quit with tape mark status */
                break;
                }

            else if (*bc == MTR_GAP)                    /* otherwise if the marker is a full gap */
                runaway_counter -= sizeof_gap;          /*   then decrement the gap counter */

            else if ((*bc & MTR_M_RHGAP) == MTR_RHGAP           /* otherwise if the marker */
              || *bc == MTR_RRGAP) {                            /*   is a half gap */
                uptr->pos = uptr->pos + sizeof (t_mtrlnt) / 2;  /*     then position forward to resync */
                bufcntr = 0;                                    /* mark the buffer as invalid to force a read */

                *bc = MTR_GAP;                                  /* reset the marker */
                runaway_counter -= sizeof_gap / 2;              /*   and decrement the gap counter */
                }

            else {                                              /* otherwise it's a record marker */
                sbc = MTR_L (*bc);                              /* extract the record length */
                uptr->pos = uptr->pos - sizeof (t_mtrlnt)       /* position to the start */
                  - (f == MTUF_F_STD ? (sbc + 1) & ~1 : sbc);   /*   of the record */
                (void)sim_fseek (uptr->fileref,                       /* seek to the data area */
                           uptr->pos + sizeof (t_mtrlnt), SEEK_SET);
                }
            }
        while (*bc == MTR_GAP && runaway_counter > 0);  /* continue until data or runaway occurs */

        if (r == MTSE_OK && runaway_counter <= 0)       /* if a tape runaway occurred */
            r = MTSE_RUNAWAY;                           /*   then report it */

        break;                                          /* otherwise the operation succeeded */

    case MTUF_F_TPC:
        ppos = sim_tape_tpc_fnd (uptr, (t_addr *) uptr->filebuf); /* find prev rec */
        (void)sim_fseek (uptr->fileref, ppos, SEEK_SET);      /* position */
        (void)sim_fread (&tpcbc, sizeof (t_tpclnt), 1, uptr->fileref);
        *bc = tpcbc;                                    /* save rec lnt */
        if (ferror (uptr->fileref))                     /* error? */
            return sim_tape_ioerr (uptr);
        if (feof (uptr->fileref)) {                     /* eof? */
            r = MTSE_EOM;
            break;
            }
        uptr->pos = ppos;                               /* spc over record */
        if (*bc == MTR_TMK) {                           /* tape mark? */
            r = MTSE_TMK;
            break;
            }
        (void)sim_fseek (uptr->fileref, uptr->pos + sizeof (t_tpclnt), SEEK_SET);
        break;

    case MTUF_F_P7B:
        for (sbc = 1, all_eof = 1; (t_addr) sbc <= uptr->pos ; sbc++) {
            (void)sim_fseek (uptr->fileref, uptr->pos - sbc, SEEK_SET);
            (void)sim_fread (&c, sizeof (uint8), 1, uptr->fileref);
            if (ferror (uptr->fileref))                 /* error? */
                return sim_tape_ioerr (uptr);
            if (feof (uptr->fileref)) {                 /* eof? */
                r = MTSE_EOM;
                break;
                }
            if ((c & P7B_DPAR) != P7B_EOF)
                all_eof = 0;
            if (c & P7B_SOR)                            /* start of record? */
                break;
            }
        uptr->pos = uptr->pos - sbc;                    /* update position */
        *bc = sbc;                                      /* save rec lnt */
        (void)sim_fseek (uptr->fileref, uptr->pos, SEEK_SET); /* for read */
        if (all_eof)                                    /* tape mark? */
            r = MTSE_TMK;
        break;

    default:
        return MTSE_FMT;
        }
sim_debug (MTSE_DBG_STR, ctx->dptr, "rd_lnt: st: %d, lnt: %d, pos: %" T_ADDR_FMT "u\n", r, *bc, uptr->pos);
return r;
}

/* Read record forward

   Inputs:
        uptr    =       pointer to tape unit
        buf     =       pointer to buffer
        bc      =       pointer to returned record length
        max     =       maximum record size
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   read error           unchanged, PNU set
   end of file/medium   unchanged, PNU set
   invalid record       unchanged, PNU set
   tape mark            updated
   data record          updated
   data record error    updated
*/

t_stat sim_tape_rdrecf (UNIT *uptr, uint8 *buf, t_mtrlnt *bc, t_mtrlnt max)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
uint32 f = MT_GET_FMT (uptr);
t_mtrlnt i, tbc, rbc;
t_addr opos;
t_stat st;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_rdrecf(unit=%d, buf=%p, max=%d)\n", (int)(uptr-ctx->dptr->units), buf, max);

opos = uptr->pos;                                       /* old position */
if (MTSE_OK != (st = sim_tape_rdlntf (uptr, &tbc)))     /* read rec lnt */
    return st;
*bc = rbc = MTR_L (tbc);                                /* strip error flag */
if (rbc > max) {                                        /* rec out of range? */
    MT_SET_PNU (uptr);
    uptr->pos = opos;
    return MTSE_INVRL;
    }
i = (t_mtrlnt)sim_fread (buf, sizeof (uint8), rbc, uptr->fileref);/* read record */
if (ferror (uptr->fileref)) {                           /* error? */
    MT_SET_PNU (uptr);
    uptr->pos = opos;
    return sim_tape_ioerr (uptr);
    }
for ( ; i < rbc; i++)                                   /* fill with 0's */
    buf[i] = 0;
if (f == MTUF_F_P7B)                                    /* p7b? strip SOR */
    buf[0] = buf[0] & P7B_DPAR;
sim_tape_data_trace(uptr, buf, rbc, "Record Read", ctx->dptr->dctrl & MTSE_DBG_DAT, MTSE_DBG_STR);
return (MTR_F (tbc)? MTSE_RECE: MTSE_OK);
}

t_stat sim_tape_rdrecf_a (UNIT *uptr, uint8 *buf, t_mtrlnt *bc, t_mtrlnt max, TAPE_PCALLBACK callback)
{
t_stat r = SCPE_OK;
    r = sim_tape_rdrecf (uptr, buf, bc, max);
return r;
}

/* Read record reverse

   Inputs:
        uptr    =       pointer to tape unit
        buf     =       pointer to buffer
        bc      =       pointer to returned record length
        max     =       maximum record size
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   read error           unchanged
   end of file          unchanged
   end of medium        updated
   invalid record       unchanged
   tape mark            updated
   data record          updated
   data record error    updated
*/

t_stat sim_tape_rdrecr (UNIT *uptr, uint8 *buf, t_mtrlnt *bc, t_mtrlnt max)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
uint32 f = MT_GET_FMT (uptr);
t_mtrlnt i, rbc, tbc;
t_stat st;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_rdrecr(unit=%d, buf=%p, max=%d)\n", (int)(uptr-ctx->dptr->units), buf, max);

if (MTSE_OK != (st = sim_tape_rdlntr (uptr, &tbc)))     /* read rec lnt */
    return st;
*bc = rbc = MTR_L (tbc);                                /* strip error flag */
if (rbc > max)                                          /* rec out of range? */
    return MTSE_INVRL;
i = (t_mtrlnt)sim_fread (buf, sizeof (uint8), rbc, uptr->fileref);/* read record */
if (ferror (uptr->fileref))                             /* error? */
    return sim_tape_ioerr (uptr);
for ( ; i < rbc; i++)                                   /* fill with 0's */
    buf[i] = 0;
if (f == MTUF_F_P7B)                                    /* p7b? strip SOR */
    buf[0] = buf[0] & P7B_DPAR;
sim_tape_data_trace(uptr, buf, rbc, "Record Read Reverse", ctx->dptr->dctrl & MTSE_DBG_DAT, MTSE_DBG_STR);
return (MTR_F (tbc)? MTSE_RECE: MTSE_OK);
}

t_stat sim_tape_rdrecr_a (UNIT *uptr, uint8 *buf, t_mtrlnt *bc, t_mtrlnt max, TAPE_PCALLBACK callback)
{
t_stat r = SCPE_OK;
    r = sim_tape_rdrecr (uptr, buf, bc, max);
return r;
}

/* Write record forward

   Inputs:
        uptr    =       pointer to tape unit
        buf     =       pointer to buffer
        bc      =       record length
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   write protect        unchanged
   write error          unchanged, PNU set
   data record          updated
*/

t_stat sim_tape_wrrecf (UNIT *uptr, uint8 *buf, t_mtrlnt bc)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
uint32 f = MT_GET_FMT (uptr);
t_mtrlnt sbc;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_wrrecf(unit=%d, buf=%p, bc=%d)\n", (int)(uptr-ctx->dptr->units), buf, bc);

sim_tape_data_trace(uptr, buf, bc, "Record Write", ctx->dptr->dctrl & MTSE_DBG_DAT, MTSE_DBG_STR);
MT_CLR_PNU (uptr);
sbc = MTR_L (bc);
if ((uptr->flags & UNIT_ATT) == 0)                      /* not attached? */
    return MTSE_UNATT;
if (sim_tape_wrp (uptr))                                /* write prot? */
    return MTSE_WRP;
if (sbc == 0)                                           /* nothing to do? */
    return MTSE_OK;
(void)sim_fseek (uptr->fileref, uptr->pos, SEEK_SET);         /* set pos */
switch (f) {                                            /* case on format */

    case MTUF_F_STD:                                    /* standard */
        sbc = MTR_L ((bc + 1) & ~1);                    /* pad odd length */
    /*FALLTHRU*/ /* fallthrough */
    case MTUF_F_E11:                                    /* E11 */
        (void)sim_fwrite (&bc, sizeof (t_mtrlnt), 1, uptr->fileref);
        (void)sim_fwrite (buf, sizeof (uint8), sbc, uptr->fileref);
        (void)sim_fwrite (&bc, sizeof (t_mtrlnt), 1, uptr->fileref);
        if (ferror (uptr->fileref)) {                   /* error? */
            MT_SET_PNU (uptr);
            return sim_tape_ioerr (uptr);
            }
        uptr->pos = uptr->pos + sbc + (2 * sizeof (t_mtrlnt));  /* move tape */
        break;

    case MTUF_F_P7B:                                    /* Pierce 7B */
        buf[0] = buf[0] | P7B_SOR;                      /* mark start of rec */
        (void)sim_fwrite (buf, sizeof (uint8), sbc, uptr->fileref);
        (void)sim_fwrite (buf, sizeof (uint8), 1, uptr->fileref); /* delimit rec */
        if (ferror (uptr->fileref)) {                   /* error? */
            MT_SET_PNU (uptr);
            return sim_tape_ioerr (uptr);
            }
        uptr->pos = uptr->pos + sbc;                    /* move tape */
        break;
        }
sim_tape_data_trace(uptr, buf, sbc, "Record Written", ctx->dptr->dctrl & MTSE_DBG_DAT, MTSE_DBG_STR);
return MTSE_OK;
}

t_stat sim_tape_wrrecf_a (UNIT *uptr, uint8 *buf, t_mtrlnt bc, TAPE_PCALLBACK callback)
{
t_stat r = SCPE_OK;
    r = sim_tape_wrrecf (uptr, buf, bc);
return r;
}

/* Write metadata forward (internal routine) */

static t_stat sim_tape_wrdata (UNIT *uptr, uint32 dat)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;

MT_CLR_PNU (uptr);
if ((uptr->flags & UNIT_ATT) == 0)                      /* not attached? */
    return MTSE_UNATT;
if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
if (sim_tape_wrp (uptr))                                /* write prot? */
    return MTSE_WRP;
(void)sim_fseek (uptr->fileref, uptr->pos, SEEK_SET);         /* set pos */
(void)sim_fwrite (&dat, sizeof (t_mtrlnt), 1, uptr->fileref);
if (ferror (uptr->fileref)) {                           /* error? */
    MT_SET_PNU (uptr);
    return sim_tape_ioerr (uptr);
    }
sim_debug (MTSE_DBG_STR, ctx->dptr, "wr_lnt: lnt: %d, pos: %" T_ADDR_FMT "u\n", dat, uptr->pos);
uptr->pos = uptr->pos + sizeof (t_mtrlnt);              /* move tape */
return MTSE_OK;
}

/* Write tape mark */

t_stat sim_tape_wrtmk (UNIT *uptr)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_wrtmk(unit=%d)\n", (int)(uptr-ctx->dptr->units));
if (MT_GET_FMT (uptr) == MTUF_F_P7B) {                  /* P7B? */
    uint8 buf = P7B_EOF;                                /* eof mark */
    return sim_tape_wrrecf (uptr, &buf, 1);             /* write char */
    }
return sim_tape_wrdata (uptr, MTR_TMK);
}

t_stat sim_tape_wrtmk_a (UNIT *uptr, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_wrtmk (uptr);
return r;
}

/* Write end of medium */

t_stat sim_tape_wreom (UNIT *uptr)
{
t_stat result;
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_wreom(unit=%d)\n", (int)(uptr-ctx->dptr->units));
if (MT_GET_FMT (uptr) == MTUF_F_P7B)                    /* cant do P7B */
    return MTSE_FMT;

result = sim_tape_wrdata (uptr, MTR_EOM);               /* write the EOM marker */

uptr->pos = uptr->pos - sizeof (t_mtrlnt);              /* restore original tape position */
MT_SET_PNU (uptr);                                      /* indicate that position was not updated */

return result;
}

t_stat sim_tape_wreom_a (UNIT *uptr, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_wreom (uptr);
return r;
}

/* Write end of medium-rewind */

t_stat sim_tape_wreomrw (UNIT *uptr)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
t_stat r;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_wreomrw(unit=%d)\n", (int)(uptr-ctx->dptr->units));
if (MT_GET_FMT (uptr) == MTUF_F_P7B)                    /* cant do P7B */
    return MTSE_FMT;
r = sim_tape_wrdata (uptr, MTR_EOM);
if (r == MTSE_OK)
    r = sim_tape_rewind (uptr);
return r;
}

t_stat sim_tape_wreomrw_a (UNIT *uptr, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_wreomrw (uptr);
return r;
}

/* Write erase gap

   Inputs:
        uptr    = pointer to tape unit
        gaplen  = length of gap in tenths of an inch

   Outputs:
        status  = operation status

   exit condition       position
   ------------------   ------------------
   unit unattached      unchanged
   unsupported format   unchanged
   write protected      unchanged
   read error           unchanged, PNU set
   write error          unchanged, PNU set
   gap written          updated

   An erase gap is represented in the tape image file by a special metadata
   value.  This value is chosen so that it is still recognizable even if it has
   been "cut in half" by a subsequent data overwrite that does not end on a
   metadatum-sized boundary.  In addition, a range of metadata values are
   reserved for detection in the reverse direction.  Erase gaps are currently
   supported only in SIMH (MTUF_F_STD) tape format.

   This implementation supports erasing gaps in the middle of a populated tape
   image and will always produce a valid image.  It also produces valid images
   when overwriting gaps with data records, with one exception: a data write
   that leaves only two bytes of gap remaining will produce an invalid tape.
   This limitation is deemed acceptable, as it is analogous to the existing
   limitation that data records cannot overwrite other data records without
   producing an invalid tape.

   Because SIMH tape images do not carry physical parameters (e.g., recording
   density), overwriting a tape image file containing gap metadata is
   problematic if the density setting is not the same as that used during
   recording.  There is no way to establish a gap of a certain length
   unequivocally in an image file, so this implementation establishes a gap of a
   certain number of bytes that reflect the desired gap length at the tape
   density in bits per inch used during writing.

   To write an erase gap, the implementation uses one of two approaches,
   depending on whether or not the current tape position is at EOM.  Erasing at
   EOM presents no special difficulties; gap metadata markers are written for
   the prescribed number of bytes.  If the tape is not at EOM, then erasing must
   take into account the existing record structure to ensure that a valid tape
   image is maintained.

   The general approach is to erase for the nominal number of bytes but to
   increase that length, if necessary, to ensure that a partially overwritten
   data record at the end of the gap can be altered to maintain validity.
   Because the smallest legal tape record requires space for two metadata
   markers plus two data bytes, an erasure that would leave less than that
   is increased to consume the entire record.  Otherwise, the final record is
   truncated appropriately by rewriting the leading and trailing length words
   appropriately.

   When reading in either direction, gap metadata markers are ignored (skipped)
   until a record length header, EOF marker, EOM marker, or physical EOF is
   encountered.  Thus, tape images containing gap metadata are transparent to
   the calling simulator (unless tape runaway support is enabled -- see the
   notes at "sim_tape_rdlntf" for details).

   The permissibility of data record lengths that are not multiples of the
   metadatum size presents a difficulty when reading.  If such an "odd length"
   record is written over a gap, half of a metadata marker will exist
   immediately after the trailing record length.

   This condition is detected when reading forward by the appearance of a
   "reversed" marker.  The value appears reversed because the value is made up
   of half of one marker and half of the next.  This is handled by seeking
   forward two bytes to resync (the stipulation above that the overwrite cannot
   leave only two bytes of gap means that at least one "whole" metadata marker
   will follow).  Reading in reverse presents a more complex problem, because
   half of the marker is from the preceding trailing record length marker and
   therefore could be any of a range of values.  However, that range is
   restricted by the SIMH tape specification requirement that record length
   metadata values must have bits 30:24 set to zero.  This allows unambiguous
   detection of the condition.

   The value chosen for gap metadata and the values reserved for "half-gap"
   detection are:

     0xFFFFFFFE            - primary gap value
     0xFFFEFFFF            - reserved (indicates half-gap in forward reads)
     0xFFFF0000:0xFFFF00FF - reserved (indicates half-gap in reverse reads)
     0xFFFF8000:0xFFFF80FF - reserved (indicates half-gap in reverse reads)

   If the tape density has been set via a previous sim_tape_set_dens call, and
   the tape format is set to SIMH format, then this routine will write a gap of
   the appropriate size.  If the density has not been set, then no action will
   be taken, and either MTSE_IOERR or MTSE_OK status will be returned, depending
   on whether SIMH or another format is selected, respectively.  A simulator
   that calls this routine must set the density beforehand; failure to do so is
   an error.  However, calling while another format is enabled is OK and is
   treated as a no-operation.  This allows a device simulator that supports
   writing erase gaps to use the same code without worrying about the tape
   format currently selected by the user.
*/

t_stat sim_tape_wrgap (UNIT *uptr, uint32 gaplen)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
t_stat st;
t_mtrlnt meta, sbc, new_len, rec_size;
t_addr gap_pos = uptr->pos;
uint32 file_size, marker_count, tape_density;
int32 gap_needed;
uint32 gap_alloc = 0;                                   /* gap currently allocated from the tape */
const uint32 format = MT_GET_FMT (uptr);                /* tape format */
const uint32 meta_size = sizeof (t_mtrlnt);             /* bytes per metadatum */
const uint32 min_rec_size = 2 + sizeof (t_mtrlnt) * 2;  /* smallest data record size */

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_wrgap(unit=%d, gaplen=%u)\n", (int)(uptr-ctx->dptr->units), gaplen);

MT_CLR_PNU (uptr);

if ((uptr->flags & UNIT_ATT) == 0)                      /* if the unit is not attached */
    return MTSE_UNATT;                                  /*   then we cannot proceed */

else if (sim_tape_wrp (uptr))                           /* otherwise if the unit is write protected */
    return MTSE_WRP;                                    /*   then we cannot write */

tape_density = bpi [MT_DENS (uptr->dynflags)];          /* get the density of the tape */

if (format != MTUF_F_STD)                               /* if erase gaps aren't supported by the format */
    return MTSE_OK;                                     /*   then take no action */
else if (tape_density == 0)                             /* otherwise if the density is not set */
    return MTSE_IOERR;                                  /*   then report an I/O error */
else                                                    /* otherwise */
    gap_needed = (gaplen * tape_density) / 10;          /*   determine the gap size needed in bytes */

file_size = sim_fsize (uptr->fileref);                  /* get file size */
sim_fseek (uptr->fileref, uptr->pos, SEEK_SET);         /* position tape */

/* Read tape records and allocate to gap until amount required is consumed.

   Read next metadatum from tape:
    - EOF or EOM: allocate remainder of bytes needed.
    - TMK or GAP: allocate sizeof(metadatum) bytes.
    - Reverse GAP: allocate sizeof(metadatum) / 2 bytes.
    - Data record: see below.

   Loop until bytes needed = 0.
*/

do {
    (void)sim_fread (&meta, meta_size, 1, uptr->fileref);     /* read metadatum */

    if (ferror (uptr->fileref)) {                       /* read error? */
        uptr->pos = gap_pos;                            /* restore original position */
        MT_SET_PNU (uptr);                              /* position not updated */
        return sim_tape_ioerr (uptr);                   /* translate error */
        }
    else
        uptr->pos = uptr->pos + meta_size;              /* move tape over datum */

    if (feof (uptr->fileref) || (meta == MTR_EOM)) {    /* at eof or eom? */
        gap_alloc = gap_alloc + gap_needed;             /* allocate remainder */
        gap_needed = 0;
        }

    else if ((meta == MTR_GAP) || (meta == MTR_TMK)) {  /* gap or tape mark? */
        gap_alloc = gap_alloc + meta_size;              /* allocate marker space */
        gap_needed = gap_needed - meta_size;            /* reduce requirement */
        }

    else if (meta == MTR_FHGAP) {                       /* half gap? */
        uptr->pos = uptr->pos - meta_size / 2;          /* backup to resync */
        sim_fseek (uptr->fileref, uptr->pos, SEEK_SET); /* position tape */
        gap_alloc = gap_alloc + meta_size / 2;          /* allocate marker space */
        gap_needed = gap_needed - meta_size / 2;        /* reduce requirement */
        }

    else if (uptr->pos +
             MTR_L (meta) + meta_size > file_size) {    /* rec len out of range? */
        gap_alloc = gap_alloc + gap_needed;             /* presume overwritten tape */
        gap_needed = 0;                                 /* allocate remainder */
        }

/* Allocate a data record:
    - Determine record size in bytes (including metadata)
    - If record size - bytes needed < smallest allowed record size,
      allocate entire record to gap, else allocate needed amount and
      truncate data record to reflect remainder.
*/
    else {                                              /* data record */
        sbc = MTR_L (meta);                             /* get record data length */
        rec_size = ((sbc + 1) & ~1) + meta_size * 2;    /* overall size in bytes */

        if (rec_size < gap_needed + min_rec_size) {         /* rec too small? */
            uptr->pos = uptr->pos - meta_size + rec_size;   /* position past record */
            sim_fseek (uptr->fileref, uptr->pos, SEEK_SET); /* move tape */
            gap_alloc = gap_alloc + rec_size;               /* allocate record */
            gap_needed = gap_needed - rec_size;             /* reduce requirement */
            }

        else {                                              /* record size OK */
            uptr->pos = uptr->pos - meta_size + gap_needed; /* position to end of gap */
            new_len = MTR_F (meta) | (sbc - gap_needed);    /* truncate to new len */
            st = sim_tape_wrdata (uptr, new_len);           /* write new rec len */

            if (st != MTSE_OK) {                            /* write OK? */
                uptr->pos = gap_pos;                        /* restore orig pos */
                return st;                                  /* PNU was set by wrdata */
                }

            uptr->pos = uptr->pos + sbc - gap_needed;       /* position to end of data */
            st = sim_tape_wrdata (uptr, new_len);           /* write new rec len */

            if (st != MTSE_OK) {                            /* write OK? */
                uptr->pos = gap_pos;                        /* restore orig pos */
                return st;                                  /* PNU was set by wrdata */
                }

            gap_alloc = gap_alloc + gap_needed;             /* allocate remainder */
            gap_needed = 0;
            }
        }
    }
while (gap_needed > 0);

uptr->pos = gap_pos;                                    /* reposition to gap start */

if (gap_alloc & (meta_size - 1)) {                      /* gap size "odd?" */
    st = sim_tape_wrdata (uptr, MTR_FHGAP);             /* write half gap marker */
    if (st != MTSE_OK) {                                /* write OK? */
        uptr->pos = gap_pos;                            /* restore orig pos */
        return st;                                      /* PNU was set by wrdata */
        }
    uptr->pos = uptr->pos - meta_size / 2;              /* realign position */
    gap_alloc = gap_alloc - 2;                          /* decrease gap to write */
    }

marker_count = gap_alloc / meta_size;                   /* count of gap markers */

do {
    st = sim_tape_wrdata (uptr, MTR_GAP);               /* write gap markers */
    if (st != MTSE_OK) {                                /* write OK? */
        uptr->pos = gap_pos;                            /* restore orig pos */
        return st;                                      /* PNU was set by wrdata */
        }
    }
while (--marker_count > 0);

return MTSE_OK;
}

t_stat sim_tape_wrgap_a (UNIT *uptr, uint32 gaplen, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
return r;
}

/* Space record forward

   Inputs:
        uptr    =       pointer to tape unit
        bc      =       pointer to size of record skipped
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   read error           unchanged, PNU set
   end of file/medium   unchanged, PNU set
   tape mark            updated
   data record          updated
   data record error    updated
*/

t_stat sim_tape_sprecf (UNIT *uptr, t_mtrlnt *bc)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
t_stat st;

*bc = 0;
if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_sprecf(unit=%d)\n", (int)(uptr-ctx->dptr->units));

st = sim_tape_rdlntf (uptr, bc);                        /* get record length */
*bc = MTR_L (*bc);
return st;
}

t_stat sim_tape_sprecf_a (UNIT *uptr, t_mtrlnt *bc, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_sprecf (uptr, bc);
return r;
}

/* Space records forward

   Inputs:
        uptr    =       pointer to tape unit
        count   =       count of records to skip
        skipped =       pointer to number of records actually skipped
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   read error           unchanged, PNU set
   end of file/medium   unchanged, PNU set
   tape mark            updated
   data record          updated
   data record error    updated
*/

t_stat sim_tape_sprecsf (UNIT *uptr, uint32 count, uint32 *skipped)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
t_stat st;
t_mtrlnt tbc;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_sprecsf(unit=%d, count=%d)\n", (int)(uptr-ctx->dptr->units), count);

*skipped = 0;
while (*skipped < count) {                              /* loopo */
    st = sim_tape_sprecf (uptr, &tbc);                  /* spc rec */
    if (st != MTSE_OK)
        return st;
    *skipped = *skipped + 1;                            /* # recs skipped */
    }
return MTSE_OK;
}

t_stat sim_tape_sprecsf_a (UNIT *uptr, uint32 count, uint32 *skipped, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_sprecsf (uptr, count, skipped);
return r;
}

/* Space record reverse

   Inputs:
        uptr    =       pointer to tape unit
        bc      =       pointer to size of records skipped
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   beginning of tape    unchanged
   read error           unchanged
   end of file          unchanged
   end of medium        updated
   tape mark            updated
   data record          updated
*/

t_stat sim_tape_sprecr (UNIT *uptr, t_mtrlnt *bc)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
t_stat st;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_sprecr(unit=%d)\n", (int)(uptr-ctx->dptr->units));

if (MT_TST_PNU (uptr)) {
    MT_CLR_PNU (uptr);
    *bc = 0;
    return MTSE_OK;
    }
st = sim_tape_rdlntr (uptr, bc);                        /* get record length */
*bc = MTR_L (*bc);
return st;
}

t_stat sim_tape_sprecr_a (UNIT *uptr, t_mtrlnt *bc, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_sprecr (uptr, bc);
return r;
}

/* Space records reverse

   Inputs:
        uptr    =       pointer to tape unit
        count   =       count of records to skip
        skipped =       pointer to number of records actually skipped
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   beginning of tape    unchanged
   read error           unchanged
   end of file          unchanged
   end of medium        updated
   tape mark            updated
   data record          updated
*/

t_stat sim_tape_sprecsr (UNIT *uptr, uint32 count, uint32 *skipped)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
t_stat st;
t_mtrlnt tbc;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_sprecsr(unit=%d, count=%d)\n", (int)(uptr-ctx->dptr->units), count);

*skipped = 0;
while (*skipped < count) {                              /* loopo */
    st = sim_tape_sprecr (uptr, &tbc);                  /* spc rec rev */
    if (st != MTSE_OK)
        return st;
    *skipped = *skipped + 1;                            /* # recs skipped */
    }
return MTSE_OK;
}

t_stat sim_tape_sprecsr_a (UNIT *uptr, uint32 count, uint32 *skipped, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_sprecsr (uptr, count, skipped);
return r;
}

/* Space files forward by record

   Inputs:
        uptr    =       pointer to tape unit
        count   =       count of files to skip
        skipped =       pointer to number of files actually skipped
        recsskipped =   pointer to number of records skipped
        check_leot =    flag to detect and stop skip between two successive tape marks
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   read error           unchanged, PNU set
   end of file/medium   unchanged, PNU set
   tape mark            updated
   data record          updated
   data record error    updated
*/

t_stat sim_tape_spfilebyrecf (UNIT *uptr, uint32 count, uint32 *skipped, uint32 *recsskipped, t_bool check_leot)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
t_stat st;
t_bool last_tapemark = FALSE;
uint32 filerecsskipped = 0;
*skipped = *recsskipped = 0;
if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_spfilebyrecf(unit=%d, count=%d, check_leot=%d)\n", (int)(uptr-ctx->dptr->units), count, check_leot);

if (check_leot) {
    t_mtrlnt rbc;

    st = sim_tape_rdlntr (uptr, &rbc);
    last_tapemark = (MTSE_TMK == st);
    if ((st == MTSE_OK) || (st == MTSE_TMK))
        sim_tape_rdlntf (uptr, &rbc);
    }
*skipped = 0;
*recsskipped = 0;
while (*skipped < count) {                              /* loopo */
    while (1) {
        st = sim_tape_sprecsf (uptr, 0x1ffffff, &filerecsskipped);/* spc recs */
        *recsskipped += filerecsskipped;
        if (st != MTSE_OK)
            break;
        }
    if (st == MTSE_TMK) {
        *skipped = *skipped + 1;                        /* # files skipped */
        if (check_leot && (filerecsskipped == 0) && last_tapemark) {
            uint32 filefileskipped;
            sim_tape_spfilebyrecr (uptr, 1, &filefileskipped, &filerecsskipped);
            *skipped = *skipped - 1;                    /* adjust # files skipped */
            return MTSE_LEOT;
            }
        last_tapemark = TRUE;
        }
    else
        return st;
    }
return MTSE_OK;
}

t_stat sim_tape_spfilebyrecf_a (UNIT *uptr, uint32 count, uint32 *skipped, uint32 *recsskipped, t_bool check_leot, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_spfilebyrecf (uptr, count, skipped, recsskipped, check_leot);
return r;
}

/* Space files forward

   Inputs:
        uptr    =       pointer to tape unit
        count   =       count of files to skip
        skipped =       pointer to number of files actually skipped
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   read error           unchanged, PNU set
   end of file/medium   unchanged, PNU set
   tape mark            updated
   data record          updated
   data record error    updated
*/

t_stat sim_tape_spfilef (UNIT *uptr, uint32 count, uint32 *skipped)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
uint32 totalrecsskipped;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_spfilef(unit=%d, count=%d)\n", (int)(uptr-ctx->dptr->units), count);

return sim_tape_spfilebyrecf (uptr, count, skipped, &totalrecsskipped, FALSE);
}

t_stat sim_tape_spfilef_a (UNIT *uptr, uint32 count, uint32 *skipped, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_spfilef (uptr, count, skipped);
return r;
}

/* Space files reverse by record

   Inputs:
        uptr    =       pointer to tape unit
        count   =       count of files to skip
        skipped =       pointer to number of files actually skipped
        recsskipped =   pointer to number of records skipped
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   beginning of tape    unchanged
   read error           unchanged
   end of file          unchanged
   end of medium        updated
   tape mark            updated
   data record          updated
*/

t_stat sim_tape_spfilebyrecr (UNIT *uptr, uint32 count, uint32 *skipped, uint32 *recsskipped)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
t_stat st;
uint32 filerecsskipped = 0;

*skipped = 0;
*recsskipped = 0;
if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_spfilebyrecr(unit=%d, count=%d)\n", (int)(uptr-ctx->dptr->units), count);

while (*skipped < count) {                              /* loopo */
    while (1) {
        st = sim_tape_sprecsr (uptr, 0x1ffffff, &filerecsskipped);/* spc recs rev */
        *recsskipped += filerecsskipped;
        if (st != MTSE_OK)
            break;
        }
    if (st == MTSE_TMK)
        *skipped = *skipped + 1;                        /* # files skipped */
    else
        return st;
    }
return MTSE_OK;
}

t_stat sim_tape_spfilebyrecr_a (UNIT *uptr, uint32 count, uint32 *skipped, uint32 *recsskipped, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_spfilebyrecr (uptr, count, skipped, recsskipped);
return r;
}

/* Space files reverse

   Inputs:
        uptr    =       pointer to tape unit
        count   =       count of files to skip
        skipped =       pointer to number of files actually skipped
   Outputs:
        status  =       operation status

   exit condition       position

   unit unattached      unchanged
   beginning of tape    unchanged
   read error           unchanged
   end of file          unchanged
   end of medium        updated
   tape mark            updated
   data record          updated
*/

t_stat sim_tape_spfiler (UNIT *uptr, uint32 count, uint32 *skipped)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
uint32 totalrecsskipped;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_spfiler(unit=%d, count=%d)\n", (int)(uptr-ctx->dptr->units), count);

return sim_tape_spfilebyrecr (uptr, count, skipped, &totalrecsskipped);
}

t_stat sim_tape_spfiler_a (UNIT *uptr, uint32 count, uint32 *skipped, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_spfiler (uptr, count, skipped);
return r;
}

/* Rewind tape */

t_stat sim_tape_rewind (UNIT *uptr)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;

if (uptr->flags & UNIT_ATT) {
    if (ctx == NULL)                                    /* if not properly attached? */
        return sim_messagef (SCPE_IERR, "Bad Attach\n");/*   that's a problem */
    sim_debug (ctx->dbit, ctx->dptr, "sim_tape_rewind(unit=%d)\n", (int)(uptr-ctx->dptr->units));
    }
uptr->pos = 0;
MT_CLR_PNU (uptr);
return MTSE_OK;
}

t_stat sim_tape_rewind_a (UNIT *uptr, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_rewind (uptr);
return r;
}

/* Position Tape */

t_stat sim_tape_position (UNIT *uptr, uint32 flags, uint32 recs, uint32 *recsskipped, uint32 files, uint32 *filesskipped, uint32 *objectsskipped)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;
t_stat r = MTSE_OK;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_position(unit=%d, flags=0x%X, recs=%d, files=%d)\n", (int)(uptr-ctx->dptr->units), flags, recs, files);

*recsskipped = *filesskipped = *objectsskipped = 0;
if (flags & MTPOS_M_REW)
    r = sim_tape_rewind (uptr);
if (r != MTSE_OK)
    return r;
if (flags & MTPOS_M_OBJ) {
    uint32 objs = recs;
    uint32 skipped;
    uint32 objsremaining = objs;

    while (*objectsskipped < objs) {                       /* loopo */
        if (flags & MTPOS_M_REV)                        /* reverse? */
            r = sim_tape_sprecsr (uptr, objsremaining, &skipped);
        else
            r = sim_tape_sprecsf (uptr, objsremaining, &skipped);
        objsremaining = objsremaining - (skipped + ((r == MTSE_TMK) ? 1 : 0));
        if ((r == MTSE_TMK) || (r == MTSE_OK))
            *objectsskipped = *objectsskipped + skipped + ((r == MTSE_TMK) ? 1 : 0);
        else
            return r;
        }
    r = MTSE_OK;
    }
else {
    uint32 fileskiprecs;

    if (flags & MTPOS_M_REV)                            /* reverse? */
        r = sim_tape_spfilebyrecr (uptr, files, filesskipped, &fileskiprecs);
    else
        r = sim_tape_spfilebyrecf (uptr, files, filesskipped, &fileskiprecs, (flags & MTPOS_M_DLE));
    if (r != MTSE_OK)
        return r;
    if (flags & MTPOS_M_REV)                            /* reverse? */
        r = sim_tape_sprecsr (uptr, recs, recsskipped);
    else
        r = sim_tape_sprecsf (uptr, recs, recsskipped);
    if (r == MTSE_TMK)
        *filesskipped = *filesskipped + 1;
    *objectsskipped = fileskiprecs + *filesskipped + *recsskipped;
    }
return r;
}

t_stat sim_tape_position_a (UNIT *uptr, uint32 flags, uint32 recs, uint32 *recsskipped, uint32 files, uint32 *filesskipped, uint32 *objectsskipped, TAPE_PCALLBACK callback)
{
t_stat r = MTSE_OK;
    r = sim_tape_position (uptr, flags, recs, recsskipped, files, filesskipped, objectsskipped);
return r;
}

/* Reset tape */

t_stat sim_tape_reset (UNIT *uptr)
{
struct tape_context *ctx = (struct tape_context *)uptr->tape_ctx;

MT_CLR_PNU (uptr);
if (!(uptr->flags & UNIT_ATT))                          /* attached? */
    return SCPE_OK;

if (ctx == NULL)                                        /* if not properly attached? */
    return sim_messagef (SCPE_IERR, "Bad Attach\n");    /*   that's a problem */
sim_debug (ctx->dbit, ctx->dptr, "sim_tape_reset(unit=%d)\n", (int)(uptr-ctx->dptr->units));

_sim_tape_io_flush(uptr);
return SCPE_OK;
}

/* Test for BOT */

t_bool sim_tape_bot (UNIT *uptr)
{
uint32 f = MT_GET_FMT (uptr);

return (uptr->pos <= fmts[f].bot)? TRUE: FALSE;
}

/* Test for end of tape */

t_bool sim_tape_eot (UNIT *uptr)
{
return (uptr->capac && (uptr->pos >= uptr->capac))? TRUE: FALSE;
}

/* Test for write protect */

t_bool sim_tape_wrp (UNIT *uptr)
{
return ((uptr->flags & MTUF_WRP) || (MT_GET_FMT (uptr) == MTUF_F_TPC))? TRUE: FALSE;
}

/* Process I/O error */

static t_stat sim_tape_ioerr (UNIT *uptr)
{
sim_printf ("%s: Magtape library I/O error: %s\n", sim_uname (uptr), strerror (errno));
clearerr (uptr->fileref);
return MTSE_IOERR;
}

/* Set tape format */

t_stat sim_tape_set_fmt (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
uint32 f;

if (uptr == NULL)
    return SCPE_IERR;
if (uptr->flags & UNIT_ATT)
    return SCPE_ALATT;
if (cptr == NULL)
    return SCPE_ARG;
for (f = 0; f < MTUF_N_FMT; f++) {
    if (fmts[f].name && (strcmp (cptr, fmts[f].name) == 0)) {
        uptr->flags = (uptr->flags & ~MTUF_FMT) |
            (f << MTUF_V_FMT) | fmts[f].uflags;
        return SCPE_OK;
        }
    }
return SCPE_ARG;
}

/* Show tape format */

t_stat sim_tape_show_fmt (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
int32 f = MT_GET_FMT (uptr);

if (fmts[f].name)
    fprintf (st, "%s format", fmts[f].name);
else fprintf (st, "invalid format");
return SCPE_OK;
}

/* Map a TPC format tape image */

static uint32 sim_tape_tpc_map (UNIT *uptr, t_addr *map, uint32 mapsize)
{
t_addr tpos, leot = 0;
t_addr tape_size;
t_tpclnt bc, last_bc = 0xFFFF;
uint32 had_double_tape_mark = 0;
size_t i;
uint32 objc, sizec;
uint32 *countmap = NULL;
uint8 *recbuf = NULL;
DEVICE *dptr = find_dev_from_unit (uptr);

if ((uptr == NULL) || (uptr->fileref == NULL))
    return 0;
countmap = (uint32 *)calloc (65536, sizeof(*countmap));
if (!countmap)
  {
    fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
             __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
recbuf = (uint8 *)malloc (65536);
if (!recbuf)
  {
    fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
             __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
tape_size = (t_addr)sim_fsize (uptr->fileref);
sim_debug (MTSE_DBG_STR, dptr, "tpc_map: tape_size: %" T_ADDR_FMT "u\n", tape_size);
for (objc = 0, sizec = 0, tpos = 0;; ) {
    sim_fseek (uptr->fileref, tpos, SEEK_SET);
    i = sim_fread (&bc, sizeof (t_tpclnt), 1, uptr->fileref);
    if (i == 0)     /* past or at eof? */
        break;
    if (countmap[bc] == 0)
        sizec++;
    ++countmap[bc];
    if (map && (objc < mapsize))
        map[objc] = tpos;
    if (bc) {
        sim_debug (MTSE_DBG_STR, dptr, "tpc_map: %d byte count at pos: %" T_ADDR_FMT "u\n", bc, tpos);
        if (sim_deb && (dptr->dctrl & MTSE_DBG_STR)) {
            (void)sim_fread (recbuf, 1, bc, uptr->fileref);
            sim_data_trace(dptr, uptr, ((dptr->dctrl & MTSE_DBG_DAT) ? recbuf : NULL), "", bc, "Data Record", MTSE_DBG_STR);
            }
        }
    else
        sim_debug (MTSE_DBG_STR, dptr, "tpc_map: tape mark at pos: %" T_ADDR_FMT "u\n", tpos);
    objc++;
    tpos = tpos + ((bc + 1) & ~1) + sizeof (t_tpclnt);
    if ((bc == 0) && (last_bc == 0)) {  /* double tape mark? */
        had_double_tape_mark = objc;
        leot = tpos;
        }
    last_bc = bc;
    }
sim_debug (MTSE_DBG_STR, dptr, "tpc_map: objc: %u, different record sizes: %u\n", objc, sizec);
for (i=0; i<65535; i++) {
    if (countmap[i]) {
        if (i == 0)
            sim_debug (MTSE_DBG_STR, dptr, "tpc_map: summary - %u tape marks\n", countmap[i]);
        else
            sim_debug (MTSE_DBG_STR, dptr, "tpc_map: summary - %u %d byte record%s\n", countmap[i], (int)i, (countmap[i] > 1) ? "s" : "");
        }
    }
if (((last_bc != 0xffff) &&
     (tpos > tape_size) &&
     (!had_double_tape_mark)) || //-V686
    (!had_double_tape_mark) ||
    ((objc == countmap[0]) &&
     (countmap[0] != 2))) {     /* Unreasonable format? */
    if (last_bc != 0xffff)
        sim_debug (MTSE_DBG_STR, dptr, "tpc_map: ERROR unexpected EOT byte count: %d\n", last_bc);
    if (tpos > tape_size)
        sim_debug (MTSE_DBG_STR, dptr, "tpc_map: ERROR next record position %" T_ADDR_FMT "u beyond EOT: %" T_ADDR_FMT "u\n", tpos, tape_size);
    if (objc == countmap[0])
        sim_debug (MTSE_DBG_STR, dptr, "tpc_map: ERROR tape cnly contains tape marks\n");
    FREE (countmap);
    FREE (recbuf);
    return 0;
    }

if ((last_bc != 0xffff) && (tpos > tape_size)) {
    sim_debug (MTSE_DBG_STR, dptr, "tpc_map: WARNING unexpected EOT byte count: %d, double tape mark before %" T_ADDR_FMT "u provides logical EOT\n", last_bc, leot);
    objc = had_double_tape_mark;
    tpos = leot;
    }
if (map)
    map[objc] = tpos;
sim_debug (MTSE_DBG_STR, dptr, "tpc_map: OK objc: %d\n", objc);
FREE (countmap);
FREE (recbuf);
return objc;
}

/* Check the basic structure of a SIMH format tape image */

static t_stat sim_tape_simh_check (UNIT *uptr)
{
return SCPE_OK;
}

/* Check the basic structure of a E11 format tape image */

static t_stat sim_tape_e11_check (UNIT *uptr)
{
return SCPE_OK;
}

/* Find the preceding record in a TPC file */

static t_addr sim_tape_tpc_fnd (UNIT *uptr, t_addr *map)
{
uint32 lo, hi, p;

if (map == NULL)
    return 0;
lo = 0;
hi = uptr->hwmark - 1;
do {
    p = (lo + hi) >> 1;
    if (uptr->pos == map[p])
        return ((p == 0)? map[p]: map[p - 1]);
    else if (uptr->pos < map[p])
        hi = p - 1;
    else lo = p + 1;
    }
while (lo <= hi);
return ((p == 0)? map[p]: map[p - 1]);
}

/* Set tape capacity */

t_stat sim_tape_set_capac (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
t_addr cap;
t_stat r;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_ARG;
if (uptr->flags & UNIT_ATT)
    return SCPE_ALATT;
cap = (t_addr) get_uint (cptr, 10, sim_taddr_64? 2000000: 2000, &r);
if (r != SCPE_OK)
    return SCPE_ARG;
uptr->capac = cap * ((t_addr) 1000000);
return SCPE_OK;
}

/* Show tape capacity */

t_stat sim_tape_show_capac (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
if (uptr->capac) {
    if (uptr->capac >= (t_addr) 1000000)
        fprintf (st, "capacity : %luMB", (unsigned long)(uptr->capac / ((t_addr) 1000000)));
    else {
        if (uptr->capac >= (t_addr) 1000)
            fprintf (st, "capacity : %luKB", (unsigned long)(uptr->capac / ((t_addr) 1000)));
        else
            fprintf (st, "capacity : %luB", (unsigned long)uptr->capac);
        }
    }
else
    fprintf (st, "capacity : unlimited");
return SCPE_OK;
}

/* Set the tape density.

   Set the density of the specified tape unit either to the value supplied or to
   the value represented by the supplied character string.

   If "desc" is NULL, then "val" must be set to one of the MT_DENS_* constants
   in sim_tape.h other than MT_DENS_NONE; the supplied value is used as the tape
   density, and the character string is ignored.  Otherwise, "desc" must point
   at an int32 value containing a set of allowed densities constructed as a
   bitwise OR of the appropriate MT_*_VALID values.  In this case, the string
   pointed to by "cptr" will be parsed for a decimal value corresponding to the
   desired density in bits per inch and validated against the set of allowed
   values.

   In either case, SCPE_ARG is returned if the density setting is not valid or
   allowed.  If the setting is OK, the new density is set into the unit
   structure, and SCPE_OK is returned.
*/

t_stat sim_tape_set_dens (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
uint32 density, new_bpi;
t_stat result = SCPE_OK;

if (uptr == NULL)                                               /* if the unit pointer is null */
    return SCPE_IERR;                                           /*   then the caller has screwed up */

else if (desc == NULL)                                          /* otherwise if a validation set was not supplied */
    if (val > 0 && val < (int32) BPI_COUNT)                     /*   then if a valid density code was supplied */
        uptr->dynflags = (uptr->dynflags & ~MTVF_DENS_MASK)     /*     then insert the code */
                           | (val << UNIT_V_DF_TAPE);           /*       in the unit flags */
    else                                                        /*   otherwise the code is invalid */
        return SCPE_ARG;                                        /*     so report a bad argument */

else {                                                          /* otherwise a validation set was supplied */
    if (cptr == NULL || *cptr == 0)                             /*   but if no value is present */
        return SCPE_MISVAL;                                     /*     then report a missing value */

    new_bpi = (uint32) get_uint (cptr, 10, UINT_MAX, &result);  /* convert the string value */

    if (result != SCPE_OK)                                      /* if the conversion failed */
        return SCPE_ARG;                                        /*   then report a bad argument */

    else for (density = 0; density < BPI_COUNT; density++)      /* otherwise validate the density */
        if (new_bpi == bpi [density]                            /* if it matches a value in the list */
          && ((1 << density) & *(const int32 *) desc)) {        /*   and it's an allowed value */
            uptr->dynflags = (uptr->dynflags & ~MTVF_DENS_MASK) /*     then store the index of the value */
                               | density << UNIT_V_DF_TAPE;     /*       in the unit flags */
            return SCPE_OK;                                     /*         and return success */
            }

    result = SCPE_ARG;                                          /* if no match, then report a bad argument */
    }

return result;                                                  /* return the result of the operation */
}

/* Show the tape density */

t_stat sim_tape_show_dens (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
uint32 tape_density;

if (uptr == NULL)                                       /* if the unit pointer is null */
    return SCPE_IERR;                                   /*   then the caller has screwed up */

else {                                                  /* otherwise get the density */
    tape_density = bpi [MT_DENS (uptr->dynflags)];      /*   of the tape from the unit flags */

    if (tape_density)                                   /* if it's set */
        fprintf (st, "density=%lu bpi",
            (unsigned long)tape_density);               /*   then report it */
    else                                                /* otherwise */
        fprintf (st, "density not set");                /*   it was never set by the caller */
    }

return SCPE_OK;
}
