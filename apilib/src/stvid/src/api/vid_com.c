/*******************************************************************************

File name   : vid_com.c

Description : Video common functions commands source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
 23 Jun 2000        Created                                         AN
 02 Feb 2001        Update for Digital Input                        JA
 06 Feb 2001        Remove DecodeHandle from AVSync                 JA
 14 Feb 2001        Memory profile management                       GG
 07 Mar 2001        Compression and decimation in GetCapability()   GG
 15 Mar 2001        New STVID_ConfigureEvent() function.            GG
 04 Nov 2002        Add support for STi5528                         HG
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdlib.h>
#include <string.h>
#endif

#include <stdevice.h>

#include "stddefs.h"
#include "stos.h"

#include "api.h"
#include "vid_rev.h"

#include "vid_err.h"

#ifdef ST_inject
#include "inject.h"
#endif /* ST_inject */

#ifdef ST_transcoder
#include "transcoder.h"
#endif

#ifdef ST_ZEUS
#ifdef ST_REMOVE_STLAYER
#include "vid_display_interface.h"
#endif /* ST_REMOVE_STLAYER */
#endif /*ST_ZEUS */

/* Private Constants -------------------------------------------------------- */
#if defined(ST_7200)
#   ifndef MME_VIDEO1_TRANSPORT_NAME
#   define MME_VIDEO1_TRANSPORT_NAME "TransportVideo1"
#   endif

#   ifndef MME_VIDEO2_TRANSPORT_NAME
#   define MME_VIDEO2_TRANSPORT_NAME "TransportVideo2"
#   endif
#else  /* #if defined(ST_7200) ... */
#   ifndef MME_VIDEO_TRANSPORT_NAME
#   define MME_VIDEO_TRANSPORT_NAME  "ups"
#   endif
#endif /* #if defined(ST_7200) ... #else ... */

typedef enum ErrorStep_e
{
    ERROR_STEP_INJECT_INIT,
    ERROR_STEP_MULTIDEC_INIT,
    ERROR_STEP_BUFF_INIT,
    ERROR_STEP_BUFF_SETMEM,
    ERROR_STEP_ORDQUEUE_INIT,
    ERROR_STEP_MULTIDEC_CONNECT,
    ERROR_STEP_PROD_INIT,
    ERROR_STEP_INP_INIT,
    ERROR_STEP_DISP_INIT,
    ERROR_STEP_AVSYNC_INIT,
    ERROR_STEP_TRICK_SPEED_INIT, /* The 2 exclude each other for one instance, so only one case needed ! */
    ERROR_STEP_LAYER_INIT,
    ERROR_STEP_REGISTER_EVENTS,
    ERROR_STEP_ERROR_RECOVERY_INIT,
    ERROR_STEP_MEMORY_ALLOCATE_LAYER_PROPERTIES,
    ERROR_STEP_MEMORY_ALLOCATE_VIEWPORT_PROPERTIES,
    ERROR_STEP_MEMORY_ALLOCATE_VIDEO_STREAM,
    ERROR_STEP_MEMORY_ALLOCATE_SOURCE,
    ERROR_STEP_NO_ERROR
} ErrorStep_t;

#define INVALID_DEVICE_INDEX (-1)

#define MAX_NUMBER_OF_FRAME_BUFFERS_PER_DECODER 18

#define DEVICE_PARAMETERS_MAX_LAYERS                5
#define DEVICE_PARAMETERS_MAX_VIEWPORTS_PER_LAYER   8

#define MAX_DEVICE_TYPES    5

/* Default synchronisation method is chosen here */
/* Note : - default mode was ON_PICTURE_CURRENTLY_DISPLAYED since ever in video driver, until release 3.12.0
          - From release 3.12.1 on, default mode was set to ON_PREPARED_PICTURE_FOR_NEXT_VSYNC, up to release 3.13.3
          - Release 3.13.4 changes it back temporarly to ON_PICTURE_CURRENTLY_DISPLAYED, because of some regressions
            found with ON_PREPARED_PICTURE_FOR_NEXT_VSYNC, and not investigated in time for being fixed for the release
          - Should be back to ON_PREPARED_PICTURE_FOR_NEXT_VSYNC as soon as possible, after investigation and fix of the regressions */
/* Note for WinCE :
          - WinCE default mode needs in any case to be ON_PREPARED_PICTURE_FOR_NEXT_VSYNC, because of the WinCE architecture that
            requires everything to be driven by clocks. This special WinCE patch could be removed when the default mode will be
            ON_PREPARED_PICTURE_FOR_NEXT_VSYNC again. */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_OSWINCE)/* Temporary fix */
#define DEFAULT_SYNCHRO_TYPE    VIDAVSYNC_CHECK_SYNCHRO_ON_PREPARED_PICTURE_FOR_NEXT_VSYNC
#else /* ST_OSWINCE */
#define DEFAULT_SYNCHRO_TYPE    VIDAVSYNC_CHECK_SYNCHRO_ON_PICTURE_CURRENTLY_DISPLAYED
#endif /* not ST_OSWINCE */

enum
{
    FDMA_NONSECURE_MODE = 0,
    FDMA_SECURE_MODE
};

/* Private Types ------------------------------------------------------------ */

/* Type used to allocate number of structures necessary for the instance */
typedef struct DeviceParameters_s
{
    U8  MaxLayers;
    U8  MaxViewPortsPerLayer;
    U8  DisplayBaseAddressNumber;
    U8  CompressedDataInputBaseAddressNumber;
#ifdef ST_inject
    BOOL    UsesInject;
#endif /* ST_inject */
#ifdef ST_producer
#ifdef ST_multidecodeswcontroller
    BOOL    UsesMultiDecodeSWController;
#endif /* ST_multidecodeswcontroller */
#endif /* ST_producer */
#ifdef ST_display
    U8  DisplayHALNumber;
#endif /* ST_display */
} DeviceParameters_t;

/* Stores pairs of (device types - number of instances of this type) */
typedef struct InstancesOfDeviceType_s
{
    STVID_DeviceType_t  DeviceType;
    U8                  NumberOfInitialisedInstances;
    /* Below : handles to instances of modules that are required only ONCE per DeviceType whatever the number of STVID instances. */
#ifdef ST_inject
    VIDINJ_Handle_t     InjecterHandle;
#endif /* ST_inject */
#ifdef ST_producer
#ifdef ST_multidecodeswcontroller
    VIDDEC_MultiDecodeHandle_t      MultiDecodeHandle;
#endif /* ST_multidecodeswcontroller */
#endif /* ST_producer */
} InstancesOfDeviceType_t;

/* Private Variables (static)------------------------------------------------ */
static VideoDevice_t DeviceArray[STVID_MAX_DEVICE];
/* Init/Open/Close/Term/GetCapability/GetStatistics/ResetStatistics protection semaphore */
static semaphore_t * InterInstancesLockingSemaphore_p = NULL;
static BOOL FirstInitDone = FALSE;
static InstancesOfDeviceType_t InstancesSummary[MAX_DEVICE_TYPES];

/* Global Variables --------------------------------------------------------- */

/* not static because used in IsValidHandle macro */
VideoUnit_t stvid_UnitArray[STVID_MAX_UNIT];

/* Private Macros ----------------------------------------------------------- */
#define MAX_2VALUES(a,b)       ( (a > b) ? a : b )

/* Private Function prototypes ---------------------------------------------- */
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t RegisterEvents(const STVID_DeviceType_t DeviceType, const ST_DeviceName_t EvtHandlerName, VideoDevice_t * const Device_p, const ST_DeviceName_t DeviceName);
static ST_ErrorCode_t FindInstancesOfSameDeviceType(const STVID_DeviceType_t DeviceType, InstancesOfDeviceType_t ** const InstancesOfDeviceType_p);
static ST_ErrorCode_t GetDecoderNumber(const STVID_DeviceType_t DeviceType, const void * RegistersBaseAddress_p, U8 * const DecoderNumber_p, InstancesOfDeviceType_t ** const InstancesOfDeviceType_p);
static ST_ErrorCode_t GetDeviceParameters(const STVID_DeviceType_t DeviceType, DeviceParameters_t * const DeviceParameters_p);
static BOOL IsErrorCodeMoreImportantThanPrevious(const ST_ErrorCode_t PreviousErrorCode, const ST_ErrorCode_t NewErrorCode);
static ST_ErrorCode_t Init(VideoDevice_t * const Device_p, const STVID_InitParams_t * const InitParams_p, const ST_DeviceName_t DeviceName);
static ST_ErrorCode_t Open(VideoUnit_t * const Unit_p, const STVID_OpenParams_t * const OpenParams_p);
static ST_ErrorCode_t Close(VideoUnit_t * const Unit_p);
static ST_ErrorCode_t Term(VideoDevice_t * const Device_p, const STVID_TermParams_t * const TermParams_p);

/* Private functions -------------------------------------------------------- */

/*******************************************************************************
Name        : GetIndexOfDeviceNamed
Description : Get the index in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
Assumptions :
Limitations :
Returns     : the index if the name was found, INVALID_DEVICE_INDEX otherwise
*******************************************************************************/
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName)
{
    S32 WantedIndex = INVALID_DEVICE_INDEX, Index = 0;

    do
    {
        /* Device initialised: check if name is matching */
        if (strcmp(DeviceArray[Index].DeviceName, WantedName) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STVID_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */

/*******************************************************************************
Name        : RegisterEvents
Description : Register video driver events not registered by modules
Parameters  : Event Handler Device Name, Device.
Assumptions : Device_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER otherwise.
              STVID_ERROR_EVENT_REGISTRATION if registration problem
*******************************************************************************/
static ST_ErrorCode_t RegisterEvents(const STVID_DeviceType_t DeviceType, const ST_DeviceName_t EvtHandlerName,
        VideoDevice_t * const Device_p, const ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t ErrorCode;
    STEVT_OpenParams_t STEVT_OpenParams;
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;

    UNUSED_PARAMETER(DeviceType);

    ErrorCode = STEVT_Open(EvtHandlerName, &STEVT_OpenParams,&(Device_p->DeviceData_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error %X while Opening EventHandler %s", ErrorCode, EvtHandlerName));
        ErrorCode = STVID_ERROR_EVENT_REGISTRATION;
    }
    else
    {
        ErrorCode = STEVT_RegisterDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                    DeviceName,
                    STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT,
                    &(Device_p->DeviceData_p->EventID[STVID_IMPOSSIBLE_WITH_MEM_PROFILE_ID])
                    );
        if (ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = STEVT_RegisterDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                    DeviceName,
                    STVID_STOPPED_EVT,
                    &(Device_p->DeviceData_p->EventID[STVID_STOPPED_ID]));
#ifdef ST_diginput
            if(ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STEVT_RegisterDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                        DeviceName,
                        STVID_DIGINPUT_WIN_CHANGE_EVT,
                        &(Device_p->DeviceData_p->EventID[STVID_DIGINPUT_WIN_CHANGE_ID]));
            }
#endif/* #ifdef ST_diginput */
        }
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error %X while registering events", ErrorCode));
            /* here undo actions done. */
            ErrorCode = STEVT_Close(Device_p->DeviceData_p->EventsHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while closing event handler after registration error."));
            }
            ErrorCode = STVID_ERROR_EVENT_REGISTRATION;
        }
        else
        {
            /* Subscribe to events: store params the same for all events */
            STEVT_SubscribeParams.SubscriberData_p = (void *) (Device_p);

            /* There's a VIDBUFF: subscribe to VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT */
            STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_BuffersImpossibleWithProfile;
            ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, DeviceName,
                            VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT, &STEVT_SubscribeParams);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error: subscribe failed, undo initialisations done */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
#ifdef ST_display
            if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->DisplayHandleIsValid))
            {
                /* There's a VIDDISP: subscribe to VIDDISP_OUT_OF_PICTURES_EVT */
                STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_DisplayOutOfPictures;
                ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, DeviceName,
                                VIDDISP_OUT_OF_PICTURES_EVT, &STEVT_SubscribeParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* Error: subscribe failed, undo initialisations done */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDDISP_OUT_OF_PICTURES_EVT !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    /* There's a VIDDISP: subscribe to VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT */
                    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_PictureCharacteristicsChange;
                    ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, DeviceName,
                                    VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT, &STEVT_SubscribeParams);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        /* Error: subscribe failed, undo initialisations done */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT !"));
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                }
            } /* no error */
#endif /* ST_display */
#ifdef ST_producer
            if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->ProducerHandleIsValid))
            {
                /* There's a VIDDEC: subscribe to VIDPROD_STOPPED_EVT */
                STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_DecoderStopped;
                ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, DeviceName,
                                VIDPROD_STOPPED_EVT, &STEVT_SubscribeParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* Error: subscribe failed, undo initialisations done */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDPROD_STOPPED_EVT !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    /* There's a VIDDEC: subscribe to VIDPROD_DECODE_ONCE_READY_EVT */
                    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_DecodeOnceReady;
                    ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, DeviceName,
                                    VIDPROD_DECODE_ONCE_READY_EVT, &STEVT_SubscribeParams);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        /* Error: subscribe failed, undo initialisations done */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDPROD_DECODE_ONCE_READY_EVT !"));
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        /* There's a VIDDEC: subscribe to VIDPROD_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT */
                        STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_NewPictureSkippedWithoutRequest;
                        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, DeviceName,
                                        VIDPROD_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT, &STEVT_SubscribeParams);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* Error: subscribe failed, undo initialisations done */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDPROD_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT !"));
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                        else
                        {
                            /* There's a VIDDEC: subscribe to VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT */
                            STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_ReadyToDecodeNewPicture;
                            ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, DeviceName,
                                                                   VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT, &STEVT_SubscribeParams);
                            if (ErrorCode != ST_NO_ERROR)
                            {
                                /* Error: subscribe failed, undo initialisations done */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT !"));
                                ErrorCode = ST_ERROR_BAD_PARAMETER;
                            }
                        }
                    }
                } /* end subscribe successful */
            } /* decode handle valid */
#endif /* ST_producer */
#ifdef ST_trickmod
            if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->TrickModeHandleIsValid))
            {
                /* There's a trickmode: subscribe to VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT */
                STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_TrickmodeDisplayParamsChange;
                ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, DeviceName,
                                VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT, &STEVT_SubscribeParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* Error: subscribe failed, undo initialisations done */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            } /* TrickModeHandleIsValid */
#endif /* ST_trickmod */
#if defined ST_speed
            if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->SpeedHandleIsValid))
            {
                /*There's a speed controller: subscribe to VIDSPEED_DISPLAY_PARAMS_CHANGE_EVT */
                STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_TrickmodeDisplayParamsChange;
                ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, DeviceName,
                                VIDSPEED_DISPLAY_PARAMS_CHANGE_EVT, &STEVT_SubscribeParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* Error: subscribe failed, undo initialisations done */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDSPEED_DISPLAY_PARAMS_CHANGE_EVT !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            } /* SpeedHandleIsValid */
#endif /* ST_speed */
#if defined ST_trickmod || defined ST_speed
#ifdef ST_display
            if ((ErrorCode == ST_NO_ERROR) && (IsTrickModeHandleOrSpeedHandleValid(Device_p)))
            {
                /* There's a trickmode or speed: subscribe to VIDDISP_PRE_DISPLAY */
                STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_TrickmodeVsyncActionSpeed;
                ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, DeviceName,
                            VIDDISP_PRE_DISPLAY_VSYNC_EVT, &STEVT_SubscribeParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* Error: subscribe failed, undo initialisations done */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDDISP_PRE_DISPLAY_VSYNC_EVT !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    /* Subscribe also to VIDDISP_POST_DISPLAY */
                    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) stvid_TrickmodePostVsyncAction;
                    ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                                                        DeviceName,
                                                        VIDDISP_POST_DISPLAY_VSYNC_EVT,
                                                        &STEVT_SubscribeParams);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        /* Error: subscribe failed, undo initialisations done */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while subscribing to VIDDISP_POST_DISPLAY_VSYNC_EVT !"));
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                }
            } /* TrickModeHandleIsValid and SpeedHandleIsValid */
#endif /* ST_display */
#endif /* defined ST_trickmod || defined ST_speed */
        } /* no error */
    } /* STEVT_Open() OK */

    return(ErrorCode);
} /* End of RegisterEvents() function */


/*******************************************************************************
Name        : FindInstancesOfSameDeviceType
Description : Look for instances of the same device type that are already initialised. If not found, take a free index
Parameters  : Device type
              pointer to return row in table of all instances, the row where this device types are counted
Assumptions : if NumberOfInitialisedInstances is 0, this means that there is no instance of the same device
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t FindInstancesOfSameDeviceType(const STVID_DeviceType_t DeviceType, InstancesOfDeviceType_t ** const InstancesOfDeviceType_p)
{
    S8 Index, IndexOfSameInstances;

    /* Look for instances of the same device type that are already initialised. If not found, take a free index. */
    IndexOfSameInstances = MAX_DEVICE_TYPES; /* Outside the table, means invalid... */
    for (Index = (MAX_DEVICE_TYPES - 1); Index >= 0; Index--)
    {
        if (InstancesSummary[Index].NumberOfInitialisedInstances == 0)
        {
            /* Index of the table is not used, so eventually take it if no other instances of same device type is found */
            IndexOfSameInstances = Index;
        }
        /* CL quick fix to deal with 7100_H264 & 7100_MPEG instances which share the same VID_INJ instance */
        else if ((InstancesSummary[Index].DeviceType == DeviceType) ||
                (((DeviceType == STVID_DEVICE_TYPE_7100_MPEG) ||
                  (DeviceType == STVID_DEVICE_TYPE_7100_H264) ||
                  (DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
                  (DeviceType == STVID_DEVICE_TYPE_7109_VC1) ||
                  (DeviceType == STVID_DEVICE_TYPE_7109_MPEG) ||
                  (DeviceType == STVID_DEVICE_TYPE_7109D_MPEG) ||
                  (DeviceType == STVID_DEVICE_TYPE_7109_H264) ||
                  (DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
                  (DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
                  (DeviceType == STVID_DEVICE_TYPE_7200_VC1) ||
                  (DeviceType == STVID_DEVICE_TYPE_7200_MPEG) ||
                  (DeviceType == STVID_DEVICE_TYPE_7200_H264) ||
                  (DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2)) &&
                 ((InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7100_MPEG) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7100_H264) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109_MPEG) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109D_MPEG) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109_H264) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109_VC1) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7200_MPEG) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7200_H264) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7200_VC1) ||
                  (InstancesSummary[Index].DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))))
        {
            /* Found some instances of same device type ! End the 'for' by breaking. */
            IndexOfSameInstances = Index;
            break;
        }
    }
    /* Take un-used or confirm used index of table */
    InstancesSummary[IndexOfSameInstances].DeviceType = DeviceType;
    *InstancesOfDeviceType_p = &(InstancesSummary[IndexOfSameInstances]);

    return(ST_NO_ERROR);
} /* end of FindInstancesOfSameDeviceType() function */

