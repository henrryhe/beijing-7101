/*********************************************************************************
File Name   : hdmi_int.c

Description : VOUT driver module. common interruptions for DVI/HDMI programmation

COPYRIGHT (C) STMicroelectronics 2004.

Date                   Modification                                     Name
----                   ------------                                     ----
08 Apr 2004            Creation                                         AC
12 Nov 2004            Add Support of ST40/OS21                         AC
*********************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#if !defined (ST_OSLINUX)
#include <string.h>
#include <stdio.h>
#endif /* ST_OSLINUX */
#include "stsys.h"

#include "hdmi_int.h"
#include "hdmi_ip.h"
#include "hdmi.h"

#if defined (STVOUT_TRACE)
#include "..\..\..\dvdgr-prj-stapigat\src\trace.h"
#endif

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Private variables (static) ---------------------------------------------- */
#if defined (WA_GNBvd44290) || defined (WA_GNBvd54182)
static U32   PrevCTS=0;
#endif

#ifdef WA_GNBvd54182
static U8   FirstTime=1;
static U32  NbTicksPerSec=0,ACRIntPrevTime=0,Refdelay=0;
#endif

#ifdef WA_GNBvd56512
static U8 PE_NormalValue=1,Nb_NormalValue=0,Nb_NonNormalValue=0,CounterAdjust=0,NumberX=8;
static U8 PE_6MNormalValue=1,Nb_6MNormalValue=0,Nb_6MNonNormalValue=0,N_6MNumberX=5,N_6MCounterAdjust=0;
static U32 Pe_Value=0,Pe6M_Value=0;

#if (STCLKRV_EXT_CLKB_MHZ!=27)
    static U8 PE_27MNormalValue=1,Nb_27MNormalValue=0,Nb_27MNonNormalValue=0;
    static U32 Pe27M_Value=0;
#endif
#endif
/* Private Macros ---------------------------------------------------------- */

/* Functions (private)------------------------------------------------------ */
static void EnterHdmiInterrupt(const HDMI_Handle_t Handle);
static void LeaveHdmiInterrupt(const HDMI_Handle_t Handle);
static void SendNewInfoframe(const HDMI_Handle_t Handle);
#if defined (WA_GNBvd44290) || defined (WA_GNBvd54182)
static void WA_GNBvd44290_Interrupt(void * Param);
#endif
#ifdef WA_GNBvd56512
static  void  WA_GNBvd56512_Interrupt(void * Param);
void  GetCountHD_PCM(void);
#endif
/* Global Variables -------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------ */

#if defined (WA_GNBvd44290)
#define PWM_INT_BIT_CMP0 0x20
#define PWM_INT_BIT_CMP1 0x40

#define C1_PWM_CNT()              *((volatile U32*)0x30003064)
#define C1_PWM_CMP1_VAL()         *((volatile U32*)0x30003024)
#define C1_PWM_CMP1_CTRL()        *((volatile U32*)0x30003050)
#define C1_PWM_INT_EN()           *((volatile U32*)0x30003054)
#define C1_PWM_INT_ACK()          *((volatile U32*)0x3000305C)

#ifdef TRACE_PIO
#define PIO_TRACE_REG       *((volatile unsigned char*)0x20022030)
#define PIO_FDMA_REG        *((volatile unsigned char*)0x20022004)
#define PIO_FDMA_REG_CLEAR  *((volatile unsigned char*)0x20022008)
#endif
#define FS_CLOCKGEN_CFG_2   *(volatile U32*)(0x380000C8)

extern void clkrv_WA_GNBvd44290( U32 * CountHD_p, U32 * CountPCM_p );
extern void stboot_WA_GNBvd44290_InstallCallback(void (*CallBack)(void *), void * Data_p);
extern void stboot_WA_GNBvd44290_UnInstallCallback(void);
#endif /* WA_GNBvd44290 */

#if defined (WA_GNBvd54182) || defined(WA_GNBvd56512)
#ifdef TRACE_PIO
#define  PIO_TRACE_REG       *((volatile unsigned char*)0xb8022030)
#define  PIO_FDMA_REG        *((volatile unsigned char*)0xb8022004)
#define  PIO_FDMA_REG_CLEAR  *((volatile unsigned char*)0xb8022008)
#endif
#ifdef WA_GNBvd56512
extern void clkrv_WA_GNBvd44290( U32 * CountHD_p, U32 * CountPCM_p );
#endif
#endif /* WA_GNBvd54182 || WA_GNBvd56512*/

/*******************************************************************************
Name        : EnterHdmiInterrupt()
Description : Routine to call at the beginning of the DVI/HDMI interrupt (first call)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnterHdmiInterrupt(const HDMI_Handle_t Handle)
{
    ((HDMI_Data_t *) Handle)->HdmiInterrupt.IsInside = TRUE;
}
/*******************************************************************************
Name        : LeaveHdmiInterrupt()
Description : Routine to call at the end of the DVI/HDMI interrupt (last call)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void LeaveHdmiInterrupt(const HDMI_Handle_t Handle)
{
    ((HDMI_Data_t *) Handle)->HdmiInterrupt.IsInside = FALSE;
}
/*******************************************************************************
Name        : stvout_EnableHdmiInterrupt
Description : Enable DVI/HDMI interrupt
Parameters  :
Assumptions : HDMI_p->HdmiInterrupt.IsInside is set TRUE at the beginning of DVI/HDMI
              interrupt, FALSE at the end
Limitations : Works properly even in case of nesting calls
Returns     :
*******************************************************************************/
void stvout_EnableHdmiInterrupt(const HDMI_Handle_t Handle)
{

    if (!(((HDMI_Data_t *) Handle)->HdmiInterrupt.IsInside))
    {
        if (((HDMI_Data_t *) Handle)->HdmiInterrupt.EnableCount == 0)
        {
             if (STOS_InterruptEnable(((HDMI_Data_t *) Handle)->HdmiInterrupt.Number,((HDMI_Data_t *) Handle)->HdmiInterrupt.Level)
                    !=  STOS_SUCCESS)
             {
                return;
             }
        }
        (((HDMI_Data_t *)Handle)->HdmiInterrupt.EnableCount)++;
    }
}
/*******************************************************************************
Name        : stvout_DisableHDMIInterrupt
Description : Disable DVI/HDMI interrupt
Parameters  :
Assumptions : HdmiInterrupt.IsInside is set TRUE at the beginning of DVI/HDMI interrupt, FALSE at the end
Limitations : Works properly even in case of nesting calls
Returns     :
*******************************************************************************/
void stvout_DisableHdmiInterrupt(const HDMI_Handle_t Handle)
{
    if (!(((HDMI_Data_t *)Handle)->HdmiInterrupt.IsInside))
    {
        if (((HDMI_Data_t *)Handle)->HdmiInterrupt.EnableCount == 1)
        {
            if (STOS_InterruptDisable(((HDMI_Data_t *) Handle)->HdmiInterrupt.Number,((HDMI_Data_t *) Handle)->HdmiInterrupt.Level)
                    != STOS_SUCCESS)
            {
                return;
            }
        }
        (((HDMI_Data_t *) Handle)->HdmiInterrupt.EnableCount)--;
    }
}
/*******************************************************************************
Name        : stvout_InterruptHandler
Description : Recept interrupt event
Parameters  : EVT standard prototype
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
#define HDMI_TIMESTAMP_FILE_SIZE 30
U32 hdmi_TimeStamp[HDMI_TIMESTAMP_FILE_SIZE][3];
U32 hdmi_Stat[7][2]={{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0}};
U32 hdmi_TimeStampIndex = 0;
U32 hdmi_TimeStampProcess = 0;
U32 hdmi_TimeStampModulo = HDMI_TIMESTAMP_FILE_SIZE;
U32 hdmi_hotplug = 0;
void stvout_InterruptHandler(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
     STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    U32 InterruptStatus;
    HDMI_Handle_t Handle =(HDMI_Handle_t) SubscriberData_p;

    /* This is Unused Parameters*/
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    /* Detect we are now inside DVI/HDMI interrupt */
    EnterHdmiInterrupt(Handle);

    /* Read the interrupt status register */
    InterruptStatus = HAL_GetInterruptStatus(((HDMI_Data_t *)Handle)->HalHdmiHandle);

    /* Adding timming recording */
    if ((InterruptStatus & HDMI_ITX_NEWFRAME) != 0)
    {
      if (hdmi_TimeStampIndex < HDMI_TIMESTAMP_FILE_SIZE)
      {
        hdmi_TimeStamp[hdmi_TimeStampIndex++][0] = time_now();
      }
      hdmi_TimeStampIndex %= hdmi_TimeStampModulo;
    }

    if ((InterruptStatus & HDMI_ITX_HOTPLUG_ENABLE) != 0)
    {
     hdmi_hotplug++;
    }

    /*Push the interrupt in the file queue*/
    PushInterruptCommand(Handle, InterruptStatus);

    /* Signal  the process interrupt that a new it occured*/
    STOS_SemaphoreSignal(((HDMI_Data_t *)Handle)->InterruptSem_p);

    /* Detect we are now leaving the DVI/HDMI interrupt */
    LeaveHdmiInterrupt(Handle);

} /* End of stvout_InterruptHandler() function */

