/*******************************************************************************

File name   : producer.c

Description : Video driver producer init source file

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
11 mar 2004        Created                                           PC
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "producer.h"

#ifdef ST_multicodec
#include "vid_prod.h"
#endif /* ST_multicodec */
#ifdef ST_oldmpeg2codec
#include "decode.h"
#endif /* ST_oldmpeg2codec */

#include "sttbx.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/* Exported functions ------------------------------------------------------- */

/*******************************************************************************
Name        : VIDPROD_Init
Description : Init producer module (multicodec or oldmpeg2codec)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDPROD_Init(const VIDPROD_InitParams_t * const InitParams_p, VIDPROD_Handle_t * const ProducerHandle_p)
{
    VIDPROD_Properties_t * VIDPROD_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Allocate a PARSER structure */
    VIDPROD_Data_p = (VIDPROD_Properties_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(VIDPROD_Properties_t));
    if (VIDPROD_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *ProducerHandle_p = (VIDPROD_Handle_t *) VIDPROD_Data_p;
/* TODO_PC (VIDPROD_Init) : Check for ValidityCheck here ? */
 /*    PARSER_Data_p->ValidityCheck = VIDCODEC_PARSER_VALID_HANDLE;*/

    /* Store parameters */
    VIDPROD_Data_p->CPUPartition_p         = InitParams_p->CPUPartition_p;

    /* First initialise pointer to table of function to NULL, then will check if affected. */
    VIDPROD_Data_p->FunctionsTable_p = NULL;

    switch (InitParams_p->DeviceType)
    {
#ifdef ST_multicodec
#ifdef STVID_USE_DMPEG_AS_CODEC
        case STVID_DEVICE_TYPE_7109D_MPEG :
#endif /* STVID_USE_DMPEG_AS_CODEC */
        case STVID_DEVICE_TYPE_7200_MPEG :
        case STVID_DEVICE_TYPE_7100_MPEG4P2 :
        case STVID_DEVICE_TYPE_7109_MPEG4P2 :
	case STVID_DEVICE_TYPE_7109_AVS :	
        case STVID_DEVICE_TYPE_7200_MPEG4P2 :
        case STVID_DEVICE_TYPE_7100_H264 :
        case STVID_DEVICE_TYPE_7109_H264 :
        case STVID_DEVICE_TYPE_7200_H264 :
        case STVID_DEVICE_TYPE_7109_VC1 :
        case STVID_DEVICE_TYPE_7200_VC1 :
            VIDPROD_Data_p->FunctionsTable_p = &VIDPROD_Functions;
            break;
#endif /* ST_multicodec */

#ifdef ST_oldmpeg2codec
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
        case STVID_DEVICE_TYPE_7015_MPEG :
        case STVID_DEVICE_TYPE_7020_MPEG :
        case STVID_DEVICE_TYPE_5528_MPEG :
        case STVID_DEVICE_TYPE_7710_MPEG :
        case STVID_DEVICE_TYPE_5100_MPEG :
        case STVID_DEVICE_TYPE_5525_MPEG :
        case STVID_DEVICE_TYPE_5105_MPEG :
        case STVID_DEVICE_TYPE_5301_MPEG :
        case STVID_DEVICE_TYPE_7100_MPEG :
        case STVID_DEVICE_TYPE_7109_MPEG :
            VIDPROD_Data_p->FunctionsTable_p = &VIDDEC_Functions;
            break;
#endif /* ST_oldmpeg2codec */

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* switch (InitParams_p->DeviceType) */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialise decoder */
        ErrorCode = (VIDPROD_Data_p->FunctionsTable_p->Init)(InitParams_p, &(((VIDPROD_Properties_t *)(*ProducerHandle_p))->InternalProducerHandle));
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: allocation failed, undo initialisations done */
        VIDPROD_Term(*ProducerHandle_p);
    }

    return(ErrorCode);
} /* End of VIDPROD_Init() function */

/*******************************************************************************
Name        : VIDPROD_Term
Description : Terminate producer module (multicodec or oldmpeg2codec)
Parameters  :
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDPROD_Term(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Properties_t * VIDPROD_Data_p;

    /* Find structure */
    VIDPROD_Data_p = (VIDPROD_Properties_t *) ProducerHandle;

/* TODO_PC (VIDPROD_Term) : Check for ValidityCheck here ? */
/*    if (PARSER_Data_p->ValidityCheck != VIDCODEC_PARSER_VALID_HANDLE) */
/*    { */
/*        return(ST_ERROR_INVALID_HANDLE); */
/*    } */

    /* Terminate HAL Decode. */
    (VIDPROD_Data_p->FunctionsTable_p->Term)(VIDPROD_Data_p->InternalProducerHandle);

/* TODO_PC (VIDPROD_Term) : Check for ValidityCheck here ? */
    /* De-validate structure */
/*    PARSER_Data_p->ValidityCheck = 0; *//* not VIDCODEC_PARSER_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    STOS_MemoryDeallocate(VIDPROD_Data_p->CPUPartition_p, (void *) VIDPROD_Data_p);

    return(ST_NO_ERROR);
} /* End of VIDPROD_Term() function */

#ifdef VALID_TOOLS
ST_ErrorCode_t VIDPROD_RegisterTrace(void)
{
    return(vidprod_RegisterTrace());
}
#endif /* VALID_TOOLS */

/* Private functions -------------------------------------------------------- */

/* End of producer.c */
