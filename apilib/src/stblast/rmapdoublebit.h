/*****************************************************************************
File Name   : rmapdoublebit.h

Description : Prototypes for ruwido routines.

Copyright (C) 2005 STMicroelectronics

History     : Created June 2005

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __RMAP_H
#define __RMAP_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"
    
/* The detection time of header must be between 330 
   us and 583 us */
#define RMAP_MAX_HEADER_ON     583
#define RMAP_MIN_HEADER_ON     330
#define RMAP_MIN_HEADER_TIME   636
#define RMAP_MAX_HEADER_TIME   752


ST_ErrorCode_t BLAST_RMapDoubleBitDecode(U32                 *UserBuf_p,
                                 	     STBLAST_Symbol_t     *SymbolBuf_p,
                                	     const U32            SymbolsAvailable,
                                 		 U32                  *NumDecoded_p,
                                 		 U32                  *SymbolsUsed_p,
                                 		 const 				  STBLAST_ProtocolParams_t *ProtocolParams_p);

#endif
