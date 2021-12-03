/*
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2017 Charles Anthony
 * Copyright (c) 2016 Michal Tomek
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

extern UNIT dia_unit [N_DIA_UNITS_MAX];
extern DEVICE dia_dev;

#define DIA_CLIENT_MAGIC 34393
struct diaClientDataStruct {
  uint magic;
  uint iomUnitIdx;
  uint chan;
  uint diaUnitIdx;
};
typedef struct diaClientDataStruct diaClientData;

// Indexed by sim unit number
struct dia_unit_data
  {
    uint mailboxAddress;
    word24 l66Addr;
    bool connected; // For UDP code, the dn355 has indicated it has booted
    uv_udp_t udpSendHandle;
    uv_udp_t udpRecvHandle;
    //uv_udp_t socket;
#define ATTACH_ADDRESS_SZ 4096
    char attachAddress [ATTACH_ADDRESS_SZ];
    diaClientData clientData;
  };

typedef struct s_dia_data
  {
    struct dia_unit_data dia_unit_data [N_DIA_UNITS_MAX];
  } t_dia_data;

extern t_dia_data dia_data;

#if 0
// dn355_mailbox.incl.pl1
//   input_sub_mbx
//       pad1:8, line_number:10, n_free_buffers:18
//       n_chars:18, op_code:9, io_cmd:9
//       n_buffers
//       { abs_addr:24, tally:12 } [24]
//       command_data

struct input_sub_mbx
  {
    word36 word1; // dn355_no; is_hsla; la_no; slot_no    // 0      word0
    word36 word2; // cmd_data_len; op_code; io_cmd        // 1      word1
    word36 n_buffers;
    word36 dcws [24];
    word36 command_data;
  };
#endif

void dia_init(void);
int dia_iom_cmd (uint iomUnitIdx, uint chan);
void diaProcessEvents (void);
