/*******************************************************************************

File name   : vid_disp.c

Description : Video display source file. Task and callbacks.

    viddisp_DisplayTaskFunc
    viddisp_NewPictureToBeDisplayed
    viddisp_PictureCandidateToBeRemovedFromDisplayQueue
    viddisp_VSyncDisplayMain
    viddisp_VSyncDisplayAux

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
14 Mar 2000        Created                                           HG
01 Feb 2001        Digital Input merge                               JA
09 Mar 2001        New VIDDISP_SetReconstructionMode function and
                   ReconstructionMode management.                    GG
16 Mar 2001        Display Event notification configuration.         GG
03 Sep 2001        Progressive VSync management.                     GG
   Sep 2002        Dual Display, all new                             MB

<---------------------------- please keep 80 col ------------------------------>

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Option for Debug only: Check that every received picture has an associated decimated picture (when decimation enabled) */
/*#define DBG_CHECK_ASSOCIATED_DECIMATED_PICTURE */

/* #define PRESERVE_FIELD_POLARITY_ON_SLAVE_DISPLAY */

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#include <assert.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "stevt.h"
#include "stvtg.h"

#include "vid_disp.h"
#include "buffers.h"
#include "halv_dis.h"
#include "queue.h"
#include "fields.h"

#ifdef ST_ordqueue
#include "ordqueue.h"
#endif /* ST_ordqueue */

#ifdef ST_crc
#include "crc.h"
#include "stdevice.h"
#include "stsys.h"
#endif /* ST_crc */

#ifdef ST_XVP_ENABLE_FGT
#include "vid_fgt.h"
#endif /* ST_XVP_ENABLE_FGT */

/* Trace System activation ---------------------------------------------- */

/* Enable this if you want to see the Traces... */
/* #define TRACE_UART */

/* Select the Traces you want */
#define TrMain                  TraceOff
#define TrLock                  TraceOff
#define TrField                 TraceOff
#define TrVSync                 TraceOn
#define TrPictPrepared          TraceOff
#define TrPictureId             TraceOff
#define TrStop                  TraceOff
#define TrFRConversion          TraceOff

#define TrWarning               TraceOn
/* TrError is now defined in vid_trace.h */

/* "vid_trace.h" should be included AFTER the definition of the traces wanted */
#include "vid_trace.h"

/* Traces : ----------------------------------------------------------------- */
#ifdef STFAE
#include "stapp_trace.h"
#define TraceMessage TRACE_Message
#define TraceState   TRACE_State
#define TraceVal     TRACE_Val
#endif

/* Enable this to activate the function checking that the field order is correct (no chronology issue) */
/*#define DBG_FIELD_PRESENTATION_ORDER*/

/* #define TRACE_DISPLAY */
/* #define TRACE_DISPLAY_VERBOSE */

/* Traces for Visual Debugger */
/* #define TRACE_DISPLAY_FRAME_BUFFER */
/* #define TRACE_DISPLAY_PICTURE_TYPE */
/* #define TRACE_DISPLAY_VSYNC */
/* #define TRACE_VISUAL_UART */

#if defined (TRACE_VISUAL_UART)
#define VISUAL_UART
#endif /* TRACE_VISUAL_UART */

#ifdef TRACE_DISPLAY_PICTURE_TYPE
#define TracePictureType(x) TraceMessage x
#else
#define TracePictureType(x)
#endif /* TRACE_DISPLAY_PICTURE_TYPE */

#ifdef TRACE_DISPLAY_FRAME_BUFFER
#define TraceFrameBuffer(x) TraceState x
#else
#define TraceFrameBuffer(x)
#endif /* TRACE_DISPLAY_FRAME_BUFFER */

#ifdef TRACE_DISPLAY_VSYNC
#define TraceVsync(x) TraceState x
#else
#define TraceVsync(x)
#endif /* TRACE_DISPLAY_VSYNC */

/* Usual traces */
/* #define TRACE_DISPLAY */
/* define or uncomment in ./makefile to set traces */

#if defined TRACE_DISPLAY || defined(TRACE_VISUAL_UART)
  #ifdef TRACE_DISPLAY_VERBOSE
    #define TraceVerbose(x) TraceBuffer(x)
  #else
    #define TraceVerbose(x)
  #endif
  #define TraceDisplay(x) TraceInfo(x)
#else
  #define TraceVerbose(x)
  #define TraceDisplay(x)
#endif

#if defined TRACE_DISPLAY || defined TRACE_DISPLAY_PICTURE_TYPE || defined TRACE_DISPLAY_FRAME_BUFFER || defined TRACE_DISPLAY_VSYNC || defined(TRACE_VISUAL_UART)
  #include "vid_trc.h"
#endif /* defined TRACE_DISPLAY || defined TRACE_DISPLAY_PICTURE_TYPE || defined TRACE_DISPLAY_FRAME_BUFFER || defined TRACE_DISPLAY_VSYNC || defined TRACE_VISUAL_UART */

/* Private Constants -------------------------------------------------------- */

/* VSync patterns */
#define MAX_VSYNC_PATTERN_VALUE             7
#define PROGRESSIVE_VSYNC_PATTERN_TOP       0
#define PROGRESSIVE_VSYNC_PATTERN_BOT       7

/* available display queue lock states */
#define UNLOCKED                            0
#define LOCK_NEXT                           1
#define LOCKED                              2

#define PUT_IN_MAIN_QUEUE       FALSE
#define PUT_IN_DECIMATED_QUEUE  TRUE

/* The Current DEI implemented on STi7109 support 720 pixels per line maximum */
#define MAX_DEI_PICTURE_WIDTH       720

/* Private Macros ----------------------------------------------------------- */

#define Virtual(DevAdd) (void*)((U32)(DevAdd)\
        -(U32)(VIDDISP_Data_p->VirtualMapping.PhysicalAddressSeenFromDevice_p)\
        +(U32)(VIDDISP_Data_p->VirtualMapping.VirtualBaseAddress_p))

#define SWAP_FIELD_TYPE(type)       ( (type == HALDISP_TOP_FIELD) ? HALDISP_BOTTOM_FIELD : HALDISP_TOP_FIELD)

#define FIELD_TYPE(Field)               (Field.FieldType == HALDISP_TOP_FIELD? "T": "B")

#define  MPEGFrame2Char(Type)  (                                        \
               (  (Type) == STVID_MPEG_FRAME_I)                         \
                ? 'I' : (  ((Type) == STVID_MPEG_FRAME_P)               \
                         ? 'P' : (  ((Type) == STVID_MPEG_FRAME_B)      \
                                  ? 'B'  : 'U') ) )

/* Private Function prototypes ---------------------------------------------- */

static void VSyncDisplay(const VIDDISP_Handle_t DisplayHandle,
                                STVTG_VSYNC_t VSync,
                                U32 LayerIdent);
static BOOL IsEventToBeNotified (const VIDDISP_Handle_t DisplayHandle,
                                const U32 Event_ID);
static void PutNewPictureOnDisplayForNextVSync(
                                VIDDISP_Handle_t DisplayHandle,
                                VIDDISP_Picture_t * const PassedNextPicture_p,
                                U32 LayerIdent);
static void ReStartQueues(VIDDISP_Picture_t * CurrentPicture_p, U32 Layer,
                                VIDDISP_Handle_t DisplayHandle);
static U8   GetPanAndScanIndex(const VIDDISP_Handle_t DisplayHandle,
                                         VIDDISP_Picture_t *Picture_p, U32 LayerIndex);
static void CheckAndSendNewPictureCharacteristicsEvents(const VIDDISP_Handle_t DisplayHandle,
                                VIDBUFF_PictureBuffer_t * NewPictureBuffer_p,
                                U32 Layer,
                                U8 PanAndScanIndex);
static void CheckAndSendNewPictureEvents(const VIDDISP_Handle_t DisplayHandle,
                                VIDBUFF_PictureBuffer_t * Picture_p,
                                U32 Layer);
static void PutBottomForNextVSync(const VIDDISP_Handle_t DisplayHandle,
                                U32 LayerIdent,
                                VIDDISP_Picture_t * NextPicture_p);
static void PutTopForNextVSync(const VIDDISP_Handle_t DisplayHandle,
                                U32 LayerIdent,
                                VIDDISP_Picture_t * NextPicture_p);
static void PutFieldForNextVSync(const VIDDISP_Handle_t DisplayHandle,
                                 U32 LayerIdent,
                                 VIDDISP_Picture_t * NextPicture_p,
                                 HALDISP_FieldType_t FieldType);

static VIDDISP_Picture_t * ComingPicture(
                                VIDBUFF_PictureBuffer_t * CurrentBuffer_p,
                                VIDDISP_Handle_t DisplayHandle);

static void FindFieldWhichMustBeFrozen(const VIDDISP_Handle_t DisplayHandle,
                                VIDDISP_Picture_t * const CurrentPicture_p);
static BOOL FindPictWhichMustBeStopped(const VIDDISP_Handle_t DisplayHandle,
                                VIDDISP_Picture_t * const CurrentPicture_p);
static void SynchronizeDisplay(const VIDDISP_Handle_t DisplayHandle,
                                U32 LayerIdent);
#ifdef STVID_FRAME_RATE_CONVERSION
static void PatchCountersForFrameRateConvertion(VIDDISP_Picture_t * Picture_p,
                                VIDDISP_Data_t *    VIDDISP_Data_p);
#endif

static BOOL DoesFieldPolarityMatchVTG(VIDDISP_Data_t * VIDDISP_Data_p, VIDDISP_Picture_t * NextPicture_p, U32 Layer, VIDDISP_FieldDisplay_t FirstFieldOnDisplay);

static void CheckMPEGFrameCentreOffsetsChange(
              const VIDDISP_Handle_t DisplayHandle,
              const VIDCOM_FrameCropInPixel_t FrameCropInPixel,
              const VIDCOM_PanAndScanIn16thPixel_t * const PanAndScanIn16thPixel,
              const U8 PanAndScanIndex,
              U32   LayerIndex);
static void FreezedDisplay(const VIDDISP_Handle_t DisplayHandle);
static void SequenceSlave(U32 Layer, const VIDDISP_Handle_t DisplayHandle);
#ifdef PRESERVE_FIELD_POLARITY_ON_SLAVE_DISPLAY
    static void SequenceSlaveWithCareOfFieldPolarity(U32 Layer, const VIDDISP_Handle_t DisplayHandle);
#else
    static void SequenceSlaveBasic(U32 Layer, const VIDDISP_Handle_t DisplayHandle);
#endif
static void PushDisplayCommandPrio(const VIDDISP_Handle_t DisplayHandle,
                                    U32 LayerIdent);
static void UpdateCurrentlyDisplayedPicture(const VIDDISP_Handle_t DisplayHandle, U32 Layer);

static void viddisp_ResetField(const VIDDISP_Handle_t DisplayHandle,
                               U32 LayerIdent,
                               HALDISP_Field_t * Field_p);
static void viddisp_ResetFieldArray(const VIDDISP_Handle_t DisplayHandle,
                                    U32 LayerIdent,
                                    HALDISP_Field_t * FieldArray_p);

static void viddisp_IdentifyFieldsToUse(const VIDDISP_Handle_t DisplayHandle,
                                        U32 LayerIdent,
                                        VIDDISP_Picture_t * NextPicture_p,
                                        HALDISP_FieldType_t FieldType);
static void viddisp_SetFieldsForNextVSync(const VIDDISP_Handle_t DisplayHandle,
                                          U32 LayerIdent,
                                          VIDDISP_Picture_t * NextPicture_p,
                                          HALDISP_FieldType_t FieldType);
static void viddisp_InitFieldArray(const VIDDISP_Handle_t DisplayHandle,
                                U32 LayerIdent,
                                HALDISP_Field_t * FieldArray_p);
static void viddisp_SetOtherFieldInSameFrame(const VIDDISP_Handle_t DisplayHandle, U32 LayerIdent);
static void viddisp_SearchNextField(const VIDDISP_Handle_t DisplayHandle,
                                U32 LayerIdent,
                                VIDDISP_Picture_t * NextPicture_p,
                                HALDISP_FieldType_t FieldType);
static void viddisp_SearchNextFieldInListOfPicturesLocked(const VIDDISP_Handle_t  DisplayHandle,
                                                      U32                         LayerIdent,
                                                      U32                         CurrentPictureIndex,
                                                      HALDISP_FieldType_t         ExpectedFieldType,
                                                      BOOL *                      FieldFound_p);
static void viddisp_SearchNextFieldInDisplayQueue(const VIDDISP_Handle_t  DisplayHandle,
                                              U32                         LayerIdent,
                                              VIDDISP_Picture_t *         NextPicture_p,
                                              U32                         CurrentPictureIndex,
                                              HALDISP_FieldType_t         ExpectedFieldType);
static void viddisp_SearchPreviousField(const VIDDISP_Handle_t DisplayHandle, U32 LayerIdent);
static void viddisp_LockFieldArray(const VIDDISP_Handle_t DisplayHandle,
                                   U32 LayerIdent,
                                   HALDISP_Field_t * FieldArray_p);
/*
static BOOL viddisp_IsDisplayPipe(const VIDDISP_Handle_t DisplayHandle,
                                            U32 LayerIdent);
*/
static void viddisp_SetFieldsProtections(const VIDDISP_Handle_t DisplayHandle,
                                         U32 LayerIdent);


#ifdef ST_XVP_ENABLE_FGT
    static void viddisp_StoreFGTParams(const VIDDISP_Handle_t  DisplayHandle,
                                       VIDDISP_Picture_t       *NextPicture_p,
                                       HALDISP_FieldType_t     FieldType);
#endif /* ST_XVP_ENABLE_FGT */

#ifdef DBG_FIELD_PRESENTATION_ORDER
static void CheckFieldsOrder(const VIDDISP_Handle_t DisplayHandle,
                             U32 LayerIdent,
                             VIDDISP_Picture_t * NextPicture_p,
                             HALDISP_FieldType_t FieldType);
#endif /* DBG_FIELD_PRESENTATION_ORDER */

/* Private Functions -------------------------------------------------------- */

/*******************************************************************************
Name        : PutBottomForNextVSync
Description : Put bottom field of the picture on display
Parameters  : Display handle, pointer to picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PutBottomForNextVSync(const VIDDISP_Handle_t DisplayHandle,
                             U32 LayerIdent,
                             VIDDISP_Picture_t * NextPicture_p)
{
    VIDDISP_Data_t      * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    NextPicture_p->Counters[LayerIdent].NextFieldOnDisplay = VIDDISP_FIELD_DISPLAY_BOTTOM;

    TrVSync((" Put %dB on %s\r\n", NextPicture_p->PictureIndex, (LayerIdent == 0) ? "MAIN" : "AUX"));

    if ( (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT)
       &&(LayerIdent != VIDDISP_Data_p->DisplayRef) )
    {
        /* Nothing to do....    */
    }
    else
    {
        /* The code for PutTopForNextVSync() and PutBottomForNextVSync() is now common in PutFieldForNextVSync() */
        PutFieldForNextVSync(DisplayHandle, LayerIdent, NextPicture_p, HALDISP_BOTTOM_FIELD);
    }

} /* End of PutBottomForNextVSync() function */

/*******************************************************************************
Name        : PutTopForNextVSync
Description : Put top field of the picture on display
Parameters  : Display handle, pointer to picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PutTopForNextVSync(const VIDDISP_Handle_t DisplayHandle,
                          U32 LayerIdent,
                          VIDDISP_Picture_t * NextPicture_p)
{
    VIDDISP_Data_t      * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    NextPicture_p->Counters[LayerIdent].NextFieldOnDisplay = VIDDISP_FIELD_DISPLAY_TOP;

    TrVSync((" Put %dT on %s\r\n", NextPicture_p->PictureIndex, (LayerIdent == 0) ? "MAIN" : "AUX"));

    if ( (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT)
       &&(LayerIdent != VIDDISP_Data_p->DisplayRef) )
    {
        /* Nothing to do....    */
    }
    else
    {
        /* The code for PutTopForNextVSync() and PutBottomForNextVSync() is now common in PutFieldForNextVSync() */
        PutFieldForNextVSync(DisplayHandle, LayerIdent, NextPicture_p, HALDISP_TOP_FIELD);
    }

} /* End of PutTopForNextVSync() function */

/*******************************************************************************
Name        : PutFieldForNextVSync
Description : Put field on display (it can be a Top or a Bottom as indicated in FieldType)
Parameters  : Display handle, pointer to picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PutFieldForNextVSync(const VIDDISP_Handle_t DisplayHandle,
                             U32 LayerIdent,
                             VIDDISP_Picture_t * NextPicture_p,
                             HALDISP_FieldType_t FieldType)
{
    STLAYER_Layer_t     LayerType;
    HALDISP_Handle_t    HALDisplayHandle;
    VIDBUFF_PictureBuffer_t *   NextPictureBuff_p;
    VIDDISP_Data_t      * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    STVID_FieldInfos_t  ExportedFieldInfos;
    LayerDisplayParams_t * LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);

    /*TrMain(("Layer %d: Put 0x%x\r\n", LayerIdent, NextPicture_p));*/

    LayerType = ((VIDDISP_Data_t *)DisplayHandle)->LayerDisplay[LayerIdent].LayerType;
    HALDisplayHandle = ( (VIDDISP_Data_t *) DisplayHandle)->HALDisplayHandle[GetHALIndex(DisplayHandle, LayerIdent)];

#ifdef DBG_FIELD_PRESENTATION_ORDER
    CheckFieldsOrder(DisplayHandle, LayerIdent, NextPicture_p, FieldType);
#endif /* DBG_FIELD_PRESENTATION_ORDER */

    /* Save reference of the field prepared for next VSync */
    VIDDISP_Data_p->LayerDisplay[LayerIdent].PicturePreparedForNextVSync_p = NextPicture_p;

    /* Save the polarity of the field prepared for this display */
    if (FieldType == HALDISP_TOP_FIELD)
    {
        LayerDisplayParams_p->IsFieldPreparedTop = TRUE;
    }
    else
    {
        LayerDisplayParams_p->IsFieldPreparedTop = FALSE;
    }

    /* Select Main or Decimated picture buffer */
    NextPictureBuff_p = GetAppropriatePictureBufferForLayer(DisplayHandle, NextPicture_p, LayerIdent);

    /* Remember if the last picture prepared for this layer was a Main or Decimated picture */
    if (NextPictureBuff_p->FrameBuffer_p->AvailableReconstructionMode == VIDBUFF_MAIN_RECONSTRUCTION)
    {
        LayerDisplayParams_p->IsLastPicturePreparedDecimated = FALSE;
    }
    else
    {
        LayerDisplayParams_p->IsLastPicturePreparedDecimated = TRUE;
    }

#ifdef ST_7100
    /* FOR 7100 ONLY:
      Display an alarm if the picture displayed on AUX is not in LMI_SYS (AUX is not able to see the LMI_VID)
      (We use the fact that CPU partition is set to LMI_SYS) */
    if ( (LayerIdent == 1) && (NextPictureBuff_p->FrameBuffer_p->Allocation.PartitionHandle != VIDDISP_Data_p->AvmemPartitionHandle) )
    {
        TrError(("Error! 7100 can not display LMI_VID picture on AUX!\r\n"));
    }
#endif /* ST_7100 */

   if (viddisp_IsDeinterlacingPossible(DisplayHandle, LayerIdent))
   {
        /* We should present the Fields Previous, Current, Next  to the Display (for Deinterlacing) */

        /* Identify what are the Fields Previous, Current, Next   */
        viddisp_IdentifyFieldsToUse(DisplayHandle, LayerIdent, NextPicture_p, FieldType);

    #if defined(ST_crc)
        if(VIDCRC_IsCRCRunning(VIDDISP_Data_p->CRCHandle))
        {
            if(!VIDCRC_IsCRCModeField(VIDDISP_Data_p->CRCHandle,
                                     &(NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos),
                                     NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard))
            {
                /* Force the Current Field (= the one which will be displayed at next VSync) to top field */
                LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].FieldType = HALDISP_TOP_FIELD;
            }
        }
    #endif /* ST_crc */

        /* Present Fields Previous, Current, Next to the Display HAL */
        HALDISP_PresentFieldsForNextVSync(HALDisplayHandle, LayerType, LayerIdent, LayerDisplayParams_p->FieldsUsedAtNextVSync_p);

        /* Decide which Fields should be protected/unprotected */
        viddisp_SetFieldsProtections(DisplayHandle, LayerIdent);
   }
   else
   {
        /* Deinterlacing not possible: Just put the current field */

        /* Clean the structure 'LayerDisplayParams_p->FieldsUsedAtNextVSync_p' before filling it with new data (this array should already be empty) */
        viddisp_ResetFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->FieldsUsedAtNextVSync_p);

        /* Set the Current Field (= the one which will be displayed at next VSync) */
        LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureBuffer_p = NextPictureBuff_p;
        LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureIndex = NextPicture_p->PictureIndex;
        LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].FieldType = FieldType;

        /* Protect the Fields that will be used at the next VSync */
        viddisp_LockFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->FieldsUsedAtNextVSync_p);

        /* Present Fields Previous, Current, Next to the Display HAL */
        HALDISP_PresentFieldsForNextVSync(HALDisplayHandle, LayerType, LayerIdent, LayerDisplayParams_p->FieldsUsedAtNextVSync_p);
   }

    /** Store the Field information in the HALDisplayHandle **/
    ( (HALDISP_Properties_t *) HALDisplayHandle)->FieldsUsedAtNextVSync_p = LayerDisplayParams_p->FieldsUsedAtNextVSync_p;
    ( (HALDISP_Properties_t *) HALDisplayHandle)->IsDisplayStopped = ( (VIDDISP_Data_p->DisplayState == VIDDISP_DISPLAY_STATE_DISPLAYING) ? FALSE : TRUE);
    ( (HALDISP_Properties_t *) HALDisplayHandle)->PeriodicFieldInversionDueToFRC = VIDDISP_Data_p->PeriodicFieldInversionDueToFRC;

    /************************************************************************************************************************/
    /* The functions HALDISP_SelectPresentationLumaTopField() and HALDISP_SelectPresentationChromaTopField() are now obsolete.
       They have no effect with SDDISPO2 and Video Display Pipes. They are kept for compatibility with older chips.
       Fields presentation is now done by HALDISP_PresentFieldsForNextVSync()                                               */
    /************************************************************************************************************************/
    if (FieldType == HALDISP_TOP_FIELD)
    {
        HALDISP_SelectPresentationLumaTopField(HALDisplayHandle, LayerType, LayerIdent, NextPictureBuff_p);
        HALDISP_SelectPresentationChromaTopField(HALDisplayHandle, LayerType, LayerIdent, NextPictureBuff_p);
    }
    else
    {
#if defined(ST_crc) && defined(ST_7109)
        BOOL ForceTopField;
        if(VIDCRC_IsCRCRunning(VIDDISP_Data_p->CRCHandle))
        {
            if(VIDCRC_IsCRCModeField(VIDDISP_Data_p->CRCHandle,
                                     &(NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos),
                                     NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard))
            {
                ForceTopField = FALSE;
            }
            else
            {
                ForceTopField = TRUE;
            }
        }
        else
        {
            ForceTopField = FALSE;
        }

        if(ForceTopField)
        {
            HALDISP_SelectPresentationLumaTopField(HALDisplayHandle, LayerType, LayerIdent, NextPictureBuff_p);
            HALDISP_SelectPresentationChromaTopField(HALDisplayHandle, LayerType, LayerIdent, NextPictureBuff_p);
        }
        else
        {
            HALDISP_SelectPresentationLumaBottomField(HALDisplayHandle, LayerType, LayerIdent, NextPictureBuff_p);
            HALDISP_SelectPresentationChromaBottomField(HALDisplayHandle, LayerType, LayerIdent, NextPictureBuff_p);
        }
#else /* defined(ST_crc) && defined(ST_7109) */
        HALDISP_SelectPresentationLumaBottomField(HALDisplayHandle, LayerType, LayerIdent, NextPictureBuff_p);
        HALDISP_SelectPresentationChromaBottomField(HALDisplayHandle, LayerType, LayerIdent, NextPictureBuff_p);
#endif /* not (defined(ST_crc) && defined(ST_7109)) */
    }

    if ( (IsEventToBeNotified (DisplayHandle, VIDDISP_NEW_FIELD_TO_BE_DISPLAYED_EVT_ID) ) &&
         (LayerIdent == VIDDISP_Data_p->DisplayRef) )
    {
        /* Set ExportedFieldInfos.FieldTypeIsTop field */
        if (FieldType == HALDISP_TOP_FIELD)
        {
            ExportedFieldInfos.DisplayTopNotBottom = TRUE;
        }
        else /* HALDISP_BOTTOM_FIELD */
        {
            ExportedFieldInfos.DisplayTopNotBottom = FALSE;
        }

        /* Set ExportedFieldInfos.FieldTypeIsTop field */
        ExportedFieldInfos.PictureBufferHandle = (STVID_PictureBufferHandle_t) NextPictureBuff_p;

        /* Notify VIDDISP_NEW_FIELD_TO_BE_DISPLAYED_EVT_ID event */
        STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                VIDDISP_Data_p->RegisteredEventsID[VIDDISP_NEW_FIELD_TO_BE_DISPLAYED_EVT_ID],
                STEVT_EVENT_DATA_TYPE_CAST &(ExportedFieldInfos));
    }

    TraceFrameBuffer(("DispFB", "0x%x", NextPictureBuff_p->FrameBuffer_p->Allocation.Address_p));
} /* end of PutFieldForNextVSync() */

