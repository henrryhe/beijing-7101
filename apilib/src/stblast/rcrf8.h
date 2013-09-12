/*****************************************************************************
File Name   : rcrf8.h

Description : Prototypes for rcrf8 coding routines.

Copyright (C) 2005 STMicroelectronics

History     : 

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __RCRF8_H
#define __RCRF8_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"

/* prototype declaration */
ST_ErrorCode_t BLAST_RCRF8Decode(U32                *UserBuf_p,
                               STBLAST_Symbol_t     *SymbolBuf_p,
                               U32                  SymbolsAvailable,
                               U32                  *NumDecoded_p,
                               U32                  *SymbolsUsed_p,
                               const STBLAST_ProtocolParams_t *ProtocolParams_p);

ST_ErrorCode_t BLAST_RCRF8Encode(const U32                *UserBuf_p,
                               const U32                  UserBufSize,
                               STBLAST_Symbol_t           *SymbolBuf_p,
                               U32                        *SymbolsEncoded_p,
                               const STBLAST_ProtocolParams_t *ProtocolParams_p);
                               

#endif
