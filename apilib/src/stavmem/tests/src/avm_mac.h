/*****************************************************************************

File name   : avm_mac.h

Description : header file for avm_mac.c

COPYRIGHT (C) ST-Microelectronics 2002.

Date                Modification                                     Name
----                ------------                                     ----
04 Dec 2002         Created from avm_spd.c                          HSdLM
*****************************************************************************/

#ifndef __AVM_MAC_H
#define __AVM_MAC_H

/* Includes --------------------------------------------------------------- */

#include "testtool.h"

/* Exported Macros -------------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

BOOL TestsAccess(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
#ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
BOOL Test1DBigData(STTST_Parse_t *Parse_p ,char *ResultSymbol_p);
#endif

#endif /* #ifndef __AVM_MAC_H */