#ifdef DBG_FIELD_PRESENTATION_ORDER
/*******************************************************************************
Name        : CheckFieldsOrder
Description : This function checks that the field prepared for next VSync is not
               anterior to the one currently on display.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void CheckFieldsOrder(const VIDDISP_Handle_t DisplayHandle,
                             U32 LayerIdent,
                             VIDDISP_Picture_t * NextPicture_p,
                             HALDISP_FieldType_t NextFieldType)
{
    VIDDISP_Data_t      *   VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_t *  LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);
    BOOL                    IsNextFieldTop = ( (NextFieldType == HALDISP_TOP_FIELD) ? TRUE : FALSE);

    if (LayerDisplayParams_p->CurrentlyDisplayedPicture_p != NULL)
    {
        if (NextPicture_p->PictureIndex < LayerDisplayParams_p->CurrentlyDisplayedPicture_p->PictureIndex)
        {
            TrError(("Error! Incorrect picture order!\r\n"));
        }
        else
        {
            if (NextPicture_p->PictureIndex == LayerDisplayParams_p->CurrentlyDisplayedPicture_p->PictureIndex)
            {
                /* Same picture index. In that case we should check the field order */
                if (LayerDisplayParams_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst == LayerDisplayParams_p->IsCurrentFieldOnDisplayTop)
                {
                    /* The 1st field is currently on display so the NextField prepared can not be anterior */
                }
                else
                {
                    /* The 2nd field is currently on display */
                    if (IsNextFieldTop != LayerDisplayParams_p->IsCurrentFieldOnDisplayTop)
                    {
                        /* The NextField prepared is different from the current field on display so this is the first field. */
                        TrError(("Error! Incorrect field order!\r\n"));
                    }
                }
            }
        }
    }

}
#endif /* DBG_FIELD_PRESENTATION_ORDER */

/*******************************************************************************
Name        : STVID_DisableFrameRateConversion
Description : Function which disables the frame rate conversion
              Set VIDDISP_Data_p->EnableFrameRateConversion to FALSE
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : void
*******************************************************************************/
void VIDDISP_DisableFrameRateConversion(const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t *  VIDDISP_Data_p=(VIDDISP_Data_t *)DisplayHandle;

    /* Set VIDDISP_Data_p->EnableFrameRateConversion flag to FALSE*/
    VIDDISP_Data_p->EnableFrameRateConversion = FALSE;

} /* End of STVID_DisableFrameRateConversion() function */


/*******************************************************************************
Name        : viddisp_DisplayTaskFunc
Description : Function of the display task
              Set VIDDISP_Data_p->DisplayTask.ToBeDeleted to end the task
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddisp_DisplayTaskFunc(const VIDDISP_Handle_t DisplayHandle)
{
    U8                  Cmd;
    U32                 LayerIdent;
    VIDDISP_Data_t *    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    BOOL                IsReferenceDisplay;

    STOS_TaskEnter(NULL);

    /* Big loop executed until ToBeDeleted is set */
    do
    {
        STOS_SemaphoreWait(VIDDISP_Data_p->DisplayOrder_p);

        /* Process all available commands together */
        while (PopDisplayCommand(DisplayHandle, &Cmd) == ST_NO_ERROR)
        {
            /* the command code is the layer-ident */
            LayerIdent = Cmd;

            STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);
#ifdef ST_smoothbackward
            {
                LayerDisplayParams_t      * LayerDisplay_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);

                if ((LayerDisplay_p->CurrentlyDisplayedPicture_p != NULL)
                    && (RemainingFieldsToDisplay(*(LayerDisplay_p->CurrentlyDisplayedPicture_p)) == 0)
                    && (LayerDisplay_p->PicturePreparedForNextVSync_p == NULL)
                    && (LayerIdent == VIDDISP_Data_p->DisplayRef))
                {
                    STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                                 VIDDISP_Data_p->RegisteredEventsID[VIDDISP_NO_NEXT_PICTURE_TO_DISPLAY_EVT_ID],
                                 STEVT_EVENT_DATA_TYPE_CAST NULL);
                }
            }
#endif /* ST_smoothbackward */
            /* Update the pointers related to the currently displayed picture */
            UpdateCurrentlyDisplayedPicture(DisplayHandle, LayerIdent);
            STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);

            if(LayerIdent == VIDDISP_Data_p->DisplayRef)
            {
                IsReferenceDisplay = TRUE;
            }
            else
            {
                IsReferenceDisplay = FALSE;
            }

           /* Notify video modules about the new VSYNC before display treatments (need for trickmode).
              This will take the Trickmode semaphore and ensure that no deadlock between trickmode and display tasks occurs
              (the semaphore are always taken in the following order: Trickmode semaphore first and Display semaphore after) */
            STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                         VIDDISP_Data_p->RegisteredEventsID[VIDDISP_PRE_DISPLAY_VSYNC_EVT_ID],
                         STEVT_EVENT_DATA_TYPE_CAST &IsReferenceDisplay); /* Don't need to give 'TOP/BOT' param */

            /* Display treatments */
            STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);
            SynchronizeDisplay(DisplayHandle, LayerIdent);
            STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);

            /* Notify video modules about the new VSYNC after display treatments (need for trickmode) */
            STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                         VIDDISP_Data_p->RegisteredEventsID[VIDDISP_POST_DISPLAY_VSYNC_EVT_ID],
                         STEVT_EVENT_DATA_TYPE_CAST &IsReferenceDisplay); /* Don't need to give 'TOP/BOT' param */



        } /* end while(controller command) */
    } while (!(VIDDISP_Data_p->DisplayTask.ToBeDeleted));

    STOS_TaskExit(NULL);
} /* End of viddisp_DisplayTaskFunc() function */

/*******************************************************************************
Name        : STVID_EnableFrameRateConversion
Description : Function which enables the frame rate conversion
              Set VIDDISP_Data_p->EnableFrameRateConversion to TRUE
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : void
*******************************************************************************/
void VIDDISP_EnableFrameRateConversion(const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t *  VIDDISP_Data_p=(VIDDISP_Data_t *)DisplayHandle;
    /* Set VIDDISP_Data_p->EnableFrameRateConversion flag to TRUE */
    VIDDISP_Data_p->EnableFrameRateConversion = TRUE;

 } /* End of STVID_EnableFrameRateConversion() function */

/*******************************************************************************
Name        : IsEventToBeNotified
Description : Test if the corresponding event is to be notified (enabled,  ...)
Parameters  : Display handle,
              event ID (defined in vid_dec.h).
Assumptions : Event_ID is supposed to be valid.
Limitations :
Returns     : Boolean, indicating if the event can be notified.
*******************************************************************************/
static BOOL IsEventToBeNotified (const VIDDISP_Handle_t DisplayHandle,
                         const U32 Event_ID)
{
    STVID_ConfigureEventParams_t * ConfigureEventParams_p;
    U32                          * NotificationSkipped_p;
    BOOL                           RetValue       = FALSE;
    VIDDISP_Data_t          * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    /* Get the event configuration. */
    ConfigureEventParams_p = &VIDDISP_Data_p->EventNotificationConfiguration[Event_ID].ConfigureEventParam;
    NotificationSkipped_p  = &VIDDISP_Data_p->EventNotificationConfiguration[Event_ID].NotificationSkipped;
    /* test if the event is enabled. */
    if (ConfigureEventParams_p->Enable)
    {
        switch (ConfigureEventParams_p->NotificationsToSkip)
        {
            case 0 :
                RetValue = TRUE;
            break;

            case 0xFFFFFFFF :
                if ( (*NotificationSkipped_p) == 0xFFFFFFFF)
                {
                    RetValue = TRUE;
                    *NotificationSkipped_p = 0;
                }
                else
                {
                    (*NotificationSkipped_p)++;
                }
            break;

            default :
                if ((*NotificationSkipped_p)++ >= ConfigureEventParams_p->NotificationsToSkip)
                {
                    RetValue = TRUE;
                    *NotificationSkipped_p = 0;
                }
            break;
        }
    }
    return(RetValue);
} /* End of IsEventToBeNotified() function. */

/*******************************************************************************
Name        : viddisp_NewPictureToBeDisplayed
Description : Function to subscribe the VIDBUFF event new picture to be
              displayed. A new picture is decoded, the fct is called
              through event-callback interface.
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void viddisp_NewPictureToBeDisplayed(STEVT_CallReason_t Reason,
                                    const ST_DeviceName_t RegistrantName,
                                    STEVT_EventConstant_t Event,
                                    const void *EventData_p,
                                    const void *SubscriberData_p)
{
#ifdef ST_ordqueue
    VIDQUEUE_ReadyForDisplayParams_t            * Params_p;
#endif /* ST_ordqueue */
    VIDBUFF_PictureBuffer_t *                   ReceivedPictureBuffer_p;
    VIDDISP_Handle_t                            DisplayHandle;
    VIDDISP_Picture_t *                         Pict1_p;
    VIDDISP_Data_t *                            VIDDISP_Data_p;
    BOOL                                        PictDecodedTooLate;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* shortcuts */
    DisplayHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDISP_Handle_t, SubscriberData_p);
    VIDDISP_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDISP_Data_t *, SubscriberData_p);
    assert((DisplayHandle != NULL) && (VIDDISP_Data_p != NULL));

    STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);

#ifdef ST_ordqueue
    Params_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDQUEUE_ReadyForDisplayParams_t *, EventData_p);
    ReceivedPictureBuffer_p = Params_p->Picture_p;
    assert((Params_p != NULL) && (ReceivedPictureBuffer_p != NULL));
#else /* ST_ordqueue */
    UNUSED_PARAMETER(EventData_p);
    ReceivedPictureBuffer_p = NULL;
    STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
    return;
#endif /* not ST_ordqueue */

    Pict1_p = NULL;
    PictDecodedTooLate = FALSE;

#ifdef DBG_CHECK_ASSOCIATED_DECIMATED_PICTURE
    {
        VIDCOM_InternalProfile_t        CommonMemoryProfile;

        if (VIDBUFF_GetMemoryProfile(VIDDISP_Data_p->BufferManagerHandle, &CommonMemoryProfile) == ST_NO_ERROR)
        {
            if (CommonMemoryProfile.ApiProfile.DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
            {
                /* Decimation is enabled: Check that an Associated Decimated picture exists */
                if (ReceivedPictureBuffer_p->AssociatedDecimatedPicture_p == NULL)
                {
                    TrError(("Error! Picture buffer 0x%x with no Associated Decimated picture!\r\n", ReceivedPictureBuffer_p));
                }
            }
        }
    }
#endif /* DBG_CHECK_ASSOCIATED_DECIMATED_PICTURE */

    VIDDISP_Data_p->FrameRatePatchCounterTemporalReference ++;

    /* make the picture a 'display element' and insert in display queue,           */
    /*----------------------------------------------------------------------------------*/
    Pict1_p = ComingPicture(ReceivedPictureBuffer_p, DisplayHandle);

    /* If ComingPicture has failed (no insertion because of Counters 0). */
    if (Pict1_p == NULL)
    {
        STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
        return;
    }

    /* Check special cases in the queue */
    /*----------------------------------*/
    if (Pict1_p->Next_p != NULL)
    {
        if ((Pict1_p->Next_p == VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p) ||
            (Pict1_p->Next_p == VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p))
        {
#ifdef STVID_DEBUG_GET_STATISTICS
            if (Pict1_p->Next_p == VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p)
            {
                if ( VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 )
                    VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedBySec ++;
                else
                    VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedByMain ++;
            }
            else
            {
                if ( VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 )
                    VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedBySec ++;
                else
                    VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedByAux ++;
            }
#endif
            TraceDisplay(("Picture decoded too late !!\r\n"));
        }
        else
        {

            if (Pict1_p->Next_p == VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].PicturePreparedForNextVSync_p)
            {
                /* the new picture should have been inserted just before the prepared */
                /* we're not doing this anymore but keep a trace of it in the stats */
#ifdef STVID_DEBUG_GET_STATISTICS
                    VIDDISP_Data_p->StatisticsPbPicturePreparedAtLastMinuteRejected ++;
#endif
                    TraceDisplay(("Prepared at last minute !!\r\n"));
            }/* end if inserted before prepared*/
        } /* end else */
    }

    if (VIDDISP_Data_p->ForceNextPict)
    {
        TrMain(("Force display with 0x%x\r\n", Pict1_p->PictureInformation_p));
        PutNewPictureOnDisplayForNextVSync(DisplayHandle, Pict1_p, VIDDISP_Data_p->DisplayRef);
        VIDDISP_Data_p->ForceNextPict = FALSE;
        if (VIDDISP_Data_p->FreezeParams.SteppingOrderReceived)
        {
            /* clear the 'step-cmd' and restore the display state */
            VIDDISP_Data_p->FreezeParams.SteppingOrderReceived  = FALSE;
        }
    }

    STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
} /* End of viddisp_NewPictureToBeDisplayed function. */

/*******************************************************************************
Name        : ComingPicture
Description : attach a picture buffer to a 'display element'
              update the counters and insert in queue.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static VIDDISP_Picture_t * ComingPicture(
                                    VIDBUFF_PictureBuffer_t * CurrentBuffer_p,
                                    VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t *    VIDDISP_Data_p = (VIDDISP_Data_t *)DisplayHandle;
    VIDDISP_Picture_t * Picture_p;
    U32                 TopCounter;
    U32                 BottomCounter;
    U8                  MainLayerId = 0;

    assert((DisplayHandle != NULL) && (CurrentBuffer_p != NULL));

    /* Check that the incoming picture buffer is valid */
    if (CurrentBuffer_p->Buffers.IsInUse == FALSE)
    {
        TrError(("Error! Incoming pict buff not in use!\r\n"));
    }
    if (!IS_PICTURE_BUFFER_IN_USE(CurrentBuffer_p) )
    {
        TrError(("Error! Invalid incoming pict buff!\r\n"));
        /* This picture is not accepted */
        return (NULL);
    }

    /* Get a display queue element */
    Picture_p = viddisp_GetPictureElement(DisplayHandle);

    if (Picture_p == NULL)
    {
        TrError(("Error! viddisp_GetPictureElement failed!\r\n"));
        return (NULL);
    }

    /* Attach the picture buffer to the display queue element */
    if (CurrentBuffer_p->FrameBuffer_p->AvailableReconstructionMode == VIDBUFF_MAIN_RECONSTRUCTION)
    {
        /* Usual case: The picture pushed to the display is a Primary FB */
        Picture_p->PrimaryPictureBuffer_p = CurrentBuffer_p;

        if (CurrentBuffer_p->AssociatedDecimatedPicture_p != NULL)
        {
            Picture_p->SecondaryPictureBuffer_p = CurrentBuffer_p->AssociatedDecimatedPicture_p;
        }
    }
    else
    {
        /* Unusual case: A secondary FB is pushed to the display */
        TrError(("Warning! Secondary FB pushed to display!\r\n"));
        Picture_p->SecondaryPictureBuffer_p = CurrentBuffer_p->AssociatedDecimatedPicture_p;
        /* No primary picture is attached in that case */
    }

    if ( (Picture_p->PrimaryPictureBuffer_p == NULL) &&
         (Picture_p->SecondaryPictureBuffer_p == NULL) )
    {
        /* PrimaryPictureBuffer_p and SecondaryPictureBuffer_p can not be NULL at the same time: This picture is not accepted */
        TrError(("Error! Incoming picture with NULL picture buffers!\r\n"));
        return (NULL);
    }

    if (Picture_p->PrimaryPictureBuffer_p != NULL)
    {
        Picture_p->PictureInformation_p = Picture_p->PrimaryPictureBuffer_p;
    }
    else
    {
        Picture_p->PictureInformation_p = Picture_p->SecondaryPictureBuffer_p;
    }

    /* Set the fields counters */

    /* Parser should set VIDCOM_PictureGenericData_t.RepeatFirstField & VIDCOM_PictureGenericData_t.RepeatProgressiveCounter
     * according to parsed standard (MPEG or H264).
     * Note in case of MPEG1, parser must set RepeatFirstField=FALSE and RepeatProgressiveCounter=0 */

    switch (CurrentBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure)
    {
        case STVID_PICTURE_STRUCTURE_TOP_FIELD :
            /* Field picture: top_field_first is 0, decode outputs one field picture */
            TopCounter = 1;
            BottomCounter = 0;
            break;

        case STVID_PICTURE_STRUCTURE_BOTTOM_FIELD :
            /* Field picture: top_field_first is 0, decode outputs one field picture */
            TopCounter = 0;
            BottomCounter = 1;
            break;

        case STVID_PICTURE_STRUCTURE_FRAME :
        default :
            /* Frame picture: top_field_first indicates which field is output first */
            /* Two fields: first field, second field */
            TopCounter    = 1;
            BottomCounter = 1;
            break;
    } /* end switch(PictureStructure) */

    Picture_p->PictureIndex = ++VIDDISP_Data_p->QueuePictureIndex;
    TrPictureId((" [%d]\r\n", Picture_p->PictureIndex));

    Picture_p->Counters[0].TopFieldCounter      = TopCounter;
    Picture_p->Counters[0].BottomFieldCounter   = BottomCounter;
    Picture_p->Counters[0].RepeatFieldCounter   = (CurrentBuffer_p->ParsedPictureInformation.PictureGenericData.RepeatFirstField ? 1:0);
    Picture_p->Counters[0].RepeatProgressiveCounter = CurrentBuffer_p->ParsedPictureInformation.PictureGenericData.RepeatProgressiveCounter;
    Picture_p->Counters[0].CounterInDisplay       = 0;
    Picture_p->Counters[0].NextFieldOnDisplay     = VIDDISP_FIELD_DISPLAY_NONE;
    Picture_p->Counters[0].CurrentFieldOnDisplay  = VIDDISP_FIELD_DISPLAY_NONE;
#ifdef ST_crc
    Picture_p->Counters[0].PreviousFieldOnDisplay = VIDDISP_FIELD_DISPLAY_NONE;
    Picture_p->Counters[0].TopFieldDisplayedCounter = 0;
    Picture_p->Counters[0].BottomFieldDisplayedCounter = 0;
    Picture_p->Counters[0].FrameDisplayDisplayedCounter = 0;
