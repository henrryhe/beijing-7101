/*******************************************************************************

File name   : vidcodec.c

Description : Video driver CODEC API source file

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
26 Jan 2004        Created                                           HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "vidcodec.h"

#ifdef ST_vcodmpg2
#include "vcodmpg2.h"
#endif /* ST_vcodmpg2 */
#ifdef ST_vcodh264
#include "vcodh264.h"
#endif /* ST_vcodh264 */
#ifdef ST_vcodvc1
#include "vcodvc1.h"
#endif /* ST_vcodvc1 */
#ifdef ST_vcodmpg4p2
#include "vcodmpg4p2.h"
#endif /* ST_vcodmpg4p2 */
#ifdef ST_vcodavs
#include "vcodavs.h"
#endif /* ST_vcodavs */

#include "sttbx.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define VIDCODEC_PARSER_VALID_HANDLE    0x01a51a50
#define VIDCODEC_DECODER_VALID_HANDLE   0x01b51b50

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/* Exported functions ------------------------------------------------------- */


/*******************************************************************************
Name        : PARSER_Init
Description : Init VICODEC parser
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t PARSER_Init(const PARSER_InitParams_t * const InitParams_p, PARSER_Handle_t * const ParserHandle_p)
{
    PARSER_Properties_t * PARSER_Data_p;
    STEVT_OpenParams_t STEVT_OpenParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Allocate a PARSER structure */
    PARSER_Data_p = (PARSER_Properties_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(PARSER_Properties_t));
    if (PARSER_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

#ifdef ST_Mpeg2CodecFromOldDecodeModule
    PARSER_Data_p->VIDDEC_Handle = (void *) (*ParserHandle_p);
#endif
    /* Allocation succeeded: make handle point on structure */
    *ParserHandle_p = (PARSER_Handle_t *) PARSER_Data_p;
    PARSER_Data_p->ValidityCheck = VIDCODEC_PARSER_VALID_HANDLE;

    /* Store parameters */
    PARSER_Data_p->CPUPartition_p         = InitParams_p->CPUPartition_p;
    PARSER_Data_p->DeviceType             = InitParams_p->DeviceType;
    PARSER_Data_p->SDRAMMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;
    PARSER_Data_p->DecoderNumber          = InitParams_p->DecoderNumber;
    PARSER_Data_p->RegistersBaseAddress_p   = InitParams_p->RegistersBaseAddress_p;
    PARSER_Data_p->CompressedDataInputBaseAddress_p = InitParams_p->CompressedDataInputBaseAddress_p;
    strcpy(PARSER_Data_p->VideoName, InitParams_p->VideoName);
    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &(PARSER_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        PARSER_Term(*ParserHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* First initialise pointer to table of function to NULL, then will check if affected. */
    PARSER_Data_p->FunctionsTable_p = NULL;

    switch (PARSER_Data_p->DeviceType)
    {
#ifdef ST_vcodvc1
        case STVID_DEVICE_TYPE_7109_VC1 :
        case STVID_DEVICE_TYPE_7200_VC1 :
            PARSER_Data_p->FunctionsTable_p = &PARSER_VC1Functions;
            break;
#endif /* ST_vcodvc1 */

#ifdef ST_vcodmpg4p2
        case STVID_DEVICE_TYPE_7100_MPEG4P2 :
        case STVID_DEVICE_TYPE_7109_MPEG4P2 :
        case STVID_DEVICE_TYPE_7200_MPEG4P2 :
            PARSER_Data_p->FunctionsTable_p = &PARSER_MPEG4P2Functions;
            break;
#endif /* ST_vcodmpg4p2 */

#ifdef ST_vcodavs
        case STVID_DEVICE_TYPE_7109_AVS :
            PARSER_Data_p->FunctionsTable_p = &PARSER_AVSFunctions;
            break;
#endif /* ST_vcodavs*/

#ifdef ST_vcodh264
        case STVID_DEVICE_TYPE_7100_H264 :
        case STVID_DEVICE_TYPE_7109_H264 :
        case STVID_DEVICE_TYPE_7200_H264 :
            PARSER_Data_p->FunctionsTable_p = &PARSER_H264Functions;
            break;
#endif /* ST_vcodh264 */

#ifdef ST_vcodmpg2
        case STVID_DEVICE_TYPE_7100_MPEG :
        case STVID_DEVICE_TYPE_7109_MPEG :
        case STVID_DEVICE_TYPE_7109D_MPEG :
        case STVID_DEVICE_TYPE_7200_MPEG :
            PARSER_Data_p->FunctionsTable_p = &PARSER_Mpeg2Functions;
            break;
#endif /* ST_vcodmpg2 */

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* switch (PARSER_Data_p->DeviceType) */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialise decoder */
        ErrorCode = (PARSER_Data_p->FunctionsTable_p->Init)(*ParserHandle_p, InitParams_p);
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: allocation failed, undo initialisations done */
        PARSER_Term(*ParserHandle_p);
    }

    return(ErrorCode);
} /* End of PARSER_Init() function */


/*******************************************************************************
Name        : PARSER_Term
Description : Terminate VICODEC parser
Parameters  :
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t PARSER_Term(const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t * PARSER_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    PARSER_Data_p = (PARSER_Properties_t *) ParserHandle;

    if (PARSER_Data_p->ValidityCheck != VIDCODEC_PARSER_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(PARSER_Data_p->EventsHandle);

    /* Terminate HAL Decode. */
    (PARSER_Data_p->FunctionsTable_p->Term)(ParserHandle);

    /* De-validate structure */
    PARSER_Data_p->ValidityCheck = 0; /* not VIDCODEC_PARSER_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    STOS_MemoryDeallocate(PARSER_Data_p->CPUPartition_p, (void *) PARSER_Data_p);

    return(ST_NO_ERROR);
} /* End of PARSER_Term() function */

#ifdef VALID_TOOLS
ST_ErrorCode_t PARSER_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#ifdef ST_vcodmpg4p2
    ErrorCode = MPEG4P2PARSER_RegisterTrace();
#endif /* ST_vcodmpg4p2 */
#ifdef ST_vcodavs
    ErrorCode = AVSPARSER_RegisterTrace();
#endif /* ST_vcodavs */
#ifdef ST_vcodh264
    ErrorCode = H264PARSER_RegisterTrace();
#endif /* ST_vcodh264 */
#ifdef ST_vcodvc1
    ErrorCode = VC1PARSER_RegisterTrace();
#endif /* ST_vcodvc1 */
#ifdef ST_vcodmpg2
    if(ErrorCode == ST_NO_ERROR)
    {
/*        ErrorCode = MPEGPARSER_RegisterTrace(); */
    }
#endif /* ST_vcodmpg2 */
    return(ErrorCode);
}
#endif /* VALID_TOOLS */


/*******************************************************************************
Name        : DECODER_Init
Description : Init VICODEC decoder
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t DECODER_Init(const DECODER_InitParams_t * const InitParams_p, DECODER_Handle_t * const DecoderHandle_p)
{
    DECODER_Properties_t * DECODER_Data_p;
    STEVT_OpenParams_t STEVT_OpenParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Allocate a DECODER structure */
    DECODER_Data_p = (DECODER_Properties_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(DECODER_Properties_t));
    if (DECODER_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

#ifdef ST_Mpeg2CodecFromOldDecodeModule
    DECODER_Data_p->VIDDEC_Handle = (void *) (*DecoderHandle_p);
#endif
    /* Allocation succeeded: make handle point on structure */
    *DecoderHandle_p = (DECODER_Handle_t *) DECODER_Data_p;
    DECODER_Data_p->ValidityCheck = VIDCODEC_DECODER_VALID_HANDLE;

    /* Store parameters */
    DECODER_Data_p->CPUPartition_p          = InitParams_p->CPUPartition_p;
    DECODER_Data_p->RegistersBaseAddress_p  = InitParams_p->RegistersBaseAddress_p;
#if defined(ST_7200)
    memset(DECODER_Data_p->MMETransportName, 0, 32);
    strncpy(DECODER_Data_p->MMETransportName, InitParams_p->MMETransportName, strlen(InitParams_p->MMETransportName));
#endif /* #if defined(ST_7200) ... */
    DECODER_Data_p->DeviceType             = InitParams_p->DeviceType;
    DECODER_Data_p->SDRAMMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;
    DECODER_Data_p->DecoderNumber          = InitParams_p->DecoderNumber;
    strcpy(DECODER_Data_p->VideoName, InitParams_p->VideoName);
    DECODER_Data_p->InterruptLevel         = InitParams_p->SharedItParams.Level;
    DECODER_Data_p->InterruptNumber        = InitParams_p->SharedItParams.Number;
    DECODER_Data_p->InterruptEvt           = InitParams_p->SharedItParams.Event;
    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &(DECODER_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        DECODER_Term(*DecoderHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* First initialise pointer to table of function to NULL, then will check if affected. */
    DECODER_Data_p->FunctionsTable_p = NULL;

    switch (DECODER_Data_p->DeviceType)
    {
#ifdef ST_vcodvc1
        case STVID_DEVICE_TYPE_7109_VC1 :
        case STVID_DEVICE_TYPE_7200_VC1 :
            DECODER_Data_p->FunctionsTable_p = &DECODER_VC1Functions;
            break;
#endif /* ST_vcodvc1 */

#ifdef ST_vcodmpg4p2
        case STVID_DEVICE_TYPE_7100_MPEG4P2 :
        case STVID_DEVICE_TYPE_7109_MPEG4P2 :
        case STVID_DEVICE_TYPE_7200_MPEG4P2 :
            DECODER_Data_p->FunctionsTable_p = &DECODER_MPEG4P2Functions;
            break;
#endif /* ST_vcodmpg4p2 */

#ifdef ST_vcodavs
        case STVID_DEVICE_TYPE_7109_AVS :
            DECODER_Data_p->FunctionsTable_p = &DECODER_AVSFunctions;
            break;
#endif /* ST_vcodavs */

#ifdef ST_vcodh264
        case STVID_DEVICE_TYPE_7100_H264 :
        case STVID_DEVICE_TYPE_7109_H264 :
        case STVID_DEVICE_TYPE_7200_H264 :
            DECODER_Data_p->FunctionsTable_p = &DECODER_H264Functions;
            break;
#endif /* ST_vcodh264 */

#ifdef ST_vcodmpg2
        case STVID_DEVICE_TYPE_7100_MPEG :
        case STVID_DEVICE_TYPE_7109_MPEG :
        case STVID_DEVICE_TYPE_7109D_MPEG :
        case STVID_DEVICE_TYPE_7200_MPEG :
            DECODER_Data_p->FunctionsTable_p = &DECODER_Mpeg2Functions;
            break;
#endif /* ST_vcodmpg2 */

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* switch (DECODER_Data_p->DeviceType) */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialise decoder */
        ErrorCode = (DECODER_Data_p->FunctionsTable_p->Init)(*DecoderHandle_p, InitParams_p);
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: allocation failed, undo initialisations done */
        DECODER_Term(*DecoderHandle_p);
    }

    return(ErrorCode);
} /* End of DECODER_Init() function */


/*******************************************************************************
Name        : DECODER_Term
Description : Terminate VICODEC parser
Parameters  :
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t DECODER_Term(const DECODER_Handle_t DecoderHandle)
{
    DECODER_Properties_t * DECODER_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    DECODER_Data_p = (DECODER_Properties_t *) DecoderHandle;

    if (DECODER_Data_p->ValidityCheck != VIDCODEC_DECODER_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(DECODER_Data_p->EventsHandle);

    /* Terminate HAL Decode. */
    (DECODER_Data_p->FunctionsTable_p->Term)(DecoderHandle);

    /* De-validate structure */
    DECODER_Data_p->ValidityCheck = 0; /* not VIDCODEC_DECODER_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    STOS_MemoryDeallocate(DECODER_Data_p->CPUPartition_p, (void *) DECODER_Data_p);

    return(ST_NO_ERROR);
} /* End of DECODER_Term() function */

#ifdef VALID_TOOLS
ST_ErrorCode_t DECODER_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#ifdef ST_vcodmpg4p2
    ErrorCode = MPEG4P2DECODER_RegisterTrace();
#endif /* ST_vcodmpg4p2 */
#if 0 /* defined(ST_vcodavs) Currently no traces in AVS codec */
    ErrorCode = AVSDECODER_RegisterTrace();
#endif /* ST_vcodavs */
#ifdef ST_vcodh264
    ErrorCode = H264DECODER_RegisterTrace();
#endif /* ST_vcodh264 */
#ifdef ST_vcodvc1
    ErrorCode = VC1DECODER_RegisterTrace();
#endif /* ST_vcodvc1 */
#ifdef ST_vcodmpg2
    if(ErrorCode == ST_NO_ERROR)
    {
/*        ErrorCode = MPEGDECODER_RegisterTrace(); */
    }
#endif /* ST_vcodmpg2 */
    return(ErrorCode);
}
#endif /* VALID_TOOLS */

/* Private functions -------------------------------------------------------- */





/* End of vidcodec.c */