/*******************************************************************************
Name        : GetDecoderNumber
Description : Get decoder number
              As decoder number is checked before allowing initialisation, this means
              the numbers returned by this function are going to limit naturally the
              number of initialisations allowed for a device type to the number of
              decoder declared to be present for this device type in this function !
              Decoder number returned is as follows:
               * Omega1: always 0
               * Omega2:
                * 7015/7020:
                        - Decoder 1 : 0
                        - Decoder 2 : 1
                        - Decoder 3 : 2
                        - Decoder 4 : 3
                        - Decoder 5 : 4
                        - Digital input SD 1 : 5
                        - Digital input SD 2 : 6
                        - Digital input HD 1 : 7
               * 5100, 5525, 5105, 5301, 7710, 7100, 7109, 5162: number of instances not limited
Parameters  : Device type, registers base address, pointer to return decoder number
              pointer to return row in table of all instances, the row where this device types are counted
Assumptions : Never returning STVID_INVALID_DECODER_NUMBER
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if can't get number
              ST_ERROR_ALREADY_INITIALIZED if already in use
*******************************************************************************/
static ST_ErrorCode_t GetDecoderNumber(const STVID_DeviceType_t DeviceType, const void * RegistersBaseAddress_p, U8 * const DecoderNumber_p, InstancesOfDeviceType_t ** const InstancesOfDeviceType_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U8 CheckCount, Index;
    BOOL RequiresCheckOfDecoderNumberUse = FALSE;
    BOOL IndexFound;

    ErrorCode = FindInstancesOfSameDeviceType(DeviceType, InstancesOfDeviceType_p); /* no error possible */

    switch (DeviceType)
    {
        /* Devices for which only one instance is allowed */
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
            if ((*InstancesOfDeviceType_p)->NumberOfInitialisedInstances > 0)
            {
                /* Impossible to have many instances with this device, error */
                ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
            }
            else
            {
                /* Default unique decoder number value : 0 */
                *DecoderNumber_p = 0;
            }
            break;

        /* Devices for which ANY NUMBER of instances is allowed */
        case STVID_DEVICE_TYPE_5100_MPEG :
        case STVID_DEVICE_TYPE_5525_MPEG :
        case STVID_DEVICE_TYPE_5105_MPEG :
        case STVID_DEVICE_TYPE_5301_MPEG :
        case STVID_DEVICE_TYPE_7710_MPEG :
        case STVID_DEVICE_TYPE_7100_MPEG :
        case STVID_DEVICE_TYPE_7100_MPEG4P2 :
        case STVID_DEVICE_TYPE_7100_H264 :
        case STVID_DEVICE_TYPE_7109_MPEG :
        case STVID_DEVICE_TYPE_7109_MPEG4P2 :
        case STVID_DEVICE_TYPE_7109_AVS :
        case STVID_DEVICE_TYPE_7109_H264 :
        case STVID_DEVICE_TYPE_7109_VC1 :
        case STVID_DEVICE_TYPE_ZEUS_MPEG :
        case STVID_DEVICE_TYPE_ZEUS_H264 :
        case STVID_DEVICE_TYPE_ZEUS_VC1 :
        case STVID_DEVICE_TYPE_7109D_MPEG :
        case STVID_DEVICE_TYPE_7200_MPEG :
        case STVID_DEVICE_TYPE_7200_MPEG4P2 :
        case STVID_DEVICE_TYPE_7200_H264 :
        case STVID_DEVICE_TYPE_7200_VC1 :
        case STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7710_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7100_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7109_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7200_DIGITAL_INPUT :

            /* Number of instances is not limited. So look for an un-used instance number, the smallest. */
            /* Now look for the decoder number, but avoid taking a number already used among the (*InstancesOfDeviceType_p)->NumberOfInitialisedInstances used ones */
            *DecoderNumber_p = 0;
            /* Check each decoder number to see if it is already in use */
            for (CheckCount = 0; CheckCount < (*InstancesOfDeviceType_p)->NumberOfInitialisedInstances; CheckCount++)
            {
                Index = 0;
                IndexFound = TRUE;
                do
                {
                    if ((DeviceArray[Index].DeviceData_p != NULL) && /* Valid instance */
                        (DeviceArray[Index].DeviceData_p->DeviceType == DeviceType)) /* Using same device type */
                    {
                        if (*DecoderNumber_p == DeviceArray[Index].DecoderNumber)
                        {
                            /* Decoder umber is already in use, try next */
                            *DecoderNumber_p += 1;
                            /* end the do-while by breaking, i.e. go to next iteration of the for() loop */
                            IndexFound = FALSE;
                            break;
                        }
                    }
                    Index++;
                } while (Index < STVID_MAX_DEVICE);
                if(IndexFound)
                {
                    break;
                }
            } /* for a number of instances of this decoder numbers */
            break;

        case STVID_DEVICE_TYPE_7015_MPEG :
        case STVID_DEVICE_TYPE_7020_MPEG :
            /* Base addresses 0x200, 0x400, 0x600, 0x800, 0xA00 correspond to decoder number 0, 1, 2, 3, 4 */
            RequiresCheckOfDecoderNumberUse = TRUE;
            switch ((((U32) RegistersBaseAddress_p) & 0xFFF))
            {
                case 0x200:
                    *DecoderNumber_p = 0;
                    break;

                case 0x400 :
                    *DecoderNumber_p = 1;
                    break;

                case 0x600 :
                    *DecoderNumber_p = 2;
                    break;

                case 0x800 :
                    *DecoderNumber_p = 3;
                    break;

                case 0xa00 :
                    *DecoderNumber_p = 4;
                    break;

                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            break;

        case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
            /* Base addresses 0x7000, 0x7100, 0x7200 correspond to decoder number 5, 6, 7 */
            RequiresCheckOfDecoderNumberUse = TRUE;
            switch ((((U32) RegistersBaseAddress_p) & 0xFFFF))
            {
                case 0x7000:
                    *DecoderNumber_p = 5;
                    break;

                case 0x7100 :
                    *DecoderNumber_p = 6;
                    break;

                case 0x7200 :
                    *DecoderNumber_p = 7;
                    break;

                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            break;

        case STVID_DEVICE_TYPE_5528_MPEG :
        case STVID_DEVICE_TYPE_STD2000_MPEG :
            /* Base addresses 0x0, 0x300 correspond to decoder number 0, 1 */
            RequiresCheckOfDecoderNumberUse = TRUE;
            switch ((((U32) RegistersBaseAddress_p) & 0xFFF))
            {
                case 0x000:
                    *DecoderNumber_p = 0;
                    break;

                case 0x300 :
                    *DecoderNumber_p = 1;
                    break;

                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            break;

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    /* If no error, may need to check if decoder nuber is not already in use. */
    if ((ErrorCode == ST_NO_ERROR) && (RequiresCheckOfDecoderNumberUse))
    {
        /* Check each decoder number of this device type to see if it is already in use */
        Index = 0;
        do
        {
            if ((DeviceArray[Index].DeviceData_p != NULL) && /* Valid instance */
                (DeviceArray[Index].DeviceData_p->DeviceType == DeviceType) && /* Using same device type */
                (*DecoderNumber_p == DeviceArray[Index].DecoderNumber))

            {
                /* Another initialised video driver is already using the current decoder: say it is already initialised */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Another video driver is already using this decoder at this base address !"));
                ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
                /* end the do-while by breaking */
                break;
            }
            Index++;
        } while (Index < STVID_MAX_DEVICE);
    }

    return(ErrorCode);
} /* End of GetDecoderNumber() function */


/*******************************************************************************
Name        : GetDeviceParameters
Description : Get device parameters
Parameters  : Device type
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t GetDeviceParameters(const STVID_DeviceType_t DeviceType, DeviceParameters_t * const DeviceParameters_p)
{
    /* Some defaults, to be overwritten in switch(DeviceType) below... */
#ifdef ST_inject
    DeviceParameters_p->UsesInject = FALSE;
#endif /* ST_inject */
#ifdef ST_producer
#ifdef ST_multidecodeswcontroller
    DeviceParameters_p->UsesMultiDecodeSWController = FALSE;
#endif /* ST_multidecodeswcontroller */
#endif /* ST_producer */
#ifdef ST_display
    DeviceParameters_p->DisplayHALNumber = 1;
#endif /* ST_display */
    /* Now check device type */
    switch (DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
            DeviceParameters_p->MaxLayers               = 1;
            DeviceParameters_p->MaxViewPortsPerLayer    = 1;
            DeviceParameters_p->DisplayBaseAddressNumber = 1;
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 1;
            break;

        case STVID_DEVICE_TYPE_5100_MPEG :
        case STVID_DEVICE_TYPE_5525_MPEG :
        case STVID_DEVICE_TYPE_5105_MPEG :
        case STVID_DEVICE_TYPE_5301_MPEG :
            DeviceParameters_p->MaxLayers               = 3;
            DeviceParameters_p->MaxViewPortsPerLayer    = 1;
            DeviceParameters_p->DisplayBaseAddressNumber = 1;
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 1;
#ifdef ST_inject
            DeviceParameters_p->UsesInject = TRUE;
#endif /* ST_inject */
            break;

        case STVID_DEVICE_TYPE_7710_MPEG :
        case STVID_DEVICE_TYPE_7100_MPEG :
        case STVID_DEVICE_TYPE_7100_MPEG4P2 :
        case STVID_DEVICE_TYPE_7100_H264 :
            DeviceParameters_p->MaxLayers               = 2;
            DeviceParameters_p->MaxViewPortsPerLayer    = 1;
            DeviceParameters_p->DisplayBaseAddressNumber = 2; /* Take InitParams_p->BaseAddress2_p */
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 1;
#ifdef ST_inject
            DeviceParameters_p->UsesInject = TRUE;
#endif /* ST_inject */
            break;

        case STVID_DEVICE_TYPE_7109_MPEG :
        case STVID_DEVICE_TYPE_7109D_MPEG :
        case STVID_DEVICE_TYPE_7109_MPEG4P2 :
        case STVID_DEVICE_TYPE_7109_AVS :
        case STVID_DEVICE_TYPE_7109_H264 :
        case STVID_DEVICE_TYPE_7109_VC1 :
        case STVID_DEVICE_TYPE_ZEUS_MPEG :
        case STVID_DEVICE_TYPE_ZEUS_H264 :
        case STVID_DEVICE_TYPE_ZEUS_VC1 :
            DeviceParameters_p->MaxLayers               = 2;
            DeviceParameters_p->MaxViewPortsPerLayer    = 1;
            DeviceParameters_p->DisplayBaseAddressNumber = 2; /* Take InitParams_p->BaseAddress2_p */
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 1;
#ifdef ST_inject
            DeviceParameters_p->UsesInject = TRUE;
#endif /* ST_inject */
#ifdef ST_display
            DeviceParameters_p->DisplayHALNumber = 2;
#endif /* ST_display */
            break;

        case STVID_DEVICE_TYPE_7200_MPEG :
        case STVID_DEVICE_TYPE_7200_MPEG4P2 :
        case STVID_DEVICE_TYPE_7200_H264 :
        case STVID_DEVICE_TYPE_7200_VC1 :
            DeviceParameters_p->MaxLayers               = 2;
            DeviceParameters_p->MaxViewPortsPerLayer    = 1;
/* BaseAddress of 2nd and 3rd  display are computed in HAL from the BaseAddress of 1st display */
            DeviceParameters_p->DisplayBaseAddressNumber = 2; /* Take InitParams_p->BaseAddress2_p */
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 1;
#ifdef ST_inject
            DeviceParameters_p->UsesInject = TRUE;
#endif /* ST_inject */
#ifdef ST_display
            DeviceParameters_p->DisplayHALNumber = 1;
#endif /* ST_display */
            break;

        case STVID_DEVICE_TYPE_7710_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7100_DIGITAL_INPUT :
            DeviceParameters_p->MaxLayers               = 2;
            DeviceParameters_p->MaxViewPortsPerLayer    = 1;
            DeviceParameters_p->DisplayBaseAddressNumber = 2; /* Take InitParams_p->BaseAddress2_p */
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 1;
            break;

        case STVID_DEVICE_TYPE_7109_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7200_DIGITAL_INPUT :
            DeviceParameters_p->MaxLayers               = 2;
            DeviceParameters_p->MaxViewPortsPerLayer    = 1;
            DeviceParameters_p->DisplayBaseAddressNumber = 2; /* Take InitParams_p->BaseAddress2_p */
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 1;
#ifdef ST_display
            DeviceParameters_p->DisplayHALNumber = 3;
#endif /* ST_display */
            break;

        case STVID_DEVICE_TYPE_7015_MPEG :
        case STVID_DEVICE_TYPE_7020_MPEG :
        case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
            DeviceParameters_p->MaxLayers               = 2;
            DeviceParameters_p->MaxViewPortsPerLayer    = 4;
            DeviceParameters_p->DisplayBaseAddressNumber = 1;
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 1;
            break;

        case STVID_DEVICE_TYPE_5528_MPEG :
            DeviceParameters_p->MaxLayers               = 1;
            DeviceParameters_p->MaxViewPortsPerLayer    = 1;
            DeviceParameters_p->DisplayBaseAddressNumber = 2; /* Take InitParams_p->BaseAddress2_p */
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 3;
            break;

        case STVID_DEVICE_TYPE_STD2000_MPEG :
            DeviceParameters_p->MaxLayers               = DEVICE_PARAMETERS_MAX_LAYERS;
            DeviceParameters_p->MaxViewPortsPerLayer    = DEVICE_PARAMETERS_MAX_VIEWPORTS_PER_LAYER;
            DeviceParameters_p->DisplayBaseAddressNumber = 3; /* display of STD2000 is not managed by STVID */
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 2;
            break;

        case STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT :
        default :
            /* Max imaginable */
            DeviceParameters_p->MaxLayers               = DEVICE_PARAMETERS_MAX_LAYERS;
            DeviceParameters_p->MaxViewPortsPerLayer    = DEVICE_PARAMETERS_MAX_VIEWPORTS_PER_LAYER;
            DeviceParameters_p->DisplayBaseAddressNumber = 1;
            DeviceParameters_p->CompressedDataInputBaseAddressNumber = 1;
            break;
    }

    return(ST_NO_ERROR);
} /* End of GetDeviceParameters() function */


/*******************************************************************************
Name        : IsErrorCodeMoreImportantThanPrevious
Description : Says whether the new error code is more significant than the previous error code
Parameters  : previous error code + new error code
Assumptions :
Limitations :
Returns     : TRUE if new error code is more important, FALSE otherwise
*******************************************************************************/
static BOOL IsErrorCodeMoreImportantThanPrevious(const ST_ErrorCode_t PreviousErrorCode, const ST_ErrorCode_t NewErrorCode)
{
    if ((PreviousErrorCode == ST_NO_ERROR) ||
        ((PreviousErrorCode == ST_ERROR_BAD_PARAMETER) && (NewErrorCode != ST_ERROR_BAD_PARAMETER) && (NewErrorCode != ST_NO_ERROR)))
    {
        /* Take new error code, it is more important than previous one */
        return(TRUE);
    }

    /* Keep previous error code */
    return(FALSE);
} /* End of IsErrorCodeMoreImportantThanPrevious() function */


/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t Init(VideoDevice_t * const Device_p, const STVID_InitParams_t * const InitParams_p, const ST_DeviceName_t DeviceName)
{
#ifdef ST_avsync
    VIDAVSYNC_Handle_t      AVSyncHandle;
    VIDAVSYNC_InitParams_t  AVSyncInitParams;
#endif /* ST_avsync */
#ifdef ST_inject
    VIDINJ_InitParams_t     InjecterInitParams;
#endif /* ST_inject */
    VIDBUFF_Handle_t        BufferHandle;
    VIDBUFF_InitParams_t    BuffersInitParams;
    VIDBUFF_Infos_t         BufferInfos;
    VIDCOM_InternalProfile_t InitMemoryProfile;
#ifdef ST_producer
    VIDPROD_Handle_t        ProducerHandle;
    VIDPROD_InitParams_t    ProducerInitParams;
    VIDPROD_Infos_t         ProducerInfos;
#ifdef ST_multidecodeswcontroller
    VIDDEC_MultiDecodeInitParams_t  MultiDecodeInitParams;
#endif /* ST_multidecodeswcontroller */
#endif /* ST_producer */

#ifdef ST_ZEUS
#ifdef ST_REMOVE_STLAYER
/* parameters for the Zeus display "glue" which replaces the stvid_display code for ZEUS ONLY.*/
	DISPINT_InitParams_t  DispIntInitParams;
	DISPINT_Handle_t      DispIntHandle;
#endif /* ST_REMOVE_STLAYER */
#endif /*ST_ZEUS */

#ifdef ST_display
    VIDDISP_Handle_t        DisplayHandle;
    VIDDISP_InitParams_t    DisplayInitParams;
#endif /* ST_display */
#ifdef ST_he_disp
    VIDHEDISP_Handle_t      HEDisplayHandle;
    VIDHEDISP_InitParams_t  HEDisplayInitParams;
#endif /* ST_he_disp */
#ifdef ST_crc
    VIDCRC_Handle_t         CRCHandle;
    VIDCRC_InitParams_t     CRCInitParams;
#endif /* ST_crc */
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t         FGTHandle;
    VIDFGT_InitParams_t     FGTInitParams;
#endif /* ST_XVP_ENABLE_FGT */
#ifdef ST_ordqueue
    VIDQUEUE_Handle_t       OrderingQueueHandle;
    VIDQUEUE_InitParams_t   OrderingQueueInitParams;
#endif /* ST_ordqueue */
#ifdef ST_trickmod
    VIDTRICK_InitParams_t   TrickModeInitParams;
    VIDTRICK_Handle_t       TrickModeHandle = NULL; /* assignment to avoid warnings */
#endif /* ST_trickmod */
#ifdef ST_speed
    VIDSPEED_InitParams_t   SpeedInitParams;
    VIDSPEED_Handle_t       SpeedHandle;
#endif /* ST_speed */
#ifdef ST_diginput
#ifndef ST_MasterDigInput
    VIDINP_InitParams_t     DigitalInputInitParams;
    VIDINP_Handle_t         DigitalInputHandle;
#endif /* ST_MasterDigInput */
#endif /*  ST_diginput */
    VideoViewPort_t         *ViewPort_p = NULL;
    U32                     Index, ErrorStepId = 0;
    U8                      DecoderNumber = 0;
    void *                  RegistersBaseAddress_p = NULL;
    void *                  RegistersBaseAddress2_p = NULL;
    void *                  RegistersBaseAddress3_p = NULL;
    DeviceParameters_t      DeviceParameters;
    InstancesOfDeviceType_t * InstancesOfDeviceType_p;
    ErrorStep_t             ErrorStep = ERROR_STEP_NO_ERROR;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
#ifdef ST_SecondInstanceSharingTheSameDecoder
    BOOL                    SpecialCaseOfSecondInstanceSharingTheSameDecoder = FALSE;
#endif /* ST_SecondInstanceSharingTheSameDecoder */

    /* Test initialisation parameters and exit if some are invalid. */
    if (InitParams_p->CPUPartition_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get HW decoder number */
    ErrorCode = GetDecoderNumber(InitParams_p->DeviceType, InitParams_p->BaseAddress_p, &DecoderNumber, &InstancesOfDeviceType_p);
#ifdef ST_SecondInstanceSharingTheSameDecoder
    if ((ErrorCode == ST_ERROR_ALREADY_INITIALIZED) &&
        (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5528_MPEG) &&
        (InstancesOfDeviceType_p->NumberOfInitialisedInstances == 1))
    {
        /* Particular case of STi5528 second instance for still decode : allow a second instance sharing the same decoder... */
        ErrorCode = ST_NO_ERROR;
        SpecialCaseOfSecondInstanceSharingTheSameDecoder = TRUE;
    }
#endif /* ST_SecondInstanceSharingTheSameDecoder */
    /* ? test decoder number not already initialised ? */
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Can't get decoder number !"));
        return(ErrorCode);
    }

    /* Get device parameters, necessary for example for allocations */
    ErrorCode = GetDeviceParameters(InitParams_p->DeviceType, &DeviceParameters); /* No error possible */

    /* Check if first instance, and there are some initialisation requirements for all coming instances */
    if (InstancesOfDeviceType_p->NumberOfInitialisedInstances == 0)
    {
#if defined(ST_inject)
        if (DeviceParameters.UsesInject)
        {
            InjecterInitParams.DeviceType       = InitParams_p->DeviceType;
            InjecterInitParams.CPUPartition_p   = InitParams_p->CPUPartition_p;
#if defined(DVD_SECURED_CHIP)
            InjecterInitParams.FDMASecureTransferMode = FDMA_SECURE_MODE;
#else
            InjecterInitParams.FDMASecureTransferMode = FDMA_NONSECURE_MODE;
#endif /* DVD_SECURED_CHIP */

            ErrorCode = VIDINJ_Init(&InjecterInitParams, &(InstancesOfDeviceType_p->InjecterHandle));
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Injecter failed to initialize !"));
                /* Undo initializations done. */
                ErrorStep = ERROR_STEP_INJECT_INIT;
            }
        }
#endif /* ST_inject */
#ifdef ST_producer
#ifdef ST_multidecodeswcontroller
        if ((ErrorCode == ST_NO_ERROR) && (DeviceParameters.UsesMultiDecodeSWController))
        {
            MultiDecodeInitParams.DeviceType = InitParams_p->DeviceType;
            MultiDecodeInitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
            MultiDecodeInitParams.DeviceBaseAddress_p = InitParams_p->DeviceBaseAddress_p;
            ErrorCode = VIDDEC_MultiDecodeInit(&MultiDecodeInitParams, &(InstancesOfDeviceType_p->MultiDecodeHandle));
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Multi-decode controller failed to initialize !"));
                /* Undo initializations done. */
                ErrorStep = ERROR_STEP_MULTIDEC_INIT;
            }
        }
#endif /* ST_multidecodeswcontroller */
#endif /* ST_producer */
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Allocate dynamic data structure */
        SAFE_MEMORY_ALLOCATE( Device_p->DeviceData_p, VideoDeviceData_t *, InitParams_p->CPUPartition_p, sizeof(VideoDeviceData_t) );
#ifdef ST_SecondInstanceSharingTheSameDecoder
        Device_p->DeviceData_p->SpecialCaseOfSecondInstanceSharingTheSameDecoder = SpecialCaseOfSecondInstanceSharingTheSameDecoder;
#endif /* ST_SecondInstanceSharingTheSameDecoder */
        if (Device_p->DeviceData_p == NULL)
        {
            /* Error of allocation */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to allocate memory for structure !"));
#ifdef ST_producer
#ifdef ST_multidecodeswcontroller
            if ((DeviceParameters.UsesMultiDecodeSWController) &&
                (InstancesOfDeviceType_p->NumberOfInitialisedInstances == 0))
            {
                if (VIDDEC_MultiDecodeTerm(InstancesOfDeviceType_p->MultiDecodeHandle) != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Multi-decode controller failed to terminate !"));
                }
            }
#endif /* ST_multidecodeswcontroller */
#endif /* ST_producer */
#ifdef ST_inject
            if ((DeviceParameters.UsesInject) &&
                (InstancesOfDeviceType_p->NumberOfInitialisedInstances == 0))
            {
                if (VIDINJ_Term(InstancesOfDeviceType_p->InjecterHandle) != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Injecter failed to terminate !"));
                }
            }
#endif
            return(ST_ERROR_NO_MEMORY);
        }

        /* Initialise the allocated structure: most of variables are set-up by init
        (handles, init parameters, etc), so just initialise the rest. */
#ifdef ST_display
        Device_p->DeviceData_p->DisplayHandleIsValid = FALSE;
#endif /* ST_display */
#ifdef ST_he_disp
        Device_p->DeviceData_p->HEDisplayHandleIsValid = FALSE;
#endif /* ST_he_disp */
#ifdef ST_crc
        Device_p->DeviceData_p->CRCHandleIsValid = FALSE;
#endif /* ST_crc */
#ifdef ST_XVP_ENABLE_FGT
        Device_p->DeviceData_p->FGTHandleIsValid = FALSE;
#endif /* ST_XVP_ENABLE_FGT */
#ifdef ST_producer
        Device_p->DeviceData_p->ProducerHandleIsValid = FALSE;
#endif /* ST_producer */
#if defined ST_speed
        Device_p->DeviceData_p->SpeedHandleIsValid = FALSE;
#endif /* ST_speed */
#if defined ST_trickmod
	    Device_p->DeviceData_p->TrickModeHandleIsValid = FALSE;
#endif /* ST_trickmod */
#ifdef ST_diginput
#ifdef ST_MasterDigInput
        Device_p->DeviceData_p->DigitalInputIsValid = FALSE;
#else
        Device_p->DeviceData_p->DigitalInputHandleIsValid = FALSE;
#endif /* ST_MasterDigInput */
#endif /* ST_diginput */
#ifdef ST_ordqueue
        Device_p->DeviceData_p->OrderingQueueHandleIsValid = FALSE;
#endif /* ST_ordqueue */

        Device_p->DeviceData_p->ErrorRecoveryTask.IsRunning = FALSE;
        Device_p->DeviceData_p->ExpectingEventStopped = FALSE;

        Device_p->DeviceData_p->UpdateDisplayQueueMode = VIDPROD_UPDATE_DISPLAY_QUEUE_DEFAULT_MODE;

        /* Calculate base registers address */
        RegistersBaseAddress_p = (void *) (((U8 *) InitParams_p->DeviceBaseAddress_p) + ((U32) InitParams_p->BaseAddress_p));
        RegistersBaseAddress2_p = (void *)(((U8 *) InitParams_p->DeviceBaseAddress_p) + ((U32) InitParams_p->BaseAddress2_p));
        RegistersBaseAddress3_p = (void *)(((U8 *) InitParams_p->DeviceBaseAddress_p) + ((U32) InitParams_p->BaseAddress3_p));

        /* Initialise video modules */
#ifdef ST_XVP_ENABLE_FGT
        FGTInitParams.CPUPartition_p        = InitParams_p->CPUPartition_p;
        FGTInitParams.AvmemPartitionHandle  = InitParams_p->AVMEMPartition;
        strcpy(FGTInitParams.EventsHandlerName,InitParams_p->EvtHandlerName);
        FGTInitParams.DeviceType            = InitParams_p->DeviceType;
        FGTInitParams.DecoderNumber         = DecoderNumber;
        strcpy(FGTInitParams.VideoName, DeviceName);

        ErrorCode = VIDFGT_Init(&FGTInitParams, &FGTHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "FGT failed to initialize !"));
            ErrorStep = ERROR_STEP_DISP_INIT;
        }
        else
        {
            Device_p->DeviceData_p->FGTHandleIsValid = TRUE;
        }
#endif /* ST_XVP_ENABLE_FGT */

        BuffersInitParams.CPUPartition_p            = InitParams_p->CPUPartition_p;
        strcpy(BuffersInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);
        BuffersInitParams.SharedMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;
        BuffersInitParams.AvmemPartitionHandle      = InitParams_p->AVMEMPartition;
        BuffersInitParams.MaxFrameBuffersInProfile  = MAX_NUMBER_OF_FRAME_BUFFERS_PER_DECODER;
        BuffersInitParams.DeviceType                = InitParams_p->DeviceType;
#ifdef ST_XVP_ENABLE_FGT
        BuffersInitParams.FGTHandle                 = FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */

        strcpy(BuffersInitParams.VideoName, DeviceName);
        switch (InitParams_p->DeviceType)
        {
            case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
            case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
            case STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT :
            case STVID_DEVICE_TYPE_7710_DIGITAL_INPUT :
            case STVID_DEVICE_TYPE_7100_DIGITAL_INPUT :
            case STVID_DEVICE_TYPE_7109_DIGITAL_INPUT :
            case STVID_DEVICE_TYPE_7200_DIGITAL_INPUT :
                /* Digital inputs write into frame buffers in 4:2:2 format (16 bits per pixel). */
                BuffersInitParams.FrameBuffersType = VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL;
                break;

            default :
                /* Decoders write into frame buffers in 4:2:0 format (12 bits per pixel).       */
                BuffersInitParams.FrameBuffersType = VIDBUFF_BUFFER_FRAME_12BITS_PER_PIXEL;
                break;
        }
        ErrorCode = VIDBUFF_Init(&BuffersInitParams, &BufferHandle);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Buffer Manager failed to initialize !"));
            /* Undo initializations done. */
            ErrorStep = ERROR_STEP_BUFF_INIT;
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
#ifdef ST_producer
        /* By default, no pending decimation to force. */
        Device_p->DeviceData_p->ForceDecimationFactor.State = FORCE_DISABLED;
        Device_p->DeviceData_p->ForceDecimationFactor.DecimationFactor = STVID_DECIMATION_FACTOR_NONE;

        /* Highest value of Picture ID : set to minimal possible value */
        Device_p->DeviceData_p->HighestExtendedPresentationOrderPictureID.ID = 0x80000000; /* Max negative */
        Device_p->DeviceData_p->HighestExtendedPresentationOrderPictureID.IDExtension = 0;
#endif /* ST_producer */

        /* Force inital memory profile : a standard profile supported by everybody:
           - 4 buffers of SD size PAL, no decimation, no compression */
        InitMemoryProfile.ApiProfile.MaxWidth        = 720;
        InitMemoryProfile.ApiProfile.MaxHeight       = 576;
#ifndef STVID_STVAPI_ARCHITECTURE
        InitMemoryProfile.ApiProfile.NbFrameStore    = 4;
        InitMemoryProfile.NbMainFrameStore  = 4;
        InitMemoryProfile.NbSecondaryFrameStore    = 0;
        InitMemoryProfile.ApiProfile.DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
        InitMemoryProfile.ApplicableDecimationFactor  = STVID_DECIMATION_FACTOR_NONE;
#else
        /* for DTV chips no macroblock reconstruction is needed for B pictures */
        /* allocating a decimated buffer for B pictures (even if they are not */
        /* reconstructed) is the easiest solution avoiding modifying */
        /* viddec too much and having a good memory profile.  */
        InitMemoryProfile.ApiProfile.NbFrameStore    = 2 ;
        InitMemoryProfile.NbMainFrameStore    = 2 ;
        InitMemoryProfile.NbSecondaryFrameStore    = 1 ;
        InitMemoryProfile.ApiProfile.DecimationFactor = STVID_DECIMATION_FACTOR_4;
        InitMemoryProfile.ApplicableDecimationFactor  = STVID_DECIMATION_FACTOR_4;
#endif

        InitMemoryProfile.ApiProfile.CompressionLevel = STVID_COMPRESSION_LEVEL_NONE;

        ErrorCode = VIDBUFF_SetMemoryProfile(BufferHandle, &InitMemoryProfile);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Buffer Manager failed to set memory profile !"));
            /* Undo initializations done. */
            ErrorStep = ERROR_STEP_BUFF_SETMEM;
        }
    }