#endif /* ST_crc */
    Picture_p->Counters[0].IsFRConversionActivated = FALSE;
    /* Default case of field management regarding frame rate conversion.   */
    VIDDISP_Data_p->FrameRateConversionNeedsCareOfFieldsPolarity = TRUE;

    /*************************************************************************/
    /* Case : Same input and output frame rate.                              */
    /* If the input frame rate is the same as the output one, we have to     */
    /* display one picture                                                   */
    /* for only one VSync duration so that we'll have 60 frames per seconds  */
    /* displayed,  according to the input frame rate.                        */
    /*************************************************************************/
    if ((Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.FrameRate == VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].VTGVSyncFrequency) ||
       ((Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.FrameRate >= 59000) && ( VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].VTGVSyncFrequency >= 59000) &&
        (Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.FrameRate <=60000)))
    {
        /* Force the amount of field to display each inserted pictures.*/
        /* In this mode, each picture has to be displayed only one VSync. */
        /* So the total amount of "field" to display must be 1 */
        /* (top field choosen).        */
        if (Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
        {
            Picture_p->Counters[0].TopFieldCounter      = 0;
            /* Attempt to fix DDTS GNBvd62704 */
            /* Don't display twice same frame buffer for each fields if the incoming stream rate equals the display rate (cas of field encoded stream P60 displayed on P60 for instance) */
#ifdef WA_GNBvd62704
            if ((VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].LayerParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN ))
            {
                Picture_p->Counters[0].BottomFieldCounter   = 0;
            }
            else
            {
                Picture_p->Counters[0].BottomFieldCounter   = 1;
            }
#else
                Picture_p->Counters[0].BottomFieldCounter   = 1;
#endif

        }
        else
        {
        /* top field choosen for TOP FIELD and FRAME pictures */
            Picture_p->Counters[0].TopFieldCounter      = 1;
            Picture_p->Counters[0].BottomFieldCounter   = 0;
        }

        Picture_p->Counters[0].RepeatFieldCounter   = 0;
        /* Moreover, remember that display should not be done according to*/
        /* fields compliancy.                                             */
        VIDDISP_Data_p->FrameRateConversionNeedsCareOfFieldsPolarity = FALSE;
    }

    /* Update reference display according to source parameters when dual display is in use */
    if (VIDDISP_Data_p->LayerDisplay[0].IsOpened && VIDDISP_Data_p->LayerDisplay[1].IsOpened)
    {
        if (( VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
              VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ) ||
              (VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 &&
              VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 ) )
        {
            if (VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
                MainLayerId = 1;
            else if (VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
                MainLayerId = 0;
        }
        else
        {
            /* Only if Dual display */
            switch (VIDDISP_Data_p->LayerDisplay[0].LayerType)
            {
                case STLAYER_OMEGA2_VIDEO1 :
                case STLAYER_7020_VIDEO1 :
                case STLAYER_HDDISPO2_VIDEO1 :
                case STLAYER_DISPLAYPIPE_VIDEO1 :
                case STLAYER_DISPLAYPIPE_VIDEO2 :
                case STLAYER_DISPLAYPIPE_VIDEO4 :
                case STLAYER_SDDISPO2_VIDEO1 :
                case STLAYER_GAMMA_GDP :
                    MainLayerId = 0;
                    break;

                case STLAYER_OMEGA2_VIDEO2 :
                case STLAYER_7020_VIDEO2 :
                case STLAYER_SDDISPO2_VIDEO2 :
                case STLAYER_HDDISPO2_VIDEO2 :
                case STLAYER_DISPLAYPIPE_VIDEO3 :
                    MainLayerId = 1;
                    break;

                default :
                    break;
            }
        }

#if defined (ST_7100) || defined (ST_7109) || defined(ST_7200)
        /* The reference display is always the MAIN */
        VIDDISP_Data_p->DisplayRef = MainLayerId;
#else
        if ( (Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Height > 576) ||
             (Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Width > 720) ||
            /* For HDDISPO2 cell, for dual display and 576I or 480I output, the DENC is used on HD output       */
            /* and the video is sent both to SD TV and HD output stage. In this case (576I/480I), the reference */
            /* display must be the MAIN */
             ((VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_HDDISPO2_VIDEO2) &&
             (VIDDISP_Data_p->LayerDisplay[MainLayerId].LayerParams.Width <= 720) &&
             (VIDDISP_Data_p->LayerDisplay[MainLayerId].LayerParams.Height <= 576) &&
             (VIDDISP_Data_p->LayerDisplay[MainLayerId].LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN) ) )
        {
            VIDDISP_Data_p->DisplayRef = MainLayerId;
        }
        else
        {
            VIDDISP_Data_p->DisplayRef = 1-MainLayerId;
        }
#endif
    }

#ifdef STVID_FRAME_RATE_CONVERSION
    if (VIDDISP_Data_p->EnableFrameRateConversion)
    {
        PatchCountersForFrameRateConvertion(Picture_p, VIDDISP_Data_p);
    }
#endif

    /* Counters set for Layer 0 are applied to Layer 1 too */
    Picture_p->Counters[1] = Picture_p->Counters[0];

    /* The new coming picture can have null counters due to Frame Rate Conversion but we
       insert it in the Queue anyway. This is necessary for backward trickmodes and deinterlacing. */
    viddisp_InsertPictureInDisplayQueue(DisplayHandle, Picture_p, &VIDDISP_Data_p->DisplayQueue_p);

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDDISP_Data_p->StatisticsPictureInsertedInQueue ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

    return(Picture_p);
} /* End of ComingPicture() function */

#ifdef STVID_FRAME_RATE_CONVERSION
/*******************************************************************************
Name        : PatchCountersForFrameRateConvertion
Description : As the function name says...
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void PatchCountersForFrameRateConvertion(VIDDISP_Picture_t * Picture_p,
                                                VIDDISP_Data_t *    VIDDISP_Data_p)
{
    U32                     VTGVSyncFrequency;
    STVID_PictureInfos_t *  CurrentPictureInfos_p;
    U32                     Modulo,Division,InputFrameRate;
    U8                      RepeatDisp = 0;
    /* Retrieve PictureInfos_p from Picture_p */
    CurrentPictureInfos_p = &(Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos);

    /* IsFRConversionActivated indicates if we will present a Top on a Bot (or  */
    /* the opposite) but also the information to the AVSYNC that an action      */
    /* is done by the DISPLAY (AVSYNC to recenter its window around an          */
    /* adjusted PTS).                                                           */
    Picture_p->Counters[0].IsFRConversionActivated = FALSE;
    VIDDISP_Data_p->PeriodicFieldInversionDueToFRC = FALSE;

    VTGVSyncFrequency = VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].VTGVSyncFrequency/*VTGFrameRate*/;
    /*************************************************************************/
    /* Case : 3:2 pull down                                                  */
    /*************************************************************************/
    /* Action is done with a mosaic of 4 picture. For every two pictures, one*/
    /* should be repeated, and management of RepeatFirstField/TopFieldFirst  */
    /* should be done too.                                                   */
    /*                                                                       */
    /* Exemple of 3:2 pull-down conversion :                                 */
    /*              IO      B1      B2      B3   ...                         */
    /*             B T     B T     B T     B T   ...: 24 Frames/Second       */
    /* Action :     |       |       |       |--> Nothing to do.              */
    /*              |       |       --> Invert field and repeat first field. */
    /*              |       --> Invert field only.                           */
    /*              --> Repeat first field only.                             */
    /* Result :    B T B   T B     T B T   B T   ...: 30 Frames/second       */
    /*************************************************************************/
    if (/* First condition : ... */
            (CurrentPictureInfos_p->VideoParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN)
            && (!Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.RepeatFirstField)
        /* Second condition, input frame rate is nearly 24Hz.               */
        /* And output frame rate is nearly 60 Frame per second.             */
        && (CurrentPictureInfos_p->VideoParams.FrameRate <= 24000) && (CurrentPictureInfos_p->VideoParams.FrameRate > 23000)
        && ((VTGVSyncFrequency >= 59000) || ((VTGVSyncFrequency >= 29000)&&(VTGVSyncFrequency <= 31000)) ) )
    {
        Picture_p->Counters[0].IsFRConversionActivated   = TRUE;

        switch (VIDDISP_Data_p->FrameRatePatchCounterTemporalReference % 4)
        {
            case 0 :
                /* First picture out of four : Repeat the first field.      */
                Picture_p->Counters[0].RepeatFieldCounter ++;
                break;

            case 1 :
                /* Second picture out of four : Invert first fields.        */
                CurrentPictureInfos_p->VideoParams.TopFieldFirst =
                (CurrentPictureInfos_p->VideoParams.TopFieldFirst ? FALSE : TRUE);

                break;

            case 2 :
                /* Third picture out of four : Invert first field.          */
                /*                             Repeat the first field.      */
                CurrentPictureInfos_p->VideoParams.TopFieldFirst =
                (CurrentPictureInfos_p->VideoParams.TopFieldFirst ? FALSE : TRUE);
                Picture_p->Counters[0].RepeatFieldCounter ++;
                break;

            case 3 :
            default :
                /* Fourth picture out of four : Nothing to do.              */
                break;
        }
    }
    /*******************************************************************/
    /* Case : PAL streams on NTSC output                               */
    /* Repeat one picture out of 5 (so 6 instead of 5 -> 25Hz to 30Hz. */
    /*******************************************************************/
    else if (/* First condition : ...                                */
    /* Input frame rate is nearly 25Hz and Output frame rate is nearly 30 Hz. */
    /*  23976, 24000, 25000, 29970, 30000, 50000, 59940, 60000 are possible   */
    (CurrentPictureInfos_p->VideoParams.FrameRate == 25000)
    &&( (VTGVSyncFrequency >= 59000)
     ||((VTGVSyncFrequency >= 29000) /* Display at 30 Hz or more */
     && (VTGVSyncFrequency <= 31000)) ))
    {
        /* ----------------------------------------------------------------- */
        /* VSyncs         |  T   B   T   B   T   B   T   B   T   B   T   B   */
        /* ----------------------------------------------------------------- */
        /* Pict           |  < P1 >  < P2 >  < P3 >  < P4 >  <     P5     >  */
        /* ----------------------------------------------------------------- */
        /* Pict Fields    |  T   B   T   B   T   B   T   B   T   T   B   B   */
        /* ----------------------------------------------------------------- */
        /* Field Polarity |  ok  ok  ok  ok  ok  ok  ok  ok  ok  in  in  ok  */
        /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
        /* ok            : T on T or B on B                                  */
        /* in (inversed) : T on B or B on T                                  */
        /* ----------------------------------------------------------------- */

        /* Update the IsFRConversionActivated flag for this picture.      */
        /* This will allow to display a Bottom field on a VSync Top  */
        /* or a Top field on a VSync Bottom (inversed).              */
        Picture_p->Counters[0].IsFRConversionActivated   = TRUE;
        /* Set a flag indicating that periodic field inversions are normal because of FRC */
        VIDDISP_Data_p->PeriodicFieldInversionDueToFRC = TRUE;

        if ((VIDDISP_Data_p->FrameRatePatchCounterTemporalReference % 5) == 0)
        {
            Picture_p->Counters[0].TopFieldCounter      *= 2;
            Picture_p->Counters[0].BottomFieldCounter   *= 2;
        }
    }

    else if (/* First condition : ...                                */
        /* Input frame rate is < 25Hz and output frame rate > Input frame rate */
        (CurrentPictureInfos_p->VideoParams.FrameRate < 25000) &&
        (CurrentPictureInfos_p->VideoParams.FrameRate < VTGVSyncFrequency))
    {
        InputFrameRate = CurrentPictureInfos_p->VideoParams.FrameRate ;
        if ( VIDDISP_Data_p->PreviousFrameRate != InputFrameRate )
        {
              VIDDISP_Data_p->FrameNumber = 0;
              VIDDISP_Data_p->FrameTobeAdded = 0;
        }
        VTGVSyncFrequency = ( (VTGVSyncFrequency + 999) / 1000 ) * 1000 ;
        InputFrameRate  = ( (InputFrameRate + 999 ) / 1000 ) * 1000 ;

        VIDDISP_Data_p->FrameNumber++;
        VIDDISP_Data_p->FrameNumber %= (InputFrameRate / 1000);

        if ( VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN )
        {
            /* This for the interlaced mode which has in parameter the number of field and not the number of frame ,
             * so frame = 2* field like PAL mode 'MODE_576I50000_13500' in this case the number of frame = 25 */
            VTGVSyncFrequency /=2;
        }

        /** This case is when the VTGFrameRate = Division * InputFrameRate + Modulo , so we need to patch all the picture to be repeated Division time
            and distribute the Modulo picture in equitable way on all picture which should be repeated **/
        Modulo = (VTGVSyncFrequency % InputFrameRate);
        Division = (VTGVSyncFrequency / InputFrameRate);

        if(VIDDISP_Data_p->FrameNumber == 1)
        {
            /** In the begining we should initialize the FrameTobeAdded  **/
            VIDDISP_Data_p->FrameTobeAdded += Modulo ;
        }
        /* Repeat the picture  Division times */
        RepeatDisp = Division;
        /* Patch the PictureCounter to take into account the value of Modulo */
        if((VIDDISP_Data_p->FrameTobeAdded > 0) && (Modulo != 0) && (((VIDDISP_Data_p->FrameNumber + 1) % (InputFrameRate/Modulo)) == 0))
        {
            RepeatDisp++;
            VIDDISP_Data_p->FrameTobeAdded--;
        }
        /* Now repeat */
        Picture_p->Counters[0].BottomFieldCounter *= RepeatDisp;
        Picture_p->Counters[0].TopFieldCounter *= RepeatDisp;

        VIDDISP_Data_p->PreviousFrameRate = CurrentPictureInfos_p->VideoParams.FrameRate ;
    }
    else if (/* First condition : ...                                */
    /* Input frame rate is nearly 30Hz and Output frame rate is nearly 25 Hz. */
    /*  23976, 24000, 25000, 29970, 30000, 50000, 59940, 60000 are possible   */
    ((CurrentPictureInfos_p->VideoParams.FrameRate >= 29000)
   &&(CurrentPictureInfos_p->VideoParams.FrameRate <= 31000))
    && ( (VTGVSyncFrequency == 50000) || (VTGVSyncFrequency == 25000)))
    {
        /* 30 picts/sec NTSC coded stream */
        /* --------------------------------------------------------------- */
        /* VSyncs         |  T   B   T   B   T   B   T   B   T   B   T   B */
        /* --------------------------------------------------------------- */
        /* Pict           |  < P1 >  < P2 >  < P3 >  < P4 >  < P5 >  *     */
        /* --------------------------------------------------------------- */
        /* Pict Fields    |  T   B   T   B   T   B   T   B    T   B  T   B */
        /* --------------------------------------------------------------- */
        /* Field Polarity |  ok  ok  ok  ok  ok  ok  ok  ok * ok  ok  */
        /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
        /* ok            : T on T or B on B                               */
        /* *             : P6 is not diplayed.                            */
        /* ---------------------------------------------------------------*/

        /* 24 picts/sec NTSC coded stream */
        /* --------------------------------------------------------------- */
        /* VSyncs         |  T   B   T   B   .   .   T   B    T   B        */
        /* --------------------------------------------------------------- */
        /* Pict           |  < P1 >  < P2 >  .   .   < P24 >  < P24 >      */
        /* --------------------------------------------------------------- */
        /* Pict Fields    |  T   B   T   B   .   .   T   B    T   B  T   B */
        /* --------------------------------------------------------------- */
        /* Field Polarity |  don't check                                   */
        /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
        /* *             : P24 is kept on display for 2 vsyncs             */
        /* --------------------------------------------------------------- */

        /* If stream alternate from 24 pict/sec shown in NTSC to 30 pict/sec
         * or vice-versa, we must detect that. Condition to detect new
         * 30 pict/sec is that we saw a picture without repeat field flag that
         * is modulo 2 temporally with the last picture that had the flag.
         * Condition to detect new 24pict/sec is to see a picture with repeat
         * field flag */
        Picture_p->Counters[0].IsFRConversionActivated   = TRUE;

        if (Picture_p->Counters[0].RepeatFieldCounter > 0)
        {
            /* Stream is 24pict/sec */
            if (VIDDISP_Data_p->FrameRatePatchCounterTemporalReference > VIDDISP_Data_p->LastFlaggedPicture)
            {
                /* If we have a recent picture with repeat field flag, remember it */
                VIDDISP_Data_p->LastFlaggedPicture = VIDDISP_Data_p->FrameRatePatchCounterTemporalReference;
            }
            /* If stream is changing from 30pict/sec to 24pict/sec, remember first picture in the new stream
             * to be able to repeat the same frame after 24 pictures */
            if ((!VIDDISP_Data_p->StreamIs24Hz) ||
                (VIDDISP_Data_p->FrameRatePatchCounterTemporalReference < VIDDISP_Data_p->InitialTemporalReferenceForRepeating))
            {
                VIDDISP_Data_p->InitialTemporalReferenceForRepeating = VIDDISP_Data_p->FrameRatePatchCounterTemporalReference;
                TrFRConversion(("\r\nFR conversion : Initial repeating is InitialTemporalReferenceForRepeating=%d PTS=%d\r\n",
                        VIDDISP_Data_p->InitialTemporalReferenceForRepeating,
                        CurrentPictureInfos_p->VideoParams.PTS.PTS));
                VIDDISP_Data_p->StreamIs24Hz = TRUE;
            }
            /* Patching counter to have all pictures displayed for only 1 vsync */
            Picture_p->Counters[0].RepeatFieldCounter --;
        }
        /* May be stream changed to 30pict/sec, so update initial reference */
        else
        {
            if ( (VIDDISP_Data_p->LastFlaggedPicture < VIDDISP_Data_p->FrameRatePatchCounterTemporalReference) &&
            ((VIDDISP_Data_p->FrameRatePatchCounterTemporalReference - VIDDISP_Data_p->LastFlaggedPicture) % 2 == 0) &&
            (VIDDISP_Data_p->StreamIs24Hz) )
            {
                /*  Stream changed to 30pict/sec, so remember first picture to be able to skip
                 * one picture each 6 */
                VIDDISP_Data_p->StreamIs24Hz = FALSE;
                VIDDISP_Data_p->InitialTemporalReferenceForSkipping = VIDDISP_Data_p->FrameRatePatchCounterTemporalReference;
                TrFRConversion(("\r\nFR conversion : Initial skipping 1 is InitialTemporalReferenceForSkipping=%d PTS=%d\r\n",
                        VIDDISP_Data_p->InitialTemporalReferenceForSkipping,
                        CurrentPictureInfos_p->VideoParams.PTS.PTS));
            }
            /* If we had a P picture that will be displayed after this one
             * (should be a B picture) and that was considered as the first one
             * in the new stream 30pict/sec, correct that by saying first one
             * in the new stream is the new picture */
            if (VIDDISP_Data_p->FrameRatePatchCounterTemporalReference < VIDDISP_Data_p->InitialTemporalReferenceForSkipping)
            {
                VIDDISP_Data_p->InitialTemporalReferenceForSkipping = VIDDISP_Data_p->FrameRatePatchCounterTemporalReference;
                TrFRConversion(("\r\nFR conversion : Initial skipping 2 is InitialTemporalReferenceForSkipping=%d PTS=%d\r\n",
                         VIDDISP_Data_p->InitialTemporalReferenceForSkipping,
                         CurrentPictureInfos_p->VideoParams.PTS.PTS));
            }
        }
        /* Do frame rate conversion : skip each 6 for 30pict/sec stream */
        if (!VIDDISP_Data_p->StreamIs24Hz)
        {
            VIDDISP_Data_p->FrameRateConversionNeedsCareOfFieldsPolarity = TRUE;
            if (((VIDDISP_Data_p->FrameRatePatchCounterTemporalReference - VIDDISP_Data_p->InitialTemporalReferenceForSkipping) % 6) == 3)
            {
                /* When no picture in the last 5 ones was skipped, skip one now */
                if (Picture_p->Counters[0].TopFieldCounter > 0)
                {
                    Picture_p->Counters[0].TopFieldCounter -= 1;
                }
                if (Picture_p->Counters[0].BottomFieldCounter > 0)
                {
                    Picture_p->Counters[0].BottomFieldCounter -= 1;
                }
                TrFRConversion(("\r\nFR conversion : Picture PTS=%d will be skipped once \r\n",CurrentPictureInfos_p->VideoParams.PTS.PTS));
            }
        }
        else
        /* Do frame rate conversion : repeat picture each 24 for 24pict/sec stream */
        {
            /* Don't care of polarity because we patch the repeat field flag */
            VIDDISP_Data_p->FrameRateConversionNeedsCareOfFieldsPolarity = FALSE;
            /* Repeat picture each 24 picture */
            if (((VIDDISP_Data_p->FrameRatePatchCounterTemporalReference - VIDDISP_Data_p->InitialTemporalReferenceForRepeating) % 24) == 11)
            {
                Picture_p->Counters[0].TopFieldCounter      *= 2;
                Picture_p->Counters[0].BottomFieldCounter   *= 2;
                TrFRConversion(("\r\nFR conversion : Picture PTS=%d will be repeated once \r\n",CurrentPictureInfos_p->VideoParams.PTS.PTS));
            }
        }
    }

}
#endif /* STVID_FRAME_RATE_CONVERSION */

/*******************************************************************************
Name        : DoesFieldPolarityMatchVTG
Description : test if the next field polarity is ok to be displayed without
              field inversion (TOP field needs to be displayed on TOP VSYNC for ex.)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL DoesFieldPolarityMatchVTG(VIDDISP_Data_t * VIDDISP_Data_p, VIDDISP_Picture_t * NextPicture_p, U32 Layer, VIDDISP_FieldDisplay_t FirstFieldOnDisplay)
{
    /* If a stepping order is received, field always matches */
    if ((VIDDISP_Data_p->FreezeParams.SteppingOrderReceived))
    {
        return TRUE;
    }

    /* Polarity is meaningful only if both stream and vtg are interlaced */
    if ( (VIDDISP_Data_p->LayerDisplay[Layer].LayerParams.ScanType != STGXOBJ_INTERLACED_SCAN) ||
         (NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.ScanType != STGXOBJ_INTERLACED_SCAN) )
    {
        return TRUE;
    }

    if ( (VIDDISP_Data_p->DisplayWithCareOfFieldsPolarity) &&
         (VIDDISP_Data_p->DisplayRef == Layer) &&
         (VIDDISP_Data_p->FrameRateConversionNeedsCareOfFieldsPolarity))
    {
        LayerDisplayParams_t  aLayerDisplay = VIDDISP_Data_p->LayerDisplay[Layer];

        if ((FirstFieldOnDisplay == VIDDISP_FIELD_DISPLAY_TOP) ||
            ((FirstFieldOnDisplay == VIDDISP_FIELD_DISPLAY_REPEAT) &&
             (NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst)))
        {
            /* Next field will be a top */
            /* We check the vtg polarity that will occur when the field will be displayed  */
            /* using the latency to guess the correct polarity from the current one */
            /* If the polarity does not match, we won't display the field */
            if (((aLayerDisplay.CurrentVSyncTop == TRUE) && ((aLayerDisplay.DisplayLatency % 2) == 1)) ||
                 ((aLayerDisplay.CurrentVSyncTop == FALSE) && ((aLayerDisplay.DisplayLatency % 2) == 0)))
            {
                TrWarning(("Top field won't be presented on Bot vtg !! \r\n"));
                return FALSE; /* can't display top field on next bottom vtg */
            }
        }
        else
        {
            /* Next field will be a bottom */
            /* We check the vtg polarity that will occur when the field will be displayed  */
            /* using the latency to guess the correct polarity from the current one */
            /* If the polarity does not match, we won't display the field */
            if (((aLayerDisplay.CurrentVSyncTop == FALSE) && ((aLayerDisplay.DisplayLatency % 2) == 1)) ||
                 ((aLayerDisplay.CurrentVSyncTop == TRUE) && ((aLayerDisplay.DisplayLatency % 2) == 0)))
            {
                TrWarning(("Bot field won't be presented on Top vtg !! \r\n"));
                return FALSE; /* can't display bottom field on next top vtg */
            }
        }
    } /* end if care of polarity */

    return TRUE;  /* If we reach this point the field polarity is OK */
}

/*******************************************************************************
Name        : viddisp_PictureCandidateToBeRemovedFromDisplayQueue
Description : Function to subscribe the VIDBUFF event picture candidate
               to be removed from display queue
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void viddisp_PictureCandidateToBeRemovedFromDisplayQueue(
                                    STEVT_CallReason_t Reason,
                                    const ST_DeviceName_t RegistrantName,
                                    STEVT_EventConstant_t Event,
                                    const void *EventData_p,
                                    const void *SubscriberData_p)
{
    VIDDISP_Data_t *            VIDDISP_Data_p;
    VIDDISP_Handle_t            DisplayHandle;

#ifdef ST_ordqueue
    VIDQUEUE_PictureCandidateToBeRemovedParams_t *  PictureCandidate_p;
#endif /* ST_ordqueue */

    U32                         Index;
    VIDDISP_Picture_t *         Picture_p;
    BOOL                        InDisplay;

    /* shortcuts */
    DisplayHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDISP_Handle_t, SubscriberData_p);
    VIDDISP_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDISP_Data_t *, SubscriberData_p);
#ifdef ST_ordqueue
    PictureCandidate_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDQUEUE_PictureCandidateToBeRemovedParams_t *, EventData_p);
#else
    UNUSED_PARAMETER(EventData_p);
#endif /* ST_ordqueue */
    Picture_p = NULL;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    TraceDisplay(("Try to remove picture %d/%d from display !!\r\n",
            PictureCandidate_p->PictureID.IDExtension,
            PictureCandidate_p->PictureID.ID));

    /* lock access */
    STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);

    /* find associated display queue element */
    Index = 0;
    InDisplay = FALSE;
    while ((!(InDisplay)) && (Index <NB_QUEUE_ELEM))
    {
#ifdef ST_ordqueue
        if ((VIDDISP_Data_p->DisplayQueueElements[Index].PictureInformation_p != NULL) && (vidcom_ComparePictureID(&VIDDISP_Data_p->DisplayQueueElements[Index].PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID,
                                    &PictureCandidate_p->PictureID) == 0))
#else /* ST_ordqueue */
        if (FALSE)
#endif /* not ST_ordqueue */
        {
            InDisplay = TRUE;
            Picture_p = &VIDDISP_Data_p->DisplayQueueElements[Index];
        }
        Index ++;
    }

    if (!InDisplay)
    {
        TraceDisplay(("... was not a display element !!\r\n"));
        STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
        return; /* exit now */
    }

    if ((Picture_p==VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p) ||
        (Picture_p==VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p) )
    {
        TraceDisplay(("... was on display !!\r\n"));
        STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
        return; /* exit now */
    }

    if ((Picture_p == VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p) ||
        (Picture_p == VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p) )
    {
        TraceDisplay(("... was prepared !!\r\n"));
        STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
        return; /* exit now */
    }

    /* ... OK, can be removed from the queue */
    viddisp_RemovePictureFromDisplayQueue(DisplayHandle,Picture_p);
    TraceDisplay(("... done !!\r\n"));
    STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);

} /* end of viddisp_PictureCandidateToBeRemovedFromDisplayQueue() function */

/*******************************************************************************
Name        : PutNewPictureOnDisplayForNextVSync
Description : Put new picture on display, to be displayed on next VSync
              Don't put the picture if the decode is not OK or if field
              doesn't match
              !!! May put the decimated if needed !!!
Parameters  : Display handle, pointer to picture (can be NULL),
              TopNotBottomOnDisplay is the field currently on display
              (picture display will start of the field of opposite polarity)
Assumptions : Display queue should not be locked during call of this function
Limitations :
Returns     : TRUE if picture displayed, FALSE if not (nothing was changed in
            display)
*******************************************************************************/
static void PutNewPictureOnDisplayForNextVSync(
                                const VIDDISP_Handle_t DisplayHandle,
                                VIDDISP_Picture_t * PassedNextPicture_p,
                                U32 Layer)
{
    VIDBUFF_PictureBuffer_t *   NextPictureBuff_p = NULL;
    VIDBUFF_PictureBuffer_t *   PictureBuffPresentedToDisplay_p;
    VIDDISP_Picture_t *         NextPicture_p;
    VIDDISP_Picture_t *         CurrentlyDisplayedPicture_p;
    STVID_PictureInfos_t *      NextPictureInfos_p;
    VIDDISP_Data_t *            VIDDISP_Data_p;
    VIDDISP_FieldDisplay_t      FirstFieldOnDisplay;
    VIDDISP_NewPicturePreparedParams_t  NewPicturePreparedParams;
    osclock_t                   TimeNow;
    U8                          PanAndScanIndex;

    /* PassedNextPicture_p can be NULL */
    assert(DisplayHandle != NULL);

    /* shortcuts */
    VIDDISP_Data_p      = (VIDDISP_Data_t *)DisplayHandle;
    CurrentlyDisplayedPicture_p = VIDDISP_Data_p->LayerDisplay[Layer].CurrentlyDisplayedPicture_p;
    /* Save the picture pointer passed as argument so it is not modified in this function */
    NextPicture_p       = PassedNextPicture_p;
    NextPictureInfos_p  = &(NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos);

    if ((Layer == VIDDISP_Data_p->DisplayRef) && (VIDDISP_Data_p->LockState == LOCKED))
    {
        TrWarning(("Queue Locked\r\n"));
        return;
    }

    if (VIDDISP_Data_p->LayerDisplay[Layer].CurrentlyDisplayedPicture_p == NextPicture_p)
    {
        if(VIDDISP_Data_p->DisplayRef == Layer)
        {
            TrWarning(("Layer %d keep same field 0x%x-0x%x\r\n", Layer, NextPicture_p->PrimaryPictureBuffer_p, NextPicture_p->SecondaryPictureBuffer_p));
            TraceDisplay(("Display %i keep same field \r\n",Layer));
            /* for blitter-display : we ask again the field */
            if (NextPicture_p->Counters[Layer].CurrentFieldOnDisplay
                    == VIDDISP_FIELD_DISPLAY_TOP)
            {
                PutTopForNextVSync(DisplayHandle, Layer, NextPicture_p);
            }
            else
            {
                PutBottomForNextVSync(DisplayHandle, Layer, NextPicture_p);
            }
        }
        return;
    }

    /* Prepare setup to display top or bottom field, and send events of new */
    /* picture */
    /* Default: next display top field , first frame of picture*/
    FirstFieldOnDisplay     = VIDDISP_FIELD_DISPLAY_TOP;

    /* Determine which frame of the picture is to be displayed */
    /*-------------------------------------------------------- */
    if (!(VIDDISP_Data_p->DisplayForwardNotBackward))
    {
        /* Display backward */
        if (((!(NextPictureInfos_p->VideoParams.TopFieldFirst))
            && (NextPicture_p->Counters[Layer].RepeatFieldCounter != 0))
            ||((NextPicture_p->Counters[Layer].BottomFieldCounter != 0)
            &&((NextPictureInfos_p->VideoParams.TopFieldFirst)
             ||(NextPicture_p->Counters[Layer].TopFieldCounter == 0))))
        {
            FirstFieldOnDisplay = VIDDISP_FIELD_DISPLAY_BOTTOM;
        }
        if (NextPictureInfos_p->VideoParams.ScanType != STGXOBJ_PROGRESSIVE_SCAN)
        {
            /* Backward interlaced */
            if (NextPicture_p->Counters[Layer].RepeatFieldCounter != 0)
            {
                /* Next displaying repeat field */
                FirstFieldOnDisplay     = VIDDISP_FIELD_DISPLAY_REPEAT;
            }
        }
    }
    else
    {
        /* Display forward */
        if (((NextPicture_p->Counters[Layer].BottomFieldCounter != 0)
        &&((!(NextPictureInfos_p->VideoParams.TopFieldFirst))
           ||(NextPicture_p->Counters[Layer].TopFieldCounter == 0)))
        ||((!(NextPictureInfos_p->VideoParams.TopFieldFirst))
           &&(NextPicture_p->Counters[Layer].RepeatFieldCounter != 0)) )
        {
            FirstFieldOnDisplay = VIDDISP_FIELD_DISPLAY_BOTTOM;
        }
    }

    /* Test if field polarity of next picutre matches the vtg. Otherwise return to wait next vsync to display picture */
    if( DoesFieldPolarityMatchVTG(VIDDISP_Data_p, NextPicture_p, Layer, FirstFieldOnDisplay) == FALSE)
    {
        return;
    }

    /* Set the prepared picture */
    VIDDISP_Data_p->LayerDisplay[Layer].PicturePreparedForNextVSync_p = NextPicture_p;
    VIDDISP_Data_p->LayerDisplay[Layer].PicturePreparedForNextVSync_p->Counters[Layer].NextFieldOnDisplay = FirstFieldOnDisplay;
    /*TrPictPrepared(("Layer %d: prepare 0x%x\r\n", Layer, VIDDISP_Data_p->LayerDisplay[Layer].PicturePreparedForNextVSync_p->PictureBuffer_p));*/

    if (Layer == VIDDISP_Data_p->DisplayRef)
    {
        TimeNow = time_now();
        /* send evt for av sync only for ref display */
        NewPicturePreparedParams.PictureInfos_p
            = &(NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos);
        NewPicturePreparedParams.TimeElapsedSinceVSyncAtNotification
            = ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION
                * ((U32) time_minus(TimeNow,VIDDISP_Data_p->LayerDisplay[Layer].TimeAtVSyncNotif)))
               / ST_GetClocksPerSecond());
        if ((VIDDISP_Data_p->LayerDisplay[Layer].VTGVSyncFrequency == 0)
            || (NewPicturePreparedParams.TimeElapsedSinceVSyncAtNotification
                >= (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION * 1000
                    / VIDDISP_Data_p->LayerDisplay[Layer].VTGVSyncFrequency)))
        {
            /* avoid a stupid value ! */
            NewPicturePreparedParams.TimeElapsedSinceVSyncAtNotification = 0;
        }
        NewPicturePreparedParams.OutputFrameRate
            = VIDDISP_Data_p->LayerDisplay[Layer].VTGVSyncFrequency;
        NewPicturePreparedParams.IsDisplayedWithCareOfPolarity
            = VIDDISP_Data_p->DisplayWithCareOfFieldsPolarity &&
            VIDDISP_Data_p->FrameRateConversionNeedsCareOfFieldsPolarity;
        NewPicturePreparedParams.IsAnyFrameRateConversionActivated
            = NextPicture_p->Counters[Layer].IsFRConversionActivated;
        if (VIDDISP_Data_p->LayerDisplay[Layer].VTGVSyncFrequency != 0)
        {
            /* Delay introduced by the Display (expressed in 90kHz units) */
            NewPicturePreparedParams.DisplayDelay = (90000 * 1000) / VIDDISP_Data_p->LayerDisplay[Layer].VTGVSyncFrequency;
        }
        else
        {
            NewPicturePreparedParams.DisplayDelay =0;
        }

        STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                     VIDDISP_Data_p->RegisteredEventsID[VIDDISP_NEW_PICTURE_PREPARED_EVT_ID],
                     STEVT_EVENT_DATA_TYPE_CAST &(NewPicturePreparedParams));

    } /* end if display is the ref and current picture is new */

    /* The synchronization might have removed the picture to be displayed on
       the next VSYNC from the display because :
       - It is too late to display that picture (we need to skip it)
       - It is too early (we need to freeze on current) */
