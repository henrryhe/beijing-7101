/*******************************************************************************

File name   : vid_dini.c

Description : Video display source file. Initialisation / Termination.

    VIDDISP_Init
    VIDDISP_Term
    VIDDISP_Reset

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
 02 Sep 2002        Created                                   Michel Bruant
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#endif

#include "stddefs.h"
#include "stvid.h"
#include "display.h"
#include "vid_disp.h"
#include "queue.h"
#include "stos.h"
#ifdef ST_ordqueue
#include "ordqueue.h"
#endif /* ST_ordqueue */

/*#define TRACE_INIT*/

#ifdef TRACE_INIT /* define or uncomment in ./makefile to set traces */
  #include "vid_trc.h"
#else
  #define TraceInfo(x)
  #define TraceError(x)
#endif

#include "sttbx.h"

/* Private Constants -------------------------------------------------------- */
#define VIDDISP_VALID_HANDLE 0x01751750

/* Private macros ----------------------------------------------------------- */

/* Private Functions prototypes --------------------------------------------- */

static ST_ErrorCode_t StopDisplayTask(const VIDDISP_Handle_t DisplayHandle);
static ST_ErrorCode_t StartDisplayTask(const VIDDISP_Handle_t DisplayHandle);
static void InitDisplayStructure(const VIDDISP_Handle_t DisplayHandle);
static void TermDisplayStructure(const VIDDISP_Handle_t DisplayHandle);

