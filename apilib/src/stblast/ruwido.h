/*****************************************************************************
File Name   : ruwido.h

Description : Prototypes for ruwido routines.

Copyright (C) 2005 STMicroelectronics

History     : Created June 2005

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __RUWIDO_H
#define __RUWIDO_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"
    
#define RUWIDO_HALF_LEVEL_DURATION           315  
#define RUWIDO_HALF_LEVEL_DURATION_MIN       215 
#define RUWIDO_HALF_LEVEL_DURATION_MAX       440
#define RUWIDO_SYMBOL_DURATION_3T_MIN        795 
#define RUWIDO_SYMBOL_DURATION_3T_MAX       1150


ST_ErrorCode_t BLAST_RuwidoDecode(U32                 *UserBuf_p,
                                  STBLAST_Symbol_t     *SymbolBuf_p,
                                  const U32            SymbolsAvailable,
                                  U32                  *NumDecoded_p,
                                  U32                  *SymbolsUsed_p);                                 	

#endif
