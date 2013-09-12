/*******************************************************************************

File name   : hdmi_drv.c

Description : hdmi module commands

COPYRIGHT (C) STMicroelectronics 2005.

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

#include "sthdmi.h"
#include "hdmi_drv.h"
#include "hdmi_src.h"
#include "hdmi_snk.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t TestColorSpaceValidity(sthdmi_Device_t * const Device_p);
static ST_ErrorCode_t FillAVIFrame( sthdmi_Unit_t * Unit_p,  STHDMI_AVIInfoFrame_t* const AviFrame_p);
static ST_ErrorCode_t FillAudioFrame( sthdmi_Unit_t * Unit_p,  STHDMI_AUDIOInfoFrame_t* const AudioFrame_p);
static ST_ErrorCode_t FillSPDFrame( sthdmi_Unit_t * Unit_p,  STHDMI_SPDInfoFrame_t* const SPDFrame_p);
static ST_ErrorCode_t FillMSFrame( sthdmi_Unit_t * Unit_p,  STHDMI_MPEGSourceInfoFrame_t* const MSFrame_p);
#if defined(STHDMI_CEC)
static BOOL sthdmi_CEC_Messages_Equal(STVOUT_CECMessage_t Message1, STVOUT_CECMessage_t Message2);
void sthdmi_CEC_CallBack(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
     STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
void sthdmi_CEC_LogicalAddress_CallBack(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
     STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
void sthdmi_HPD_CallBack(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
     STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
static void  CEC_TaskFunction (sthdmi_Device_t * const Device_p);
#endif
/* Functions ---------------------------------------------------------------- */

#if defined(STHDMI_CEC)
/*-----------------31/01/07 10:53AM------------------
 * Tests if Two messages have same content
 * --------------------------------------------------*/
static BOOL sthdmi_CEC_Messages_Equal(STVOUT_CECMessage_t Message1, STVOUT_CECMessage_t Message2)
{
    U8      index;
    BOOL    Equal = TRUE;

    if(Message1.Source != Message2.Source)
        {
            Equal = FALSE;
        }
    if(Message1.Destination != Message2.Destination)
        {
            Equal = FALSE;
        }
    if(Message1.DataLength != Message2.DataLength)
        {
            Equal = FALSE;
        }
    if(Equal)
        {
            index = 0;
            while(Equal && (index < Message1.DataLength))
            {
                if(Message1.Data[index] != Message2.Data[index])
                {
                    Equal = FALSE;
                }
                index++;
            }
        }
    return(Equal);
} /* end of sthdmi_CEC_Messages_Equal() */

/*-----------------30/01/07 09:35AM------------------
 * CallBack of the CEC messages events
 * --------------------------------------------------*/
void sthdmi_CEC_CallBack(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
     STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    sthdmi_Device_t * Device_p;
    STVOUT_CECMessageInfo_t *  MessageInfo_p;
    Device_p = (sthdmi_Device_t *) SubscriberData_p;
    MessageInfo_p = (STVOUT_CECMessageInfo_t *) EventData_p;


    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    Device_p->CEC_Params.CurrentReceivedMessageInfo = (*MessageInfo_p);
    Device_p->CEC_Params.Notify |= CEC_NOTIFY_MESSAGE;
    STOS_SemaphoreSignal(Device_p->CEC_Sem_p);

} /* End of sthdmi_CEC_CallBack() */

/*-----------------05/03/07  2:11PM------------------
 * CallBack of the Logical Address Allocated event
 * --------------------------------------------------*/
void sthdmi_CEC_LogicalAddress_CallBack(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
     STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    sthdmi_Device_t * Device_p;
    STVOUT_CECDevice_t CECDevice;
    Device_p = (sthdmi_Device_t *) SubscriberData_p;
    CECDevice = (* (STVOUT_CECDevice_t *) EventData_p);

    Device_p->CEC_Params.CECDevice = CECDevice;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    Device_p->CEC_Params.Notify |= CEC_NOTIFY_LOGIC_ADDRESS;
    STOS_SemaphoreSignal(Device_p->CEC_Sem_p);

} /* End of sthdmi_CEC_CallBack() */

