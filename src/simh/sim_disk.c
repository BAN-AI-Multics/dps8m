/*
 * sim_disk.c: simulator disk support library
 *
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: b2c7f6c3-f62a-11ec-9f60-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2011 Mark Pizzolato
 * Copyright (c) 2021-2024 The DPS8M Development Team
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
 * Except as contained in this notice, the names of Mark Pizzolato shall not
 * be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from Mark
 * Pizzolato.
 *
 * ---------------------------------------------------------------------------
 */

/*
 * Public routines:
 *
 * sim_disk_attach           attach disk unit
 * sim_disk_detach           detach disk unit
 * sim_disk_attach_help      help routine for attaching disks
 * sim_disk_rdsect           read disk sectors
 * sim_disk_rdsect_a         read disk sectors asynchronously
 * sim_disk_wrsect           write disk sectors
 * sim_disk_wrsect_a         write disk sectors asynchronously
 * sim_disk_unload           unload or detach a disk as needed
 * sim_disk_reset            reset unit
 * sim_disk_wrp              TRUE if write protected
 * sim_disk_isavailable      TRUE if available for I/O
 * sim_disk_size             get disk size
 * sim_disk_set_fmt          set disk format
 * sim_disk_show_fmt         show disk format
 * sim_disk_set_capac        set disk capacity
 * sim_disk_show_capac       show disk capacity
 * sim_disk_data_trace       debug support
 */

#define _FILE_OFFSET_BITS 64    /* Set 64-bit file offset for I/O operations */

#include "sim_defs.h"
#include "sim_disk.h"

#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>

#include "../decNumber/decContext.h"
#include "../decNumber/decNumberLocal.h"

#if defined(_WIN32)
# include <windows.h>
#endif /* if defined(_WIN32) */

#if !defined(DECLITEND)
# error Unknown platform endianness
#endif /* if !defined(DECLITEND) */

#if defined(FREE)
# undef FREE
#endif /* if defined(FREE) */
#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

struct disk_context {
    DEVICE              *dptr;              /* Device for unit (access to debug flags) */
    uint32              dbit;               /* debugging bit */
    uint32              sector_size;        /* Disk Sector Size (of the pseudo disk) */
    uint32              capac_factor;       /* Units of Capacity (2 = word, 1 = byte) */
    uint32              xfer_element_size;  /* Disk Bus Transfer size (1 - byte, 2 - word, 4 - longword) */
    uint32              storage_sector_size;/* Sector size of the containing storage */
    uint32              removable;          /* Removable device flag */
    uint32              auto_format;        /* Format determined dynamically */
    };

#define disk_ctx up8                        /* Field in Unit structure which points to the disk_context */

/* Forward declarations */

struct sim_disk_fmt {
    const char          *name;                          /* name */
    int32               uflags;                         /* unit flags */
    int32               fmtval;                         /* Format type value */
    t_stat              (*impl_fnc)(void);              /* Implemented Test Function */
    };

static struct sim_disk_fmt fmts[DKUF_N_FMT] = {
    { "SIMH", 0, DKUF_F_STD, NULL},
    { NULL,   0, 0,          0   } };

/* Set disk format */

t_stat sim_disk_set_fmt (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
uint32 f;

if (uptr == NULL)
    return SCPE_IERR;
if (cptr == NULL)
    return SCPE_ARG;
for (f = 0; f < DKUF_N_FMT && fmts[f].name; f++) {
    if (fmts[f].name && (strcmp (cptr, fmts[f].name) == 0)) {
        if ((fmts[f].impl_fnc) && (fmts[f].impl_fnc() != SCPE_OK))
            return SCPE_NOFNC;
        uptr->flags = (uptr->flags & ~DKUF_FMT) |
            (fmts[f].fmtval << DKUF_V_FMT) | fmts[f].uflags;
        return SCPE_OK;
        }
    }
return SCPE_ARG;
}

/* Show disk format */

t_stat sim_disk_show_fmt (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
int32 f = DK_GET_FMT (uptr);
size_t i;

for (i = 0; i < DKUF_N_FMT; i++)
    if (fmts[i].fmtval == f) {
        fprintf (st, "%s format", fmts[i].name);
        return SCPE_OK;
        }
fprintf (st, "invalid format");
return SCPE_OK;
}

/* Set disk capacity */

t_stat sim_disk_set_capac (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
t_offset cap;
t_stat r;
DEVICE *dptr = find_dev_from_unit (uptr);

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_ARG;
if (uptr->flags & UNIT_ATT)
    return SCPE_ALATT;
cap = (t_offset) get_uint (cptr, 10, sim_taddr_64? 2000000: 2000, &r);
if (r != SCPE_OK)
    return SCPE_ARG;
uptr->capac = (t_addr)((cap * ((t_offset) 1000000))/((dptr->flags & DEV_SECTORS) ? 512 : 1));
return SCPE_OK;
}

/* Show disk capacity */

t_stat sim_disk_show_capac (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
const char *cap_units = "B";
DEVICE *dptr = find_dev_from_unit (uptr);
t_offset capac = ((t_offset)uptr->capac)*((dptr->flags & DEV_SECTORS) ? 512 : 1);

if ((dptr->dwidth / dptr->aincr) == 16)
    cap_units = "W";
if (capac) {
    if (capac >= (t_offset) 1000000)
        fprintf (st, "capacity=%luM%s", (unsigned long) (capac / ((t_offset) 1000000)), cap_units);
    else if (uptr->capac >= (t_addr) 1000)
        fprintf (st, "capacity=%luK%s", (unsigned long) (capac / ((t_offset) 1000)), cap_units);
    else fprintf (st, "capacity=%lu%s", (unsigned long) capac, cap_units);
    }
else fprintf (st, "undefined capacity");
return SCPE_OK;
}

/* Test for available */

t_bool sim_disk_isavailable (UNIT *uptr)
{
if (!(uptr->flags & UNIT_ATT))                          /* attached? */
    return FALSE;
switch (DK_GET_FMT (uptr)) {                            /* case on format */
    case DKUF_F_STD:                                    /* SIMH format */
        return TRUE;
        /*NOTREACHED*/ /* unreachable */
        break;
    default:
        return FALSE;
    }
}

t_bool sim_disk_isavailable_a (UNIT *uptr, DISK_PCALLBACK callback)
{
t_bool r = FALSE;
    r = sim_disk_isavailable (uptr);
return r;
}

/* Test for write protect */

t_bool sim_disk_wrp (UNIT *uptr)
{
return (uptr->flags & DKUF_WRP)? TRUE: FALSE;
}

