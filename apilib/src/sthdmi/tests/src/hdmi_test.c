
/************************************************************************

File name   : Hdmi_test.c
Description : HDMI macros
Note        : All functions return TRUE if error for CLILIB compatibility

COPYRIGHT (C) STMicroelectronics 2005.

Date          Modification                                    Initials
----          ------------                                    --------
28 Feb 2005   Created                                         AC
25 Jul 2006   Add the HDMI test functionalities               WA
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stddefs.h"
#include "stdevice.h"
#ifdef ST_OSLINUX
	#define STTBX_Print(x) printf x
	#include "compat.h"
#else
	#include "sttbx.h"
#endif
#include "clavmem.h"
#include "stpio.h"
#include "stsys.h"
#include "testtool.h"
#include "stdenc.h"
#include "startup.h"
#include "api_cmd.h"
#include "clevt.h"
#include "vtg_cmd.h"
#include "denc_cmd.h"
#include "vout_cmd.h"
#include "vid_cmd.h"
#include "aud_cmd.h"
#include "vmix_cmd.h"
#include "sthdmi.h"
#include "stvmix.h"
#include "stlayer.h"
#include "stvout.h"
                       /*============================================================================*/
                       /*======================for the HDMI_Switch_Format============================*/
#include "stvtg.h"     /*============================================================================*/

#ifdef __FULL_DEBUG__
#include "../../src/hdmi_drv.h"
#endif

#if defined(HDMI_HDCP_ON)
#include "hdcp_cmd.h"
#endif
/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#define HDMI_NO_MEMORY_TEST_MAX_INIT 3  /* STHDMI_MAX_DEVICE + 1 */
#define VTG_INTERRUPT_LEVEL         7
#define HDMI_INTERRUPT_LEVEL        8
#define RAM1_BASE_ADDRESS                      0xA4000000 /* LMI_SYS */ /*======for the HDMI_Switch_Format=======*/

#define VOUT_MAX_KSV_SIZE       5*4
/*static U8 ForbiddenKSV[VOUT_MAX_KSV_SIZE] ={0xd5,0xda,0xd5,0xd5,0x34,
                                            0xe6,0xe6,0xe6,0xe6,0xc5,
                                            0x14,0x14,0x14,0x14,0xe4,
                                            0xa9,0xa9,0xa9,0xa9,0x74};*/
                                            /* Key1:Sharp; Key2:Sony; Key3:Hitachi; Key4:Sagem change VOUT_MAX_KSV_SIZE*/
#define FORBIDDEN_LIST_NUMBER   4


#define HDMI_CEC_NOTIFY_CLEAR_ALL            0x00
#define HDMI_CEC_NOTIFY_COMMAND              0x01
#define HDMI_CEC_NOTIFY_HPD                  0x02
#define HDMI_CEC_NOTIFY_EDID                 0x04
#define HDMI_CEC_NOTIFY_LOGIC_ADDRESS        0x08

#ifndef HDMI_TASK_CEC_PRIORITY
#if defined (ST_OS20)
#define HDMI_TASK_CEC_PRIORITY   7
#endif
#if defined (ST_OS21)
#define HDMI_TASK_CEC_PRIORITY   125
#endif
#if defined (ST_OSLINUX)
#define HDMI_TASK_CEC_PRIORITY   12
#endif
#endif

#define CEC_Debug_Print(X) STTBX_Print X
/* Private Variables (static)------------------------------------------------ */

static char HDMI_Msg[255];            /* text for trace */

#ifdef STHDMI_CEC
typedef struct HDMI_Task_s {

    task_t*                         Task_p;
    void*                           TaskStack;
    tdesc_t                         TaskDesc;
    BOOL    IsRunning;        /* TRUE if task is running, FALSE otherwise */
    BOOL    ToBeDeleted;      /* Set TRUE to ask task to end in order to delete it */
} HDMI_Task_t;

typedef struct HDMI_CECStruct_s
{
    U8                              LogicalAddress;
    HDMI_Task_t                     CEC_Task;
    semaphore_t*                    CEC_Sem_p;
    BOOL                            CEC_TaskStarted;
    U8                              Notify;
    STHDMI_CEC_CommandInfo_t        CurrentReceivedCommandInfo;
    STHDMI_CEC_PhysicalAddress_t    PhysicalAddress;
    STVOUT_CECDevice_t              CECDevice;
    BOOL                            CanSendCommand;         /*FALSE:  TRUE (after L@ allocation), FALSE (on UnPlug) */
    BOOL                            ActiveSource;           /*FALSE:  TRUE (on OneTouchPlay), FALSE (on user SwitchOff, on Standby request, Other Device ActiveSource) */
    BOOL                            Standby;                /*TRUE:   TRUE (on Standby requested, user SwitchOff), FALSE (One Touch Play) */
    char                            Language[3];            /* The Preferrable Language requested */
}HDMI_CECStruct_t;
#endif

typedef struct HDMI_Context_s
{
 STHDMI_Handle_t         HDMI_Handle;
 STEVT_Handle_t          EVT_Handle;
 STVOUT_Handle_t         VOUT_Handle;
 STVTG_Handle_t          VTG_Handle;
 STDENC_Handle_t         DENC_Handle;
 ST_DeviceName_t         AUD_DeviceName;
 ST_DeviceName_t         VTG_DeviceName;
 STVOUT_State_t          VOUT_State;
 BOOL                    HDMI_AutomaticEnableDisable;
 BOOL                    HDMI_IsEnabled;
 STVOUT_OutputType_t     HDMI_OutputType;
 STHDMI_ScanInfo_t       HDMI_ScanInfo;
 STVOUT_ColorSpace_t     HDMI_Colorimetry;
 STGXOBJ_AspectRatio_t   HDMI_AspectRatio;
 STGXOBJ_AspectRatio_t   HDMI_ActiveAspectRatio;
 STHDMI_PictureScaling_t HDMI_PictureScaling;
  U32                     HDMI_AudioFrequency;
 U32                     HDMI_IV0_Key;
 U32                     HDMI_IV1_Key;
 U32                     HDMI_KV0_Key;
 U32                     HDMI_KV1_Key;
 U32                     HDMI_DeviceKeys[80];
#ifdef STHDMI_CEC
 HDMI_CECStruct_t        HDMI_CECStruct;
#endif
} HDMI_Context_t;

/*HDMI_Handle_t HANDLE;*/  /*====================== for HDMI_statistics=====================*/

/* Global Variables --------------------------------------------------------- */
 #define HDMI_MAX_NUMBER 1
 #define VMIX_MAX_NUMBER 1
 #define VTG_MAX_NUMBER  1
 ST_DeviceName_t I2C_DeviceName[]={"I2CBACK","I2CFRONT"};
 ST_DeviceName_t PIO_DeviceName[]={"PIO0","PIO1","PIO2","PIO3","PIO4","PIO5"};
 ST_DeviceName_t EVT_DeviceName[1];
 ST_DeviceName_t GFX_LAYER_DeviceName[1];
 ST_Partition_t *system_partition_stfae;
 ST_DeviceName_t HDMI_DeviceName[]={"HDMI0"};
 ST_DeviceName_t VTG_DeviceName[]={"VTG"};
 ST_DeviceName_t DENC_DeviceName[]={"DENC"};
 ST_DeviceName_t VOUT_DeviceName[]={"VOUT"};
 /*ST_DeviceName_t AUD_DeviceName[]={"AUD0"};*/
 ST_DeviceName_t CLKRV_DeviceName[]={"CLKRV"};
 /*ST_DeviceName_t EVT_DeviceName[]={"EVT"};*/
 ST_DeviceName_t   VMIX_OutDeviceNames[3];
 ST_DeviceName_t   VMIX_DeviceName[]={"VMIX"};
 ST_DeviceName_t   LAYER_DeviceName[]={"VID_LAYER0","VID_LAYER1"};
 STVMIX_LayerDisplayParams_t * VMIX_LayerArray[VMIX_MAX_NUMBER][5];
 STVMIX_LayerDisplayParams_t   VMIX_LayerParams[VMIX_MAX_NUMBER][5];
 STAVMEM_PartitionHandle_t AVMEM_PartitionHandle[1];
 STGXOBJ_Bitmap_t* GFX_GXOBJ_Bitmap[VTG_MAX_NUMBER];
 STLAYER_Handle_t LAYER_Handle[VMIX_MAX_NUMBER];
 STVMIX_Handle_t  VMIX_Handle[VMIX_MAX_NUMBER];
 STHDMI_Handle_t  HDMI_Handle[HDMI_MAX_NUMBER];
 STVTG_Handle_t   VTG_Handle[HDMI_MAX_NUMBER];
 STDENC_Handle_t  DENC_Handle[HDMI_MAX_NUMBER];
 STVOUT_Handle_t  VOUT_Handle[HDMI_MAX_NUMBER];
 HDMI_Context_t   HDMI_Context[HDMI_MAX_NUMBER];
 STLAYER_Handle_t GFX_LAYER_Handle[VTG_MAX_NUMBER];
 STLAYER_ViewPortHandle_t GFX_LAYER_ViewPortHandle[VTG_MAX_NUMBER];

 /* Private Constants -------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
#if 0
static void HDMI_Display_Formats(void);
#endif
static BOOL HDMI_TestLimitInit( STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL HDMI_TestInfoFrame(STTST_Parse_t *pars_p, char *result_sym_p);
static BOOL HDMI_SwitchFormat(STTST_Parse_t *pars_p, char *result_sym_p );

BOOL Test_CmdStart(void);
void os20_main(void *ptr);
/*static BOOL HDMI_DisplaySinkEDID(STTST_Parse_t *pars_p, char *result_sym_p );*/
/*static void HDMI_DisplayFormats(void);*/
/*static void HDMI_GetStatistics(STHDMI_Handle_t HDMI_Handle );*/
ST_ErrorCode_t VTG_SetMode(U32 VTG_Index,STVTG_TimingMode_t VTG_NewMode);
ST_ErrorCode_t HDMI_IsScreenDVIOnly(U32 DeviceId,BOOL *DVIOnlyNotHDMI);
ST_ErrorCode_t HDMI_EnableOutput(U32 DeviceId);
ST_ErrorCode_t HDMI_DisableOutput(U32 DeviceId);
static void HDMIi_HotplugCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p);
#ifdef STHDMI_CEC
static void HDMIi_CEC_LogicalAddress_CallBack(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void HDMIi_CEC_Command_CallBack(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void HDMI_CEC_TaskFunction (void);
BOOL HDMI_HandleCECCommand  (STHDMI_CEC_Command_t*   const CEC_Command_p);
static BOOL HDMI_OneTouchPlay(STTST_Parse_t *pars_p, char *result_sym_p);
static BOOL HDMI_UserSwitchOff(STTST_Parse_t *pars_p, char *result_sym_p);
static BOOL HDMI_Standby(STTST_Parse_t *pars_p, char *result_sym_p);
#endif
static void HDMIi_AudioCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p);
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


/* Extern functions prototypes-----------------------------------------------*/
BOOL VID_RegisterCmd2 (void);
BOOL VID_Inj_RegisterCmd (void);
#if defined (ST_OSLINUX)
extern void VIDTEST_Init(void);
#endif

/* Functions ---------------------------------------------------------------- */


/*-------------------------------------------------------------------------
 * Function : HDMI_Display_Formats
  * Description : this is a table of correspondance between integers and different video formats:
 *  (C code program because macro are not convenient for this test)
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
#if 0
static void HDMI_Display_Formats(void)
{
 printf("enter the appropriate integer to choose the corresponding video format\n\n");
 printf("1=======================================>STVTG_TIMING_MODE_720P59940_74176");
 printf("2=======================================>STVTG_TIMING_MODE_720P60000_74250");
 printf("3=======================================>STVTG_TIMING_MODE_720P50000_74250");
 printf("4=======================================>STVTG_TIMING_MODE_576P50000_27000");
 printf("5=======================================>STVTG_TIMING_MODE_576I50000_13500");
 printf("6=======================================>STVTG_TIMING_MODE_480P60000_27027");
 printf("7=======================================>STVTG_TIMING_MODE_480P59940_27000");
 printf("8=======================================>STVTG_TIMING_MODE_480I59940_13500");
 printf("9=======================================>STVTG_TIMING_MODE_480I60000_13514");
 printf("10=======================================>STVTG_TIMING_MODE_1080I50000_74250_1");
 printf("11======================================>STVTG_TIMING_MODE_1080I60000_74250");
 printf("12======================================>STVTG_TIMING_MODE_1080I59940_74176");
 printf("13======================================>STVTG_TIMING_MODE_480P59940_25175");
 printf("14======================================>STVTG_TIMING_MODE_480P60000_25200");

}/* end of HDMI_isplay_Formats*/
#endif
/* ========================================================================
   Name:        VTG_SetMode
   Description: Set mode of the vtg on the fly
   ======================================================================== */

ST_ErrorCode_t VTG_SetMode(U32 VTG_Index,STVTG_TimingMode_t VTG_NewMode)
{
#if !defined(ST_7100)&&!defined(ST_7109)
 return(ST_NO_ERROR);
#else
 ST_ErrorCode_t                  ErrCode=ST_NO_ERROR;
 /*STVMIX_TermParams_t             VMIX_TermParams;*/
 STVMIX_ScreenParams_t           VMIX_ScreenParams;
 STLAYER_TermParams_t            LAYER_TermParams;
 STLAYER_InitParams_t            LAYER_InitParams;
 STLAYER_OpenParams_t            LAYER_OpenParams;
 STLAYER_LayerParams_t           LAYER_LayerParams;
 STLAYER_ViewPortParams_t        LAYER_ViewPortParams;
 STLAYER_ViewPortSource_t        LAYER_ViewPortSource;
 STLAYER_GlobalAlpha_t           LAYER_GlobalAlpha;
 STVTG_InitParams_t              VTG_InitParams;
 STVTG_OpenParams_t              VTG_OpenParams;
 STVTG_TimingMode_t              VTG_TimingMode;
 STVTG_ModeParams_t              VTG_ModeParams;
 STVTG_OptionalConfiguration_t   VTG_OptionalConfiguration;
 /*STGXOBJ_BitmapAllocParams_t     GXOBJ_BitmapAllocParams1;*/
 /*STGXOBJ_BitmapAllocParams_t     GXOBJ_BitmapAllocParams2;*/
 STVOUT_InitParams_t             VOUT_InitParams;
 STVOUT_OpenParams_t             VOUT_OpenParams;
 STVOUT_OutputParams_t           VOUT_OutputParams;
 STVOUT_TermParams_t             VOUT_TermParams;
 STVTG_TermParams_t              VTG_TermParams;
 STHDMI_InitParams_t             HDMI_InitParams;
 STHDMI_OpenParams_t             HDMI_OpenParams;
 STHDMI_TermParams_t             HDMI_TermParams;
 STEVT_DeviceSubscribeParams_t   EVT_SubcribeParams;
 STEVT_OpenParams_t              EVT_OpenParams;
 /*STAVMEM_AllocBlockParams_t      AVMEM_AllocBlockParams;*/
 U32                             NbLayers,GFX_Index,VOUT_Index,HDMI_Index,LAYER_Index,VMIX_Index;

 /* Check VTG index provided */
 /* ======================== */
 if (VTG_Index>VTG_MAX_NUMBER)
  {
   return(ST_ERROR_BAD_PARAMETER);
  }

 /* For VTG 0 */
 /* ========= */
 if (VTG_Index==0)
  {
   VMIX_Index  = 0;
   GFX_Index   = 0;
   LAYER_Index = 0;
   VOUT_Index  = 2;
   HDMI_Index  = 0;
  }

 /* For VTG 1 */
 /* ========= */
 if (VTG_Index==1)
  {
   VMIX_Index  = 1;
   LAYER_Index = 1;
   GFX_Index   = 1;
   VOUT_Index  = 0xFFFFFFFF;
   HDMI_Index  = 0xFFFFFFFF;
  }

 /* Close the mixer */
 /* =============== */
 if (VMIX_Index!=0xFFFFFFFF)
  {
   ErrCode=STVMIX_Disable(VMIX_Handle[VMIX_Index]);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ErrCode);
    }
   ErrCode=STVMIX_DisconnectLayers(VMIX_Handle[VMIX_Index]);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ErrCode);
    }
  }

 /* Close the graphic view port */
 /* =========================== */
 if (GFX_Index!=0xFFFFFFFF)
  {
   ErrCode=STLAYER_CloseViewPort(GFX_LAYER_ViewPortHandle[GFX_Index]);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ErrCode);
    }
   ErrCode=STLAYER_Close(GFX_LAYER_Handle[GFX_Index]);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ErrCode);
    }
   LAYER_TermParams.ForceTerminate=TRUE;
   ErrCode=STLAYER_Term(GFX_LAYER_DeviceName[GFX_Index],&LAYER_TermParams);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ErrCode);
    }
  }

 /* Close the HDMI interface */
 /* ======================== */
 if (HDMI_Index!=0xFFFFFFFF)
  {
    ErrCode=STEVT_UnsubscribeDeviceEvent(HDMI_Context[HDMI_Index].EVT_Handle,HDMI_Context[HDMI_Index].VTG_DeviceName,STVOUT_CHANGE_STATE_EVT);
    if (ErrCode!=ST_NO_ERROR)
    {
         return(ErrCode);
    }
    ErrCode=STHDMI_Close(HDMI_Context[HDMI_Index].HDMI_Handle);
    if (ErrCode!=ST_NO_ERROR)
    {
        return(ErrCode);
    }
    HDMI_TermParams.ForceTerminate=TRUE;
    ErrCode=STHDMI_Term(HDMI_DeviceName[HDMI_Index],&HDMI_TermParams);
    if (ErrCode!=ST_NO_ERROR)
    {
        return(ErrCode);
    }
  }

 /* Close the VOUT interface */
 /* ======================== */
 if (VOUT_Index!=0xFFFFFFFF)
  {
    ErrCode=STVOUT_Disable(VOUT_Handle[VOUT_Index]);
    if ((ErrCode!=ST_NO_ERROR)&&(ErrCode!=STVOUT_ERROR_VOUT_NOT_ENABLE))
    {
         return(ErrCode);
    }
    ErrCode=STVOUT_Close(VOUT_Handle[VOUT_Index]);
    if (ErrCode!=ST_NO_ERROR)
    {
         return(ErrCode);
    }
    VOUT_TermParams.ForceTerminate=TRUE;
    ErrCode=STVOUT_Term(VOUT_DeviceName[VOUT_Index],&VOUT_TermParams);
    if (ErrCode!=ST_NO_ERROR)
    {
         return(ErrCode);
    }
  }
   /* Close the VTG interface */
 /* ======================== */
 if (VTG_Index!=0xFFFFFFFF)
  {
    ErrCode=STVTG_Close(VOUT_Handle[VOUT_Index]);
    if (ErrCode!=ST_NO_ERROR)
    {
         return(ErrCode);
    }
    VTG_TermParams.ForceTerminate=TRUE;
    ErrCode=STVTG_Term(VTG_DeviceName[VTG_Index],&VTG_TermParams);
    if (ErrCode!=ST_NO_ERROR)
    {
         return(ErrCode);
    }
  }
  /* Initialize the VTG 1 */
  /* ==================== */
 memset(&VTG_InitParams,0,sizeof(STVTG_InitParams_t));
 memset(&VTG_OpenParams,0,sizeof(STVTG_OpenParams_t));
 VTG_InitParams.DeviceType                          = STVTG_DEVICE_TYPE_VTG_CELL_7100;
 VTG_InitParams.MaxOpen                             = 3;
