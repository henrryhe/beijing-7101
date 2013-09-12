/*******************************************************************************

File name   : transcodec.c

Description : Video driver Transcodec source file

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
19 Seo 2006        Created                                           OD
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"

#include "transcodec.h"

#ifdef ST_mpg2toh264
#include "mpg2toh264.h"
#endif /* ST_mpg2toh264 */

#ifdef ST_OSLINUX
#include "compat.h"
#else
#include "sttbx.h"
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
/* TODO: decide with STAPI which value to choose */
#define TRANSCODEC_SOURCE_VALID_HANDLE    0x01a51c50
#define TRANSCODEC_TARGET_VALID_HANDLE    0x01a51d50
#define TRANSCODEC_XCODE_VALID_HANDLE    0x01a51e50

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/* Exported functions ------------------------------------------------------- */


/*******************************************************************************
Name        : SOURCE_Init
Description : Init TRANSCODEC Source decoder
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t SOURCE_Init(const SOURCE_InitParams_t * const InitParams_p, SOURCE_Handle_t * const SourceTranscodecHandle_p)
{
    SOURCE_Properties_t *       SOURCE_Data_p;
    STEVT_OpenParams_t          STEVT_OpenParams;
    ST_ErrorCode_t              ErrorCode;

    ErrorCode = ST_NO_ERROR;
    
    /* Allocate a PARSER structure */
    SOURCE_Data_p = (SOURCE_Properties_t *) memory_allocate(InitParams_p->CPUPartition_p, sizeof(SOURCE_Properties_t));
    if (SOURCE_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *SourceTranscodecHandle_p = (SOURCE_Handle_t *) SOURCE_Data_p;
    SOURCE_Data_p->ValidityCheck = TRANSCODEC_SOURCE_VALID_HANDLE;

    /* Store parameters */
    SOURCE_Data_p->CPUPartition_p           = InitParams_p->CPUPartition_p;
    SOURCE_Data_p->DeviceType               = InitParams_p->DeviceType;
    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EvtHandlerName, &STEVT_OpenParams, &(SOURCE_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        SOURCE_Term(*SourceTranscodecHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* First initialise pointer to table of function to NULL, then will check if affected. */
    SOURCE_Data_p->FunctionsTable_p = NULL;

    switch (SOURCE_Data_p->DeviceType)
    {
#ifdef ST_mpg2toh264
        case STVID_DEVICE_TYPE_7109D_MPEG :
            SOURCE_Data_p->FunctionsTable_p = &SOURCE_MPG2Functions;
            break;
#endif /* ST_mpg2toh264 */

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* switch (SOURCE_Data_p->DeviceType) */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialise source decoder */
        ErrorCode = (SOURCE_Data_p->FunctionsTable_p->Init)(*SourceTranscodecHandle_p, InitParams_p);
    }

    return(ErrorCode);
} /* End of SOURCE_Init() function */


/*******************************************************************************
Name        : SOURCE_Term
Description : Terminate TRANSCODEC Source decoder
Parameters  :
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t SOURCE_Term(const SOURCE_Handle_t SourceTranscodecHandle)
{
    SOURCE_Properties_t * SOURCE_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    SOURCE_Data_p = (SOURCE_Properties_t *) SourceTranscodecHandle;

    if (SOURCE_Data_p->ValidityCheck != TRANSCODEC_SOURCE_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(SOURCE_Data_p->EventsHandle);

    /* Terminate HAL Decode. */
    (SOURCE_Data_p->FunctionsTable_p->Term)(SourceTranscodecHandle);

    /* De-validate structure */
    SOURCE_Data_p->ValidityCheck = 0; /* not TRANSCODEC_SOURCE_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    memory_deallocate(SOURCE_Data_p->CPUPartition_p, (void *) SOURCE_Data_p);

    return(ST_NO_ERROR);
} /* End of SOURCE_Term() function */

/*******************************************************************************
Name        : TARGET_Init
Description : Init TRANSCODEC Target decoder
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t TARGET_Init(const TARGET_InitParams_t * const InitParams_p, TARGET_Handle_t * const TargetTranscodecHandle_p)
{
    TARGET_Properties_t *       TARGET_Data_p;
    STEVT_OpenParams_t          STEVT_OpenParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    /* Allocate a TARGET structure */
    TARGET_Data_p = (TARGET_Properties_t *) memory_allocate(InitParams_p->CPUPartition_p, sizeof(TARGET_Properties_t));
    if (TARGET_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *TargetTranscodecHandle_p = (TARGET_Handle_t *) TARGET_Data_p;
    TARGET_Data_p->ValidityCheck = TRANSCODEC_TARGET_VALID_HANDLE;


    /* Store parameters */
    TARGET_Data_p->CPUPartition_p           = InitParams_p->CPUPartition_p;
    TARGET_Data_p->DeviceType               = InitParams_p->DeviceType;
    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EvtHandlerName, &STEVT_OpenParams, &(TARGET_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        TARGET_Term(*TargetTranscodecHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* First initialise pointer to table of function to NULL, then will check if affected. */
    TARGET_Data_p->FunctionsTable_p = NULL;

    switch (TARGET_Data_p->DeviceType)
    {
#ifdef ST_mpg2toh264
        case STVID_DEVICE_TYPE_7109_H264 :
            TARGET_Data_p->FunctionsTable_p = &TARGET_H264Functions;
            break;
#endif /* ST_mpg2toh264 */

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* switch (TARGET_Data_p->DeviceType) */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialise decoder */
        ErrorCode = (TARGET_Data_p->FunctionsTable_p->Init)(*TargetTranscodecHandle_p, InitParams_p);
    }

    return(ErrorCode);
} /* End of TARGET_Init() function */

/*******************************************************************************
Name        : TARGET_Term
Description : Terminate Transcodec Target decoder
Parameters  :
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t TARGET_Term(const TARGET_Handle_t TargetTranscodecHandle)
{
    TARGET_Properties_t * TARGET_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    TARGET_Data_p = (TARGET_Properties_t *) TargetTranscodecHandle;

    if (TARGET_Data_p->ValidityCheck != TRANSCODEC_TARGET_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(TARGET_Data_p->EventsHandle);

    /* Terminate HAL Decode. */
    (TARGET_Data_p->FunctionsTable_p->Term)(TargetTranscodecHandle);

    /* De-validate structure */
    TARGET_Data_p->ValidityCheck = 0; /* not TRANSCODEC_TARGET_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    memory_deallocate(TARGET_Data_p->CPUPartition_p, (void *) TARGET_Data_p);

    return(ST_NO_ERROR);
} /* End of TARGET_Term() function */

/*******************************************************************************
Name        : XCODE_Init
Description : Init TRANSCODEC Transform
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t XCODE_Init(const XCODE_InitParams_t * const InitParams_p, XCODE_Handle_t * const XcodeHandle_p)
{
    XCODE_Properties_t * XCODE_Data_p;
    STEVT_OpenParams_t STEVT_OpenParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Allocate a XCODE structure */
    XCODE_Data_p = (XCODE_Properties_t *) memory_allocate(InitParams_p->STVID_XcodeInitParams.CPUPartition_p, sizeof(XCODE_Properties_t));
    if (XCODE_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *XcodeHandle_p = (XCODE_Handle_t *) XCODE_Data_p;
    XCODE_Data_p->ValidityCheck = TRANSCODEC_XCODE_VALID_HANDLE;

    /* Store parameters */
    XCODE_Data_p->CPUPartition_p           = InitParams_p->STVID_XcodeInitParams.CPUPartition_p;
    XCODE_Data_p->AvmemPartitionHandle     = InitParams_p->STVID_XcodeInitParams.AVMEMPartition;
    XCODE_Data_p->SDRAMMemoryBaseAddress_p = InitParams_p->STVID_XcodeInitParams.SharedMemoryBaseAddress_p;
    
    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->STVID_XcodeInitParams.EvtHandlerName, &STEVT_OpenParams, &(XCODE_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        XCODE_Term(*XcodeHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* First initialise pointer to table of function to NULL, then will check if affected. */
    XCODE_Data_p->FunctionsTable_p = NULL;

#ifdef ST_mpg2toh264
    XCODE_Data_p->FunctionsTable_p = &XCODE_MPG2TOH264Functions;
#endif /* ST_mpg2toh264 */

    if (XCODE_Data_p->FunctionsTable_p == NULL)
    {
      ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialise decoder */
        ErrorCode = (XCODE_Data_p->FunctionsTable_p->Init)(*XcodeHandle_p, InitParams_p);
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: allocation failed, undo initialisations done */
        XCODE_Term(*XcodeHandle_p);
    }

    return(ErrorCode);
} /* End of XCODE_Init() function */


/*******************************************************************************
Name        : XCODE_Term
Description : Terminate VICODEC parser
Parameters  :
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t XCODE_Term(const XCODE_Handle_t XcodeHandle)
{
    XCODE_Properties_t * XCODE_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    XCODE_Data_p = (XCODE_Properties_t *) XcodeHandle;

    if (XCODE_Data_p->ValidityCheck != TRANSCODEC_XCODE_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(XCODE_Data_p->EventsHandle);

    /* Terminate HAL Decode. */
    (XCODE_Data_p->FunctionsTable_p->Term)(XcodeHandle);

    /* De-validate structure */
    XCODE_Data_p->ValidityCheck = 0; /* not TRANSCODEC_XCODE_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    memory_deallocate(XCODE_Data_p->CPUPartition_p, (void *) XCODE_Data_p);

    return(ST_NO_ERROR);
} /* End of XCODE_Term() function */

/* Private functions -------------------------------------------------------- */





/* End of vidcodec.c */