/* Get Disk size */

t_offset sim_disk_size (UNIT *uptr)
{
switch (DK_GET_FMT (uptr)) {                            /* case on format */
    case DKUF_F_STD:                                    /* SIMH format */
        return sim_fsize_ex (uptr->fileref);
        /*NOTREACHED*/ /* unreachable */
        break;
    default:
        return (t_offset)-1;
    }
}

/* Read Sectors */

static t_stat _sim_disk_rdsect (UNIT *uptr, t_lba lba, uint8 *buf, t_seccnt *sectsread, t_seccnt sects)
{
t_offset da;
uint32 err, tbc;
size_t i;
struct disk_context *ctx = (struct disk_context *)uptr->disk_ctx;

sim_debug (ctx->dbit, ctx->dptr, "_sim_disk_rdsect(unit=%lu, lba=0x%X, sects=%lu)\n",
           (unsigned long)(uptr-ctx->dptr->units), lba, (unsigned long)sects);

da = ((t_offset)lba) * ctx->sector_size;
tbc = sects * ctx->sector_size;
if (sectsread)
    *sectsread = 0;
err = sim_fseeko (uptr->fileref, da, SEEK_SET);          /* set pos */
if (!err) {
    i = sim_fread (buf, ctx->xfer_element_size, tbc/ctx->xfer_element_size, uptr->fileref);
    if (i < tbc/ctx->xfer_element_size)                 /* fill */
        (void)memset (&buf[i*ctx->xfer_element_size], 0, tbc-(i*ctx->xfer_element_size));
    err = ferror (uptr->fileref);
    if ((!err) && (sectsread))
        *sectsread = (t_seccnt)((i*ctx->xfer_element_size+ctx->sector_size-1)/ctx->sector_size);
    }
return err;
}

t_stat sim_disk_rdsect (UNIT *uptr, t_lba lba, uint8 *buf, t_seccnt *sectsread, t_seccnt sects)
{
t_stat r;
struct disk_context *ctx = (struct disk_context *)uptr->disk_ctx;
t_seccnt sread = 0;

sim_debug (ctx->dbit, ctx->dptr, "sim_disk_rdsect(unit=%lu, lba=0x%X, sects=%lu)\n",
           (unsigned long)(uptr-ctx->dptr->units), lba, (unsigned long)sects);

if ((sects == 1) &&                                    /* Single sector reads beyond the end of the disk */
    (lba >= (uptr->capac*ctx->capac_factor)/(ctx->sector_size/((ctx->dptr->flags & DEV_SECTORS) ? 512 : 1)))) {
    (void)memset (buf, '\0', ctx->sector_size);        /* are bad block management efforts - zero buffer */
    if (sectsread)
        *sectsread = 1;
    return SCPE_OK;                                     /* return success */
    }

if ((0 == (ctx->sector_size & (ctx->storage_sector_size - 1))) ||   /* Sector Aligned & whole sector transfers */
    ((0 == ((lba*ctx->sector_size) & (ctx->storage_sector_size - 1))) &&
     (0 == ((sects*ctx->sector_size) & (ctx->storage_sector_size - 1))))) {
    switch (DK_GET_FMT (uptr)) {                        /* case on format */
        case DKUF_F_STD:                                /* SIMH format */
            return _sim_disk_rdsect (uptr, lba, buf, sectsread, sects);
            /*NOTREACHED*/ /* unreachable */
            break;
        default:
            return SCPE_NOFNC;
        }
//    if (sectsread)
//        *sectsread = sread;
//    if (r != SCPE_OK)
//        return r;
//    sim_buf_swap_data (buf, ctx->xfer_element_size, (sread * ctx->sector_size) / ctx->xfer_element_size);
//    return r;
    }
else { /* Unaligned and/or partial sector transfers */
    uint8 *tbuf = (uint8*) malloc (sects*ctx->sector_size + 2*ctx->storage_sector_size);
    t_lba sspsts = ctx->storage_sector_size/ctx->sector_size; /* sim sectors in a storage sector */
    t_lba tlba = lba & ~(sspsts - 1);
    t_seccnt tsects = sects + (lba - tlba);

    tsects = (tsects + (sspsts - 1)) & ~(sspsts - 1);
    if (sectsread)
        *sectsread = 0;
    if (tbuf == NULL)
        return SCPE_MEM;
    switch (DK_GET_FMT (uptr)) {                        /* case on format */
        case DKUF_F_STD:                                /* SIMH format */
            r = _sim_disk_rdsect (uptr, tlba, tbuf, &sread, tsects);
            break;
        default:
            FREE (tbuf);
            return SCPE_NOFNC;
        }
    if (r == SCPE_OK) {
        memcpy (buf, tbuf + ((lba - tlba) * ctx->sector_size), sects * ctx->sector_size);
        if (sectsread) {
            *sectsread = sread - (lba - tlba);
            if (*sectsread > sects)
                *sectsread = sects;
            }
        }
    FREE (tbuf);
    return r;
    }
}

t_stat sim_disk_rdsect_a (UNIT *uptr, t_lba lba, uint8 *buf, t_seccnt *sectsread, t_seccnt sects, DISK_PCALLBACK callback)
{
t_stat r = SCPE_OK;
    r = sim_disk_rdsect (uptr, lba, buf, sectsread, sects);
return r;
}

/* Write Sectors */

static t_stat _sim_disk_wrsect (UNIT *uptr, t_lba lba, uint8 *buf, t_seccnt *sectswritten, t_seccnt sects)
{
t_offset da;
uint32 err, tbc;
size_t i;
struct disk_context *ctx = (struct disk_context *)uptr->disk_ctx;

sim_debug (ctx->dbit, ctx->dptr, "_sim_disk_wrsect(unit=%lu, lba=0x%X, sects=%lu)\n",
           (unsigned long)(uptr-ctx->dptr->units), lba, (unsigned long)sects);

da = ((t_offset)lba) * ctx->sector_size;
tbc = sects * ctx->sector_size;
if (sectswritten)
    *sectswritten = 0;
err = sim_fseeko (uptr->fileref, da, SEEK_SET);          /* set pos */
if (!err) {
    i = sim_fwrite (buf, ctx->xfer_element_size, tbc/ctx->xfer_element_size, uptr->fileref);
    err = ferror (uptr->fileref);
    if ((!err) && (sectswritten))
        *sectswritten = (t_seccnt)((i*ctx->xfer_element_size+ctx->sector_size-1)/ctx->sector_size);
    }
return err;
}

