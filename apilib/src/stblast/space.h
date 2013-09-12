/*****************************************************************************
File Name   : space.h

Description : Prototypes for space coding routines.

Copyright (C) 2001 STMicroelectronics

History     : Split from blastint May 2001 (PW)

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __SPACE_H
#define __SPACE_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"

ST_ErrorCode_t BLAST_SpaceEncode(const U32                  *UserBuf_p,
                                 const U32                  UserBufSize,
                                 STBLAST_Symbol_t           *SymbolBuf_p,
                                 const U32                  SymbolBufSize,
                                 U32                        *SymbolsEncoded_p,
                                 const STBLAST_ProtocolParams_t *ProtocolParams_p
                                );

ST_ErrorCode_t BLAST_SpaceDecode(U32                  *UserBuf_p,
                                 const U32            UserBufSize,
                                 STBLAST_Symbol_t     *SymbolBuf_p,
                                 const U32            SymbolsAvailable,
                                 U32                  *NumDecoded_p,
                                 U32                  *SymbolsUsed_p,
                                 const STBLAST_ProtocolParams_t *ProtocolParams_p,
                                 U32 TimeDiff);

#endif /* #ifndef __SPACE_H */
