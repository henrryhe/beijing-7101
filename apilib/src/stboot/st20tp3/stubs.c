/*****************************************************************************
File Name   : cache.c

Description : Backend stub functions.

NB: The st20tp3 has no backend initialization nor sdram interface.  Therefore
    these functions are stubbed.

Copyright (C) 2000 STMicroelectronics

History     :

    24/05/99  First attempt.

Reference   :
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stdio.h>
#include "stlite.h"                     /* STLite includes */
#include "stsys.h"                      /* STAPI includes */

#include "stboot.h"
#include "sdram.h"
#include "be_init.h"

/* Private types/constants ------------------------------------------------ */

/* Private variables ------------------------------------------------------ */

/* Private function prototypes -------------------------------------------- */

/* Function definitions --------------------------------------------------- */

ST_ErrorCode_t stboot_BackendInit(STBOOT_Backend_t *BackendType)
{
    return ST_NO_ERROR;
}

BOOL stboot_InitSDRAM(U32 Frequency, STBOOT_DramMemorySize_t MemorySize)
{
    return FALSE;
}

/* End of stubs.c */