t_stat sim_disk_wrsect (UNIT *uptr, t_lba lba, uint8 *buf, t_seccnt *sectswritten, t_seccnt sects)
{
struct disk_context *ctx = (struct disk_context *)uptr->disk_ctx;
uint32 f = DK_GET_FMT (uptr);
t_stat r;
uint8 *tbuf = NULL;

sim_debug (ctx->dbit, ctx->dptr, "sim_disk_wrsect(unit=%lu, lba=0x%X, sects=%lu)\n",
           (unsigned long)(uptr-ctx->dptr->units), lba, (unsigned long)sects);

if (uptr->dynflags & UNIT_DISK_CHK) {
    DEVICE *dptr = find_dev_from_unit (uptr);
    uint32 capac_factor = ((dptr->dwidth / dptr->aincr) == 16) ? 2 : 1; /* capacity units (word: 2, byte: 1) */
    t_lba total_sectors = (t_lba)((uptr->capac*capac_factor)/(ctx->sector_size/((dptr->flags & DEV_SECTORS) ? 512 : 1)));
    t_lba sect;

    for (sect = 0; sect < sects; sect++) {
        t_lba offset;
        t_bool sect_error = FALSE;

        for (offset = 0; offset < ctx->sector_size; offset += sizeof(uint32)) {
            if (*((uint32 *)&buf[sect*ctx->sector_size + offset]) != (uint32)(lba + sect)) {
                sect_error = TRUE;
                break;
                }
            }
        if (sect_error) {
            uint32 save_dctrl = dptr->dctrl;
            FILE *save_sim_deb = sim_deb;

            sim_printf ("\n%s%lu: Write Address Verification Error on lbn %lu(0x%X) of %lu(0x%X).\n",
                        sim_dname (dptr), (unsigned long)(uptr-dptr->units),
                        (unsigned long)((unsigned long)lba+(unsigned long)sect),
                        (int)((int)lba+(int)sect), (unsigned long)total_sectors, (int)total_sectors);
            dptr->dctrl = 0xFFFFFFFF;
            sim_deb = save_sim_deb ? save_sim_deb : stdout;
            sim_disk_data_trace (uptr, buf+sect*ctx->sector_size, lba+sect, ctx->sector_size,    "Found", TRUE, 1);
            dptr->dctrl = save_dctrl;
            sim_deb = save_sim_deb;
            }
        }
    }
if (f == DKUF_F_STD)
    return _sim_disk_wrsect (uptr, lba, buf, sectswritten, sects);
if ((0 == (ctx->sector_size & (ctx->storage_sector_size - 1))) ||   /* Sector Aligned & whole sector transfers */
    ((0 == ((lba*ctx->sector_size) & (ctx->storage_sector_size - 1))) &&
     (0 == ((sects*ctx->sector_size) & (ctx->storage_sector_size - 1))))) {
        sim_end = DECLITEND;
        if (sim_end || (ctx->xfer_element_size == sizeof (char)))
            switch (DK_GET_FMT (uptr)) {                            /* case on format */
                default:
                    return SCPE_NOFNC;
            }

        tbuf = (uint8*) malloc (sects * ctx->sector_size);
        if (NULL == tbuf)
            return SCPE_MEM;
        sim_buf_copy_swapped (tbuf, buf, ctx->xfer_element_size, (sects * ctx->sector_size) / ctx->xfer_element_size);

        switch (DK_GET_FMT (uptr)) {                            /* case on format */
            default:
                r = SCPE_NOFNC;
                break;
            }
        }
else { /* Unaligned and/or partial sector transfers */
    t_lba sspsts = ctx->storage_sector_size/ctx->sector_size; /* sim sectors in a storage sector */
    t_lba tlba = lba & ~(sspsts - 1);
    t_seccnt tsects = sects + (lba - tlba);

    tbuf = (uint8*) malloc (sects*ctx->sector_size + 2*ctx->storage_sector_size);
    tsects = (tsects + (sspsts - 1)) & ~(sspsts - 1);
    if (sectswritten)
        *sectswritten = 0;
    if (tbuf == NULL)
        return SCPE_MEM;
    /* Partial Sector writes require a read-modify-write sequence for the partial sectors */
    if ((lba & (sspsts - 1)) ||
        (sects < sspsts))
        switch (DK_GET_FMT (uptr)) {                            /* case on format */
            default:
                r = SCPE_NOFNC;
                break;
            }
    if ((tsects > sspsts) &&
        ((sects + lba - tlba) & (sspsts - 1)))
        switch (DK_GET_FMT (uptr)) {                            /* case on format */
            default:
                r = SCPE_NOFNC;
                break;
            }
    sim_buf_copy_swapped (tbuf + (lba & (sspsts - 1)) * ctx->sector_size,
                          buf, ctx->xfer_element_size, (sects * ctx->sector_size) / ctx->xfer_element_size);
    switch (DK_GET_FMT (uptr)) {                            /* case on format */
        default:
            r = SCPE_NOFNC;
            break;
        }
    if ((r == SCPE_OK) && sectswritten) { //-V560
        *sectswritten -= (lba - tlba);
        if (*sectswritten > sects)
            *sectswritten = sects;
        }
    }
FREE (tbuf);
return r;
}

t_stat sim_disk_wrsect_a (UNIT *uptr, t_lba lba, uint8 *buf, t_seccnt *sectswritten, t_seccnt sects, DISK_PCALLBACK callback)
{
t_stat r = SCPE_OK;
    r =  sim_disk_wrsect (uptr, lba, buf, sectswritten, sects);
return r;
}

t_stat sim_disk_unload (UNIT *uptr)
{
switch (DK_GET_FMT (uptr)) {                            /* case on format */
    case DKUF_F_STD:                                    /* Simh */
        return sim_disk_detach (uptr);
    default:
        return SCPE_NOFNC;
    }
}

static void _sim_disk_io_flush (UNIT *uptr)
{
uint32 f = DK_GET_FMT (uptr);

switch (f) {                                            /* case on format */
    case DKUF_F_STD:                                    /* Simh */
        (void)fflush (uptr->fileref);
        break;
        }
}

static t_stat _err_return (UNIT *uptr, t_stat stat)
{
FREE (uptr->filename);
uptr->filename = NULL;
FREE (uptr->disk_ctx);
uptr->disk_ctx = NULL;
return stat;
}