#ifdef ST_OSWINCE   /* Temporary fix */
    NextPicture_p = VIDDISP_Data_p->LayerDisplay[Layer].PicturePreparedForNextVSync_p;
    if(NextPicture_p == NULL)
#else
    if(VIDDISP_Data_p->LayerDisplay[Layer].PicturePreparedForNextVSync_p == NULL)
#endif
    {
        /*  The display task is trying to put a  new
         * picture on the display. So we have to care, that if the
         * synchronization decides to discard the current picture, we might
         * need to lock the display queue. We lock the queue only if the
         * current picture has no more fields to be displayed. */
        if((CurrentlyDisplayedPicture_p != NULL)
           &&(RemainingFieldsToDisplay(*CurrentlyDisplayedPicture_p) == 0))
        {
            if (VIDDISP_Data_p->LockState == UNLOCKED)
            {
                TraceDisplay(("Display Queue Lock Next \r\n"));
                VIDDISP_Data_p->LockState = LOCK_NEXT;
                VIDDISP_Data_p->ForceNextPict   = TRUE;
            }
            else
            {
                /* Too late : lock the queue to restart it properly */
                VIDDISP_Data_p->LockState       = LOCKED;
                VIDDISP_Data_p->ForceNextPict   = FALSE;
                /* to be sure that the next unlock of the display will be */
                /* done correctly */
                TraceDisplay(("Display Queue already locked !!\r\n"));
#ifdef STVID_DEBUG_GET_STATISTICS
                VIDDISP_Data_p->StatisticsPbQueueLockedByLackOfPicture++;
                /* VIDBUFF_PrintPictureBuffersStatus(VIDDISP_Data_p->BufferManagerHandle); Useful function to help debugging */
#endif /* STVID_DEBUG_GET_STATISTICS */
            }
        }
        /* Nothing to display */
        return;
    }

    /* Update the next picture buffer as the next picture to be displayed on
       the next VSYNC might have been changed by the Synchronization process */
#ifdef ST_OSWINCE   /* Temporary fix */
    NextPictureBuff_p = GetAppropriatePictureBufferForLayer(DisplayHandle, NextPicture_p, Layer);
#else
    NextPicture_p = VIDDISP_Data_p->LayerDisplay[Layer].PicturePreparedForNextVSync_p;
    /* Select Main or Decimated picture buffer */
    NextPictureBuff_p = GetAppropriatePictureBufferForLayer(DisplayHandle, NextPicture_p, Layer);
#endif

    /* OK, Set registers (if it was KO then the function has returned) */
    /*-----------------------------------------------------------------*/
    if (!(VIDDISP_Data_p->LayerDisplay[Layer].ManualPresentation))
    {
        PanAndScanIndex = GetPanAndScanIndex(DisplayHandle, NextPicture_p, Layer);

        /* Test if picture characteristics have changed and inform world before putting field */
        /* Need to be done before call to PutxxxForNextVSync to be sure to have the right */
        /* picture parameters set to program display */
        CheckAndSendNewPictureCharacteristicsEvents(DisplayHandle, NextPictureBuff_p, Layer, PanAndScanIndex);

        /* set pointers and properties */
        VIDDISP_ShowPictureLayer(DisplayHandle,
                                NextPictureBuff_p,
                                Layer,
                                VIDDISP_SHOW_VIDEO);
        /* Temporary fix to avoid NULL pointer crash */
        if (VIDDISP_Data_p->DisplayQueue_p == NULL)
        {
            TrError(("Error! DisplayQueue_p is NULL!\r\n"));
            return;
        }

        /* set field */
        /* When changing direction the chosen field to be displayed will be the last one before changing the direction */
        if ((((FirstFieldOnDisplay == VIDDISP_FIELD_DISPLAY_TOP)
              ||((FirstFieldOnDisplay == VIDDISP_FIELD_DISPLAY_REPEAT)&&(NextPictureInfos_p->VideoParams.TopFieldFirst)))
             &&((VIDDISP_Data_p->DisplayForwardNotBackward && VIDDISP_Data_p->LastDisplayDirectionIsForward)
              ||(!VIDDISP_Data_p->DisplayForwardNotBackward && !VIDDISP_Data_p->LastDisplayDirectionIsForward)))
            ||((VIDDISP_Data_p->DisplayQueue_p->Counters[Layer].NextFieldOnDisplay == VIDDISP_FIELD_DISPLAY_TOP)
             &&((VIDDISP_Data_p->DisplayForwardNotBackward && !VIDDISP_Data_p->LastDisplayDirectionIsForward)
              ||(!VIDDISP_Data_p->DisplayForwardNotBackward && VIDDISP_Data_p->LastDisplayDirectionIsForward))))
        {
            if(VIDDISP_Data_p->DisplayRef == Layer)
            {
                PutTopForNextVSync(DisplayHandle, Layer, NextPicture_p);
                TraceVerbose(("Pict (Top) ExtDispPicID %d/%d PTS %lu prepared on %i\r\n",
                    NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                    NextPictureInfos_p->VideoParams.PTS.PTS,
                    Layer));
            }

            /* Be sure that te field we restart with will be displayed only once */
            if ((VIDDISP_Data_p->DisplayForwardNotBackward && !VIDDISP_Data_p->LastDisplayDirectionIsForward)||
                (!VIDDISP_Data_p->DisplayForwardNotBackward && VIDDISP_Data_p->LastDisplayDirectionIsForward))
            {
                if ((VIDDISP_Data_p->DisplayForwardNotBackward &&
                     NextPictureInfos_p->VideoParams.TopFieldFirst) ||
                    (!VIDDISP_Data_p->DisplayForwardNotBackward &&
                     !NextPictureInfos_p->VideoParams.TopFieldFirst))
                {
                        NextPicture_p->Counters[Layer].TopFieldCounter = 0;
                }
                else
                {
                    NextPicture_p->Counters[Layer].TopFieldCounter = 0;
                    NextPicture_p->Counters[Layer].BottomFieldCounter = 0;
                    NextPicture_p->Counters[Layer].RepeatFieldCounter = 0;
                }
            }
        }
        else
        {
            if(VIDDISP_Data_p->DisplayRef == Layer)
            {
                PutBottomForNextVSync(DisplayHandle, Layer, NextPicture_p);
                TraceVerbose(("Pict (Bot) ExtDispPicID %d/%d PTS %lu prepared on %i\r\n",
                        NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                        NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                        NextPictureInfos_p->VideoParams.PTS.PTS,
                        Layer));
            }
            /* Be sure that te field we restart with will be displayed only once */
            if ((VIDDISP_Data_p->DisplayForwardNotBackward && !VIDDISP_Data_p->LastDisplayDirectionIsForward)||
                (!VIDDISP_Data_p->DisplayForwardNotBackward && VIDDISP_Data_p->LastDisplayDirectionIsForward))
            {
                if ((VIDDISP_Data_p->DisplayForwardNotBackward &&
                    !NextPictureInfos_p->VideoParams.TopFieldFirst) ||
                    (!VIDDISP_Data_p->DisplayForwardNotBackward &&
                    NextPictureInfos_p->VideoParams.TopFieldFirst))
                {
                        NextPicture_p->Counters[Layer].BottomFieldCounter = 0;
                }
                else
                {
                    NextPicture_p->Counters[Layer].TopFieldCounter = 0;
                    NextPicture_p->Counters[Layer].BottomFieldCounter = 0;
                    NextPicture_p->Counters[Layer].RepeatFieldCounter = 0;
                }
            }
        }
    }/* ...All registers are OK, they will be latched on next VSync */
    else
    {
        TraceVerbose(("Pict ExtDispPicID %d/%d PTS %lu prepared on %i\r\n",
                NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                NextPictureInfos_p->VideoParams.PTS.PTS,
                Layer));
    }
    /* Frame by frame transition between BW and FW and reverse */
    if (VIDDISP_Data_p->DisplayForwardNotBackward)
    {
        VIDDISP_Data_p->LastDisplayDirectionIsForward = TRUE;
    }
    else
    {
        VIDDISP_Data_p->LastDisplayDirectionIsForward = FALSE;
    }

    /* Cancel MPEG display discontinuity */
    VIDDISP_Data_p->HasDiscontinuityInMPEGDisplay = FALSE;

    /* declare the new picture prepared as 'on display' */
    VIDBUFF_SetDisplayStatus(VIDDISP_Data_p->BufferManagerHandle,
                    VIDBUFF_ON_DISPLAY, NextPictureBuff_p);
    NextPicture_p->Counters[Layer].CounterInDisplay = VIDDISP_Data_p->LayerDisplay[Layer].FrameBufferHoldTime;
    /* Unlock display queue (needed if this function is called for unlocking at last minute) */
    if (Layer == VIDDISP_Data_p->DisplayRef)
    {
        VIDDISP_Data_p->LockState = UNLOCKED;
    }

    /* NextPictureBuff_p has been presented for Next VSync */
    PictureBuffPresentedToDisplay_p = NextPictureBuff_p;

    /* inform world : */
    CheckAndSendNewPictureEvents(DisplayHandle, PictureBuffPresentedToDisplay_p, Layer);
    TracePictureType(("DispType","%c%d",
       (NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I ? 'I' :
        NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P ? 'P' :
        NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B ? 'B' : '?'),
        NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID
    ));

#ifdef ST_speed
#ifdef STVID_DEBUG_GET_STATISTICS
    if(Layer == 0)
    {
        if ( NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)
        {
            VIDDISP_Data_p->StatisticsSpeedDisplayBFramesNb++;
        }
        if ( NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)
        {
            VIDDISP_Data_p->StatisticsSpeedDisplayPFramesNb++;
        }
        if ( NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)
        {
            VIDDISP_Data_p->StatisticsSpeedDisplayIFramesNb++;
        }
    }
#endif /* STVID_DEBUG_GET_STATISTICS */
#endif /*ST_speed*/
} /* End of PutNewPictureOnDisplayForNextVSync() function */

/*******************************************************************************
Name        : GetAppropriatePictureBufferForLayer
Description : This function takes as input a Picture_t element and a layer number.
              It returns the picture buffer that should be used on this layer (depending
              of the recommended reconstruction mode)
Parameters  :
Assumptions : Picture_p should be pointing to a valid PictureBuffer_p
Limitations :
Returns     : Null or a valid Picture Buffer pointer
*******************************************************************************/
VIDBUFF_PictureBuffer_t * GetAppropriatePictureBufferForLayer(const VIDDISP_Handle_t    DisplayHandle,
                                                              VIDDISP_Picture_t *       Picture_p,
                                                              U32                       Layer)
{
    VIDDISP_Data_t *            VIDDISP_Data_p;
    VIDBUFF_PictureBuffer_t *   PictureBuff_p = NULL;

    VIDDISP_Data_p      = (VIDDISP_Data_t *) DisplayHandle;

    if (Picture_p == NULL)
    {
        TrError(("Error! Invalid argument in GetAppropriatePictureBufferForLayer!\r\n"));
        return (NULL);
    }

    if ( (Picture_p->PrimaryPictureBuffer_p == NULL) &&
         (Picture_p->SecondaryPictureBuffer_p == NULL) )
    {
        TrError(("Critical Error! Invalid Picture element!\r\n"));
        return (NULL);
    }

    if (LAYER_NEEDS_DECIMATED_PICTURE(Layer))
    {
        /*** This layer needs a Decimated picture ***/
        if (Picture_p->SecondaryPictureBuffer_p != NULL)
        {
            PictureBuff_p = Picture_p->SecondaryPictureBuffer_p;
        }
        else
        {
            TrError(("Error! Decimated picture buffer not available when required!\r\n"));
            /* We have no other choice than using the primary picture */
            PictureBuff_p = Picture_p->PrimaryPictureBuffer_p;
        }
    }
    else
    {
        /*** This layer needs a Main picture ***/
        if (Picture_p->PrimaryPictureBuffer_p != NULL)
        {
            PictureBuff_p = Picture_p->PrimaryPictureBuffer_p;
        }
        else
        {
            TrError(("Error! Primary picture buffer not available when required!\r\n"));
            /* We have no other choice than using the secondary picture */
            PictureBuff_p = Picture_p->SecondaryPictureBuffer_p;
        }
    }

    return (PictureBuff_p);
}


/*******************************************************************************
Name        : CheckMPEGFrameCentreOffsetsChange
Description : Check if MPEG FrameCentreOffsets have changed and react by
              sending corresponding EVT
Parameters  : Display handle, picture information, field to consider the MPEG
              FrameCentreOffsets
Assumptions : Should be called only if layer==master !!
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CheckMPEGFrameCentreOffsetsChange(
            const VIDDISP_Handle_t DisplayHandle,
            const VIDCOM_FrameCropInPixel_t FrameCropInPixel,
            const VIDCOM_PanAndScanIn16thPixel_t * const PanAndScanIn16thPixel_p,
            const U8 PanAndScanIndex,
            U32 LayerIndex)
{
    VIDDISP_Data_t      * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    VIDDISP_PictureCharacteristicsChangeParams_t    CharacteristicsChangeParams;

    if ( (PanAndScanIn16thPixel_p[PanAndScanIndex].FrameCentreHorizontalOffset
                != VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.PanAndScanIn16thPixel.FrameCentreHorizontalOffset)
      || (PanAndScanIn16thPixel_p[PanAndScanIndex].FrameCentreVerticalOffset
                != VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.PanAndScanIn16thPixel.FrameCentreVerticalOffset))
    {
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.PanAndScanIn16thPixel = PanAndScanIn16thPixel_p[PanAndScanIndex];
        CharacteristicsChangeParams.CharacteristicsChanging = VIDDISP_CHANGING_ONLY_FRAME_CENTER_OFFSETS;
        CharacteristicsChangeParams.PictureInfos_p = NULL; /* ! in vid_comp ! */
        CharacteristicsChangeParams.FrameCropInPixel = FrameCropInPixel;
        CharacteristicsChangeParams.PanAndScanIn16thPixel = PanAndScanIn16thPixel_p[PanAndScanIndex];
        CharacteristicsChangeParams.LayerType  = VIDDISP_Data_p->LayerDisplay[LayerIndex].LayerType;

                STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                    VIDDISP_Data_p->RegisteredEventsID[VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT_ID],
                    STEVT_EVENT_DATA_TYPE_CAST &CharacteristicsChangeParams);
     }
} /* End of CheckMPEGFrameCentreOffsetsChange() function */

/*******************************************************************************
Name        : GetPanAndScanIndex
Description :
Parameters  : Display handle, picture, LayerIndex
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static U8 GetPanAndScanIndex(const VIDDISP_Handle_t DisplayHandle,
                                         VIDDISP_Picture_t *Picture_p, U32 LayerIndex)
{
    VIDDISP_Data_t *        VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    U8                      PanAndScanIndex = VIDCOM_PAN_AND_SCAN_INDEX_FIRST;
    STVID_PictureInfos_t *  NewPictureInfos_p = &(Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos);

    /* Determine which frame of the picture is to be displayed */
    /*-------------------------------------------------------- */
    if (!(VIDDISP_Data_p->DisplayForwardNotBackward))
    {
        if (NewPictureInfos_p->VideoParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN)
        {
            /* Backward progressive */
            if (Picture_p->Counters[LayerIndex].RepeatProgressiveCounter < VIDCOM_PAN_AND_SCAN_INDEX_REPEAT)
            {
                /* Next displaying first, second or third picture depending */
                /* on RepeatProgressiveCounter */
                PanAndScanIndex = Picture_p->Counters[LayerIndex].RepeatProgressiveCounter;
            }
            else
            {
                /* Next displaying third picture */
                PanAndScanIndex = VIDCOM_PAN_AND_SCAN_INDEX_REPEAT;
            }
        }
        else
        {
            /* Backward interlaced */
            if (Picture_p->Counters[LayerIndex].RepeatFieldCounter != 0)
            {
                /* Next displaying repeat field */
                PanAndScanIndex = VIDCOM_PAN_AND_SCAN_INDEX_REPEAT;
            }
            else if ((Picture_p->Counters[LayerIndex].TopFieldCounter != 0)
                   &&(Picture_p->Counters[LayerIndex].BottomFieldCounter !=0))
            {
                /* Next displaying second field of picture */
                PanAndScanIndex = VIDCOM_PAN_AND_SCAN_INDEX_SECOND;
            }
        }
    }

    return (PanAndScanIndex);
}


/*******************************************************************************
Name        : CheckAndSendNewCharacteristicsPictureEvents
Description : Checks if the picture Characteristics are changing since last one, and send
              corresponding events
Parameters  : Display handle, picture, LayerIndex
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CheckAndSendNewPictureCharacteristicsEvents(const VIDDISP_Handle_t DisplayHandle,
                                         VIDBUFF_PictureBuffer_t * NewPictureBuffer_p, U32 LayerIndex, U8 PanAndScanIndex)
{
    STVID_PictureInfos_t *                  NewPictureInfos_p = &(NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos);
    VIDDISP_PictureCharacteristicsChangeParams_t CharacteristicsChangeParams;
    const VIDCOM_PanAndScanIn16thPixel_t *  PanAndScanIn16thPixel_p;
    VIDDISP_Data_t *                        VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    BOOL                                    ChangeMPEGFrameCentreOffsets = FALSE;
#ifdef ST_crc
    BOOL                                    CurrentCRCCheckMode;
    STGXOBJ_ScanType_t                      CurrentCRCScanType;
#endif /* ST_crc */

    /* Find structure */
    CharacteristicsChangeParams.CharacteristicsChanging = VIDDISP_CHANGING_ONLY_FRAME_CENTER_OFFSETS;

    /* shortcuts */
    PanAndScanIn16thPixel_p = &(NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[PanAndScanIndex]);

    /* When pan and scan change, no need to notify that everything has changed
     * (should be treated apart, but this is not the case now, to be done later!!!!!) */
    if ((PanAndScanIn16thPixel_p->FrameCentreHorizontalOffset
                != VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.PanAndScanIn16thPixel.FrameCentreHorizontalOffset)
      ||(PanAndScanIn16thPixel_p->FrameCentreVerticalOffset
                != VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.PanAndScanIn16thPixel.FrameCentreVerticalOffset))
    {
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.PanAndScanIn16thPixel = *PanAndScanIn16thPixel_p;
        ChangeMPEGFrameCentreOffsets = TRUE;
    }
    else
    {
        ChangeMPEGFrameCentreOffsets = FALSE;
    }

#ifdef ST_crc
    if(VIDCRC_IsCRCRunning(VIDDISP_Data_p->CRCHandle))
    {
        CurrentCRCCheckMode = TRUE;

        /* Modif TD for VC1 */
        if(VIDCRC_IsCRCModeField(VIDDISP_Data_p->CRCHandle,
                                 &(NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos),
                                 NewPictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard))
        {
            CurrentCRCScanType = STGXOBJ_INTERLACED_SCAN;
        }
        else
        {
            CurrentCRCScanType = STGXOBJ_PROGRESSIVE_SCAN;
        }
    }
    else
    {
        CurrentCRCCheckMode = FALSE;
        CurrentCRCScanType = (STGXOBJ_ScanType_t) (-1);
    }
#endif /* ST_crc */

    /* First of all, notify layer about changes */
    if ( (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.AspectRatio != NewPictureInfos_p->BitmapParams.AspectRatio) ||
         (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.PictureFrameRate != NewPictureInfos_p->VideoParams.FrameRate) ||
         (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.ScanType != NewPictureInfos_p->VideoParams.ScanType) ||
         (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.ColorSpaceConversion != NewPictureInfos_p->BitmapParams.ColorSpaceConversion) ||
         (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.Reconstruction != NewPictureBuffer_p->FrameBuffer_p->AvailableReconstructionMode) ||
         (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.DecimationFactor != NewPictureInfos_p->VideoParams.DecimationFactors) ||
         (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.Height != NewPictureInfos_p->BitmapParams.Height) ||
         (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.Width != NewPictureInfos_p->BitmapParams.Width) ||
         (VIDDISP_Data_p->IsVTGChangedSinceLastVSync) ||
#ifdef ST_crc
         (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.CRCCheckMode != CurrentCRCCheckMode) ||
         (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.CRCScanType != CurrentCRCScanType) ||
#endif /* ST_crc */
         (ChangeMPEGFrameCentreOffsets)
       )
    {
#ifdef ST_crc
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.CRCCheckMode = CurrentCRCCheckMode;
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.CRCScanType = CurrentCRCScanType;
#endif /* ST_crc */

        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.ColorSpaceConversion
                    = NewPictureInfos_p->BitmapParams.ColorSpaceConversion;
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.Reconstruction
                    = NewPictureBuffer_p->FrameBuffer_p->AvailableReconstructionMode;
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.DecimationFactor
                    = NewPictureInfos_p->VideoParams.DecimationFactors;

        /* One of the characteristics is changing: send */
        /* VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT */
        CharacteristicsChangeParams.CharacteristicsChanging = VIDDISP_CHANGING_EVERYTHING;
        CharacteristicsChangeParams.PictureInfos_p = (STVID_PictureInfos_t *) NewPictureInfos_p;
        /* Intra video event : */
        TraceVerbose(("Caract changed (%i)\r\n",LayerIndex));
        CharacteristicsChangeParams.FrameCropInPixel = NewPictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel;
        CharacteristicsChangeParams.PanAndScanIn16thPixel = *PanAndScanIn16thPixel_p;
        CharacteristicsChangeParams.LayerType = VIDDISP_Data_p->LayerDisplay[LayerIndex].LayerType;
#ifdef ST_crc
        CharacteristicsChangeParams.CRCCheckMode = CurrentCRCCheckMode;
        CharacteristicsChangeParams.CRCScanType = CurrentCRCScanType;
#endif /* ST_crc */



        STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                    VIDDISP_Data_p->RegisteredEventsID[VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT_ID],
                    STEVT_EVENT_DATA_TYPE_CAST &CharacteristicsChangeParams);


        /* If VTG has changed, layer have already been notified, so reset the flag */
        VIDDISP_Data_p->IsVTGChangedSinceLastVSync = FALSE;
    }
} /* End of CheckAndSendNewPictureCharacteristicsEvents() function */


/*******************************************************************************
Name        : CheckAndSendNewPictureEvents
Description : Checks if the picture is changing since last one, and send
              corresponding events
Parameters  : Display handle, picture, LayerIndex
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CheckAndSendNewPictureEvents(const VIDDISP_Handle_t DisplayHandle,
                                         VIDBUFF_PictureBuffer_t * Picture_p, U32 LayerIndex)
{
     STVID_PictureInfos_t * CurrentPictureInfos_p
                        = &(Picture_p->ParsedPictureInformation.PictureGenericData.PictureInfos);
    STVID_PictureInfos_t ExportedPictureInfos;
    STVID_PictureParams_t ExportedPictureParams;
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    VIDBUFF_PictureBuffer_t * AssociatedPicture_p;


    /* Rebuild old picture params STVID_PictureParams_t */
    vidcom_PictureInfosToPictureParams(CurrentPictureInfos_p, &ExportedPictureParams,
            Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID);

    /* At this point DISPLAY is informed about all video parameters useful to display correctly current picture
     * Send event of current picture display parameters commit */
    STEVT_Notify(VIDDISP_Data_p->EventsHandle,
            VIDDISP_Data_p->RegisteredEventsID[VIDDISP_COMMIT_NEW_PICTURE_PARAMS_EVT_ID],
            STEVT_EVENT_DATA_TYPE_CAST NULL);

    /* Second notify application about changes*/

    /* Send events of new picture displayed */
    if ((IsEventToBeNotified (DisplayHandle, VIDDISP_DISPLAY_NEW_FRAME_EVT_ID))
     && (LayerIndex == VIDDISP_Data_p->DisplayRef))
    {
        STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                    VIDDISP_Data_p->RegisteredEventsID[VIDDISP_DISPLAY_NEW_FRAME_EVT_ID],
                    STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureParams));
    }
    if ((IsEventToBeNotified (DisplayHandle, VIDDISP_NEW_PICTURE_TO_BE_DISPLAYED_EVT_ID)) &&
        (LayerIndex == VIDDISP_Data_p->DisplayRef))
    {
        memcpy(&ExportedPictureInfos, CurrentPictureInfos_p, sizeof(STVID_PictureInfos_t));
        ExportedPictureInfos.BitmapParams.Data1_p = Virtual(ExportedPictureInfos.BitmapParams.Data1_p);
        ExportedPictureInfos.BitmapParams.Data2_p = Virtual(ExportedPictureInfos.BitmapParams.Data2_p);

        /* Setting Decimated Data before sending the event: */
        AssociatedPicture_p = Picture_p->AssociatedDecimatedPicture_p;
        if(AssociatedPicture_p != NULL)  /* The decimated picture exists */
        {
            if(ExportedPictureInfos.VideoParams.DecimationFactors == STVID_DECIMATION_FACTOR_NONE)
            {   /* Setting the right DecimationFactors before sending the event */
                ExportedPictureInfos.VideoParams.DecimationFactors
                        = AssociatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors;
            }

            ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data1_p = Virtual(ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data1_p);
            ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data2_p = Virtual(ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data2_p);
        }

        STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                VIDDISP_Data_p->RegisteredEventsID[VIDDISP_NEW_PICTURE_TO_BE_DISPLAYED_EVT_ID],
                STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureInfos));
    }

    /* aspect_ratio_information is changing: send */
    /* STVID_ASPECT_RATIO_CHANGE_EVT */
    if (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.AspectRatio
                                    != CurrentPictureInfos_p->BitmapParams.AspectRatio)
    {
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.AspectRatio
                        = CurrentPictureInfos_p->BitmapParams.AspectRatio;
        if (LayerIndex == VIDDISP_Data_p->DisplayRef)
        {
            /* export STVID_DisplayAspectRatio_t data (DDTS GNBvd36974) */
            STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                        VIDDISP_Data_p->RegisteredEventsID[VIDDISP_ASPECT_RATIO_CHANGE_EVT_ID],
                        STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureParams.Aspect));
        }
    }

    /* frame_rate_code is changing: send STVID_FRAME_RATE_CHANGE_EVT */
    if (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.PictureFrameRate
                        != CurrentPictureInfos_p->VideoParams.FrameRate)
    {
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.PictureFrameRate
                        = CurrentPictureInfos_p->VideoParams.FrameRate;
        if (LayerIndex == VIDDISP_Data_p->DisplayRef)
        {
            STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                        VIDDISP_Data_p->RegisteredEventsID[VIDDISP_FRAME_RATE_CHANGE_EVT_ID],
                        STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureParams));
        }
    }

    /* ScanType is changing: send STVID_SCAN_TYPE_CHANGE_EVT */
    if (VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.ScanType
                            != CurrentPictureInfos_p->VideoParams.ScanType)
    {
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.ScanType
                        = CurrentPictureInfos_p->VideoParams.ScanType;
        if (LayerIndex == VIDDISP_Data_p->DisplayRef)
        {
            STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                    VIDDISP_Data_p->RegisteredEventsID[VIDDISP_SCAN_TYPE_CHANGE_EVT_ID],
                    STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureParams));
        }
    }

    /* One of the sizes is changing: send STVID_RESOLUTION_CHANGE_EVT */
    if ((VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.Height
                        != CurrentPictureInfos_p->BitmapParams.Height)
      ||(VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.Width
                        != CurrentPictureInfos_p->BitmapParams.Width))
    {
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.Height
                    = CurrentPictureInfos_p->BitmapParams.Height;
        VIDDISP_Data_p->LayerDisplay[LayerIndex].DisplayCaract.Width
                    = CurrentPictureInfos_p->BitmapParams.Width;
        if (LayerIndex == VIDDISP_Data_p->DisplayRef)
        {
            STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                     VIDDISP_Data_p->RegisteredEventsID[VIDDISP_RESOLUTION_CHANGE_EVT_ID],
                    STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureParams));
        }
    }
} /* End of CheckAndSendNewPictureEvents() function */