#ifdef ST_ordqueue
    if (ErrorCode == ST_NO_ERROR)
    {
        OrderingQueueInitParams.CPUPartition_p      = InitParams_p->CPUPartition_p;
        strcpy(OrderingQueueInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);
        OrderingQueueInitParams.BufferManagerHandle = BufferHandle;
        strcpy(OrderingQueueInitParams.VideoName, DeviceName);
        ErrorCode = VIDQUEUE_Init(&OrderingQueueInitParams, &OrderingQueueHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Ordering queue failed to initialize !"));
            /* Undo initializations done. */
            ErrorStep = ERROR_STEP_ORDQUEUE_INIT;
        }
        else
        {
            Device_p->DeviceData_p->OrderingQueueHandleIsValid = TRUE;
        }
    }
#endif /* ST_ordqueue */

    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "One component failed to initialize !"));
    }

#ifdef ST_producer
    else if ((InitParams_p->DeviceType == STVID_DEVICE_TYPE_5508_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5510_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5512_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5514_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5516_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5517_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5518_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5528_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_STD2000_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5100_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5525_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5105_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5301_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7710_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7100_H264) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109D_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109_H264) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109_VC1)  ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_ZEUS_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_ZEUS_H264) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_ZEUS_VC1)  ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7200_H264) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7200_VC1)  ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7015_MPEG) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7020_MPEG) )
    {
#ifdef ST_multidecodeswcontroller
        /* Connect mutli-decode controller for device types needing it */
        if (DeviceParameters.UsesMultiDecodeSWController)
        {
            /* In any case, connect to the multi-instance software controller */
            ErrorCode = VIDDEC_MultiDecodeConnect(InstancesOfDeviceType_p->MultiDecodeHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error: cannot use multi-instance sw controller, undo initialisations done */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module-decode controller failed to connect !"));
                /* Undo initializations done. */
                ErrorStep = ERROR_STEP_MULTIDEC_CONNECT;
            }
        }
#endif /* ST_multidecodeswcontroller */

        if (ErrorCode == ST_NO_ERROR)
        {
            /* Now initialise decoder. */
            ProducerInitParams.CPUPartition_p        = InitParams_p->CPUPartition_p;
            strcpy(ProducerInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);
            ProducerInitParams.BufferManagerHandle   = BufferHandle;
#ifdef ST_XVP_ENABLE_FGT
            ProducerInitParams.FGTHandle             = FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
#if defined(ST_7200)
            memset(ProducerInitParams.MMETransportName, 0, 32);
            if (InitParams_p->BaseAddress_p == (void*)DELTA_0_BASE_ADDRESS)
            {
                strncpy(ProducerInitParams.MMETransportName, MME_VIDEO1_TRANSPORT_NAME, strlen(MME_VIDEO1_TRANSPORT_NAME));
            }
            else if (InitParams_p->BaseAddress_p == (void*)DELTA_1_BASE_ADDRESS)
            {
                strncpy(ProducerInitParams.MMETransportName, MME_VIDEO2_TRANSPORT_NAME, strlen(MME_VIDEO2_TRANSPORT_NAME));
            }
#endif /* #if defined(ST_7200) ... */

#ifdef ST_ordqueue
            ProducerInitParams.OrderingQueueHandle   = OrderingQueueHandle;
#endif /* ST_ordqueue */
            ProducerInitParams.InterruptEvent        = InitParams_p->InterruptEvent;
            ProducerInitParams.SharedMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;
            strcpy(ProducerInitParams.InterruptEventName, InitParams_p->InterruptEventName);
            ProducerInitParams.InstallVideoInterruptHandler = InitParams_p->InstallVideoInterruptHandler;
            ProducerInitParams.InterruptNumber       = InitParams_p->InterruptNumber;
            ProducerInitParams.InterruptLevel        = InitParams_p->InterruptLevel;
#ifdef STVID_STVAPI_ARCHITECTURE
            ProducerInitParams.MB2RInterruptNumber = InitParams_p->MB2RInterruptNumber;
            ProducerInitParams.MB2RInterruptLevel = InitParams_p->MB2RInterruptLevel;
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
            strcpy(ProducerInitParams.VideoName, DeviceName);
            ProducerInitParams.RegistersBaseAddress  = RegistersBaseAddress_p;
            if (DeviceParameters.CompressedDataInputBaseAddressNumber == 3)
            {
                ProducerInitParams.CompressedDataInputBaseAddress_p = RegistersBaseAddress3_p;
            }
            else
            {
                ProducerInitParams.CompressedDataInputBaseAddress_p = RegistersBaseAddress_p;
            }
            ProducerInitParams.BitBufferAllocated    = InitParams_p->BitBufferAllocated;
            ProducerInitParams.BitBufferAddress_p    = InitParams_p->BitBufferAddress_p;
            ProducerInitParams.BitBufferSize         = InitParams_p->BitBufferSize;
            ProducerInitParams.UserDataSize          = InitParams_p->UserDataSize;
            ProducerInitParams.DeviceType            = InitParams_p->DeviceType;
            ProducerInitParams.DecoderNumber         = DecoderNumber;
            strcpy(ProducerInitParams.ClockRecoveryName, InitParams_p->ClockRecoveryName);
            ProducerInitParams.AvmemPartitionHandle  = InitParams_p->AVMEMPartition;
#ifdef ST_inject
            ProducerInitParams.InjecterHandle        = InstancesOfDeviceType_p->InjecterHandle; /* Used only for appropriate devices */
#endif /* ST_inject */
#ifdef ST_multidecodeswcontroller
            ProducerInitParams.MultiDecodeHandle     = MultiDecodeHandle; /* Used only for devices returned by vidcom_IsDeviceTypeUsingMultiDecodeController() */
#endif /* ST_multidecodeswcontroller */
            ProducerInitParams.CodecMode             = STVID_CODEC_MODE_DECODE;
#ifdef ST_SecondInstanceSharingTheSameDecoder
            if (Device_p->DeviceData_p->SpecialCaseOfSecondInstanceSharingTheSameDecoder)
            {
                /* Check each decoder number of this device type to see if it is already in use */
                Index = 0;
                do
                {
                    if ((DeviceArray[Index].DeviceData_p != NULL) && /* Valid instance */
                        (DeviceArray[Index].DeviceData_p->DeviceType == InitParams_p->DeviceType) && /* Using same device type */
                        (DecoderNumber == DeviceArray[Index].DecoderNumber))

                    {
                        /* Another initialised video driver is already using the current decoder: use its decode handle */
                        ProducerHandle = DeviceArray[Index].DeviceData_p->ProducerHandle;
                        VIDPROD_InitSecond(&ProducerInitParams, ProducerHandle);
                        /* BE CARREFUL ! Only call to VIDDEC_DecodeSinglePicture() is allowed with this instance ! */
                        break;
                    }
                    Index++;
                } while (Index < STVID_MAX_DEVICE);
            }
            else
#endif /* ST_SecondInstanceSharingTheSameDecoder */
            {
                /* Normal case */
                ErrorCode = VIDPROD_Init(&ProducerInitParams, &ProducerHandle);
            }
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Producer module failed to initialize !"));
                /* Undo initializations done. */
                ErrorStep = ERROR_STEP_PROD_INIT;
            }
            else
            {
                Device_p->DeviceData_p->ProducerHandleIsValid = TRUE;
                ErrorCode = VIDPROD_GetInfos(ProducerHandle, &ProducerInfos);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Producer module failed to retrieve codec specific infos !"));
                    /* Undo initializations done. Disp_Init step is the nearest state suitable for the current state */
                    ErrorStep = ERROR_STEP_DISP_INIT;
                }
                else
                {
                    BufferInfos.FrameBufferAdditionalDataBytesPerMB = ProducerInfos.FrameBufferAdditionalDataBytesPerMB;
                    ErrorCode = VIDBUFF_SetInfos(BufferHandle, &BufferInfos);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Buffer Manager failed to set codec specific infos !"));
                        /* Undo initializations done. Disp_Init step is the nearest state suitable for the current state */
                        ErrorStep = ERROR_STEP_DISP_INIT;
                    }
                }
            }
        } /* end no error in init of multi-decode module. */
    }