/* VTG_InitParams.Target.VtgCell3.BaseAddress_p       = (void *)ST7100_VTG1_BASE_ADDRESS;
 VTG_InitParams.Target.VtgCell3.DeviceBaseAddress_p = (void *)0x00000000;
 VTG_InitParams.Target.VtgCell3.InterruptNumber     = ST7100_VTG_0_INTERRUPT;*/
 VTG_InitParams.Target.VtgCell3.InterruptLevel      = VTG_INTERRUPT_LEVEL;
 strcpy(VTG_InitParams.Target.VtgCell3.ClkrvName,CLKRV_DeviceName[0]);
 strcpy(VTG_InitParams.EvtHandlerName,"EVT");
 ErrCode=STVTG_Init(VTG_DeviceName[0], &VTG_InitParams);
 if (ErrCode!=ST_NO_ERROR)
 {
   	return(ErrCode);
 }
 ErrCode=STVTG_Open(VTG_DeviceName[0],&VTG_OpenParams,&VTG_Handle[0]);

 if (ErrCode!=ST_NO_ERROR)
 {
   	return(ErrCode);
 }
 /* Set the timing screen params */
 /* ============================ */
 if (VTG_Index!=0xFFFFFFFF)
  {
   ErrCode=STVTG_SetMode(VTG_Handle[VTG_Index],VTG_NewMode);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ErrCode);
    }
   VTG_OptionalConfiguration.Option                = STVTG_OPTION_EMBEDDED_SYNCHRO;
   VTG_OptionalConfiguration.Value.EmbeddedSynchro = TRUE;
   ErrCode=STVTG_SetOptionalConfiguration(VTG_Handle[VTG_Index],&VTG_OptionalConfiguration);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ErrCode);
    }
  }
 /* Initialize VOUT output */
 /* ====================== */
 if ((VOUT_Index!=0xFFFFFFFF)&&(HDMI_Index!=0xFFFFFFFF))
  {
   memset(&VOUT_InitParams,0,sizeof(STVOUT_InitParams_t));
   memset(&VOUT_OpenParams,0,sizeof(STVOUT_OpenParams_t));
   memset(&VOUT_OutputParams,0,sizeof(STVOUT_OutputParams_t));
   VOUT_InitParams.DeviceType                                                = STVOUT_DEVICE_TYPE_7100;
   VOUT_InitParams.OutputType                                                = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
   VOUT_InitParams.CPUPartition_p                                            = system_partition_stfae;
   VOUT_InitParams.MaxOpen                                                   = 3;
#if defined(ST_7100)
   VOUT_InitParams.Target.HDMICell.InterruptNumber                           = ST7100_HDMI_INTERRUPT;
#endif
#if defined(ST_7109)
   VOUT_InitParams.Target.HDMICell.InterruptNumber                           = ST7109_HDMI_INTERRUPT;
#endif
   VOUT_InitParams.Target.HDMICell.InterruptLevel                            = HDMI_INTERRUPT_LEVEL;
/*#if defined(DISPLAY_HD)*/
   VOUT_InitParams.Target.HDMICell.HSyncActivePolarity                       = FALSE;
   VOUT_InitParams.Target.HDMICell.VSyncActivePolarity                       = FALSE;
/*#else
   VOUT_InitParams.Target.HDMICell.HSyncActivePolarity                       = TRUE;
   VOUT_InitParams.Target.HDMICell.VSyncActivePolarity                       = TRUE;
#endif*/
#if defined(HDMI_HDCP_ON)
   VOUT_InitParams.Target.HDMICell.IsHDCPEnable                              = TRUE;
#else
   VOUT_InitParams.Target.HDMICell.IsHDCPEnable                              = FALSE;
#endif
#if defined(ST_7100)
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p       = (void *)ST7100_HDMI_BASE_ADDRESS;
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p = (void *)ST7100_HDCP_BASE_ADDRESS;
#endif
#if defined(ST_7109)
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p       = (void *)ST7109_HDMI_BASE_ADDRESS;
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p = (void *)ST7109_HDCP_BASE_ADDRESS;
#endif
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p = NULL;
#if defined(cocoref)||defined(cocoref_v2)||defined(mb411)
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.HPD_Bit             = PIO_BIT_2;
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed       = FALSE;
   strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.I2CDevice,I2C_DeviceName[0]);
   strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.PIODevice,PIO_DeviceName[2]);
#endif
#if defined(custom11)||defined(custom13)||defined(custom14)||defined(custom16)
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.HPD_Bit             = PIO_BIT_6;
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed       = FALSE;
   strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.I2CDevice,I2C_DeviceName[0]);
   strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.PIODevice,PIO_DeviceName[4]);
#endif
#if defined(custom21)
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.HPD_Bit             = PIO_BIT_7;
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed       = TRUE;
   strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.I2CDevice,I2C_DeviceName[0]);
   strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.PIODevice,PIO_DeviceName[4]);
#endif

#ifdef STVOUT_INVERTED_HPD /* Because it might be inverted in any of previous boards */
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed       = TRUE;
#endif

   strcpy(VOUT_InitParams.Target.HDMICell.VTGName,VTG_DeviceName[VTG_Index]);
   strcpy(VOUT_InitParams.Target.HDMICell.EventsHandlerName,EVT_DeviceName[0]);
  ErrCode=STVOUT_Init(VOUT_DeviceName[VOUT_Index],&VOUT_InitParams);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ErrCode);
    }
   ErrCode=STVOUT_Open(VOUT_DeviceName[VOUT_Index],&VOUT_OpenParams,&VOUT_Handle[VOUT_Index]);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ErrCode);
    }
   VOUT_OutputParams.HDMI.AudioFrequency = 48000;
#if defined(HDMI_FORCE_DVI)
   VOUT_OutputParams.HDMI.ForceDVI       = TRUE;
#else
   VOUT_OutputParams.HDMI.ForceDVI       = FALSE;
#endif
#if defined(HDMI_HDCP_ON)
   VOUT_OutputParams.HDMI.IsHDCPEnable   = TRUE;
#else
   VOUT_OutputParams.HDMI.IsHDCPEnable   = FALSE;
#endif
   ErrCode=STVOUT_SetOutputParams(VOUT_Handle[VOUT_Index],&VOUT_OutputParams);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ErrCode);
    }
  }
 /* Initialize HDMI output */
 /* ====================== */
 if (HDMI_Index!=0xFFFFFFFF)
  {
   memset(&HDMI_InitParams,0,sizeof(STHDMI_InitParams_t));
   memset(&HDMI_OpenParams,0,sizeof(STHDMI_OpenParams_t));
   memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
   memset(&EVT_OpenParams,0,sizeof(STEVT_OpenParams_t));
   HDMI_InitParams.DeviceType     = STHDMI_DEVICE_TYPE_7100;
   HDMI_InitParams.OutputType     = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
   HDMI_InitParams.MaxOpen        = 1;
   HDMI_InitParams.CPUPartition_p = system_partition_stfae;
   HDMI_InitParams.AVIType        = STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO;
   HDMI_InitParams.SPDType        = STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE;
   HDMI_InitParams.MSType         = STHDMI_MS_INFOFRAME_FORMAT_VER_ONE;
   HDMI_InitParams.AUDIOType      = STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE;
   HDMI_InitParams.VSType         = STHDMI_VS_INFOFRAME_FORMAT_VER_ONE;
   strcpy(HDMI_InitParams.Target.OnChipHdmiCell.VTGDeviceName  , VTG_DeviceName[VTG_Index]);
   strcpy(HDMI_InitParams.Target.OnChipHdmiCell.VOUTDeviceName , VOUT_DeviceName[VOUT_Index]);
   strcpy(HDMI_InitParams.Target.OnChipHdmiCell.EvtDeviceName  , EVT_DeviceName[0]);
   ErrCode=STHDMI_Init(HDMI_DeviceName[HDMI_Index],&HDMI_InitParams);
   if (ErrCode!=ST_NO_ERROR)
   {
     return(ErrCode);
   }
   ErrCode=STHDMI_Open(HDMI_DeviceName[HDMI_Index],&HDMI_OpenParams,&HDMI_Handle[HDMI_Index]);
   if (ErrCode!=ST_NO_ERROR)
   {
     return(ErrCode);
   }
   HDMI_Context[HDMI_Index].HDMI_Handle                 = HDMI_Handle[HDMI_Index];
   HDMI_Context[HDMI_Index].VTG_Handle                  = VTG_Handle[VTG_Index];
   HDMI_Context[HDMI_Index].VOUT_Handle                 = VOUT_Handle[VOUT_Index];
   HDMI_Context[HDMI_Index].VOUT_State                  = STVOUT_NO_RECEIVER;
   HDMI_Context[HDMI_Index].HDMI_AutomaticEnableDisable = TRUE;
   HDMI_Context[HDMI_Index].HDMI_IsEnabled              = FALSE;
   HDMI_Context[HDMI_Index].HDMI_OutputType             = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
   HDMI_Context[HDMI_Index].HDMI_ScanInfo               = STHDMI_SCAN_INFO_OVERSCANNED;
   HDMI_Context[HDMI_Index].HDMI_Colorimetry            = STVOUT_ITU_R_601;
   HDMI_Context[HDMI_Index].HDMI_AspectRatio            = STGXOBJ_ASPECT_RATIO_16TO9;
   HDMI_Context[HDMI_Index].HDMI_ActiveAspectRatio      = STGXOBJ_ASPECT_RATIO_16TO9;
   HDMI_Context[HDMI_Index].HDMI_PictureScaling         = STHDMI_PICTURE_NON_UNIFORM_SCALING;
   HDMI_Context[HDMI_Index].HDMI_AudioFrequency         = 48000;
   strcpy(HDMI_Context[HDMI_Index].VTG_DeviceName,VTG_DeviceName[VTG_Index]);
   EVT_SubcribeParams.NotifyCallback=HDMIi_HotplugCallback;
   ErrCode|=STEVT_Open(EVT_DeviceName[0],&EVT_OpenParams,&HDMI_Context[HDMI_Index].EVT_Handle);
   ErrCode|=STEVT_SubscribeDeviceEvent(HDMI_Context[HDMI_Index].EVT_Handle,VTG_DeviceName[VTG_Index],STVOUT_CHANGE_STATE_EVT,&EVT_SubcribeParams);
   if (ErrCode!=ST_NO_ERROR)
   {
      return(ErrCode);
   }
   ErrCode=STVOUT_Start(HDMI_Context[HDMI_Index].VOUT_Handle);
   if (ErrCode!=ST_NO_ERROR)
   {
      return(ErrCode);
   }
   ErrCode=STVOUT_Disable(HDMI_Context[HDMI_Index].VOUT_Handle);
   if (ErrCode!=ST_NO_ERROR)
   {
      return(ErrCode);
   }

   ErrCode=STVOUT_Enable(HDMI_Context[HDMI_Index].VOUT_Handle);
   if (ErrCode!=ST_NO_ERROR)
   {
      return(ErrCode);
   }
  }

 /* Create the graphic layer */
 /* ======================== */

 /* Get the screen params */
 /* --------------------- */
 ErrCode=STVTG_GetMode(VTG_Handle[VTG_Index],&VTG_TimingMode,&VTG_ModeParams);
 if (ErrCode!=ST_NO_ERROR)
 {
      return(ErrCode);
 }

 /* Set the initialization structure */
 /* -------------------------------- */
 memset(&LAYER_InitParams ,0,sizeof(STLAYER_InitParams_t));
 memset(&LAYER_OpenParams ,0,sizeof(STLAYER_OpenParams_t));
 memset(&LAYER_LayerParams,0,sizeof(STLAYER_LayerParams_t));
 LAYER_LayerParams.AspectRatio                = STGXOBJ_ASPECT_RATIO_4TO3;
 LAYER_LayerParams.ScanType                   = VTG_ModeParams.ScanType;
 LAYER_LayerParams.Width                      = VTG_ModeParams.ActiveAreaWidth;
 LAYER_LayerParams.Height                     = VTG_ModeParams.ActiveAreaHeight;
 LAYER_InitParams.CPUPartition_p              = DriverPartition_p;
 LAYER_InitParams.CPUBigEndian                = FALSE;
 LAYER_InitParams.SharedMemoryBaseAddress_p   = (void *)RAM1_BASE_ADDRESS;
 LAYER_InitParams.MaxHandles                  = 2;
 LAYER_InitParams.AVMEM_Partition             = (STAVMEM_PartitionHandle_t)NULL; /*AVMEM_PartitionHandle[0];*/
 LAYER_InitParams.MaxViewPorts                = 2;
 LAYER_InitParams.ViewPortNodeBuffer_p        = NULL;
 LAYER_InitParams.NodeBufferUserAllocated     = FALSE;
 LAYER_InitParams.ViewPortBuffer_p            = NULL;
 LAYER_InitParams.ViewPortBufferUserAllocated = FALSE;
 LAYER_InitParams.LayerParams_p               = &LAYER_LayerParams;
 LAYER_InitParams.LayerType                   = STLAYER_GAMMA_GDP;