/******************************************************************************
Name        : stvout_ProcessInterrupt
Description : Gives the Interrupt process In VOUT device
Parameters  : Handle (IN/OUT) : pointer on VOUT device to access and update
             InterruptStatus   : value of hdmi_interrupt_status register
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
U32 First_General_Int = 0;
void stvout_ProcessInterrupt(const HDMI_Handle_t Handle, U32 InterruptStatus)
{
  HDMI_Data_t *Handle_p;

  Handle_p = (HDMI_Data_t *)Handle;

  /* Process interrupts in logical order of occurence in case many occur at the
  same time */

  while (InterruptStatus != 0)
  {
    /* Stay inside interrupt routine until all the bits have been tested */
    if ((InterruptStatus & HDMI_ITX_ENABLE) != 0)
    {
      InterruptStatus ^=HDMI_ITX_ENABLE ;
      /*----------------------------------------------------------------------
         Interrupt after soft reset completion.
      ----------------------------------------------------------------------*/
      if ((InterruptStatus & HDMI_ITX_SW_RESET_ENABLE) != 0)
      {
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("Int Proc","Soft Reset");
#endif
        InterruptStatus ^= HDMI_ITX_SW_RESET_ENABLE;
        /* HDMI Cell soft reset completion*/
      }
      /*--------------------------------------------------------------------
        Interrupt after info frame transmission.
      --------------------------------------------------------------------*/
      if ((InterruptStatus & HDMI_ITX_INFOFRAME_ENABLE)!=0)
      {
#if defined(STVOUT_INTERRUPT_TRACE)
        /*TraceState("Int Proc","Infoframe");*/
#endif
        InterruptStatus ^= HDMI_ITX_INFOFRAME_ENABLE;
       /* hdmi_Stat[1][0]++;
        hdmi_Stat[1][1] = time_now();    */
        if (Handle_p->IsInfoFramePending)
        {
          /* Driver was waiting an Info Frame interrupt so generate a fake NEWFRAME in ordre to reset InfoFrame */
          InterruptStatus |= HDMI_ITX_NEWFRAME_ENABLE;
          Handle_p->IsInfoFramePending = FALSE;
          Handle_p->InfoFrameStatus = 0;
        }
        else
        {
          SendNewInfoframe(Handle);
        }
        Handle_p->Statistics.InterruptInfoFrameCount++;
      }
      /*--------------------------------------------------------------------
        Interrupt after capture of first active pixel
      ---------------------------------------------------------------------*/
      if ((InterruptStatus & HDMI_ITX_PIX_CAPTURE_ENABLE)!=0)
      {
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("Int Proc","Pix Capture");
#endif
        InterruptStatus ^= HDMI_ITX_PIX_CAPTURE_ENABLE;
        PushControllerCommand(Handle, CONTROLLER_COMMAND_PIX_CAPTURE);
        STOS_SemaphoreSignal(Handle_p->ChangeState_p);
        Handle_p->Statistics.InterruptPixelCaptureCount++;

      }
      /*--------------------------------------------------------------------
         Interrupt after Hot plug detection signal
       ---------------------------------------------------------------------*/
      if ((InterruptStatus & HDMI_ITX_HOTPLUG_ENABLE)!=0)
      {
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("Int Proc","HOT PLUG");
#endif
        InterruptStatus ^= HDMI_ITX_HOTPLUG_ENABLE;
        PushControllerCommand(Handle, CONTROLLER_COMMAND_HPD);
        STOS_SemaphoreSignal(Handle_p->ChangeState_p);
        Handle_p->Statistics.InterruptHotPlugCount++;
      }
      /*--------------------------------------------------------------------
         Interrupt after  DLL Lock
      ---------------------------------------------------------------------*/
      if ((InterruptStatus & HDMI_ITX_DLL_LOCK_ENABLE)!=0)
      {
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("Int Proc","DLL Lock");
#endif
        InterruptStatus ^= HDMI_ITX_DLL_LOCK_ENABLE;
        Handle_p->Statistics.InterruptDllLockCount++;
      }
      /*--------------------------------------------------------------------
       Interrupt on completion of general control pack  et Xmission
      --------------------------------------------------------------------*/
      if ((InterruptStatus & HDMI_ITX_GENCTRL_PACKET_ENABLE)!=0)
      {
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("Int Proc","Gen Ctrl Packet");
#endif
        InterruptStatus ^= HDMI_ITX_GENCTRL_PACKET_ENABLE;
        /* the general control packet is given to formatter. It is expected that
        * the packet is subsequently transmitted */
        Handle_p->Statistics.InterruptGeneralControlCount++;
#if 0
        if (Handle_p->IsInfoFramePending)
        {
          /* Driver was waiting an Info Frame interrupt so generate a fake NEWFRAME in ordre to reset InfoFrame */
          InterruptStatus |= HDMI_ITX_NEWFRAME_ENABLE;
          Handle_p->IsInfoFramePending = FALSE;
        }
        else
        {
          /* Info Frame Sent correctly so clear the infostatus and sent the next one */
          Handle_p->InfoFrameStatus ^= 0;
          SendNewInfoframe(Handle);
        }
#else
        if (First_General_Int == 0)
          First_General_Int = time_now();
#endif
      }
      /*--------------------------------------------------------------------
        Interrupt on new (start of) frame
      --------------------------------------------------------------------*/
      if ((InterruptStatus & HDMI_ITX_NEWFRAME_ENABLE)!=0)
      {
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("Int Proc","New Frame");
#endif
        hdmi_Stat[0][0]++;
        hdmi_Stat[0][1] = time_now();

        InterruptStatus ^= HDMI_ITX_NEWFRAME_ENABLE;
        if((Handle_p->InfoFrameStatus != 0) && (Handle_p->InfoFrameFuse < 2))
        {
          /* There is an Info Frame Pending so wait the next interrupt of InfoFrame before restarting the sending of Info Frame */
          Handle_p->Statistics.InfoFrameOverRunError++;
          Handle_p->IsInfoFramePending = TRUE;
          Handle_p->InfoFrameFuse++;
        }
        else
        {
          /* Driver send each frame :
                - General Control Packet
                - AVI Frame
                - Audio Frame
             and one extra InfoFrame (VS, SPD, MPEG)
          */
          Handle_p->InfoFrameFuse = 0;
          Handle_p->InfoFrameStatus |= Handle_p->InfoFrameSettings & HDMI_EACH_FRAME_INFOFRAME;
          switch (Handle_p->Statistics.InterruptNewFrameCount % 3)
          {
            case 0 :
              Handle_p->InfoFrameStatus |= Handle_p->InfoFrameSettings & HDMI_VS_FRAME;
              break;
            case 1 :
              Handle_p->InfoFrameStatus |= Handle_p->InfoFrameSettings & HDMI_SPD_FRAME;
              break;
            case 2 :
              Handle_p->InfoFrameStatus |= Handle_p->InfoFrameSettings & HDMI_MPEG_FRAME;
              break;
          }
          /* Timming record */
          /*if (hdmi_TimeStampProcess < HDMI_TIMESTAMP_FILE_SIZE)
            hdmi_TimeStamp[hdmi_TimeStampProcess++][1] = time_now();
          hdmi_TimeStampProcess %= hdmi_TimeStampModulo;*/
          if(!Handle_p->WaitingForInfoFrameInt)
          {
            SendNewInfoframe(Handle);
          }
          Handle_p->Statistics.InterruptNewFrameCount++;
        }

      }
      /*----------------------------------------------------------------------
       Interrupt when Spdif FIFO overrun
       *-------------------------------------------------------------------- */
      if ((InterruptStatus & HDMI_ITX_SPDIF_OVERRUN_ENABLE)!=0)
      {
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("Int Proc","SPDIF Overrun");
#endif
        InterruptStatus ^= HDMI_ITX_SPDIF_OVERRUN_ENABLE;
        Handle_p->Statistics.SpdifFiFoOverRunCount++;
      }
    }
  }
  Handle_p->Statistics.NoMoreInterrupt++;
} /*End of stvout_ProcessInterrupt*/