/*-----------------30/01/07 04:16PM------------------
 * CallBack of the HPD Message events
 * --------------------------------------------------*/
void sthdmi_HPD_CallBack(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
     STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    sthdmi_Device_t * Device_p;
    STVOUT_State_t             VOUT_State;
    Device_p = (sthdmi_Device_t *) SubscriberData_p;
    VOUT_State = *(STVOUT_State_t *)EventData_p;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    switch(VOUT_State)
    {
        case STVOUT_NO_RECEIVER         :
            Device_p->CEC_Params.IsEDIDRetrived = FALSE;
            Device_p->CEC_Params.IsPhysicalAddressValid = FALSE;
            break;
        case STVOUT_RECEIVER_CONNECTED  :
            Device_p->CEC_Params.Notify |= CEC_NOTIFY_HPD;
            STOS_SemaphoreSignal(Device_p->CEC_Sem_p);
            break;
        default : /* Not Concerned */
            break;
    }
} /* End of sthdmi_HPD_CallBack() */

/*-----------------29/01/07 04:36PM------------------
 * Manages all used and generated event
 * --------------------------------------------------*/
ST_ErrorCode_t sthdmi_Register_Subscribe_Events(sthdmi_Device_t * const Device_p)
{
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STEVT_SubscribeParams.SubscriberData_p = (void*)Device_p;

    STEVT_RegisterDeviceEvent(  Device_p->EvtHandle, Device_p->VtgName,
                                STHDMI_CEC_COMMAND_EVT, &(Device_p->RegisteredEventsID[HDMI_CEC_COMMAND_EVT_ID]));
    if(ErrorCode == ST_NO_ERROR)
    {
        STEVT_SubscribeParams.NotifyCallback   = (STEVT_DeviceCallbackProc_t)sthdmi_CEC_CallBack;
        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->EvtHandle, (char*)Device_p->VtgName, STVOUT_CEC_MESSAGE_EVT, &STEVT_SubscribeParams);
    }
    if(ErrorCode == ST_NO_ERROR)
    {
        STEVT_SubscribeParams.NotifyCallback   = (STEVT_DeviceCallbackProc_t)sthdmi_CEC_LogicalAddress_CallBack;
        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->EvtHandle, (char*)Device_p->VtgName, STVOUT_CEC_LOGIC_ADDRESS_EVT, &STEVT_SubscribeParams);
    }

    if(ErrorCode == ST_NO_ERROR)
    {
        STEVT_SubscribeParams.NotifyCallback   = (STEVT_DeviceCallbackProc_t)sthdmi_HPD_CallBack;
        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->EvtHandle, (char*)Device_p->VtgName, STVOUT_CHANGE_STATE_EVT, &STEVT_SubscribeParams);
    }

    return(ErrorCode);
} /* sthdmi_Register_Subscribe_Events() */

/*-----------------31/01/07 12:08AM------------------
 * Manages all used and generated event
 * --------------------------------------------------*/
ST_ErrorCode_t sthdmi_UnRegister_UnSubscribe_Events(sthdmi_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if(ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_UnsubscribeDeviceEvent(Device_p->EvtHandle, (char*)Device_p->VtgName, STVOUT_CHANGE_STATE_EVT);
    }
    if(ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_UnsubscribeDeviceEvent(Device_p->EvtHandle, (char*)Device_p->VtgName, STVOUT_CEC_MESSAGE_EVT);
    }
    if(ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_UnsubscribeDeviceEvent(Device_p->EvtHandle, (char*)Device_p->VtgName, STVOUT_CEC_LOGIC_ADDRESS_EVT);
    }
    STEVT_UnregisterDeviceEvent(Device_p->EvtHandle, Device_p->VtgName, STHDMI_CEC_COMMAND_EVT);

    return(ErrorCode);

} /* end of sthdmi_UnRegister_UnSubscribe_Events() */

/*-----------------29/01/07 03:19PM------------------
 * CEC Task Function
 * --------------------------------------------------*/