#if defined(ST_7100)
 LAYER_InitParams.BaseAddress_p               = (void *)ST7100_GDP1_LAYER_BASE_ADDRESS;
#endif
#if defined(ST_7109)
 LAYER_InitParams.BaseAddress_p               = (void *)ST7109_GDP1_LAYER_BASE_ADDRESS;
#endif
 LAYER_InitParams.BaseAddress2_p              = NULL;
 LAYER_InitParams.DeviceBaseAddress_p         = NULL;
 strcpy(LAYER_InitParams.EventHandlerName,EVT_DeviceName[0]);

 /* Init the layer */
 /* -------------- */
 ErrCode=STLAYER_Init(GFX_LAYER_DeviceName[GFX_Index],&LAYER_InitParams);
 if (ErrCode!=ST_NO_ERROR)
 {
     return(ErrCode);
 }
 ErrCode=STLAYER_Open(GFX_LAYER_DeviceName[GFX_Index],&LAYER_OpenParams,&GFX_LAYER_Handle[GFX_Index]);
 if (ErrCode!=ST_NO_ERROR)
 {
      return(ErrCode);
 }

 /* Set the screen params */
 /* --------------------- */
 ErrCode=STLAYER_SetLayerParams(GFX_LAYER_Handle[GFX_Index],&LAYER_LayerParams);
 if (ErrCode!=ST_NO_ERROR)
 {
      return(ErrCode);
 }
 /* Create a view port */
 /* ------------------ */
 memset(&LAYER_ViewPortParams,0,sizeof(STLAYER_ViewPortParams_t));
 memset(&LAYER_ViewPortSource,0,sizeof(STLAYER_ViewPortSource_t));
 /* Declare the viewport */
 LAYER_ViewPortParams.Source_p                  = &LAYER_ViewPortSource;
 LAYER_ViewPortParams.InputRectangle.PositionX  = 0;
 LAYER_ViewPortParams.InputRectangle.PositionY  = 0;
 LAYER_ViewPortParams.InputRectangle.Width      = VTG_ModeParams.ActiveAreaWidth;
 LAYER_ViewPortParams.InputRectangle.Height     = VTG_ModeParams.ActiveAreaHeight;
 LAYER_ViewPortParams.OutputRectangle.PositionX = 0;
 LAYER_ViewPortParams.OutputRectangle.PositionY = 0;
 LAYER_ViewPortParams.OutputRectangle.Width     = VTG_ModeParams.ActiveAreaWidth;
 LAYER_ViewPortParams.OutputRectangle.Height    = VTG_ModeParams.ActiveAreaHeight;
 LAYER_ViewPortSource.Data.BitMap_p             = (STGXOBJ_Bitmap_t *) &GFX_GXOBJ_Bitmap[GFX_Index];
 LAYER_ViewPortSource.Palette_p                 = NULL;
 LAYER_ViewPortSource.SourceType                = STLAYER_GRAPHIC_BITMAP;
 /* Open the view port */
 ErrCode=STLAYER_OpenViewPort(GFX_LAYER_Handle[GFX_Index],&LAYER_ViewPortParams,&GFX_LAYER_ViewPortHandle[GFX_Index]);
 if (ErrCode!=ST_NO_ERROR)
 {
      return(ErrCode);
 }
 /* Set global alpha */
 LAYER_GlobalAlpha.A0=0x00;
 LAYER_GlobalAlpha.A1=0x80;
 ErrCode=STLAYER_SetViewPortAlpha(GFX_LAYER_ViewPortHandle[GFX_Index],&LAYER_GlobalAlpha);
 if (ErrCode!=ST_NO_ERROR)
 {
     return(ErrCode);
 }
 /* Setup Mixer params */
 /* ================== */

 /* Set mixer screen parameters */
 /* --------------------------- */
 VMIX_ScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
 VMIX_ScreenParams.ScanType    = VTG_ModeParams.ScanType;
 VMIX_ScreenParams.FrameRate   = VTG_ModeParams.FrameRate;
 VMIX_ScreenParams.Width       = VTG_ModeParams.ActiveAreaWidth;
 VMIX_ScreenParams.Height      = VTG_ModeParams.ActiveAreaHeight;
 VMIX_ScreenParams.XStart      = VTG_ModeParams.ActiveAreaXStart;
 VMIX_ScreenParams.YStart      = VTG_ModeParams.ActiveAreaYStart;
 ErrCode=STVMIX_SetScreenParams(VMIX_Handle[VMIX_Index],&VMIX_ScreenParams);
 if (ErrCode!=ST_NO_ERROR)
 {
      return(ErrCode);
 }
 /* Connect the layer device name to the mixer */
 /* ------------------------------------------ */
 NbLayers=0;
 strcpy(VMIX_LayerParams[VMIX_Index][NbLayers].DeviceName,LAYER_DeviceName[LAYER_Index]);
 VMIX_LayerParams[VMIX_Index][NbLayers].ActiveSignal=TRUE;
 VMIX_LayerArray[VMIX_Index][NbLayers]=&VMIX_LayerParams[VMIX_Index][NbLayers];
 NbLayers++;
 if (GFX_LAYER_Handle[GFX_Index]!=0)
 {
   strcpy(VMIX_LayerParams[VMIX_Index][NbLayers].DeviceName,GFX_LAYER_DeviceName[GFX_Index]);
   VMIX_LayerParams[VMIX_Index][NbLayers].ActiveSignal=TRUE;
   VMIX_LayerArray[VMIX_Index][NbLayers]=&VMIX_LayerParams[VMIX_Index][NbLayers];
   NbLayers++;
 }
 ErrCode=STVMIX_ConnectLayers(VMIX_Handle[VMIX_Index],(const STVMIX_LayerDisplayParams_t *const *)&VMIX_LayerArray[VMIX_Index][0],NbLayers);
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }
 ErrCode=STVMIX_Enable(VMIX_Handle[VMIX_Index]);
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }


 /* Return no errors */
 /* ================ */
 return(ST_NO_ERROR);


#endif

}

