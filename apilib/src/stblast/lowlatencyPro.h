/*****************************************************************************
File Name   : lowlatency.h

Description : Prototypes for Low Latency coding routines.

Copyright (C) 2001 STMicroelectronics

History     : 

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LOWLATENCYPRO_H
#define __LOWLATENCYPRO_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"

#define LOW_LATENCYPRO_SYSTEM_BITS        4
#define LOW_LATENCYPRO_USER_BITS          4
#define LOW_LATENCYPRO_LONG_ADDR_BITS     20
#define LOW_LATENCYPRO_CHECKSUM_BITS      4

#define LOW_MAX_12T  8000  /* 680 * 12 */ 
#define LOW_MIN_12T  6900  /* 580 * 12 */
                           

ST_ErrorCode_t BLAST_LowLatencyProEncode(const U32                  *UserBuf_p,
	                                     STBLAST_Symbol_t           *SymbolBuf_p,
	                                     U32                        *SymbolsEncoded_p,
	                                     const STBLAST_ProtocolParams_t *ProtocolParams_p);                           

ST_ErrorCode_t BLAST_LowLatencyProDecode(U32                  *UserBuf_p,
                                         STBLAST_Symbol_t     *SymbolBuf_p,
                                         U32                   SymbolsAvailable,
                                         U32                  *NumDecoded_p,
                                         U32                  *SymbolsUsed_p,
                                         const STBLAST_ProtocolParams_t *ProtocolParams_p);

#endif
