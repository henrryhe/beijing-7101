/*******************************************************************************
File name   : hdmi_api.h

Description : VOUT driver header file for HDMI block

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
03 May 2004        Created                                          AC
17 Nov 2004        Add Support of ST40/OS21                         AC
*******************************************************************************/

#ifndef __API_H
#define __API_H

/* Includes ----------------------------------------------------------------- */
#include "hdmi.h"
#if defined (STVOUT_HDCP_PROTECTION)
#include "hdcp.h"
#endif
#include "vout_drv.h"
#include "hal_hdmi.h"
#include "stevt.h"
#if !defined (ST_OSLINUX)
#ifdef STVOUT_CEC_PIO_COMPARE
#include "stpwm.h"
#endif
#endif /* ST_OSLINUX */
#ifdef STVOUT_CEC_PIO_COMPARE
#include "cec.h"
#endif

#include "sttbx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

#ifndef STVOUT_TASK_HDCP_PRIORITY
#if defined (ST_OS20)
#define STVOUT_TASK_HDCP_PRIORITY   14
#endif
#if defined (ST_OS21)
#define STVOUT_TASK_HDCP_PRIORITY   250
#endif
#if defined (ST_OSLINUX)
#define STVOUT_TASK_HDCP_PRIORITY   25
#endif
#endif

#ifdef STVOUT_CEC_PIO_COMPARE
#ifndef STVOUT_TASK_CEC_PRIORITY
#if defined (ST_OS20)
#define STVOUT_TASK_CEC_PRIORITY   7
#endif
#if defined (ST_OS21)
#define STVOUT_TASK_CEC_PRIORITY   125
#endif
#if defined (ST_OSLINUX)
#define STVOUT_TASK_CEC_PRIORITY   12
#endif
#endif
#endif

#ifndef STVOUT_TASK_INTERRUPT_PROCESS_PRIORITY
#ifdef ST_OS20
#define STVOUT_TASK_INTERRUPT_PROCESS_PRIORITY      15
#endif
#ifdef ST_OS21
#define STVOUT_TASK_INTERRUPT_PROCESS_PRIORITY      255
#endif
#if defined (ST_OSLINUX)
/*#define STVOUT_AUTOMATIC_INFOFRAME_THREAD_PRIORITY  30*/
#define STVOUT_TASK_INTERRUPT_PROCESS_PRIORITY  60
#endif /* ST_OSLINUX */
#endif

/* Max 6 IT's before any of them are processed ? */
/* we do not need that much interrupt buffer
  interrupt should be process asap */
#define INTERRUPTS_BUFFER_SIZE           6
#define CONTROLLER_COMMANDS_BUFFER_SIZE  6

#define DEFAULT_FILL_VALUE_BEFORE_MEMORY_DEALLOCATION  0xc3

#if defined (ST_OSLINUX)
#define STHDMI_IRQ_NAME                                      "sthdmi"
#endif /* ST_OSLINUX */

#ifndef  MAX_BYTE_READ
#define  MAX_BYTE_READ                     256
#endif

#ifndef  I2C_BASE_ADDRESS
#define  I2C_BASE_ADDRESS                  0xA0

#endif

#define  HDMI_AUDIO_N                      0x1800
#define  HDMI_GP_CONFIG                    0x01

#ifndef  DEFAULT_CHL0_DATA
#define  DEFAULT_CHL0_DATA                 0x000000AA
#endif
#ifndef  DEFAULT_CHL1_DATA
#define  DEFAULT_CHL1_DATA                 0x00000055
#endif

#ifndef  DEFAULT_CHL2_DATA
#define  DEFAULT_CHL2_DATA                 0x00000054
#endif

/* Private variables (static) --------------------------------------------- */

/* Global Variables ------------------------------------------------------- */

enum
{
  HDMI_CHANGE_STATE_EVT_ID ,
  HDMI_REVOKED_KSV_EVT_ID,
  HDMI_CEC_MESSAGE_EVT_ID,
  HDMI_CEC_LOGIC_ADDRESS_NOTIFY_EVT_ID,
  HDMI_NB_REGISTERED_EVENTS_IDS
};
enum
{
   HDMI_SELF_INSTALLED_INTERRUPT_EVT
};