/*-------------------------------------------------------------------------
 * Function : HDMI_SwitchFormat
  * Description : switches to the assigned video format :
 *  (C code program because macro are not convenient for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_SwitchFormat(STTST_Parse_t *pars_p, char *result_sym_p )
{   BOOL RetErr;
    int LVar;

    UNUSED_PARAMETER(result_sym_p);

    /* get video format (default: 1=STVTG_TIMING_MODE_720P59940_74176) */
    RetErr = STTST_GetInteger( pars_p, 1, &LVar );
    if ( RetErr || (LVar < 1 || LVar > 14) )
    {
        STTST_TagCurrentLine( pars_p, "Expected integer between 1 and 14");
        RetErr = TRUE;
    }
    else       /*RetErr == FALSE  */
    memset(VTG_Handle,0,sizeof(VTG_Handle));
    switch(LVar)
       {
        case 1  : VTG_SetMode(0,STVTG_TIMING_MODE_720P59940_74176);
        case 2  : VTG_SetMode(0,STVTG_TIMING_MODE_720P60000_74250);
        case 3  : VTG_SetMode(0,STVTG_TIMING_MODE_720P50000_74250);
        case 4  : VTG_SetMode(0,STVTG_TIMING_MODE_576P50000_27000);
        case 5  : VTG_SetMode(0,STVTG_TIMING_MODE_576I50000_13500);
        case 6  : VTG_SetMode(0,STVTG_TIMING_MODE_480P60000_27027);
        case 7  : VTG_SetMode(0,STVTG_TIMING_MODE_480P59940_27000);
        case 8  : VTG_SetMode(0,STVTG_TIMING_MODE_480I59940_13500);
        case 9  : VTG_SetMode(0,STVTG_TIMING_MODE_480I60000_13514);
        case 10 : VTG_SetMode(0,STVTG_TIMING_MODE_1080I50000_74250_1);
        case 11 : VTG_SetMode(0,STVTG_TIMING_MODE_1080I60000_74250);
        case 12 : VTG_SetMode(0,STVTG_TIMING_MODE_1080I59940_74176);
        case 13 : VTG_SetMode(0,STVTG_TIMING_MODE_480P59940_25175);
        case 14 : VTG_SetMode(0,STVTG_TIMING_MODE_480P60000_25200);
      }
      return(RetErr);
}
/*-------------------------------------------------------------------------
 * Function : HDMI_TestLimitInit
 * Description : test limit ST_ERROR_NO_MEMORY :
 *  (C code program because macro are not convenient for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_TestLimitInit(STTST_Parse_t *pars_p, char *result_sym_p)
{
    STHDMI_InitParams_t HDMIInitParams;
    STHDMI_TermParams_t HDMITermParams;
    BOOL RetErr;
    U16 NbErr,i;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    ST_ErrorCode_t ErrorCodeTerm = ST_NO_ERROR;
    int LVar;
    char HdmiName[10];

    STTBX_Print(( "Call Init function  ...\n" ));
    NbErr = 0;

    /* get device type (default: 1=7710) */
    RetErr = STTST_GetInteger( pars_p, 1, &LVar );
    if ( RetErr || (LVar < 1 || LVar > 3) )
    {
        STTST_TagCurrentLine( pars_p, "Expected device type 1=7710 2=7100 3=7200");
        RetErr = TRUE;
    }
    else      /* RetErr == FALSE  */
    {
        HDMIInitParams.DeviceType      = (STHDMI_DeviceType_t)(LVar-1);
        HDMIInitParams.CPUPartition_p  = DriverPartition_p;
        HDMIInitParams.MaxOpen         = 1;
        switch ( HDMIInitParams.DeviceType)
        {
            case STHDMI_DEVICE_TYPE_7710 :
            case STHDMI_DEVICE_TYPE_7100 :
            case STHDMI_DEVICE_TYPE_7200 :
                HDMIInitParams.OutputType = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
                strcpy(HDMIInitParams.Target.OnChipHdmiCell.VTGDeviceName,STVTG_DEVICE_NAME);
                strcpy(HDMIInitParams.Target.OnChipHdmiCell.VOUTDeviceName,STVOUT_DEVICE_NAME);
                strcpy(HDMIInitParams.Target.OnChipHdmiCell.EvtDeviceName,STEVT_DEVICE_NAME);
                HDMIInitParams.AVIType = (STHDMI_AVIInfoFrameFormat_t)2;
                HDMIInitParams.SPDType = (STHDMI_SPDInfoFrameFormat_t)1;
                HDMIInitParams.MSType  = (STHDMI_MSInfoFrameFormat_t)1;
                HDMIInitParams.AUDIOType = (STHDMI_AUDIOInfoFrameFormat_t)1;
                HDMIInitParams.VSType = (STHDMI_VSInfoFrameFormat_t)1;
                /*HDMIInitParams.EdidType = (STHDMI_EDIDTimingExtType_t)3; */
                break;
            default :
                break;
        }
        /* artificial method to get HDMI_NO_MEMORY_TEST_MAX_INIT Init :*/
        /* the last init must generate a ST_ERROR_NO_MEMORY */
        for (i=0; (i < HDMI_NO_MEMORY_TEST_MAX_INIT) && (ErrorCode == ST_NO_ERROR); i++ )
        {
            sprintf(HdmiName, "Hdmi%d", i);
            ErrorCode = STHDMI_Init( HdmiName, &HDMIInitParams);

        }
        if ( ErrorCode != ST_ERROR_NO_MEMORY )
        {
            STTBX_Print(( "  STHDMI_Init() : unexpected return code=0x%0x \n", ErrorCode ));
            NbErr++;
        }
        for (i=0; (i < (HDMI_NO_MEMORY_TEST_MAX_INIT-1)) && (ErrorCodeTerm == ST_NO_ERROR); i++ )
        {
            sprintf(HdmiName, "Hdmi%d", i);
            HDMITermParams.ForceTerminate = 0;
            ErrorCodeTerm = STHDMI_Term( HdmiName, &HDMITermParams);
        }
        if ( ErrorCodeTerm != ST_NO_ERROR)
        {
            STTBX_Print(( "  STHDMI_Term() : unexpected return code=0x%0x \n", ErrorCodeTerm ));
            NbErr++;
        }

        /* test result */
        if ( NbErr == 0 )
        {
            STTBX_Print(( "### HDMI_TestLimitInit() result : ok, ST_ERROR_NO_MEMORY returned as expected\n" ));
            RetErr = FALSE;
        }
        else
        {
            STTBX_Print(( "### HDMI_TestLimitInit() result : failed ! \n"));
            RetErr = TRUE;
        }
        STTST_AssignInteger( result_sym_p, NbErr, FALSE);

    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of HDMI_TestLimitInit() */

/*-------------------------------------------------------------------------
 * Function : Test_CmdStart
 *            Definition of test register command
 * Input    :
 * Output   :
 * Return   : TRUE if ok, FALSE if error
 * ----------------------------------------------------------------------*/
BOOL Test_CmdStart(void)
{
    BOOL RetErr = FALSE;

    RetErr = VID_RegisterCmd2();
    #ifdef STI7710_CUT2x
      RetErr |= STTST_AssignString ("CHIP_CUT", "STI7710_CUT2x", TRUE);
    #else
      RetErr |= STTST_AssignString ("CHIP_CUT", "", TRUE);
    #endif
    RetErr |= STTST_RegisterCommand( "HDMI_TestLimit", HDMI_TestLimitInit,"Call Init API function to test ST_ERROR_NO_MEMORY error");
    RetErr |= STTST_RegisterCommand( "HDMI_TestInfoFrame", HDMI_TestInfoFrame,"Call Init API function to test HDMI infoframes transmission");
    RetErr |= STTST_RegisterCommand( "HDMI_SwitchFormat", HDMI_SwitchFormat,"Call Init API function to test switch video fomats");
#ifdef STHDMI_CEC
    RetErr |= STTST_RegisterCommand( "HDMI_OneTouchPlay", HDMI_OneTouchPlay,"One Touch Play : IMView, TxtView, ActSrc");
    RetErr |= STTST_RegisterCommand( "HDMI_UserSwitchOff", HDMI_UserSwitchOff,"User requested a Switch Off");
    RetErr |= STTST_RegisterCommand( "HDMI_Standby", HDMI_Standby,"Standby Appli + Others");
#endif
    /*RetErr |= STTST_RegisterCommand( "HDMI_DisplaySinkEDID", HDMI_DisplaySinkEDID,"Call Init API function to display the Sink EDID");*/
    if ( RetErr )
    {
        RetErr = VID_Inj_RegisterCmd();
    }
    /* In the case of Linux we need to initialise the kernel module of vidtest */
#if defined (ST_OSLINUX)
        VIDTEST_Init();
#endif

    if (!RetErr)
    {
        sprintf( HDMI_Msg, "HDMI_TestCommand()  \t: failed !\n" );
    }
    else
    {
        sprintf( HDMI_Msg, "HDMI_TestCommand()  \t: ok\n" );
    }
    STTBX_Print(( HDMI_Msg ));

    return(RetErr);
}

#ifdef STHDMI_CEC
/* ========================================================================
   Name:        HDMI_CEC_TaskFunction
   Description: Task that handles all of the CEC Features Replies
   ======================================================================== */
static void  HDMI_CEC_TaskFunction (void)
{

U8      index = 0;
BOOL    IsFound = FALSE;
BOOL    IsReceived = FALSE;
BOOL    IsCommand_Handled = FALSE;
STHDMI_CEC_Command_t        Command;
STHDMI_CEC_CommandInfo_t    CommandInfo;
STVOUT_State_t              State;
ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

STOS_TaskEnter(NULL);

    STOS_TaskDelay(ST_GetClocksPerSecond());/* Let Setup Process before starting Action */
    do
    {
        STOS_SemaphoreWait(HDMI_Context[0].HDMI_CECStruct.CEC_Sem_p);

        if((HDMI_Context[0].HDMI_CECStruct.Notify & HDMI_CEC_NOTIFY_COMMAND) == HDMI_CEC_NOTIFY_COMMAND)
        {
            switch(HDMI_Context[0].HDMI_CECStruct.CurrentReceivedCommandInfo.Status)
            {
                case STVOUT_CEC_STATUS_TX_SUCCEED :
                case STVOUT_CEC_STATUS_TX_FAILED :
                    break;
                case STVOUT_CEC_STATUS_RX_SUCCEED :
                    IsReceived = TRUE;
                    IsCommand_Handled = HDMI_HandleCECCommand(&HDMI_Context[0].HDMI_CECStruct.CurrentReceivedCommandInfo.Command);
                    break;
                default :
                    break;
            }
            HDMI_Context[0].HDMI_CECStruct.Notify &= ~HDMI_CEC_NOTIFY_COMMAND;
        }

        if((HDMI_Context[0].HDMI_CECStruct.Notify & HDMI_CEC_NOTIFY_LOGIC_ADDRESS) == HDMI_CEC_NOTIFY_LOGIC_ADDRESS)
        {
            HDMI_Context[0].HDMI_CECStruct.CanSendCommand = TRUE;
            /* HDMI_Context[0].HDMI_CECStruct.CECDevice Updated */
            HDMI_Context[0].HDMI_CECStruct.Notify &= ~HDMI_CEC_NOTIFY_LOGIC_ADDRESS;
        }


    }while(!(HDMI_Context[0].HDMI_CECStruct.CEC_Task.ToBeDeleted));

STOS_TaskExit(NULL);
} /* end of HDMI_CEC_TaskFunction() */
#endif /* STHDMI_CEC */

/*-------------------------------------------------------------------------
 * Function : HDMI_TestInfoFrame
 * Description : test the infoframe's sent  :
 *  (C code program because macro are not convenient for this test)
 * Input    :
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_TestInfoFrame(STTST_Parse_t *pars_p, char *result_sym_p)
{

 ST_ErrorCode_t                ErrCode=ST_NO_ERROR;
 BOOL 						   RetErr =FALSE;
 STHDMI_InitParams_t           HDMI_InitParams;
 STHDMI_OpenParams_t           HDMI_OpenParams;
 STEVT_DeviceSubscribeParams_t EVT_SubcribeParams;
 STEVT_OpenParams_t            EVT_OpenParams;
 STVTG_InitParams_t            VTG_InitParams;
 STVTG_OpenParams_t 		   VTG_OpenParams;
 STVTG_ModeParams_t    		   VTG_ModeParams;
 STVOUT_InitParams_t   		   VOUT_InitParams;
 STVOUT_OpenParams_t   		   VOUT_OpenParams;
 STVOUT_OutputParams_t         VOUT_OutputParams;
 STVTG_TimingMode_t    		   VTG_TimingMode;
 STVMIX_InitParams_t   		   VMIX_InitParams;
 STVMIX_OpenParams_t   		   VMIX_OpenParams;
 STVMIX_ScreenParams_t 		   VMIX_ScreenParams;
 STLAYER_InitParams_t   	   LAYER_InitParams;
 STLAYER_OpenParams_t   	   LAYER_OpenParams;
 STLAYER_LayerParams_t  	   LAYER_LayerParams;
 U32                           NbLayers;
 STVTG_OptionalConfiguration_t VTG_OptionalConfiguration;
 /*STVOUT_State_t                State;*/
 #ifdef STHDMI_CEC
 HDMI_Task_t * task_p;
 #endif

 UNUSED_PARAMETER(pars_p);
 UNUSED_PARAMETER(result_sym_p);

 /* Reset Handles */
 /* ============= */
 memset(DENC_Handle,0,sizeof(DENC_Handle));
 memset(VOUT_Handle,0,sizeof(VOUT_Handle));
 memset(HDMI_Handle,0,sizeof(HDMI_Handle));
 /*memset(HDMI_Context,0,sizeof(HDMI_Context));*/
 memset(VMIX_Handle,0,sizeof(VMIX_Handle));
 memset(LAYER_Handle,0,sizeof(LAYER_Handle));
 memset(VMIX_LayerArray,0,sizeof(VMIX_LayerArray));

 /* Initialize the VTG 1 */
 /* ==================== */
 memset(&VTG_InitParams,0,sizeof(STVTG_InitParams_t));
 memset(&VTG_OpenParams,0,sizeof(STVTG_OpenParams_t));
 VTG_InitParams.DeviceType                          = STVTG_DEVICE_TYPE_VTG_CELL_7100;
 VTG_InitParams.MaxOpen                             = 3;
 #if defined(ST_7100)
 VTG_InitParams.Target.VtgCell3.BaseAddress_p       = (void *)ST7100_VTG1_BASE_ADDRESS;
 VTG_InitParams.Target.VtgCell3.DeviceBaseAddress_p = (void *)0x00000000;
 VTG_InitParams.Target.VtgCell3.InterruptNumber     = ST7100_VTG_0_INTERRUPT;
#endif
#if defined(ST_7109)
 VTG_InitParams.Target.VtgCell3.BaseAddress_p       = (void *)ST7109_VTG1_BASE_ADDRESS;
 VTG_InitParams.Target.VtgCell3.DeviceBaseAddress_p = (void *)0x00000000;
 VTG_InitParams.Target.VtgCell3.InterruptNumber     = ST7109_VTG_0_INTERRUPT;
#endif


 VTG_InitParams.Target.VtgCell3.InterruptLevel      = VTG_INTERRUPT_LEVEL;
 strcpy(VTG_InitParams.Target.VtgCell3.ClkrvName,CLKRV_DeviceName[0]);
 strcpy(VTG_InitParams.EvtHandlerName,"EVT");
 ErrCode=STVTG_Init(VTG_DeviceName[0], &VTG_InitParams);                                                                        /* STVTG_Init */
 if (ErrCode!=ST_NO_ERROR)
 {
    return(ErrCode);
 }
 ErrCode=STVTG_Open(VTG_DeviceName[0],&VTG_OpenParams,&VTG_Handle[0]);                                                          /* STVTG_Open */
 if (ErrCode!=ST_NO_ERROR)
 {
    return(ErrCode);
 }
  /* For 720P configuration (1280x720x50hz) */
   ErrCode=STVTG_SetMode(VTG_Handle[0],STVTG_TIMING_MODE_720P60000_74250);                                                      /* STVTG_SetMode */
 if (ErrCode!=ST_NO_ERROR)
 {
        return(ErrCode);
 }
 VTG_OptionalConfiguration.Option                = STVTG_OPTION_EMBEDDED_SYNCHRO;
 VTG_OptionalConfiguration.Value.EmbeddedSynchro = TRUE;
 ErrCode=STVTG_SetOptionalConfiguration(VTG_Handle[0],&VTG_OptionalConfiguration);                                              /* STVTG_SetOptionalConfiguration */
 if (ErrCode!=ST_NO_ERROR)
 {
    return(ErrCode);
 }
 /* Get the screen params */
 /* --------------------- */
 ErrCode=STVTG_GetMode(VTG_Handle[0],&VTG_TimingMode,&VTG_ModeParams);                                                          /* STVTG_GetMode */
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }
/* VOUT for HDMI */
 /* ============= */
 memset(&VOUT_InitParams,0,sizeof(STVOUT_InitParams_t));
 memset(&VOUT_OpenParams,0,sizeof(STVOUT_OpenParams_t));
 memset(&VOUT_OutputParams,0,sizeof(STVOUT_OutputParams_t));
 VOUT_InitParams.DeviceType                                                = STVOUT_DEVICE_TYPE_7100;
 VOUT_InitParams.OutputType                                                = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
 VOUT_InitParams.CPUPartition_p                                            = DriverPartition_p;
 VOUT_InitParams.MaxOpen                                                   = 3;
#if defined(ST_7100)
 VOUT_InitParams.Target.HDMICell.InterruptNumber                           = ST7100_HDMI_INTERRUPT;
#endif
#if defined(ST_7109)
 VOUT_InitParams.Target.HDMICell.InterruptNumber                           = ST7109_HDMI_INTERRUPT;
#endif

 VOUT_InitParams.Target.HDMICell.InterruptLevel                            = HDMI_INTERRUPT_LEVEL;
 VOUT_InitParams.Target.HDMICell.HSyncActivePolarity                       = FALSE;
 VOUT_InitParams.Target.HDMICell.VSyncActivePolarity                       = FALSE;
#if defined(HDMI_HDCP_ON)
 VOUT_InitParams.Target.HDMICell.IsHDCPEnable                              = TRUE;
#else
 VOUT_InitParams.Target.HDMICell.IsHDCPEnable                              = FALSE;
#endif
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p = NULL;
#if defined(ST_7100)
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p       = (void *)ST7100_HDMI_BASE_ADDRESS;
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p = (void *)ST7100_HDCP_BASE_ADDRESS;
#endif
#if defined(ST_7109)
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p       = (void *)ST7109_HDMI_BASE_ADDRESS;
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p = (void *)ST7109_HDCP_BASE_ADDRESS;
#endif
#if defined(cocoref)||defined(cocoref_v2)||defined(mb411)
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.HPD_Bit             = PIO_BIT_2;
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed       = FALSE;
 strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.I2CDevice,I2C_DeviceName[0]);
 strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.PIODevice,"PIO2");
#endif
#if defined(custom11)||defined(custom13)||defined(custom14)||defined(custom16)
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.HPD_Bit             = PIO_BIT_6;
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed       = FALSE;
 strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.I2CDevice,I2C_DeviceName[0]);
 strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.PIODevice,PIO_DeviceName[4]);
#endif
#if defined(custom21)
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.HPD_Bit             = PIO_BIT_7;
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed       = TRUE;
 strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.I2CDevice,I2C_DeviceName[0]);
 strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.PIODevice,PIO_DeviceName[4]);
#endif
#ifdef STVOUT_INVERTED_HPD /* Because it might be inverted in any of previous boards */
   VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed       = TRUE;
#endif

 strcpy(VOUT_InitParams.Target.HDMICell.VTGName,VTG_DeviceName[0]);
 strcpy(VOUT_InitParams.Target.HDMICell.EventsHandlerName,"EVT");

#ifdef STVOUT_CEC_PIO_COMPARE
 strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCellOne.PWMName,"PWM");
 strcpy(VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCellOne.CECPIOName,PIO_DeviceName[5]); /*1.0; for capture1 5.7*/
 VOUT_InitParams.Target.HDMICell.Target.OnChipHdmiCellOne.CECPIO_BitNumber = PIO_BIT_7;
#endif

 ErrCode=STVOUT_Init(VOUT_DeviceName[0],&VOUT_InitParams);                                                                      /* STVOUT_Init */
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }

 ErrCode=STVOUT_Open(VOUT_DeviceName[0],&VOUT_OpenParams,&VOUT_Handle[0]);                                                      /* STVOUT_Open */
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }
 VOUT_OutputParams.HDMI.AudioFrequency = 48000;
#if defined(HDMI_FORCE_DVI)
 VOUT_OutputParams.HDMI.ForceDVI       = TRUE;
#else
 VOUT_OutputParams.HDMI.ForceDVI       = FALSE;
#endif
#if defined(HDMI_HDCP_ON)
 VOUT_OutputParams.HDMI.IsHDCPEnable   = TRUE;
#else
 VOUT_OutputParams.HDMI.IsHDCPEnable   = FALSE;
#endif

 ErrCode=STVOUT_SetOutputParams(VOUT_Handle[0],&VOUT_OutputParams);                                                             /* STVOUT_SetOutputParams */
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }
 /* Initialize the HDMI interface */
 /* ============================= */
 memset(&HDMI_InitParams,0,sizeof(STHDMI_InitParams_t));
 memset(&HDMI_OpenParams,0,sizeof(STHDMI_OpenParams_t));
 HDMI_InitParams.DeviceType     = STHDMI_DEVICE_TYPE_7100;
 HDMI_InitParams.OutputType     = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
 HDMI_InitParams.MaxOpen        = 1;
 HDMI_InitParams.CPUPartition_p = DriverPartition_p;
 HDMI_InitParams.AVIType        = STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO;
 HDMI_InitParams.SPDType        = STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE;
 HDMI_InitParams.MSType         = STHDMI_MS_INFOFRAME_FORMAT_VER_ONE;
 HDMI_InitParams.AUDIOType      = STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE;
 HDMI_InitParams.VSType         = STHDMI_VS_INFOFRAME_FORMAT_VER_ONE;
 strcpy(HDMI_InitParams.Target.OnChipHdmiCell.VTGDeviceName  , "VTG");
 strcpy(HDMI_InitParams.Target.OnChipHdmiCell.VOUTDeviceName , "VOUT");
 strcpy(HDMI_InitParams.Target.OnChipHdmiCell.EvtDeviceName  , "EVT");
 ErrCode=STHDMI_Init(HDMI_DeviceName[0],&HDMI_InitParams);                                                                      /* STHDMI_Init */
 if (ErrCode!=ST_NO_ERROR)
 {
    return(ErrCode);
 }
 /* Open an handle */
 /* ============== */
 ErrCode = STHDMI_Open(HDMI_DeviceName[0],&HDMI_OpenParams,&HDMI_Handle[0]);                                                      /* STHDMI_Open */
 if (ErrCode!=ST_NO_ERROR)
 {
    return(ErrCode);
 }
 /* Create HDMI context for callback */
 /* ================================ */
 HDMI_Context[0].HDMI_Handle                 = HDMI_Handle[0];
 HDMI_Context[0].VTG_Handle                  = VTG_Handle[0];
 HDMI_Context[0].VOUT_Handle                 = VOUT_Handle[0];
 HDMI_Context[0].VOUT_State                  = STVOUT_NO_RECEIVER;
 HDMI_Context[0].HDMI_AutomaticEnableDisable = TRUE;
 HDMI_Context[0].HDMI_IsEnabled              = FALSE;
 HDMI_Context[0].HDMI_OutputType             = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
 HDMI_Context[0].HDMI_ScanInfo               = STHDMI_SCAN_INFO_OVERSCANNED;
 HDMI_Context[0].HDMI_Colorimetry            = STVOUT_ITU_R_601;
 HDMI_Context[0].HDMI_AspectRatio            = STGXOBJ_ASPECT_RATIO_16TO9;
	  /*	 STGXOBJ_ASPECT_RATIO_16TO9;*/

 HDMI_Context[0].HDMI_ActiveAspectRatio      = STGXOBJ_ASPECT_RATIO_16TO9;
 HDMI_Context[0].HDMI_PictureScaling         = STHDMI_PICTURE_NON_UNIFORM_SCALING;
 HDMI_Context[0].HDMI_AudioFrequency         = 48000;
