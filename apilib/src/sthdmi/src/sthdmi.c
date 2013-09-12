/*******************************************************************************

File name   : sthdmi.c

Description : HDMI driver module.

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
28 Feb 2005        Created                                          AC
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#ifdef ST_OSLINUX
#include "sthdmi_core.h"
#else
#include "sttbx.h"
#endif    /* ST_OSLINUX */

#include "sthdmi.h"
#include "hdmi_drv.h"
#include "stos.h"
#include "hdmi_rev.h"

/* Private Constants -------------------------------------------------------- */

#define INVALID_DEVICE_INDEX (-1)


#if defined(ST_7710)||defined (ST_7100)||defined(ST_7109)|| defined (ST_7200)

#define STHDMI_MAX_INFOFRAME_CONF 3
#define STHDMI_MAX_INFOFRAME_TYPE 5


/*  AVI  SPD  MS AUD VS  */
static const InfoFrameType_t CheckInfoFrame[STHDMI_MAX_INFOFRAME_CONF][STHDMI_MAX_INFOFRAME_TYPE]=
{
    {INFO_FRAME_NONE, INFO_FRAME_NONE, INFO_FRAME_NONE, INFO_FRAME_NONE, INFO_FRAME_NONE},
    {INFO_FRAME_VER_ONE, INFO_FRAME_NONE, INFO_FRAME_NONE, INFO_FRAME_NONE, INFO_FRAME_NONE},
    {INFO_FRAME_VER_TWO, INFO_FRAME_VER_ONE, INFO_FRAME_VER_ONE, INFO_FRAME_VER_ONE, INFO_FRAME_VER_ONE}
};
#endif


/* Private Variables (static)------------------------------------------------ */

static sthdmi_Device_t *DeviceArray;
static semaphore_t * InstancesAccessControl_p;   /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;

/* Global Variables --------------------------------------------------------- */
/* not static because used in IsValidHandle macro */
sthdmi_Unit_t *sthdmi_UnitArray;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(sthdmi_Device_t * const Device_p, const STHDMI_InitParams_t * const InitParams_p);
static ST_ErrorCode_t Open(sthdmi_Unit_t * const Unit_p);
static ST_ErrorCode_t Close(sthdmi_Unit_t * const Unit_p);
static ST_ErrorCode_t Term(sthdmi_Device_t * const Device_p);
static void GetCapability( sthdmi_Device_t *const Device_p, STHDMI_Capability_t * const Capability_p);
static void GetSinkCapability(sthdmi_Device_t *const Device_p, STHDMI_SinkCapability_t * const Capability_p);
static void GetSourceCapability(sthdmi_Device_t *const Device_p, STHDMI_SourceCapability_t * const Capability_p);
static ST_ErrorCode_t TestOutputValidity(const STHDMI_InitParams_t * const InitParams_p );
static ST_ErrorCode_t TestAVIValidity(const STHDMI_InitParams_t * const InitParams_p );
static ST_ErrorCode_t TestSPDValidity(const STHDMI_InitParams_t * const InitParams_p );
static ST_ErrorCode_t TestMSValidity(const STHDMI_InitParams_t * const InitParams_p );
static ST_ErrorCode_t TestAudioValidity(const STHDMI_InitParams_t * const InitParams_p );
static ST_ErrorCode_t TestVSValidity(const STHDMI_InitParams_t * const InitParams_p );
static ST_ErrorCode_t TestAvailableInfoFrame(const STHDMI_InitParams_t  * const InitParams_p);
static ST_ErrorCode_t TestInfoFrameCompatibility(const STHDMI_InitParams_t * const InitParams_p);
static ST_ErrorCode_t FindInfoFrameConfiguration (const InfoFrameType_t InfoFrame[]);
/*******************************************************************************
Name        : EnterCriticalSection
Description : Used to protect critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnterCriticalSection(void)
{
    static BOOL InstancesAccessControlInitialized = FALSE;

    STOS_TaskLock();
    if (!InstancesAccessControlInitialized)
    {
        InstancesAccessControlInitialized = TRUE;
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        InstancesAccessControl_p = STOS_SemaphoreCreateFifo(NULL,1);


    }
    STOS_TaskUnlock();

    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(InstancesAccessControl_p);

} /* End of EnterCriticalSection() function */

/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    /* Release the Init/Open/Close/Term protection semaphore */
     STOS_SemaphoreSignal(InstancesAccessControl_p);
} /* End of LeaveCriticalSection() function */