/*******************************************************************************
Name        : stvout_InterruptProcessTask
Description : Process interrupt the sooner if there is one
Parameters  : HDMI handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stvout_InterruptProcessTask(const HDMI_Handle_t Handle)
{
    U32 InterruptStatus;
    HDMI_Data_t *Handle_p = (HDMI_Data_t *)Handle;

    STOS_TaskEnter(NULL);
while (!(Handle_p->InterruptTask.ToBeDeleted))
    {
#if defined (ST_OS21)
        if (STOS_SemaphoreWait(Handle_p->InterruptSem_p) != OS21_SUCCESS)
        {
          /* this shouldn't happend but in that case just loop */
          continue;
        }
#else  /* ST_OS20 */
        if (STOS_SemaphoreWait(Handle_p->InterruptSem_p)!=0)/*Always returns 0*/
        {
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("Int","T out imm");
#endif
            /* this shouldn't happend but in that case just loop */
          continue;
        }
#endif /* ST_OS20 */
        if (PopInterruptCommand(Handle_p, &InterruptStatus) == ST_ERROR_NO_MEMORY)
        {
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("Int","Err No mem");
#endif
          /* No interrupt has been sent so it may be the driver who want to stop this task */
          continue;
        }
#if defined(STVOUT_INTERRUPT_TRACE)
        /*TraceState("Int","Process Int");*/
#endif
        /* Interrupt is pending just process it */
        InterruptStatus = InterruptStatus & Handle_p->HdmiInterrupt.Mask;
        stvout_ProcessInterrupt(Handle_p, InterruptStatus);
    }
    STOS_TaskExit(NULL);
} /* end of stvout_InterruptProcessTask()*/
/*******************************************************************************
Name        : SelfInstalledHdmiInterruptHandler
Description : When installing interrupt handler ourselves, interrupt function
Parameters  : Pointer to HDMI handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
#if defined(ST_OS21) || defined(ST_OSWINCE)
   int SelfInstalledHdmiInterruptHandler(void * Param)
#endif /* ST_OS21 */
#if defined (ST_OS20)
   void SelfInstalledHdmiInterruptHandler(void * Param)
#endif /* ST_OS20 */
#if defined(ST_OSLINUX)
   irqreturn_t SelfInstalledHdmiInterruptHandler(int irq, void * Param, struct pt_regs * regs)
#endif

{
    #if defined(ST_OSLINUX)
    #if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
       irqreturn_t     IrqValid = IRQ_NONE;
    #else
       BOOL            IrqValid = FALSE; /* If Irq is one of those recorded, this flag becomes TRUE */
    #endif
    #endif /* ST_OSLINUX */

    HDMI_Data_t * const HDMI_Data_p = (HDMI_Data_t *) Param;

    #if defined ST_OSLINUX
    STEVT_Notify(HDMI_Data_p->EventsHandle, HDMI_Data_p->HdmiInterrupt.EventID, STEVT_EVENT_DATA_TYPE_CAST NULL);
    #else
    STEVT_Notify(HDMI_Data_p->EventsHandle, HDMI_Data_p->HdmiInterrupt.EventID, (const void *) NULL);
    #endif

#if defined ST_OSLINUX
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
    IrqValid = IRQ_HANDLED;
#else
    IrqValid = FALSE;
#endif

    return (IrqValid);
#endif  /* ST_OSLINUX */

#if defined(ST_OS21) || defined(ST_OSWINCE)
    return(OS21_SUCCESS);
#endif /* ST_OS21 */

} /* End of SelfInstalledHdmiInterruptHandler() */
/******************************************************************************
Name        : SendNewInfoframe
Description : Send info Frame sorted by priority.
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SendNewInfoframe(const HDMI_Handle_t Handle)
{
    HDMI_Data_t * Handle_p;
#ifdef WA_GNBvd54182
    U32         ACRIntTimeNow=0,ACRIntTimeElapsed=0,PrevdelayLoop,delayLoop=0,ACRDeltaTime=0;
#endif
#if defined (WA_GNBvd44290)|| defined (WA_GNBvd54182)
    U32           ActualProg;
#endif
#ifdef WA_WITH_PREIT
    U32 CurrentTime;
#endif
    Handle_p = (HDMI_Data_t *)Handle;

    STOS_InterruptLock();

    Handle_p->WaitingForInfoFrameInt = TRUE;
    /* Check if info frame can be sent */
    if (Handle_p->InfoFrameStatus == 0)
    {
        Handle_p->WaitingForInfoFrameInt = FALSE;
        STOS_InterruptUnlock();
        return;
    }

     /* Each info frame are sorted by priority */
    /* General Control Packet: First packet to be transmitted after Vsync active edge (New Frame) */
    /* HDMI standard requires it to be send in the first 384 pixels                               */
    if ((Handle_p->InfoFrameStatus & HDMI_GENERAL_CONTROL) != 0)
    {
        HAL_AVMute(Handle_p->HalHdmiHandle, Handle_p->AVMute);
        Handle_p->Statistics.InfoFrameCountGeneralControl++;
        Handle_p->InfoFrameStatus ^= HDMI_GENERAL_CONTROL;
#if defined(STVOUT_INTERRUPT_TRACE)
       /* TraceState("I F","Gen Ctrl"); */
#endif
        /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,"GC INFOFRAME SENT"));*/
        /* NO return; because Doensn't generate another interrupt & doens't interfer */
        /* with the ones below when frame is beeing sent                             */
    }