static void  CEC_TaskFunction (sthdmi_Device_t * const Device_p)
{

U8      index = 0;
BOOL    IsFound = FALSE;
BOOL    IsReceived = FALSE;
BOOL    IsCommand_Handled = FALSE;
STHDMI_CEC_Command_t        Command;
STHDMI_CEC_CommandInfo_t    CommandInfo;
ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

STOS_TaskEnter(NULL);

    STOS_TaskDelay(ST_GetClocksPerSecond());/* Let Setup Process before starting Action */

    do
    {
        STOS_SemaphoreWait(Device_p->CEC_Sem_p);

        if(Device_p->CEC_Params.Notify & CEC_NOTIFY_MESSAGE)
        {
            switch(Device_p->CEC_Params.CurrentReceivedMessageInfo.Status)
            {
                case STVOUT_CEC_STATUS_TX_SUCCEED :
                case STVOUT_CEC_STATUS_TX_FAILED :
                    /*find Find the notified MESSAGE in local BUFFER */
                    STOS_SemaphoreWait(Device_p->CECStruct_Access_Sem_p);
                        while( (index < CEC_MESSAGES_BUFFER_SIZE)&&(!IsFound) )
                        {
                            /*compare Messages */
                            if(!Device_p->CEC_Params.CEC_Cmd_Msg_Buffer[index].IsCaseFree)
                            {
                                if(sthdmi_CEC_Messages_Equal(Device_p->CEC_Params.CEC_Cmd_Msg_Buffer[index].Message, Device_p->CEC_Params.CurrentReceivedMessageInfo.Message))
                                {
                                    IsFound = TRUE;
                                }
                            }
                            index++;
                        }
                        if(IsFound)
                        {
                            /* Remove the message from the Buffer */
                            Device_p->CEC_Params.CEC_Cmd_Msg_Buffer[index].IsCaseFree = TRUE;
                            /* Also take the Command you need to notify before releasing the Access semaphore */
                            Command = Device_p->CEC_Params.CEC_Cmd_Msg_Buffer[index].Command;
                        }
                        else
                        {
                            /* Message not found, we were not asked to send this message, so we don't have to notify a command out of it */
                        }
                    STOS_SemaphoreSignal(Device_p->CECStruct_Access_Sem_p);
                    break;
                case STVOUT_CEC_STATUS_RX_SUCCEED :
                    /* We have to generate the command out of the binary message */
                    ErrorCode = sthdmi_CECParseMessage(Device_p, &(Device_p->CEC_Params.CurrentReceivedMessageInfo.Message), &Command);
                    if ( ErrorCode == ST_NO_ERROR )
                    {
                        IsReceived = TRUE;
                        IsCommand_Handled = sthdmi_HandleCECCommand(Device_p, &Command);
                    }
                    else
                    {
                        /* Command must be ignored */
                    }
                    /* if Error, we can notify the user by for example, couldn't parse message */
                    break;
                default :
                    break;
            }
            /* Then Generate Command_Info and notify it */
            if( IsFound || (IsReceived && !IsCommand_Handled) )
            {
                CommandInfo.Status = Device_p->CEC_Params.CurrentReceivedMessageInfo.Status;
                CommandInfo.Command = Command;
                /* Notify the user */
                #if defined ST_OSLINUX
                STEVT_Notify(Device_p->EvtHandle,
                                Device_p->RegisteredEventsID[HDMI_CEC_COMMAND_EVT_ID],
                                STEVT_EVENT_DATA_TYPE_CAST &CommandInfo);
                #else

                STEVT_Notify(Device_p->EvtHandle,
                                Device_p->RegisteredEventsID[HDMI_CEC_COMMAND_EVT_ID],
                                (const void*) &CommandInfo);
                #endif
            }
            Device_p->CEC_Params.Notify &= ~CEC_NOTIFY_MESSAGE;
        }

        /*if(Device_p->CEC_Params.Notify & CEC_NOTIFY_HPD)
        {*/
            /* Here we should get the EDID, parse the Physical Address, As we know it we would be able to answer querries about it */
            /* Get Vendor Specific Data Block - Byte [4,5] [A,B,C,D] */
            /* Update Device_p->CEC_Params. Structure */
            /*STVOUT_CECPhysicalAddressAvailable(Device_p->VoutHandle);
            Device_p->CEC_Params.Notify &= ~CEC_NOTIFY_HPD;
        }*/
        if(Device_p->CEC_Params.Notify & CEC_NOTIFY_EDID)
        {
            if(Device_p->CEC_Params.IsPhysicalAddressValid)
            {
                STVOUT_CECPhysicalAddressAvailable(Device_p->VoutHandle);
            }
            Device_p->CEC_Params.Notify &= ~CEC_NOTIFY_EDID;
        }
        if(Device_p->CEC_Params.Notify & CEC_NOTIFY_LOGIC_ADDRESS)
        {
            Command.Source = Device_p->CEC_Params.CECDevice.LogicalAddress;
            Command.Destination = 15; /*Broadcast*/
            Command.CEC_Opcode = STHDMI_CEC_OPCODE_REPORT_PHYSICAL_ADDRESS;
            Command.CEC_Operand.ReportPhysicalAddress.PhysicalAddress = Device_p->CEC_Params.PhysicalAddress;
            switch(Device_p->CEC_Params.CECDevice.Role)
            {
                case STVOUT_CEC_ROLE_TV :
                    Command.CEC_Operand.ReportPhysicalAddress.DeviceType   = STHDMI_CEC_DEVICE_TYPE_TV;
                    break;
                case STVOUT_CEC_ROLE_RECORDER :
                    Command.CEC_Operand.ReportPhysicalAddress.DeviceType   = STHDMI_CEC_DEVICE_TYPE_RECORDING_DEVICE;
                    break;
                case STVOUT_CEC_ROLE_TUNER :
                    Command.CEC_Operand.ReportPhysicalAddress.DeviceType   = STHDMI_CEC_DEVICE_TYPE_TUNER;
                    break;
                case STVOUT_CEC_ROLE_PLAYBACK :
                    Command.CEC_Operand.ReportPhysicalAddress.DeviceType   = STHDMI_CEC_DEVICE_TYPE_PLAYBACK_DEVICE;
                    break;
                case STVOUT_CEC_ROLE_AUDIO :
                    Command.CEC_Operand.ReportPhysicalAddress.DeviceType   = STHDMI_CEC_DEVICE_TYPE_AUDIO_SYSTEM;
                    break;
                default:
                    break;
            }
            Command.Retries = 2;
            sthdmi_CEC_Send_Command(Device_p, &Command);
            Device_p->CEC_Params.Notify &= ~CEC_NOTIFY_LOGIC_ADDRESS;
        }


    }while(!(Device_p->CEC_Task.ToBeDeleted));

STOS_TaskExit(NULL);
} /* end of CEC_TaskFunction() */