#endif /* ST_producer */
#ifdef ST_diginput
    else if ((InitParams_p->DeviceType == STVID_DEVICE_TYPE_7015_DIGITAL_INPUT) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7100_DIGITAL_INPUT) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT) ||
             (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7710_DIGITAL_INPUT))
    {
#ifdef ST_MasterDigInput
        Device_p->DeviceData_p->DigitalInputIsValid = TRUE;
#else
        DigitalInputInitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
        strcpy(DigitalInputInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);
        strcpy(DigitalInputInitParams.VINName, InitParams_p->VINName);
        strcpy(DigitalInputInitParams.ClockRecoveryName, InitParams_p->ClockRecoveryName);
        strcpy(DigitalInputInitParams.DeviceName, DeviceName);
        DigitalInputInitParams.InterruptEvent = InitParams_p->InterruptEvent;
        strcpy(DigitalInputInitParams.InterruptEventName, InitParams_p->InterruptEventName);
        DigitalInputInitParams.BufferManagerHandle = BufferHandle;
        DigitalInputInitParams.DeviceType = InitParams_p->DeviceType;
        DigitalInputInitParams.RegistersBaseAddress_p = RegistersBaseAddress_p;
        ErrorCode = VIDINP_Init(&DigitalInputInitParams, &DigitalInputHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Digital input module failed to initialize !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;     /* find an other error name */
            /* Undo initializations done. */
            ErrorStep = ERROR_STEP_INP_INIT;
        }
        else
        {
            Device_p->DeviceData_p->DigitalInputHandleIsValid = TRUE;
        }

#endif /* ST_MasterDigInput */
    }
#endif /* ST_diginput */
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "No Input module to initialize !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        /* Undo initializations done. */
        ErrorStep = ERROR_STEP_INP_INIT;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
            /* Init optional modules here */

#ifdef ST_ZEUS
#ifdef ST_REMOVE_STLAYER
/* enable the Zeus display "glue" which replaces the stvid_display code.*/
			DispIntInitParams.CPUPartition_p        = InitParams_p->CPUPartition_p;
            strcpy(DispIntInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);
            strcpy(DispIntInitParams.VideoName, DeviceName);
            ErrorCode = DISPINT_Init(&DispIntInitParams, &DispIntHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Zeus Display Interface failed to initialize !"));
                /* Undo initializations done. */
                ErrorStep = ERROR_STEP_DISP_INIT;
            }
#endif /* ST_REMOVE_STLAYER */
#endif /*ST_ZEUS */

#ifdef ST_crc
            CRCInitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
            strcpy(CRCInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);

            CRCInitParams.DeviceType            = InitParams_p->DeviceType;
            CRCInitParams.DecoderNumber         = DecoderNumber;
            strcpy(CRCInitParams.VideoName, DeviceName);

            ErrorCode = VIDCRC_Init(&CRCInitParams, &CRCHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CRC failed to initialize !"));
                /* Undo initializations done. */
                ErrorStep = ERROR_STEP_DISP_INIT;
            }
            else
            {
                Device_p->DeviceData_p->CRCHandleIsValid = TRUE;
            }
#endif /* ST_crc */

#ifdef ST_display
            DisplayInitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
            DisplayInitParams.AvmemPartitionHandle = InitParams_p->AVMEMPartition;
            strcpy(DisplayInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);
            DisplayInitParams.BufferManagerHandle   = BufferHandle;
#ifdef ST_crc
            DisplayInitParams.CRCHandle  = CRCHandle;
#endif /* ST_crc */

#ifdef ST_XVP_ENABLE_FGT
			DisplayInitParams.FGTHandle  = FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */

            if (DeviceParameters.DisplayBaseAddressNumber == 2)
            {
                /* Display may have a different base address from decode on some chips */
                DisplayInitParams.RegistersBaseAddress[0] = RegistersBaseAddress2_p;
            }
            else
            {
                DisplayInitParams.RegistersBaseAddress[0] = RegistersBaseAddress_p;
            }

            /* If 2 different display HALs are used in the same video instance, */
            /* use BaseAddress2 for first HAL and BaseAddress3 for second HAL.  */
            /* DisplayHALNumber could be equal to 3, for Digital_Input device types, supporting Graphics display hal */
            if (DeviceParameters.DisplayHALNumber >= 2)
            {
                DisplayInitParams.RegistersBaseAddress[1] = RegistersBaseAddress3_p;
                /* For Graphics Display Hal */
                DisplayInitParams.RegistersBaseAddress[2] = RegistersBaseAddress2_p;
            }

            DisplayInitParams.DeviceType            = InitParams_p->DeviceType;
            DisplayInitParams.DecoderNumber         = DecoderNumber;
            DisplayInitParams.MaxDisplayHAL         = DeviceParameters.DisplayHALNumber;
            strcpy(DisplayInitParams.VideoName, DeviceName);

            ErrorCode = VIDDISP_Init(&DisplayInitParams, &DisplayHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Display failed to initialize !"));
                /* Undo initializations done. */
                ErrorStep = ERROR_STEP_DISP_INIT;
            }
            else
            {
                Device_p->DeviceData_p->DisplayHandleIsValid = TRUE;
            }
#endif /* ST_display */
#ifdef ST_he_disp
            HEDisplayInitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
            strcpy(HEDisplayInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);
			      HEDisplayInitParams.BufferManagerHandle   = BufferHandle;
#ifdef ST_crc
			HEDisplayInitParams.CRCHandle    = CRCHandle;
#endif /* ST_crc */
            HEDisplayInitParams.DeviceType            = InitParams_p->DeviceType;
            HEDisplayInitParams.DecoderNumber         = DecoderNumber;
            strcpy(HEDisplayInitParams.VideoName, DeviceName);

            ErrorCode = VIDHEDISP_Init(&HEDisplayInitParams, &HEDisplayHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "HE Display failed to initialize !"));
                /* Undo initializations done. */
                ErrorStep = ERROR_STEP_DISP_INIT;
            }
            else
            {
                Device_p->DeviceData_p->HEDisplayHandleIsValid = TRUE;
            }
#endif /* ST_he_disp */

#ifdef ST_avsync
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Init AVSYNC */
                AVSyncInitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
                strcpy(AVSyncInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);
                AVSyncInitParams.DeviceType     = InitParams_p->DeviceType;
                AVSyncInitParams.SynchroType    = DEFAULT_SYNCHRO_TYPE;
                strcpy(AVSyncInitParams.ClockRecoveryName, InitParams_p->ClockRecoveryName);
#ifdef ST_display
                AVSyncInitParams.DisplayHandle  = DisplayHandle;
#endif /* ST_display */
#ifdef ST_producer
                if (Device_p->DeviceData_p->ProducerHandleIsValid)
                {
                    AVSyncInitParams.IsDecodeActive = TRUE;
                    AVSyncInitParams.DecodeHandle  = ProducerHandle;
                }
                else
                {
                    AVSyncInitParams.IsDecodeActive = FALSE;
                }
#endif /* ST_producer */
                AVSyncInitParams.AVSYNCDriftThreshold = InitParams_p->AVSYNCDriftThreshold;
                strcpy(AVSyncInitParams.VideoName, DeviceName);
                ErrorCode = VIDAVSYNC_Init(&AVSyncInitParams, &AVSyncHandle);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AVSync failed to initialize !"));
                    /* Undo initializations done. */
                    ErrorStep = ERROR_STEP_AVSYNC_INIT;
                }
            }
#endif /* ST_avsync */

#ifdef ST_producer
            if (ErrorCode == ST_NO_ERROR)
            {
#if defined ST_trickmod
                if ((Device_p->DeviceData_p->ProducerHandleIsValid) &&
#ifdef ST_SecondInstanceSharingTheSameDecoder
                    (!(Device_p->DeviceData_p->SpecialCaseOfSecondInstanceSharingTheSameDecoder)) &&
#endif /* ST_SecondInstanceSharingTheSameDecoder */
                    /* Trickmodes only supported for MPEG instances before STi7200 */
                    /* CAUTION : DeviceType's in this group should be the opposite one's as for VIDSPEED module ! */
                    ((InitParams_p->DeviceType != STVID_DEVICE_TYPE_7100_H264) &&
                    (InitParams_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG4P2) &&
                    (InitParams_p->DeviceType != STVID_DEVICE_TYPE_7109_H264) &&
                    (InitParams_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG4P2) &&
                    (InitParams_p->DeviceType != STVID_DEVICE_TYPE_7109_AVS) &&
                    (InitParams_p->DeviceType != STVID_DEVICE_TYPE_7109_VC1) &&
                    (InitParams_p->DeviceType != STVID_DEVICE_TYPE_7109D_MPEG) &&
                    (InitParams_p->DeviceType != STVID_DEVICE_TYPE_7200_MPEG) &&
                    (InitParams_p->DeviceType != STVID_DEVICE_TYPE_7200_H264) &&
                    (InitParams_p->DeviceType != STVID_DEVICE_TYPE_7200_MPEG4P2) &&
                    (InitParams_p->DeviceType != STVID_DEVICE_TYPE_7200_VC1)))
                {
                    strcpy(TrickModeInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);
                    TrickModeInitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
                    TrickModeInitParams.AvmemPartitionHandle = InitParams_p->AVMEMPartition;
                    TrickModeInitParams.DecodeHandle = ProducerHandle;
#ifdef ST_display
                    TrickModeInitParams.DisplayHandle = DisplayHandle;
#endif /* ST_display */
#ifdef ST_smoothbackward
                    TrickModeInitParams.BufferManagerHandle = BufferHandle;
#endif /* ST_smoothbackward */
#ifdef ST_ordqueue
                    TrickModeInitParams.OrderingQueueHandle = OrderingQueueHandle;
#endif /* ST_ordqueue */
                    strcpy(TrickModeInitParams.VideoName, DeviceName);
                    TrickModeInitParams.DecoderNumber = DecoderNumber;
                    if(  (InitParams_p->DeviceType == STVID_DEVICE_TYPE_5100_MPEG)
                        ||(InitParams_p->DeviceType == STVID_DEVICE_TYPE_5525_MPEG)
                        ||(InitParams_p->DeviceType == STVID_DEVICE_TYPE_5105_MPEG)
                        ||(InitParams_p->DeviceType == STVID_DEVICE_TYPE_5301_MPEG))
                    {
                        TrickModeInitParams.DisplayDeviceAdditionnalFieldLatency = 4;
                    }
                    else
                    {
                        TrickModeInitParams.DisplayDeviceAdditionnalFieldLatency = 0;
                    }
                    ErrorCode = VIDTRICK_Init(&TrickModeInitParams, &TrickModeHandle);
                    /* InitTrickMode */
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Trick Mode failed to initialize !"));
                        /* Undo initializations done. */
                        ErrorStep = ERROR_STEP_TRICK_SPEED_INIT;
                    }
                    else
                    {
                        Device_p->DeviceData_p->TrickModeHandleIsValid = TRUE;
                    }
                } /* right device types */