#if defined (WA_GNBvd44290) || defined (WA_GNBvd54182)
    if ((Handle_p->InfoFrameStatus & HDMI_WA_ACR) != 0)
    {
        /* Accurate timing necessary for sending ACR in the interrupt */
        ActualProg = Handle_p->LastInterrupt + Handle_p->SleepTime;
#ifdef TRACE_UART
        if(ActualProg > C1_PWM_CNT())
        {
            if((ActualProg - C1_PWM_CNT()) > 900)
            {
                trace( "Interrupt is too early now %d expected %d\n\r", C1_PWM_CNT(), ActualProg);
            }
        }
        else
        {
            if((C1_PWM_CNT() - ActualProg) > 900)
            {
                trace( "Interrupt is too late now %d expected %d\n\r", C1_PWM_CNT(), ActualProg);
            }
        }
#endif
#if defined (WA_WITH_PREIT)&& defined(ST_7710)
#ifdef TRACE_PIO
        PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
        /* SET PIO FDMA */
        PIO_FDMA_REG = 0x10;
#endif /* TRACE_PIO */
        if(ActualProg < Handle_p->LastInterrupt)
        {
            while(C1_PWM_CNT() > Handle_p->SleepTime);
        }
        CurrentTime = C1_PWM_CNT();
        while((CurrentTime < ActualProg) &&
              ((ActualProg > CurrentTime) ? ((ActualProg - CurrentTime) < 1800) :
               ((CurrentTime - ActualProg) < 1800)))
        {
            CurrentTime = C1_PWM_CNT();
        }
#ifdef TRACE_PIO
        /* CLEAR PIO FDMA */
        PIO_FDMA_REG_CLEAR = 0x10;
#endif /* TRACE_PIO */

#endif /* WA_WITH_PREIT */
#ifdef ST_7710
        /* Program next interrupt now */
        Handle_p->LastInterrupt =  C1_PWM_CNT();
#ifdef WA_WITH_PREIT
        /* Program next interrupt now */
        C1_PWM_CMP1_VAL() = Handle_p->LastInterrupt + (Handle_p->SleepTime - 1600); /* 1600 is 256 us */
#else
        /* Program next interrupt now */
        C1_PWM_CMP1_VAL() = Handle_p->LastInterrupt + Handle_p->SleepTime;
#endif /* WA_WITH_PREIT */
#endif
#ifdef TRACE_UART
        if((C1_PWM_CMP1_VAL() < C1_PWM_CNT()) && (C1_PWM_CMP1_VAL() > (Handle_p->SleepTime - 1600)))
        {
            trace( "ACR Impossible : Should never happen !!!\n\r");
        }
#endif


#ifdef WA_GNBvd54182
        if(FirstTime)
       {
            FirstTime=0;
            ACRIntPrevTime= time_now();
            NbTicksPerSec= ST_GetClocksPerSecond();
            Refdelay= (U32)((3000)*NbTicksPerSec/(1000*1000));
        }
        else
        {
            ACRIntTimeNow= time_now();
            ACRDeltaTime=time_minus(ACRIntTimeNow,ACRIntPrevTime);

            ACRIntTimeElapsed=(U32)(1000*((double)ACRDeltaTime/((double)NbTicksPerSec/1000)));  /*time counter in us*/
            if(ACRDeltaTime < Refdelay)
            {
                PrevdelayLoop = time_now();
                do
                {
                    delayLoop= time_minus(time_now(),PrevdelayLoop);
                }while(delayLoop<(Refdelay-ACRDeltaTime));
            }
            ACRIntPrevTime= time_now();
        }
#endif

#ifdef TRACE_PIO
        /* Set PIO FDMA */
        PIO_FDMA_REG = 0x10;
        PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif /* TRACE_PIO */

        HAL_SendInfoFrame(Handle_p->HalHdmiHandle, Handle_p->InfoFrameBuffer[STVOUT_INFOFRAME_TYPE_ACR]);
        Handle_p->Statistics.InfoFrameCount[STVOUT_INFOFRAME_TYPE_ACR]++;
        Handle_p->InfoFrameStatus ^= HDMI_WA_ACR;

#if defined(STVOUT_INTERRUPT_TRACE)
                    TraceState("I F "," ACR");
#endif
#ifdef TRACE_PIO
        /* CLEAR PIO FDMA */
        PIO_FDMA_REG_CLEAR = 0x10;
#endif /* TRACE_PIO */
         Handle_p->WaitingForInfoFrameInt = FALSE;
        STOS_InterruptUnlock();
        return;
    }
#endif /* WA_GNBvd44290 */
    if ((Handle_p->InfoFrameStatus & HDMI_AVI_FRAME) != 0)
    {
        hdmi_Stat[2][0]++;
        hdmi_Stat[2][1] = time_now();
        HAL_SendInfoFrame(Handle_p->HalHdmiHandle, Handle_p->InfoFrameBuffer[STVOUT_INFOFRAME_TYPE_AVI]);
        Handle_p->Statistics.InfoFrameCount[STVOUT_INFOFRAME_TYPE_AVI]++;
        Handle_p->InfoFrameStatus ^= HDMI_AVI_FRAME;
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("I F","AVI");
#endif
        Handle_p->WaitingForInfoFrameInt = FALSE;
        STOS_InterruptUnlock();
        return;
    }
    if ((Handle_p->InfoFrameStatus & HDMI_AUDIO_FRAME) != 0)
    {
        hdmi_Stat[3][0]++;
        hdmi_Stat[3][1] = time_now();
        HAL_SendInfoFrame(Handle_p->HalHdmiHandle, Handle_p->InfoFrameBuffer[STVOUT_INFOFRAME_TYPE_AUDIO]);
        Handle_p->Statistics.InfoFrameCount[STVOUT_INFOFRAME_TYPE_AUDIO]++;
        Handle_p->InfoFrameStatus ^= HDMI_AUDIO_FRAME;
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("I F","Audio");
#endif
        Handle_p->WaitingForInfoFrameInt = FALSE;
        STOS_InterruptUnlock();
        return;
    }

    if ((Handle_p->InfoFrameStatus & HDMI_SPD_FRAME) != 0)
    {
        hdmi_Stat[4][0]++;
        hdmi_Stat[4][1] = time_now();
        HAL_SendInfoFrame(Handle_p->HalHdmiHandle, Handle_p->InfoFrameBuffer[STVOUT_INFOFRAME_TYPE_SPD]);
        Handle_p->Statistics.InfoFrameCount[STVOUT_INFOFRAME_TYPE_SPD]++;
        Handle_p->InfoFrameStatus ^= HDMI_SPD_FRAME;
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("I F","SPD");
#endif
        Handle_p->WaitingForInfoFrameInt = FALSE;
        STOS_InterruptUnlock();
        return;
    }
    if ((Handle_p->InfoFrameStatus & HDMI_MPEG_FRAME) != 0)
    {
        hdmi_Stat[5][0]++;
        hdmi_Stat[5][1] = time_now();
        HAL_SendInfoFrame(Handle_p->HalHdmiHandle, Handle_p->InfoFrameBuffer[STVOUT_INFOFRAME_TYPE_MPEG]);
        Handle_p->Statistics.InfoFrameCount[STVOUT_INFOFRAME_TYPE_MPEG]++;
        Handle_p->InfoFrameStatus ^= HDMI_MPEG_FRAME;

#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("I F","MPEG");
#endif
        Handle_p->WaitingForInfoFrameInt = FALSE;
        STOS_InterruptUnlock();
        return;
    }
    if ((Handle_p->InfoFrameStatus & HDMI_VS_FRAME) != 0)
    {
        hdmi_Stat[6][0]++;
        hdmi_Stat[6][1] = time_now();
        HAL_SendInfoFrame(Handle_p->HalHdmiHandle, Handle_p->InfoFrameBuffer[STVOUT_INFOFRAME_TYPE_VS]);
        Handle_p->Statistics.InfoFrameCount[STVOUT_INFOFRAME_TYPE_VS]++;
        Handle_p->InfoFrameStatus ^= HDMI_VS_FRAME;
#if defined(STVOUT_INTERRUPT_TRACE)
        TraceState("I F","VS");
#endif
        Handle_p->WaitingForInfoFrameInt = FALSE;
        STOS_InterruptUnlock();
        return;
    }

    Handle_p->WaitingForInfoFrameInt = FALSE;
    STOS_InterruptUnlock();
    return;
} /* end of SendNewInfoframe()*/

