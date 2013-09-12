/*******************************************************************************

File name   : halv_dis.c

Description : HAL video display source file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
06 Jul 2000        Created                                           HG
01 Feb 2001        Digital Input merge                               JA
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

#include "halv_dis.h"

#ifdef ST_omega1
#include "hv_dis1.h"
#endif /* ST_omega1 */

#ifdef ST_omega2
#include "hv_dis2.h"
#ifdef ST_diginput
#include "hv_di_sd.h"
#include "hv_di_hd.h"
#endif /* ST_diginput */
#endif /* ST_omega2 */

#ifdef ST_sddispo2
#include "hv_dis8.h"
#endif /* ST_sddispo2 */

#ifdef ST_displaypipe
#include "hv_vdp.h"
#endif /* ST_displaypipe */

#if (!defined (ST_sddispo2) && !defined (ST_displaypipe))
#ifdef ST_diginput
#include "hv_gx_di.h"
#endif /* ST_diginput */
#endif /* !defined (ST_sddispo2) && !defined (ST_displaypipe) */

#ifdef ST_blitdisp
#include "blitdisp.h"
#endif

#ifdef ST_graphics
#include "hv_gdp.h"
#endif /* ST_graphics */

#include "sttbx.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

 /* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
 #define HALDISP_VALID_HANDLE 0x0175175f


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : HALDISP_Init
Description : Init display HAL
Parameters  :
Assumptions :
Limitations :
Returns     : in params: HAL display handle
              ST_NO_ERROR if success
              ST_ERROR_NO_MEMORY if no memory
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t HALDISP_Init(const HALDISP_InitParams_t * const InitParams_p,
                            HALDISP_Handle_t * const DisplayHandle_p,
                            const U8 HALDisplayNumber)
{
    HALDISP_Properties_t * HALDISP_Data_p;
    STEVT_OpenParams_t STEVT_OpenParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Allocate a HALDISP structure */
    SAFE_MEMORY_ALLOCATE(HALDISP_Data_p, HALDISP_Properties_t *, InitParams_p->CPUPartition_p, sizeof(HALDISP_Properties_t));
    if (HALDISP_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    /* Initialization of the HALDISP_Data_p fields to 0x0 */
    memset( HALDISP_Data_p, 0x0, sizeof(HALDISP_Properties_t));

    /* Allocation succeeded: make handle point on structure */
    *DisplayHandle_p = (HALDISP_Handle_t *) HALDISP_Data_p;
    HALDISP_Data_p->ValidityCheck = HALDISP_VALID_HANDLE;

    /* Store parameters */
    HALDISP_Data_p->CPUPartition_p         = InitParams_p->CPUPartition_p;
    HALDISP_Data_p->AvmemPartitionHandleForMotionBuffers         = InitParams_p->AvmemPartitionHandleForMotionBuffers;
    HALDISP_Data_p->RegistersBaseAddress_p = InitParams_p->RegistersBaseAddress_p;
    HALDISP_Data_p->DeviceType             = InitParams_p->DeviceType;
    HALDISP_Data_p->DecoderNumber          = InitParams_p->DecoderNumber;
    strcpy(HALDISP_Data_p->VideoName, InitParams_p->VideoName);
#ifdef ST_crc
    HALDISP_Data_p->CRCHandle          = InitParams_p->CRCHandle;
#endif /* ST_crc */

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &(HALDISP_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        HALDISP_Term(*DisplayHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (HALDISP_Data_p->DeviceType)
    {
#ifdef ST_omega1
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
            HALDISP_Data_p->FunctionsTable_p = &HALDISP_Omega1Functions;
            break;
#endif /* ST_omega1 */

#ifdef ST_omega2
        case STVID_DEVICE_TYPE_7015_MPEG :
        case STVID_DEVICE_TYPE_7020_MPEG :
            HALDISP_Data_p->FunctionsTable_p = &HALDISP_Omega2FunctionsMPEG;
            break;
#ifdef ST_diginput
        case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
            /* 7015/7020 - Digital input SD 1 : 5
                         - Digital input SD 2 : 6
                         - Digital input HD 1 : 7 */
            switch (InitParams_p->DecoderNumber)
            {
                case 5:
                case 6:
                    HALDISP_Data_p->FunctionsTable_p = &HALDISP_Omega2FunctionsStdDefinition;
                    break;
                case 7:
                    HALDISP_Data_p->FunctionsTable_p = &HALDISP_Omega2FunctionsHighDefinition;
                    break;
                default:
                    return(ST_ERROR_BAD_PARAMETER);
            }
            break;
#endif /* ST_diginput */
#endif /* ST_omega2 */

#if defined(ST_7100) && defined(NATIVE_CORE)
/* When using 7100 VSOC we have a deltamu so VC1 format is available */
        case STVID_DEVICE_TYPE_7100_MPEG :
        case STVID_DEVICE_TYPE_7100_H264 :
        case STVID_DEVICE_TYPE_7100_MPEG4P2 :
        case STVID_DEVICE_TYPE_7109_VC1 :
            HALDISP_Data_p->FunctionsTable_p = &HALDISP_Sddispo2Functions;
            break;
		 /* default is error for that case */

#endif /* (NATIVE_CORE) && (ST_7100) */

#if defined(ST_sddispo2) && !defined(NATIVE_CORE)
/* When using the chip 7100  */
        case STVID_DEVICE_TYPE_7710_MPEG :
#ifdef ST_diginput
        case STVID_DEVICE_TYPE_7710_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7100_DIGITAL_INPUT :
#endif /* ST_diginput */
        case STVID_DEVICE_TYPE_7100_MPEG :
        case STVID_DEVICE_TYPE_7100_H264 :
        case STVID_DEVICE_TYPE_7100_MPEG4P2 :
        case STVID_DEVICE_TYPE_ZEUS_MPEG :
        case STVID_DEVICE_TYPE_ZEUS_H264 :
        case STVID_DEVICE_TYPE_ZEUS_VC1 :
        case STVID_DEVICE_TYPE_5528_MPEG :
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
        case STVID_DEVICE_TYPE_7109_MPEG :
        case STVID_DEVICE_TYPE_7109D_MPEG :
        case STVID_DEVICE_TYPE_7109_H264 :
        case STVID_DEVICE_TYPE_7109_VC1 :
#endif /* defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */
            HALDISP_Data_p->FunctionsTable_p = &HALDISP_Sddispo2Functions;
            break;
#endif /* ST_sddispo2 */

#if defined(ST_sddispo2) && defined(ST_displaypipe)
/* When using the chip 7109  */
        case STVID_DEVICE_TYPE_7109_MPEG :
        case STVID_DEVICE_TYPE_7109D_MPEG :
        case STVID_DEVICE_TYPE_7109_H264 :
        case STVID_DEVICE_TYPE_7109_VC1 :
        case STVID_DEVICE_TYPE_7109_MPEG4P2 :
        case STVID_DEVICE_TYPE_7109_AVS :
#ifdef ST_diginput
        case STVID_DEVICE_TYPE_7109_DIGITAL_INPUT :
#endif /* ST_diginput */
            switch (HALDisplayNumber)
            {
                case 0 :
                    /* MAIN display */
                    HALDISP_Data_p->FunctionsTable_p = &HALDISP_DisplayPipeFunctions;
                    break;

#ifdef ST_graphics
                case 2 :
                    /* Graphics display */
                    HALDISP_Data_p->FunctionsTable_p = &HALDISP_GraphicsDisplayFunctions;
                    break;
#endif /* ST_graphics */

                default:
                    /* and case HALDisplayNumber == 1 */
                    /* AUX display */
                    HALDISP_Data_p->FunctionsTable_p = &HALDISP_Sddispo2Functions;
                    break;
            }
            break;
#else
            UNUSED_PARAMETER(HALDisplayNumber);
#endif /* ST_sddispo2 && ST_displaypipe */

#if defined(ST_displaypipe)
        case STVID_DEVICE_TYPE_7200_MPEG :
        case STVID_DEVICE_TYPE_7200_H264 :
        case STVID_DEVICE_TYPE_7200_VC1 :
        case STVID_DEVICE_TYPE_7200_MPEG4P2 :
#ifdef ST_diginput
        case STVID_DEVICE_TYPE_7200_DIGITAL_INPUT :
#endif /* ST_diginput */
            HALDISP_Data_p->FunctionsTable_p = &HALDISP_DisplayPipeFunctions;
            break;
#endif /* ST_displaypipe */

#ifdef ST_blitdisp
        case STVID_DEVICE_TYPE_5100_MPEG :
        case STVID_DEVICE_TYPE_5525_MPEG :
        case STVID_DEVICE_TYPE_5105_MPEG :
        case STVID_DEVICE_TYPE_5301_MPEG :
            HALDISP_Data_p->FunctionsTable_p = &HALDISP_BlitterDisplayFunctions;
            break;
#endif /* ST_blitdisp */

#if !defined (ST_sddispo2) && !defined(ST_displaypipe)
#ifdef ST_diginput
        case STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT :
            HALDISP_Data_p->FunctionsTable_p = &HALDISP_FunctionsGenericDigitalInput;
            break;
#endif /* ST_diginput */
#endif /* !defined (ST_sddispo2) && !defined(ST_displaypipe) */

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* switch (HALDISP_Data_p->DeviceType) */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialize display */
        ErrorCode = (HALDISP_Data_p->FunctionsTable_p->Init)(*DisplayHandle_p);
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: allocation failed, undo initializations */
        HALDISP_Term(*DisplayHandle_p);
    }

    return(ErrorCode);
} /* end of HALDISP_Init() function */


/*******************************************************************************
Name        : HALDISP_Term
Description : Terminate display HAL
Parameters  : Display handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t HALDISP_Term(const HALDISP_Handle_t DisplayHandle)
{
    HALDISP_Properties_t * HALDISP_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    HALDISP_Data_p = (HALDISP_Properties_t *) DisplayHandle;

    if (HALDISP_Data_p->ValidityCheck != HALDISP_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(HALDISP_Data_p->EventsHandle);

    /* Terminate HAL Display. */
    (HALDISP_Data_p->FunctionsTable_p->Term)(DisplayHandle);

    /* De-validate structure */
    HALDISP_Data_p->ValidityCheck = 0; /* not HALDISP_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(HALDISP_Data_p, HALDISP_Data_p->CPUPartition_p,  sizeof(HALDISP_Properties_t));

    return(ErrorCode);
}

/* End of halv_dis.c */