/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t Init(sthdmi_Device_t * const Device_p, const STHDMI_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    ST_DeviceName_t     TmpVtgName;
    STVTG_Handle_t      VtgHandle;
    STVTG_OpenParams_t  VtgOpenParams;
    ST_DeviceName_t     TmpVoutName;
    STVOUT_Handle_t     VoutHandle;
    STVOUT_OpenParams_t VoutOpenParams;
    ST_DeviceName_t     TmpEvtName;
    STEVT_Handle_t      EvtHandle;
    STEVT_OpenParams_t  EvtOpenParams;


     /* Test some initialization parameters and Exit if some of them are invalid*/
    ErrorCode = TestOutputValidity(InitParams_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }

     /* InitParams exploitation and other actions */

    switch (InitParams_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
             strcpy(TmpVtgName,InitParams_p->Target.OnChipHdmiCell.VTGDeviceName);
             strcpy(TmpVoutName,InitParams_p->Target.OnChipHdmiCell.VOUTDeviceName);
             strcpy(TmpEvtName,InitParams_p->Target.OnChipHdmiCell.EvtDeviceName);
             break;
        default :
             return(ST_ERROR_BAD_PARAMETER);
            break;
    }
    if (OK(ErrorCode))
    {
        ErrorCode = TestAVIValidity(InitParams_p);
    }
    if (OK(ErrorCode))
    {
        ErrorCode = TestSPDValidity(InitParams_p);
    }
    if (OK(ErrorCode))
    {
        ErrorCode = TestMSValidity (InitParams_p);
    }
    if (OK(ErrorCode))
    {
        ErrorCode = TestAudioValidity(InitParams_p);
    }
    if (OK(ErrorCode))
    {
        ErrorCode = TestVSValidity(InitParams_p);
    }
    if (OK(ErrorCode))
    {
        switch (InitParams_p->DeviceType)
        {
            case STHDMI_DEVICE_TYPE_7710 : /* no break */
            case STHDMI_DEVICE_TYPE_7100 : /* no break */
            case STHDMI_DEVICE_TYPE_7200 :
                 if (OK(ErrorCode))
                 {
                    ErrorCode = STVTG_Open(TmpVtgName,&VtgOpenParams,&VtgHandle);
                 }
                 if (OK(ErrorCode))
                 {
                    Device_p->VtgHandle = VtgHandle;
                    strcpy(Device_p->VtgName, TmpVtgName);
                    Device_p->VtgName[ST_MAX_DEVICE_NAME-1]='\0';
                 }
                 else
                 {
                    ErrorCode = STHDMI_ERROR_VTG_UNKOWN;
                    return(ErrorCode);

                 }
                 if (OK(ErrorCode))
                 {
                 ErrorCode = STVOUT_Open(TmpVoutName, &VoutOpenParams,&VoutHandle);
                 }
                 if (OK(ErrorCode))
                 {
                    Device_p->VoutHandle = VoutHandle;
                    strcpy(Device_p->VoutName, TmpVoutName);
                    Device_p->VoutName[ST_MAX_DEVICE_NAME-1]='\0';
                 }
                 else
                 {
                   ErrorCode = STHDMI_ERROR_VOUT_UNKOWN;
                   return(ErrorCode);
                 }
                 if (OK(ErrorCode))
                 {
                 ErrorCode = STEVT_Open(TmpEvtName,&EvtOpenParams,&EvtHandle);
                 }
                 if (OK(ErrorCode))
                 {
                    Device_p->EvtHandle = EvtHandle;
                    strcpy(Device_p->EvtName,TmpEvtName);
                    Device_p->EvtName[ST_MAX_DEVICE_NAME-1]='\0';
                 }
                 else
                 {
                    ErrorCode = STHDMI_ERROR_EVT_UNKNOWN;
                    return(ErrorCode);
                 }
                 break;
            default :

                 return(ST_ERROR_BAD_PARAMETER);
                break;
        }
    }
    else
    {
       ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    /*if (OK(ErrorCode)) ErrorCode = TestEdidValidity(InitParams_p);*/
    if (OK(ErrorCode))
    {
    ErrorCode = TestAvailableInfoFrame(InitParams_p);
    }
    if (OK(ErrorCode))
    {
        switch (InitParams_p->DeviceType)
        {
            case STHDMI_DEVICE_TYPE_7710 : /* no break */
            case STHDMI_DEVICE_TYPE_7100 : /* no break */
            case STHDMI_DEVICE_TYPE_7200 :
                 Device_p->AVIType = InitParams_p->AVIType;
                 Device_p->SPDType = InitParams_p->SPDType;
                 Device_p->MSType = InitParams_p->MSType;
                 Device_p->AUDIOType = InitParams_p->AUDIOType;
                 Device_p->VSType = InitParams_p->VSType;
                 break;
             default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }

    }
    else
    {
        ErrorCode =ST_ERROR_BAD_PARAMETER;
    }

    if (OK(ErrorCode))
    {
    ErrorCode = TestInfoFrameCompatibility (InitParams_p);
    }
    if (OK(ErrorCode))
    {
        Device_p->DeviceType = InitParams_p->DeviceType;
        Device_p->OutputType = InitParams_p->OutputType;
        Device_p->CPUPartition_p = InitParams_p->CPUPartition_p;
    }
    if (OK(ErrorCode))
    {
        ErrorCode = InitInternalStruct (Device_p);
    }
#if defined(STHDMI_CEC)
    if (OK(ErrorCode))
    {
        ErrorCode = sthdmi_Register_Subscribe_Events(Device_p);
    }
    if (OK(ErrorCode))
    {
        ErrorCode = sthdmi_StartCECTask(Device_p);
    }
#endif

    return(ErrorCode);
} /* end of Init()*/
/*******************************************************************************
Name        : Open
Description : API specific actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Open function
*******************************************************************************/
static ST_ErrorCode_t Open(sthdmi_Unit_t * const Unit_p)
{
        ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
        UNUSED_PARAMETER (Unit_p);
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
static ST_ErrorCode_t Close(sthdmi_Unit_t * const Unit_p)
{
     ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
     UNUSED_PARAMETER (Unit_p);
     return(ErrorCode);
} /* End of Close() function */

/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(sthdmi_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#if defined(STHDMI_CEC)
    if (OK(ErrorCode))
    {
        ErrorCode = sthdmi_StopCECTask(Device_p);
    }

    if (OK(ErrorCode))
    {
        ErrorCode = sthdmi_UnRegister_UnSubscribe_Events(Device_p);
    }
#endif
    if (OK(ErrorCode))
    {
        ErrorCode = TermInternalStruct(Device_p);
    }

       /* closes all instances opened */
        switch (Device_p->DeviceType)
        {
            case STHDMI_DEVICE_TYPE_7710 : /* no break */
            case STHDMI_DEVICE_TYPE_7100 : /* no break */
            case STHDMI_DEVICE_TYPE_7200 :
                if (OK(ErrorCode))
                {
                    ErrorCode = STVOUT_Close(Device_p->VoutHandle);
                }
                if (OK(ErrorCode))
                {
                    ErrorCode = STVTG_Close(Device_p->VtgHandle);
                }
                if (OK(ErrorCode))
                {
                    ErrorCode = STEVT_Close(Device_p->EvtHandle);
                }
                break;
            default :
                break;
        }

        return(ErrorCode);
} /* End of Term() function */

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
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STHDMI_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */

/*-----------------16/03/05 4:53PM------------------
 * test the validity of desired output,
 * input : Init parameter
 * output : Error code
 * --------------------------------------------------*/

static ST_ErrorCode_t TestOutputValidity(const STHDMI_InitParams_t * const InitParams_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (InitParams_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
             if (!((InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_RGB888) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422)))
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            break;
            default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return (ErrorCode);
}

/*-----------------16/03/05 4:58PM------------------
 * test the validity of EDID table,
 * input : Init parameter
 * output : Error code
 * --------------------------------------------------*/
/*static ST_ErrorCode_t TestEdidValidity(const STHDMI_InitParams_t * const InitParams_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (InitParams_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 :
        case STHDMI_DEVICE_TYPE_7100 :
             if (!((InitParams_p->EdidType == STHDMI_EDIDTIMING_EXT_TYPE_NONE)||
                   (InitParams_p->EdidType == STHDMI_EDIDTIMING_EXT_TYPE_VER_ONE)||
                   (InitParams_p->EdidType == STHDMI_EDIDTIMING_EXT_TYPE_VER_TWO)||
                   (InitParams_p->EdidType == STHDMI_EDIDTIMING_EXT_TYPE_VER_THREE)))
             {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
             }
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return (ErrorCode);
}                            */

/*-----------------16/03/05 4:58PM------------------
 * test the validity of AVI info frame,
 * input : Init parameter
 * output : Error code
 * --------------------------------------------------*/
static ST_ErrorCode_t TestAVIValidity(const STHDMI_InitParams_t * const InitParams_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (InitParams_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
             if (!((InitParams_p->AVIType == STHDMI_AVI_INFOFRAME_FORMAT_NONE)||
                  (InitParams_p->AVIType == STHDMI_AVI_INFOFRAME_FORMAT_VER_ONE)||
                 (InitParams_p->AVIType == STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO)))
             {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
             }
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return (ErrorCode);
}

/*-----------------16/03/05 4:58PM------------------
 * test the validity of SPD info frame,
 * input : Init parameter
 * output : Error code
 * --------------------------------------------------*/
static ST_ErrorCode_t TestSPDValidity(const STHDMI_InitParams_t * const InitParams_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (InitParams_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 :
        case STHDMI_DEVICE_TYPE_7100 :
        case STHDMI_DEVICE_TYPE_7200 :
             if (!((InitParams_p->SPDType == STHDMI_SPD_INFOFRAME_FORMAT_NONE)||
                  (InitParams_p->SPDType == STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE)))
             {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
             }
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return (ErrorCode);
}

/*-----------------16/03/05 4:58PM------------------
 * test the validity of MS info frame,
 * input : Init parameter
 * output : Error code
 * --------------------------------------------------*/
static ST_ErrorCode_t TestMSValidity(const STHDMI_InitParams_t * const InitParams_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    switch (InitParams_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
            if (!((InitParams_p->MSType == STHDMI_MS_INFOFRAME_FORMAT_NONE)||
                  (InitParams_p->MSType == STHDMI_MS_INFOFRAME_FORMAT_VER_ONE)))
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            break;
        default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;

            break;
     }
     return(ErrorCode);
}
/*-----------------16/03/05 4:58PM------------------
 * test the validity of Audio info frame,
 * input : Init parameter
 * output : Error code
 * --------------------------------------------------*/
static ST_ErrorCode_t TestAudioValidity(const STHDMI_InitParams_t * const InitParams_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    switch (InitParams_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
            if (!((InitParams_p->MSType == STHDMI_AUDIO_INFOFRAME_FORMAT_NONE)||
                 (InitParams_p->MSType == STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE)))
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            break;
       default :
               ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
}

/*-----------------16/03/05 4:58PM------------------
 * test the validity of Vendor specific info frame,
 * input : Init parameter
 * output : Error code
 * --------------------------------------------------*/
static ST_ErrorCode_t TestVSValidity(const STHDMI_InitParams_t * const InitParams_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (InitParams_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
             if (!((InitParams_p->VSType == STHDMI_VS_INFOFRAME_FORMAT_NONE)||
                   (InitParams_p->VSType == STHDMI_VS_INFOFRAME_FORMAT_VER_ONE)))
             {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
             }
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
}
/*-----------------17/03/05 17:01PM------------------
 * Check whether the info frame configuration is permitted.
 * input : info frame types
 * output : Error code
 * --------------------------------------------------*/

static ST_ErrorCode_t FindInfoFrameConfiguration (const InfoFrameType_t InfoFrame[])
{
    int i=0; int j=0;
    ST_ErrorCode_t   ErrorCode = ST_NO_ERROR;
    InfoFrameType_t  TmpInfoFrame[STHDMI_MAX_INFOFRAME_TYPE];
    BOOL             IsConfigFound = FALSE;
    BOOL             IsCompatible = TRUE;

    while ((i <= STHDMI_MAX_INFOFRAME_CONF)&&(!IsConfigFound))
    {
        memcpy(TmpInfoFrame,CheckInfoFrame[i], sizeof(InfoFrameType_t)*STHDMI_MAX_INFOFRAME_TYPE);

        while ((j <STHDMI_MAX_INFOFRAME_TYPE) &&(IsCompatible))
        {
            if (TmpInfoFrame[j]!=InfoFrame[j])
            {
             IsCompatible =FALSE;
             break; /* jump to the next config */
            }
            j++;
        }
        if ((j == STHDMI_MAX_INFOFRAME_TYPE)&&(IsCompatible))
        {
            IsConfigFound=TRUE;
            break; /* Configuration found */
        }
        else
        {
            j=0;i++;IsCompatible=TRUE;
        }
    }
    if (!IsConfigFound)
    {
        ErrorCode = STHDMI_ERROR_INVALID_PACKET_FORMAT;
    }
    return(ErrorCode);
} /* end of FindInfoFrameConfiguration */
/*-----------------17/03/05 17:01PM------------------
 * test the compatibity between the info frames desired,
 * and the info frames already programmed.
 * input : init parameter
 * output : Error code
 * --------------------------------------------------*/
static ST_ErrorCode_t TestInfoFrameCompatibility(const STHDMI_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    InfoFrameType_t InfoFrameProgrammed[5];

    U32 Index =0;
    switch (InitParams_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
                     switch (DeviceArray[Index].AVIType)
                     {
                        case STHDMI_AVI_INFOFRAME_FORMAT_NONE :
                             InfoFrameProgrammed[0] = INFO_FRAME_NONE;
                             break;
                        case STHDMI_AVI_INFOFRAME_FORMAT_VER_ONE :
                             InfoFrameProgrammed[0] = INFO_FRAME_VER_ONE;
                             break;
                         case STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO :
                             InfoFrameProgrammed[0] = INFO_FRAME_VER_TWO;
                             break;
                         default:
                             break;
                     }
                     switch (DeviceArray[Index].SPDType)
                     {
                        case STHDMI_SPD_INFOFRAME_FORMAT_NONE :
                             InfoFrameProgrammed[1] = INFO_FRAME_NONE;
                             break;
                        case STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE :
                             InfoFrameProgrammed[1] = INFO_FRAME_VER_ONE;
                             break;
                        default :
                            break;
                     }
                    switch (DeviceArray[Index].MSType)
                    {
                        case STHDMI_MS_INFOFRAME_FORMAT_NONE :
                             InfoFrameProgrammed[2] = INFO_FRAME_NONE;
                             break;
                        case STHDMI_MS_INFOFRAME_FORMAT_VER_ONE :
                             InfoFrameProgrammed[2] = INFO_FRAME_VER_ONE;
                             break;
                        default :
                            break;
                     }
                     switch (DeviceArray[Index].AUDIOType)
                     {
                        case STHDMI_AUDIO_INFOFRAME_FORMAT_NONE :
                             InfoFrameProgrammed[3] = INFO_FRAME_NONE;
                             break;
                        case STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE :
                             InfoFrameProgrammed[3] = INFO_FRAME_VER_ONE;
                             break;
                        default :
                            break;
                     }
                     switch (DeviceArray[Index].VSType)
                     {
                        case STHDMI_VS_INFOFRAME_FORMAT_NONE :
                             InfoFrameProgrammed[4] = INFO_FRAME_NONE;
                             break;
                        case STHDMI_VS_INFOFRAME_FORMAT_VER_ONE :
                             InfoFrameProgrammed[4] = INFO_FRAME_VER_ONE;
                             break;
                        default :
                            break;
                     }
                     ErrorCode = FindInfoFrameConfiguration  (InfoFrameProgrammed);
            break;
        default :
            break;
    }

    return(ErrorCode);
}

/* ----------------------------------------------------------------------------------------------------------
 *   Available Info Frames :
 *
 * Device     AVIinfoFrames   SPDinfoFrame   AudioinfoFrame  MSinfoFrame  VSinfoFrame  Remarks
 * 7710       AVI ver 2       SPD ver 1     Audio ver 1      MS ver 1     VS ver 1     Support of EIA/CEA861B.
 * 710X       AVI ver 2       SPD ver 1     Audio ver 1      MS ver 1     VS ver 1     Support of EIA/CEA861B.
 * 7200       AVI ver 2       SPD ver 1     Audio ver 1      MS ver 1     VS ver 1     Support of EIA/CEA861B.
 * SIL9190    AVI ver1        none          none             none         none         Support of EIA/CEA861A.
 * others     none            none          none             none         none         No EIA/CEA support.
 * --------------------------------------------------------------------------------------------------------- */

static ST_ErrorCode_t TestAvailableInfoFrame(const STHDMI_InitParams_t  * const InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    switch (InitParams_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
             /*if (!((InitParams_p->AVIType == STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO)&&
                   (InitParams_p->SPDType == STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE)&&
                   (InitParams_p->MSType == STHDMI_MS_INFOFRAME_FORMAT_VER_ONE)&&
                   (InitParams_p->AUDIOType == STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE)&&
                   (InitParams_p->VSType == STHDMI_VS_INFOFRAME_FORMAT_VER_ONE)))

             {
                  ErrorCode = STHDMI_ERROR_NOT_AVAILABLE;
             }                    */
            break;
       default :
            if (!((InitParams_p->AVIType == STHDMI_AVI_INFOFRAME_FORMAT_NONE)&&
                   (InitParams_p->SPDType == STHDMI_SPD_INFOFRAME_FORMAT_NONE)&&
                   (InitParams_p->MSType == STHDMI_MS_INFOFRAME_FORMAT_NONE)&&
                   (InitParams_p->AUDIOType == STHDMI_AUDIO_INFOFRAME_FORMAT_NONE)&&
                   (InitParams_p->VSType == STHDMI_VS_INFOFRAME_FORMAT_NONE)))

             {
                  ErrorCode = STHDMI_ERROR_NOT_AVAILABLE;
             }
            break;
    }
    return(ErrorCode);
}

/* -----------------------------------------------------------------
 * returns informations on the capability supported by the interface.
 * input : device (internal driver structure), capability (pointer)
 * output : capability
 * ----------------------------------------------------------------- */

static void GetCapability( sthdmi_Device_t *Device_p, STHDMI_Capability_t *Capability_p)
{
   switch (Device_p->DeviceType)
   {

    case STHDMI_DEVICE_TYPE_7710 : /*no break*/ /* HDMI Interface 's supported outputs */
    case STHDMI_DEVICE_TYPE_7100 : /* no break */
    case STHDMI_DEVICE_TYPE_7200 :
         Capability_p->SupportedOutputs = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888|
                                          STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444|
                                          STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422;
         Capability_p->IsEDIDDataSupported =TRUE;
         Capability_p->IsDataIslandSupported =TRUE;
         Capability_p->IsVideoSupported =TRUE;
         Capability_p->IsAudioSupported =TRUE;

         break;
    default :
        break;
   }

} /* end of GetCapability() */

/* -----------------------------------------------------------------
 * returns informations on the capability supported by the Source.
 * input : device (internal driver structure), capability (pointer)
 * output : capability
 * ----------------------------------------------------------------- */
static void GetSourceCapability( sthdmi_Device_t *Device_p, STHDMI_SourceCapability_t *Capability_p)
{


  Capability_p->AVIInfoFrameSelected = Device_p->AVIType;
  Capability_p->SPDInfoFrameSelected = Device_p->SPDType;
  Capability_p->MSInfoFrameSelected = Device_p->MSType;
  Capability_p->AudioInfoFrameSelected = Device_p->AUDIOType;
  Capability_p->VSInfoFrameSelected= Device_p->VSType;

  switch (Device_p->DeviceType)
  {
    case STHDMI_DEVICE_TYPE_7710 :/* no break */
    case STHDMI_DEVICE_TYPE_7100 : /*no break */
    case STHDMI_DEVICE_TYPE_7200 :
         Capability_p->AVIInfoFrameSupported = STHDMI_AVI_INFOFRAME_FORMAT_VER_ONE|
                                               STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO;

         Capability_p->SPDInfoFrameSupported = STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE;

         Capability_p->MSInfoFrameSupported  = STHDMI_MS_INFOFRAME_FORMAT_VER_ONE;
         Capability_p->AudioInfoFrameSupported = STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE;
         Capability_p->VSInfoFrameSupported = STHDMI_VS_INFOFRAME_FORMAT_VER_ONE;
        break;
    default :
          Capability_p->AVIInfoFrameSupported = STHDMI_AVI_INFOFRAME_FORMAT_NONE;                                               ;
          Capability_p->SPDInfoFrameSupported = STHDMI_SPD_INFOFRAME_FORMAT_NONE;
          Capability_p->MSInfoFrameSupported  = STHDMI_MS_INFOFRAME_FORMAT_NONE;
          Capability_p->AudioInfoFrameSupported = STHDMI_AUDIO_INFOFRAME_FORMAT_NONE;
          Capability_p->VSInfoFrameSupported = STHDMI_VS_INFOFRAME_FORMAT_NONE;
        break;
  }
} /* end of GetSourceCapability() */

/* -----------------------------------------------------------------
 * returns informations on the capability supported by the Monitor.
 * input : device (internal driver structure), capability (pointer)
 * output : capability
 * ----------------------------------------------------------------- */
static void GetSinkCapability( sthdmi_Device_t *Device_p, STHDMI_SinkCapability_t *Capability_p)
{
  Capability_p->IsLCDTimingDataSupported=FALSE;
  Capability_p->IsColorInfoSupported =FALSE;
  Capability_p->IsDviDataSupported =FALSE;
  Capability_p->IsTouchScreenDataSupported =FALSE;

  switch (Device_p->DeviceType)
  {
    case STHDMI_DEVICE_TYPE_7710 : /* no break */
    case STHDMI_DEVICE_TYPE_7100 : /* no break */
    case STHDMI_DEVICE_TYPE_7200 :
         Capability_p->IsBasicEDIDSupported=TRUE;
         /* If the driver is capable of supporting Sink EDID extension version 3,
          * it was backword compatible to earlier ones (ie version 1 and 2)*/
         Capability_p->EDIDBasicVersion =1;
         Capability_p->EDIDBasicRevision =3;

         /* If the driver is capable of supporting Sink EDID extension version 3,
          * it was backword compatible to earlier ones (ie version 1 and 2)*/
         Capability_p->EDIDExtRevision=3;

         /* Several EDID extension were not yet been defined by VESA.
          * Only Additional data and map block were supported by the driver */
         Capability_p->IsAdditionalDataSupported=TRUE;
         Capability_p->IsBlockMapSupported =TRUE;

         break;
    default :

         break;
  }

} /* end of GetSinkCapability() */

/*
******************************************************************************
Public Functions
******************************************************************************
*/

/*
--------------------------------------------------------------------------------
Get revision of sthdmi API
--------------------------------------------------------------------------------
*/
ST_Revision_t STHDMI_GetRevision(void)
{

    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
     */

    return(HDMI_Revision);
} /* End of STHDMI_GetRevision() function */

/*
--------------------------------------------------------------------------------
Get capabilities of xxxxx API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STHDMI_GetCapability(const ST_DeviceName_t DeviceName, STHDMI_Capability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!(FirstInitDone))
    {
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
            GetCapability( &DeviceArray[DeviceIndex], Capability_p);
        }
    }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STHDMI_GetCapability() function */

/*
--------------------------------------------------------------------------------
Get Source capabilities of xxxxx API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STHDMI_GetSourceCapability(const ST_DeviceName_t DeviceName, STHDMI_SourceCapability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!(FirstInitDone))
    {
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
            /* Fill Source capability structure */
            GetSourceCapability( &DeviceArray[DeviceIndex], Capability_p);
        }
    }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STHDMI_GetSourceCapability() function */

/*
--------------------------------------------------------------------------------
Get Monitor capabilities of xxxxx API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STHDMI_GetSinkCapability(const ST_DeviceName_t DeviceName, STHDMI_SinkCapability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!(FirstInitDone))
    {
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
            /* Fill Sink capability structure */
            GetSinkCapability( &DeviceArray[DeviceIndex], Capability_p);
        }
    }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STHDMI_GetSinkCapability() function */

/*
--------------------------------------------------------------------------------
Initialise xxxxx driver
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STHDMI_Init(const ST_DeviceName_t DeviceName, const STHDMI_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    char Msg[100];

    sprintf( Msg, "STHDMI_Init :");
    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||                        /* There must be parameters */
        (InitParams_p->MaxOpen > STHDMI_MAX_OPEN) ||     /* No more than MAX_OPEN open handles supported */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0') ||    /* Device name should not be empty */
        (InitParams_p->CPUPartition_p == NULL)
       )
    {
        sthdmi_Report( Msg, ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        /* Allocate dynamic data structure */
        DeviceArray = (sthdmi_Device_t *) memory_allocate(InitParams_p->CPUPartition_p, STHDMI_MAX_DEVICE * sizeof(sthdmi_Device_t));
        /*AddDynamicDataSize(STHDMI_MAX_DEVICE * sizeof(sthdmi_Device_t)); */
        if (DeviceArray == NULL)
        {
            /* Error of allocation */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to allocate memory for device structure !"));
            return(ST_ERROR_NO_MEMORY);
        }
        sthdmi_UnitArray = (sthdmi_Unit_t *) memory_allocate(InitParams_p->CPUPartition_p, STHDMI_MAX_UNIT * sizeof(sthdmi_Unit_t));
        /*AddDynamicDataSize(STHDMI_MAX_UNIT * sizeof(sthdmi_Unit_t));  */
        if (sthdmi_UnitArray == NULL)
        {
            /* Error of allocation */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to allocate memory for unit structure !"));
            return(ST_ERROR_NO_MEMORY);
        }
        for (Index = 0; Index < STHDMI_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
            /*DeviceArray[Index].OutputType = (STHDMI_OutputFormat_t)0;*/
        }

        for (Index = 0; Index < STHDMI_MAX_UNIT; Index++)
        {
            sthdmi_UnitArray[Index].UnitValidity = 0;
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
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STHDMI_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STHDMI_MAX_DEVICE)
        {
            /* All devices initialised */
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            ErrorCode = Init(&DeviceArray[Index], InitParams_p);

            if (OK(ErrorCode))
            {
                /* Device available and successfully initialised: register device name */
                strcpy(DeviceArray[Index].DeviceName, DeviceName);
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                DeviceArray[Index].MaxOpen = InitParams_p->MaxOpen;

                sprintf( Msg, "STHDMI_Init Device '%s' :",
                        DeviceArray[Index].DeviceName);
            }
        } /* End exists device not initialised */
    } /* End device not already initialised */

    LeaveCriticalSection();

    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* End of STHDMI_Init() function */

/*
--------------------------------------------------------------------------------
open ...
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STHDMI_Open(const ST_DeviceName_t DeviceName, const STHDMI_OpenParams_t * const OpenParams_p, STHDMI_Handle_t *Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    char Msg[100];

    sprintf( Msg, "STHDMI_Open :");
    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        EnterCriticalSection();

        if (!FirstInitDone)
        {
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
                /* Look for a free unit and check all opened units to check if MaxOpen is not reached */
                UnitIndex = STHDMI_MAX_UNIT;
                OpenedUnitForThisInit = 0;
                for (Index = 0; Index < STHDMI_MAX_UNIT; Index++)
                {
                    if ((sthdmi_UnitArray[Index].UnitValidity == STHDMI_VALID_UNIT) && (sthdmi_UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                    {
                        OpenedUnitForThisInit ++;
                    }
                    if (sthdmi_UnitArray[Index].UnitValidity != STHDMI_VALID_UNIT)
                    {
                        /* Found a free handle structure */
                        UnitIndex = Index;
                    }
                }
                if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxOpen) || (UnitIndex >= STHDMI_MAX_UNIT))
                {
                    /* None of the units is free or MaxOpen reached */
                    ErrorCode = ST_ERROR_NO_FREE_HANDLES;
                }
                else
                {
                    *Handle_p = (STHDMI_Handle_t) &sthdmi_UnitArray[UnitIndex];
                    sthdmi_UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex];

                    /* API specific actions after opening */
                    ErrorCode = Open(&sthdmi_UnitArray[UnitIndex]);

                    if (OK(ErrorCode))
                    {
                        /* Register opened handle */
                        sthdmi_UnitArray[UnitIndex].UnitValidity = STHDMI_VALID_UNIT;
                        sprintf( Msg, "STHDMI_Open Device '%s' :", DeviceName);
                    }
                } /* End found unit unused */
            } /* End device valid */
        } /* End FirstInitDone */

        LeaveCriticalSection();
    }

    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* End fo STHDMI_Open() function */