typedef enum HDMI_State_e
{
    HDMI_RESETTING, /* hdmi cell's soft reset in progress.              */
    HDMI_RUNNING,   /* Normal mode of hdmi's cell.                      */
    HDMI_STOPPING   /* hdmi's cell is stopping (or is being deleted).   */
}HDMI_State_t;

typedef struct HDMI_Interrupt_s {
    U32  Mask;                  /* Always contains the value of the interrupt mask, avoids reading it everytime */
    U32  Status;
    BOOL IsInside;
    S32  EnableCount;
    BOOL InstallHandler;       /* FALSE when using interrupt events, TRUE when installing handler*/
    U32  Level;                /* Valid if InstallHandler is TRUE */
    U32  Number;               /* Valid if InstallHandler is TRUE */
    STEVT_EventID_t EventID;   /* Valid if InstallHandler is TRUE */
} HDMI_Interrupt_t;

typedef struct HDMI_Task_s {

    task_t*                         Task_p;
    void*                           TaskStack;
    tdesc_t                         TaskDesc;
    BOOL    IsRunning;        /* TRUE if task is running, FALSE otherwise */
    BOOL    ToBeDeleted;      /* Set TRUE to ask task to end in order to delete it */
} HDMI_Task_t;

typedef enum ControllerCommandID_e {
    CONTROLLER_COMMAND_RESET,
    CONTROLLER_COMMAND_HPD,
    CONTROLLER_COMMAND_PIX_CAPTURE,
    NB_CONTROLLER_COMMANDS   /* Keep this one as the last one */
} ControllerCommandID_t;

typedef struct HDMI_OutputSize_t
{
    U32 XStart;
    U32 XStop;
    U32 YStart;
    U32 YStop;
}HDMI_OutputSize_t;

typedef struct HDMI_RevokedKSV_s
{
    U32 Number;       /* In HDCP topology, the maximum devices connected is 128 */
    U8 List[128*5];   /* In worst case, all KSV's devices are revoked*/
}HDMI_RevokedKSV_t;

typedef struct HDMI_Statistics_s
{
    U32 InterruptNewFrameCount;
    U32 InterruptHotPlugCount;
    U32 InterruptSoftResetCount;
    U32 InterruptInfoFrameCount;
    U32 InterruptPixelCaptureCount;
    U32 InterruptDllLockCount;
    U32 InfoFrameOverRunError;
    U32 SpdifFiFoOverRunCount;
    U32 InfoFrameCountGeneralControl;
    U32 InterruptGeneralControlCount;
    U32 NoMoreInterrupt;
    U32 InfoFrameCount[STVOUT_INFOFRAME_LAST];
}HDMI_Statistics_t;

typedef enum HDMI_InfoFrame_Type_e
{
    HDMI_VS_FRAME        = 1 << (STVOUT_INFOFRAME_TYPE_VS),
    HDMI_AVI_FRAME       = 1 << (STVOUT_INFOFRAME_TYPE_AVI),
    HDMI_AUDIO_FRAME     = 1 << (STVOUT_INFOFRAME_TYPE_AUDIO),
    HDMI_SPD_FRAME       = 1 << (STVOUT_INFOFRAME_TYPE_SPD),
    HDMI_MPEG_FRAME      = 1 << (STVOUT_INFOFRAME_TYPE_MPEG),
    HDMI_WA_ACR          = 1 << (STVOUT_INFOFRAME_TYPE_ACR),
    HDMI_GENERAL_CONTROL = 1 << (STVOUT_INFOFRAME_LAST)
} HDMI_InfoFrame_Type_t;

/* Adding a mask to send this infoframe at each frame */
#define HDMI_EACH_FRAME_INFOFRAME (HDMI_WA_ACR | HDMI_AVI_FRAME | HDMI_AUDIO_FRAME | HDMI_GENERAL_CONTROL)