/******************************************************************************
Name        : WA_GNBvd44290_Interrupt
Description : Send ACR info Frame.
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
#if defined (WA_GNBvd44290)|| defined (WA_GNBvd54182)

static void WA_GNBvd44290_Interrupt(void * Param)
{
    HDMI_Data_t  *Handle_p;
#ifdef WA_GNBvd44290
    U32         Count_PixelHD, Count_PCM;
#endif
    static  U8  ACRBuffer[12];

#ifdef TRACE_UART
    static U32 PrevCount_PixelHD, PrevCount_PCM;
#endif

    Handle_p = (HDMI_Data_t *)Param;

    /* Set header for first 4 bytes */
    *(U32*)(&ACRBuffer[0]) = 0x1;
    *(U32*)(&ACRBuffer[4]) = Handle_p->NValue;


#ifdef WA_GNBvd44290
    clkrv_WA_GNBvd44290 (&Count_PixelHD, &Count_PCM);
     if((Count_PixelHD != 0) && (Count_PCM != 0))
    {
        Handle_p->CTSValue = (U32)(((double) Count_PixelHD * (double)Handle_p->NValue * 100) / (double)Count_PCM);
        #ifdef ST_7710
        if (FS_CLOCKGEN_CFG_2 & 0x4)
        {
            Handle_p->CTSValue /= 2;
        }
        #endif
        Handle_p->CTSOffset += (U32)(Handle_p->CTSValue - ((Handle_p->CTSValue / 100) * 100));
        Handle_p->CTSValue = Handle_p->CTSValue / 100;
        if(Handle_p->CTSOffset >= 100)
        {
            Handle_p->CTSOffset -= 100;
            Handle_p->CTSValue++;
        }
    }
#endif
#ifdef TRACE_UART
    if((((PrevCTS - Handle_p->CTSValue) > 1) && (Handle_p->CTSValue < PrevCTS)) ||
       (((Handle_p->CTSValue - PrevCTS) > 1) && (Handle_p->CTSValue > PrevCTS)) ||
       (PrevCount_PixelHD != Count_PixelHD) || (PrevCount_PCM != Count_PCM)
        )
    {
        trace("Sleep=%d us N=%d CntHD=%u CntPCM=%u CTS=%u Delta=%d Off=%d\r\n",
              Handle_p->SleepTime*1000000/ST_GetClocksPerSecond(),
              Handle_p->NValue, Count_PixelHD, Count_PCM,
              Handle_p->CTSValue, Handle_p->CTSValue - PrevCTS,
              Handle_p->CTSOffset
            );
        PrevCount_PixelHD = Count_PixelHD;
        PrevCount_PCM = Count_PCM;
    }
#endif /* TRACE_UART */
    if(Handle_p->CTSValue != PrevCTS)
    {
        *(U32*)(&ACRBuffer[8]) = Handle_p->CTSValue;
        HAL_ComputeInfoFrame(Handle_p->HalHdmiHandle, ACRBuffer,
                             Handle_p->InfoFrameBuffer[STVOUT_INFOFRAME_TYPE_ACR], 12-4);
        PrevCTS = Handle_p->CTSValue;

    }

    Handle_p->InfoFrameStatus |= HDMI_WA_ACR;
    if(!Handle_p->WaitingForInfoFrameInt)
    {
        SendNewInfoframe(Handle_p);
    }
}
#endif
/******************************************************************************
Name        : WA_GNBvd44290_Install
Description : Install Call back of C1_timer interrupt.
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
#ifdef WA_GNBvd44290
ST_ErrorCode_t WA_GNBvd44290_Install(const HDMI_Handle_t Handle)
{
    HDMI_Data_t * HDMI_Data_p = (HDMI_Data_t *) Handle;

    stboot_WA_GNBvd44290_InstallCallback(WA_GNBvd44290_Interrupt, HDMI_Data_p);
    HDMI_Data_p->LastInterrupt = C1_PWM_CNT();
    C1_PWM_CMP1_VAL() = HDMI_Data_p->LastInterrupt + HDMI_Data_p->SleepTime;
#ifdef TRACE_UART
    trace( "ACR WA Start at %d\n\r", HDMI_Data_p->LastInterrupt);
#endif
    C1_PWM_INT_EN() = C1_PWM_INT_EN() | (PWM_INT_BIT_CMP1);
    return(ST_NO_ERROR);
}

/******************************************************************************
Name        : WA_GNBvd44290_UnInstall
Description : Uninstall Call back of C1_timer interrupt.
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/

ST_ErrorCode_t WA_GNBvd44290_UnInstall(const HDMI_Handle_t Handle)
{
#ifdef TRACE_UART
    HDMI_Data_t * HDMI_Data_p = (HDMI_Data_t *) Handle;

    trace( "ACR WA Stopped at %d\n\r", HDMI_Data_p->LastInterrupt);
#endif
    C1_PWM_INT_EN() = C1_PWM_INT_EN() & (~(PWM_INT_BIT_CMP1));
    stboot_WA_GNBvd44290_UnInstallCallback();
    return(ST_NO_ERROR);
}
#endif /* WA_GNBvd44290 */
#if defined (WA_GNBvd54182) || defined (WA_GNBvd56512)
/*******************************************************************************
Name        : WA_PWMInterruptInstall
Description : Install Call back of pwm interrupt.
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
ST_ErrorCode_t WA_PWMInterruptInstall(const HDMI_Handle_t Handle)
{
    HDMI_Data_t * HDMI_Data_p = (HDMI_Data_t *) Handle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STPWM_OpenParams_t          PwmOpenParams;
    STPWM_CallbackParam_t       PwmCallbackParams;
    char PWMDeviceName[5];

    if (!HDMI_Data_p->IsPWMInitialized)
    {
        HDMI_Data_p->IsPWMInitialized = TRUE;
        PwmOpenParams.C4.ChannelNumber = STPWM_CHANNEL_0;
        PwmOpenParams.C4.PrescaleFactor = 0x69;
        PwmOpenParams.PWMUsedFor = STPWM_Timer;
        strncpy(PWMDeviceName, "PWM", sizeof(PWMDeviceName));
        ErrorCode = STPWM_Open (PWMDeviceName, &PwmOpenParams, &HDMI_Data_p->PwmHandle);
        if (ErrorCode != ST_NO_ERROR)
            return(ST_ERROR_OPEN_HANDLE);

        /* Set PWM Mark to Space Ratio to 50:50 */
        ErrorCode= STPWM_SetRatio (HDMI_Data_p->PwmHandle, STPWM_MARK_MAX / 2);
        if (ErrorCode != ST_NO_ERROR)
            return(ST_ERROR_BAD_PARAMETER);

        #ifdef WA_GNBvd56512
        PwmCallbackParams.CallBack = WA_GNBvd56512_Interrupt;
        #endif
        #ifdef WA_GNBvd54182
        PwmCallbackParams.CallBack = WA_GNBvd44290_Interrupt;
        #endif
        PwmCallbackParams.CallbackData_p = HDMI_Data_p;
        ErrorCode = STPWM_SetCallback(HDMI_Data_p->PwmHandle,&PwmCallbackParams, STPWM_Timer);
        if (ErrorCode != ST_NO_ERROR)
            return(ST_ERROR_BAD_PARAMETER);

    }
    return(ErrorCode);
}/* end of WA_PWMInterruptInstall() */
/*******************************************************************************
Name        : WA_PWMInterruptUnInstall
Description : UnInstall the Call back of PWM timer interrupt.
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
ST_ErrorCode_t WA_PWMInterruptUnInstall(const HDMI_Handle_t Handle)
{
    HDMI_Data_t * HDMI_Data_p;
    STPWM_CallbackParam_t       PwmCallbackParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_p = (HDMI_Data_t *) Handle;

    if (HDMI_Data_p->IsPWMInitialized)
    {
       HDMI_Data_p->IsPWMInitialized =FALSE;

       PwmCallbackParams.CallBack = 0;
       PwmCallbackParams.CallbackData_p = 0;
       ErrorCode = STPWM_SetCallback(HDMI_Data_p->PwmHandle,&PwmCallbackParams, STPWM_Timer);
        if (ErrorCode != ST_NO_ERROR)
            return(ST_ERROR_BAD_PARAMETER);

       ErrorCode = STPWM_Close (HDMI_Data_p->PwmHandle);
       if (ErrorCode != ST_NO_ERROR)
            return(ST_ERROR_BAD_PARAMETER);

    }
    return(ST_NO_ERROR);
}/* end of WA_PWMInterruptUnInstall()*/
#endif
#ifdef WA_GNBvd56512
/*******************************************************************************
Name        : WA_GNBvd56512_Interrupt
Description : Install the Call back for PWM timer interrupt regarding the HDCP WA
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static  void  WA_GNBvd56512_Interrupt(void * Param)
{

HDMI_Data_t  *Handle_p = (HDMI_Data_t *)Param;

#if (STCLKRV_EXT_CLKB_MHZ==27)
                switch(Handle_p->PixelClock)
                {

                     case 0:  /* PIXEL_CLOCK_25200 */
                            /*Sol for Pix_HD= 25.200MHz :PE_NormalValue=28087 ,Fine tuning of Pix clocks*/
