/*******************************************************************************

File name   : vid_sync.h

Description : Video AVSYNC common header file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
 7 Sep 2000        Created                                          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_AVSYNC_COMMON_H
#define __VIDEO_AVSYNC_COMMON_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "avsync.h"

#include "stclkrv.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
enum {
    VIDAVSYNC_BACK_TO_SYNC_EVT_ID = 0,  /* Always set first element as 0 as it will be used for a table */
    VIDAVSYNC_OUT_OF_SYNC_EVT_ID,
    VIDAVSYNC_SYNCHRONISATION_CHECK_EVT_ID,
    VIDAVSYNC_NEXT_PICTURE_DISPLAY_DELAYED_EVT_ID,
    VIDAVSYNC_NB_REGISTERED_EVENTS_IDS  /* always keep as last one to count event ID's */
};

/* Exported Types ----------------------------------------------------------- */
typedef struct VIDAVSYNC_Data_s {
    ST_Partition_t *    CPUPartition_p;
    STEVT_Handle_t      EventsHandle;
    STEVT_EventID_t     RegisteredEventsID[VIDAVSYNC_NB_REGISTERED_EVENTS_IDS];
    struct /* Display events notification configuration */
    {
        /* Contains informations : Event enable/disable notification and */
        /* Number of event notification to skip before notifying the event. */
        STVID_ConfigureEventParams_t ConfigureEventParam;
        /* Number of notification skipped since the last one. */
        U32             NotificationSkipped;
    } EventNotificationConfiguration [VIDAVSYNC_NB_REGISTERED_EVENTS_IDS];
    STVID_SynchronisationInfo_t     SynchronisationInfo;
    STCLKRV_Handle_t    STClkrvHandle;
#ifdef ST_display
    VIDDISP_Handle_t    DisplayHandle;
#endif /* ST_display */
#ifdef ST_producer
    BOOL                IsDecodeActive;
    VIDPROD_Handle_t    DecodeHandle;
    U32                 NbPicturesToWaitBeforeAllowNewSkipDecode;
#endif /* ST_producer */
    STVID_DeviceType_t  DeviceType;
    VIDAVSYNC_SynchroType_t SynchroType;
    U32                 AVSYNCDriftThreshold;       /* threshold for manual synchronisation */
    U32                 AutomaticDriftThreshold;    /* threshold for automatic synchronisation */
    struct  /* Frame Rate Conversion Params */
    {
        BOOL            IsLargerAutomaticSynchronizationErrorUsed;/* Remember the synchronization error factor used.          */
                                                                  /* Set to "1" indicates a larger error window is used before*/
                                                                  /* performing action to be synchronized.                    */
        U32             CurrentInputFrameRate;                    /* Current stream frame rate.                               */
        U32             CurrentOutputFrameRate;                   /* Current display frame rate.                              */
        S32             Margin;                                   /* When a Frame Rate Conversion is operated by the Display, */
                                                                  /* an action of skip or repeat is taken at each X number of */
                                                                  /* pictures. So we have to consider the worst delay         */
                                                                  /* (1 action late) in the STC-PTS comparison.               */
    } FrameRateConversion;

    U32                 CurrentDriftThreshold;      /* current threshold used: either AVSYNCDriftThreshold or AutomaticDriftThreshold */
    BOOL                IsSynchroFieldAccurate;     /* TRUE if AVSync is currently in mode so that it can perform repeat or   */
                                                    /* skip actions field per field.                                          */
    U32                 VTGVsyncDurationMicroSecond; /* Duration of one VSync in us */
    S32                 SynchronizationDelay;       /* Additional delay given by application */
    BOOL                AutomaticSynchronizationEnabled;    /* TRUE if automatic synchronisation enabled by application */
    BOOL                SuspendSynchronisation; /* TRUE to prevent synchronisation to be done, for example when trickmodes are active */
    U32                 ValidityCheck;
    BOOL                IsSynchronisationFirstCheck;

    /* Working parameters */
    semaphore_t *       AVSyncParamsLockingSemaphore_p;   /* Semaphore used to protect parameters */
    BOOL                OutOfSynchronization;   /* TRUE when event out of synchronisation was sent (event back to sync is to be sent) */
    S32                 MissingSkipIfPosRepeatIfNegFieldsCounter;
    BOOL                StreamContainsTruePTS;  /* TRUE if steam contains true PTS's (non-interpolated) : this is a transport stream and synchronisation can take place */
    BOOL                PreviousPictureCheckedWasSynchronizedOk;
    U32                 NbConsecutiveHugeDifference;
    BOOL                BigRestoreInProgress;   /* 'too much' huge diff was counted : try to sync if this flag is true */
    STVID_PTS_t         LastGetSTCValue;        /* Store the last STC */
    STVID_PTS_t         LastPTSValue; /* Store the latest PTS */

#ifdef STVID_DEBUG_GET_STATISTICS
    /* Statistics variables */
    U32 StatisticsAvsyncSkippedFields;                  /* Counts number of fields skipped because of AVSYNC module */
    U32 StatisticsAvsyncRepeatedFields;                 /* Counts number of fields repeated because of AVSYNC module */
    U32 StatisticsAvsyncExtendedSTCAvailable;           /* Counts number of time an extended STC was obtained successfully from STCLKRV */
    U32 StatisticsAvsyncPictureWithNonInterpolatedPTS;  /* Counts number of pictures with PTS's in the stream (non interpolated) */
    U32 StatisticsAvsyncPictureCheckedSynchronizedOk;   /* Counts number of pictures that were checked to be syncronized OK (need PTS+STC+enablesync+speed100) */
    U32 StatisticsAvsyncPTSInconsistency;               /* Counts number of time the PTS of the current picture is not greater than the PTS of the previous picture */
    U32 StatisticsAvsyncMaxRepeatedFields;              /* Counts max number of fields required to be repeated at the display */
    U32 StatisticsAvsyncFailedToSkipFields;             /* Counts number of fields that could not be skipped because display could not */
#endif /* STVID_DEBUG_GET_STATISTICS */
} VIDAVSYNC_Data_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */
BOOL vidavsync_IsEventToBeNotified(const VIDAVSYNC_Handle_t AVSyncHandle, const U32 Event_ID);

/* Exported Functions ------------------------------------------------------- */




/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_AVSYNC_COMMON_H */

/* End of vid_sync.h */