#ifdef ST_crc
/*******************************************************************************
Name        : viddisp_LockCRCBuffer
Description : Lock the picture buffer used for CRC computation to be able to
              ensure picture buffers are still available on next vsync
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_LockCRCBuffer(VIDDISP_Data_t *  VIDDISP_Data_p, const LayerDisplayParams_t * LayerDisplay_p)
{
    /* TAKE/RELEASE of buffer is need only for SW mode and for frame dump */
#if defined(STVID_CRC_ALLOW_FRAME_DUMP) || defined(STVID_SOFTWARE_CRC_MODE)
   /* Release previous CRC picture buffer and take new one if picture has changed */
    if(VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p != LayerDisplay_p->CurrentlyDisplayedPicture_p)
    {
        if(VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p != NULL)
        {
            RELEASE_PICTURE_BUFFER(VIDDISP_Data_p->BufferManagerHandle,
                                                         VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p->PrimaryPictureBuffer_p,
                                                         VIDCOM_VIDDISP_MODULE_BASE);
            VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p = NULL;
        }

        if( TAKE_PICTURE_BUFFER(VIDDISP_Data_p->BufferManagerHandle,
                                                    LayerDisplay_p->CurrentlyDisplayedPicture_p->PrimaryPictureBuffer_p,
                                                    VIDCOM_VIDDISP_MODULE_BASE) == ST_NO_ERROR)
        {
            VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p = LayerDisplay_p->CurrentlyDisplayedPicture_p;
            VIDDISP_Data_p->AreCRCParametersAvailable = TRUE;
        }
        else
        {
            VIDDISP_Data_p->AreCRCParametersAvailable = FALSE;
        }
    }
#else   /* otherwise do nothing */
    UNUSED_PARAMETER(VIDDISP_Data_p);
    UNUSED_PARAMETER(LayerDisplay_p);
#endif /* defined(STVID_CRC_ALLOW_FRAME_DUMP) || defined(STVID_SOFTWARE_CRC_MODE) */
}

/*******************************************************************************
Name        : viddisp_UnlockCRCBuffer
Description : Unlock the previously locked buffer used for CRC
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddisp_UnlockCRCBuffer(VIDDISP_Data_t *  VIDDISP_Data_p)
{
    /* TAKE/RELEASE of buffer is need only for SW mode and for frame dump */
#if defined(STVID_CRC_ALLOW_FRAME_DUMP) || defined(STVID_SOFTWARE_CRC_MODE)
    /* Release previous CRC picture buffer if needed */
    if(VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p != NULL)
    {
        RELEASE_PICTURE_BUFFER(VIDDISP_Data_p->BufferManagerHandle,
                                                     VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p->PrimaryPictureBuffer_p,
                                                     VIDCOM_VIDDISP_MODULE_BASE);
        VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p = NULL;
    }
#else   /* otherwise do nothing */
    UNUSED_PARAMETER(VIDDISP_Data_p);
#endif /* defined(STVID_CRC_ALLOW_FRAME_DUMP) || defined(STVID_SOFTWARE_CRC_MODE) */
}

/*******************************************************************************
Name        : viddisp_CheckCRCFieldRepeating
Description : Check and store result of field repeating that will be used for CRC checking
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_CheckCRCFieldRepeating(VIDDISP_Data_t *  VIDDISP_Data_p, LayerDisplayParams_t * LayerDisplay_p)
{
    BOOL  IsCRCModeField;

    /* Determine ScanType used for CRC */
    IsCRCModeField = VIDCRC_IsCRCModeField(VIDDISP_Data_p->CRCHandle,
                                                                         &(LayerDisplay_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos),
                                                                         LayerDisplay_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard);
    /* Update Displayed Counters */
    if(IsCRCModeField)
    {
        if(LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].CurrentFieldOnDisplay == VIDDISP_FIELD_DISPLAY_TOP)
        {
            LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].TopFieldDisplayedCounter++;
        }
        if(LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].CurrentFieldOnDisplay == VIDDISP_FIELD_DISPLAY_BOTTOM)
        {
            LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].BottomFieldDisplayedCounter++;
        }
    }
    else
    {
        LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].FrameDisplayDisplayedCounter++;
    }
    /* Determine if picture has been repeated */

    VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastPicture = FALSE;
    VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastButOnePicture = FALSE;
    if(IsCRCModeField)
    {
        if(LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].CurrentFieldOnDisplay == VIDDISP_FIELD_DISPLAY_TOP)
        {   /* Top field picture */
            if(LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].TopFieldDisplayedCounter > 1)
            {
                if(LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].PreviousFieldOnDisplay == VIDDISP_FIELD_DISPLAY_TOP)
                {
                    VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastPicture = TRUE;
                }
                else
                {
                    VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastButOnePicture = TRUE;
                }
            }
        }
        else if(LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].CurrentFieldOnDisplay == VIDDISP_FIELD_DISPLAY_BOTTOM)
        {   /* Bottom field picture */
            if(LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].BottomFieldDisplayedCounter > 1)
            {
                if(LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].PreviousFieldOnDisplay == VIDDISP_FIELD_DISPLAY_BOTTOM)
                {
                    VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastPicture = TRUE;
                }
                else
                {
                    VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastButOnePicture = TRUE;
                }
            }
        }
    }
    else
    {
        if(LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].FrameDisplayDisplayedCounter > 1)
        {
            VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastPicture = TRUE;
        }
    }
    LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].PreviousFieldOnDisplay = LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[0].CurrentFieldOnDisplay;
}

/*******************************************************************************
Name        : viddisp_ProcessCRC
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_ProcessCRC(VIDDISP_Data_t *  VIDDISP_Data_p, LayerDisplayParams_t * LayerDisplay_p)
{
    /* Spec: The Y_CRC_CHECK_SUM, UV_CRC_CHECK_SUM registers are updated at the end of the current video pipe
    processing when the last pixel of the last line has been written in the pixel FIFO.
    The value in these registers is available until end of processing of the next field.

    It means that the values we read here are the CRCs for the picture presented at VSYNC-2 .

            VSYNC                      VSYNC+1                          VSYNC+2
        -----|----------------------------|-------------------------------|------------------------
             | Picture N is presented     | Field N is displayed via      | CRC of Field N is
                (PutFieldForNextVSync)        pixel fifo                       available
                                          | CurrentlyDisplayedPicture     | CurrentlyDisplayedPicture
                                             points to Field N                points to Field N+1

    (Use a fixed font to see correctly this graph)
 */

    if (VIDDISP_Data_p->AreCRCParametersAvailable)
    {
            /* First check current CRC values with the related picture information */
            if(VIDCRC_IsCRCCheckRunning(VIDDISP_Data_p->CRCHandle))
            {
                /* Register computed CRC and check against reference */
                VIDCRC_CheckCRC(VIDDISP_Data_p->CRCHandle,
                                                &(VIDDISP_Data_p->PictureUsedForCRCParameters.PictureInfos),
                                                VIDDISP_Data_p->PictureUsedForCRCParameters.ExtendedPresentationOrderPictureID,
                                                VIDDISP_Data_p->PictureUsedForCRCParameters.DecodingOrderFrameID,
                                                VIDDISP_Data_p->PictureUsedForCRCParameters.PictureNumber,
                                                VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastPicture,
                                                VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastButOnePicture,
#if defined(STVID_CRC_ALLOW_FRAME_DUMP) || defined(STVID_SOFTWARE_CRC_MODE)
                                                VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p->PrimaryPictureBuffer_p->FrameBuffer_p->Allocation.Address_p,
                                                VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p->PrimaryPictureBuffer_p->FrameBuffer_p->Allocation.Address2_p,
#else
                                                NULL,
                                                NULL,
#endif
                                                STSYS_ReadRegDev32LE(CRC_LUMA_BASE_ADDRESS),
                                                STSYS_ReadRegDev32LE(CRC_CHROMA_BASE_ADDRESS));
            }

            /* Save current CRC infos in the CRC queue if needed */
            if(VIDCRC_IsCRCQueueingRunning(VIDDISP_Data_p->CRCHandle))
            {
                if(VIDDISP_Data_p->PictureUsedForCRCParameters.PictureNumber != VIDDISP_Data_p->PreviousCRCPictureNumber)   /* Store only new CRC */
                {
                    VIDDISP_Data_p->PictureUsedForCRCParameters.CRCDataToEnqueue.LumaCRC= STSYS_ReadRegDev32LE(CRC_LUMA_BASE_ADDRESS);
                    VIDDISP_Data_p->PictureUsedForCRCParameters.CRCDataToEnqueue.ChromaCRC= STSYS_ReadRegDev32LE(CRC_CHROMA_BASE_ADDRESS);
                    VIDCRC_CRCEnqueue(VIDDISP_Data_p->CRCHandle, VIDDISP_Data_p->PictureUsedForCRCParameters.CRCDataToEnqueue);

                    VIDDISP_Data_p->PreviousCRCPictureNumber = VIDDISP_Data_p->PictureUsedForCRCParameters.PictureNumber ;
                }
            }
    }

    /* if a picture is on the display and CRC is running then store current picture data for next VSYNC CRC value */
    if (LayerDisplay_p->CurrentlyDisplayedPicture_p != NULL && VIDCRC_IsCRCRunning(VIDDISP_Data_p->CRCHandle))
    {
        viddisp_CheckCRCFieldRepeating(VIDDISP_Data_p, LayerDisplay_p);

        /* Save current picture parameters that will be used at next VSYNC for CRC checking */
        /* Need to copy the useful picture parameters locally because  */
        /* CurrentlyDisplayedPicture_p content can be incorrect on next VSYNC */
        VIDDISP_Data_p->PictureUsedForCRCParameters.PictureInfos = LayerDisplay_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos;
        VIDDISP_Data_p->PictureUsedForCRCParameters.ExtendedPresentationOrderPictureID = LayerDisplay_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
        VIDDISP_Data_p->PictureUsedForCRCParameters.DecodingOrderFrameID = LayerDisplay_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID;
        VIDDISP_Data_p->PictureUsedForCRCParameters.PictureNumber = LayerDisplay_p->CurrentlyDisplayedPicture_p->PrimaryPictureBuffer_p->PictureNumber;

        VIDDISP_Data_p->PictureUsedForCRCParameters.CRCDataToEnqueue.IsReferencePicture = LayerDisplay_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.IsReference;
        VIDDISP_Data_p->PictureUsedForCRCParameters.CRCDataToEnqueue.PictureOrderCount  =  LayerDisplay_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID;
        VIDDISP_Data_p->PictureUsedForCRCParameters.CRCDataToEnqueue.IsFieldBottomNotTop = (LayerDisplay_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD);
        VIDDISP_Data_p->PictureUsedForCRCParameters.CRCDataToEnqueue.IsFieldCRC = VIDCRC_IsCRCModeField(VIDDISP_Data_p->CRCHandle,
                                                                                                                                                                               &(LayerDisplay_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos),
                                                                                                                                                                               LayerDisplay_p->CurrentlyDisplayedPicture_p->PictureInformation_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard);

        VIDDISP_Data_p->AreCRCParametersAvailable = TRUE;

        viddisp_LockCRCBuffer(VIDDISP_Data_p, LayerDisplay_p);
    }
    else
    {   /* CRC is stopped because either no more picture is displayed or CRC has been disabled */
        viddisp_UnlockCRCBuffer(VIDDISP_Data_p);

        VIDDISP_Data_p->AreCRCParametersAvailable = FALSE;
        VIDDISP_Data_p->PreviousCRCPictureNumber = -1;
    }
}

#endif /* ifdef ST_crc */


/*******************************************************************************
Name        : SynchronizeDisplay
Description : Take actions on VSync, (under display task context)
Parameters  :
Assumptions : To be called on each VSync under task context
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SynchronizeDisplay(const VIDDISP_Handle_t DisplayHandle,
                               U32 Layer)
{
    LayerDisplayParams_t      * LayerDisplay_p;
    VIDDISP_Data_t *            VIDDISP_Data_p;
    VIDDISP_Picture_t *         CurrentPicture_p;
    VIDDISP_Picture_t *         NextDisplayedPicture_p;
    VIDDISP_FieldDisplay_t      NextFieldOnDisplay;
    VIDDISP_VsyncDisplayNewPictureParams_t  VsyncDisplayNewPictureParams;
    VIDDISP_DisplayState_t      RememberState;
    ST_ErrorCode_t              Error;
    U8                          OldRepeatProgressiveCounter;
    osclock_t                   TimeNow;
    STVID_PictureInfos_t *      CurrentPictureInfos_p;
    STVID_PictureParams_t       ExportedPictureParams;
    U8                          RemainingFields;

    /* shortcuts */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplay_p = &(VIDDISP_Data_p->LayerDisplay[Layer]);
    RememberState = VIDDISP_Data_p->DisplayState; /* Needed only to avoid GCC warning */

    TraceVerbose(("\r\n VSYNC (%i),(%s)\r\n",Layer,
                LayerDisplay_p->CurrentVSyncTop? "Top" : "Bot"));
    TraceVsync(("VSync", LayerDisplay_p->CurrentVSyncTop ? "Top" : "Bot"));
#if defined TRACE_DISPLAY
    if ((LayerDisplay_p == NULL) ||
        (LayerDisplay_p->CurrentlyDisplayedPicture_p == NULL) ||
        ( (LayerDisplay_p->CurrentlyDisplayedPicture_p->PrimaryPictureBuffer_p == NULL) && (LayerDisplay_p->CurrentlyDisplayedPicture_p->SecondaryPictureBuffer_p == NULL) ) )
    {
        TraceVerbose(("NoPic\r\n"));
    }
    else
    {
        CurrentPicture_p = LayerDisplay_p->CurrentlyDisplayedPicture_p;
        TraceVerbose(("Cur(%c%d~%d%c)\r\n",(CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B?'B':
                                ((CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P?'P':'I'))),
                                  CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                  CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                 (CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD?'T':
                                ((CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD?'B':'F')))
        ));
    }
#endif /* TRACE_DISPLAY */

#ifdef ST_crc
    if(Layer == 0)
    {   /* Only main display has CRC IP attached on 7100/7109 */
        viddisp_ProcessCRC(VIDDISP_Data_p, LayerDisplay_p);
    }