#ifdef TRACE_PIO
						PIO_FDMA_REG = 0x10;
                        PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif
                        if(PE_NormalValue)
                        {
                            Nb_NormalValue++;
                            Pe_Value=*((volatile int*) 0xb900001C);
                            if(Nb_NormalValue == 6)
                            {
                                PE_NormalValue=0;
                                Nb_NormalValue=0;
                                Pe_Value--;
                            }
                        }
                        else
                        {
                            Nb_NonNormalValue++;
                            Pe_Value=*((volatile int*) 0xb900001C);
                            if(Nb_NonNormalValue == 1)
                            {
                                Nb_NonNormalValue=0;
                                Pe_Value++;
                                PE_NormalValue=1;
                            }
                        }
                        *((volatile int*) 0xb9000010)=0xC0DE;
                        *((volatile int*) 0xb9000020)=0x0;
                        *((volatile int*) 0xb900001C)=Pe_Value;
                        *((volatile int*) 0xb9000020)=0x1;
#ifdef TRACE_PIO
						PIO_FDMA_REG_CLEAR = 0x10;
#endif
                    break;

                    case 1:  /* PIXEL_CLOCK_25174 */
                            /*Sol for Pix_HD= 25.176MHz :PE_NormalValue=27530   ,Fine tuning of Pix clocks*/
#ifdef TRACE_PIO
                        PIO_FDMA_REG = 0x10;
                        PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif
						if(PE_NormalValue)
                        {
                            Nb_NormalValue++;
                            Pe_Value=*((volatile int*) 0xb900001C);
                            if(Nb_NormalValue <= 59)
                            {
                                PE_NormalValue=0;
                                Pe_Value-=2;
                            }
                            else if(Nb_NormalValue == 63)
                            {
                                Nb_NormalValue=0;
                                PE_NormalValue=0;
                                Pe_Value-=2;
                            }
                        }
                        else
                        {
                            Nb_NonNormalValue++;
                            Pe_Value=*((volatile int*) 0xb900001C);
                            if(Nb_NonNormalValue == 1)
                            {
                                Pe_Value+=2;
                                PE_NormalValue=1;
                                Nb_NonNormalValue=0;
                            }
                        }
                        *((volatile int*) 0xb9000010)=0xC0DE;
                        *((volatile int*) 0xb9000020)=0x0;
                        *((volatile int*) 0xb900001C)=Pe_Value;
                        *((volatile int*) 0xb9000020)=0x1;
#ifdef TRACE_PIO
                        PIO_FDMA_REG_CLEAR = 0x10;
#endif
                    break;
                    case 2: /* PIXEL_CLOCK_27000 */
                           /*nothing to do for 27MHz*/
                    break;
                    case 3:/* PIXEL_CLOCK_27027 */
                            /*Sol1  for Pix_HD= 27.027MHz :PE_NormalValue=1048 ,Fine tuning of Pix clocks*/
#ifdef TRACE_PIO
                        PIO_FDMA_REG = 0x10;
                        PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif
                        if(PE_NormalValue)
                        {
                            Nb_NormalValue++;
                            Pe_Value=*((volatile int*) 0xb900001C);
                            if(Nb_NormalValue == 9)
                            {
                               PE_NormalValue=0;
                               Nb_NormalValue=0;
                               Pe_Value--;
                            }

                        }
                        else
                        {
                            Nb_NonNormalValue++;
                            Pe_Value=*((volatile int*) 0xb900001C);

                            if(CounterAdjust==32)
                            {
                                NumberX= 9;
                            }
                            else
                            {
                                NumberX= 8;
                            }
                            if(Nb_NonNormalValue == NumberX)
                            {
                                CounterAdjust++;
                                if(CounterAdjust==33)
                                    CounterAdjust=0;

                                PE_NormalValue=1;
                                Nb_NonNormalValue=0;
                                Pe_Value++;
                            }

                        }
                        *((volatile int*) 0xb9000010)=0xC0DE;
                        *((volatile int*) 0xb9000020)=0x0;
                        *((volatile int*) 0xb900001C)=Pe_Value;
                        *((volatile int*) 0xb9000020)=0x1;
#ifdef TRACE_PIO
                        PIO_FDMA_REG_CLEAR = 0x10;
#endif
                    break;
                    case 6: /* PIXEL_CLOCK_74176 */
