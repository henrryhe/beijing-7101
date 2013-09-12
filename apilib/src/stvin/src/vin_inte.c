/*******************************************************************************

File name   : vin_inte.c

Description : Video input interrupt source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
10 Aou 2000        Created                                          JA
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <string.h>
#endif /* ST_OSLINUX */

#include "sttbx.h"
#include "stddefs.h"
#include "stevt.h"
#include "stsys.h"
#include "stclkrv.h"

#include "vin_inte.h"
#include "vin_com.h"
#include "halv_vin.h"


/* Private preliminary definitions (internal use only) ---------------------- */
#define STVIN_MAX_ANCILLARY_DATA_SIZE           (21*944)/* Standard case ! 21 lines, 944 pixels/line PAL-SQ Standard */
/*#define TRACE_UART*/
/*#define TRACE_FRAME_BUFFER_STATUS*/

#ifdef TRACE_FRAME_BUFFER_STATUS
#define TraceFrameBufferStatus(x) TraceState x
#else
#define TraceFrameBufferStatus(x)
#endif  /* TRACE_FRAME_BUFFER_STATUS */

#ifdef TRACE_UART
# define VIN_TraceBuffer(x) TraceBuffer(x)
void VIN_TracePictureBufferQueue (stvin_Device_t * const Device_p);
void VIN_TracePictureStructure (STVID_PictureStructure_t PictureStructure, U32 ExtendedTemporalReference);
#else
# define VIN_TraceBuffer(x)
# define VIN_TracePictureBufferQueue(x)
# define VIN_TracePictureStructure(x,y)
#endif

#if defined TRACE_UART || defined TRACE_FRAME_BUFFER_STATUS
# include "../../tests/src/trace.h"
#endif

#define ANCILLARY_PKT_HDR_LEN       6

/* #define DIG_INP_DEBUG 1 */

/* Private Function prototypes ---------------------------------------------- */

static ST_ErrorCode_t UpdateFrameBuffer(stvin_Device_t * const Device_p,
        const STVID_PictureStructure_t PictureStructure);
