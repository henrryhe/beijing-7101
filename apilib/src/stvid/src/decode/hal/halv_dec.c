/*******************************************************************************

File name   : halv_dec.c

Description : HAL video decode source file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
06 Jul 2000        Created                                           HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"
#include "vid_ctcm.h"

#include "halv_dec.h"

#ifdef ST_omega1
#include "hv_dec1.h"
#endif
#ifdef ST_omega2
#include "hv_dec2.h"
#endif
#ifdef ST_lcmpegx1
#include "lcmpegx1.h"
#endif
#ifdef ST_sdmpgo2
#include "hv_dec8.h"
#endif

#include "sttbx.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

 /* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
 #define HALDEC_VALID_HANDLE 0x0155155f

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/* Exported functions ------------------------------------------------------- */


/*******************************************************************************
Name        : HALDEC_Init
Description : Init decode HAL
Parameters  : Init params, pointer to handle to return, pointer to decode number to return
Assumptions : Pointers are not NULL
Limitations :
Returns     : in params: HAL decode handle
*******************************************************************************/
ST_ErrorCode_t HALDEC_Init(const HALDEC_InitParams_t * const DecodeInitParams_p, HALDEC_Handle_t * const DecodeHandle_p)
{
    HALDEC_Properties_t * HALDEC_Data_p;
    STEVT_OpenParams_t STEVT_OpenParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Allocate a HALDEC structure */
    SAFE_MEMORY_ALLOCATE(HALDEC_Data_p, HALDEC_Properties_t *, DecodeInitParams_p->CPUPartition_p, sizeof(HALDEC_Properties_t));
    if (HALDEC_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *DecodeHandle_p = (HALDEC_Handle_t *) HALDEC_Data_p;
    HALDEC_Data_p->ValidityCheck = HALDEC_VALID_HANDLE;

    /* Store parameters */
    HALDEC_Data_p->CPUPartition_p         = DecodeInitParams_p->CPUPartition_p;
    HALDEC_Data_p->RegistersBaseAddress_p = DecodeInitParams_p->RegistersBaseAddress_p;
    HALDEC_Data_p->DeviceType             = DecodeInitParams_p->DeviceType;
    HALDEC_Data_p->SDRAMMemoryBaseAddress_p = DecodeInitParams_p->SharedMemoryBaseAddress_p;
    HALDEC_Data_p->DecoderNumber          = DecodeInitParams_p->DecoderNumber;
    HALDEC_Data_p->AvmemPartition         = DecodeInitParams_p->AvmemPartition;
    HALDEC_Data_p->CompressedDataInputBaseAddress_p = DecodeInitParams_p->CompressedDataInputBaseAddress_p;
    strcpy(HALDEC_Data_p->VideoName, DecodeInitParams_p->VideoName);
#ifdef ST_inject
    HALDEC_Data_p->InjecterHandle         = DecodeInitParams_p->InjecterHandle;
#endif /* ST_inject */
#ifdef ST_multidecodeswcontroller
    HALDEC_Data_p->MultiDecodeHandle      = DecodeInitParams_p->MultiDecodeHandle;
#endif /* ST_multidecodeswcontroller */
    HALDEC_Data_p->InterruptLevel         = DecodeInitParams_p->SharedItParams.Level;
    HALDEC_Data_p->InterruptNumber        = DecodeInitParams_p->SharedItParams.Number;
    HALDEC_Data_p->InterruptEvt           = DecodeInitParams_p->SharedItParams.Event;
    /* Bitbuffer params */
    HALDEC_Data_p->BitBufferSize          = DecodeInitParams_p->BitBufferSize;
    /* Open EVT handle */
    ErrorCode = STEVT_Open(DecodeInitParams_p->EventsHandlerName, &STEVT_OpenParams, &(HALDEC_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        HALDEC_Term(*DecodeHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* First initialise pointer to table of function to NULL, then will check if affected. */
    HALDEC_Data_p->FunctionsTable_p = NULL;

    switch (HALDEC_Data_p->DeviceType)
    {
#ifdef ST_omega1
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
            HALDEC_Data_p->FunctionsTable_p = &HALDEC_Omega1Functions;
            break;
#endif /* ST_omega1 */

#ifdef ST_omega2
        case STVID_DEVICE_TYPE_7015_MPEG :
        case STVID_DEVICE_TYPE_7020_MPEG :
            HALDEC_Data_p->FunctionsTable_p = &HALDEC_Omega2Functions;
            break;
#endif /* ST_omega2 */

#ifdef ST_sdmpgo2
        case STVID_DEVICE_TYPE_5528_MPEG :
        case STVID_DEVICE_TYPE_STD2000_MPEG :
            HALDEC_Data_p->FunctionsTable_p = &HALDEC_Sdmpgo2Functions;
            break;
#endif /* ST_sdmpgo2 */

#ifdef ST_lcmpegx1
        case STVID_DEVICE_TYPE_7710_MPEG :
        case STVID_DEVICE_TYPE_7100_MPEG :
        case STVID_DEVICE_TYPE_7109_MPEG :
        case STVID_DEVICE_TYPE_5100_MPEG :
        case STVID_DEVICE_TYPE_5525_MPEG :
        case STVID_DEVICE_TYPE_5105_MPEG :
        case STVID_DEVICE_TYPE_5301_MPEG :
            HALDEC_Data_p->FunctionsTable_p = &HALDEC_LcmpegFunctions;
            break;
#endif /* ST_lcmpegx1 */

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* switch (HALDEC_Data_p->DeviceType) */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialise decoder */
        ErrorCode = (HALDEC_Data_p->FunctionsTable_p->Init)(*DecodeHandle_p);
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: allocation failed, undo initialisations done */
        HALDEC_Term(*DecodeHandle_p);
    }

    return(ErrorCode);
} /* End of HALDEC_Init() function */


/*******************************************************************************
Name        : HALDEC_Term
Description : Terminate decode HAL
Parameters  : Decode handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t HALDEC_Term(const HALDEC_Handle_t DecodeHandle)
{
    HALDEC_Properties_t * HALDEC_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    HALDEC_Data_p = (HALDEC_Properties_t *) DecodeHandle;

    if (HALDEC_Data_p == NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (HALDEC_Data_p->ValidityCheck != HALDEC_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(HALDEC_Data_p->EventsHandle);

    /* Terminate HAL Decode. */
    (HALDEC_Data_p->FunctionsTable_p->Term)(DecodeHandle);

    /* De-validate structure */
    HALDEC_Data_p->ValidityCheck = 0; /* not HALDEC_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(HALDEC_Data_p, HALDEC_Data_p->CPUPartition_p, sizeof(HALDEC_Properties_t));

    return(ST_NO_ERROR);
} /* End of HALDEC_Term() function */




/* Private functions -------------------------------------------------------- */



/* End of halv_dec.c */
