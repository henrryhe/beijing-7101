/*****************************************************************************
File Name   : rc6a.h

Description : Prototypes for rc6a coding routines.

Copyright (C) 2002 STMicroelectronics

History     : Created April 2002 (MV)

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __RC6A_H
#define __RC6A_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"

#define RC6A_MAX_6T (U16)3360
#define RC6A_MIN_6T (U16)2178.667
#define RC6A_MAX_2T (U16)1124.667
#define RC6A_MIN_2T (U16)671.333

ST_ErrorCode_t BLAST_RC6AEncode(const U32                  *UserBuf_p,
                                STBLAST_Symbol_t           *SymbolBuf_p,
                                U32                        *SymbolsEncoded_p,
                                const STBLAST_ProtocolParams_t *ProtocolParams_p);

ST_ErrorCode_t BLAST_RC6ADecode(U32                 *UserBuf_p,
                                STBLAST_Symbol_t    *SymbolBuf_p,
                                U32                 SymbolsAvailable,
                                U32                 *NumDecoded_p,
                                U32                 *SymbolsUsed_p,
                                const STBLAST_ProtocolParams_t *ProtocolParams_p);

#endif
