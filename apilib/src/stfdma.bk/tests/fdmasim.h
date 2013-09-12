/*****************************************************************************
File Name   : fdmasim.h

Description : STFDMA test harness: FDMA simulator header file

History     : 
              
Copyright (C) 2002 STMicroelectronics

Reference   :
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __FDMASIM_H
#define __FDMASIM_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"         /* Standard definitions */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */
    
/* Exported Functions ----------------------------------------------------- */
ST_ErrorCode_t FDMASIM_Init(U32 *BaseAddress_p, U32 InterruptNumber,
                            ST_Partition_t *DriverPartition_p);


void FDMASIM_Term(void);

U32 FDMASIM_GetReg(U32 Offset);

void FDMASIM_SetReg(U32 Offset, U32 Value);    

#ifdef __cplusplus
}
#endif

#endif
/* EOF */
















