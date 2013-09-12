/*****************************************************************************
File Name   : shift.h

Description : Prototypes for rc6a coding routines.

Copyright (C) 2002 STMicroelectronics

History     : Created April 2002 (MV)

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __SHIFT_H
#define __SHIFT_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"

ST_ErrorCode_t BLAST_ShiftEncode(const U32                  *UserBuf_p,
                                 const U32                  UserBufSize,
                                 STBLAST_Symbol_t           *SymbolBuf_p,
                                 const U32                  SymbolBufSize,
                                 U32                        *SymbolsEncoded_p,
                                 const STBLAST_ProtocolParams_t *ProtocolParams_p);

      

ST_ErrorCode_t BLAST_ShiftDecode(U32                  *UserBuf_p,
                                 const U32            UserBufSize,
                                 STBLAST_Symbol_t     *SymbolBuf_p,
                                 const U32            SymbolsAvailable,
                                 U32                  *NumDecoded_p,
                                 U32                  *SymbolsUsed_p,
                                 const STBLAST_ProtocolParams_t *ProtocolParams_p);

#endif
