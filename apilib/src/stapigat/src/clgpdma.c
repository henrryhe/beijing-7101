/*******************************************************************************

File name   : clgpdma.c

Description : GPDMA configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
09 Apr 2002        Created                                          HSdLM
28 Oct 2002        Change init for single interrupt line platforms  BS
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "testcfg.h"

#ifdef USE_GPDMA

#define USE_AS_FRONTEND

#include "genadd.h"
#include "stdevice.h"
#include "stddefs.h"

#include "stgpdma.h"
#include "clgpdma.h"

#include "clevt.h"

/* functions exported by STGPDMA simulator */
#ifdef STGPDMA_SIM
void GPDMA_Init(partition_t *);
U32 *GPDMA_Start (char *, U32 InterruptNumber, U32 NumberOfChannels);
void GPDMA_Finish (U32 *);
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#define GPDMA_CHANNEL   2

/* Private Variables (static)------------------------------------------------ */

static void *Address_p;

/* Global Variables --------------------------------------------------------- */

extern ST_Partition_t *NCachePartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : GPDMA_UseInit
Description : Initialize the GPDMA driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL GPDMA_UseInit()
{
    STGPDMA_InitParams_t GPDMAInitParams;
    BOOL RetOk = TRUE;

    Address_p = (void *)GPDMA_BASE_ADDRESS;

#ifdef STGPDMA_SIM
    STTBX_Print (("\nInitializing GPDMA Simulator..."));
    GPDMA_Init(NCachePartition_p);
    Address_p = (void *)GPDMA_Start ("GPDMA", GPDMA_BASE_INTERRUPT,4);
    STTBX_Print (("\nDONE."));
#endif

    GPDMAInitParams.DeviceType      = STGPDMA_DEVICE_GPDMAC;
    strcpy (GPDMAInitParams.EVTDeviceName, STEVT_DEVICE_NAME);
    GPDMAInitParams.DriverPartition = NCachePartition_p;
    GPDMAInitParams.BaseAddress     = Address_p;
#ifdef STGPDMA_SINGLE_INTERRUPT
    GPDMAInitParams.InterruptNumber = GPDMA_BASE_INTERRUPT;
#else
    GPDMAInitParams.InterruptNumber = GPDMA_BASE_INTERRUPT + GPDMA_CHANNEL;
#endif /* STGPDMA_SINGLE_INTERRUPT */
    GPDMAInitParams.InterruptLevel  = 5;
    GPDMAInitParams.ChannelNumber   = GPDMA_CHANNEL;
    GPDMAInitParams.FreeListSize    = 2;
    GPDMAInitParams.MaxHandles      = 2;
    GPDMAInitParams.MaxTransfers    = 4;

    if ((STGPDMA_Init(STGPDMA_DEVICE_NAME,&GPDMAInitParams)) == ST_NO_ERROR)
    {
        printf("STGPDMA initialized, \trevision=%s\n",STGPDMA_GetRevision());
    }
    else
    {
        printf("STGPDMA_Init() failed !\n");
        RetOk = FALSE;
    }
    return(RetOk);
} /* end of GPDMA_UseInit() */


/*******************************************************************************
Name        : GPDMA_Term
Description : Initialise the STGPDMA driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void GPDMA_Term(void)
{
    ST_ErrorCode_t     ErrCode = ST_NO_ERROR;
    STGPDMA_TermParams_t TermParams;

    ErrCode = STGPDMA_Term( STGPDMA_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_NO_ERROR)
    {
        printf("STGPDMA_Term(%s): ok\n", STGPDMA_DEVICE_NAME);
    }
    else
    {
        printf("STGPDMA_Term(%s): error=0x%0x !\n", STGPDMA_DEVICE_NAME, ErrCode);
    }
#ifdef STGPDMA_SIM
    /* Stop the GPDMA simulators */
    GPDMA_Finish (Address_p);
#endif

} /* end GPDMA_Term() */

#endif /* USE_GPDMA */
/* End of clgpdma.c */