#endif /* ST_trickmod */
#if defined ST_speed
                if ((Device_p->DeviceData_p->ProducerHandleIsValid) &&
                    /* Trickmodes only supported for instances not MPEG, or after STi7200 */
                    /* CAUTION : DeviceType's in this group should be the opposite one's as for VIDTRICK module ! */
                    ((InitParams_p->DeviceType == STVID_DEVICE_TYPE_7100_H264) ||
                    (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
                    (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109_H264) ||
                    (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
                    (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
                    (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109_VC1) ||
                    (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7109D_MPEG) ||
                    (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7200_H264) ||
                    (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2) ||
                    (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG) ||
                    (InitParams_p->DeviceType == STVID_DEVICE_TYPE_7200_VC1)))
				{
                    strcpy(SpeedInitParams.EventsHandlerName, InitParams_p->EvtHandlerName);
                    SpeedInitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
                    SpeedInitParams.AvmemPartitionHandle = InitParams_p->AVMEMPartition;
                    SpeedInitParams.ProducerHandle = ProducerHandle;
#ifdef ST_display
                    SpeedInitParams.DisplayHandle = DisplayHandle;
#endif /* ST_display */
#ifdef ST_ordqueue
                    SpeedInitParams.OrderingQueueHandle = OrderingQueueHandle;
#endif /* ST_ordqueue */
                    strcpy(SpeedInitParams.VideoName, DeviceName);
                    SpeedInitParams.DecoderNumber = DecoderNumber;
                    SpeedInitParams.DisplayDeviceAdditionnalFieldLatency = 0;
                    ErrorCode = VIDSPEED_Init(&SpeedInitParams, &SpeedHandle);
                    /* InitSpeed */
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Speed failed to initialize !"));
                        /* Undo initializations done. */
                        ErrorStep = ERROR_STEP_TRICK_SPEED_INIT;
                    }
                    else
                    {
                        Device_p->DeviceData_p->SpeedHandleIsValid = TRUE;
                    }
                } /* right device types */
#endif /* ST_speed*/
            }
#endif /* ST_producer */

            /* Test if all optional modules init succeed */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = RegisterEvents(InitParams_p->DeviceType, InitParams_p->EvtHandlerName, Device_p, DeviceName);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* Undo initializations done. */
                    ErrorStep = ERROR_STEP_REGISTER_EVENTS;
                }
                else
                {
                    Device_p->DeviceData_p->CPUPartition_p = InitParams_p->CPUPartition_p;
                    Device_p->DeviceData_p->AVMEMPartitionHandle = InitParams_p->AVMEMPartition;
/*                    Device_p->DeviceData_p->SharedMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;*/ /* not used */
                    Device_p->DeviceData_p->DeviceType      = InitParams_p->DeviceType;
#ifdef ST_avsync
                    Device_p->DeviceData_p->AVSyncHandle    = AVSyncHandle;
#endif /* ST_avsync */
#ifdef ST_producer
                    Device_p->DeviceData_p->ProducerHandle   = ProducerHandle;
#ifdef ST_multidecodeswcontroller
                    Device_p->DeviceData_p->MultiDecodeHandle = InstancesOfDeviceType_p->MultiDecodeHandle;
#endif /* ST_multidecodeswcontroller */
#endif /* ST_producer */
#ifdef ST_display
                    Device_p->DeviceData_p->DisplayHandle   = DisplayHandle;
#endif /* ST_display */
#ifdef ST_he_disp
                    Device_p->DeviceData_p->HEDisplayHandle   = HEDisplayHandle;
#endif /* ST_he_disp */
#ifdef ST_crc
                    Device_p->DeviceData_p->CRCHandle   = CRCHandle;
#endif /* ST_crc */
#ifdef ST_XVP_ENABLE_FGT
                    Device_p->DeviceData_p->FGTHandle   = FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
#ifdef ST_ordqueue
                    Device_p->DeviceData_p->OrderingQueueHandle   = OrderingQueueHandle;
#endif /* ST_ordqueue */
                    Device_p->DeviceData_p->BuffersHandle   = BufferHandle;
#ifdef ST_diginput
#ifndef ST_MasterDigInput
                    Device_p->DeviceData_p->DigitalInputHandle = DigitalInputHandle;
#endif /* ST_MasterDigInput */
#endif /* ST_diginput */
#if defined ST_trickmod
                    Device_p->DeviceData_p->TrickModeHandle = TrickModeHandle;
#endif /* ST_trickmod */
#if defined ST_speed
                    Device_p->DeviceData_p->SpeedHandle = SpeedHandle;
#endif /*ST_speed */
					Device_p->DeviceData_p->DeviceBaseAddress_p = InitParams_p->DeviceBaseAddress_p;
                    Device_p->DeviceData_p->NumberOfOpenSinceInit = 0;
                    Device_p->DeviceData_p->StopMode = STVID_STOP_NOW;
                    /* A default state for start parameters */
                    Device_p->DeviceData_p->StartParams.RealTime = FALSE;
                    Device_p->DeviceData_p->StartParams.UpdateDisplay = TRUE;
                    Device_p->DeviceData_p->StartParams.BrdCstProfile = STVID_BROADCAST_DVB;
                    Device_p->DeviceData_p->StartParams.StreamType = STVID_STREAM_TYPE_ES;
                    Device_p->DeviceData_p->StartParams.StreamID = STVID_IGNORE_ID;
                    Device_p->DeviceData_p->StartParams.DecodeOnce = FALSE;
#ifdef STVID_DEBUG_GET_STATISTICS
                    Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForFirstPictureDetected = 0;
                    Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForFirstPictureDecoded = 0;
                    Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForNextPicture = 0;
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
					/* Start Params related */
					Device_p->DeviceData_p->VideoAlreadyStarted = FALSE;
					memset (&Device_p->DeviceData_p->LastStartParams, 0x0, sizeof(STVID_StartParams_t)); /* fill in data with 0x0 by default */

					/* Freeze Params related */
					Device_p->DeviceData_p->VideoAlreadyFreezed = FALSE;
					memset (&Device_p->DeviceData_p->LastFreezeParams, 0x0, sizeof(STVID_Freeze_t)); /* fill in data with 0x0 by default */
#endif /* STVID_DEBUG_GET_STATUS */

                    Device_p->DeviceData_p->VTGFrameRate = 0;

                    /* Get AV mem mapping */
                    STAVMEM_GetSharedMemoryVirtualMapping(&(Device_p->DeviceData_p->VirtualMapping));

                    /* Initialise video in stop state */
                    Device_p->DeviceData_p->Status = VIDEO_STATUS_STOPPED;

                    /* Init semaphore for display params protection */
                    Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p = STOS_SemaphoreCreatePriority(InitParams_p->CPUPartition_p,1);

                    /* Ensure nobody else accesses display params */
                    semaphore_wait(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

                    /* Initialise display params to the more restrictive by default, so that also trickmode structure
                    is initialised so, and if there is no interaction from VIDTRICK, the mix of the 2 sent to VIDDISP in
                    CheckDisplayParamsChanges will be the right one. */
#ifdef ST_display
                    Device_p->DeviceData_p->DisplayParamsForAPI.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
#ifdef ST_producer
                    if (Device_p->DeviceData_p->ProducerHandleIsValid)
                    {
                        /* For decode only default is CARE */
                        Device_p->DeviceData_p->DisplayParamsForAPI.DisplayStart = VIDDISP_DISPLAY_START_CARE_MPEG_FRAME;
                    }
#endif /* ST_producer */
#ifdef ST_diginput
#ifdef ST_MasterDigInput
                    if (Device_p->DeviceData_p->DigitalInputIsValid)
#else
                    if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
#endif /* ST_MasterDigInput */
                    {
                        Device_p->DeviceData_p->DisplayParamsForAPI.DisplayStart
                                = VIDDISP_DISPLAY_START_AS_AVAILABLE;
                    }
#endif /* ST_diginput */

                    /* The 2 params below should never change in API, only trickmode module will change them according to its needs */
                    Device_p->DeviceData_p->DisplayParamsForAPI.DisplayForwardNotBackward = TRUE;
                    Device_p->DeviceData_p->DisplayParamsForAPI.DisplayWithCareOfFieldsPolarity = TRUE;
#if defined ST_speed || defined ST_trickmod
                    Device_p->DeviceData_p->DisplayParamsForTrickMode = Device_p->DeviceData_p->DisplayParamsForAPI;
#endif /* ST_speed || ST_trickmod */
#endif /* ST_display */
                    /* Ensure nobody else accesses display params */
                    semaphore_signal(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

                    /* Init error recovery: FULL by default if decode, NONE otherwise */
#ifdef ST_producer
                    if (Device_p->DeviceData_p->ProducerHandleIsValid)
                    {
                        /* init error recovery needs event handler and decode handle ! */
                        ErrorCode = stvid_InitErrorRecovery(Device_p, STVID_ERROR_RECOVERY_FULL, DeviceName, DecoderNumber);
                    }
                    else
#endif /* ST_producer */
                     {
                         /* init error recovery needs event handler and decode handle ! */
                         ErrorCode = stvid_InitErrorRecovery(Device_p, STVID_ERROR_RECOVERY_NONE, DeviceName, DecoderNumber);
                     }
                     if (ErrorCode != ST_NO_ERROR)
                     {
                         STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery could not be setup !"));
                         /* Undo initializations done. */
                         ErrorStep = ERROR_STEP_ERROR_RECOVERY_INIT;
                     }
                     else
                     {
                         /* Allocate Layer structures */
                         Device_p->DeviceData_p->MaxLayerNumber = DeviceParameters.MaxLayers;
                         SAFE_MEMORY_ALLOCATE(Device_p->DeviceData_p->LayerProperties_p, VideoLayer_t*,
                                              InitParams_p->CPUPartition_p,
                                              Device_p->DeviceData_p->MaxLayerNumber * sizeof(VideoLayer_t));
                         if (Device_p->DeviceData_p->LayerProperties_p == NULL)
                         {
                             STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to allocate memory !"));
                             ErrorCode = ST_ERROR_NO_MEMORY;
                             /* Undo initializations done. */
                             ErrorStep = ERROR_STEP_MEMORY_ALLOCATE_LAYER_PROPERTIES;
                         }
                         else
                         {
                             /* Initialise layer structures */
                             for (Index = 0; Index < Device_p->DeviceData_p->MaxLayerNumber; Index++)
                             {
                                 Device_p->DeviceData_p->LayerProperties_p[Index].IsOpened = FALSE;
                                 Device_p->DeviceData_p->LayerProperties_p[Index].NumberVideoViewportsAssociated = FALSE;
                                 Device_p->DeviceData_p->LayerProperties_p[Index].LayerName[0] = '\0';
                             }

                             /* Allocate Viewport structures */
                             Device_p->DeviceData_p->MaxViewportNumber = DeviceParameters.MaxViewPortsPerLayer * Device_p->DeviceData_p->MaxLayerNumber;
                             Device_p->DeviceData_p->MaxViewportNumberPerLayer = DeviceParameters.MaxViewPortsPerLayer;
                             SAFE_MEMORY_ALLOCATE(Device_p->DeviceData_p->ViewportProperties_p, VideoViewPort_t*,
                                                  InitParams_p->CPUPartition_p,
                                                  Device_p->DeviceData_p->MaxViewportNumber * sizeof(VideoViewPort_t));
                             if (Device_p->DeviceData_p->ViewportProperties_p == NULL)
                             {
                                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to allocate memory !"));
                                 ErrorCode = ST_ERROR_NO_MEMORY;
                                 /* Undo initializations done. */
                                 ErrorStep = ERROR_STEP_MEMORY_ALLOCATE_VIEWPORT_PROPERTIES;
                             }
                             else
                             {
                                 /* Initialise Viewport structures and allocate further */
                                 ViewPort_p = (VideoViewPort_t *) (Device_p->DeviceData_p->ViewportProperties_p);
                                 for (Index = 0; Index < Device_p->DeviceData_p->MaxViewportNumber; Index++)
                                 {
                                     ViewPort_p->Identity.ViewPortOpenedAndValidityCheck = 0; /* not VIDAPI_VALID_VIEWPORT_HANDLE */

                                     /* ViewPort_p->Identity.LayerName[0] = '\0'; */
                                     ViewPort_p->Identity.Layer_p = NULL;
                                     ViewPort_p->Identity.Device_p = NULL;
                                     SAFE_MEMORY_ALLOCATE(ViewPort_p->Params.VideoViewportParameters.Source_p,
                                                          STLAYER_ViewPortSource_t *,
                                                          InitParams_p->CPUPartition_p,
                                                          sizeof(STLAYER_ViewPortSource_t));
                                     if (ViewPort_p->Params.VideoViewportParameters.Source_p == NULL)
                                     {
                                         STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to allocate memory !"));
                                         ErrorCode = ST_ERROR_NO_MEMORY;
                                         /* Undo initializations done. */
                                         ErrorStep = ERROR_STEP_MEMORY_ALLOCATE_SOURCE;
                                         ErrorStepId = Index;
                                         /* Quit 'for' loop */
                                         break;
                                     }
                                     ViewPort_p++;
                                 }

                                 if (ErrorCode == ST_NO_ERROR)
                                 {
                                     ViewPort_p = (VideoViewPort_t *) (Device_p->DeviceData_p->ViewportProperties_p);
                                     for (Index = 0; Index < Device_p->DeviceData_p->MaxViewportNumber; Index++)
                                     {
                                         SAFE_MEMORY_ALLOCATE(ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p,
                                                             STLAYER_StreamingVideo_t *,
                                                             InitParams_p->CPUPartition_p,
                                                             MAX_2VALUES( sizeof(STLAYER_StreamingVideo_t), sizeof(STGXOBJ_Bitmap_t)));
                                         if (ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p == NULL)
                                         {
                                             STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to allocate memory !"));
                                             ErrorCode = ST_ERROR_NO_MEMORY;
                                             /* Undo initializations done. */
                                             ErrorStep = ERROR_STEP_MEMORY_ALLOCATE_VIDEO_STREAM;
                                             ErrorStepId = Index;
                                             /* Quit 'for' loop */
                                             break;
                                         }
                                         ViewPort_p++;
                                     }

                                     if (ErrorCode == ST_NO_ERROR)
                                     {
                                         /* Only when we are very sure of success: we consider that the decoder number is in use */
                                         Device_p->DecoderNumber = DecoderNumber;

                                         /* Init semaphore for layers/viewports protection */
                                         Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p =
                                                STOS_SemaphoreCreatePriority(InitParams_p->CPUPartition_p,1);
                                        /* Init semaphore for VTG name or params protection */
                                        Device_p->DeviceData_p->VTGParamsChangeLockingSemaphore_p =
                                        STOS_SemaphoreCreatePriority(InitParams_p->CPUPartition_p,1);
                                     }
                                 }
                             }
                         }
                     } /* end error recovery initialised */
                }  /* End register events successed */
            }   /* End all optional modules init succeed */
    } /* End buffer manager init successed */

    /* Error Recovery Mechanism: undo initialisations done if failure in the middle */
    switch (ErrorStep)
    {
        case ERROR_STEP_NO_ERROR :
            /* No error: go on, nothing to undo */
            /* Aknowledge that there is now one more instance of this device type */
            InstancesOfDeviceType_p->NumberOfInitialisedInstances += 1;
            break;

            /* No break between errors to do all that follows in case of one error. */
        case ERROR_STEP_MEMORY_ALLOCATE_VIDEO_STREAM :
            ViewPort_p = (VideoViewPort_t *) (Device_p->DeviceData_p->ViewportProperties_p);
            for (Index = 0; Index < ErrorStepId; Index++)
            {
                SAFE_MEMORY_DEALLOCATE(ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p,
                                       Device_p->DeviceData_p->CPUPartition_p,
                                       MAX_2VALUES( sizeof(STLAYER_StreamingVideo_t), sizeof(STGXOBJ_Bitmap_t)));
                ViewPort_p++;
            }
            ErrorStepId = Device_p->DeviceData_p->MaxViewportNumber;
        case ERROR_STEP_MEMORY_ALLOCATE_SOURCE :
            ViewPort_p = (VideoViewPort_t *) (Device_p->DeviceData_p->ViewportProperties_p);
            for (Index = 0; Index < ErrorStepId; Index++)
            {
                SAFE_MEMORY_DEALLOCATE(ViewPort_p->Params.VideoViewportParameters.Source_p,
                                       Device_p->DeviceData_p->CPUPartition_p,
                                       sizeof(STLAYER_ViewPortSource_t));
                ViewPort_p++;
            }
            SAFE_MEMORY_DEALLOCATE(Device_p->DeviceData_p->ViewportProperties_p,
                                   Device_p->DeviceData_p->CPUPartition_p,
                                   Device_p->DeviceData_p->MaxViewportNumber * sizeof(VideoViewPort_t));
        case ERROR_STEP_MEMORY_ALLOCATE_VIEWPORT_PROPERTIES :
            SAFE_MEMORY_DEALLOCATE(Device_p->DeviceData_p->LayerProperties_p,
                                   Device_p->DeviceData_p->CPUPartition_p,
                                   Device_p->DeviceData_p->MaxLayerNumber * sizeof(VideoLayer_t));
        case ERROR_STEP_MEMORY_ALLOCATE_LAYER_PROPERTIES :
            if (stvid_TermErrorRecovery(Device_p) != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery termination failed !"));
            }
        case ERROR_STEP_ERROR_RECOVERY_INIT :
        case ERROR_STEP_REGISTER_EVENTS :
            if (STEVT_Close(Device_p->DeviceData_p->EventsHandle) != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while closing event handler"));
            }
        case ERROR_STEP_LAYER_INIT :
#if defined ST_speed
            if (Device_p->DeviceData_p->SpeedHandleIsValid)
            {
                if (VIDSPEED_Term(SpeedHandle) != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Speed failed to terminate !"));
                }
                Device_p->DeviceData_p->SpeedHandleIsValid = FALSE;
            }
#endif /* ST_speed */
#if defined ST_trickmod
            if (Device_p->DeviceData_p->TrickModeHandleIsValid)
            {
                if (VIDTRICK_Term(TrickModeHandle) != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TrickMode failed to terminate !"));
                }
                Device_p->DeviceData_p->TrickModeHandleIsValid = FALSE;
            }
#endif /* ST_trickmod */
        case ERROR_STEP_TRICK_SPEED_INIT :
#ifdef ST_avsync
            if (VIDAVSYNC_Term(AVSyncHandle) != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AVSync failed to terminate !"));
            }
#endif /* ST_avsync */
        case ERROR_STEP_AVSYNC_INIT :
#ifdef ST_display
            if (VIDDISP_Term(DisplayHandle) != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Display failed to terminate !"));
            }
#endif /* ST_display */
        case ERROR_STEP_DISP_INIT :
#ifdef ST_producer
            if (Device_p->DeviceData_p->ProducerHandleIsValid)
            {
#ifdef ST_SecondInstanceSharingTheSameDecoder
                if (!(Device_p->DeviceData_p->SpecialCaseOfSecondInstanceSharingTheSameDecoder))
                {
#endif /* ST_SecondInstanceSharingTheSameDecoder */
                    if (VIDPROD_Term(ProducerHandle) != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Decoder failed to terminate !"));
                    }
#ifdef ST_SecondInstanceSharingTheSameDecoder
                }
#endif /* ST_SecondInstanceSharingTheSameDecoder */
            }
#endif /* ST_producer */
#ifdef ST_diginput
#ifdef ST_MasterDigInput
            if (Device_p->DeviceData_p->DigitalInputIsValid)
            {
                Device_p->DeviceData_p->DigitalInputIsValid = FALSE;
            }
#else
            if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
            {
                if (VIDINP_Term(DigitalInputHandle) != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Digital Input failed to terminate !"));
                }
                Device_p->DeviceData_p->DigitalInputHandleIsValid = FALSE;
            }
#endif /* ST_MasterDigInput */
#endif /* ST_diginput */
        case ERROR_STEP_PROD_INIT :
#ifdef ST_producer
#ifdef ST_multidecodeswcontroller
            /* disconnect from multi-decode sw controller */
            if (DeviceParameters.UsesMultiDecodeSWController)
            {
                /* Disconnect instance from multi-decode software controller */
                if (VIDDEC_MultiDecodeDisconnect(MultiDecodeHandle) != ST_NO_ERROR)
                {
                    /* Error: cannot disconnect from multi-instance sw controller */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module-decode controller failed to disconnect !"));
                }
            }
#endif /* ST_multidecodeswcontroller */
#endif /* ST_producer */
        case ERROR_STEP_INP_INIT :
        case ERROR_STEP_MULTIDEC_CONNECT :
#ifdef ST_ordqueue
            if (VIDQUEUE_Term(OrderingQueueHandle) != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Ordering Queue Manager failed to terminate !"));
            }
#endif /* ST_ordqueue */
        case ERROR_STEP_ORDQUEUE_INIT :
        case ERROR_STEP_BUFF_SETMEM :
            if (VIDBUFF_Term(BufferHandle) != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Buffer Manager failed to terminate !"));
            }
        case ERROR_STEP_BUFF_INIT :
            /* Security: for all error cases, consider decoder number not in use... */
            Device_p->DecoderNumber = STVID_INVALID_DECODER_NUMBER;
            /* De-allocate dynamic data structure */
            SAFE_MEMORY_DEALLOCATE(Device_p->DeviceData_p, InitParams_p->CPUPartition_p, sizeof(VideoDeviceData_t));
            Device_p->DeviceData_p = NULL;
            /* Undo multi-decode controller init if done. */
#ifdef ST_producer
#ifdef ST_multidecodeswcontroller
            if ((DeviceParameters.UsesMultiDecodeSWController) &&
                (InstancesOfDeviceType_p->NumberOfInitialisedInstances == 0))
            {
                if (VIDDEC_MultiDecodeTerm(InstancesOfDeviceType_p->MultiDecodeHandle) != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Multi-decode controller failed to terminate !"));
                }
            }
#endif /* ST_multidecodeswcontroller */
#endif /* ST_producer */
        case ERROR_STEP_MULTIDEC_INIT :
#ifdef ST_inject
            if ((DeviceParameters.UsesInject) &&
                (InstancesOfDeviceType_p->NumberOfInitialisedInstances == 0))
            {
                if (VIDINJ_Term(InstancesOfDeviceType_p->InjecterHandle) != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Injecter failed to terminate !"));
                }
            }
#endif /* ST_inject */
        case ERROR_STEP_INJECT_INIT :
        default :
            break;
    }

    return(ErrorCode);
} /* End of Init() function */


/*******************************************************************************
Name        : Open
Description : API specific actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Open function
*******************************************************************************/
#define MAX_S32_POSITIVE_VALUE     0x7FFFFFFF

static ST_ErrorCode_t Open(VideoUnit_t * const Unit_p, const STVID_OpenParams_t * const OpenParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#ifdef ST_avsync
    S32 PositiveSyncDelay;
#endif
    if (Unit_p->Device_p->DeviceData_p == NULL)
    {
        /* Error case ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef ST_avsync
    /* Take SyncDelay parameter into account only the first time ! */
    if (Unit_p->Device_p->DeviceData_p->NumberOfOpenSinceInit == 0)
    {
        /* OpenParams_p->SyncDelay is declared as an unsigned int but is passed to a function that accepts S32 for
           parameter. As values > 0x7FFFFFFF (highest possible positive value) are unlikely but still possible,
           limit the value passed to VIDAVSYNC_SetSynchronizationDelay at MAX_S32_POSITIVE_VALUE so that it is
           not considered as a negative number.
           SyncDelay cannot be modified directly as it is passed as a const */
        PositiveSyncDelay = (OpenParams_p->SyncDelay > MAX_S32_POSITIVE_VALUE)? MAX_S32_POSITIVE_VALUE : OpenParams_p->SyncDelay;
        ErrorCode = VIDAVSYNC_SetSynchronizationDelay(Unit_p->Device_p->DeviceData_p->AVSyncHandle, (S32) PositiveSyncDelay);

        /* Cannot return ST_ERROR_INVALID_HANDLE because STVID_Open assigns the handle, but we can still pass back
           error as ST_ERROR_BAD_PARAMETER */
        if(ErrorCode == ST_ERROR_INVALID_HANDLE)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
#endif /* ST_avsync */

    /* If success of Open(): increment number of open handles */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Increment number of open handles */
        Unit_p->Device_p->DeviceData_p->NumberOfOpenSinceInit ++;
    }

    return(ErrorCode);
} /* End of Open() function */


/*******************************************************************************
Name        : Close
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Close function
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t Close(VideoUnit_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (Unit_p->Device_p->DeviceData_p == NULL)
    {
        /* Error case ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

#if 0   /* no need..., would allow all close followed by open to set again SyncDelay, but
           SyncDelay is taken into account only for the first open handle of a video decoder, i.e. at the first call
           to STVID_Open() after initialization, or at the first call to STVID_Open() after all open handles are closed
           with STVID_Close(). It is not taken into account for subsequent calls to STVID_Open() (multiple open handles).*/

    /* If success of Close(): decrement number of open handles. */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Decrement number of open handles */
        Unit_p->Device_p->DeviceData_p->NumberOfOpenSinceInit --;
    } /* end close success */
#endif

    return(ErrorCode);
} /* End of Close() function */


/*******************************************************************************
Name        : Term
Description : API specific terminations: should go-on as much as possible even if failures
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(VideoDevice_t * const Device_p, const STVID_TermParams_t * const TermParams_p)
{
    VideoViewPort_t *       ViewPort_p;
    U32                     Index;
#ifdef ST_composition
#ifndef STVID_STVAPI_ARCHITECTURE
    U32                     k;
    BOOL                    Failed;
#endif /* ifdef STVID_STVAPI_ARCHITECTURE */
#endif /* ST_composition*/
    STVID_Freeze_t          Freeze;
#ifdef ST_multidecodeswcontroller
    U8                      NumberOfInitOfSameDeviceTypeOnSameDevice;
#endif /* ST_multidecodeswcontroller */
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR, FinalErrorCode = ST_NO_ERROR;
/*    U8                      NumberOfViewportInLayer = 0;*/
#if defined ST_inject || (defined ST_producer && defined ST_multidecodeswcontroller)
    DeviceParameters_t      DeviceParameters;
