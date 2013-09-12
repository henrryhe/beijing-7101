/*****************************************************************************
File Name   : rcmm.h

Description : Prototypes for rcmm 24/32 coding routines.

Copyright (C) 2004 STMicroelectronics

History     : 

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __RCMM_H
#define __RCMM_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"

/* The detection time of header must be between 330 
   us and 583 us */
#define RCMM_MAX_HEADER_ON     583
#define RCMM_MIN_HEADER_ON     330
#define RCMM_MIN_HEADER_TIME   636
#define RCMM_MAX_HEADER_TIME   752


ST_ErrorCode_t BLAST_RcmmEncode(const U32                  *UserBuf_p,
                               STBLAST_Symbol_t           *SymbolBuf_p,
                               U32                        *SymbolsEncoded_p,
                               const STBLAST_ProtocolParams_t *ProtocolParams_p);
                            

ST_ErrorCode_t BLAST_RcmmDecode(U32                  *UserBuf_p,
                                STBLAST_Symbol_t     *SymbolBuf_p,
                                U32            		 SymbolsAvailable,
                                U32                  *NumDecoded_p,
                                U32                  *SymbolsUsed_p,
                                const STBLAST_ProtocolParams_t *ProtocolParams_p);

#endif