#if defined(HDMI_HDCP_ON)
 HDMI_Context[0].HDMI_IV0_Key                = HDMI_IV_0_KEY_VALUE;
 HDMI_Context[0].HDMI_IV1_Key                = HDMI_IV_1_KEY_VALUE;
 HDMI_Context[0].HDMI_KV0_Key                = HDMI_KSV_0_KEY_VALUE;
 HDMI_Context[0].HDMI_KV1_Key                = HDMI_KSV_1_KEY_VALUE;
 memcpy(HDMI_Context[0].HDMI_DeviceKeys,HDMI_DeviceKeys,sizeof(HDMI_DeviceKeys));
#endif
 strcpy(HDMI_Context[0].AUD_DeviceName,"AUDIO");
 strcpy(HDMI_Context[0].VTG_DeviceName,"VTG");

 /* Subscribe to callback */
 /* ===================== */
 memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
 memset(&EVT_OpenParams ,0,sizeof(STEVT_OpenParams_t));
 EVT_SubcribeParams.NotifyCallback = HDMIi_HotplugCallback;
 ErrCode|=STEVT_Open("EVT",&EVT_OpenParams,&HDMI_Context[0].EVT_Handle);                                                        /* STEVT_Open */
 ErrCode|=STEVT_SubscribeDeviceEvent(HDMI_Context[0].EVT_Handle, "VTG",STVOUT_CHANGE_STATE_EVT, &EVT_SubcribeParams);           /* Subscribe STVOUT_CHANGE_STATE_EVT */
 if (ErrCode!=ST_NO_ERROR)
 {
       return(ErrCode);
 }
 /* Subscribe to audio event in order to change audio frequency on HDMI screen */
 /* ========================================================================== */
 memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
 EVT_SubcribeParams.NotifyCallback=HDMIi_AudioCallback;
 ErrCode=STEVT_SubscribeDeviceEvent(HDMI_Context[0].EVT_Handle,"AUD0",STAUD_NEW_FREQUENCY_EVT,&EVT_SubcribeParams);             /* Subscribe STAUD_NEW_FREQUENCY_EVT */
 if (ErrCode!=ST_NO_ERROR)
 {
     return(ErrCode);
 }

#ifdef STHDMI_CEC
 /* Subscribe to Logical Address */
 /* ============================ */
 memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
 EVT_SubcribeParams.NotifyCallback = HDMIi_CEC_LogicalAddress_CallBack;
 ErrCode = STEVT_SubscribeDeviceEvent(HDMI_Context[0].EVT_Handle,"VTG", STVOUT_CEC_LOGIC_ADDRESS_EVT, &EVT_SubcribeParams);     /* Subscribe STVOUT_CEC_LOGIC_ADDRESS_EVT */
 if (ErrCode!=ST_NO_ERROR)
 {
     return(ErrCode);
 }

 /* Subscribe to CEC Command */
 /* ============================ */
 memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
 EVT_SubcribeParams.NotifyCallback = HDMIi_CEC_Command_CallBack;
 ErrCode = STEVT_SubscribeDeviceEvent(HDMI_Context[0].EVT_Handle,"VTG", STHDMI_CEC_COMMAND_EVT, &EVT_SubcribeParams);             /* Subscribe STHDMI_CEC_COMMAND_EVT */
 if (ErrCode!=ST_NO_ERROR)
 {
     return(ErrCode);
 }
 #endif

 /* Start the HDMI state machine */
 /* ============================ */
 ErrCode = STVOUT_Start(HDMI_Context[0].VOUT_Handle);                                                                             /* STVOUT_Start */
 if (ErrCode!=ST_NO_ERROR)
 {
       return(ErrCode);
 }

#ifdef STHDMI_CEC
            HDMI_Context[0].HDMI_CECStruct.CEC_Task.IsRunning = FALSE;
            HDMI_Context[0].HDMI_CECStruct.CEC_Task.ToBeDeleted = FALSE;
            HDMI_Context[0].HDMI_CECStruct.CEC_TaskStarted = FALSE;
            HDMI_Context[0].HDMI_CECStruct.CEC_Sem_p = STOS_SemaphoreCreateFifo(NULL,0); /* Task start Blocked */

            HDMI_Context[0].HDMI_CECStruct.CanSendCommand = FALSE;
            HDMI_Context[0].HDMI_CECStruct.ActiveSource   = FALSE;
            HDMI_Context[0].HDMI_CECStruct.Standby        = TRUE;
            sprintf(HDMI_Context[0].HDMI_CECStruct.Language,"ENG");

            task_p  = &(HDMI_Context[0].HDMI_CECStruct.CEC_Task);
            if (task_p->IsRunning)
            {
                return(ST_ERROR_ALREADY_INITIALIZED);
            }
            HDMI_Context[0].HDMI_CECStruct.Notify = HDMI_CEC_NOTIFY_CLEAR_ALL;
            task_p->ToBeDeleted = FALSE;
    ErrCode |= STOS_TaskCreate ((void (*) (void*))HDMI_CEC_TaskFunction,
                        (void*)NULL,
                        (DriverPartition_p),
                         1024*16,
                        &(task_p->TaskStack),
                        (DriverPartition_p),
                        &(task_p->Task_p),
                        &(task_p->TaskDesc),
                        HDMI_TASK_CEC_PRIORITY,
                        "HDMI[?].CECTask",
                        0 /*task_flags_high_priority_process*/);

    if(ErrCode != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    task_p->IsRunning  = TRUE;

#endif


 /* Set Vmix parameters */
 memset(&VMIX_InitParams  ,0,sizeof(STVMIX_InitParams_t));
 memset(&VMIX_OpenParams  ,0,sizeof(STVMIX_OpenParams_t));
 memset(&VMIX_ScreenParams,0,sizeof(STVMIX_ScreenParams_t));

 VMIX_InitParams.CPUPartition_p      = DriverPartition_p;
#if defined(ST_7100)
 VMIX_InitParams.DeviceType          = STVMIX_GENERIC_GAMMA_TYPE_7100;
 VMIX_InitParams.BaseAddress_p       = (void *)ST7100_VMIX1_BASE_ADDRESS;
#endif
#if defined(ST_7109)
 VMIX_InitParams.DeviceType          = STVMIX_GENERIC_GAMMA_TYPE_7109;
 VMIX_InitParams.BaseAddress_p       = (void *)ST7109_VMIX1_BASE_ADDRESS;
#endif
 VMIX_InitParams.DeviceBaseAddress_p = NULL;
 VMIX_InitParams.MaxOpen             = 2; /* 1+1 for STHDMI */
 VMIX_InitParams.MaxLayer            = 5;
 VMIX_InitParams.OutputArray_p       = VMIX_OutDeviceNames;
 strcpy(VMIX_OutDeviceNames[0],VOUT_DeviceName[0]); /* RGB  */
 strcpy(VMIX_OutDeviceNames[1],"\0");
 strcpy(VMIX_InitParams.VTGName,VTG_DeviceName[0]);
 strcpy(VMIX_InitParams.EvtHandlerName,"EVT");
 /* Init the VMIX */
 /* ------------- */
 ErrCode = STVMIX_Init(VMIX_DeviceName[0],&VMIX_InitParams);                                                                    /* STVMIX_Init */
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }
 ErrCode=STVMIX_Open(VMIX_DeviceName[0],&VMIX_OpenParams,&VMIX_Handle[0]);                                                      /* STVMIX_Open */
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }
 ErrCode=STVMIX_SetTimeBase(VMIX_Handle[0],VTG_DeviceName[0]);                                                                  /* STVMIX_SetTimeBase */
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }
 /* Set the screen params */
 /* --------------------- */
 ErrCode=STVTG_GetMode(VTG_Handle[0],&VTG_TimingMode,&VTG_ModeParams);                                                          /* STVTG_GetMode */
 if (ErrCode!=ST_NO_ERROR)
 {
    return(ErrCode);
 }

 /* Set the initialization structure for layer */
 /* -------------------------------- */
 memset(&LAYER_InitParams ,0,sizeof(STLAYER_InitParams_t));
 memset(&LAYER_OpenParams ,0,sizeof(STLAYER_OpenParams_t));
 memset(&LAYER_LayerParams,0,sizeof(STLAYER_LayerParams_t));
 LAYER_LayerParams.AspectRatio                = STGXOBJ_ASPECT_RATIO_16TO9;
 LAYER_LayerParams.ScanType                   = VTG_ModeParams.ScanType;
 LAYER_LayerParams.Width                      = VTG_ModeParams.ActiveAreaWidth;
 LAYER_LayerParams.Height                     = VTG_ModeParams.ActiveAreaHeight;
 LAYER_InitParams.CPUPartition_p              = DriverPartition_p;
 LAYER_InitParams.CPUBigEndian                = FALSE;
 LAYER_InitParams.SharedMemoryBaseAddress_p   = (void *)RAM1_BASE_ADDRESS;/*(void *)NULL;*/
 LAYER_InitParams.MaxHandles                  = 2;
 LAYER_InitParams.AVMEM_Partition             = (STAVMEM_PartitionHandle_t)NULL; /*AVMEM_PartitionHandle[0];*/
 LAYER_InitParams.MaxViewPorts                = 4;
 LAYER_InitParams.ViewPortNodeBuffer_p        = NULL;
 LAYER_InitParams.NodeBufferUserAllocated     = FALSE;
 LAYER_InitParams.ViewPortBuffer_p            = NULL;
 LAYER_InitParams.ViewPortBufferUserAllocated = FALSE;
 LAYER_InitParams.LayerParams_p               = &LAYER_LayerParams;
 LAYER_InitParams.LayerType                   = STLAYER_HDDISPO2_VIDEO1;
#if defined(ST_7100)
  LAYER_InitParams.BaseAddress_p               = (void *)ST7100_VID1_LAYER_BASE_ADDRESS;
  LAYER_InitParams.BaseAddress2_p              = (void *)ST7100_DISP0_BASE_ADDRESS;
#endif
#if defined(ST_7109)
  LAYER_InitParams.BaseAddress_p               = (void *)ST7109_VID1_LAYER_BASE_ADDRESS;
  LAYER_InitParams.BaseAddress2_p              = (void *)ST7109_DISP0_BASE_ADDRESS;
#endif
  LAYER_InitParams.DeviceBaseAddress_p         = NULL;
  strcpy(LAYER_InitParams.EventHandlerName,"EVT");

 /* Init the LAYER */
 /* -------------- */
 ErrCode=STLAYER_Init(LAYER_DeviceName[0],&LAYER_InitParams);                                                                   /* STLAYER_Init */
 if (ErrCode!=ST_NO_ERROR)
 {
    return(ErrCode);
 }
 VMIX_ScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
 VMIX_ScreenParams.ScanType    = VTG_ModeParams.ScanType;
 VMIX_ScreenParams.FrameRate   = VTG_ModeParams.FrameRate;
 VMIX_ScreenParams.Width       = VTG_ModeParams.ActiveAreaWidth;
 VMIX_ScreenParams.Height      = VTG_ModeParams.ActiveAreaHeight;
 VMIX_ScreenParams.XStart      = VTG_ModeParams.ActiveAreaXStart;
 VMIX_ScreenParams.YStart      = VTG_ModeParams.ActiveAreaYStart;
 ErrCode = STVMIX_SetScreenParams(VMIX_Handle[0],&VMIX_ScreenParams);                                                           /* STVMIX_SetScreenParams */
 if (ErrCode!=ST_NO_ERROR)
 {
    return(ErrCode);
 }
 NbLayers=0;
 strcpy(VMIX_LayerParams[0][NbLayers].DeviceName,LAYER_DeviceName[0]);
 VMIX_LayerParams[0][NbLayers].ActiveSignal=TRUE;
 VMIX_LayerArray[0][NbLayers]=&VMIX_LayerParams[0][NbLayers];
 NbLayers++;
 if (GFX_LAYER_Handle[0]!=0)
 {
   strcpy(VMIX_LayerParams[0][NbLayers].DeviceName,GFX_LAYER_DeviceName[0]);
   VMIX_LayerParams[0][NbLayers].ActiveSignal=TRUE;
   VMIX_LayerArray[0][NbLayers]=&VMIX_LayerParams[0][NbLayers];
   NbLayers++;
  }
 ErrCode=STVMIX_ConnectLayers(VMIX_Handle[0],(const STVMIX_LayerDisplayParams_t *const *)&VMIX_LayerArray[0][0],NbLayers);      /* STVMIX_ConnectLayers */
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }
 ErrCode=STVMIX_Enable(VMIX_Handle[0]);                                                                                         /* STVMIX_Enable */
 if (ErrCode!=ST_NO_ERROR)
 {
    return(ErrCode);
 }

 return(API_EnableError ? RetErr : FALSE );
}/* end of HDMI_TestInfoFrame() */

/* ========================================================================
   Name:        HDMIi_HotplugCallback
   Description: When the hdmi screen is plugged/unplugged or change state
   ======================================================================== */