#ifdef TRACE_PIO
                        PIO_FDMA_REG = 0x10;
                        PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif
                        if(PE_NormalValue)
                        {
                                Nb_NormalValue++;
                                Pe_Value=*((volatile int*) 0xb900001C);
                                PE_NormalValue=0;
                                Nb_NormalValue=0;
                                Pe_Value--;
                        }
                        else
                        {
                            Nb_NonNormalValue++;
                            Pe_Value=*((volatile int*) 0xb900001C);
                            if(Nb_NonNormalValue != 13)
                            {
                                Pe_Value++;
                                PE_NormalValue=1;
                            }
                            else
                                Nb_NonNormalValue=0;
                        }
                        *((volatile int*) 0xb9000010)=0xC0DE;
                        *((volatile int*) 0xb9000020)=0x0;
                        *((volatile int*) 0xb900001C)=Pe_Value;
                        *((volatile int*) 0xb9000020)=0x1;
#ifdef TRACE_PIO
                        PIO_FDMA_REG_CLEAR = 0x10;
#endif
                    break;

                    case 7: /* PIXEL_CLOCK_74250 */
                            /*Sol for Pix_HD= 74.25MHz :PE_NormalValue=23831   ,Fine tuning of Pix clocks ,Test OK: 41h*/
#ifdef TRACE_PIO
                        PIO_FDMA_REG = 0x10;
                        PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif
                        if(PE_NormalValue)
                        {
                            Nb_NormalValue++;
                            Pe_Value=*((volatile int*) 0xb900001C);
                            if(Nb_NormalValue == 8)
                            {
                                PE_NormalValue=0;
                                Nb_NormalValue=0;
                                Pe_Value++;
                            }
                        }
                        else
                        {
                            Nb_NonNormalValue++;
                            Pe_Value=*((volatile int*) 0xb900001C);
                            if(Nb_NonNormalValue == 3)
                            {
                                Nb_NonNormalValue=0;
                                Pe_Value--;
                                PE_NormalValue=1;
                            }
                        }
                        *((volatile int*) 0xb9000010)=0xC0DE;
                        *((volatile int*) 0xb9000020)=0x0;
                        *((volatile int*) 0xb900001C)=Pe_Value;
                        *((volatile int*) 0xb9000020)=0x1;
#ifdef TRACE_PIO
                        PIO_FDMA_REG_CLEAR = 0x10;
#endif
                        break;
               default:
                    break;

                }
                if(Handle_p->Out.HDMI.AudioFrequency == 44100)
                {
#ifdef TRACE_PIO
                    PIO_FDMA_REG = 0x10;
                    PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif
                    /* Update Audio clocks*/
                    if(PE_6MNormalValue)
                    {
                            Nb_6MNormalValue++;
                            Pe6M_Value=*((volatile int*) 0xB9210014);
                            if(N_6MCounterAdjust==14)
                            {
                                N_6MNumberX= 6;
                            }
                            else
                            {
                                N_6MNumberX= 5;
                            }

                            if(Nb_6MNormalValue == N_6MNumberX)
                            {
                                N_6MCounterAdjust++;
                                if(N_6MCounterAdjust==15)
                                    N_6MCounterAdjust=0;

                                PE_6MNormalValue=0;
                                Nb_6MNormalValue=0;
                                Pe6M_Value+=2;
                            }
                    }
                    else
                    {
                        Nb_6MNonNormalValue++;
                        Pe6M_Value=*((volatile int*) 0xB9210014);
                        if(Nb_6MNonNormalValue == 8)
                        {
                            Nb_6MNonNormalValue=0;
                            Pe6M_Value-=2;
                            PE_6MNormalValue=1;
                        }
                    }
                    *((volatile int*) 0xB921001C)=0x0;
                    *((volatile int*) 0xB9210014)=Pe6M_Value;
                    *((volatile int*) 0xB921001C)=0x1;

                    *((volatile int*) 0xB921003C)=0x0;
                    *((volatile int*) 0xB9210034)=Pe6M_Value;
                    *((volatile int*) 0xB921003C)=0x1;
#ifdef TRACE_PIO
                    PIO_FDMA_REG_CLEAR = 0x10;
#endif
                }