#if defined(__xlc__)
# pragma pack(1)
#else
# pragma pack(push,1)
#endif
typedef struct _ODS2_HomeBlock
    {
    uint32 hm2_l_homelbn;
    uint32 hm2_l_alhomelbn;
    uint32 hm2_l_altidxlbn;
    uint8  hm2_b_strucver;
    uint8  hm2_b_struclev;
    uint16 hm2_w_cluster;
    uint16 hm2_w_homevbn;
    uint16 hm2_w_alhomevbn;
    uint16 hm2_w_altidxvbn;
    uint16 hm2_w_ibmapvbn;
    uint32 hm2_l_ibmaplbn;
    uint32 hm2_l_maxfiles;
    uint16 hm2_w_ibmapsize;
    uint16 hm2_w_resfiles;
    uint16 hm2_w_devtype;
    uint16 hm2_w_rvn;
    uint16 hm2_w_setcount;
    uint16 hm2_w_volchar;
    uint32 hm2_l_volowner;
    uint32 hm2_l_reserved;
    uint16 hm2_w_protect;
    uint16 hm2_w_fileprot;
    uint16 hm2_w_reserved;
    uint16 hm2_w_checksum1;
    uint32 hm2_q_credate[2];
    uint8  hm2_b_window;
    uint8  hm2_b_lru_lim;
    uint16 hm2_w_extend;
    uint32 hm2_q_retainmin[2];
    uint32 hm2_q_retainmax[2];
    uint32 hm2_q_revdate[2];
    uint8  hm2_r_min_class[20];
    uint8  hm2_r_max_class[20];
    uint8  hm2_r_reserved[320];
    uint32 hm2_l_serialnum;
    uint8  hm2_t_strucname[12];
    uint8  hm2_t_volname[12];
    uint8  hm2_t_ownername[12];
    uint8  hm2_t_format[12];
    uint16 hm2_w_reserved2;
    uint16 hm2_w_checksum2;
    } ODS2_HomeBlock;

typedef struct _ODS2_FileHeader
    {
    uint8  fh2_b_idoffset;
    uint8  fh2_b_mpoffset;
    uint8  fh2_b_acoffset;
    uint8  fh2_b_rsoffset;
    uint16 fh2_w_seg_num;
    uint16 fh2_w_structlev;
    uint16 fh2_w_fid[3];
    uint16 fh2_w_ext_fid[3];
    uint16 fh2_w_recattr[16];
    uint32 fh2_l_filechar;
    uint16 fh2_w_remaining[228];
    } ODS2_FileHeader;

typedef union _ODS2_Retreval
    {
        struct
            {
            unsigned fm2___fill   : 14;       /* type specific data               */
            unsigned fm2_v_format : 2;        /* format type code                 */
            } fm2_r_word0_bits;
        struct
            {
            unsigned fm2_v_exact    : 1;      /* exact placement specified        */
            unsigned fm2_v_oncyl    : 1;      /* on cylinder allocation desired   */
            unsigned fm2___fill     : 10;
            unsigned fm2_v_lbn      : 1;      /* use LBN of next map pointer      */
            unsigned fm2_v_rvn      : 1;      /* place on specified RVN           */
            unsigned fm2_v_format0  : 2;
            } fm2_r_map_bits0;
        struct
            {
            unsigned fm2_b_count1   : 8;      /* low byte described below         */
            unsigned fm2_v_highlbn1 : 6;      /* high order LBN                   */
            unsigned fm2_v_format1  : 2;
            unsigned fm2_w_lowlbn1  : 16;     /* low order LBN                    */
            } fm2_r_map_bits1;
        struct
            {
            struct
                {
                unsigned fm2_v_count2   : 14; /* count field                      */
                unsigned fm2_v_format2  : 2;
                unsigned fm2_l_lowlbn2  : 16; /* low order LBN                    */
                } fm2_r_map2_long0;
            uint16 fm2_l_highlbn2;            /* high order LBN                   */
            } fm2_r_map_bits2;
        struct
            {
            struct
                {
                unsigned fm2_v_highcount3 : 14; /* low order count field          */
                unsigned fm2_v_format3  : 2;
                unsigned fm2_w_lowcount3 : 16;  /* high order count field         */
                } fm2_r_map3_long0;
            uint32 fm2_l_lbn3;
            } fm2_r_map_bits3;
    } ODS2_Retreval;

typedef struct _ODS2_StorageControlBlock
    {
    uint8  scb_b_strucver;   /* 1 */
    uint8  scb_b_struclev;   /* 2 */
    uint16 scb_w_cluster;
    uint32 scb_l_volsize;
    uint32 scb_l_blksize;
    uint32 scb_l_sectors;
    uint32 scb_l_tracks;
    uint32 scb_l_cylinder;
    uint32 scb_l_status;
    uint32 scb_l_status2;
    uint16 scb_w_writecnt;
    uint8  scb_t_volockname[12];
    uint32 scb_q_mounttime[2];
    uint16 scb_w_backrev;
    uint32 scb_q_genernum[2];
    uint8  scb_b_reserved[446];
    uint16 scb_w_checksum;
    } ODS2_SCB;
#if defined(__xlc__)
# pragma pack(reset)
#else
# pragma pack(pop)
#endif

static uint16
ODS2Checksum (void *Buffer, uint16 WordCount)
    {
    int i;
    uint16 CheckSum = 0;
    uint16 *Buf = (uint16 *)Buffer;

    for (i=0; i<WordCount; i++)
        CheckSum += Buf[i];
    return CheckSum;
    }