#endif /* ST_crc */

    if ((Layer == VIDDISP_Data_p->DisplayRef) && (LayerDisplay_p->NewPictureDisplayedOnCurrentVSync))
    {
            TimeNow = time_now();
            /* shortcut: */
            CurrentPicture_p = LayerDisplay_p->CurrentlyDisplayedPicture_p;
            /* send evt for av sync only for ref display */
            VsyncDisplayNewPictureParams.PictureInfos_p
                       = &(CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos);
            VsyncDisplayNewPictureParams.TimeElapsedSinceVSyncAtNotification
                       = ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION
                       * ((U32) time_minus(TimeNow,LayerDisplay_p->TimeAtVSyncNotif)))
                      / ST_GetClocksPerSecond());
            if ((LayerDisplay_p->VTGVSyncFrequency == 0)
            || (VsyncDisplayNewPictureParams.TimeElapsedSinceVSyncAtNotification
                >= (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION * 1000
                       / LayerDisplay_p->VTGVSyncFrequency)))
            {
                /* avoid a stupid value ! */
                VsyncDisplayNewPictureParams.TimeElapsedSinceVSyncAtNotification = 0;
            }
            VsyncDisplayNewPictureParams.OutputFrameRate
                    = LayerDisplay_p->VTGVSyncFrequency;
            VsyncDisplayNewPictureParams.IsDisplayedWithCareOfPolarity
                    = VIDDISP_Data_p->DisplayWithCareOfFieldsPolarity &&
                 VIDDISP_Data_p->FrameRateConversionNeedsCareOfFieldsPolarity;
            VsyncDisplayNewPictureParams.IsAnyFrameRateConversionActivated
                    = CurrentPicture_p->Counters[Layer].IsFRConversionActivated;
            if (VIDDISP_Data_p->LayerDisplay[Layer].VTGVSyncFrequency != 0)
            {
                /* Delay introduced by the Display (expressed in 90kHz units) */
                VsyncDisplayNewPictureParams.DisplayDelay = (90000 * 1000) / VIDDISP_Data_p->LayerDisplay[Layer].VTGVSyncFrequency;
            }
            else
            {
                VsyncDisplayNewPictureParams.DisplayDelay =0;
            }

            STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                         VIDDISP_Data_p->RegisteredEventsID[VIDDISP_VSYNC_DISPLAY_NEW_PICTURE_EVT_ID],
                         STEVT_EVENT_DATA_TYPE_CAST &(VsyncDisplayNewPictureParams));

    } /* end if display is the ref and current picture is new */

    /* Free the unused pictures */
    /*--------------------------*/
    viddisp_PurgeQueues(Layer, DisplayHandle);

    /* Sequence the slave display: */
    /*-----------------------------*/
    if (VIDDISP_Data_p->DisplayRef != Layer) /* slave display only */
    {
        /* Only allow slave display to be updated if all viewport parameters are correctly set */
        if(VIDDISP_Data_p->IsSlaveSequencerAllowedToRun)
        {
            SequenceSlave(Layer, DisplayHandle);
        }
        return;
    }

    /* from this point : we sequence the reference display */
    /* (slave has returned) */

    /* Sequence the reference display: */
    /*---------------------------------*/

    /* shortcut: */
    CurrentPicture_p = LayerDisplay_p->CurrentlyDisplayedPicture_p;

    /* Stop request */
    if (VIDDISP_Data_p->StopParams.StopRequestIsPending)
    {
        /* When paused and requesting stop, freeze display */
        if (VIDDISP_Data_p->DisplayState == VIDDISP_DISPLAY_STATE_PAUSED)
        {
            VIDDISP_Data_p->DisplayState = VIDDISP_DISPLAY_STATE_FREEZED;
        }

        /* Test if current pict satisfy a stop condition */
        FindPictWhichMustBeStopped(DisplayHandle, CurrentPicture_p);
        /* When  requested picture will never be found, freeze display immediatly */
        if (VIDDISP_Data_p->DisplayQueue_p == NULL)
        {
            VIDDISP_Data_p->StopParams.StopRequestIsPending     = FALSE;
            if (VIDDISP_Data_p->StopParams.StopMode != VIDDISP_STOP_NOW)
            {
                VIDDISP_Data_p->NotifyStopWhenFinish    = TRUE;
            }
            VIDDISP_Data_p->DisplayState = VIDDISP_DISPLAY_STATE_FREEZED;
            TrStop(("Display freezed !! \r\n"));
        }

        if (!VIDDISP_Data_p->StopParams.StopRequestIsPending)
        {
            if(VIDDISP_Data_p->DoReleaseDeiReferencesOnFreeze == TRUE)
            {
                /* The Display has been stopped */
                /* Release all DEI references */
                viddisp_UnlockAllFields(DisplayHandle, 0);
                viddisp_UnlockAllFields(DisplayHandle, 1);
            }
            STOS_SemaphoreSignal(VIDDISP_Data_p->StopParams.ActuallyStoppedSemaphore_p);
        }
    }
    /* test freeze/pause condition  except for decode once mode & display locked */
    if (!((VIDDISP_Data_p->LockState == LOCKED)&&(VIDDISP_Data_p->HasDiscontinuityInMPEGDisplay )))
    {
        /* Test if current field satisfy a freeze/pause condition */
        if ((VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending) ||
            (VIDDISP_Data_p->FreezeParams.PauseRequestIsPending))
        {
            FindFieldWhichMustBeFrozen(DisplayHandle,CurrentPicture_p);
        }
    }
    /* remember the display state and patch it to do a 'step' if needed */
    if (VIDDISP_Data_p->FreezeParams.SteppingOrderReceived)
    {
        RememberState                   = VIDDISP_Data_p->DisplayState;
        VIDDISP_Data_p->LockState       = UNLOCKED;
        VIDDISP_Data_p->DisplayState    = VIDDISP_DISPLAY_STATE_DISPLAYING;
        /* this will do a step, DisplayState will be restored later */
    }

    /* Sequencement depends on the display state : */

    /* STATE == PAUSED */
    /*-----------------*/
    if (VIDDISP_Data_p->DisplayState == VIDDISP_DISPLAY_STATE_PAUSED)
    {
        FreezedDisplay(VIDDISP_Data_p);
        /* nothing to do, the display is paused */
    }

    /* STATE == FREEZED */
    /*------------------*/
    else if (VIDDISP_Data_p->DisplayState == VIDDISP_DISPLAY_STATE_FREEZED)
    {
        if (VIDDISP_Data_p->NotifyStopWhenFinish)
        {
            VIDDISP_Data_p->NotifyStopWhenFinish    = FALSE;

            if (CurrentPicture_p != NULL)
            {
                /* Rebuild old picture params STVID_PictureParams_t */
                vidcom_PictureInfosToPictureParams(&(CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos),
                        &ExportedPictureParams,
                        CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID);
                STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                         VIDDISP_Data_p->RegisteredEventsID[VIDDISP_OUT_OF_PICTURES_EVT_ID],
                         STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureParams));
            }
            else
            {
                STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                         VIDDISP_Data_p->RegisteredEventsID[VIDDISP_OUT_OF_PICTURES_EVT_ID],
                         STEVT_EVENT_DATA_TYPE_CAST NULL);
            }

        }
        FreezedDisplay(VIDDISP_Data_p);
        /* decode still running : decrement counters of next to keep a good */
        /* sequencement, (ref display only) */
        if ((CurrentPicture_p != NULL)&&(CurrentPicture_p->Next_p != NULL))
        {
            if (VIDDISP_Data_p->DisplayForwardNotBackward)
            {
                viddisp_DecrementFieldCounterForward(CurrentPicture_p->Next_p,
                                         &NextFieldOnDisplay,Layer);
            }
            else
            {
                viddisp_DecrementFieldCounterBackward(CurrentPicture_p->Next_p,
                                        &NextFieldOnDisplay,Layer);
            }
            if (NextFieldOnDisplay == VIDDISP_FIELD_DISPLAY_NONE)
            {
                viddisp_RemovePictureFromDisplayQueue(DisplayHandle, CurrentPicture_p->Next_p);
            }
        }
        /* to be sure next restart/resume will be done in right order: */
        VIDDISP_Data_p->LockState = LOCKED;
        /* to be sure that the next unlock of the display will be done */
        /* correctly */
    }

    /* STATE == DISPLAYING */
    /*---------------------*/
    else /* eq : display state is displaying */
    {
        /* if this sequence is just a 'step'... */
        if (VIDDISP_Data_p->FreezeParams.SteppingOrderReceived)
        {
            VIDDISP_Data_p->DisplayState              = RememberState;
            VIDDISP_Data_p->FreezeParams.Freeze.Field = STVID_FREEZE_FIELD_CURRENT;
        }

        /* try to start the pump if not started */
        /*-------------------------------------*/
        if ((CurrentPicture_p == NULL) || (VIDDISP_Data_p->LockState == LOCKED))
        {
            ReStartQueues(CurrentPicture_p,Layer,DisplayHandle);
            return; /* that's all for this VSync */
        }

        CurrentPictureInfos_p = &(CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos);



        /* the pump was already started : continue */
        /*-----------------------------------------*/
        OldRepeatProgressiveCounter
                  = CurrentPicture_p->Counters[Layer].RepeatProgressiveCounter;
        if (VIDDISP_Data_p->DisplayForwardNotBackward)
        {
            Error = viddisp_DecrementFieldCounterForward(CurrentPicture_p, &NextFieldOnDisplay,Layer);
        }
        else
        {
            Error = viddisp_DecrementFieldCounterBackward(CurrentPicture_p, &NextFieldOnDisplay,Layer);
        }
        if (Error)
        {
            /* issue, or field doesn't match vtg (may occure, not a bug) */
            TraceDisplay(("Counter issue !! \r\n"));
            /* (counters of current picture were already null) */
            /* we prefer patch the next field to take the next pict */
            NextFieldOnDisplay = VIDDISP_FIELD_DISPLAY_NONE;
        }

#if defined(TRACE_VISUAL_UART)
        TraceState("NextFld", "%d", NextFieldOnDisplay);
#endif

        switch(NextFieldOnDisplay)
        {
            case VIDDISP_FIELD_DISPLAY_SAME:
                TraceDisplay(("Display %i keep same field \r\n",Layer));
                /* for blitter-display : we ask again the field */
                if (CurrentPicture_p->Counters[Layer].CurrentFieldOnDisplay
                        == VIDDISP_FIELD_DISPLAY_TOP)
                {
                    PutTopForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
                }
                else
                {
                    PutBottomForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
                }

                break;
            case VIDDISP_FIELD_DISPLAY_NONE:
                /* Set display status to "last field on display" to allow
                rewrite on the current framebuffer for platforms with overwrite
                feature */
                if (CurrentPicture_p->PrimaryPictureBuffer_p != NULL)
                {
                    VIDBUFF_SetDisplayStatus(
                                      VIDDISP_Data_p->BufferManagerHandle,
                                      VIDBUFF_LAST_FIELD_ON_DISPLAY,
                                      CurrentPicture_p->PrimaryPictureBuffer_p);
                }
                if (CurrentPicture_p->SecondaryPictureBuffer_p != NULL)
                {
                    VIDBUFF_SetDisplayStatus(
                                      VIDDISP_Data_p->BufferManagerHandle,
                                      VIDBUFF_LAST_FIELD_ON_DISPLAY,
                                      CurrentPicture_p->SecondaryPictureBuffer_p);
                }

                /* picture is finished for this display : */
                /* the current display can do a step for next VSync */
                /*--------------------------------------------------*/
                if(CurrentPicture_p->Next_p != NULL)
                {
                    /* Check if the frame rate conversion patched the display counters to skip the next picture */
                    RemainingFields = RemainingFieldsToDisplay(*(CurrentPicture_p->Next_p));
                    if(RemainingFields == 0)
                    {
                        if(CurrentPicture_p->Next_p->Next_p != NULL)
                        {
                            /* No need to check that remaining fields for the Next_p->Next_p is different from
                             * zero, because the frame rate conversion never removes two consecutive pictures */
                            NextDisplayedPicture_p = CurrentPicture_p->Next_p->Next_p;
                        }
                        else
                        {
                            NextDisplayedPicture_p = CurrentPicture_p;
                        }
                    }
                    else
                    {
                        NextDisplayedPicture_p = CurrentPicture_p->Next_p;
                    }
#if 0
                    /* OLO_TO_DO When Multi Field presentation is enabled we should ensure that the NextDisplayedPicture has a Next Picture */
                    if (NextDisplayedPicture_p->Next_p == NULL)
                    {
                        TrMain(("Layer %d. No next-next Pict\r\n", Layer));
                        NextDisplayedPicture_p = CurrentPicture_p;
                    }
#endif
                }
                else
                {
                    NextDisplayedPicture_p = CurrentPicture_p;
                    TrMain(("Layer %d. No next Pict!\r\n", Layer));
                }
                /* Check if a new picture is available in the display queue, and put in on display */
                if(NextDisplayedPicture_p != CurrentPicture_p)
                {
                    PutNewPictureOnDisplayForNextVSync( DisplayHandle,
                                                    NextDisplayedPicture_p,
                                                    Layer);
                }
                else
                {
                    TrWarning(("No next pict avail\r\n"));
                    TraceDisplay(("Display %i keep same field \r\n",Layer));
                    /* for blitter-display : we ask again the field */
                    if (CurrentPicture_p->Counters[Layer].CurrentFieldOnDisplay
                            == VIDDISP_FIELD_DISPLAY_TOP)
                    {
                        PutTopForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
                    }
                    else
                    {
                        PutBottomForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
                    }
                    if (VIDDISP_Data_p->LockState == UNLOCKED)
                    {
                        TraceDisplay(("Display Queue Lock Next \r\n"));
                        VIDDISP_Data_p->LockState       = LOCK_NEXT;
                        VIDDISP_Data_p->ForceNextPict   = TRUE;
                    }
                    else
                    {
                        /* Too late : lock the queue to restart it properly */
                        VIDDISP_Data_p->LockState       = LOCKED;
                        VIDDISP_Data_p->ForceNextPict   = FALSE;
                        /* to be sure that the next unlock of the display will be */
                        /* done correctly */
                        TraceDisplay(("Display Queue already locked !!\r\n"));
#ifdef STVID_DEBUG_GET_STATISTICS
                    VIDDISP_Data_p->StatisticsPbQueueLockedByLackOfPicture++;
                    /*VIDBUFF_PrintPictureBuffersStatus(VIDDISP_Data_p->BufferManagerHandle); Useful function to help debugging */
#endif /* STVID_DEBUG_GET_STATISTICS */
                    }
                } /* end if no next picture, (has returned); else continue : */
                break; /* end case VIDDISP_FIELD_DISPLAY_NONE */

            case VIDDISP_FIELD_DISPLAY_TOP:
                PutTopForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
                if (CurrentPictureInfos_p->VideoParams.ScanType != STGXOBJ_PROGRESSIVE_SCAN)
                {
                    if (CurrentPictureInfos_p->VideoParams.TopFieldFirst)
                    {
                        CheckMPEGFrameCentreOffsetsChange(DisplayHandle,
                            CurrentPicture_p->PictureInformation_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel,
                            CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel,
                            VIDCOM_PAN_AND_SCAN_INDEX_FIRST, Layer);
                    }
                    else
                    {
                        CheckMPEGFrameCentreOffsetsChange(DisplayHandle,
                            CurrentPicture_p->PictureInformation_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel,
                            CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel,
                            VIDCOM_PAN_AND_SCAN_INDEX_SECOND, Layer);
                    }
                }
                break;

            case VIDDISP_FIELD_DISPLAY_BOTTOM:
                PutBottomForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
                if (CurrentPictureInfos_p->VideoParams.ScanType != STGXOBJ_PROGRESSIVE_SCAN)
                {
                    if (!(CurrentPictureInfos_p->VideoParams.TopFieldFirst))
                    {
                        CheckMPEGFrameCentreOffsetsChange(DisplayHandle,
                            CurrentPicture_p->PictureInformation_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel,
                            CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel,
                            VIDCOM_PAN_AND_SCAN_INDEX_FIRST, Layer);
                    }
                    else
                    {
                        CheckMPEGFrameCentreOffsetsChange(DisplayHandle,
                            CurrentPicture_p->PictureInformation_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel,
                            CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel,
                            VIDCOM_PAN_AND_SCAN_INDEX_SECOND, Layer);
                    }
                }
                break;

            case VIDDISP_FIELD_DISPLAY_REPEAT:
                if (CurrentPictureInfos_p->VideoParams.TopFieldFirst)
                {
                    PutTopForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
                }
                else
                {
                    PutBottomForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
                }
                if (CurrentPictureInfos_p->VideoParams.ScanType != STGXOBJ_PROGRESSIVE_SCAN)
                {
                    CheckMPEGFrameCentreOffsetsChange(DisplayHandle,
                        CurrentPicture_p->PictureInformation_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel,
                        CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel,
                        VIDCOM_PAN_AND_SCAN_INDEX_REPEAT, Layer);
                }
                break;
        }/* end switch next field on display */

        /* Update frame center offsets if required */
        if ((CurrentPictureInfos_p->VideoParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN)
         &&(OldRepeatProgressiveCounter
             != CurrentPicture_p->Counters[Layer].RepeatProgressiveCounter))
        {
            if (!(VIDDISP_Data_p->DisplayForwardNotBackward))
            {   /* Backward */
                CheckMPEGFrameCentreOffsetsChange(DisplayHandle,
                    CurrentPicture_p->PictureInformation_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel,
                    CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel,
                    CurrentPicture_p->Counters[Layer].RepeatProgressiveCounter,
                    Layer);
            }
            else
            {   /* Forward */
                CheckMPEGFrameCentreOffsetsChange(DisplayHandle,
                    CurrentPicture_p->PictureInformation_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel,
                    CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel,
                    CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.RepeatProgressiveCounter
                       - CurrentPicture_p->Counters[Layer].RepeatProgressiveCounter,
                    Layer);
            }
        }
    } /* end state displaying ('else') */
    if (VIDDISP_Data_p->FreezeParams.SteppingOrderReceived)
    {
        /* clear the 'step-cmd' and restore the display state */
        VIDDISP_Data_p->FreezeParams.SteppingOrderReceived  = FALSE;
    }
} /* End of SynchronizeDisplay() function */

/*******************************************************************************
Name        : ReStartQueues
Description : initiate the queue extraction (called if nothing is on display)
*******************************************************************************/
static void ReStartQueues(VIDDISP_Picture_t * CurrentPicture_p, U32 Layer,
                          VIDDISP_Handle_t DisplayHandle)
{
    LayerDisplayParams_t *      LayerDisplay_p;
    VIDDISP_Data_t *            VIDDISP_Data_p;
    VIDDISP_Picture_t *         PictureToRestartFrom_p;

    assert(DisplayHandle != NULL);

    /* shortcuts */
    VIDDISP_Data_p  = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplay_p = &(VIDDISP_Data_p->LayerDisplay[Layer]);

    assert(LayerDisplay_p != NULL);

    /* OK, from this point, we can try to start the display queue extraction */
    /*-----------------------------------------------------------------------*/

    /* Case : it is the first start (the display is black) */
    if (CurrentPicture_p == NULL)
    {
        if (VIDDISP_Data_p->DisplayQueue_p != NULL)
        {
            VIDDISP_Data_p->LockState = UNLOCKED;
            TraceDisplay(("First pict in display %d \r\n",Layer));
            PutNewPictureOnDisplayForNextVSync(DisplayHandle,
                                    VIDDISP_Data_p->DisplayQueue_p,Layer);
        }
    }
    /* Case : it is a re-start */
    else if (CurrentPicture_p->Next_p != NULL) /* Restart normal with next */
    {
        VIDDISP_Data_p->LockState = UNLOCKED;
        TraceDisplay(("Restart display %d \r\n",Layer));
        PutNewPictureOnDisplayForNextVSync(DisplayHandle,
                                        CurrentPicture_p->Next_p,
                                        Layer);
    }
    else
    {
        /* Check if the currently displayed picture is still in the display queue */
        PictureToRestartFrom_p = VIDDISP_Data_p->DisplayQueue_p;

        if (PictureToRestartFrom_p != NULL)
        {
            while(  (PictureToRestartFrom_p->Next_p != NULL)
                &&(VIDDISP_Data_p->LayerDisplay[Layer].CurrentlyDisplayedPicture_p != PictureToRestartFrom_p))
            {
                PictureToRestartFrom_p = PictureToRestartFrom_p->Next_p;
            }
            /* Try to restart from the next picture to the currently displayed, if the currently
            * displayed is still in the display queue. */
            if(PictureToRestartFrom_p == VIDDISP_Data_p->LayerDisplay[Layer].CurrentlyDisplayedPicture_p)
            {
                if(PictureToRestartFrom_p->Next_p != NULL)
                {
                    PictureToRestartFrom_p = PictureToRestartFrom_p->Next_p;
                }
            }
            else
            {
                /* The currently displayed picture was removed from the display queue.
                * This means that all the pictures in the display are more recent than
                * the currently displayed and then we have to restart from the first one */
                PictureToRestartFrom_p = VIDDISP_Data_p->DisplayQueue_p;
            }
            if (PictureToRestartFrom_p != CurrentPicture_p)
            {
                TraceDisplay(("Restart display %d on primary queue\r\n",Layer));
                PutNewPictureOnDisplayForNextVSync(DisplayHandle,
                                                PictureToRestartFrom_p,
                                                Layer);
                VIDDISP_Data_p->LockState = UNLOCKED;
            } /* if (PictureToRestartFrom_p != CurrentPicture_p) */
            else
            {
                if (CurrentPicture_p->Counters[Layer].CurrentFieldOnDisplay
                        == VIDDISP_FIELD_DISPLAY_TOP)
                {
                    PutTopForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
                }
                else
                {
                    PutBottomForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
                }
                VIDDISP_Data_p->LockState = LOCKED;
            }
        } /* if (PictureToRestartFrom_p != NULL) */
        else
        {
            VIDDISP_Data_p->LockState = LOCKED;
        }
    }
} /* end of ReStartQueues */

/*******************************************************************************
Name        : SequenceSlave
Description :
*******************************************************************************/
static void SequenceSlave(U32 Layer, const VIDDISP_Handle_t DisplayHandle)
{
#ifdef PRESERVE_FIELD_POLARITY_ON_SLAVE_DISPLAY
    SequenceSlaveWithCareOfFieldPolarity(Layer, DisplayHandle);
#else
    SequenceSlaveBasic(Layer, DisplayHandle);
#endif
} /* End of SequenceSlave() */

#ifdef PRESERVE_FIELD_POLARITY_ON_SLAVE_DISPLAY
/*******************************************************************************
Name        : SequenceSlaveWithCareOfFieldPolarity
Description : New algorithm for selecting the picture to display on Slave.
              This new algorithm try to ensure that the polarity of the slave VSync
              is respected. So the Slave will prepare the same picture than the
              Master but it can take a delay of one VSync to preserve its polarity.
*******************************************************************************/
static void SequenceSlaveWithCareOfFieldPolarity(U32 Layer, const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Picture_t         * PicturePreparedByMasterDisplay_p;
    VIDDISP_Picture_t         * PictureDisplayedByMasterDisplay_p;
    VIDDISP_Picture_t         * PicturePreparedForSlaveDisplay_p = NULL;
    LayerDisplayParams_t      * SlaveDisplay_p;
    LayerDisplayParams_t      * MasterDisplay_p;
    VIDDISP_Data_t            * VIDDISP_Data_p;
    BOOL                        IsNextOutputVSyncTop;

    /* shortcuts */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    SlaveDisplay_p = &(VIDDISP_Data_p->LayerDisplay[Layer]);
    MasterDisplay_p = &(VIDDISP_Data_p->LayerDisplay[1-Layer]);

    PicturePreparedByMasterDisplay_p = MasterDisplay_p->PicturePreparedForNextVSync_p;
    PictureDisplayedByMasterDisplay_p = MasterDisplay_p->CurrentlyDisplayedPicture_p;

    if (PictureDisplayedByMasterDisplay_p == NULL)
    {
        return; /* Cannot continue because ref display is not started */
    }

    if ( (SlaveDisplay_p->CurrentlyDisplayedPicture_p != PictureDisplayedByMasterDisplay_p) &&
         (SlaveDisplay_p->CurrentlyDisplayedPicture_p != NULL) )
    {
        if (SlaveDisplay_p->CurrentlyDisplayedPicture_p->PrimaryPictureBuffer_p != NULL)
        {
            VIDBUFF_SetDisplayStatus(
                              VIDDISP_Data_p->BufferManagerHandle,
                              VIDBUFF_LAST_FIELD_ON_DISPLAY,
                              SlaveDisplay_p->CurrentlyDisplayedPicture_p->PrimaryPictureBuffer_p);
        }
        if (SlaveDisplay_p->CurrentlyDisplayedPicture_p->SecondaryPictureBuffer_p != NULL)
        {
            VIDBUFF_SetDisplayStatus(
                              VIDDISP_Data_p->BufferManagerHandle,
                              VIDBUFF_LAST_FIELD_ON_DISPLAY,
                              SlaveDisplay_p->CurrentlyDisplayedPicture_p->SecondaryPictureBuffer_p);
        }
    }

    /* Get the polarity of the next Vsync on this layer */
    IsNextOutputVSyncTop = VIDDISP_Data_p->LayerDisplay[Layer].IsNextOutputVSyncTop;

    /*** Compute what should be the next picture on slave display ***/

    /* If the source is progressive we don't need to check neither the fields order nor the polarity */
    if (PictureDisplayedByMasterDisplay_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN)
    {
        /* We prepare the same picture for slave as the last one prepared for the Master (if it exists) */
        if (PicturePreparedByMasterDisplay_p != NULL)
        {
            PicturePreparedForSlaveDisplay_p = PicturePreparedByMasterDisplay_p;
        }
        else
        {
            PicturePreparedForSlaveDisplay_p = PictureDisplayedByMasterDisplay_p;
        }

        /* Save the picture prepared for Slave display */
        SlaveDisplay_p->PicturePreparedForNextVSync_p = PicturePreparedForSlaveDisplay_p;

        /* The next picture is computed, display it */
        PutNewPictureOnDisplayForNextVSync(DisplayHandle, PicturePreparedForSlaveDisplay_p, Layer);

        PutTopForNextVSync(DisplayHandle, Layer, PicturePreparedForSlaveDisplay_p);

        /* Leave the function now */
        return;
    }


    /* Check if the picture currently displayed by the Master has the good polarity */
    if (IsNextOutputVSyncTop == MasterDisplay_p->IsCurrentFieldOnDisplayTop)
    {
        PicturePreparedForSlaveDisplay_p = PictureDisplayedByMasterDisplay_p;
    }
    else
    {
        /* The picture currently displayed by the Master doesn't have the right polarity for Slave */
        /* Check the polarity of the picture PREPARED by the Master (if it exists) */
        if (PicturePreparedByMasterDisplay_p != NULL)
        {
            if (IsNextOutputVSyncTop == MasterDisplay_p->IsFieldPreparedTop)
            {
                PicturePreparedForSlaveDisplay_p = PicturePreparedByMasterDisplay_p;
            }
        }
    }

    if (PicturePreparedForSlaveDisplay_p != NULL)
    {
        /* A picture with good polarity has been found.
           Check that it is posterior to the one currently displayed by Slave (if it exists) */
        if (SlaveDisplay_p->CurrentlyDisplayedPicture_p != NULL)
        {
            if (PicturePreparedForSlaveDisplay_p->PictureIndex < SlaveDisplay_p->CurrentlyDisplayedPicture_p->PictureIndex)
            {
                /* PicturePreparedForSlaveDisplay_p is anterior to the picture currently on display so it is not acceptable */
                PicturePreparedForSlaveDisplay_p = NULL;
            }
            else
            {
                if (PicturePreparedForSlaveDisplay_p->PictureIndex == SlaveDisplay_p->CurrentlyDisplayedPicture_p->PictureIndex)
                {
                    /* The picture indexes are the same.
                       - If the picture is interlaced, we must check the fields order.
                       - If the picture is progressive, the fields order doesn't matter */

                    /* In case of 3:2 Pulldown a field of a progressive picture can be repeated when "RepeatFirstField" is set. In that case the field order doesn't matter */
                    if ( (PicturePreparedForSlaveDisplay_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ||
                         (PicturePreparedForSlaveDisplay_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.RepeatFirstField) )
                    {
                        /* the fields order doesn't matter */
                    }
                    else
                    {
                        /* Interlaced picture: Check the fields order */
                        if (IsNextOutputVSyncTop == SlaveDisplay_p->IsCurrentFieldOnDisplayTop)     /* IsNextOutputVSyncTop is used to know the polarity of PicturePreparedForSlaveDisplay_p */
                        {
                            /* PicturePreparedForSlaveDisplay_p has the same polarity than the field currently on display so it is not anterior and we can display it */
                        }
                        else
                        {
                            /* PicturePreparedForSlaveDisplay_p has a different polarity than the field currently on display: Check that is it not anterior to the field currently displayed */
                            if (VIDDISP_Data_p->DisplayForwardNotBackward)
                            {
                                /* Display going forward */
                                if (PicturePreparedForSlaveDisplay_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst != SlaveDisplay_p->IsCurrentFieldOnDisplayTop)
                                {
                                    /* Picture Top Field first and Slave currently displaying Bottom field
                                                                     OR
                                       Picture Bottom Field first and Slave currently displaying Top field

                                       => The slave display is currently displaying the 2nd field so we can't find a better posterior field. */
                                    PicturePreparedForSlaveDisplay_p = NULL;
                                }
                            }
                            else
                            {
                                /* Display going backward */
                                if (PicturePreparedForSlaveDisplay_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst == SlaveDisplay_p->IsCurrentFieldOnDisplayTop)
                                {
                                    /* Picture Top Field first and Slave currently displaying Top field
                                                                     OR
                                       Picture Bottom Field first and Slave currently displaying Bottom field

                                       => The slave display is currently displaying the 1st field so we can't find a better field when displaying backward. */
                                    PicturePreparedForSlaveDisplay_p = NULL;
                                }
                            }
                        }
                    }
               }
            }
        }
        else
        {
            /* No picture is currently displayed by Slave so there is no chronology to check */
        }
    }

    /* At this stage we have check the polarity and the fields chronology */
    if (PicturePreparedForSlaveDisplay_p != NULL)
    {
        /* An appropriate picture has been found */

        /* Save the picture prepared for Slave display */
        SlaveDisplay_p->PicturePreparedForNextVSync_p = PicturePreparedForSlaveDisplay_p;

        /* The next picture is computed, display it */
        PutNewPictureOnDisplayForNextVSync(DisplayHandle, PicturePreparedForSlaveDisplay_p, Layer);

        if (IsNextOutputVSyncTop)
        {
            PutTopForNextVSync(DisplayHandle, Layer, PicturePreparedForSlaveDisplay_p);
        }
        else
        {
            PutBottomForNextVSync(DisplayHandle, Layer, PicturePreparedForSlaveDisplay_p);
        }
    }
    else
    {
        /* No appropriate picture was found so we repeat the same field (if it exists)
        A field inversion will occur but it will be compensated by the VHSRC */
        TrVSync((" No appropriate field for AUX\r\n"));
        if (SlaveDisplay_p->CurrentlyDisplayedPicture_p != NULL)
        {
            /* Repeat the exact same field */
            if (SlaveDisplay_p->IsCurrentFieldOnDisplayTop)
            {
                PutTopForNextVSync(DisplayHandle, Layer, SlaveDisplay_p->CurrentlyDisplayedPicture_p);
            }
            else
            {
                PutBottomForNextVSync(DisplayHandle, Layer, SlaveDisplay_p->CurrentlyDisplayedPicture_p);
            }
        }
    }

}   /* End of SequenceSlaveWithCareOfFieldPolarity() */

#else /* PRESERVE_FIELD_POLARITY_ON_SLAVE_DISPLAY */

/*******************************************************************************
Name        : SequenceSlaveBasic
Description : Old algorithm for selecting the picture to display on Slave.
              The picture prepared for Slave will be the same than the last one
              prepared for the Master. This is done independentely of the Slave
              VSync polarity (so field inversions can happen).
*******************************************************************************/
static void SequenceSlaveBasic(U32 Layer, const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Picture_t         * PicturePreparedByMasterDisplay_p;
    VIDDISP_Picture_t         * PictureDisplayedByMasterDisplay_p;
    VIDDISP_Picture_t         * PictureDisplayedBySlaveDisplay_p;
    VIDDISP_Picture_t         * PicturePreparedForSlaveDisplay_p = NULL;
    LayerDisplayParams_t      * SlaveDisplay_p;
    LayerDisplayParams_t      * MasterDisplay_p;
    VIDDISP_Data_t            * VIDDISP_Data_p;
    LayerDisplayParams_t *      LayerDisplayParams_p;

    /* shortcuts */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[Layer]);

    SlaveDisplay_p = &(VIDDISP_Data_p->LayerDisplay[Layer]);
    MasterDisplay_p = &(VIDDISP_Data_p->LayerDisplay[1-Layer]);

    PicturePreparedByMasterDisplay_p = MasterDisplay_p->PicturePreparedForNextVSync_p;
    PictureDisplayedByMasterDisplay_p = MasterDisplay_p->CurrentlyDisplayedPicture_p;
    PictureDisplayedBySlaveDisplay_p = SlaveDisplay_p->CurrentlyDisplayedPicture_p;

    if (PictureDisplayedByMasterDisplay_p == NULL)
    {
        return; /* Cannot continue because ref display is not started */
    }

    if ( (SlaveDisplay_p->CurrentlyDisplayedPicture_p != PictureDisplayedByMasterDisplay_p) &&
         (SlaveDisplay_p->CurrentlyDisplayedPicture_p != NULL) )
    {
        if (SlaveDisplay_p->CurrentlyDisplayedPicture_p->PrimaryPictureBuffer_p != NULL)
        {
            VIDBUFF_SetDisplayStatus(
                              VIDDISP_Data_p->BufferManagerHandle,
                              VIDBUFF_LAST_FIELD_ON_DISPLAY,
                              SlaveDisplay_p->CurrentlyDisplayedPicture_p->PrimaryPictureBuffer_p);
        }
        if (SlaveDisplay_p->CurrentlyDisplayedPicture_p->SecondaryPictureBuffer_p != NULL)
        {
            VIDBUFF_SetDisplayStatus(
                              VIDDISP_Data_p->BufferManagerHandle,
                              VIDBUFF_LAST_FIELD_ON_DISPLAY,
                              SlaveDisplay_p->CurrentlyDisplayedPicture_p->SecondaryPictureBuffer_p);
        }
    }

    /*** Compute what should be the next picture on slave display ***/
    if (PicturePreparedByMasterDisplay_p != NULL)
    {
        PicturePreparedForSlaveDisplay_p = PicturePreparedByMasterDisplay_p;
    }
    else
    {
        PicturePreparedForSlaveDisplay_p = PictureDisplayedByMasterDisplay_p;
    }

    /* PicturePreparedForSlaveDisplay_p can not be NULL here because PictureDisplayedByMasterDisplay_p is not NULL */

    /* Save the picture prepared for Slave display */
    SlaveDisplay_p->PicturePreparedForNextVSync_p = PicturePreparedForSlaveDisplay_p;

    /* The next picture is computed, display it */
    PutNewPictureOnDisplayForNextVSync(DisplayHandle, PicturePreparedForSlaveDisplay_p, Layer);

    /* Prepare the same polarity as Master display */
    if (MasterDisplay_p->IsFieldPreparedTop)
    {
        PutTopForNextVSync(DisplayHandle, Layer, PicturePreparedForSlaveDisplay_p);
    }
    else
    {
        PutBottomForNextVSync(DisplayHandle, Layer, PicturePreparedForSlaveDisplay_p);
    }

    /* Workaround for DDTS GNBvd66788 (for every chips with 2 independant VSync) */
#if defined(ST_7100) || defined(ST_7710) || defined(ST_7109) || defined(ST_7200)
    if (PictureDisplayedBySlaveDisplay_p != NULL)
    {
        if (PictureDisplayedBySlaveDisplay_p->PictureIndex < PicturePreparedForSlaveDisplay_p->PictureIndex)
        {
            /* The Slave display is about to move to a new picture */
            if (PictureDisplayedBySlaveDisplay_p->PictureIndex < PictureDisplayedByMasterDisplay_p->PictureIndex)
            {
                /* The picture currently displayed by slave is not used anymore by the master */
                if ( (LayerDisplayParams_p->IsLastPicturePreparedDecimated) &&
                     (PictureDisplayedBySlaveDisplay_p->SecondaryPictureBuffer_p != NULL) )
                {
                    /* The primary picture is not needed anymore so we can release it immediately */
                    /* Information will now be taken in the secondary picture buffer */
                    PictureDisplayedBySlaveDisplay_p->PictureInformation_p = PictureDisplayedBySlaveDisplay_p->SecondaryPictureBuffer_p;
                    TrWarning(("Release Primary Pict 0x%x\r\n", PictureDisplayedBySlaveDisplay_p->PrimaryPictureBuffer_p));
                    viddisp_ReleasePictureBuffer(DisplayHandle, PictureDisplayedBySlaveDisplay_p->PrimaryPictureBuffer_p);
                    PictureDisplayedBySlaveDisplay_p->PrimaryPictureBuffer_p = NULL;
                }
            }
        }
    }
#endif /* defined(ST_7100) || defined(ST_7710) || defined(ST_7109) || defined(ST_7200) */

}

