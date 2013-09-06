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


ST_ErrorCode_t CH_BLAST_RC6AEncode(const U32                  *UserBuf_p,
                               const U32                  UserBufSize,
                               STBLAST_Symbol_t           *SymbolBuf_p,
                               const U32                  SymbolBufSize,
                               U32                        *SymbolsEncoded_p,
                               const STBLAST_ProtocolParams_t *ProtocolParams_p);

ST_ErrorCode_t CH_BLAST_RC6ADecode(
							   U32              *UserBuf_p,
                               const U32            UserBufSize,
                               STBLAST_Symbol_t     *SymbolBuf_p,
                               U32                  SymbolsAvailable,
                               U32                  *NumDecoded_p,
                               U32                  *SymbolsUsed_p);

#endif
