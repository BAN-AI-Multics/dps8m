/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 3ceeac54-f62e-11ec-b6e7-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2018 Charles Anthony
 * Copyright (c) 2021-2023 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#ifndef DPS8_EM_CONSTS_H
# define DPS8_EM_CONSTS_H

////////////////
//
// System components: SCU, IOM, CPU
//
////////////////

// SCU
enum { N_SCU_UNITS_MAX = 8 };

// IOM
enum { N_IOM_UNITS_MAX = 4 };

// CPU
enum { N_CPU_UNITS_MAX = 8 };

////////////////
//
// Controllers
//
////////////////

// URP (Unit record processor_
enum { N_URP_UNITS_MAX = 16 };

// ABSI
enum { N_ABSI_UNITS_MAX = 1 };

// MGP (Chaosnet)
enum { N_MGP_UNITS_MAX =  2 };

// FNP
enum { N_FNP_UNITS_MAX = 16 };

// OPC (Operator console)
enum { N_OPC_UNITS_MAX =  8 };

// MTP
enum { N_MTP_UNITS_MAX = 16 };

// MSP
enum { N_MSP_UNITS_MAX = 16 };

// IPC
enum { N_IPC_UNITS_MAX = 16 };

// DIA
enum { N_DIA_UNITS_MAX = 16 };

////////////////
//
// Peripherals
//
////////////////

// Tape drive
enum { N_MT_UNITS_MAX  = 34 };

// Printer
enum { N_PRT_UNITS_MAX = 34 };

// Socket controller
enum { N_SKC_UNITS_MAX = 64 };

// Card reader
enum { N_RDR_UNITS_MAX = 16 };

// Card punch
enum { N_PUN_UNITS_MAX = 16 };

// Disk
enum { N_DSK_UNITS_MAX = 64 };

//
// Memory
//

enum { MEMSIZE = MEM_SIZE_MAX };

//
// Controller ports
//

enum { MAX_CTLR_PORTS = 8 };

//
// CPU ports
//

enum { N_DPS8M_CPU_PORTS = 4 };
enum { N_L68_CPU_PORTS   = 8 };
enum { N_CPU_PORTS       = 8 }; // For data structures, allocate the worst case and enforce limit at run-time

#endif // DPS8_EM_CONSTS_H
