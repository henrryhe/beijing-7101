/*******************************************************************************
File name   : hdmi_api.c

Description : VOUT driver contains API functions

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
08 Apr 2004         Created                                          AC
12 Nov 2004         Add Support of ST40/OS21                         AC
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#ifdef ST_OSLINUX
#include "stvout_core.h"
#endif    /* ST_OSLINUX */

#include "hdmi_api.h"
#include "hdmi_int.h"
#include "hdmi_ip.h"
#ifdef STVOUT_CEC_PIO_COMPARE
#include "cec.h"
#endif
/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */
#define HDMI_AUDIO_FREQUENCY  32000
/* Private variables (static) --------------------------------------------- */
HDMI_Data_t *HDMI_Driver_p;


/* Global Variables ------------------------------------------------------- */

BOOL    IsHDCPWAEnabled = FALSE;
/* Private Macros --------------------------------------------------------- */


/* Private Function prototypes -------------------------------------------- */
static void InitHdmiStructure (const HDMI_Handle_t Handle);
static void TermHdmiStructure (const HDMI_Handle_t Handle);
/* Exported Functions ----------------------------------------------------- */

/*******************************************************************************
Name        : VOUT_HDMI_Init
Description : Initialise a HDMI/DVI cell
Parameters  : Init params, pointer to hdmi handle returned if success
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if memory allocation fails
              STVID_ERROR_EVENT_REGISTRATION if problem registering events
*******************************************************************************/
ST_ErrorCode_t VOUT_HDMI_Init(const STVOUT_InitParams_t * const InitParams_p, HDMI_Handle_t * const HdmiHandle_p)
{
    HDMI_Data_t * HDMI_Data_p;
    STEVT_OpenParams_t STEVT_OpenParams;
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    HALHDMI_InitParams_t HALInitParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    char HdmiString[5];

    if ((HdmiHandle_p == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate a HDMI Data structure */
    HDMI_Data_p = (HDMI_Data_t *)memory_allocate(InitParams_p->CPUPartition_p, sizeof(HDMI_Data_t));
    if (HDMI_Data_p == NULL)
    {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }


    /* Allocation succeeded: make handle point on structure */
      HDMI_Driver_p = (HDMI_Data_t *)HDMI_Data_p;
     *HdmiHandle_p = (HDMI_Data_t *) HDMI_Data_p;
      HDMI_Data_p->ValidityCheck = HDMI_VALID_HANDLE;


     /* Remember partition */
     HDMI_Data_p->CPUPartition_p = InitParams_p->CPUPartition_p;

     /* Allocate Sink Buffer for EDID retrieval*/
     HDMI_Data_p->SinkBuffer_p = (U8*) memory_allocate(HDMI_Data_p->CPUPartition_p, MAX_BYTE_READ*128);
     if (HDMI_Data_p->SinkBuffer_p == NULL)
     {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
     }
     memset (HDMI_Data_p->SinkBuffer_p,0, MAX_BYTE_READ*128);

     /* Init HDMI structure */
     InitHdmiStructure(*HdmiHandle_p);

     /* Save some parameters in HDMI structure*/
     strncpy(HDMI_Data_p->VTGName, InitParams_p->Target.HDMICell.VTGName,sizeof(HDMI_Data_p->VTGName)-1);
#ifdef STVOUT_CEC_PIO_COMPARE
     strncpy(HDMI_Data_p->I2CName, InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.I2CDevice,sizeof(HDMI_Data_p->I2CName));
#else
     strncpy(HDMI_Data_p->I2CName, InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.I2CDevice,sizeof(HDMI_Data_p->I2CName)-1);
#endif
#ifdef STVOUT_CEC_PIO_COMPARE
     strncpy(HDMI_Data_p->PWMName, InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.PWMName,sizeof(HDMI_Data_p->PWMName));
     strncpy(HDMI_Data_p->CECPIOName, InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.CECPIOName,sizeof(HDMI_Data_p->CECPIOName));
     HDMI_Data_p->CECPIO_BitNumber = InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.CECPIO_BitNumber;
#endif
     HDMI_Data_p->DeviceType = InitParams_p->DeviceType;
     HDMI_Data_p->OutputType = InitParams_p->OutputType;
     HDMI_Data_p->HSyncActivePolarity = InitParams_p->Target.HDMICell.HSyncActivePolarity;
     HDMI_Data_p->VSyncActivePolarity = InitParams_p->Target.HDMICell.VSyncActivePolarity;
#ifdef STVOUT_HDCP_PROTECTION
     HDMI_Data_p->IsHDCPEnable   = InitParams_p->Target.HDMICell.IsHDCPEnable;
     HDMI_Data_p->Out.HDMI.IsHDCPEnable   = InitParams_p->Target.HDMICell.IsHDCPEnable;
     HDMI_Data_p->Second_Part_Authentication=0;
     HDMI_Data_p->Time_Stop_Sec_Part=0;
#endif
     /* Activate HDMI Interface by default*/
     HDMI_Data_p->Out.HDMI.ForceDVI = FALSE;
	 /* Audio Frequency is 48KHz by default*/
	 HDMI_Data_p->Out.HDMI.AudioFrequency = HDMI_AUDIO_FREQUENCY;


    /* Initialise HAL: interrupts could eventually occur from that moment on. */
    /* Fill the different fiels of Initparams */
    /* The following params will not be used by all HAL's */
    HALInitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
    HALInitParams.OutputType = InitParams_p->OutputType;
    HALInitParams.DeviceType = InitParams_p->DeviceType;
#ifdef STVOUT_CEC_PIO_COMPARE
    HALInitParams.DeviceBaseAddress_p= InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.DeviceBaseAddress_p;
    HALInitParams.RegisterBaseAddress_p = InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.BaseAddress_p;
    HALInitParams.SecondBaseAddress_p = InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.SecondBaseAddress_p;
    HALInitParams.HPD_Bit   = InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.HPD_Bit;
    strncpy(HALInitParams.PIODevice,InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.PIODevice,sizeof(HALInitParams.PIODevice));
    HALInitParams.IsHPDInversed    = InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.IsHPDInversed;
#else
    HALInitParams.DeviceBaseAddress_p= InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p;
    HALInitParams.RegisterBaseAddress_p = InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p;
    HALInitParams.SecondBaseAddress_p = InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p;
    HALInitParams.HPD_Bit   = InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.HPD_Bit;
    strncpy(HALInitParams.PIODevice,InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.PIODevice,sizeof(HALInitParams.PIODevice)-1);
    HALInitParams.IsHPDInversed    = InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed;
#endif

    /* Copy the the Horizental and Vertical sync polarity*/
    HALInitParams.HSyncActivePolarity = InitParams_p->Target.HDMICell.HSyncActivePolarity;
    HALInitParams.VSyncActivePolarity = InitParams_p->Target.HDMICell.VSyncActivePolarity;

    /* call hal init function */
    ErrorCode = HAL_Init(&HALInitParams, &(HDMI_Data_p->HalHdmiHandle));

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: HAL init failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module DVI/HDMI init: could not initialize HAL DVI/HDMI Cell !"));
        Term(HDMI_Data_p->HalHdmiHandle);
        return(ErrorCode);
    }
    HDMI_HANDLE_LOCK(HDMI_Data_p); /* Protect from accesses in InternalTaskFunction */

    /* Start the internal task */
    ErrorCode = StartInternalTask(*HdmiHandle_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: task creation failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module HDMI/DVI init: could not start the internal task ! (%s)", (ErrorCode == ST_ERROR_ALREADY_INITIALIZED?"already exists":"failed")));

		HDMI_HANDLE_UNLOCK(HDMI_Data_p);

        /* Stop the internal task */
        StopInternalTask(*HdmiHandle_p);

        Term(HDMI_Data_p->HalHdmiHandle);


        if (ErrorCode == ST_ERROR_ALREADY_INITIALIZED)
        {
            /* Change error code because ST_ERROR_ALREADY_INITIALIZED is not allowed for _Init() functions */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        return(ErrorCode);
    }


    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->Target.HDMICell.EventsHandlerName, &STEVT_OpenParams, &(HDMI_Data_p->EventsHandle));

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module DVI/HDMI init: could not open EVT !"));

		HDMI_HANDLE_UNLOCK(HDMI_Data_p);
        Term(HDMI_Data_p->HalHdmiHandle);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* Prepare common subscribe parameters */
    STEVT_SubscribeParams.SubscriberData_p = (void *) (*HdmiHandle_p);

    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(HDMI_Data_p->EventsHandle,
                                          InitParams_p->Target.HDMICell.VTGName,
                                          STVOUT_CHANGE_STATE_EVT,
                                          &(HDMI_Data_p->RegisteredEventsID[HDMI_CHANGE_STATE_EVT_ID]));

    ErrorCode = STEVT_RegisterDeviceEvent(HDMI_Data_p->EventsHandle,
                                          InitParams_p->Target.HDMICell.VTGName,
                                          STVOUT_REVOKED_KSV_EVT,
                                          &(HDMI_Data_p->RegisteredEventsID[HDMI_REVOKED_KSV_EVT_ID]));

#ifdef STVOUT_CEC_PIO_COMPARE
    ErrorCode = STEVT_RegisterDeviceEvent(HDMI_Data_p->EventsHandle,
                                          InitParams_p->Target.HDMICell.VTGName,
                                          STVOUT_CEC_MESSAGE_EVT,
                                          &(HDMI_Data_p->RegisteredEventsID[HDMI_CEC_MESSAGE_EVT_ID]));

    ErrorCode = STEVT_RegisterDeviceEvent(HDMI_Data_p->EventsHandle,
                                          InitParams_p->Target.HDMICell.VTGName,
                                          STVOUT_CEC_LOGIC_ADDRESS_EVT,
                                          &(HDMI_Data_p->RegisteredEventsID[HDMI_CEC_LOGIC_ADDRESS_NOTIFY_EVT_ID]));
#endif


    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module DVI/HDMI init: could not register events !"));

		HDMI_HANDLE_UNLOCK(HDMI_Data_p);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Install interrupt handler ourselves: invent an event number that should not be used anywhere else ! */
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)stvout_InterruptHandler;
    ErrorCode = STEVT_SubscribeDeviceEvent(HDMI_Data_p->EventsHandle, InitParams_p->Target.HDMICell.VTGName, HDMI_SELF_INSTALLED_INTERRUPT_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
     /* Error: subscribe failed, undo initialisations done */
     STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module DVI/HDMI init: could not subscribe to internal interrupt event !"));
	 HDMI_HANDLE_UNLOCK(HDMI_Data_p);

     return(ST_ERROR_BAD_PARAMETER);
    }
    else
    {
            /* Install interrupt handler ourselves: invent an event number that should not be used anywhere else ! */
            STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)stvout_InterruptHandler;
            /* Register ourselves interrupt event */
            ErrorCode = STEVT_RegisterDeviceEvent(HDMI_Data_p->EventsHandle, InitParams_p->Target.HDMICell.VTGName, HDMI_SELF_INSTALLED_INTERRUPT_EVT, &(HDMI_Data_p->HdmiInterrupt.EventID));
            if (ErrorCode != ST_NO_ERROR)
            {
             /* Error: notify failed, undo initialisations done */
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module DVI/HDMI init: could not notify internal interrupt event !"));

			 HDMI_HANDLE_UNLOCK(HDMI_Data_p);

             return(ST_ERROR_BAD_PARAMETER);
            }
            else
            {
               /* Install interrupt handler before HAL is initialised, i.e. before HW is starting being activated or having a default state.
               This is to be sure that no interrupt can occur when before installing the interrupt handler, as recommended in OS20 doc. */
               HDMI_Data_p->HdmiInterrupt.Level = InitParams_p->Target.HDMICell.InterruptLevel;
               HDMI_Data_p->HdmiInterrupt.Number = InitParams_p->Target.HDMICell.InterruptNumber;
               STOS_InterruptLock();
               strncpy(HdmiString, "HDMI",sizeof(HdmiString)-1);

               if ( STOS_InterruptInstall(HDMI_Data_p->HdmiInterrupt.Number,  HDMI_Data_p->HdmiInterrupt.Level,
                             SelfInstalledHdmiInterruptHandler,&HdmiString[0],(void*)HDMI_Data_p) != STOS_SUCCESS)
               {
                   STOS_InterruptUnlock();
                   STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem installing interrupt !"));

				   HDMI_HANDLE_UNLOCK(HDMI_Data_p);
                   return(ST_ERROR_BAD_PARAMETER);
               }
               STOS_InterruptUnlock();

               if (STOS_InterruptEnable(HDMI_Data_p->HdmiInterrupt.Number,HDMI_Data_p->HdmiInterrupt.Level) !=  STOS_SUCCESS)
               {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem enabling interrupt !"));

					HDMI_HANDLE_UNLOCK(HDMI_Data_p);
                    return(ST_ERROR_BAD_PARAMETER);
               }

            } /* End interrupt event register OK */
    } /* end interrupt event subscribe OK */