/*
--------------------------------------------------------------------------------
Close ...
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STHDMI_Close(STHDMI_Handle_t Handle)
{
    sthdmi_Unit_t *Unit_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    char Msg[100];

    sprintf( Msg, "STHDMI_Close :");
    EnterCriticalSection();

    if (!(FirstInitDone))
    {
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
            Unit_p = (sthdmi_Unit_t *) Handle;

            /* API specific actions before closing */
            ErrorCode = Close(Unit_p);

            /* Close only if no errors */
            if (OK(ErrorCode))
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;

                sprintf( Msg, "STHDMI_Close Device '%s' :", Unit_p->Device_p->DeviceName);
            }
        } /* End handle valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* End of STHDMI_Close() function */

/*
--------------------------------------------------------------------------------
Terminate xxxxx driver
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STHDMI_Term(const ST_DeviceName_t DeviceName, const STHDMI_TermParams_t *const TermParams_p)
{
    sthdmi_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    char Msg[100];

    sprintf( Msg, "STHDMI_Term :");
    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        EnterCriticalSection();

        if (!FirstInitDone)
        {
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
                UnitIndex = 0;
                Unit_p = sthdmi_UnitArray;
                while ((UnitIndex < STHDMI_MAX_UNIT) && (!Found))
                {
                    Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STHDMI_VALID_UNIT));
                    Unit_p++;
                    UnitIndex++;
                }

                if (Found)
                {
                    if (TermParams_p->ForceTerminate)
                    {
                        UnitIndex = 0;
                        Unit_p = sthdmi_UnitArray;
                        while (UnitIndex < STHDMI_MAX_UNIT)
                        {
                            if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STHDMI_VALID_UNIT))
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

                                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
                            }
                            Unit_p++;
                            UnitIndex++;
                        }
                    } /* End ForceTerminate: closed all opened handles */
                    else
                    {
                        /* Can't term if there are handles still opened, and ForceTerminate not set */
                        ErrorCode = ST_ERROR_OPEN_HANDLE;
                    }
                } /* End found handle not closed */

                /* Terminate if OK */
                if (OK(ErrorCode))
                {
                    /* API specific terminations */
                    ErrorCode = Term(&DeviceArray[DeviceIndex]);
                    /* Don't leave instance semi-terminated: terminate as much as possible */
                    /* free device */
                    DeviceArray[DeviceIndex].DeviceName[0] = '\0';
                    sprintf( Msg, "STHDMI_Term Device '%s' :", DeviceName);

                } /* End terminate OK */
            } /* End valid device */
        } /* End FirstInitDone */

        LeaveCriticalSection();
    }

    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* End of STVOUT_Term() function */

/*-----------------5/23/01 12:06PM------------------
 * display informations, failed or ok.
 * input : string to display, Error code
 * output : none
 * --------------------------------------------------*/
void sthdmi_Report( char *stringfunction, ST_ErrorCode_t ErrorCode)
{
    char Msg[100];

    sprintf( Msg, stringfunction);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s   failed, error 0x%x", Msg, ErrorCode));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s   Done", Msg));

    }
}

/* ----------------------------- End of file ------------------------------ */




