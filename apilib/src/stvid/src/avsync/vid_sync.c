/*******************************************************************************

File name   : vid_sync.c

Description : Video AVSYNC source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
 7 Sep 2000        Created                                          HG
26 Jan 2001        Added VIDAVSYNC_IsSynchronizationEnabled()       HG
16 May 2001        Added quicker adjustment by large repeats when
                   big mis-synchronisation detected                 HG
16 Dec 2001        When huge difference: don't do synchro actions.
                   This is necessary for injection loops            HG
 7 Jan 2002        Implement skip on decode when skip on display has
                   no effect, necessary for ATSC                    HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Should be defined by default. Undefine below in order to do synchronization
 on interpolated PTS's. There is no reason to do synchronization on interpolated
 PTS, the real PTS are sufficient to have a correct synchronisation. */
#define AVSYNC_DONT_SYNCHRONIZE_ON_INTERPOLATED_PTS

/* Should not be define only in case of debug in order to save in a file the STC and the PTS. */
/* The aim is then to put the extracted data in a chart.                                      */
/*#define STORE_STC_PTS_IN_FILE*/
/* Should only be definied in case of debug, in order to save in a file PTS, STC, ClockDifference */
/* and the value of the flags of STVID_SynchronisationInfo_t   */
/*#define STORE_SYNCHRO_INFO_IN_FILE*/

/* Define to add debug info and traces (use the one OR the other, not both) */
/* #define TRACE_AVSYNC */    /* avsync management */
/* #define TRACE_CLOCK */  /* PTS/PCR curves */
/* #define TRACE_EXTENDEDSTC */ /* Extended STC trace */
/* #define TRACE_VISUAL_UART */

#if defined TRACE_AVSYNC
#define TRACE_UART
#endif



#if defined TRACE_UART || defined TRACE_CLOCK || defined (TRACE_VISUAL_UART)
#ifdef ST_OSLINUX
#define TraceBuffer(x) printk x
#else
#include "trace.h"
#endif
#endif /* TRACE_UART || TRACE_CLOCK*/


/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <limits.h>
#include <stdio.h>
#include <string.h>
#endif

#include "sttbx.h"

#ifdef STORE_STC_PTS_IN_FILE
#include <debug.h>
#endif /* STORE_STC_PTS_IN_FILE */
#ifdef STORE_SYNCHRO_INFO_IN_FILE
#include <debug.h>
#endif /* STORE_SYNCHRO_INFO_IN_FILE */
#include "stddefs.h"

#include "vid_ctcm.h"

#include "vid_sync.h"

#ifdef ST_producer
#include "producer.h"
#endif /* ST_producer */

#ifdef ST_display
#include "display.h"
#endif /* ST_display */

#include "stevt.h"
#include "stclkrv.h"
#include "stvtg.h"
#include "stos.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define VIDAVSYNC_VALID_HANDLE 0x01351350

/* Max 2 VSync duration (50Hz) */
#define DEFAULT_AUTOMATIC_DRIFT_THRESHOLD_WHEN_NO_VTG   (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / 25)
/*#define DEFAULT_AVSYNC_DRIFT_THRESHOLD 2160*/ /* 2160 = 24ms at 90kHz */

/*#define DEFAULT_AVSYNC_DRIFT_THRESHOLD 2160*/ /* 2160 = 24ms at 90kHz */

/* Drift in skip after which we try to skip decodes (in number of fields) */
#define SKIP_DRIFT_REQUIRING_DECODE_SKIP_B 1
#define SKIP_DRIFT_REQUIRING_DECODE_SKIP_UNTIL_I  12
#define SKIP_DRIFT_REQUIRING_UNCONDITIONAL_DECODE_SKIP_UNTIL_I  (3 * SKIP_DRIFT_REQUIRING_DECODE_SKIP_UNTIL_I)
/* Number of pictures to wait for before allowing new skip of decodes */
#define NB_PICS_BEFORE_NEW_SKIP_DECODE_B_SKIPPED 3 /* skip 1 for an eventual reference decode, 1 for the B skip, +1 because of the way the counter is decremented */
#define NB_PICS_BEFORE_NEW_SKIP_DECODE_NEXT_I_SKIPPED 4 /* skip 3(?2) for all the skips, +1 because of the way the counter is decremented */

/* Number of pictures to wait for taking actions on decode in case of huge difference */
#define NB_PICS_BEFORE_SKIP_DECODE_WHEN_HUGE_DIFFERENCE 80 /* as huge difference is here in order to throw away spurious bad values, don't react very quickly. Wait for many confirmation of very bad synchro */
#define NB_PICS_BEFORE_NEW_SKIP_DECODE_NEXT_SEQ_SKIPPED 6 /* skip 5(?2) for all the skips, +1 because of the way the counter is decremented */

/* Define name below in order to enlarge video driver tolerance to out of sync (in automatic mode)
  This can be used as a temporary work-around for platforms having clocks trouble while still debugging */
/*#define STVID_LARGER_SYNCHRONISATION_TOLERANCE*/

/* Possible AutomaticDriftThreshold values (percent value) */
#if defined STVID_LARGER_SYNCHRONISATION_TOLERANCE
#define VIDAVSYNC_AUTOMATIC_SYNCHRONIZATION_ERROR_FACTOR    150 /* Main tolerance on the AVSync control.                      */
#else /* STVID_LARGER_SYNCHRONISATION_TOLERANCE */
#define VIDAVSYNC_AUTOMATIC_SYNCHRONIZATION_ERROR_FACTOR    105 /* Main tolerance on the AVSync control.                      */
/* It means : STC - Step x 1.05 <  PTS  < STC + Step x 1.05   */
/* with Step = NbTickPerSec (3600 in PAL, 3003 in NTSC 29.97) */
/* PTS can be 5% major or minor around the theorical value it should have. */
#endif /* STVID_LARGER_SYNCHRONISATION_TOLERANCE */

#define SYNCHRONIZATION_ERROR_SECURITY_MARGING            10    /* Factor error sucurity offset (used if frame convertion is  */
                                                                /* detected.                                                  */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */
#ifdef STORE_STC_PTS_IN_FILE
#define MESURES_NB  100
long int            FileHandlePTS;         /* File Handle of the file to be copied */
long int            FileHandleSTC;         /* File Handle of the file to be copied */
U16                 IndexFile;
U32                 STCValue[MESURES_NB];
U32                 PTSValue[MESURES_NB];
#endif /* STORE_STC_PTS_IN_FILE */

#ifdef STORE_SYNCHRO_INFO_IN_FILE
#define MESURES_NB  100
long int            FileHandlePTS;         /* File Handle of the file to be copied */
long int            FileHandleSTC;         /* File Handle of the file to be copied */
long int            FileHandleClockDifference;
long int            FileHandleIsSynchro;
long int            FileHandleIsLoosing;
long int            FileHandleIsBack;
U16                 IndexFile;
U32                 STCValue[MESURES_NB];
U32                 PTSValue[MESURES_NB];
S32                 ClockDiffValue[MESURES_NB];
BOOL                 IsSynchroValue[MESURES_NB];
BOOL                 IsLoosingValue[MESURES_NB];
BOOL                 IsBackValue[MESURES_NB];
#endif /* STORE_SYNCHRO_INFO_IN_FILE */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static void InitAVSyncStructure(const VIDAVSYNC_Handle_t AVSyncHandle);
#ifdef ST_display
static ST_ErrorCode_t ActionRepeat(const VIDAVSYNC_Handle_t AVSyncHandle, const U32 NbFieldsToRepeat, const STVID_PictureStructure_t CurrentPictureStructure);
static ST_ErrorCode_t ActionSkip(const VIDAVSYNC_Handle_t AVSyncHandle, const U32 NbFieldsToSkip, const STVID_PictureStructure_t CurrentPictureStructure);
static void ApplyCurrentDriftThreshold (const VIDAVSYNC_Handle_t AVSyncHandle);
static ST_ErrorCode_t CheckSynchronisation(const VIDAVSYNC_Handle_t AVSyncHandle, const VIDDISP_VsyncDisplayNewPictureParams_t * const Params_p);
static void NewPictureReady(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void PerformMissedSynchronisationActions(const VIDAVSYNC_Handle_t AVSyncHandle, const STVID_PictureStructure_t CurrentPictureStructure);
static void NewVTG(VIDAVSYNC_Handle_t AVSyncHandle, U32 VTGFrameRate);
static void ComputeSTCandPTSClockDifference(STVID_PTS_t STC, STVID_PTS_t PTS, S32 * ClockDifference);
#endif /* ST_display */
static void ResetSynchronisation(const VIDAVSYNC_Handle_t AVSyncHandle);
static void TermAVSyncStructure(const VIDAVSYNC_Handle_t AVSyncHandle);

#ifdef ST_display
static ST_ErrorCode_t IsPTSConsistent(const VIDAVSYNC_Handle_t AVSyncHandle, const STVID_PTS_t PTS);
static BOOL IsPTSEqualToLastPTS(const VIDAVSYNC_Handle_t AVSyncHandle, const STVID_PTS_t PTS);
#endif /* ST_display */

/* Public Functions --------------------------------------------------------- */

/*******************************************************************************
Name        : VIDAVSYNC_ConfigureEvent
Description : Configures notification of video synchronisation events.
Parameters  : Video synchronisation handle
              Event to allow or forbid.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if done successfully
              ST_ERROR_BAD_PARAMETER if the event is not supported.
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_ConfigureEvent(const VIDAVSYNC_Handle_t AVSyncHandle, const STEVT_EventID_t Event,
        const STVID_ConfigureEventParams_t * const Params_p)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    switch (Event)
    {
        case STVID_SYNCHRONISATION_CHECK_EVT :
            VIDAVSYNC_Data_p->EventNotificationConfiguration[VIDAVSYNC_SYNCHRONISATION_CHECK_EVT_ID].NotificationSkipped = 0;
            VIDAVSYNC_Data_p->EventNotificationConfiguration[VIDAVSYNC_SYNCHRONISATION_CHECK_EVT_ID].ConfigureEventParam = (*Params_p);
            break;

        default :
            /* Error case. This event can't be configured. */
            ErrCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    /* Return the configuration's result. */
    return(ErrCode);
} /* End of VIDAVSYNC_ConfigureEvent() function */