#ifdef STVOUT_CEC_PIO_COMPARE
        if(ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = CEC_InterruptsInstall(HDMI_Data_p);
        }
        if(ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = CEC_TaskStart(HDMI_Data_p);
        }
#endif
    HDMI_HANDLE_UNLOCK(HDMI_Data_p);
    return(ErrorCode);

} /* end of VOUT_HDMI_Init() function */
/*******************************************************************************
Name        : InitHdmiStructure
Description : Initialise HDMI structure
Parameters  : HDMI handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitHdmiStructure (const HDMI_Handle_t Handle)
{

    HDMI_Data_t * HDMI_Data_p = (HDMI_Data_t *) Handle;
    HDMI_Data_p->DataAccessSem_p = STOS_SemaphoreCreateFifo(NULL,1);
    HDMI_Data_p->InternalState = HDMI_RUNNING;
    HDMI_Data_p->CurrentState  = STVOUT_NO_RECEIVER;
    HDMI_Data_p->Internaltask.IsRunning = FALSE;
    HDMI_Data_p->Internaltask.ToBeDeleted = FALSE;
    HDMI_Data_p->InternalTaskStarted = FALSE;

    HDMI_Data_p->InterruptTask.IsRunning = FALSE;
    HDMI_Data_p->InterruptTask.ToBeDeleted = FALSE;
#ifdef STVOUT_CEC_PIO_COMPARE
    HDMI_Data_p->CEC_Task.IsRunning = FALSE;
    HDMI_Data_p->CEC_Task.ToBeDeleted = FALSE;
    HDMI_Data_p->CEC_TaskStarted = FALSE;
    HDMI_Data_p->CEC_Sem_p = STOS_SemaphoreCreateFifo(NULL,1);
    HDMI_Data_p->CEC_Params.CECStruct_Access_Sem_p = STOS_SemaphoreCreateFifo(NULL,1);
#endif


    HDMI_Data_p->InterruptSem_p = STOS_SemaphoreCreateFifo(NULL,0);

    HDMI_Data_p->ChangeState_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0);

    HDMI_Data_p->ReceiveVsync_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0);

    /* Initialization needed for the internal state machine work*/
    HDMI_Data_p->EnableCell = TRUE;
    HDMI_Data_p->PixelCaptured =FALSE;
    HDMI_Data_p->IsI2COpened = FALSE;
    HDMI_Data_p->PixelClock =  (HAL_PixelClockFreq_t)0;
    HDMI_Data_p->VSyncFreq = (HAL_VsyncFreq_t)2;
    HDMI_Data_p->I2CHandle =0;


    /* Initialise the interrupts  */
    HDMI_Data_p->HdmiInterrupt.Mask = 0;
    HDMI_Data_p->HdmiInterrupt.IsInside = FALSE;
    HDMI_Data_p->HdmiInterrupt.EnableCount = 0;

    /* Set the HDMI interrupt Handler */
     HDMI_Data_p->HdmiInterrupt.InstallHandler =TRUE;

    /* Initialise interrupts queue */
    HDMI_Data_p->InterruptsBuffer.Data_p = HDMI_Data_p->InterruptsData;
    HDMI_Data_p->InterruptsBuffer.TotalSize = sizeof(HDMI_Data_p->InterruptsData) / sizeof(U32);
    HDMI_Data_p->InterruptsBuffer.BeginPointer_p = HDMI_Data_p->InterruptsBuffer.Data_p;
    HDMI_Data_p->InterruptsBuffer.UsedSize = 0;
    HDMI_Data_p->InterruptsBuffer.MaxUsedSize = 0;

    /* Initialise Commands queue */
    HDMI_Data_p->CommandsBuffer.Data_p = HDMI_Data_p->CommandsData;
    HDMI_Data_p->CommandsBuffer.TotalSize = sizeof(HDMI_Data_p->CommandsData);
    HDMI_Data_p->CommandsBuffer.BeginPointer_p = HDMI_Data_p->CommandsBuffer.Data_p;
    HDMI_Data_p->CommandsBuffer.UsedSize = 0;
    HDMI_Data_p->CommandsBuffer.MaxUsedSize = 0;

    HDMI_Data_p->IsInfoFramePending = FALSE;
    HDMI_Data_p->InfoFrameSettings = 0;
    HDMI_Data_p->InfoFrameFuse = 0;
    HDMI_Data_p->InfoFrameStatus = 0;
    HDMI_Data_p->WaitingForInfoFrameInt = FALSE;

    memset((void*)HDMI_Data_p->InfoFrameBuffer, 0,sizeof(U32) * 8 * STVOUT_INFOFRAME_LAST);
    memset((void*) &HDMI_Data_p->Statistics, 0, sizeof(HDMI_Statistics_t));

    HDMI_Data_p->IsKeysTransfered = FALSE;

#ifdef STVOUT_HDCP_PROTECTION
    HDMI_Data_p->IsHDCPEnable = TRUE;
    memset((void*)HDMI_Data_p->RepeaterKSVFifo, 0, sizeof(U8)*64*11);
	memset((void*)&HDMI_Data_p->HDCPSinkParams, 0, sizeof(STVOUT_HDCPSinkParams_t));
	memset((void*)HDMI_Data_p->HashResult, 0 , sizeof(U32)*5);
    HDMI_Data_p->RepeaterBStatus =0;
    HDMI_Data_p->ForbiddenKSVNumber =0;
    HDMI_Data_p->ForbiddenKSVList_p =NULL;
    HDMI_Data_p->RevokedKSV.Number=0;
    memset ((void*)HDMI_Data_p->RevokedKSV.List, 0, sizeof(U8)*128*5);
    memset ((void*)HDMI_Data_p->BKSV, 0, sizeof(U8)*5);
#endif
#if defined (WA_GNBvd54182)|| defined(WA_GNBvd56512)
    HDMI_Data_p->IsPWMInitialized =FALSE;
#endif
    HDMI_Data_p->AVMute = TRUE;
} /* end of InitHdmiStructure() function*/