/*******************************************************************************
Name        : VIDDISP_Init
Description : Initialise display
Parameters  : Init params, returns display handle
Assumptions : InitParams_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if already initialised
              STVID_ERROR_EVENT_REGISTRATION if problem registering event
              ST_ERROR_BAD_PARAMETER if bad parameter
*******************************************************************************/
ST_ErrorCode_t VIDDISP_Init(const VIDDISP_InitParams_t * const InitParams_p,
                            VIDDISP_Handle_t * const DisplayHandle_p)
{
    VIDDISP_Data_t * VIDDISP_Data_p;
    HALDISP_InitParams_t HALInitParams;
    STEVT_OpenParams_t STEVT_OpenParams;
    ST_ErrorCode_t ErrorCode;
    U8 HALNumber;

    if ((DisplayHandle_p == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    TraceInfo(("Video Display Init...\r\n"));

    /* Allocate the main structure for the Display module. */
    SAFE_MEMORY_ALLOCATE(VIDDISP_Data_p, VIDDISP_Data_t *, InitParams_p->CPUPartition_p, sizeof(VIDDISP_Data_t));
    if (VIDDISP_Data_p == NULL)
    {
        /* Error: allocation failed */
        TraceError(ST_ERROR_NO_MEMORY);
        return(ST_ERROR_NO_MEMORY);
    }

    TraceInfo(("...memory allocation..........OK\r\n"));

    /* Remember partition */
    VIDDISP_Data_p->CPUPartition_p = InitParams_p->CPUPartition_p;
    VIDDISP_Data_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;

    /* Allocation succeeded: make handle point on structure */
    *DisplayHandle_p = (VIDDISP_Handle_t *) VIDDISP_Data_p;
    VIDDISP_Data_p->ValidityCheck = VIDDISP_VALID_HANDLE;

    /* Store parameters */
    VIDDISP_Data_p->BufferManagerHandle = InitParams_p->BufferManagerHandle;
    VIDDISP_Data_p->DeviceType          = InitParams_p->DeviceType;
    VIDDISP_Data_p->DecoderNumber       = InitParams_p->DecoderNumber;

#ifdef ST_crc
    VIDDISP_Data_p->CRCHandle           = InitParams_p->CRCHandle;
#endif
#ifdef ST_XVP_ENABLE_FGT
    VIDDISP_Data_p->FGTHandle           = InitParams_p->FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
    InitDisplayStructure(*DisplayHandle_p);
    TraceInfo(("...variables initialisation...OK\r\n"));

    /* Save number of display HAL initalized for later use in VIDDISP_Term */
    VIDDISP_Data_p->DisplayHALNumber = InitParams_p->MaxDisplayHAL;

    /* Map display registers */
    /* This mapping has to be done even for OS not needing registers mapping such as OS21 */
    /* This constraint is due to the fact most of the OS (WINCE, Linux, ...) need register mapping before */
    /* being able to access them ... */
    for (HALNumber = 0; HALNumber < InitParams_p->MaxDisplayHAL; HALNumber++)
    {
        VIDDISP_Data_p->MappedRegistersBaseAddress_p[HALNumber] = (unsigned long *) STOS_MapRegisters(InitParams_p->RegistersBaseAddress[HALNumber], STVID_MAPPED_WIDTH, "VID DISP");
        STTBX_Print(("VID DISP io kernel address of phys 0x%.8x = 0x%.8x (HAL %u)\n", (U32)InitParams_p->RegistersBaseAddress[HALNumber], (U32)VIDDISP_Data_p->MappedRegistersBaseAddress_p[HALNumber], HALNumber));
    }

    /* Get AV mem mapping */
    STAVMEM_GetSharedMemoryVirtualMapping(&VIDDISP_Data_p->VirtualMapping );

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName,
                        &STEVT_OpenParams,
                        &(VIDDISP_Data_p->EventsHandle));
    /* a second open maybe usefull if the two display-layers use the same */
    /* vtg : stevt driver will accept the subscrition only if different   */
    /* are used ... */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName,
                        &STEVT_OpenParams,
                        &(VIDDISP_Data_p->AuxEventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        TraceError(STVID_ERROR_EVENT_REGISTRATION);
        VIDDISP_Term(*DisplayHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                STVID_ASPECT_RATIO_CHANGE_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                        [VIDDISP_ASPECT_RATIO_CHANGE_EVT_ID]));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                STVID_FRAME_RATE_CHANGE_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                        [VIDDISP_FRAME_RATE_CHANGE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                STVID_SCAN_TYPE_CHANGE_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                        [VIDDISP_SCAN_TYPE_CHANGE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_PRE_DISPLAY_VSYNC_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                        [VIDDISP_PRE_DISPLAY_VSYNC_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_POST_DISPLAY_VSYNC_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                        [VIDDISP_POST_DISPLAY_VSYNC_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                STVID_RESOLUTION_CHANGE_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                        [VIDDISP_RESOLUTION_CHANGE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                STVID_DISPLAY_NEW_FRAME_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                        [VIDDISP_DISPLAY_NEW_FRAME_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_NEW_PICTURE_PREPARED_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                 [VIDDISP_NEW_PICTURE_PREPARED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                 [VIDDISP_NEW_PICTURE_TO_BE_DISPLAYED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                              [VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_VSYNC_DISPLAY_NEW_PICTURE_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                   [VIDDISP_VSYNC_DISPLAY_NEW_PICTURE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_VSYNC_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                            [VIDDISP_VSYNC_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_VTG_SCANTYPE_CHANGE_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                            [VIDDISP_VTG_SCANTYPE_CHANGE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_OUT_OF_PICTURES_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                            [VIDDISP_OUT_OF_PICTURES_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_FIELDS_REPEATED_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                            [VIDDISP_FIELDS_REPEATED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_COMMIT_NEW_PICTURE_PARAMS_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                            [VIDDISP_COMMIT_NEW_PICTURE_PARAMS_EVT_ID]));
    }

#ifdef ST_smoothbackward
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_NO_NEXT_PICTURE_TO_DISPLAY_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                                  [VIDDISP_NO_NEXT_PICTURE_TO_DISPLAY_EVT_ID]));
    }
#endif /* ST_smoothbackward */
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                VIDDISP_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                           [VIDDISP_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                InitParams_p->VideoName,
                                STVID_NEW_FIELD_TO_BE_DISPLAYED_EVT,
                                &(VIDDISP_Data_p->RegisteredEventsID
                           [VIDDISP_NEW_FIELD_TO_BE_DISPLAYED_EVT_ID]));
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        TraceError(STVID_ERROR_EVENT_REGISTRATION);
        VIDDISP_Term(*DisplayHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    TraceInfo(("...events registration........OK\r\n"));

    HALInitParams.CPUPartition_p          = InitParams_p->CPUPartition_p;
    HALInitParams.DeviceType              = InitParams_p->DeviceType;
    HALInitParams.DecoderNumber           = InitParams_p->DecoderNumber;
    strcpy(HALInitParams.EventsHandlerName, InitParams_p->EventsHandlerName);
    strcpy(HALInitParams.VideoName, InitParams_p->VideoName);
#ifdef ST_crc
    HALInitParams.CRCHandle               = InitParams_p->CRCHandle;
#endif /* ST_crc */
    for (HALNumber = 0; HALNumber < InitParams_p->MaxDisplayHAL; HALNumber++)
    {
        /* Init each HAL separately. Only Registers base address parameter may change. */
        HALInitParams.RegistersBaseAddress_p = VIDDISP_Data_p->MappedRegistersBaseAddress_p[HALNumber];
        ErrorCode = HALDISP_Init(&HALInitParams, &(VIDDISP_Data_p->HALDisplayHandle[HALNumber]), HALNumber);

        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: HAL init failed, undo initializations */
            TraceError(ErrorCode);
            VIDDISP_Term(*DisplayHandle_p);
            return(ErrorCode);
        }
        TraceInfo(("...HAL #%u initialization......OK\r\n", HALNumber + 1));
    }

    /* Start display task */
    ErrorCode = StartDisplayTask(*DisplayHandle_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: task creation failed, undo initialisations done */
        TraceError(ErrorCode);
        VIDDISP_Term(*DisplayHandle_p);
        if (ErrorCode == ST_ERROR_ALREADY_INITIALIZED)
        {
            /* Change error code because ST_ERROR_ALREADY_INITIALIZED */
            /* is not allowed for _Init() functions */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        return(ErrorCode);
    }

    TraceInfo(("...task initialisation........OK\r\n"));

#ifdef ST_ordqueue
	ErrorCode = VIDDISP_EnableDisplay(*DisplayHandle_p, InitParams_p->VideoName);
    if (ErrorCode != ST_NO_ERROR)
	{
		VIDDISP_Term(*DisplayHandle_p);
	}
#endif /* ST_ordqueue */

    VIDDISP_Data_p->QueuePictureIndex = 0;
#ifdef STVID_FRAME_RATE_CONVERSION
    VIDDISP_Data_p->PreviousFrameRate=0;
#endif /** STVID_FRAME_RATE_CONVERSION **/
    TraceInfo(("...Done.\r\n"));

    return(ErrorCode);
} /* end of VIDDISP_Init() */

/*******************************************************************************
Name        : VIDDISP_Reset
Description : Frees the currently displayed picture
Parameters  : Display handle
Assumptions : The video driver is stopped
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t VIDDISP_Reset(const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t * VIDDISP_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDDISP_Picture_t*  CurrentlyDisplayedPicture_p;

    TraceInfo(("Resetting Display...\r\n"));

    /* Find structure */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (VIDDISP_Data_p->ValidityCheck != VIDDISP_VALID_HANDLE)
    {
        TraceError(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }

    STOS_MutexLock(((VIDDISP_Data_t *) DisplayHandle)->ContextLockingMutex_p);
    /* Free buffers for layer 0 */
    CurrentlyDisplayedPicture_p = VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p;
    VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p   = NULL;
    if (CurrentlyDisplayedPicture_p != NULL)
    {
        viddisp_RemovePictureFromDisplayQueue(DisplayHandle, CurrentlyDisplayedPicture_p);
    }
    /* Free buffers for layer 1 */
    CurrentlyDisplayedPicture_p = VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p;
    VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p   = NULL;
    if (CurrentlyDisplayedPicture_p != NULL)
    {
        viddisp_RemovePictureFromDisplayQueue(DisplayHandle, CurrentlyDisplayedPicture_p);
    }

    VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p = NULL;
    VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p = NULL;

    VIDDISP_Data_p->LayerDisplay[0].NewPictureDisplayedOnCurrentVSync = FALSE;
    VIDDISP_Data_p->LayerDisplay[1].NewPictureDisplayedOnCurrentVSync = FALSE;

    VIDDISP_Data_p->LayerDisplay[0].PictDisplayedWhenDisconnect_p = NULL;
    VIDDISP_Data_p->LayerDisplay[1].PictDisplayedWhenDisconnect_p = NULL;

    CurrentlyDisplayedPicture_p = VIDDISP_Data_p->DisplayQueue_p;
    if (CurrentlyDisplayedPicture_p != NULL)
    {
        viddisp_RemovePictureFromDisplayQueue(DisplayHandle, CurrentlyDisplayedPicture_p);
    }

#ifdef ST_crc
    viddisp_UnlockCRCBuffer(VIDDISP_Data_p);
#endif

    /* Release all DEI references */
    viddisp_UnlockAllFields(DisplayHandle, 0);
    viddisp_UnlockAllFields(DisplayHandle, 1);

    STOS_MutexRelease(((VIDDISP_Data_t *) DisplayHandle)->ContextLockingMutex_p);

    return(ErrorCode);
} /* end of VIDDISP_Reset() */

/*******************************************************************************
Name        : VIDDISP_Term
Description : Terminate display, undo all what was done at init
Parameters  : Display handle
Assumptions : Term can be called in init when init process fails: function
                to undo things done at init should not cause trouble to the
                system
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t VIDDISP_Term(const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t * VIDDISP_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U8 HALNumber;

    TraceInfo(("Video Display Termination...\r\n"));

    /* Find structure */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (VIDDISP_Data_p->ValidityCheck != VIDDISP_VALID_HANDLE)
    {
        TraceError(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Un-subscribe and un-register events: this is done automatically by */
    /* STEVT_Close() */
    StopDisplayTask(DisplayHandle);
    TraceInfo(("...task termination...........OK\r\n"));

    /* Terminate HALs */
    for (HALNumber = 0; HALNumber < VIDDISP_Data_p->DisplayHALNumber; HALNumber++)
    {
        HALDISP_Term(VIDDISP_Data_p->HALDisplayHandle[HALNumber]);
        TraceInfo(("...HAL #%u termination.........OK\r\n", HALNumber + 1));
        /* Unmap display registers */
        STOS_UnmapRegisters(VIDDISP_Data_p->MappedRegistersBaseAddress_p[HALNumber], STVID_MAPPED_WIDTH);
    }

#ifdef ST_crc
    viddisp_UnlockCRCBuffer(VIDDISP_Data_p);
#endif

    viddisp_UnlockAllFields(DisplayHandle, 0);
    viddisp_UnlockAllFields(DisplayHandle, 1);

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(VIDDISP_Data_p->EventsHandle);
    ErrorCode = STEVT_Close(VIDDISP_Data_p->AuxEventsHandle);

    TermDisplayStructure(DisplayHandle);
    TraceInfo(("...variables reset............OK\r\n"));

    /* De-validate structure */
    VIDDISP_Data_p->ValidityCheck = 0; /* not VIDDISP_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(VIDDISP_Data_p, VIDDISP_Data_p->CPUPartition_p, sizeof(VIDDISP_Data_t));
    TraceInfo(("...memory deallocation........OK\r\n"));

    TraceInfo(("...Done.\r\n"));

    return(ErrorCode);

} /* end of VIDDISP_Term() */

/*******************************************************************************
Name        : VIDDISP_DisableDisplay
Description : Disable the display
Parameters  : display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
*******************************************************************************/
ST_ErrorCode_t VIDDISP_DisableDisplay(const VIDDISP_Handle_t DisplayHandle, const ST_DeviceName_t deviceName)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *)DisplayHandle;

#ifdef ST_ordqueue
	ErrorCode = STEVT_UnsubscribeDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                    deviceName,
                                    VIDQUEUE_READY_FOR_DISPLAY_EVT);
	if (ErrorCode != ST_NO_ERROR)
	{
		/* Error: unsubscribe failed */
		ErrorCode = ST_ERROR_BAD_PARAMETER;
	}
	else
	{
		ErrorCode = STEVT_UnsubscribeDeviceEvent(VIDDISP_Data_p->EventsHandle,
                                    deviceName,
                                    VIDQUEUE_PICTURE_CANDIDATE_TO_BE_REMOVED_FROM_DISPLAY_QUEUE_EVT);
		if (ErrorCode != ST_NO_ERROR)
		{
			/* Error: unsubscribe failed */
			ErrorCode = ST_ERROR_BAD_PARAMETER;
		}
	}
#endif /* ST_ordqueue */

	return(ErrorCode);
}

/*******************************************************************************
Name        : VIDDISP_EnableDisplay
Description : Enable the display
Parameters  : display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
*******************************************************************************/
ST_ErrorCode_t VIDDISP_EnableDisplay(const VIDDISP_Handle_t DisplayHandle, const ST_DeviceName_t deviceName)
{
		ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	#ifdef ST_ordqueue
		STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
	#endif /* ST_ordqueue */

		VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

	#ifdef ST_ordqueue
		/* Subscribe to VIDBUFF new picture to be displayed EVT */
		STEVT_SubscribeParams.NotifyCallback     = viddisp_NewPictureToBeDisplayed;
		STEVT_SubscribeParams.SubscriberData_p      = (void *) (DisplayHandle);
		ErrorCode = STEVT_SubscribeDeviceEvent(VIDDISP_Data_p->EventsHandle,
										deviceName,
										VIDQUEUE_READY_FOR_DISPLAY_EVT,
										&STEVT_SubscribeParams);
		if (ErrorCode != ST_NO_ERROR)
		{
			/* Error: subscribe failed, undo initialisations done */
			TraceError(ST_ERROR_BAD_PARAMETER);
			return(ST_ERROR_BAD_PARAMETER);
		}
		STEVT_SubscribeParams.NotifyCallback
							= viddisp_PictureCandidateToBeRemovedFromDisplayQueue;
		ErrorCode = STEVT_SubscribeDeviceEvent(VIDDISP_Data_p->EventsHandle,
					 deviceName,
					 VIDQUEUE_PICTURE_CANDIDATE_TO_BE_REMOVED_FROM_DISPLAY_QUEUE_EVT,
					 &STEVT_SubscribeParams);
		if (ErrorCode != ST_NO_ERROR)
		{
			/* Error: subscribe failed, undo initialisations done */
			TraceError(ST_ERROR_BAD_PARAMETER);
			return(ST_ERROR_BAD_PARAMETER);
		}
		TraceInfo(("...events subscription........OK\r\n"));
	#endif /* ST_ordqueue */

		return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : StartDisplayTask
Description : Start the task of display control
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
static ST_ErrorCode_t StartDisplayTask(const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t *    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    VIDCOM_Task_t *     const Task_p = &(((VIDDISP_Data_t *) DisplayHandle)->DisplayTask);
    char                TaskName[21] = "STVID[?].DisplayTask";
    ST_ErrorCode_t      ErrorCode ;

    if (Task_p->IsRunning)
    {
        TraceError(ST_ERROR_ALREADY_INITIALIZED);
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + ((VIDDISP_Data_t *) DisplayHandle)->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;

    ErrorCode = STOS_TaskCreate ( (void (*) (void*))viddisp_DisplayTaskFunc,
                                           (void*) DisplayHandle,
                                           VIDDISP_Data_p->CPUPartition_p,
                                           STVID_TASK_STACK_SIZE_DISPLAY,
                                           &Task_p->TaskStack,
                                           VIDDISP_Data_p->CPUPartition_p,
                                           &Task_p->Task_p,
                                           &Task_p->TaskDesc,
                                           STVID_TASK_PRIORITY_DISPLAY,
                                           TaskName,
#ifdef STVID_MEASURES /* High priority process for measures for 1us precision */
                                            (task_flags_high_priority_process | task_flags_no_min_stack_size) /* .. compilation with flag */
#else /* #ifdef STVID_MEASURES */
                                            task_flags_no_min_stack_size  /* ............................. compilation without flag */
#endif /* #ifdef STVID_MEASURES */
                                    );
    if (ErrorCode != ST_NO_ERROR)
    {
        TraceError(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    Task_p->IsRunning  = TRUE;
    return(ST_NO_ERROR);
} /* End of StartDisplayTask() function */

/*******************************************************************************
Name        : StopDisplayTask
Description : Stop the task of display control
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
static ST_ErrorCode_t StopDisplayTask(const VIDDISP_Handle_t DisplayHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDDISP_Data_t *) DisplayHandle)->DisplayTask);
    task_t *TaskList_p ;
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (!(Task_p->IsRunning))
    {
        TraceError(ST_ERROR_ALREADY_INITIALIZED);
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    TaskList_p = Task_p->Task_p;

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    /* schedule the task to term it */
    semaphore_signal(((VIDDISP_Data_t *) DisplayHandle)->DisplayOrder_p);

    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete (Task_p->Task_p,
                                  VIDDISP_Data_p->CPUPartition_p,
                                  Task_p->TaskStack,
                                  VIDDISP_Data_p->CPUPartition_p);
    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
} /* End of StopDisplayTask() function */

/*******************************************************************************
Name        : InitDisplayStructure
Description : Initialise display structure
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitDisplayStructure(const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    U32 LoopCounter;

    /* non-sense but better to initialise to a default value */
    VIDDISP_Data_p->LockState = 0;
    VIDDISP_Data_p->ForceNextPict = FALSE;
    VIDDISP_Data_p->LayerDisplay[0].VTGVSyncFrequency = (STLAYER_Layer_t) 0;
    VIDDISP_Data_p->LayerDisplay[1].LayerType = (STLAYER_Layer_t) 0;
    VIDDISP_Data_p->LayerDisplay[0].TimeAtVSyncNotif = (osclock_t) 0;
    VIDDISP_Data_p->LayerDisplay[1].TimeAtVSyncNotif = (osclock_t) 0;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.ScanType = STGXOBJ_INTERLACED_SCAN;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.PictureFrameRate = 0;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.Height = 0;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.Width = 0;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.PanAndScanIn16thPixel.FrameCentreHorizontalOffset = 0;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.PanAndScanIn16thPixel.FrameCentreVerticalOffset = 0;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.PanAndScanIn16thPixel.DisplayHorizontalSize = 0;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.PanAndScanIn16thPixel.DisplayVerticalSize = 0;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.PanAndScanIn16thPixel.HasDisplaySizeRecommendation = FALSE;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.Reconstruction = VIDBUFF_NONE_RECONSTRUCTION;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.ColorSpaceConversion = STGXOBJ_CONVERSION_MODE_UNKNOWN;
#ifdef ST_crc
    VIDDISP_Data_p->AreCRCParametersAvailable = FALSE;
    VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastPicture = FALSE;
    VIDDISP_Data_p->PictureUsedForCRCParameters.IsRepeatedLastButOnePicture = FALSE;
#if defined(STVID_CRC_ALLOW_FRAME_DUMP) || defined(STVID_SOFTWARE_CRC_MODE)
    VIDDISP_Data_p->PictureUsedForCRCParameters.Picture_p = NULL;
#endif
    VIDDISP_Data_p->PreviousCRCPictureNumber = -1;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.CRCCheckMode = FALSE;
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.CRCScanType = (STGXOBJ_ScanType_t) (-1);
#endif  /* ST_crc */
    VIDDISP_Data_p->LayerDisplay[0].DisplayCaract.VTGScanType = STGXOBJ_INTERLACED_SCAN;

    /* Now element 1 */
    VIDDISP_Data_p->LayerDisplay[1].DisplayCaract = VIDDISP_Data_p->LayerDisplay[0].DisplayCaract;
    for (LoopCounter = 0; LoopCounter < NB_QUEUE_ELEM; LoopCounter++)
    {
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Next_p = NULL;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].PrimaryPictureBuffer_p = NULL;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].SecondaryPictureBuffer_p = NULL;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].PictureInformation_p = NULL;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Counters[0].TopFieldCounter = 0;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Counters[0].BottomFieldCounter = 0;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Counters[0].RepeatFieldCounter = 0;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Counters[0].RepeatProgressiveCounter = 0;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Counters[0].NextFieldOnDisplay = VIDDISP_FIELD_DISPLAY_NONE;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Counters[0].CurrentFieldOnDisplay = VIDDISP_FIELD_DISPLAY_NONE;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Counters[0].CounterInDisplay = 0;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Counters[0].IsFRConversionActivated = FALSE;
        VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Counters[1] = VIDDISP_Data_p->DisplayQueueElements[LoopCounter].Counters[0];
    }

    (VIDDISP_Data_p->LayerDisplay[0].VTGName)[0] = '\0';
    VIDDISP_Data_p->LayerDisplay[0].VTGVSyncFrequency = 0;
    (VIDDISP_Data_p->LayerDisplay[1].VTGName)[0] = '\0';
    VIDDISP_Data_p->LayerDisplay[1].VTGVSyncFrequency = 0;

    /* Initialise commands queue */
    VIDDISP_Data_p->DisplayCommandsBuffer.Data_p
                                = VIDDISP_Data_p->DisplayCommandsData;
    VIDDISP_Data_p->DisplayCommandsBuffer.TotalSize
                                = sizeof(VIDDISP_Data_p->DisplayCommandsData);
    VIDDISP_Data_p->DisplayCommandsBuffer.BeginPointer_p
                                = VIDDISP_Data_p->DisplayCommandsBuffer.Data_p;
    VIDDISP_Data_p->DisplayCommandsBuffer.UsedSize  = 0;
    VIDDISP_Data_p->DisplayQueue_p                  = NULL;
    VIDDISP_Data_p->DisplayRef                      = 0;


    VIDDISP_Data_p->DisplayOrder_p = STOS_SemaphoreCreateFifo(VIDDISP_Data_p->CPUPartition_p,0);
    VIDDISP_Data_p->ContextLockingMutex_p = STOS_MutexCreateFifo();

#ifdef BENCHMARK_WINCESTAPI
	P_ADDSEMAPHORE(VIDDISP_Data_p->DisplayOrder_p , "VIDDISP_Data_p->DisplayOrder_p");
    P_ADDSEMAPHORE(VIDDISP_Data_p->ContextLockingMutex_p , "VIDDISP_Data_p->ContextLockingSemaphore_p");
#endif

    VIDDISP_Data_p->DisplayTask.IsRunning = FALSE;

    /* Display initialisations */
    VIDDISP_Data_p->LayerDisplay[0].DisplayLatency                = 1;
    VIDDISP_Data_p->LayerDisplay[1].DisplayLatency                = 1;
    VIDDISP_Data_p->LayerDisplay[0].FrameBufferHoldTime           = 1;
    VIDDISP_Data_p->LayerDisplay[1].FrameBufferHoldTime           = 1;
    VIDDISP_Data_p->LayerDisplay[0].IsOpened                      = FALSE;
    VIDDISP_Data_p->LayerDisplay[1].IsOpened                      = FALSE;
    VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p = NULL;
    VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p = NULL;
    VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p   = NULL;
    VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p   = NULL;
    VIDDISP_Data_p->LayerDisplay[0].NewPictureDisplayedOnCurrentVSync = FALSE;
    VIDDISP_Data_p->LayerDisplay[1].NewPictureDisplayedOnCurrentVSync = FALSE;
    VIDDISP_Data_p->LayerDisplay[0].IsCurrentFieldOnDisplayTop = TRUE;
    VIDDISP_Data_p->LayerDisplay[1].IsCurrentFieldOnDisplayTop = TRUE;
    VIDDISP_Data_p->LayerDisplay[0].IsFieldPreparedTop = TRUE;
    VIDDISP_Data_p->LayerDisplay[1].IsFieldPreparedTop = TRUE;
    VIDDISP_Data_p->LayerDisplay[0].PictDisplayedWhenDisconnect_p = NULL;
    VIDDISP_Data_p->LayerDisplay[1].PictDisplayedWhenDisconnect_p = NULL;
    VIDDISP_Data_p->LayerDisplay[0].ReconstructionMode
                                        = VIDBUFF_NONE_RECONSTRUCTION;
    VIDDISP_Data_p->LayerDisplay[1].ReconstructionMode
                                        = VIDBUFF_NONE_RECONSTRUCTION;

    /* Set default display */
    VIDDISP_Data_p->LayerDisplay[0].ManualPresentation  = FALSE;
    VIDDISP_Data_p->LayerDisplay[0].PresentationDelay   = 0;
    VIDDISP_Data_p->LayerDisplay[1].ManualPresentation  = FALSE;
    VIDDISP_Data_p->LayerDisplay[1].PresentationDelay   = 0;
    VIDDISP_Data_p->HasDiscontinuityInMPEGDisplay       = FALSE;
    VIDDISP_Data_p->DisplayState                = VIDDISP_DISPLAY_STATE_PAUSED;
    VIDDISP_Data_p->NotifyStopWhenFinish                = FALSE;
    VIDDISP_Data_p->DoReleaseDeiReferencesOnFreeze      = TRUE;

    VIDDISP_Data_p->DisplayForwardNotBackward = TRUE;
    VIDDISP_Data_p->LastDisplayDirectionIsForward = TRUE;
    /* Stepping order received field too.            */
    VIDDISP_Data_p->FreezeParams.PauseRequestIsPending  = FALSE;
    VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending = FALSE;
    VIDDISP_Data_p->FreezeParams.SteppingOrderReceived  = FALSE;
    VIDDISP_Data_p->FreezeParams.Freeze.Mode = STVID_FREEZE_MODE_NONE;
    VIDDISP_Data_p->FreezeParams.Freeze.Field = STVID_FREEZE_FIELD_CURRENT;

    /* No Stop Request Pending */
    VIDDISP_Data_p->StopParams.StopRequestIsPending     = FALSE;
    VIDDISP_Data_p->StopParams.StopMode                 = VIDDISP_STOP_NOW;
    VIDDISP_Data_p->StopParams.RequestStopPicturePTS.PTS = 0;
    VIDDISP_Data_p->StopParams.RequestStopPicturePTS.PTS33 = FALSE;
    VIDDISP_Data_p->StopParams.ActuallyStoppedSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(VIDDISP_Data_p->CPUPartition_p, 0);

    /* Required Care of polarity is supposed to be TRUE */
    VIDDISP_Data_p->DisplayWithCareOfFieldsPolarity             = TRUE;
    VIDDISP_Data_p->FrameRateConversionNeedsCareOfFieldsPolarity= TRUE;

    /* Initialise the event notification configuration. */
    for (LoopCounter=0; LoopCounter<VIDDISP_NB_REGISTERED_EVENTS_IDS;
            LoopCounter++)
    {
        VIDDISP_Data_p->EventNotificationConfiguration[LoopCounter]
                                .NotificationSkipped                = 0;
        VIDDISP_Data_p->EventNotificationConfiguration[LoopCounter]
                                .ConfigureEventParam.NotificationsToSkip = 0;
        VIDDISP_Data_p->EventNotificationConfiguration[LoopCounter]
                                .ConfigureEventParam.Enable         = TRUE;
    }

    VIDDISP_Data_p->LayerDisplay[0].CurrentVSyncTop             = FALSE;
    VIDDISP_Data_p->LayerDisplay[1].CurrentVSyncTop             = FALSE;
    VIDDISP_Data_p->LayerDisplay[0].IsNextOutputVSyncTop        = FALSE;
    VIDDISP_Data_p->LayerDisplay[1].IsNextOutputVSyncTop        = FALSE;
    /* Impossible value set at init.    */
    VIDDISP_Data_p->LayerDisplay[0].VSyncMemoryArray            = 1;
    VIDDISP_Data_p->LayerDisplay[1].VSyncMemoryArray            = 1;
    /* No VTG connected (for the moment)*/
    VIDDISP_Data_p->IsVTGChangedSinceLastVSync                  = FALSE;

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDDISP_Data_p->StatisticsPictureInsertedInQueue            = 0;
    VIDDISP_Data_p->StatisticsPictureDisplayedByMain            = 0;
    VIDDISP_Data_p->StatisticsPictureDisplayedByAux             = 0;
    VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedByMain   = 0;
    VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedByAux    = 0;
    VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedByMain    = 0;
    VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedByAux     = 0;
    VIDDISP_Data_p->StatisticsPbQueueLockedByLackOfPicture      = 0;
    VIDDISP_Data_p->StatisticsPbDisplayQueueOverflow            = 0;
    VIDDISP_Data_p->StatisticsPictureDisplayedBySec             = 0;
    VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedBySec    = 0;
    VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedBySec     = 0;
    VIDDISP_Data_p->StatisticsPbPicturePreparedAtLastMinuteRejected  = 0;

#ifdef ST_speed
    VIDDISP_Data_p->StatisticsSpeedDisplayIFramesNb             = 0;
    VIDDISP_Data_p->StatisticsSpeedDisplayPFramesNb             = 0;
    VIDDISP_Data_p->StatisticsSpeedDisplayBFramesNb             = 0;
#endif /*ST_speed*/
#endif /* STVID_DEBUG_GET_STATISTICS */
    VIDDISP_Data_p->InitialTemporalReferenceForSkipping            = 0;
    VIDDISP_Data_p->InitialTemporalReferenceForRepeating           = 0;
    VIDDISP_Data_p->LastFlaggedPicture                             = 0;
    VIDDISP_Data_p->StreamIs24Hz                                   = FALSE;
    VIDDISP_Data_p->EnableFrameRateConversion                      = TRUE;
    VIDDISP_Data_p->FrameRatePatchCounterTemporalReference         = 0;
    VIDDISP_Data_p->IsSlaveSequencerAllowedToRun                    = FALSE;
    VIDDISP_Data_p->PeriodicFieldInversionDueToFRC = FALSE;

    viddisp_InitMultiFieldPresentation(DisplayHandle, 0);
    viddisp_InitMultiFieldPresentation(DisplayHandle, 1);

} /* End of InitDisplayStructure() function */

/*******************************************************************************
Name        : TermDisplayStructure
Description : Terminate display structure
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TermDisplayStructure(const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

     STOS_SemaphoreDelete(VIDDISP_Data_p->CPUPartition_p,VIDDISP_Data_p->DisplayOrder_p);
     STOS_MutexDelete(VIDDISP_Data_p->ContextLockingMutex_p);
     STOS_SemaphoreDelete(VIDDISP_Data_p->CPUPartition_p,VIDDISP_Data_p->StopParams.ActuallyStoppedSemaphore_p);
} /* End of TermDisplayStructure() function */




