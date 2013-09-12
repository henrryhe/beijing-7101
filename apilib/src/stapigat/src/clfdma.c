/*******************************************************************************

File name   : clfdma.c

Description : FDMA configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
20 Nov 2002        Created                                          HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "testcfg.h"

#ifdef USE_FDMA

#define USE_AS_FRONTEND

#include "genadd.h"
#include "stdevice.h"
#include "stddefs.h"

#include "stcommon.h"

#include "stfdma.h"
#include "clfdma.h"


#if !defined ST_5100 && !defined ST_7710 && !defined ST_5517 && !defined ST_5105 && !defined ST_5301 && !defined ST_8010 \
 && !defined ST_7100 && !defined ST_7109 && !defined ST_5188 && !defined ST_5525 && !defined ST_5107 && !defined ST_7200
# error Fdma not supported for this chip
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#ifndef FDMA_INTERRUPT
#include "interrupt_st40.h"
#define FDMA_INTERRUPT 			OS21_INTERRUPT_FDMA_MBOX
#endif

#ifndef FDMA_INTERRUPT_LEVEL
#define FDMA_INTERRUPT_LEVEL  	2 /* dummy value for compilation, waiting definition */
#endif

#ifndef FDMA_BASE_ADDRESS
#define FDMA_BASE_ADDRESS     	0xB9220000 /* dummy value for compilation, waiting definition */
#endif
#if defined (ST_7200)
#define STFDMA_DEVICE_NUMBER    STFDMA_2
#else
#define STFDMA_DEVICE_NUMBER    STFDMA_1
#endif
/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* --- Extern variables (from startup.c) ---------------------------------- */
extern ST_Partition_t *DriverPartition_p;
extern ST_Partition_t *NCachePartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : FDMA_Init
Description : Initialize the FDMA driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL FDMA_Init()
{
    STFDMA_InitParams_t FDMAInitParams;
    ST_ErrorCode_t      ErrorCode;
    BOOL RetOk = TRUE;
#ifdef ST_5517
    FDMAInitParams.DeviceType          = STFDMA_DEVICE_FDMA_1;
#else
    FDMAInitParams.DeviceType          = STFDMA_DEVICE_FDMA_2;
#endif
    FDMAInitParams.FDMABlock           = STFDMA_DEVICE_NUMBER;
    FDMAInitParams.DriverPartition_p   = DriverPartition_p;
    FDMAInitParams.NCachePartition_p   = NCachePartition_p;


#if defined (ST_7100) || defined (ST_7109) || defined (ST_5188) || defined (ST_7200)
	FDMAInitParams.NumberCallbackTasks = 1;
#else
	FDMAInitParams.NumberCallbackTasks = 0;
#endif
    FDMAInitParams.InterruptLevel      = FDMA_INTERRUPT_LEVEL;
#if defined (ST_7200)
    FDMAInitParams.InterruptNumber     = FDMA_1_INTERRUPT;
    FDMAInitParams.BaseAddress_p       = (void *)FDMA_1_BASE_ADDRESS;
#else
    FDMAInitParams.InterruptNumber     = FDMA_INTERRUPT;
	FDMAInitParams.BaseAddress_p       = (void *)FDMA_BASE_ADDRESS;
#endif
    FDMAInitParams.ClockTicksPerSecond = ST_GetClocksPerSecond();

    ErrorCode = STFDMA_Init(STFDMA_DEVICE_NAME, &FDMAInitParams);

    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STFDMA initialized, \trevision=%s\n",STFDMA_GetRevision());
    }
    else
    {
        printf("STFDMA_Init() failed: error=0x%0x !\n", ErrorCode);
        RetOk = FALSE;
    }
    return(RetOk);
} /* end of FDMA_Init() */


/*******************************************************************************
Name        : FDMA_Term
Description : Initialise the STFDMA driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void FDMA_Term(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    ErrorCode = STFDMA_Term( STFDMA_DEVICE_NAME, FALSE, STFDMA_DEVICE_NUMBER);

    if (ErrorCode != ST_NO_ERROR)
    {
        printf("STFDMA_Term(%s, ForceTerminate=FALSE): Warning !! error=0x%0x !\n", STFDMA_DEVICE_NAME, ErrorCode);
        ErrorCode = STFDMA_Term( STFDMA_DEVICE_NAME, TRUE, STFDMA_DEVICE_NUMBER);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STFDMA_Term(%s): ok\n", STFDMA_DEVICE_NAME);
    }
    else
    {
        printf("STFDMA_Term(%s): error=0x%0x !\n", STFDMA_DEVICE_NAME, ErrorCode);
    }

} /* end FDMA_Term() */

#endif /* USE_FDMA */
/* End of clfdma.c */

