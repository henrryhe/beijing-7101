/*****************************************************************************
File Name   : rc5.h

Description : Prototypes for rc5 coding routines.

Copyright (C) 2001 STMicroelectronics

History     : Split from blastint May 2001 (PW)

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __RC5_H
#define __RC5_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"

/* One bit period is defined to be ~27.778us x 32 */
#define RC5_BIT_PERIOD (27*32)
#define RC5_BIT_PERIOD_MAX 1296

/* To give tight control over timing ,Earlier BOUNDAY_LIMIT was (BIT_PERIOD/2) */
/* #define BOUNDARY_LIMIT 150 */
/* Done For 5516 Brick Board */
#define BOUNDARY_LIMIT 300

ST_ErrorCode_t BLAST_RC5Encode(const U32                  *UserBuf_p,
  							   const U32                  UserBufSize,
                               STBLAST_Symbol_t           *SymbolBuf_p,
                               U32                        *SymbolsEncoded_p);

ST_ErrorCode_t BLAST_RC5Decode(U32                  *UserBuf_p,
                               STBLAST_Symbol_t     *SymbolBuf_p,
                               U32                  SymbolsAvailable,
                               U32                  *NumDecoded_p,
                               U32                  *SymbolsUsed_p);

#endif