static t_offset get_filesystem_size (UNIT *uptr)
{
DEVICE *dptr;
t_addr saved_capac;
t_offset temp_capac = 512 * (t_offset)0xFFFFFFFFu;  /* Make sure we can access the largest sector */
uint32 capac_factor;
ODS2_HomeBlock Home;
ODS2_FileHeader Header;
ODS2_Retreval *Retr;
ODS2_SCB Scb;
uint16 CheckSum1, CheckSum2;
uint32 ScbLbn = 0;
t_offset ret_val = (t_offset)-1;

if ((dptr = find_dev_from_unit (uptr)) == NULL)
    return ret_val;
capac_factor = ((dptr->dwidth / dptr->aincr) == 16) ? 2 : 1; /* save capacity units (word: 2, byte: 1) */
saved_capac = uptr->capac;
/* cppcheck-suppress signConversion */
uptr->capac = (t_addr)(temp_capac/(capac_factor*((dptr->flags & DEV_SECTORS) ? 512 : 1)));
if (sim_disk_rdsect (uptr, 1, (uint8 *)&Home, NULL, 1))
    goto Return_Cleanup;
/* cppcheck-suppress comparePointers */
CheckSum1 = ODS2Checksum (&Home, (uint16)((((char *)&Home.hm2_w_checksum1)-((char *)&Home.hm2_l_homelbn))/2));
/* cppcheck-suppress comparePointers */
CheckSum2 = ODS2Checksum (&Home, (uint16)((((char *)&Home.hm2_w_checksum2)-((char *)&Home.hm2_l_homelbn))/2));
if ((Home.hm2_l_homelbn   == 0)  ||
    (Home.hm2_l_alhomelbn == 0)  ||
    (Home.hm2_l_altidxlbn == 0)  ||
   ((Home.hm2_b_struclev  != 2)  &&
    (Home.hm2_b_struclev  != 5)) ||
    (Home.hm2_b_strucver  == 0)  ||
    (Home.hm2_w_cluster   == 0)  ||
    (Home.hm2_w_homevbn   == 0)  ||
    (Home.hm2_w_alhomevbn == 0)  ||
    (Home.hm2_w_ibmapvbn  == 0)  ||
    (Home.hm2_l_ibmaplbn  == 0)  ||
    (Home.hm2_w_resfiles  >= Home.hm2_l_maxfiles) ||
    (Home.hm2_w_ibmapsize == 0)  ||
    (Home.hm2_w_resfiles   < 5)  ||
    (Home.hm2_w_checksum1 != CheckSum1) ||
    (Home.hm2_w_checksum2 != CheckSum2))
    goto Return_Cleanup;
if (sim_disk_rdsect (uptr, Home.hm2_l_ibmaplbn+Home.hm2_w_ibmapsize+1, (uint8 *)&Header, NULL, 1))
    goto Return_Cleanup;
CheckSum1 = ODS2Checksum (&Header, 255);
if (CheckSum1 != *(((uint16 *)&Header)+255)) //-V1032 /* Verify Checksum on BITMAP.SYS file header */
    goto Return_Cleanup;
Retr = (ODS2_Retreval *)(((uint16*)(&Header))+Header.fh2_b_mpoffset);
/* The BitMap File has a single extent, which may be preceded by a placement descriptor */
if (Retr->fm2_r_word0_bits.fm2_v_format == 0)
    Retr = (ODS2_Retreval *)(((uint16 *)Retr)+1); //-V1032 /* skip placement descriptor */
switch (Retr->fm2_r_word0_bits.fm2_v_format)
    {
    case 1:
        ScbLbn = (Retr->fm2_r_map_bits1.fm2_v_highlbn1<<16)+Retr->fm2_r_map_bits1.fm2_w_lowlbn1;
        break;
    case 2:
        ScbLbn = (Retr->fm2_r_map_bits2.fm2_l_highlbn2<<16)+Retr->fm2_r_map_bits2.fm2_r_map2_long0.fm2_l_lowlbn2;
        break;
    case 3:
        ScbLbn = Retr->fm2_r_map_bits3.fm2_l_lbn3;
        break;
    }
Retr = (ODS2_Retreval *)(((uint16 *)Retr)+Retr->fm2_r_word0_bits.fm2_v_format+1);
if (sim_disk_rdsect (uptr, ScbLbn, (uint8 *)&Scb, NULL, 1))
    goto Return_Cleanup;
CheckSum1 = ODS2Checksum (&Scb, 255);
if (CheckSum1 != *(((uint16 *)&Scb)+255)) //-V1032 /* Verify Checksum on Storage Control Block */
    goto Return_Cleanup;
if ((Scb.scb_w_cluster != Home.hm2_w_cluster) ||
    (Scb.scb_b_strucver != Home.hm2_b_strucver) ||
    (Scb.scb_b_struclev != Home.hm2_b_struclev))
    goto Return_Cleanup;
if (!sim_quiet) {
    sim_printf ("%s%lu: '%s' Contains ODS%lu File system:\n",
                sim_dname (dptr), (unsigned long)(uptr-dptr->units), uptr->filename, (unsigned long)Home.hm2_b_struclev);
    sim_printf ("%s%lu: Volume Name: %12.12s ",
                sim_dname (dptr), (unsigned long)(uptr-dptr->units), Home.hm2_t_volname);
    sim_printf ("Format: %12.12s ",
                Home.hm2_t_format);
    sim_printf ("SectorsInVolume: %lu\n",
                (unsigned long)Scb.scb_l_volsize);
    }
ret_val = ((t_offset)Scb.scb_l_volsize) * 512;

Return_Cleanup:
uptr->capac = saved_capac;
return ret_val;
}

