/*
///
/// @file     : .\h264preproc_transformer.c
///
/// @brief    :
///
/// @par OWNER: Jerome Audu
///
/// @author   : Jerome Audu
///
/// @par SCOPE:
///
/// @date     : 2003-10-02
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///
*/

/* Define to add debug info and traces */
#ifdef STVID_DEBUG_PREPROC
#define STTBX_REPORT
#endif

#if !defined ST_OSLINUX
#include <assert.h>
#include <stlite.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "sttbx.h"

#include "h264preproc_transformer.h"
#include "h264preproc.h"
#include "preprocessor.h"

/* Local Variables ---------------------------------------------------------- */
static U32  NextCommandId = 0;

/* Public functions --------------------------------------------------------- */

/*******************************************************************************
Name        :  H264Preproc_InitTransformer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
ST_ErrorCode_t H264Preproc_InitTransformer (H264Preproc_InitParams_t *H264Preproc_InitParams_p, H264PreprocHandle_t *Handle)
*******************************************************************************/
ST_ErrorCode_t H264Preproc_InitTransformer (H264Preproc_InitParams_t *H264Preproc_InitParams_p, H264PreprocHandle_t *Handle)
{
    H264Preprocessor_Data_t    *H264Preprocessor_Data_p;
    ST_ErrorCode_t              Error;

    H264Preprocessor_Data_p = STOS_MemoryAllocate(H264Preproc_InitParams_p->CPUPartition_p, sizeof(H264Preprocessor_Data_t));
    if(H264Preprocessor_Data_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"insufficient memory for allocation [H264Preprocessor_Data_p] semaphore.\n"));
        return(ST_NO_ERROR);
    }
    *Handle = (H264PreprocHandle_t)H264Preprocessor_Data_p;

    memset(H264Preprocessor_Data_p, 0, sizeof(H264Preprocessor_Data_t));

    H264Preprocessor_Data_p->CPUPartition_p = H264Preproc_InitParams_p->CPUPartition_p;
    H264Preprocessor_Data_p->AvmemPartitionHandle = H264Preproc_InitParams_p->AvmemPartitionHandle;
    H264Preprocessor_Data_p->CallbackUserData = H264Preproc_InitParams_p->CallbackUserData;
    H264Preprocessor_Data_p->Callback = H264Preproc_InitParams_p->Callback;

    Error = H264PreprocHardware_Init(H264Preproc_InitParams_p, &(H264Preprocessor_Data_p->H264PreprocessorHardwareHandle));
    if(Error != ST_NO_ERROR)
    {
        H264Preprocessor_Data_p->H264PreprocessorHardwareHandle = NULL;
        /* free instance local memory */
        STOS_MemoryDeallocate(H264Preprocessor_Data_p->CPUPartition_p, H264Preprocessor_Data_p);
        H264Preprocessor_Data_p = NULL;
        return(ST_ERROR_NO_MEMORY);
    }

    return(ST_NO_ERROR);
} /* end of H264Preproc_InitTransformer */

#if defined(DVD_SECURED_CHIP)
/*******************************************************************************
Name        :  H264Preproc_SetupTransformerWA_GNB42332
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
ST_ErrorCode_t H264Preproc_SetupTransformerWA_GNB42332 (H264PreprocHandle_t Handle, const STVID_SetupParams_t * const SetupParams_p)
*******************************************************************************/
ST_ErrorCode_t H264Preproc_SetupTransformerWA_GNB42332 (H264PreprocHandle_t Handle, const STAVMEM_PartitionHandle_t AVMEMPartition)
{
    H264Preprocessor_Data_t *H264Preprocessor_Data_p = (H264Preprocessor_Data_t *)Handle;
    ST_ErrorCode_t           ErrorCode = ST_NO_ERROR;

    ErrorCode = H264PreprocHardware_SetupReservedPartitionForH264PreprocWA_GNB42332(H264Preprocessor_Data_p->H264PreprocessorHardwareHandle, AVMEMPartition);

    return ErrorCode;

} /* end of H264Preproc_SetupTransformerWA_GNB42332() */
#endif /* defined(DVD_SECURED_CHIP) */

