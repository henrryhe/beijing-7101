/*****************************************************************************
File Name   : rmap.h

Description : Prototypes for ruwido routines.

Copyright (C) 2006 STMicroelectronics

History     : Created Jan 2006

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __RSTEP_H
#define __RSTEP_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"
      
#define RMAP_HALF_LEVEL_DURATION         250
#define RMAP_HALF_LEVEL_DURATION_MAX     300 /* 20% offset  */

ST_ErrorCode_t BLAST_RMapDecode (U32                  *UserBuf_p,
                                 STBLAST_Symbol_t     *SymbolBuf_p,
                                 const U32            SymbolsAvailable,
                                 U32                  *NumDecoded_p,
                                 const STBLAST_ProtocolParams_t *ProtocolParams_p);

#endif