t_stat sim_disk_attach (UNIT *uptr, const char *cptr, size_t sector_size, size_t xfer_element_size, t_bool dontautosize,
                        uint32 dbit, const char *dtype, uint32 pdp11tracksize, int completion_delay)
{
struct disk_context *ctx;
DEVICE *dptr;
FILE *(*open_function)(const char *filename, const char *mode) = sim_fopen;
FILE *(*create_function)(const char *filename, t_offset desiredsize) = NULL;
t_offset (*size_function)(FILE *file) = NULL;
t_stat (*storage_function)(FILE *file, uint32 *sector_size, uint32 *removable) = NULL;
t_bool created = FALSE, copied = FALSE;
t_bool auto_format = FALSE;
t_offset capac, filesystem_capac;

if (uptr->flags & UNIT_DIS)                             /* disabled? */
    return SCPE_UDIS;
if (!(uptr->flags & UNIT_ATTABLE))                      /* not attachable? */
    return SCPE_NOATT;
if ((dptr = find_dev_from_unit (uptr)) == NULL)
    return SCPE_NOATT;
if (sim_switches & SWMASK ('F')) {                      /* format spec? */
    char gbuf[CBUFSIZE];
    cptr = get_glyph (cptr, gbuf, 0);                   /* get spec */
    if (*cptr == 0)                                     /* must be more */
        return SCPE_2FARG;
    if (sim_disk_set_fmt (uptr, 0, gbuf, NULL) != SCPE_OK)
        return sim_messagef (SCPE_ARG, "Invalid Disk Format: %s\n", gbuf);
    sim_switches = sim_switches & ~(SWMASK ('F'));      /* Record Format specifier already processed */
    auto_format = TRUE;
    }
open_function = sim_fopen;
size_function = sim_fsize_ex;
uptr->filename = (char *) calloc (CBUFSIZE, sizeof (char));/* alloc name buf */
uptr->disk_ctx = ctx = (struct disk_context *)calloc(1, sizeof(struct disk_context));
if (!ctx)
{
  fprintf(stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
          __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
  (void)raise(SIGUSR2);
  /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
  abort();
}
if ((uptr->filename == NULL) || (uptr->disk_ctx == NULL))
    return _err_return (uptr, SCPE_MEM);
#if defined(__GNUC__)
# if !defined(__clang_version__)
#  if __GNUC__ > 7
#   if !defined(__INTEL_COMPILER)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-truncation"
#   endif /* if !defined(__INTEL_COMPILER) */
#  endif /* if __GNUC__ > 7 */
# endif /* if !defined(__clang_version__) */
#endif /* if defined(__GNUC__) */
strncpy (uptr->filename, cptr, CBUFSIZE);               /* save name */
#if defined(__GNUC__)
# if !defined(__clang_version__)
#  if __GNUC__ > 7
#   if !defined(__INTEL_COMPILER)
#    pragma GCC diagnostic pop
#   endif /* if !defined(__INTEL_COMPILER) */
#  endif /* if __GNUC__ > 7 */
# endif /* if !defined(__clang_version__) */
#endif /* if defined(__GNUC__) */
ctx->sector_size = (uint32)sector_size;                 /* save sector_size */
ctx->capac_factor = ((dptr->dwidth / dptr->aincr) == 16) ? 2 : 1; /* save capacity units (word: 2, byte: 1) */
ctx->xfer_element_size = (uint32)xfer_element_size;     /* save xfer_element_size */
ctx->dptr = dptr;                                       /* save DEVICE pointer */
ctx->dbit = dbit;                                       /* save debug bit */
sim_debug (ctx->dbit, ctx->dptr, "sim_disk_attach(unit=%lu,filename='%s')\n",
           (unsigned long)(uptr-ctx->dptr->units), uptr->filename);
ctx->auto_format = auto_format;                         /* save that we auto selected format */
ctx->storage_sector_size = (uint32)sector_size;         /* Default */
if ((sim_switches & SWMASK ('R')) ||                    /* read only? */
    ((uptr->flags & UNIT_RO) != 0)) {
    if (((uptr->flags & UNIT_ROABLE) == 0) &&           /* allowed? */
        ((uptr->flags & UNIT_RO) == 0))
        return _err_return (uptr, SCPE_NORO);           /* no, error */
    uptr->fileref = open_function (cptr, "rb");         /* open rd only */
    if (uptr->fileref == NULL)                          /* open fail? */
        return _err_return (uptr, SCPE_OPENERR);        /* yes, error */
    uptr->flags = uptr->flags | UNIT_RO;                /* set rd only */
    if (!sim_quiet) {
        sim_printf ("%s%lu: unit is read only (%s)\n", sim_dname (dptr),
                    (unsigned long)(uptr-dptr->units), cptr);
        }
    }
else {                                                  /* normal */
    uptr->fileref = open_function (cptr, "rb+");        /* open r/w */
    if (uptr->fileref == NULL) {                        /* open fail? */
        if ((errno == EROFS) || (errno == EACCES)) {    /* read only? */
            if ((uptr->flags & UNIT_ROABLE) == 0)       /* allowed? */
                return _err_return (uptr, SCPE_NORO);   /* no error */
            uptr->fileref = open_function (cptr, "rb"); /* open rd only */
            if (uptr->fileref == NULL)                  /* open fail? */
                return _err_return (uptr, SCPE_OPENERR);/* yes, error */
            uptr->flags = uptr->flags | UNIT_RO;        /* set rd only */
            if (!sim_quiet)
                sim_printf ("%s%lu: unit is read only (%s)\n", sim_dname (dptr),
                            (unsigned long)(uptr-dptr->units), cptr);
            }
        else {                                          /* doesn't exist */
            if (sim_switches & SWMASK ('E'))            /* must exist? */
                return _err_return (uptr, SCPE_OPENERR);/* yes, error */
            if (create_function) //-V547 /* cppcheck-suppress internalAstError */
                uptr->fileref = create_function (cptr,  /* create new file */
                                                 ((t_offset)uptr->capac)*ctx->capac_factor*((dptr->flags & DEV_SECTORS) ? 512 : 1));
            else
                uptr->fileref = open_function (cptr, "wb+");/* open new file */
            if (uptr->fileref == NULL)                  /* open fail? */
                return _err_return (uptr, SCPE_OPENERR);/* yes, error */
            if (!sim_quiet)
                sim_printf ("%s%lu: creating new file (%s)\n", sim_dname (dptr),
                            (unsigned long)(uptr-dptr->units), cptr);
            created = TRUE;
            }
        }                                               /* end if null */
    }                                                   /* end else */
uptr->flags = uptr->flags | UNIT_ATT;
uptr->pos = 0;

/* Get Device attributes if they are available */
if (storage_function) //-V547
    storage_function (uptr->fileref, &ctx->storage_sector_size, &ctx->removable);

if ((created) && (!copied)) { //-V560
    t_stat r = SCPE_OK;
    uint8 *secbuf = (uint8 *)calloc (1, ctx->sector_size);       /* alloc temp sector buf */
    if (secbuf == NULL)
        r = SCPE_MEM;
    if (r == SCPE_OK)
        r = sim_disk_wrsect (uptr,
                             (t_lba)(((((t_offset)uptr->capac)*ctx->capac_factor*((dptr->flags & DEV_SECTORS) ? 512 : 1)) - \
                              ctx->sector_size)/ctx->sector_size), secbuf, NULL, 1); /* Write Last Sector */
    if (r == SCPE_OK)
        r = sim_disk_wrsect (uptr, (t_lba)(0), secbuf, NULL, 1); /* Write First Sector */
    FREE (secbuf);
    if (r != SCPE_OK) {
        sim_disk_detach (uptr);                         /* report error now */
        remove (cptr);                                  /* remove the create file */
        return SCPE_OPENERR;
        }
    if (sim_switches & SWMASK ('I')) {                  /* Initialize To Sector Address */
        uint8 *init_buf = (uint8*) malloc (1024*1024);
        t_lba lba, sect;
        uint32 capac_factor = ((dptr->dwidth / dptr->aincr) == 16) ? 2 : 1; /* capacity units (word: 2, byte: 1) */
        t_seccnt sectors_per_buffer = (t_seccnt)((1024*1024)/sector_size);
        t_lba total_sectors = (t_lba)((uptr->capac*capac_factor)/(sector_size/((dptr->flags & DEV_SECTORS) ? 512 : 1)));
        t_seccnt sects = sectors_per_buffer;

        if (!init_buf) {
            sim_disk_detach (uptr);                         /* report error now */
            remove (cptr);
            return SCPE_MEM;
            }
        for (lba = 0; (lba < total_sectors) && (r == SCPE_OK); lba += sects) {
            sects = sectors_per_buffer;
            if (lba + sects > total_sectors)
                sects = total_sectors - lba;
            for (sect = 0; sect < sects; sect++) {
                t_lba offset;
                for (offset = 0; offset < sector_size; offset += sizeof(uint32))
                    *((uint32 *)&init_buf[sect*sector_size + offset]) = (uint32)(lba + sect);
                }
            r = sim_disk_wrsect (uptr, lba, init_buf, NULL, sects);
            if (r != SCPE_OK) {
                FREE (init_buf);
                sim_disk_detach (uptr);                         /* report error now */
                remove (cptr);                                  /* remove the create file */
                return SCPE_OPENERR;
                }
            if (!sim_quiet)
                sim_printf ("%s%lu: Initialized To Sector Address %luMB.  %lu%% complete.\r",
                            sim_dname (dptr), (unsigned long)(uptr-dptr->units),
                            (unsigned long)((((float)lba)*sector_size)/1000000),
                            (unsigned long)((((float)lba)*100)/total_sectors));
            }
        if (!sim_quiet)
            sim_printf ("%s%lu: Initialized To Sector Address %luMB.  100%% complete.\n",
                        sim_dname (dptr), (unsigned long)(uptr-dptr->units),
                        (unsigned long)((((float)lba)*sector_size)/1000000));
        FREE (init_buf);
        }
    }

if (sim_switches & SWMASK ('K')) {
    t_stat r = SCPE_OK;
    t_lba lba, sect;
    uint32 capac_factor = ((dptr->dwidth / dptr->aincr) == 16) ? 2 : 1; /* capacity units (word: 2, byte: 1) */
    t_seccnt sectors_per_buffer = (t_seccnt)((1024*1024)/sector_size);
    t_lba total_sectors = (t_lba)((uptr->capac*capac_factor)/(sector_size/((dptr->flags & DEV_SECTORS) ? 512 : 1)));
    t_seccnt sects = sectors_per_buffer;
    uint8 *verify_buf = (uint8*) malloc (1024*1024);

    if (!verify_buf) {
        sim_disk_detach (uptr);                         /* report error now */
        return SCPE_MEM;
        }
    for (lba = 0; (lba < total_sectors) && (r == SCPE_OK); lba += sects) {
        sects = sectors_per_buffer;
        if (lba + sects > total_sectors)
            sects = total_sectors - lba;
        r = sim_disk_rdsect (uptr, lba, verify_buf, NULL, sects);
        if (r == SCPE_OK) {
            for (sect = 0; sect < sects; sect++) {
                t_lba offset;
                t_bool sect_error = FALSE;

                for (offset = 0; offset < sector_size; offset += sizeof(uint32)) {
                    if (*((uint32 *)&verify_buf[sect*sector_size + offset]) != (uint32)(lba + sect)) {
                        sect_error = TRUE;
                        break;
                        }
                    }
                if (sect_error) {
                    uint32 save_dctrl = dptr->dctrl;
                    FILE *save_sim_deb = sim_deb;

                    sim_printf ("\n%s%lu: Verification Error on lbn %lu(0x%X) of %lu(0x%X).\n", sim_dname (dptr),
                                (unsigned long)(uptr-dptr->units),
                                (unsigned long)((unsigned long)lba+(unsigned long)sect),
                                (int)((int)lba+(int)sect),
                                (unsigned long)total_sectors,
                                (int)total_sectors);
                    dptr->dctrl = 0xFFFFFFFF;
                    sim_deb = stdout;
                    sim_disk_data_trace (uptr, verify_buf+sect*sector_size, lba+sect, sector_size,
                                         "Found", TRUE, 1);
                    dptr->dctrl = save_dctrl;
                    sim_deb = save_sim_deb;
                    }
                }
            }
        if (!sim_quiet)
            sim_printf ("%s%lu: Verified containing Sector Address %luMB.  %lu%% complete.\r",
                        sim_dname (dptr), (unsigned long)(uptr-dptr->units),
                        (unsigned long)((((float)lba)*sector_size)/1000000),
                        (unsigned long)((((float)lba)*100)/total_sectors));
        }
    if (!sim_quiet)
        sim_printf ("%s%lu: Verified containing Sector Address %luMB.  100%% complete.\n",
                    sim_dname (dptr), (unsigned long)(uptr-dptr->units),
                    (unsigned long)((((float)lba)*sector_size)/1000000));
    FREE (verify_buf);
    uptr->dynflags |= UNIT_DISK_CHK;
    }

filesystem_capac = get_filesystem_size (uptr);
capac = size_function (uptr->fileref);
if (capac && (capac != (t_offset)-1)) {
    if (dontautosize) {
        t_addr saved_capac = uptr->capac;

        if ((filesystem_capac != (t_offset)-1) &&
            (filesystem_capac > (((t_offset)uptr->capac)*ctx->capac_factor*((dptr->flags & DEV_SECTORS) ? 512 : 1)))) {
            if (!sim_quiet) {
                uptr->capac = (t_addr)(filesystem_capac/(ctx->capac_factor*((dptr->flags & DEV_SECTORS) ? 512 : 1)));
                sim_printf ("%s%lu: The file system on the disk %s is larger than simulated device (%s > ",
                            sim_dname (dptr), (unsigned long)(uptr-dptr->units), cptr, sprint_capac (dptr, uptr));
                uptr->capac = saved_capac;
                sim_printf ("%s)\n", sprint_capac (dptr, uptr));
                }
            sim_disk_detach (uptr);
            return SCPE_OPENERR;
            }
        if ((capac < (((t_offset)uptr->capac)*ctx->capac_factor*((dptr->flags & DEV_SECTORS) ? 512 : 1))) && \
            (DKUF_F_STD != DK_GET_FMT (uptr))) {
            if (!sim_quiet) {
                uptr->capac = (t_addr)(capac/(ctx->capac_factor*((dptr->flags & DEV_SECTORS) ? 512 : 1)));
                sim_printf ("%s%lu: non expandable disk %s is smaller than simulated device (%s < ",
                            sim_dname (dptr), (unsigned long)(uptr-dptr->units), cptr, sprint_capac (dptr, uptr));
                uptr->capac = saved_capac;
                sim_printf ("%s)\n", sprint_capac (dptr, uptr));
                }
            sim_disk_detach (uptr);
            return SCPE_OPENERR;
            }
        }
    else {
        if ((filesystem_capac != (t_offset)-1) &&
            (filesystem_capac > capac))
            capac = filesystem_capac;
        if ((capac > (((t_offset)uptr->capac)*ctx->capac_factor*((dptr->flags & DEV_SECTORS) ? 512 : 1))) ||
            (DKUF_F_STD != DK_GET_FMT (uptr)))
            uptr->capac = (t_addr)(capac/(ctx->capac_factor*((dptr->flags & DEV_SECTORS) ? 512 : 1)));
        }
    }

uptr->io_flush = _sim_disk_io_flush;

return SCPE_OK;
}

t_stat sim_disk_detach (UNIT *uptr)
{
struct disk_context *ctx;
int (*close_function)(FILE *f);
FILE *fileref;

if ((uptr == NULL) || !(uptr->flags & UNIT_ATT))
    return SCPE_NOTATT;

ctx = (struct disk_context *)uptr->disk_ctx;
fileref = uptr->fileref;

//sim_debug (ctx->dbit, ctx->dptr, "sim_disk_detach(unit=%lu,filename='%s')\n",
//           (unsigned long)(uptr-ctx->dptr->units), uptr->filename);

switch (DK_GET_FMT (uptr)) {                            /* case on format */
    case DKUF_F_STD:                                    /* Simh */
        close_function = fclose;
        break;
    default:
        return SCPE_IERR;
        }
if (!(uptr->flags & UNIT_ATTABLE))                      /* attachable? */
    return SCPE_NOATT;
if (!(uptr->flags & UNIT_ATT))                          /* attached? */
    return SCPE_OK;
if (NULL == find_dev_from_unit (uptr))
    return SCPE_OK;

if (uptr->io_flush)
    uptr->io_flush (uptr);                              /* flush buffered data */

uptr->flags &= ~(UNIT_ATT | UNIT_RO);
uptr->dynflags &= ~(UNIT_NO_FIO | UNIT_DISK_CHK);
FREE (uptr->filename);
uptr->filename = NULL;
uptr->fileref = NULL;
if (uptr->disk_ctx) {
  FREE (uptr->disk_ctx);
  uptr->disk_ctx = NULL;
}
uptr->io_flush = NULL;
if (ctx && ctx -> auto_format)
    sim_disk_set_fmt (uptr, 0, "SIMH", NULL);           /* restore file format */
if (close_function (fileref) == EOF)
    return SCPE_IOERR;
return SCPE_OK;
}

t_stat sim_disk_attach_help(FILE *st, DEVICE *dptr, const UNIT *uptr, int32 flag, const char *cptr)
{
fprintf (st, "%s Disk Attach Help\n\n", dptr->name);
fprintf (st, "Disk files are stored in the following format:\n\n");
fprintf (st, "    SIMH   An unstructured binary file of the size appropriate\n");
fprintf (st, "           for the disk drive being simulated.\n\n");

if (0 == (uptr-dptr->units)) {
    if (dptr->numunits > 1) {
        uint32 i;

        for (i=0; i < dptr->numunits; ++i)
            if (dptr->units[i].flags & UNIT_ATTABLE)
                fprintf (st, "  sim> ATTACH {switches} %s%lu diskfile\n", dptr->name, (unsigned long)i);
        }
    else
        fprintf (st, "  sim> ATTACH {switches} %s diskfile\n", dptr->name);
    }
else
    fprintf (st, "  sim> ATTACH {switches} %s diskfile\n\n", dptr->name);
fprintf (st, "\n%s attach command switches\n", dptr->name);
fprintf (st, "    -R          Attach Read Only.\n");
fprintf (st, "    -E          Must Exist (if not specified an attempt to create the indicated\n");
fprintf (st, "                disk container will be attempted).\n");
fprintf (st, "    -F          Open the indicated disk container in a specific format\n");
fprintf (st, "    -I          Initialize newly created disk so that each sector contains its\n");
fprintf (st, "                sector address\n");
fprintf (st, "    -K          Verify that the disk contents contain the sector address in each\n");
fprintf (st, "                sector.  Whole disk checked at attach time and each sector is\n");
fprintf (st, "                checked when written.\n");
fprintf (st, "    -V          Perform a verification pass to confirm successful data copy\n");
fprintf (st, "                operation.\n");
fprintf (st, "    -U          Fix inconsistencies which are overridden by the -O switch\n");
fprintf (st, "    -Y          Answer Yes to prompt to overwrite last track (on disk create)\n");
fprintf (st, "    -N          Answer No to prompt to overwrite last track (on disk create)\n");
return SCPE_OK;
}

t_stat sim_disk_reset (UNIT *uptr)
{
struct disk_context *ctx = (struct disk_context *)uptr->disk_ctx;

if (!(uptr->flags & UNIT_ATT))                          /* attached? */
    return SCPE_OK;

sim_debug (ctx->dbit, ctx->dptr, "sim_disk_reset(unit=%lu)\n", (unsigned long)(uptr-ctx->dptr->units));

_sim_disk_io_flush(uptr);
return SCPE_OK;
}

t_stat sim_disk_perror (UNIT *uptr, const char *msg)
{
int saved_errno = errno;

if (!(uptr->flags & UNIT_ATTABLE))                      /* not attachable? */
    return SCPE_NOATT;
switch (DK_GET_FMT (uptr)) {                            /* case on format */
    case DKUF_F_STD:                                    /* SIMH format */
        perror (msg);
        sim_printf ("%s %s: %s (Error %d)\r\n", sim_uname(uptr), msg, xstrerror_l(saved_errno), saved_errno);
    /*FALLTHRU*/ /* fall through */ /* fallthrough */
    default:
        ;
    }
return SCPE_OK;
}

t_stat sim_disk_clearerr (UNIT *uptr)
{
if (!(uptr->flags & UNIT_ATTABLE))                      /* not attachable? */
    return SCPE_NOATT;
switch (DK_GET_FMT (uptr)) {                            /* case on format */
    case DKUF_F_STD:                                    /* SIMH format */
        clearerr (uptr->fileref);
        break;
    default:
        ;
    }
return SCPE_OK;
}

void sim_disk_data_trace(UNIT *uptr, const uint8 *data, size_t lba, size_t len, const char* txt, int detail, uint32 reason)
{
struct disk_context *ctx = (struct disk_context *)uptr->disk_ctx;

if (sim_deb && (ctx->dptr->dctrl & reason)) {
    char pos[32];

    (void)sprintf (pos, "lbn: %08X ", (unsigned int)lba);
    sim_data_trace(ctx->dptr, uptr, (detail ? data : NULL), pos, len, txt, reason);
    }
}