static ST_ErrorCode_t UpdateAncillaryDataBuffer(stvin_Device_t * const Device_p);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : stvin_InterruptHandler
Description : Recept interrupt event
Parameters  : EVT standard prototype
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
#if !(defined(ST_7710) || defined(ST_5528) || defined(ST_7100) || defined(ST_7109))
void stvin_InterruptHandler(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
#else
STOS_INTERRUPT_DECLARE(stvin_InterruptHandler, Param)
#endif /* defined(ST_7710) || defined(ST_5528) || defined(ST_7100) defined(ST_7109) */
{
    stvin_Device_t * Device_p;
    U32 InterruptStatus;

#if !(defined(ST_7710) || defined(ST_5528) || defined(ST_7100) || defined(ST_7109))
    Device_p = (stvin_Device_t *) SubscriberData_p;
#else
    Device_p = (stvin_Device_t*) Param;
#endif /* !(defined(ST_7710) || defined(ST_5528) || defined(ST_7100) defined(ST_7109)) */
    /* Read the interrupt status register */
    InterruptStatus = HALVIN_GetInterruptStatus(Device_p->HALInputHandle);

    /* Read the interrupt status register */
    if (InterruptStatus != 0)
    {
        PushInterruptCommand(Device_p, InterruptStatus);

        /* Signal controller that a new command is given */
        STOS_SemaphoreSignal(Device_p->ForTask.InputOrder_p);
   }

    STOS_INTERRUPT_EXIT(STOS_SUCCESS);

} /* End of stvin_InterruptHandler() function */


/*******************************************************************************
Name        : stvin_ProcessInterrupt
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stvin_ProcessInterrupt(stvin_Device_t * const Device_p, const U32 InterruptStatus)
{
    U32 VideoInterruptStatus;
    U8 TrapExit;
    U8 Tmp8;

    /* ******************************  DEBUG TRACES  ****************************** */
    VIN_TraceBuffer(("\r\n** VSYNC VIN (%s) **\r\n ",
        ( (InterruptStatus&HALVIN_VERTICAL_SYNC_BOTTOM_MASK) == HALVIN_VERTICAL_SYNC_BOTTOM_MASK ? "Bot" :
          (InterruptStatus&HALVIN_VERTICAL_SYNC_TOP_MASK)    == HALVIN_VERTICAL_SYNC_TOP_MASK    ? "Top" : "BotTop") ));

    /* Pop VSync Controller command */
    while (PopVSyncControllerCommand(Device_p, &Tmp8) == ST_NO_ERROR)
    {
        VIN_TraceBuffer(("** PopVSyncControllerCommand : %d \r\n", Tmp8));
        switch ((AfterVsyncCommandID_t) Tmp8)
        {
            HALVIN_Handle_t HALHandle;
            HALVIN_Properties_t * HALVIN_Data_p;

            case SYNC_SET_IO_WINDOW :
                HALHandle = (HALVIN_Handle_t)(Device_p->HALInputHandle);
                HALVIN_Data_p = (HALVIN_Properties_t *) HALHandle;

                HALVIN_SetSizeOfTheFrame(HALHandle,
                                         HALVIN_Data_p->VideoParams_p->FrameWidth,
                                         HALVIN_Data_p->VideoParams_p->FrameHeight);
                break;
            default:
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Unknown command!"));
                break;
        } /* end switch(command) */
    } /* end while(controller command) */

    /* Mask the IT's not used */
    VideoInterruptStatus = InterruptStatus & Device_p->VideoInterrupt.Mask;

    if (Device_p->ForTask.FirstStart == TRUE)
    {
        if (Device_p->VideoParams.ScanType == STGXOBJ_INTERLACED_SCAN)
        {
            if (VideoInterruptStatus != HALVIN_VERTICAL_SYNC_TOP_MASK)
            {
            /* ******************************  DEBUG TRACES  ****************************** */
                VIN_TraceBuffer(("Bot Vsync trashed"));
            return;
            }
            /* At this time, TOP frame in progress */
            Device_p->ForTask.NextPictureStructureExpected = STVID_PICTURE_STRUCTURE_BOTTOM_FIELD;
        }
        else
        {
            Device_p->ForTask.NextPictureStructureExpected = STVID_PICTURE_STRUCTURE_FRAME;
        }
        /* Flag FirstStart will be reset when processing the first interrupt.   */
    } /* (Device_p->ForTask.FirstStart == TRUE) */

    TrapExit = 2;
    while ((VideoInterruptStatus != 0)  && ((TrapExit--) != 0))
    {
        /* Stay inside interrupt routine until all the bits have been tested */

        switch (Device_p->ForTask.NextPictureStructureExpected)
        {
            case STVID_PICTURE_STRUCTURE_FRAME :
                    /* Erase the current interrupt status.              */
                    VideoInterruptStatus &= !(HALVIN_VERTICAL_SYNC_BOTTOM_MASK | HALVIN_VERTICAL_SYNC_TOP_MASK);

                    /* Update pointer to get the frame in the memory    */
                    UpdateFrameBuffer(Device_p, STVID_PICTURE_STRUCTURE_FRAME);

                    /* Ancillary Data Pointer */
                    if (Device_p->AncillaryDataEventEnabled)
                    {
                        UpdateAncillaryDataBuffer(Device_p);
                    }
                break;

            case STVID_PICTURE_STRUCTURE_TOP_FIELD :
                /*----------------------------------------------------------------------
                VSB Bit VSYNC Bottom.
                ----------------------------------------------------------------------*/
                if ((VideoInterruptStatus & HALVIN_VERTICAL_SYNC_BOTTOM_MASK) != 0)

                {
                    VideoInterruptStatus ^= HALVIN_VERTICAL_SYNC_BOTTOM_MASK;

                    /* Update pointer to get the frame in the memory */
                    UpdateFrameBuffer(Device_p, STVID_PICTURE_STRUCTURE_TOP_FIELD);
                    /* Warning: TOP! because it's for the next Frame! */

                    Device_p->ForTask.NextPictureStructureExpected = STVID_PICTURE_STRUCTURE_BOTTOM_FIELD;

                    /* Ancillary Data Pointer */
                    if (Device_p->AncillaryDataEventEnabled)
                    {
                        UpdateAncillaryDataBuffer(Device_p);
                    }
                } /* end HALVIN_VERTICAL_SYNC_BOTTOM_MASK interrupt handling */
                break;

            case STVID_PICTURE_STRUCTURE_BOTTOM_FIELD :
                /*----------------------------------------------------------------------
                VST Bit VSYNC Top.
                ----------------------------------------------------------------------*/
                if ((VideoInterruptStatus & HALVIN_VERTICAL_SYNC_TOP_MASK) != 0)
                {
                    VideoInterruptStatus ^= HALVIN_VERTICAL_SYNC_TOP_MASK;

                    /* Update pointer to get the frame in the memory */
                    UpdateFrameBuffer(Device_p, STVID_PICTURE_STRUCTURE_BOTTOM_FIELD);
                    /* Warning: BOTTOM! because it's for the next Frame! */

                    Device_p->ForTask.NextPictureStructureExpected = STVID_PICTURE_STRUCTURE_TOP_FIELD;

                    /* AncillaryData */
                    if (Device_p->AncillaryDataEventEnabled)
                    {
                        UpdateAncillaryDataBuffer(Device_p);
                    }

                } /* end HALVIN_VERTICAL_SYNC_TOP_MASK interrupt handling */
                break;

            default :
                /* Should never happen. Set a default value !!! */
                Device_p->ForTask.NextPictureStructureExpected = STVID_PICTURE_STRUCTURE_FRAME;
                break;

        } /* End of switch (Device_p->ForTask.NextPictureStructureExpected) */
    } /* End of while(VideoInterruptStatus != 0) */

} /* End of vidinp_ProcessInterrupt() function */