typedef struct HDMI_Data_s
{
    ST_Partition_t *        CPUPartition_p;    /* Where the module can allocate memory for its internal usage */
    STEVT_Handle_t          EventsHandle;
    STEVT_EventID_t         RegisteredEventsID[HDMI_NB_REGISTERED_EVENTS_IDS];
    U8                      CommandsData[CONTROLLER_COMMANDS_BUFFER_SIZE];
    U32                     InterruptsData[INTERRUPTS_BUFFER_SIZE];
    CommandsBuffer32_t      InterruptsBuffer;  /* Data concerning the interrupts commands queue */
    CommandsBuffer_t        CommandsBuffer;    /* Data concerning the task controller commands queue */
    HAL_Handle_t            HalHdmiHandle;
    U32                     ValidityCheck;
    semaphore_t *           DataAccessSem_p; /* Used to make accesses to this structure thread-safe */

    /* For the interrupt controller*/
    HDMI_Interrupt_t        HdmiInterrupt;

    /* Internal state machine */
    HDMI_Task_t             Internaltask;
    BOOL                    InternalTaskStarted;
    HDMI_Task_t             InterruptTask;
    STVOUT_State_t          CurrentState;
    HDMI_State_t            InternalState;      /* Variable that summurize the current state of internal task.  */
    /* Adding semaphore communication between interrupt and process task */
    semaphore_t *           InterruptSem_p;
    semaphore_t*            ReceiveVsync_p;

    /* Set output params*/
    STVOUT_DeviceType_t     DeviceType;
    STVOUT_OutputParams_t   Out;
    STVOUT_OutputType_t     OutputType;
    STVOUT_HDCPParams_t     HDCPParams;
    HDMI_OutputSize_t       OutputSize;
    ST_DeviceName_t         VTGName;
    ST_DeviceName_t         I2CName;
    STI2C_Handle_t          I2CHandle;
    BOOL                    HSyncActivePolarity;
    BOOL                    VSyncActivePolarity;
    BOOL                    EnableCell;             /* In order to  enable the interface: enable the DVI or HDMI cell*/

    /* HDCP encryption fields*/
#if defined (STVOUT_HDCP_PROTECTION)
    U32                     WaitCounter;            /* A minimum rate for the link integrity verification :2 seconds*/
    U32                     RepeatCounter;          /* This verification was made twice when it was failed*/
    BOOL                    IsHDCPEnable;
    U32                     IRate;
    U8                      RepeaterKSVFifo[64*11]; /* in blocks of 512 bit */
    U8*                     ForbiddenKSVList_p;
    U8                      ForbiddenKSVNumber;
    STVOUT_HDCPSinkParams_t HDCPSinkParams;
    U16                     RepeaterBStatus;
    U32                     HashResult[5];
    BOOL                    Second_Part_Authentication;
    U32                     Time_Stop_Sec_Part;
    HDMI_RevokedKSV_t       RevokedKSV;
    U8                      BKSV[5];
#endif

    BOOL                    IsReceiverConnected;
    BOOL                    PixelCaptured;/*Kept : well initialized, might be used in the future*/
    BOOL                    IsKeysTransfered;
    BOOL                    IsI2COpened;
    BOOL                    AVMute;                 /* General control packet for MUTE */
    /* programming info frame */
    BOOL                    IsInfoFramePending;
    HDMI_InfoFrame_Type_t   InfoFrameSettings;      /* InfoFrame that will be transmitted each new frame */
    HDMI_InfoFrame_Type_t   InfoFrameStatus;        /* InfoFrame that still need to be send within the new frame */
    U32                     InfoFrameFuse;        /* fuse to reset the info frame state machine */
    U32                     InfoFrameBuffer[STVOUT_INFOFRAME_LAST][8];
    U32                     NValue;
    HDMI_Statistics_t       Statistics;
    semaphore_t*            ChangeState_p;  /* for State machine transition */

    #if defined (WA_GNBvd44290) || defined (WA_GNBvd54182)

    BOOL                    GenerateACR;
    BOOL                    IsFirstTime;
    U32                     CTSOffset;
    U32                     SleepTime;
    U32                     LastInterrupt;
    #endif /* WA_GNBvd44290 || defined (WA_GNBvd54182)*/
    #if defined(WA_GNBvd54182)|| defined (WA_GNBvd56512) || defined (STVOUT_CEC_PIO_COMPARE)
    STPWM_Handle_t          PwmHandle;
    BOOL                    IsPWMInitialized;
    #endif
    U32                     CTSValue;
    BOOL                    WaitingForInfoFrameInt;  /* A new infoframe interrupt was sent will occur soon */
    U8*                     SinkBuffer_p;
    U32                     AudioFrequency;
	HAL_PixelClockFreq_t    PixelClock;
    HAL_VsyncFreq_t         VSyncFreq;
#ifdef STVOUT_CEC_PIO_COMPARE
    HDMI_CECStruct_t        CEC_Params;
    HDMI_Task_t             CEC_Task;
    BOOL                    CEC_TaskStarted;
    semaphore_t*            CEC_Sem_p;
    ST_DeviceName_t         PWMName;
    ST_DeviceName_t         CECPIOName;
    U8                      CECPIO_BitNumber;
#endif
 }HDMI_Data_t;