#endif /*defined ST_inject || (defined ST_producer && defined ST_multidecodeswcontroller) */
    InstancesOfDeviceType_t * InstancesOfDeviceType_p;

    if (Device_p->DeviceData_p == NULL)
    {
        /* Error case ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    UNUSED_PARAMETER(TermParams_p);

    stvid_EnterCriticalSection(Device_p);

    ErrorCode = FindInstancesOfSameDeviceType(Device_p->DeviceData_p->DeviceType, &InstancesOfDeviceType_p); /* no error possible */
#if defined ST_inject || (defined ST_producer && defined ST_multidecodeswcontroller)
    ErrorCode = GetDeviceParameters(Device_p->DeviceData_p->DeviceType, &DeviceParameters); /* No error possible */
#endif /*defined ST_inject || (defined ST_producer && defined ST_multidecodeswcontroller) */

    /* Before terminating: stop the decoder (if not already the case). */
    if (GET_VIDEO_STATUS(Device_p) != VIDEO_STATUS_STOPPED)
    {
        /* Decoder not already stopped: stop it to be clean. */
        Freeze.Field = STVID_FREEZE_FIELD_TOP;
        Freeze.Mode = STVID_FREEZE_MODE_NONE;
        /* Stop to de-allocate frame buffers right now. */
        ErrorCode = stvid_ActionStop(Device_p, STVID_STOP_NOW, &Freeze);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error stopping video at termination !"));
            /* This should not cause close to fail ? */
            ErrorCode = ST_NO_ERROR;
        }
    } /* end decoder not stopped */

    /* Ensure nobody else accesses layers/viewports. */
    semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    /* Ensure nobody else accesses VTG name or params */
    semaphore_wait(Device_p->DeviceData_p->VTGParamsChangeLockingSemaphore_p);
    /* Ensure nobody else accesses display params */
    semaphore_wait(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

#ifdef ST_composition
#ifndef STVID_STVAPI_ARCHITECTURE
    /* We are now terminating: close all the opened instances of Video ViewPort. */
    Failed = FALSE;
    k = 0;
    while (k<Device_p->DeviceData_p->MaxViewportNumber)
    {
        ViewPort_p = &Device_p->DeviceData_p->ViewportProperties_p[k];
        if(ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE)
        {
            ErrorCode = stvid_ActionDisableOutputWindow(ViewPort_p);
            if (ErrorCode != ST_NO_ERROR)
            {
                Failed = TRUE;
            }
            ErrorCode = stvid_ActionCloseViewPort(ViewPort_p, NULL);
            if (ErrorCode != ST_NO_ERROR)
            {
                Failed = TRUE;
            }
        }
        if (Failed)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Viewports close failed !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
            {
                FinalErrorCode = ErrorCode;
            }
        }
        k++;
    }
#endif /* ifdef STVID_STVAPI_ARCHITECTURE */
#endif /* ST_composition */
    /* Remove protection to terminate modules which delete some tasks: indeed, if
       we are inside a protected area, we may go to a blocking state: the task may
       be inside a STEVT_Notify(), inside our callback which can be also protected,
       so waiting on the protection semaphore. In that case, we will wait for ever !*/
    stvid_LeaveCriticalSection(Device_p);

    /* Term video modules : the strategy is to term all the modules. If some fail, continue with the others. */
    /* Always terminate as much as possible ! */
#if defined ST_speed
    /* Termination of the trick mode module. */
    if (Device_p->DeviceData_p->SpeedHandleIsValid)
    {
        ErrorCode = VIDSPEED_Term(Device_p->DeviceData_p->SpeedHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Trick modes module termination failed !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
            {
                FinalErrorCode = ErrorCode;
            }
        }
        Device_p->DeviceData_p->SpeedHandleIsValid = FALSE;
    }
#endif /* ST_speed */
#if defined ST_trickmod
    /* Termination of the trick mode module. */
    if (Device_p->DeviceData_p->TrickModeHandleIsValid)
    {
        ErrorCode = VIDTRICK_Term(Device_p->DeviceData_p->TrickModeHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Trick modes module termination failed !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
            {
                FinalErrorCode = ErrorCode;
            }
        }
        Device_p->DeviceData_p->TrickModeHandleIsValid = FALSE;
    }
#endif /* ST_trickmod */

    /* Close event handler. This also un-registers and un-subscribes all events. */
    ErrorCode = STEVT_Close(Device_p->DeviceData_p->EventsHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error while closing event handler"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
        {
            FinalErrorCode = ErrorCode;
        }
    }

    /* Terminate error recovery task. */
    ErrorCode = stvid_TermErrorRecovery(Device_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery termination failed !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
        {
            FinalErrorCode = ErrorCode;
        }
    }

#ifdef ST_avsync
    /* Termination of the AVSync component. */
    ErrorCode = VIDAVSYNC_Term(Device_p->DeviceData_p->AVSyncHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AVSync termination failed !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
        {
            FinalErrorCode = ErrorCode;
        }
    }
#endif /* ST_avsync */

#ifdef ST_display
    /* Termination of the display module */
    ErrorCode = VIDDISP_Term(Device_p->DeviceData_p->DisplayHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Display termination failed !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
        {
            FinalErrorCode = ErrorCode;
        }
    }
#endif /* ST_display */

#ifdef ST_diginput
    /* Termination of digital input */
#ifdef ST_MasterDigInput
    if (Device_p->DeviceData_p->DigitalInputIsValid)
    {
        Device_p->DeviceData_p->DigitalInputIsValid = FALSE;
    }
#else
    if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
    {
        ErrorCode = VIDINP_Term(Device_p->DeviceData_p->DigitalInputHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Digital input termination failed !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;     /* find an other error name */
            if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
            {
                FinalErrorCode = ErrorCode;
            }
        }
        Device_p->DeviceData_p->DigitalInputHandleIsValid = FALSE;
    }
#endif /* not ST_MasterDigInput */
#endif /* ST_diginput */

#ifdef ST_producer
    /* Termination of the decode module. */
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
#ifdef ST_SecondInstanceSharingTheSameDecoder
        /* Terminate only if first instance. This is a simplification : this implies that
           the second instance must be is terminated BEFORE the first, otherwise it would not work any more ! */
        if (!(Device_p->DeviceData_p->SpecialCaseOfSecondInstanceSharingTheSameDecoder))
        {
#endif /* ST_SecondInstanceSharingTheSameDecoder */
            ErrorCode = VIDPROD_Term(Device_p->DeviceData_p->ProducerHandle);
#ifdef ST_SecondInstanceSharingTheSameDecoder
        }
#endif /* ST_SecondInstanceSharingTheSameDecoder */

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Decode module termination failed !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
            {
                FinalErrorCode = ErrorCode;
            }
        }
        Device_p->DeviceData_p->ProducerHandleIsValid = FALSE;

#ifdef ST_multidecodeswcontroller
        /* Check if multi-decode module is also to be terminated */
        if (DeviceParameters.UsesMultiDecodeSWController)
        {
            /* Disconnect instance from multi-decode software controller */
            ErrorCode = VIDDEC_MultiDecodeDisconnect(InstancesOfDeviceType_p->MultiDecodeHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error: cannot disconnect from multi-instance sw controller */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module-decode controller failed to disconnect !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
                {
                    FinalErrorCode = ErrorCode;
                }
            }
        } /* End of chips using multi-decode controller */
#endif /* ST_multidecodeswcontroller */
    }
#endif /* ST_producer */

#ifdef ST_XVP_ENABLE_FGT
    /* Termination of Film Grain.                                                  */
    if (Device_p->DeviceData_p->FGTHandleIsValid)
    {
        if (VIDFGT_Term(Device_p->DeviceData_p->FGTHandle) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Film Grain termination failed !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
            {
                FinalErrorCode = ErrorCode;
            }
        }
        Device_p->DeviceData_p->FGTHandleIsValid = FALSE;
    }
#endif /* ST_XVP_ENABLE_FGT */

#ifdef ST_ordqueue
    /* Termination of the Ordering Queue Manager Component.                          */
    if (Device_p->DeviceData_p->OrderingQueueHandleIsValid)
    {
        if (VIDQUEUE_Term(Device_p->DeviceData_p->OrderingQueueHandle) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Ordering Queue termination failed !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
            {
                FinalErrorCode = ErrorCode;
            }
        }
        Device_p->DeviceData_p->OrderingQueueHandleIsValid = FALSE;
    }
#endif /* ST_ordqueue */

    /* Termination of the Buffer Manager Component.                          */
    if (VIDBUFF_Term(Device_p->DeviceData_p->BuffersHandle) != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Buffer manager termination failed !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
        {
            FinalErrorCode = ErrorCode;
        }
    }

    stvid_EnterCriticalSection(Device_p);

    /* Deallocation of the memory used for our local ViewPort structures. */
    /* Source_p->Data structure is deallocated by STVID_CloseViewPort(). */
    ViewPort_p = (VideoViewPort_t *) (Device_p->DeviceData_p->ViewportProperties_p);
    for (Index = 0; Index < Device_p->DeviceData_p->MaxViewportNumber; Index ++)
    {
        SAFE_MEMORY_DEALLOCATE(ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p,
                               Device_p->DeviceData_p->CPUPartition_p,
                               MAX_2VALUES( sizeof(STLAYER_StreamingVideo_t), sizeof(STGXOBJ_Bitmap_t)));
        SAFE_MEMORY_DEALLOCATE(ViewPort_p->Params.VideoViewportParameters.Source_p,
                               Device_p->DeviceData_p->CPUPartition_p, sizeof(STLAYER_ViewPortSource_t));
        ViewPort_p++;
    }

    SAFE_MEMORY_DEALLOCATE(Device_p->DeviceData_p->ViewportProperties_p,
                            Device_p->DeviceData_p->CPUPartition_p,
                            Device_p->DeviceData_p->MaxViewportNumber * sizeof(VideoViewPort_t));
    SAFE_MEMORY_DEALLOCATE(Device_p->DeviceData_p->LayerProperties_p,
                            Device_p->DeviceData_p->CPUPartition_p,
                            Device_p->DeviceData_p->MaxLayerNumber * sizeof(VideoLayer_t));

    /* Delete semaphore for display params protection */
    STOS_SemaphoreDelete(Device_p->DeviceData_p->CPUPartition_p, Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);
    /* Delete semaphore for VTG name or params protection */
    STOS_SemaphoreDelete(Device_p->DeviceData_p->CPUPartition_p, Device_p->DeviceData_p->VTGParamsChangeLockingSemaphore_p);
    /* Delete semaphore for layers/viewports protection */
    STOS_SemaphoreDelete(Device_p->DeviceData_p->CPUPartition_p, Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

    /* We have succeded to terminate: consider decoder number no more in use */
    Device_p->DecoderNumber = STVID_INVALID_DECODER_NUMBER;
    /* De-allocate dynamic data structure */
    SAFE_MEMORY_DEALLOCATE(Device_p->DeviceData_p, Device_p->DeviceData_p->CPUPartition_p,  sizeof(VideoDeviceData_t));
    Device_p->DeviceData_p = NULL;

    /* Aknowledge that there is now one less instance of this device type */
    InstancesOfDeviceType_p->NumberOfInitialisedInstances -= 1;

    /* Now, if required, terminate objects instanciated once for many instances of the same DeviceType */
    if (InstancesOfDeviceType_p->NumberOfInitialisedInstances == 0)
    {
        /* This was last instance of this device type */
#ifdef ST_producer
#ifdef ST_multidecodeswcontroller
        if (DeviceParameters.UsesMultiDecodeSWController)
        {
            ErrorCode = VIDDEC_MultiDecodeTerm(InstancesOfDeviceType_p->MultiDecodeHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Multi-decode controller failed to terminate !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
                {
                    FinalErrorCode = ErrorCode;
                }
            }
        }
#endif /* ST_multidecodeswcontroller */
#endif /* ST_producer */
#ifdef ST_inject
        if (DeviceParameters.UsesInject)
        {
            ErrorCode = VIDINJ_Term(InstancesOfDeviceType_p->InjecterHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Injecter failed to terminate !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                if (IsErrorCodeMoreImportantThanPrevious(FinalErrorCode, ErrorCode))
                {
                    FinalErrorCode = ErrorCode;
                }
            }
        }
#endif /* ST_inject */
    } /* This was last instance of this device type */

    stvid_LeaveCriticalSection(Device_p);

    return(FinalErrorCode);
} /* End of Term() function */



/* Exported functions ------------------------------------------------------- */

/*******************************************************************************
Name        : stvid_EnterCriticalSection
Description : Enter in a section protected against mutual access of the same object
              (data base of Devices and Units) by different processes.
Parameters  : * VideoDevice_p, pointer to a video device, to protect access to device data.
              * NULL, to protect access to static table of devices and units.
Assumptions : If VideoDevice_p is not NULL, the pointer is supposed to be
              correct (Valid VideoDevice_t pointed).
Limitations :
Returns     : Nothing.
*******************************************************************************/
void stvid_EnterCriticalSection(VideoDevice_t * VideoDevice_p)
{
    if (VideoDevice_p == NULL)
    {
        /* Enter this section only if before first Init (InterInstancesLockingSemaphore_p default set at NULL) */
        if (InterInstancesLockingSemaphore_p == NULL)
        {
            semaphore_t * TemporaryInterInstancesLockingSemaphore_p;
            BOOL          TemporarySemaphoreNeedsToBeDeleted;

            /* We can't use the instance's CPUPartition_p as it resides inside the structure that we want to protect
             * and may not even exist when stvid_EnterCriticalFunction() is called. So we pass NULL as a partition
             * and STOS_SemaphoreCreatePriority will know how to react depending on OS. */
            TemporaryInterInstancesLockingSemaphore_p = STOS_SemaphoreCreatePriority(NULL, 1);

            /* This case is to protect general access of video functions, like init/open/close and term */
            /* We do not want to call semaphore_create within the task_lock/task_unlock boundaries because
             * semaphore_create internally calls semaphore_wait and can cause normal scheduling to resume,
             * therefore unlocking the critical region */
            STOS_TaskLock();
            if (InterInstancesLockingSemaphore_p == NULL)
            {
                InterInstancesLockingSemaphore_p = TemporaryInterInstancesLockingSemaphore_p;
                TemporarySemaphoreNeedsToBeDeleted = FALSE;
            }
            else
            {
                /* Remember to delete the temporary semaphore, if the InterInstancesLockingSemaphore_p
                 * was already created by another video instance */
                TemporarySemaphoreNeedsToBeDeleted = TRUE;
            }
            STOS_TaskUnlock();
            /* Delete the temporary semaphore */
            if(TemporarySemaphoreNeedsToBeDeleted)
            {
                STOS_SemaphoreDelete(NULL, TemporaryInterInstancesLockingSemaphore_p);
            }
        }
        STOS_SemaphoreWait(InterInstancesLockingSemaphore_p);
    }
    else
    {
        /* This case is for a specific function of the device (start, stop, ...) */
        STOS_SemaphoreWait(VideoDevice_p->DeviceControlLockingSemaphore_p);
    }

} /* end of stvid_EnterCriticalSection function */


/*******************************************************************************
Name        : stvid_LeaveCriticalSection
Description : Leave the section protected against mutual access of the same object
              (database of Devices and Units) by different processes.
Parameters  : VideoDevice_p, pointer to a video device.
Assumptions : If VideoDevice_p is not NULL, the pointer is supposed to be
              correct (Valid VideoDevice_t pointed).
Limitations :
Returns     : Nothing.
*******************************************************************************/
void stvid_LeaveCriticalSection(VideoDevice_t * VideoDevice_p)
{
    if (VideoDevice_p == NULL)
    {
        /* Release the Init/Open/Close/Term protection semaphore */
        STOS_SemaphoreSignal(InterInstancesLockingSemaphore_p);
    }
    else
    {
        STOS_SemaphoreSignal(VideoDevice_p->DeviceControlLockingSemaphore_p);
    }
} /* end of stvid_LeaveCriticalSection function */

/*******************************************************************************
Name        : stvid_GetDeviceNameArray
Description : Fill given DeviceNameArray with availables devices names.
Parameters  :
Assumptions :
Limitations :
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t stvid_GetDeviceNameArray(ST_DeviceName_t* DeviceNameArray, U32* NbDevices_p)
{
	ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
	int             Index; /* Count */

	/* Init Index */
    Index = 0;

	/* Look in DeviceArray for availables devices */
	while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STVID_MAX_DEVICE))
    {
		/* Copy valid DeviceName */
		strcpy(DeviceNameArray[Index], DeviceArray[Index].DeviceName);

		Index++;
    }

	/* Save Nb Devices */
	(*NbDevices_p) = Index;

	return(ErrorCode);
}