/*******************************************************************************
Name        : UpdateAncillaryDataBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static ST_ErrorCode_t UpdateAncillaryDataBuffer(stvin_Device_t * const Device_p)
{

    STVIN_UserData_t            EvtData;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    U8                          CurrentIndex, OldIndex, NextIndex =0;
    char                        *OverflowData_p;
    char                        ResetPattern = 0x00;
    U8                          *BufferForDvp;
    U32                         WritenDataLen;
    U32                         CurrentPacketDataLen;
    U32                         LastPositionInAncillaryDataBuffer = 0;
    U32                         i;
#ifdef WA_WRONG_CIRCULAR_DATA_WRITE
    U8                          CircularBufferRestartOfset;
#endif

    CurrentIndex = (Device_p->ForTask.AncillaryDataCurrentIndex) % Device_p->AncillaryDataBufferNumber;
    OldIndex = (-1 + Device_p->AncillaryDataBufferNumber + Device_p->ForTask.AncillaryDataCurrentIndex) % Device_p->AncillaryDataBufferNumber;
    NextIndex = (1 + Device_p->ForTask.AncillaryDataCurrentIndex) % Device_p->AncillaryDataBufferNumber;

    Device_p->ForTask.AncillaryDataCurrentIndex = NextIndex;

    if (Device_p->EnableAncillaryDatatEventAfterStart)
    {
        if ((Device_p->DeviceType == STVIN_DEVICE_TYPE_ST7710_DVP_INPUT) ||
            (Device_p->DeviceType == STVIN_DEVICE_TYPE_ST5528_DVP_INPUT) ||
            (Device_p->DeviceType == STVIN_DEVICE_TYPE_ST7100_DVP_INPUT) ||
            (Device_p->DeviceType == STVIN_DEVICE_TYPE_ST7109_DVP_INPUT))
        {
            /* Here we'll use a circular data buffer */

#ifdef WA_WRONG_CIRCULAR_DATA_WRITE
            /* For 7710 the hard writes data, after reaching the Ancillary Data End Adress, on the (Ancillary Data Base Adress
             * - random value less than 64). The work arround is to look for the first byte within the 64 ones just before the
             * Ancillary Data Base Adress. */

            CircularBufferRestartOfset = 0;
            if (Device_p->CircularLoopDone)
            {
                for(i=1; i<=64; i++)
                {
                    if (*(((U8 *)Device_p->AncillaryDataTable[OldIndex].VirtualAddress_p)-i) != 0xfe)
                    {
                        CircularBufferRestartOfset = i;
                    }
                }
                Device_p->CircularLoopDone = FALSE;
            }
#endif /* WA_WRONG_CIRCULAR_DATA_WRITE */

            WritenDataLen = Device_p->AncillaryDataBufferSize/* + 16*/ + STVIN_MAX_ANCILLARY_DATA_SIZE;

            BufferForDvp = (U8 *)Device_p->AncillaryDataTable[OldIndex].VirtualAddress_p;

#ifdef WA_WRONG_CIRCULAR_DATA_WRITE
            BufferForDvp -= CircularBufferRestartOfset;
            WritenDataLen -= CircularBufferRestartOfset;