/* Private Macros --------------------------------------------------------- */

/* Exported Macros--------------------------------------------------------- */

/* Passing (HDMI_Handle_t), returns TRUE if the handle is valid, FALSE otherwise  */
#define IsValidHdmiHandle(Handle)     (((HDMI_Data_t*)Handle)->ValidityCheck== HDMI_VALID_HANDLE)

/* Exported Functions ----------------------------------------------------- */
ST_ErrorCode_t VOUT_HDMI_Init           (const STVOUT_InitParams_t *   const InitParams_p,
                                         HDMI_Handle_t *               const HdmiHandle_p);
ST_ErrorCode_t HDMI_Term                (const HDMI_Handle_t           Handle);
ST_ErrorCode_t HDMI_Open                (const HDMI_Handle_t           Handle);
ST_ErrorCode_t HDMI_Close               (const HDMI_Handle_t           Handle);
ST_ErrorCode_t HDMI_GetSinkInformation  (HDMI_Handle_t                 const HdmiHandle,
                                         STVOUT_TargetInformation_t *  const TargetInfo_p);
ST_ErrorCode_t HDMI_GetState            (STVOUT_State_t *              const  State_p,
                                         HDMI_Handle_t                 const HdmiHandle);

ST_ErrorCode_t HDMI_SendData            (HDMI_Handle_t                 const HdmiHandle,
                                         const STVOUT_InfoFrameType_t  InfoFrameType,
                                         U8*                           Buffer_p,
                                         U32                           Size);
ST_ErrorCode_t HDMI_Start               (HDMI_Handle_t                 const Handle);
ST_ErrorCode_t HDMI_Enable              (HDMI_Handle_t                 const Handle);
ST_ErrorCode_t HDMI_Disable             (HDMI_Handle_t                 const Handle);

ST_ErrorCode_t HDMI_SetParams           (HDMI_Handle_t const Handle, const STVOUT_OutputParams_t OutputParams);
ST_ErrorCode_t HDMI_GetParams           (HDMI_Handle_t const Handle, STVOUT_OutputParams_t * const OutputParams_p);

ST_ErrorCode_t HDMI_GetStatistics       (HDMI_Handle_t const HdmiHandle,
                                         STVOUT_Statistics_t * const  Statistics_p
                                         );

ST_ErrorCode_t HDMI_SetOutputType        (HDMI_Handle_t const Handle,
                                         STVOUT_OutputType_t OutputType);

#if defined (STVOUT_HDCP_PROTECTION)
ST_ErrorCode_t HDMI_SetHDCPParams        (HDMI_Handle_t const HdmiHandle,
                                          const STVOUT_HDCPParams_t* const HDCPParams
                                        );

ST_ErrorCode_t HDMI_UpdateForbiddenKSVs  (HDMI_Handle_t const HdmiHandle,
                                          U8* const KSVList_p,
                                          U32  const KSVNumber
                                         );

ST_ErrorCode_t HDMI_EnableDefaultOutput   (HDMI_Handle_t const HdmiHandle,
                                           const STVOUT_DefaultOutput_t*        const DefaultOutput_p
                                           );
ST_ErrorCode_t HDMI_DisableDefaultOutput  (HDMI_Handle_t const HdmiHandle);

#ifdef WA_GNBvd56512
ST_ErrorCode_t HDMI_WA_GNBvd56512_Install(const HDMI_Handle_t Handle);
ST_ErrorCode_t HDMI_WA_GNBvd56512_UnInstall(const HDMI_Handle_t Handle);
#endif


#endif

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif
#endif /* #ifndef __API_H */