/*******************************************************************************
Name        : VIDAVSYNC_DisableSynchronization
Description : Disable synchronization
Parameters  : AVSYNC handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_INVALID_HANDLE if invalid handle
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_DisableSynchronization(const VIDAVSYNC_Handle_t AVSyncHandle)
{
    if (((VIDAVSYNC_Data_t *) AVSyncHandle == NULL) || (((VIDAVSYNC_Data_t *) AVSyncHandle)->ValidityCheck != VIDAVSYNC_VALID_HANDLE))
    {
        /* Invalid handle */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Ensure nobody else accesses avsync parameters */
    semaphore_wait(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    ((VIDAVSYNC_Data_t *) AVSyncHandle)->AutomaticSynchronizationEnabled = FALSE;
    /* Select threshold for manual synchronization mode */
    ((VIDAVSYNC_Data_t *) AVSyncHandle)->CurrentDriftThreshold = ((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSYNCDriftThreshold;

    /* Clear memory of missing skip/repeat */
    ((VIDAVSYNC_Data_t *) AVSyncHandle)->MissingSkipIfPosRepeatIfNegFieldsCounter = 0;

    /* Ensure nobody else accesses avsync parameters */
    semaphore_signal(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Synchronization disabled."));

    return(ST_NO_ERROR);
} /* End of VIDAVSYNC_DisableSynchronization() function */


/*******************************************************************************
Name        : VIDAVSYNC_EnableSynchronization
Description : Enable synchronization
Parameters  : AVSYNC handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_INVALID_HANDLE if invalid handle
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_EnableSynchronization(const VIDAVSYNC_Handle_t AVSyncHandle)
{
    if (((VIDAVSYNC_Data_t *) AVSyncHandle == NULL) || (((VIDAVSYNC_Data_t *) AVSyncHandle)->ValidityCheck != VIDAVSYNC_VALID_HANDLE))
    {
        /* Invalid handle */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Ensure nobody else accesses avsync parameters */
    semaphore_wait(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    if (!(((VIDAVSYNC_Data_t *) AVSyncHandle)->AutomaticSynchronizationEnabled))
    {
        /* If synchronization was disabled: reset synchro before enabling it. */
        ResetSynchronisation(AVSyncHandle);
    }

    ((VIDAVSYNC_Data_t *) AVSyncHandle)->AutomaticSynchronizationEnabled = TRUE;
    /* Select threshold for automatic synchronization mode */
    ((VIDAVSYNC_Data_t *) AVSyncHandle)->CurrentDriftThreshold = ((VIDAVSYNC_Data_t *) AVSyncHandle)->AutomaticDriftThreshold;

    /* Ensure nobody else accesses avsync parameters */
    semaphore_signal(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Synchronization enabled."));

    return(ST_NO_ERROR);
} /* End of  VIDAVSYNC_EnableSynchronization() function */


#ifdef STVID_ENABLE_SYNCHRONIZATION_DELAY
/*******************************************************************************
Name        : VIDAVSYNC_GetSynchronizationDelay
Description : Get synchronization delay.
Parameters  : AVSYNC handle, delay (positive or negative)
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_INVALID_HANDLE if invalid handle
              ST_ERROR_BAD_PARAMETER  if SynchronizationDelay_p is NULL
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_GetSynchronizationDelay(const VIDAVSYNC_Handle_t AVSyncHandle, S32 * const SynchronizationDelay_p)
{
    if (((VIDAVSYNC_Data_t *) AVSyncHandle == NULL) || (((VIDAVSYNC_Data_t *) AVSyncHandle)->ValidityCheck != VIDAVSYNC_VALID_HANDLE))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (SynchronizationDelay_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Ensure nobody else accesses avsync parameters */
    semaphore_wait(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    *SynchronizationDelay_p = ((VIDAVSYNC_Data_t *) AVSyncHandle)->SynchronizationDelay ;

    /* Ensure nobody else accesses avsync parameters */
    semaphore_signal(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    return(ST_NO_ERROR);
} /* End of VIDAVSYNC_GetSynchronizationDelay() function */
#endif /* STVID_ENABLE_SYNCHRONIZATION_DELAY */


/*******************************************************************************
Name        : VIDAVSYNC_Init
Description : Initialise AVSYNC
Parameters  : Init params, returns AVSYNC handle
              Synchronization is enabled by default after that
Assumptions : InitParams_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if already initialised
              STVID_ERROR_EVENT_REGISTRATION if problem with event registration
              ST_ERROR_BAD_PARAMETER if bad parameter
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_Init(const VIDAVSYNC_InitParams_t * const InitParams_p, VIDAVSYNC_Handle_t * const AVSyncHandle_p)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p;
    STEVT_OpenParams_t STEVT_OpenParams;
#ifdef ST_display
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
#endif /* ST_display */
    STCLKRV_OpenParams_t STCkrvOpenParams;
    ST_ErrorCode_t ErrorCode;

    /* Allocate a HALDEC structure */
    SAFE_MEMORY_ALLOCATE(VIDAVSYNC_Data_p, VIDAVSYNC_Data_t *, InitParams_p->CPUPartition_p, sizeof(VIDAVSYNC_Data_t));
    if (VIDAVSYNC_Data_p == NULL)
    {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Remember partition */
    VIDAVSYNC_Data_p->CPUPartition_p = InitParams_p->CPUPartition_p;

    /* Allocation succeeded: make handle point on structure */
    *AVSyncHandle_p = (VIDAVSYNC_Handle_t *) VIDAVSYNC_Data_p;
    VIDAVSYNC_Data_p->ValidityCheck = VIDAVSYNC_VALID_HANDLE;

    InitAVSyncStructure(*AVSyncHandle_p);

    /* Store parameters */
    VIDAVSYNC_Data_p->DeviceType    = InitParams_p->DeviceType;
    VIDAVSYNC_Data_p->SynchroType   = InitParams_p->SynchroType;
#ifdef ST_display
    VIDAVSYNC_Data_p->DisplayHandle = InitParams_p->DisplayHandle;
#endif /* ST_display */
#ifdef ST_producer
    VIDAVSYNC_Data_p->IsDecodeActive = InitParams_p->IsDecodeActive;
    VIDAVSYNC_Data_p->DecodeHandle  = InitParams_p->DecodeHandle;
    VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode = 0;
#endif /* ST_producer */
    if (InitParams_p->AVSYNCDriftThreshold != 0)
    {
        /* Take threshold for avsync drift */
        VIDAVSYNC_Data_p->AVSYNCDriftThreshold = InitParams_p->AVSYNCDriftThreshold;
    }
    else
    {
        /* Can't set threshold to 0, so take default value instead */
        VIDAVSYNC_Data_p->AVSYNCDriftThreshold = DEFAULT_AUTOMATIC_DRIFT_THRESHOLD_WHEN_NO_VTG;
    }

    /* Init semaphore for avsync parameters protection */
    VIDAVSYNC_Data_p->AVSyncParamsLockingSemaphore_p = STOS_SemaphoreCreatePriority(VIDAVSYNC_Data_p->CPUPartition_p,1);

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &(VIDAVSYNC_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module AVSync init: could not open EVT !"));
        VIDAVSYNC_Term(*AVSyncHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(VIDAVSYNC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_BACK_TO_SYNC_EVT, &(VIDAVSYNC_Data_p->RegisteredEventsID[VIDAVSYNC_BACK_TO_SYNC_EVT_ID]));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDAVSYNC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_OUT_OF_SYNC_EVT, &(VIDAVSYNC_Data_p->RegisteredEventsID[VIDAVSYNC_OUT_OF_SYNC_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDAVSYNC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_SYNCHRONISATION_CHECK_EVT, &(VIDAVSYNC_Data_p->RegisteredEventsID[VIDAVSYNC_SYNCHRONISATION_CHECK_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDAVSYNC_Data_p->EventsHandle, InitParams_p->VideoName, VIDAVSYNC_NEXT_PICTURE_DISPLAY_DELAYED_EVT, &(VIDAVSYNC_Data_p->RegisteredEventsID[VIDAVSYNC_NEXT_PICTURE_DISPLAY_DELAYED_EVT_ID]));
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module AVSync init: could not register event !"));
        VIDAVSYNC_Term(*AVSyncHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

#ifdef ST_display
    /* Subscribe to events */
    STEVT_SubscribeParams.NotifyCallback = NewPictureReady;
    STEVT_SubscribeParams.SubscriberData_p = (void *) (*AVSyncHandle_p);
/*    ErrorCode = STEVT_SubscribeDeviceEvent(VIDAVSYNC_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT, &STEVT_SubscribeParams);*/
/*    if (ErrorCode != ST_NO_ERROR)*/
/*    {*/
        /* Error: subscribe failed, undo initialisations done */
/*        VIDAVSYNC_Term(*AVSyncHandle_p);*/
/*        return(ST_ERROR_BAD_PARAMETER);*/
/*    }*/
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDAVSYNC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDISP_VSYNC_DISPLAY_NEW_PICTURE_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module AVSync init: could not subscribe to VIDDISP_VSYNC_DISPLAY_NEW_PICTURE_EVT !"));
        VIDAVSYNC_Term(*AVSyncHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDAVSYNC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDISP_NEW_PICTURE_PREPARED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module AVSync init: could not subscribe to VIDDISP_NEW_PICTURE_PREPARED_EVT !"));
        VIDAVSYNC_Term(*AVSyncHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif /* ST_display */

    /* Open clock recovery driver */
    ErrorCode = STCLKRV_Open(InitParams_p->ClockRecoveryName, &STCkrvOpenParams, &(VIDAVSYNC_Data_p->STClkrvHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module AVSync init: could not open CLKRV !"));
        VIDAVSYNC_Term(*AVSyncHandle_p);
        return(STVID_ERROR_SYSTEM_CLOCK);
    }

#ifdef TRACE_UART
    TraceBuffer(("\rAVSync module initialized.\r\n"));
#endif /* TRACE_UART */

#ifdef STORE_STC_PTS_IN_FILE
    FileHandlePTS = debugopen ( "PTS.txt" , "w" );
    FileHandleSTC = debugopen ( "STC.txt" , "w" );
    IndexFile = 0;
#endif /* STORE_STC_PTS_IN_FILE */
#ifdef STORE_SYNCHRO_INFO_IN_FILE
    FileHandlePTS = debugopen ( "PTS.txt" , "w" );
    FileHandleSTC = debugopen ( "STC.txt" , "w" );
    FileHandleClockDifference = debugopen ( "ClockDiff.txt" , "w" );
    FileHandleIsSynchro = debugopen ( "IsSynchro.txt" , "w" );
    FileHandleIsLoosing = debugopen ( "IsLoosing.txt" , "w" );
    FileHandleIsBack = debugopen ( "IsBack.txt" , "w" );
    IndexFile = 0;
#endif /* STORE_SYNCHRO_INFO_IN_FILE */
    /* Reset synchronization */
    ResetSynchronisation(*AVSyncHandle_p);

    return(ErrorCode);
} /* end of VIDAVSYNC_Init() */


/*******************************************************************************
Name        : VIDAVSYNC_IsSynchronizationEnabled
Description : Says whether the synchronisation is enabled
Parameters  : AVSYNC handle
Assumptions :
Limitations :
Returns     : TRUE if AVSYNC initialised and enabled
              FALSE if AVSYNC not initialisde or disabled
*******************************************************************************/
BOOL VIDAVSYNC_IsSynchronizationEnabled(const VIDAVSYNC_Handle_t AVSyncHandle)
{
    if (((VIDAVSYNC_Data_t *) AVSyncHandle == NULL) || (((VIDAVSYNC_Data_t *) AVSyncHandle)->ValidityCheck != VIDAVSYNC_VALID_HANDLE))
    {
        /* Invalid AVSYNC handle */
        return(FALSE);
    }

    if (((VIDAVSYNC_Data_t *) AVSyncHandle)->AutomaticSynchronizationEnabled)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
} /* End of VIDAVSYNC_IsSynchronizationEnabled() function */


#ifdef ST_display
/*******************************************************************************
Name        : NewVTG
Description : To be called when a new VTG is connected
Parameters  : AVSync handle, VTG name
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static void NewVTG(VIDAVSYNC_Handle_t AVSyncHandle, U32 VTGFrameRate)
{
    VIDAVSYNC_Data_t *  VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;
    U32                 AutomaticDriftThreshold;

    /* Ensure nobody else accesses avsync parameters */
    semaphore_wait(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

        if (VTGFrameRate == 0)
        {
            VIDAVSYNC_Data_p->VTGVsyncDurationMicroSecond = STVID_DEFAULT_VSYNC_DURATION_MICROSECOND_WHEN_NO_VTG;
            VIDAVSYNC_Data_p->AutomaticDriftThreshold = DEFAULT_AUTOMATIC_DRIFT_THRESHOLD_WHEN_NO_VTG;
        }
        else
        {
            VIDAVSYNC_Data_p->VTGVsyncDurationMicroSecond = 1000000000 / VTGFrameRate; /* VTG FrameRate given in mHz */
             /* Default value of AutomaticDriftThreshold : normal case (1.1 VSync synchronization window).  */

            /* Calculate local automatic drift threshold (in Micro second unit).    */
            AutomaticDriftThreshold =
                    ((VIDAVSYNC_Data_p->VTGVsyncDurationMicroSecond) * VIDAVSYNC_AUTOMATIC_SYNCHRONIZATION_ERROR_FACTOR) /100;
            /* Convert it threshold in 90KHz unit.                                  */
            AutomaticDriftThreshold = (AutomaticDriftThreshold * (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION/1000)) / 1000;
            /* Store the result.                                                    */
            VIDAVSYNC_Data_p->AutomaticDriftThreshold = AutomaticDriftThreshold ;
        }

    /* Update the current drift threshold value.                    */
    ApplyCurrentDriftThreshold (AVSyncHandle);

    /* Ensure nobody else accesses avsync parameters */
    semaphore_signal(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

} /* End of NewVTG() function */
#endif /* ST_display */


/*******************************************************************************
Name        : VIDAVSYNC_ResetSynchronization
Description : Reset synchronisation actions. To be called at STVID_Start() to cancel all syncs.
Parameters  : AVSync handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_ResetSynchronization(const VIDAVSYNC_Handle_t AVSyncHandle)
{
    if (((VIDAVSYNC_Data_t *) AVSyncHandle == NULL) || (((VIDAVSYNC_Data_t *) AVSyncHandle)->ValidityCheck != VIDAVSYNC_VALID_HANDLE))
    {
        /* Invalid handle */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Ensure nobody else accesses avsync parameters */
    semaphore_wait(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    ResetSynchronisation(AVSyncHandle);

    /* Ensure nobody else accesses avsync parameters */
    semaphore_signal(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    return(ST_NO_ERROR);
} /* End of VIDAVSYNC_ResetSynchronization() function */


/*******************************************************************************
Name        : VIDAVSYNC_ResumeSynchronizationActions
Description : Resume actions of automatic synchronization
Parameters  : AVSync handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_ResumeSynchronizationActions(const VIDAVSYNC_Handle_t AVSyncHandle)
{
    if (((VIDAVSYNC_Data_t *) AVSyncHandle == NULL) || (((VIDAVSYNC_Data_t *) AVSyncHandle)->ValidityCheck != VIDAVSYNC_VALID_HANDLE))
    {
        /* Invalid handle */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Ensure nobody else accesses avsync parameters */
    semaphore_wait(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    if (((VIDAVSYNC_Data_t *) AVSyncHandle)->SuspendSynchronisation)
    {
        /* If synchronization was suspent: reset synchro before resuming it. */
        ResetSynchronisation(AVSyncHandle);
    }

    ((VIDAVSYNC_Data_t *) AVSyncHandle)->SuspendSynchronisation = FALSE;

    /* Ensure nobody else accesses avsync parameters */
    semaphore_signal(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Synchronization actions resumed."));

    return(ST_NO_ERROR);
} /* End of VIDAVSYNC_ResumeSynchronizationActions() function */


/*******************************************************************************
Name        : VIDAVSYNC_SetSynchronizationDelay
Description : Set synchronization delay (delay deduced from PCR at each PCR check)
Parameters  : AVSYNC handle, delay (positive or negative)
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_INVALID_HANDLE if invalid handle
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_SetSynchronizationDelay(const VIDAVSYNC_Handle_t AVSyncHandle, const S32 SynchronizationDelay)
{
    if (((VIDAVSYNC_Data_t *) AVSyncHandle == NULL) || (((VIDAVSYNC_Data_t *) AVSyncHandle)->ValidityCheck != VIDAVSYNC_VALID_HANDLE))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Ensure nobody else accesses avsync parameters */
    semaphore_wait(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    ((VIDAVSYNC_Data_t *) AVSyncHandle)->SynchronizationDelay = SynchronizationDelay;

    /* Ensure nobody else accesses avsync parameters */
    semaphore_signal(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Synchronization delay set to %d.", SynchronizationDelay));

    return(ST_NO_ERROR);
} /* End of VIDAVSYNC_SetSynchronizationDelay() function */


/*******************************************************************************
Name        : VIDAVSYNC_SuspendSynchronizationActions
Description : Suspend actions of automatic synchronization, for example while trickmode control active
Parameters  : AVSync handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_SuspendSynchronizationActions(const VIDAVSYNC_Handle_t AVSyncHandle)
{
    if (((VIDAVSYNC_Data_t *) AVSyncHandle == NULL) || (((VIDAVSYNC_Data_t *) AVSyncHandle)->ValidityCheck != VIDAVSYNC_VALID_HANDLE))
    {
        /* Invalid handle */
        return(ST_ERROR_INVALID_HANDLE);
    }

    ((VIDAVSYNC_Data_t *) AVSyncHandle)->SuspendSynchronisation = TRUE;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Synchronization actions suspent."));

    return(ST_NO_ERROR);
} /* End of VIDAVSYNC_SuspendSynchronizationActions() function */


/*******************************************************************************
Name        : VIDAVSYNC_Term
Description : Terminate AVSYNC, undo all what was done at init
Parameters  : AVSync handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_Term(const VIDAVSYNC_Handle_t AVSyncHandle)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if ((VIDAVSYNC_Data_p == NULL) || (VIDAVSYNC_Data_p->ValidityCheck != VIDAVSYNC_VALID_HANDLE))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Ensure nobody else accesses avsync parameters */
    semaphore_wait(((VIDAVSYNC_Data_t *) AVSyncHandle)->AVSyncParamsLockingSemaphore_p);

    /* Un-subscribe and un-register events: this is done automatically by STEVT_Close() */

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(VIDAVSYNC_Data_p->EventsHandle);
    STCLKRV_Close(VIDAVSYNC_Data_p->STClkrvHandle);

    /* Delete semaphore for avsync parameters protection */
    STOS_SemaphoreDelete(VIDAVSYNC_Data_p->CPUPartition_p,VIDAVSYNC_Data_p->AVSyncParamsLockingSemaphore_p);

    TermAVSyncStructure(AVSyncHandle);

    /* De-validate structure */
    VIDAVSYNC_Data_p->ValidityCheck = 0; /* not VIDAVSYNC_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(VIDAVSYNC_Data_p, VIDAVSYNC_Data_p->CPUPartition_p, sizeof(VIDAVSYNC_Data_t));

#ifdef STORE_STC_PTS_IN_FILE
    debugclose ( FileHandlePTS );
    debugclose ( FileHandleSTC );
#endif /* STORE_STC_PTS_IN_FILE */
#ifdef STORE_SYNCHRO_INFO_IN_FILE
    debugclose ( FileHandlePTS );
    debugclose ( FileHandleSTC );
    debugclose ( FileHandleClockDifference );
    debugclose ( FileHandleIsSynchro );
    debugclose ( FileHandleIsLoosing );
    debugclose ( FileHandleIsBack );
#endif /* STORE_SYNCHRO_INFO_IN_FILE */

    return(ErrorCode);

} /* End of VIDAVSYNC_Term() */



/* Functions Exported in module --------------------------------------------- */




/* Private Functions -------------------------------------------------------- */


#ifdef ST_display
/*******************************************************************************
Name        : ActionRepeat
Description : Actions to repeat
Parameters  : AVSync handle, number of fields to repeat
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t ActionRepeat(const VIDAVSYNC_Handle_t AVSyncHandle, const U32 NbFieldsToRepeat, const STVID_PictureStructure_t CurrentPictureStructure)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;
    U32             NumberToDo;
    U32             NumberDone;
    ST_ErrorCode_t  ErrorCode;
    STVID_PictureInfos_t PictureInfos;

    UNUSED_PARAMETER(CurrentPictureStructure);

#ifdef STVID_DEBUG_GET_STATISTICS
	if (NbFieldsToRepeat > VIDAVSYNC_Data_p->StatisticsAvsyncMaxRepeatedFields)
	{
		VIDAVSYNC_Data_p->StatisticsAvsyncMaxRepeatedFields = NbFieldsToRepeat;
	}
#endif /* STVID_DEBUG_GET_STATISTICS */

    NumberToDo = NbFieldsToRepeat;
	sttbx_Print("sync repeat %d\n", NumberToDo);
#if defined (TRACE_VISUAL_UART)
    TraceState("CHECK SYNC", "Repeat %d fields", NumberToDo);
#endif

    /* Try to repeat fields at display. This call must be always done, although
     * if there is no picture on display, because it will remove the picture
     * prepared for the next VSYNC when synchronising on the picture to be
     * displayed on the next VSYNC */
    ErrorCode = VIDDISP_RepeatFields(VIDAVSYNC_Data_p->DisplayHandle, NumberToDo, &NumberDone);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* If error: no action was done */
        NumberDone = 0;
    }

    if(VIDAVSYNC_Data_p->SynchroType == VIDAVSYNC_CHECK_SYNCHRO_ON_PREPARED_PICTURE_FOR_NEXT_VSYNC)
    {
        /* Check if there is a picture on display. It is important to keep this
         * call after the call to VIDDISP_RepeatFields that is able to remove the
         * picture prepared for next VSYNC */
        ErrorCode = VIDDISP_GetDisplayedPictureInfos(VIDAVSYNC_Data_p->DisplayHandle, &PictureInfos);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* No picture on display : advise the outside world, that the synchro
               will delay the display of the current picture by 2 VSYNCS (because
               the display will take care of the polarity of the current picture,
               and will propose it again for display after 2 VSYNCS */
            NumberToDo = 2;
            STEVT_Notify(VIDAVSYNC_Data_p->EventsHandle,
                         VIDAVSYNC_Data_p->RegisteredEventsID[VIDAVSYNC_NEXT_PICTURE_DISPLAY_DELAYED_EVT_ID],
                         STEVT_EVENT_DATA_TYPE_CAST &NumberToDo);
            return(ST_NO_ERROR);
        }
    }

#ifdef TRACE_UART
    /*    TraceBuffer(("-Repeat(%d/%d)", NumberDone, NumberToDo));*/
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
    VIDAVSYNC_Data_p->StatisticsAvsyncRepeatedFields += NumberDone;
#endif /* STVID_DEBUG_GET_STATISTICS */
    if (NumberDone < NumberToDo)
    {
        /* Should not happen because repeat always possible ? */
        /* Action could not be completed: remember missing fields to re-do action later if possible... */
#ifdef TRACE_UART
        /* TraceBuffer(("-MisSkipCnt%d>", VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter));*/
#endif /* TRACE_UART */
        /* Security to store a difference of U32 into a S32 */
        if ((NumberDone - NumberToDo) > (U32)INT_MIN)
        {
            VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter = NumberDone - NumberToDo;
        }
        else
        {
            VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter = INT_MIN;
        }
#ifdef TRACE_UART
        /*  TraceBuffer(("%d", VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter));*/
#endif /* TRACE_UART */
    } /* end missing repeat */
    else
    {
        /* Reset counter when back to synchronisation */
        VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter = 0;
    }

    return(ST_NO_ERROR);

} /* end of ActionRepeat() function */


/*******************************************************************************
Name        : ActionSkip
Description : Actions to skip
Parameters  : AVSync handle, number of fields to skip
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t ActionSkip(const VIDAVSYNC_Handle_t AVSyncHandle, const U32 NbFieldsToSkip, const STVID_PictureStructure_t CurrentPictureStructure)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;
    U32             NumberToDo;
    U32             NumberDone = 0;
    U32             SkipDriftBeforeDecodeSkipB;
    ST_ErrorCode_t  ErrorCode;
#if defined (TRACE_VISUAL_UART)
    TraceState("CHECK SYNC", "Skip %d fields", NbFieldsToSkip);
#endif /* TRACE_VISUAL_UART */
    /* Determine the skip action accuracy.                                  */
    if (VIDAVSYNC_Data_p->IsSynchroFieldAccurate)
    {
        /* AVSync is field accurate. That means it can perform action skip  */
        /* field per field. The drift threshold is then reduced to 1 field  */
        /* before performing the skip action.                               */
        SkipDriftBeforeDecodeSkipB = SKIP_DRIFT_REQUIRING_DECODE_SKIP_B;
    }
    else
    {
        /* AVSync is 2 * fields accurate.                                   */
        SkipDriftBeforeDecodeSkipB = 2 * SKIP_DRIFT_REQUIRING_DECODE_SKIP_B;
    }

    NumberToDo = NbFieldsToSkip;
sttbx_Print("sync skip %d\n", NumberToDo);
    switch(VIDAVSYNC_Data_p->SynchroType)
    {
        case VIDAVSYNC_CHECK_SYNCHRO_ON_PICTURE_CURRENTLY_DISPLAYED:
#ifdef ST_producer
            if (VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode != 0)
            {
                if (NumberToDo <=2)
                {
#ifdef TRACE_UART
                    /*     TraceBuffer(("-TooEarlyToSkip(2)"));*/
#endif /* TRACE_UART */
                    /* Do nothing */
                    return(ST_NO_ERROR);
                }
                NumberToDo -=2;
            }
#endif /* ST_producer */
            ErrorCode = VIDDISP_SkipFields(VIDAVSYNC_Data_p->DisplayHandle, NumberToDo, &NumberDone, TRUE, VIDDISP_SKIP_STARTING_FROM_CURRENTLY_DISPLAYED_PICTURE);
            break;
        case VIDAVSYNC_CHECK_SYNCHRO_ON_PREPARED_PICTURE_FOR_NEXT_VSYNC:
            ErrorCode = VIDDISP_SkipFields(VIDAVSYNC_Data_p->DisplayHandle, NumberToDo, &NumberDone, TRUE, VIDDISP_SKIP_STARTING_FROM_PREPARED_PICTURE_FOR_NEXT_VSYNC);
            break;
        default:
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
            break;
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* If error: no action was done */
        NumberDone = 0;
    }

#ifdef TRACE_UART
    /*TraceBuffer(("-Skip(%d/%d)", NumberDone, NumberToDo));*/
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
    VIDAVSYNC_Data_p->StatisticsAvsyncSkippedFields += NumberDone;
#endif /* STVID_DEBUG_GET_STATISTICS */
    if (NumberDone < NumberToDo)
    {
        /* Action could not be completed: remember missing fields to re-do action later if possible... */
#ifdef TRACE_UART
       /* TraceBuffer(("-MisSkipCnt%d>", VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter));*/
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
        VIDAVSYNC_Data_p->StatisticsAvsyncFailedToSkipFields += (NumberToDo - NumberDone);
#endif /* STVID_DEBUG_GET_STATISTICS */
        /* Security to store a difference of U32 into a S32 */
        if ((NumberToDo - NumberDone) < ((S32)LONG_MAX))
        {
            VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter = NumberToDo - NumberDone;
        }
        else
        {
            VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter = (S32)LONG_MAX;
        }
#ifdef ST_producer
        if ((VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode != 0)
            && (VIDAVSYNC_Data_p->SynchroType == VIDAVSYNC_CHECK_SYNCHRO_ON_PREPARED_PICTURE_FOR_NEXT_VSYNC))
        {
            return(ST_NO_ERROR);
        }
#endif /* ST_producer */
#ifdef TRACE_UART
       /* TraceBuffer(("%d", VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter));*/
#endif /* TRACE_UART */
#ifdef ST_producer
        /* For streams with very long decode time close to one VSync, skip on display has no effect because decode time keeps the late.
        So we skip on decode under some conditions, so that synchro can work (ex: ATSC decodes). */
        if ((VIDAVSYNC_Data_p->IsDecodeActive) &&
            (!(VIDAVSYNC_Data_p->PreviousPictureCheckedWasSynchronizedOk)) /* Wait for confirmation (same case for next picture) before taking action on decode */
           )
        {
            /* Need to skip a number of fields but display cannot do it: try to skip decode */
            if ((NumberToDo >= SKIP_DRIFT_REQUIRING_UNCONDITIONAL_DECODE_SKIP_UNTIL_I) && ((NumberToDo - NumberDone) >= SKIP_DRIFT_REQUIRING_UNCONDITIONAL_DECODE_SKIP_UNTIL_I))
            {
                /* Unconditional skip because we are really very late, so a lot of skip is required ! */
                /*ErrorCode =*/ VIDPROD_SkipData(VIDAVSYNC_Data_p->DecodeHandle, VIDPROD_SKIP_NEXT_B_PICTURE); /* to ensure at least one picture will be skipped for the case the next sequence group or I is just next start code... */
                /*ErrorCode =*/ VIDPROD_SkipData(VIDAVSYNC_Data_p->DecodeHandle, VIDPROD_SKIP_UNTIL_NEXT_I);
                /* Because this will cause a big skip of decode, wait for some pictures before trying again to skip decode until next I */
                VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode = NB_PICS_BEFORE_NEW_SKIP_DECODE_NEXT_I_SKIPPED;
#ifdef TRACE_UART
       /*         TraceBuffer(("-UNCONDDECSKIP(UNTIL_NEXT_I)"));*/
#endif /* TRACE_UART */
            }
            else if (VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode == 0)
            {
                /* As we are quite close to synchro, wait for effect before re-skipping */

                if ((NumberToDo >= SKIP_DRIFT_REQUIRING_DECODE_SKIP_UNTIL_I) && ((NumberToDo - NumberDone) >= SKIP_DRIFT_REQUIRING_DECODE_SKIP_UNTIL_I))
                {
                    /* Skip large (until next I), but as we are quite close to synchro, wait for effect before re-skipping */
                    /*ErrorCode =*/ VIDPROD_SkipData(VIDAVSYNC_Data_p->DecodeHandle, VIDPROD_SKIP_NEXT_B_PICTURE); /* to ensure at least one picture will be skipped for the case the next sequence group or I is just next start code... */
                    /*ErrorCode =*/ VIDPROD_SkipData(VIDAVSYNC_Data_p->DecodeHandle, VIDPROD_SKIP_UNTIL_NEXT_I);
                    /* Because this will cause a big skip of decode, wait for some pictures before trying again to skip decode until next I */
                    VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode = NB_PICS_BEFORE_NEW_SKIP_DECODE_NEXT_I_SKIPPED;
#ifdef TRACE_UART
       /*             TraceBuffer(("-DECSKIP(UNTIL_NEXT_I)"));*/
#endif /* TRACE_UART */
                }
                else if ((NumberToDo - NumberDone) >= SkipDriftBeforeDecodeSkipB)
                {
                    /* Skip just one picture, and as we are quite close to synchro, wait for effect before re-skipping */
                    /*ErrorCode =*/ VIDPROD_SkipData(VIDAVSYNC_Data_p->DecodeHandle, VIDPROD_SKIP_NEXT_B_PICTURE);
                    /* If skipping a B but not so close to synchonisation, don't need to wait for some pictures before eventually re-skip decode of a B */
                    if ((NumberToDo - NumberDone) <= (2 * SkipDriftBeforeDecodeSkipB))
                    {
                        /* Because this will cause a skip of decode and we are close to synchronisation, wait for some pictures before trying again to skip decode of a B */
                        VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode = NB_PICS_BEFORE_NEW_SKIP_DECODE_B_SKIPPED;
                    }
#ifdef TRACE_UART
       /*             TraceBuffer(("-DECSKIP(NEXT_B)"));*/
#endif /* TRACE_UART */
                }
            }
            if (CurrentPictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
            {
                /* Field pictures: wait twice more pictures before retrying to skip decode ! */
                VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode *= 2;
            }
        }
#ifdef TRACE_UART
        else
        {
       /*     TraceBuffer(("-avoidDECSKIP:n=%d,prevOK=%d", VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode,
                VIDAVSYNC_Data_p->PreviousPictureCheckedWasSynchronizedOk));*/
        }
#endif /* TRACE_UART */
#endif /* ST_producer */
    } /* end missing skip */
    else
    {
        /* Reset counter when back to synchronisation */
        VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter = 0;
    }

    return(ST_NO_ERROR);

} /* end of ActionSkip() function */

/*******************************************************************************
Name        : CheckSynchronisation
Description : Check synchronisation
Parameters  : AVSync handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success in synchronisation
              ST_ERROR_BAD_PARAMETER if error of params
              STVID_ERROR_SYSTEM_CLOCK if synchronisation is not done because of system clock
              STVID_ERROR_NOT_AVAILABLE if synchronisation is not done because not possible
*******************************************************************************/
static ST_ErrorCode_t CheckSynchronisation(const VIDAVSYNC_Handle_t AVSyncHandle,
        const VIDDISP_VsyncDisplayNewPictureParams_t * const Params_p)
{
    STCLKRV_ExtendedSTC_t ExtendedSTC;
    STVID_PTS_t         CurrentSTC;
    STVID_PTS_t         Threshold;
    U32             NumberToDo, PTSDifference;
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;
    ST_ErrorCode_t ErrorCode;
    U32             DriftThresholdStep;
    U32             ThisAutomaticDriftThreshold;
    U32             NbFieldAccuracy;            /* Number of field that AVSync will skip or repeat when out of Synchro, and */
                                                /* synchro enable. It depends on display mode (care or don't care polarity).*/
    BOOL            PreventSynchroActionsOnBadPTS;      /* Don't allow normal synchronization actions due to inconsistent PTS with filed picture */

#ifdef STORE_STC_PTS_IN_FILE
    char Buffer_p[20];
    char FinalBuffer_p[20*MESURES_NB];
#endif /* STORE_STC_PTS_IN_FILE */
#ifdef STORE_SYNCHRO_INFO_IN_FILE
    char Buffer_p[20];
    char FinalBuffer_p[20*MESURES_NB];
#endif /* STORE_SYNCHRO_INFO_IN_FILE */

    /* static U8 err=0; */

    if ((VIDAVSYNC_Data_p == NULL) || (Params_p->PictureInfos_p == NULL))
    {
        /* Can't use data of picture, so can't compare PTS, so can't do anything ! */
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* ---------------------------------------------------------------------- */
    /* Update VTG parameters if they have changed     ------------------------*/
    /* ---------------------------------------------------------------------- */
    if(Params_p->OutputFrameRate != VIDAVSYNC_Data_p->FrameRateConversion.CurrentOutputFrameRate)
    {
        NewVTG(AVSyncHandle,Params_p->OutputFrameRate);
        VIDAVSYNC_Data_p->FrameRateConversion.CurrentOutputFrameRate = Params_p->OutputFrameRate;
    }
    /* ---------------------------------------------------------------------- */
    /* Good parameters, now set AVSync accuracy value ------------------------*/
    /* ---------------------------------------------------------------------- */
    if (!Params_p->IsDisplayedWithCareOfPolarity)
    {
        /* The display is not caring about polarity, and progressive frames   */
        /* are displayed. The synchro. should be performed fields by fields.  */
        NbFieldAccuracy = 1;
        VIDAVSYNC_Data_p->IsSynchroFieldAccurate = TRUE;
    }
    else
    {
        /* The display is caring about polarity or interlaced frames are dis- */
        /* -played. The skip or repeat action  should be performed 2 fields   */
        /* by 2 fields.                                                       */
        NbFieldAccuracy = 2;
        VIDAVSYNC_Data_p->IsSynchroFieldAccurate = FALSE;
    }

    /* ---------------------------------------------------------------------- */
    /* Good AutomaticDriftThreshold, now check clock in the stream -----------*/
    /* ---------------------------------------------------------------------- */

#ifdef ST_producer
    /* Decrement counter of pictures to wait for before attempting to skip decode again */
    if (VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode > 0)
    {
        VIDAVSYNC_Data_p->NbPicturesToWaitBeforeAllowNewSkipDecode --;
    }
#endif /* ST_producer */

    PreventSynchroActionsOnBadPTS = FALSE;

    if (!(Params_p->PictureInfos_p->VideoParams.PTS.Interpolated))
    {
#ifdef STVID_DEBUG_GET_STATISTICS
        VIDAVSYNC_Data_p->StatisticsAvsyncPictureWithNonInterpolatedPTS ++;

        /* ------------------------------ */
        /* -- Check if PTS consistency -- */
        /* ------------------------------ */
        if (IsPTSConsistent(AVSyncHandle, Params_p->PictureInfos_p->VideoParams.PTS) != ST_NO_ERROR)
        {
            VIDAVSYNC_Data_p->StatisticsAvsyncPTSInconsistency ++;
 #ifdef TRACE_UART
          /*  TraceBuffer(("-PTS Inconsistency %d\r\n",Params_p->PictureInfos_p->VideoParams.PTS.PTS ));*/
 #endif /* TRACE_UART */
        }
        else
        {
 #ifdef TRACE_UART
/*            TraceBuffer(("-PTS Ok %d\r\n",Params_p->PictureInfos_p->VideoParams.PTS.PTS ));*/
 #endif /* TRACE_UART */
        }
#endif /* STVID_DEBUG_GET_STATISTICS */


        /* ---------------------------------------------------------- */
        /* -- Check if PTS is increasing and not equal to last one -- */
        /* ---------------------------------------------------------- */
        if ((Params_p->PictureInfos_p->VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME) &&
            ((IsPTSConsistent(AVSyncHandle, Params_p->PictureInfos_p->VideoParams.PTS) != ST_NO_ERROR) ||
             (IsPTSEqualToLastPTS(AVSyncHandle, Params_p->PictureInfos_p->VideoParams.PTS) == TRUE)))
        {
            PreventSynchroActionsOnBadPTS = TRUE;
        }
        /* Update the Last PTS for the next time. */
        VIDAVSYNC_Data_p->LastPTSValue = Params_p->PictureInfos_p->VideoParams.PTS;
    }

    /* Check if steam contains clocks to synchronize on. */
    if (!(VIDAVSYNC_Data_p->StreamContainsTruePTS))
    {
        if (!(Params_p->PictureInfos_p->VideoParams.PTS.Interpolated))
        {
            /* Stream is PES or MPEG-1 transport: it contains true PTS's */
            VIDAVSYNC_Data_p->StreamContainsTruePTS = TRUE;
        }
        else
        {
#if defined (TRACE_VISUAL_UART)
            TraceState("CHECK SYNC", "NoPTS");
#endif
            /* Can't synchronize if stream doesn't contain PTS's ! */
#ifdef TRACE_UART
            TraceBuffer(("-noPTS"));
#endif /* TRACE_UART */
            return(STVID_ERROR_NOT_AVAILABLE);
        }
    } /* end no PTS is the stream */

    /* ------------------------------------------------------ */
    /* -- Good clock in the stream, now check system clock -- */
    /* ------------------------------------------------------ */

    /* Read STC/PCR to compare PTS to it */
    ErrorCode = STCLKRV_GetExtendedSTC(VIDAVSYNC_Data_p->STClkrvHandle, &ExtendedSTC);

	/* Convert STCLKRV STC to our PTS format */
    CurrentSTC.PTS      = ExtendedSTC.BaseValue;
    CurrentSTC.PTS33    = (ExtendedSTC.BaseBit32 != 0);
    CurrentSTC.Interpolated = FALSE; /* just for sanity */
    CurrentSTC.IsValid  = TRUE;

#ifdef TRACE_EXTENDEDSTC
    TraceBuffer(("----> Extended STC:%lu\n", ExtendedSTC.BaseValue));
#endif

    /* Check if twice the same STC value. It may be the case before STCLKRV invalidates
    the clock. In that case, treat it as if there were no STC. */
    /* This has been seen: picture's PTS going up and GetSTC always returning the same value !
    In that case, better don't do anything than repeat(bigger&bigger) leading to very bad effect */
	/* Check only U32 of U33 value, but this is enough to compare identical values: few chance to have same U32 with bit different for two consecutive values ! */
    if ((ErrorCode == ST_NO_ERROR) && (CurrentSTC.PTS == VIDAVSYNC_Data_p->LastGetSTCValue.PTS))
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER; /* stupid value, just to go out of the function */
    }
    VIDAVSYNC_Data_p->LastGetSTCValue = CurrentSTC;
    /* If any error, quit function. A bad STC make synchronization impossible */
    if (ErrorCode != ST_NO_ERROR)
    {
        if (ErrorCode == STCLKRV_ERROR_PCR_UNAVAILABLE)
        {
            /* PCR discontinuity: cancel previous memory of skip/repeat */
            VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter = 0;
        }
        /* No STC available or error: can't tell about synchronization ! */
#if defined (TRACE_VISUAL_UART)
        TraceState("CHECK SYNC", "NoPCR");
#endif
#ifdef TRACE_UART
        TraceBuffer(("-noPCR"));
#endif /* TRACE_UART */
        return(STVID_ERROR_SYSTEM_CLOCK);
    }

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDAVSYNC_Data_p->StatisticsAvsyncExtendedSTCAvailable ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

    /* ------------------------------------------------------------------------- */
    /* -- Good clocks (stream and system clock), now check if actions enabled -- */
    /* ------------------------------------------------------------------------- */

    /* Check if synchronization actions are active or suspent
    Note: all checks above are done anyway to correctly set state of the AVSync module regarding valid clocks */
    if (/*(VIDAVSYNC_Data_p->AutomaticSynchronizationEnabled) &&*/ (VIDAVSYNC_Data_p->SuspendSynchronisation))
    {
        /* Synchronization actions are suspent while synchronization is enabled: should do nothing ! */
#ifdef TRACE_UART
        TraceBuffer(("-NOSync"));
#endif /* TRACE_UART */
        return(ST_NO_ERROR);
    }

    /* -------------------------------------------------------------------------------------- */
    /* -- Good clocks and actions enabled, now correct clock values to be able to use them -- */
    /* -------------------------------------------------------------------------------------- */

    /* Correct STC: we are now a little bit later than VSync, so VIDDISP tells
    how far we are and we correct the STC, to have exactly STC at previous Vsync */
    vidcom_SubstractU32ToPTS(CurrentSTC, Params_p->TimeElapsedSinceVSyncAtNotification);

    /* Adjust STC with synchronization delay (offset given by application) */
    if (VIDAVSYNC_Data_p->SynchronizationDelay > 0)
    {
        vidcom_SubstractU32ToPTS(CurrentSTC, (U32) VIDAVSYNC_Data_p->SynchronizationDelay);
    }
    else if (VIDAVSYNC_Data_p->SynchronizationDelay < 0)
    {
        vidcom_AddU32ToPTS(CurrentSTC, (U32) (-(VIDAVSYNC_Data_p->SynchronizationDelay)));
    }

    /* Add one VSync duration if we are checking synchro on the picture prepared
       for the next VSync, not the currently displayed */
    if(VIDAVSYNC_Data_p->SynchroType == VIDAVSYNC_CHECK_SYNCHRO_ON_PREPARED_PICTURE_FOR_NEXT_VSYNC)
    {
        vidcom_AddU32ToPTS(CurrentSTC, (VIDAVSYNC_Data_p->VTGVsyncDurationMicroSecond * (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION/1000)) / 1000);
    }

#ifdef TRACE_CLOCK
    {
        /* debug : trace : time-STCcorrected-PTS-BBL */
        /*
        U8 V10,V11;
        U32 Level;
        V10 = *(U8 *) (0x17);
        V11 = (*(U8 *) (0x16)) & 0x7F;
        Level = (((U32) V11) << 16) | (((U32) V10) << 8);-------55xx only */
        TraceBuffer(("\r\n%c", ((Params_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)?'B':((Params_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)?'P':'I'))));
        TraceBuffer(("%d-",
            (Params_p->PictureInfos_p->VideoParams.TimeCode.Hours << 24) |
            (Params_p->PictureInfos_p->VideoParams.TimeCode.Minutes << 16) |
            (Params_p->PictureInfos_p->VideoParams.TimeCode.Seconds << 8) |
            (Params_p->PictureInfos_p->VideoParams.TimeCode.Frames)
        ));
        TraceBuffer(("%d-%d-%d-%d",time_now(),CurrentSTC.PTS,
/*        TraceBuffer(("%d-%d-%d-%d\r\n",time_now(),CurrentSTC.PTS,*/
                Params_p->PictureInfos_p->VideoParams.PTS.PTS,/*Level*/0));
/*
        TraceBuffer(("\n time:%lu ---> STC:%lu - PTS:%lu\n",(unsigned long) time_now(), CurrentSTC.PTS, Params_p->PictureInfos_p->VideoParams.PTS.PTS));
*/
    }
#endif /* TRACE_CLOCK */
#if defined (TRACE_VISUAL_UART)
    if(CurrentSTC.PTS > Params_p->PictureInfos_p->VideoParams.PTS.PTS)
    {
        TraceMessage("STC-PTS","%lu", CurrentSTC.PTS - Params_p->PictureInfos_p->VideoParams.PTS.PTS);
    }
    else
    {
        TraceMessage("PTC-CTS","%lu", Params_p->PictureInfos_p->VideoParams.PTS.PTS - CurrentSTC.PTS);
    }
#endif

    /* ---------------------------------------------------------------------- */
    /* Clocks converted, now check if values are not outside ranges, causing very bad actions like repeat(1000fields) ! */
    /* ---------------------------------------------------------------------- */

    /* Is delay between STC and PTS more than too many seconds */
    if (IsThereHugeDifferenceBetweenPTSandSTC(Params_p->PictureInfos_p->VideoParams.PTS, CurrentSTC))
    {
        /* Too large difference between STC and PTS: there must be a bad value somewhere, don't synchronize.
        This prevents from the following error cases:
         * PTS huge change due to application channel change without proper call to STVID_Start()
         * STC huge change due to application channel change without invalidating
           STCLKRV's STC (STCLKRV has histeresys, i.e. it only invalidates STC by
           itself after a few bad values)
         * isolated completely bad STC or PTS value retrieved in hardware, not detectable
        Note: cases of channel change can occur when packet injector stream is looping !
        */
        VIDAVSYNC_Data_p->NbConsecutiveHugeDifference ++;
#ifdef TRACE_UART
       /* if (vidcom_IsPTSGreaterThanPTS(Params_p->PictureInfos_p->VideoParams.PTS, CurrentSTC))
        {
            TraceBuffer(("\r\n-Big diff:PTS>STC=%ds!", (Params_p->PictureInfos_p->VideoParams.PTS.PTS - CurrentSTC.PTS + STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / STVID_MPEG_CLOCKS_ONE_SECOND_DURATION));
        }
        else
        {
            TraceBuffer(("\r\n-Big diff:PTS<STC=%ds!", (CurrentSTC.PTS - Params_p->PictureInfos_p->VideoParams.PTS.PTS + STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / STVID_MPEG_CLOCKS_ONE_SECOND_DURATION));
        }*/
#endif /* TRACE_UART */
        /* Actions on decode for very large late cases : after many huge differences, return an error */
        if((VIDAVSYNC_Data_p->NbConsecutiveHugeDifference < NB_PICS_BEFORE_SKIP_DECODE_WHEN_HUGE_DIFFERENCE)
            &&(!VIDAVSYNC_Data_p->BigRestoreInProgress))
        {
#ifdef TRACE_UART
            /* TraceBuffer(("\r\nClock Error, may be a re-loop (return)."));*/
#endif /* TRACE_UART */
#if defined (TRACE_VISUAL_UART)
            TraceState("CHECK SYNC", "ErrClock");
#endif
            return(STVID_ERROR_SYSTEM_CLOCK);
        }
        else
        {
            VIDAVSYNC_Data_p->BigRestoreInProgress = TRUE;
#ifdef TRACE_UART
            /* TraceBuffer(("\r\nToo many big diff : try to restore anyway(continue)"));*/
#endif /* TRACE_UART */
#if defined (TRACE_VISUAL_UART)
            TraceState("CHECK SYNC", "Too many big diff try to restore anyway(continue)");
#endif
        }
    } /* end huge difference in clocks detected */
    VIDAVSYNC_Data_p->NbConsecutiveHugeDifference = 0;

    if (PreventSynchroActionsOnBadPTS)
    {
        /* Synchronization actions are suspent while synchronization is enabled: should do nothing ! */
#if defined (TRACE_VISUAL_UART)
        TraceState("CHECK SYNC", "BadPTS");
#endif
#ifdef TRACE_UART
        TraceBuffer(("-NOSync"));
#endif /* TRACE_UART */
        return(ST_NO_ERROR);
    }

#ifdef AVSYNC_DONT_SYNCHRONIZE_ON_INTERPOLATED_PTS
    /* ---------------------------------------------------------------------- */
    /* Good clocks, good stream, actions enabled, check if synchronisation takes place on interpolated PTS's */
    /* ---------------------------------------------------------------------- */
    /* Restriction of synchronisation on true PTS's (not interpolated one's) */
    if (Params_p->PictureInfos_p->VideoParams.PTS.Interpolated)
    {
#if defined (TRACE_VISUAL_UART)
TraceState("CHECK SYNC", "iPTS");
#endif
        /* Don't do synchronisation on interpolated PTS, just return. */
#ifdef TRACE_UART
        TraceBuffer(("-iPTS"));
#endif /* TRACE_UART */
        /* Although we don't check synchronisation on interpolated PTS's, try to
          skip or repeat anyway if late actions could not be done... */
        PerformMissedSynchronisationActions(AVSyncHandle, Params_p->PictureInfos_p->VideoParams.PictureStructure);

        return(STVID_ERROR_NOT_AVAILABLE);
    }
#endif /* AVSYNC_DONT_SYNCHRONIZE_ON_INTERPOLATED_PTS */

    /* ---------------------------------------------------------------------- */
    /* Good clocks, good stream, actions enabled, now check synchronisation --*/
    /* ---------------------------------------------------------------------- */

    if (Params_p->IsAnyFrameRateConversionActivated)
    {
#ifdef TRACE_UART
        /* Display type of image */
/*        TraceBuffer(("\r\nSynchroWindowChange->%d", Params_p->IsAnyFrameRateConversionActivated));*/
#endif /* TRACE_UART */

        /* Calculate local automatic drift threshold (in Micro second unit). Calculate the step between 2 FRAMES (2 VSyncs) */
        /* or 2 FIELDS in 90KHz unit.                                                                                       */
        DriftThresholdStep = (VIDAVSYNC_Data_p->VTGVsyncDurationMicroSecond * (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION/1000)) / 1000;
        /* Enlarge window because of frame rate conversion */
        ThisAutomaticDriftThreshold = VIDAVSYNC_Data_p->CurrentDriftThreshold + DriftThresholdStep;
    }
    else
    {
        ThisAutomaticDriftThreshold = VIDAVSYNC_Data_p->CurrentDriftThreshold;
    }

    Threshold = CurrentSTC;
    vidcom_AddU32ToPTS(Threshold, ThisAutomaticDriftThreshold);

#ifdef STORE_STC_PTS_IN_FILE
    if (IndexFile < MESURES_NB)
    {
            /*STCValue[IndexFile] = Threshold.PTS;
            PTSValue[IndexFile] = (Params_p->PictureInfos_p->VideoParams.PTS.PTS);*/
            STCValue[IndexFile] = STCAtNextVSync.PTS;
            PTSValue[IndexFile] = Params_p->PictureInfos_p->VideoParams.PTS.PTS;
        IndexFile++;
    }
    else
    {
        strcpy (Buffer_p,"");
        strcpy (FinalBuffer_p,"");
        for (IndexFile = 0; IndexFile < MESURES_NB; IndexFile++)
        {
            sprintf(Buffer_p,"%u \n", STCValue[IndexFile]);
            strcat (FinalBuffer_p, Buffer_p);
        }
        debugwrite(FileHandleSTC, FinalBuffer_p, strlen(FinalBuffer_p) );

        strcpy (Buffer_p,"");
        strcpy (FinalBuffer_p,"");
        for (IndexFile = 0; IndexFile < MESURES_NB; IndexFile++)
        {
            sprintf(Buffer_p,"%u \n", PTSValue[IndexFile]);
            strcat (FinalBuffer_p, Buffer_p);
        }
        debugwrite(FileHandlePTS, FinalBuffer_p, strlen(FinalBuffer_p) );

        IndexFile = 0;
    }
#endif /* STORE_STC_PTS_IN_FILE */

    /* Clocks corrected: check synchronisation */
    if (vidcom_IsPTSGreaterThanPTS(Params_p->PictureInfos_p->VideoParams.PTS, Threshold))
    {
        /* PTS is in advance: request slow down */
        if (VIDAVSYNC_Data_p->AutomaticSynchronizationEnabled)
        {
            /* Synchronization enabled: repeat "NbFieldAccuracy" field(s) (1 or 2 fields regarding the care of */
            /* polarity of display).                                                                            */
            PTSDifference
                    = (((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / 1000) * NbFieldAccuracy * VIDAVSYNC_Data_p->VTGVsyncDurationMicroSecond) / 1000);
            NumberToDo = 0;
            do
            {
                /* Seek number n of fields to repeat */
                NumberToDo += NbFieldAccuracy;
                vidcom_AddU32ToPTS(Threshold, PTSDifference);
            } while(vidcom_IsPTSGreaterThanPTS(Params_p->PictureInfos_p->VideoParams.PTS, Threshold));

#ifdef TRACE_UART
            /* Display type of image */
/*            TraceBuffer(("\r\nREAP:+%c", ((Params_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)?'B':((Params_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)?'P':'I'))));*/
/*            TraceBuffer(("\r\n-STC=%d PTS=%d Th=%d NumberToDo=%d \r\n", ExtendedSTC.BaseValue, Params_p->PictureInfos_p->VideoParams.PTS.PTS, VIDAVSYNC_Data_p->CurrentDriftThreshold, NumberToDo));*/
            if (ExtendedSTC.BaseValue < Params_p->PictureInfos_p->VideoParams.PTS.PTS)
            {
       /*         TraceBuffer(("-%c%d-%d(%d)",
                        '>',
                        ExtendedSTC.BaseValue,
                        Params_p->PictureInfos_p->VideoParams.PTS.PTS,
                        Params_p->PictureInfos_p->VideoParams.PTS.PTS - ExtendedSTC.BaseValue ));
            }
            else
            {
                TraceBuffer(("-%c%d-%d(%d)",
                        '<',
                        ExtendedSTC.BaseValue,
                        Params_p->PictureInfos_p->VideoParams.PTS.PTS,
                        ExtendedSTC.BaseValue - Params_p->PictureInfos_p->VideoParams.PTS.PTS));   */
            }
#endif /* TRACE_UART */

            /* ErrorCode =*/ ActionRepeat(AVSyncHandle, NumberToDo, Params_p->PictureInfos_p->VideoParams.PictureStructure);

        } /* end of automatic synchronisation enabled */
            /* Synchronization disabled: send events if just loosing synchronisation (not at every check) */
            if (!(VIDAVSYNC_Data_p->OutOfSynchronization))
            {
                /* Loosing synchronisation */
                VIDAVSYNC_Data_p->OutOfSynchronization = TRUE;
                STEVT_Notify(VIDAVSYNC_Data_p->EventsHandle, VIDAVSYNC_Data_p->RegisteredEventsID[VIDAVSYNC_OUT_OF_SYNC_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
            }
#ifdef TRACE_UART
/*            TraceBuffer(("-advance!!"));*/
#endif /* TRACE_UART */
        VIDAVSYNC_Data_p->PreviousPictureCheckedWasSynchronizedOk = FALSE;

        if (VIDAVSYNC_Data_p->IsSynchronisationFirstCheck)
        {
            VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk = FALSE;
            VIDAVSYNC_Data_p->SynchronisationInfo.IsBackToSynchronisation = FALSE;
            VIDAVSYNC_Data_p->SynchronisationInfo.IsLoosingSynchronisation = TRUE;
        }
        else
        {
            VIDAVSYNC_Data_p->SynchronisationInfo.IsLoosingSynchronisation = FALSE;
            if ( VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk)
            {
                VIDAVSYNC_Data_p->SynchronisationInfo.IsLoosingSynchronisation = TRUE;
            }
            VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk = FALSE;
            VIDAVSYNC_Data_p->SynchronisationInfo.IsBackToSynchronisation = FALSE;
        }

    } /* end PTS in advance */
    else
    {
        /* PTS not in advance: check if it is late */
        Threshold = CurrentSTC;
        vidcom_SubstractU32ToPTS(Threshold, ThisAutomaticDriftThreshold);
        if (vidcom_IsPTSLessThanPTS(Params_p->PictureInfos_p->VideoParams.PTS, Threshold))
        {
            /* PTS is late: request speed up */
            if (VIDAVSYNC_Data_p->AutomaticSynchronizationEnabled)
            {
                /* Synchronization enabled: skip "NbFieldAccuracy" field(s) (1 or 2 fields regarding the care of    */
                /* polarity of display).                                                                            */
                PTSDifference =
                        (((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / 1000) * NbFieldAccuracy * VIDAVSYNC_Data_p->VTGVsyncDurationMicroSecond) / 1000); /* Two VSync duration */
                NumberToDo = 0;
                do
                {
                    /* Seek number n of fields to repeat */
                    NumberToDo += NbFieldAccuracy;
                    vidcom_SubstractU32ToPTS(Threshold, PTSDifference);
                } while(vidcom_IsPTSLessThanPTS(Params_p->PictureInfos_p->VideoParams.PTS, Threshold));
#ifdef TRACE_UART
                /* Display type of image */
/*                TraceBuffer(("\r\nSKIP:+%c", ((Params_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)?'B':((Params_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)?'P':'I'))));*/
/*                TraceBuffer(("\r\n-STC=%d PTS=%d Th=%d NumberToDo=%d \r\n", ExtendedSTC.BaseValue, Params_p->PictureInfos_p->VideoParams.PTS.PTS, VIDAVSYNC_Data_p->CurrentDriftThreshold, NumberToDo));*/
/*                if (ExtendedSTC.BaseValue < Params_p->PictureInfos_p->VideoParams.PTS.PTS)
                {
                    TraceBuffer(("-%c%d-%d(%d)",
                            '>',
                            ExtendedSTC.BaseValue,
                            Params_p->PictureInfos_p->VideoParams.PTS.PTS,
                            Params_p->PictureInfos_p->VideoParams.PTS.PTS - ExtendedSTC.BaseValue ));
                }
                else
                {
                    TraceBuffer(("-%c%d-%d(%d)",
                            '<',
                            ExtendedSTC.BaseValue,
                            Params_p->PictureInfos_p->VideoParams.PTS.PTS,
                            ExtendedSTC.BaseValue - Params_p->PictureInfos_p->VideoParams.PTS.PTS));
                }                                                                                    */
#endif /* TRACE_UART */

                /* Do skip actions */
                /* ErrorCode =*/ ActionSkip(AVSyncHandle, NumberToDo, Params_p->PictureInfos_p->VideoParams.PictureStructure);

            } /* end of automatic synchronisation enabled */
                /* Synchronization disabled: send events if just loosing synchronisation (not at every check) */
                if (!(VIDAVSYNC_Data_p->OutOfSynchronization))
                {
                    /* Loosing synchronisation */
                    VIDAVSYNC_Data_p->OutOfSynchronization = TRUE;
                    STEVT_Notify(VIDAVSYNC_Data_p->EventsHandle, VIDAVSYNC_Data_p->RegisteredEventsID[VIDAVSYNC_OUT_OF_SYNC_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                }
#ifdef TRACE_UART
       /*         TraceBuffer(("-late!!"));*/
#endif /* TRACE_UART */

            VIDAVSYNC_Data_p->PreviousPictureCheckedWasSynchronizedOk = FALSE;
            if (VIDAVSYNC_Data_p->IsSynchronisationFirstCheck)
            {
                VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk = FALSE;
                VIDAVSYNC_Data_p->SynchronisationInfo.IsBackToSynchronisation = FALSE;
                VIDAVSYNC_Data_p->SynchronisationInfo.IsLoosingSynchronisation = TRUE;
            }
            else
            {
                VIDAVSYNC_Data_p->SynchronisationInfo.IsLoosingSynchronisation = FALSE;
                if ( VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk)
                {
                    VIDAVSYNC_Data_p->SynchronisationInfo.IsLoosingSynchronisation = TRUE;
                }
                VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk = FALSE;
                VIDAVSYNC_Data_p->SynchronisationInfo.IsBackToSynchronisation = FALSE;
            }


        } /* end PTS late */
        else
        {
#if defined (TRACE_VISUAL_UART)
TraceState("CHECK SYNC", "SyncOK");
#endif
            /* Synchronisation OK (not late and not in advance) */
#ifdef TRACE_UART
            /* Display type of image */
/*            TraceBuffer(("+%c", ((Params_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)?'B':((Params_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)?'P':'I'))));*/
/*            TraceBuffer(("ok", ExtendedSTC.BaseValue, Params_p->PictureInfos_p->VideoParams.PTS.PTS));*/
/*            TraceBuffer(("ok(%d,%d)", ExtendedSTC.BaseValue, Params_p->PictureInfos_p->VideoParams.PTS.PTS));*/
#endif /* TRACE_UART */
            VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter = 0;
            VIDAVSYNC_Data_p->PreviousPictureCheckedWasSynchronizedOk = TRUE;
            VIDAVSYNC_Data_p->BigRestoreInProgress = FALSE;

            if (VIDAVSYNC_Data_p->OutOfSynchronization)
            {
                /* Reaching synchronisation */
                VIDAVSYNC_Data_p->OutOfSynchronization = FALSE;
                STEVT_Notify(VIDAVSYNC_Data_p->EventsHandle, VIDAVSYNC_Data_p->RegisteredEventsID[VIDAVSYNC_BACK_TO_SYNC_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
            }

            if (VIDAVSYNC_Data_p->IsSynchronisationFirstCheck)
            {
                VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk = TRUE;
                VIDAVSYNC_Data_p->SynchronisationInfo.IsLoosingSynchronisation = FALSE;
                VIDAVSYNC_Data_p->SynchronisationInfo.IsBackToSynchronisation = TRUE;
            }
            else
            {
                if (!(VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk) )
                {
                    VIDAVSYNC_Data_p->SynchronisationInfo.IsBackToSynchronisation = TRUE;
                }
                else
                {
                    VIDAVSYNC_Data_p->SynchronisationInfo.IsBackToSynchronisation = FALSE;
                }
                VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk = TRUE;
                VIDAVSYNC_Data_p->SynchronisationInfo.IsLoosingSynchronisation = FALSE;
            }

#ifdef STVID_DEBUG_GET_STATISTICS
            VIDAVSYNC_Data_p->StatisticsAvsyncPictureCheckedSynchronizedOk ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
        } /* end synchronisation OK */
    } /* end not in advance */
    if ( vidavsync_IsEventToBeNotified(AVSyncHandle, VIDAVSYNC_SYNCHRONISATION_CHECK_EVT_ID) )
/* if (vidavsync_IsEventEnabled(AVSyncHandle, VIDAVSYNC_SYNCHRONISATION_CHECK_EVT_ID) */
    {
        ComputeSTCandPTSClockDifference(CurrentSTC,Params_p->PictureInfos_p->VideoParams.PTS,
                &VIDAVSYNC_Data_p->SynchronisationInfo.ClocksDifference);
        STEVT_Notify(VIDAVSYNC_Data_p->EventsHandle,
                     VIDAVSYNC_Data_p->RegisteredEventsID[VIDAVSYNC_SYNCHRONISATION_CHECK_EVT_ID],
                    STEVT_EVENT_DATA_TYPE_CAST &(VIDAVSYNC_Data_p->SynchronisationInfo));
    }
    VIDAVSYNC_Data_p->IsSynchronisationFirstCheck = FALSE;

#ifdef STORE_SYNCHRO_INFO_IN_FILE
    if ( vidavsync_IsEventToBeNotified(AVSyncHandle, VIDAVSYNC_SYNCHRONISATION_CHECK_EVT_ID) )
    {
        if (IndexFile < MESURES_NB)
        {
            /*STCValue[IndexFile] = Threshold.PTS;
            PTSValue[IndexFile] = (Params_p->PictureInfos_p->VideoParams.PTS.PTS);*/
            STCValue[IndexFile] = STCAtNextVSync.PTS;
            PTSValue[IndexFile] = Params_p->PictureInfos_p->VideoParams.PTS.PTS;
            ClockDiffValue[IndexFile] = VIDAVSYNC_Data_p->SynchronisationInfo.ClocksDifference;
            IsSynchroValue[IndexFile] = VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk;
            IsLoosingValue[IndexFile] = VIDAVSYNC_Data_p->SynchronisationInfo.IsLoosingSynchronisation;
            IsBackValue[IndexFile] = VIDAVSYNC_Data_p->SynchronisationInfo.IsBackToSynchronisation;
            IndexFile++;
        }
        else
        {
            strcpy (Buffer_p,"");
            strcpy (FinalBuffer_p,"");
            for (IndexFile = 0; IndexFile < MESURES_NB; IndexFile++)
            {
                sprintf(Buffer_p,"%u \n", STCValue[IndexFile]);
                strcat (FinalBuffer_p, Buffer_p);
            }
            debugwrite(FileHandleSTC, FinalBuffer_p, strlen(FinalBuffer_p) );

            strcpy (Buffer_p,"");
            strcpy (FinalBuffer_p,"");
            for (IndexFile = 0; IndexFile < MESURES_NB; IndexFile++)
            {
                sprintf(Buffer_p,"%u \n", PTSValue[IndexFile]);
                strcat (FinalBuffer_p, Buffer_p);
            }
            debugwrite(FileHandlePTS, FinalBuffer_p, strlen(FinalBuffer_p) );

            strcpy (Buffer_p,"");
            strcpy (FinalBuffer_p,"");
            for (IndexFile = 0; IndexFile < MESURES_NB; IndexFile++)
            {
                sprintf(Buffer_p,"%u \n", ClockDiffValue[IndexFile]);
                strcat (FinalBuffer_p, Buffer_p);
            }
            debugwrite(FileHandleClockDifference, FinalBuffer_p, strlen(FinalBuffer_p) );

            strcpy (Buffer_p,"");
            strcpy (FinalBuffer_p,"");
            for (IndexFile = 0; IndexFile < MESURES_NB; IndexFile++)
            {
                sprintf(Buffer_p,"%u \n", IsSynchroValue[IndexFile]);
                strcat (FinalBuffer_p, Buffer_p);
            }
            debugwrite(FileHandleIsSynchro, FinalBuffer_p, strlen(FinalBuffer_p) );

            strcpy (Buffer_p,"");
            strcpy (FinalBuffer_p,"");
            for (IndexFile = 0; IndexFile < MESURES_NB; IndexFile++)
            {
                sprintf(Buffer_p,"%u \n", IsLoosingValue[IndexFile]);
                strcat (FinalBuffer_p, Buffer_p);
            }
            debugwrite(FileHandleIsLoosing, FinalBuffer_p, strlen(FinalBuffer_p) );

            strcpy (Buffer_p,"");
            strcpy (FinalBuffer_p,"");
            for (IndexFile = 0; IndexFile < MESURES_NB; IndexFile++)
            {
                sprintf(Buffer_p,"%u \n", IsBackValue[IndexFile]);
                strcat (FinalBuffer_p, Buffer_p);
            }
            debugwrite(FileHandleIsBack, FinalBuffer_p, strlen(FinalBuffer_p) );

            IndexFile = 0;
        }
    }
#endif /* STORE_SYNCHRO_INFO_IN_FILE */

    return(ST_NO_ERROR);

} /* End of CheckSynchronisation() function */

/*******************************************************************************
Name        : ApplyCurrentDriftThreshold
Description : Apply the current drift threshold according to mode.
Parameters  : AVSync handle
Assumptions :
Limitations :
Returns     : Nothing.
*******************************************************************************/
static void ApplyCurrentDriftThreshold (const VIDAVSYNC_Handle_t AVSyncHandle)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;

    if (VIDAVSYNC_Data_p->AutomaticSynchronizationEnabled)
    {
        VIDAVSYNC_Data_p->CurrentDriftThreshold = VIDAVSYNC_Data_p->AutomaticDriftThreshold;
    }
    else
    {
        VIDAVSYNC_Data_p->CurrentDriftThreshold = VIDAVSYNC_Data_p->AVSYNCDriftThreshold;
    }
} /* End of ApplyCurrentDriftThreshold() function. */
#endif /* ST_display */
/*******************************************************************************
Name        : ComputePTSandSTCClockDifference
Description : Takes the value of STC and PTS on 33 bits and returns the difference
              in units of 90Khz on a S32
Parameters  : PTS, STC and ClockDifference
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ComputeSTCandPTSClockDifference(STVID_PTS_t STC, STVID_PTS_t PTS, S32 * ClockDifference)
{

 U32 temp32;

    if ( ((STC.PTS33 == 0) && (PTS.PTS33 == 0))
        || ((STC.PTS33 == 1) && (PTS.PTS33 == 1)))
    /* STC and PTS are both on 32 or on 33 bits. */
    {
        if ( STC.PTS >= PTS.PTS)
        {
            temp32 = STC.PTS - PTS.PTS;  /* PTS is late */
            if ( (temp32&0x80000000)==0x80000000)
            {
            /* In this case, the difference is greater or equal to 0x80000000, then as the ClockDifference*/
            /* returned is on S32, it is set to maximum positive */
                *ClockDifference = 0x7FFFFFFF;  /* synchronisation lost, late. Set to max positive */
            }
            else
            {
                *ClockDifference = temp32;
            }
        }
        else
        {
            temp32 = PTS.PTS - STC.PTS;
            if ( (temp32&0x80000000)==0x80000000 )
            {
            /* In this case, the difference PTS - STC is greater or equal to 0x80000000, then as the ClockDifference*/
            /* returned is on S32, it is set to maximum negative because PTS is greater than STC */

                *ClockDifference =  0x80000000;  /* synchronisation lost, in advance. Set to max negative*/
            }
            else
            {
                *ClockDifference = STC.PTS - PTS.PTS;

            }
        }
    }
    else if ( (STC.PTS33==1) && (PTS.PTS33 == 0))
    {
    /* STC is on 33 bits and PTS on 32. */
        if ( STC.PTS >= PTS.PTS )
        {
        /* In this case STC is greater than PTS of at least 2exp32 so ClockDifference is set to maximum positive. */
            *ClockDifference = 0x7FFFFFFF;  /* Synchronisation lost, late. Set to max positive */
        }
        else
        {
         /* Else PTS and STC are right shifted by 1 so that the difference can be a U32. Greatest bit of STC is set */
         /* to 1: it is 33rd bit which is now 32nd. And the difference is computed. */
            STC.PTS = STC.PTS>>1;
            STC.PTS = STC.PTS | 0x80000000;
            PTS.PTS = PTS.PTS>>1;
            temp32 = STC.PTS - PTS.PTS;
            if ( (temp32&0xC0000000) != 0 )
            {
            /* In this case, the difference STC/2 - PTS/2 is greater or equal to 0x40000000, then as the ClockDifference */
            /* returned is on S32, it is set to maximum positive because STC is greater than PTS */
                *ClockDifference =  0x7FFFFFFF;  /* synchronisation lost, late. Set to max positive*/
            }
            else
            {
                *ClockDifference = temp32<<1;
            }
        }
    }
    else if ( (STC.PTS33==0) && (PTS.PTS33==1))
    {
        /* PTS is on 33 bits and STC on 32. */
        if ( PTS.PTS >= STC.PTS)
        {
            /* In this case PTS is greater than STC of at least 2exp32 so ClockDifference is set to maximum positive. */
            *ClockDifference = 0x80000000;  /* Synchronisation lost, in advance. Set to max negative */
        }
        else /* ( PTS.PTS < STC.PTS)*/
        {
         /* Else PTS and STC are right shifted by 1 so that the difference can be a U32. Greatest bit of PTS is set */
         /* to 1: it is 33rd bit which is now 32nd. And the difference is computed. */
            STC.PTS = STC.PTS/2;
            PTS.PTS = PTS.PTS/2;
            PTS.PTS = PTS.PTS | 0x80000000;
            temp32 = PTS.PTS - STC.PTS;
            if ( (temp32&0xC0000000) != 0 )
            {
            /* In this case, the difference PTS/2 - STC/2 is greater or equal to 0x40000000, then as the ClockDifference */
            /* returned is on S32, it is set to maximum negative because PTS is greater than STC */
                *ClockDifference = 0x80000000; /* Synchronisation lost, in advance. Set to max negative*/
            }
            else
            {
                *ClockDifference = (STC.PTS - PTS.PTS)<<1;
            }
        }
     }
 } /* End of ComputeSTCandPTSClockDifference() function */
/*******************************************************************************
Name        : InitAVSyncStructure
Description : Initialise AVSync structure
Parameters  : AVSync handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitAVSyncStructure(const VIDAVSYNC_Handle_t AVSyncHandle)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;

    VIDAVSYNC_Data_p->FrameRateConversion.CurrentOutputFrameRate = 0;
    VIDAVSYNC_Data_p->IsSynchroFieldAccurate                    = FALSE;
    VIDAVSYNC_Data_p->AutomaticDriftThreshold                   = DEFAULT_AUTOMATIC_DRIFT_THRESHOLD_WHEN_NO_VTG;
    VIDAVSYNC_Data_p->CurrentDriftThreshold                     = VIDAVSYNC_Data_p->AutomaticDriftThreshold;

    VIDAVSYNC_Data_p->VTGVsyncDurationMicroSecond = STVID_DEFAULT_VSYNC_DURATION_MICROSECOND_WHEN_NO_VTG;

    /* Parameters */
    VIDAVSYNC_Data_p->SynchronizationDelay = 0;

    /* Set enable by default */
    VIDAVSYNC_Data_p->AutomaticSynchronizationEnabled = TRUE;
    VIDAVSYNC_Data_p->SuspendSynchronisation = FALSE;

    /* No desynchronisation at init */
    VIDAVSYNC_Data_p->OutOfSynchronization = FALSE;
    VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter = 0;
    VIDAVSYNC_Data_p->StreamContainsTruePTS = FALSE;
    VIDAVSYNC_Data_p->PreviousPictureCheckedWasSynchronizedOk = FALSE;
    VIDAVSYNC_Data_p->NbConsecutiveHugeDifference = 0;
    VIDAVSYNC_Data_p->BigRestoreInProgress = FALSE;
    VIDAVSYNC_Data_p->LastGetSTCValue.PTS = 0;
    VIDAVSYNC_Data_p->LastGetSTCValue.PTS33 = FALSE;
    VIDAVSYNC_Data_p->EventNotificationConfiguration[VIDAVSYNC_SYNCHRONISATION_CHECK_EVT_ID].ConfigureEventParam.Enable = FALSE;
    VIDAVSYNC_Data_p->SynchronisationInfo.IsSynchronisationOk = FALSE;
    VIDAVSYNC_Data_p->SynchronisationInfo.IsLoosingSynchronisation = FALSE;
    VIDAVSYNC_Data_p->SynchronisationInfo.IsBackToSynchronisation = FALSE;
    VIDAVSYNC_Data_p->IsSynchronisationFirstCheck = TRUE;
    VIDAVSYNC_Data_p->LastPTSValue.PTS = 0;
    VIDAVSYNC_Data_p->LastPTSValue.PTS33 = FALSE;
    VIDAVSYNC_Data_p->LastPTSValue.Interpolated = FALSE;

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDAVSYNC_Data_p->StatisticsAvsyncSkippedFields = 0;
    VIDAVSYNC_Data_p->StatisticsAvsyncRepeatedFields = 0;
    VIDAVSYNC_Data_p->StatisticsAvsyncExtendedSTCAvailable = 0;
    VIDAVSYNC_Data_p->StatisticsAvsyncPictureWithNonInterpolatedPTS = 0;
    VIDAVSYNC_Data_p->StatisticsAvsyncPictureCheckedSynchronizedOk = 0;
    VIDAVSYNC_Data_p->StatisticsAvsyncPTSInconsistency = 0;
    VIDAVSYNC_Data_p->StatisticsAvsyncMaxRepeatedFields = 0;
    VIDAVSYNC_Data_p->StatisticsAvsyncFailedToSkipFields = 0;
#endif /* STVID_DEBUG_GET_STATISTICS */

} /* End of InitAVSyncStructure() function */


/*******************************************************************************
Name        : vidavsync_IsEventToBeNotified
Description : Test if the corresponding event is to be notified (enabled,  ...)
Parameters  : AVSync handle,
              event ID (defined in vid_sync.h).
Assumptions : Event_ID is supposed to be valid.
Limitations :
Returns     : Boolean, indicating if the event can be notified.
*******************************************************************************/
 BOOL vidavsync_IsEventToBeNotified(const VIDAVSYNC_Handle_t AVSyncHandle, const U32 Event_ID)
{
    BOOL RetValue = FALSE;
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;

    STVID_ConfigureEventParams_t * ConfigureEventParams_p;
    U32                          * NotificationSkipped_p;

    /* Get the event configuration. */
    ConfigureEventParams_p = &VIDAVSYNC_Data_p->EventNotificationConfiguration[Event_ID].ConfigureEventParam;
    NotificationSkipped_p  = &VIDAVSYNC_Data_p->EventNotificationConfiguration[Event_ID].NotificationSkipped;

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
                if ( (*NotificationSkipped_p)++ >= ConfigureEventParams_p->NotificationsToSkip)
                {
                    RetValue = TRUE;
                    (*NotificationSkipped_p) = 0;
                }
                break;
        }
    }

    /* Return the result... */
    return(RetValue);

}     /* End of vidavsync_IsEventToBeNotified() function */

#ifdef ST_display
/*******************************************************************************
Name        : NewPictureReady
Description : Function to subscribe the new pictur eready events
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void NewPictureReady(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDAVSYNC_Handle_t AVSyncHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDAVSYNC_Handle_t, SubscriberData_p);
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;
    ST_ErrorCode_t ErrorCode;

	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);

    /* Treat event */
    switch (Event)
    {
        case VIDDISP_VSYNC_DISPLAY_NEW_PICTURE_EVT :
            if(VIDAVSYNC_Data_p->SynchroType == VIDAVSYNC_CHECK_SYNCHRO_ON_PICTURE_CURRENTLY_DISPLAYED)
            {
                ErrorCode = CheckSynchronisation(AVSyncHandle, (const VIDDISP_VsyncDisplayNewPictureParams_t *) EventData_p);
            }
            break;
        case VIDDISP_NEW_PICTURE_PREPARED_EVT :
            if(VIDAVSYNC_Data_p->SynchroType == VIDAVSYNC_CHECK_SYNCHRO_ON_PREPARED_PICTURE_FOR_NEXT_VSYNC)
            {
                ErrorCode = CheckSynchronisation(AVSyncHandle, (const VIDDISP_VsyncDisplayNewPictureParams_t *) EventData_p);
            }
            break;
        default :
            break;
    }
} /* End of NewPictureReady() function */

/*******************************************************************************
Name        : PerformMissedSynchronisationActions
Description : Check counter of missed synchronisation actions and try to re-do them
Parameters  : AVSync handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PerformMissedSynchronisationActions(const VIDAVSYNC_Handle_t AVSyncHandle, const STVID_PictureStructure_t CurrentPictureStructure)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;
/*    ST_ErrorCode_t ErrorCode;*/

    if (VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter > 2)
    {
        /* Use the same skip actions when here as when in CheckSynchronisation with non-Interpolated PTS */
        /* ErrorCode =*/ ActionSkip(AVSyncHandle, VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter & (~1), CurrentPictureStructure); /* Round to even value */
    } /* end skip was not done */
    else if (VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter < (-2))
    {
        /* ErrorCode =*/ ActionRepeat(AVSyncHandle, (-(VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter)) & (~1), CurrentPictureStructure); /* Round to even value */
    } /* end repeat was not done */
} /* End of PerformMissedSynchronisationActions() function */
#endif /* ST_display */


/*******************************************************************************
Name        : ResetSynchronisation
Description : Reset synchronisation (used when loosing clock, at init)
              After this, synchro process will wait for the following conditions before synchronizing:
              * a non-interpolated PTS is coming
              *
Parameters  : AVSync handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ResetSynchronisation(const VIDAVSYNC_Handle_t AVSyncHandle)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;

    VIDAVSYNC_Data_p->MissingSkipIfPosRepeatIfNegFieldsCounter = 0;
    VIDAVSYNC_Data_p->StreamContainsTruePTS = FALSE;
    VIDAVSYNC_Data_p->BigRestoreInProgress = FALSE;
    VIDAVSYNC_Data_p->IsSynchronisationFirstCheck = TRUE;
    VIDAVSYNC_Data_p->LastPTSValue.PTS = 0;
    VIDAVSYNC_Data_p->LastPTSValue.PTS33 = FALSE;
    VIDAVSYNC_Data_p->LastPTSValue.Interpolated = FALSE;
} /* End of ResetSynchronisation() function */


/*******************************************************************************
Name        : TermAVSyncStructure
Description : Terminate AVSync structure
Parameters  : AVSync handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TermAVSyncStructure(const VIDAVSYNC_Handle_t AVSyncHandle)
{
/*    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;*/
    UNUSED_PARAMETER(AVSyncHandle);

} /* End of TermAVSyncStructure() function */


#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : VIDAVSYNC_GetStatistics
Description : Get the statistics of the avsync module.
Parameters  : Video avsync handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_GetStatistics(const VIDAVSYNC_Handle_t AVSyncHandle, STVID_Statistics_t * const Statistics_p)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Statistics_p->AvsyncSkippedFields                   = VIDAVSYNC_Data_p->StatisticsAvsyncSkippedFields;
    Statistics_p->AvsyncRepeatedFields                  = VIDAVSYNC_Data_p->StatisticsAvsyncRepeatedFields;
    Statistics_p->AvsyncExtendedSTCAvailable            = VIDAVSYNC_Data_p->StatisticsAvsyncExtendedSTCAvailable;
    Statistics_p->AvsyncPictureWithNonInterpolatedPTS   = VIDAVSYNC_Data_p->StatisticsAvsyncPictureWithNonInterpolatedPTS;
    Statistics_p->AvsyncPictureCheckedSynchronizedOk    = VIDAVSYNC_Data_p->StatisticsAvsyncPictureCheckedSynchronizedOk;
    Statistics_p->AvsyncPTSInconsistency                = VIDAVSYNC_Data_p->StatisticsAvsyncPTSInconsistency;
    Statistics_p->AvsyncMaxRepeatedFields               = VIDAVSYNC_Data_p->StatisticsAvsyncMaxRepeatedFields;
    Statistics_p->AvsyncFailedToSkipFields              = VIDAVSYNC_Data_p->StatisticsAvsyncFailedToSkipFields;

    return(ST_NO_ERROR);
} /* End of VIDAVSYNC_GetStatistics() function */

/*******************************************************************************
Name        : VIDAVSYNC_ResetStatistics
Description : Reset the statistics of the avsync module.
Parameters  : Video avsync handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER if Statistics_p == NULL
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_ResetStatistics(const VIDAVSYNC_Handle_t AVSyncHandle, const STVID_Statistics_t * const Statistics_p)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Reset parameters that are said to be reset (giving value != 0) */
    if (Statistics_p->AvsyncSkippedFields != 0)
    {
        VIDAVSYNC_Data_p->StatisticsAvsyncSkippedFields = 0;
    }
    if (Statistics_p->AvsyncRepeatedFields != 0)
    {
        VIDAVSYNC_Data_p->StatisticsAvsyncRepeatedFields = 0;
    }
    if (Statistics_p->AvsyncExtendedSTCAvailable != 0)
    {
        VIDAVSYNC_Data_p->StatisticsAvsyncExtendedSTCAvailable = 0;
    }
    if (Statistics_p->AvsyncPictureWithNonInterpolatedPTS != 0)
    {
        VIDAVSYNC_Data_p->StatisticsAvsyncPictureWithNonInterpolatedPTS = 0;
    }
    if (Statistics_p->AvsyncPictureCheckedSynchronizedOk != 0)
    {
        VIDAVSYNC_Data_p->StatisticsAvsyncPictureCheckedSynchronizedOk = 0;
    }
    if (Statistics_p->AvsyncPTSInconsistency != 0)
    {
        VIDAVSYNC_Data_p->StatisticsAvsyncPTSInconsistency = 0;
    }
    if (Statistics_p->AvsyncMaxRepeatedFields != 0)
    {
        VIDAVSYNC_Data_p->StatisticsAvsyncMaxRepeatedFields = 0;
    }
    if (Statistics_p->AvsyncFailedToSkipFields != 0)
    {
        VIDAVSYNC_Data_p->StatisticsAvsyncFailedToSkipFields = 0;
    }

    return(ST_NO_ERROR);
} /* End of VIDAVSYNC_ResetStatistics() function */
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
/*******************************************************************************
Name        : VIDAVSYNC_GetStatus
Description : Get the status of the avsync module.
Parameters  : Video avsync handle, status.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDAVSYNC_GetStatus(const VIDAVSYNC_Handle_t AVSyncHandle, STVID_Status_t * const Status_p)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;

    if (Status_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Ensure nobody else accesses avsync parameters */
    semaphore_wait(VIDAVSYNC_Data_p->AVSyncParamsLockingSemaphore_p);

    Status_p->SyncDelay = VIDAVSYNC_Data_p->SynchronizationDelay ;

    /* Ensure nobody else accesses avsync parameters */
    semaphore_signal(VIDAVSYNC_Data_p->AVSyncParamsLockingSemaphore_p);


    return(ST_NO_ERROR);
} /* End of VIDAVSYNC_GetStatus() function */
#endif /* STVID_DEBUG_GET_STATUS */


#ifdef ST_display
/*******************************************************************************
Name        : IsPTSConsistent
Description : Check the current PTS is up to the previous one. If not, it is an
              inconsistency.
Parameters  : AVSync handle, current PTS
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if PTS > LastPTS
              ST_ERROR_BAD_PARAMETER if LastPTS > PTS.
*******************************************************************************/
static ST_ErrorCode_t IsPTSConsistent(const VIDAVSYNC_Handle_t AVSyncHandle, const STVID_PTS_t PTS)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    /* Check PTS are increasing. */
    if (vidcom_IsPTSGreaterThanPTS(VIDAVSYNC_Data_p->LastPTSValue, PTS))
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    return(ErrorCode);
} /* end of IsPTSConsistent() function */

/*******************************************************************************
Name        : IsPTSEqualToLastPTS
Description : Check the current PTS is up to the previous one. If not, it is an
              inconsistency.
Parameters  : AVSync handle, current PTS
Assumptions :
Limitations :
Returns     : TRUE if LastPTS == PTS
              FALSE if not equal.
*******************************************************************************/
static BOOL IsPTSEqualToLastPTS(const VIDAVSYNC_Handle_t AVSyncHandle, const STVID_PTS_t PTS)
{
    VIDAVSYNC_Data_t * VIDAVSYNC_Data_p = (VIDAVSYNC_Data_t *) AVSyncHandle;

    return( (VIDAVSYNC_Data_p->LastPTSValue.PTS33 == PTS.PTS33) && (VIDAVSYNC_Data_p->LastPTSValue.PTS == PTS.PTS));
} /* end of IsPTSEqualToLastPTS() function */
#endif /* ST_display */


/* End of vid_sync.c */