#endif /* WA_WRONG_CIRCULAR_DATA_WRITE */


            EvtData.BufferOverflow = FALSE;
            EvtData.Buff_p = NULL;
            EvtData.Length = 0;
            i = Device_p->LastPositionInAncillaryDataBuffer;
            while ((BufferForDvp[i] != ResetPattern) && (i < WritenDataLen))
            {
                if ( BufferForDvp[i] == 0xff )
                {
                    if (Device_p->VideoParams.ExtraParams.StandardDefinitionParams.AncillaryDataType == STVIN_ANC_DATA_RAW_CAPTURE)
                    {
                        CurrentPacketDataLen = Device_p->VideoParams.ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength;
                    }
                    else
                    {
                        /* Find length of packet (size of data is given in byte 3) */
                        CurrentPacketDataLen = 4 * (BufferForDvp[(i + 3) % WritenDataLen] & 0x3f ); /* bits 6 & 7 parity bits so mask off */
                        CurrentPacketDataLen = CurrentPacketDataLen + ANCILLARY_PKT_HDR_LEN - 1; /* minus 1 as the end byte (a filler) is not copied into BufferForDVP  */
                    }

                    EvtData.Buff_p = (&BufferForDvp[i]);
                    EvtData.Length = CurrentPacketDataLen;
                    if( (EvtData.Buff_p != NULL) && (EvtData.Length != 0) )
                    {
                            EvtData.BufferOverflow = FALSE;
                            STEVT_Notify(Device_p->EventsHandle, Device_p->RegisteredEventsID[STVIN_USER_DATA_EVT_ID],
                                    (const void *) (&EvtData));
                    }
                    i += CurrentPacketDataLen;
                }
                LastPositionInAncillaryDataBuffer = (i%WritenDataLen);
                i++;
            }
            Device_p->LastPositionInAncillaryDataBuffer = LastPositionInAncillaryDataBuffer;

        } /* 7710 || 7100 || 7109 || 5528 */
        else
        {
            EvtData.Buff_p = (const void *) (Device_p->AncillaryDataTable[OldIndex].VirtualAddress_p);
            if ((char) *((char *)(Device_p->AncillaryDataTable[OldIndex].VirtualAddress_p)) == 0)
            {
                /* No data received. */
                EvtData.Length = 0;
            }
            else
            {
                if (Device_p->VideoParams.ExtraParams.StandardDefinitionParams.AncillaryDataType == STVIN_ANC_DATA_RAW_CAPTURE)
                {
                    EvtData.Length = Device_p->VideoParams.ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength;
                }
                else
                {
                    EvtData.Length = Device_p->AncillaryDataBufferSize;
                }
            }

            OverflowData_p = (char *)(Device_p->AncillaryDataTable[OldIndex].VirtualAddress_p) + Device_p->AncillaryDataBufferSize + 1;
            EvtData.BufferOverflow = (*OverflowData_p == 0x00 ? FALSE : TRUE);

            STEVT_Notify(Device_p->EventsHandle, Device_p->RegisteredEventsID[STVIN_USER_DATA_EVT_ID],
                    (const void *) (&EvtData));
        }
    }
    else
    {
        /* The first time, do not send the event. */
        Device_p->EnableAncillaryDatatEventAfterStart = TRUE;
    }

     if ((Device_p->DeviceType != STVIN_DEVICE_TYPE_ST7710_DVP_INPUT) &&
         (Device_p->DeviceType != STVIN_DEVICE_TYPE_ST5528_DVP_INPUT) &&
         (Device_p->DeviceType != STVIN_DEVICE_TYPE_ST7100_DVP_INPUT) &&
         (Device_p->DeviceType != STVIN_DEVICE_TYPE_ST7109_DVP_INPUT))
    {
       /* Fill the start of buffer with zero values.                           */
        /* PS : size is AncillaryDataBufferSize+1 because the array element in  */
        /* AncillaryDataBufferSize+1 will be tested for overflow.               */
        ErrorCode = STAVMEM_FillBlock1D((void *)&ResetPattern, sizeof (char),
                Device_p->AncillaryDataTable[NextIndex].VirtualAddress_p,
                Device_p->AncillaryDataBufferSize + 1);

        /* And set its address as buffer to be filled in ancillary datat capture.   */
        HALVIN_SetAncillaryDataPointer(Device_p->HALInputHandle,
                Device_p->AncillaryDataTable[NextIndex].Address_p, Device_p->AncillaryDataBufferSize);
    }
    else
    {
        if (Device_p->LastPositionInAncillaryDataBuffer > Device_p->AncillaryDataBufferSize)
        {   /* here we have to use the new data buffer */

            /* Fill the start of buffer with zero values.                           */
            /* PS : size is AncillaryDataBufferSize+1 because the array element in  */
            /* AncillaryDataBufferSize+1 will be tested for overflow.               */
            ErrorCode = STAVMEM_FillBlock1D((void *)&ResetPattern, sizeof (char),
                    Device_p->AncillaryDataTable[NextIndex].VirtualAddress_p,
                    Device_p->AncillaryDataBufferSize + 1);

#ifdef WA_WRONG_CIRCULAR_DATA_WRITE
            for(i=1; i<=64; i++)
            {
                *(((U8 *)Device_p->AncillaryDataTable[NextIndex].VirtualAddress_p)-i) = 0xfe;
            }
            Device_p->CircularLoopDone = TRUE;
#endif
            Device_p->LastPositionInAncillaryDataBuffer = 0;
            /* And set its address as buffer to be filled in ancillary datat capture.   */
            HALVIN_SetAncillaryDataPointer(Device_p->HALInputHandle,
                    Device_p->AncillaryDataTable[NextIndex].Address_p, Device_p->AncillaryDataBufferSize);
        }
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : UpdateFrameBuffer
Description : Called under VSync (top or bottom), this function updates frame
              buffers on display.
Parameters  : Device_p, device parameters.
              PictureStructure, structure of picture to be input.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, if OK, other if error.
*******************************************************************************/
static ST_ErrorCode_t UpdateFrameBuffer(stvin_Device_t * const Device_p,
        const STVID_PictureStructure_t PictureStructure)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVID_GetPictureBufferParams_t GetPictureBufferParams;

    U8 PreviousIndex;
    STVID_PictureBufferHandle_t     PreviousPictureBufferHandle;
    STVID_PictureBufferDataParams_t PreviousPictureBufferDataParams;

    U8 CurrentIndex;
    STVID_PictureBufferHandle_t     CurrentPictureBufferHandle;
    STVID_PictureBufferDataParams_t CurrentPictureBufferDataParams;

    /* Parameters of next buffer to be prepared.                            */
    U8 NextIndex;
    STVID_PictureBufferHandle_t     NextPictureBufferHandle;
    STVID_PictureBufferDataParams_t NextPictureBufferDataParams;


    U8 LastChanceBufferIndex;

#ifdef ST_stclkrv
    STCLKRV_ExtendedSTC_t ExtendedSTC;
#endif
    STVID_PTS_t CurrentSTC;

    /* Get the CurrentIndex : Index of buffer lastly prepared.              */
    CurrentIndex = Device_p->ForTask.PictureBufferCurrentIndex;

    /* Calculate the previous index.                                                */
    PreviousIndex = (CurrentIndex==0 ? STVIN_MAX_NB_FRAME_IN_VIDEO-1 : CurrentIndex-1);

    /* Increment the current index, so manage the next position of buffer.          */
    Device_p->ForTask.PictureBufferCurrentIndex++;
    if (Device_p->ForTask.PictureBufferCurrentIndex == STVIN_MAX_NB_FRAME_IN_VIDEO)
    {
        Device_p->ForTask.PictureBufferCurrentIndex = 0;
    }
    /* Then, it becomes the next index (Next picture buffer to be filled by D1).    */
    NextIndex   = Device_p->ForTask.PictureBufferCurrentIndex;

    /* Get the current picture buffer parameters.                                   */
    CurrentPictureBufferHandle      = (Device_p->PictureBufferHandle[CurrentIndex]);
    CurrentPictureBufferDataParams  = (Device_p->PictureBufferParams[CurrentIndex]);

    PreviousPictureBufferHandle     = (Device_p->PictureBufferHandle[PreviousIndex]);
    PreviousPictureBufferDataParams = (Device_p->PictureBufferParams[PreviousIndex]);

    /* ******************************     STEP  1    ****************************** */
    /* **              Prepare a picture buffer for the next D1 input            ** */
    /* **************************************************************************** */
    if (Device_p->DummyBufferUseStatus == BOTH_BUFFER)
    {
        Device_p->DummyBufferUseStatus = NONE;
    }
    if (Device_p->PreviousBufferUseStatus == BOTH_BUFFER)
    {
        Device_p->PreviousBufferUseStatus = NONE;
    }

    if ( (Device_p->DummyBufferUseStatus == NONE) && (Device_p->PreviousBufferUseStatus == NONE) )
    {
        /* Want to get frame and display: get a frame first */
        GetPictureBufferParams.PictureStructure     = PictureStructure;
        GetPictureBufferParams.TopFieldFirst        = ((PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD) ? FALSE : TRUE);
        GetPictureBufferParams.ExpectingSecondField = ((PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD) ? TRUE : FALSE);
        GetPictureBufferParams.ExtendedTemporalReference = Device_p->ForTask.ExtendedTemporalReference;
        GetPictureBufferParams.PictureWidth         = ((HALVIN_Properties_t *) ((HALVIN_Handle_t)Device_p->HALInputHandle))->InputWindow.OutputWinWidth;
        GetPictureBufferParams.PictureHeight        = ((HALVIN_Properties_t *) ((HALVIN_Handle_t)Device_p->HALInputHandle))->InputWindow.OutputWinHeight;

        /* Get a frame to input in */
        ErrorCode = STVID_GetPictureBuffer(Device_p->VideoHandle,
                                        &GetPictureBufferParams,
                                        &NextPictureBufferDataParams,
                                        &NextPictureBufferHandle);
        Device_p->ExtendedTemporalReference[NextIndex]
                = GetPictureBufferParams.ExtendedTemporalReference;
    }

    if ((ErrorCode != ST_NO_ERROR) || (Device_p->DummyBufferUseStatus != NONE)
         || (Device_p->PreviousBufferUseStatus != NONE))
    {
        /* Couldn't get a frame.                                    */
        if (Device_p->DummyProgOrTopFieldBufferHandle != (STVID_PictureBufferHandle_t) NULL)
        {
            /* At least on dummy buffer is available. So use it.    */
                VIN_TraceBuffer(("!! WARNING! Dummy buffer used !!!!\r\n"));

            /* ***** No available free buffer !!!! Use a Dummy one.     */
            if (PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
            {
                NextPictureBufferHandle     = Device_p->DummyBotFieldBufferHandle;
                NextPictureBufferDataParams = Device_p->DummyBotFieldBufferParams;
                Device_p->DummyBufferUseStatus  |= BOTH_BUFFER;
            }
            else
            {
                NextPictureBufferHandle     = Device_p->DummyProgOrTopFieldBufferHandle;
                NextPictureBufferDataParams = Device_p->DummyProgOrTopFieldBufferParams;
                if (PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
                {
                    Device_p->DummyBufferUseStatus  |= FRAME_OR_TOP_BUFFER;
                }
                else
                {
                    Device_p->DummyBufferUseStatus  |= BOTH_BUFFER;
                }
            }
        }
        else
        {
            /* At least one buffer is available. So use it.                 */
                VIN_TraceBuffer(("!! WARNING! Previous buffer used !!!!\r\n"));

            /* The use of dummy buffers is not allowed. Use last inserted   */
            /* buffer instead.                                              */
            if (PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
            {
                LastChanceBufferIndex = CurrentIndex;
                Device_p->PreviousBufferUseStatus = BOTH_BUFFER;
            }
            else
            {
                if (PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
                {
                    Device_p->PreviousBufferUseStatus = FRAME_OR_TOP_BUFFER;
                }
                else
                {
                    Device_p->PreviousBufferUseStatus = BOTH_BUFFER;
                }
                LastChanceBufferIndex = PreviousIndex;
            }
            NextPictureBufferHandle     = Device_p->PictureBufferHandle[LastChanceBufferIndex];
            NextPictureBufferDataParams = Device_p->PictureBufferParams[LastChanceBufferIndex];
        }
    }

    /* Update Picture buffer & Register */
    UpdatePictureBuffer(Device_p, PictureStructure,
            &(Device_p->PictureInfos[NextIndex]), &NextPictureBufferDataParams);
    Device_p->PictureBufferHandle[NextIndex] = NextPictureBufferHandle;
    Device_p->PictureBufferParams[NextIndex] = NextPictureBufferDataParams;
#define MEHOD_CURRENT

#if defined(MEHOD_CURRENT)
        Device_p->ForTask.FirstStart = FALSE;
#else
    if (Device_p->ForTask.FirstStart == TRUE)
    {
        /* It's the first s/hSYNC to manage after the start. Do not display */
        /* the buffer as presentation registers are not yet set !!!         */
        Device_p->ForTask.FirstStart = FALSE;
        Device_p->ForTask.ExtendedTemporalReference ++ ;
        return (ST_NO_ERROR);
    }
#endif

    /* ******************************     STEP  2    ****************************** */
    /* **         Insert In display queue the Picture just filled by D1 :        ** */
    /* **         i.e. accessed by Current/PreviousIndex.                        ** */
    /* **************************************************************************** */

#if defined(MEHOD_CURRENT)
    if ( (CurrentPictureBufferHandle != NULL) &&
         (CurrentPictureBufferHandle != Device_p->DummyProgOrTopFieldBufferHandle) &&
         (CurrentPictureBufferHandle != Device_p->DummyBotFieldBufferHandle) )
    {
#ifdef ST_stclkrv
        /* Read STC/PCR to compare PTS to it */
        ErrorCode = STCLKRV_GetExtendedSTC(Device_p->STClkrvHandle, &ExtendedSTC);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* No STC available or error: can't tell about synchronization ! */
            CurrentSTC.PTS      =  0;
            CurrentSTC.PTS33    =  FALSE;

#ifdef nodef
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No STC available or error"));
#endif /*nodef*/
        }
        CurrentSTC.PTS      =  ExtendedSTC.BaseValue;
        CurrentSTC.PTS33    = (ExtendedSTC.BaseBit32 != 0);
#else /*ST_stclkrv*/
        /* No STC available : can't tell about synchronization !                */
        CurrentSTC.PTS      =  0;
        CurrentSTC.PTS33    =  FALSE;
#endif /*ST_stclkrv*/

        /* TimeStamp */
        Device_p->PictureInfos[CurrentIndex].VideoParams.PTS.PTS    = CurrentSTC.PTS;
        Device_p->PictureInfos[CurrentIndex].VideoParams.PTS.PTS33  = CurrentSTC.PTS33;
        Device_p->PictureInfos[CurrentIndex].VideoParams.PTS.Interpolated = TRUE;

          /* ******************************  DEBUG TRACES  ****************************** */
            VIN_TraceBuffer(("\r\n >> Disp"));
            VIN_TracePictureStructure (
                    Device_p->PictureInfos[CurrentIndex].VideoParams.PictureStructure ,
                    Device_p->ExtendedTemporalReference[CurrentIndex]);
            VIN_TraceBuffer((" [%x]\r\n",
                    Device_p->PictureBufferParams[CurrentIndex].Data1_p));
        TraceFrameBufferStatus(("Disp","0x%08x",Device_p->PictureBufferParams[CurrentIndex].Data1_p));
      /* Insert the last buffer in the video display queue.                           */
        ErrorCode = STVID_DisplayPictureBuffer(Device_p->VideoHandle,
                (Device_p->PictureBufferHandle[CurrentIndex]),
                &(Device_p->PictureInfos[CurrentIndex]));

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_DisplayPictureBuffer != ST_NO_ERROR"));
        }
    }
    else
    {
        /* ******************************  DEBUG TRACES  ****************************** */
        VIN_TraceBuffer(("No disp : Error !!!"));
    }
#else
    if ( (PreviousPictureBufferHandle != NULL) &&
         (PreviousPictureBufferHandle != Device_p->DummyProgOrTopFieldBufferHandle) &&
         (PreviousPictureBufferHandle != Device_p->DummyBotFieldBufferHandle) )
    {
#ifdef ST_stclkrv
        /* Read STC/PCR to compare PTS to it */
        ErrorCode = STCLKRV_GetExtendedSTC(Device_p->STClkrvHandle, &ExtendedSTC);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* No STC available or error: can't tell about synchronization ! */
            CurrentSTC.PTS      =  0;
            CurrentSTC.PTS33    =  FALSE;

#ifdef nodef
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No STC available or error"));
#endif /*nodef*/
        }
        CurrentSTC.PTS      =  ExtendedSTC.BaseValue;
        CurrentSTC.PTS33    = (ExtendedSTC.BaseBit32 != 0);

#else /*ST_stclkrv*/

        /* No STC available : can't tell about synchronization !                */
        CurrentSTC.PTS      =  0;
        CurrentSTC.PTS33    =  FALSE;

#endif /*ST_stclkrv*/

        /* TimeStamp */
        Device_p->PictureInfos[PreviousIndex].VideoParams.PTS.PTS    = CurrentSTC.PTS;
        Device_p->PictureInfos[PreviousIndex].VideoParams.PTS.PTS33  = CurrentSTC.PTS33;
        Device_p->PictureInfos[PreviousIndex].VideoParams.PTS.Interpolated = TRUE;

        /* ******************************  DEBUG TRACES  ****************************** */
            VIN_TraceBuffer(("\r\n >> Disp"));
            VIN_TracePictureStructure (
                    Device_p->PictureInfos[PreviousIndex].VideoParams.PictureStructure ,
                    Device_p->ExtendedTemporalReference[PreviousIndex]);
            VIN_TraceBuffer((" [%x]\r\n",
                    Device_p->PictureBufferParams[PreviousIndex].Data1_p));
        TraceFrameBufferStatus(("Disp","0x%08x",Device_p->PictureBufferParams[PreviousIndex].Data1_p));
        /* Insert the last buffer in the video display queue.                           */
        ErrorCode = STVID_DisplayPictureBuffer(Device_p->VideoHandle,
                (Device_p->PictureBufferHandle[PreviousIndex]),
                &(Device_p->PictureInfos[PreviousIndex]));

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_DisplayPictureBuffer != ST_NO_ERROR"));
        }
    }
    else
    {
        /* ******************************  DEBUG TRACES  ****************************** */
            VIN_TraceBuffer(("No disp : Error !!!"));
    }
#endif /* MEHOD_CURRENT */

    /* Add ExtendedTemporalReference only in FullFrame picture or TOP Frame         */
    if ( (PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD) ||
         (PictureStructure == STVID_PICTURE_STRUCTURE_FRAME) )
    {
        Device_p->ForTask.ExtendedTemporalReference++;
    }

    return ErrorCode;

} /* End of UpdateFrameBuffer() function */