/*******************************************************************************
Name        :  H264Preproc_TermTransformer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
ST_ErrorCode_t H264Preproc_TermTransformer(H264PreprocHandle_t Handle)
*******************************************************************************/
ST_ErrorCode_t H264Preproc_TermTransformer(H264PreprocHandle_t Handle)
{
    H264Preprocessor_Data_t *H264Preprocessor_Data_p = (H264Preprocessor_Data_t *)Handle;
    ST_ErrorCode_t           Error = ST_NO_ERROR;

    if(H264Preprocessor_Data_p->H264PreprocessorHardwareHandle != 0)
    {
        Error = H264PreprocHardware_Term(H264Preprocessor_Data_p->H264PreprocessorHardwareHandle);
        H264Preprocessor_Data_p->H264PreprocessorHardwareHandle = 0;
    }

    if(Error != ST_NO_ERROR)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    /* free instance local memory */
    if(H264Preprocessor_Data_p != NULL)
    {
        STOS_MemoryDeallocate(H264Preprocessor_Data_p->CPUPartition_p, H264Preprocessor_Data_p);
        H264Preprocessor_Data_p = NULL;
    }

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : H264Preproc_ProcessCommand
Description : Push the MME TRANSORM command in an internal command queue.
                    - Allocates/Create a frame_object for containing the parameters
                  relative to the submitted command
                - Fill the frame Object with the command parameters
                - Enqueue the frame_object in the "preProcCmdQueue". This command
                  aueue is Dequeueed as soon as a Preprocessor ressource is available.
                - Signal the S2 semaphore for indicating that a new command has been
                  push in the queue.
Parameters  : H264SingleFramePreprocessor_t : The parent object address
              H264PreprocCommand_t          : The command
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if done successfully
              ST_ERROR_BAD_PARAMETER if failed.
ST_ErrorCode_t H264Preproc_ProcessCommand(H264PreprocHandle_t Handle, H264PreprocCommand_t *H264PreprocCommand_p)
*******************************************************************************/
ST_ErrorCode_t H264Preproc_ProcessCommand(H264PreprocHandle_t Handle, H264PreprocCommand_t *H264PreprocCommand_p)
{
    H264Preprocessor_Data_t   *H264Preprocessor_Data_p = (H264Preprocessor_Data_t *)Handle;

    STOS_TaskLock();
    H264PreprocCommand_p->CmdId = NextCommandId++;
    STOS_TaskUnlock();
    H264PreprocCommand_p->Error = ST_NO_ERROR;

    return(H264PreprocHardware_ProcessPicture(H264Preprocessor_Data_p->H264PreprocessorHardwareHandle,
                                              H264PreprocCommand_p,
                                              H264Preprocessor_Data_p->Callback,
                                              H264Preprocessor_Data_p->CallbackUserData));
} /* end of H264Preproc_ProcessCommand */

#if !defined PERF1_TASK3
/*******************************************************************************
Name        : H264Preproc_IsHardwareBusy
Description : Asks hardware layer to check if an hardware resource is available
              for next command
Parameters  :

Assumptions :
Limitations :
Returns     : ST_NO_ERROR if done successfully
              ST_ERROR_BAD_PARAMETER if failed.
ST_ErrorCode_t H264Preproc_IsHardwareBusy(H264PreprocHandle_t Handle, BOOL *Busy)
*******************************************************************************/
ST_ErrorCode_t H264Preproc_IsHardwareBusy(H264PreprocHandle_t Handle, BOOL *Busy)
{
    H264Preprocessor_Data_t   *H264Preprocessor_Data_p = (H264Preprocessor_Data_t *)Handle;

    return(H264PreprocHardware_IsHardwareBusy(H264Preprocessor_Data_p->H264PreprocessorHardwareHandle, Busy));
}
#endif /* PERF1_TASK3 */

ST_ErrorCode_t H264Preproc_AbortCommand(H264PreprocHandle_t Handle, H264PreprocCommand_t *H264PreprocCommand_p)
{
    H264Preprocessor_Data_t   *H264Preprocessor_Data_p = (H264Preprocessor_Data_t *)Handle;

    return(H264PreprocHardware_AbortCommand(H264Preprocessor_Data_p->H264PreprocessorHardwareHandle, H264PreprocCommand_p));
}