/*******************************************************************************
Name        : HDMI_Term
Description : Terminate HDMI Cell, undo all what was done at init
Parameters  : HDMI handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t HDMI_Term(const HDMI_Handle_t Handle)
{
    HDMI_Data_t * HDMI_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    HDMI_Data_p = (HDMI_Data_t *) Handle;

    if (HDMI_Data_p->ValidityCheck != HDMI_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
#ifdef STVOUT_CEC_PIO_COMPARE
    if(ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = CEC_InterruptsUnInstall(HDMI_Data_p);
    }
    if(ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = CEC_TaskStop(HDMI_Data_p);
    }
#endif

    /*Stop the internal task */
    StopInternalTask (Handle);

    /* De-validate structure */
    HDMI_Data_p->ValidityCheck = 0;

    /* The state machine was stopping*/
    HDMI_Data_p->InternalState = HDMI_STOPPING;

    /* Terminate HAL: no interrupt shall be able to occur after that */
    HAL_Term(HDMI_Data_p->HalHdmiHandle);

    if (HDMI_Data_p->HdmiInterrupt.InstallHandler)
    {
        /* Un-install interrupt handler after HAL has been terminated, i.e. after HW is fully de-activated.
        This is to be sure that no interrupt can occur after un-installing the interrupt handler, as recommended in OS20 doc. */
    #if defined ST_OSLINUX
		/* should clear int first... !*/
		/* Enabling the requested interrupt */
        disable_irq(HDMI_Data_p->HdmiInterrupt.Number);
    #endif
      if (STOS_InterruptDisable(HDMI_Data_p->HdmiInterrupt.Number,HDMI_Data_p->HdmiInterrupt.Level) != STOS_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem disabling interrupt !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        STOS_InterruptLock();
        /* uninstall  interrupts */
        if (STOS_InterruptUninstall(HDMI_Data_p->HdmiInterrupt.Number,HDMI_Data_p->HdmiInterrupt.Level,(void *)HDMI_Data_p) !=STOS_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem un-installing interrupt !"));
            ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
        }

        #ifdef ST_OS20
        if ( STOS_InterruptEnable(HDMI_Data_p->HdmiInterrupt.Number,HDMI_Data_p->HdmiInterrupt.Level) !=  STOS_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem enabling interrupt !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        #endif /* ST_OS20 */
        STOS_InterruptUnlock();
    }

    /* Un-subscribe and un-register events: this is done automatically by STEVT_Close() */

    STEVT_UnregisterDeviceEvent(HDMI_Data_p->EventsHandle, HDMI_Data_p->VTGName, STVOUT_CHANGE_STATE_EVT);

    STEVT_UnregisterDeviceEvent(HDMI_Data_p->EventsHandle, HDMI_Data_p->VTGName, STVOUT_REVOKED_KSV_EVT);

#ifdef STVOUT_CEC_PIO_COMPARE
    STEVT_UnregisterDeviceEvent(HDMI_Data_p->EventsHandle, HDMI_Data_p->VTGName, STVOUT_CEC_MESSAGE_EVT);
    STEVT_UnregisterDeviceEvent(HDMI_Data_p->EventsHandle, HDMI_Data_p->VTGName, STVOUT_CEC_LOGIC_ADDRESS_EVT);
#endif

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(HDMI_Data_p->EventsHandle);
    /* Destroy HDMI structure */
    TermHdmiStructure (Handle);
    /* De-allocate : data inside Sink Buffer cannot be used after! */
    memory_deallocate(HDMI_Data_p->CPUPartition_p, (void *) HDMI_Data_p->SinkBuffer_p);

#ifdef STVOUT_HDCP_PROTECTION
    memory_deallocate(HDMI_Data_p->CPUPartition_p, (void*) HDMI_Data_p->ForbiddenKSVList_p);
#endif
    /* De-allocate last: data inside cannot be used after ! */
    memory_deallocate(HDMI_Data_p->CPUPartition_p, (void *) HDMI_Data_p);

    return(ErrorCode);
} /* end of HDMI_Term() function */

/*******************************************************************************
Name        : TermHdmiStructure
Description : Terminate HDMI structure
Parameters  : HDMI handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TermHdmiStructure (const HDMI_Handle_t Handle)
{
    HDMI_Data_t * HDMI_Data_p = (HDMI_Data_t *) Handle;
#ifdef STVOUT_HDCP_PROTECTION
    HDMI_Data_p->IsKeysTransfered = FALSE;
    if (HDMI_Data_p->ForbiddenKSVNumber !=0)
    {
       memory_deallocate(HDMI_Data_p->CPUPartition_p, (void*) HDMI_Data_p->ForbiddenKSVList_p);
       HDMI_Data_p->ForbiddenKSVNumber =0;
    }
#endif
    STOS_SemaphoreDelete(NULL,HDMI_Data_p->ChangeState_p);
    STOS_SemaphoreDelete(NULL,HDMI_Data_p->InterruptSem_p);
    STOS_SemaphoreDelete(NULL,HDMI_Data_p->DataAccessSem_p);
    STOS_SemaphoreDelete(NULL,HDMI_Data_p->ReceiveVsync_p);

#ifdef STVOUT_CEC_PIO_COMPARE
    STOS_SemaphoreDelete(NULL, HDMI_Data_p->CEC_Sem_p);
    STOS_SemaphoreDelete(NULL, HDMI_Data_p->CEC_Params.CECStruct_Access_Sem_p);
#endif
} /* end of TermHdmiStructure ()*/
/*******************************************************************************
Name        : HDMI_Open
Description : Open Handle to HDMI/DVI Cell
Parameters  : Hdmi handle
Assumptions :
Limitations :
Returns     : in params: HAL handle
              ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t HDMI_Open(const HDMI_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_t *  Data_p;

	HDMI_HANDLE_LOCK(Handle);

    Data_p = (HDMI_Data_t *) Handle;
    ErrorCode = HAL_Open(Data_p->HalHdmiHandle);


	HDMI_HANDLE_UNLOCK(Handle);

    return(ErrorCode);
} /* end of HAL_Open() function */
/*******************************************************************************
Name        : HAL_Close
Description : Close opened Handle to HDMI/DVI Cell
Parameters  : Hal Hdmi handle
Assumptions :
Limitations :
Returns     : in params: HAL handle
              ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t HDMI_Close(const HDMI_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_t * Data_p ;

	HDMI_HANDLE_LOCK(Handle);
    Data_p = (HDMI_Data_t *) Handle;

    ErrorCode = HAL_Close(Data_p->HalHdmiHandle);

	HDMI_HANDLE_UNLOCK(Handle);

    return(ErrorCode);
} /* end of HAL_Close() function */
/*******************************************************************************
Name        : HDMI_Start
Description : Start internal State Machine
Parameters  : HDMI handle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t HDMI_Start (HDMI_Handle_t const Handle)
{
    ST_ErrorCode_t               ErrorCode = ST_NO_ERROR;
    HDMI_Data_t                  *Handle_p = (HDMI_Data_t *)Handle;

	HDMI_HANDLE_LOCK(Handle);

    if (!(Handle_p->InternalTaskStarted))
    {
      if (Handle_p->ChangeState_p == NULL)
      {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
      }
      else
      {
        Handle_p->InternalTaskStarted = TRUE;
        STOS_SemaphoreSignal(Handle_p->ChangeState_p);
      }
    }

	HDMI_HANDLE_UNLOCK(Handle);

    return (ErrorCode);
}
/*******************************************************************************
Name        : HDMI_Enable
Description : Enable the DVI/HDMI interface
Parameters  : HDMI handle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t HDMI_Enable (HDMI_Handle_t const Handle)
{
    ST_ErrorCode_t               ErrorCode = ST_NO_ERROR;
    STVTG_Handle_t               VTGHandle;
    STVTG_OpenParams_t           VTGOpenParams;
    STVTG_TimingMode_t           VTGTimingMode;
    STVTG_ModeParams_t           VTGModeParams;
	STVTG_OptionalConfiguration_t   OptionalConfiguration;
    HAL_OutputParams_t           OutParams;
    HAL_TimingInterfaceParams_t  TimingParams;
    HAL_ControlParams_t          ControlParams;
    HAL_OutputFormat_t           Format = HDMI_RGB888;
    HAL_AudioParams_t            AudioParams;
    HDMI_Data_t * Data_p;
    #if defined (ST_7109)&&defined(HDMI_AUDIO_256FS)
    U32 CutId;
    #endif
    /*Initialization of variables */
    TimingParams.EnaPixrepetition = FALSE;
    TimingParams.DllConfig = 0;


    HDMI_HANDLE_LOCK(Handle);
    {
        Data_p = (HDMI_Data_t *) Handle;
        if (Data_p->EnableCell)
        {
            ErrorCode = STVOUT_ERROR_VOUT_ENABLE;
        }
        else
        {
            switch (Data_p->OutputType)
            {
                case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /* no break */
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break */
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :


                    /* It deals with DVI or HDMI protocol*/
                     OutParams.DVINotHDMI = Data_p->Out.HDMI.ForceDVI;

                    /* It deals with DVI/ HDMI interface with encryption*/
                 #ifdef STVOUT_HDCP_PROTECTION
                    OutParams.HDCPEnable =Data_p->IsHDCPEnable;
                 #endif
                    /* The parameters of VTG should be checked before enabling hardware cell*/
                    ErrorCode = STVTG_Open(Data_p->VTGName, &VTGOpenParams, &VTGHandle);

                    if ( ErrorCode!= ST_NO_ERROR )
                    {
                        HDMI_HANDLE_UNLOCK(Handle);
                        return(ST_ERROR_OPEN_HANDLE);
                    }
                    else
                    {
                        ErrorCode = STVTG_GetMode (VTGHandle, &VTGTimingMode, &VTGModeParams);

                        if ( ErrorCode!= ST_NO_ERROR )
                        {
                            STVTG_Close(VTGHandle);
							HDMI_HANDLE_UNLOCK(Handle);
                            return( ST_ERROR_BAD_PARAMETER);
                        }
                        switch (VTGTimingMode)
                        {
                           case STVTG_TIMING_MODE_480I60000_13514 :
                           case STVTG_TIMING_MODE_480I60000_12285 :
                           case STVTG_TIMING_MODE_480I59940_13500 :
                           case STVTG_TIMING_MODE_480I59940_12273 :
                                /* Pixel sent 2 times only for : 720*480i@ 59/60Hz and 720*576i@ 50Hz video description*/
                                TimingParams.EnaPixrepetition = TRUE;
                                /* The dll_config ratio = 3 for SD Timing mode */
                                TimingParams.DllConfig = SD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_27000;
                                /* Set the VSync Frequency for these video formats (60Hz & 59.94Hz)*/
                                Data_p->VSyncFreq = VSYNC_FREQ_60;
                                break;
                           case STVTG_TIMING_MODE_576I50000_13500 :
                           case STVTG_TIMING_MODE_576I50000_14750 :
                                /* Pixel sent 2 times only for : 720*480i@ 59/60Hz and 720*576i@ 50Hz video description*/
                                TimingParams.EnaPixrepetition = TRUE;
                                /* The dll_config ratio = 3 for SD Timing mode */
                                TimingParams.DllConfig = SD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_27000;
                                /* Set the VSync Frequency for these video formats*/
                                Data_p->VSyncFreq = VSYNC_FREQ_50;
                                break;
                           case STVTG_TIMING_MODE_480P60000_27027 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 3 for SD Timing mode */
                                TimingParams.DllConfig = SD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_27027;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_60;
                                break;
                           case STVTG_TIMING_MODE_480P60000_25200 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 3 for SD Timing mode */
                                TimingParams.DllConfig = SD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_25200;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_60;
                                break;
                           case STVTG_TIMING_MODE_480P59940_25175 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 3 for SD Timing mode */
                                TimingParams.DllConfig = SD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_25174;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_60;
                                break;
                           case STVTG_TIMING_MODE_480P60000_24570 :
                           case STVTG_TIMING_MODE_480P59940_27000 :
                           case STVTG_TIMING_MODE_480P59940_24545 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 3 for SD Timing mode */
                                TimingParams.DllConfig = SD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_27000;
                                /* Set the VSync Frequency for these video formats (60Hz &59.94Hz)*/
                                Data_p->VSyncFreq = VSYNC_FREQ_60;
                                break;
                           case STVTG_TIMING_MODE_576P50000_27000 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 3 for SD Timing mode */
                                TimingParams.DllConfig = SD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_27000;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_50;
                                break;
                            case STVTG_TIMING_MODE_1080I60000_74250 :
                            case STVTG_TIMING_MODE_720P60000_74250 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 2 for HD Timing mode */
                                TimingParams.DllConfig = HD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_74250;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_60;
                                break;
                            case STVTG_TIMING_MODE_1080I50000_74250 :
                            case STVTG_TIMING_MODE_1080I50000_74250_1 :
                            case STVTG_TIMING_MODE_720P50000_74250 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 2 for HD Timing mode */
                                TimingParams.DllConfig = HD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_74250;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_50;

                                break;
                           case STVTG_TIMING_MODE_1080I59940_74176 :
                           case STVTG_TIMING_MODE_1035I59940_74176 :
                           case STVTG_TIMING_MODE_720P59940_74176 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 2 for HD Timing mode */
                                TimingParams.DllConfig = HD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_74176;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_60;
                           break;
                           case STVTG_TIMING_MODE_1080P29970_74176 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 2 for HD Timing mode */
                                TimingParams.DllConfig = HD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_74176;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_30;
                           break;
                           case STVTG_TIMING_MODE_1080P23976_74176 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 2 for HD Timing mode */
                                TimingParams.DllConfig = HD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_74176;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_24;
								break;
                           case STVTG_TIMING_MODE_1080P30000_74250 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 2 for HD Timing mode */
                                TimingParams.DllConfig = HD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_74250;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_30;
								break;
                           case STVTG_TIMING_MODE_1080P25000_74250 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 2 for HD Timing mode */
                                TimingParams.DllConfig = HD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_74250;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_25;
								break;
						   case STVTG_TIMING_MODE_1080P24000_74250 :
                                /* No pixel repetition for these Timing modes */
                                TimingParams.EnaPixrepetition = FALSE;
                                /* The dll_config ratio = 2 for HD Timing mode */
                                TimingParams.DllConfig = HD_DllCONFIG_RATIO;
                                /* Set Pixel Clock for SD Tming Mode */
                                Data_p->PixelClock = PIXEL_CLOCK_74250;
                                /* Set the VSync Frequency for these video formats */
                                Data_p->VSyncFreq = VSYNC_FREQ_24;
								break;
                           default :
                                break;
                        }
                        /* Set the timing interface*/
                        HAL_SetInterfaceTiming(Data_p->HalHdmiHandle, TimingParams);

                        if (Data_p->Out.HDMI.ForceDVI)
                        {
                            /* Setting parameters for DVI protocol */
                            /* Enable OESS signal with DVI interface*/
                            OutParams.EssNotOess= FALSE;

                            /* Supported Output format for DVI protocol*/
                            if (Data_p->OutputType != STVOUT_OUTPUT_DIGITAL_HDMI_RGB888)
                            {
                              STVTG_Close(VTGHandle);
                              HDMI_HANDLE_UNLOCK(Handle);
                              return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                            }
                            else
                            {
                              Format = HDMI_RGB888;
                            }
                       }
                       else  /* Setting parameters for HDMI Protocol */
                       {
                            /* Enable ESS signal with HDMI protocol*/
                            OutParams.EssNotOess= TRUE;

                           /* Set input Format RGB888, YCbCr444 or YCbCr422*/
                           switch (Data_p->OutputType)
                           {
                               case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
                                    Format = HDMI_RGB888;
                                    break;
                               case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
                                    Format = HDMI_YCBCR444;
                                    break;
                               case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                                    Format = HDMI_YCBCR422;
                                    break;
                               default :
                                    STVTG_Close(VTGHandle);
                                    HDMI_HANDLE_UNLOCK(Handle);
                                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                                    break;
                           }
                       }
                      /* Set the Output params*/
                      HAL_SetOutputParams(Data_p->HalHdmiHandle, OutParams);

                     /* Set Input Format */
                     HAL_SetInputFormat (Data_p->HalHdmiHandle, Format);

                     Data_p->AVMute = FALSE;

                     if (!OutParams.DVINotHDMI)
                     {
                           /* Set the Info frame control parameter with default values */
                            ControlParams.MaxDelay = MAX_DELAY_16_VSYNCS;
                            ControlParams.MinExts = MIN_EXTS_CTL_PERIOD;

                            /* Set infoframe interrupt to be serviced => Activate global InfoFrame generation */
                            Data_p->InfoFrameSettings = HDMI_GENERAL_CONTROL;

                            /* Enable all interrupts */
                            Data_p->HdmiInterrupt.Mask = HDMI_ITX_ENABLE | HDMI_ITX_SW_RESET_ENABLE |
                                HDMI_ITX_INFOFRAME_ENABLE | HDMI_ITX_PIX_CAPTURE_ENABLE |
                                HDMI_ITX_HOTPLUG_ENABLE | HDMI_ITX_NEWFRAME_ENABLE | HDMI_ITX_GENCTRL_PACKET_ENABLE;

                            /* Enable the corresponding interrupts*/

							HAL_SetInterruptMask(Data_p->HalHdmiHandle, Data_p->HdmiInterrupt.Mask);

                            HAL_ControlInfoFrame(Data_p->HalHdmiHandle, ControlParams);
                     }
                     else
                     {
                           /* Enable all interrupts */
                            Data_p->HdmiInterrupt.Mask = HDMI_ITX_ENABLE | HDMI_ITX_SW_RESET_ENABLE |
                                  HDMI_ITX_PIX_CAPTURE_ENABLE | HDMI_ITX_HOTPLUG_ENABLE;

                            /* Enable the corresponding interrupts*/
							HAL_SetInterruptMask(Data_p->HalHdmiHandle, Data_p->HdmiInterrupt.Mask);

                     }
                     /* Set Audio parameters */
                     switch (Data_p->DeviceType)
                     {
                        case STVOUT_DEVICE_TYPE_7710 :

                             AudioParams.Numberofchannel = 2;   /* Two channels audio */
                             AudioParams.SpdifDiv = 2;          /* Divide input SPDIF clock by 2 before using */
                             AudioParams.ClearFiFos = 0;        /* No FIFO overrun cleared!! */
                             HAL_SetAudioParams(Data_p->HalHdmiHandle, AudioParams);
                             /* Set the general purpose configuration */
                             /*HAL_SetGPConfig(Data_p->HalHdmiHandle,HDMI_GP_CONFIG);*/
                            break;
                        case STVOUT_DEVICE_TYPE_7100 :
                             /* implementing the work arround 128*FS*/
                             AudioParams.Numberofchannel = 2;   /* Two channels audio */
                             #if defined (ST_7109)&& defined(HDMI_AUDIO_256FS)
                             /* The fix is only valid for 7109 cut>=2*/
                             CutId = ST_GetCutRevision();
                             if (CutId>=0xB0)
                             {
                                /* 256*FS is routed to HDMI*/
                                AudioParams.SpdifDiv = 2;
                             }
                             else
                             {
                                /* 128*FS is routed to HDMI*/
                                AudioParams.SpdifDiv = 0;
                             }
                             #else
                             AudioParams.SpdifDiv = 0;          /* Divide input SPDIF clock by 2 before using */
                             #endif
                             AudioParams.ClearFiFos = 0;        /* No FIFO overrun cleared!! */
                             HAL_SetAudioParams(Data_p->HalHdmiHandle, AudioParams);

                            /* The configuration of the Audio subpackets */
                            HAL_SetAudioSamplePacket(Data_p->HalHdmiHandle,0);
                            break;

                        case STVOUT_DEVICE_TYPE_7200 :
                             /* implementing the work arround 128*FS*/
                             AudioParams.Numberofchannel = 2;   /* Two channels audio */
                             AudioParams.SpdifDiv = 0;          /* Divide input SPDIF clock by 2 before using */
                             AudioParams.ClearFiFos = 0;        /* No FIFO overrun cleared!! */
                             HAL_SetAudioParams(Data_p->HalHdmiHandle, AudioParams);

                            /* The configuration of the Audio subpackets */
                            HAL_SetAudioSamplePacket(Data_p->HalHdmiHandle,0);
                            break;

                        default :
                            break;
                    }
                    #if (!defined (WA_GNBvd44290)&&!defined(WA_GNBvd54182))
                    switch (Data_p->Out.HDMI.AudioFrequency)
                    {
                      case 32000 :
                          ErrorCode = HAL_SetAudioNValue(Data_p->HalHdmiHandle, Data_p->PixelClock, SAMPLING_FREQ_32_KHZ,
                                                           Data_p->VSyncFreq);
                          break;
                      case 44100 :
                           ErrorCode = HAL_SetAudioNValue(Data_p->HalHdmiHandle, Data_p->PixelClock, SAMPLING_FREQ_44_1_KHZ,
                                                          Data_p->VSyncFreq);
                          break;
                      case 48000 :
                           ErrorCode = HAL_SetAudioNValue(Data_p->HalHdmiHandle, Data_p->PixelClock, SAMPLING_FREQ_48_KHZ,
                                                          Data_p->VSyncFreq);
                           break;
                      default :
                           break;
                    }
                    #endif
					/* Get HSYNC and VSYNC Polarities*/
                    if ((Data_p->DeviceType == STVOUT_DEVICE_TYPE_7100 )||(Data_p->DeviceType == STVOUT_DEVICE_TYPE_7710)
                         || (Data_p->DeviceType == STVOUT_DEVICE_TYPE_7200))

                    {
                        OptionalConfiguration.Option = STVTG_OPTION_VSYNC_POLARITY;
                        ErrorCode = STVTG_GetOptionalConfiguration (VTGHandle, &OptionalConfiguration);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STVTG_Close(VTGHandle);
                            HDMI_HANDLE_UNLOCK(Handle);
                            return( ST_ERROR_BAD_PARAMETER);
                        }
                        Data_p->VSyncActivePolarity = !OptionalConfiguration.Value.IsVSyncPositive;
                    }

                    /* Set Horizental and Vertical sync polarity*/
                    HAL_SetSyncPolarity(Data_p->HalHdmiHandle, Data_p->VSyncActivePolarity);

                    /* Set Default video settings (Blue Screen) on channel 0,1 and 2*/
                    /*Hal_SendDefaultData(Data_p->HalHdmiHandle, DEFAULT_CHL0_DATA, \
                                       DEFAULT_CHL1_DATA, DEFAULT_CHL2_DATA);      */

                    /*Calculation of output size*/
                    Data_p->OutputSize.XStart =  VTGModeParams.DigitalActiveAreaXStart + 1;
                    Data_p->OutputSize.XStop  = Data_p->OutputSize.XStart + VTGModeParams.ActiveAreaWidth - 1;

                    if (VTGModeParams.ScanType == STGXOBJ_INTERLACED_SCAN)
                    {
                        Data_p->OutputSize.YStart = VTGModeParams.DigitalActiveAreaYStart / 2;
                        Data_p->OutputSize.YStop  =  Data_p->OutputSize.YStart + VTGModeParams.ActiveAreaHeight / 2 - 1;
                    }
                    else
                    {
                        Data_p->OutputSize.YStart =  VTGModeParams.DigitalActiveAreaYStart;
                        Data_p->OutputSize.YStop  =  Data_p->OutputSize.YStart + VTGModeParams.ActiveAreaHeight - 1;
                    }
                    /*Set the HDMI video active parameters */
                    HAL_SetOutputSize(Data_p->HalHdmiHandle,Data_p->OutputSize.XStart, Data_p->OutputSize.XStop,\
                    Data_p->OutputSize.YStart,Data_p->OutputSize.YStop);

					/*Test if the state machine was already running or resetting*/
                    if (Data_p->InternalState != HDMI_STOPPING)
                    {
                        /* Enable the DVI/HDMI interface*/
                        ErrorCode = HAL_Enable(Data_p->HalHdmiHandle);
                    }
                    else
                    {
                        STVTG_Close(VTGHandle);
                        HDMI_HANDLE_UNLOCK(Handle);
                        return(ST_ERROR_BAD_PARAMETER);
                    }

                    /* Enable the DVI/HDMI interface*/
                    Data_p->EnableCell = TRUE;
                    STOS_SemaphoreSignal(Data_p->ChangeState_p);

                    /* close the VTG instance */
                    STVTG_Close(VTGHandle);
                }
                break;
                default :
                ErrorCode= ST_ERROR_BAD_PARAMETER;
                break;
            }
        }
    }
    HDMI_HANDLE_UNLOCK(Handle);
    return(ErrorCode);
} /* end of HDMI_Enable()*/