#endif /* PRESERVE_FIELD_POLARITY_ON_SLAVE_DISPLAY */

/*******************************************************************************
Name        : FindPictWhichMustBeStopped
Description :
Assumptions : CurrentPicture_p is displayed on the ref display
Returns     : FALSE if required picture to stop on will never be found
*******************************************************************************/
static BOOL FindPictWhichMustBeStopped(const VIDDISP_Handle_t DisplayHandle,
                                    VIDDISP_Picture_t * const CurrentPicture_p)
{
    VIDDISP_Data_t *    VIDDISP_Data_p;
    STVID_PTS_t *       PictureToBeFrozenPTS_p;
    STVID_PTS_t*        CurrentPicturePTS_p;

    /* shortcut */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    VIDDISP_Data_p->FreezeParams.PauseRequestIsPending  = FALSE;
    VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending = FALSE;
    if (CurrentPicture_p == NULL)
    {
        TrStop(("No picture in display yet ... \r\n"));
        return(TRUE);
    }
    TrStop(("Stop ..."));
    if (VIDDISP_Data_p->StopParams.StopMode == VIDDISP_STOP_ON_PTS)
    {
        CurrentPicturePTS_p = &(CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS);
        PictureToBeFrozenPTS_p = &(VIDDISP_Data_p->StopParams.RequestStopPicturePTS);

        if ((CurrentPicturePTS_p == NULL)
                || (PictureToBeFrozenPTS_p == NULL))
        {
            TrStop((" PTS error !! \r\n"));
            return TRUE;                                   /* --> KO, return   */
        }
        if ((CurrentPicturePTS_p->PTS33 == PictureToBeFrozenPTS_p->PTS33)
        && (CurrentPicturePTS_p->PTS == PictureToBeFrozenPTS_p->PTS))
        {
            TrStop((" PTS matches \r\n"));         /* --> OK, continue */
        }
        else
        {
            TrStop((" PTS doesn't matches \r\n"));
            return TRUE;                                   /* --> KO, return but last pict could be found later  */
        }
    } /* end stop on pts */
    else if (VIDDISP_Data_p->StopParams.StopMode == VIDDISP_STOP_LAST_PICTURE_IN_QUEUE)
    {
        /* Stop if last picture in queue reached. */
        if (CurrentPicture_p->Next_p == NULL)
        {
            /* last picture in display queue */
            TrStop((" last pict matches \r\n"));   /* --> OK, continue */
        }
        else
        {
            TrStop((" last pict not found !! \r\n"));
            return TRUE;                                   /* --> KO, return but last pict could be found later   */
        }
    }
    else /* case : stop now */
    {
        TrStop((" Now \r\n"));                     /* --> OK, continue */
    }

    /* set the new display state : */
    VIDDISP_Data_p->StopParams.StopRequestIsPending     = FALSE;
    VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending = TRUE;
    if (VIDDISP_Data_p->StopParams.StopMode != VIDDISP_STOP_NOW)
    {
        VIDDISP_Data_p->NotifyStopWhenFinish    = TRUE;
    }
    else
    {
        /* stop-now : patch the freeze params */
        VIDDISP_Data_p->FreezeParams.Freeze.Field = STVID_FREEZE_FIELD_CURRENT;
    }
    TrStop(("Display stopped !! \r\n"));
    return (TRUE);
}

/*******************************************************************************
Name        : FindFieldWhichMustBeFrozen
Description :
Assumptions : CurrentPicture_p is displayed on the ref display
*******************************************************************************/
static void FindFieldWhichMustBeFrozen(const VIDDISP_Handle_t DisplayHandle,
                                    VIDDISP_Picture_t * const CurrentPicture_p)
{
    VIDDISP_Data_t *    VIDDISP_Data_p;

    /* shortcut */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (CurrentPicture_p == NULL)
    {
        TraceDisplay(("No picture in display yet ... \r\n"));
        return;
    }
    TraceDisplay(("Freeze or Pause ..."));
    /* if Force Mode, don't Care of polarity */
    if ((VIDDISP_Data_p->FreezeParams.Freeze.Mode == STVID_FREEZE_MODE_FORCE) ||
       ((VIDDISP_Data_p->FreezeParams.Freeze.Mode == STVID_FREEZE_MODE_NO_FLICKER) &&
        (CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.ScanType == STGXOBJ_INTERLACED_SCAN)))
    {
        if (VIDDISP_Data_p->FreezeParams.Freeze.Field == STVID_FREEZE_FIELD_CURRENT)
        {
            TraceDisplay((" current field matches \r\n"));
        }
        else if (VIDDISP_Data_p->FreezeParams.Freeze.Field == STVID_FREEZE_FIELD_NEXT)
        {
            TraceDisplay((" next field \r\n"));
            VIDDISP_Data_p->FreezeParams.Freeze.Field = STVID_FREEZE_FIELD_CURRENT;
            /* Field to Be frozen will be found next time ... */
            return;
        }
        else if ((VIDDISP_Data_p->FreezeParams.Freeze.Field == STVID_FREEZE_FIELD_TOP)
                && (CurrentPicture_p->Counters[VIDDISP_Data_p->DisplayRef].CurrentFieldOnDisplay == VIDDISP_FIELD_DISPLAY_TOP))
        {
            TraceDisplay((" Top field matches \r\n"));
        }
        else if ((VIDDISP_Data_p->FreezeParams.Freeze.Field == STVID_FREEZE_FIELD_BOTTOM)
                && (CurrentPicture_p->Counters[VIDDISP_Data_p->DisplayRef].CurrentFieldOnDisplay == VIDDISP_FIELD_DISPLAY_BOTTOM))
        {
            TraceDisplay((" Bot field matches \r\n"));
        }
        else
        {
            TraceDisplay((" field not found \r\n"));
            if (!(VIDDISP_Data_p->LockState == LOCKED))
            {
                /* Field not matching and display on-going: exit function and wait for next field (on next VSync),
                 field should match then. */
                return;
            }
            /* ELSE Field not matching and display is locked: there is no chance that a new field matches as display is locked.
               So freeze anyway and the good field will be displayed on next VSync. */
        }
    } /* end force mode */
    else
    {
        /* case : STVID_FREEZE_MODE_NONE or */
        /* if NO_FLICKER_MODE AND STREAM IS INTERLACED */
        /* Both fields have to be displayed. But if 1 field picture, */
        /* force display to that field */
        TraceDisplay((" mode none or stream interlaced \r\n"));
    }

    /* set the new display state : */
    if (VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending)
    {
        VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending = FALSE;
        VIDDISP_Data_p->DisplayState = VIDDISP_DISPLAY_STATE_FREEZED;
        TraceDisplay(("Display freezed !! \r\n"));
    }
    else
    {
        VIDDISP_Data_p->FreezeParams.PauseRequestIsPending = FALSE;
        VIDDISP_Data_p->DisplayState = VIDDISP_DISPLAY_STATE_PAUSED;
        TraceDisplay(("Display paused !! \r\n"));
    }
}
/*******************************************************************************
Name        : FreezedDisplay
Description :
Assumptions : Manage the field selection when the display is freezed (or paused
              or stopped)
*******************************************************************************/
static void FreezedDisplay(const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t *            VIDDISP_Data_p;
    U32                         Layer;
    VIDDISP_Picture_t *         CurrentPicture_p;

    /* shortcuts */
    VIDDISP_Data_p      = (VIDDISP_Data_t *) DisplayHandle;
    Layer               = VIDDISP_Data_p->DisplayRef;
    CurrentPicture_p    = VIDDISP_Data_p->LayerDisplay[Layer].CurrentlyDisplayedPicture_p;

    if (CurrentPicture_p == NULL)
    {
        return; /* no picture is displayed yet, we can return */
    }

    if ((VIDDISP_Data_p->FreezeParams.Freeze.Mode == STVID_FREEZE_MODE_FORCE)
        ||(VIDDISP_Data_p->FreezeParams.Freeze.Mode == STVID_FREEZE_MODE_NO_FLICKER))
    {
        /* Clear toggle and set field according to freeze.field */
        if (VIDDISP_Data_p->FreezeParams.Freeze.Field == STVID_FREEZE_FIELD_TOP)
        {
            PutTopForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
        }
        else if (VIDDISP_Data_p->FreezeParams.Freeze.Field == STVID_FREEZE_FIELD_BOTTOM)
        {
            PutBottomForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
        }
        else if(VIDDISP_Data_p->FreezeParams.Freeze.Field == STVID_FREEZE_FIELD_CURRENT)
        {
            TraceVerbose(("Freeze Current Pict:%d\r\n",CurrentPicture_p->Counters[VIDDISP_Data_p->DisplayRef].CurrentFieldOnDisplay));
            if (CurrentPicture_p->Counters[VIDDISP_Data_p->DisplayRef].CurrentFieldOnDisplay == VIDDISP_FIELD_DISPLAY_TOP)
            {
                PutTopForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
            }
            else
            {
                PutBottomForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
            }
        }
        VIDDISP_Data_p->FreezeParams.Freeze.Field = STVID_FREEZE_FIELD_CURRENT;
        /* else : nothing to do: we have done 'step' cmds */
    }/* end mode-force and mode antiflicker */

    else if (VIDDISP_Data_p->FreezeParams.Freeze.Mode == STVID_FREEZE_MODE_NONE)
    {
        /* Set toggle and set field according to vtg */
        if (( (VIDDISP_Data_p->LayerDisplay[Layer].CurrentVSyncTop)  && ((VIDDISP_Data_p->LayerDisplay[Layer].DisplayLatency % 2) == 1)) ||
           ((!(VIDDISP_Data_p->LayerDisplay[Layer].CurrentVSyncTop)) && ((VIDDISP_Data_p->LayerDisplay[Layer].DisplayLatency % 2) == 0)))
        {
            PutBottomForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
        }
        else
        {
            PutTopForNextVSync(DisplayHandle, Layer, CurrentPicture_p);
        }
    } /* end if mode-none */
}



/*******************************************************************************
Name        : viddisp_VSyncDisplayMain
Description : VSync call back for main display
*******************************************************************************/
void viddisp_VSyncDisplayMain(STEVT_CallReason_t Reason,
        const ST_DeviceName_t RegistrantName,
        STEVT_EventConstant_t Event, const void *EventData_p,
        const void *SubscriberData_p)
{
    VIDDISP_Handle_t    DisplayHandle;
    VIDDISP_Data_t * VIDDISP_Data_p;
    STVTG_VSYNC_t *     VSync_p;
    STGXOBJ_ScanType_t previousVTGScanType;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    DisplayHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDISP_Handle_t, SubscriberData_p);
    VSync_p = CAST_CONST_HANDLE_TO_DATA_TYPE(STVTG_VSYNC_t *, EventData_p);

    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    previousVTGScanType = VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.VTGScanType;

    TrVSync(("MAIN VSync %s\r\n", (*VSync_p == STVTG_TOP) ? "Top" : "Bot"));

    /* Main VSync : Ident=0 */
    VSyncDisplay(DisplayHandle, *VSync_p, 0);
    /* Notify new VTG ScanType */
    if(VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.VTGScanType != previousVTGScanType)
    {
        STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                     VIDDISP_Data_p->RegisteredEventsID[VIDDISP_VTG_SCANTYPE_CHANGE_EVT_ID],
                     STEVT_EVENT_DATA_TYPE_CAST &VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.VTGScanType);
    }
}

/*******************************************************************************
Name        : viddisp_VSyncDisplayAux
Description : VSync call back for aux display
*******************************************************************************/
void viddisp_VSyncDisplayAux(STEVT_CallReason_t Reason,
        const ST_DeviceName_t RegistrantName,
        STEVT_EventConstant_t Event, const void *EventData_p,
        const void *SubscriberData_p)
{
    VIDDISP_Handle_t    DisplayHandle;
    STVTG_VSYNC_t *     VSync_p;
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    DisplayHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDISP_Handle_t, SubscriberData_p);
    VSync_p = CAST_CONST_HANDLE_TO_DATA_TYPE(STVTG_VSYNC_t *, EventData_p);

    TrVSync(("AUX VSync %s\r\n", (*VSync_p == STVTG_TOP) ? "Top" : "Bot"));

    /* Aux VSync : Ident=1 */
    VSyncDisplay(DisplayHandle, *VSync_p, 1);
}

/*******************************************************************************
Name        : VSyncDisplay
Description : Common VSync call back
              NB: An history of three VSync is memorized (VSyncMemoryArray
              display data structure). This history is compared to the
              progressive patterns (only TOP or only BOTTOM). If a progressive
              configuration is detected
              the alternation of VSync polarity is then simulated.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void VSyncDisplay(const VIDDISP_Handle_t DisplayHandle,
                         STVTG_VSYNC_t VSync,
                         U32 LayerIdent)
{
    VIDDISP_Data_t * VIDDISP_Data_p;
    VIDDISP_VsyncLayer_t VIDDISP_VsyncLayer;

    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    /* Record time */
    VIDDISP_Data_p->LayerDisplay[LayerIdent].TimeAtVSyncNotif = time_now();
    VIDDISP_VsyncLayer.VSync = VSync;
    VIDDISP_VsyncLayer.LayerIdent = LayerIdent;

    /*TrVSync(("VSync Disp 0x%x Lay %d\r\n", DisplayHandle, LayerIdent));*/

    /* Notify display VSync for HAL's */
    STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                 VIDDISP_Data_p->RegisteredEventsID[VIDDISP_VSYNC_EVT_ID],
                  STEVT_EVENT_DATA_TYPE_CAST &VIDDISP_VsyncLayer);

    /* Remember the history of VSync polarity.  */
    VIDDISP_Data_p->LayerDisplay[LayerIdent].VSyncMemoryArray =
        ( ((VIDDISP_Data_p->LayerDisplay[LayerIdent].VSyncMemoryArray) << 1)
          | (VSync == STVTG_TOP ? 1 : 0)) & (MAX_VSYNC_PATTERN_VALUE);

    if ((VIDDISP_Data_p->LayerDisplay[LayerIdent].VSyncMemoryArray == PROGRESSIVE_VSYNC_PATTERN_TOP) ||
        (VIDDISP_Data_p->LayerDisplay[LayerIdent].VSyncMemoryArray == PROGRESSIVE_VSYNC_PATTERN_BOT))
    {
        /* Progressive Output */
        VIDDISP_Data_p->LayerDisplay[LayerIdent].DisplayCaract.VTGScanType = STGXOBJ_PROGRESSIVE_SCAN;
        VIDDISP_Data_p->LayerDisplay[LayerIdent].IsNextOutputVSyncTop = TRUE;

        /* Check the last field polarity sent to the display and send the other one */
        if (VIDDISP_Data_p->LayerDisplay[LayerIdent].CurrentVSyncTop)
        {
            /* It was TOP, now display on BOT field.     */
            VIDDISP_Data_p->LayerDisplay[LayerIdent].CurrentVSyncTop  = FALSE;
        }
        else
        {
            /* It was BOT, now display on TOP field.        */
            VIDDISP_Data_p->LayerDisplay[LayerIdent].CurrentVSyncTop  = TRUE;
        }
    }
    else
    {
        /* An error occured, or normal case with intelraced output: send the current VSync polarity */
        VIDDISP_Data_p->LayerDisplay[LayerIdent].DisplayCaract.VTGScanType = STGXOBJ_INTERLACED_SCAN;

        if (VSync == STVTG_TOP)
        {
            VIDDISP_Data_p->LayerDisplay[LayerIdent].IsNextOutputVSyncTop = FALSE;
            VIDDISP_Data_p->LayerDisplay[LayerIdent].CurrentVSyncTop  = TRUE;
        }
        else
        {
            VIDDISP_Data_p->LayerDisplay[LayerIdent].IsNextOutputVSyncTop = TRUE;
            VIDDISP_Data_p->LayerDisplay[LayerIdent].CurrentVSyncTop  = FALSE;
        }
    }

    /* Send the command to the display process: */
    /* the command identifier is the LayerIdent. */
    if (LayerIdent == VIDDISP_Data_p->DisplayRef)
    {
        /* insert Master cmd in prioritary mode */
        PushDisplayCommandPrio(DisplayHandle, LayerIdent);
    }
    else
    {
        PushDisplayCommand(DisplayHandle, LayerIdent);
    }
    STOS_SemaphoreSignal(VIDDISP_Data_p->DisplayOrder_p);
} /* End of VSyncDisplay() function */

/*******************************************************************************
Name        : PushDisplayCommandPrio
Description : Push a prioritary cmd: will be the first poped.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void PushDisplayCommandPrio(const VIDDISP_Handle_t DisplayHandle,
                                    U32 LayerIdent)
{
    U32 CmdsNumber, i;
    U8 Cmds[DISPLAY_COMMANDS_BUFFER_SIZE];

    CmdsNumber = 0;

    /* store cmds queue image */
    while (PopDisplayCommand(DisplayHandle,&Cmds[CmdsNumber]) == ST_NO_ERROR)
    {
        CmdsNumber ++;
    }
    /* insert new cmd first */
    PushDisplayCommand(DisplayHandle, LayerIdent);
    /* re-insert the cmds */
    for (i = 0; i < CmdsNumber; i++)
    {
        PushDisplayCommand(DisplayHandle,Cmds[i]);
    }
} /* End of PushDisplayCommandPrio() function */

/*******************************************************************************
Name        : UpdateCurrentlyDisplayedPicture
Description : This function is called when a new VSync happens, to update the
              pointers related to the currently displayed picture.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void UpdateCurrentlyDisplayedPicture(const VIDDISP_Handle_t DisplayHandle, U32 Layer)
{
    LayerDisplayParams_t      * LayerDisplay_p;
    VIDDISP_Data_t            * VIDDISP_Data_p;
    VIDDISP_Picture_t *       CurrentPicture_p; /* Used to save PicturePreparedForNextVSync_p, just to be sure it doesn't become NULL and cause CurrentlyDisplayedPicture_p to be NULL also */

    /* shortcuts */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplay_p = &(VIDDISP_Data_p->LayerDisplay[Layer]);

    TrPictPrepared(("Layer %d: Cur=0x%x Next=0x%x\r\n",
        Layer,
        (LayerDisplay_p->CurrentlyDisplayedPicture_p == NULL ? NULL : LayerDisplay_p->CurrentlyDisplayedPicture_p->PrimaryPictureBuffer_p),
        (LayerDisplay_p->PicturePreparedForNextVSync_p == NULL ? NULL : LayerDisplay_p->PicturePreparedForNextVSync_p->PrimaryPictureBuffer_p) ));

    /* Update pointers : the picture prepared on the last VSync is now the
       currently displayed */
    CurrentPicture_p = LayerDisplay_p->PicturePreparedForNextVSync_p;

    /* 09/11/2007: PicturePreparedForNextVSync_p is now set even if the prepared picture was not new. */
    if (LayerDisplay_p->CurrentlyDisplayedPicture_p == LayerDisplay_p->PicturePreparedForNextVSync_p)
    {
        /* The last prepared picture was not new: We will simply toggle the field displayed */
        CurrentPicture_p = NULL;
    }

    if (CurrentPicture_p != NULL)
    {
        LayerDisplay_p->NewPictureDisplayedOnCurrentVSync = TRUE;
        LayerDisplay_p->CurrentlyDisplayedPicture_p = CurrentPicture_p;
        LayerDisplay_p->PicturePreparedForNextVSync_p = NULL;
        LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[Layer].CurrentFieldOnDisplay
            = LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[Layer].NextFieldOnDisplay;
        LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[Layer].NextFieldOnDisplay = VIDDISP_FIELD_DISPLAY_NONE;
        LayerDisplay_p->IsCurrentFieldOnDisplayTop = LayerDisplay_p->IsFieldPreparedTop;

#ifdef STVID_DEBUG_GET_STATISTICS
        if (Layer == 0)
        {
            if ( LayerDisplay_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4 )
            {
                /* Secondary display */
                if (LayerDisplay_p->IsLastPicturePreparedDecimated)
                {
                    VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedBySec ++;
                }
                else
                {
                    VIDDISP_Data_p->StatisticsPictureDisplayedBySec ++;
                }
            }
            else
            {
                /* Main display */
                if (LayerDisplay_p->IsLastPicturePreparedDecimated)
                {
                    VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedByMain ++;
                }
                else
                {
                    VIDDISP_Data_p->StatisticsPictureDisplayedByMain ++;
                }
            }
        }
        else if (Layer == 1)
        {
            if ( LayerDisplay_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4 )
            {
                /* Secondary display */
                if (LayerDisplay_p->IsLastPicturePreparedDecimated)
                {
                    VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedBySec ++;
                }
                else
                {
                    VIDDISP_Data_p->StatisticsPictureDisplayedBySec ++;
                }
            }
            else
            {   /* Aux display */
                if (LayerDisplay_p->IsLastPicturePreparedDecimated)
                {
                    VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedByAux ++;
                }
                else
                {
                    VIDDISP_Data_p->StatisticsPictureDisplayedByAux ++;
                }
            }
        }


#endif /* STVID_DEBUG_GET_STATISTICS */
    } /* end if there was a prepared */

    /* Update the current field on display */
    /*-------------------------------------*/
    else
    {
        LayerDisplay_p->NewPictureDisplayedOnCurrentVSync = FALSE;
        LayerDisplay_p->IsCurrentFieldOnDisplayTop = LayerDisplay_p->IsFieldPreparedTop;
        if (LayerDisplay_p->CurrentlyDisplayedPicture_p != NULL)
        {
            if (LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[Layer].NextFieldOnDisplay != VIDDISP_FIELD_DISPLAY_NONE)
            {
                LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[Layer].CurrentFieldOnDisplay
                            = LayerDisplay_p->CurrentlyDisplayedPicture_p->Counters[Layer].NextFieldOnDisplay;
            }
        }
    }
} /* End of UpdateCurrentlyDisplayedPicture() function */

/*******************************************************************************
Name        : viddisp_ResetField
Description : This function reset and release a Field
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_ResetField(const VIDDISP_Handle_t DisplayHandle, U32 LayerIdent, HALDISP_Field_t * Field_p)
{
    VIDDISP_Data_t      * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    UNUSED_PARAMETER(LayerIdent);

    if (Field_p == NULL)
    {
        return;
    }

    if (Field_p->PictureBuffer_p != NULL)
    {
        TrLock(("Layer %d: Unlock Picture 0x%x\r\n", LayerIdent, Field_p->PictureBuffer_p));
        RELEASE_PICTURE_BUFFER(VIDDISP_Data_p->BufferManagerHandle, Field_p->PictureBuffer_p, VIDCOM_DEI_MODULE_BASE);
        Field_p->PictureBuffer_p = NULL;
    }
    Field_p->PictureIndex = 0;
    /* "Field_p->FieldType" does not matter */
}

/*******************************************************************************
Name        : viddisp_ResetFieldArray
Description : This function reset and release a Field Array.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_ResetFieldArray(const VIDDISP_Handle_t DisplayHandle,
                                    U32 LayerIdent,
                                    HALDISP_Field_t * FieldArray_p)
{
    U32     i;

    if (FieldArray_p == NULL)
    {
        return;
    }

    for (i=0; i<VIDDISP_NBR_OF_FIELD_REFERENCES; i++)
    {
        viddisp_ResetField(DisplayHandle, LayerIdent, &(FieldArray_p[i]) );
    }
}

/*******************************************************************************
Name        : viddisp_InitMultiFieldPresentation
Description : This function initializes the variables for Multi Field Presentation
Parameters  : Display handle, pointer to picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddisp_InitMultiFieldPresentation(const VIDDISP_Handle_t DisplayHandle,
                                        U32 LayerIdent)
{
    VIDDISP_Data_t      * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_t * LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);

    /* Init the content of the Field arrays "Fields1", "Fields2" "Fields3" */
    viddisp_InitFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->Fields1);
    viddisp_InitFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->Fields2);
    viddisp_InitFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->Fields3);

    /* Init Field Arrays used as reference by the Deinterlacer */
    LayerDisplayParams_p->FieldsUsedAtNextVSync_p = LayerDisplayParams_p->Fields1;
    LayerDisplayParams_p->FieldsUsedByHw_p = LayerDisplayParams_p->Fields2;
    LayerDisplayParams_p->FieldsNotUsed_p = LayerDisplayParams_p->Fields3;
    LayerDisplayParams_p->FieldsToUnlock_p = NULL;
}