/*******************************************************************************
Name        : UpdatePictureBuffer
Description :
Parameters  : Device_p,
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, because no error should occur.
*******************************************************************************/
ST_ErrorCode_t UpdatePictureBuffer(stvin_Device_t * const Device_p,
                                   const STVID_PictureStructure_t PictureStructure,
                                   STVID_PictureInfos_t*  const PictureInfos_p,
                                   const STVID_PictureBufferDataParams_t * const PictureBufferParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* ******************************  DEBUG TRACES  ****************************** */
        VIN_TraceBuffer((">> Prepare "));
        VIN_TracePictureStructure (PictureStructure, Device_p->ForTask.ExtendedTemporalReference);
        VIN_TraceBuffer(("[%x] (%d)", PictureBufferParams_p->Data1_p, time_now()));
    TraceFrameBufferStatus(("Prep","0x%08x",PictureBufferParams_p->Data1_p));


    /* Bitmap params */
    PictureInfos_p->BitmapParams.ColorType      = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
#if defined(ST_7710) || defined(ST_7100) || defined(ST_7109) || defined(ST_5528)
    switch (Device_p->InputMode) {
        case STVIN_HD_YCbCr_1280_720_P_CCIR :
        case STVIN_HD_YCbCr_720_480_P_CCIR :
        case STVIN_HD_RGB_1024_768_P_EXT :
        case STVIN_HD_RGB_800_600_P_EXT :
        case STVIN_HD_RGB_640_480_P_EXT :
            PictureInfos_p->BitmapParams.BitmapType     = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
            break;
        case STVIN_SD_NTSC_720_480_I_CCIR :
        case STVIN_SD_NTSC_640_480_I_CCIR :
        case STVIN_SD_PAL_720_576_I_CCIR :
        case STVIN_SD_PAL_768_576_I_CCIR :
        case STVIN_HD_YCbCr_1920_1080_I_CCIR :
        default :
                PictureInfos_p->BitmapParams.BitmapType     = STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM;
            break;
    }
#else
    PictureInfos_p->BitmapParams.BitmapType     = STGXOBJ_BITMAP_TYPE_MB;
#endif /* defined(ST_7710) || defined(ST_7100) || defined(ST_7109) || defined(ST_5528) */
    PictureInfos_p->BitmapParams.PreMultipliedColor     = FALSE;
    PictureInfos_p->BitmapParams.ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
    PictureInfos_p->BitmapParams.AspectRatio    = STGXOBJ_ASPECT_RATIO_4TO3;
    /*
    PictureInfos_p->BitmapParams.Width = Device_p->VideoParams.FrameWidth;
    PictureInfos_p->BitmapParams.Height = Device_p->VideoParams.FrameHeight;
    */
    PictureInfos_p->BitmapParams.Width  = ((HALVIN_Properties_t *)
                        Device_p->HALInputHandle)->InputWindow.OutputWinWidth;
    PictureInfos_p->BitmapParams.Height = ((HALVIN_Properties_t *)
                        Device_p->HALInputHandle)->InputWindow.OutputWinHeight;
    PictureInfos_p->BitmapParams.Pitch  = ((HALVIN_Properties_t *)
                        Device_p->HALInputHandle)->InputWindow.OutputWinWidth;
    PictureInfos_p->BitmapParams.Offset = 0;
    PictureInfos_p->BitmapParams.Data1_p= PictureBufferParams_p->Data1_p;
    PictureInfos_p->BitmapParams.Size1  = PictureBufferParams_p->Size1;
    PictureInfos_p->BitmapParams.Data2_p= PictureBufferParams_p->Data2_p;
    PictureInfos_p->BitmapParams.Size2  = PictureBufferParams_p->Size2;
    PictureInfos_p->BitmapParams.SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;

    /* Video Params */
    PictureInfos_p->VideoParams.FrameRate   = Device_p->InputFrameRate;

    PictureInfos_p->VideoParams.ScanType    = Device_p->VideoParams.ScanType;
    /* Picture has to be a "B" picture. Otherwise, it won't work with 2     */
    /* (set by the memory profile) !!!!                                     */
    PictureInfos_p->VideoParams.MPEGFrame   = STVID_MPEG_FRAME_B;
    PictureInfos_p->VideoParams.PictureStructure    = PictureStructure;
    PictureInfos_p->VideoParams.TopFieldFirst       =
                ((PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD) ? FALSE : TRUE);

    /* TODO: We *must* get and update this parameter according the MemoryProfile! */
    PictureInfos_p->VideoParams.CompressionLevel  = STVID_COMPRESSION_LEVEL_NONE;
    PictureInfos_p->VideoParams.DecimationFactors = Device_p->CurrentVideoMemoryProfile.DecimationFactor;


    if ((PictureInfos_p->BitmapParams.BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM) ||
        (PictureInfos_p->BitmapParams.BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE))
    {
        if(PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
        {
            PictureInfos_p->BitmapParams.Data1_p =
                (void*) ((U32)PictureInfos_p->BitmapParams.Data1_p + (2*PictureInfos_p->BitmapParams.Width));
        }
        HALVIN_SetReconstructedFramePointer(Device_p->HALInputHandle,
                                        PictureStructure,                    /* Top / bottom */
                                        PictureBufferParams_p->Data1_p,
                                        (U32 *) ((U32 *) (PictureBufferParams_p->Data1_p) + (PictureInfos_p->BitmapParams.Width/2)));
    }
    else
    {
        HALVIN_SetReconstructedFramePointer(Device_p->HALInputHandle,
                                        PictureStructure,                    /* Top / bottom */
                                        PictureBufferParams_p->Data1_p,
                                        PictureBufferParams_p->Data2_p);
    }

    return(ErrorCode);
} /* End of UpdatePictureBuffer() function. */

#ifdef TRACE_UART
/*******************************************************************************
Name        : VIN_TracePictureBufferQueue
Description : Print on UART port the picture buffer queue.
Parameters  : Device_p, device handle.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIN_TracePictureBufferQueue (stvin_Device_t * const Device_p)
{
    int LoopCnt;

    VIN_TraceBuffer(("{"));
    for (LoopCnt = 0 ; LoopCnt < STVIN_MAX_NB_FRAME_IN_VIDEO ; LoopCnt ++)
    {
        VIN_TraceBuffer(("%c", ((Device_p->PictureBufferHandle[LoopCnt]== NULL ? 'X' :
        (Device_p->PictureInfos[LoopCnt].VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD ? 'T' :
        Device_p->PictureInfos[LoopCnt].VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD ? 'B' : 'F'))) ));
    }
    VIN_TraceBuffer(("}"));

} /* End of VIN_TracePictureBufferQueue() function */

/*******************************************************************************
Name        : VIN_TracePictureStructure
Description : Print on UART port the picture structure (top, bottom or frame).
Parameters  : PictureStructure
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIN_TracePictureStructure (STVID_PictureStructure_t PictureStructure, U32 ExtendedTemporalReference)
{
    VIN_TraceBuffer(("(%s", (PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD    ? "Top" :
                              PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD ? "Bot" : "Frm") ));
    VIN_TraceBuffer(("%d)", ExtendedTemporalReference));

} /* End of VIN_TracePictureStructure() function */

#endif /* TRACE_UART */

/* End of vin_inte.c */