/*******************************************************************************
Name        : HDMI_Disable
Description : Disable the DVI/HDMI interface
Parameters  : HDMI handle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t HDMI_Disable (HDMI_Handle_t const Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_t * Data_p;

    HDMI_HANDLE_LOCK(Handle);
    {
        Data_p = (HDMI_Data_t *) Handle;

        if (!Data_p->EnableCell)
        {
             /* Return no error if already disabled */
             ErrorCode = ST_NO_ERROR; /*STVOUT_ERROR_VOUT_NOT_ENABLE;*/
        }
        else
        {
             switch (Data_p->OutputType)
            {
                 /* Reserved only for HDMI outputs*/
               case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /* no break */
               case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break*/
               case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
#if defined (WA_GNBvd44290) || defined (WA_GNBvd54182)
                if(Data_p->GenerateACR)
                {
                    /* Don't test error here. can be added if necessary */
                    #ifdef WA_GNBvd44290
                    WA_GNBvd44290_UnInstall(Data_p);
                    #else
                    WA_PWMInterruptUnInstall(Data_p);
                    #endif
                }
#endif /* WA_GNBvd44290 || WA_GNBvd54182*/

                  /* Activate only basic interrupts */
                   Data_p->HdmiInterrupt.Mask = HDMI_ITX_ENABLE | HDMI_ITX_SW_RESET_ENABLE;

                   /* Enable the corresponding interrupts */
				   HAL_SetInterruptMask(Data_p->HalHdmiHandle, Data_p->HdmiInterrupt.Mask);

                   /* Disable the DVI/HDMI interface*/
                    Data_p->EnableCell = FALSE;
                    Data_p->AVMute = TRUE;
                    ErrorCode = HAL_Disable(Data_p->HalHdmiHandle);
                    STOS_SemaphoreSignal(Data_p->ChangeState_p);
                    break;
                default :
                    ErrorCode= ST_ERROR_BAD_PARAMETER;
                    break;
            }
        }
    }

    HDMI_HANDLE_UNLOCK(Handle);
    return(ErrorCode);
} /* end of HDMI_Disable()*/