/*******************************************************************************
Name        : viddisp_IdentifyFieldsToUse
Description : This function identifies what are the Fields that should be used by the Display.
                    It makes a distinction between the Field Triplet currently used by the hardware and
                    the Field Triplet that will be used at next VSync.
Parameters  : Display handle, pointer to picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_IdentifyFieldsToUse(const VIDDISP_Handle_t DisplayHandle,
                                        U32 LayerIdent,
                                        VIDDISP_Picture_t * NextPicture_p,
                                        HALDISP_FieldType_t FieldType)
{
    VIDDISP_Data_t      * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_t * LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);

    /* A new VSync has occured:
         - The Fields which were called "Fields used at next VSync" now becomes the "Fields used by the hardware"
         - The Fields which were used by the hardware are not needed anymore */

    /* We temporary save the pointer to the Fields which were used by the hardware
        because we want to Unlock them later */
    LayerDisplayParams_p->FieldsToUnlock_p = LayerDisplayParams_p->FieldsUsedByHw_p;

    /* The "Fields used at next VSync" now becomes the "Fields used by the hardware"
        NB: The Fields remain Lock! */
    LayerDisplayParams_p->FieldsUsedByHw_p = LayerDisplayParams_p->FieldsUsedAtNextVSync_p;

    /* The Unused Field array will be now used to store the Fields that will be used at Next VSync */
    LayerDisplayParams_p->FieldsUsedAtNextVSync_p = LayerDisplayParams_p->FieldsNotUsed_p;

    /* Clean the structure 'LayerDisplayParams_p->FieldsUsedAtNextVSync_p' before filling it with new data (this array should already be empty) */
    viddisp_ResetFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->FieldsUsedAtNextVSync_p);

    /* Look for the Fields to display at next VSync... */
    viddisp_SetFieldsForNextVSync(DisplayHandle, LayerIdent, NextPicture_p, FieldType);

}

/*******************************************************************************
Name        : viddisp_SetFieldsForNextVSync
Description : This function determines what are the Fields Previous, Current, Next for the Next VSync.
                    (the Current Field is the Field which will be displayed at next VSync)
Parameters  : Display handle, pointer to picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_SetFieldsForNextVSync(const VIDDISP_Handle_t DisplayHandle,
                                          U32 LayerIdent,
                                          VIDDISP_Picture_t * NextPicture_p,
                                          HALDISP_FieldType_t FieldType)
{
    STVID_PictureStructure_t    PictureStructure;
    VIDDISP_Data_t      *       VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_t *      LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);

    /* Set the Current Field (= the one which will be displayed at next VSync) */
    if (LayerDisplayParams_p->IsLastPicturePreparedDecimated)
    {
        LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureBuffer_p = NextPicture_p->SecondaryPictureBuffer_p;
    }
    else
    {
        LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureBuffer_p = NextPicture_p->PrimaryPictureBuffer_p;
    }
    LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureIndex = NextPicture_p->PictureIndex;
    LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].FieldType = FieldType;

#ifndef ST_transcoder

#ifdef ST_XVP_ENABLE_FGT
    viddisp_StoreFGTParams( DisplayHandle, NextPicture_p, FieldType);
#endif /* ST_XVP_ENABLE_FGT */

    /* Fields 'Previous' and 'Next' should be set and locked only if extra frame buffers have been allocated */
    if (VIDDISP_Data_p->LayerDisplay[LayerIdent].NbrOfExtraPrimaryFrameBuffers == 0)
    {
        TrField(("Error Layer %d! Extra Frame Buffers are required for Deinterlacing!\r\n", LayerIdent));
        return;
    }

    /* First check if picture decoding context exists (in case of vid_clear before vid_start, it could be null)  */
    if(NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureDecodingContext_p != NULL)
    {
        /* HD Source Picture can not be managed because they will not fit in Extra Frame Buffers (that are SD only) */
        if (NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Width > MAX_DEI_PICTURE_WIDTH)
        {
            /* OLO_TO_DO: This checking on MAX_DEI_PICTURE_WIDTH will not work when chipset with HD DEI will exist */
            TrField(("Source has more than %d pixels per line: Fields Previous and Next not set\r\n", MAX_DEI_PICTURE_WIDTH));
            return;
        }
    }
    else
    {
        TrField(("Picture Decoding context not available: Fields Previous and Next not set\r\n"));
        return;
    }

    /* OLO_TO_DO: Manage Fields Previous and Next when display is frozen */
    if ( (VIDDISP_Data_p->StopParams.StopRequestIsPending) ||
         (VIDDISP_Data_p->DisplayState != VIDDISP_DISPLAY_STATE_DISPLAYING) )
    {
        if(VIDDISP_Data_p->DoReleaseDeiReferencesOnFreeze != TRUE)
        {
        /*TrWarning(("Layer %d is not Displaying: Fields Previous and Next not set\r\n", LayerIdent));*/
        }
        else
        {
            /* Display is being stopped or is not displaying: We don't set references to the Previous and Next Fields */
            return;
        }
    }

    PictureStructure = NextPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure;

    if (PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
    {
        /* The Picture is made of 2 Fields: Extract information about the "other" Field */
        viddisp_SetOtherFieldInSameFrame(DisplayHandle, LayerIdent);
    }

    /* Search for Next Field (if it has not already been found in the current Frame) */
    if (LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureBuffer_p == NULL)
    {
        viddisp_SearchNextField(DisplayHandle, LayerIdent, NextPicture_p, FieldType);
    }

    /* Search for Previous Field (if it has not already been found in the current Frame) */
    if (LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureBuffer_p == NULL)
    {
        viddisp_SearchPreviousField(DisplayHandle, LayerIdent);
    }

    if (LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureBuffer_p == NULL)
    {
        TrField(("Previous field missing\r\n"));
    }
    if (LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureBuffer_p == NULL)
    {
        TrField(("Next field missing\r\n"));
    }

    TrField(("Layer: %d\r\n", LayerIdent));
    TrField(("Previous Field: %d %s\r\n", LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureIndex, FIELD_TYPE(LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD]) ));
    TrField(("Current Field: %d %s\r\n", LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureIndex, FIELD_TYPE(LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD]) ));
    TrField(("Next Field: %d %s\r\n", LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureIndex, FIELD_TYPE(LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD]) ));

#endif /* ST_transcoder */
}

/*******************************************************************************
Name        : viddisp_SetOtherFieldInSameFrame
Description : This function set references to the other Field in the same Frame.
              This other Field can be the Previous or Next Field (depending of TopFieldFirst)

              This function should be called only if the Picture is a Frame
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_SetOtherFieldInSameFrame(const VIDDISP_Handle_t DisplayHandle, U32 LayerIdent)
{
    BOOL                            TopFieldFirst;
    HALDISP_Field_t            OtherFieldInSameFrame; /* Field in the same Frame but with the other parity */
    VIDDISP_Data_t      * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_t * LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);

    TopFieldFirst = LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst;

    /* Set a reference to the other Field of the same picture */
    OtherFieldInSameFrame.PictureBuffer_p = LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureBuffer_p;
    OtherFieldInSameFrame.PictureIndex = LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureIndex;
    OtherFieldInSameFrame.FieldType = SWAP_FIELD_TYPE(LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].FieldType);

    if (VIDDISP_Data_p->DisplayForwardNotBackward)
    {
        /* Display is going forward */
        if ( ( (TopFieldFirst) && (LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].FieldType == HALDISP_TOP_FIELD) ) ||
              ( (!TopFieldFirst)  && (LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].FieldType == HALDISP_BOTTOM_FIELD) ) )
        {
            /* The Current field is First in the Frame so the other Field is the Next Field */
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureBuffer_p = OtherFieldInSameFrame.PictureBuffer_p;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureIndex = OtherFieldInSameFrame.PictureIndex;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].FieldType = OtherFieldInSameFrame.FieldType;
        }
        else
        {
            /* The Current field is 2nd in the Frame so the other Field is the Previous Field */
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureBuffer_p = OtherFieldInSameFrame.PictureBuffer_p;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureIndex = OtherFieldInSameFrame.PictureIndex;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].FieldType = OtherFieldInSameFrame.FieldType;
        }
    }
    else
    {
        /* Display is going backward */
        if ( ( (TopFieldFirst) && (LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].FieldType == HALDISP_TOP_FIELD) ) ||
              ( (!TopFieldFirst)  && (LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].FieldType == HALDISP_BOTTOM_FIELD) ) )
        {
            /* The Current field is First in the Frame so the other Field is the Previous Field (because we're going backward!!!) */
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureBuffer_p = OtherFieldInSameFrame.PictureBuffer_p;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureIndex = OtherFieldInSameFrame.PictureIndex;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].FieldType = OtherFieldInSameFrame.FieldType;
        }
        else
        {
            /* The Current field is 2nd in the Frame so the other Field is the Next Field (because we're going backward!!!) */
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureBuffer_p = OtherFieldInSameFrame.PictureBuffer_p;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureIndex = OtherFieldInSameFrame.PictureIndex;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].FieldType = OtherFieldInSameFrame.FieldType;
        }
    }

}

/*******************************************************************************
Name        : SearchNextField
Description : This function looks for the Next Field. We first look in the list of Fields currently locked.
                    If we don't find it, we will look for it in the Display Queue.
                    Most of the time the 2nd case would be enough but this is not the case when we do a
                    Freeze. In that case only the list of Pictures currently locked contain the Next Field.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_SearchNextField(const VIDDISP_Handle_t DisplayHandle,
                                    U32 LayerIdent,
                                    VIDDISP_Picture_t * NextPicture_p,
                                    HALDISP_FieldType_t FieldType)
{
    U32                     CurrentPictureIndex;
    BOOL                    FieldFound;
    HALDISP_FieldType_t     ExpectedFieldType;

    /* This is the PictureIndex of the Frame containing the Current Field */
    CurrentPictureIndex = NextPicture_p->PictureIndex;
    ExpectedFieldType = SWAP_FIELD_TYPE(FieldType);

    /* We're looking for a Picture with a PictureIndex > CurrentPictureIndex
       (The case of a Picture with the same PictureIndex has already been managed in viddisp_SetOtherFieldInSameFrame() )*/

    /* First we look in the list of pictures currently locked */
    viddisp_SearchNextFieldInListOfPicturesLocked(DisplayHandle,
                                                  LayerIdent,
                                                  CurrentPictureIndex,
                                                  ExpectedFieldType,
                                                  &FieldFound);
    if (FieldFound == FALSE)
    {
        /* Then we look in the Display Queue... */
        viddisp_SearchNextFieldInDisplayQueue(DisplayHandle,
                                              LayerIdent,
                                              NextPicture_p,
                                              CurrentPictureIndex,
                                              ExpectedFieldType);
    }

}

/*******************************************************************************
Name        : viddisp_SearchNextFieldInListOfPicturesLocked
Description : This function looks for the Next Field in the list of Pictures currently Locked
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_SearchNextFieldInListOfPicturesLocked(const VIDDISP_Handle_t  DisplayHandle,
                                                          U32                     LayerIdent,
                                                          U32                     CurrentPictureIndex,
                                                          HALDISP_FieldType_t     ExpectedFieldType,
                                                          BOOL *                  FieldFound_p)
{
    VIDDISP_Data_t *     VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_t * LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);
    int                  i;

    *FieldFound_p = FALSE;

    /* Go through all the Fields currently used by the hardware (in chronological order)  */
    for (i=0; i<VIDDISP_NBR_OF_FIELD_REFERENCES ; i++)
    {
        if ( (LayerDisplayParams_p->FieldsUsedByHw_p[i].PictureBuffer_p != NULL) &&
             (LayerDisplayParams_p->FieldsUsedByHw_p[i].PictureIndex > CurrentPictureIndex) &&
             (LayerDisplayParams_p->FieldsUsedByHw_p[i].FieldType == ExpectedFieldType) )
        {
            /* We've found a Field greater than the Current Field and with the right Field Type: We take it as the Next Field */
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureBuffer_p = LayerDisplayParams_p->FieldsUsedByHw_p[i].PictureBuffer_p;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureIndex = LayerDisplayParams_p->FieldsUsedByHw_p[i].PictureIndex;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].FieldType = LayerDisplayParams_p->FieldsUsedByHw_p[i].FieldType;
            *FieldFound_p = TRUE;
            break;   /* Leave the FOR loop... */
        }
    }

}

/*******************************************************************************
Name        : viddisp_SearchNextFieldInDisplayQueue
Description : This function looks for the Next Field in the Display Queue
                    It uses the linked list to go through the list of pictures to display.
                    The PictureIndex is used to ignore the repeated pictures.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_SearchNextFieldInDisplayQueue(const VIDDISP_Handle_t  DisplayHandle,
                                                  U32                     LayerIdent,
                                                  VIDDISP_Picture_t *     NextPicture_p,
                                                  U32                     CurrentPictureIndex,
                                                  HALDISP_FieldType_t     ExpectedFieldType)
{
    VIDDISP_Picture_t *         Picture_p;
    VIDDISP_Data_t *            VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_t * LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);
    UNUSED_PARAMETER(ExpectedFieldType);

    /* Init Picture pointer to the Picture containing the Current Field */
    Picture_p = NextPicture_p;

    /* A picture may be repeated in the Picture Queue so we look for
         the first Picture with a DIFFERENT Picture Id */
    while (Picture_p != NULL)
    {
        if (Picture_p->PictureIndex != CurrentPictureIndex)
        {
            /* We've found a picture with a different Picture Id */
            if (LayerDisplayParams_p->IsLastPicturePreparedDecimated)
            {
                LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureBuffer_p = Picture_p->SecondaryPictureBuffer_p;
            }
            else
            {
                LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureBuffer_p = Picture_p->PrimaryPictureBuffer_p;
            }
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].PictureIndex = Picture_p->PictureIndex;

            /* Check the Picture Structure in order to find what is the First Field */
            switch (Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure)
            {
                case STVID_PICTURE_STRUCTURE_FRAME:
                    /* This Picture has a Frame Structure */
                    if (VIDDISP_Data_p->DisplayForwardNotBackward)
                    {
                        /* Display is going forward: Check which Field comes first*/
                        if (Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst)
                        {
                            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].FieldType = HALDISP_TOP_FIELD;
                        }
                        else
                        {
                            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].FieldType = HALDISP_BOTTOM_FIELD;
                        }
                    }
                    else
                    {
                        /* Display is going backward: Check which Field comes first*/
                        if (Picture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst)
                        {
                            /* Display is going backward and Top Field is First so the Next Field is the Bottom Field */
                            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].FieldType = HALDISP_BOTTOM_FIELD;
                        }
                        else
                        {
                            /* Display is going backward and Bottom Field is First so the Next Field is the Top Field */
                            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].FieldType = HALDISP_TOP_FIELD;
                        }
                    }
                    break;

                case STVID_PICTURE_STRUCTURE_TOP_FIELD:
                     /* This Picture has a Field Structure so we don't have to bother about which Field comes first */
                     LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].FieldType = HALDISP_TOP_FIELD;
                     break;

                case STVID_PICTURE_STRUCTURE_BOTTOM_FIELD:
                     /* This Picture has a Field Structure so we don't have to bother about which Field comes first */
                     LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_NEXT_FIELD].FieldType = HALDISP_BOTTOM_FIELD;
                     break;
            }

            break; /* Leave the while loop... */
        }

        Picture_p = Picture_p->Next_p;
    }
}


/*******************************************************************************
Name        : SearchPreviousField
Description : This function looks for the Previous Field.
                    The Previous Field is no more in the Display queue because it has already been displayed
                    so we look for it in the Fields currently used (and locked) by the hardware
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_SearchPreviousField(const VIDDISP_Handle_t DisplayHandle, U32 LayerIdent)
{
    U32                     PictureIndex;
    S32                     i;
    VIDDISP_Data_t *        VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_t * LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);
    HALDISP_FieldType_t     ExpectedFieldType;
    BOOL    FieldFound = FALSE;

    /* This is the PictureIndex of the Frame containing the Current Field */
    PictureIndex = LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureIndex;
    ExpectedFieldType = SWAP_FIELD_TYPE(LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].FieldType);

    /* Go through all the Fields currently used by the hardware (in inverse chronological order) */
    for (i=(VIDDISP_NBR_OF_FIELD_REFERENCES-1); i>=0 ; i--)
    {
        if ( (LayerDisplayParams_p->FieldsUsedByHw_p[i].PictureBuffer_p != NULL) &&
              (LayerDisplayParams_p->FieldsUsedByHw_p[i].PictureIndex < PictureIndex) &&
              (LayerDisplayParams_p->FieldsUsedByHw_p[i].FieldType == ExpectedFieldType) )
        {
            /* We've found a Field "older" than the Current Field and with the expected Field Type: We take it as the Previous Field */
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureBuffer_p = LayerDisplayParams_p->FieldsUsedByHw_p[i].PictureBuffer_p;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureIndex = LayerDisplayParams_p->FieldsUsedByHw_p[i].PictureIndex;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].FieldType = LayerDisplayParams_p->FieldsUsedByHw_p[i].FieldType;
            FieldFound = TRUE;
            break;   /* Leave the FOR loop... */
        }
    }

    if (!FieldFound)
    {
        /* We haven't found any Field with a lower PictureIndex and the right Type.
            If the Current Field used by the hardware has the right parity and if it is different than the Current Field used at Next Vsync, we
            take it as the Previous Field */
        if ( (LayerDisplayParams_p->FieldsUsedByHw_p[VIDDISP_CURRENT_FIELD].PictureBuffer_p != LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].PictureBuffer_p) ||
             (LayerDisplayParams_p->FieldsUsedByHw_p[VIDDISP_CURRENT_FIELD].FieldType != LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_CURRENT_FIELD].FieldType) )
        {
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureBuffer_p = LayerDisplayParams_p->FieldsUsedByHw_p[VIDDISP_CURRENT_FIELD].PictureBuffer_p;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].PictureIndex = LayerDisplayParams_p->FieldsUsedByHw_p[VIDDISP_CURRENT_FIELD].PictureIndex;
            LayerDisplayParams_p->FieldsUsedAtNextVSync_p[VIDDISP_PREVIOUS_FIELD].FieldType = LayerDisplayParams_p->FieldsUsedByHw_p[VIDDISP_CURRENT_FIELD].FieldType;
        }
    }
}

/*******************************************************************************
Name        : viddisp_InitFieldArray
Description : This function inits a Field Array.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_InitFieldArray(const VIDDISP_Handle_t DisplayHandle,
                                    U32 LayerIdent,
                                    HALDISP_Field_t * FieldArray_p)
{
    U32     i;
    UNUSED_PARAMETER(DisplayHandle);
    UNUSED_PARAMETER(LayerIdent);

    if (FieldArray_p == NULL)
    {
        return;
    }

    for (i=0; i<VIDDISP_NBR_OF_FIELD_REFERENCES; i++)
    {
        FieldArray_p[i].PictureBuffer_p = NULL;
        FieldArray_p[i].PictureIndex = 0;
        /* "FieldArray_p[i].FieldType" does not matter */
    }
}


/*******************************************************************************
Name        : viddisp_LockFieldArray
Description : This function locks a Field array (while these Fields are used by the deinterlacer).
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_LockFieldArray(const VIDDISP_Handle_t DisplayHandle,
                                   U32 LayerIdent,
                                   HALDISP_Field_t * FieldArray_p)
{
    U32       i;
    VIDDISP_Data_t      * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    UNUSED_PARAMETER(LayerIdent);

    for(i=0; i<VIDDISP_NBR_OF_FIELD_REFERENCES; i++)
    {
        if (FieldArray_p[i].PictureBuffer_p != NULL)
        {
            TrLock(("Layer %d: Lock Picture 0x%x\r\n", LayerIdent, FieldArray_p[i].PictureBuffer_p));
            TAKE_PICTURE_BUFFER(VIDDISP_Data_p->BufferManagerHandle, FieldArray_p[i].PictureBuffer_p, VIDCOM_DEI_MODULE_BASE);
        }
    }
}

/*******************************************************************************
Name        : viddisp_UnlockAllFields
Description : This function unlocks all the Field Arrays and reset their content
Parameters  :
Assumptions : This function should be called only when the Display Terminates
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddisp_UnlockAllFields(const VIDDISP_Handle_t DisplayHandle, U32 LayerIdent)
{
    VIDDISP_Data_t *   VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_t * LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);

    viddisp_ResetFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->FieldsUsedAtNextVSync_p);
    viddisp_ResetFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->FieldsUsedByHw_p);
    viddisp_ResetFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->FieldsNotUsed_p);
    viddisp_ResetFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->FieldsToUnlock_p);

    TrMain(("Layer %d: All Field arrays have been unlocked and reset\r\n", LayerIdent));
}

/*******************************************************************************
Name        : viddisp_IsDeinterlacingPossible
Description : This function check if it is necessary to find the Fields Previous, Current, Next for Deinterlacing
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
BOOL viddisp_IsDeinterlacingPossible(const VIDDISP_Handle_t DisplayHandle, U32 LayerIdent)
{
    BOOL                      DeinterlacingPossible;
    VIDDISP_Data_t *   VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    /* Check if Deinterlacing is possible with this kind of Layer */
    switch (VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType)
    {
        case STLAYER_DISPLAYPIPE_VIDEO1 :   /* main HD for 7109 */
        case STLAYER_DISPLAYPIPE_VIDEO2 :   /* main HD for 7200 */

            /* Field Previous, Current, Next needed for Deinterlacing */
            DeinterlacingPossible = TRUE;
            break;

        default:
            DeinterlacingPossible = FALSE;
            break;
    }

    return (DeinterlacingPossible);
}

#if 0
/*******************************************************************************
Name        : viddisp_IsDisplayPipe
Description : This function check if the layer is a display pipe
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static BOOL viddisp_IsDisplayPipe(const VIDDISP_Handle_t DisplayHandle,
                                  U32 LayerIdent)
{
    BOOL                      IsDisplayPipe;
    VIDDISP_Data_t *   VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    /* Check if this kind of Layer is a display pipe one */
    switch (VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType)
    {
        case STLAYER_DISPLAYPIPE_VIDEO1 :   /* main HD for 7109 */
        case STLAYER_DISPLAYPIPE_VIDEO2 :   /* main HD for 7200 */
        case STLAYER_DISPLAYPIPE_VIDEO3 :   /* Local SD (SD0) for 7200. Unused otherwise*/
        case STLAYER_DISPLAYPIPE_VIDEO4 :   /* Remote SD(SD1) for 7200. Unused otherwise*/
            IsDisplayPipe = TRUE;
            break;

        default:
            IsDisplayPipe = FALSE;
            break;
    }

    return (IsDisplayPipe);
}
#endif

/*******************************************************************************
Name        : viddisp_SetFieldsProtections
Description : This function decides which Fields should be locked or unlocked.
              We have to protect:
              - the Fields used by the hardware
              - the Fields that will be used at Next VSync
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_SetFieldsProtections(const VIDDISP_Handle_t DisplayHandle,
                                         U32 LayerIdent)
{
    VIDDISP_Data_t *            VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    LayerDisplayParams_t * LayerDisplayParams_p = &(VIDDISP_Data_p->LayerDisplay[LayerIdent]);

    /* Protect the Fields that will be used at the next VSync */
    viddisp_LockFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->FieldsUsedAtNextVSync_p);

    /* Unprotect the Fields previously used by the hardware */
    viddisp_ResetFieldArray(DisplayHandle, LayerIdent, LayerDisplayParams_p->FieldsToUnlock_p);

    /* The Fields Unlocked are not needed anymore so this Field array is available for reuse */
    LayerDisplayParams_p->FieldsNotUsed_p = LayerDisplayParams_p->FieldsToUnlock_p;
}

#ifdef ST_XVP_ENABLE_FGT
/*******************************************************************************
Name        : viddisp_StoreFGTParams
Description : Function to store FGT params in FieldsUsedAtNextVSync_p
Parameters  : Display handle, Layer params, Next Picture data
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void viddisp_StoreFGTParams(     const VIDDISP_Handle_t  DisplayHandle,
                                        VIDDISP_Picture_t       *NextPicture_p,
                                        HALDISP_FieldType_t     FieldType)
{
    VIDFGT_TransformParam_t     *FGTparams_p;
    VIDDISP_Data_t              *VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    /* if FGT has bee activated (database created and STLAYER_XVPInit done) */
    if (VIDFGT_IsActivated(VIDDISP_Data_p->FGTHandle))
    {
        /* get FGT params from picture parsing header */
        VIDFGT_UpdateFilmGrainParams(NextPicture_p->PrimaryPictureBuffer_p, VIDDISP_Data_p->FGTHandle);
        FGTparams_p = (VIDFGT_TransformParam_t *)VIDFGT_GetFlexVpParamsPointer(VIDDISP_Data_p->FGTHandle);

        /* if params exists and video is playing */
        if (FGTparams_p)
        {
            FGTparams_p->UserZoomOut     = 0x08;
            FGTparams_p->FieldType       = (U32)FieldType;
            FGTparams_p->PictureSize     = (NextPicture_p->PictureInformation_p->PictureDecodingContext.GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Height << 16)
                                            + NextPicture_p->PictureInformation_p->PictureDecodingContext.GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Width;
        }
    }
}
#endif /* ST_XVP_ENABLE_FGT */

/******************************************************************************/


/* End of vid_disp.c */
/******************************************************************************/