/*
--------------------------------------------------------------------------------
Configures notification of video driver's events.
Returns     : ST_NO_ERROR if configuration done successfully.
              ST_ERROR_BAD_PARAMETER if one parameter is bad.
              ST_ERROR_INVALID_HANDLE if handle is invalid.
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_ConfigureEvent(const STVID_Handle_t Handle, const STEVT_EventID_t Event,
        const STVID_ConfigureEventParams_t * const Params_p)
{
    ST_ErrorCode_t  ErrorCode;

    /* Exit now if parameters are bad */
    if (!IsValidHandle(Handle))
    {
        /* The Handle is invalid, exit now. */
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Params_p == NULL)
    {
        /* There must be parameters, exit now. */
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (Event)
    {
        /* ----- Decode events ----- */
        case STVID_NEW_PICTURE_DECODED_EVT :
#ifdef ST_producer
            /* Event's notification is done in decode module */
            if (((VideoUnit_t *)(Handle))->Device_p->DeviceData_p->ProducerHandleIsValid)
            {
                ErrorCode = VIDPROD_ConfigureEvent( ((VideoUnit_t *)(Handle))->Device_p->DeviceData_p->ProducerHandle, Event, Params_p);
            }
            else
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#else /* ST_producer */
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* not ST_producer */
            break;
        case STVID_NEW_PICTURE_ORDERED_EVT :
#ifdef ST_ordqueue
            /* Event's notification is done in decode module */
            if (((VideoUnit_t *)(Handle))->Device_p->DeviceData_p->OrderingQueueHandleIsValid)
            {
                ErrorCode = VIDQUEUE_ConfigureEvent( ((VideoUnit_t *)(Handle))->Device_p->DeviceData_p->OrderingQueueHandle, Event, Params_p);
            }
            else
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#else /* ST_ordqueue */
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* not ST_ordqueue */
            break;
        /* ----- Display events ----- */
        case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT :
        case STVID_DISPLAY_NEW_FRAME_EVT :
		case STVID_NEW_FIELD_TO_BE_DISPLAYED_EVT :
#ifdef ST_display
            /* Event's notifications are done in display module */
            ErrorCode = VIDDISP_ConfigureEvent( ((VideoUnit_t *)(Handle))->Device_p->DeviceData_p->DisplayHandle, Event, Params_p);
#else /* ST_display */
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* not ST_display */
            break;
        /* Synchronisation events */
        case STVID_SYNCHRONISATION_CHECK_EVT :
#ifdef ST_avsync
            ErrorCode = VIDAVSYNC_ConfigureEvent( ((VideoUnit_t *)(Handle))->Device_p->DeviceData_p->AVSyncHandle, Event, Params_p);
#else /* ST_avsync */
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* not ST_avsync */
            break;

        default :
            /* Bad parameter, exit now. */
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    return(ErrorCode);

} /* End of STVID_ConfigureEvent() function. */


/*
--------------------------------------------------------------------------------
Get capabilities of the video driver instance
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_GetCapability(const ST_DeviceName_t DeviceName,
    STVID_Capability_t  * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    /* Structures dynamic in capabilities must be allocated at init, and not in this function.
       Otherwise they are freed when going out of this function ! */
/*    STVID_ScalingFactors_t HorizontalFactors[2];*/
/*    STVID_ScalingFactors_t VerticalFactors[7];*/
    VideoDevice_t * Device_p;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    stvid_EnterCriticalSection(NULL);

    if (! FirstInitDone)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Init should be called prior to STVID_GetCapability !"));
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill capability structure */
            Device_p = &(DeviceArray[DeviceIndex]);

            Capability_p->SupportedBroadcastProfile = STVID_BROADCAST_DVB | STVID_BROADCAST_DIRECTV | STVID_BROADCAST_ATSC;
            Capability_p->SupportedCodingMode = STVID_CODING_MODE_MB;
            Capability_p->SupportedColorType  = 0;
#ifdef ST_producer
            if (Device_p->DeviceData_p->ProducerHandleIsValid)
            {
                Capability_p->SupportedColorType |= STVID_COLOR_TYPE_YUV420;
            }
#endif /* ST_producer */
#ifdef ST_diginput
#ifdef ST_MasterDigInput
            if (Device_p->DeviceData_p->DigitalInputIsValid)
#else
            if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
#endif /* ST_MasterDigInput */
            {
                Capability_p->SupportedColorType |= STVID_COLOR_TYPE_YUV422;
            }
#endif /* ST_diginput */
            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_7015_MPEG :
                case STVID_DEVICE_TYPE_7020_MPEG :
                    Capability_p->SupportedCompressionLevel = STVID_COMPRESSION_LEVEL_NONE | STVID_COMPRESSION_LEVEL_1
                                                            | STVID_COMPRESSION_LEVEL_2;
                    Capability_p->SupportedDecimationFactor = STVID_DECIMATION_FACTOR_NONE | STVID_DECIMATION_FACTOR_2
                                                            | STVID_DECIMATION_FACTOR_4;
                    break;
                case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
                    Capability_p->SupportedCompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
                    Capability_p->SupportedDecimationFactor = STVID_DECIMATION_FACTOR_NONE | STVID_DECIMATION_FACTOR_2
                                                            | STVID_DECIMATION_FACTOR_4;
                    break;
                case STVID_DEVICE_TYPE_5528_MPEG :
                case STVID_DEVICE_TYPE_7710_MPEG :
                case STVID_DEVICE_TYPE_ZEUS_MPEG :
                case STVID_DEVICE_TYPE_ZEUS_VC1 :
                case STVID_DEVICE_TYPE_STD2000_MPEG :
                case STVID_DEVICE_TYPE_7710_DIGITAL_INPUT :
                    Capability_p->SupportedCompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
                    Capability_p->SupportedDecimationFactor = STVID_DECIMATION_FACTOR_NONE | STVID_DECIMATION_FACTOR_2
                                                            | STVID_DECIMATION_FACTOR_4;
                    break;
                case STVID_DEVICE_TYPE_7100_MPEG :
                case STVID_DEVICE_TYPE_7109_MPEG :
                case STVID_DEVICE_TYPE_7109D_MPEG :
                case STVID_DEVICE_TYPE_7109_VC1 :
                case STVID_DEVICE_TYPE_7200_MPEG :
                case STVID_DEVICE_TYPE_7200_VC1 :
                case STVID_DEVICE_TYPE_7100_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7109_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7200_DIGITAL_INPUT :
                    Capability_p->SupportedCompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
                    Capability_p->SupportedDecimationFactor = STVID_DECIMATION_FACTOR_NONE | STVID_DECIMATION_FACTOR_2
                                                            | STVID_DECIMATION_FACTOR_4 | STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2;
                    break;
                case STVID_DEVICE_TYPE_7100_H264 :
                case STVID_DEVICE_TYPE_7109_H264 :
                case STVID_DEVICE_TYPE_7200_H264 :
                    Capability_p->SupportedCompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
                    Capability_p->SupportedDecimationFactor = STVID_DECIMATION_FACTOR_NONE | STVID_DECIMATION_FACTOR_2
                                                            | STVID_DECIMATION_FACTOR_H4 | STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2;
                    break;
                case STVID_DEVICE_TYPE_7100_MPEG4P2 :
                case STVID_DEVICE_TYPE_7109_MPEG4P2 :
                case STVID_DEVICE_TYPE_7109_AVS :
                case STVID_DEVICE_TYPE_ZEUS_H264 :
                case STVID_DEVICE_TYPE_7200_MPEG4P2 :
                default :
                    Capability_p->SupportedCompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
                    Capability_p->SupportedDecimationFactor = STVID_DECIMATION_FACTOR_NONE;
                    break;
            }
            Capability_p->SupportedDisplayARConversion =
                STVID_DISPLAY_AR_CONVERSION_PAN_SCAN |
                STVID_DISPLAY_AR_CONVERSION_LETTER_BOX |
                STVID_DISPLAY_AR_CONVERSION_COMBINED |
                STVID_DISPLAY_AR_CONVERSION_IGNORE;
            Capability_p->SupportedDisplayAspectRatio =
                STVID_DISPLAY_ASPECT_RATIO_16TO9 |
                STVID_DISPLAY_ASPECT_RATIO_4TO3;
            Capability_p->SupportedErrorRecoveryMode =
                STVID_ERROR_RECOVERY_FULL |
                STVID_ERROR_RECOVERY_HIGH |
                STVID_ERROR_RECOVERY_PARTIAL |
                STVID_ERROR_RECOVERY_NONE;
            Capability_p->SupportedFreezeMode = STVID_FREEZE_MODE_NONE |
                STVID_FREEZE_MODE_FORCE |
                STVID_FREEZE_MODE_NO_FLICKER;
            Capability_p->SupportedFreezeField = STVID_FREEZE_FIELD_TOP |
                STVID_FREEZE_FIELD_BOTTOM |
                STVID_FREEZE_FIELD_CURRENT |
                STVID_FREEZE_FIELD_NEXT;
            Capability_p->SupportedPicture = STVID_PICTURE_LAST_DECODED | STVID_PICTURE_DISPLAYED;
            Capability_p->SupportedScreenScanType = STVID_SCAN_TYPE_INTERLACED | STVID_SCAN_TYPE_PROGRESSIVE;
            Capability_p->SupportedStop =
                STVID_STOP_NOW |
                STVID_STOP_WHEN_NEXT_REFERENCE |
                STVID_STOP_WHEN_NEXT_I |
                STVID_STOP_WHEN_END_OF_DATA;
#ifdef ST_producer
            if (Device_p->DeviceData_p->ProducerHandleIsValid)
            {
                Capability_p->SupportedStreamType =
                    STVID_STREAM_TYPE_ES |
                    STVID_STREAM_TYPE_PES |
                    STVID_STREAM_TYPE_MPEG1_PACKET;
            }
            else
#endif /* ST_producer */
            {
                Capability_p->SupportedStreamType = 0;
            }
#ifdef ST_diginput
#ifdef ST_MasterDigInput
            if (Device_p->DeviceData_p->DigitalInputIsValid)
#else
            if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
#endif /* ST_MasterDigInput */
            {
                Capability_p->SupportedStreamType |= STVID_STREAM_TYPE_UNCOMPRESSED;
            }
#endif /* ST_diginput */
            Capability_p->ProfileCapable = TRUE;
            Capability_p->StillPictureCapable = TRUE;
            Capability_p->ManualInputWindowCapable = TRUE;
            Capability_p->ManualOutputWindowCapable = TRUE;
            Capability_p->ColorKeyingCapable = FALSE;
            /* PSI and HD-PIP capabilities */
            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_7015_MPEG :
                case STVID_DEVICE_TYPE_7020_MPEG :
                    if ( (Device_p->DecoderNumber == 0) ||
                         (Device_p->DecoderNumber == 1) )
                    {
                        /* HD-PIP feature only available on video decoder 1 and 2.  */
                        Capability_p->HDPIPCapable  = TRUE;
                    }
                    else
                    {
                        Capability_p->HDPIPCapable  = FALSE;
                    }
                    Capability_p->PSICapable = TRUE;
                    break;

                case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
                    Capability_p->PSICapable    = TRUE;
                    Capability_p->HDPIPCapable  = FALSE;
                    break;

                default:
                    /* Other chips don't support PSI nor HD-PIP */
                    Capability_p->PSICapable    = FALSE;
                    Capability_p->HDPIPCapable  = FALSE;
                    break;
            }
            Capability_p->SupportedWinAlign = STVID_WIN_ALIGN_TOP_LEFT |
                STVID_WIN_ALIGN_VCENTRE_LEFT |
                STVID_WIN_ALIGN_BOTTOM_LEFT |
                STVID_WIN_ALIGN_TOP_RIGHT |
                STVID_WIN_ALIGN_VCENTRE_RIGHT |
                STVID_WIN_ALIGN_BOTTOM_RIGHT |
                STVID_WIN_ALIGN_BOTTOM_HCENTRE |
                STVID_WIN_ALIGN_TOP_HCENTRE |
                STVID_WIN_ALIGN_VCENTRE_HCENTRE;
            Capability_p->SupportedWinSize = STVID_WIN_SIZE_DONT_CARE;
            Capability_p->InputWindowHeightMin = 0;
            Capability_p->InputWindowWidthMin = 0;
            Capability_p->InputWindowPositionXPrecision = 1;
            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_5510_MPEG :
                    Capability_p->InputWindowPositionYPrecision = 16;
                    break;

                case STVID_DEVICE_TYPE_5512_MPEG :
                    Capability_p->InputWindowPositionYPrecision = 2;
                    break;

                default :
                    Capability_p->InputWindowPositionYPrecision = 1;
                    break;
            }
            Capability_p->InputWindowWidthPrecision = 1;
            Capability_p->OutputWindowHeightMin = 0;
            Capability_p->OutputWindowWidthMin = 0;
            Capability_p->OutputWindowPositionYPrecision = 1;
            /* DDTS: GNBvd44181: Input & Output precision is 1 for 5100, 5105 & 5301 */
            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_5100_MPEG :
                case STVID_DEVICE_TYPE_5105_MPEG :
                case STVID_DEVICE_TYPE_5301_MPEG :
                    Capability_p->InputWindowHeightPrecision = 1;
                    Capability_p->OutputWindowPositionXPrecision = 1;
                    Capability_p->OutputWindowWidthPrecision = 1;
                    Capability_p->OutputWindowHeightPrecision = 1;
                    break;

                default :
                    Capability_p->InputWindowHeightPrecision = 2; /* 1 ? */
                    Capability_p->OutputWindowPositionXPrecision = 2; /* 1 ? */
                    Capability_p->OutputWindowWidthPrecision = 2; /* 1 ? */
                    Capability_p->OutputWindowHeightPrecision = 2; /* 1 ? */
                    break;
            }

            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_5100_MPEG :
                case STVID_DEVICE_TYPE_5525_MPEG :
                case STVID_DEVICE_TYPE_5105_MPEG :
                case STVID_DEVICE_TYPE_5301_MPEG :
                    Device_p->DeviceData_p->HorizontalFactors[0].N = 1;
                    Device_p->DeviceData_p->HorizontalFactors[0].M = 16;
                    Device_p->DeviceData_p->HorizontalFactors[1].N = 8;
                    Device_p->DeviceData_p->HorizontalFactors[1].M = 1;
                    Capability_p->SupportedHorizontalScaling.Continuous = TRUE;
                    Capability_p->SupportedHorizontalScaling.NbScalingFactors = 2;
                    break;
                case STVID_DEVICE_TYPE_7015_MPEG :
                case STVID_DEVICE_TYPE_7020_MPEG :
                case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
                    Device_p->DeviceData_p->HorizontalFactors[0].N = 1;
                    Device_p->DeviceData_p->HorizontalFactors[0].M = 3;
                    Device_p->DeviceData_p->HorizontalFactors[1].N = 6;
                    Device_p->DeviceData_p->HorizontalFactors[1].M = 1;
                    Capability_p->SupportedHorizontalScaling.Continuous = TRUE;
                    Capability_p->SupportedHorizontalScaling.NbScalingFactors = 2;
                    break;
                default :
                    Device_p->DeviceData_p->HorizontalFactors[0].N = 1;
                    Device_p->DeviceData_p->HorizontalFactors[0].M = 4;
                    Device_p->DeviceData_p->HorizontalFactors[1].N = 8;
                    Device_p->DeviceData_p->HorizontalFactors[1].M = 1;
                    Capability_p->SupportedHorizontalScaling.Continuous = TRUE;
                    Capability_p->SupportedHorizontalScaling.NbScalingFactors = 2;
                    break;
            }
            Capability_p->SupportedHorizontalScaling.ScalingFactors_p = Device_p->DeviceData_p->HorizontalFactors;
            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_5510_MPEG :
                case STVID_DEVICE_TYPE_5512_MPEG :
                    Device_p->DeviceData_p->VerticalFactors[0].N = 1;
                    Device_p->DeviceData_p->VerticalFactors[0].M = 4;
                    Device_p->DeviceData_p->VerticalFactors[1].N = 3;
                    Device_p->DeviceData_p->VerticalFactors[1].M = 8;
                    Device_p->DeviceData_p->VerticalFactors[2].N = 1;
                    Device_p->DeviceData_p->VerticalFactors[2].M = 2;
                    Device_p->DeviceData_p->VerticalFactors[3].N = 3;
                    Device_p->DeviceData_p->VerticalFactors[3].M = 4;
                    Device_p->DeviceData_p->VerticalFactors[4].N = 7;
                    Device_p->DeviceData_p->VerticalFactors[4].M = 8;
                    Device_p->DeviceData_p->VerticalFactors[5].N = 1;
                    Device_p->DeviceData_p->VerticalFactors[5].M = 1;
                    Device_p->DeviceData_p->VerticalFactors[6].N = 2;
                    Device_p->DeviceData_p->VerticalFactors[6].M = 1;
                    Capability_p->SupportedVerticalScaling.Continuous = FALSE;
                    Capability_p->SupportedVerticalScaling.NbScalingFactors = 7;
                    break;

                case STVID_DEVICE_TYPE_5508_MPEG :
                case STVID_DEVICE_TYPE_5518_MPEG :
                case STVID_DEVICE_TYPE_5514_MPEG :
                case STVID_DEVICE_TYPE_5516_MPEG :
                case STVID_DEVICE_TYPE_5517_MPEG :
                case STVID_DEVICE_TYPE_5528_MPEG :
                case STVID_DEVICE_TYPE_7100_H264 :
                case STVID_DEVICE_TYPE_7100_MPEG :
                case STVID_DEVICE_TYPE_7100_MPEG4P2 :
                case STVID_DEVICE_TYPE_7109_H264 :
                case STVID_DEVICE_TYPE_7109_MPEG :
                case STVID_DEVICE_TYPE_7109D_MPEG :
                case STVID_DEVICE_TYPE_7109_MPEG4P2 :
                case STVID_DEVICE_TYPE_7109_AVS :
                case STVID_DEVICE_TYPE_7109_VC1 :
                case STVID_DEVICE_TYPE_ZEUS_MPEG :
                case STVID_DEVICE_TYPE_ZEUS_H264 :
                case STVID_DEVICE_TYPE_ZEUS_VC1 :
                case STVID_DEVICE_TYPE_7200_MPEG :
                case STVID_DEVICE_TYPE_7200_H264 :
                case STVID_DEVICE_TYPE_7200_VC1 :
                case STVID_DEVICE_TYPE_7200_MPEG4P2 :
                case STVID_DEVICE_TYPE_7710_MPEG :
                case STVID_DEVICE_TYPE_7710_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7100_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7109_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7200_DIGITAL_INPUT :
                    Device_p->DeviceData_p->VerticalFactors[0].N = 1;
                    Device_p->DeviceData_p->VerticalFactors[0].M = 4;
                    Device_p->DeviceData_p->VerticalFactors[1].N = 8;
                    Device_p->DeviceData_p->VerticalFactors[1].M = 1;
                    Capability_p->SupportedVerticalScaling.Continuous = TRUE;
                    Capability_p->SupportedVerticalScaling.NbScalingFactors = 2;
                    break;

                case STVID_DEVICE_TYPE_5100_MPEG :
                case STVID_DEVICE_TYPE_5525_MPEG :
                case STVID_DEVICE_TYPE_5105_MPEG :
                case STVID_DEVICE_TYPE_5301_MPEG :
                    Device_p->DeviceData_p->VerticalFactors[0].N = 1;
                    Device_p->DeviceData_p->VerticalFactors[0].M = 16;
                    Device_p->DeviceData_p->VerticalFactors[1].N = 8;
                    Device_p->DeviceData_p->VerticalFactors[1].M = 1;
                    Capability_p->SupportedVerticalScaling.Continuous = TRUE;
                    Capability_p->SupportedVerticalScaling.NbScalingFactors = 2;
                    break;
                case STVID_DEVICE_TYPE_7015_MPEG :
                case STVID_DEVICE_TYPE_7020_MPEG :
                case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
                    Device_p->DeviceData_p->VerticalFactors[0].N = 1;
                    Device_p->DeviceData_p->VerticalFactors[0].M = 4;
                    Device_p->DeviceData_p->VerticalFactors[1].N = 6;
                    Device_p->DeviceData_p->VerticalFactors[1].M = 1;
                    Capability_p->SupportedVerticalScaling.Continuous = TRUE;
                    Capability_p->SupportedVerticalScaling.NbScalingFactors = 2;
                    break;

                default :
                    /* Say only zoom 1 capable when no info */
                    Device_p->DeviceData_p->VerticalFactors[0].N = 1;
                    Device_p->DeviceData_p->VerticalFactors[0].M = 1;
                    Capability_p->SupportedVerticalScaling.Continuous = FALSE;
                    Capability_p->SupportedVerticalScaling.NbScalingFactors = 1;
                    break;
            }
            Capability_p->SupportedVerticalScaling.ScalingFactors_p = Device_p->DeviceData_p->VerticalFactors;
            Capability_p->DecoderNumber = Device_p->DecoderNumber; /* Retrieved at init */

            Capability_p->SupportedSetupObject = STVID_SETUP_FRAME_BUFFERS_PARTITION;
            Capability_p->SupportedSetupObject |= STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION;
            Capability_p->SupportedSetupObject |= STVID_SETUP_DATA_INPUT_BUFFER_PARTITION;
            Capability_p->SupportedSetupObject |= STVID_SETUP_BIT_BUFFER_PARTITION;

            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_7109_H264 :
                case STVID_DEVICE_TYPE_7200_H264 :
                    Capability_p->SupportedSetupObject |= STVID_SETUP_FDMA_NODES_PARTITION;

                    #if defined (DVD_SECURED_CHIP)
                        Capability_p->SupportedSetupObject |= STVID_SETUP_ES_COPY_BUFFER;
                        Capability_p->SupportedSetupObject |= STVID_SETUP_ES_COPY_BUFFER_PARTITION;
                    #endif /* DVD_SECURED_CHIP */
                case STVID_DEVICE_TYPE_7100_H264 :
                    Capability_p->SupportedSetupObject |= STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION;
                    Capability_p->SupportedSetupObject |= STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION;
                    Capability_p->SupportedSetupObject |= STVID_SETUP_PICTURE_PARAMETER_BUFFERS_PARTITION;
                    break;

                case STVID_DEVICE_TYPE_7109_MPEG :
                case STVID_DEVICE_TYPE_7109D_MPEG :
                case STVID_DEVICE_TYPE_7200_MPEG :
                    Capability_p->SupportedSetupObject |= STVID_SETUP_FDMA_NODES_PARTITION;
                    #if 0 /* DG TO DO */
                        #if defined (DVD_SECURED_CHIP)
                            Capability_p->SupportedSetupObject |= STVID_SETUP_ES_COPY_BUFFER;
                            Capability_p->SupportedSetupObject |= STVID_SETUP_ES_COPY_BUFFER_PARTITION;
                        #endif /* DVD_SECURED_CHIP */
                    #endif
                case STVID_DEVICE_TYPE_7100_MPEG :
                    Capability_p->SupportedSetupObject |= STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION;
                    break;

                case STVID_DEVICE_TYPE_7109_VC1 :
                case STVID_DEVICE_TYPE_7200_VC1 :
                    Capability_p->SupportedSetupObject |= STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION;
                    Capability_p->SupportedSetupObject |= STVID_SETUP_FDMA_NODES_PARTITION;
                    Capability_p->SupportedSetupObject |= STVID_SETUP_PICTURE_PARAMETER_BUFFERS_PARTITION;
                    #if 0 /* DG TO DO */
                        #if defined (DVD_SECURED_CHIP)
                            Capability_p->SupportedSetupObject |= STVID_SETUP_ES_COPY_BUFFER;
                            Capability_p->SupportedSetupObject |= STVID_SETUP_ES_COPY_BUFFER_PARTITION;
                        #endif /* DVD_SECURED_CHIP */
                    #endif
                    break;

                case STVID_DEVICE_TYPE_7109_MPEG4P2 :
                case STVID_DEVICE_TYPE_7109_AVS :
                case STVID_DEVICE_TYPE_7200_MPEG4P2 :
                    Capability_p->SupportedSetupObject |= STVID_SETUP_FDMA_NODES_PARTITION;
                case STVID_DEVICE_TYPE_7100_MPEG4P2 :
                    Capability_p->SupportedSetupObject |= STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION;
                    break;

                default :
                    break;
            }

        } /* End is valid device */
    }

    stvid_LeaveCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_GetCapability() function */

