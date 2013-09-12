/*****************************************************************************
File Name   : lowlatency.h

Description : Prototypes for Low Latency coding routines.

Copyright (C) 2001 STMicroelectronics

History     : 

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LOWLATENCY_H
#define __LOWLATENCY_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"

#define LOW_LATENCY_SYSTEM_BITS     4
#define LOW_LATENCY_CHECKSUM_BITS   4

#define LOW_MAX_12T  8000  /* 680 * 12 */ 
#define LOW_MIN_12T  6900  /* 580 * 12 */


ST_ErrorCode_t BLAST_LowLatencyEncode(const U32           *UserBuf_p,
                                      STBLAST_Symbol_t    *SymbolBuf_p,
                                      U32                 *SymbolsEncoded_p,
                                      const STBLAST_ProtocolParams_t *ProtocolParams_p);
                            

ST_ErrorCode_t BLAST_LowLatencyDecode(U32                  *UserBuf_p,
                                      STBLAST_Symbol_t     *SymbolBuf_p,
                               		  U32                  SymbolsAvailable,
                                      U32                  *NumDecoded_p,
                                      U32                  *SymbolsUsed_p,
                                      const STBLAST_ProtocolParams_t *ProtocolParams_p);

#endif