/*******************************************************************************
Name        : HDMI_SetParams
Description : Set parameters of HDMI interface
Parameters  : HDMI handle,  OutputParams( parameter)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t HDMI_SetParams (HDMI_Handle_t const Handle, const STVOUT_OutputParams_t OutputParams)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	HAL_OutputParams_t           OutParams;
    HDMI_Data_t * Data_p;

	HDMI_HANDLE_LOCK(Handle);
    {
        Data_p = (HDMI_Data_t *) Handle;
        switch (Data_p->OutputType)
        {
                 /* Reserved only for HDMI outputs*/
               case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /* no break */
               case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break*/
               case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :

                   /* Set HDMI interface parameters*/
                    Data_p->Out.HDMI.ForceDVI = OutputParams.HDMI.ForceDVI;
     #if defined (STVOUT_HDCP_PROTECTION)
                    Data_p->Out.HDMI.IsHDCPEnable  = OutputParams.HDMI.IsHDCPEnable;
     #endif
                    Data_p->Out.HDMI.AudioFrequency  = OutputParams.HDMI.AudioFrequency;

                    /* Enable/Disable Info frame transmission whether it is HDMI/DVI interface*/
                    if (Data_p->Out.HDMI.ForceDVI)
                    {
                        Data_p->HdmiInterrupt.Mask = HDMI_ITX_ENABLE | HDMI_ITX_SW_RESET_ENABLE |
                                  HDMI_ITX_PIX_CAPTURE_ENABLE | HDMI_ITX_HOTPLUG_ENABLE;

                        HAL_SetInterruptMask(Data_p->HalHdmiHandle, Data_p->HdmiInterrupt.Mask);

                    }
                    else
                    {
                        Data_p->HdmiInterrupt.Mask = HDMI_ITX_ENABLE | HDMI_ITX_SW_RESET_ENABLE |
                                HDMI_ITX_INFOFRAME_ENABLE | HDMI_ITX_PIX_CAPTURE_ENABLE |
                                HDMI_ITX_HOTPLUG_ENABLE | HDMI_ITX_NEWFRAME_ENABLE | HDMI_ITX_GENCTRL_PACKET_ENABLE;

                        HAL_SetInterruptMask(Data_p->HalHdmiHandle, Data_p->HdmiInterrupt.Mask);
                    }
                    #if (!defined (WA_GNBvd44290)&&!defined(WA_GNBvd54182))
                    switch (Data_p->Out.HDMI.AudioFrequency)
                    {
                        	case 32000 :
                                    ErrorCode = HAL_SetAudioNValue(Data_p->HalHdmiHandle, Data_p->PixelClock, SAMPLING_FREQ_32_KHZ,
                                                                       Data_p->VSyncFreq);
                             		break;
                       		case 44100 :
                                    ErrorCode = HAL_SetAudioNValue(Data_p->HalHdmiHandle, Data_p->PixelClock, SAMPLING_FREQ_44_1_KHZ,
                                                                        Data_p->VSyncFreq);
                             		break;
                       		case 48000 :
                                    ErrorCode = HAL_SetAudioNValue(Data_p->HalHdmiHandle, Data_p->PixelClock, SAMPLING_FREQ_48_KHZ,
                                                                        Data_p->VSyncFreq);
                             		break;
                       		default :
                             		break;
                    }
                    #endif


					/* Change HW configuration for the new software params */
                    OutParams.DVINotHDMI = OutputParams.HDMI.ForceDVI;

         #if defined (STVOUT_HDCP_PROTECTION)
                    /* Necessary for the State Machine*/
                    Data_p->IsHDCPEnable = OutputParams.HDMI.IsHDCPEnable;
                    OutParams.HDCPEnable = OutputParams.HDMI.IsHDCPEnable;
         #endif
					if ( OutParams.DVINotHDMI)
				    {
						OutParams.EssNotOess = FALSE;
					}
					else
					{
						OutParams.EssNotOess = TRUE;
					}
					HAL_SetOutputParams(Data_p->HalHdmiHandle, OutParams);

                    break;
                default :
                    ErrorCode= ST_ERROR_BAD_PARAMETER;
                    break;
        }
   }


	HDMI_HANDLE_UNLOCK(Handle);

    return(ErrorCode);
} /* end of HDMI_SetParams */

