/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: fa4a490d-f62e-11ec-bbb0-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2019-2021 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#if !defined(API)
# define N_SYMBOLS    1024
#else
# define N_SYMBOLS    0
#endif /* if !defined(API) */

#define SYMBOL_SZ     32
enum symbolType {
  SYM_EMPTY         = 0,
  SYM_STATE_OFFSET  = 1,  // Offset from state of system state
  SYM_STRUCT_OFFSET = 2,  // Offset from state of parent structure
  SYM_STRUCT_SZ     = 3,  // Structure size
  SYM_CONSTANT      = 4,
  SYM_ENUM          = 5,
};

enum valueType {
  SYM_UNDEF         = 0,
  SYM_STRING        = 10,
  SYM_PTR           = 20,
  SYM_ARRAY         = 21,
  SYM_SZ            = 30,

  SYM_UINT8         = 800,
  SYM_UINT8_1       = 801,
  SYM_UINT8_3       = 803,
  SYM_UINT8_6       = 806,
  SYM_UINT16        = 1600,
  SYM_UINT16_9      = 1609,
  SYM_UINT16_12     = 1612,
  SYM_UINT16_14     = 1614,
  SYM_UINT16_15     = 1615,
  SYM_UINT32        = 3200,
  SYM_UINT32_18     = 3218,
  SYM_UINT32_24     = 3224,
  SYM_UINT32_27     = 3227,
  SYM_UINT64        = 6400,
  SYM_UINT64_36     = 6436,
};

struct symbol_s {
  char     name[SYMBOL_SZ];
  uint32_t symbolType;
  uint32_t valueType;
  uint32_t value;
};

#define SYMTAB_HDR    "dps8m symtab"
#define SYMTAB_HDR_SZ 16
#define SYMTAB_VER    1

struct symbolTable_s {
  char    symtabHdr[SYMTAB_HDR_SZ];    // = STATE_HDR
  int32_t symtabVer;                   // = STATE_VER
  struct  symbol_s symbols[N_SYMBOLS];
};

#define STATE_HDR     "dps8m state"
#define STATE_HDR_SZ  16
#define STATE_VER     1

struct system_state_s {
  // The first three are fixed layout
  char        stateHdr[STATE_HDR_SZ];  // = STATE_HDR
  int32_t     stateVer;                // = STATE_VER
  struct      symbolTable_s symbolTable;
#if !defined(API)
  char        commit_id [41];
  volAtomic   word36 M [MEMSIZE];
  cpu_state_t cpus [N_CPU_UNITS_MAX];
  struct cables_s cables;
#endif /* if !defined(API) */
};

#if !defined(API)
extern struct system_state_s * system_state;
#else
int sim_iglock     =  0;
int sim_nolock     =  0;
int sim_randompst  =  0;
int sim_randstate  =  0;
int sim_nostate    =  0;
#endif /* if !defined(API) */
