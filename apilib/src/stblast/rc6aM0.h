/*****************************************************************************
File Name   : rc6a_Mode0.h

Description : Prototypes for rc6a coding routines.

Copyright (C) 2002 STMicroelectronics

History     : Created April 2002 (MV)

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __RC6A_MODE0_H
#define __RC6A_MODE0_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"


#define RC6AM0_MAX_6T 3360
#define RC6AM0_MIN_6T 2179
#define RC6AM0_MAX_3T 1573
#define RC6AM0_MIN_3T 1125

ST_ErrorCode_t BLAST_RC6AEncode_Mode0(const U32                  *UserBuf_p,
                                      STBLAST_Symbol_t           *SymbolBuf_p,
                                      U32                        *SymbolsEncoded_p,
                             		  const STBLAST_ProtocolParams_t *ProtocolParams_p);

ST_ErrorCode_t BLAST_RC6ADecode_Mode0(U32                  *UserBuf_p,
                                      STBLAST_Symbol_t     *SymbolBuf_p,
		                              U32                  SymbolsAvailable,
		                              U32                  *NumDecoded_p,
		                              U32                  *SymbolsUsed_p,
		                              const STBLAST_ProtocolParams_t *ProtocolParams_p);

#endif
