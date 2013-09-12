/************************************************************************
COPYRIGHT (C) STMicroelectronics 2006

File name   : vid_cmd.c
Description : Definition of video commands
 *            in order to test each function of STVID API 3.0.0 or later
 *            with Testtool
 *
Note        : All functions return TRUE if error for Testtool compatibility
 *
Date          Modification                                      Initials
----          ------------                                      --------
19 Sep 2002   Add functions related to digital input (STVID 3.4.3) FQ
14 Aug 2001   Updated for STVID 3.1.0                              FQ
13 Mar 2001   New functions                                        FQ
04 Dec 2000   Multi-handles                                        FQ
11 Aug 2000   Creation (adaptation of previous vid_test.c)         FQ
************************************************************************/
/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "testcfg.h"
#include "genadd.h"
#include "stvid.h"
#include "stddefs.h"
#include "stdevice.h"


#if defined ST_OSLINUX & defined DEBUG_LOCK
#include "iocstapigat.h"
#endif

#include "stos.h"
#include "sttbx.h"

#include "testtool.h"
#include "clevt.h"
#ifdef USE_CLKRV
#include "clclkrv.h"
#endif
#if defined (ST_7015) || defined(ST_7020)
#include "clintmr.h"
#endif /* 7015 and 7020 */
#include "api_cmd.h"
#include "startup.h"
#include "stsys.h"
#include "stcommon.h"
#ifdef USE_CLKRV
#include "stclkrv.h"
#endif
#ifdef USE_DEMUX
#include "stdemux.h"
#endif /* USE_DEMUX */
#include "vid_inj.h"
#include "vid_util.h"
#include "vid_cmd.h"
#include "lay_cmd.h"

#ifdef STVID_CPULOAD
#include "cpu_load.h"
#endif

#if !defined ST_OSLINUX
#include "clavmem.h" /* needed for MAX_PARTITIONS */
#endif

#ifdef DVD_TRANSPORT_LINK
#include "pti.h"
#endif /* DVD_TRANSPORT_LINK */

#ifndef DELTA_BASE_ADDRESS

#ifdef ST_7100
#define DELTA_BASE_ADDRESS tlm_to_virtual(0xB9214000)

#elif defined(ST_7109)

#ifdef NATIVE_CORE
#define DELTA_BASE_ADDRESS   tlm_to_virtual(0xB9214000)
#else /* NATIVE_CORE */
#define DELTA_BASE_ADDRESS   tlm_to_virtual(0xB9540000)
#endif /* NATIVE_CORE */
#endif /* ST_7100 */

#endif  /* DELTA_BASE_ADDRESS */

#ifndef DELTA_PP0_INTERRUPT_NUMBER
   #if defined(ST_7100)
       #define DELTA_PP0_INTERRUPT_NUMBER ST7100_VID_DPHI_PRE0_INTERRUPT
   #elif defined(ST_7109)
       #define DELTA_PP0_INTERRUPT_NUMBER ST7109_VID_DPHI_PRE0_INTERRUPT
   #elif defined(ST_7200)
       #define DELTA_PP0_INTERRUPT_NUMBER ST7200_VID_DMU0_PRE0_INTERRUPT
       #define DELTA_PP1_INTERRUPT_NUMBER ST7200_VID_DMU1_PRE0_INTERRUPT
   #endif /* ST_7100, ST_7109, ST_7200 */
#endif /* DELTA_PP0_INTERRUPT_NUMBER */



/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

#define VID_PRINT(x) { \
    /* Check lenght */ \
    if (strlen(x) > sizeof(x)) { \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_ErrorPrint((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

#define VID_PRINT_ERROR(x) { \
    /* Check lenght */ \
    if (strlen(x) > sizeof(x)) { \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_ErrorPrint((x)); \
        return(TRUE); \
    } \
    STTBX_ErrorPrint((x)); \
} \

#define  VIDSETUPOBJECTTOSTRING(Type)  \
        (  ((Type) == STVID_SETUP_FRAME_BUFFERS_PARTITION) ? "STVID_SETUP_FRAME_BUFFERS_PARTITION" : \
        (  ((Type) == STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION) ? "STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION" : \
        (  ((Type) == STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION) ? "STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION" : \
        (  ((Type) == STVID_SETUP_FDMA_NODES_PARTITION) ? "STVID_SETUP_FDMA_NODES_PARTITION" : \
        (  ((Type) == STVID_SETUP_ES_COPY_BUFFER_PARTITION) ? "STVID_SETUP_ES_COPY_BUFFER_PARTITION" : \
        (  ((Type) == STVID_SETUP_ES_COPY_BUFFER) ? "STVID_SETUP_ES_COPY_BUFFER" : \
        (  ((Type) == STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION) ? "STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION" : \
        (  ((Type) == STVID_SETUP_DATA_INPUT_BUFFER_PARTITION) ? "STVID_SETUP_DATA_INPUT_BUFFER_PARTITION" : \
        (  ((Type) == STVID_SETUP_BIT_BUFFER_PARTITION) ? "STVID_SETUP_BIT_BUFFER_PARTITION" : \
        (  ((Type) == STVID_SETUP_PICTURE_PARAMETER_BUFFERS_PARTITION) ? "STVID_SETUP_PICTURE_PARAMETER_BUFFERS_PARTITION" :"???"))))))))))


/* --- Constants (default values) -------------------------------------- */
#if defined (ST_7020) && (defined(mb382) || defined(mb314))
#define db573 /* clearer #define for below */
#endif

#define DEFAULT_SCREEN_WIDTH     720         /* nb of horiz. pixels */
#define DEFAULT_SCREEN_HEIGHT    576         /* nb of vert. pixels */
#define DEFAULT_SCREEN_XOFFSET   132         /* time of horiz. pixels */
#define DEFAULT_SCREEN_YOFFSET    22         /* time of vert. pixels */
#define NB_MAX_OF_COPIED_PICTURES  2         /* pictures in memory for show/hide tests */

/* #define BBF_EMI_FRAME_BUFFERS_LMI */ /* to be set if Bit buffer and frame buffers have to be allocated in two */
                                        /* different partitions ! */

#define DELAY_1S                    1000000     /* Expressed in microseconds */

/* --- Global variables ------------------------------------------------ */
STVID_ViewPortHandle_t  ViewPortHndl0;       /* handle for viewport */

STVID_Handle_t  DefaultVideoHandle = (STVID_Handle_t) NULL;

static char VID_Msg[1024];                    /* text for trace */

static STVID_PictureParams_t VID_PictParams;                /* for get picture params & alloc. params */
static STVID_PictureInfos_t  VID_PictInfos;                 /* for get picture infos & alloc. params */
static STVID_DisplayPictureInfos_t  VID_DisplayPictInfos;   /* for get picture infos */
static STVID_PictureParams_t PictParamSrc;
static STVID_PictureParams_t PictParamDest;

STVID_PictureParams_t CopiedPictureParams[NB_MAX_OF_COPIED_PICTURES];

S16  NbOfCopiedPictures;

STEVT_Handle_t  EvtSubHandle;         /* handle for the subscriber */

#ifdef STVID_INJECTION_BREAK_WORKAROUND
extern void PTI_VideoWorkaroundPleaseStopInjection(
    STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData,
    const void *SubscriberData_p);
extern void PTI_VideoWorkaroundThanksInjectionCanGoOn(
    STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData,
    const void *SubscriberData_p);
#endif /* STVID_INJECTION_BREAK_WORKAROUND */

#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
static const char * VhsrcFilterType[] =
{
    "FILTER_A",
    "FILTER_B",
    "FILTER_C",
    "FILTER_D",
    "FILTER_E",
    "FILTER_F",
    "FILTER_CHRX2",
};
#endif /* STVID_DEBUG_GET_DISPLAY_PARAMS */

/* --- Prototype ------------------------------------------------------- */

/* --- Externals ------------------------------------------------------- */

extern BOOL VID_GetEventNumber(STTST_Parse_t *pars_p, STEVT_EventConstant_t *EventNumber);

/*-------------------------------------------------------------------------
 * Function : VID_ProvidedDataToBeInjected
 *            Callback used to inject in CD fifo data given by video driver
 *            This has been implemented for 5508 & 5518 chip only
 *            (due to register missing in video part)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
#if !defined ST_OSLINUX /* 5508 & 5518 chip not supported for LINUX */
static void VID_ProvidedDataToBeInjected(
    STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData,
    const void *SubscriberData_p)
{
#ifdef DVD_TRANSPORT_LINK
    BOOL RetErr;
    STVID_ProvidedDataToBeInjected_t* ProvidedData_p;
#endif /* DVD_TRANSPORT_LINK */
    BOOL DestinationNotFound = FALSE;

    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    if (Reason != CALL_REASON_NOTIFY_CALL)
    {
        return;
    }
    /* Inject buffer for decoder N in CD fifo N */
    /* Actually, only required on 5508 & 5518 */
    switch((U32)SubscriberData_p)
    {

#if defined (ST_5518) || defined (ST_5508)
        case VIDEO_BASE_ADDRESS:
            break;
#endif /* ST_5518 || ST_5508 */
        default :
            DestinationNotFound = TRUE;
            break;
    }

    if (DestinationNotFound)
    {
        STTBX_ErrorPrint(("\nUnexpected event ProvidedDataToBeInjected for this chip !!!\n\n"));
        return;
    }

#ifdef DVD_TRANSPORT_LINK
    ProvidedData_p = (STVID_ProvidedDataToBeInjected_t *) EventData;

    RetErr = pti_video_dma_state(discard_data );
    if (!(RetErr))
    {
        RetErr = pti_video_dma_reset();
    }
    if (!(RetErr))
    {
        RetErr = pti_video_dma_open() ;
    }
    if (!(RetErr))
    {
        RetErr = pti_video_dma_inject((U8*)ProvidedData_p->Address_p,
                                       ProvidedData_p->SizeInBytes);
    }
    if (!(RetErr))
    {
        RetErr = pti_video_dma_synchronize();
    }
    if (!(RetErr))
    {
        RetErr = pti_video_dma_close() ;
    }
    if (RetErr)
    {
        /* Error can be due to a concurrent access to injection */
        STTBX_ErrorPrint(("\nError occured while injecting provided buffer from evt !!!\n\n"));
    }
#else
   UNUSED_PARAMETER(EventData);
#endif /* DVD_TRANSPORT_LINK */
    return;
}
#endif  /* !LINUX */

/*#######################################################################*/
/*###################### VIDEO REGISTER COMMANDS ########################*/
/*######################  (in alphabetic order)  ########################*/
/*#######################################################################*/


/*-------------------------------------------------------------------------
 * Function : VID_Clear
 *            Clears the display by changing the currently displayed picture
 *            with a picture filled with a pattern or by freeing the currently
 *            displayed picture buffer
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_Clear(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    STVID_ClearParams_t ClearParams;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* --- get Handle --- */

    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
            return(TRUE);
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* ------ get Clearmode ----- */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ClearMode (0=Black, 1=Pattern, 2=FreeDispBuffer)" );
            return(TRUE);
        }
    }

    switch(LVar)
    {
        case 0:
            ClearParams.ClearMode = STVID_CLEAR_DISPLAY_BLACK_FILL;
            break;
        case 1:
            ClearParams.ClearMode = STVID_CLEAR_DISPLAY_PATTERN_FILL;
            break;
        case 2:
        default:
            ClearParams.ClearMode = STVID_CLEAR_FREEING_DISPLAY_FRAME_BUFFER;
            break;
    }
    /* ----- get Luma ------- */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Y pattern address" );
        return(TRUE);
    }
    ClearParams.PatternAddress1_p = (void*)LVar;
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Y pattern size" );
        return(TRUE);
    }
    ClearParams.PatternSize1      = LVar;

    /* ----- get C ------- */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected C pattern address" );
        return(TRUE);
    }
    ClearParams.PatternAddress2_p = (void*)LVar;
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected C pattern size" );
        return(TRUE);
    }
    ClearParams.PatternSize2      = LVar;

    ErrCode = STVID_Clear(CurrentHandle, &ClearParams);

    sprintf(VID_Msg, "STVID_Clear(handle=0x%x,Mode=%s) : ",
                 CurrentHandle,(ClearParams.ClearMode == STVID_CLEAR_DISPLAY_BLACK_FILL)   ? "BLACK_FILL" :
                               (ClearParams.ClearMode == STVID_CLEAR_DISPLAY_PATTERN_FILL) ? "PATTERN_FILL" : "FREEING");
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            break ;
        case STVID_ERROR_DECODER_RUNNING :
            API_ErrorCount++;
            RetErr = TRUE;
            strcat(VID_Msg, "decoder not stopped !\n" );
            break ;
        case ST_ERROR_BAD_PARAMETER :
            API_ErrorCount++;
            RetErr = TRUE;
            strcat(VID_Msg, "bad parameter !\n" );
            break ;
        case ST_ERROR_INVALID_HANDLE :
            API_ErrorCount++;
            RetErr = TRUE;
            strcat(VID_Msg, "invalid handle !\n" );
            break ;
         case STVID_ERROR_MEMORY_ACCESS :
            API_ErrorCount++;
            RetErr = TRUE;
            strcat(VID_Msg, "memory access error !\n" );
            break ;
         case STVID_ERROR_NOT_AVAILABLE :
            API_ErrorCount++;
            RetErr = TRUE;
            strcat(VID_Msg, "video error not available !\n" );
            break ;
         default :
            API_ErrorCount++;
            RetErr = TRUE;
            VID_PRINT((VID_Msg ));
            sprintf(VID_Msg, "unexpected error [0x%X] !\n", ErrCode );
            break ;
    }

    if(RetErr == FALSE)
    {
        VID_PRINT((VID_Msg ));
    }
    else
    {
        VID_PRINT_ERROR((VID_Msg ));
    }


    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );
}/* end VID_Clear() */

/*-------------------------------------------------------------------------
 * Function : VID_Close
 *            Close the decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Close(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    BOOL HandleFound;
    BOOL found = FALSE;
    U32 inj_idx = 0;

    ErrCode = ST_ERROR_INVALID_HANDLE;
    RetErr = FALSE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
        while ((!found) && (inj_idx < VIDEO_MAX_DEVICE))
        {
            if ( VID_Inj_GetDriverHandle(inj_idx) == CurrentHandle)
            {
                /* Resets total bit buffersize record for this handle */
                VID_Inj_ResetBitBufferSize(inj_idx);
                found = TRUE;
            }
            inj_idx++;
        }
        if (!found)
        {
            STTBX_ErrorPrint(("Error: impossible to find stream injection context associated to this video handle !!!\n"));
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
        /* Resets total bit buffersize record for this handle */
        VID_Inj_ResetBitBufferSize(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_Close(handle=0x%x) : ", CurrentHandle );

        ErrCode = STVID_Close(CurrentHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );

                HandleFound = FALSE;
                LVar = 0;
                while ((LVar <= VIDEO_MAX_DEVICE) && (!HandleFound))
                {
                    if ( VID_Inj_GetDriverHandle(LVar) == CurrentHandle)
                    {
                        HandleFound = TRUE;
                        VID_Inj_ResetDeviceAndHandle(LVar);
                    }
                    else
                    {
                        LVar ++;
                    }
                }

                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_Close() */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_CloseViewPort
 *            Close the viewport
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_CloseViewPort(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    S32 LVar;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_CloseViewPort(viewportHndl=0x%x) : ", (S32) CurrentViewportHndl );
        ErrCode = STVID_CloseViewPort(CurrentViewportHndl );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_CloseViewPort() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_ConfigureEvent
 *            Configure video driver event notification
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_ConfigureEvent(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL            RetErr;
    ST_ErrorCode_t  ErrCode;
/*    STEVT_EventID_t EventID;*/
    STVID_Handle_t  CurrentHandle;
    STEVT_EventConstant_t EvtNo;
    STVID_ConfigureEventParams_t ConfigureParams;
    S32 LVar;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    /* Get event name or number, change it into a number */
    RetErr = VID_GetEventNumber(pars_p, &EvtNo);
    if (RetErr)
    {
        return(API_EnableError ? TRUE : FALSE );
    }

    RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
    ConfigureParams.Enable = (BOOL) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected enable/disable flag (TRUE=notify FALSE=don't notify)" );
    }

    /* nb of event to skip before being notified */
    RetErr = STTST_GetInteger(pars_p, 0, &LVar );
    ConfigureParams.NotificationsToSkip = (U32) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected nb of event to skip before being notified (0 to 0xffffffff, 0=no skip)" );
    }

    if (!(RetErr))
    {
        ErrCode = STVID_ConfigureEvent (CurrentHandle, (STEVT_EventID_t) (EvtNo), &ConfigureParams);
        sprintf(VID_Msg, "STVID_ConfigureEvent(Handle=0x%x,Event=0x%x,Enable=%d,NotificationToSkip=%d) : ",
                    CurrentHandle, EvtNo, ConfigureParams.Enable, ConfigureParams.NotificationsToSkip );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_ConfigureEvent() */

/*-------------------------------------------------------------------------
 * Function : VID_DataInjectionCompleted
 *            Informs the driver that data injection is completed
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DataInjectionCompleted(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    STVID_DataInjectionCompletedParams_t DInjParams;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get TransferRelatedToPrevious */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, FALSE, &LVar );
        DInjParams.TransferRelatedToPrevious = (BOOL) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected TransferRelatedToPrevious (TRUE/FALSE)" );
        }
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_DataInjectionCompleted(Hndl=0x%x,transfer=%d) : ",
                (S32)CurrentHandle, DInjParams.TransferRelatedToPrevious );

        ErrCode = STVID_DataInjectionCompleted(CurrentHandle, &DInjParams );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_DataInjectionCompleted() */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_DisableBorderAlpha
 *            Disable the viewport border alpha
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DisableBorderAlpha(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "VID_DisableBorderAlpha(viewportHndl=0x%x) : ", (S32) CurrentViewportHndl );
        ErrCode = STVID_DisableBorderAlpha(CurrentViewportHndl );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED:
                API_ErrorCount++;
                strcat(VID_Msg, "Feature is not supported !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE:
                API_ErrorCount++;
                strcat(VID_Msg, "Feature is not available !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_DisableBorderAlpha() */
/*-------------------------------------------------------------------------
 * Function : VID_DisableColorKey
 *            Disable the color keying
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DisableColorKey(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_DisableColorKey(viewportHndl=0x%x) : ", (S32) CurrentViewportHndl );

        ErrCode = STVID_DisableColorKey(CurrentViewportHndl );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_DisableColorKey() */

/*-------------------------------------------------------------------------
 * Function : VID_DisableFrameRateConversion
 *            Disable frame rate conversion
 * Input    :  *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DisableFrameRateConversion(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
    }
    else
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "VID_DisableFrameRateConversion(Hndl=0x%x) : ", (S32) CurrentHandle );
        ErrCode = STVID_DisableFrameRateConversion(CurrentHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );
} /* end VID_DisableFrameRateConversion() */

/*-------------------------------------------------------------------------
 * Function : VID_DisableOutputWindow
 *            Disable the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DisableOutputWindow(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_DisableOutputWindow(viewportHndl=0x%x) : ", (S32) CurrentViewportHndl );
        ErrCode = STVID_DisableOutputWindow(CurrentViewportHndl );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_DisableOutputWindow() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_DisableSynchronisation
 *            Disable synchronisation with PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DisableSynchronisation(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;
    RetErr = FALSE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_DisableSynchronisation(handle=0x%x) : ", CurrentHandle );
        ErrCode = STVID_DisableSynchronisation(CurrentHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_DisableSynchronisation() */

/*-------------------------------------------------------------------------
 * Function : VID_DisplayPictureBuffer
 *            Gives a picture buffer to display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DisplayPictureBuffer(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    STVID_PictureBufferHandle_t CurrentPictBuffHandle;
    STVID_PictureInfos_t  PictureInfo;
    U32 CurrentAddress1, CurrentAddress2;
    U32 CurrentSize1, CurrentSize2;
    S32 LVar;

    ErrCode = ST_ERROR_INVALID_HANDLE;
    RetErr = FALSE;
    CurrentPictBuffHandle = 0;
    CurrentAddress1 = 0;
    CurrentAddress2 = 0;
    CurrentSize1 = 0;
    CurrentSize2 = 0;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get picture buffer handle */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        CurrentPictBuffHandle = (STVID_PictureBufferHandle_t)LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
    }
    /* get picture address */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        CurrentAddress1 = (U32)LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Address1 of frame buffer" );
        }
    }
    /* get picture size */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        CurrentSize1 = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected picture size1 (bytes)" );
        }
    }
    /* get picture address */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        CurrentAddress2 = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Address2 of frame buffer" );
        }
    }
    /* get picture size */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        CurrentSize2 = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected picture size2 (bytes)" );
        }
    }

    memset(&PictureInfo, 0, sizeof(PictureInfo));
    /* Caution : as there are too many parameters to enter on command line, */
    /*           this code has been restricted on a 720x576 yuv picture !   */
    /*           (to be improved later to allow more complete tests,        */
    /*           it should depend on previous vid_restore and vid_getpbuff) */

    PictureInfo.BitmapParams.ColorType = STVID_COLOR_TYPE_YUV420;
    PictureInfo.BitmapParams.BitmapType = STGXOBJ_BITMAP_TYPE_MB;
    PictureInfo.BitmapParams.PreMultipliedColor = FALSE;
    PictureInfo.BitmapParams.ColorSpaceConversion = STGXOBJ_COLOR_KEY_TYPE_RGB888;  /* 4:2:2 */
    PictureInfo.BitmapParams.AspectRatio = STVID_DISPLAY_ASPECT_RATIO_SQUARE;
    PictureInfo.BitmapParams.Width = 720;
    PictureInfo.BitmapParams.Height = 576;
    PictureInfo.BitmapParams.Pitch = 720; /*FrameWidth*/
    PictureInfo.BitmapParams.Offset = 0;
    PictureInfo.BitmapParams.Data1_p = (void *)CurrentAddress1;
    PictureInfo.BitmapParams.Size1 = CurrentSize1 ;
    PictureInfo.BitmapParams.Data2_p = (void *)CurrentAddress2;
    PictureInfo.BitmapParams.Size2 = CurrentSize2;
    PictureInfo.BitmapParams.SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
    PictureInfo.BitmapParams.BigNotLittle = 1;  /* little indian */
    /****** value found during debug on sti5505.yuv
    PictureInfo.CodingMode = STVID_CODING_MODE_MB;
    PictureInfo.FrameRate = 25000;
    PictureInfo.MPEGFrame = STVID_MPEG_FRAME_I;
    PictureInfo.TopfieldFirst = FALSE;
    PictureInfo.TimeCode.Hours = 1;
    PictureInfo.TimeCode.Minute = 32;
    PictureInfo.TimeCode.Interpolated = FALSE;
    PictureInfo.PTS = 7200;
    PictureInfo.PTS33 = 0;
    PictureInfo.Interpolated = TRUE;
    PictureInfo.pChromaOffset = 2088960;
    PictureInfo.pTemporalReference = 20224; ******/
    PictureInfo.VideoParams.FrameRate = 25000;
    PictureInfo.VideoParams.ScanType = STVID_SCAN_TYPE_PROGRESSIVE;
    PictureInfo.VideoParams.MPEGFrame = STVID_MPEG_FRAME_I;
    PictureInfo.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
    PictureInfo.VideoParams.TopFieldFirst = FALSE;
    PictureInfo.VideoParams.CompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
    PictureInfo.VideoParams.DecimationFactors = STVID_DECIMATION_FACTOR_H2 /*STVID_DECIMATION_FACTOR_NONE*/;
    /*PictureBuffer_p->PictureInfos.VideoParams.TimeCode = VIDINP_Data_p->ForTask.TimeCode;*/



    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "VID_DisplayPictureBuffer(handle=0x%x,pictbuffhndl=0x%x,&pictinfo=0x%x) : ",
                CurrentHandle, (int)CurrentPictBuffHandle, (int)&PictureInfo );
        ErrCode = STVID_DisplayPictureBuffer(CurrentHandle, CurrentPictBuffHandle, &PictureInfo );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
        sprintf(VID_Msg, "   bitmap : ColorType=%d BitmapType=%d PreMultipliedColor=%d ColorSpaceConversion=%d \n",
                PictureInfo.BitmapParams.ColorType,
                PictureInfo.BitmapParams.BitmapType,
                PictureInfo.BitmapParams.PreMultipliedColor,
                PictureInfo.BitmapParams.ColorSpaceConversion);
        VID_PRINT((VID_Msg ));
        sprintf(VID_Msg, "   AspectRatio=%d Width=%d Height=%d Pitch=%d Offset=%d Data1=0x%x Size1=%d\n",
                PictureInfo.BitmapParams.AspectRatio,
                PictureInfo.BitmapParams.Width,
                PictureInfo.BitmapParams.Height,
                PictureInfo.BitmapParams.Pitch,
                PictureInfo.BitmapParams.Offset,
                (int)PictureInfo.BitmapParams.Data1_p,
                PictureInfo.BitmapParams.Size1);
        VID_PRINT((VID_Msg ));
        sprintf(VID_Msg, "   Data2=0x%x Size2=%d SubByteFormat=%d BigNotLittle=%d\n",
                (int)PictureInfo.BitmapParams.Data2_p,
                PictureInfo.BitmapParams.Size2,
                PictureInfo.BitmapParams.SubByteFormat,
                PictureInfo.BitmapParams.BigNotLittle);
        VID_PRINT((VID_Msg ));
        sprintf(VID_Msg, "   FrameRate=%d ScanType=%d MPEGFrame=%d PictStruct=%d \n",
                PictureInfo.VideoParams.FrameRate,
                PictureInfo.VideoParams.ScanType,
                PictureInfo.VideoParams.MPEGFrame,
                PictureInfo.VideoParams.PictureStructure);
        VID_PRINT((VID_Msg ));
        sprintf(VID_Msg, "   video : TopFieldFirst=%d Hours=%d Minutes=%d Seconds=%d Frames=%d Interpolated=%d\n",
                PictureInfo.VideoParams.TopFieldFirst,
                PictureInfo.VideoParams.TimeCode.Hours,
                PictureInfo.VideoParams.TimeCode.Minutes,
                PictureInfo.VideoParams.TimeCode.Seconds,
                PictureInfo.VideoParams.TimeCode.Frames,
                PictureInfo.VideoParams.TimeCode.Interpolated);
        VID_PRINT((VID_Msg ));
        sprintf(VID_Msg, "   PTS=%d PTS33=%d Interpolated=%d CompressionLevel=%d DecimationFactors=%d \n",
                PictureInfo.VideoParams.PTS.PTS,
                PictureInfo.VideoParams.PTS.PTS33,
                PictureInfo.VideoParams.PTS.Interpolated,
                PictureInfo.VideoParams.CompressionLevel,
                PictureInfo.VideoParams.DecimationFactors);
        VID_PRINT((VID_Msg ));
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_DisplayPictureBuffer() */


/*-------------------------------------------------------------------------
 * Function : VID_GetDisplayPictureInfo
 *            Get a picture information
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetDisplayPictureInfo(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    STVID_PictureBufferHandle_t CurrentPictBuffHandle;
    S32 LVar;
    U8      panandscan;
    ErrCode = ST_ERROR_INVALID_HANDLE;
    RetErr = FALSE;
    CurrentPictBuffHandle = 0;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get picture buffer handle */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        CurrentPictBuffHandle = (STVID_PictureBufferHandle_t)LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetDisplayPictureInfo(CurrentHandle, CurrentPictBuffHandle, &VID_DisplayPictInfos );
        sprintf(VID_Msg, "STVID_GetDisplayPictureInfo(handle=0x%x,pictbufhandle=%d,&params=0x%x) : ",
                 CurrentHandle, (U32)CurrentPictBuffHandle, (U32)&VID_DisplayPictInfos );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                VID_PRINT((VID_Msg ));
               for (panandscan = 0; panandscan < STVID_MAX_NUMBER_OF_PAN_AND_SCAN; panandscan++)
               {
                sprintf(VID_Msg, " PanAndScanIn16thPixel\n   =============\n") ;
                sprintf(VID_Msg, "%s   FrameCentreHorizontalOffset=%d FrameCentreVerticalOffset=%d DisplayHorizontalSize=%d\n ",
                         VID_Msg, VID_DisplayPictInfos.PanAndScanIn16thPixel[panandscan].FrameCentreHorizontalOffset,
                         VID_DisplayPictInfos.PanAndScanIn16thPixel[panandscan].FrameCentreVerticalOffset, VID_DisplayPictInfos.PanAndScanIn16thPixel[panandscan].DisplayHorizontalSize) ;
                sprintf(VID_Msg, "%s   DisplayVerticalSize=%d HasDisplaySizeRecommendation=%d\n",
                         VID_Msg, VID_DisplayPictInfos.PanAndScanIn16thPixel[panandscan].DisplayVerticalSize,
                         VID_DisplayPictInfos.PanAndScanIn16thPixel[panandscan].HasDisplaySizeRecommendation) ;
                 VID_PRINT((VID_Msg ));
                }

                sprintf(VID_Msg, "   FrameCropInPixel\n   =============\n") ;
                sprintf(VID_Msg, "%s   LeftOffset=%d RightOffset=%d TopOffset=%d BottomOffset=%d\n",
                         VID_Msg, VID_DisplayPictInfos.FrameCropInPixel.LeftOffset,
                         VID_DisplayPictInfos.FrameCropInPixel.RightOffset, VID_DisplayPictInfos.FrameCropInPixel.TopOffset,
                         VID_DisplayPictInfos.FrameCropInPixel.BottomOffset) ;
                VID_PRINT((VID_Msg ));

                sprintf(VID_Msg, "    NumberOfPanAndScan\n   =============\n") ;
                sprintf(VID_Msg, "%s   NumberOfPanAndScan=%d\n",
                         VID_Msg, VID_DisplayPictInfos.NumberOfPanAndScan );

                STTST_AssignInteger("RET_VAL1", VID_DisplayPictInfos.PanAndScanIn16thPixel[0].FrameCentreHorizontalOffset, FALSE);
                STTST_AssignInteger("RET_VAL2", VID_DisplayPictInfos.PanAndScanIn16thPixel[0].FrameCentreVerticalOffset, FALSE);
                STTST_AssignInteger("RET_VAL3", VID_DisplayPictInfos.PanAndScanIn16thPixel[0].DisplayHorizontalSize, FALSE);
                STTST_AssignInteger("RET_VAL4", VID_DisplayPictInfos.PanAndScanIn16thPixel[0].DisplayVerticalSize, FALSE);
                STTST_AssignInteger("RET_VAL5", VID_DisplayPictInfos.FrameCropInPixel.LeftOffset, FALSE);
                STTST_AssignInteger("RET_VAL6", VID_DisplayPictInfos.FrameCropInPixel.RightOffset, FALSE);
                STTST_AssignInteger("RET_VAL7", VID_DisplayPictInfos.FrameCropInPixel.TopOffset, FALSE);
                STTST_AssignInteger("RET_VAL8", VID_DisplayPictInfos.FrameCropInPixel.BottomOffset, FALSE);

                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING :
                API_ErrorCount++;
                strcat(VID_Msg, "can't retrieve info. while decoding !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "no picture of the required picture type available !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetDisplayPictureInfo() */


/*-------------------------------------------------------------------------
 * Function : VID_DuplicatePicture
 *            Copy a picture to a user defined area
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DuplicatePicture(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    RetErr = TRUE;
    UNUSED_PARAMETER(pars_p );
    sprintf(VID_Msg, "STVID_DuplicatePicture(src=0x%x,dest=0x%x) : ", (S32)&PictParamSrc, (S32)&PictParamDest );
    ErrCode = STVID_DuplicatePicture(&PictParamSrc, &PictParamDest );
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            break;
        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(VID_Msg, "one parameter is invalid !\n" );
            break;
        case STVID_ERROR_MEMORY_ACCESS:
            API_ErrorCount++;
            strcat(VID_Msg, "memory access problem !\n" );
            break;
        default:
            API_ErrorCount++;
            sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
            break;
    } /* end switch */
    if(RetErr == FALSE)
    {
        VID_PRINT((VID_Msg ));
    }
    else
    {
        VID_PRINT_ERROR((VID_Msg ));
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_DuplicatePicture() */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_EnableBorderAlpha
 *            Enable the viewport border alpha
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_EnableBorderAlpha(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "VID_EnableBorderAlpha(viewportHndl=0x%x) : ", (S32) CurrentViewportHndl );
        ErrCode = STVID_EnableBorderAlpha(CurrentViewportHndl );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE:
                API_ErrorCount++;
                strcat(VID_Msg, "Feature is not available !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED:
                API_ErrorCount++;
                strcat(VID_Msg, "Feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_EnableBorderAlpha() */

/*-------------------------------------------------------------------------
 * Function : VID_EnableColorKey
 *            Enable the color key
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_EnableColorKey(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_EnableColorKey(viewportHndl=0x%x) : ", (S32) CurrentViewportHndl );

        ErrCode = STVID_EnableColorKey(CurrentViewportHndl );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_EnableColorKey() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_EnableFrameRateConversion
 *            Enable frame rate conversion
 * Input    :  *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_EnableFrameRateConversion(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
    }
    else
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "VID_EnableFrameRateConversion(Hndl=0x%x) : ", (S32) CurrentHandle );
        ErrCode = STVID_EnableFrameRateConversion(CurrentHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );
} /* end VID_EnableFrameRateConversion() */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_EnableOutputWindow
 *            Enable the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_EnableOutputWindow(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_EnableOutputWindow(viewportHndl=0x%x) : ", (S32) CurrentViewportHndl );
        ErrCode = STVID_EnableOutputWindow(CurrentViewportHndl );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_EnableOutputWindow() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_EnableSynchronisation
 *            Enable synchronisation with PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_EnableSynchronisation(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_EnableSynchronisation(handle=0x%x) : ", CurrentHandle );
        ErrCode = STVID_EnableSynchronisation(CurrentHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_EnableSynchronisation() */

/*-------------------------------------------------------------------------
 * Function : VID_ForceDecimationFactor
 *
 *
 *
 * Input    :
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_ForceDecimationFactor(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    STVID_ForceDecimationFactorParams_t ForceDecimationFactorParams;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* --- get Handle --- */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
            return(TRUE);
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    /* ------ get ForceMode ----- */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ForceMode (0=Disabled, 1=Enabled)" );
            return(TRUE);
        }
    }
    switch(LVar)
    {
        case 1:
            ForceDecimationFactorParams.ForceMode = STVID_FORCE_MODE_ENABLED;
            break;
        case 0:
        default:
            ForceDecimationFactorParams.ForceMode = STVID_FORCE_MODE_DISABLED;
            break;
    }

    /* ----- get DecimationFactor ------- */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected DecimationFactor (0=NONE, 1=H2, 2=V2, 3=H2V2, 4=H4, 8=V4, 12=H4V4)" );
            return(TRUE);
        }
    }
    ForceDecimationFactorParams.DecimationFactor = (STVID_DecimationFactor_t)LVar;

    /* ----- call STVID_ForceDecimationFactor ------- */
    ErrCode = STVID_ForceDecimationFactor(CurrentHandle, &ForceDecimationFactorParams);

    sprintf(VID_Msg, "STVID_ForceDecimationFactor(handle=0x%x, Mode=%d, Decim=%d) : ",
            CurrentHandle, ForceDecimationFactorParams.ForceMode, ForceDecimationFactorParams.DecimationFactor);

    /* ----- display errors return ------- */
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            break ;
            /* TODO */
         default :
            API_ErrorCount++;
            RetErr = TRUE;
            strcat(VID_Msg, "unknown error !\n" );
            break ;
    }

    if(RetErr == FALSE)
    {
        VID_PRINT((VID_Msg ));
    }
    else
    {
        VID_PRINT_ERROR((VID_Msg ));
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );
}/* end VID_ForceDecimationFactor() */

/*-------------------------------------------------------------------------
 * Function : VID_ForceDecimationFactor
 *
 *
 *
 * Input    :
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetDecimationFactor(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    STVID_DecimationFactor_t DecimationFactor;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* --- get Handle --- */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
            return(TRUE);
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    /* ----- call STVID_ForceDecimationFactor ------- */
    ErrCode = STVID_GetDecimationFactor(CurrentHandle, &DecimationFactor);

    sprintf(VID_Msg, "STVID_GetDecimationFactor(handle=0x%x, Decimation=%d) : ",
            CurrentHandle, DecimationFactor);

    /* ----- display errors return ------- */
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            STTST_AssignInteger("RET_VAL1", DecimationFactor, FALSE);
            break ;
            /* TODO */
         default :
            API_ErrorCount++;
            RetErr = TRUE;
            strcat(VID_Msg, "unknown error !\n" );
            break ;
    }

    if(RetErr == FALSE)
    {
        VID_PRINT((VID_Msg ));
    }
    else
    {
        VID_PRINT_ERROR((VID_Msg ));
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );
}/* end VID_GetDecimationFactor() */

/*-------------------------------------------------------------------------
 * Function : VID_Freeze
 *            Freeze the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Freeze(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_Freeze_t Freeze;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get freeze mode */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_FREEZE_MODE_FORCE, &LVar );
        Freeze.Mode = (STVID_FreezeMode_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected Mode (%d=none %d=force %d=no_flicker)" ,
                        STVID_FREEZE_MODE_NONE, STVID_FREEZE_MODE_FORCE, STVID_FREEZE_MODE_NO_FLICKER );
            STTST_TagCurrentLine(pars_p, VID_Msg);
        }
    }
    /* get freeze field */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_FREEZE_FIELD_TOP, &LVar );
        Freeze.Field = (STVID_FreezeField_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected Field (%d=top %d=bot %d=current %d=next)" ,
                    STVID_FREEZE_FIELD_TOP, STVID_FREEZE_FIELD_BOTTOM,
                    STVID_FREEZE_FIELD_CURRENT, STVID_FREEZE_FIELD_NEXT );
            STTST_TagCurrentLine(pars_p, VID_Msg);
        }
    }

    if (!(RetErr))
    {
        ErrCode = STVID_Freeze(CurrentHandle, &Freeze );
        sprintf(VID_Msg, "STVID_Freeze(handle=0x%x,(&freeze=x%x)) : ",
                 CurrentHandle, (int)&Freeze );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case STVID_ERROR_DECODER_FREEZING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is freezing !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_STOPPED:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is stopped !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            case STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE:
                API_ErrorCount++;
                strcat(VID_Msg, "impossible with memory profile !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        /* Explicit trace */
        sprintf(VID_Msg, "%s   freeze.mode=%d ", VID_Msg, Freeze.Mode);
        switch(Freeze.Mode)
        {
            case STVID_FREEZE_MODE_NONE :
                strcat(VID_Msg, "(STVID_FREEZE_MODE_NONE)\n");
                break;
            case STVID_FREEZE_MODE_FORCE :
                strcat(VID_Msg, "(STVID_FREEZE_MODE_FORCE)\n");
                break;
            case STVID_FREEZE_MODE_NO_FLICKER :
                strcat(VID_Msg, "(STVID_FREEZE_MODE_NO_FLICKER)\n");
                break;
            default:
                strcat(VID_Msg, "(UNDEFINED !)\n");
                break;
        } /* end switch */
        sprintf(VID_Msg, "%s   freeze.field=%d ", VID_Msg, Freeze.Field);
        switch(Freeze.Field)
        {
            case STVID_FREEZE_FIELD_TOP :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_TOP)\n");
                break;
            case STVID_FREEZE_FIELD_BOTTOM :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_BOTTOM)\n");
                break;
            case STVID_FREEZE_FIELD_CURRENT :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_CURRENT)\n");
                break;
            case STVID_FREEZE_FIELD_NEXT :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_NEXT)\n");
                break;
            default:
                strcat(VID_Msg, "(UNDEFINED !)\n");
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_Freeze() */

#ifndef STVID_NO_DISPLAY
/*------------------------------------------------------------------------
 * Function : VID_GetAlignIOWindows
 *            Get the closiest window from the specified one
 *            matching hardware constraints
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetAlignIOWindows(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    S32 InputWinPositionX;
    S32 InputWinPositionY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    S32 OutputWinPositionX;
    S32 OutputWinPositionY;
    U32 OutputWinWidth;
    U32 OutputWinHeight;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    S32 Trace = TRUE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        Trace = (BOOL) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Trace (TRUE/FALSE)" );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetAlignIOWindows(CurrentViewportHndl,
                                           &InputWinPositionX, &InputWinPositionY,
                                           &InputWinWidth, &InputWinHeight,
                                           &OutputWinPositionX, &OutputWinPositionY,
                                           &OutputWinWidth, &OutputWinHeight );

        sprintf(VID_Msg, "STVID_GetAlignIOWindows(viewportHndl=0x%x,ix=%d,iy=%d,iw=%d,ih=%d,ox=%d,oy=%d,ow=%d,oh=%d) : ",
                 (U32)CurrentViewportHndl,
                 InputWinPositionX, InputWinPositionY, InputWinWidth, InputWinHeight,
                 OutputWinPositionX, OutputWinPositionY, OutputWinWidth, OutputWinHeight );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", InputWinPositionX, FALSE);
                STTST_AssignInteger("RET_VAL2", InputWinPositionY, FALSE);
                STTST_AssignInteger("RET_VAL3", InputWinWidth, FALSE);
                STTST_AssignInteger("RET_VAL4", InputWinHeight, FALSE);
                STTST_AssignInteger("RET_VAL5", OutputWinPositionX, FALSE);
                STTST_AssignInteger("RET_VAL6", OutputWinPositionY, FALSE);
                STTST_AssignInteger("RET_VAL7", OutputWinWidth, FALSE);
                STTST_AssignInteger("RET_VAL8", OutputWinHeight, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "information not available (no stream to display) !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if (Trace)
        {
            if(RetErr == FALSE)
            {
                VID_PRINT((VID_Msg ));
            }
            else
            {
                VID_PRINT_ERROR((VID_Msg ));
            }
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetAlignIOWindows() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*------------------------------------------------------------------------
 * Function : VID_GetBitBufferFreeSize
 *            Get the free memory size of bit buffer
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetBitBufferFreeSize(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    U32 BBFreeSize;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_NO_ERROR;
    BBFreeSize = 0;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetBitBufferFreeSize(CurrentHandle, &BBFreeSize );
        sprintf(VID_Msg, "VID_GetBitBufferFreeSize(hndl=0x%x,freesize=%d) : ",
                 (U32)CurrentHandle, BBFreeSize );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", BBFreeSize, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetBitBufferFreeSize() */

/*------------------------------------------------------------------------
 * Function : VID_GetBitBufferParams
 *            Get the size and the BaseAddress of bit buffer
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetBitBufferParams(STTST_Parse_t *pars_p, char *result_sym_p)
{

    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    U32 * BaseAddress;
    U32 InitSize;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_NO_ERROR;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetBitBufferParams(CurrentHandle, (void**)&BaseAddress,&InitSize);
        sprintf(VID_Msg, "VID_GetBitBufferParams(hndl=0x%x,BaseAddress=0x%x,InitSize=%d) : ",
                 (U32)CurrentHandle,(U32) BaseAddress,InitSize);
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", (U32)BaseAddress, FALSE);
                STTST_AssignInteger("RET_VAL2", InitSize, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

 return (FALSE);

} /* end VID_GetBitBufferParams() */


/*-------------------------------------------------------------------------
 * Function : VID_GetCapability
 *            Retreive driver's capabilities
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetCapability(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_Capability_t Capability;
    BOOL RetErr;
    U16 Count;
    char Name[2*ST_MAX_DEVICE_NAME]; /* allow big name for error case test */
    ST_DeviceName_t *DeviceName;

    /* get DeviceName */
    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, Name, sizeof(Name) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected DeviceName" );
    /*}
    else
    {*/
        STTST_AssignInteger(result_sym_p, ST_ERROR_UNKNOWN_DEVICE, FALSE);
        /*return(TRUE);*/
    }
    DeviceName = (ST_DeviceName_t *)Name ;

    RetErr = TRUE;
    sprintf(VID_Msg, "STVID_GetCapability(device=%s,&capa=%ld) : ",
             *DeviceName, (long)&Capability );
    ErrCode = STVID_GetCapability(*DeviceName, &Capability);
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            sprintf(VID_Msg, "%s   brdcstprof=%d codingmode=%d colortype=%d ARconv=%d AR=%d freezemode=%d freezefld=%d\n" ,
                    VID_Msg,
                    Capability.SupportedBroadcastProfile,
                    Capability.SupportedCodingMode, Capability.SupportedColorType,
                    Capability.SupportedDisplayARConversion, Capability.SupportedDisplayAspectRatio,
                    Capability.SupportedFreezeMode, Capability.SupportedFreezeField );
            sprintf(VID_Msg, "%s   pic=%d scantype=%d stop=%d streamtype=%d stillpictcapa=%d\n",
                    VID_Msg,
                    Capability.SupportedPicture, Capability.SupportedScreenScanType,
                    Capability.SupportedStop, Capability.SupportedStreamType,
                    Capability.StillPictureCapable );
            sprintf(VID_Msg, "%s   inputWcapa=%d outputWapa=%d Walign=%d Wsize=%d\n",
                    VID_Msg,
                    Capability.ManualInputWindowCapable, Capability.ManualOutputWindowCapable,
                    Capability.SupportedWinAlign, Capability.SupportedWinSize );
            VID_PRINT((VID_Msg ));
            sprintf(VID_Msg, "   INPUT heightmin=%d widthmin=%d xprec=%d yprec=%d widthprec=%d heightprec=%d\n",
                    Capability.InputWindowHeightMin, Capability.InputWindowWidthMin,
                    Capability.InputWindowPositionXPrecision, Capability.InputWindowPositionYPrecision,
                    Capability.InputWindowWidthPrecision, Capability.InputWindowHeightPrecision );
            sprintf(VID_Msg, "%s   OUTPUT heightmin=%d widthmin=%d xprec=%d yprec=%d widthprec=%d heightprec=%d\n",
                    VID_Msg,
                    Capability.OutputWindowHeightMin, Capability.OutputWindowWidthMin,
                    Capability.OutputWindowPositionXPrecision, Capability.OutputWindowPositionYPrecision,
                    Capability.OutputWindowWidthPrecision, Capability.OutputWindowHeightPrecision );
            VID_PRINT((VID_Msg ));
            sprintf(VID_Msg, "   ContinuousHorizontalScaling=%s",
                    Capability.SupportedHorizontalScaling.Continuous ? "TRUE " : "FALSE");
            for (Count = 0; Count < Capability.SupportedHorizontalScaling.NbScalingFactors && Count < 15 ; Count++)
            {
                sprintf(VID_Msg, "%s [%d/%d]", VID_Msg,
                    (Capability.SupportedHorizontalScaling.ScalingFactors_p + Count)->N,
                    (Capability.SupportedHorizontalScaling.ScalingFactors_p + Count)->M);
            }
            VID_PRINT((VID_Msg ));
            sprintf(VID_Msg, "\n   ContinuousVerticalScaling=%s  ",
                        Capability.SupportedVerticalScaling.Continuous ? "TRUE " : "FALSE");
            for (Count = 0; Count < Capability.SupportedVerticalScaling.NbScalingFactors && Count < 15; Count++)
            {
                sprintf(VID_Msg, "%s [%d/%d]", VID_Msg,
                    (Capability.SupportedVerticalScaling.ScalingFactors_p + Count)->N,
                    (Capability.SupportedVerticalScaling.ScalingFactors_p + Count)->M);
            }
            strcat(VID_Msg, "\n");
            VID_PRINT((VID_Msg ));
            sprintf(VID_Msg, "   colorkeycapa=%d PSIcapa=%d HDPIPCapa=%d\n" ,
                    Capability.ColorKeyingCapable, Capability.PSICapable, Capability.HDPIPCapable);
            sprintf(VID_Msg, "%s   profilecapa=%d compression=%d decimationfact=%d errorrecovery=%d\n",
                    VID_Msg,
                    Capability.ProfileCapable, Capability.SupportedCompressionLevel,
                    Capability.SupportedDecimationFactor, Capability.SupportedDecimationFactor
                    );
            VID_PRINT((VID_Msg ));
            sprintf(VID_Msg, "   decodernumber=%d setupobject=%d\n",
                    Capability.DecoderNumber, Capability.SupportedSetupObject);

            /* Set data required for windowing tests : */
            Count = Capability.SupportedHorizontalScaling.NbScalingFactors - 1;
            STTST_AssignInteger("RET_VAL1", Capability.SupportedHorizontalScaling.ScalingFactors_p->M, FALSE);
            STTST_AssignInteger("RET_VAL2", (Capability.SupportedHorizontalScaling.ScalingFactors_p + Count)->N, FALSE);
            Count = Capability.SupportedVerticalScaling.NbScalingFactors - 1;
            STTST_AssignInteger("RET_VAL3", Capability.SupportedVerticalScaling.ScalingFactors_p->M, FALSE);
            STTST_AssignInteger("RET_VAL4", (Capability.SupportedVerticalScaling.ScalingFactors_p + Count)->N, FALSE);

            STTST_AssignInteger("RET_VAL10", Capability.InputWindowPositionXPrecision, FALSE);
            STTST_AssignInteger("RET_VAL11", Capability.InputWindowPositionYPrecision, FALSE);
            STTST_AssignInteger("RET_VAL12", Capability.InputWindowWidthPrecision, FALSE);
            STTST_AssignInteger("RET_VAL13", Capability.InputWindowHeightPrecision, FALSE);
            STTST_AssignInteger("RET_VAL14", Capability.OutputWindowPositionXPrecision, FALSE);
            STTST_AssignInteger("RET_VAL15", Capability.OutputWindowPositionYPrecision, FALSE);
            STTST_AssignInteger("RET_VAL16", Capability.OutputWindowWidthPrecision, FALSE);
            STTST_AssignInteger("RET_VAL17", Capability.OutputWindowHeightPrecision, FALSE);
            break;
        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(VID_Msg, "one parameter is invalid !\n" );
            break;
        case ST_ERROR_UNKNOWN_DEVICE:
            API_ErrorCount++;
            strcat(VID_Msg, "the specified device is not initialized !\n" );
            break;
        default:
            API_ErrorCount++;
            sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
            break;
    } /* end switch */
    if(RetErr == FALSE)
    {
        VID_PRINT((VID_Msg ));
    }
    else
    {
        VID_PRINT_ERROR((VID_Msg ));
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetCapability() */


/*-------------------------------------------------------------------------
 * Function : VID_GetDecodedPictures
 *            Get decoded pictures type
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetDecodedPictures(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_DecodedPictures_t DecodedPictures;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "VID_GetDecodedPictures(handle=0x%x,&decodedpict=0x%x) : ",
                          CurrentHandle, (S32)&DecodedPictures );
        ErrCode = STVID_GetDecodedPictures(CurrentHandle, &DecodedPictures );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                sprintf(VID_Msg, "%s   decodedpictures=%d ",
                            VID_Msg, (S32)DecodedPictures ) ;
                switch(DecodedPictures)
                {
                    case STVID_DECODED_PICTURES_ALL:
                        strcat(VID_Msg, "(ALL)\n");
                        break;
                    case STVID_DECODED_PICTURES_IP:
                        strcat(VID_Msg, "(IP)\n");
                        break;
                    case STVID_DECODED_PICTURES_I:
                        strcat(VID_Msg, "(I)\n");
                        break;
                    case STVID_DECODED_PICTURES_FIRST_I :
                        strcat(VID_Msg, "(FIRST_I)\n");
                        break;
                } /* end switch */
                STTST_AssignInteger("RET_VAL1", (S32)DecodedPictures, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetDecodedPictures() */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_GetDisplayARConversion
 *            Get pel ratio conversion
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetDisplayARConversion(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_DisplayAspectRatioConversion_t RatioCv;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_GetDisplayAspectRatioConversion(CurrentViewportHndl, &RatioCv );
        sprintf(VID_Msg, "STVID_GetDisplayAspectRatioConversion(viewporthndl=0x%x,ratio=%d) : ",
                          (U32)CurrentViewportHndl, RatioCv );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", RatioCv, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetDisplayARConversion() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_GetErrorRecoveryMode
 *            Get current error recovery mode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetErrorRecoveryMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_ErrorRecoveryMode_t Mode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetErrorRecoveryMode(CurrentHandle, &Mode );
        sprintf(VID_Msg, "STVID_GetErrorRecoveryMode(handle=0x%x,mode=%d) : ",
                          CurrentHandle, Mode );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", Mode, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetErrorRecoveryMode() */

/*-------------------------------------------------------------------------
 * Function : VID_GetHDPIPParams
 *            Get HDPIP parameters.
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetHDPIPParams(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t      ErrCode;
    STVID_HDPIPParams_t HDPIPParams;
    BOOL                RetErr;
    STVID_Handle_t      CurrentHandle;
    S32                 LVar;

    RetErr  = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetHDPIPParams(CurrentHandle, &HDPIPParams );
        sprintf(VID_Msg, "STVID_GetHDPIPParams(handle=0x%x, %s, Width=%d, Height=%d) : ",
                          CurrentHandle, (HDPIPParams.Enable ? "enable" : "disable"),
                         HDPIPParams. WidthThreshold, HDPIPParams.HeightThreshold );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", HDPIPParams.Enable, FALSE);
                STTST_AssignInteger("RET_VAL2", HDPIPParams.WidthThreshold, FALSE);
                STTST_AssignInteger("RET_VAL3", HDPIPParams.HeightThreshold, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter in invalid !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetHDPIPParams() */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_GetInputWindowMode
 *            Get input window parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetInputWindowMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_WindowParams_t WinParams;
    BOOL AutoMode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_GetInputWindowMode(CurrentViewportHndl, &AutoMode, &WinParams );
        sprintf(VID_Msg, "STVID_GetInputWindowMode(viewporthndl=0x%x,automode=%d,align=%d,size=%d) : ",
                 (U32)CurrentViewportHndl, AutoMode, WinParams.Align, WinParams.Size );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                if (WinParams.Align!=STVID_WIN_ALIGN_TOP_LEFT
                    && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_LEFT
                    && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_LEFT
                    && WinParams.Align!=STVID_WIN_ALIGN_TOP_RIGHT
                    && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_RIGHT
                    && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_RIGHT
                    && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_HCENTRE
                    && WinParams.Align!=STVID_WIN_ALIGN_TOP_HCENTRE
                    && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_HCENTRE)
                {
                    sprintf(VID_Msg, "%swrong Align value = %d !\n",
                             VID_Msg, WinParams.Align );
                    API_ErrorCount++;
                }
                else
                {
                    RetErr = FALSE;
                    strcat(VID_Msg, "ok\n" );
                    STTST_AssignInteger("RET_VAL1", AutoMode, FALSE);
                    STTST_AssignInteger("RET_VAL2", WinParams.Align, FALSE);
                    STTST_AssignInteger("RET_VAL3", WinParams.Size, FALSE);
                }
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetInputWindowMode() */

/*-------------------------------------------------------------------------
 * Function : VID_GetIOWindows
 *            Retrieve the input and output window size and position
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetIOWindows(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    S32 InputWinPositionX;
    S32 InputWinPositionY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    S32 OutputWinPositionX;
    S32 OutputWinPositionY;
    U32 OutputWinWidth;
    U32 OutputWinHeight;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_GetIOWindows(CurrentViewportHndl,
                                      &InputWinPositionX, &InputWinPositionY,
                                      &InputWinWidth, &InputWinHeight,
                                      &OutputWinPositionX, &OutputWinPositionY,
                                      &OutputWinWidth, &OutputWinHeight );
        sprintf(VID_Msg, "STVID_GetIOWindows(viewporthndl=0x%x,ix=%d,iy=%d,iw=%d,ih=%d,ox=%d,oy=%d,ow=%d,oh=%d) : ",
                 (U32)CurrentViewportHndl,
                 InputWinPositionX, InputWinPositionY, InputWinWidth, InputWinHeight,
                 OutputWinPositionX, OutputWinPositionY, OutputWinWidth, OutputWinHeight );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", InputWinPositionX, FALSE);
                STTST_AssignInteger("RET_VAL2", InputWinPositionY, FALSE);
                STTST_AssignInteger("RET_VAL3", InputWinWidth, FALSE);
                STTST_AssignInteger("RET_VAL4", InputWinHeight, FALSE);
                STTST_AssignInteger("RET_VAL5", OutputWinPositionX, FALSE);
                STTST_AssignInteger("RET_VAL6", OutputWinPositionY, FALSE);
                STTST_AssignInteger("RET_VAL7", OutputWinWidth, FALSE);
                STTST_AssignInteger("RET_VAL8", OutputWinHeight, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetIOWindows() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_GetMemoryProfile
 *            Retrieve the input and output window size and position
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetMemoryProfile(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_MemoryProfile_t MemProfile;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetMemoryProfile(CurrentHandle, &MemProfile );
        sprintf(VID_Msg, "STVID_GetMemoryProfile(handle=0x%x,&memprof=0x%x) : ",
                 (U32)CurrentHandle, (int)&MemProfile );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", MemProfile.MaxWidth, FALSE);
                STTST_AssignInteger("RET_VAL2", MemProfile.MaxHeight, FALSE);
                STTST_AssignInteger("RET_VAL3", MemProfile.NbFrameStore, FALSE);
                STTST_AssignInteger("RET_VAL4", MemProfile.CompressionLevel, FALSE);
                STTST_AssignInteger("RET_VAL5", MemProfile.DecimationFactor, FALSE);
                switch (MemProfile.NbFrameStore)
                {
                    case STVID_OPTIMISED_NB_FRAME_STORE_ID:
                        STTST_AssignInteger("RET_VAL6", MemProfile.FrameStoreIDParams.OptimisedNumber.Main, FALSE);
                        if (MemProfile.DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
                        {
                            STTST_AssignInteger("RET_VAL7", MemProfile.FrameStoreIDParams.OptimisedNumber.Decimated, FALSE);
                            sprintf(VID_Msg,
                                "%s   maxwidth=%d,maxheight=%d,nbframestore=%d,compress=%d,decim=%d,optnbmain=%d,optnbdecim=%d \n",
                                VID_Msg, MemProfile.MaxWidth, MemProfile.MaxHeight,
                                MemProfile.NbFrameStore, MemProfile.CompressionLevel, MemProfile.DecimationFactor,
                                MemProfile.FrameStoreIDParams.OptimisedNumber.Main,
                                MemProfile.FrameStoreIDParams.OptimisedNumber.Decimated);
                        }
                        else
                        {
                            sprintf(VID_Msg,
                                "%s   maxwidth=%d,maxheight=%d,nbframestore=%d,compress=%d,decim=%d,optnbmain=%d \n",
                                VID_Msg, MemProfile.MaxWidth, MemProfile.MaxHeight,
                                MemProfile.NbFrameStore, MemProfile.CompressionLevel, MemProfile.DecimationFactor,
                                MemProfile.FrameStoreIDParams.OptimisedNumber.Main);
                        }
                        break;

                    case STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID:
                        STTST_AssignInteger("RET_VAL6", MemProfile.FrameStoreIDParams.VariableInFixedSize.Main, FALSE);
                        if (MemProfile.DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
                        {
                            STTST_AssignInteger("RET_VAL7", MemProfile.FrameStoreIDParams.VariableInFixedSize.Decimated, FALSE);
                            sprintf(VID_Msg,
                                "%s   maxwidth=%d,maxheight=%d,nbframestore=%d,compress=%d,decim=%d,MaxSizeInMain=%d,MaxSizeInDecim=%d \n",
                                VID_Msg, MemProfile.MaxWidth, MemProfile.MaxHeight,
                                MemProfile.NbFrameStore, MemProfile.CompressionLevel, MemProfile.DecimationFactor,
                                MemProfile.FrameStoreIDParams.VariableInFixedSize.Main,
                                MemProfile.FrameStoreIDParams.VariableInFixedSize.Decimated);
                        }
                        else
                        {
                            sprintf(VID_Msg,
                                "%s   maxwidth=%d,maxheight=%d,nbframestore=%d,compress=%d,decim=%d,MaxSizeInMain=%d \n",
                                VID_Msg, MemProfile.MaxWidth, MemProfile.MaxHeight,
                                MemProfile.NbFrameStore, MemProfile.CompressionLevel, MemProfile.DecimationFactor,
                                MemProfile.FrameStoreIDParams.VariableInFixedSize.Main);
                        }
                        break;

                    case STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID:
                        STTST_AssignInteger("RET_VAL6", MemProfile.FrameStoreIDParams.VariableInFullPartition.Main, FALSE);
                        if (MemProfile.DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
                        {
                            STTST_AssignInteger("RET_VAL7", MemProfile.FrameStoreIDParams.VariableInFullPartition.Decimated, FALSE);
                            sprintf(VID_Msg,
                                "%s   maxwidth=%d,maxheight=%d,nbframestore=%d,compress=%d,decim=%d,PartitionMain=0x%x,PartitionDecim=0x%x \n",
                                VID_Msg, MemProfile.MaxWidth, MemProfile.MaxHeight,
                                MemProfile.NbFrameStore, MemProfile.CompressionLevel, MemProfile.DecimationFactor,
                                MemProfile.FrameStoreIDParams.VariableInFullPartition.Main,
                                MemProfile.FrameStoreIDParams.VariableInFullPartition.Decimated);
                        }
                        else
                        {
                            sprintf(VID_Msg,
                                "%s   maxwidth=%d,maxheight=%d,nbframestore=%d,compress=%d,decim=%d,PartitionMain=0x%x \n",
                                VID_Msg, MemProfile.MaxWidth, MemProfile.MaxHeight,
                                MemProfile.NbFrameStore, MemProfile.CompressionLevel, MemProfile.DecimationFactor,
                                MemProfile.FrameStoreIDParams.VariableInFullPartition.Main);
                        }
                        break;

                    default:
                        sprintf(VID_Msg, "%s   maxwidth=%d,maxheight=%d,nbframestore=%d,compress=%d,decim=%d \n",
                            VID_Msg, MemProfile.MaxWidth, MemProfile.MaxHeight,
                            MemProfile.NbFrameStore, MemProfile.CompressionLevel,
                            MemProfile.DecimationFactor);
                        break;
                }
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetMemoryProfile() */

#ifndef STVID_NO_DISPLAY
  /*-------------------------------------------------------------------------
 * Function : VID_GetOutputWindowMode
 *            Get output window parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetOutputWindowMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_WindowParams_t WinParams;
    BOOL AutoMode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_GetOutputWindowMode(CurrentViewportHndl, &AutoMode, &WinParams );
        sprintf(VID_Msg, "STVID_GetOutputWindowMode(viewporthndl=0x%x,automode=%d,align=%d,size=%d) : ",
                 (int)CurrentViewportHndl, AutoMode, WinParams.Align, WinParams.Size );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                if (WinParams.Align!=STVID_WIN_ALIGN_TOP_LEFT
                    && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_LEFT
                    && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_LEFT
                    && WinParams.Align!=STVID_WIN_ALIGN_TOP_RIGHT
                    && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_RIGHT
                    && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_RIGHT
                    && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_HCENTRE
                    && WinParams.Align!=STVID_WIN_ALIGN_TOP_HCENTRE
                    && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_HCENTRE)
                {
                    sprintf(VID_Msg, "%sNO_ERROR returned with wrong Align value = %d !\n",
                             VID_Msg, WinParams.Align );
                    API_ErrorCount++;
                }
                else
                {
                    RetErr = FALSE;
                    strcat(VID_Msg, "ok\n" );
                    STTST_AssignInteger("RET_VAL1", AutoMode, FALSE);
                    STTST_AssignInteger("RET_VAL2", WinParams.Align, FALSE);
                    STTST_AssignInteger("RET_VAL3", WinParams.Size, FALSE);
                }
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetOutputWindowMode() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_GetPictureAllocParams
 *            Get parameters to allocate space for the picture
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetPictureAllocParams(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STAVMEM_AllocBlockParams_t AllocParams;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetPictureAllocParams(CurrentHandle, &VID_PictParams, &AllocParams );
        sprintf(VID_Msg, "STVID_GetPictureAllocParams(handle=0x%x,&pictparam=0x%x,&allocparams=0x%x) : ",
                          CurrentHandle, (U32)&VID_PictParams, (U32)&AllocParams );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                sprintf(VID_Msg, "%s   codingmode=%d colortype=%d framerate=%d data=%d size=%d\n",
                         VID_Msg, VID_PictParams.CodingMode,
                         VID_PictParams.ColorType, VID_PictParams.FrameRate,
                         (U32)VID_PictParams.Data, VID_PictParams.Size ) ;
                sprintf(VID_Msg, "%s   height=%d width=%d\n",
                         VID_Msg, VID_PictParams.Height, VID_PictParams.Width ) ;
                sprintf(VID_Msg, "%s   aspect=%d scantype=%d mpegframe=%d topfieldfirst=%d\n",
                         VID_Msg, VID_PictParams.Aspect,
                         VID_PictParams.ScanType, VID_PictParams.MPEGFrame,
                         VID_PictParams.TopFieldFirst );
                sprintf(VID_Msg, "%s   hours=%d minutes=%d seconds=%d frames=%d interpolated=%d\n",
                         VID_Msg, VID_PictParams.TimeCode.Hours,
                         VID_PictParams.TimeCode.Minutes, VID_PictParams.TimeCode.Seconds,
                         VID_PictParams.TimeCode.Frames, VID_PictParams.TimeCode.Interpolated );
                sprintf(VID_Msg, "%s   pts=%d pts33=%d ptsinterpolated=%d\n",
                         VID_Msg, VID_PictParams.PTS.PTS,
                         VID_PictParams.PTS.PTS33, VID_PictParams.PTS.Interpolated );
                sprintf(VID_Msg, "%s   partitionhndl=%d size=%d align=%d\n",
                         VID_Msg, AllocParams.PartitionHandle,
                         AllocParams.Size, AllocParams.Alignment );
                sprintf(VID_Msg, "%s   mode=%d nbfranges=%d &range=%d\n",
                         VID_Msg, AllocParams.AllocMode,
                         AllocParams.NumberOfForbiddenRanges, (U32)AllocParams.ForbiddenRangeArray_p );
                sprintf(VID_Msg, "%s   nbfborders=%d &borders=%d\n",
                         VID_Msg, AllocParams.NumberOfForbiddenBorders,
                         (U32)AllocParams.ForbiddenBorderArray_p );
                STTST_AssignInteger("RET_VAL1", VID_PictParams.CodingMode, FALSE);
                STTST_AssignInteger("RET_VAL2", VID_PictParams.ColorType, FALSE);
                STTST_AssignInteger("RET_VAL3", VID_PictParams.FrameRate, FALSE);
                STTST_AssignInteger("RET_VAL4", (U32)VID_PictParams.Data, FALSE);
                STTST_AssignInteger("RET_VAL5", VID_PictParams.Size, FALSE);
                STTST_AssignInteger("RET_VAL6", VID_PictParams.Height, FALSE);
                STTST_AssignInteger("RET_VAL7", VID_PictParams.Width, FALSE);
                STTST_AssignInteger("RET_VAL8", VID_PictParams.Aspect, FALSE);
                STTST_AssignInteger("RET_VAL9", VID_PictParams.MPEGFrame, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetPictureAllocParams() */

/*------------------------------------------------------------------------
 * Function : VID_GetPictureBuffer
 *            Get picture buffer informations
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetPictureBuffer(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    STVID_GetPictureBufferParams_t CurrentParams;
    STVID_PictureBufferDataParams_t CurrentPictureBufferParams;
    STVID_PictureBufferHandle_t CurrentPictureBufferHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_NO_ERROR;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get structure */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_PICTURE_STRUCTURE_FRAME, &LVar );
        CurrentParams.PictureStructure = (STVID_PictureStructure_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected PictureStructure (%d=top %d=bottom %d=frame=default)",
                STVID_PICTURE_STRUCTURE_TOP_FIELD, STVID_PICTURE_STRUCTURE_BOTTOM_FIELD,
                STVID_PICTURE_STRUCTURE_FRAME );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get TopFieldFirst */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        CurrentParams.TopFieldFirst = (BOOL) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected TopFieldFirst (TRUE=default, or FALSE)" );
        }
    }
    /* get ExpectingSecondField */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        CurrentParams.ExpectingSecondField = (BOOL) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ExpectingSecondField (TRUE=default, or FALSE)" );
        }
    }
    /* get ExtendedTemporalReference */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        CurrentParams.ExtendedTemporalReference = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ExtendedTemporalReference" );
        }
    }
    /* get PictureWidth */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 720, &LVar );
        CurrentParams.PictureWidth = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected PictureWidth" );
        }
    }
    /* get PictureHeight */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 576, &LVar );
        CurrentParams.PictureHeight = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected PictureHeight" );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetPictureBuffer(CurrentHandle, &CurrentParams,
                            &CurrentPictureBufferParams, &CurrentPictureBufferHandle );
        sprintf(VID_Msg, "VID_GetPictureBuffer(hndl=0x%x,&params=0x%x,&pictbuffparam=0x%x,pictbuffhndl=0x%x) : ",
                 (U32)CurrentHandle, (int)&CurrentParams, (int)&CurrentPictureBufferParams, (int)CurrentPictureBufferHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", (U32)CurrentPictureBufferParams.Data1_p, FALSE);
                STTST_AssignInteger("RET_VAL2", (U32)CurrentPictureBufferParams.Size1, FALSE);
                STTST_AssignInteger("RET_VAL3", (U32)CurrentPictureBufferParams.Data2_p, FALSE);
                STTST_AssignInteger("RET_VAL4", (U32)CurrentPictureBufferParams.Size2, FALSE);
                STTST_AssignInteger("RET_VAL5", (U32)CurrentPictureBufferHandle, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_NO_MEMORY:
                API_ErrorCount++;
                strcat(VID_Msg, "no memory (no free picture buffer) !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE:
                API_ErrorCount++;
                strcat(VID_Msg, "not available (impossible to alloc. a 2nd field buffer because no 1st field picture) !\n" );
                break;
            case STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE:
                API_ErrorCount++;
                strcat(VID_Msg, "impossible with memory profile (picture bigger than profile) !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
        sprintf(VID_Msg, "   param : PictStructure=%d TopFieldFirst=%d ExpectingSecondField=%d \n",
                CurrentParams.PictureStructure,
                CurrentParams.TopFieldFirst,
                CurrentParams.ExpectingSecondField);
        VID_PRINT((VID_Msg ));
        sprintf(VID_Msg, "           ExtendedTemporalReference=%d PictWidth=%d PictHeight=%d \n",
                CurrentParams.ExtendedTemporalReference,
                CurrentParams.PictureWidth,
                CurrentParams.PictureHeight);
        VID_PRINT((VID_Msg ));
        sprintf(VID_Msg, "   pict buff data param : Data1=0x%x Size1=%d \n",
                (int)CurrentPictureBufferParams.Data1_p,
                CurrentPictureBufferParams.Size1);
        VID_PRINT((VID_Msg ));
        sprintf(VID_Msg, "                          Data2=0x%x Size2=%d \n",
                (int)CurrentPictureBufferParams.Data2_p,
                CurrentPictureBufferParams.Size2);
        VID_PRINT((VID_Msg ));
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetPictureBuffer() */

/*-------------------------------------------------------------------------
 * Function : VID_GetPictureParams
 *            Get info. about a picture
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetPictureParams(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_Picture_t PictureType;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get type */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_PICTURE_DISPLAYED, &LVar );
        PictureType = (STVID_Picture_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected Name(%d=last decoded, %d=displayed) ",
                              STVID_PICTURE_LAST_DECODED, STVID_PICTURE_DISPLAYED );
            STTST_TagCurrentLine(pars_p, VID_Msg );
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetPictureParams(CurrentHandle, PictureType, &VID_PictParams );
        sprintf(VID_Msg, "STVID_GetPictureParams(handle=0x%x,picttype=%d,&params=0x%x) : ",
                 CurrentHandle, PictureType, (U32)&VID_PictParams );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                sprintf(VID_Msg, "%s   codingmode=%d colortype=%d framerate=%d data=0x%x size=%d\n",
                         VID_Msg, VID_PictParams.CodingMode,
                         VID_PictParams.ColorType, VID_PictParams.FrameRate,
                         (U32)VID_PictParams.Data, VID_PictParams.Size ) ;
                sprintf(VID_Msg, "%s   height=%d width=%d\n",
                         VID_Msg, VID_PictParams.Height, VID_PictParams.Width ) ;
                sprintf(VID_Msg, "%s   aspect=%d scantype=%d mpegframe=%d topfieldfirst=%d\n",
                         VID_Msg, VID_PictParams.Aspect,
                         VID_PictParams.ScanType, VID_PictParams.MPEGFrame,
                         VID_PictParams.TopFieldFirst );
                sprintf(VID_Msg, "%s   hours=%d minutes=%d seconds=%d frames=%d interpolated=%d\n",
                         VID_Msg, VID_PictParams.TimeCode.Hours,
                         VID_PictParams.TimeCode.Minutes, VID_PictParams.TimeCode.Seconds,
                         VID_PictParams.TimeCode.Frames, VID_PictParams.TimeCode.Interpolated );
                sprintf(VID_Msg, "%s   pts=%d pts33=%d ptsinterpolated=%d\n",
                         VID_Msg, VID_PictParams.PTS.PTS,
                         VID_PictParams.PTS.PTS33, VID_PictParams.PTS.Interpolated );
                STTST_AssignInteger("RET_VAL1", PictureType, FALSE);
                STTST_AssignInteger("RET_VAL2", VID_PictParams.CodingMode, FALSE);
                STTST_AssignInteger("RET_VAL3", VID_PictParams.ColorType, FALSE);
                STTST_AssignInteger("RET_VAL4", VID_PictParams.FrameRate, FALSE);
                STTST_AssignInteger("RET_VAL5", (U32)VID_PictParams.Data, FALSE);
                STTST_AssignInteger("RET_VAL6", VID_PictParams.Size, FALSE);
                STTST_AssignInteger("RET_VAL7", VID_PictParams.Height, FALSE);
                STTST_AssignInteger("RET_VAL8", VID_PictParams.Width, FALSE);
                STTST_AssignInteger("RET_VAL9", VID_PictParams.Aspect, FALSE);
                STTST_AssignInteger("RET_VAL10", VID_PictParams.ScanType, FALSE);
                STTST_AssignInteger("RET_VAL11", VID_PictParams.MPEGFrame, FALSE);
                STTST_AssignInteger("RET_VAL12", VID_PictParams.TopFieldFirst, FALSE);
                STTST_AssignInteger("RET_VAL13", VID_PictParams.TimeCode.Hours, FALSE);
                STTST_AssignInteger("RET_VAL14", VID_PictParams.TimeCode.Minutes, FALSE);
                STTST_AssignInteger("RET_VAL15", VID_PictParams.TimeCode.Seconds, FALSE);
                STTST_AssignInteger("RET_VAL16", VID_PictParams.TimeCode.Frames, FALSE);
                STTST_AssignInteger("RET_VAL17", VID_PictParams.TimeCode.Interpolated, FALSE);
                STTST_AssignInteger("RET_VAL18", VID_PictParams.PTS.PTS, FALSE);
                STTST_AssignInteger("RET_VAL19", VID_PictParams.PTS.PTS33, FALSE);
                STTST_AssignInteger("RET_VAL20", VID_PictParams.PTS.Interpolated, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING :
                API_ErrorCount++;
                strcat(VID_Msg, "can't retrieve info. while decoding !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "no picture of the required picture type available !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetPictureParams() */

/*-------------------------------------------------------------------------
 * Function : VID_GetPictureInfos
 *            Get infos about a picture
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetPictureInfos(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_Picture_t PictureType;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get type */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_PICTURE_DISPLAYED, &LVar );
        PictureType = (STVID_Picture_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected Name(%d=last decoded, %d=displayed) ",
                              STVID_PICTURE_LAST_DECODED, STVID_PICTURE_DISPLAYED );
            STTST_TagCurrentLine(pars_p, VID_Msg );
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetPictureInfos(CurrentHandle, PictureType, &VID_PictInfos );
        sprintf(VID_Msg, "STVID_GetPictureInfos(handle=0x%x,picttype=%d,&params=0x%x) : ",
                 CurrentHandle, PictureType, (U32)&VID_PictInfos );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                sprintf(VID_Msg, "%s   BitmapParams\n   =============\n", VID_Msg) ;
                sprintf(VID_Msg, "%s   ColorType=%d BitmapType=%d PreMultipliedColor=%d\n",
                         VID_Msg, VID_PictInfos.BitmapParams.ColorType,
                         VID_PictInfos.BitmapParams.BitmapType, VID_PictInfos.BitmapParams.PreMultipliedColor) ;
                sprintf(VID_Msg, "%s   ColorSpaceConversion=%d AspectRatio=%d Width=%d Height=%d\n",
                         VID_Msg, VID_PictInfos.BitmapParams.ColorSpaceConversion,
                         VID_PictInfos.BitmapParams.AspectRatio, VID_PictInfos.BitmapParams.Width,
                         VID_PictInfos.BitmapParams.Height) ;
                VID_PRINT((VID_Msg ));
                sprintf(VID_Msg, "   Pitch=%d Offset=%d Data1_p=0x%x Size1=%d\n",
                         VID_PictInfos.BitmapParams.Pitch,
                         VID_PictInfos.BitmapParams.Offset, (U32)VID_PictInfos.BitmapParams.Data1_p,
                         VID_PictInfos.BitmapParams.Size1) ;
                sprintf(VID_Msg, "%s   Data2_p=0x%x Size2=%d SubByteFormat=%d BigNotLittle=%d\n",
                         VID_Msg, (U32)VID_PictInfos.BitmapParams.Data2_p,VID_PictInfos.BitmapParams.Size2,
                         VID_PictInfos.BitmapParams.SubByteFormat, VID_PictInfos.BitmapParams.BigNotLittle) ;
                VID_PRINT((VID_Msg ));
                sprintf(VID_Msg, "   VideoParams\n   =============\n") ;
                sprintf(VID_Msg, "%s   FrameRate=%d ScanType=%d MPEGFrame=%d PictureStructure=%d\n",
                         VID_Msg, VID_PictInfos.VideoParams.FrameRate,
                         VID_PictInfos.VideoParams.ScanType, VID_PictInfos.VideoParams.MPEGFrame,
                         VID_PictInfos.VideoParams.PictureStructure) ;
                sprintf(VID_Msg, "%s   TopFieldFirst=%d CompressionLevel=%d DecimationFactors=%d\n",
                         VID_Msg, VID_PictInfos.VideoParams.TopFieldFirst, VID_PictInfos.VideoParams.CompressionLevel,
                         VID_PictInfos.VideoParams.DecimationFactors) ;
                VID_PRINT((VID_Msg ));
                sprintf(VID_Msg, "   Hours=%d Minutes=%d Seconds=%d Frames=%d Interpolated=%d\n",
                         VID_PictInfos.VideoParams.TimeCode.Hours,
                         VID_PictInfos.VideoParams.TimeCode.Minutes, VID_PictInfos.VideoParams.TimeCode.Seconds,
                         VID_PictInfos.VideoParams.TimeCode.Frames, VID_PictInfos.VideoParams.TimeCode.Interpolated );
                sprintf(VID_Msg, "%s   PTS=%d PTS33=%d PTSInterpolated=%d\n",
                         VID_Msg, VID_PictInfos.VideoParams.PTS.PTS,
                         VID_PictInfos.VideoParams.PTS.PTS33, VID_PictInfos.VideoParams.PTS.Interpolated );

                STTST_AssignInteger("RET_VAL1", VID_PictInfos.BitmapParams.ColorType, FALSE);
                STTST_AssignInteger("RET_VAL2", VID_PictInfos.BitmapParams.BitmapType, FALSE);
                STTST_AssignInteger("RET_VAL3", VID_PictInfos.BitmapParams.PreMultipliedColor, FALSE);
                STTST_AssignInteger("RET_VAL4", VID_PictInfos.BitmapParams.ColorSpaceConversion, FALSE);
                STTST_AssignInteger("RET_VAL5", VID_PictInfos.BitmapParams.AspectRatio, FALSE);
                STTST_AssignInteger("RET_VAL6", VID_PictInfos.BitmapParams.Width, FALSE);
                STTST_AssignInteger("RET_VAL7", VID_PictInfos.BitmapParams.Height, FALSE);
                STTST_AssignInteger("RET_VAL8", VID_PictInfos.BitmapParams.Pitch, FALSE);
                STTST_AssignInteger("RET_VAL9", VID_PictInfos.BitmapParams.Offset, FALSE);
                STTST_AssignInteger("RET_VAL10", (U32)VID_PictInfos.BitmapParams.Data1_p, FALSE);
                STTST_AssignInteger("RET_VAL11", VID_PictInfos.BitmapParams.Size1, FALSE);
                STTST_AssignInteger("RET_VAL12", (U32)VID_PictInfos.BitmapParams.Data2_p, FALSE);
                STTST_AssignInteger("RET_VAL13", VID_PictInfos.BitmapParams.Size2, FALSE);
                STTST_AssignInteger("RET_VAL14", VID_PictInfos.BitmapParams.SubByteFormat, FALSE);
                STTST_AssignInteger("RET_VAL15", VID_PictInfos.BitmapParams.BigNotLittle, FALSE);
                STTST_AssignInteger("RET_VAL16", VID_PictInfos.VideoParams.FrameRate, FALSE);
                STTST_AssignInteger("RET_VAL17", VID_PictInfos.VideoParams.ScanType, FALSE);
                STTST_AssignInteger("RET_VAL18", VID_PictInfos.VideoParams.MPEGFrame, FALSE);
                STTST_AssignInteger("RET_VAL19", VID_PictInfos.VideoParams.PictureStructure, FALSE);
                STTST_AssignInteger("RET_VAL20", VID_PictInfos.VideoParams.TopFieldFirst, FALSE);
                STTST_AssignInteger("RET_VAL21", VID_PictInfos.VideoParams.CompressionLevel, FALSE);
                STTST_AssignInteger("RET_VAL22", VID_PictInfos.VideoParams.DecimationFactors, FALSE);
                STTST_AssignInteger("RET_VAL23", VID_PictInfos.VideoParams.TimeCode.Hours, FALSE);
                STTST_AssignInteger("RET_VAL24", VID_PictInfos.VideoParams.TimeCode.Minutes, FALSE);
                STTST_AssignInteger("RET_VAL25", VID_PictInfos.VideoParams.TimeCode.Seconds, FALSE);
                STTST_AssignInteger("RET_VAL26", VID_PictInfos.VideoParams.TimeCode.Frames, FALSE);
                STTST_AssignInteger("RET_VAL27", VID_PictInfos.VideoParams.TimeCode.Interpolated, FALSE);
                STTST_AssignInteger("RET_VAL28", VID_PictInfos.VideoParams.PTS.PTS, FALSE);
                STTST_AssignInteger("RET_VAL29", VID_PictInfos.VideoParams.PTS.PTS33, FALSE);
                STTST_AssignInteger("RET_VAL30", VID_PictInfos.VideoParams.PTS.Interpolated, FALSE);
                STTST_AssignInteger("RET_VAL31", (U32)VID_PictInfos.PictureBufferHandle, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING :
                API_ErrorCount++;
                strcat(VID_Msg, "can't retrieve info. while decoding !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "no picture of the required picture type available !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetPictureInfos() */


/*-------------------------------------------------------------------------
 * Function : VID_GetRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetRevision(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_Revision_t RevisionNumber;

    UNUSED_PARAMETER(pars_p);

    RevisionNumber = STVID_GetRevision();

    sprintf(VID_Msg, "STVID_GetRevision() : revision=%s\n", RevisionNumber );
    VID_PRINT((VID_Msg ));
    STTST_AssignInteger(result_sym_p, ST_NO_ERROR, FALSE);

    return (FALSE );

} /* end VID_GetRevision() */


/*-------------------------------------------------------------------------
 * Function : VID_GetSpeed
 *            Get play speed
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetSpeed(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    S32 Speed;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_GetSpeed(CurrentHandle, &Speed );
        sprintf(VID_Msg, "STVID_GetSpeed(handle=0x%x,speed=%d) : ", CurrentHandle, Speed );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", Speed, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_GetSpeed() */

/*-------------------------------------------------------------------------
 * Function : VID_GetStatistics
 *            Retreive driver's statistics
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetStatistics(STTST_Parse_t *pars_p, char *result_sym_p)
{
#ifndef STVID_DEBUG_GET_STATISTICS
    return(FALSE);
#else /* STVID_DEBUG_GET_STATISTICS */
    ST_ErrorCode_t ErrCode;
    STVID_Statistics_t Statistics;
    BOOL RetErr;
    char Name[2*ST_MAX_DEVICE_NAME]; /* allow big name for error case test */
    ST_DeviceName_t *DeviceName;

    /* get DeviceName */
    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, Name, sizeof(Name) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected DeviceName" );
        return(TRUE);
    }
    DeviceName = (ST_DeviceName_t *)Name ;

    RetErr = TRUE;
    sprintf(VID_Msg, "STVID_GetStatistics(device=%s,&statistics=%ld) : ",
             *DeviceName, (long)&Statistics );

    ErrCode = STVID_GetStatistics(*DeviceName, &Statistics);
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;

            strcat(VID_Msg, "ok\n" );
            sprintf(VID_Msg, "\n ApiPbLiveResetWaitForFirstPictureDetected.....%d", Statistics.ApiPbLiveResetWaitForFirstPictureDetected );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n ApiPbLiveResetWaitForFirstPictureDecoded......%d", Statistics.ApiPbLiveResetWaitForFirstPictureDecoded );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n ApiPbLiveResetWaitForNextPicture..............%d", Statistics.ApiPbLiveResetWaitForNextPicture );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n AvsyncSkippedFields...................%d", Statistics.AvsyncSkippedFields );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n AvsyncRepeatedFields..................%d", Statistics.AvsyncRepeatedFields );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n AvsyncMaxRepeatedFields...............%d", Statistics.AvsyncMaxRepeatedFields );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n AvsyncFailedToSkipFields..............%d", Statistics.AvsyncFailedToSkipFields );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n AvsyncExtendedSTCAvailable............%d", Statistics.AvsyncExtendedSTCAvailable );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n AvsyncPictureWithNonInterpolatedPTS...%d", Statistics.AvsyncPictureWithNonInterpolatedPTS );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n AvsyncPictureCheckedSynchronizedOk....%d", Statistics.AvsyncPictureCheckedSynchronizedOk );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n AvsyncPTSInconsistency................%d", Statistics.AvsyncPTSInconsistency );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeHardwareSoftReset...............%d", Statistics.DecodeHardwareSoftReset );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeStartCodeFound..................%d", Statistics.DecodeStartCodeFound );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeSequenceFound...................%d", Statistics.DecodeSequenceFound );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeUserDataFound...................%d", Statistics.DecodeUserDataFound );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePictureFound....................%d", Statistics.DecodePictureFound );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePictureFoundMPEGFrameI..........%d", Statistics.DecodePictureFoundMPEGFrameI );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePictureFoundMPEGFrameP..........%d", Statistics.DecodePictureFoundMPEGFrameP );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePictureFoundMPEGFrameB..........%d", Statistics.DecodePictureFoundMPEGFrameB );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePictureSkippedRequested.........%d", Statistics.DecodePictureSkippedRequested );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePictureSkippedNotRequested......%d", Statistics.DecodePictureSkippedNotRequested );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePictureDecodeLaunched...........%d", Statistics.DecodePictureDecodeLaunched );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeStartConditionVbvDelay..........%d", Statistics.DecodeStartConditionVbvDelay );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeStartConditionPtsTimeComparison.%d", Statistics.DecodeStartConditionPtsTimeComparison );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeStartConditionVbvBufferSize.....%d", Statistics.DecodeStartConditionVbvBufferSize );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeInterruptStartDecode............%d", Statistics.DecodeInterruptStartDecode );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeInterruptPipelineIdle...........%d", Statistics.DecodeInterruptPipelineIdle );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeInterruptDecoderIdle............%d", Statistics.DecodeInterruptDecoderIdle );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeInterruptBitBufferEmpty.........%d", Statistics.DecodeInterruptBitBufferEmpty );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeInterruptBitBufferFull..........%d", Statistics.DecodeInterruptBitBufferFull );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbStartCodeFoundInvalid.................%d", Statistics.DecodePbStartCodeFoundInvalid );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbStartCodeFoundVideoPES................%d", Statistics.DecodePbStartCodeFoundVideoPES );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbMaxNbInterruptSyntaxErrorPerPicture...%d",
                                                                       Statistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbInterruptSyntaxError..................%d", Statistics.DecodePbInterruptSyntaxError );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbInterruptDecodeOverflowError..........%d",
                                                                               Statistics.DecodePbInterruptDecodeOverflowError );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbInterruptDecodeUnderflowError.........%d", Statistics.DecodePbInterruptDecodeUnderflowError );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbDecodeTimeOutError....................%d", Statistics.DecodePbDecodeTimeOutError );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbInterruptMisalignmentError............%d", Statistics.DecodePbInterruptMisalignmentError );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbInterruptQueueOverflow................%d", Statistics.DecodePbInterruptQueueOverflow );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbHeaderFifoEmpty.......................%d", Statistics.DecodePbHeaderFifoEmpty );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbVbvSizeGreaterThanBitBuffer...........%d", Statistics.DecodePbVbvSizeGreaterThanBitBuffer );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeMinBitBufferLevelReached........%d", Statistics.DecodeMinBitBufferLevelReached );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeMaxBitBufferLevelReached........%d", Statistics.DecodeMaxBitBufferLevelReached );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbSequenceNotInMemProfileSkipped........%d", Statistics.DecodePbSequenceNotInMemProfileSkipped );
            STTST_Print((VID_Msg ));
#ifdef STVID_HARDWARE_ERROR_EVENT
            sprintf(VID_Msg, "\n DecodePbHardwareErrorMissingPID...............%d", Statistics.DecodePbHardwareErrorMissingPID );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbHardwareErrorSyntaxError..............%d", Statistics.DecodePbHardwareErrorSyntaxError );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbHardwareErrorTooSmallPicture..........%d", Statistics.DecodePbHardwareErrorTooSmallPicture );
            STTST_Print((VID_Msg ));
#endif /* STVID_HARDWARE_ERROR_EVENT */
            sprintf(VID_Msg, "\n DecodePbParserError..........%d", Statistics.DecodePbParserError );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbPreprocError..........%d", Statistics.DecodePbPreprocError );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodePbFirmwareError..........%d", Statistics.DecodePbFirmwareError );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DecodeGNBvd42696Error..........%d", Statistics.DecodeGNBvd42696Error );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPictureInsertedInQueue.........%d", Statistics.DisplayPictureInsertedInQueue );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPictureInsertedInQueueDecima...%d", Statistics.DisplayPictureInsertedInQueueDecimated );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPictureDisplayedByMain.........%d", Statistics.DisplayPictureDisplayedByMain);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPictureDisplayedByAux..........%d", Statistics.DisplayPictureDisplayedByAux);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPictureDisplayedBySec..........%d", Statistics.DisplayPictureDisplayedBySec);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPictureDisplayedDecimByMain....%d", Statistics.DisplayPictureDisplayedDecimatedByMain);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPictureDisplayedDecimByAux.....%d", Statistics.DisplayPictureDisplayedDecimatedByAux);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPictureDisplayedDecimBySec.....%d", Statistics.DisplayPictureDisplayedDecimatedBySec);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPbQueueLockedByLackOfPicture...........%d", Statistics.DisplayPbQueueLockedByLackOfPicture );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPbQueueOverflow........................%d", Statistics.DisplayPbQueueOverflow );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPbPictureTooLateRejectedByMain.........%d", Statistics.DisplayPbPictureTooLateRejectedByMain);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPbPictureTooLateRejectedByAux..........%d", Statistics.DisplayPbPictureTooLateRejectedByAux);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPbPictureTooLateRejectedBySec..........%d", Statistics.DisplayPbPictureTooLateRejectedBySec);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DisplayPbPicturePreparedAtLastMinuteRejected..........%d", Statistics.DisplayPbPicturePreparedAtLastMinuteRejected);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n QueuePictureInsertedInQueue...........%d", Statistics.QueuePictureInsertedInQueue);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n QueuePictureRemovedFromQueue..........%d", Statistics.QueuePictureRemovedFromQueue);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n QueuePicturePushedToDisplay...........%d", Statistics.QueuePicturePushedToDisplay);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n QueuePbPictureTooLateRejected.................%d", Statistics.QueuePbPictureTooLateRejected);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedDisplayedIFramesNb...............%d", Statistics.SpeedDisplayIFramesNb);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedDisplayedPFramesNb...............%d", Statistics.SpeedDisplayPFramesNb);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedDisplayedBFramesNb...............%d", Statistics.SpeedDisplayBFramesNb);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedMaxBitRate.......................%d", Statistics.MaxBitRate);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedMinBitRate.......................%d", Statistics.MinBitRate);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedLastBitRate......................%d", Statistics.LastBitRate);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedMaxPositiveDriftRequested........%d", Statistics.MaxPositiveDriftRequested);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedMaxNegativeDriftRequested........%d", Statistics.MaxNegativeDriftRequested);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedDecodedIPicturesNb...............%d", Statistics.NbDecodedPicturesI);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedDecodedPPicturesNb...............%d", Statistics.NbDecodedPicturesP);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedDecodedBPicturesNb...............%d", Statistics.NbDecodedPicturesB);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedSkipReturnNone...................%d", Statistics.SpeedSkipReturnNone);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SpeedRepeatReturnNone.................%d", Statistics.SpeedRepeatReturnNone);
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n");
            break;
        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(VID_Msg, "one parameter is invalid !\n" );
            break;
        case ST_ERROR_UNKNOWN_DEVICE:
            API_ErrorCount++;
            strcat(VID_Msg, "the specified device is not initialized !\n" );
            break;
        default:
            API_ErrorCount++;
            sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
            break;
    } /* end switch */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    STTST_Print((VID_Msg ));
    return (API_EnableError ? RetErr : FALSE );
#endif /* STVID_DEBUG_GET_STATISTICS on */
} /* end of VID_GetStatistics() */

/*-------------------------------------------------------------------------
 * Function : VID_GetStatisticsProblems
 *            Retrieve driver's statistics problems
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetStatisticsProblems(STTST_Parse_t *pars_p, char *result_sym_p)
{
#ifndef STVID_DEBUG_GET_STATISTICS
    return(FALSE);
#else /* STVID_DEBUG_GET_STATISTICS */

#define VID_MAX_STATISTICS_PB_LIST 8

    ST_ErrorCode_t          ErrCode;
    STVID_Statistics_t      Statistics;
    BOOL                    RetErr;
    BOOL                    CriteriaIsFound;
    char                    Name[2*ST_MAX_DEVICE_NAME]; /* allow big name for error case test */
    ST_DeviceName_t         *DeviceName;
    ST_DeviceName_t         ClkrvName;
#ifdef USE_CLKRV
    STCLKRV_OpenParams_t    stclkrv_OpenParams;
    STCLKRV_Handle_t        CLKRVHndl;
    STCLKRV_ExtendedSTC_t   ExtendedSTC;
    U32                     CheckSync1, CheckSync2, CheckSync3, CheckMin, CheckPreviousSTC, CheckDrift;
    ST_ErrorCode_t          ErrCode2;
#endif
    U32                     i, Value;
    U32                     CheckTry;
    char StrMask[256], * StrFound_p, * StrEval_p; /* Mask to check only some parameters */
    const char StrMaskList [VID_MAX_STATISTICS_PB_LIST][16] = {
        "LiveReset", "MissingPID", "HWSyntaxError", "TooSmallPicture", "TooLateRejected", "CheckSynchro", "TimeOutErr", "IntSyntaxError"
    };
    /* abbreviations : IntSyntaxError is DecodePbInterruptSyntaxError, HWSyntaxError is DecodePbHardwareErrorSyntaxError */

    /* get DeviceName */
    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, Name, sizeof(Name) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected DeviceName" );
        STTST_AssignInteger(result_sym_p, ST_ERROR_UNKNOWN_DEVICE, FALSE);
        return(TRUE);
    }
    DeviceName = (ST_DeviceName_t *)Name ;

#ifdef USE_CLKRV
    CheckSync1 = 0;
    CheckSync2 = 0;
    CheckSync3 = 0;
    CheckPreviousSTC = 0;
    RetErr = STTST_GetString( pars_p, STCLKRV_DEVICE_NAME, ClkrvName, sizeof(ClkrvName) );
#else
    RetErr = STTST_GetString( pars_p, "", ClkrvName, sizeof(ClkrvName) );
#endif
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected clock recovery device name" );
        return( FALSE );
    }

#ifdef STVID_HARDWARE_ERROR_EVENT
    RetErr = STTST_GetString(pars_p, "LiveReset MissingPID HWSyntaxError TooSmallPicture TooLateRejected TimeOutErr IntSyntaxError",
                              StrMask, sizeof(StrMask) );
#else /* STVID_HARDWARE_ERROR_EVENT */
    RetErr = STTST_GetString(pars_p, "LiveReset TooLateRejected TimeOutErr IntSyntaxError",
                              StrMask, sizeof(StrMask) );
#endif /* STVID_HARDWARE_ERROR_EVENT */
    if (RetErr)
    {
#ifdef STVID_HARDWARE_ERROR_EVENT
        STTST_TagCurrentLine(pars_p,
                              "expected string mask (default is \"LiveReset MissingPID HWSyntaxError TooSmallPicture TooLateRejected TimeOutErr IntSyntaxError\")");
#else /* STVID_HARDWARE_ERROR_EVENT */
        STTST_TagCurrentLine(pars_p,
                              "expected string mask (default is \"LiveReset TooLateRejected TimeOutErr IntSyntaxError\")");
#endif /* STVID_HARDWARE_ERROR_EVENT */
        STTBX_Print(("\tOther possible string: CheckSynchro\n"));
        STTBX_Print (("\tExpect a concatenation of the following string seperated by a blank space\n\t"));
        for (i=0; i<VID_MAX_STATISTICS_PB_LIST; i++)
        {
            STTBX_Print (("\"%s\" ", StrMaskList[i]));
        }
        STTBX_Print (("\n\tEach string can be followed by the number not to exceed"));
        STTBX_Print (("\n\t(default is 0 except 7 for CheckSynchro)\n"));
        return(TRUE);
    }

    RetErr = TRUE;
    sprintf(VID_Msg, "VID_GetStatisticsProblems(device=%s,&statistics=%ld) : ",
             *DeviceName, (long)&Statistics );

    ErrCode = STVID_GetStatistics(*DeviceName, &Statistics);
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            strcat(VID_Msg, "ok\n" );
            RetErr = FALSE;
            STTST_Print((VID_Msg ));
            CriteriaIsFound = FALSE;

            for (i = 0; i < VID_MAX_STATISTICS_PB_LIST; i++)
            {
                StrFound_p = strstr(StrMask, StrMaskList[i]);
                if (StrFound_p != NULL)
                {
                    CriteriaIsFound = TRUE;
                    /* Check if a number present after the mask */
                    StrFound_p += strlen(StrMaskList[i]);
                    Value = (U32)strtoul(StrFound_p, &StrEval_p, 10);
                    if (StrEval_p == StrFound_p)
                    {
                        Value = 0;
                        if (i==5)
                        {
                            /* Default value for CheckSynchro is 7 */
                            Value = 7;
                        }
                    }
                    switch(i)
                    {
                        case 0:
                            if (Statistics.ApiPbLiveResetWaitForFirstPictureDetected +
                                Statistics.ApiPbLiveResetWaitForFirstPictureDecoded +
                                Statistics.ApiPbLiveResetWaitForNextPicture      > Value)
                            {
                                sprintf(VID_Msg, "   ApiPbLiveReset........................%d expect < %d\n",
                                        (Statistics.ApiPbLiveResetWaitForFirstPictureDetected +
                                        Statistics.ApiPbLiveResetWaitForFirstPictureDecoded +
                                        Statistics.ApiPbLiveResetWaitForNextPicture) , Value );
                                STTST_Print((VID_Msg ));
                                API_ErrorCount++;
                            }
                            break;

#ifdef STVID_HARDWARE_ERROR_EVENT
                        case 1:
                            if (Statistics.DecodePbHardwareErrorMissingPID > Value)
                            {
                                sprintf(VID_Msg, "   DecodePbHardwareErrorMissingPID.......%d expect < %d\n",
                                         Statistics.DecodePbHardwareErrorMissingPID, Value );
                                STTST_Print((VID_Msg ));
                                API_ErrorCount++;
                            }
                            break;

                        case 2:
                            if (Statistics.DecodePbHardwareErrorSyntaxError > Value)
                            {
                                sprintf(VID_Msg, "   DecodePbHardwareErrorSyntaxError......%d expect < %d\n",
                                         Statistics.DecodePbHardwareErrorSyntaxError, Value );
                                STTST_Print((VID_Msg ));
                                API_ErrorCount++;
                            }
                            break;

                        case 3:
                            if (Statistics.DecodePbHardwareErrorTooSmallPicture > Value)
                            {
                                sprintf(VID_Msg, "   DecodePbHardwareErrorTooSmallPicture..%d expect < %d\n",
                                         Statistics.DecodePbHardwareErrorTooSmallPicture, Value );
                                STTST_Print((VID_Msg ));
                                API_ErrorCount++;
                            }
                            break;
#else /* STVID_HARDWARE_ERROR_EVENT */
                        case 1:
                        case 2:
                        case 3:
                            STTBX_ErrorPrint(("   Hardware error not available.\n"));
                            break;
#endif /* STVID_HARDWARE_ERROR_EVENT */

                        case 4:
                            if (Statistics.DisplayPbPictureTooLateRejectedByMain > Value)
                            {
                                sprintf(VID_Msg, "   DisplayPbPictureTooLateRejectedByMain.......%d expect < %d\n",
                                         Statistics.DisplayPbPictureTooLateRejectedByMain, Value);
                                STTST_Print((VID_Msg ));
                                API_ErrorCount++;
                            }
                            if (Statistics.QueuePbPictureTooLateRejected > Value)
                            {
                                sprintf(VID_Msg, "   QueuePbPictureTooLateRejected.......%d expect < %d\n",
                                        Statistics.QueuePbPictureTooLateRejected, Value);
                                STTST_Print((VID_Msg ));
                                API_ErrorCount++;
                            }
                            /* Added this test because we removed the prepare at last minute feature as it should not happen */
                            if (Statistics.DisplayPbPicturePreparedAtLastMinuteRejected > 0)
                            {
                                sprintf(VID_Msg, "   DisplayPbPicturePreparedAtLastMinuteRejected.......%d expect 0\n",
                                        Statistics.DisplayPbPicturePreparedAtLastMinuteRejected);
                                STTST_Print((VID_Msg ));
                                API_ErrorCount++;
                            }
                            break;

                        case 5:
                            STTBX_Print(("Testing synchronisation, please wait ...."));
                            /* CheckSynchro memorizes value of STCAvailable, NonInterpolatedPTS and CheckedSynchronizedOk */
                            /* Wait 5 seconds and check that values have been incremented of at least Value */
                            /* Do it twice if looping was detected */
                            CheckTry = 0;
#if defined USE_CLKRV
                            ErrCode = STCLKRV_Open(ClkrvName, &stclkrv_OpenParams, &CLKRVHndl);
                            if ( ErrCode != ST_NO_ERROR)
                            {
                                STTBX_ErrorPrint(("\nNot able to open Clock recovery %s !!!\n", ClkrvName));
                                API_ErrorCount++;
                                CheckTry = 3;
                            }
                            else
                            {
                                do
                                {
                                    CheckTry++;
                                    if (CheckTry == 2)
                                    {
                                        /* Wait 0.5sec in case of looping */
                                        STOS_TaskDelayUs(DELAY_1S/2);   /* 0.5s */

                                        /* Get back the new statistics. No need to check errors */
                                        STVID_GetStatistics(*DeviceName, &Statistics);

                                        STTBX_Print(("\nLooping have been detected! Retrying one more time..."));
                                    }

                                    CheckSync1 = Statistics.AvsyncExtendedSTCAvailable;
                                    CheckSync2 = Statistics.AvsyncPictureWithNonInterpolatedPTS;
                                    CheckSync3 = Statistics.AvsyncPictureCheckedSynchronizedOk;

#ifndef STPTI_UNAVAILABLE
                                    ErrCode2 = STCLKRV_GetExtendedSTC(CLKRVHndl, &ExtendedSTC);
                                    if (ErrCode2 != ST_NO_ERROR)
                                    {
                                        /* No STC */
                                        STTBX_ErrorPrint(("\nNot able to retrieve STC clock !!!\n"));
                                        API_ErrorCount++;
                                        CheckTry = 3;
                                        break;
                                    }
                                    CheckPreviousSTC = ExtendedSTC.BaseValue;

                                    /* Wait 5sec */
                                    STOS_TaskDelayUs(5 * DELAY_1S);  /* 5s */

                                    ErrCode2 = STCLKRV_GetExtendedSTC(CLKRVHndl, &ExtendedSTC);
                                    if (ErrCode2 != ST_NO_ERROR)
                                    {
                                        /* No STC */
                                        API_ErrorCount++;
                                        CheckTry = 3;
                                        break;
                                    }
#else /* STPTI_UNAVAILABLE */
                                    /* No PTI, no STC */
                                    STTBX_ErrorPrint(("\nNo STC clock because no PTI !!!\n"));
                                    API_ErrorCount++;
                                    CheckTry = 3;
                                    break;
#endif /* STPTI_UNAVAILABLE */

                                } while((ExtendedSTC.BaseValue <= CheckPreviousSTC) && (CheckTry <= 1));
                                STCLKRV_Close(CLKRVHndl);
                            }

                            if ((ExtendedSTC.BaseValue <= CheckPreviousSTC) || (CheckTry >= 3))
                            {
                                STTBX_ErrorPrint(("\nDon't succeed to test synchro...\n"));
                                API_ErrorCount++;
                            }
                            else
                            {
                                /* Get back the new statistics. No need to check errors */
                                STVID_GetStatistics(*DeviceName, &Statistics);

                                CheckSync1 = Statistics.AvsyncExtendedSTCAvailable - CheckSync1;
                                CheckSync2 = Statistics.AvsyncPictureWithNonInterpolatedPTS - CheckSync2;
                                CheckSync3 = Statistics.AvsyncPictureCheckedSynchronizedOk - CheckSync3;

                                /* Get minimum of CheckSync1 & CheckSync2 */
                                CheckMin = (CheckSync2 > CheckSync1) ? CheckSync1 : CheckSync2;

                                /* Check that value have been incremented by Value (default is 7) */
                                /* DVD stream send at least one PTS every 0.7 sec => 5 seconds gives 7.14 PTS */
                                /* or min(CheckSync1,CheckSync2) should be greater than CheckSync3 + possible drift */
                                /* (arbitrary let say 10% of all PTS received with a minimum of 3) due to differences */
                                /* between stream frequency and display and other things ... */
                                /* Noticed that a 20% error is observed when playing a PAL stream with ouput in  NTSC */
                                /* and vice versa (tried with Jo & echo streams)*/
                                CheckDrift = (CheckMin/10 > 3) ? CheckMin/10 : 3;
                                if ((CheckSync1 < Value ) || (CheckSync2 < Value ) || (CheckSync3 < Value ) ||
                                   (CheckMin > (CheckSync3 + CheckDrift)))
                                {
                                    STTST_Print(("Failed because"));
                                    if ((CheckSync1 < Value ) || (CheckSync2 < Value ) || (CheckSync3 < Value ))
                                    {
                                        sprintf(VID_Msg,
                                                 "\n   Following figures should have be increased by %d during 5 seconds\n", Value);
                                        STTST_Print((VID_Msg ));
                                    }
                                    else
                                    {
                                        sprintf(VID_Msg,
                                                 "     SynchronizedOk+%d <= min(NonInterPTS,STCAvailable)\n", CheckDrift);
                                        STTST_Print((VID_Msg ));
                                    }
                                    sprintf(VID_Msg,
                                             "     AvsyncExtendedSTCAvailable.............. increased by %d\n", CheckSync1);
                                    STTST_Print((VID_Msg ));

                                    sprintf(VID_Msg,
                                             "     AvsyncPictureWithNonInterpolatedPTS..... increased by %d\n", CheckSync2);
                                    STTST_Print((VID_Msg ));
                                    sprintf(VID_Msg,
                                             "     AvsyncPictureCheckedSynchronizedOk...... increased by %d\n", CheckSync3);
                                    STTST_Print((VID_Msg ));
                                    API_ErrorCount++;
                                }
                                else
                                {
                                    STTST_Print(("ok\n"));
                                }

                            }
#else /* not USE_CLKRV */
                            STTBX_ErrorPrint(("\nSTCLKRV not in use !\n"));
#endif /* not USE_CLKRV */
                            break;

                        case 6:
                            if (Statistics.DecodePbDecodeTimeOutError > Value)
                            {
                                sprintf(VID_Msg, "   DecodePbDecodeTimeOutError............%d expect < %d\n",
                                         Statistics.DecodePbDecodeTimeOutError, Value);
                                STTST_Print((VID_Msg ));
                                API_ErrorCount++;
                            }
                            break;

                        case 7:
                            if (Statistics.DecodePbInterruptSyntaxError > Value)
                            {
                                sprintf(VID_Msg, "   DecodePbInterruptSyntaxError..........%d expect < %d\n",
                                         Statistics.DecodePbInterruptSyntaxError, Value);
                                STTST_Print((VID_Msg ));
                                API_ErrorCount++;
                            }
                            break;

                        default:
                            STTBX_ErrorPrint(("   Statistics not implemented !!\n"));
                            break;
                    } /* end switch */
                }
            } /* end for */
            if (!(CriteriaIsFound))
            {
                STTBX_ErrorPrint(("   Unknown mask [%s]\n",StrMask));
            }
            break;

        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(VID_Msg, "one parameter is invalid !\n" );
            STTST_Print((VID_Msg ));
            break;
        case ST_ERROR_UNKNOWN_DEVICE:
            API_ErrorCount++;
            strcat(VID_Msg, "the specified device is not initialized !\n" );
            STTST_Print((VID_Msg ));
            break;
        default:
            API_ErrorCount++;
            sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
            STTST_Print((VID_Msg ));
            break;
    } /* end switch */
    return (API_EnableError ? RetErr : FALSE );
#endif /* STVID_DEBUG_GET_STATISTICS on */
} /* end of VID_GetStatisticsProblems() */

/*-------------------------------------------------------------------------
 * Function : VID_ResetAllStatistics
 *            Resets driver's statistics
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_ResetAllStatistics(STTST_Parse_t *pars_p, char *result_sym_p)
{
#ifndef STVID_DEBUG_GET_STATISTICS
    return(FALSE);
#else /* STVID_DEBUG_GET_STATISTICS */
    ST_ErrorCode_t ErrCode;
    STVID_Statistics_t Statistics;
    BOOL RetErr;
    char Name[2*ST_MAX_DEVICE_NAME]; /* allow big name for error case test */
    ST_DeviceName_t *DeviceName;

    /* get DeviceName */
    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, Name, sizeof(Name) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected DeviceName" );
    /*}
    else
    {*/
        STTST_AssignInteger(result_sym_p, ST_ERROR_UNKNOWN_DEVICE, FALSE);
        /*return(TRUE);*/
    }
    DeviceName = (ST_DeviceName_t *)Name ;

    RetErr = TRUE;
    sprintf(VID_Msg, "STVID_ResetStatistics(device=%s,&statistics=%ld) : ",
             *DeviceName, (long)&Statistics );

    /* Clear all statistics ! */
    memset(&Statistics, 0xFF, sizeof(Statistics));
    ErrCode = STVID_ResetStatistics(*DeviceName, &Statistics);
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            break;
        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(VID_Msg, "one parameter is invalid !\n" );
            break;
        case ST_ERROR_UNKNOWN_DEVICE:
            API_ErrorCount++;
            strcat(VID_Msg, "the specified device is not initialized !\n" );
            break;
        default:
            API_ErrorCount++;
            sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
            break;
    } /* end switch */
    STTST_Print((VID_Msg ));
    return (API_EnableError ? RetErr : FALSE );
#endif /* STVID_DEBUG_GET_STATISTICS on */

} /* end of VID_ResetAllStatistics() */

#ifdef STVID_DEBUG_GET_STATUS
/*-------------------------------------------------------------------------
 * Function : vid_GetScanType
 *            Retreive ScanType string
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void vid_GetScanType(U8 Index, char *String)
{
    switch (Index)
    {
        case 1 :
            strcpy(String, "STVID_SCAN_TYPE_PROGRESSIVE");
            break;
        case 2 :
            strcpy(String, "STVID_SCAN_TYPE_INTERLACED");
            break;
        default:
            strcpy(String, "UNKNOWN");
            break;
    } /* end switch */
}

/*-------------------------------------------------------------------------
 * Function : vid_GetMPEGStandard
 *            Retreive ScanType string
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void vid_GetMPEGStandard(U8 Index, char *String)
{
    switch (Index)
    {
        case 1 :
            strcpy(String, "MPEG 1");
            break;
        case 2 :
            strcpy(String, "MPEG 2");
            break;
        case 4 :
            strcpy(String, "MPEG-4 Part 10 - H264");
            break;
        case 8 :
            strcpy(String, "VC1");
            break;
        case 16 :
            strcpy(String, "MPEG-4 Part 2 - DivX");
            break;
        default:
            strcpy(String, "UNKNOWN");
            break;
    } /* end switch */
}

/*-------------------------------------------------------------------------
 * Function : vid_GetStateMachine
 *            Retreive ScanType string
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void vid_GetStateMachine(U8 Index, char *String)
{
    switch (Index)
    {
        case 0 :
            strcpy(String, "VIDEO_STATE_STOPPED");
            break;
        case 1 :
            strcpy(String, "VIDEO_STATE_RUNNING");
            break;
        case 2 :
            strcpy(String, "VIDEO_STATE_PAUSING");
            break;
        case 3 :
            strcpy(String, "VIDEO_STATE_FREEZING");
            break;
        default:
            strcpy(String, "UNKNOWN");
            break;
    } /* end switch */
}
#endif /* STVID_DEBUG_GET_STATUS */


/*-------------------------------------------------------------------------
 * Function : VID_GetStatus
 *            Retreive driver's status
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetStatus(STTST_Parse_t *pars_p, char *result_sym_p)
{
#ifndef STVID_DEBUG_GET_STATUS
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    return(FALSE);
#else /* STVID_DEBUG_GET_STATUS */
    ST_ErrorCode_t ErrCode;
    STVID_Status_t Status;
    BOOL RetErr;
    char Name[2*ST_MAX_DEVICE_NAME]; /* allow big name for error case test */
    ST_DeviceName_t *DeviceName;
    char TempString[30];

    /* get DeviceName */
    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, Name, sizeof(Name) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected DeviceName" );
        return(TRUE);
    }
    DeviceName = (ST_DeviceName_t *)Name ;

    RetErr = TRUE;
    sprintf(VID_Msg, "STVID_GetStatus(device=%s,&status=%ld) : ",
             *DeviceName, (long)&Status );

    ErrCode = STVID_GetStatus(*DeviceName, &Status);

    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;

            strcat(VID_Msg, "ok\n" );
            STTST_Print((VID_Msg ));

            /* Allocated buffers */
            sprintf(VID_Msg, "\n BitBufferAddress_p....................0x%x", (U32)Status.BitBufferAddress_p );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n BitBufferSize.........................%d", (U32)Status.BitBufferSize );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DataInputBufferAddress_p..............0x%x", (U32)Status.DataInputBufferAddress_p );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DataInputBufferSize...................%d", (U32)Status.DataInputBufferSize );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SCListBufferAddress_p.................0x%x", (U32)Status.SCListBufferAddress_p );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SCListBufferSize......................%d", (U32)Status.SCListBufferSize );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n IntermediateBuffersAddress_p..........0x%x", (U32)Status.IntermediateBuffersAddress_p );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n IntermediateBuffersSize...............%d", (U32)Status.IntermediateBuffersSize );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n FdmaNodesSize.........................%d", (U32)Status.FdmaNodesSize );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n InjectFlushBufferSize.................%d", (U32)Status.InjectFlushBufferSize );
            STTST_Print((VID_Msg ));

            /* Memory profile related */
            sprintf(VID_Msg, "\n");
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n MemoryProfile.........................");
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + MaxHeight...........................%d", Status.MemoryProfile.MaxHeight );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + MaxWidth............................%d", Status.MemoryProfile.MaxWidth );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + NbFrameStore........................%d", Status.MemoryProfile.NbFrameStore );
            STTST_Print((VID_Msg ));

            /* SyncDelay related */
            sprintf(VID_Msg, "\n");
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SyncDelay.............................%d", Status.SyncDelay );
            STTST_Print((VID_Msg ));

            /* StateMachine related */
            sprintf(VID_Msg, "\n");
            STTST_Print((VID_Msg ));
            vid_GetStateMachine(Status.StateMachine, TempString);
            sprintf(VID_Msg, "\n StateMachine..........................%s", TempString );
            STTST_Print((VID_Msg ));

            /* LastStartParams related */
            sprintf(VID_Msg, "\n");
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n VideoAlreadyStarted...................%s", ((Status.VideoAlreadyStarted == TRUE) ? "TRUE" : "FALSE"));
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n LastStartParams.......................");
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + RealTime............................%d", Status.LastStartParams.RealTime );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + UpdateDisplay.......................%d", Status.LastStartParams.UpdateDisplay );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + BrdCstProfile.......................%d", Status.LastStartParams.BrdCstProfile );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + StreamType..........................%d", Status.LastStartParams.StreamType );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + StreamID............................%d", Status.LastStartParams.StreamID );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + DecodeOnce..........................%d", Status.LastStartParams.DecodeOnce );
            STTST_Print((VID_Msg ));

            /* LastFreezeParams related */
            sprintf(VID_Msg, "\n");
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n VideoAlreadyFreezed...................%s", ((Status.VideoAlreadyFreezed == TRUE) ? "TRUE" : "FALSE"));
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n LastFreezeParams......................");
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + Mode................................%d", Status.LastFreezeParams.Mode );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + Field...............................%d", Status.LastFreezeParams.Field );
            STTST_Print((VID_Msg ));

            /* ES, PES, SC Pointers related */
            sprintf(VID_Msg, "\n");
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n BitBufferReadPointer..................0x%x", (U32)Status.BitBufferReadPointer );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n BitBufferWritePointer.................0x%x", (U32)Status.BitBufferWritePointer );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DataInputBufferReadPointer............0x%x", (U32)Status.DataInputBufferReadPointer );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n DataInputBufferWritePointer...........0x%x", (U32)Status.DataInputBufferWritePointer );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SCListBufferReadPointer...............0x%x", (U32)Status.SCListBufferReadPointer );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SCListBufferWritePointer..............0x%x", (U32)Status.SCListBufferWritePointer );
            STTST_Print((VID_Msg ));

            /* SequenceInfo related */
            sprintf(VID_Msg, "\n");
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n SequenceInfo_p........................%x", (U32)Status.SequenceInfo_p );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + Height..............................%d", Status.SequenceInfo_p->Height );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + Width...............................%d", Status.SequenceInfo_p->Width );
            STTST_Print((VID_Msg ));

            vid_GetScanType(Status.SequenceInfo_p->ScanType, TempString);
            sprintf(VID_Msg, "\n + ScanType............................%s", TempString );
            STTST_Print((VID_Msg ));

            sprintf(VID_Msg, "\n + FrameRate...........................%d", Status.SequenceInfo_p->FrameRate );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + BitRate.............................%d", Status.SequenceInfo_p->BitRate );
            STTST_Print((VID_Msg ));

            vid_GetMPEGStandard(Status.SequenceInfo_p->MPEGStandard, TempString);
            sprintf(VID_Msg, "\n + MPEGStandard........................%s", TempString );
            STTST_Print((VID_Msg ));

            sprintf(VID_Msg, "\n + IsLowDelay..........................%d", Status.SequenceInfo_p->IsLowDelay );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + VBVBufferSize.......................%d", Status.SequenceInfo_p->VBVBufferSize );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + StreamID............................%d", Status.SequenceInfo_p->StreamID );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + ProfileAndLevelIndication...........%d", Status.SequenceInfo_p->ProfileAndLevelIndication );
            STTST_Print((VID_Msg ));
            sprintf(VID_Msg, "\n + VideoFormat.........................%d", Status.SequenceInfo_p->VideoFormat );
            STTST_Print((VID_Msg ));

            sprintf(VID_Msg, "\n");
            STTST_Print((VID_Msg ));

            break;
        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(VID_Msg, "one parameter is invalid !\n" );
            STTST_Print((VID_Msg ));
            break;
        case ST_ERROR_UNKNOWN_DEVICE:
            API_ErrorCount++;
            strcat(VID_Msg, "the specified device is not initialized !\n" );
            STTST_Print((VID_Msg ));
            break;
        default:
            API_ErrorCount++;
            sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
            STTST_Print((VID_Msg ));
            break;
    } /* end switch */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );
#endif /* STVID_DEBUG_GET_STATUS on */
} /* end of VID_GetStatus() */



#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_GetViewPortAlpha
 *            Get viewport global Alpha value
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetViewPortAlpha(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STLAYER_GlobalAlpha_t VPAlpha;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_GetViewPortAlpha(CurrentViewportHndl, &VPAlpha );
        sprintf(VID_Msg, "STVID_GetViewPortAlpha(viewporthndl=0x%x, A0=%d, A1=%d) : ",
                          (S32) CurrentViewportHndl, (U8)VPAlpha.A0, (U8)VPAlpha.A1);
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", VPAlpha.A0, FALSE);
                STTST_AssignInteger("RET_VAL2", VPAlpha.A1, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE:
                API_ErrorCount++;
                strcat(VID_Msg, "Feature is not available !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_GetViewPortAlpha() */

/*-------------------------------------------------------------------------
 * Function : VID_GetViewPortColorKey
 *            Get viewport color key
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetViewPortColorKey(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STGXOBJ_ColorKey_t VPColorKey;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_GetViewPortColorKey(CurrentViewportHndl, &VPColorKey );

        sprintf(VID_Msg, "STVID_GetViewPortColorKey(viewporthndl=0x%x,&VPColorKey=0x%x) : ",
                          (S32) CurrentViewportHndl, (S32)&VPColorKey);
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                sprintf(VID_Msg, "%s   ColorKey.Type=%d ColorKey.Value.CLUTI1=%d %d %d %d\n",
                            VID_Msg, VPColorKey.Type,
                            VPColorKey.Value.CLUT1.PaletteEntryMin,
                            VPColorKey.Value.CLUT1.PaletteEntryMax,
                            VPColorKey.Value.CLUT1.PaletteEntryOut,
                            VPColorKey.Value.CLUT1.PaletteEntryEnable );
                sprintf(VID_Msg, "%s   ColorKey.Value.CLUTI8=%d %d %d %d\n",
                            VID_Msg,
                            VPColorKey.Value.CLUT8.PaletteEntryMin,
                            VPColorKey.Value.CLUT8.PaletteEntryMax,
                            VPColorKey.Value.CLUT8.PaletteEntryOut,
                            VPColorKey.Value.CLUT8.PaletteEntryEnable );
                VID_PRINT((VID_Msg));
                sprintf(VID_Msg, "   ColorKey.Value.RGB888.R=%d %d %d %d .G=%d %d %d %d .B=%d %d %d %d\n",
                            VPColorKey.Value.RGB888.RMin,
                            VPColorKey.Value.RGB888.RMax,
                            VPColorKey.Value.RGB888.ROut,
                            VPColorKey.Value.RGB888.REnable,
                            VPColorKey.Value.RGB888.GMin,
                            VPColorKey.Value.RGB888.GMax,
                            VPColorKey.Value.RGB888.GOut,
                            VPColorKey.Value.RGB888.GEnable,
                            VPColorKey.Value.RGB888.BMin,
                            VPColorKey.Value.RGB888.BMax,
                            VPColorKey.Value.RGB888.BOut,
                            VPColorKey.Value.RGB888.BEnable);
                sprintf(VID_Msg, "   ColorKey.Value.SignedYCbCr888.Y=%d %d %d %d .Cb=%d %d %d %d .Cr=%d %d %d %d\n",
                            VPColorKey.Value.SignedYCbCr888.YMin,
                            VPColorKey.Value.SignedYCbCr888.YMax,
                            VPColorKey.Value.SignedYCbCr888.YOut,
                            VPColorKey.Value.SignedYCbCr888.YEnable,
                            VPColorKey.Value.SignedYCbCr888.CbMin,
                            VPColorKey.Value.SignedYCbCr888.CbMax,
                            VPColorKey.Value.SignedYCbCr888.CbOut,
                            VPColorKey.Value.SignedYCbCr888.CbEnable,
                            VPColorKey.Value.SignedYCbCr888.CrMin,
                            VPColorKey.Value.SignedYCbCr888.CrMax,
                            VPColorKey.Value.SignedYCbCr888.CrOut,
                            VPColorKey.Value.SignedYCbCr888.CrEnable );
                sprintf(VID_Msg, "   ColorKey.Value.UnsignedYCbCr888.Y=%d %d %d %d .Cb=%d %d %d %d .Cr=%d %d %d %d\n",
                            VPColorKey.Value.UnsignedYCbCr888.YMin,
                            VPColorKey.Value.UnsignedYCbCr888.YMax,
                            VPColorKey.Value.UnsignedYCbCr888.YOut,
                            VPColorKey.Value.UnsignedYCbCr888.YEnable,
                            VPColorKey.Value.UnsignedYCbCr888.CbMin,
                            VPColorKey.Value.UnsignedYCbCr888.CbMax,
                            VPColorKey.Value.UnsignedYCbCr888.CbOut,
                            VPColorKey.Value.UnsignedYCbCr888.CbEnable,
                            VPColorKey.Value.UnsignedYCbCr888.CrMin,
                            VPColorKey.Value.UnsignedYCbCr888.CrMax,
                            VPColorKey.Value.UnsignedYCbCr888.CrOut,
                            VPColorKey.Value.UnsignedYCbCr888.CrEnable );
                STTST_AssignInteger("RET_VAL1", (S32)VPColorKey.Type, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE:
                API_ErrorCount++;
                strcat(VID_Msg, "Feature is not available !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_GetViewPortColorKey() */

/*-------------------------------------------------------------------------
 * Function : VID_GetViewPortPSI
 *            Get Picture Structure Improvement param.
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetViewPortPSI(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    char  Mode[10];
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STLAYER_PSI_t PSIParam;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    if (!(RetErr))
    {
        /* Then get video filtering type.       */
        RetErr = STTST_GetInteger(pars_p, 0xFF, &LVar );
        if (RetErr)
        {
            sprintf(VID_Msg, "expected Filter type (AF:%d, GB:%d, TINT:%d, SAT:%d, ER:%d, PEAK:%d, DCI:%d, BC:%d)",
                STLAYER_VIDEO_CHROMA_AUTOFLESH, STLAYER_VIDEO_CHROMA_GREENBOOST, STLAYER_VIDEO_CHROMA_TINT,
                STLAYER_VIDEO_CHROMA_SAT, STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT, STLAYER_VIDEO_LUMA_PEAKING,
                STLAYER_VIDEO_LUMA_DCI,STLAYER_VIDEO_LUMA_BC);
            STTST_TagCurrentLine(pars_p, VID_Msg);
        }
    }
    if (!(RetErr))
    {
        PSIParam.VideoFiltering = LVar;
        ErrCode = STVID_GetViewPortPSI(CurrentViewportHndl, &PSIParam );
        sprintf(VID_Msg, "STVID_GetViewPortPSI(handle=0x%x) : ",
                 (U32)CurrentViewportHndl);

            switch (ErrCode)
            {
                case ST_NO_ERROR :
                    strcat(VID_Msg, "ok\n");
                    VID_PRINT((VID_Msg ));

                    /* Get working mode.    */
                    sprintf(Mode, "%s",
                            (PSIParam.VideoFilteringControl == STLAYER_DISABLE) ? "DISABLE" :
                            (PSIParam.VideoFilteringControl == STLAYER_ENABLE_AUTO_MODE1) ? "AUTO 1" :
                            (PSIParam.VideoFilteringControl == STLAYER_ENABLE_AUTO_MODE2) ? "AUTO 2" :
                            (PSIParam.VideoFilteringControl == STLAYER_ENABLE_AUTO_MODE3) ? "AUTO 3" :
                            (PSIParam.VideoFilteringControl == STLAYER_ENABLE_MANUAL) ? "MANUAL" : "UNKNOWN");

                    switch (PSIParam.VideoFiltering)
                    {
                        case STLAYER_VIDEO_CHROMA_AUTOFLESH :
                            sprintf(VID_Msg, "AUTO FLESH (%s): Ctrl %d%%, Width %s, Axis %s\n",
                                Mode,
                                PSIParam.VideoFilteringParameters.AutoFleshParameters.AutoFleshControl,
                                (PSIParam.VideoFilteringParameters.AutoFleshParameters.QuadratureFleshWidthControl == LARGE_WIDTH) ? "LARGE" :
                                (PSIParam.VideoFilteringParameters.AutoFleshParameters.QuadratureFleshWidthControl == MEDIUM_WIDTH)? "MEDIUM":
                                "SMALL",
                                (PSIParam.VideoFilteringParameters.AutoFleshParameters.AutoFleshAxisControl == AXIS_116_6) ? "116_6" :
                                (PSIParam.VideoFilteringParameters.AutoFleshParameters.AutoFleshAxisControl == AXIS_121_0) ? "121_0" :
                                (PSIParam.VideoFilteringParameters.AutoFleshParameters.AutoFleshAxisControl == AXIS_125_5) ? "125_5" :
                                "AXIS_130_2");
                            break;

                        case STLAYER_VIDEO_CHROMA_GREENBOOST :
                            sprintf(VID_Msg, "GREEN BOOST (%s): Ctrl %d%%\n",
                                Mode,
                                PSIParam.VideoFilteringParameters.GreenBoostParameters.GreenBoostControl);
                            break;

                        case STLAYER_VIDEO_CHROMA_TINT :
                            sprintf(VID_Msg, "TINT (%s): Ctrl %d%%\n",
                                Mode,
                                PSIParam.VideoFilteringParameters.TintParameters.TintRotationControl);
                            break;

                        case STLAYER_VIDEO_CHROMA_SAT :
                            sprintf(VID_Msg, "SAT (%s): Ctrl %d%%\n",
                                Mode,
                                PSIParam.VideoFilteringParameters.SatParameters.SaturationGainControl);
                            break;

                        case STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT :
                            sprintf(VID_Msg, "EDGE REPLACEMENT (%s): Ctrl %d%%, Frequency %s\n",
                                Mode,
                                PSIParam.VideoFilteringParameters.EdgeReplacementParameters.GainControl,
                                (PSIParam.VideoFilteringParameters.EdgeReplacementParameters.FrequencyControl == HIGH_FREQ_FILTER) ? "HIGH" :
                                (PSIParam.VideoFilteringParameters.EdgeReplacementParameters.FrequencyControl == MEDIUM_FREQ_FILTER) ? "MEDIUM" :
                                "LOW");
                            break;

                        case STLAYER_VIDEO_LUMA_PEAKING :
                            sprintf(VID_Msg, "LUMA PEAKING (%s): VPeak %d%%, VCore %d%%, HPeak %d%%, HCore %d%%, Fs/Fc %d%%, Sien %d\n",
                                Mode,
                                PSIParam.VideoFilteringParameters.PeakingParameters.VerticalPeakingGainControl,
                                PSIParam.VideoFilteringParameters.PeakingParameters.CoringForVerticalPeaking,
                                PSIParam.VideoFilteringParameters.PeakingParameters.HorizontalPeakingGainControl,
                                PSIParam.VideoFilteringParameters.PeakingParameters.CoringForHorizontalPeaking,
                                PSIParam.VideoFilteringParameters.PeakingParameters.HorizontalPeakingFilterSelection,
                                PSIParam.VideoFilteringParameters.PeakingParameters.SINECompensationEnable);
                            break;

                        case STLAYER_VIDEO_LUMA_DCI :
                            sprintf(VID_Msg, "LUMA DCI (%s): Core %d%%, FirstPixel %d, LastPixel %d, FirstLine %d, LastLine %d\n",
                                Mode,
                                PSIParam.VideoFilteringParameters.DCIParameters.CoringLevelGainControl,
                                PSIParam.VideoFilteringParameters.DCIParameters.FirstPixelAnalysisWindow,
                                PSIParam.VideoFilteringParameters.DCIParameters.LastPixelAnalysisWindow,
                                PSIParam.VideoFilteringParameters.DCIParameters.FirstLineAnalysisWindow,
                                PSIParam.VideoFilteringParameters.DCIParameters.LastLineAnalysisWindow);
                            break;

                        case STLAYER_VIDEO_LUMA_BC :
                            sprintf(VID_Msg, "BriSat (%s): Bri %d Sat %d \n",
                                Mode,
                                PSIParam.VideoFilteringParameters.BCParameters.BrightnessGainControl,
                                PSIParam.VideoFilteringParameters.BCParameters.ContrastGainControl);
                            break;

                        default :
                                sprintf(VID_Msg, "Unexpected video filtering type  !!!\n");
                            break;
                    } /* end switch */
                RetErr = FALSE;
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "feature not available !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_GetViewPortPSI() */


#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
/*-------------------------------------------------------------------------
 * Function : VID_GetVideoDisplayParams
 *            Get viewport global Alpha value
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetVideoDisplayParams(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STLAYER_VideoDisplayParams_t VidDispParams;
    S32 LVar;
    int b=0;
    int k=0;
    int Option=0;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        /* By default Option=0x0B (Bits 1,2,3,4 sets) */
        RetErr = STTST_GetInteger(pars_p, 0x0F, &Option );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Option mask: Bit 1=VHSRSC params, Bit 2=Input/Output params, Bit 3=DEI/Decimation/Scan params, Bit 4=VHSRC Filters, Bit 5=VHSRC Filters coeffs" );
        }

        RetErr = TRUE;
        ErrCode = STVID_GetVideoDisplayParams(CurrentViewportHndl, &VidDispParams );
        sprintf(VID_Msg, "\nSTVID_GetVideoDisplayParams(viewporthndl=0x%x): \n",
                          (S32) CurrentViewportHndl);

        switch (ErrCode)
        {
            case ST_NO_ERROR :
                strcat(VID_Msg, "ok\n");
                VID_PRINT((VID_Msg));
                if(Option&0x2)
                {
                    sprintf(VID_Msg, "Input Params:\n");
                    VID_PRINT((VID_Msg ));
                    sprintf(VID_Msg, "  PosX=%d, PosY=%d, Width=%d, Height=%d\n",
                                      (U32)VidDispParams.InputRectangle.PositionX/16, (U32)VidDispParams.InputRectangle.PositionY/16,(U32)VidDispParams.InputRectangle.Width,(U32)VidDispParams.InputRectangle.Height);
                    VID_PRINT((VID_Msg ));

                    sprintf(VID_Msg, "Output Params:\n");
                    VID_PRINT((VID_Msg ));
                    sprintf(VID_Msg, "  PosX=%d, PosY=%d, Width=%d, Height=%d\n",
                                      (U32)VidDispParams.OutputRectangle.PositionX, (U32)VidDispParams.OutputRectangle.PositionY,(U32)VidDispParams.OutputRectangle.Width,(U32)VidDispParams.OutputRectangle.Height);
                    VID_PRINT((VID_Msg ));

                    RetErr |= STTST_AssignInteger("RET_VAL1",  VidDispParams.InputRectangle.PositionX/16, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL2",  VidDispParams.InputRectangle.PositionY/16, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL3",  VidDispParams.InputRectangle.Width, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL4",  VidDispParams.InputRectangle.Height, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL5",  VidDispParams.OutputRectangle.PositionX, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL6",  VidDispParams.OutputRectangle.PositionY, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL7",  VidDispParams.OutputRectangle.Width, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL8",  VidDispParams.OutputRectangle.Height, FALSE);
                }
                if (Option&0x1)
                {
                    sprintf(VID_Msg, "LumaHorizontal:\n");
                    VID_PRINT((VID_Msg ));
                    sprintf(VID_Msg, "  Increment=0x%x, InitialPhase=0x%x, PixelRepeat=0x%x\n",
                                      (U32)VidDispParams.VHSRCparams.LumaHorizontal.Increment, (U32)VidDispParams.VHSRCparams.LumaHorizontal.InitialPhase,(U32)VidDispParams.VHSRCparams.LumaHorizontal.PixelRepeat);
                    VID_PRINT((VID_Msg ));
                    sprintf(VID_Msg, "ChromaHorizontal:\n");
                    VID_PRINT((VID_Msg ));
                    sprintf(VID_Msg, "  Increment=0x%x, InitialPhase=0x%x, PixelRepeat=0x%x\n",
                                      (U32)VidDispParams.VHSRCparams.ChromaHorizontal.Increment, (U32)VidDispParams.VHSRCparams.ChromaHorizontal.InitialPhase,(U32)VidDispParams.VHSRCparams.ChromaHorizontal.PixelRepeat);
                    VID_PRINT((VID_Msg ));

                    sprintf(VID_Msg, "LumaTop:\n");
                    VID_PRINT((VID_Msg ));

                    sprintf(VID_Msg, "  Increment=0x%x, InitialPhase=0x%x, PixelRepeat=0x%x\n",
                                      (U32)VidDispParams.VHSRCparams.LumaTop.Increment, (U32)VidDispParams.VHSRCparams.LumaTop.InitialPhase,(U32)VidDispParams.VHSRCparams.LumaTop.LineRepeat);
                    VID_PRINT((VID_Msg ));

                    sprintf(VID_Msg, "ChromaTop:\n");
                    VID_PRINT((VID_Msg ));

                    sprintf(VID_Msg, "  Increment=0x%x, InitialPhase=0x%x, PixelRepeat=0x%x\n",
                                      (U32)VidDispParams.VHSRCparams.ChromaTop.Increment, (U32)VidDispParams.VHSRCparams.ChromaTop.InitialPhase,(U32)VidDispParams.VHSRCparams.ChromaTop.LineRepeat);
                    VID_PRINT((VID_Msg ));

                    sprintf(VID_Msg, "LumaBot:\n");
                    VID_PRINT((VID_Msg ));

                    sprintf(VID_Msg, "  Increment=0x%x, InitialPhase=0x%x, PixelRepeat=0x%x\n",
                                      (U32)VidDispParams.VHSRCparams.LumaBot.Increment, (U32)VidDispParams.VHSRCparams.LumaBot.InitialPhase,(U32)VidDispParams.VHSRCparams.LumaBot.LineRepeat);
                    VID_PRINT((VID_Msg ));

                    sprintf(VID_Msg, "ChromaBot:\n");
                    VID_PRINT((VID_Msg ));

                    sprintf(VID_Msg, "  Increment=0x%x, InitialPhase=0x%x, PixelRepeat=0x%x\n",
                                      (U32)VidDispParams.VHSRCparams.ChromaBot.Increment, (U32)VidDispParams.VHSRCparams.ChromaBot.InitialPhase,(U32)VidDispParams.VHSRCparams.ChromaBot.LineRepeat);
                    VID_PRINT((VID_Msg ));

                    RetErr |= STTST_AssignInteger("RET_VAL1",  VidDispParams.VHSRCparams.LumaHorizontal.Increment, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL2",  VidDispParams.VHSRCparams.LumaHorizontal.InitialPhase, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL3",  VidDispParams.VHSRCparams.LumaHorizontal.PixelRepeat, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL4",  VidDispParams.VHSRCparams.ChromaHorizontal.Increment, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL5",  VidDispParams.VHSRCparams.ChromaHorizontal.InitialPhase, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL6",  VidDispParams.VHSRCparams.ChromaHorizontal.PixelRepeat, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL7",  VidDispParams.VHSRCparams.LumaTop.Increment, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL8",  VidDispParams.VHSRCparams.LumaTop.InitialPhase, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL9",  VidDispParams.VHSRCparams.LumaTop.LineRepeat, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL10",  VidDispParams.VHSRCparams.ChromaTop.Increment, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL11",  VidDispParams.VHSRCparams.ChromaTop.InitialPhase, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL12",  VidDispParams.VHSRCparams.ChromaTop.LineRepeat, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL13",  VidDispParams.VHSRCparams.LumaBot.Increment, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL14",  VidDispParams.VHSRCparams.LumaBot.InitialPhase, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL15",  VidDispParams.VHSRCparams.LumaBot.LineRepeat, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL16",  VidDispParams.VHSRCparams.ChromaBot.Increment, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL17",  VidDispParams.VHSRCparams.ChromaBot.InitialPhase, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL18",  VidDispParams.VHSRCparams.ChromaBot.LineRepeat, FALSE);
                }
                if(Option&0x4)
                {
                    switch(VidDispParams.DeiMode)
                    {
                        case STLAYER_DEI_MODE_OFF:
                            sprintf(VID_Msg, "DEI Mode: MODE_OFF\n");
                        VID_PRINT((VID_Msg ));
                            break;
                        case STLAYER_DEI_MODE_AUTO:
                            sprintf(VID_Msg, "DEI Mode: MODE_AUTO\n");
                        VID_PRINT((VID_Msg ));
                           break;
                        case STLAYER_DEI_MODE_BYPASS:
                            sprintf(VID_Msg, "DEI Mode: MODE_BYPASS\n");
                        VID_PRINT((VID_Msg ));
                           break;
                        case STLAYER_DEI_MODE_VERTICAL:
                            sprintf(VID_Msg, "DEI Mode: MODE_VERTICAL\n");
                        VID_PRINT((VID_Msg ));
                            break;
                        case STLAYER_DEI_MODE_DIRECTIONAL:
                            sprintf(VID_Msg, "DEI Mode: MODE_DIRECTIONAL\n");
                        VID_PRINT((VID_Msg ));
                           break;
                        case STLAYER_DEI_MODE_FIELD_MERGING:
                            sprintf(VID_Msg, "DEI Mode: MODE_FIELD_MERGING\n");
                        VID_PRINT((VID_Msg ));
                            break;
                        case STLAYER_DEI_MODE_MEDIAN_FILTER:
                            sprintf(VID_Msg, "DEI Mode: MODE_MEDIAN_FILTER\n");
                        VID_PRINT((VID_Msg ));
                           break;
                        case STLAYER_DEI_MODE_MLD:
                            sprintf(VID_Msg, "DEI Mode: MODE_MLD\n");
                        VID_PRINT((VID_Msg ));
                            break;
                        case STLAYER_DEI_MODE_LMU:
                            sprintf(VID_Msg, "DEI Mode: MODE_LMU\n");
                        VID_PRINT((VID_Msg ));
                            break;
                        default:
                            printf("DEIMode: Unknown type !!!\n");
                            break;
                    }

                    switch(VidDispParams.VerticalDecimFactor)
                    {
                        case STLAYER_DECIMATION_FACTOR_NONE:
                            sprintf(VID_Msg, "VerticalDecimFactor: FACTOR_NONE\n");
                            VID_PRINT((VID_Msg ));
                            break;
                        case STLAYER_DECIMATION_FACTOR_2:
                            sprintf(VID_Msg, "VerticalDecimFactor: FACTOR_2\n");
                            VID_PRINT((VID_Msg ));
                            break;
                        case STLAYER_DECIMATION_FACTOR_4:
                            sprintf(VID_Msg, "VerticalDecimFactor: FACTOR_4\n");
                            VID_PRINT((VID_Msg ));
                            break;
                        case STLAYER_DECIMATION_MPEG4_P2:
                            sprintf(VID_Msg, "VerticalDecimFactor: MPEG4_P2\n");
                            VID_PRINT((VID_Msg ));
                            break;
                         default:
                            printf("VerticalDecimFactor: Unknown type !!!\n");
                            break;
                    }

                    switch(VidDispParams.HorizontalDecimFactor)
                    {
                       case STLAYER_DECIMATION_FACTOR_NONE:
                           sprintf(VID_Msg, "HorizontalDecimFactor: FACTOR_NONE\n");
                           VID_PRINT((VID_Msg ));
                           break;
                       case STLAYER_DECIMATION_FACTOR_2:
                           sprintf(VID_Msg, "HorizontalDecimFactor: FACTOR_2\n");
                           VID_PRINT((VID_Msg ));
                           break;
                       case STLAYER_DECIMATION_FACTOR_4:
                           sprintf(VID_Msg, "HorizontalDecimFactor: FACTOR_4\n");
                           VID_PRINT((VID_Msg ));
                           break;
                       case STLAYER_DECIMATION_MPEG4_P2:
                           sprintf(VID_Msg, "HorizontalDecimFactor: MPEG4_P2\n");
                           VID_PRINT((VID_Msg ));
                           break;
                       default:
                           printf("HorizontalDecimFactor: Unknown type !!!\n");
                           break;
                    }

                    switch(VidDispParams.SourceScanType)
                    {
                        case STGXOBJ_PROGRESSIVE_SCAN:
                            sprintf(VID_Msg, "SourceScanType: PROGRESSIVE\n");
                            VID_PRINT((VID_Msg ));
                            break;
                        case STGXOBJ_INTERLACED_SCAN:
                            sprintf(VID_Msg, "SourceScanType: INTERLACED\n");
                            VID_PRINT((VID_Msg ));
                            break;
                         default:
                            printf("SourceScanType: Unknown type !!!\n");
                            break;
                    }

                    RetErr |= STTST_AssignInteger("RET_VAL1",  VidDispParams.DeiMode, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL2",  VidDispParams.VerticalDecimFactor, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL3",  VidDispParams.HorizontalDecimFactor, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL4",  VidDispParams.SourceScanType, FALSE);
                }
                if (Option&0x8)
                {
                    sprintf(VID_Msg, "LumaVerticalCoefType    : %s\n", VhsrcFilterType[VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoefType] );
                    VID_PRINT((VID_Msg ));
                    sprintf(VID_Msg, "ChromaVerticalCoefType  : %s\n", VhsrcFilterType[VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoefType] );
                    VID_PRINT((VID_Msg ));
                    sprintf(VID_Msg, "LumaHorizontalCoefType  : %s\n", VhsrcFilterType[VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoefType] );
                    VID_PRINT((VID_Msg ));
                    sprintf(VID_Msg, "ChromaHorizontalCoefType: %s\n", VhsrcFilterType[VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoefType] );
                    VID_PRINT((VID_Msg ));

                    RetErr |= STTST_AssignInteger("RET_VAL1",  VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoefType, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL2",  VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoefType, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL3",  VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoefType, FALSE);
                    RetErr |= STTST_AssignInteger("RET_VAL4",  VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoefType, FALSE);
                }
                if (Option&0x10)
                {
                    sprintf(VID_Msg, "LumaHorizontalCoef \n");
                    VID_PRINT((VID_Msg ));

                    for(b=0;b<8;b++)
                    {
                       sprintf(VID_Msg, "%4d %4d %4d ",VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[16*b],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+1],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+2]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d    %4d %4d ",VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+4],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+4],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+5]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d %4d    %4d ",VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+6],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+7],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+8]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d %4d %4d    ",VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+9],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+10],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+11]);
                        VID_PRINT((VID_Msg));
                        sprintf(VID_Msg, "%4d %4d %4d %4d\n",VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+12],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+13],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+14],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+15]);
                        VID_PRINT((VID_Msg));
                    }
                    for(k=0;k<3;k++)
                    {
                        sprintf(VID_Msg, "%4d %4d %4d %4d    ",VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[128+(4*k)],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[128+(4*k)+1],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[128+(4*k)+2],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[128+(4*k)+3]);
                        VID_PRINT((VID_Msg));
                    }
                    sprintf(VID_Msg, "\n");
                    VID_PRINT((VID_Msg));

                    sprintf(VID_Msg, "ChromaHorizontalCoef \n");
                    VID_PRINT((VID_Msg ));

                    for(b=0;b<8;b++)
                    {
                       sprintf(VID_Msg, "%4d %4d %4d ",VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[16*b],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+1],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+2]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d    %4d %4d ",VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+4],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+4],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+5]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d %4d    %4d ",VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+6],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+7],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+8]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d %4d %4d    ",VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+9],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+10],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+11]);
                        VID_PRINT((VID_Msg));
                        sprintf(VID_Msg, "%4d %4d %4d %4d\n",VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[(16*b)+12],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+13],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+14],VidDispParams.VHSRCVideoDisplayFilters.LumaHorizontalCoef[(16*b)+15]);
                        VID_PRINT((VID_Msg));
                    }
                    for(k=0;k<3;k++)
                    {
                        sprintf(VID_Msg, "%4d %4d %4d %4d    ",VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[128+(4*k)],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[128+(4*k)+1],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[128+(4*k)+2],VidDispParams.VHSRCVideoDisplayFilters.ChromaHorizontalCoef[128+(4*k)+3]);
                        VID_PRINT((VID_Msg));
                    }
                    sprintf(VID_Msg, "\n");
                    VID_PRINT((VID_Msg));

                    sprintf(VID_Msg, "LumaVerticalCoef \n");
                    VID_PRINT((VID_Msg ));

                    for(b=0;b<5;b++)
                    {
                       sprintf(VID_Msg, "%4d %4d %4d ",VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[16*b],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+1],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+2]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d    %4d %4d ",VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+4],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+4],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+5]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d %4d    %4d ",VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+6],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+7],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+8]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d %4d %4d    ",VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+9],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+10],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+11]);
                        VID_PRINT((VID_Msg));
                        sprintf(VID_Msg, "%4d %4d %4d %4d\n",VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+12],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+13],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+14],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[(16*b)+15]);
                        VID_PRINT((VID_Msg));
                    }
                    for(k=0;k<2;k++)
                    {
                        sprintf(VID_Msg, "%4d %4d %4d %4d    ",VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[80+(4*k)],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[80+(4*k)+1],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[80+(4*k)+2],VidDispParams.VHSRCVideoDisplayFilters.LumaVerticalCoef[80+(4*k)+3]);
                        VID_PRINT((VID_Msg));
                    }
                    sprintf(VID_Msg, "\n");
                    VID_PRINT((VID_Msg));

                    sprintf(VID_Msg, "ChromaVerticalCoef \n");
                    VID_PRINT((VID_Msg));
                    for(b=0;b<5;b++)
                    {
                       sprintf(VID_Msg, "%4d %4d %4d ",VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[16*b],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+1],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+2]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d    %4d %4d ",VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+4],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+4],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+5]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d %4d    %4d ",VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+6],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+7],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+8]);
                        VID_PRINT((VID_Msg));
                       sprintf(VID_Msg, "%4d %4d %4d    ",VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+9],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+10],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+11]);
                        VID_PRINT((VID_Msg));
                        sprintf(VID_Msg, "%4d %4d %4d %4d\n",VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+12],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+13],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+14],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[(16*b)+15]);
                        VID_PRINT((VID_Msg));
                    }
                    for(k=0;k<2;k++)
                    {
                        sprintf(VID_Msg, "%4d %4d %4d %4d    ",VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[80+(4*k)],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[80+(4*k)+1],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[80+(4*k)+2],VidDispParams.VHSRCVideoDisplayFilters.ChromaVerticalCoef[80+(4*k)+3]);
                        VID_PRINT((VID_Msg));
                    }
                    sprintf(VID_Msg, "\n");
                    VID_PRINT((VID_Msg));
                }
                RetErr = FALSE;
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "feature not available !\n" );
                VID_PRINT_ERROR((VID_Msg ));
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                VID_PRINT_ERROR((VID_Msg ));
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                VID_PRINT_ERROR((VID_Msg ));
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                VID_PRINT_ERROR((VID_Msg ));
                break;
        }
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );
} /* end of VID_GetVideoDisplayParams() */

#endif /* STVID_DEBUG_GET_DISPLAY_PARAMS */


/*-------------------------------------------------------------------------
 * Function : VID_GetViewPortSpecialMode
 *            Get Special Mode parameters.
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetViewPortSpecialMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
#ifdef ST_7020
# define MAX_NB_ZONE 5
#elif defined ST_5528 || defined ST_7710 || defined ST_7100 || defined ST_7109 || defined ST_7200 || defined ST_ZEUS
# define MAX_NB_ZONE 3
#else
# define MAX_NB_ZONE 1
#endif

    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    U16 cpt;
    char Mode[15];
    U16  Zones[MAX_NB_ZONE];
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STLAYER_OutputMode_t OutputMode;
    STLAYER_OutputWindowSpecialModeParams_t Params;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    if (!(RetErr))
    {
        Params.SpectacleModeParams.SizeOfZone_p = Zones;


        ErrCode = STVID_GetViewPortSpecialMode(CurrentViewportHndl, &OutputMode, &Params );
        sprintf(VID_Msg, "STVID_GetViewPortSpecialMode(handle=0x%x) : ",
                 (U32)CurrentViewportHndl);

            switch (ErrCode)
            {
                case ST_NO_ERROR :
                    strcat(VID_Msg, "ok\n");
                    VID_PRINT((VID_Msg));

                    /* Get working mode.    */
                    sprintf(Mode, "   %s",
                            (OutputMode == STLAYER_NORMAL_MODE)    ? "NORMAL" :
                            (OutputMode == STLAYER_SPECTACLE_MODE) ? "SPECTACLE" : "UNKNOWN");

                    switch (OutputMode)
                    {
                        case STLAYER_NORMAL_MODE :
                            sprintf(VID_Msg, "%s\n", Mode);

                            break;
                        case STLAYER_SPECTACLE_MODE:
                            sprintf(VID_Msg, "%s (%d zones :[",
                                Mode,
                                Params.SpectacleModeParams.NumberOfZone);
                            VID_PRINT((VID_Msg));

                            for (cpt=0; cpt<Params.SpectacleModeParams.NumberOfZone-1; cpt++)
                            {
                                sprintf(VID_Msg, "%d,",Zones[cpt]);
                                VID_PRINT((VID_Msg));
                            }
                            sprintf(VID_Msg, "%d] with Effect %d%%)\n",
                                Zones[Params.SpectacleModeParams.NumberOfZone-1],
                                Params.SpectacleModeParams.EffectControl);
                            break;
                        default :
                                sprintf(VID_Msg, "Unexpected video special mode\n");
                            break;
                    }
                RetErr = FALSE;
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "feature not available !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_GetViewPortSpecialMode() */

/*-------------------------------------------------------------------------
 * Function : VID_HidePicture
 *            Restore the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_HidePicture(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_HidePicture(CurrentViewportHndl );
        sprintf(VID_Msg, "STVID_HidePicture(viewporthndl=0x%x) : ", (U32)CurrentViewportHndl );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING :
                API_ErrorCount++;
                strcat(VID_Msg, "picture already hidden while decoding !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_HidePicture() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_Init
 *            Initialisation
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Init(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    ST_DeviceName_t DeviceName;
    S32 LVar;
#if defined(ST_5528) && defined (ST_7020)
    S32 SVar,CVar;
#endif /* defined(ST_5528) && defined (ST_7020)*/
#if defined ST_7015 || defined(ST_7020)
# ifndef ST_MasterDigInput
    ST_DeviceName_t VINDeviceName;
# endif /* ST_MasterDigInput */
#endif
    char InputType[16];
#if defined (ST_7200)
    void *DeltaBaseAddress = NULL;
    U32 DeltaInterruptNumber = 0;
    char DMUName[16] ;
    S32 DMUNumber = 0;
#endif
    STVID_InitParams_t InitParams;
    BOOL RetErr;

#if defined STVID_INJECTION_BREAK_WORKAROUND || !defined ST_OSLINUX
    ST_ErrorCode_t SubscribeErrorCode;
    STEVT_DeviceSubscribeParams_t DevSubscribeParams;
#endif

    BOOL    PlatformWithOnlyVideoDecodeNoDigitalInput = TRUE;

#if defined (mb376) && defined (BBF_EMI_FRAME_BUFFERS_LMI)
    STAVMEM_AllocBlockParams_t AvmemAllocParams;
    STAVMEM_BlockHandle_t BitBufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
#endif

    ErrCode = ST_ERROR_BAD_PARAMETER;
    memset(&InitParams, 0, sizeof(InitParams)); /* set all params to null */

    /* get DeviceName */
    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected DeviceName" );
    }

#if defined ST_5510
    InitParams.DeviceType = STVID_DEVICE_TYPE_5510_MPEG;
#elif defined ST_5512
    InitParams.DeviceType = STVID_DEVICE_TYPE_5512_MPEG;
#elif defined ST_5508
    InitParams.DeviceType = STVID_DEVICE_TYPE_5508_MPEG;
#elif defined ST_5514
    InitParams.DeviceType = STVID_DEVICE_TYPE_5514_MPEG;
#elif defined ST_5516
    InitParams.DeviceType = STVID_DEVICE_TYPE_5516_MPEG;
#elif defined ST_5517
    InitParams.DeviceType = STVID_DEVICE_TYPE_5517_MPEG;
#elif defined ST_5518
    InitParams.DeviceType = STVID_DEVICE_TYPE_5518_MPEG;
#elif defined ST_5528
    InitParams.DeviceType = STVID_DEVICE_TYPE_5528_MPEG;
    PlatformWithOnlyVideoDecodeNoDigitalInput   = FALSE;
#elif defined ST_5100
    InitParams.DeviceType = STVID_DEVICE_TYPE_5100_MPEG;
#elif defined ST_5525
    InitParams.DeviceType = STVID_DEVICE_TYPE_5525_MPEG;
#elif defined ST_5105
    InitParams.DeviceType = STVID_DEVICE_TYPE_5105_MPEG;
#elif defined ST_5107
    InitParams.DeviceType = STVID_DEVICE_TYPE_5107_MPEG;
#elif defined ST_5188
    InitParams.DeviceType = STVID_DEVICE_TYPE_5188_MPEG;
#elif defined ST_5301
    InitParams.DeviceType = STVID_DEVICE_TYPE_5301_MPEG;
#elif defined ST_5162
    InitParams.DeviceType = STVID_DEVICE_TYPE_5162_MPEG;
#elif defined ST_7020
    InitParams.DeviceType = STVID_DEVICE_TYPE_7020_MPEG;
    PlatformWithOnlyVideoDecodeNoDigitalInput   = FALSE;
#elif defined ST_7710
    InitParams.DeviceType = STVID_DEVICE_TYPE_7710_MPEG;
    PlatformWithOnlyVideoDecodeNoDigitalInput   = FALSE;
#elif defined ST_7100
    InitParams.DeviceType = STVID_DEVICE_TYPE_7100_MPEG;
    PlatformWithOnlyVideoDecodeNoDigitalInput   = FALSE;
#elif defined ST_7109
    InitParams.DeviceType = STVID_DEVICE_TYPE_7109_MPEG;
    PlatformWithOnlyVideoDecodeNoDigitalInput   = FALSE;
#elif defined ST_ZEUS
    InitParams.DeviceType = STVID_DEVICE_TYPE_ZEUS_MPEG;
    PlatformWithOnlyVideoDecodeNoDigitalInput   = FALSE;
#elif defined ST_7200
    InitParams.DeviceType = STVID_DEVICE_TYPE_7200_MPEG;
    PlatformWithOnlyVideoDecodeNoDigitalInput   = FALSE;
#endif


/**************************** SETUP for STi55xx *******************************/
#if !defined (mb295) && !defined (mb290) && !defined (db573) && !(defined(mb376)&&defined(ST_7020))
    InitParams.BaseAddress_p = (void*) VIDEO_BASE_ADDRESS;
    InitParams.DeviceBaseAddress_p = (void*) 0;
    InitParams.SharedMemoryBaseAddress_p = (void*) SDRAM_BASE_ADDRESS;
    InitParams.InstallVideoInterruptHandler = TRUE;
    InitParams.InterruptNumber = VIDEO_INTERRUPT;
    InitParams.InterruptLevel = VIDEO_INTERRUPT_LEVEL;
#endif

/*************** Setup for all platforms having only video decode *************/
    if ((!(RetErr)) && (PlatformWithOnlyVideoDecodeNoDigitalInput))
    {
        RetErr = STTST_GetString(pars_p, "V", InputType, sizeof(InputType) );
        if (RetErr || ( strcmp(InputType,"V")!=0 && strcmp(InputType,"v")!=0 && strcmp(InputType,"MPEG")!=0))
        {
            STTST_TagCurrentLine(pars_p, "expected input type (\"V\" or \"MPEG\"=video)" );
        }
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
#if defined ST_5510 || defined ST_5512 || defined ST_5508 || defined ST_5518 || defined ST_5514 || defined ST_5516 || defined ST_5517
        /* Old platforms with single instance capacity */
        if (RetErr || (LVar != 1))
        {
            STTST_TagCurrentLine(pars_p, "expected video instance, VID1=1");
            RetErr = TRUE;
        }
#else
        /* New platforms with multi-instance capacity : STi5100, etc. */
        if (RetErr || (LVar < 1 || LVar > 4))
        {
            STTST_TagCurrentLine(pars_p, "expected video instance, VID1=1, VID2=2, etc.");
            RetErr = TRUE;
        }
#endif
    }


/**************************** SETUP for STi5528 alone *************************/
#if defined (ST_5528) && !defined (ST_7020) && !defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE)
    /* get input type as VIDEO on these boards support MPEG and digital input */
    if (!(RetErr))
    {
        RetErr = STTST_GetString(pars_p, "V", InputType, sizeof(InputType) );
        if (RetErr || ( strcmp(InputType,"V")!=0 && strcmp(InputType,"v")!=0 && strcmp(InputType,"MPEG")!=0
                     && strcmp(InputType,"D")!=0 && strcmp(InputType,"d")!=0 ) )
        {
            STTST_TagCurrentLine(pars_p, "expected input type (\"V\" or \"MPEG\"=video \"D\"=digital)" );
        }
    }
    if ( strcmp(InputType,"V")==0 || strcmp(InputType,"v")==0 || strcmp(InputType,"MPEG")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_5528_MPEG;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 4))
        {
            STTST_TagCurrentLine(pars_p, "expected 5528 video , VID1=1, VID2=2, Still1=3, Still2=4");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 3 :
                InitParams.BaseAddress_p = (void*) VIDEO_BASE_ADDRESS;
                InitParams.BaseAddress2_p = (void *) ST5528_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p =  (void *) ST5528_VIDEO_CD_FIFO_BASE_ADDRESS;
                if (LVar == 3)
                {
                    InitParams.BaseAddress3_p =  (void *) 0; /* No CD-FIFO for still picture decode */
                }
                InitParams.InterruptNumber = VIDEO1_INTERRUPT;
                break;
            case 2 :
            case 4 :
                InitParams.BaseAddress_p = (void*) ST5528_VID2_BASE_ADDRESS;
                InitParams.BaseAddress2_p = (void *) ST5528_DISP1_BASE_ADDRESS;
                InitParams.BaseAddress3_p =  (void *) ST5528_VIDEO_CD_FIFO2_BASE_ADDRESS;
                if (LVar == 4)
                {
                    InitParams.BaseAddress3_p =  (void *) 0; /* No CD-FIFO for still picture decode */
                }
                InitParams.InterruptNumber = VIDEO2_INTERRUPT;
                break;
            default:
                InitParams.BaseAddress_p = (void*) 0;
                break;
        }
    }
    /* else if (InputType[0]=='d' || InputType[0]=='D') */
    /* { */
        /* */
    /* }  */
    else
    {
        RetErr = TRUE;
    }
#endif /* defined(ST_5528) */

/**************************** SETUP for STi7710 *******************************/
#if defined (ST_7710)
    /* Here, set specific parameters for STi7710 chip.                          */
    /* get input type as VIDEO on these boards support MPEG and digital input   */
    if (!(RetErr))
    {
        RetErr = STTST_GetString(pars_p, "V", InputType, sizeof(InputType) );
        if (RetErr || ( strcmp(InputType,"V")!=0 && strcmp(InputType,"v")!=0 && strcmp(InputType,"MPEG")!=0
                     && strcmp(InputType,"D")!=0 && strcmp(InputType,"d")!=0 ) )
        {
            STTST_TagCurrentLine(pars_p, "expected input type (\"V\" or \"MPEG\"=video \"D\"=digital)" );
        }
    }
    if ( strcmp(InputType,"V")==0 || strcmp(InputType,"v")==0 || strcmp(InputType,"MPEG")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7710_MPEG;

        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7710 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
                InitParams.BaseAddress_p = (void*) VIDEO_BASE_ADDRESS;
                InitParams.BaseAddress2_p = (void *) ST7710_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p =  (void *) 0;
                InitParams.InterruptNumber = VIDEO_INTERRUPT;
                break;
            case 2 :
                InitParams.BaseAddress_p = (void*) VIDEO_BASE_ADDRESS;
                InitParams.BaseAddress2_p = (void *) ST7710_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p =  (void *) 0;
                InitParams.InterruptNumber = VIDEO_INTERRUPT;
                break;
            default:
                InitParams.BaseAddress_p = (void*) 0;
                break;
        }
    }
    else if (InputType[0]=='d' || InputType[0]=='D')
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7710_DIGITAL_INPUT;
        InitParams.BaseAddress_p = (void*) DVP_BASE_ADDRESS;
        InitParams.BaseAddress2_p = (void *) ST7710_DISP0_BASE_ADDRESS;
        InitParams.BaseAddress3_p =  (void *) 0;
        InitParams.InterruptNumber = DVP_INTERRUPT;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
    }
    else
    {
        RetErr = TRUE;
    }
#endif /* defined(ST_7710) */
/**************************** SETUP for STi7100 *******************************/
#if defined (ST_7100)
    /* Here, set specific parameters for STi7100 chip.                          */
    /* get input type as VIDEO on these boards support MPEG and digital input   */
    if (!(RetErr))
    {
        RetErr = STTST_GetString(pars_p, "V", InputType, sizeof(InputType) );
        if (RetErr || ( strcmp(InputType,"V")!=0 && strcmp(InputType,"v")!=0 && strcmp(InputType,"MPEG")!=0
                     && strcmp(InputType,"D")!=0 && strcmp(InputType,"d")!=0
#if defined(NATIVE_CORE)   /* on 710x VSOC simulator VC1 is available */
                     && strcmp(InputType,"VC1")!=0 && strcmp(InputType,"vc1")!=0
#endif
                     && strcmp(InputType,"H264")!=0 && strcmp(InputType,"h264")!=0
                     && strcmp(InputType,"MPEG4P2")!=0 && strcmp(InputType,"mpeg4p2")!=0 ) )
        {
            STTST_TagCurrentLine(pars_p, "expected input type (\"V\" or \"MPEG\"=video Mpeg \"H264\"=H264 \"MPEG4P2\"=MPEG4P2 \"D\"=digital)" );
            RetErr = TRUE;
        }
    }
    if ( strcmp(InputType,"V")==0 || strcmp(InputType,"v")==0 || strcmp(InputType,"MPEG")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7100_MPEG;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7100 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress_p  = (void *) VIDEO_BASE_ADDRESS;
                InitParams.BaseAddress2_p = (void *) ST7100_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) 0;
                InitParams.InterruptNumber = VIDEO_INTERRUPT;
                break;
            default:
                InitParams.BaseAddress_p  = (void *) 0;
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"h264")==0 || strcmp(InputType,"H264")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7100_H264;
        InitParams.BaseAddress_p = (void*) DELTA_BASE_ADDRESS;
        InitParams.InterruptNumber = DELTA_PP0_INTERRUPT_NUMBER;
        InitParams.BaseAddress3_p = (void *) 0;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7100 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress2_p = (void *) ST7100_DISP0_BASE_ADDRESS;
                break;
            default:
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
#ifdef NATIVE_CORE
    else if ( strcmp(InputType,"vc1")==0 || strcmp(InputType,"VC1")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7109_VC1;   /*workaround for VSOC as 7109 VSOC do not exist*/
        InitParams.BaseAddress_p = (void*) DELTA_BASE_ADDRESS;
        InitParams.InterruptNumber = DELTA_PP0_INTERRUPT_NUMBER;

        InitParams.BaseAddress3_p =  (void *) 0;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7100 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
#ifndef DISPLAYPIPE_DEBUG
                InitParams.BaseAddress2_p = (void *) ST7100_DISP0_BASE_ADDRESS;
#else
                InitParams.BaseAddress2_p = (void *)VDPRegs;
#endif  /* DISPLAYPIPE_DEBUG */
                break;
            case 2 :
                InitParams.BaseAddress2_p = (void *) ST7100_DISP0_BASE_ADDRESS;
                break;
            default:
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
#endif  /* NATIVE_CORE */
    else if ( strcmp(InputType,"mpeg4p2")==0 || strcmp(InputType,"MPEG4P2")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7100_MPEG4P2;
        InitParams.BaseAddress_p = (void*) DELTA_BASE_ADDRESS;
        InitParams.InterruptNumber = DELTA_PP0_INTERRUPT_NUMBER;
        InitParams.BaseAddress3_p =  (void *) 0;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7100 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress2_p = (void *) ST7100_DISP0_BASE_ADDRESS;
                break;
            default:
                InitParams.BaseAddress2_p = (void*) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"D")==0 || strcmp(InputType,"d")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7100_DIGITAL_INPUT;
        InitParams.BaseAddress_p = (void*) DVP_BASE_ADDRESS;
        InitParams.BaseAddress2_p = (void *) ST7100_DISP0_BASE_ADDRESS;
        InitParams.BaseAddress3_p =  (void *) 0;
        InitParams.InterruptNumber = DVP_INTERRUPT;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
    }
    else
    {
        RetErr = TRUE;
    }
#endif /* defined(ST_7100) */
/**************************** SETUP for STi7109 *******************************/
#if defined (ST_7109)
    /* Here, set specific parameters for STi7109 chip.                          */
    /* get input type as VIDEO on these boards support MPEG and digital input   */
    if (!(RetErr))
    {
        RetErr = STTST_GetString(pars_p, "V", InputType, sizeof(InputType) );
        if (RetErr || ( strcmp(InputType,"V")!=0 && strcmp(InputType,"v")!=0 && strcmp(InputType,"MPEG")!=0
                     && strcmp(InputType,"D")!=0 && strcmp(InputType,"d")!=0
                     && strcmp(InputType,"DMPEG")!=0 && strcmp(InputType,"dmpeg")!=0
                     && strcmp(InputType,"H264")!=0 && strcmp(InputType,"h264")!=0
                     && strcmp(InputType,"VC1")!=0 && strcmp(InputType,"vc1")!=0
                     && strcmp(InputType,"AVS")!=0 && strcmp(InputType,"avs")!=0
                     && strcmp(InputType,"MPEG4P2")!=0 && strcmp(InputType,"mpeg4p2")!=0 ) )
        {
            STTST_TagCurrentLine(pars_p, "expected input type (\"V\" or \"MPEG\"=video Mpeg or \"DMPEG\"=video Mpeg Delta \"H264\"=H264 \"VC1\"=VC1 \"MPEG4P2\"=MPEG4P2 \"D\"=digital)" );
        }
    }
    if ( strcmp(InputType,"V")==0 || strcmp(InputType,"v")==0 || strcmp(InputType,"MPEG")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7109_MPEG;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7109 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress_p  = (void *) VIDEO_BASE_ADDRESS;
                InitParams.BaseAddress2_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                InitParams.InterruptNumber = VIDEO_INTERRUPT;
                break;
            default:
                InitParams.BaseAddress_p  = (void *) 0;
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"DMPEG")==0 || strcmp(InputType,"dmpeg")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7109D_MPEG;
        InitParams.BaseAddress_p = (void*) DELTA_BASE_ADDRESS;
        InitParams.InterruptNumber = DELTA_PP0_INTERRUPT_NUMBER;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7109 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress2_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                break;
            default:
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"h264")==0 || strcmp(InputType,"H264")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7109_H264;
        InitParams.BaseAddress_p = (void*) DELTA_BASE_ADDRESS;
        InitParams.InterruptNumber = DELTA_PP0_INTERRUPT_NUMBER;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7109 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress2_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                break;
            default:
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"vc1")==0 || strcmp(InputType,"VC1")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7109_VC1;
        InitParams.BaseAddress_p = (void*) DELTA_BASE_ADDRESS;
        InitParams.InterruptNumber = DELTA_PP0_INTERRUPT_NUMBER;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7109 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
#ifndef DISPLAYPIPE_DEBUG
                InitParams.BaseAddress2_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) ST7109_DISP0_BASE_ADDRESS;
#else
                InitParams.BaseAddress2_p = (void *)VDPRegs;
#endif  /* DISPLAYPIPE_DEBUG */
                break;
            case 2 :
                InitParams.BaseAddress2_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                break;
            default:
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"mpeg4p2")==0 || strcmp(InputType,"MPEG4P2")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7109_MPEG4P2;
        InitParams.BaseAddress_p = (void*) DELTA_BASE_ADDRESS;
        InitParams.InterruptNumber = DELTA_PP0_INTERRUPT_NUMBER;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7109 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress2_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                break;
            default:
                InitParams.BaseAddress2_p = (void*) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"avs")==0 || strcmp(InputType,"AVS")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7109_AVS;
        InitParams.BaseAddress_p = (void*) DELTA_BASE_ADDRESS;
        InitParams.InterruptNumber = DELTA_PP0_INTERRUPT_NUMBER;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7109 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress2_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) ST7109_DISP0_BASE_ADDRESS;
                break;
            default:
                InitParams.BaseAddress2_p = (void*) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"D")==0 || strcmp(InputType,"d")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7109_DIGITAL_INPUT;
        InitParams.BaseAddress_p = (void*) DVP_BASE_ADDRESS;
        InitParams.BaseAddress2_p = (void *) ST7109_DISP0_BASE_ADDRESS;
        InitParams.BaseAddress3_p =  (void *) ST7109_DISP0_BASE_ADDRESS;
        InitParams.InterruptNumber = DVP_INTERRUPT;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
    }

    else
    {
        RetErr = TRUE;
    }
#endif /* defined (ST_7109) */


/**************************** SETUP for STi7200 *******************************/
#if defined (ST_7200)
    /* Here, set specific parameters for STi7200           chip.                */
    /* get input type as VIDEO on these boards support MPEG and digital input   */
    if (!(RetErr))
    {
        RetErr = STTST_GetString(pars_p, "V", InputType, sizeof(InputType) );
        if (RetErr || ( strcmp(InputType,"V")!=0 && strcmp(InputType,"v")!=0 && strcmp(InputType,"MPEG")!=0
                     && strcmp(InputType,"D")!=0 && strcmp(InputType,"d")!=0
                     && strcmp(InputType,"H264")!=0 && strcmp(InputType,"h264")!=0
                     && strcmp(InputType,"VC1")!=0 && strcmp(InputType,"vc1")!=0
                     && strcmp(InputType,"MPEG4P2")!=0 && strcmp(InputType,"mpeg4p2")!=0 ) )
        {
            STTST_TagCurrentLine(pars_p, "expected input type (\"V\" or \"MPEG\"=video Mpeg \"H264\"=H264 \"VC1\"=VC1 \"MPEG4P2\"=MPEG4P2 \"D\"=digital)" );
        }
    }

    /* Get Decoder name */
    /* Select the decoder number from parameters */
    RetErr = STTST_GetString(pars_p, "DMU0", DMUName, sizeof(DMUName));
    if (RetErr ||
        ((strncmp(DMUName, "DMU0", 4) != 0) &&
         (strncmp(DMUName, "DMU1", 4) != 0))   )
    {
        STTST_TagCurrentLine(pars_p, "expected 7200 decoder name: \"DMU0\", \"DMU1\"");
        RetErr = TRUE;
    }
    else
    {
        if (strncmp(DMUName, "DMU0", 4) == 0)
        {
            DMUNumber = 0;
            DeltaBaseAddress = (void *) ST7200_DELTAMU_0_BASE_ADDRESS;
            DeltaInterruptNumber = ST7200_VID_DMU0_PRE0_INTERRUPT;
        }
        else if (strncmp(DMUName, "DMU1", 4) == 0)
        {
            DMUNumber = 1;
            DeltaBaseAddress = (void *) ST7200_DELTAMU_1_BASE_ADDRESS;
            DeltaInterruptNumber = ST7200_VID_DMU1_PRE0_INTERRUPT;
        }
    }

    if ( strcmp(InputType,"V")==0 || strcmp(InputType,"v")==0 || strcmp(InputType,"MPEG")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7200_MPEG;  /*update with 7200 */
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7200 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress_p  = (void *) DeltaBaseAddress;
                InitParams.BaseAddress2_p = (void *) DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) 0;
                InitParams.InterruptNumber = DeltaInterruptNumber;
                break;
            default:
                InitParams.BaseAddress_p  = (void *) 0;
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"h264")==0 || strcmp(InputType,"H264")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7200_H264;
        InitParams.BaseAddress_p = (void*) DeltaBaseAddress;
        InitParams.InterruptNumber = DeltaInterruptNumber;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7200 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress2_p = (void *) DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
            default:
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"vc1")==0 || strcmp(InputType,"VC1")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7200_VC1;
        InitParams.BaseAddress_p = (void*) DeltaBaseAddress;
        InitParams.InterruptNumber = DeltaInterruptNumber;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7200 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
#ifndef DISPLAYPIPE_DEBUG
                InitParams.BaseAddress2_p = (void *) DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) 0;
#else
                InitParams.BaseAddress2_p = (void *)VDPRegs;
                InitParams.BaseAddress3_p = (void *) 0;
#endif  /* DISPLAYPIPE_DEBUG */
                break;
            case 2 :
                InitParams.BaseAddress2_p = (void *) DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
            default:
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"mpeg4p2")==0 || strcmp(InputType,"MPEG4P2")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7200_MPEG4P2;
        InitParams.BaseAddress_p = (void*) DeltaBaseAddress;
        InitParams.InterruptNumber = DeltaInterruptNumber;
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 2))
        {
            STTST_TagCurrentLine(pars_p, "expected 7200 video , VID1=1, VID2=2");
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
            case 2 :
                InitParams.BaseAddress2_p = (void *) DISP0_BASE_ADDRESS;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
            default:
                InitParams.BaseAddress2_p = (void *) 0;
                InitParams.BaseAddress3_p = (void *) 0;
                break;
        }
    }
    else if ( strcmp(InputType,"D")==0 || strcmp(InputType,"d")==0 )
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7200_DIGITAL_INPUT;
        InitParams.BaseAddress_p = (void*) DVP_BASE_ADDRESS;
        InitParams.BaseAddress2_p = (void*)DISP0_BASE_ADDRESS;
        InitParams.BaseAddress3_p =  (void*)0;
        InitParams.InterruptNumber = DVP_INTERRUPT;
        /* RetErr = STTST_GetInteger(pars_p, 1, &LVar); */
    }
    else
    {
        RetErr = TRUE;
    }


#endif /* defined (ST_7200) */


/**************** SETUP for STi7020 (but not on mb376 with STi5528) ***********/
#if (defined (mb295) || defined (mb290) || defined (db573)) && !defined (mb376)
    /* get input type as VIDEO on these boards support MPEG and digital input */
    if (!(RetErr))
    {
        RetErr = STTST_GetString(pars_p, "V", InputType, sizeof(InputType) );
        if (RetErr || ( strcmp(InputType,"V")!=0 && strcmp(InputType,"v")!=0 && strcmp(InputType,"MPEG")!=0
                     && strcmp(InputType,"D")!=0 && strcmp(InputType,"d")!=0 ) )
        {
            STTST_TagCurrentLine(pars_p, "expected input type (\"V\" or \"MPEG\"=video \"D\"=digital)" );
        }
    }
    if ( strcmp(InputType,"V")==0 || strcmp(InputType,"v")==0 || strcmp(InputType,"MPEG")==0 )
    {
#if defined (ST_7015)
        InitParams.DeviceType = STVID_DEVICE_TYPE_7015_MPEG;
#else
        InitParams.DeviceType = STVID_DEVICE_TYPE_7020_MPEG;
#endif
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 5))
        {
#if defined (ST_7015)
            STTST_TagCurrentLine(pars_p, "expected 7015 video , VID1=1, VID2=2, VID3=3, VID4=4, VID5=5");
#else
            STTST_TagCurrentLine(pars_p, "expected 7020 video , VID1=1, VID2=2, VID3=3, VID4=4, VID5=5");
#endif
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_VID1_OFFSET;
                InitParams.InterruptEvent = ST7015_VID1_INT;
#else
                InitParams.BaseAddress_p = (void*) ST7020_VID1_OFFSET;
                InitParams.InterruptEvent = ST7020_VID1_INT;
#endif
                break;
            case 2 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_VID2_OFFSET;
                InitParams.InterruptEvent = ST7015_VID2_INT;
#else
                InitParams.BaseAddress_p = (void*) ST7020_VID2_OFFSET;
                InitParams.InterruptEvent = ST7020_VID2_INT;
#endif
                break;
            case 3 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_VID3_OFFSET;
                InitParams.InterruptEvent = ST7015_VID3_INT;
#else
                InitParams.BaseAddress_p = (void*) ST7020_VID3_OFFSET;
                InitParams.InterruptEvent = ST7020_VID3_INT;
#endif
                break;
            case 4 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_VID4_OFFSET;
                InitParams.InterruptEvent = ST7015_VID4_INT;
#else
                InitParams.BaseAddress_p = (void*) ST7020_VID4_OFFSET;
                InitParams.InterruptEvent = ST7020_VID4_INT;
#endif
                break;
            case 5 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_VID5_OFFSET;
                InitParams.InterruptEvent = ST7015_VID5_INT;
#else
                InitParams.BaseAddress_p = (void*) ST7020_VID5_OFFSET;
                InitParams.InterruptEvent = ST7020_VID5_INT;
#endif
                break;
            default:
                InitParams.BaseAddress_p = (void*) 0;
                break;
        }
#if defined (ST_7015)
        InitParams.DeviceBaseAddress_p = (void*) STI7015_BASE_ADDRESS;
#else
        InitParams.DeviceBaseAddress_p = (void*) STI7020_BASE_ADDRESS;
#endif
        InitParams.SharedMemoryBaseAddress_p = (void*) (SDRAM_WINDOW_BASE_ADDRESS - SDRAM_WINDOW_BASE_ADDRESS);
        InitParams.InstallVideoInterruptHandler = FALSE;
        strcpy(InitParams.InterruptEventName, STINTMR_DEVICE_NAME);
    }
    else if ( strcmp(InputType,"D")==0 || strcmp(InputType,"d")==0 )
    {
#if defined (ST_7015)
        InitParams.DeviceType = STVID_DEVICE_TYPE_7015_DIGITAL_INPUT;
#else
        InitParams.DeviceType = STVID_DEVICE_TYPE_7020_DIGITAL_INPUT;
#endif
        RetErr = STTST_GetInteger(pars_p, 1, &LVar);
        if (RetErr || (LVar < 1 || LVar > 3))
        {
#if defined (ST_7015)
            STTST_TagCurrentLine(pars_p, "expected 7015 input , SD1=1, SD2=2, HD=3");
#else
            STTST_TagCurrentLine(pars_p, "expected 7020 input , SD1=1, SD2=2, HD=3");
#endif
            RetErr = TRUE;
        }
        switch (LVar)
        {
            case 1 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_SDIN1_OFFSET; /* for digital SD1 input */
                InitParams.InterruptEvent = ST7015_SDIN1_INT; /* for digital input */
#else
                InitParams.BaseAddress_p = (void*) ST7020_SDIN1_OFFSET; /* for digital SD1 input */
                InitParams.InterruptEvent = ST7020_SDIN1_INT; /* for digital input */
#endif
                break;
            case 2 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_SDIN2_OFFSET; /* for digital SD2 input */
                InitParams.InterruptEvent = ST7015_SDIN2_INT; /* for digital input */
#else
                InitParams.BaseAddress_p = (void*) ST7020_SDIN2_OFFSET; /* for digital SD2 input */
                InitParams.InterruptEvent = ST7020_SDIN2_INT; /* for digital input */
#endif
                break;
            case 3 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_HDIN_OFFSET; /* for digital HD input */
                InitParams.InterruptEvent = ST7015_HDIN_INT; /* for digital input */
#else
                InitParams.BaseAddress_p = (void*) ST7020_HDIN_OFFSET; /* for digital HD input */
                InitParams.InterruptEvent = ST7020_HDIN_INT; /* for digital input */
#endif
                break;
            default:
                InitParams.BaseAddress_p = (void*) 0;
                break;
        }
#if defined (ST_7015)
        InitParams.DeviceBaseAddress_p = (void*) STI7015_BASE_ADDRESS;
#else
        InitParams.DeviceBaseAddress_p = (void*) STI7020_BASE_ADDRESS;
#endif
        InitParams.SharedMemoryBaseAddress_p = (void*) (SDRAM_WINDOW_BASE_ADDRESS - SDRAM_WINDOW_BASE_ADDRESS);
        InitParams.InstallVideoInterruptHandler = FALSE;
        strcpy(InitParams.InterruptEventName, STINTMR_DEVICE_NAME);
#ifndef ST_MasterDigInput
        if (!RetErr)
        {
            /* get VIN DeviceName */
            RetErr = STTST_GetString(pars_p, "", VINDeviceName, sizeof(VINDeviceName) );
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "expected VIN DeviceName" );
            }
            strcpy(InitParams.VINName, VINDeviceName);
        }
#endif /* ST_MasterDigInput */
    }
    else
    {
        RetErr = TRUE;
    }
#endif /* defined ST_7015 || defined(ST_7020) */

/******************* SETUP for STi7020 on mb376 + STi5528 *********************/
#if defined(ST_5528) && defined(ST_7020)
    /* get input type as VIDEO on these boards support MPEG and digital input */
    if (!(RetErr))
    {
        RetErr = STTST_GetString(pars_p, "V", InputType, sizeof(InputType) );
        if (RetErr || ( strcmp(InputType,"V")!=0 && strcmp(InputType,"v")!=0 && strcmp(InputType,"MPEG")!=0
                     && strcmp(InputType,"D")!=0 && strcmp(InputType,"d")!=0 ) )
        {
            STTST_TagCurrentLine(pars_p, "expected input type (\"V\" or \"MPEG\"=video \"D\"=digital)" );
        }
    }
    InitParams.DeviceBaseAddress_p = (void*) 0;
    InitParams.InstallVideoInterruptHandler = TRUE;
    InitParams.InterruptLevel = VIDEO_INTERRUPT_LEVEL;

#ifndef ST_MasterDigInput
            if (!RetErr)
            {
                /* get VIN DeviceName */
                RetErr = STTST_GetString(pars_p, "", VINDeviceName, sizeof(VINDeviceName) );
                if (RetErr)
                {
                    STTST_TagCurrentLine(pars_p, "expected VIN DeviceName" );
                }
                strcpy(InitParams.VINName, VINDeviceName);
            }
#endif /* ST_MasterDigInput */

    RetErr = STTST_GetInteger(pars_p, 1, &SVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected integer value");
        RetErr = TRUE;
    }
#endif /* defined(ST_5528) && defined (ST_7020)*/


    /* get Evt Name */
    if (!RetErr)
    {
        RetErr = STTST_GetString(pars_p, STEVT_DEVICE_NAME, InitParams.EvtHandlerName, \
                                  sizeof(InitParams.EvtHandlerName) );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected EVT name");
        }
    }
    /* get Clkrv Name */
    if (!RetErr)
    {
#ifdef USE_CLKRV
        RetErr = STTST_GetString(pars_p, STCLKRV_DEVICE_NAME, InitParams.ClockRecoveryName, \
                                  sizeof(InitParams.ClockRecoveryName) );
#else /* not USE_CLKRV */
        RetErr = STTST_GetString(pars_p, "", InitParams.ClockRecoveryName, \
                                  sizeof(InitParams.ClockRecoveryName) );
#endif /* not USE_CLKRV */
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected CLKRV name");
        }
    }

    /* get MaxOpen */
    if (!RetErr)
    {
        InitParams.MaxOpen = 1;
#if defined (mb295) || defined (mb290) || defined (db573) || defined (ST_5528) || defined (ST_51xx_Device) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
        InitParams.MaxOpen = 2;
#endif
        RetErr = STTST_GetInteger(pars_p, (S32) InitParams.MaxOpen, &LVar );
        InitParams.MaxOpen = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected MaxOpen" );
        }
    }

    /* Get BitBufferAllocated flag */
    if (!RetErr)
    {
        RetErr = STTST_GetInteger(pars_p, FALSE, &LVar );
        if (LVar != 0)
        {
            InitParams.BitBufferAllocated = TRUE;
        }
        else
        {
        InitParams.BitBufferAllocated = FALSE;
        }
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected BitBufferAllocated (TRUE or FALSE)" );
        }
    }

    /* Get BitBufferAddress_p field */
    if (!RetErr)
    {
#if !defined ST_OSLINUX
        if (GlobalExternalBitBuffer.IsAllocated)
        {
            /* Retrieve parameters from globally allocated bit buffer. */
            RetErr = STTST_GetInteger(pars_p, (S32) GlobalExternalBitBuffer.Address_p, &LVar );
        }
        else
#endif
        {
            RetErr = STTST_GetInteger(pars_p, 0x11, &LVar );
        }
        InitParams.BitBufferAddress_p = (void *) LVar;
        if ((RetErr) || ((InitParams.BitBufferAllocated) && (LVar == 0x11))) /* No parameter provided */
        {
            /* Require bit buffer address when BitBufferAllocated parameter is TRUE, otherwise we don't know where the bit buffer is ! */
            STTST_TagCurrentLine(pars_p, "expected bit buffer address !" );
        }
    }

    /* Get BitBufferSize field */
    if (!RetErr)
    {
#if !defined ST_OSLINUX
        if (GlobalExternalBitBuffer.IsAllocated)
        {
            /* Retrieve parameters from globally allocated bit buffer. */
            RetErr = STTST_GetInteger(pars_p, (S32) GlobalExternalBitBuffer.Size, &LVar );
        }
        else
#endif
        {
            RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        }
        InitParams.BitBufferSize = (U32) LVar;
        if ((RetErr) || ((InitParams.BitBufferAllocated)&&(LVar == 0))) /* No parameter provided, or size 0 */
        {
            /* Require bit buffer size when BitBufferAllocated parameter is TRUE, otherwise we don't know what is the size of the bit buffer ! */
            STTST_TagCurrentLine(pars_p, "expected bit buffer size ! (cannot be 0)" );
        }
    }

    if (!RetErr)
    {
        /* set other init. parameters */
#if !defined ST_OSLINUX
#if defined (mb376) && defined (BBF_EMI_FRAME_BUFFERS_LMI)
        AvmemAllocParams.PartitionHandle = AvmemPartitionHandle[1];
        AvmemAllocParams.Size = 380928; /* SD bit buffer size */
        AvmemAllocParams.Alignment = 256;
        AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AvmemAllocParams.NumberOfForbiddenRanges = 0;
        AvmemAllocParams.ForbiddenRangeArray_p = NULL;
        AvmemAllocParams.NumberOfForbiddenBorders = 0;
        AvmemAllocParams.ForbiddenBorderArray_p = NULL;
        ErrCode = STAVMEM_AllocBlock(&AvmemAllocParams, &BitBufferHandle);
        if (ErrCode == ST_NO_ERROR)
        {
            ErrCode = STAVMEM_GetBlockAddress(BitBufferHandle, &InitParams.BitBufferAddress_p);
        }
        InitParams.BitBufferAllocated = TRUE;
        InitParams.BitBufferSize = 380928;
#endif
#endif
        InitParams.UserDataSize = VID_MAX_USER_DATA;
#if defined(ST_5528) && defined(ST_7020)
        RetErr = STTST_GetInteger(pars_p, STVID_DEVICE_TYPE_7020_MPEG, &CVar);
        if (RetErr || ((CVar != 9) && (CVar != 12)))
        {
            STTST_TagCurrentLine(pars_p, "Expected device type (9=7020,12=5528)");
            RetErr = TRUE;
        }
        InitParams.DeviceType = (STVID_DeviceType_t)CVar;
        if (InitParams.DeviceType == STVID_DEVICE_TYPE_7020_MPEG)
        {
            InitParams.BaseAddress_p = (void*) (STI7020_BASE_ADDRESS+ST7020_VID_OFFSET);
            InitParams.SharedMemoryBaseAddress_p = (void*) SDRAM_BASE_ADDRESS;
            if ( strcmp(InputType,"V")==0 || strcmp(InputType,"v")==0 || strcmp(InputType,"MPEG")==0 )
            {
                if (SVar < 1 || SVar > 5)
                {
                    STTST_TagCurrentLine(pars_p, "expected 7020 video , VID1=1, VID2=2, VID3=3, VID4=4, VID5=5");
                    RetErr = TRUE;
                }
                switch (SVar)
                {
                    case 1 :
                        InitParams.BaseAddress_p = (void*) ST7020_VID1_OFFSET;
                        InitParams.InterruptEvent = ST7020_VID1_INT;
                        break;
                    case 2 :
                        InitParams.BaseAddress_p = (void*) ST7020_VID2_OFFSET;
                        InitParams.InterruptEvent = ST7020_VID2_INT;
                        break;
                    case 3 :
                        InitParams.BaseAddress_p = (void*) ST7020_VID3_OFFSET;
                        InitParams.InterruptEvent = ST7020_VID3_INT;
                        break;
                    case 4 :
                        InitParams.BaseAddress_p = (void*) ST7020_VID4_OFFSET;
                        InitParams.InterruptEvent = ST7020_VID4_INT;
                        break;
                    case 5 :
                        InitParams.BaseAddress_p = (void*) ST7020_VID5_OFFSET;
                        InitParams.InterruptEvent = ST7020_VID5_INT;
                        break;
                    default:
                        InitParams.BaseAddress_p = (void*) 0;
                        break;
                }
                InitParams.DeviceBaseAddress_p = (void*) STI7020_BASE_ADDRESS;
                InitParams.SharedMemoryBaseAddress_p = (void*) (SDRAM_WINDOW_BASE_ADDRESS - SDRAM_WINDOW_BASE_ADDRESS);
                InitParams.InstallVideoInterruptHandler = FALSE;
                strcpy(InitParams.InterruptEventName, STINTMR_DEVICE_NAME);
            }
            else if ( strcmp(InputType,"D")==0 || strcmp(InputType,"d")==0 )
            {
                InitParams.DeviceType = STVID_DEVICE_TYPE_7020_DIGITAL_INPUT;
                if (SVar < 1 || SVar > 3)
                {
                    STTST_TagCurrentLine(pars_p, "expected 7020 input , SD1=1, SD2=2, HD=3");
                    RetErr = TRUE;
                }
                switch (SVar)
                {
                    case 1 :
                        InitParams.BaseAddress_p = (void*) ST7020_SDIN1_OFFSET; /* for digital SD1 input */
                        InitParams.InterruptEvent = ST7020_SDIN1_INT; /* for digital input */
                        break;
                    case 2 :
                        InitParams.BaseAddress_p = (void*) ST7020_SDIN2_OFFSET; /* for digital SD2 input */
                        InitParams.InterruptEvent = ST7020_SDIN2_INT; /* for digital input */
                        break;
                    case 3 :
                        InitParams.BaseAddress_p = (void*) ST7020_HDIN_OFFSET; /* for digital HD input */
                        InitParams.InterruptEvent = ST7020_HDIN_INT; /* for digital input */
                        break;
                    default:
                        InitParams.BaseAddress_p = (void*) 0;
                        break;
                }
                InitParams.DeviceBaseAddress_p = (void*) STI7020_BASE_ADDRESS;
                InitParams.SharedMemoryBaseAddress_p = (void*) (SDRAM_WINDOW_BASE_ADDRESS - SDRAM_WINDOW_BASE_ADDRESS);
                InitParams.InstallVideoInterruptHandler = FALSE;
                strcpy(InitParams.InterruptEventName, STINTMR_DEVICE_NAME);
            }
            else
            {
                RetErr = TRUE;
            }
#if !defined ST_OSLINUX
            InitParams.AVMEMPartition = AvmemPartitionHandle[0];
#endif
        } /* if STVID_DEVICE_TYPE_7020_MPEG */
        else
        {
            InitParams.BaseAddress_p = (void*) ST5528_VIDEO_BASE_ADDRESS;
            InitParams.SharedMemoryBaseAddress_p = (void*) MB376_SDRAM_BASE_ADDRESS;
            if ( strcmp(InputType,"V")==0 || strcmp(InputType,"v")==0 || strcmp(InputType,"MPEG")==0 )
            {
                InitParams.DeviceType = STVID_DEVICE_TYPE_5528_MPEG;
                 if (SVar < 1 || SVar > 2)
                {
                    STTST_TagCurrentLine(pars_p, "expected 5528 video , VID1=1, VID2=2");
                    RetErr = TRUE;
                }
                switch (SVar)
                {
                    case 1 :
                        InitParams.BaseAddress_p = (void*) ST5528_VIDEO_BASE_ADDRESS;
                        InitParams.BaseAddress2_p = (void *) ST5528_DISP0_BASE_ADDRESS;
                        InitParams.BaseAddress3_p =  (void *) ST5528_VIDEO_CD_FIFO_BASE_ADDRESS;
                        InitParams.InterruptNumber = ST5528_VID1_INTERRUPT;
                        break;
                    case 2 :
                        InitParams.BaseAddress_p = (void*) ST5528_VID2_BASE_ADDRESS;
                        InitParams.BaseAddress2_p = (void *) ST5528_DISP1_BASE_ADDRESS;
                        InitParams.BaseAddress3_p =  (void *) ST5528_VIDEO_CD_FIFO2_BASE_ADDRESS;
                        InitParams.InterruptNumber = ST5528_VID2_INTERRUPT;
                        break;
                    default:
                        InitParams.BaseAddress_p = (void*) 0;
                        break;
                }
            }
            else
            {
                RetErr = TRUE;
            }
#if !defined ST_OSLINUX
            InitParams.AVMEMPartition = AvmemPartitionHandle[1];
#endif
    }  /* if not STVID_DEVICE_TYPE_7020_MPEG */

#else /* Used by all chips except 5528+7020 */

#ifdef ST_OSLINUX
        /* use LMI_SYS by default */
        InitParams.AVMEMPartition = (STAVMEM_PartitionHandle_t) 0; /* Partition 0: Only the index is passed, conversion done in kstvid */
#else
        InitParams.AVMEMPartition = AvmemPartitionHandle[0];
#endif /* LINUX & KERNEL */

#endif /* defined(ST_5528) && defined(ST_7020) */

      /* all input param. have been checked */
      if (!(RetErr))
      {
        InitParams.CPUPartition_p = DriverPartition_p;
        InitParams.AVSYNCDriftThreshold = 2 * (90000 / 50); /* 2 * VSync max time */

        ErrCode = STVID_Init(DeviceName, &InitParams);
#if defined(ST_7200)
        sprintf(VID_Msg, "STVID_Init(device=%s,maxopen=%d,DMU=DMU%d) Driver revision %s: ",
                 DeviceName, InitParams.MaxOpen, DMUNumber, STVID_GetRevision());
#else  /* defined(ST_7200) */
        sprintf(VID_Msg, "STVID_Init(device=%s,maxopen=%d) Driver revision %s: ",
                 DeviceName, InitParams.MaxOpen, STVID_GetRevision());
#endif /* not defined(ST_7200) */
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                for(LVar=0;LVar<VIDEO_MAX_DEVICE;LVar++)
                {
                    if (VID_Inj_IsDeviceNameUndefined(LVar)==TRUE)
                    {
                        VID_Inj_SetDeviceName(LVar, DeviceName);
#if defined(STVID_USE_CRC) || defined(STVID_VALID)
                        VID_CRC[LVar].DeviceType = InitParams.DeviceType;
                        VID_CRC[LVar].BaseAddress_p = InitParams.BaseAddress_p;
                        VID_CRC[LVar].CPUPartition_p = InitParams.CPUPartition_p;
#endif /* defined(STVID_USE_CRC) || defined(STVID_VALID) */
                        break;
                    }
                }
                /* */
#ifdef STVID_INJECTION_BREAK_WORKAROUND
                /* Subscribe to event stop & restore injection */
                /* Give information about video decoder used */
                SubscribeErrorCode = ST_NO_ERROR;

                DevSubscribeParams.SubscriberData_p = (U32*)InitParams.BaseAddress_p;
                DevSubscribeParams.NotifyCallback =
                    (STEVT_DeviceCallbackProc_t) PTI_VideoWorkaroundPleaseStopInjection;
                SubscribeErrorCode |= STEVT_SubscribeDeviceEvent(EvtSubHandle, DeviceName, (STEVT_EventConstant_t)
                                                                  STVID_WORKAROUND_PLEASE_STOP_INJECTION_EVT,
                                                                  &DevSubscribeParams);
                DevSubscribeParams.NotifyCallback =
                    (STEVT_DeviceCallbackProc_t) PTI_VideoWorkaroundThanksInjectionCanGoOn;
                SubscribeErrorCode |= STEVT_SubscribeDeviceEvent(EvtSubHandle, DeviceName, (STEVT_EventConstant_t)
                                                                  STVID_WORKAROUND_THANKS_INJECTION_CAN_GO_ON_EVT,
                                                                  &DevSubscribeParams);
                if (SubscribeErrorCode != ST_NO_ERROR)
                {
                    STTBX_ErrorPrint(("Failed to subscribe workarounds injection !!\n" ));
                    API_ErrorCount++;
                    RetErr = TRUE;
                }
#endif /* STVID_INJECTION_BREAK_WORKAROUND */

#ifndef ST_OSLINUX /* 5508 & 5518 chip not supported for LINUX */
                /* Subscribe to event PROVIDED_DATA_TO_BE_INJECTED */
                /* Actually, this event is only generated for 5518 chips */
                /* This event requests that application inject buffer given */
                /* by video driver into the CD fifo */
                /* Give information about video decoder used */
                SubscribeErrorCode = ST_NO_ERROR;


                DevSubscribeParams.SubscriberData_p = (U32*)InitParams.BaseAddress_p;
                DevSubscribeParams.NotifyCallback =
                    (STEVT_DeviceCallbackProc_t) VID_ProvidedDataToBeInjected;
                SubscribeErrorCode |= STEVT_SubscribeDeviceEvent(EvtSubHandle,
                                                                 DeviceName,
                                                                 (STEVT_EventConstant_t) STVID_PROVIDED_DATA_TO_BE_INJECTED_EVT,
                                                                 &DevSubscribeParams);

                if (SubscribeErrorCode != ST_NO_ERROR)
                {
                    STTBX_ErrorPrint(("Failed to subscribe PROVIDED_DATA_TO_BE_INJECTED  !!\n" ));
                    API_ErrorCount++;
                    RetErr = TRUE;
                }
#endif
                break;

            case STVID_ERROR_SYSTEM_CLOCK :
                API_ErrorCount++;
                strcat(VID_Msg, "connection error to the system clock !\n" );
                break;
            case STVID_ERROR_EVENT_REGISTRATION :
                API_ErrorCount++;
                strcat(VID_Msg, "events registration error !\n" );
                break;
            case ST_ERROR_NO_MEMORY :
                API_ErrorCount++;
                strcat(VID_Msg, "not enough space !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_ALREADY_INITIALIZED :
                API_ErrorCount++;
                strcat(VID_Msg, "the decoder is already initialized !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch ErrCode */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }

        if ( (strcmp(InputType,"H264")==0) || (strcmp(InputType,"h264")==0) )
        {
            sprintf(VID_Msg, "\ttype=%d (H264)", InitParams.DeviceType);
        }
        else if ( (strcmp(InputType,"VC1")==0) || (strcmp(InputType,"vc1")==0) )
        {
            sprintf(VID_Msg, "\ttype=%d (vc1)", InitParams.DeviceType);
        }
        else if ( (strcmp(InputType,"MPEG4P2")==0) || (strcmp(InputType,"mpeg4p2")==0) )
        {
            sprintf(VID_Msg, "\ttype=%d (MPEG4P2)", InitParams.DeviceType);
        }
        else if ( (strcmp(InputType,"AVS")==0) || (strcmp(InputType,"avs")==0) )
        {
            sprintf(VID_Msg, "\ttype=%d (AVS)", InitParams.DeviceType);
        }
        else if ( strcmp(InputType,"D")==0 ||strcmp(InputType,"d")==0 )
        {
            sprintf(VID_Msg, "\ttype=%d (Digital)", InitParams.DeviceType);
        }
        else
        {
            sprintf(VID_Msg, "\ttype=%d (MPEG)", InitParams.DeviceType);
        }
        sprintf(VID_Msg, "%s baseaddr=0x%x devicebaseaddr=0x%x \n\tbballoc=0x%x bitbuff=0x%x size=%x \n",
                 VID_Msg,
                 (U32)InitParams.BaseAddress_p,
                 (U32)InitParams.DeviceBaseAddress_p,
                 (U32)InitParams.BitBufferAllocated,
                 (U32)InitParams.BitBufferAddress_p,
                 (U32)InitParams.BitBufferSize );
        sprintf(VID_Msg, "%s\tintevt=%d evt=%s instint=%d intnb=%d intlev=%d \n",
                 VID_Msg,
                 (U32)InitParams.InterruptEvent,
                 InitParams.InterruptEventName,
                 (U32)InitParams.InstallVideoInterruptHandler,
                 (U32)InitParams.InterruptNumber,
                 (U32)InitParams.InterruptLevel );
        VID_PRINT((VID_Msg ));
        sprintf(VID_Msg, "\tpartition=0x%x memaddr=0x%x AVMEMpart=0x%x maxopen=%d \n",
                 (U32)InitParams.CPUPartition_p,
                 (U32)InitParams.SharedMemoryBaseAddress_p,
                 InitParams.AVMEMPartition,
                 (U32)InitParams.MaxOpen );

#ifndef ST_MasterDigInput
        sprintf(VID_Msg, "%s\tuserdatasize=%d vin=%s evthdl=%s clkrv=%s avsyncthreshold=%d \n",
                 VID_Msg,
                 (U32)InitParams.UserDataSize,
                 InitParams.VINName,
                 InitParams.EvtHandlerName,
                 InitParams.ClockRecoveryName,
                 (U32)InitParams.AVSYNCDriftThreshold );
#else
        sprintf(VID_Msg, "%s\tuserdatasize=%d evthdl=%s clkrv=%s avsyncthreshold=%d \n",
                 VID_Msg,
                 (U32)InitParams.UserDataSize,
                 InitParams.EvtHandlerName,
                 InitParams.ClockRecoveryName,
                 (U32)InitParams.AVSYNCDriftThreshold );
#endif /* ST_MasterDigInput */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
     } /* !RetErr (all input param. have been checked) */

    } /* end set other init. parameters */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_Init() */

/*-------------------------------------------------------------------------
 * Function : VID_InjectDiscontinuity
 *            Informs an injection discontinuity to the driver
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_InjectDiscontinuity(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_Handle_t CurrentHandle;
    BOOL RetErr;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_InjectDiscontinuity(CurrentHandle );

        sprintf(VID_Msg, "STVID_InjectDiscontinuity(handle=0x%x) : ", CurrentHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of STVID_InjectDiscontinuity() */

/*-------------------------------------------------------------------------
 * Function : VID_Open
 *            Share physical decoder usage
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Open(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_OpenParams_t OpenParams;
    ST_DeviceName_t DeviceName;
    STVID_Handle_t CurrentHandle;
    BOOL RetErr;
    S32 LVar;

    ErrCode = ST_ERROR_UNKNOWN_DEVICE;
    memset(&OpenParams, 0, sizeof(OpenParams));

    /* get DeviceName */
    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected DeviceName" );
    }
    /* get SyncDelay */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        OpenParams.SyncDelay = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected SyncDelay" );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_Open(DeviceName, &OpenParams, &CurrentHandle );
        sprintf(VID_Msg, "STVID_Open(device=%s,syncdelay=%d,handle=0x%x) : ",
                 DeviceName, OpenParams.SyncDelay, CurrentHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                for(LVar=0;LVar<VIDEO_MAX_DEVICE;LVar++)
                {
                    if (VID_Inj_IsDeviceNameIdentical(LVar,DeviceName)==TRUE)
                    {
                        VID_Inj_SetDriverHandle(LVar,CurrentHandle);
                        break;
                    }
                }
                break;
            case ST_ERROR_UNKNOWN_DEVICE :
                API_ErrorCount++;
                strcat(VID_Msg, "the specified device is not initialized !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_NO_FREE_HANDLES :
                API_ErrorCount++;
                strcat(VID_Msg, "no free handle (too many open connections) !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    STTST_AssignInteger("RET_VAL1", CurrentHandle, FALSE);
    STTST_AssignInteger(result_sym_p, (int)CurrentHandle, FALSE);


    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_Open() */

#ifndef  STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_OpenViewPort
 *            Open a viewport for a decoder on a layer
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_OpenViewPort(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_ViewPortParams_t ViewPortParams;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STVID_Handle_t CurrentHandle;
    BOOL RetErr;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;
    memset(&ViewPortParams, 0, sizeof(ViewPortParams));

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get LayerName */
    if (!(RetErr))
    {
        RetErr = STTST_GetString(pars_p, STLAYER_VID_DEVICE_NAME, ViewPortParams.LayerName, \
                        sizeof(ViewPortParams.LayerName) );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected LayerName");
        }
    }

    if (!(RetErr))
    {
        ErrCode = STVID_OpenViewPort(CurrentHandle, &ViewPortParams, &CurrentViewportHndl );
        sprintf(VID_Msg, "STVID_OpenViewPort(handle=0x%x,layer=%s,viewporthndl=0x%x) : ",
                 CurrentHandle, ViewPortParams.LayerName, (U32)CurrentViewportHndl );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                /*if (API_TestMode == TEST_MODE_MONO_HANDLE)
                {*/
                    ViewPortHndl0 = CurrentViewportHndl; /* default viewport */
                /*}*/
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                /* Pass a NULL handle in case of bad initialization so that the driver   */
                /* recognizes that the handle value is incorrect (for subsequent call to */
                /* VID_EnableOutputWindow() for example) */
                ViewPortHndl0 = NULL;
                CurrentViewportHndl = NULL;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                /* Pass a NULL handle in case of bad initialization so that the driver   */
                /* recognizes that the handle value is incorrect (for subsequent call to */
                /* VID_EnableOutputWindow() for example) */
                ViewPortHndl0 = NULL;
                CurrentViewportHndl = NULL;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                /* Pass a NULL handle in case of bad initialization so that the driver   */
                /* recognizes that the handle value is incorrect (for subsequent call to */
                /* VID_EnableOutputWindow() for example) */
                ViewPortHndl0 = NULL;
                CurrentViewportHndl = NULL;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger("ERRORCODE", (S32)ErrCode, FALSE);
    STTST_AssignInteger("RET_VAL1", (S32) CurrentViewportHndl, FALSE);
    STTST_AssignInteger(result_sym_p, (int)CurrentViewportHndl, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_OpenViewPort() */
#endif /*#ifndef  STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_Pause
 *            Stop the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Pause(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_Freeze_t Freeze;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get freeze mode */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_FREEZE_MODE_FORCE, &LVar );
        Freeze.Mode = (STVID_FreezeMode_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected Mode (%d=none %d=force %d=no_flicker)" ,
                        STVID_FREEZE_MODE_NONE, STVID_FREEZE_MODE_FORCE, STVID_FREEZE_MODE_NO_FLICKER );
            STTST_TagCurrentLine(pars_p, VID_Msg);
        }
    }
    /* get freeze field */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_FREEZE_FIELD_TOP, &LVar );
        Freeze.Field = (STVID_FreezeField_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected Field (%d=top %d=bot %d=current %d=next)" ,
                    STVID_FREEZE_FIELD_TOP, STVID_FREEZE_FIELD_BOTTOM,
                    STVID_FREEZE_FIELD_CURRENT, STVID_FREEZE_FIELD_NEXT );
            STTST_TagCurrentLine(pars_p, VID_Msg);
        }
    }

    if (!(RetErr))
    {
        ErrCode = STVID_Pause(CurrentHandle, &Freeze );
        sprintf(VID_Msg, "STVID_Pause(handle=0x%x,&freeze=0x%x) : ",
                 CurrentHandle, (int)&Freeze );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_PAUSING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is pausing !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING_IN_RT_MODE:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is running in real time !\n" );
                break;
            case STVID_ERROR_DECODER_STOPPED:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is stopped !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        /* Explicit trace */
        sprintf(VID_Msg, "%s   freeze.mode=%d ", VID_Msg, Freeze.Mode);
        switch(Freeze.Mode)
        {
            case STVID_FREEZE_MODE_NONE :
                strcat(VID_Msg, "(STVID_FREEZE_MODE_NONE)\n");
                break;
            case STVID_FREEZE_MODE_FORCE :
                strcat(VID_Msg, "(STVID_FREEZE_MODE_FORCE)\n");
                break;
            case STVID_FREEZE_MODE_NO_FLICKER :
                strcat(VID_Msg, "(STVID_FREEZE_MODE_NO_FLICKER)\n");
                break;
            default:
                strcat(VID_Msg, "(UNDEFINED !)\n");
                break;
        }
        sprintf(VID_Msg, "%s   freeze.field=%d ", VID_Msg, Freeze.Field);
        switch(Freeze.Field)
        {
            case STVID_FREEZE_FIELD_TOP :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_TOP)\n");
                break;
            case STVID_FREEZE_FIELD_BOTTOM :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_BOTTOM)\n");
                break;
            case STVID_FREEZE_FIELD_CURRENT :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_CURRENT)\n");
                break;
            case STVID_FREEZE_FIELD_NEXT :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_NEXT)\n");
                break;
            default:
                strcat(VID_Msg, "(UNDEFINED !)\n");
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_Pause() */

/*-------------------------------------------------------------------------
 * Function : VID_PauseSynchro
 *            Resume the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_PauseSynchro(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STTS_t Time;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get duration */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        Time = (STTS_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected duration of the pause");
        }
    }

    if (!(RetErr))
    {
        ErrCode = STVID_PauseSynchro(CurrentHandle, Time );
        sprintf(VID_Msg, "STVID_PauseSynchro(handle=0x%x) : ", CurrentHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] ! \n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_PauseSynchro() */

/*------------------------------------------------------------------------
 * Function : VID_ReleasePictureBuffer
 *            Release a not used picture buffer
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_ReleasePictureBuffer(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    STVID_PictureBufferHandle_t CurrentPictureBufferHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_NO_ERROR;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get picture handle */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_PICTURE_STRUCTURE_FRAME, &LVar );
        CurrentPictureBufferHandle = (STVID_PictureBufferHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected PictureHandle" );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_ReleasePictureBuffer(CurrentHandle, CurrentPictureBufferHandle );
        sprintf(VID_Msg, "STVID_ReleasePictureBuffer(hndl=0x%x,pictbuffhndl=0x%x) : ",
                 (U32)CurrentHandle, (int)CurrentPictureBufferHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_ReleasePictureBuffer() */

/*-------------------------------------------------------------------------
 * Function : VID_GetViewPortQualityOptimizations
 *            Get viewport quality optimisations
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetViewPortQualityOptimizations(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetErr;
    STVID_ViewPortHandle_t  CurrentViewportHndl;
    STLAYER_QualityOptimizations_t  RequestedOptimizations;
    S32                     LVar;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
        CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
        }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        ErrCode = STVID_GetViewPortQualityOptimizations(CurrentViewportHndl, &RequestedOptimizations);

        RetErr = TRUE;
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                sprintf(VID_Msg, "ViewPort Quality optimizations: \n");
                VID_PRINT((VID_Msg ));
                sprintf(VID_Msg, "   DoForceStartOnEvenLine = %d\n", RequestedOptimizations.DoForceStartOnEvenLine);
                VID_PRINT((VID_Msg ));
                sprintf(VID_Msg, "   DoNotRescaleForZoomCloseToUnity = %d\n", RequestedOptimizations.DoNotRescaleForZoomCloseToUnity);
                VID_PRINT((VID_Msg ));
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                sprintf(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                sprintf(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "unexpected error [%X] !\n", ErrCode );
                break;
        }
        if(RetErr == TRUE)
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
        else
        {
            STTST_AssignInteger("RET_VAL1", RequestedOptimizations.DoForceStartOnEvenLine, FALSE);
            STTST_AssignInteger("RET_VAL2", RequestedOptimizations.DoNotRescaleForZoomCloseToUnity, FALSE);
        }
    } /* end if */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetRequestedDeinterlacingMode() */

/*-------------------------------------------------------------------------
 * Function : VID_SetViewPortQualityOptimizations
 *            Set viewport quality optimisations
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetViewPortQualityOptimizations(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetErr;
    STVID_ViewPortHandle_t  CurrentViewportHndl;
    STLAYER_QualityOptimizations_t  RequestedOptimizations;
    S32                     LVar;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
        CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
        }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }

    /* get DoForceStartOnEvenLine flag */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar);
        RequestedOptimizations.DoForceStartOnEvenLine= (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected DoForceStartOnEvenLine" );
        }
    }

    /* get DoNotRescaleForZoomCloseToUnity flag */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar);
        RequestedOptimizations.DoNotRescaleForZoomCloseToUnity= (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected DoNotRescaleForZoomCloseToUnity" );
        }
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        ErrCode = STVID_SetViewPortQualityOptimizations(CurrentViewportHndl, &RequestedOptimizations);
    } /* end if */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_SetRequestedDeinterlacingMode() */

#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
/*-------------------------------------------------------------------------
 * Function : VID_GetRequestedDeinterlacingMode
 *            Get requested Deinterlacing mode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetRequestedDeinterlacingMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetErr;
    STVID_ViewPortHandle_t  CurrentViewportHndl;
    STLAYER_DeiMode_t       RequestedMode;
    S32                     LVar;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
        CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
        }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        ErrCode = STVID_GetRequestedDeinterlacingMode(CurrentViewportHndl, &RequestedMode);

        RetErr = TRUE;
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                sprintf(VID_Msg, "DEI mode currently requested = %d\n", RequestedMode );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                sprintf(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                sprintf(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "unexpected error [%X] !\n", ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }

    } /* end if */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_GetRequestedDeinterlacingMode() */

/*-------------------------------------------------------------------------
 * Function : VID_SetRequestedDeinterlacingMode
 *            Set requested Deinterlacing mode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetRequestedDeinterlacingMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetErr;
    STVID_ViewPortHandle_t  CurrentViewportHndl;
    STLAYER_DeiMode_t       RequestedMode;
    S32                     LVar;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
        CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
        }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }

    /* get RequestedMode */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar);
        RequestedMode = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Mode" );
        }
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        ErrCode = STVID_SetRequestedDeinterlacingMode(CurrentViewportHndl, RequestedMode);
    } /* end if */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_SetRequestedDeinterlacingMode() */
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */

/*-------------------------------------------------------------------------
 * Function : VID_Resume
 *            Resume the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Resume(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_Resume(CurrentHandle );
        sprintf(VID_Msg, "STVID_Resume(handle=0x%x) : ", CurrentHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case STVID_ERROR_DECODER_NOT_PAUSING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is not pausing nor freezing!\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] ! \n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_Resume() */

/*-------------------------------------------------------------------------
 * Function : VID_SetDecodedPictures
 *            Set decoded pictures type
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetDecodedPictures(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_DecodedPictures_t DecodedPictures;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get type of pictures */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, (int)STVID_DECODED_PICTURES_ALL, &LVar );
        DecodedPictures = (STVID_DecodedPictures_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg,"expected Decoded Picture (%d=ALL %d=IP %d=I)",
                    STVID_DECODED_PICTURES_ALL, STVID_DECODED_PICTURES_IP, STVID_DECODED_PICTURES_I );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "VID_SetDecodedPictures(handle=0x%x,decodedpict=%d) : ",
                          CurrentHandle, (S32)DecodedPictures );
        ErrCode = STVID_SetDecodedPictures(CurrentHandle, DecodedPictures );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE:
                API_ErrorCount++;
                strcat(VID_Msg, "impossible with memory profile !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_SetDecodedPictures() */

/*-------------------------------------------------------------------------
 * Function : VID_EnableDeblocking
 *            Enable Deblocking
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_EnableDeblocking(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "VID_EnableDeblocking(handle=0x%x) : ",
                          CurrentHandle );
        ErrCode = STVID_EnableDeblocking(CurrentHandle);
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING :
                API_ErrorCount++;
                RetErr = TRUE;
                strcat(VID_Msg, "decoder not stopped !\n" );
                break ;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE:
                API_ErrorCount++;
                strcat(VID_Msg, "impossible with memory profile !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_EnableDeblocking() */

/*-------------------------------------------------------------------------
 * Function : VID_DisableDeblocking
 *            Disable Deblocking
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_DisableDeblocking(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "VID_DisableDeblocking(handle=0x%x) : ",
                          CurrentHandle );
        ErrCode = STVID_DisableDeblocking(CurrentHandle);
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING :
                API_ErrorCount++;
                RetErr = TRUE;
                strcat(VID_Msg, "decoder not stopped !\n" );
                break ;
            case STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE:
                API_ErrorCount++;
                strcat(VID_Msg, "impossible with memory profile !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_DisableDeblocking() */
#ifdef VIDEO_DEBLOCK_DEBUG
/*-------------------------------------------------------------------------
 * Function : VID_SetDeblockingStrength
 *            set Deblocking Strength
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetDeblockingStrength(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    int DeblockingStrength;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    /* get Deblocking Strength */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, (int)TRUE, &LVar );
        DeblockingStrength = (U8)LVar;
        if (RetErr)
        {
            sprintf(VID_Msg,"expected Deblocking Strength (from 0 to 50)");
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "VID_SetDeblockingStrength(handle=0x%x, DeblocStrength=%d) : ",
                          CurrentHandle, DeblockingStrength);
        ErrCode = STVID_SetDeblockingStrength(CurrentHandle, DeblockingStrength);
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE:
                API_ErrorCount++;
                strcat(VID_Msg, "impossible with memory profile !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_SetDeblockingStrength() */
#endif /* VIDEO_DEBLOCK_DEBUG */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_SetDisplayARConversion
 *            Set pel ratio conversion
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetDisplayARConversion(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_DisplayAspectRatioConversion_t RatioCv;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
        CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
        }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }
    /* get RatioCv */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_DISPLAY_AR_CONVERSION_PAN_SCAN, &LVar);
        RatioCv = (STVID_DisplayAspectRatioConversion_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected RatioCv (%x=pan&scan,%x=letter box,"
                    "%x=combined,%x=ignore)",
                     STVID_DISPLAY_AR_CONVERSION_PAN_SCAN,
                     STVID_DISPLAY_AR_CONVERSION_LETTER_BOX,
                     STVID_DISPLAY_AR_CONVERSION_COMBINED,
                     STVID_DISPLAY_AR_CONVERSION_IGNORE);
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        /* set conversion ratio */
        sprintf(VID_Msg, "STVID_SetDisplayAspectRatioConversion(viewporthndl=0x%x,ratio=%d) : ",
                 (U32)CurrentViewportHndl, RatioCv );
        ErrCode = STVID_SetDisplayAspectRatioConversion(CurrentViewportHndl, RatioCv );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
               RetErr = FALSE;
               strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetDisplayARConversion() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_SetErrorRecoveryMode
 *            Set driver behavior regarding error recovery
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetErrorRecoveryMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_ErrorRecoveryMode_t Mode;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get Mode */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_ERROR_RECOVERY_FULL, &LVar);
        Mode = (STVID_ErrorRecoveryMode_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected Mode (%x=full,%x=high,%x=partial,%x=none)",
                     STVID_ERROR_RECOVERY_FULL,
                     STVID_ERROR_RECOVERY_HIGH,
                     STVID_ERROR_RECOVERY_PARTIAL,
                     STVID_ERROR_RECOVERY_NONE);
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_SetErrorRecoveryMode(hanle=0x%x,mode=%d) : ",
                 CurrentHandle, Mode );
        ErrCode = STVID_SetErrorRecoveryMode(CurrentHandle, Mode);
        switch (ErrCode)
        {
            case ST_NO_ERROR :
               RetErr = FALSE;
               strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetErrorRecoveryMode() */

/*-------------------------------------------------------------------------
 * Function : VID_SetHDPIPParams
 *            Set HDPIP parameters.
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetHDPIPParams(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t              ErrCode;
    STVID_HDPIPParams_t         HDPIP_Params;
    BOOL                        RetErr = FALSE;
    STVID_Handle_t              CurrentHandle;
    S32 LVar;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get HDPIP enable/disable status */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar);
        HDPIP_Params.Enable = (BOOL) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected WidthThreshold");
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get WidthThreshold */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar);
        HDPIP_Params.WidthThreshold = (U32) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected WidthThreshold");
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get HeightThreshold */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar);
        HDPIP_Params.HeightThreshold = (U32) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected HeightThreshold");
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_SetHDPIPParams(hanle=0x%x,%s,Width=%d,Height=%d) : ",
                 CurrentHandle, (HDPIP_Params.Enable ? "enable" : "disable"),
                 HDPIP_Params.WidthThreshold, HDPIP_Params.HeightThreshold );
        ErrCode = STVID_SetHDPIPParams(CurrentHandle, &HDPIP_Params);
        switch (ErrCode)
        {
            case ST_NO_ERROR :
               RetErr = FALSE;
               strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING :
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is running !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetHDPIPParams() */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_SetInputWindowMode
 *            Set input window parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_SetInputWindowMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode = ST_ERROR_INVALID_HANDLE;
    STVID_WindowParams_t WinParams;
    BOOL Auto;
    BOOL RetErr = FALSE;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    BOOL Trace = TRUE;

    /* get ViewPortHandle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
        CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
        }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }
    /* get Auto */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, FALSE, (S32 *)&Auto );
        if (RetErr)
        {
            sprintf(VID_Msg, "expected auto (%d=TRUE,%d=FALSE)", TRUE, FALSE );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get Align */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_WIN_ALIGN_TOP_LEFT, &LVar );
        WinParams.Align = (STVID_WinAlign_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected align (%d,%d,%d,%d,%d,%d,%d,%d,%d)",
                STVID_WIN_ALIGN_TOP_LEFT,
                STVID_WIN_ALIGN_VCENTRE_LEFT,
                STVID_WIN_ALIGN_BOTTOM_LEFT,
                STVID_WIN_ALIGN_TOP_RIGHT,
                STVID_WIN_ALIGN_VCENTRE_RIGHT,
                STVID_WIN_ALIGN_BOTTOM_RIGHT,
                STVID_WIN_ALIGN_BOTTOM_HCENTRE,
                STVID_WIN_ALIGN_TOP_HCENTRE,
                STVID_WIN_ALIGN_VCENTRE_HCENTRE );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get Size */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_WIN_SIZE_DONT_CARE, &LVar );
        WinParams.Size = (STVID_WinSize_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected size (fix=%d,dont_care=%d,incr=%d,decr=%d)",
                     STVID_WIN_SIZE_FIXED,
                     STVID_WIN_SIZE_DONT_CARE,
                     STVID_WIN_SIZE_INCREASE,
                     STVID_WIN_SIZE_DECREASE );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get Trace */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        Trace = (BOOL) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Trace (TRUE/FALSE)" );
        }
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        sprintf(VID_Msg, "STVID_SetInputWindowMode(viewporthndl=0x%x,auto=%d,(align=%d,size=%d)) : ",
                 (U32)CurrentViewportHndl, Auto, WinParams.Align, WinParams.Size );
        RetErr = TRUE;
        ErrCode = STVID_SetInputWindowMode(CurrentViewportHndl, Auto, &WinParams);
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(Trace)
        {
            if(RetErr == FALSE)
            {
                VID_PRINT((VID_Msg ));
            }
            else
            {
                VID_PRINT_ERROR((VID_Msg ));
            }
        }

    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetInputWindowMode() */


/*-------------------------------------------------------------------------
 * Function : VID_SetIOWindows
 *            Adjust the window size and position
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetIOWindows(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    S16 InputWinPositionX;
    S16 InputWinPositionY;
    S16 InputWinWidth;
    S16 InputWinHeight;
    S16 OutputWinPositionX;
    S16 OutputWinPositionY;
    S16 OutputWinWidth;
    S16 OutputWinHeight;
    BOOL Trace;
    BOOL RetErr = FALSE;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
        CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
        }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }
    /* get InputWinPositionX */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        InputWinPositionX = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected InputWinPositionX" );
        }
    }
    /* get InputWinPositionY */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        InputWinPositionY = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected InputWinPositionY" );
        }
    }
    /* get InputWinWidth */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, DEFAULT_SCREEN_WIDTH, &LVar );
        InputWinWidth = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected InputWinWidth" );
        }
    }
    /* get InputWinHeight */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, DEFAULT_SCREEN_HEIGHT, &LVar );
        InputWinHeight = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected InputWinHeight" );
        }
    }
    /* get OutputWinPositionX */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        OutputWinPositionX = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected OutputWinPositionX" );
        }
    }
    /* get OutputWinPositionY */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        OutputWinPositionY = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected OutputWinPositionY" );
        }
    }
    /* get OutputWinWidth */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, DEFAULT_SCREEN_WIDTH, &LVar );
        OutputWinWidth = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected OutputWinWidth" );
        }
    }
    /* get OutputWinHeight */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, DEFAULT_SCREEN_HEIGHT, &LVar );
        OutputWinHeight = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected OutputWinHeight" );
        }
    }
    /* get Trace (Sept 2000 : argument added because STTBX_Print can't be always disable) */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        Trace = (BOOL) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Trace (TRUE/FALSE)" );
        }
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        sprintf(VID_Msg, "STVID_SetIOWindows(viewporthndl=0x%x,ix=%d,iy=%d,iw=%d,ih=%d,ox=%d,oy=%d,ow=%d,oh=%d) : ",
                 (U32)CurrentViewportHndl,
                 InputWinPositionX, InputWinPositionY, InputWinWidth, InputWinHeight,
                 OutputWinPositionX, OutputWinPositionY, OutputWinWidth, OutputWinHeight );
        ErrCode = STVID_SetIOWindows(CurrentViewportHndl,
                                      InputWinPositionX, InputWinPositionY,
                                      InputWinWidth, InputWinHeight,
                                      OutputWinPositionX, OutputWinPositionY,
                                      OutputWinWidth, OutputWinHeight );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                RetErr = TRUE;
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                RetErr = TRUE;
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if (Trace)
        {
            if(RetErr == FALSE)
            {
                VID_PRINT((VID_Msg ));
            }
            else
            {
                VID_PRINT_ERROR((VID_Msg ));
            }
        }
    } /* end if */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetIOWindows() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_SetMemoryProfile
 *            Set amount of memory
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_SetMemoryProfile(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_MemoryProfile_t MemoryProfile;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;
    S32 DefaultNbFrameStore ;
    S32 LVar;
    S32 DefaultMainFixedSize, DefaultDecimatedFixedSize;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;
    DefaultMainFixedSize = 0;
    DefaultDecimatedFixedSize = 0;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get MaxWidth */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 720, &LVar );
        MemoryProfile.MaxWidth = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected MaxWidth (default is 720)" );
        }
    }
    /* get MaxHeight */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 576, &LVar );
        MemoryProfile.MaxHeight = (U32) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected MaxHeight (default is 576)" );
        }
    }
    /* get NbFrameStore */
    if (!(RetErr))
    {
        RetErr = STTST_EvaluateInteger("MEM_PROF_BUFFER_NB", &DefaultNbFrameStore, 10 );
        if (RetErr)
        {
            STTBX_ErrorPrint(("Error : variable MEM_PROF_BUFFER_NB undefined !\n"));
        }
        if (!(RetErr))
        {
            RetErr = STTST_GetInteger(pars_p, DefaultNbFrameStore, &LVar  );
            MemoryProfile.NbFrameStore = (U8)LVar;
            if (RetErr)
            {
                sprintf(VID_Msg, "expected NbFrameStore (default is MEM_PROF_BUFFER_NB=%d), or %d to use optimized API", DefaultNbFrameStore, STVID_OPTIMISED_NB_FRAME_STORE_ID );
                STTST_TagCurrentLine(pars_p, VID_Msg );
            }
        }
    }
    /* get CompressionLevel */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 1, &LVar );
        MemoryProfile.CompressionLevel = (STVID_CompressionLevel_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected CompressionLevel" );
        }
    }
    /* get DecimationFactor */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        MemoryProfile.DecimationFactor = (STVID_DecimationFactor_t)LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected DecimationFactor (0=NONE, 1=H2, 2=V2, 3=H2V2, 4=H4, 8=V4, 12=H4V4)" );
        }
    }
    /* get OptimisedNumber: Main & Decimated */
    if (!(RetErr))
    {
        switch (MemoryProfile.NbFrameStore)
        {
            case STVID_OPTIMISED_NB_FRAME_STORE_ID:
                RetErr = STTST_GetInteger(pars_p, DefaultNbFrameStore, &LVar  );
                MemoryProfile.FrameStoreIDParams.OptimisedNumber.Main = (U8)LVar;
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected OptimisedNumber.Main (default is MEM_PROF_BUFFER_NB=%d)", DefaultNbFrameStore );
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }

                if (!(RetErr))
                {
                    if (MemoryProfile.DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
                    {
                        RetErr = STTST_GetInteger(pars_p, DefaultNbFrameStore, &LVar  );
                        MemoryProfile.FrameStoreIDParams.OptimisedNumber.Decimated = (U8)LVar;
                        if (RetErr)
                        {
                            sprintf(VID_Msg, "expected OptimisedNumber.Decimated (default is MEM_PROF_BUFFER_NB=%d)",
                                DefaultNbFrameStore );
                            STTST_TagCurrentLine(pars_p, VID_Msg );
                        }
                    }
                    else
                    {
                        MemoryProfile.FrameStoreIDParams.OptimisedNumber.Decimated = 0;
                    }
                }
                break;

             case STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID:
                /* Set 6 HD frame buffer size by default */
                DefaultMainFixedSize = 1920*(1088+32)*9; /* Set to 1920x(1088 + 2 Macroblocks lines) x 1.5 x (4 + 2) buffers */
                RetErr = STTST_GetInteger(pars_p, DefaultMainFixedSize, &LVar  );
                MemoryProfile.FrameStoreIDParams.VariableInFixedSize.Main = (U32)LVar;
                MemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated = 0;
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected VariableInFixedSize.Main (default is %d)", DefaultMainFixedSize );
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                if (!(RetErr))
                {
                    if (MemoryProfile.DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
                    {
                        DefaultDecimatedFixedSize = 720*(576+32)*27; /* Set to 720x(576 + 2 Macroblocks lines) x 1.5 x (16 + 2) buffers (16 refs max + 1 decode + 1 display, 720x576 because of dual display SD limitation on 7100) */;
                        RetErr = STTST_GetInteger(pars_p, DefaultDecimatedFixedSize, &LVar  );
                        MemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated = (U32)LVar;
                        if (RetErr)
                        {
                            sprintf(VID_Msg, "expected VariableInFixedSize.Decimated (default is %d)",
                                DefaultDecimatedFixedSize );
                            STTST_TagCurrentLine(pars_p, VID_Msg );
                        }
                    }
                    else
                    {
                        MemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated = 0;
                    }
                }
                break;

             case STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID:
#ifdef ST_OSLINUX
                RetErr = STTST_GetInteger(pars_p, (STAVMEM_PartitionHandle_t) 1, &LVar ); /* Default is Partition 1 (LMI_VID) */
#else
                RetErr = STTST_GetInteger(pars_p, AvmemPartitionHandle[0], &LVar );
#endif /* LINUX */
                if (RetErr)
                {
                    sprintf(VID_Msg, "Expected Avmem Partition Handle (LMI_SYS_AVMEM_HANDLE, LMI_VID_AVMEM_HANDLE)");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                else
                {
                    MemoryProfile.FrameStoreIDParams.VariableInFullPartition.Main = (U32)LVar;
#ifdef ST_OSLINUX
                    RetErr = STTST_GetInteger(pars_p, (STAVMEM_PartitionHandle_t) 1, &LVar ); /* Default is Partition 1 (LMI_VID) */
#else
                    RetErr = STTST_GetInteger(pars_p, AvmemPartitionHandle[0], &LVar );
#endif /* LINUX */
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "Expected Avmem Partition Handle (LMI_SYS_AVMEM_HANDLE, LMI_VID_AVMEM_HANDLE)");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                    else
                    {
                        MemoryProfile.FrameStoreIDParams.VariableInFullPartition.Decimated = (U32)LVar;
                    }
                }
                break;
            default:
                break;
        }
    }


    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_SetMemoryProfile(CurrentHandle, &MemoryProfile );
        switch (MemoryProfile.NbFrameStore)
        {
            case STVID_OPTIMISED_NB_FRAME_STORE_ID :
                if (MemoryProfile.DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
                {
                    sprintf(VID_Msg,
                    "STVID_SetMemoryProfile(handle=0x%x,(width=%d,height=%d,nbfr=%d[OPTIMISED_NB],compress=%d,decim=%d,optnbmain=%d,optnbdecim=%d)) : ",
                    CurrentHandle, MemoryProfile.MaxWidth, MemoryProfile.MaxHeight,
                    MemoryProfile.NbFrameStore, MemoryProfile.CompressionLevel, MemoryProfile.DecimationFactor,
                    MemoryProfile.FrameStoreIDParams.OptimisedNumber.Main,
                    MemoryProfile.FrameStoreIDParams.OptimisedNumber.Decimated );
                }
                else
                {
                    sprintf(VID_Msg,
                    "STVID_SetMemoryProfile(handle=0x%x,(width=%d,height=%d,nbfr=%d[OPTIMISED_NB],compress=%d,decim=%d,optnbmain=%d)) : ",
                    CurrentHandle, MemoryProfile.MaxWidth, MemoryProfile.MaxHeight,
                    MemoryProfile.NbFrameStore, MemoryProfile.CompressionLevel, MemoryProfile.DecimationFactor,
                    MemoryProfile.FrameStoreIDParams.OptimisedNumber.Main );
                }
                break;

            case STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID :
                sprintf(VID_Msg,
                "STVID_SetMemoryProfile(handle=0x%x,(width=%d,height=%d,nbfr=%d[VARIABLE_IN_FIXED_SIZE],compress=%d,decim=%d,mainMaxAllocSize=%d,decimMaxAllocSize=%d)) : ",
                CurrentHandle, MemoryProfile.MaxWidth, MemoryProfile.MaxHeight,
                MemoryProfile.NbFrameStore,
                MemoryProfile.CompressionLevel, MemoryProfile.DecimationFactor,
                MemoryProfile.FrameStoreIDParams.VariableInFixedSize.Main,
                MemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated );
                break;

            case STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID :
                sprintf(VID_Msg,
                "STVID_SetMemoryProfile(handle=0x%x,(width=%d,height=%d,nbfr=%d[VARIABLE_IN_FULL_PARTITION],compress=%d,decim=%d,AVMEM Handle for main FB=0x%x,AVMEM Handle for decim FB=0x%x)) : ",
                CurrentHandle, MemoryProfile.MaxWidth, MemoryProfile.MaxHeight,
                MemoryProfile.NbFrameStore,
                MemoryProfile.CompressionLevel, MemoryProfile.DecimationFactor,
                MemoryProfile.FrameStoreIDParams.VariableInFullPartition.Main,
                MemoryProfile.FrameStoreIDParams.VariableInFullPartition.Decimated );
                break;

            default :
                sprintf(VID_Msg, "STVID_SetMemoryProfile(handle=0x%x,(width=%d,height=%d,nbfr=%d,compress=%d,decim=%d)) : ",
                     CurrentHandle, MemoryProfile.MaxWidth, MemoryProfile.MaxHeight,
                     MemoryProfile.NbFrameStore, MemoryProfile.CompressionLevel,
                     MemoryProfile.DecimationFactor );
                break;
        } /* end switch */

        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is running !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }

    } /* end if */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetMemoryProfile() */

#ifdef ST_XVP_ENABLE_FLEXVP
/*-------------------------------------------------------------------------
 * Function : VID_ActivateXVPProcess
 *            Enable or disable FlexVP process
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_ActivateXVPProcess(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetErr = FALSE;
    STLAYER_ProcessId_t     ProcessID;
    char                    ProcessName[10] = "";
    STVID_ViewPortHandle_t  ViewportHandle;
    S32                     LVar;

    /* get viewport handle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    ViewportHandle = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }

    /* get process name */
    if (!RetErr)
    {
        RetErr = STTST_GetString(pars_p, "", ProcessName, sizeof(ProcessName) );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ProcessName");
        }
    }

    if (!RetErr)
    {
        if (!strcmp(ProcessName, "FGT"))
        {
            ProcessID = STLAYER_PROCESSID_FGT;
        }
        else
        {
            STTST_TagCurrentLine(pars_p, "Bad ProcessName value");
        }
    }

    /* execute function */
    if (!RetErr)
    {
        sprintf(    VID_Msg,
                    "STVID_ActivateXVPProcess(VPhandle=0x%x, ProcessID=0x%x) : ",
                    (S32)ViewportHandle, ProcessID);

        ErrCode = STVID_ActivateXVPProcess(ViewportHandle, ProcessID);
        switch (ErrCode)
        {
        case ST_ERROR_ALREADY_INITIALIZED :
            strcat( VID_Msg, "database already exists, " );

        case ST_NO_ERROR :
            strcat( VID_Msg, "ok\n" );
            break;

        default :
            strcat( VID_Msg, "failed\n" );
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }

    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_DeactivateXVPProcess
 *            Enable or disable FlexVP process
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DeactivateXVPProcess(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetErr = FALSE;
    STLAYER_ProcessId_t     ProcessID;
    char                    ProcessName[10] = "";
    STVID_ViewPortHandle_t  ViewportHandle;
    S32                     LVar;

    /* get viewport handle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    ViewportHandle = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }

    /* get process name */
    if (!RetErr)
    {
        RetErr = STTST_GetString(pars_p, "", ProcessName, sizeof(ProcessName) );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ProcessName");
        }
    }

    if (!RetErr)
    {
        if (!strcmp(ProcessName, "FGT"))
        {
            ProcessID = STLAYER_PROCESSID_FGT;
        }
        else
        {
            STTST_TagCurrentLine(pars_p, "Bad ProcessName value");
        }
    }

    /* execute function */
    if (!RetErr)
    {
        sprintf(    VID_Msg,
                    "STVID_DeactivateXVPProcess(VPhandle=0x%x, ProcessID=0x%x) : ",
                    (S32)ViewportHandle, ProcessID);

        ErrCode = STVID_DeactivateXVPProcess(ViewportHandle, ProcessID);
        if (ErrCode != ST_NO_ERROR)
        {
            strcat( VID_Msg, "failed\n" );
        }
        else
        {
            strcat( VID_Msg, "ok\n" );
        }

        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }

    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_ShowXVPProcess
 *            Enable or disable FlexVP process
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_ShowXVPProcess(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetErr = FALSE;
    STLAYER_ProcessId_t     ProcessID;
    char                    ProcessName[10] = "";
    STVID_ViewPortHandle_t  ViewportHandle;
    S32                     LVar;

    /* get viewport handle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    ViewportHandle = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }

    /* get process name */
    if (!RetErr)
    {
        RetErr = STTST_GetString(pars_p, "", ProcessName, sizeof(ProcessName) );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ProcessName");
        }
    }

    if (!RetErr)
    {
        if (!strcmp(ProcessName, "FGT"))
        {
            ProcessID = STLAYER_PROCESSID_FGT;
        }
        else
        {
            STTST_TagCurrentLine(pars_p, "Bad ProcessName value");
        }
    }

    /* execute function */
    if (!RetErr)
    {
        ErrCode = STVID_ShowXVPProcess(ViewportHandle, ProcessID);
        sprintf(    VID_Msg,
                    "STVID_ShowXVPProcess(VPhandle=0x%x, ProcessID=0x%x) : ",
                    (unsigned int)ViewportHandle, ProcessID);
        if (!ErrCode)
        {
            strcat( VID_Msg, "ok\n" );
        }
        else
        {
            strcat( VID_Msg, "failed\n" );
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }

    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_HideFlexVPProcess
 *            Enable or disable FlexVP process
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_HideXVPProcess(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetErr = FALSE;
    STLAYER_ProcessId_t     ProcessID;
    char                    ProcessName[10] = "";
    STVID_ViewPortHandle_t  ViewportHandle;
    S32                     LVar;

    /* get viewport handle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    ViewportHandle = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }

    /* get process name */
    if (!RetErr)
    {
        RetErr = STTST_GetString(pars_p, "", ProcessName, sizeof(ProcessName) );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ProcessName");
        }
    }

    if (!RetErr)
    {
        if (!strcmp(ProcessName, "FGT"))
        {
            ProcessID = STLAYER_PROCESSID_FGT;
        }
        else
        {
            STTST_TagCurrentLine(pars_p, "Bad ProcessName value");
        }
    }

    /* execute function */
    if (!RetErr)
    {
        ErrCode = STVID_HideXVPProcess(ViewportHandle, ProcessID);
        sprintf(    VID_Msg,
                    "STVID_HideXVPProcess(VPhandle=0x%x, ProcessID=0x%x) : ",
                    (unsigned int)ViewportHandle, ProcessID);
        if (!ErrCode)
        {
            strcat( VID_Msg, "ok\n" );
        }
        else
        {
            strcat( VID_Msg, "failed\n" );
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }

    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE);
}
#endif /* ST_XVP_ENABLE_FLEXVP */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_SetOutputWindowMode
 *            Set output window parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_SetOutputWindowMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode = ST_ERROR_INVALID_HANDLE;
    STVID_WindowParams_t WinParams;
    BOOL Auto;
    BOOL RetErr = FALSE;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    S32 LVar;
    BOOL Trace = TRUE;

    /* get ViewPortHandle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
        CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
        }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }
    /* get auto mode*/
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, FALSE, &LVar );
        Auto = (BOOL) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected auto mode (%d=TRUE,%d=FALSE)", TRUE, FALSE );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get align */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_WIN_ALIGN_TOP_LEFT, &LVar );
        WinParams.Align = (STVID_WinAlign_t)LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected align (%d,%d,%d,%d,%d,%d,%d,%d,%d)",
                STVID_WIN_ALIGN_TOP_LEFT,
                STVID_WIN_ALIGN_VCENTRE_LEFT,
                STVID_WIN_ALIGN_BOTTOM_LEFT,
                STVID_WIN_ALIGN_TOP_RIGHT,
                STVID_WIN_ALIGN_VCENTRE_RIGHT,
                STVID_WIN_ALIGN_BOTTOM_RIGHT,
                STVID_WIN_ALIGN_BOTTOM_HCENTRE,
                STVID_WIN_ALIGN_TOP_HCENTRE,
                STVID_WIN_ALIGN_VCENTRE_HCENTRE );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get size */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_WIN_SIZE_DONT_CARE, &LVar );
        WinParams.Size = (STVID_WinSize_t) LVar;
        if (RetErr)
        {
            sprintf(VID_Msg, "expected size (fix=%d,dont_care=%d,incr=%d,decr=%d)",
                     STVID_WIN_SIZE_FIXED,
                     STVID_WIN_SIZE_DONT_CARE,
                     STVID_WIN_SIZE_INCREASE,
                     STVID_WIN_SIZE_DECREASE );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get Trace */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        Trace = (BOOL) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Trace (TRUE/FALSE)" );
        }
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_SetOutputWindowMode(CurrentViewportHndl, Auto, &WinParams );
        sprintf(VID_Msg, "STVID_SetOutputWindowMode(viewporthndl=0x%x,auto=%d,(align=%d,size=%d)) : ",
                 (U32)CurrentViewportHndl, Auto, WinParams.Align, WinParams.Size );
        switch ( ErrCode )
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat( VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf( VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(Trace)
        {
            if(RetErr == FALSE)
            {
                VID_PRINT((VID_Msg ));
            }
            else
            {
                VID_PRINT_ERROR((VID_Msg ));
            }
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetOutputWindowMode() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_Setup
 *            Setup objects required by the video driver
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_Setup(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_SetupParams_t SetupParams;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get Object */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_SETUP_FRAME_BUFFERS_PARTITION, &LVar );
        if (RetErr)
        {
#if defined(DVD_SECURED_CHIP)
            sprintf(VID_Msg,
                "Expected SetupObject (%d=FRAME_BUFFERS_PARTITION, %d=INTERMEDIATE_BUFFER_PARTITION, %d=DECIMATED_FRAME_BUFFERS_PARTITION, %d=FDMA_NODES_PARTITION," \
                " %d=PICTURE_PARAMETER_BUFFERS_PARTITION, %d=STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION, %d=STVID_SETUP_DATA_INPUT_BUFFER_PARTITION, %d=STVID_SETUP_BIT_BUFFER_PARTITION," \
                " %d=STVID_SETUP_ES_COPY_BUFFER_PARTITION, %d=ES_COPY_BUFFER)",
                    STVID_SETUP_FRAME_BUFFERS_PARTITION,
                    STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION,
                    STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION,
                    STVID_SETUP_FDMA_NODES_PARTITION,
                    STVID_SETUP_PICTURE_PARAMETER_BUFFERS_PARTITION,
                    STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION,
                    STVID_SETUP_DATA_INPUT_BUFFER_PARTITION,
                    STVID_SETUP_BIT_BUFFER_PARTITION,
                    STVID_SETUP_ES_COPY_BUFFER_PARTITION,
                    STVID_SETUP_ES_COPY_BUFFER);
#else
            sprintf(VID_Msg,
                "Expected SetupObject (%d=FRAME_BUFFERS_PARTITION, %d=INTERMEDIATE_BUFFER_PARTITION, %d=DECIMATED_FRAME_BUFFERS_PARTITION, %d=FDMA_NODES_PARTITION," \
                " %d=PICTURE_PARAMETER_BUFFERS_PARTITION, %d=STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION, %d=STVID_SETUP_DATA_INPUT_BUFFER_PARTITION, %d=STVID_SETUP_BIT_BUFFER_PARTITION",
                    STVID_SETUP_FRAME_BUFFERS_PARTITION,
                    STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION,
                    STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION,
                    STVID_SETUP_FDMA_NODES_PARTITION,
                    STVID_SETUP_PICTURE_PARAMETER_BUFFERS_PARTITION,
                    STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION,
                    STVID_SETUP_DATA_INPUT_BUFFER_PARTITION,
                    STVID_SETUP_BIT_BUFFER_PARTITION);
#endif /* DVD_SECURED_CHIP */
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
        else
        {
            SetupParams.SetupObject = (STVID_SetupObject_t) LVar;
        }
    }

    switch (SetupParams.SetupObject)
    {
        case STVID_SETUP_FRAME_BUFFERS_PARTITION:
        case STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION:
        case STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION:
        case STVID_SETUP_FDMA_NODES_PARTITION:
        case STVID_SETUP_PICTURE_PARAMETER_BUFFERS_PARTITION:
        case STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION:
        case STVID_SETUP_DATA_INPUT_BUFFER_PARTITION:
        case STVID_SETUP_BIT_BUFFER_PARTITION:
#if defined(DVD_SECURED_CHIP)
        case STVID_SETUP_ES_COPY_BUFFER_PARTITION:
#endif /* DVD_SECURED_CHIP */
            /* get Avmem Partition Handle */
            if (!(RetErr))
            {
                #ifdef ST_OSLINUX
                    RetErr = STTST_GetInteger(pars_p, (STAVMEM_PartitionHandle_t) 1, &LVar); /* Default is Partition 1 (LMI_VID) */
                #else
                    RetErr = STTST_GetInteger(pars_p, AvmemPartitionHandle[0], &LVar);
                #endif /* LINUX */

                if (RetErr)
                {
                    #if defined(DVD_SECURED_CHIP)
                        STTST_TagCurrentLine(pars_p, "Expected Avmem Partition Handle (LMI_SYS_AVMEM_HANDLE, LMI_VID_AVMEM_HANDLE, NON_SECURE_AVMEM_HANDLE, SECURE_AVMEM_HANDLE, RESERVED_AVMEM_HANDLE)");
                    #else
                        STTST_TagCurrentLine(pars_p, "Expected Avmem Partition Handle (LMI_SYS_AVMEM_HANDLE, LMI_VID_AVMEM_HANDLE)");
                    #endif /* DVD_SECURED_CHIP */
                }
                else
                {
                    SetupParams.SetupSettings.AVMEMPartition = (STAVMEM_PartitionHandle_t) LVar;
                }
            }
            break;

#if defined(DVD_SECURED_CHIP)
        case STVID_SETUP_ES_COPY_BUFFER:
            /**********************************************/
            /* Fill-in setup parameters for single buffer */
            /**********************************************/

            /* get BufferSize */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger(pars_p, (200*1024), &LVar); /* Default is ES_COPY_BUFFER_SIZE (200kB) */

                if (RetErr)
                {
                    STTST_TagCurrentLine(pars_p, "Expected Buffer size");
                }
                else
                {
                    SetupParams.SetupSettings.SingleBuffer.BufferSize = (U32) LVar;
                }
            }

            /* Get BufferAllocated */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger(pars_p, FALSE, &LVar ); /* Default is FALSE */

                if (RetErr)
                {
                    STTST_TagCurrentLine(pars_p, "Expected BufferAllocated info" );
                }
                else
                {
                    SetupParams.SetupSettings.SingleBuffer.BufferAllocated = (BOOL) LVar;
                }
            }

            /* Get Buffer Address (just in case the buffer is allocated by application already */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger(pars_p, 0, &LVar );

                if(SetupParams.SetupSettings.SingleBuffer.BufferAllocated)
                {
                    if (RetErr)
                    {
                        STTST_TagCurrentLine(pars_p, "Expected Buffer Address" );
                    }
                    else
                    {
                        SetupParams.SetupSettings.SingleBuffer.BufferAddress_p = (void *) LVar;
                    }
                }
                else if((U32)LVar != 0)
                {
                    STTST_TagCurrentLine(pars_p, "Warning: Buffer Address is expected only if BufferAllocated info is TRUE" );
                }
            }
            break;
#endif /* DVD_SECURED_CHIP */

        default:
            RetErr = TRUE;
            STTST_TagCurrentLine(pars_p, "Unknown Setup object");
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_Setup(CurrentHandle, &SetupParams);

        if (SetupParams.SetupObject == STVID_SETUP_ES_COPY_BUFFER)  /* And all future buffer objects */
        {
            if (SetupParams.SetupSettings.SingleBuffer.BufferAllocated)
            {
                sprintf(VID_Msg, "STVID_Setup(handle=0x%x, Object=%s)\r\n\tBuffer Allocated by application: TRUE - Size=%d, Address: 0x%x -> ",
                    CurrentHandle,
                    VIDSETUPOBJECTTOSTRING(SetupParams.SetupObject),
                    SetupParams.SetupSettings.SingleBuffer.BufferSize,
                    (U32)SetupParams.SetupSettings.SingleBuffer.BufferAddress_p);
            }
            else
            {
                sprintf(VID_Msg, "STVID_Setup(handle=0x%x, Object=%s)\r\n\tBuffer Allocated by application: FALSE -> ",
                    CurrentHandle,
                    VIDSETUPOBJECTTOSTRING(SetupParams.SetupObject));
            }
        }
        else
        {
            sprintf(VID_Msg, "STVID_Setup(handle=0x%x, Object=%s, Avmem Partition=0x%x): ", CurrentHandle,
                  VIDSETUPOBJECTTOSTRING(SetupParams.SetupObject),
                  SetupParams.SetupSettings.AVMEMPartition);
        }

        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one of the supplied parameter is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "it is not allowed to call this function at this time !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one of the supplied parameter is not supported !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder not stopped!\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_Setup() */

#if defined(DVD_SECURED_CHIP)
/*-------------------------------------------------------------------------
 * Function : VID_SetupH264PreprocWA_GNBvd42332
 *            Setup objects for H264 preproc WA
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetupH264PreprocWA_GNBvd42332(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    STAVMEM_PartitionHandle_t AVMEMPartition;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get Object */
    if (!(RetErr))
    {
        #ifdef ST_OSLINUX
            RetErr = STTST_GetInteger(pars_p, (STAVMEM_PartitionHandle_t) 0, &LVar); /* Default is Partition 0 (LMI_SYS) */
        #else
            RetErr = STTST_GetInteger(pars_p, AvmemPartitionHandle[0], &LVar);
        #endif /* LINUX */

        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
        else
        {
            AVMEMPartition = (STAVMEM_PartitionHandle_t) LVar;
        }
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_SetupReservedPartitionForH264PreprocWA_GNB42332(CurrentHandle, AVMEMPartition);

        sprintf(VID_Msg, "STVID_SetupH264PreprocWA_GNB42332(handle=0x%x, partition 0x%08x) -> ",
                    CurrentHandle,
                    AVMEMPartition);

        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one of the supplied parameter is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "it is not allowed to call this function at this time !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one of the supplied parameter is not supported !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder not stopped!\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetupH264PreprocWA_GNBvd42332() */
#endif /* DVD_SECURED_CHIP */

/*-------------------------------------------------------------------------
 * Function : VID_SetSpeed
 *            Set play speed
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetSpeed(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    S32 Speed;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get Speed */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Speed );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Speed" );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_SetSpeed(CurrentHandle, Speed );
        sprintf(VID_Msg, "STVID_SetSpeed(handle=0x%x,speed=%d) : ", CurrentHandle, Speed );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                STTST_AssignInteger("RET_VAL1", Speed, FALSE);
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING_IN_RT_MODE :
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is running in real time !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetSpeed() */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_SetViewPortAlpha
 *            Set viewport global alpha value
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetViewPortAlpha(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t  CurrentViewportHndl;
    STLAYER_GlobalAlpha_t   VPAlpha;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
        CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
        }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }

    /* get Alpha value (A0) */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar);
        VPAlpha.A0 = (U8) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Alpha value (A0) (0 to 128)" );
        }
    }
    /* get Alpha value (A1) */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar);
        VPAlpha.A1 = (U8) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Alpha value (A1) (0 to 128)" );
        }
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_SetViewPortAlpha (viewporthndl=0x%x, Alpha : A0=%d, A1=%d) : ",
                 (U32)CurrentViewportHndl, VPAlpha.A0, VPAlpha.A1 );
        ErrCode = STVID_SetViewPortAlpha(CurrentViewportHndl, &VPAlpha );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                strcat(VID_Msg, "ok\n" );
                RetErr = FALSE;
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE:
                API_ErrorCount++;
                strcat(VID_Msg, "Feature is not available !\n" );
                break;
           case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }

    } /* end if */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_SetViewPortAlpha() */

/*-------------------------------------------------------------------------
 * Function : VID_SetViewPortColorKey
 *            Set viewport color key
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetViewPortColorKey(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STGXOBJ_ColorKey_t VPColorKey;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        /* color key */
        memset(&VPColorKey, 0, sizeof(VPColorKey));
        /**VPColorKey.Type = 0;
        VPColorKey.Value.CLUT1.PaletteEntryMin = 0;
        VPColorKey.Value.CLUT1.PaletteEntryMax = 0;
        VPColorKey.Value.CLUT1.PaletteEntryOut = 0;
        VPColorKey.Value.CLUT1.PaletteEntryEnable = 0;
        VPColorKey.Value.CLUT8.PaletteEntryMin = 0;
        VPColorKey.Value.CLUT8.PaletteEntryMax = 0;
        VPColorKey.Value.CLUT8.PaletteEntryOut = 0;
        VPColorKey.Value.CLUT8.PaletteEntryEnable = 0;
        VPColorKey.Value.RGB888.RMin = 0;
        VPColorKey.Value.RGB888.RMax = 0;
        VPColorKey.Value.RGB888.ROut = 0;
        VPColorKey.Value.RGB888.REnable = 0;
        VPColorKey.Value.RGB888.GMin = 0;
        VPColorKey.Value.RGB888.GMax = 0;
        VPColorKey.Value.RGB888.GOut = 0;
        VPColorKey.Value.RGB888.GEnable = 0;
        VPColorKey.Value.RGB888.BMin = 0;
        VPColorKey.Value.RGB888.BMax = 0;
        VPColorKey.Value.RGB888.BOut = 0;
        VPColorKey.Value.RGB888.BEnable = 0;
        VPColorKey.Value.SignedYCbCr888.YMin = 0;
        VPColorKey.Value.SignedYCbCr888.YMax = 0;
        VPColorKey.Value.SignedYCbCr888.YOut = 0;
        VPColorKey.Value.SignedYCbCr888.YEnable = 0;
        VPColorKey.Value.SignedYCbCr888.CbMin = 0;
        VPColorKey.Value.SignedYCbCr888.CbMax = 0;
        VPColorKey.Value.SignedYCbCr888.CbOut = 0;
        VPColorKey.Value.SignedYCbCr888.CbEnable = 0;
        VPColorKey.Value.SignedYCbCr888.CrMin = 0;
        VPColorKey.Value.SignedYCbCr888.CrMax = 0;
        VPColorKey.Value.SignedYCbCr888.CrOut = 0;
        VPColorKey.Value.SignedYCbCr888.CrEnable = 0;
        VPColorKey.Value.UnsignedYCbCr888.YMin = 0;
        VPColorKey.Value.UnsignedYCbCr888.YMax = 0;
        VPColorKey.Value.UnsignedYCbCr888.YOut = 0;
        VPColorKey.Value.UnsignedYCbCr888.YEnable = 0;
        VPColorKey.Value.UnsignedYCbCr888.CbMin = 0;
        VPColorKey.Value.UnsignedYCbCr888.CbMax = 0;
        VPColorKey.Value.UnsignedYCbCr888.CbOut = 0;
        VPColorKey.Value.UnsignedYCbCr888.CbEnable = 0;
        VPColorKey.Value.UnsignedYCbCr888.CrMin = 0;
        VPColorKey.Value.UnsignedYCbCr888.CrMax = 0;
        VPColorKey.Value.UnsignedYCbCr888.CrOut = 0;
        VPColorKey.Value.UnsignedYCbCr888.CrEnable = 0;
        **/
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_SetViewPortColorKey(viewporthndl=0x%x,&VPColorKey=0x%x) : ",
                          (S32) CurrentViewportHndl, (S32)&VPColorKey);

        ErrCode = STVID_SetViewPortColorKey(CurrentViewportHndl, &VPColorKey );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE:
                API_ErrorCount++;
                strcat(VID_Msg, "Feature is not available !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_SetViewPortColorKey() */

/*-------------------------------------------------------------------------
 * Function : VID_SetViewPortPSI
 *            Set Picture Structure Improvement param.
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetViewPortPSI(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STLAYER_PSI_t                   PSIParam;
    S32 LVar;
    BOOL Trace = TRUE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        /* Get filter type.  */
        RetErr = STTST_GetInteger(pars_p, STLAYER_VIDEO_CHROMA_SAT, &LVar );
        if (RetErr)
        {
            sprintf(VID_Msg, "expected Filter type (AF:%d, GB:%d, TINT:%d, SAT:%d, ER:%d, PEAK:%d, DCI:%d, BC:%d)",

                STLAYER_VIDEO_CHROMA_AUTOFLESH, STLAYER_VIDEO_CHROMA_GREENBOOST, STLAYER_VIDEO_CHROMA_TINT,
                STLAYER_VIDEO_CHROMA_SAT, STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT, STLAYER_VIDEO_LUMA_PEAKING,
                STLAYER_VIDEO_LUMA_DCI,STLAYER_VIDEO_LUMA_BC);

            STTST_TagCurrentLine(pars_p, VID_Msg);
        }
        else
        {
            PSIParam.VideoFiltering = LVar;
            /* Get filter control.  */
            RetErr = STTST_GetInteger(pars_p, 0, &LVar);
            if (RetErr)
            {
                sprintf(VID_Msg, "expected Filter control (OFF:%d, AUTO1:%d, AUTO2:%d, AUTO3:%d, MANUAL:%d)"
                        , STLAYER_DISABLE
                        , STLAYER_ENABLE_AUTO_MODE1
                        , STLAYER_ENABLE_AUTO_MODE2
                        , STLAYER_ENABLE_AUTO_MODE3
                        , STLAYER_ENABLE_MANUAL);
                STTST_TagCurrentLine(pars_p, VID_Msg );
            }
            else
            {
                PSIParam.VideoFilteringControl = LVar ;
            }
        }
    }
    /* Now, get input parameters. */
    if (!(RetErr))
    {
        switch (PSIParam.VideoFiltering)
        {
            case STLAYER_VIDEO_CHROMA_AUTOFLESH :
                /* Get filter control.  */
                RetErr = STTST_GetInteger(pars_p, 50,
                        (S32*)&PSIParam.VideoFilteringParameters.AutoFleshParameters.AutoFleshControl );
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected AutoFlesh control (percent basis :0 to 100)");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                else
                {
                    RetErr = STTST_GetInteger(pars_p, MEDIUM_WIDTH, &LVar );
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected Quadrature Width (LARGE:%d, MEDIUM:%d, SMALL:%d)"
                                , LARGE_WIDTH, MEDIUM_WIDTH, SMALL_WIDTH);
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                    else
                    {
                        PSIParam.VideoFilteringParameters.AutoFleshParameters.QuadratureFleshWidthControl = LVar;
                        RetErr = STTST_GetInteger(pars_p, AXIS_121_0, &LVar );
                        if (RetErr)
                        {
                            sprintf(VID_Msg, "expected Flesh Axis control from %d to %d (nominal=%d)",
                                    AXIS_116_6, AXIS_130_2, AXIS_121_0);
                            STTST_TagCurrentLine(pars_p, VID_Msg );
                        }
                        else
                        {
                           PSIParam.VideoFilteringParameters.AutoFleshParameters.AutoFleshAxisControl = LVar;
                        }
                    }
                }
                break;
            case STLAYER_VIDEO_CHROMA_GREENBOOST :
                /* Get Green boost gain control.    */
                RetErr = STTST_GetInteger(pars_p, 0,
                        (S32*)&PSIParam.VideoFilteringParameters.GreenBoostParameters.GreenBoostControl );
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected Green boost gain control (percent basis :-100 to 100)");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                break;
            case STLAYER_VIDEO_CHROMA_TINT :
                /* Get Tint rotation gain control.  */
                RetErr = STTST_GetInteger(pars_p, 0,
                        (S32*)&PSIParam.VideoFilteringParameters.TintParameters.TintRotationControl );
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected Tint rotation gain control (percent basis :-100 to 100)");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                break;
            case STLAYER_VIDEO_CHROMA_SAT :
                /* Get Saturation gain control.     */
                RetErr = STTST_GetInteger(pars_p, 0,
                        (S32*)&PSIParam.VideoFilteringParameters.SatParameters.SaturationGainControl );
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected Saturation gain control (percent basis :-100 to 100)");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                break;
            case STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT :
                /* Get Edge replacement gain control.           */
                RetErr = STTST_GetInteger(pars_p, 0,
                        (S32*)&PSIParam.VideoFilteringParameters.EdgeReplacementParameters.GainControl);
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected gain control (percent basis :0 to 100)");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                else
                {
                    /* Get Edge replacement frequency control.  */
                    RetErr = STTST_GetInteger(pars_p, 0, &LVar);
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected Frequency control (HIGH:%d, MEDIUM:%d or LOW:%d)",
                                HIGH_FREQ_FILTER, MEDIUM_FREQ_FILTER, LOW_FREQ_FILTER);
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                    else
                    {
                        PSIParam.VideoFilteringParameters.EdgeReplacementParameters.FrequencyControl = LVar;
                    }
                }
                break;
            case STLAYER_VIDEO_LUMA_PEAKING :
                /* Vertical peaking gain control.           */
                RetErr = STTST_GetInteger(pars_p, 0,
                        (S32*)&PSIParam.VideoFilteringParameters.PeakingParameters.VerticalPeakingGainControl);
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected Vertical peaking gain control (percent basis :-100 to 100)");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                /* Coring for vertical peaking.             */
                if (!RetErr)
                {
                    RetErr = STTST_GetInteger(pars_p, 0,
                            (S32*)&PSIParam.VideoFilteringParameters.PeakingParameters.CoringForVerticalPeaking);
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected Coring for vertical peaking (percent basis :0 to 100)");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                }
                if (!RetErr)
                {
                    /* Horizontal peaking gain control.     */
                    RetErr = STTST_GetInteger(pars_p, 0,
                            (S32*)&PSIParam.VideoFilteringParameters.PeakingParameters.HorizontalPeakingGainControl);
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected Horizontal peaking gain control (percent basis :-100 to 100)");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                }
                /* Coring for Horizontal peaking.           */
                if (!RetErr)
                {
                    RetErr = STTST_GetInteger(pars_p, 0,
                            (S32*)&PSIParam.VideoFilteringParameters.PeakingParameters.CoringForHorizontalPeaking);
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected Coring for vertical peaking (percent basis :0 to 100)");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                }
                if (!RetErr)
                {
                    RetErr = STTST_GetInteger(pars_p, 50,
                            (S32*)&PSIParam.VideoFilteringParameters.PeakingParameters.HorizontalPeakingFilterSelection);
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected Peaking filter selection (Fs/Fc: 0 to 50)");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                }
                if (!RetErr)
                {
                    RetErr = STTST_GetInteger(pars_p, FALSE,
                            (S32*)&PSIParam.VideoFilteringParameters.PeakingParameters.SINECompensationEnable);
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected Sinx/x enable state ");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                }
                break;
            case STLAYER_VIDEO_LUMA_DCI :
                /* Coring level of DCI.     */
                RetErr = STTST_GetInteger(pars_p, 0,
                        (S32*)&PSIParam.VideoFilteringParameters.DCIParameters.CoringLevelGainControl);
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected Coring level for DCI (percent basis :0 to 100)");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                if (!RetErr)
                {
                    RetErr = STTST_GetInteger(pars_p, 0,
                            (S32*)&PSIParam.VideoFilteringParameters.DCIParameters.FirstPixelAnalysisWindow);
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected First pixel of Analysis window");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                }
                if (!RetErr)
                {
                    RetErr = STTST_GetInteger(pars_p, 0,
                            (S32*)&PSIParam.VideoFilteringParameters.DCIParameters.LastPixelAnalysisWindow);
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected Last pixel of Analysis window");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                }
                if (!RetErr)
                {
                    RetErr = STTST_GetInteger(pars_p, 0,
                            (S32*)&PSIParam.VideoFilteringParameters.DCIParameters.FirstLineAnalysisWindow);
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected First line of Analysis window");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                }
                if (!RetErr)
                {
                    RetErr = STTST_GetInteger(pars_p, 0,
                            (S32*)&PSIParam.VideoFilteringParameters.DCIParameters.LastLineAnalysisWindow);
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected Last line of Analysis window");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                }
                break;
            case STLAYER_VIDEO_LUMA_BC :
                /* Get Brightness gain control.  */
                RetErr = STTST_GetInteger(pars_p, 0,
                        (S32*)&PSIParam.VideoFilteringParameters.BCParameters.BrightnessGainControl );
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected Brightness gain control (percent basis :-100 to 100)");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                /* Get Contrast gain control.  */
                RetErr = STTST_GetInteger(pars_p, 50,
                        (S32*)&PSIParam.VideoFilteringParameters.BCParameters.ContrastGainControl );
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected Contrast gain control (percent basis :0 to 100)");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                break;

            default :
                    sprintf(VID_Msg, "Unexpected video filtering type  !!!");
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                break;
        } /* end switch */
    }

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        Trace = (BOOL) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Trace (TRUE/FALSE)" );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_SetViewPortPSI(CurrentViewportHndl, &PSIParam );

        sprintf(VID_Msg, "STVID_SetViewPortPSI(handle=0x%x,&PSIParam=0x%x) : ",
                 (U32)CurrentViewportHndl, (U32)&PSIParam );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "feature not available !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if (Trace)
        {
            if(RetErr == FALSE)
            {
                VID_PRINT((VID_Msg ));
            }
            else
            {
                VID_PRINT_ERROR((VID_Msg ));
            }
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetViewPortPSI() */

/*-------------------------------------------------------------------------
 * Function : VID_SetViewPortSpecialMode
 *            Set Special mode param.
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetViewPortSpecialMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STLAYER_OutputMode_t OutputMode;
    STLAYER_OutputWindowSpecialModeParams_t Params;
    S32 LVar;
    U32 cpt;
    U16 Zone[MAX_NB_ZONE];
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }
    else
    {
        /* Get Output mode  */
        RetErr = STTST_GetInteger(pars_p, STLAYER_NORMAL_MODE, &LVar );
        if (RetErr)
        {
            sprintf(VID_Msg, "expected output mode Normal (0), Spectacle (1)");
            STTST_TagCurrentLine(pars_p, VID_Msg);
        }
        else
        {
            OutputMode = LVar;
            /* Get Effect power (percent unit).  */
            RetErr = STTST_GetInteger(pars_p, 50, (S32*)&LVar );
            if (RetErr)
            {
                sprintf(VID_Msg, "expected Power Filter control 0 to 100%%");
                STTST_TagCurrentLine(pars_p, VID_Msg);
            }
            else
            {
                Params.SpectacleModeParams.EffectControl = (U16)LVar;
                /* Get Number of zone  */
                RetErr = STTST_GetInteger(pars_p, MAX_NB_ZONE, &LVar);
                if (RetErr)
                {
                    sprintf(VID_Msg, "expected number of zone < %d", MAX_NB_ZONE);
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                }
                else
                {
                    Params.SpectacleModeParams.NumberOfZone = (U16)LVar;
                    /* Get Zones... */
                    for (cpt=0;cpt<Params.SpectacleModeParams.NumberOfZone && !RetErr; cpt++)
                    {
                        RetErr = STTST_GetInteger(pars_p, 0, &LVar);
                        Zone[cpt] = (U16)(LVar);
                    }
                    if (RetErr)
                    {
                        sprintf(VID_Msg, "expected zone size (percent unit)");
                        STTST_TagCurrentLine(pars_p, VID_Msg );
                    }
                    else
                    {
                        Params.SpectacleModeParams.SizeOfZone_p = Zone;
                    }
                }
            }
        }
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STVID_SetViewPortSpecialMode(CurrentViewportHndl, OutputMode, &Params );

        sprintf(VID_Msg, "STVID_SetViewPortSpecialMode(handle=0x%x,Mode=%d,&Params=0x%x) : ",
                 (U32)CurrentViewportHndl, OutputMode, (U32)&Params );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "feature not available !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SetViewPortSpecialMode() */

/*-------------------------------------------------------------------------
 * Function : VID_ShowPicture
 *            Restore the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_ShowPicture(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_PictureParams_t PictParams;
    STVID_Freeze_t Freeze;
    BOOL RetErr;
    S32 PictureNo;
    S32 LVar;
    STVID_ViewPortHandle_t CurrentViewportHndl;
/*** STVID_PictureParams_t SourcePictureParams; for debug */
    RetErr = FALSE;
    if (NbOfCopiedPictures == 0)
    {
        STTST_TagCurrentLine(pars_p, "VID_ShowPicture() : no picture to show in memory !" );
        return(API_EnableError ? TRUE : FALSE );
    }

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
        CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
        }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }
    /* get picture number */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, NbOfCopiedPictures, &PictureNo );
        if (RetErr)
        {
            sprintf(VID_Msg, "expected Picture No. (max is %d)", NbOfCopiedPictures );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    if (PictureNo > NbOfCopiedPictures)
    {
        PictureNo = NbOfCopiedPictures ;
    }
    if (PictureNo <= 0)
    {
        PictureNo = 1 ;
    }

    Freeze.Field = STVID_FREEZE_FIELD_TOP;
    Freeze.Mode = STVID_FREEZE_MODE_FORCE;
    PictParams = CopiedPictureParams[PictureNo-1];

/*** for debug (trace+check data)
STTBX_Print(("NbOfCopiedPictures=%d PictureNo=%d\n",NbOfCopiedPictures,PictureNo));
STVID_GetPictureParams(VID_Inj_GetDriverHandle(0), STVID_PICTURE_DISPLAYED, &SourcePictureParams);
SourcePictureParams.Data = PictParams.Data;
if (memcmp((char *)&SourcePictureParams,(char *)&PictParams,sizeof(SourcePictureParams))!=0)
        STTBX_Print(("STVID_DuplicatePicture(): picture params badly copied !!!\n"));
if (memcmp((char *)SourcePictureParams.Data,(char *)&PictParams.Data, SourcePictureParams.Size)!=0)
        STTBX_Print(("STVID_DuplicatePicture(): data badly copied !!!\n"));
***/
    if (pars_p->line_p[pars_p->par_pos] == '\0')  /* no param on line */
    {
        RetErr = FALSE;
    }
    else
    {
        if (!(RetErr))
        {
            RetErr = STTST_GetInteger(pars_p, STVID_FREEZE_MODE_FORCE, &LVar );
            Freeze.Mode = (STVID_FreezeMode_t) LVar;
            if (RetErr)
            {
                sprintf(VID_Msg, "expected Mode (%d=none %d=force %d=no_flicker)" ,
                            STVID_FREEZE_MODE_NONE, STVID_FREEZE_MODE_FORCE, STVID_FREEZE_MODE_NO_FLICKER );
                STTST_TagCurrentLine(pars_p, VID_Msg);
            }
        }
        /* get freeze field */
        if (!(RetErr))
        {
            RetErr = STTST_GetInteger(pars_p, STVID_FREEZE_FIELD_TOP, &LVar );
            Freeze.Field = (STVID_FreezeField_t) LVar;
            if (RetErr)
            {
                sprintf(VID_Msg, "expected Field (%d=top %d=bot %d=current %d=next)" ,
                        STVID_FREEZE_FIELD_TOP, STVID_FREEZE_FIELD_BOTTOM,
                        STVID_FREEZE_FIELD_CURRENT, STVID_FREEZE_FIELD_NEXT );
                STTST_TagCurrentLine(pars_p, VID_Msg);
            }
        }
    } /* end if input param. */

    if (!(RetErr)) /* if parameters are ok */
    {
        RetErr = TRUE;
        ErrCode = STVID_ShowPicture(CurrentViewportHndl, &PictParams, &Freeze );
        sprintf(VID_Msg, "STVID_ShowPicture(viewporthndl=0x%x,&param=0x%x,&freeze=0x%x) : ",
                 (S32) CurrentViewportHndl, (S32)&PictParams, (S32)&Freeze );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                sprintf(VID_Msg, "%s   codingmode=%d colortype=%d mpegframe=%d height=%d width=%d aspect=%d \n",
                         VID_Msg, PictParams.CodingMode,
                         PictParams.ColorType, PictParams.MPEGFrame,
                         PictParams.Height,
                         PictParams.Width, PictParams.Aspect );
                sprintf(VID_Msg, "%s   scantype=%d topfieldfirst=%d framerate=%d hours=%d minutes=%d seconds=%d \n",
                         VID_Msg, PictParams.ScanType,
                         PictParams.TopFieldFirst, PictParams.FrameRate,
                         PictParams.TimeCode.Hours, PictParams.TimeCode.Minutes,
                         PictParams.TimeCode.Seconds );
                sprintf(VID_Msg, "%s   frames=%d interpolated=%d pts=%d pts33=%d ptsinterpolated=%d freezemode=%d freezefield=%d \n",
                         VID_Msg, PictParams.TimeCode.Frames,
                         PictParams.TimeCode.Interpolated, PictParams.PTS.PTS,
                         PictParams.PTS.PTS33, PictParams.PTS.Interpolated,
                         Freeze.Mode, Freeze.Field );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat(VID_Msg, "invalid handle !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder currently running !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_ShowPicture */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_SkipSynchro
 *            Skip some pictures
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SkipSynchro(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STTS_t Time;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get time to skip */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        Time = (STTS_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected time to skip" );
        }
    }
    if (!(RetErr)) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_SkipSynchro(handle=0x%x,time=%d) : ",
                 CurrentHandle, Time );
        ErrCode = STVID_SkipSynchro(CurrentHandle, Time );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE:
                API_ErrorCount++;
                strcat(VID_Msg, "feature not available !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    } /* end if */
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_SkipSynchro() */

/*-------------------------------------------------------------------------
 * Function : VID_Start
 *            Start decoding of input stream
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_Start(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_StartParams_t StartParams;
    S32 LVar;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
    /* get RealTime */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected RealTime decoding (TRUE=yes FALSE=no)" );
        }
    }
    /* get UpdateDisplay  */
    if (!(RetErr))
    {
        StartParams.RealTime = (BOOL)LVar;
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected UpdateDisplay (TRUE=yes FALSE=no)" );
        }
    }
    /* get stream type */
    if (!(RetErr))
    {
        StartParams.UpdateDisplay = (BOOL)LVar;
#ifdef STVID_DIRECTV
        RetErr = STTST_GetInteger(pars_p, STVID_STREAM_TYPE_ES, &LVar);
#else
        RetErr = STTST_GetInteger(pars_p, STVID_STREAM_TYPE_PES, &LVar);
#endif
        if (RetErr)
        {
          sprintf(VID_Msg, "expected StreamType (%x=ES,%x=PES,%x=Mpeg1 pack.)",
                   STVID_STREAM_TYPE_ES, STVID_STREAM_TYPE_PES,
                   STVID_STREAM_TYPE_MPEG1_PACKET );
          STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get stream id (default : accept all packets in decoder) */
    if (!(RetErr))
    {
        StartParams.StreamType = LVar;
        RetErr = STTST_GetInteger(pars_p, STVID_IGNORE_ID, &LVar);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected id (from 0 to 15)");
        }
    }
    /* get broadcast type */
    if (!(RetErr))
    {
        StartParams.StreamID = (U8)LVar;
#ifdef STVID_DIRECTV
        RetErr = STTST_GetInteger(pars_p, STVID_BROADCAST_DIRECTV, &LVar);
#else
/*    #if defined (ST_7015) || defined(ST_7020)*/
/*        RetErr = STTST_GetInteger(pars_p, STVID_BROADCAST_ATSC, &LVar);*/
/*    #else*/
        /* Default is DVB for everybody */
        RetErr = STTST_GetInteger(pars_p, STVID_BROADCAST_DVB, &LVar);
/*    #endif*/
#endif
        if (RetErr)
        {
            sprintf(VID_Msg, "expected broadcast type (%d=DVB,%d=DirecTV,%d=ATSC,%d=DVD)",
                    STVID_BROADCAST_DVB, STVID_BROADCAST_DIRECTV,
                    STVID_BROADCAST_ATSC, STVID_BROADCAST_DVD );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get DecodeOnce */
    if (!(RetErr))
    {
        StartParams.BrdCstProfile = (U8)LVar;
        RetErr = STTST_GetInteger(pars_p, FALSE, &LVar);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected DecodeOnce (FALSE=all picture, TRUE=1 picture)" );
        }
        else
        {
            StartParams.DecodeOnce = (U8)LVar;
        }
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_Start(handle=0x%x,&param=0x%x) : ",
                 CurrentHandle, (S32)&StartParams );
        ErrCode = STVID_Start(CurrentHandle, &StartParams );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                /* Store info (for injection task) that the video is started for this handle  */
                /* because VID_GetBufferFreeSize() return total free size when stopped        */
                /* This means that the injection task is feeding continuously the bit buffer. */
                for(LVar=0;LVar<VIDEO_MAX_DEVICE;LVar++)
                {
                    if ( VID_Inj_GetDriverHandle(LVar) == CurrentHandle)
                    {
                        VID_Inj_SetVideoDriverState(LVar, STATE_DRIVER_STARTED);
                    }
                }
                break;
            case STVID_ERROR_DECODER_PAUSING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder already pausing !\n" );
                break;
            case STVID_ERROR_DECODER_FREEZING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is freezing !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder already decoding !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_NO_MEMORY:
                API_ErrorCount++;
                strcat(VID_Msg, "not enough space !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }

        /* Trace */
        sprintf(VID_Msg, "   realtime=%d updatedisplay=%d streamtype=%d ",
                 StartParams.RealTime,
                 StartParams.UpdateDisplay, StartParams.StreamType );
        switch(StartParams.StreamType)
        {
            case STVID_STREAM_TYPE_ES:
                strcat(VID_Msg, "(ES) \n");
                break;
            case STVID_STREAM_TYPE_PES:
                strcat(VID_Msg, "(PES) \n");
                break;
            case STVID_STREAM_TYPE_MPEG1_PACKET:
                strcat(VID_Msg, "(Mpeg1 Packet) \n");
                break;
            default:
                strcat(VID_Msg, "Unknown!\n");
                break;
        } /* end switch */
        sprintf(VID_Msg, "%s   streamid=%d broadcast=%d ",
                 VID_Msg,
                 StartParams.StreamID, StartParams.BrdCstProfile);
        switch(StartParams.BrdCstProfile)
        {
            case STVID_BROADCAST_DVB:
                strcat(VID_Msg, "(DVB)");
                break;
            case STVID_BROADCAST_DIRECTV:
                strcat(VID_Msg, "(DIRECTV)");
                break;
            case STVID_BROADCAST_ATSC:
                strcat(VID_Msg, "(ATSC)");
                break;
            case STVID_BROADCAST_DVD:
                strcat(VID_Msg, "(DVD)");
                break;
            default:
                strcat(VID_Msg, "Unknown!\n");
                break;
        } /* end switch */
        sprintf(VID_Msg, "%s decodeonce=%d\n",
                 VID_Msg, StartParams.DecodeOnce );
        VID_PRINT((VID_Msg ));

    } /* end if */

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_Start */

/*-------------------------------------------------------------------------
 * Function : VID_Step
 *            Displays next field when paused or freezed
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_Step(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_Step(handle=0x%x) :",
                 CurrentHandle );

        ErrCode = STVID_Step(CurrentHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case STVID_ERROR_DECODER_STOPPED:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is stopped !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is still running !\n" );
                break;
            case STVID_ERROR_DECODER_NOT_PAUSING :
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is not pausing !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED:
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_Step() */

/*-------------------------------------------------------------------------
 * Function : VID_Stop
 *            Stop decoding and display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_Stop(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_Stop_t StopMode;
    STVID_Freeze_t Freeze;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    char ItemModeNo[80];
    S32 LVar;

    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;
    /* default modes */
    StopMode = STVID_STOP_NOW;
    Freeze.Mode = STVID_FREEZE_MODE_FORCE;
    Freeze.Field = STVID_FREEZE_FIELD_TOP;

    /* --- get Handle --- */

    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    /* --- get stop mode --- */

    if (!(RetErr))
    {
        RetErr = STTST_GetItem(pars_p, "", ItemModeNo, sizeof(ItemModeNo));
        if (strlen(ItemModeNo)>0)
        {
            RetErr = STTST_EvaluateInteger(ItemModeNo, &LVar, 10);
            if (RetErr)
            {
                RetErr = FALSE;
                if (strcmp(ItemModeNo,"STVID_STOP_WHEN_NEXT_REFERENCE")==0)
                {
                    StopMode = STVID_STOP_WHEN_NEXT_REFERENCE;
                }
                else if (strcmp(ItemModeNo,"STVID_STOP_WHEN_END_OF_DATA")==0)
                {
                    StopMode = STVID_STOP_WHEN_END_OF_DATA;
                }
                else if (strcmp(ItemModeNo,"STVID_STOP_NOW")==0)
                {
                    StopMode = STVID_STOP_NOW;
                }
                else if (strcmp(ItemModeNo,"STVID_STOP_WHEN_NEXT_I")==0)
                {
                    StopMode = STVID_STOP_WHEN_NEXT_I;
                }
                else
                {
                    sprintf(VID_Msg, "expected Stop Mode (%d=next ref,%d=end of data,%d=now, %d=next I)",
                        STVID_STOP_WHEN_NEXT_REFERENCE, STVID_STOP_WHEN_END_OF_DATA,
                        STVID_STOP_NOW, STVID_STOP_WHEN_NEXT_I);
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                    RetErr = TRUE;
                }
            }
            else
            {
                StopMode = LVar;
            }
        }
    }

    /* --- get freeze mode --- */

    if (!(RetErr))
    {
        RetErr = STTST_GetItem(pars_p, "", ItemModeNo, sizeof(ItemModeNo));
        if (strlen(ItemModeNo)>0)
        {
            RetErr = STTST_EvaluateInteger(ItemModeNo, &LVar, 10);
            if (RetErr)
            {
                RetErr = FALSE;
                if (strcmp(ItemModeNo,"STVID_FREEZE_MODE_NONE")==0)
                {
                    Freeze.Mode = STVID_FREEZE_MODE_NONE;
                }
                else if (strcmp(ItemModeNo,"STVID_FREEZE_MODE_FORCE")==0)
                {
                    Freeze.Mode = STVID_FREEZE_MODE_FORCE;
                }
                else if (strcmp(ItemModeNo,"STVID_STOP_NOW")==0)
                {
                    Freeze.Mode = STVID_STOP_NOW;
                }
                else if (strcmp(ItemModeNo,"STVID_FREEZE_MODE_NO_FLICKER")==0)
                {
                    Freeze.Mode = STVID_FREEZE_MODE_NO_FLICKER;
                }
                else
                {
                    sprintf(VID_Msg, "expected Freeze Mode (%d=none,%d=force,%d=no_flicker)",
                        STVID_FREEZE_MODE_NONE, STVID_FREEZE_MODE_FORCE,
                        STVID_FREEZE_MODE_NO_FLICKER);
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                    RetErr = TRUE;
                }
            }
            else
            {
                Freeze.Mode = LVar ;
            }
        }
    }

    /* --- get freeze field --- */

    if (!(RetErr))
    {
        RetErr = STTST_GetItem(pars_p, "", ItemModeNo, sizeof(ItemModeNo));
        if (strlen(ItemModeNo)>0)
        {
            RetErr = STTST_EvaluateInteger(ItemModeNo, &LVar, 10);
            if (RetErr)
            {
                RetErr = FALSE;
                if (strcmp(ItemModeNo,"STVID_FREEZE_FIELD_TOP")==0)
                {
                    Freeze.Field = STVID_FREEZE_FIELD_TOP;
                }
                else if (strcmp(ItemModeNo,"STVID_FREEZE_FIELD_BOTTOM")==0)
                {
                    Freeze.Field = STVID_FREEZE_FIELD_BOTTOM;
                }
                else if (strcmp(ItemModeNo,"STVID_FREEZE_FIELD_CURRENT")==0)
                {
                    Freeze.Field = STVID_FREEZE_FIELD_CURRENT;
                }
                else if (strcmp(ItemModeNo,"STVID_FREEZE_FIELD_NEXT")==0)
                {
                    Freeze.Field = STVID_FREEZE_FIELD_NEXT;
                }
                else
                {
                    sprintf(VID_Msg, "expected Freeze Field (%d=top,%d=bottom,%d=current,%d=next)",
                        STVID_FREEZE_FIELD_TOP, STVID_FREEZE_FIELD_BOTTOM,
                        STVID_FREEZE_FIELD_CURRENT, STVID_FREEZE_FIELD_NEXT );
                    STTST_TagCurrentLine(pars_p, VID_Msg );
                    RetErr = TRUE;
                }
            }
            else
            {
                Freeze.Field = LVar ;
            }
        }
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_Stop(handle=0x%x,stopmode=%d,&freeze=0x%x) :",
                 CurrentHandle, StopMode, (int)&Freeze );
        ErrCode = STVID_Stop(CurrentHandle, StopMode, &Freeze );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                /* Give info to injection task that the video is stopped for this handle */
                /* because VID_GetBufferFreeSize() return total freee size when stopped  */
                /* This means that the injection task is feeding the bit buffer.         */
                for(LVar=0;LVar<VIDEO_MAX_DEVICE;LVar++)
                {
                    if ( VID_Inj_GetDriverHandle(LVar) == CurrentHandle)
                        break;
                }
                if (LVar != VIDEO_MAX_DEVICE)
                {
                    VID_Inj_SetVideoDriverState(LVar, STATE_DRIVER_STOPPED);
                    if (StopMode==STVID_STOP_WHEN_END_OF_DATA)
                    {
                        VID_Inj_SetVideoDriverState(LVar, STATE_DRIVER_STOP_ENDOFDATA_ASKED);
                    }
                }
                break;
            case STVID_ERROR_DECODER_STOPPED:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder already stopped !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED:
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */

        /* More explicit trace */
        sprintf(VID_Msg, "%s   stopmode=%d ", VID_Msg, StopMode);
        switch(StopMode)
        {
            case STVID_STOP_WHEN_NEXT_REFERENCE :
                strcat(VID_Msg, "(STVID_STOP_WHEN_NEXT_REFERENCE)\n");
                break;
            case STVID_STOP_WHEN_END_OF_DATA :
                strcat(VID_Msg, "(STVID_STOP_WHEN_END_OF_DATA)\n");
                break;
            case STVID_STOP_NOW :
                strcat(VID_Msg, "(STVID_STOP_NOW)\n");
                break;
            case STVID_STOP_WHEN_NEXT_I :
                strcat(VID_Msg, "(STVID_STOP_WHEN_NEXT_I)\n");
                break;
            default:
                strcat(VID_Msg, " (UNDEFINED !)");
                break;
        } /* end switch */
        sprintf(VID_Msg, "%s   freeze.mode=%d ", VID_Msg, Freeze.Mode);
        switch(Freeze.Mode)
        {
            case STVID_FREEZE_MODE_NONE :
                strcat(VID_Msg, "(STVID_FREEZE_MODE_NONE)\n");
                break;
            case STVID_FREEZE_MODE_FORCE :
                strcat(VID_Msg, "(STVID_FREEZE_MODE_FORCE)\n");
                break;
            case STVID_FREEZE_MODE_NO_FLICKER :
                strcat(VID_Msg, "(STVID_FREEZE_MODE_NO_FLICKER)\n");
                break;
            default:
                strcat(VID_Msg, "(UNDEFINED !)\n");
                break;
        } /* end switch */
        sprintf(VID_Msg, "%s   freeze.field=%d ", VID_Msg, Freeze.Field);
        switch(Freeze.Field)
        {
            case STVID_FREEZE_FIELD_TOP :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_TOP)\n");
                break;
            case STVID_FREEZE_FIELD_BOTTOM :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_BOTTOM)\n");
                break;
            case STVID_FREEZE_FIELD_CURRENT :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_CURRENT)\n");
                break;
            case STVID_FREEZE_FIELD_NEXT :
                strcat(VID_Msg, "(STVID_FREEZE_FIELD_NEXT)\n");
                break;
            default:
                strcat(VID_Msg, "(UNDEFINED !)\n");
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_Stop() */

/*-------------------------------------------------------------------------
 * Function : VID_TakePictureBuffer
 *            Terminate the video decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_TakePictureBuffer(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_Handle_t CurrentHandle;
    STVID_PictureBufferHandle_t CurrentPictureBufferHandle;
    S32 LVar;
    BOOL RetErr;

    RetErr = FALSE;
    ErrCode = ST_NO_ERROR;

    /* --- get Handle --- */

    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    /* get PictureBufferHandle */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, STVID_PICTURE_STRUCTURE_FRAME, &LVar );
        CurrentPictureBufferHandle = (STVID_PictureBufferHandle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected PictureBufferHandle" );
        }
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_TakePictureBuffer(handle=0x%x,pictbufferhndl=0x%x) :",
                 CurrentHandle, (int)CurrentPictureBufferHandle );
        ErrCode = STVID_TakePictureBuffer(CurrentHandle, CurrentPictureBufferHandle );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end of VID_TakePictureBuffer() */

/*-------------------------------------------------------------------------
 * Function : VID_Term
 *            Terminate the video decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_Term(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    ST_DeviceName_t DeviceName;
    STVID_TermParams_t TermParams;
    S32 LVar;
    BOOL RetErr;

#if defined STVID_INJECTION_BREAK_WORKAROUND || !defined ST_OSLINUX
    ST_ErrorCode_t UnsubscribeErrorCode;
#endif

    ErrCode = ST_ERROR_BAD_PARAMETER;

    /* get DeviceName */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected DeviceName" );
    }
    /* get term param */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected ForceTerminate (TRUE, FALSE)" );
        }
        TermParams.ForceTerminate = (BOOL)LVar;
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf (VID_Msg, "STVID_Term(device=%s,forceterminate=%d) : ", DeviceName, TermParams.ForceTerminate );
        ErrCode = STVID_Term(DeviceName, &TermParams );

        switch(ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
#ifdef STVID_INJECTION_BREAK_WORKAROUND
                /* Unsubscribe to event stop & restore injection */
                UnsubscribeErrorCode = STEVT_UnsubscribeDeviceEvent(EvtSubHandle, DeviceName,
                                                                    (STEVT_EventConstant_t)
                                                                    STVID_WORKAROUND_PLEASE_STOP_INJECTION_EVT);
                UnsubscribeErrorCode |= STEVT_UnsubscribeDeviceEvent(EvtSubHandle, DeviceName,
                                                                     (STEVT_EventConstant_t)
                                                                    STVID_WORKAROUND_THANKS_INJECTION_CAN_GO_ON_EVT);
                if (UnsubscribeErrorCode != ST_NO_ERROR)
                {
                    STTBX_ErrorPrint(("Failed to unsubscribe workarounds injection !!\n" ));
                    API_ErrorCount++;
                    RetErr = TRUE;
                }
#endif /* STVID_INJECTION_BREAK_WORKAROUND */

#ifndef ST_OSLINUX /* 5508 & 5518 chip not supported for LINUX */

                /* Unsubscribe to event PROVIDED_DATA_TO_BE_INJECTED */
                UnsubscribeErrorCode = STEVT_UnsubscribeDeviceEvent(EvtSubHandle, DeviceName,
                                                                    (STEVT_EventConstant_t)
                                                                    STVID_PROVIDED_DATA_TO_BE_INJECTED_EVT);
                if (UnsubscribeErrorCode != ST_NO_ERROR)
                {
                    STTBX_ErrorPrint(("Failed to unsubscribe workarounds injection !!\n" ));
                    API_ErrorCount++;
                    RetErr = TRUE;
                }
#endif
                break;

            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;

            case ST_ERROR_UNKNOWN_DEVICE:
                API_ErrorCount++;
                strcat(VID_Msg, "the specified device is not initialized !\n" );
                break;

            case ST_ERROR_OPEN_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "Open handle error !\n");
                break;

            default :
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(RetErr == FALSE)
        {
            VID_PRINT((VID_Msg ));
        }
        else
        {
            VID_PRINT_ERROR((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end VID_Term */

#if defined ST_OSLINUX & defined DEBUG_LOCK
static BOOL VID_IntLockPrint(STTST_Parse_t *pars_p, char *result_sym_p)
{
    int IntLockI, IntLockJ;

    STAPIGAT_IntLockPrint(&IntLockPrint, IntLockInVar, IntLockOutVar);

    if (IntLockPrint)
    {
        printf("******* Lock / Unlock table ********\n");
        IntLockI=0;
        do
        {
            if ((strcmp(IntLockInVar[IntLockI].file,IntLockOutVar[IntLockI].file) != 0) || (IntLockI<10))
            {
                for (IntLockJ=-4;IntLockJ<4;IntLockJ++)
                {
                    if (((IntLockI+IntLockJ)>=0) && ((IntLockI+IntLockJ) < INT_LOCK_INDEX_MAX))
                    {
                        printf("IntLockInVar [%d]:%s:%d:%d\n", IntLockI+IntLockJ, IntLockInVar[IntLockI+IntLockJ].file, IntLockInVar[IntLockI+IntLockJ].location, IntLockInVar[IntLockI+IntLockJ].time);
                        printf("IntLockOutVar[%d]:%s:%d:%d\n", IntLockI+IntLockJ, IntLockOutVar[IntLockI+IntLockJ].file, IntLockOutVar[IntLockI+IntLockJ].location, IntLockOutVar[IntLockI+IntLockJ].time);
                    }
                }
                IntLockI+=4-1;
            }
            IntLockI++;
        } while (IntLockI<INT_LOCK_INDEX_MAX);

        printf("End of table\n");
        IntLockPrint = 0;
    }
    else
    {
        printf("-> Lock / Unlock table not ready, please redo later\n");
    }

    return(FALSE);

} /* End of VID_IntLockPrint() function. */
#endif

/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/


/*-------------------------------------------------------------------------
 * Function : VID_RegisterCmd
 *            Register video command (1 for each API function)
 * Input    :
 * Output   :
 * Return   : TRUE if success
 * ----------------------------------------------------------------------*/
BOOL VID_RegisterCmd (void)
{
    BOOL RetErr;
    U32  i;
    STEVT_OpenParams_t EvtParams;

    /* STVID register commands with help message  */
    /* (by alphabetic order of commands = display order) */

    RetErr = FALSE;

    RetErr |= STTST_RegisterCommand("VID_CLEAR",     VID_Clear,                 "[<Handle>] <ClearMode> <LumaPatternAddr> <LumaPatternSize>\n\t\t <ChromaPatternAddr> <ChromaPatternSize> Clears display & memory");
    RetErr |= STTST_RegisterCommand("VID_CLOSE",     VID_Close,                 "<Handle> Close");
    RetErr |= STTST_RegisterCommand("VID_CONFEVENT", VID_ConfigureEvent,        "[<Handle>]<Evt><Flag><DecodeToSkip> Configures event notify");
    RetErr |= STTST_RegisterCommand("VID_DATAINJCOMPL", VID_DataInjectionCompleted,  "[<Handle>]<TransfRtP> Signals the end of data injection");
#ifndef STVID_NO_DISPLAY
    RetErr |= STTST_RegisterCommand("VID_COLORKEY",  VID_EnableColorKey,        "<ViewportHndl> Enable the color key");
    RetErr |= STTST_RegisterCommand("VID_CLOSEVP",   VID_CloseViewPort,         "<ViewportHndl> Close viewport");
    RetErr |= STTST_RegisterCommand("VID_DISBORDERA",VID_DisableBorderAlpha,    "<ViewportHndl> disable viewport border alpha");
    RetErr |= STTST_RegisterCommand("VID_DISP",      VID_EnableOutputWindow,    "<ViewportHndl> Enable the display of the video layer");
    RetErr |= STTST_RegisterCommand("VID_ENABORDERA",VID_EnableBorderAlpha,     "<ViewportHndl> enable viewport border alpha");
    RetErr |= STTST_RegisterCommand("VID_GETCOLORKEY", VID_GetViewPortColorKey,  "<Handle> Get the viewport color key");
    RetErr |= STTST_RegisterCommand("VID_GETALIGN",  VID_GetAlignIOWindows,     "<ViewportHndl> Get the closiest window");
    RetErr |= STTST_RegisterCommand("VID_GETVPPSI",  VID_GetViewPortPSI,        "<ViewportHndl> Get Picture Structure Improvement parameters");
    RetErr |= STTST_RegisterCommand("VID_GETQUALITYOPTIMIZATIONS",  VID_GetViewPortQualityOptimizations,        "<ViewportHndl> Get Quality Optimizations parameters");
    RetErr |= STTST_RegisterCommand("VID_GETVPMODE", VID_GetViewPortSpecialMode,"<ViewportHndl> Get viewport special mode");
    RetErr |= STTST_RegisterCommand("VID_GETVPALP",  VID_GetViewPortAlpha,      "<ViewportHndl> Get Viewport alpha value");
    RetErr |= STTST_RegisterCommand("VID_HIDE",      VID_HidePicture,           "<ViewportHndl> Restore the display");
    RetErr |= STTST_RegisterCommand("VID_GETRATIO",  VID_GetDisplayARConversion,"<ViewportHndl> Get pel aspect ratio conversion");
    RetErr |= STTST_RegisterCommand("VID_GETIO",     VID_GetIOWindows,          "<ViewportHndl> Get IO windows size & position");
    RetErr |= STTST_RegisterCommand("VID_GETIN",     VID_GetInputWindowMode,    "<ViewportHndl> Get input window mode");
    RetErr |= STTST_RegisterCommand("VID_GETOUT",    VID_GetOutputWindowMode,   "<ViewportHndl> Get output window parameters");
    RetErr |= STTST_RegisterCommand("VID_DISFRAMERATE",VID_DisableFrameRateConversion,    "<Handle> Disable frame rate conversion");
#endif /*#ifndef STVID_NO_DISPLAY*/
    RetErr |= STTST_RegisterCommand("VID_DISPPBUFF", VID_DisplayPictureBuffer,  "<Handle><PictBuffHndl> Display a picture buffer");
    RetErr |= STTST_RegisterCommand("VID_GETDISPPICTINFO", VID_GetDisplayPictureInfo,  "<Handle><PictBuffHndl><DisplayPictInfos> Display picture information ");
    RetErr |= STTST_RegisterCommand("VID_DUPLICATE", VID_DuplicatePicture,      "Copy a picture");
    RetErr |= STTST_RegisterCommand("VID_ENAFRAMERATE", VID_EnableFrameRateConversion,    "<Handle> Enable frame rate conversion");
    RetErr |= STTST_RegisterCommand("VID_FORCEDF", VID_ForceDecimationFactor,   "<Handle> <Mode> <Factor> Forces the current decimation factor");
    RetErr |= STTST_RegisterCommand("VID_FREEZE",    VID_Freeze,                "[<Handle>] <Mode> <Field> Freeze the decoding");
    RetErr |= STTST_RegisterCommand("VID_GETBBFSIZE", VID_GetBitBufferFreeSize, "<Handle> Get bit buffer free size");
    RetErr |= STTST_RegisterCommand("VID_GETBBFPARAMS", VID_GetBitBufferParams, "<Handle> Get bit buffer Params");
    RetErr |= STTST_RegisterCommand("VID_GETCAPA",   VID_GetCapability,         "<DeviceName> Get driver's capability");
    RetErr |= STTST_RegisterCommand("VID_GETDECIM",     VID_GetDecimationFactor,   "<Handle> Get the current decimation factor");
    RetErr |= STTST_RegisterCommand("VID_GETDECODP", VID_GetDecodedPictures,    "<Handle> Get decoded pictures type");
    RetErr |= STTST_RegisterCommand("VID_GETERR",    VID_GetErrorRecoveryMode,  "<Handle> Get error recovery mode");
    RetErr |= STTST_RegisterCommand("VID_GETHDPIP",  VID_GetHDPIPParams,        "<Handle> Get HDPIP Params");
    RetErr |= STTST_RegisterCommand("VID_GETPALLOCP",VID_GetPictureAllocParams, "<Handle> Get params to allocate space to picture");
    RetErr |= STTST_RegisterCommand("VID_GETPPARAM", VID_GetPictureParams,      "[<Handle>]<PictType> Get picture parameters");
    #ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
    RetErr |= STTST_RegisterCommand("VID_GETVIDDISPARAMS", VID_GetVideoDisplayParams,      "[<Handle>]<OptionMask> Get display parameters");
    #endif
    RetErr |= STTST_RegisterCommand("VID_GETPINFO",  VID_GetPictureInfos,      "[<Handle>]<PictType> Get picture parameters");
    RetErr |= STTST_RegisterCommand("VID_GETPBUFF",  VID_GetPictureBuffer,      "[<Handle>]<Struct><TopFieldFirst><ExpectingSecondField>\n\t\t<ExtendedTemporalRef><Width><Heigth> Retrieves a picture buffer");
    RetErr |= STTST_RegisterCommand("VID_GETMEMPROF",VID_GetMemoryProfile,      "<Handle> Get memory profile");
#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
    RetErr |= STTST_RegisterCommand("VID_GETRQSTDEIMODE",  VID_GetRequestedDeinterlacingMode,      "<ViewportHndl> Get requested Deinterlacing mode");
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */
    RetErr |= STTST_RegisterCommand("VID_GETSPEED",  VID_GetSpeed,              "<Handle> Get the speed");
    RetErr |= STTST_RegisterCommand("VID_GETSTAT",  VID_GetStatistics,          "<DeviceName> Get driver's statistics");
    RetErr |= STTST_RegisterCommand("VID_GETPBSTAT", VID_GetStatisticsProblems, "<DeviceName> <ClkrvName> <string mask> Get driver's statistics problems");
    RetErr |= STTST_RegisterCommand("VID_GETSTATUS",  VID_GetStatus,          "<DeviceName> Get driver's status");
#ifdef ST_MasterDigInput
    #if defined(ST_7100) || defined (ST_7109) || defined (ST_7200) || defined (ST_ZEUS)
        /* Warning: Keep the comment under 509 characters to comply with ISO C89 standard */
        RetErr |= STTST_RegisterCommand("VID_INIT", VID_Init,  " <DeviceName><\"V|DMPEG\"><VIDDev#><EvtDevName><ClkDevName> Init for Mpeg\n\t\t<DeviceName><\"H264\"><VIDDev#><EvtDevName><ClkDevName> Init for H264\n\t\t<DeviceName><\"VC1\"><VIDDev#><EvtDevName><ClkDevName> Init for VC1\n\t\t<DeviceName><\"MPEG4P2\"><VIDDev#><EvtDevName><ClkDevName> Init for MPEG4P2\n\t\t<DeviceName><\"AVS\"><VIDDev#><EvtDevName><ClkDevName> Init for Avs\n\t\t<DeviceName><\"D\"><1=SD1|2=SD2|3=HD0><EvtDevName><ClkDevName> Init for video with D1");
    #else
        RetErr |= STTST_RegisterCommand("VID_INIT", VID_Init,  "<DeviceName><\"V|DMPEG\"><VIDDev#><EvtDevName><ClkDevName> Initialization for video Mpeg\n\t\t<DeviceName><\"D\"><1=SD1|2=SD2|3=HD0><EvtDevName><ClkDevName> Initialization for video with D1");
    #endif
#else
    RetErr |= STTST_RegisterCommand("VID_INIT",     VID_Init,   "<DeviceName><\"V\"><VIDDev#><EvtDevName><ClkDevName><BB-allocated><BB-address><BB-size> Initialization for video\n\t\t<DeviceName><\"D\"><1=SD1|2=SD2|3=HD0><VINDevName><EvtDevName><ClkDevName> Initialization for video with D1");
#endif /* ST_MasterDigInput */
    RetErr |= STTST_RegisterCommand("VID_INJDISCONT", VID_InjectDiscontinuity,  "<Handle> Informs the driver that a discontinuity will occur");
    RetErr |= STTST_RegisterCommand("VID_OPEN",      VID_Open,                  "<DeviceName><SyncDelay> Share physical decoder usage");
#ifndef STVID_NO_DISPLAY
    RetErr |= STTST_RegisterCommand("VID_NOCOLORKEY",VID_DisableColorKey,       "<ViewportHndl> Disable the color key");
    RetErr |= STTST_RegisterCommand("VID_NODISP",    VID_DisableOutputWindow,   "<ViewportHndl> Disable the display of the video layer");
    RetErr |= STTST_RegisterCommand("VID_OPENVP",    VID_OpenViewPort,          "[<Handle>]<Layer#> Open a viewport");
    RetErr |= STTST_RegisterCommand("VID_SETIN",     VID_SetInputWindowMode,    "[<ViewportHndl>]<AutoMode><Align> Set input window mode");
    RetErr |= STTST_RegisterCommand("VID_SETIO",     VID_SetIOWindows,
              "[<ViewportHndl>]<InX><InY><InWidth><InHeight><OutX><OutY>\n\t\t<OutWidth><OutHeight> Set IO window size & position" );
    RetErr |= STTST_RegisterCommand("VID_SETQUALITYOPTIMIZATIONS",  VID_SetViewPortQualityOptimizations,        "<ViewportHndl> <DoForceStartOnEvenLine> <DoNotRescaleForZoomCloseToUnity> Set Quality Optimizations parameters");
    RetErr |= STTST_RegisterCommand("VID_SETVPPSI",  VID_SetViewPortPSI,        "<ViewportHndl> Set Picture Structure Improvement parameters");
    RetErr |= STTST_RegisterCommand("VID_SETVPMODE", VID_SetViewPortSpecialMode,"<ViewportHndl><Mode><Effect><NbZone><Zones...> Set viewport special mode");
    RetErr |= STTST_RegisterCommand("VID_SETVPALP",  VID_SetViewPortAlpha,      "<ViewportHndl> Set Viewport alpha value");
    RetErr |= STTST_RegisterCommand("VID_SETCOLORKEY", VID_SetViewPortColorKey, "<Handle> Set the viewport color key");
#endif /*#ifndef STVID_NO_DISPLAY*/
    RetErr |= STTST_RegisterCommand("VID_NOSYNC",    VID_DisableSynchronisation,"<Handle> Disable the video synchronisation with PCR");
    RetErr |= STTST_RegisterCommand("VID_PAUSE",     VID_Pause,                 "<Handle> <Mode> <Field> Pause the decoding");
    RetErr |= STTST_RegisterCommand("VID_PAUSESYNC", VID_PauseSynchro,          "[<Handle>]<Time> Pause the decoding for synchro. purpose");
    RetErr |= STTST_RegisterCommand("VID_RELPBUFF",  VID_ReleasePictureBuffer,  "[<Handle>]<PictBuffHandle> Release the picture buffer");
    RetErr |= STTST_RegisterCommand("VID_RESETSTAT", VID_ResetAllStatistics,    "<DeviceName> Reset all driver's statistics");
#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
    RetErr |= STTST_RegisterCommand("VID_RQSTDEIMODE",  VID_SetRequestedDeinterlacingMode,      "<ViewportHndl> <Mode> Set requested Deinterlacing mode");
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */
    RetErr |= STTST_RegisterCommand("VID_RESUME",    VID_Resume,                "[<Handle>] Resume a previously paused decoding");
    RetErr |= STTST_RegisterCommand("VID_REV",       VID_GetRevision,           "Get driver revision number");
    RetErr |= STTST_RegisterCommand("VID_SETDECODP", VID_SetDecodedPictures,    "[<Handle>]<Type> Set decoded pictures type");
    RetErr |= STTST_RegisterCommand("VID_DEBLOCK",   VID_EnableDeblocking,      "[<Handle>] enable deblocking");
    RetErr |= STTST_RegisterCommand("VID_NODEBLOCK", VID_DisableDeblocking,     "[<Handle>] disable deblocking");
#ifdef VIDEO_DEBLOCK_DEBUG
    RetErr |= STTST_RegisterCommand("VID_SETDEBLOCK", VID_SetDeblockingStrength,"[<Handle>] <Strength> set deblocking Strength");
#endif /* VIDEO_DEBLOCK_DEBUG */
    RetErr |= STTST_RegisterCommand("VID_SETERR",    VID_SetErrorRecoveryMode,  "[<Handle>]<Mode> Set behavior regarding errors");
    RetErr |= STTST_RegisterCommand("VID_SETHDPIP",  VID_SetHDPIPParams,        "[<Handle>]<Enable><Width><Height> Set HDPIP parameters");
    RetErr |= STTST_RegisterCommand("VID_SETMEMPROF", VID_SetMemoryProfile,
              "[<Handle>]<MaxWidth><MaxHeight><NbFrameStore><Compression><Decimation> Set memory profile\n\t\t[<Handle>]<MaxWidth><MaxHeight><80><Compression><0><NbMainFrameStore> Set memory profile optimized\n\t\t[<Handle>]<MaxWidth><MaxHeight><80><Compression><Decimation><NbMainFrameStore><NbDecimFrameStore> Set memory profile optimized with decimation\n\t\t[<Handle>]<MaxWidth><MaxHeight><81><Compression><Decimation><MainMaxTotalSize><DecimMaxTotalSize> Set memory profile dynamic with decimation");
#ifndef STVID_NO_DISPLAY
    RetErr |= STTST_RegisterCommand("VID_SETOUT",    VID_SetOutputWindowMode,   "[<ViewportHndl>]<Auto><Align> Set output window parameters");
    RetErr |= STTST_RegisterCommand("VID_SETRATIO",  VID_SetDisplayARConversion,
              "[<ViewportHdnl>]<RatioCv> Set ratio conv. between stream and TV");
    RetErr |= STTST_RegisterCommand("VID_SHOW",      VID_ShowPicture,           "[<ViewportHndl>]<PictureNb> Display the specific picture\n" );
#endif /*#ifndef STVID_NO_DISPLAY*/
    RetErr |= STTST_RegisterCommand("VID_SETUP",     VID_Setup,                 "[<Handle>]<Object><Avmem Partition> Setups objects required by the video driver");
#if defined(DVD_SECURED_CHIP)
    RetErr |= STTST_RegisterCommand("VID_SETUPWAGNBvd42332", VID_SetupH264PreprocWA_GNBvd42332, "[<Handle>]<Avmem Partition> Setups objects required by the H264 preproc WA GNBvd42332");
#endif /* DVD_SECURED_CHIP */
    RetErr |= STTST_RegisterCommand("VID_SETSPEED",  VID_SetSpeed,              "[<Handle>]<Speed> Set the speed");

    RetErr |= STTST_RegisterCommand("VID_SKIPSYNC",  VID_SkipSynchro,           "[<Handle>]<Time> Skip pictures");
    RetErr |= STTST_RegisterCommand("VID_START",     VID_Start,
              "[<Handle>]<RealTime><UpdateDisplay><StreamType><StreamID><Broadcast><DecodeOnce>\n\t\t Start the decoding of input stream");
    RetErr |= STTST_RegisterCommand("VID_STEP",      VID_Step,                  "[<Handle>] Displays next field when paused or freezed");
    RetErr |= STTST_RegisterCommand("VID_STOP",      VID_Stop,
              "[<Handle>]<StopMode><Field><Mode> Stop decoding the input stream");
    RetErr |= STTST_RegisterCommand("VID_SYNC",      VID_EnableSynchronisation, "<Handle> Enable video synchro with PCR");
    RetErr |= STTST_RegisterCommand("VID_TAKPBUFF",  VID_TakePictureBuffer,     "[<Handle>]<PictBuffHandle> Take a picture buffer");
    RetErr |= STTST_RegisterCommand("VID_TERM",      VID_Term,                  "<DeviceName><ForceTerminate> Terminate");

#if defined ST_OSLINUX & defined DEBUG_LOCK
    RetErr |= STTST_RegisterCommand("VID_INTLOCK",   VID_IntLockPrint,          "Collects recorded data from kernel lock/unlock and prints them");
#endif

#ifdef ST_XVP_ENABLE_FLEXVP
    RetErr |= STTST_RegisterCommand("VID_ACTIPROC",  VID_ActivateXVPProcess,    "<ViewportHndl><ProcessName> Activate an XVP process");
    RetErr |= STTST_RegisterCommand("VID_DEACPROC",  VID_DeactivateXVPProcess,  "<ViewportHndl><ProcessName> Deactivate an XVP process");
    RetErr |= STTST_RegisterCommand("VID_SHOWPROC",  VID_ShowXVPProcess,        "<ViewportHndl><ProcessName> Show an XVP process");
    RetErr |= STTST_RegisterCommand("VID_HIDEPROC",  VID_HideXVPProcess,        "<ViewportHndl><ProcessName> Hide an XVP process");

#endif /* ST_XVP_ENABLE_FLEXVP */

    /* --- Constants --- */

    RetErr |= STTST_AssignInteger("PAL",  0, TRUE);
    RetErr |= STTST_AssignInteger("NTSC", 1, TRUE);
    RetErr |= STTST_AssignInteger("STTES",   STVID_STREAM_TYPE_ES, TRUE);
    RetErr |= STTST_AssignInteger("STTPES",  STVID_STREAM_TYPE_PES, TRUE);
    RetErr |= STTST_AssignInteger("STTMP1P", STVID_STREAM_TYPE_MPEG1_PACKET, TRUE);

    RetErr |= STTST_AssignInteger("AR169",   STVID_DISPLAY_ASPECT_RATIO_16TO9, TRUE);
    RetErr |= STTST_AssignInteger("AR43",    STVID_DISPLAY_ASPECT_RATIO_4TO3, TRUE);
    RetErr |= STTST_AssignInteger("AR2211",  STVID_DISPLAY_ASPECT_RATIO_221TO1, TRUE);
    RetErr |= STTST_AssignInteger("ARSQ",    STVID_DISPLAY_ASPECT_RATIO_SQUARE, TRUE);

    RetErr |= STTST_AssignInteger("ARCONV_PS", STVID_DISPLAY_AR_CONVERSION_PAN_SCAN, TRUE);
    RetErr |= STTST_AssignInteger("ARCONV_LB", STVID_DISPLAY_AR_CONVERSION_LETTER_BOX, TRUE);
    RetErr |= STTST_AssignInteger("ARCONV_CO", STVID_DISPLAY_AR_CONVERSION_COMBINED, TRUE);
    RetErr |= STTST_AssignInteger("ARCONV_IG", STVID_DISPLAY_AR_CONVERSION_IGNORE, TRUE);

    /* Drive mapping depends on the Operating System ; use the exact lower/uppercase on Linux */
#ifdef ST_OSLINUX
    RetErr |= STTST_AssignString("BISTRIMROOT",   "./files/gnx419/bitstream/", FALSE);
    RetErr |= STTST_AssignString("SDSARNOFFS",    "./files/gnx419/bitstream/mpeg2/es/DVB_Sarnoffs/525/", FALSE);
    RetErr |= STTST_AssignString("SDSIFSARNOFFS", "./files/gnx419/bitstream/mpeg2/es/DVB_Sarnoffs/525_HALF/", FALSE);
    RetErr |= STTST_AssignString("SDPALSARNOFFS", "./files/gnx419/bitstream/mpeg2/es/DVB_Sarnoffs/625/", FALSE);
#if defined(STVID_USE_SPLITTED_SCRIPTS)
    RetErr |= STTST_AssignString("PATH_SCRIPTS",  "./sscripts/vid/", FALSE);
    RetErr |= STTST_AssignString("PATH_FIRMWARES","./sscripts/vid/", FALSE);
    RetErr |= STTST_AssignString("PATH_TESTS",    "./sscripts/vid/", FALSE);
#else
    RetErr |= STTST_AssignString("PATH_SCRIPTS",  "./scripts/vid/", FALSE);
    RetErr |= STTST_AssignString("PATH_FIRMWARES","./scripts/vid/", FALSE);
    RetErr |= STTST_AssignString("PATH_TESTS",    "./scripts/vid/", FALSE);
#endif
#elif defined(ST_OSWINCE)
    RetErr |= STTST_AssignString("BISTRIMROOT",   "//gnx168/gnx1685/", FALSE);
    RetErr |= STTST_AssignString("SDSARNOFFS",    "//gnx419/bitstream$/bitstream/mpeg2/es/DVB_Sarnoffs/525/", FALSE);
    RetErr |= STTST_AssignString("SDSIFSARNOFFS", "//gnx419/bitstream$/bitstream/mpeg2/es/DVB_Sarnoffs/525_HALF/", FALSE);
    RetErr |= STTST_AssignString("SDPALSARNOFFS", "//gnx419/bitstream$/bitstream/mpeg2/es/DVB_Sarnoffs/625/", FALSE);
    RetErr |= STTST_AssignString("PATH_SCRIPTS",  "../../../scripts/", FALSE);
    RetErr |= STTST_AssignString("PATH_FIRMWARES","../../../../video_firmware/", FALSE);
    RetErr |= STTST_AssignString("PATH_TESTS",    "../../", FALSE);
#else
    RetErr |= STTST_AssignString("BISTRIMROOT",   "//gnx419/bitstream$/bitstream/", FALSE);
    RetErr |= STTST_AssignString("SDSARNOFFS",    "//gnx419/bitstream$/bitstream/mpeg2/es/DVB_Sarnoffs/525/", FALSE);
    RetErr |= STTST_AssignString("SDSIFSARNOFFS", "//gnx419/bitstream$/bitstream/mpeg2/es/DVB_Sarnoffs/525_HALF/", FALSE);
    RetErr |= STTST_AssignString("SDPALSARNOFFS", "//gnx419/bitstream$/bitstream/mpeg2/es/DVB_Sarnoffs/625/", FALSE);
#if defined(STVID_USE_SPLITTED_SCRIPTS)
    RetErr |= STTST_AssignString("PATH_SCRIPTS",  "../../../sscripts/", FALSE);
#else
    RetErr |= STTST_AssignString("PATH_SCRIPTS",  "../../../scripts/", FALSE);
#endif /* defined(STVID_USE_SPLITTED_SCRIPTS) */
    RetErr |= STTST_AssignString("PATH_FIRMWARES","../../../../video_firmware/", FALSE);
    RetErr |= STTST_AssignString("PATH_TESTS",    "../../", FALSE);
#endif
    /***RetErr |= STTST_AssignInteger("ERR_NO_ERROR", ST_NO_ERROR, TRUE);
    RetErr |= STTST_AssignInteger("ERR_INVALID_HANDLE", ST_ERROR_INVALID_HANDLE, TRUE);***/

    /* --- Variables & returned values --- */
    RetErr |= STTST_AssignInteger("TRANSCODE",  0, FALSE);

    RetErr |= STTST_AssignInteger("TEST_MODE",  1, FALSE); /* default is multi-handle for macros */
    API_TestMode = TEST_MODE_MULTI_HANDLES ;

#ifdef STVID_DIRECTV
    RetErr |= STTST_AssignInteger("MODE",  1, FALSE); /* default is NTSC */
#else
    RetErr |= STTST_AssignInteger("MODE",  0, FALSE); /* default is PAL */
#endif

    RetErr |= STTST_AssignInteger("RET_VAL1",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL2",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL3",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL4",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL5",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL6",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL7",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL8",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL9",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL10",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL11",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL12",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL13",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL14",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL15",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL16",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL17",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL18",  0, FALSE);

    RetErr |= STTST_AssignInteger("HDLVID1",    0, FALSE); /* video handle */
    RetErr |= STTST_AssignInteger("HDLVID2",    0, FALSE);
    RetErr |= STTST_AssignInteger("HDLVP1VID1",  0, FALSE); /* viewport handle */
    RetErr |= STTST_AssignInteger("HDLVP2VID1",  0, FALSE);
    RetErr |= STTST_AssignInteger("HDLVP1VID2",  0, FALSE); /* viewport handle */
    RetErr |= STTST_AssignInteger("HDLVP2VID2",  0, FALSE);
    RetErr |= STTST_AssignInteger("HDLDENC",    0, FALSE); /* denc handle */
    RetErr |= STTST_AssignInteger("HDLDENC2",    0, FALSE); /* denc handle */
    RetErr |= STTST_AssignInteger("HDLVTGMAIN", 0, FALSE); /* VTG main output handle */
    RetErr |= STTST_AssignInteger("HDLVTGAUX",  0, FALSE); /* VTG aux output handle */
    RetErr |= STTST_AssignInteger("HDLVMIX1",   0, FALSE); /* VMIX1 handle */
    RetErr |= STTST_AssignInteger("HDLVMIX2",   0, FALSE); /* VMIX2 handle */
    RetErr |= STTST_AssignInteger("HDLVOUT",    0, FALSE); /* SD output handle */
    RetErr |= STTST_AssignInteger("HDLVOUT2",    0, FALSE); /* SD output 2 handle */
    RetErr |= STTST_AssignInteger("HDLVOUTDIG", 0, FALSE); /* digital HD output handle */
    RetErr |= STTST_AssignInteger("HDLVOUTHDANA", 0, FALSE); /* analog HD output handle */
    RetErr |= STTST_AssignInteger("HDLE7114",   0, FALSE); /* digital input interface (EXTVIN) handle */
    RetErr |= STTST_AssignInteger("HDLVIN0",    0, FALSE); /* digital input 1 handle */
    RetErr |= STTST_AssignInteger("HDLVIN1",    0, FALSE); /* digital input 2 handle */
    RetErr |= STTST_AssignInteger("MEM_PROF_BUFFER_NB", 3, FALSE); /* profile : nb of frame buffers */
#if defined STVID_USE_ENCODER
    RetErr |= STTST_AssignInteger("HDLTRA1",     0, FALSE);
    RetErr |= STTST_AssignInteger("HDLTRA1S",    0, FALSE);
    RetErr |= STTST_AssignInteger("HDLTRA1T",    0, FALSE);
#endif /* STVID_USE_ENCODER */

#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
    #ifdef ST_OSLINUX
        /* Only index is passed. Conversion is done in kstvid */
        RetErr |= STTST_AssignInteger("LMI_SYS_AVMEM_HANDLE", 0, TRUE);
        RetErr |= STTST_AssignInteger("LMI_VID_AVMEM_HANDLE", 1, TRUE);
        #if defined(DVD_SECURED_CHIP)
            RetErr |= STTST_AssignInteger("NON_SECURE_AVMEM_HANDLE", 2, TRUE);
            RetErr |= STTST_AssignInteger("SECURE_AVMEM_HANDLE", 3, TRUE);
            /*  For linux, need for changes in test modules to modify AVMEM partitions settings, so the following partition will not be used for now */
            /*    RetErr |= STTST_AssignInteger("LMI_MAIN_BUFFERS_HANDLE", 3, TRUE);*/
            /*    RetErr |= STTST_AssignInteger("LMI_DECIM_BUFFERS_HANDLE", 4, TRUE);*/
        #else
            /*  For linux, need for changes in test modules to modify AVMEM partitions settings, so the following partition will not be used for now */
            /*    RetErr |= STTST_AssignInteger("LMI_MAIN_BUFFERS_HANDLE", 2, TRUE);*/
            /*    RetErr |= STTST_AssignInteger("LMI_DECIM_BUFFERS_HANDLE", 3, TRUE);*/
        #endif /* DVD_SECURED_CHIP */
    #else /* ST_OSLINUX */
        RetErr |= STTST_AssignInteger("LMI_SYS_AVMEM_HANDLE", AvmemPartitionHandle[0], TRUE);
        RetErr |= STTST_AssignInteger("LMI_VID_AVMEM_HANDLE", AvmemPartitionHandle[1], TRUE);
        #if defined(DVD_SECURED_CHIP)
            #if defined(ST_OSWINCE)
                RetErr |= STTST_AssignInteger("NON_SECURE_AVMEM_HANDLE", AvmemPartitionHandle[0], TRUE);
                RetErr |= STTST_AssignInteger("SECURE_AVMEM_HANDLE", AvmemPartitionHandle[2], TRUE);
                RetErr |= STTST_AssignInteger("RESERVED_AVMEM_HANDLE", AvmemPartitionHandle[3], TRUE);
            #else
                /* Same mapping in OS21 as for WinCE */
                RetErr |= STTST_AssignInteger("NON_SECURE_AVMEM_HANDLE", AvmemPartitionHandle[0], TRUE);
                RetErr |= STTST_AssignInteger("SECURE_AVMEM_HANDLE", AvmemPartitionHandle[2], TRUE);
                RetErr |= STTST_AssignInteger("RESERVED_AVMEM_HANDLE", AvmemPartitionHandle[3], TRUE);
            #endif /* ST_OSWINCE */
            /*    RetErr |= STTST_AssignInteger("LMI_MAIN_BUFFERS_HANDLE", AvmemPartitionHandle[3], TRUE);*/
            /*    RetErr |= STTST_AssignInteger("LMI_DECIM_BUFFERS_HANDLE", AvmemPartitionHandle[4], TRUE);*/
        #else /* DVD_SECURED_CHIP */
            /*    RetErr |= STTST_AssignInteger("LMI_MAIN_BUFFERS_HANDLE", AvmemPartitionHandle[2], TRUE);*/
            /*    RetErr |= STTST_AssignInteger("LMI_DECIM_BUFFERS_HANDLE", AvmemPartitionHandle[3], TRUE);*/
        #endif /* DVD_SECURED_CHIP */
    #endif /* ST_OSLINUX */
#elif defined (ST_ZEUS)
    RetErr |= STTST_AssignInteger("LMI_SYS_AVMEM_HANDLE",  AvmemPartitionHandle[0], TRUE);
    RetErr |= STTST_AssignInteger("LMI_VID1_AVMEM_HANDLE", AvmemPartitionHandle[1], TRUE);
    RetErr |= STTST_AssignInteger("LMI_VID2_AVMEM_HANDLE", AvmemPartitionHandle[2], TRUE);
#elif defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    /* One partition on emulation platform */
    RetErr |= STTST_AssignInteger("LMI_SYS_AVMEM_HANDLE", AvmemPartitionHandle[0], TRUE);
    RetErr |= STTST_AssignInteger("LMI_VID_AVMEM_HANDLE", AvmemPartitionHandle[0], TRUE);
#endif

    /* Assign constant values to STVID_Setup objects */
    RetErr |= STTST_AssignInteger("FRAME_BUFFERS_PARTITION",                STVID_SETUP_FRAME_BUFFERS_PARTITION, TRUE);
    RetErr |= STTST_AssignInteger("DECODER_INTERMEDIATE_BUFFER_PARTITION",  STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION, TRUE);
    RetErr |= STTST_AssignInteger("DECIMATED_FRAME_BUFFERS_PARTITION",      STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION, TRUE);
    RetErr |= STTST_AssignInteger("FDMA_NODES_PARTITION",                   STVID_SETUP_FDMA_NODES_PARTITION, TRUE);
    RetErr |= STTST_AssignInteger("PICTURE_PARAMETER_BUFFERS_PARTITION",    STVID_SETUP_PICTURE_PARAMETER_BUFFERS_PARTITION, TRUE);
    RetErr |= STTST_AssignInteger("ES_COPY_BUFFER_PARTITION",               STVID_SETUP_ES_COPY_BUFFER_PARTITION, TRUE);
    RetErr |= STTST_AssignInteger("BUFFER_ES_COPY_BUFFER",                  STVID_SETUP_ES_COPY_BUFFER, TRUE);
    RetErr |= STTST_AssignInteger("SCLIST_PARTITION",                       STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION, TRUE);
    RetErr |= STTST_AssignInteger("PES_BUFFER_PARTITION",                   STVID_SETUP_DATA_INPUT_BUFFER_PARTITION, TRUE);
    RetErr |= STTST_AssignInteger("BIT_BUFFER_PARTITION",                   STVID_SETUP_BIT_BUFFER_PARTITION, TRUE);

#ifdef VALID_TOOLS
    if (STVID_RegisterTraces() != ST_NO_ERROR)
    {
        STTBX_ErrorPrint(("Failed register conditional traces !!\n"));
        RetErr =TRUE;
    }
#endif /* VALID_TOOLS */

#ifdef STUB_INJECT
    STTST_AssignInteger("VID_BBADDRESS", 0, FALSE);
    STTST_AssignInteger("VID_BBSIZE", 0, FALSE);
    STTST_AssignInteger("VID_STREAMSIZE", 0, FALSE);
#endif /* STUB_INJECT */

#if defined(STVID_USE_SPLITTED_SCRIPTS)
    STTST_AssignInteger("VID_SSCRIPTS", 1, FALSE);
#else
    STTST_AssignInteger("VID_SSCRIPTS", 0, FALSE);
#endif /* defined(STVID_USE_SPLITTED_SCRIPTS) */


    /* Init global structures */
    for (i=0; i < VIDEO_MAX_DEVICE; i++)
    {
        VID_Inj_ResetDeviceAndHandle(i);
    }

    if (STEVT_Open(STEVT_DEVICE_NAME, &EvtParams, &EvtSubHandle) != ST_NO_ERROR)
    {
        STTBX_ErrorPrint(("Failed to open handle on event device !!\n"));
        RetErr =TRUE;
    }

    if (RetErr)
    {
        STTST_Print(("VID_RegisterCmd()     : failed !\n" ));
    }
    else
    {
        STTST_Print(("VID_RegisterCmd()     : ok\n" ));
    }
    return(RetErr ? FALSE : TRUE);

} /* end VID_RegisterCmd */


/*#######################################################################*/