void HDMIi_HotplugCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
 U32            i;
 ST_ErrorCode_t ErrCode=ST_NO_ERROR;

 UNUSED_PARAMETER(Reason);
 UNUSED_PARAMETER(Event);
 UNUSED_PARAMETER(SubscriberData_p);

 for (i=0;i<HDMI_MAX_NUMBER;i++)
  {
   if (strcmp(HDMI_Context[i].VTG_DeviceName,RegistrantName)==0) break;
  }
 if (i==HDMI_MAX_NUMBER)
  {
  printf("HDMIi_HotplugCallback(?):**ERROR** !!! Event Received from unknow HDMI interface !!!\n");
   return;
  }
 if (EventData==NULL)
  {
   printf("HDMIi_HotplugCallback(%d):**ERROR** !!! EventData pointer is NULL !!!\n",i);
   return;
  }

 /* Look for state change */
 /* ===================== */
 printf("HDMIi_HotplugCallback(%d): Changing state from ",i);
 switch(HDMI_Context[i].VOUT_State)
  {
   case STVOUT_DISABLED                   : printf("STVOUT_DISABLED to ");                   break;
   case STVOUT_ENABLED                    : printf("STVOUT_ENABLED to ");                    break;
   case STVOUT_NO_RECEIVER                : printf("STVOUT_NO_RECEIVER to ");                break;
   case STVOUT_RECEIVER_CONNECTED         : printf("STVOUT_RECEIVER_CONNECTED to ");         break;
   case STVOUT_NO_ENCRYPTION              : printf("STVOUT_NO_ENCRYPTION to ");              break;
   case STVOUT_NO_HDCP_RECEIVER           : printf("STVOUT_NO_HDCP_RECEIVER to ");           break;
   case STVOUT_AUTHENTICATION_IN_PROGRESS : printf("STVOUT_AUTHENTICATION_IN_PROGRESS to "); break;
   case STVOUT_AUTHENTICATION_FAILED      : printf("STVOUT_AUTHENTICATION_FAILED to ");      break;
   case STVOUT_AUTHENTICATION_SUCCEEDED   : printf("STVOUT_AUTHENTICATION_SUCCEEDED to ");   break;
   default                                : printf("STVOUT_UNKNOWN to ");                    break;
  }
 HDMI_Context[i].VOUT_State=*((const STVOUT_State_t*)EventData);
 /*printf("the event data of hotplug is  is   %d \n\r", *(STVOUT_State_t *)EventData);*/
 switch(HDMI_Context[i].VOUT_State)
  {
   case STVOUT_DISABLED                   : printf("STVOUT_DISABLED\n");                    break;
   case STVOUT_ENABLED                    : printf("STVOUT_ENABLED\n");                     break;
   case STVOUT_NO_RECEIVER                : printf("STVOUT_NO_RECEIVER\n");                 break;
   case STVOUT_RECEIVER_CONNECTED         : printf("STVOUT_RECEIVER_CONNECTED\n");          break;
   case STVOUT_NO_ENCRYPTION              : printf("STVOUT_NO_ENCRYPTION\n");               break;
   case STVOUT_NO_HDCP_RECEIVER           : printf("STVOUT_NO_HDCP_RECEIVER\n");            break;
   case STVOUT_AUTHENTICATION_IN_PROGRESS : printf("STVOUT_AUTHENTICATION_IN_PROGRESS\n");  break;
   case STVOUT_AUTHENTICATION_FAILED      : printf("STVOUT_AUTHENTICATION_FAILED\n");       break;
   case STVOUT_AUTHENTICATION_SUCCEEDED   : printf("STVOUT_AUTHENTICATION_SUCCEEDED\n");    break;
   default                                : printf("STVOUT_UNKNOWN\n");                     break;
  }

 /* Check new state */
 /* =============== */
 switch(HDMI_Context[i].VOUT_State)
  {
   /* No receiver is connected */
   /* ------------------------ */
   case STVOUT_NO_RECEIVER:
#ifdef STHDMI_CEC
    HDMI_Context[0].HDMI_CECStruct.CanSendCommand = FALSE;
#endif
    /* Disable the output if automatic enable/disable */
    if (HDMI_Context[i].HDMI_AutomaticEnableDisable==TRUE)
    {
      ErrCode=HDMI_DisableOutput(i);
      if (ErrCode!=ST_NO_ERROR)
      {
        printf("HDMIi_HotplugCallback(%d):**ERROR** !!! Unable to disable the HDMI interface !!!\n",i);
        break;
      }
    }
    break;

   /* Receiver is connected */
   /* --------------------- */
   case STVOUT_RECEIVER_CONNECTED :
    /* Enable the output if automatic enable/disable */
    if (HDMI_Context[i].HDMI_AutomaticEnableDisable==TRUE)
    {
      ErrCode = HDMI_EnableOutput(i);
      if (ErrCode!=ST_NO_ERROR)
      {
        printf("HDMIi_HotplugCallback(%d):**ERROR** !!! Unable to enable the HDMI interface !!!\n",i);
        break;
      }
    }
    break;
    default:
    break;
  }
}/* end of HDMIi_HotplugCallback() */

#ifdef STHDMI_CEC
/*-------------------------------------------------------------------------
 * Function : HDMI_OneTouchPlay
 * Description :
 * Input    :
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_OneTouchPlay(STTST_Parse_t *pars_p, char *result_sym_p)
{
    STHDMI_CEC_Command_t Command;
    int LVar;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL           RetErr = FALSE;


    HDMI_Context[0].HDMI_CECStruct.Standby = FALSE;
    CEC_Debug_Print((("We  Go Out Of Standby\n")));
    HDMI_Context[0].HDMI_CECStruct.ActiveSource   = TRUE;  /* We got order from the user to start the Show */
    CEC_Debug_Print((("We  Become the Active Source\n")));

    STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
    RetErr = STTST_GetInteger( pars_p, 1, &LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected 1/0 For ImageViewOn");
        RetErr = TRUE;
    }
    else
    {
        if(LVar == 1)
        {
            Command.Source = HDMI_Context[0].HDMI_CECStruct.CECDevice.LogicalAddress ;
            Command.Destination = 0; /* TV */
            Command.CEC_Opcode = STHDMI_CEC_OPCODE_IMAGE_VIEW_ON;
            Command.Retries = 2;
            STHDMI_CECSendCommand(HDMI_Handle[0], &Command);
        }
    }

    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected 1/0 For TextViewOn");
        RetErr = TRUE;
    }
    else
    {
        if(LVar == 1)
        {
            Command.Source = HDMI_Context[0].HDMI_CECStruct.CECDevice.LogicalAddress ;
            Command.Destination = 0; /* TV */
            Command.CEC_Opcode = STHDMI_CEC_OPCODE_TEXT_VIEW_ON;
            Command.Retries = 2;
            STHDMI_CECSendCommand(HDMI_Handle[0], &Command);
        }
    }

    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected 1/0 For Active Source");
        RetErr = TRUE;
    }
    else
    {
        if(LVar == 1)
        {
            Command.Source = HDMI_Context[0].HDMI_CECStruct.CECDevice.LogicalAddress ;
            Command.Destination = 0; /* TV */
            Command.CEC_Opcode = STHDMI_CEC_OPCODE_ACTIVE_SOURCE;
            Command.CEC_Operand.PhysicalAddress = HDMI_Context[0].HDMI_CECStruct.PhysicalAddress;
            Command.Retries = 2;
            STHDMI_CECSendCommand(HDMI_Handle[0], &Command);
        }
    }
    STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */

    return(API_EnableError ? RetErr : FALSE );
} /* end of HDMI_OneTouchPlay() */

/*-------------------------------------------------------------------------
 * Function : HDMI_UserSwitchOff
 * Description :
 * Input    :
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_UserSwitchOff(STTST_Parse_t *pars_p, char *result_sym_p)
{
    STHDMI_CEC_Command_t Command;
    int LVar;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL           RetErr = FALSE;


    if(HDMI_Context[0].HDMI_CECStruct.ActiveSource)
    {
        RetErr = STTST_GetInteger( pars_p, 1, &LVar );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "Expected 1/0 For Inactive Source");
            RetErr = TRUE;
        }
        else
        {
            if(LVar == 1)
            {
                Command.Source = HDMI_Context[0].HDMI_CECStruct.CECDevice.LogicalAddress ;
                Command.Destination = 15; /* BRAODCAST */
                Command.CEC_Opcode = STHDMI_CEC_OPCODE_INACTIVE_SOURCE;
                Command.CEC_Operand.PhysicalAddress = HDMI_Context[0].HDMI_CECStruct.PhysicalAddress;
                Command.Retries = 2;
                STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
                STHDMI_CECSendCommand(HDMI_Handle[0], &Command);
                STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
            }
        }
    }

    HDMI_Context[0].HDMI_CECStruct.Standby = TRUE;
    CEC_Debug_Print((("Appli User request Standby\n")));
    HDMI_Context[0].HDMI_CECStruct.ActiveSource   = FALSE;
    CEC_Debug_Print((("We are no more the Active Source\n")));

    return(API_EnableError ? RetErr : FALSE );
} /* end of HDMI_UserSwitchOff() */

/*-------------------------------------------------------------------------
 * Function : HDMI_Standby
 * Description :
 * Input    :
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_Standby(STTST_Parse_t *pars_p, char *result_sym_p)
{
    STHDMI_CEC_Command_t Command;
    int LVar;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL           RetErr = FALSE;


    RetErr = STTST_GetInteger( pars_p, 1, &LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected 1/0 For Broadcast/TV");
        RetErr = TRUE;
    }
    else
    {
        Command.Source = HDMI_Context[0].HDMI_CECStruct.CECDevice.LogicalAddress ;
        if(LVar == 1)
        {
            Command.Destination = 15; /* BRAODCAST */
        }
        else
        {
            Command.Destination = 0; /* TV */
        }
        Command.CEC_Opcode = STHDMI_CEC_OPCODE_STANDBY;
        Command.Retries = 2;
        STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
        STHDMI_CECSendCommand(HDMI_Handle[0], &Command);
        STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
    }

    HDMI_Context[0].HDMI_CECStruct.Standby = TRUE;
    CEC_Debug_Print((("Appli Standby\n")));
    HDMI_Context[0].HDMI_CECStruct.ActiveSource   = FALSE;
    CEC_Debug_Print((("We are no more the Active Source\n")));
    if(LVar == 1)
    {
        CEC_Debug_Print((("Standby All Equipment\n")));
    }
    else
    {
        CEC_Debug_Print((("Standby TV\n")));
    }

    return(API_EnableError ? RetErr : FALSE );
} /* end of HDMI_Standby() */

/* ========================================================================
   Name:        HDMIi_CEC_LogicalAddress_CallBack
   Description:
   ======================================================================== */

void HDMIi_CEC_LogicalAddress_CallBack(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
     STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    HDMI_Context[0].HDMI_CECStruct.CECDevice = (* (STVOUT_CECDevice_t *) EventData_p);
    HDMI_Context[0].HDMI_CECStruct.Notify |= HDMI_CEC_NOTIFY_LOGIC_ADDRESS;
    STOS_SemaphoreSignal(HDMI_Context[0].HDMI_CECStruct.CEC_Sem_p);

} /* End of HDMIi_CEC_LogicalAddress_CallBack() */

/* ========================================================================
   Name:        HDMIi_CEC_Command_CallBack
   Description:
   ======================================================================== */

void HDMIi_CEC_Command_CallBack(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
     STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    HDMI_Context[0].HDMI_CECStruct.CurrentReceivedCommandInfo = (* (STHDMI_CEC_CommandInfo_t *) EventData_p);
    HDMI_Context[0].HDMI_CECStruct.Notify |= HDMI_CEC_NOTIFY_COMMAND;
    STOS_SemaphoreSignal(HDMI_Context[0].HDMI_CECStruct.CEC_Sem_p);

} /* End of HDMIi_CEC_Command_CallBack() */
#endif

/* ========================================================================
   Name:        HDMIi_AudioCallback
   Description: When the audio frequency changed, this callback is called
   ======================================================================== */
void HDMIi_AudioCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
 U32                   i,NewAudioFrequency;
 STVOUT_OutputParams_t VOUT_OutputParams;
 ST_ErrorCode_t        ErrCode=ST_NO_ERROR;

 /* Detect from which AUDIO comes the event */
 /* ======================================= */
 UNUSED_PARAMETER(Reason);
 UNUSED_PARAMETER(Event);
 UNUSED_PARAMETER(SubscriberData_p);

 for (i=0;i<HDMI_MAX_NUMBER;i++)
  {
   if (HDMI_Context[i].HDMI_Handle!=0)
    {
     if (strcmp(RegistrantName,HDMI_Context[i].AUD_DeviceName)==0) break;
    }
  }
 /* If it is a very strange event, not attached to any AUDIO */
 if (i==HDMI_MAX_NUMBER)
  {
   printf("HDMIi_AudioCallback(?):**ERROR** !!! Event Received from unknow HDMI interface !!!\n");
   return;
  }
 if (EventData==NULL)
  {
   printf("HDMIi_AudioCallback(%d):**ERROR** !!! EventData pointer is NULL !!!\n",i);
   return;
  }
 /* Get new frequency */
 NewAudioFrequency=(*(const U32*)EventData);
printf("the event data of audio is     %d \n\r", (*(const U32*)EventData));


 /* If there is a real frequency changed */
 /* ==================================== */
 if (HDMI_Context[i].HDMI_AudioFrequency!=NewAudioFrequency)
  {
   HDMI_Context[i].HDMI_AudioFrequency=NewAudioFrequency;
   /* If the HDMI is on, affect HDMI output */
   /* ------------------------------------- */
   if (HDMI_Context[i].HDMI_IsEnabled==TRUE)
    {
     ErrCode=STVOUT_GetOutputParams(HDMI_Context[i].VOUT_Handle,&VOUT_OutputParams);
     if (ErrCode!=ST_NO_ERROR)
      {
       printf("HDMIi_HotplugCallback(%d):**ERROR** !!! Unable to get output params !!!\n",i);
       return;
      }
     VOUT_OutputParams.HDMI.AudioFrequency=HDMI_Context[i].HDMI_AudioFrequency;
     ErrCode=STVOUT_SetOutputParams(HDMI_Context[i].VOUT_Handle,&VOUT_OutputParams);
     if (ErrCode!=ST_NO_ERROR)
      {
       printf("HDMIi_HotplugCallback(%d):**ERROR** !!! Unable to set output params !!!\n",i);
       return;
      }
    }
  }
}/* HDMIi_AudioCallback */
#ifdef STHDMI_CEC
/* ========================================================================
   Name:        HDMI_HandleCECCommand
   Description:
   ======================================================================== */