/*******************************************************************************
Name        : HDMI_GetParams
Description : Get parameters of HDMI interface
Parameters  : HDMI handle, OutputParams_p (out pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t HDMI_GetParams (HDMI_Handle_t const Handle, STVOUT_OutputParams_t * const OutputParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_t * Data_p;

    if (OutputParams_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
    else
    {
         HDMI_HANDLE_LOCK(Handle);
         {
            Data_p = (HDMI_Data_t *) Handle;
            switch (Data_p->OutputType)
            {
                /* only for HDMI outputs*/
                case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /* no break */
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break*/
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                    OutputParams_p->HDMI.ForceDVI = Data_p->Out.HDMI.ForceDVI;
                    #if defined (STVOUT_HDCP_PROTECTION)
                    OutputParams_p->HDMI.IsHDCPEnable = Data_p->Out.HDMI.IsHDCPEnable;
                    #endif
                    OutputParams_p->HDMI.AudioFrequency = Data_p->Out.HDMI.AudioFrequency;
                    break;
                default :
                    ErrorCode= ST_ERROR_BAD_PARAMETER;
                    break;
            }
        }
        HDMI_HANDLE_UNLOCK(Handle);
    }

    return(ErrorCode);
} /* end of HDMI_GetParams() */
/*******************************************************************************
Name        : HDMI_SetOutputType
Description : Set parameters of HDMI interface
Parameters  : HDMI handle,  OutputParams( parameter)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t HDMI_SetOutputType (HDMI_Handle_t const Handle, STVOUT_OutputType_t OutputType)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_t * Data_p;
    HAL_OutputFormat_t           Format = HDMI_RGB888;

	HDMI_HANDLE_LOCK(Handle);
    Data_p = (HDMI_Data_t *) Handle;
    Data_p->OutputType = OutputType;
    ((HALHDMI_Properties_t *)(Data_p->HalHdmiHandle))->OutputType = OutputType;
    if ( (Data_p->Out.HDMI.ForceDVI)&&(Data_p->OutputType != STVOUT_OUTPUT_DIGITAL_HDMI_RGB888) )
    {
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
    else
    {
        /* Set input Format RGB888, YCbCr444 or YCbCr422*/
        switch (Data_p->OutputType)
        {
            case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
                Format = HDMI_RGB888;
                break;
            case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
                Format = HDMI_YCBCR444;
                break;
            case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                Format = HDMI_YCBCR422;
                break;
            default :
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
        }
        HAL_SetInputFormat (Data_p->HalHdmiHandle, Format);

    }

	HDMI_HANDLE_UNLOCK(Handle);
    return(ErrorCode);
} /* end of HDMI_SetOutputType */
/*******************************************************************************
Name        : HDMI_GetSinkInformation
Description : Get Sink information: (Receiver/Repeater,HDCP capable,
              E-DID capablities...)
Parameters  : Hdmi handle
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t HDMI_GetSinkInformation(HDMI_Handle_t const HdmiHandle,
                                       STVOUT_TargetInformation_t* const TargetInfo_p
                                       )
{
    ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;
    STI2C_Handle_t        I2CHandle;
    STI2C_OpenParams_t    OpenParams;
    STI2C_Params_t        I2CParams;
    int i=1,nb_pages=0, chaine[2]={0,0};
    U32                   NumWritten,NumRead;
    HDMI_Data_t *         HDMI_Data_p;

	HDMI_HANDLE_LOCK(HdmiHandle);
    HDMI_Data_p = (HDMI_Data_t *) HdmiHandle;

    if (HDMI_Data_p->CurrentState != STVOUT_NO_RECEIVER)
    {

        OpenParams.I2cAddress  = I2C_BASE_ADDRESS;
        OpenParams.AddressType = STI2C_ADDRESS_7_BITS;  /* 7 bits I2C addressing */
        OpenParams.BusAccessTimeOut = 100000;
        ErrorCode = STI2C_Open (HDMI_Data_p->I2CName, &OpenParams, &I2CHandle);

        if (ErrorCode != ST_NO_ERROR)
        {
			HDMI_HANDLE_UNLOCK(HdmiHandle);
            return(ST_ERROR_OPEN_HANDLE);
        }

        else
        {
              /* Get Sink buffer pointer */
             TargetInfo_p->SinkInfo.Buffer_p = HDMI_Data_p->SinkBuffer_p;

            /*Delay necessary from Hot plug to EDID reading */
            STOS_TaskDelay(ST_GetClocksPerSecond()/5);
            STI2C_Lock(I2CHandle,TRUE);
            /* Write offset register */
            TargetInfo_p->SinkInfo.Buffer_p[0]=0;
            ErrorCode=STI2C_WriteNoStop(I2CHandle,TargetInfo_p->SinkInfo.Buffer_p,1,100,&NumWritten);
            if (ErrorCode!=ST_NO_ERROR )
            {
                 STTBX_Print(("STI2C_WriteNoStop : 0x%x\n",OpenParams.I2cAddress));
                 ErrorCode = STI2C_Unlock(I2CHandle);
                 ErrorCode = STI2C_Close(I2CHandle);
                 HDMI_HANDLE_UNLOCK(HdmiHandle);
                 return(ST_ERROR_BAD_PARAMETER);
            }
            /* Read first page */
             TargetInfo_p->SinkInfo.Size = 0;
             ErrorCode =  STI2C_Read (I2CHandle, TargetInfo_p->SinkInfo.Buffer_p, MAX_BYTE_READ, 100, &NumRead);
             TargetInfo_p->SinkInfo.Size+=NumRead;
             if (ErrorCode != ST_NO_ERROR)
             {
                 STTBX_Print(("STI2C_Read : 0x%x\n",OpenParams.I2cAddress));
                 ErrorCode = STI2C_Unlock(I2CHandle);
                 ErrorCode = STI2C_Close(I2CHandle);
				 HDMI_HANDLE_UNLOCK(HdmiHandle);
                 return(ST_ERROR_BAD_PARAMETER);
             }

             /* Get number of pages */
             i        = 1;
             nb_pages = (*(TargetInfo_p->SinkInfo.Buffer_p + 126))/2 ;

             /* Get the number of pages */
             while (i<=nb_pages)
             {
              /* Change address : now talking to EDID */
              I2CParams.I2cAddress       = 0x60;
              I2CParams.AddressType      = OpenParams.AddressType;
              I2CParams.BaudRate         = OpenParams.BaudRate;
              I2CParams.BusAccessTimeOut = OpenParams.BusAccessTimeOut;
              ErrorCode = STI2C_SetParams(I2CHandle, &I2CParams);
              if (ErrorCode!=ST_NO_ERROR )
              {
                   STTBX_Print(("STI2C_SetParams() : 0x%x\n",OpenParams.I2cAddress));
                   ErrorCode = STI2C_Unlock(I2CHandle);
                   ErrorCode = STI2C_Close(I2CHandle);
                   HDMI_HANDLE_UNLOCK(HdmiHandle);
                   return(ST_ERROR_BAD_PARAMETER);
              }

              chaine[1]=i;
              ErrorCode = STI2C_WriteNoStop(I2CHandle,(U8 *)&chaine[1],1,0,&NumWritten);
              if (ErrorCode!=ST_NO_ERROR )
              {
                 STTBX_Print(("STI2C_WriteNoStop() : 0x%x\n",OpenParams.I2cAddress));
                 ErrorCode = STI2C_Unlock(I2CHandle);
                 ErrorCode = STI2C_Close(I2CHandle);
                 HDMI_HANDLE_UNLOCK(HdmiHandle);
                 return(ST_ERROR_BAD_PARAMETER);
              }

              /* Change address : now talking to EDID */
              I2CParams.I2cAddress       = I2C_BASE_ADDRESS;
              I2CParams.AddressType      = OpenParams.AddressType;
              I2CParams.BaudRate         = OpenParams.BaudRate;
              I2CParams.BusAccessTimeOut = OpenParams.BusAccessTimeOut;
              ErrorCode = STI2C_SetParams(I2CHandle, &I2CParams);
              if (ErrorCode!=ST_NO_ERROR )
              {
                 STTBX_Print(("STI2C_SetParams() : 0x%x\n",OpenParams.I2cAddress));
                 ErrorCode = STI2C_Unlock(I2CHandle);
                 ErrorCode = STI2C_Close(I2CHandle);
                 HDMI_HANDLE_UNLOCK(HdmiHandle);
                 return(ST_ERROR_BAD_PARAMETER);
              }

              /* Read the page */
              ErrorCode=STI2C_WriteNoStop(I2CHandle,(U8 *)&chaine[0],1,0,&NumWritten);
              if (ErrorCode!=ST_NO_ERROR )
              {
                 STTBX_Print(("STI2C_WriteNoStop() : 0x%x\n",OpenParams.I2cAddress));
                 ErrorCode = STI2C_Unlock(I2CHandle);
                 ErrorCode = STI2C_Close(I2CHandle);
                 HDMI_HANDLE_UNLOCK(HdmiHandle);
                 return(ST_ERROR_BAD_PARAMETER);
              }

              /* Fill up the structure */
              ErrorCode =  STI2C_Read (I2CHandle, (TargetInfo_p->SinkInfo.Buffer_p + MAX_BYTE_READ*i), MAX_BYTE_READ, 100, &NumRead);
              TargetInfo_p->SinkInfo.Size+=NumRead;
              if (ErrorCode != ST_NO_ERROR)
              {
                  STTBX_Print(("STI2C_Read() : 0x%x\n",OpenParams.I2cAddress));
                  ErrorCode = STI2C_Unlock(I2CHandle);
                  ErrorCode = STI2C_Close(I2CHandle);
                  HDMI_HANDLE_UNLOCK(HdmiHandle);
                  return(ST_ERROR_BAD_PARAMETER);
              }

              /* Next page */
                    i++;
               }

            /* Unlock I2C */
            ErrorCode = STI2C_Unlock(I2CHandle);
            ErrorCode = STI2C_Close(I2CHandle);

        }
    }
    else
    {
		ErrorCode = STVOUT_ERROR_VOUT_NOT_AVAILABLE;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "No receiver :no need to collect sink Information!"));
    }

    /* check  if the sink was a simple receiver or a repeater for DVI with encryption interface */
    /* Start copy the HDCP capabilities for the */
    /*Copy the Buffer address to the pointer address of the EDID table, this task is only made in the use of HDMI interface*/
	HDMI_HANDLE_UNLOCK(HdmiHandle);
    return(ErrorCode);
} /* end of HDMI_GetSinkInformation()*/
/*******************************************************************************
Name        : HDMI_GetState
Description : Get the current state of the internal state machine
Parameters  : Hdmi handle, State (pointer)
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t HDMI_GetState(STVOUT_State_t * const State_p, HDMI_Handle_t  const HdmiHandle)
{
     ST_ErrorCode_t ErrorCode= ST_NO_ERROR;
     HDMI_Data_t *  HDMI_Data_p;

	HDMI_HANDLE_LOCK(HdmiHandle);
    HDMI_Data_p = (HDMI_Data_t *)HdmiHandle;

    *State_p= HDMI_Data_p->CurrentState;

	HDMI_HANDLE_UNLOCK(HdmiHandle);
    return(ErrorCode);
} /* end of HDMI_GetState() */
/*******************************************************************************
Name        : HDMI_SendData
Description : Send the info frame is specific for the HDMI interface.
Parameters  : Hdmi handle, Buffer_p (pointer) and Size.
Assumptions :
Limitations : According to the HDMI specification, Only One Info Frame can be transmitted
              in one packet.So, Info Frames can not be transmitted within Info Packet.
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t HDMI_SendData(HDMI_Handle_t const HdmiHandle,
                            const STVOUT_InfoFrameType_t InfoFrameType, U8* Buffer_p, U32 Size)
{
     ST_ErrorCode_t ErrorCode= ST_NO_ERROR;

     STVTG_Handle_t            VTGHandle;
     STVTG_OpenParams_t        VTGOpenParams;
     HAL_PixelClockFreq_t      EnumPixClock  = 0;
     HAL_AudioSamplingFreq_t   EnumAudioFreq = 0;
     HAL_ACR_t                 ACR;
     STVTG_TimingMode_t        VTGTimingMode;
     STVTG_ModeParams_t        VTGModeParams;
#if defined (WA_GNBvd44290) || defined (WA_GNBvd54182)
     U8                        ACRBuffer[20];
#endif
     HDMI_Data_t *  HDMI_Data_p;

     if ((Buffer_p==NULL)||(Size==0))
     {
        return(ST_ERROR_BAD_PARAMETER);
     }

	HDMI_Data_p = (HDMI_Data_t *)HdmiHandle;

	HDMI_HANDLE_LOCK(HdmiHandle);
    if (HDMI_Data_p->CurrentState != STVOUT_NO_RECEIVER)
    {
      /* Compute header and packet bytes as words */
      HAL_ComputeInfoFrame(HDMI_Data_p->HalHdmiHandle, Buffer_p,
                     HDMI_Data_p->InfoFrameBuffer[InfoFrameType], Size);

      if(InfoFrameType == STVOUT_INFOFRAME_TYPE_ACR)
      {

        switch (HDMI_Data_p->Out.HDMI.AudioFrequency)
        {
            case 32000: EnumAudioFreq = SAMPLING_FREQ_32_KHZ; break;
            case 44100: EnumAudioFreq = SAMPLING_FREQ_44_1_KHZ; break;
            case 48000: EnumAudioFreq = SAMPLING_FREQ_48_KHZ; break;
            /* Other cases not managed => set 48000 kHz */
            default:
                EnumAudioFreq = SAMPLING_FREQ_48_KHZ;
                HDMI_Data_p->Out.HDMI.AudioFrequency = 48000;
                break;
        }
        /* The parameters of VTG should be checked before enabling hardware cell*/
        ErrorCode = STVTG_Open(HDMI_Data_p->VTGName, &VTGOpenParams, &VTGHandle);

        if ( ErrorCode!= ST_NO_ERROR )
        {
            HDMI_HANDLE_UNLOCK(HdmiHandle);
            return(ST_ERROR_OPEN_HANDLE);
        }
        else
       {
            ErrorCode = STVTG_GetMode (VTGHandle, &VTGTimingMode, &VTGModeParams);

             switch(VTGTimingMode)
            {
                case STVTG_TIMING_MODE_480P60000_25200 :
                    EnumPixClock = PIXEL_CLOCK_25200;
                    break;
                case STVTG_TIMING_MODE_480P59940_25175 :
                    EnumPixClock = PIXEL_CLOCK_25174;
                    break;
                case STVTG_TIMING_MODE_480P60000_27027 :
                    EnumPixClock = PIXEL_CLOCK_27027;
                    break;

                case STVTG_TIMING_MODE_480I59940_13500 :
                case STVTG_TIMING_MODE_480P29970_13500 :
                case STVTG_TIMING_MODE_576I50000_13500 :
                    /* HDMI is doing pixel repeat for SD mode */
                    EnumPixClock = PIXEL_CLOCK_27000;
                    break;

                case STVTG_TIMING_MODE_480P59940_27000 :
                case STVTG_TIMING_MODE_576P50000_27000 :
                    EnumPixClock = PIXEL_CLOCK_27000;
                    break;

                case STVTG_TIMING_MODE_1080I60000_74250 :
                case STVTG_TIMING_MODE_1080I50000_74250 :
                case STVTG_TIMING_MODE_1080I50000_74250_1 :
                case STVTG_TIMING_MODE_1080P30000_74250 :
                case STVTG_TIMING_MODE_1080P25000_74250 :
                case STVTG_TIMING_MODE_1080P24000_74250 :
                case STVTG_TIMING_MODE_1035I60000_74250 :
                case STVTG_TIMING_MODE_720P60000_74250 :
                case STVTG_TIMING_MODE_720P50000_74250 :
                    EnumPixClock = PIXEL_CLOCK_74250;
                    break;

                case STVTG_TIMING_MODE_1080I59940_74176 :
                case STVTG_TIMING_MODE_1080P29970_74176 :
                case STVTG_TIMING_MODE_1080P23976_74176 :
                case STVTG_TIMING_MODE_1035I59940_74176 :
                case STVTG_TIMING_MODE_720P59940_74176 :
                    EnumPixClock = PIXEL_CLOCK_74176;
                    break;
                default:
                    break;
            }
           /* Retrieve the N and CTS according the pixel clock and audio sampling frequency */

          ErrorCode = HAL_SetACRPacket(HDMI_Data_p->HalHdmiHandle, EnumPixClock,EnumAudioFreq, &ACR);
          HDMI_Data_p->NValue = ACR.N;
          HDMI_Data_p->CTSValue = ACR.CTS;

          #if (!defined (WA_GNBvd44290)&&!defined (WA_GNBvd54182))
                /* Set hardware generate ACR frame */
                HAL_SetAudioRegeneration(HDMI_Data_p->HalHdmiHandle, HDMI_Data_p->NValue);

          #else
           /* Activate software generation of ACR */
           HDMI_Data_p->GenerateACR = (BOOL)Buffer_p[16];
           if (!HDMI_Data_p->GenerateACR)
           {
               HAL_SetAudioRegeneration(HDMI_Data_p->HalHdmiHandle, HDMI_Data_p->NValue);
           }
           else
           {
              /* Disable hardware generation of ACR frame */
              HAL_SetAudioRegeneration(HDMI_Data_p->HalHdmiHandle, 0x1);

              /* Set header for first 4 bytes */
             *(U32*)(&ACRBuffer[0]) = 0x1;

             /* Compute sleep time in us between audio ACR genration */
             /* Set sleep time from buffer info */
             HDMI_Data_p->SleepTime = ACR.N * 10000 / 128 / (HDMI_Data_p->Out.HDMI.AudioFrequency / 100);

             *(U32*)(&ACRBuffer[12]) = HDMI_Data_p->SleepTime;

             /* Reset CTS value & offset */
             *(U32*)(&ACRBuffer[4]) = ACR.N;
             *(U32*)(&ACRBuffer[8]) = ACR.CTS;

             /* Compute header and packet bytes as words for ACR Packets */
             /* Overwrite the packet already formatted */
             HAL_ComputeInfoFrame(HDMI_Data_p->HalHdmiHandle, ACRBuffer,
                           HDMI_Data_p->InfoFrameBuffer[InfoFrameType], Size);

              HDMI_Data_p->CTSOffset = 0;
              /* Set sleep time from buffer info */
              HDMI_Data_p->SleepTime = (*((U32*)(&ACRBuffer[12])) * (ST_GetClocksPerSecond()/1000))/1000;
              /* Enable WA */
              #ifdef WA_GNBvd54182
              ErrorCode = WA_PWMInterruptInstall(HDMI_Data_p);
              #endif
              #ifdef WA_GNBvd44290
              ErrorCode = WA_GNBvd44290_Install(HDMI_Data_p);
              #endif

            }
          #endif

          HAL_SetChannelStatusBit(HDMI_Data_p->HalHdmiHandle, EnumAudioFreq);

          /* close the VTG instance */
          STVTG_Close(VTGHandle);

       }

      }
      else
      {
          /* Enable the info frame generation */
         HDMI_Data_p->InfoFrameSettings |= 1 << (InfoFrameType);
       }

    }
    else
    {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "No receiver :no need to send Info Frame!"));
    }

	HDMI_HANDLE_UNLOCK(HdmiHandle);
    return(ErrorCode);
} /* end of HDMI_SendData()*/