#ifdef VALID_TOOLS
/*
--------------------------------------------------------------------------------
 Registers trace items for all modules
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_RegisterTraces(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#ifdef ST_producer
#ifdef ST_multicodec
    ErrorCode = VIDPROD_RegisterTrace();
#endif /*  ST_multicodec */
#endif /* ST_producer */
#ifdef ST_display
    ErrorCode = VIDDISP_RegisterTrace();
#endif /* ST_display */
#ifdef ST_crc
  ErrorCode = VIDCRC_RegisterTrace();
#endif /* ST_crc */
  return(ErrorCode);
} /* End of STVID_RegisterTraces() function */
#endif /* VALID_TOOLS */

/*
--------------------------------------------------------------------------------
Get revision of the video driver
--------------------------------------------------------------------------------
*/
ST_Revision_t STVID_GetRevision(void)
{
    /* use VID_Revision from vid_rev.h, the string must be updated in vid_rev.h now */
    return(VID_Revision);
}


/*
--------------------------------------------------------------------------------
Initializes a Video Decoder and gives it the name DeviceName.
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_Init(const ST_DeviceName_t DeviceName,
    const STVID_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||                       /* There must be parameters */
        (InitParams_p->MaxOpen > STVID_MAX_OPEN) ||     /* No more than MAX_OPEN open handles supported */
        (InitParams_p->MaxOpen == 0) ||                 /* Cannot be 0 ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Software protection : the application cannot call a 2nd STVID_Init()
    while the first one is not finished. It cannot either begin a STVID_Term function */
    stvid_EnterCriticalSection(NULL);

    /* First time: initialise devices and units as empty */
    if (!(FirstInitDone))
    {
        for (Index = 0; Index < STVID_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
            DeviceArray[Index].DecoderNumber = STVID_INVALID_DECODER_NUMBER;
            DeviceArray[Index].DeviceData_p = NULL;
        }

        for (Index = 0; Index < STVID_MAX_UNIT; Index++)
        {
            stvid_UnitArray[Index].UnitValidity = 0;
        }
        for (Index = 0; Index < MAX_DEVICE_TYPES; Index++)
        {
            InstancesSummary[Index].DeviceType = 0; /* whatever, as nb below is 0 this is not important */
            InstancesSummary[Index].NumberOfInitialisedInstances = 0;
        }
        /* Process this only once */
        FirstInitDone = TRUE;
    }

    /* Check if device already initialised and return error if so */
    if (GetIndexOfDeviceNamed(DeviceName) != INVALID_DEVICE_INDEX)
    {
        /* Device name found */
        ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Look for a non-initialised device and return error if none is available */
        Index = 0;
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STVID_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STVID_MAX_DEVICE)
        {
            /* All devices initialised */
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            ErrorCode = Init(&DeviceArray[Index], InitParams_p, DeviceName);

            if (ErrorCode == ST_NO_ERROR)
            {
                /* Device available and successfully initialised: register device name */
                strcpy(DeviceArray[Index].DeviceName, DeviceName);
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                DeviceArray[Index].DeviceData_p->MaxOpen = InitParams_p->MaxOpen;

                /* Initialise semaphore used to protect device data. */
                DeviceArray[Index].DeviceControlLockingSemaphore_p =
                STOS_SemaphoreCreatePriority(DeviceArray[Index].DeviceData_p->CPUPartition_p,1);
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialised", DeviceArray[Index].DeviceName));
            }
        }
    }

    stvid_LeaveCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_Init() function */

/*
--------------------------------------------------------------------------------
Open video
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_Open(const ST_DeviceName_t DeviceName,
        const STVID_OpenParams_t * const OpenParams_p,
        STVID_Handle_t * const Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Software protection : the application cannot call a 2nd STVID_Open()
       while the first one is not finished. */
    stvid_EnterCriticalSection(NULL);

    /* Check the init has been done prior to STVID_Open. */
    if (!FirstInitDone)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "STVID_Init should be called prior to STVID_Open"));
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            stvid_EnterCriticalSection(&DeviceArray[DeviceIndex]);
            /* Look for a free unit and check all opened units to check if MaxOpen is not reached */
            UnitIndex = STVID_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STVID_MAX_UNIT; Index++)
            {
                if ((stvid_UnitArray[Index].UnitValidity == STVID_VALID_UNIT) && (stvid_UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (stvid_UnitArray[Index].UnitValidity != STVID_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].DeviceData_p->MaxOpen) || (UnitIndex >= STVID_MAX_UNIT))
            {
                /* None of the units is free or MaxOpen reached */
                ErrorCode = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STVID_Handle_t) &stvid_UnitArray[UnitIndex];
                stvid_UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex];

                /* API specific actions after opening */
                ErrorCode = Open(&stvid_UnitArray[UnitIndex], OpenParams_p);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    stvid_UnitArray[UnitIndex].UnitValidity = STVID_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Video handle opened on device '%s'", DeviceName));
                }
            } /* End found unit unused */
            stvid_LeaveCriticalSection(&DeviceArray[DeviceIndex]);
        } /* End device valid */
    } /* End FirstInitDone */

    /* End of the critical section. */
    stvid_LeaveCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_Open() function */


/*
--------------------------------------------------------------------------------
Close video
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_Close(const STVID_Handle_t Handle)
{
    VideoUnit_t * Unit_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Software protection : the application cannot call a 2nd STVID_Close()
     while the first one is not finished. */
    stvid_EnterCriticalSection(NULL);

    /* Check if an init has been done. */
    if (!(FirstInitDone))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
            "STVID_Init should be called prior to STVID_Close"));
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        if (!IsValidHandle(Handle))
        {
            ErrorCode = ST_ERROR_INVALID_HANDLE;
        }
        else
        {
            Unit_p = (VideoUnit_t *) Handle;

            stvid_EnterCriticalSection(Unit_p->Device_p);

            /* API specific actions before closing */
            ErrorCode = Close(Unit_p);

            /* Close only if no errors */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Video handle closed on device '%s'", Unit_p->Device_p->DeviceName));
            }
            stvid_LeaveCriticalSection(Unit_p->Device_p);
        }
    }

    /* End of the critical section.                                            */
    stvid_LeaveCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_Close() function */


/*
--------------------------------------------------------------------------------
Terminate video driver
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_Term(const ST_DeviceName_t DeviceName,
        const STVID_TermParams_t * const TermParams_p)
{
    VideoUnit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    ST_Partition_t *      Partition_p ;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')            /* Device name should not be empty */
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Software protection : the application cannot call a 2nd STVID_Term()
       while the first one is not finished. */
    stvid_EnterCriticalSection(NULL);

    /* Check the init has been done prior to STVID_Term. */
    if (! FirstInitDone)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Init should be "
                      "called prior to STVID_Term"));
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if NOT so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Check if there is still 'open' on this device */
/*            Found = FALSE;*/
            UnitIndex = 0;
            Unit_p = stvid_UnitArray;
            while ((UnitIndex < STVID_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVID_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    UnitIndex = 0;
                    Unit_p = stvid_UnitArray;
                    while (UnitIndex < STVID_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVID_VALID_UNIT))
                        {
                            /* Found an open instance: close it ! */
                            ErrorCode = Close(Unit_p);

                            if (ErrorCode != ST_NO_ERROR)
                            {
                                /* If error: don't care, force close to force terminate... */
                                ErrorCode = ST_NO_ERROR;
                            }

                            /* Un-register opened handle whatever the error */
                            Unit_p->UnitValidity = 0;

                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Video handle closed on device '%s'", Unit_p->Device_p->DeviceName));
                        }
                        Unit_p++;
                        UnitIndex++;
                    }
                }
                else
                {
                    ErrorCode = ST_ERROR_OPEN_HANDLE;
                }
            }

            /* Terminate if OK */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* save  DeviceArray[DeviceIndex].DeviceData_p->CPUPartition_p
                 * in an auxilary variable to be used for STOS_SemaphoreDelete */
                Partition_p = DeviceArray[DeviceIndex].DeviceData_p->CPUPartition_p;
                /* API specific terminations */
                ErrorCode = Term(&DeviceArray[DeviceIndex], TermParams_p);

/* Don't leave instance semi-terminated: terminate as much as possible, to enable next Init() to work ! */
/*                if (ErrorCode == ST_NO_ERROR)*/
/*                {*/
                    /* Device found: desallocate memory, free device */
                    DeviceArray[DeviceIndex].DeviceName[0] = '\0';

                    /* Take the section semaphore not to block an other task by destructing the semaphore */

                    stvid_EnterCriticalSection(&DeviceArray[DeviceIndex]);

                STOS_SemaphoreDelete(Partition_p,
                            DeviceArray[DeviceIndex].DeviceControlLockingSemaphore_p);

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
/*                }*/
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */

    /* End of the critical section. */
    stvid_LeaveCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_Term() function */

#ifdef VALID_TOOLS
/*-----------------7/8/2005 1:23PM------------------
   STVID_CRCSetMSGLOGHandle
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if parameter problem
              STVID_ERROR_MEMORY_ACCESS if memory copy fails
 * --------------------------------------------------*/
ST_ErrorCode_t STVID_SetMSGLOGHandle(const STVID_Handle_t VideoHandle, MSGLOG_Handle_t MSGLOG_Handle)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    VideoDevice_t  *Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;
#ifdef ST_crc
       ErrorCode = VIDCRC_SetMSGLOGHandle(Device_p->DeviceData_p->CRCHandle, MSGLOG_Handle);
#endif /* ST_crc */
#ifdef ST_producer
        if(ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = VIDPROD_SetMSGLOGHandle(Device_p->DeviceData_p->ProducerHandle, MSGLOG_Handle);
        }
#endif /* ST_producer */
    }
    return(ErrorCode);
} /* End of STVID_CRCSetMSGLOGHandle() function */
#endif /* VALID_TOOLS */


#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : STVID_GetStatistics
Description :
Parameters  : IN  : Video Handle.
              OUT : Data Interface Parameters.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR.
*******************************************************************************/
ST_ErrorCode_t STVID_GetStatistics(const ST_DeviceName_t DeviceName, STVID_Statistics_t  * const Statistics_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VideoDevice_t * Device_p;

    /* Exit now if parameters are bad */
    if ((Statistics_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    memset (Statistics_p, 0x0, sizeof(STVID_Statistics_t)); /* fill in data with 0x0 by default */

    stvid_EnterCriticalSection(NULL);

    if (! FirstInitDone)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Init should be called prior to STVID_GetStatistics !"));
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill statistics structure */
            Device_p = &(DeviceArray[DeviceIndex]);
            stvid_EnterCriticalSection(Device_p);

            /* Affect the return data here from all the STVID components */

            /* API (error recovery) */
            Statistics_p->ApiPbLiveResetWaitForFirstPictureDetected = Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForFirstPictureDetected ;
            Statistics_p->ApiPbLiveResetWaitForFirstPictureDecoded  = Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForFirstPictureDecoded ;
            Statistics_p->ApiPbLiveResetWaitForNextPicture  = Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForNextPicture ;

#ifdef ST_producer
            /* Producer */
            if (Device_p->DeviceData_p->ProducerHandleIsValid)
            {
                ErrorCode = VIDPROD_GetStatistics(Device_p->DeviceData_p->ProducerHandle, Statistics_p);
            }
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_GetStatistics() : failed !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
#endif /* ST_producer */
#ifdef ST_ordqueue
            if (Device_p->DeviceData_p->OrderingQueueHandleIsValid)
            {
                ErrorCode = VIDQUEUE_GetStatistics(Device_p->DeviceData_p->OrderingQueueHandle, Statistics_p);
            }
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDQUEUE_GetStatistics() : failed !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
#endif /* ST_ordqueue */
#ifdef ST_display
            /* Display */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = VIDDISP_GetStatistics(Device_p->DeviceData_p->DisplayHandle, Statistics_p);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDISP_GetStatistics() : failed !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }
#endif /* ST_display */
#ifdef ST_avsync
            /* AVSync */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = VIDAVSYNC_GetStatistics(Device_p->DeviceData_p->AVSyncHandle, Statistics_p);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDAVSYNC_GetStatistics() : failed !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }
#endif /* ST_avsync */
#ifdef ST_speed
            /* Speed */
            if (Device_p->DeviceData_p->SpeedHandleIsValid)
            {
                ErrorCode = VIDSPEED_GetStatistics(Device_p->DeviceData_p->SpeedHandle, Statistics_p);
            }
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDSPEED_GetStatistics() : failed !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
#endif /* ST_speed */
            stvid_LeaveCriticalSection(Device_p);
        } /* End is valid device */
    }

    stvid_LeaveCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_GetStatistics() function */



/*******************************************************************************
Name        : STVID_ResetStatistics
Description :
Parameters  : IN  : Video Handle.
              OUT : Data Interface Parameters.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR.
*******************************************************************************/
ST_ErrorCode_t STVID_ResetStatistics(const ST_DeviceName_t DeviceName, const STVID_Statistics_t * const Statistics_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VideoDevice_t * Device_p;

    /* Exit now if parameters are bad */
    if ((Statistics_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    stvid_EnterCriticalSection(NULL);

    if (! FirstInitDone)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Init should be called prior to STVID_GetStatistics !"));
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill statistics structure */
            Device_p = &(DeviceArray[DeviceIndex]);
            stvid_EnterCriticalSection(Device_p);

            /* Affect the return data here from all the STVID components */

            /* API (error recovery) */
            if (Statistics_p->ApiPbLiveResetWaitForFirstPictureDetected != 0)
            {
                Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForFirstPictureDetected = 0;
            }
            if (Statistics_p->ApiPbLiveResetWaitForFirstPictureDecoded != 0)
            {
                Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForFirstPictureDecoded = 0;
            }
            if (Statistics_p->ApiPbLiveResetWaitForNextPicture != 0)
            {
                Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForNextPicture = 0;
            }

#ifdef ST_producer
            /* Producer */
            if (Device_p->DeviceData_p->ProducerHandleIsValid)
            {
                ErrorCode = VIDPROD_ResetStatistics(Device_p->DeviceData_p->ProducerHandle, Statistics_p);
            }
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_ResetStatistics() : failed !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
#endif /* ST_producer */
#ifdef ST_ordqueue
            if (Device_p->DeviceData_p->OrderingQueueHandleIsValid)
            {
                ErrorCode = VIDQUEUE_ResetStatistics(Device_p->DeviceData_p->OrderingQueueHandle, Statistics_p);
            }
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDQUEUE_ResetStatistics() : failed !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
#endif /* ST_ordqueue */
#ifdef ST_display
            /* Display */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = VIDDISP_ResetStatistics(Device_p->DeviceData_p->DisplayHandle, Statistics_p);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDISP_ResetStatistics() : failed !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }
#endif /* ST_display */
#ifdef ST_avsync
            /* AVSync */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = VIDAVSYNC_ResetStatistics(Device_p->DeviceData_p->AVSyncHandle, Statistics_p);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDAVSYNC_ResetStatistics() : failed !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }
#endif /* ST_avsync */
#ifdef ST_speed
            /* Speed */
            if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->SpeedHandleIsValid))
            {
                ErrorCode = VIDSPEED_ResetStatistics(Device_p->DeviceData_p->SpeedHandle, Statistics_p);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDSPEED_ResetStatistics() : failed !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }
#endif /* ST_speed */
            stvid_LeaveCriticalSection(Device_p);
        } /* End is valid device */
    }

    stvid_LeaveCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_ResetStatistics() function */
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
/*******************************************************************************
Name        : STVID_GetStatus
Description :
Parameters  : IN  : Video Handle.
              OUT : Data Interface Parameters.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR.
*******************************************************************************/
ST_ErrorCode_t STVID_GetStatus(const ST_DeviceName_t DeviceName, STVID_Status_t  * const Status_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VideoDevice_t * Device_p;
	VIDCOM_InternalProfile_t InternalProfile;

    /* Exit now if parameters are bad */
    if ((Status_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    memset (Status_p, 0x0, sizeof(STVID_Status_t)); /* fill in data with 0x0 by default */

    stvid_EnterCriticalSection(NULL);

    if (! FirstInitDone)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Init should be called prior to STVID_GetStatus !"));
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill status structure */
            Device_p = &(DeviceArray[DeviceIndex]);
            stvid_EnterCriticalSection(Device_p);

#ifdef ST_producer
            /* Producer */
            if (Device_p->DeviceData_p->ProducerHandleIsValid)
            {
                ErrorCode = VIDPROD_GetStatus(Device_p->DeviceData_p->ProducerHandle, Status_p);
            }
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_GetStatus() : failed !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
#endif /* ST_producer */
#ifdef ST_avsync
            /* AVSync */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = VIDAVSYNC_GetStatus(Device_p->DeviceData_p->AVSyncHandle, Status_p);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDAVSYNC_GetStatus() : failed !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }
#endif /* ST_avsync */


			/* Get Memory profile */
			ErrorCode = VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &InternalProfile);

    		if (ErrorCode == ST_NO_ERROR)
    		{
    		    Status_p->MemoryProfile = InternalProfile.ApiProfile;
    		}

			/* Get State Machine */
			Status_p->StateMachine = Device_p->DeviceData_p->Status;

			/* Start Params related */
			Status_p->VideoAlreadyStarted = Device_p->DeviceData_p->VideoAlreadyStarted;
			Status_p->LastStartParams = Device_p->DeviceData_p->LastStartParams;

			/* Freeze Params related */
			Status_p->VideoAlreadyFreezed = Device_p->DeviceData_p->VideoAlreadyFreezed;
			Status_p->LastFreezeParams = Device_p->DeviceData_p->LastFreezeParams;

            stvid_LeaveCriticalSection(Device_p);
        } /* End is valid device */
    }

    stvid_LeaveCriticalSection(NULL);

    return(ErrorCode);
} /* End of STVID_GetStatus() function */
#endif /* STVID_DEBUG_GET_STATUS */
/* End of vid_com.c*/
