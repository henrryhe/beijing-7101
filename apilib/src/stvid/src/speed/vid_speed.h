/*******************************************************************************

File name   : vid_speed.h

Description : Video trick mode common header file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
 12 Jan 2006       Created                                          RH
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_SPEED_COMMON_H
#define __VIDEO_SPEED_COMMON_H


/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"

#include "producer.h"
#include "display.h"

#include "stevt.h"
#include "stavmem.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

enum
{
    VIDSPEED_SPEED_DRIFT_THRESHOLD_EVT_ID = 0,
    VIDSPEED_DISPLAY_PARAMS_CHANGE_EVT_ID,  /* Careful: this event should not be notified inside a VIDSPEED API call from API module, because API module subscribes to this event and this would lead to a block because of semaphore lock ! */
    VIDSPEED_NB_REGISTERED_EVENTS_IDS /* always keep as last one to count event ID's */
};



/* Exported Types ----------------------------------------------------------- */
/******************************************************************************************/
/******* Start of the Stub to compile Speed while DECODE not fully implemented. *******/
/******************************************************************************************/


typedef struct VIDSPEED_Data_s
{
    U32                                 ValidityCheck;
    ST_Partition_t *                    CPUPartition_p;
    U8                                  DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    /* Handles */
    STEVT_Handle_t                      EventsHandle;
	VIDDISP_Handle_t                    DisplayHandle;
	VIDPROD_Handle_t			   		DecodeHandle;
	VIDPROD_Handle_t					ProducerHandle;
    STAVMEM_PartitionHandle_t           AvmemPartitionHandle;
#ifdef ST_ordqueue
    VIDQUEUE_Handle_t                   OrderingQueueHandle;
#endif /* ST_ordqueue */
    /* Real Time Constraints */
    VIDCOM_Task_t                       SpeedTask;
    semaphore_t *                       SpeedParamsSemaphore_p;
    semaphore_t *                       WakeSpeedTaskUp_p;

    /* Display params */
    VIDDISP_DisplayParams_t             DisplayParams;
    /* Registered events and data to reach asked speed if necessary */
    STEVT_EventID_t                     RegisteredEventsID[VIDSPEED_NB_REGISTERED_EVENTS_IDS];
/* if I pictures are very near in the stream, then skipping some I picture is authorized */
    S32                                 NbFieldsIfPosToSkipIfNegToRepeat;
	S32									AverageFieldsNb;
    S32                                 DriftTime;
	U32									ZeroCounter;
    BOOL                                WaitFirstDecode;
    BOOL                                EventDisplayChangeToBeNotifiedInTask;
    /* Trick mode management params */
    struct
    {
        S16                             VirtualClockCounter;
        S16                             Speed;
        S16                             LastSpeed;
        S16                             RequiredSpeed;
        U16                             BitRateValue;   /* Unit used 400bits/s */
        U32                             FrameRateValue; /* Unit used = mHz     */
        STGXOBJ_ScanType_t              VTGScanType;

        BOOL                            SkipAllB;
        BOOL                            SkipAllP;
    } MainParams;

    struct
    {
        BOOL                            RequiredSpeedTakenInAccount;
        BOOL                            MustSkipAllPPictures;
        BOOL                            SpeedControlEnabled;
        BOOL                            SecondFieldHasToBeSkipped;
        BOOL                            WaitingForASecondField;
        BOOL                            SpeedReinitNeeded;
        BOOL                            DisplayManagedBySpeed;
    } Flags;
    /* Backward I only infos */
    struct
    {
        BOOL                            IsBufferParsed;
        U32                             NbDisplayedPictures;
        BOOL                            IsDisplayEnable;
        BOOL                            IsBufferReadyForDisplay;
        U32                             NbIPicturesInDisplayedBuffer;
        U32                             NbIPicturesInParsedBuffer;
        U32                             NbNotIPicturesInParsedBuffer;
        U32                             NbVsyncToStayOnDisplay;
        U32                             TotalNbIFields;
        U32                             TotalNbNotIFields;
        U32                             NbVsyncToDisplayEachField;
        U32                             NewNbVsyncToDisplayEachField;
        U32                             BufferTreatmentTime;
        U32                             BufferDecodeTime;
        U32                             NbVsyncUsedToDisplayTheBuffer;
        /*U32                             NbVsyncMeasuredToDisplayTheBuffer;*/
        U32                             VSyncUsed;
        BOOL                            IsNewBufferStartedDecoded;
        U32                             NbIPicturesForNextDisplayedBuffer;
        U32                             TimeToDisplayEachField;
        U32                             TimeToDecodeIPicture;
        void *                          PreviousBitBufferUsedForDisplay_p;
        S32                             NBFieldsLate;
        S32                             PreviousSkip;
        BOOL                            IsDisplayedBufferChanged;
        BOOL                            DoesUnderflowEventOccur;
        VIDBUFF_PictureBuffer_t         CurrentlyDisplayedPictureBuffer;
    } BackwardIOnly;
#ifdef STVID_DEBUG_GET_STATISTICS
        STVID_Statistics_t              Statistics;
#endif /*STVID_DEBUG_GET_STATISTICS*/

} VIDSPEED_Data_t;


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_SPEED_COMMON_H*/

/* End of vid_speed.h */