BOOL HDMI_HandleCECCommand  (STHDMI_CEC_Command_t*   const CEC_Command_p
                             )
{
    STHDMI_CEC_Command_t Command_Response;
    BOOL IsCommandHandled = FALSE;
    switch (CEC_Command_p->CEC_Opcode)
    {
        case STHDMI_CEC_OPCODE_ABORT_MESSAGE            :
            /* Test Command, Already Handled by the Driver */
            break;
        case STHDMI_CEC_OPCODE_ACTIVE_SOURCE            :
            /* Another Source Became Active, so we're Active no more */
            HDMI_Context[0].HDMI_CECStruct.ActiveSource   = FALSE;
            CEC_Debug_Print((("We're not the Active Source\n")));
            break;
        case STHDMI_CEC_OPCODE_FEATURE_ABORT            :
            /* One request couldn't be fulfilled */
            break;
        case STHDMI_CEC_OPCODE_GIVE_DEVICE_POWER_STATUS :
            if(CEC_Command_p->Destination != 15)
            {
                Command_Response.Source = HDMI_Context[0].HDMI_CECStruct.CECDevice.LogicalAddress;
                Command_Response.Destination = CEC_Command_p->Source;
                Command_Response.CEC_Opcode = STHDMI_CEC_OPCODE_REPORT_POWER_STATUS;
                if(HDMI_Context[0].HDMI_CECStruct.Standby)
                {
                        Command_Response.CEC_Operand.PowerStatus = STHDMI_CEC_POWER_STATUS_STANDBY;
                }
                else
                {
                        Command_Response.CEC_Operand.PowerStatus = STHDMI_CEC_POWER_STATUS_ON;
                }
                Command_Response.Retries = 2;
                STHDMI_CECSendCommand(HDMI_Handle[0], &Command_Response);
            }
            break;
        case STHDMI_CEC_OPCODE_GIVE_PHYSICAL_ADDRESS    :
            /* Already Handled by the Driver */
            break;
        case STHDMI_CEC_OPCODE_IMAGE_VIEW_ON            :
            /* This is addressed only for TV */
            break;
        case STHDMI_CEC_OPCODE_POLLING_MESSAGE          :
            /* Polling, Already Handled by the Driver */
            break;
        case STHDMI_CEC_OPCODE_REPORT_PHYSICAL_ADDRESS  :
            /* Result of a Give Physical Address Request */
            break;
        case STHDMI_CEC_OPCODE_REPORT_POWER_STATUS      :
            /* Result of Give Device Power Status */
            break;
        case STHDMI_CEC_OPCODE_REQUEST_ACTIVE_SOURCE    :
            /* We answer this request if we're concerned */
            if(HDMI_Context[0].HDMI_CECStruct.ActiveSource)
            {
                if(CEC_Command_p->Destination == 15)
                {
                    Command_Response.Source = HDMI_Context[0].HDMI_CECStruct.CECDevice.LogicalAddress;
                    Command_Response.Destination = 15;
                    Command_Response.CEC_Opcode = STHDMI_CEC_OPCODE_ACTIVE_SOURCE;
                    Command_Response.CEC_Operand.PhysicalAddress = HDMI_Context[0].HDMI_CECStruct.PhysicalAddress;
                    Command_Response.Retries = 2;
                    STHDMI_CECSendCommand(HDMI_Handle[0], &Command_Response);
                }
            }
            break;
        case STHDMI_CEC_OPCODE_SET_MENU_LANGUAGE        :
            /* Application Should Set The Menu Language */
            if(   ( (CEC_Command_p->CEC_Operand.Language[0]=='E')&&(CEC_Command_p->CEC_Operand.Language[1]=='N')&&(CEC_Command_p->CEC_Operand.Language[2]=='G') )
               || ( (CEC_Command_p->CEC_Operand.Language[0]=='F')&&(CEC_Command_p->CEC_Operand.Language[1]=='R')&&(CEC_Command_p->CEC_Operand.Language[2]=='A') )
               || ( (CEC_Command_p->CEC_Operand.Language[0]=='I')&&(CEC_Command_p->CEC_Operand.Language[1]=='T')&&(CEC_Command_p->CEC_Operand.Language[2]=='A') )
              )
            {
                strcpy(HDMI_Context[0].HDMI_CECStruct.Language,CEC_Command_p->CEC_Operand.Language);
                CEC_Debug_Print((("Language %s Selected\n",HDMI_Context[0].HDMI_CECStruct.Language)));
            }
            else
            {
                CEC_Debug_Print((("Language %s Not Supported\n",CEC_Command_p->CEC_Operand.Language)));
            }

            break;
        case STHDMI_CEC_OPCODE_STANDBY                  :
            /* Appli have to switch off */
            HDMI_Context[0].HDMI_CECStruct.ActiveSource   = FALSE;
            /* Stop the diplay  <!> */
            CEC_Debug_Print((("We're not the Active Source\n")));
            HDMI_Context[0].HDMI_CECStruct.Standby        = TRUE;
            CEC_Debug_Print((("We Go To Standby\n")));
            break;
        case STHDMI_CEC_OPCODE_TEXT_VIEW_ON             :
            /* This is addressed only for TV */
            break;
        default                            :
            /* Otherwise, this is a feature not supported */
            if( CEC_Command_p->Destination != 15 )
            {
                Command_Response.Source = CEC_Command_p->Destination;
                Command_Response.Destination = CEC_Command_p->Source;
                Command_Response.CEC_Opcode = STHDMI_CEC_OPCODE_FEATURE_ABORT;
                Command_Response.CEC_Operand.FeatureAbort.FeatureOpcode = CEC_Command_p->CEC_Opcode;
                Command_Response.CEC_Operand.FeatureAbort.AbortReason   = STHDMI_CEC_ABORT_REASON_UNRECOGNIZED_OPCODE;
                Command_Response.Retries = 2;
                STHDMI_CECSendCommand(HDMI_Handle[0], &Command_Response);
            }
            break;
    }
    return (IsCommandHandled);
}
#endif

/* ========================================================================
   Name:        HDMI_EnableOutput
   Description: Enable the HDMI output
   ======================================================================== */
ST_ErrorCode_t HDMI_EnableOutput(U32 DeviceId)
{
 STHDMI_OutputWindows_t  HDMI_OutputWindows;
 STHDMI_AVIInfoFrame_t   HDMI_AVIInfoFrame;
 STHDMI_AUDIOInfoFrame_t HDMI_AUDIOInfoFrame;
 STVOUT_OutputParams_t   VOUT_OutputParams;
#if defined(HDMI_HDCP_ON)
 STVOUT_HDCPParams_t     VOUT_HDCPParams;
#endif
 STVTG_TimingMode_t      VTG_TimingMode;
 STVTG_ModeParams_t      VTG_ModeParams;
 ST_ErrorCode_t          ErrCode=ST_NO_ERROR;

 /* Check if device id exists */
 /* ========================= */
 if (DeviceId>=HDMI_MAX_NUMBER)
  {
   printf("HDMI_EnableOutput(?):**ERROR** !!! Invalid HDMI device identifier !!!\n");
   return(ST_ERROR_BAD_PARAMETER);
  }
 if (HDMI_Context[DeviceId].HDMI_Handle==0)
  {
   printf("HDMI_EnableOutput(?):**ERROR** !!! HDMI device identifier not initialized !!!\n");
   return(ST_ERROR_BAD_PARAMETER);
  }

 /* Get the current output parameters */
 /* ================================= */
 ErrCode=STVOUT_GetOutputParams(HDMI_Context[DeviceId].VOUT_Handle,&VOUT_OutputParams);
 if (ErrCode!=ST_NO_ERROR)
  {
   printf("HDMI_EnableOutput(%d):**ERROR** !!! Unable to get output params !!!\n",DeviceId);
   return(ST_ERROR_BAD_PARAMETER);
  }

 /* Check if the screen is DVI only or not */
 /* ====================================== */
#if defined(HDMI_FORCE_DVI)
 VOUT_OutputParams.HDMI.ForceDVI = TRUE;
#else
  {
 	BOOL IsDVIOnly;
    ErrCode = HDMI_IsScreenDVIOnly(DeviceId,&IsDVIOnly);
    if (ErrCode!=ST_NO_ERROR)
   {
        printf("HDMI_EnableOutput(%d):**ERROR** !!! Unable to detect if this is a DVI or HDMI screen !!!\n",DeviceId);
        return(ST_ERROR_BAD_PARAMETER);
   }
   VOUT_OutputParams.HDMI.ForceDVI=IsDVIOnly;
  }
#endif

 /* Set the output params */
 /* ===================== */
 ErrCode=STVOUT_SetOutputParams(HDMI_Context[DeviceId].VOUT_Handle,&VOUT_OutputParams);
 if (ErrCode!=ST_NO_ERROR)
 {
   printf("HDMI_EnableOutput(%d):**ERROR** !!! Unable to set output params !!!\n",DeviceId);
   return(ST_ERROR_BAD_PARAMETER);
 }

#if defined(HDMI_HDCP_ON)
 /* Check if we need to setup the keys when HDCP is on */
 /* ================================================== */
 if (VOUT_OutputParams.HDMI.IsHDCPEnable==TRUE)
 {
   memset(&VOUT_HDCPParams,0,sizeof(STVOUT_HDCPParams_t));
   VOUT_HDCPParams.IV_0        = (U32)HDMI_Context[DeviceId].HDMI_IV0_Key;
   VOUT_HDCPParams.IV_1        = (U32)HDMI_Context[DeviceId].HDMI_IV1_Key;
   VOUT_HDCPParams.KSV_0       = (U32)HDMI_Context[DeviceId].HDMI_KV0_Key;
   VOUT_HDCPParams.KSV_1       = (U32)HDMI_Context[DeviceId].HDMI_KV1_Key;
   VOUT_HDCPParams.IRate       = 0;
   VOUT_HDCPParams.IsACEnabled = FALSE;
   memcpy(&VOUT_HDCPParams.DeviceKeys,HDMI_Context[DeviceId].HDMI_DeviceKeys,sizeof(HDMI_Context[DeviceId].HDMI_DeviceKeys));
   ErrCode=STVOUT_SetHDCPParams(HDMI_Context[DeviceId].VOUT_Handle,&VOUT_HDCPParams);
   if (ErrCode!=ST_NO_ERROR)
   {
     printf("HDMI_EnableOutput(%d):**ERROR** !!! Unable to setup HDCP keys !!!\n",DeviceId);
     return(ST_ERROR_BAD_PARAMETER);
   }
 }
#endif

 /* Enable the output */
 /* ================= */
 ErrCode=STVOUT_Enable(HDMI_Context[DeviceId].VOUT_Handle);
 if (ErrCode!=ST_NO_ERROR)
 {
   printf("HDMI_EnableOutput(%d):**ERROR** !!! Unable to enable the HDMI interface !!!\n",DeviceId);
   return(ST_ERROR_BAD_PARAMETER);
 }
 HDMI_Context[DeviceId].HDMI_IsEnabled=TRUE;

 /* Get the screen params */
 /* ===================== */
 ErrCode=STVTG_GetMode(HDMI_Context[DeviceId].VTG_Handle,&VTG_TimingMode,&VTG_ModeParams);
 if (ErrCode!=ST_NO_ERROR)
 {
   printf("HDMI_EnableOutput(%d):**ERROR** !!! Unable to get the VTG parameters !!!\n",DeviceId);
   return(ST_ERROR_BAD_PARAMETER);
 }
 /* Set the HDMI output window */
 /* ========================== */
 memset(&HDMI_OutputWindows,0,sizeof(STHDMI_OutputWindows_t));
 HDMI_OutputWindows.OutputWinX      = 0;
 HDMI_OutputWindows.OutputWinY      = 0;
 HDMI_OutputWindows.OutputWinWidth  = VTG_ModeParams.ActiveAreaWidth;
 HDMI_OutputWindows.OutputWinHeight = VTG_ModeParams.ActiveAreaHeight;
 ErrCode=STHDMI_SetOutputWindows(HDMI_Context[DeviceId].HDMI_Handle,&HDMI_OutputWindows);
 if (ErrCode!=ST_NO_ERROR)
 {
   printf("HDMI_EnableOutput(%d):**ERROR** !!! Unable to set the HDMI I/O window size !!!\n",DeviceId);
   return(ST_ERROR_BAD_PARAMETER);
 }

 /* Send the AVI info frame */
 /* ======================= */
 memset(&HDMI_AVIInfoFrame,0,sizeof(STHDMI_AVIInfoFrame_t));
 HDMI_AVIInfoFrame.AVI861B.FrameType                  = STHDMI_INFOFRAME_TYPE_AVI;
 HDMI_AVIInfoFrame.AVI861B.FrameVersion               = 2;
 HDMI_AVIInfoFrame.AVI861B.FrameLength                = 13;
 HDMI_AVIInfoFrame.AVI861B.HasActiveFormatInformation = TRUE;
 HDMI_AVIInfoFrame.AVI861B.OutputType                 = HDMI_Context[DeviceId].HDMI_OutputType;
 HDMI_AVIInfoFrame.AVI861B.ScanInfo                   = HDMI_Context[DeviceId].HDMI_ScanInfo;
 HDMI_AVIInfoFrame.AVI861B.Colorimetry                = HDMI_Context[DeviceId].HDMI_Colorimetry;
 HDMI_AVIInfoFrame.AVI861B.AspectRatio                = HDMI_Context[DeviceId].HDMI_AspectRatio;
 HDMI_AVIInfoFrame.AVI861B.ActiveAspectRatio          = HDMI_Context[DeviceId].HDMI_ActiveAspectRatio;
 HDMI_AVIInfoFrame.AVI861B.PictureScaling             = HDMI_Context[DeviceId].HDMI_PictureScaling;
 ErrCode=STHDMI_FillAVIInfoFrame(HDMI_Context[DeviceId].HDMI_Handle,&HDMI_AVIInfoFrame);
 if (ErrCode!=ST_NO_ERROR)
 {
   printf("HDMI_EnableOutput(%d):**ERROR** !!! Unable to send the AVI Info frame !!!\n",DeviceId);
   return(ST_ERROR_BAD_PARAMETER);
 }

 /* Send the audio info frame */
 /* ========================= */
 memset(&HDMI_AUDIOInfoFrame,0,sizeof(STHDMI_AUDIOInfoFrame_t));
 HDMI_AUDIOInfoFrame.FrameType         = STHDMI_INFOFRAME_TYPE_AUDIO;
 HDMI_AUDIOInfoFrame.FrameVersion      = 2;
 HDMI_AUDIOInfoFrame.CodingType        = STAUD_STREAM_CONTENT_NULL;
 HDMI_AUDIOInfoFrame.SampleSize        = 0xFF;
 HDMI_AUDIOInfoFrame.DownmixInhibit    = FALSE;
 ErrCode=STHDMI_FillAudioInfoFrame(HDMI_Context[DeviceId].HDMI_Handle,&HDMI_AUDIOInfoFrame);
 if (ErrCode!=ST_NO_ERROR)
 {
   printf("HDMI_EnableOutput(%d):**ERROR** !!! Unable to send the AUDIO Info frame !!!\n",DeviceId);
   return(ST_ERROR_BAD_PARAMETER);
 }

 /* Return no errors */
 /* ================ */
 return(ST_NO_ERROR);
}
/* ========================================================================
   Name:        HDMI_DisableOutput
   Description: Disable the HDMI output
   ======================================================================== */
ST_ErrorCode_t HDMI_DisableOutput(U32 DeviceId)
{
 ST_ErrorCode_t ErrCode=ST_NO_ERROR;

 /* Check if device id exists */
 /* ========================= */
 if (DeviceId>=HDMI_MAX_NUMBER)
 {
   printf("HDMI_DisableOutput(?):**ERROR** !!! Invalid HDMI device identifier !!!\n");
   return(ST_ERROR_BAD_PARAMETER);
 }
 if (HDMI_Context[DeviceId].HDMI_Handle==0)
 {
   printf("HDMI_DisableOutput(?):**ERROR** !!! HDMI device identifier not initialized !!!\n");
   return(ST_ERROR_BAD_PARAMETER);
 }
 /* Disable output */
 /* ============== */
 ErrCode=STVOUT_Disable(HDMI_Context[DeviceId].VOUT_Handle);
 if (ErrCode!=ST_NO_ERROR)
 {
   printf("HDMI_DisableOutput(%d):**ERROR** !!! Unable to disable the HDMI interface !!!\n",DeviceId);
   return(ST_ERROR_BAD_PARAMETER);
 }
 HDMI_Context[DeviceId].HDMI_IsEnabled=FALSE;
 /* Return no errors */
 /* ================ */
 return(ST_NO_ERROR);
}
/* ========================================================================
   Name:        HDMI_IsScreenDVIOnly
   Description: This function returns true if the screen is DVI only
   ======================================================================== */