/*******************************************************************************
Name        : HDMI_SetHDCPParams
Description : Set HDCP Device Keys
Parameters  : Hdmi handle,  HDCPParams (pointer)
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
#if defined (STVOUT_HDCP_PROTECTION)
ST_ErrorCode_t HDMI_SetHDCPParams(HDMI_Handle_t const HdmiHandle,
                                  const STVOUT_HDCPParams_t* const HDCPParams_p
                                 )
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_t * HDMI_Data_p;

	HDMI_HANDLE_LOCK(HdmiHandle);
    {
            HDMI_Data_p = (HDMI_Data_t *)HdmiHandle;
            HDMI_Data_p->HDCPParams.IV_0 = HDCPParams_p->IV_0;
            HDMI_Data_p->HDCPParams.IV_1 = HDCPParams_p->IV_1;
            HDMI_Data_p->HDCPParams.KSV_0 = HDCPParams_p->KSV_0;
            HDMI_Data_p->HDCPParams.KSV_1 = HDCPParams_p->KSV_1;
            HDMI_Data_p->HDCPParams.IRate = HDCPParams_p->IRate;
            HDMI_Data_p->HDCPParams.IsACEnabled = HDCPParams_p->IsACEnabled;
            memcpy(HDMI_Data_p->HDCPParams.DeviceKeys, HDCPParams_p->DeviceKeys, sizeof(U32)*80);
            HDMI_Data_p->IsKeysTransfered = TRUE;
            STOS_SemaphoreSignal(HDMI_Data_p->ChangeState_p);
    }
	HDMI_HANDLE_UNLOCK(HdmiHandle);
    return(ErrorCode);
} /* end of HDMI_SetHDCPParams()*/
/*******************************************************************************
Name        : HDMI_EnableDefaultOutput
Description : Set HDCP Device Keys
Parameters  : Hdmi handle,  DefaultOutput_p (pointer)
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t HDMI_EnableDefaultOutput(HDMI_Handle_t const HdmiHandle,
                                       const STVOUT_DefaultOutput_t*        const DefaultOutput_p
                                       )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_t * HDMI_Data_p;

    HDMI_HANDLE_LOCK(HdmiHandle);
    {
            HDMI_Data_p = (HDMI_Data_t *)HdmiHandle;

            /*Enable Default output*/
            HAL_EnableDefaultOuput(HDMI_Data_p->HalHdmiHandle, TRUE);

            /* Set Default Output*/
            Hal_SendDefaultData(HDMI_Data_p->HalHdmiHandle, DefaultOutput_p->DataChannel0, \
                                       DefaultOutput_p->DataChannel1, DefaultOutput_p->DataChannel2);
    }
    HDMI_HANDLE_UNLOCK(HdmiHandle);
    return(ErrorCode);
} /* end of HDMI_EnableDefaultOutput()*/
/*******************************************************************************
Name        : HDMI_DisableDefaultOutput
Description : Disable
Parameters  : Hdmi handle,  DefaultOutput_p (pointer)
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t HDMI_DisableDefaultOutput(HDMI_Handle_t const HdmiHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_t * HDMI_Data_p;

    HDMI_HANDLE_LOCK(HdmiHandle);
    {
            HDMI_Data_p = (HDMI_Data_t *)HdmiHandle;
            /*Enable Default output*/
            HAL_EnableDefaultOuput(HDMI_Data_p->HalHdmiHandle, FALSE);
            /* Set Default Output*/
            Hal_SendDefaultData(HDMI_Data_p->HalHdmiHandle, 0, 0, 0);
    }
    HDMI_HANDLE_UNLOCK(HdmiHandle);
    return(ErrorCode);
} /* end of HDMI_EnableDefaultOutput()*/
/*******************************************************************************
Name        : HDMI_UpdateForbiddenKSVs
Description : Refill the list of forbidden KSVs that should fails the
 *            authentication
Parameters  : Hdmi handle,  KSVList_p (pointer), KSVNumber
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t HDMI_UpdateForbiddenKSVs (HDMI_Handle_t const HdmiHandle,
                                         U8* const KSVList_p,
                                         U32  const KSVNumber
                                        )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_t * HDMI_Data_p;

	HDMI_HANDLE_LOCK(HdmiHandle);
    {
          HDMI_Data_p = (HDMI_Data_t *)HdmiHandle;
          if(HDMI_Data_p->ForbiddenKSVNumber==0)
          {
             HDMI_Data_p->ForbiddenKSVList_p = (U8*) memory_allocate(HDMI_Data_p->CPUPartition_p, KSVNumber*5*sizeof(U8));
          }
          else
          {
             if( KSVNumber > HDMI_Data_p->ForbiddenKSVNumber )
             {
                memory_deallocate (HDMI_Data_p->CPUPartition_p, (void*) HDMI_Data_p->ForbiddenKSVList_p);
                HDMI_Data_p->ForbiddenKSVList_p = (U8*) memory_allocate(HDMI_Data_p->CPUPartition_p, KSVNumber*5*sizeof(U8));
             }
         }
         memcpy(HDMI_Data_p->ForbiddenKSVList_p, KSVList_p, KSVNumber*5*sizeof(U8));
         HDMI_Data_p->ForbiddenKSVNumber = KSVNumber;
    }
	HDMI_HANDLE_UNLOCK(HdmiHandle);
    return(ErrorCode);
}/* end of HDMI_UpdateForbiddenKSVs()*/