#else /*(STCLKRV_EXT_CLKB_MHZ==30)*/
                switch(Handle_p->PixelClock)
                {
                     case 0:  /* PIXEL_CLOCK_25200 */
                            /*Sol for Pix_HD= 25.200MHz :PE_NormalValue=31208,   Fine tuning of Pix clocks*/
#ifdef TRACE_PIO
                        PIO_FDMA_REG = 0x10;
                        PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif
                        Pe_Value=*((volatile int*) 0xb900001C);
                        if((Pe_Value >= (31208-5))&& (Pe_Value <= (31208+5)))
                        {
                            if(PE_NormalValue)
                            {
                                Nb_NormalValue++;
                                Pe_Value=*((volatile int*) 0xb900001C);
                                if(Nb_NormalValue == 17)
                                {
                                    PE_NormalValue=0;
                                    Nb_NormalValue=0;
                                    Pe_Value-=2;
                                }
                            }
                            else
                            {
                                Nb_NonNormalValue++;
                                Pe_Value=*((volatile int*) 0xb900001C);
                                if(Nb_NonNormalValue == 4)
                                {
                                    Nb_NonNormalValue=0;
                                    Pe_Value+=2;
                                    PE_NormalValue=1;
                                }
                            }
                            *((volatile int*) 0xb9000010)=0xC0DE;
                            *((volatile int*) 0xb9000020)=0x0;
                            *((volatile int*) 0xb900001C)=Pe_Value;
                            *((volatile int*) 0xb9000020)=0x1;
                        }
#ifdef TRACE_PIO
                        PIO_FDMA_REG_CLEAR = 0x10;
#endif
                    break;
                    case 1:  /* PIXEL_CLOCK_25174 */
                            /*Sol for Pix_HD= 25.176MHz :PE_NormalValue=27530  ,Fine tuning of Pix clocks*/
                            /*not Covered by the HDCP WA*/
                    break;
                    case 2: /* PIXEL_CLOCK_27000 */
                            /*Sol for Pix_HD= 27MHz :PE_NormalValue=7282   ,Fine tuning of Pix clocks*/
                        Pe_Value=*((volatile int*) 0xb900001C);
                        if((Pe_Value >= (7282-5))&& (Pe_Value <= (7282+5)))
                        {
                            if(PE_NormalValue)
                            {
                                    Nb_NormalValue++;
                                    Pe_Value=*((volatile int*) 0xb900001C);
                                    if(Nb_NormalValue == 14)
                                    {
                                        PE_NormalValue=0;
                                        Nb_NormalValue=0;
                                        Pe_Value--;
                                    }
                            }
                            else
                            {
                                Nb_NonNormalValue++;
                                Pe_Value=*((volatile int*) 0xb900001C);
                                if(Nb_NonNormalValue == 4)
                                {
                                    Nb_NonNormalValue=0;
                                    Pe_Value++;
                                    PE_NormalValue=1;
                                }
                            }
                            *((volatile int*) 0xb9000010)=0xC0DE;
                            *((volatile int*) 0xb9000020)=0x0;
                            *((volatile int*) 0xb900001C)=Pe_Value;
                            *((volatile int*) 0xb9000020)=0x1;
                        }
                    break;
                    case 3:/* PIXEL_CLOCK_27027 */
                            /*Sol1  for Pix_HD= 27.027MHz :PE_NormalValue=7864 ,Fine tuning of Pix clocks*/
#ifdef TRACE_PIO
                        PIO_FDMA_REG = 0x10;
                        PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif
                        Pe_Value=*((volatile int*) 0xb900001C);
                        if((Pe_Value >= (7864-5))&& (Pe_Value <= (7864+5)))
                        {
                            if(PE_NormalValue)
                            {
                                Nb_NormalValue++;
                                Pe_Value=*((volatile int*) 0xb900001C);
                                if(Nb_NormalValue == 22)
                                {
                                    NumberX= 16;
                                }
                                else
                                {
                                    NumberX= 14;
                                }
                                if(Nb_NormalValue == NumberX)
                                {
                                    CounterAdjust++;
                                    if(CounterAdjust==23)
                                        CounterAdjust=0;
                                PE_NormalValue=0;
                                Nb_NormalValue=0;
                                Pe_Value--;
                                }
                            }
                            else
                            {
                                Nb_NonNormalValue++;
                                Pe_Value=*((volatile int*) 0xb900001C);
                                if(Nb_NonNormalValue == 5)
                                {
                                    Nb_NonNormalValue=0;
                                    Pe_Value++;
                                    PE_NormalValue=1;
                                }
                            }
                            *((volatile int*) 0xb9000010)=0xC0DE;
                            *((volatile int*) 0xb9000020)=0x0;
                            *((volatile int*) 0xb900001C)=Pe_Value;
                            *((volatile int*) 0xb9000020)=0x1;
                        }
#ifdef TRACE_PIO
                        PIO_FDMA_REG_CLEAR = 0x10;
#endif
                    break;
                    case 6: /* PIXEL_CLOCK_74176 */
                        /*not Covered by the HDCP WA*/
                    break;

                    case 7: /* PIXEL_CLOCK_74250 */
                            /*Sol for Pix_HD= 74.25MHz :PE_NormalValue=4634*/   /*Fine tuning of Pix clocks*/ /*Test OK: 41h*/
#ifdef TRACE_PIO
                        PIO_FDMA_REG = 0x10;
                        PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif
                        Pe_Value=*((volatile int*) 0xb900001C);
                        if((Pe_Value >= (4634-5))&& (Pe_Value <= (4634+5)))
                        {
                            if(PE_NormalValue)
                            {
                                    Nb_NormalValue++;
                                    Pe_Value=*((volatile int*) 0xb900001C);
                                    if(CounterAdjust==13)
                                    {
                                        NumberX=7;
                                    }
                                    else
                                    {
                                        NumberX= 6;
                                    }
                                    if(Nb_NormalValue == NumberX)
                                    {
                                        CounterAdjust++;
                                        if(CounterAdjust==14)
                                            CounterAdjust=0;

                                        PE_NormalValue=0;
                                        Nb_NormalValue=0;
                                        Pe_Value--;
                                    }
                            }
                            else
                            {
                                Nb_NonNormalValue++;
                                Pe_Value=*((volatile int*) 0xb900001C);
                                if(Nb_NonNormalValue == 1)
                                {
                                    Pe_Value++;
                                    PE_NormalValue=1;
                                    Nb_NonNormalValue=0;
                                }
                            }
                            *((volatile int*) 0xb9000010)=0xC0DE;
                            *((volatile int*) 0xb9000020)=0x0;
                            *((volatile int*) 0xb900001C)=Pe_Value;
                            *((volatile int*) 0xb9000020)=0x1;
                        }
#ifdef TRACE_PIO
                        PIO_FDMA_REG_CLEAR = 0x10;
#endif
                        break;
               default:
                    break;

                }
                    /* Fine tune the SD_Clock: 27MHz */
                if(PE_27MNormalValue)
                {
                        Nb_27MNormalValue++;
                        Pe27M_Value=*((volatile int*) 0xb9000064);
                        if(Nb_27MNormalValue == 14)
                        {
                            PE_27MNormalValue=0;
                            Nb_27MNormalValue=0;
                            Pe27M_Value--;
                        }
                }
                else
                {
                    Nb_27MNonNormalValue++;
                    Pe27M_Value=*((volatile int*) 0xb9000064);
                    if(Nb_27MNonNormalValue == 4)
                    {
                        Nb_27MNonNormalValue=0;
                        Pe27M_Value++;
                        PE_27MNormalValue=1;
                    }
                }
                *((volatile int*) 0xb9000010)=0xC0DE;
                *((volatile int*) 0xb9000068)=0x0;
                *((volatile int*) 0xb9000064)=Pe27M_Value;
                *((volatile int*) 0xb9000068)=0x1;


                if(Handle_p->Out.HDMI.AudioFrequency == 44100)
                {
#ifdef TRACE_PIO
                    PIO_FDMA_REG = 0x10;
                    PIO_TRACE_REG = (0x10 | PIO_TRACE_REG);
#endif
                    /* Update Audio clocks*/
                    if(PE_6MNormalValue)
                    {
                            Nb_6MNormalValue++;
                            Pe6M_Value=*((volatile int*) 0xB9210014);
                            if((N_6MCounterAdjust==9)|| (N_6MCounterAdjust==19)||(N_6MCounterAdjust==29)||(N_6MCounterAdjust==39)||(N_6MCounterAdjust==52))
                            {
                                N_6MNumberX= 8;
                            }
                            else
                            {
                                N_6MNumberX= 7;
                            }

                            if(Nb_6MNormalValue == N_6MNumberX)
                            {
                                N_6MCounterAdjust++;
                                if(N_6MCounterAdjust==53)
                                    N_6MCounterAdjust=0;
                                PE_6MNormalValue=0;
                                Nb_6MNormalValue=0;
                                Pe6M_Value++;
                            }
                    }
                    else
                    {
                        Nb_6MNonNormalValue++;
                        Pe6M_Value=*((volatile int*) 0xB9210014);
                        if(Nb_6MNonNormalValue == 4)
                        {
                            Nb_6MNonNormalValue=0;
                            Pe6M_Value--;
                            PE_6MNormalValue=1;
                        }
                    }
                    *((volatile int*) 0xB921001C)=0x0;
                    *((volatile int*) 0xB9210014)=Pe6M_Value;
                    *((volatile int*) 0xB921001C)=0x1;

                    *((volatile int*) 0xB921003C)=0x0;
                    *((volatile int*) 0xB9210034)=Pe6M_Value;
                    *((volatile int*) 0xB921003C)=0x1;
#ifdef TRACE_PIO
                    PIO_FDMA_REG_CLEAR = 0x10;
#endif
                }

#endif

}/* WA_GNBvd56512_Interrupt */
/*******************************************************************************
Name        : GetCountHD_PCM
Description : It's just for debug
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void GetCountHD_PCM(void)
{
    U32	 Count_PixelHD, Count_PCM;
	U32  Count_PixelHD1=0,Error_Pix=0;

	clkrv_WA_GNBvd44290 (&Count_PixelHD, &Count_PCM);
	if(Count_PixelHD !=0)
	{
#if (STCLKRV_EXT_CLKB_MHZ==27)
		Count_PixelHD1= (U32)Count_PixelHD/25;
		Error_Pix = (Count_PixelHD - 25*Count_PixelHD1);
		STTBX_Print(("CPixelHD= %d .. C_PCM= %d .. Error_Pix= %d\n",(int)(Count_PixelHD1),(int)(Count_PCM/25),Error_Pix));
#else
        Count_PixelHD1= (U32)Count_PixelHD/10;
        Error_Pix = (Count_PixelHD - 10*Count_PixelHD1);
        STTBX_Print(("CPixelHD= %d .. C_PCM= %d .. Error_Pix= %d\n",(int)(Count_PixelHD1),(int)(Count_PCM/5),Error_Pix));
#endif
	}

 } /*GetCountHD_PCM */
#endif/* WA_GNBvd56512 */
/* End of intr.c file */
/* --------------------------------- End of file ---------------------------- */