ST_ErrorCode_t HDMI_IsScreenDVIOnly(U32 DeviceId,BOOL *DVIOnlyNotHDMI)
{
 ST_ErrorCode_t     ErrCode=ST_NO_ERROR;
 STVOUT_State_t     VOUT_State;
 STHDMI_EDIDSink_t  HDMI_EDIDSink;
 BOOL               SinkIsDVIOnly;
#ifdef STHDMI_EDID_DETAILS
 STVOUT_TargetInformation_t TargetInfo;
 int i=0, l=0, j=0, k=0;
#endif

 /* Check if device id exists */
 /* ------------------------- */
 if (DeviceId>=HDMI_MAX_NUMBER)
 {
   printf("HDMI_IsScreenDVIOnly(?):**ERROR** !!! Invalid HDMI device identifier !!!\n");
   return(ST_ERROR_BAD_PARAMETER);
 }
 if (HDMI_Context[DeviceId].HDMI_Handle==0)
 {
   printf("HDMI_IsScreenDVIOnly(?):**ERROR** !!! HDMI device identifier not initialized !!!\n");
   return(ST_ERROR_BAD_PARAMETER);
 }
 /* Try to get HDMI state */
 /* --------------------- */
 ErrCode=STVOUT_GetState(HDMI_Context[DeviceId].VOUT_Handle , &VOUT_State);
 if (ErrCode != ST_NO_ERROR)
 {
   printf("HDMI_IsScreenDVIOnly():**ERROR** !!! Unable to get state of the HDMI !!!\n");
   return(ST_ERROR_BAD_PARAMETER);
 }
 if (VOUT_State == STVOUT_NO_RECEIVER)
 {
   printf("HDMI_IsScreenDVIOnly():**ERROR** !!! There is not screen on HDMI !!!\n");
   return(ST_ERROR_BAD_PARAMETER);
 }
 /* Get EDID of the sink */
 /* -------------------- */
 memset(&HDMI_EDIDSink,0,sizeof(STHDMI_EDIDSink_t));

/* AllocData.Size = sizeof(STHDMI_EDIDBlockExtension_t) ;
 AllocData.Alignment = 16 ;
 STLAYER_AllocData( 0x123, &LayAllocData, &HDMI_EDIDSink.EDIDExtension_p );*/

/* HDMI_EDIDSink.EDIDExtension_p = (STHDMI_EDIDBlockExtension_t*) memory_allocate (HDMI_InitParams.CPUPartition_p, \
                                        sizeof(STHDMI_EDIDBlockExtension_t)); */
/* memset(HDMI_EDIDSink.EDIDExtension_p,0,sizeof(STHDMI_EDIDBlockExtension_t)); */

 ErrCode = STHDMI_FillSinkEDID(HDMI_Context[DeviceId].HDMI_Handle, &HDMI_EDIDSink);
 if (ErrCode!=ST_NO_ERROR)
 {
   printf("HDMI_IsScreenDVIOnly():**ERROR** !!! Unable to get sink informations !!!\n");
   return(ST_ERROR_BAD_PARAMETER);
 }

#ifdef STHDMI_EDID_DETAILS
 /* EDID raw details */
 /*------------------*/
 ErrCode = STVOUT_GetTargetInformation(HDMI_Context[DeviceId].VOUT_Handle, &TargetInfo);
 if (ErrCode!=ST_NO_ERROR)
 {
   printf("HDMI_IsScreenDVIOnly():**ERROR** !!! Unable to get sink raw informations !!!\n");
   return(ST_ERROR_BAD_PARAMETER);
 }

 for ( l=0; l<(256/32); l++)
 {
   for ( j=0; j<32; j++)
   {
        printf(" 0x%2x ", *(TargetInfo.SinkInfo.Buffer_p + (k++)) );
   }
   printf("\n");
 }
 printf("\n");
 #endif

 /* Analyse EDID */
 /* ------------ */
#ifdef STHDMI_CEC
 HDMI_Context[0].HDMI_CECStruct.PhysicalAddress = HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.DataBlock.VendorData.PhysicalAddress;
#endif
 SinkIsDVIOnly = TRUE;
 if (HDMI_EDIDSink.EDIDExtension_p != NULL)
  {
   if ((HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.Tag == 0x02)&&
        (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.RevisionNumber == 3))
    {
      if ((HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.DataBlock.VendorData.RegistrationId[0] == 0x03)&&
         (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.DataBlock.VendorData.RegistrationId[1] == 0x0C)&&
         (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.DataBlock.VendorData.RegistrationId[2] == 0x00))
      {
        SinkIsDVIOnly = FALSE;
      }
    }
  }
#ifdef STHDMI_EDID_DETAILS
  /*if Screen is HDMI */
    if(SinkIsDVIOnly==FALSE)
    {
        for (i=0;i<8 ;i++)
        {
            printf("%x",HDMI_EDIDSink.EDIDBasic.EDIDHeader[i]);
        }
            printf("\n");
            printf("the ID Manufacture Name is:_____________________ ");
        for (i=0;i<3 ;i++)
        {
            printf("%c ",HDMI_EDIDSink.EDIDBasic.EDIDProdDescription.IDManufactureName[i]);
        }
        printf("\n");
        printf("the ID Product Code is_____________________ %x\n ",HDMI_EDIDSink.EDIDBasic.EDIDProdDescription.IDProductCode);
        printf("\n");
        printf("the ID Serial Number is___________________ %d\n ",HDMI_EDIDSink.EDIDBasic.EDIDProdDescription.IDSerialNumber);
        printf("\n") ;
        printf("the Week of manufacter is________________________ %d\n ",HDMI_EDIDSink.EDIDBasic.EDIDProdDescription.WeekOfManufacture);
        printf("\n");
        printf("the Year of manufacter is_______________________ %d\n ",HDMI_EDIDSink.EDIDBasic.EDIDProdDescription.YearOfManufacture);
        printf("\n");
        printf("the EDID verion Number is_______________________ %d\n ",HDMI_EDIDSink.EDIDBasic.EDIDInfos.Version);
        printf("\n");
        printf("the EDID revision Number is______________________ %d\n ",HDMI_EDIDSink.EDIDBasic.EDIDInfos.Revision);
        printf("\n");
        printf("the Video Input Definition **SignalLevelMax** is______________ %c\n ",HDMI_EDIDSink.EDIDBasic.EDIDDisplayParams.VideoInput.SignalLevelMax);
        printf("\n");
        printf("the Video Input Definition **SignalLevelMin** is_________________ %c\n ",HDMI_EDIDSink.EDIDBasic.EDIDDisplayParams.VideoInput.SignalLevelMin);
        printf("\n");
        printf("the Maximum Horizental Image size is_________________ %d\n ",HDMI_EDIDSink.EDIDBasic.EDIDDisplayParams.MaxHorImageSize);
        printf("\n");
        printf("the Maximum veretical image size is______________________ %d\n ",HDMI_EDIDSink.EDIDBasic.EDIDDisplayParams.MaxVerImageSize);
        printf("\n");
        printf("the display gamma (multiplied by 100) is____________________ %u\n ",HDMI_EDIDSink.EDIDBasic.EDIDDisplayParams.DisplayGamma);
        printf("\n");
        /*printf("the Feature Support is_______________________ %c\n ",HDMI_EDIDSink.EDIDBasic.EDIDDisplayParams.FeatureSupport); */
        printf("\n");
        printf(" the Color Characteristic are:\n");
        /*printf("the RG low Bits is________________________ %x\n ",RGLowBits);*/
        /*printf("the B low Bits is_________________ %x\n ",RGLowBits);*/
        printf("\n");
        printf("the chroma info, Red X *1000* is_____________________ %u\n ",HDMI_EDIDSink.EDIDBasic.EDIDColorParams.Red_x);
        printf("\n");
        printf("the Chroma info, Red Y *1000 * is_____________________ %u\n ",HDMI_EDIDSink.EDIDBasic.EDIDColorParams.Red_y);
        printf("\n");
        printf("the Chroma info, Green X *1000 * is_____________________ %u\n ",HDMI_EDIDSink.EDIDBasic.EDIDColorParams.Green_x);
        printf("\n");
        printf("the Chroma info, Green Y *1000 * is___________________ %u\n ",HDMI_EDIDSink.EDIDBasic.EDIDColorParams.Green_y);
        printf("\n");
        printf("the Chroma info, Blue X *1000 * is___________________ %u\n ",HDMI_EDIDSink.EDIDBasic.EDIDColorParams.Blue_x);
        printf("\n");
        printf("the Chroma info, Blue Y *1000 * is_________________ %u\n ",HDMI_EDIDSink.EDIDBasic.EDIDColorParams.Blue_y);
        printf("\n");
        printf("the Chroma info, White X *1000 * is_____________________ %u\n ",HDMI_EDIDSink.EDIDBasic.EDIDColorParams.White_x);
        printf("\n");
        printf("the Chroma info, White Y *1000 * is %u\n ",HDMI_EDIDSink.EDIDBasic.EDIDColorParams.White_y);
        printf("\n");
        printf("the Established TimingI is %c\n ",HDMI_EDIDSink.EDIDBasic.EDIDEstablishedTiming.Timing1);
        printf("\n");
        printf("the Established TimingII is %c\n ",HDMI_EDIDSink.EDIDBasic.EDIDEstablishedTiming.Timing2);
        printf("\n");
        printf("the Manufacturer Timing is %d\n ",HDMI_EDIDSink.EDIDBasic.EDIDEstablishedTiming.IsManTimingSupported);
        printf("\n");
        printf("Standard Timing Identification: \n");
        for (i=0; i<8 ;i++)
        {
           if (!HDMI_EDIDSink.EDIDBasic.EDIDStdTiming[i].IsStandardTimingUsed)
               printf("standard Timing ID: not used");
           else
           {
               printf("standard Timing ID: used");
               printf("HorizontalActive is: %d",HDMI_EDIDSink.EDIDBasic.EDIDStdTiming[i].HorizontalActive);
               printf("VerticalActive is: %d",HDMI_EDIDSink.EDIDBasic.EDIDStdTiming[i].VerticalActive);
               printf("RefreshRate is: %d",HDMI_EDIDSink.EDIDBasic.EDIDStdTiming[i].RefreshRate);
           }
        }
        for (i=0; i<2 ;i++)
        {
               printf("Detailed Timing/Decsriptor Block %d is:\n\n",i);
               printf("%dX%d\n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].HImageSize,HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].VImageSize);
               printf("Pixel Clock: %d MHz\n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].PixelClock);
               printf("Horizental Image Size: %d mm            ",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].HImageSize);
               printf("Vertical Image Size: %d mm \n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].VImageSize);
           if (HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].IsInterlaced)
               printf("Refreshed Mode: interlaced");
           else
               printf("Refreshed Mode: Non-interlaced");
            switch(HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].Stereo)
               {
                   case STHDMI_EDID_NO_STEREO                     : printf("Normal Display: No Stereo\n");
                   case STHDMI_EDID_STEREO_RIGHT_IMAGE            : printf("Normal Display: Field Sequentiel stereo, right image when stereo sync=1\n");
                   case STHDMI_EDID_STEREO_LEFT_IMAGE             : printf("Normal Display: Field Sequentiel stereo, left image when stereo sync =1\n");
                   case STHDMI_EDID_STEREO_2WAY_INTERLEAVED_RIGHT : printf("Normal Display: 2-way interleaved stereo, right image on even lines\n");
                   case STHDMI_EDID_STEREO_2WAY_INTERLEAVED_LEFT  : printf("Normal Display: 2-way interleaved stereo, left image on even lines\n");
                   case STHDMI_EDID_STEREO_4WAY_INTERLEAVED       : printf("Normal Display: 4-way interleaved stereo\n");
                   case STHDMI_EDID_STEREO_SBS_INTERLEAVED        : printf("Normal Display: Side-by-side interleaved stereo\n");
                }
                   printf(" Horizontal:\n\n ");
                   printf("Active Count: %d              \n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].HorActivePixel);
                   printf("Blanking Count Count: %d              \n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].HorBlankingPixel);
                   printf("Sync Offset: %d              \n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].HSyncOffset);
                   printf("Sync Pulse Width: %d              \n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].HSyncWidth);
                   printf("Border: %d              \n ",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].HBorder);
                   printf("Frequency: deduced value\n\n");
                   printf(" Vertical:\n\n ");
                   printf("Active Count: %d              \n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].VerActiveLine);
                   printf("Blanking Count: %d              \n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].VerBlankingLine);
                   printf("Sync Offset: %d              \n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].VSyncOffset);
                   printf("Sync Pulse Width: %d              \n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].VSyncWidth);
                   printf("Border: %d              \n",HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].VBorder);
                   printf("Frequency: deduced value\n\n");
              switch(HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].SyncDescription)
               {
                   case STHDMI_EDID_SYNC_ANALOG_COMPOSITE                     : printf(" Analog composite, ");
                   case STHDMI_EDID_SYNC_BIP_ANALOG_COMPOSITE                 : printf("Bipolar Analog Composite, ");
                   case STHDMI_EDID_SYNC_DIGITAL_COMPOSITE                    : printf("Digital Composite, ");
                   case STHDMI_EDID_SYNC_DIGITAL_SEPARATE                     : printf("Digital Separate, ");
               }

            if (HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].IsHPolarityPositive)
                printf("Horizontal Polarity (+) \n");
            else
                printf("Horizontal Polarity (-) \n");

            if (HDMI_EDIDSink.EDIDBasic.EDIDDetailedDesc[i].IsHPolarityPositive)
                printf("Vertical Polarity (+) \n");
            else
                printf("Vertical Polarity (-) \n");
                printf("___________________________________________________________________________________________________");
      }
            printf("Block No: (%d) Extension EDID Block(s)\n",HDMI_EDIDSink.EDIDBasic.EDIDExtensionFlag);

            printf("Checksum: (%d) Extension EDID Block(s)\n",HDMI_EDIDSink.EDIDBasic.EDIDChecksum);
            printf("CheckSum OK\n\n");
            printf("___________________________________________________________________________\n");
            printf("___________________________________________________________________________\n");
            printf("ST Microelectronics Image\n");
            printf("EDID Timing Extension Version 3\n");
            printf("___________________________________________________________________________\n");
            printf("___________________________________________________________________________\n");
            printf("Extended Block Type: CEA 861B\n");

           printf("Detailed timing blocks start at byte: (%x)\n",HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.Offset);
           printf("Native Format: (%x)\n",HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.FormatDescription.NativeFormatNum);
            printf("DTV");

      if  (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.FormatDescription.IsBasicAudioSupported)
                     printf("(Basic Audio) ");

      if  (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.FormatDescription.IsUnderscanSink)
                     printf("(Underscaned)\n");
      else
                     printf("(Overscaned)\n");
      if  (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.FormatDescription.IsYCbCr444)
                     printf("YCbCr(4:4:4)\n");

      if  (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.FormatDescription.IsYCbCr422)
                     printf("YCbCr(4:2:2)\n\n");
                     printf("Video Short Block Description:\n");
                     printf("Audio Short Block Description:\n");

 }
#endif /* STHDMI_EDID_DETAILS */

 /* Free EDID */
 /* --------- */
 if (HDMI_EDIDSink.EDIDExtension_p!=NULL)
  {
   ErrCode = STHDMI_FreeEDIDMemory(HDMI_Context[DeviceId].HDMI_Handle);
   if (ErrCode != ST_NO_ERROR)
    {
     printf("HDMI_IsScreenDVIOnly():**ERROR** !!! Unable to deallocate EDID informations !!!\n");
     return(ST_ERROR_BAD_PARAMETER);
    }
  }
 /* Return no errors */
 /* ---------------- */
 *DVIOnlyNotHDMI = SinkIsDVIOnly;
 return(ST_NO_ERROR);
}

/*#########################################################################
 *                                 MAIN
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : os20_main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void os20_main(void *ptr)
{
    UNUSED_PARAMETER(ptr);
    STAPIGAT_Init();
    STAPIGAT_Term();
} /* end os20_main */


/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);
#ifdef ST_OS21
    printf ("\nBOOT ...\n");
    setbuf(stdout, NULL);
#endif
        os20_main(NULL);


    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}
/* end of vout_test.c */