/*-----------------29/01/07 02:40PM------------------
 * Start Internal Task
 * --------------------------------------------------*/
ST_ErrorCode_t  sthdmi_StartCECTask(sthdmi_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode;
    HDMI_Task_t * const task_p  = &(Device_p->CEC_Task);

    ErrorCode = ST_NO_ERROR;

    if (task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }
    /* Params initialized before task is started */
    Device_p->CEC_Params.Notify = CEC_NOTIFY_CLEAR_ALL;
    Device_p->CEC_Params.IsEDIDRetrived = FALSE;
    Device_p->CEC_Params.IsPhysicalAddressValid = FALSE;

    task_p->ToBeDeleted = FALSE;
    ErrorCode = STOS_TaskCreate ((void (*) (void*))CEC_TaskFunction,
                        (void*)Device_p,
                        (Device_p->CPUPartition_p),
                         1024*16,
                        &(task_p->TaskStack),
                        (Device_p->CPUPartition_p),
                        &(task_p->Task_p),
                        &(task_p->TaskDesc),
                        STHDMI_TASK_CEC_PRIORITY,
                        "STHDMI[?].CECTask",
                        0 /*task_flags_high_priority_process*/);

    if(ErrorCode != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    task_p->IsRunning  = TRUE;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STHDMI CEC task created.\n"));

    return (ErrorCode);
} /* end of sthdmi_StartCECTask() */

/*-----------------31/01/07 12:11AM------------------
 * Stops Internal Task
 * --------------------------------------------------*/
ST_ErrorCode_t sthdmi_StopCECTask(sthdmi_Device_t * const Device_p)
{
    HDMI_Task_t * const task_p  = &(Device_p->CEC_Task);
    ST_ErrorCode_t          Error = ST_NO_ERROR;

    if (!(task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }
    /* End the function of the task here... */
    task_p->ToBeDeleted = TRUE;
    STOS_SemaphoreSignal(Device_p->CEC_Sem_p);
    STOS_TaskWait( &task_p->Task_p, TIMEOUT_INFINITY );
    Error = STOS_TaskDelete ( task_p->Task_p,
            Device_p->CPUPartition_p,
            task_p->TaskStack,
            Device_p->CPUPartition_p);
    if(Error != ST_NO_ERROR)
    {
        return(Error);
    }
    task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);

}/* end of sthdmi_StopCECTask() */
#endif /* STHDMI_CEC */

/*-----------------04/03/06 11:37AM------------------
 * Init Internal Structure
 * --------------------------------------------------*/
 ST_ErrorCode_t  InitInternalStruct  (sthdmi_Device_t * const Device_p)
 {
#ifdef STHDMI_CEC
    U8 i;
#endif
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
            Device_p->VideoLength=0;
            Device_p->AudioLength=0;
            Device_p->VendorLength=0;
            Device_p->EDIDExtensionFlag=0;
            memset(&Device_p->EDIDProductDesc,0, sizeof(STHDMI_EDIDProdDescription_t));
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
#ifdef STHDMI_CEC
            Device_p->CEC_Task.IsRunning = FALSE;
            Device_p->CEC_Task.ToBeDeleted = FALSE;
            Device_p->CEC_TaskStarted = FALSE;
            Device_p->CEC_Sem_p = STOS_SemaphoreCreateFifo(NULL,0); /* Task start Blocked */
            Device_p->CECStruct_Access_Sem_p = STOS_SemaphoreCreateFifo(NULL,1); /* Structure Starts Accessible */
            for(i = 0; i < CEC_MESSAGES_BUFFER_SIZE; i++)
            {
                Device_p->CEC_Params.CEC_Cmd_Msg_Buffer[i].IsCaseFree = TRUE;
            }
#endif
    return (ErrorCode);
} /* end of InitInternalStruct()*/

/*-----------------31/01/07 12:14AM------------------
 * Term Internal Structure
 * --------------------------------------------------*/
ST_ErrorCode_t  TermInternalStruct(sthdmi_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#ifdef STHDMI_CEC
            STOS_SemaphoreDelete(NULL,Device_p->CEC_Sem_p);
            STOS_SemaphoreDelete(NULL,Device_p->CECStruct_Access_Sem_p);
#else
    UNUSED_PARAMETER(Device_p);
#endif
    return (ErrorCode);
} /* end of TermInternalStruct() */

 /*-----------------16/03/05 4:58PM------------------
 * test the validity of desired Color Space,
 * input : Device parameter
 * output : Error code
 * --------------------------------------------------*/
static ST_ErrorCode_t TestColorSpaceValidity(sthdmi_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
             if (!((Device_p->Colorimetry== STVOUT_ITU_R_601)||
                   (Device_p->Colorimetry== STVOUT_ITU_R_709)))
             {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
             }
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return (ErrorCode);
} /* end of TestColorSpaceValidity()*/

/*-----------------01/07/05 12:21AM------------------
 * affect internal structure to prepare programmation
 * of AVI info Frames.
 * --------------------------------------------------*/
static ST_ErrorCode_t FillAVIFrame( sthdmi_Unit_t * Unit_p,  STHDMI_AVIInfoFrame_t* const AviFrame_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Unit_p->Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
            Unit_p->Device_p->Colorimetry = AviFrame_p->AVI861B.Colorimetry;
            ErrorCode = TestColorSpaceValidity(Unit_p->Device_p);

            if (ErrorCode == ST_NO_ERROR)
            {
                Unit_p->Device_p->OutputType = AviFrame_p->AVI861B.OutputType;
                Unit_p->Device_p->PictureScaling = AviFrame_p->AVI861B.PictureScaling;
                Unit_p->Device_p->ActiveAspectRatio = AviFrame_p->AVI861B.ActiveAspectRatio;
                Unit_p->Device_p->AspectRatio = AviFrame_p->AVI861B.AspectRatio;
            }
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return (ErrorCode);
} /* end of FillAVIFrame()*/
/*-----------------01/07/05 12:21AM------------------
 * affect internal structure to prepare programmation
 * of audio info Frames.
 * --------------------------------------------------*/
static ST_ErrorCode_t FillAudioFrame( sthdmi_Unit_t * Unit_p,  STHDMI_AUDIOInfoFrame_t* const AudioFrame_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    switch (Unit_p->Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 :  /* no break */
        case STHDMI_DEVICE_TYPE_7100 :  /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
            Unit_p->Device_p->SampleSize= AudioFrame_p->SampleSize;
            Unit_p->Device_p->LevelShift = AudioFrame_p->LevelShift;
            Unit_p->Device_p->DownmixInhibit = AudioFrame_p->DownmixInhibit;
            Unit_p->Device_p->ChannelCount  = AudioFrame_p->ChannelCount;
            Unit_p->Device_p->CodingType  = AudioFrame_p->CodingType;
            Unit_p->Device_p->SamplingFrequency  = AudioFrame_p->SamplingFrequency;
            Unit_p->Device_p->ChannelAlloc  = AudioFrame_p->ChannelAlloc;
            Unit_p->Device_p->DecoderObject = AudioFrame_p->DecoderObject;
            break;
        default :
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(Error);
 } /* end of FillAudioFrame()*/

 /*-----------------01/07/05 12:21AM------------------
 * affect internal structure to prepare programmation
 * of spd info Frames.
 * --------------------------------------------------*/
static ST_ErrorCode_t FillSPDFrame( sthdmi_Unit_t * Unit_p,  STHDMI_SPDInfoFrame_t* const SPDFrame_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    switch (Unit_p->Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
           strncpy(Unit_p->Device_p->VendorName, SPDFrame_p->VendorName,STHDMI_VENDORNAME_LENGTH);
           strncpy(Unit_p->Device_p->ProductDesc, SPDFrame_p->ProductDescription,STHDMI_PRODUCTDSC_LENGTH);
            Unit_p->Device_p->SrcDeviceInfo = SPDFrame_p->SourceDeviceInfo;
            break;
        default :
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(Error);
 } /* end of FillSPDFrame()*/

 /*-----------------01/07/05 12:21AM------------------
 * affect internal structure to prepare programmation
 * of MPEG Source info Frames.
 * --------------------------------------------------*/
static ST_ErrorCode_t FillMSFrame( sthdmi_Unit_t * Unit_p,  STHDMI_MPEGSourceInfoFrame_t* const MSFrame_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Unit_p->Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
            Unit_p->Device_p->MPEGBitRate= MSFrame_p->MPEGBitRate;
            Unit_p->Device_p->IsFieldRepeated = MSFrame_p->IsFieldRepeated;
            Unit_p->Device_p->MepgFrameType = MSFrame_p->MepgFrameType;
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
 } /* end of FillMSFrame()*/

/*-----------------2/23/2006 10:24AM----------------
 * affect internal structure to prepare programmation
* of VS info frames.
 * --------------------------------------------------*/
static ST_ErrorCode_t FillVSFrame( sthdmi_Unit_t * Unit_p,  STHDMI_VendorSpecInfoFrame_t* const VSInfoFrame_p)
            {
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Unit_p->Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
           Unit_p->Device_p->FrameLength = VSInfoFrame_p->FrameLength;
           Unit_p->Device_p->RegistrationId = VSInfoFrame_p->RegistrationId;
		   /*Copy of vendor Specific payload*/
          if (VSInfoFrame_p->VSPayload_p!=NULL)
          {
            memcpy(Unit_p->Device_p->VSPayload, VSInfoFrame_p->VSPayload_p, STHDMI_VSPAYLOAD_LENGTH);
          }

          break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
 } /* end of FillVSFrame()*/

/*
******************************************************************************
Public Functions
******************************************************************************
*/
/*******************************************************************************
Name        :   STHDMI_FillAVIInfoframe
Description :   Collect Auxilary information, Fill AVI info frames
                compliant with EIA/CEA861, EIA/CEA861A and EIA/CEA861B and
                Send this info frame.
Parameters  :   Handle , AVIInfoFrame(pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t  STHDMI_FillAVIInfoFrame(
                const STHDMI_Handle_t                Handle,
                STHDMI_AVIInfoFrame_t*               const   AVIInfoFrame_p
                )

{
    ST_ErrorCode_t  ErrorCode;
    sthdmi_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STHDMI_FillAVIInfoFrame :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (sthdmi_Unit_t *) Handle;
        if (AVIInfoFrame_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {   ErrorCode = FillAVIFrame(Unit_p, AVIInfoFrame_p);
            if (ErrorCode != ST_NO_ERROR)
              return ErrorCode;

            ErrorCode = sthdmi_FillAviInfoFrame(Unit_p, AVIInfoFrame_p);
        }
    }

    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* end of STHDMI_FillAVIInfoframe() */
/*******************************************************************************
Name        :   STHDMI_FillSPDInfoframe
Description :   Collect Source product description information, Fill SPD info
                frames compliant with EIA/CEA861, EIA/CEA861A and EIA/CEA861B.
                and Send this info frame.
Parameters  :   Handle , SPDInfoFrame_p(pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t  STHDMI_FillSPDInfoFrame(
                const STHDMI_Handle_t         Handle,
                STHDMI_SPDInfoFrame_t*        const  SPDInfoFrame_p
                )

{
    ST_ErrorCode_t  ErrorCode;
    sthdmi_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STHDMI_FillSPDInfoFrame :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (sthdmi_Unit_t *) Handle;
        if (SPDInfoFrame_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
           ErrorCode = FillSPDFrame(Unit_p, SPDInfoFrame_p);
           if (ErrorCode != ST_NO_ERROR)
              return (ErrorCode);
           ErrorCode = sthdmi_FillSPDInfoFrame(Unit_p, SPDInfoFrame_p);

        }

    }
    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* end of STHDMI_FillSPDInfoframe() */
/*******************************************************************************
Name        :   STHDMI_FillAudioInfoframe
Description :   Collect Audio information, Fill Audio info frames
                compatible with EIA/CEA861, EIA/CEA861A, EIA/CEA861B
                and send this info frame.
Parameters  :   Handle , AudioInfoFrame_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t  STHDMI_FillAudioInfoFrame(
               const STHDMI_Handle_t          Handle,
               STHDMI_AUDIOInfoFrame_t*       const AudioInfoFrame_p
               )

{
    ST_ErrorCode_t  ErrorCode;
    sthdmi_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STHDMI_FillAudioInfoFrame :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (sthdmi_Unit_t *) Handle;
        if (AudioInfoFrame_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
           ErrorCode = FillAudioFrame(Unit_p,AudioInfoFrame_p);
           if (ErrorCode != ST_NO_ERROR)
              return ErrorCode;

           ErrorCode = sthdmi_FillAudioInfoFrame(Unit_p, AudioInfoFrame_p);
        }

    }
    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* end of STHDMI_FillAudioInfoframe() */

/*******************************************************************************
Name        :   STHDMI_FillMSInfoframe
Description :   Collect MPEG Source information, Fill MS info frames
                compatible with EIA/CEA861, EIA/CEA861A and EIA/CEA861B
                and send this info frame.
Parameters  :   Handle , MSInfoFrame_p(pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t  STHDMI_FillMSInfoFrame(
                const STHDMI_Handle_t                      Handle,
                STHDMI_MPEGSourceInfoFrame_t*              const  MSInfoFrame_p
                )

{
    ST_ErrorCode_t  ErrorCode;
    sthdmi_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STHDMI_FillMSInfoFrame :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (sthdmi_Unit_t *) Handle;
        if (MSInfoFrame_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            ErrorCode = FillMSFrame(Unit_p,MSInfoFrame_p);
            if (ErrorCode != ST_NO_ERROR)
              return ErrorCode;
            ErrorCode = sthdmi_FillMSInfoFrame(Unit_p, MSInfoFrame_p);
        }

    }
    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* end of STHDMI_FillMSInfoframe() */

/*******************************************************************************
Name        :   STHDMI_FillVSInfoframe
Description :   Collect vendor specific information, Fill VS info frames
                compatible with EIA/CEA861, EIA/CEA861A and EIA/CEA861B.
                and send this info frame.
Parameters  :   Handle , VSInfoFrame_p(pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STHDMI_FillVSInfoFrame(
               const STHDMI_Handle_t                      Handle,
               STHDMI_VendorSpecInfoFrame_t*              const  VSInfoFrame_p
               )
{
    ST_ErrorCode_t  ErrorCode;
    sthdmi_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STHDMI_FillVSInfoFrame :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (sthdmi_Unit_t *) Handle;
        if (VSInfoFrame_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            ErrorCode = FillVSFrame(Unit_p,VSInfoFrame_p);
            if (ErrorCode != ST_NO_ERROR)
              return ErrorCode;
            ErrorCode = sthdmi_FillVSInfoFrame(Unit_p, VSInfoFrame_p);
        }
    }
    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* end of STHDMI_FillVSInfoframe()*/

/*******************************************************************************
Name        :   STHDMI_FillSinkEDID
Description :   Fill EDID Table compatible with EIA/CEA861B.
Parameters  :   Handle , EdidBuffer_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STHDMI_FillSinkEDID(
               const STHDMI_Handle_t           Handle,
               STHDMI_EDIDSink_t* const   EdidBuffer_p
               )
{
    ST_ErrorCode_t  ErrorCode;
    sthdmi_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STHDMI_FillSinkEDID :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (sthdmi_Unit_t *) Handle;
        if (EdidBuffer_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            ErrorCode = sthdmi_FillSinkEDID(Unit_p, EdidBuffer_p);
        }

    }
    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);

} /*end of STHDMI_FillSinkEDID()*/

/*******************************************************************************
Name        :   STHDMI_SetOutputWindows
Description :   Set output window size and position
Parameters  :   Handle , OutputWindows_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STHDMI_SetOutputWindows(
               const STHDMI_Handle_t                 Handle,
               const STHDMI_OutputWindows_t*         const   OutputWindows_p
               )
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    sthdmi_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STHDMI_SetOutputWindows :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (sthdmi_Unit_t *) Handle;
        if (OutputWindows_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            Unit_p->Device_p->OutputWindows.OutputWinX = OutputWindows_p->OutputWinX;
            Unit_p->Device_p->OutputWindows.OutputWinY = OutputWindows_p->OutputWinY;
            Unit_p->Device_p->OutputWindows.OutputWinWidth = OutputWindows_p->OutputWinWidth;
            Unit_p->Device_p->OutputWindows.OutputWinHeight = OutputWindows_p->OutputWinHeight;
        }
    }
    sthdmi_Report( Msg, ErrorCode);

  return (ErrorCode);
}/* end of STHDMI_SetIOWindows()*/

/*******************************************************************************
Name        :   STHDMI_FreeEDIDMemory
Description :   Free Memory allocation.
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STHDMI_FreeEDIDMemory(
               const STHDMI_Handle_t                 Handle
               )
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    sthdmi_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STHDMI_FreeEDIDMemory :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (sthdmi_Unit_t *) Handle;
        ErrorCode = sthdmi_FreeEDIDMemory(Unit_p);
    }
    sthdmi_Report( Msg, ErrorCode);

  return (ErrorCode);
}/* end of STHDMI_SetIOWindows()*/

#if defined (STHDMI_CEC)
/*******************************************************************************
Name        :   STHDMI_CECSendCommand
Description :
Parameters  :   Handle , CEC_CommandInfo_p (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STHDMI_CECSendCommand(   const STHDMI_Handle_t Handle,
                                        STHDMI_CEC_Command_t*   const CEC_Command_p
                                    )

{
    ST_ErrorCode_t  ErrorCode;
    sthdmi_Unit_t   *Unit_p;
    char Msg[100];

    sprintf( Msg, "STHDMI_CECSendCommand :");
    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (sthdmi_Unit_t *) Handle;
        if (CEC_Command_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            ErrorCode = sthdmi_CEC_Send_Command(Unit_p->Device_p,CEC_Command_p);
        }
    }
    sthdmi_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* End of STHDMI_CECSendCommand() function */

#endif  /* STHDMI_CEC */
/* End of hdmi_drv.c */
/* ------------------------------- End of file ---------------------------- */