#ifdef  WA_GNBvd56512
/*******************************************************************************
Name        : HDMI_WA_GNBvd56512_Install
Description : Install Call back of pio interrupt.
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
ST_ErrorCode_t HDMI_WA_GNBvd56512_Install(HDMI_Handle_t const HdmiHandle)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    IsHDCPWAEnabled = TRUE;
    ErrorCode = WA_PWMInterruptInstall (HdmiHandle);
    return(ErrorCode);
}
/*******************************************************************************
Name        : HDMI_WA_GNBvd56512_UnInstall
Description : UnInstall Call back of PWM timer Interrupt
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
ST_ErrorCode_t HDMI_WA_GNBvd56512_UnInstall(HDMI_Handle_t const HdmiHandle)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    IsHDCPWAEnabled = FALSE;
    ErrorCode = WA_PWMInterruptUnInstall(HdmiHandle);
    return(ErrorCode);
}
#endif
#endif
/*******************************************************************************
Name        : HDMI_GetStatistics
Description : Get HDMI statistics
Parameters  : Hdmi handle,  Statistics_p (pointer)
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t HDMI_GetStatistics(HDMI_Handle_t const HdmiHandle,
                                   STVOUT_Statistics_t * const  Statistics_p
                                 )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_t * HDMI_Data_p;

	HDMI_HANDLE_LOCK(HdmiHandle);
    {
            HDMI_Data_p = (HDMI_Data_t *)HdmiHandle;
            Statistics_p->InterruptNewFrameCount =  HDMI_Data_p->Statistics.InterruptNewFrameCount;
            Statistics_p->InterruptHotPlugCount =  HDMI_Data_p->Statistics.InterruptHotPlugCount;
            Statistics_p->InterruptSoftResetCount =  HDMI_Data_p->Statistics.InterruptSoftResetCount;
            Statistics_p->InterruptInfoFrameCount =  HDMI_Data_p->Statistics.InterruptInfoFrameCount;
            Statistics_p->InterruptPixelCaptureCount =  HDMI_Data_p->Statistics.InterruptPixelCaptureCount;
            Statistics_p->InterruptDllLockCount =  HDMI_Data_p->Statistics.InterruptDllLockCount;
            Statistics_p->InfoFrameOverRunError =  HDMI_Data_p->Statistics.InfoFrameOverRunError;
            Statistics_p->SpdifFiFoOverRunCount =  HDMI_Data_p->Statistics.SpdifFiFoOverRunCount;
            Statistics_p->InfoFrameCountGeneralControl =  HDMI_Data_p->Statistics.InfoFrameCountGeneralControl;
            Statistics_p->InterruptGeneralControlCount =  HDMI_Data_p->Statistics.InterruptGeneralControlCount;
    }
	HDMI_HANDLE_UNLOCK(HdmiHandle);
    return(ErrorCode);
}/* end of HDMI_GetStatistics */
/* end of Hdmi_api.c file */
/* ------------------------------- End of file ---------------------------- */

