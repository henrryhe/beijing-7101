/*******************************************************************************

File name : vtg_test.c

Description : STVTG tests main file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
17 Apr 2002        Created                                          HSdLM
11 Oct 2002        Add support for STi5517                          HSdLM
16 Apr 2003        Add support for STi5528 (VFE2).                  HSdLM
06 Sep 2004        Add support for ST40/OS21                        MH
************************************************************************/

/* Private Definitions (internal use only) ---------------------------------- */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef ST_OS20
    #include "task.h"
#endif

#ifdef ST_OS21
#ifndef ST_OSWINCE
    #include "os21/task.h"
#endif
#endif

/* STAPI includes */
#include "stdevice.h"
#include "stddefs.h"

/* STAPI libraries */
#include "stos.h"
#include "stcommon.h"
#include "testtool.h"

#include "stsys.h"

#include "sttbx.h"

#include "stgxobj.h"
#include "stevt.h"
#include "stvtg.h"
/* Tests dependencies  */
#include "api_cmd.h"
#include "denc_cmd.h"
#include "vtg_cmd.h"
#include "clevt.h"
#include "startup.h"
#ifdef STVTG_USE_CLKRV
#include "clclkrv.h"
#endif

/* External global variables ------------------------------------------------ */
/* Private Types ------------------------------------------------------------ */
typedef struct
{
    BOOL Top;
    BOOL Bottom;
    BOOL FirstVsync;
    U32  VsyncDelay;
    U32  DelayRef;
} VSYNC_Param_t;

typedef enum
{
#ifndef ST_OSWINCE
/* NO_ERROR is already defined by winerror.h*/
  NO_ERROR=0,
#endif  /*ST_OSWINCE*/
  TWO_BOT_VSYNC=1,
  TWO_TOP_VSYNC=2,
  VSYNC_DELAY=4
} Vsync_Error_t;

 typedef struct
{
    U32 Time;
    U32 Time1;
    U32 Time2;
    U32 TimeRef;
    BOOL TimeRef_OK;
    BOOL Time1_OK;
    BOOL Time2_OK;
} Time_t;

typedef struct
{
    U32 Top;
    U32 Bottom;
    U32 TopMissed;
    U32 BottomMissed;
    BOOL ClockStored;
    osclock_t ClockFirst;
    osclock_t ClockLast;
    VSYNC_Param_t Value;
    Vsync_Error_t Error;
    Time_t VsyncTime;
    STGXOBJ_ScanType_t ScanType_1;
} VTG_Vsync_t;


/* Private Constants -------------------------------------------------------- */

#if defined (ST_5508) || defined (ST_5518) || defined (ST_5510) || defined (ST_5512) || \
    defined (ST_5516) || ((defined (ST_5514) || defined (ST_5517)) && !defined(ST_7020))
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_DENC
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      DENC_BASE_ADDRESS
#elif defined (ST_7015)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VTG_CELL_7015
#define VTG_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define VTGCMD_BASE_ADDRESS      ST7015_VTG1_OFFSET
#elif defined (ST_7020)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VTG_CELL_7020
#define VTG_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define VTGCMD_BASE_ADDRESS      ST7020_VTG1_OFFSET
#elif defined (ST_5528)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VFE2
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5528_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5528_VTG_VSYNC_INTERRUPT
#elif defined (ST_5100)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VFE2
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5100_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5100_VTG_VSYNC_INTERRUPT
#elif defined (ST_5105)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VFE2
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5105_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5105_VTG_VSYNC_INTERRUPT
#elif defined (ST_5188)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VFE2
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5188_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5188_VTG_VSYNC_INTERRUPT
#elif defined (ST_5107)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VFE2
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5107_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5107_VTG_VSYNC_INTERRUPT
#elif defined (ST_5162)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VFE2
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5162_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5162_VTG_VSYNC_INTERRUPT
#elif defined (ST_5301)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VFE2
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5301_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5301_VTG_VSYNC_INTERRUPT
#elif defined (ST_5525)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VFE2
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5525_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5525_VTG_VSYNC_INTERRUPT
#elif defined (ST_GX1)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VFE
#define VTG_DEVICE_BASE_ADDRESS  0                      /* waiting to be defined in vob chip */
#define VTGCMD_BASE_ADDRESS      (S32)STGX1_VTG_BASE_ADDRESS
#elif defined (ST_7710)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VTG_CELL_7710
#define VTG1_OFFSET              ST7710_VTG1_BASE_ADDRESS
#define VTG2_OFFSET              ST7710_VTG2_BASE_ADDRESS
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST7710_VTG1_BASE_ADDRESS
#elif defined (ST_7100)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VTG_CELL_7100
#define VTG1_OFFSET              ST7100_VTG1_BASE_ADDRESS
#define VTG2_OFFSET              ST7100_VTG2_BASE_ADDRESS
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST7100_VTG1_BASE_ADDRESS
#elif defined (ST_7109)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VTG_CELL_7100
#define VTG1_OFFSET              ST7109_VTG1_BASE_ADDRESS
#define VTG2_OFFSET              ST7109_VTG2_BASE_ADDRESS
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST7109_VTG1_BASE_ADDRESS
#elif defined (ST_7200)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VTG_CELL_7200
#define VTG1_BASE_ADDRESS        ST7200_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS        ST7200_VTG2_BASE_ADDRESS
#define VTG3_BASE_ADDRESS        ST7200_VTG3_BASE_ADDRESS
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST7200_VTG1_BASE_ADDRESS
#elif defined (ST_7111)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VTG_CELL_7200
#define VTG1_BASE_ADDRESS        ST7111_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS        ST7111_VTG2_BASE_ADDRESS
#define VTG3_BASE_ADDRESS        ST7111_VTG3_BASE_ADDRESS
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST7111_VTG1_BASE_ADDRESS
#elif defined (ST_7105)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VTG_CELL_7200
#define VTG1_BASE_ADDRESS        ST7105_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS        ST7105_VTG2_BASE_ADDRESS
#define VTG3_BASE_ADDRESS        ST7105_VTG3_BASE_ADDRESS
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST7105_VTG1_BASE_ADDRESS
#else
#error Not defined processor
#endif

#define STACK_SIZE             4096 /* STACKSIZE must oversize the estimated stack usage of the driver */
#define STRING_DEVICE_LENGTH   80
#define VSYNC_COUNT_TIME       2 /* seconds. WARNING! increasing this value can lead to overflow! */


/* Private Variables (static)------------------------------------------------ */
static STVTG_Handle_t VTGHandle;

static char Test_Msg[200];
#if defined (ST_OS20)
#ifndef ST_5528
static STVTG_InitParams_t InitVTGParam;
static STVTG_OpenParams_t OpenVTGParam;
static STVTG_Capability_t VTGCapability;
static STVTG_TermParams_t TermVTGParam;
static STVTG_TimingMode_t StackModeVtg;
static STVTG_ModeParams_t StackModeParams;
static STVTG_SlaveModeParams_t SlaveModeParams_s;
static STVTG_OptionalConfiguration_t StackOptionalParams_s;
#endif
#endif /* ST_OS20 */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define TEST_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(x)){ \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

/* Private Function prototypes ---------------------------------------------- */
static void VTG_ReceiveEvtVSync(STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void *EventData,
                                void *SubscriberData_p);
BOOL Test_CmdStart(void);
void os20_main(void *ptr);

#ifdef ST_OS20
static void ReportError(int Error, char* FunctionName);
static void test_overhead(void *dummy);
static void test_typical(void *dummy);
void metrics_Stack_Test(void);
#endif

/*#######################################################################*/
/*########################## VTG TEST FUNCTIONS #########################*/
/*#######################################################################*/

/* Functions ---------------------------------------------------------------- */
#ifdef ST_OS20
static void ReportError(int Error, char* FunctionName)
{
    if ((Error) != ST_NO_ERROR)
    {
        printf( "ERROR: %s returned 0x%0x\n", FunctionName, Error);
    }
}

/*******************************************************************************
Name        : test_typical
Description : calculates the stack usage made by the driver for its typical
              conditions of use
Parameters  : None
Assumptions : STDENC must be initialized before test metrics function call
Limitations : Make sure to not define local variables within the function
              but use module static globals instead in order to not pollute the
              stack usage calculation.
Returns     : None
*******************************************************************************/
void test_typical(void *dummy)
{
    char Msg_string[100];
    ST_ErrorCode_t Err;
    Err = ST_NO_ERROR;

    UNUSED_PARAMETER(dummy);
    InitVTGParam.DeviceType = VTG_DEVICE_TYPE;
    if(InitVTGParam.DeviceType == STVTG_DEVICE_TYPE_DENC)
    {
        /* STDENC must be initialized before on testtool command line */
        strcpy(InitVTGParam.Target.DencName, STDENC_DEVICE_NAME);
    }
    else
    {
#if defined (ST_7015)
        InitVTGParam.Target.VtgCell.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        InitVTGParam.Target.VtgCell.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (InitVTGParam.Target.VtgCell.BaseAddress_p == (void*)ST7015_VTG1_OFFSET)
        {
            InitVTGParam.Target.VtgCell.InterruptEvent = ST7015_VTG1_INT;
        }
        else if (InitVTGParam.Target.VtgCell.BaseAddress_p == (void*)ST7015_VTG2_OFFSET)
        {
            InitVTGParam.Target.VtgCell.InterruptEvent = ST7015_VTG2_INT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        strcpy(InitVTGParam.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#elif defined (ST_7020)
        InitVTGParam.Target.VtgCell.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        InitVTGParam.Target.VtgCell.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (InitVTGParam.Target.VtgCell.BaseAddress_p == (void*)ST7020_VTG1_OFFSET)
        {
            InitVTGParam.Target.VtgCell.InterruptEvent = ST7020_VTG1_INT;
        }
        else if (InitVTGParam.Target.VtgCell.BaseAddress_p == (void*)ST7020_VTG2_OFFSET)
        {
            InitVTGParam.Target.VtgCell.InterruptEvent = ST7020_VTG2_INT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        strcpy(InitVTGParam.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#elif defined (ST_7710)
        InitVTGParam.Target.VtgCell3.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        InitVTGParam.Target.VtgCell3.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (InitVTGParam.Target.VtgCell3.BaseAddress_p == (void*)VTG1_OFFSET)
        {
#ifdef WA_GNBvd35956
            /* Install DVP Interrupt number instead of VOS1 one.*/
            InitVTGParam.Target.VtgCell3.InterruptNumber = ST7710_DVP_INTERRUPT;
#else

            InitVTGParam.Target.VtgCell3.InterruptNumber = ST7710_VOS_0_INTERRUPT;
#endif
        }
        else if (InitVTGParam.Target.VtgCell3.BaseAddress_p == (void*)VTG2_OFFSET)
        {
            InitVTGParam.Target.VtgCell3.InterruptNumber = ST7710_VOS_1_INTERRUPT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        InitVTGParam.Target.VtgCell3.InterruptLevel      = 4;
#elif defined (ST_7100)
        InitVTGParam.Target.VtgCell3.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        InitVTGParam.Target.VtgCell3.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (InitVTGParam.Target.VtgCell3.BaseAddress_p == (void*)VTG1_OFFSET)
        {
            InitVTGParam.Target.VtgCell3.InterruptNumber = ST7100_VTG_0_INTERRUPT;
        }
        else if (InitVTGParam.Target.VtgCell3.BaseAddress_p == (void*)VTG2_OFFSET)
        {
            InitVTGParam.Target.VtgCell3.InterruptNumber = ST7100_VTG_1_INTERRUPT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        InitVTGParam.Target.VtgCell3.InterruptLevel      = 4;
#elif defined (ST_7109)
        InitVTGParam.Target.VtgCell3.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        InitVTGParam.Target.VtgCell3.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (InitVTGParam.Target.VtgCell3.BaseAddress_p == (void*)VTG1_OFFSET)
        {
            InitVTGParam.Target.VtgCell3.InterruptNumber = ST7109_VTG_0_INTERRUPT;
        }
        else if (InitVTGParam.Target.VtgCell3.BaseAddress_p == (void*)VTG2_OFFSET)
        {
            InitVTGParam.Target.VtgCell3.InterruptNumber = ST7109_VTG_1_INTERRUPT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        InitVTGParam.Target.VtgCell3.InterruptLevel      = 4;
#elif defined (ST_7200)||defined(ST_7111)||defined(ST_7105)
        InitVTGParam.Target.VtgCell3.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        InitVTGParam.Target.VtgCell3.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (InitVTGParam.Target.VtgCell3.BaseAddress_p == (void*)VTG1_BASE_ADDRESS)
        {
			#if defined(ST_7200)
            InitVTGParam.Target.VtgCell3.InterruptNumber =  ST7200_VTG_MAIN0_INTERRUPT;
			#elif defined(ST_7111)
			InitVTGParam.Target.VtgCell3.InterruptNumber =  ST7111_VTG_MAIN0_INTERRUPT;
			#elif defined(ST_7105)
			InitVTGParam.Target.VtgCell3.InterruptNumber =  ST7105_VTG_MAIN0_INTERRUPT;
			#endif
        }
        else if (InitVTGParam.Target.VtgCell3.BaseAddress_p == (void*)VTG2_BASE_ADDRESS)
        {
			#if defined(ST_7200)
            InitVTGParam.Target.VtgCell3.InterruptNumber = ST7200_VTG_AUX0_INTERRUPT;
			#elif defined(ST_7111)
			InitVTGParam.Target.VtgCell3.InterruptNumber = ST7111_VTG_AUX0_INTERRUPT;
			#elif defined(ST_7105)
			InitVTGParam.Target.VtgCell3.InterruptNumber = ST7105_VTG_AUX0_INTERRUPT;
			#endif
        }
        else if (InitVTGParam.Target.VtgCell3.BaseAddress_p == (void*)VTG3_BASE_ADDRESS)
        {
			#if defined(ST_7200)
            InitVTGParam.Target.VtgCell3.InterruptNumber = ST7200_VTG_SD0_INTERRUPT;
			#elif defined(ST_7111)
			InitVTGParam.Target.VtgCell3.InterruptNumber = ST7111_VTG_SD0_INTERRUPT;
			#elif defined(ST_7105)
			InitVTGParam.Target.VtgCell3.InterruptNumber = ST7105_VTG_SD0_INTERRUPT;
			#endif
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        InitVTGParam.Target.VtgCell3.InterruptLevel      = 4;

#elif defined (ST_5528)
        InitVTGParam.Target.VtgCell2.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        InitVTGParam.Target.VtgCell2.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        InitVTGParam.Target.VtgCell2.InterruptNumber     = VTG_INTERRUPT_NUMBER;
        InitVTGParam.Target.VtgCell2.InterruptLevel      = 7;
#elif defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)||defined(ST_5162)
        InitVTGParam.Target.VtgCell2.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        InitVTGParam.Target.VtgCell2.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        InitVTGParam.Target.VtgCell2.InterruptNumber     = VTG_INTERRUPT_NUMBER;
        InitVTGParam.Target.VtgCell2.InterruptLevel      = 0;
#endif
    }

    /* Other inits */
    InitVTGParam.MaxOpen = 1;
    strcpy(InitVTGParam.EvtHandlerName, STEVT_DEVICE_NAME);
#ifdef STVTG_VSYNC_WITHOUT_VIDEO
    InitVTGParam.VideoBaseAddress_p       = (void*) VIDEO_BASE_ADDRESS;
    InitVTGParam.VideoDeviceBaseAddress_p = (void*) 0;
    InitVTGParam.VideoInterruptNumber     = VIDEO_INTERRUPT;
    InitVTGParam.VideoInterruptLevel      = VIDEO_INTERRUPT_LEVEL;
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */

    Err = STVTG_Init( STVTG_DEVICE_NAME, &InitVTGParam);
    strcpy(Msg_string, "STVTG_Init");
    ReportError(Err, &Msg_string[0]);

    Err = STVTG_Open( STVTG_DEVICE_NAME, &OpenVTGParam, &VTGHandle);
   strcpy(Msg_string, "STVTG_Open");
   ReportError(Err, &Msg_string[0]);

    /* PAL mode */
    StackModeVtg = STVTG_TIMING_MODE_576I50000_13500;
    Err = STVTG_SetMode(VTGHandle, StackModeVtg);
    strcpy(Msg_string, "STVTG_SetMode(PAL)");
    ReportError(Err, &Msg_string[0]);

    /* Get the parameters from the VTG */
    Err = STVTG_GetMode(VTGHandle, &StackModeVtg, &StackModeParams);
    strcpy(Msg_string, "STVTG_GetMode(PAL)");
    ReportError(Err, &Msg_string[0]);

    /* Get Capability */
    Err = STVTG_GetCapability( STVTG_DEVICE_NAME, &VTGCapability);
    strcpy(Msg_string, "STVTG_GetCapability");
    ReportError(Err, &Msg_string[0]);

    /* Program one HD mode */
    if ( (InitVTGParam.DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7015) || (InitVTGParam.DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7020)
                   || (InitVTGParam.DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7710))
    {
        StackModeVtg = STVTG_TIMING_MODE_1035I60000_74250;
        Err = STVTG_SetMode(VTGHandle, StackModeVtg);
        strcpy(Msg_string, "STVTG_SetMode(HD)");
        ReportError(Err, &Msg_string[0]);

        /* Get the parameters from the VTG */
        Err = STVTG_GetMode(VTGHandle, &StackModeVtg, &StackModeParams);
        strcpy(Msg_string, "STVTG_GetMode(HD)");
        ReportError(Err, &Msg_string[0]);
    }

    /* Back to NTSC mode */
    StackModeVtg = STVTG_TIMING_MODE_576I50000_13500;
    Err = STVTG_SetMode(VTGHandle, StackModeVtg);
    strcpy(Msg_string, "STVTG_SetMode(NTSC)");
    ReportError(Err, &Msg_string[0]);

    /* Get the parameters from the VTG */
    Err = STVTG_GetMode(VTGHandle, &StackModeVtg, &StackModeParams);
    strcpy(Msg_string, "STVTG_GetMode(NTSC)");
    ReportError(Err, &Msg_string[0]);

    /* Program one slave mode */
    SlaveModeParams_s.SlaveMode = 0;
    if(InitVTGParam.DeviceType == STVTG_DEVICE_TYPE_DENC)
    {
        SlaveModeParams_s.Target.Denc.DencSlaveOf = STVTG_DENC_SLAVE_OF_ODDEVEN;
        SlaveModeParams_s.Target.Denc.FreeRun = 0;
        SlaveModeParams_s.Target.Denc.OutSyncAvailable = 0;
        SlaveModeParams_s.Target.Denc.SyncInAdjust = 1;
    }
    else{
        SlaveModeParams_s.Target.VtgCell.HSlaveOf = 0;
        SlaveModeParams_s.Target.VtgCell.VSlaveOf = 0;
        SlaveModeParams_s.Target.VtgCell.HRefVtgIn = STVTG_VTG_ID_1;
        SlaveModeParams_s.Target.VtgCell.VRefVtgIn = STVTG_VTG_ID_1;
        SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMin = 200;
        SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMax = 600;
        SlaveModeParams_s.Target.VtgCell.ExtVSyncShape = 1;
        SlaveModeParams_s.Target.VtgCell.IsExtVSyncRisingEdge = 1;
        SlaveModeParams_s.Target.VtgCell.IsExtHSyncRisingEdge = 0;
       }
    Err = STVTG_SetSlaveModeParams(VTGHandle, &SlaveModeParams_s);
    strcpy(Msg_string, "STVTG_SetSlaveModeParams()");
    ReportError(Err, &Msg_string[0]);

    /* Execute this mode */
    StackModeVtg = STVTG_TIMING_MODE_SLAVE;
    Err = STVTG_SetMode(VTGHandle, StackModeVtg);
    strcpy(Msg_string, "STVTG_SetMode(SLAVE)");
    ReportError(Err, &Msg_string[0]);

    /* Get the parameters from the VTG */
    Err = STVTG_GetSlaveModeParams(VTGHandle, &SlaveModeParams_s);
    strcpy(Msg_string, "STVTG_GetSlaveModeParams()");
    ReportError(Err, &Msg_string[0]);

    /* Back to NTSC mode */
    StackModeVtg = STVTG_TIMING_MODE_576I50000_13500;
    Err = STVTG_SetMode(VTGHandle, StackModeVtg);
    strcpy(Msg_string, "STVTG_SetMode(NTSC)");
    ReportError(Err, &Msg_string[0]);

    /* Set one parameter */
    if(InitVTGParam.DeviceType == STVTG_DEVICE_TYPE_DENC)
    {
        StackOptionalParams_s.Option = STVTG_OPTION_HSYNC_POLARITY;
        StackOptionalParams_s.Value.IsHSyncPositive = TRUE;
    }
    else
    {
        StackOptionalParams_s.Option = STVTG_OPTION_EMBEDDED_SYNCHRO;
        StackOptionalParams_s.Value.EmbeddedSynchro = TRUE;
    }
    Err = STVTG_SetOptionalConfiguration(VTGHandle, &StackOptionalParams_s);
    strcpy(Msg_string, "STVTG_SetOptionalConfiguration()");
    ReportError(Err, &Msg_string[0]);

    Err = STVTG_GetOptionalConfiguration(VTGHandle, &StackOptionalParams_s);
    strcpy(Msg_string, "STVTG_GetOptionalConfiguration()");
    ReportError(Err, &Msg_string[0]);

     if(InitVTGParam.DeviceType == STVTG_DEVICE_TYPE_DENC)
    {
        StackOptionalParams_s.Option = STVTG_OPTION_HSYNC_POLARITY;
        StackOptionalParams_s.Value.IsHSyncPositive = FALSE;
    }
    else
    {
        StackOptionalParams_s.Option = STVTG_OPTION_EMBEDDED_SYNCHRO;
        StackOptionalParams_s.Value.EmbeddedSynchro = FALSE;
    }

    Err = STVTG_SetOptionalConfiguration(VTGHandle, &StackOptionalParams_s);
    strcpy(Msg_string, "STVTG_SetOptionalConfiguration()");
    ReportError(Err, &Msg_string[0]);

    Err = STVTG_GetOptionalConfiguration(VTGHandle, &StackOptionalParams_s);
    strcpy(Msg_string, "STVTG_GetOptionalConfiguration()");
    ReportError(Err, &Msg_string[0]);

    Err = STVTG_Close(VTGHandle);
    strcpy(Msg_string, "STVTG_Close");
    ReportError(Err, &Msg_string[0]);

    TermVTGParam.ForceTerminate = FALSE;
    Err = STVTG_Term( STVTG_DEVICE_NAME, &TermVTGParam);
    strcpy(Msg_string, "STVTG_Term");
    ReportError(Err, &Msg_string[0]);
}

void test_overhead(void *dummy)
{
    char Msg_string[100];
    ST_ErrorCode_t Err;
    Err = ST_NO_ERROR;
   UNUSED_PARAMETER(dummy);
   strcpy(Msg_string, "test_overhead");
   ReportError(Err, &Msg_string[0]);
}

/*******************************************************************************
Name        : metrics_Stack_Test
Description : launch tasks to calculate the stack usage made by the driver
              for an Init Term cycle and in its typical conditions of use
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
void metrics_Stack_Test(void)
{
    task_t *task_p;
    task_status_t status;
    int overhead_stackused;
    char funcname[3][20]= {
        "test_overhead",
        "test_typical",
        "NULL"
    };
    void (*func_table[])(void *) = {
        test_overhead,
        test_typical,
        NULL
    };
    void (*func)(void *);
    int i;

    overhead_stackused = 0;
    printf("*************************************\n");
    printf("* Stack usage calculation beginning *\n");
    printf("*************************************\n");
    for (i = 0; func_table[i] != NULL; i++)
    {
        func = func_table[i];

        /* Start the task */
        task_p = task_create(func, NULL, STACK_SIZE, MAX_USER_PRIORITY, "stack_test", task_flags_no_min_stack_size);

        /* Wait for task to complete */
        task_wait(&task_p, 1, TIMEOUT_INFINITY);

        /* Dump stack usage */
        task_status(task_p, &status, task_status_flags_stack_used);
        /* store overhead value */
        if (i==0)
        {
            printf("*-----------------------------------*\n");
            overhead_stackused = status.task_stack_used;
            printf("%s \t func=0x%08lx stack = %d bytes used\n", &funcname[i][0], (long) func,
                    status.task_stack_used);
            printf("*-----------------------------------*\n");
        }
        else
        {
            printf("%s \t func=0x%08lx stack = %d bytes used (%d - %d overhead)\n", &funcname[i][0], (long) func,
                    status.task_stack_used-overhead_stackused,status.task_stack_used,overhead_stackused);
        }
        /* Tidy up */
        task_delete(task_p);
    }
    printf("*************************************\n");
    printf("*    Stack usage calculation end    *\n");
    printf("*************************************\n");
}
#endif /* ST_OS20 */

/*-------------------------------------------------------------------------
 * Function : VTG_Event
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_Event( STTST_Parse_t *pars_p, char *result_sym_p )
{
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr,RetErr_1=FALSE;
    BOOL RetErr_2=FALSE;
    unsigned long  FieldRateComputed, ListenPeriodGot, VSyncAll;
    U32 Var, TickPerSecond, FieldRate, ClockElapsed, DeltaF;
    STGXOBJ_ScanType_t ScanType;
    STEVT_DeviceSubscribeParams_t   SubscribeParams;
    STEVT_OpenParams_t              OpenParams;
    STEVT_Handle_t                  EvtHandle;


    VTG_Vsync_t VSyncInfo;
    ST_ErrorCode_t ErrCode;

    UNUSED_PARAMETER(result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, sizeof(DeviceName));
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName");
        return(TRUE);
    }

    /* Get Type of VSync TOP/BOTTOM or BOTTOM Only */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_INTERLACED_SCAN , (S32*)&Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected ScanType (default interlaced)");
        return(TRUE);
    }
    ScanType = (STGXOBJ_ScanType_t) Var;

    /* Get Frame rate */
    RetErr = STTST_GetInteger( pars_p, 50000, (S32*)&Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected FieldRate (default 50000 millihertz)");
        return(TRUE);
    }
    FieldRate = Var;

    /* Get evt handle */
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams, &EvtHandle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STEVT_Open fails : error = 0x%0x\n", ErrCode ));
        return(TRUE);
    }

    /* Subscribe to VSync event */
    SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VTG_ReceiveEvtVSync;
    SubscribeParams.SubscriberData_p = &VSyncInfo;
    VSyncInfo.Top = 0;
    VSyncInfo.Bottom = 0;
    VSyncInfo.TopMissed = 0;
    VSyncInfo.BottomMissed = 0;
    VSyncInfo.ClockStored = FALSE;
    VSyncInfo.ClockLast = VSyncInfo.ClockFirst = 0;
    VSyncInfo.Value.FirstVsync = TRUE;
    VSyncInfo.Value.Top = FALSE;
    VSyncInfo.Value.Bottom = FALSE;
    VSyncInfo.Error = NO_ERROR;
    VSyncInfo.Value.VsyncDelay = 0;
    VSyncInfo.Value.DelayRef = 0;
    VSyncInfo.VsyncTime.Time = 0;
    VSyncInfo.VsyncTime.Time1 = 0;
    VSyncInfo.VsyncTime.Time2 = 0;
    VSyncInfo.VsyncTime.TimeRef = 0;
    VSyncInfo.VsyncTime.TimeRef_OK = FALSE;
    VSyncInfo.VsyncTime.Time1_OK = FALSE;
    VSyncInfo.VsyncTime.Time2_OK = FALSE;
    VSyncInfo.ScanType_1 = ScanType;

    sprintf( Test_Msg, "VTG_Event(%s,Scan=%s,listen period requested=%ds) : ", \
             DeviceName,
             (ScanType == STGXOBJ_INTERLACED_SCAN) ? "INTER" :
             (ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" : "?????", VSYNC_COUNT_TIME);
    TEST_PRINT(Test_Msg);

    TickPerSecond = ST_GetClocksPerSecond();

    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle, DeviceName, STVTG_VSYNC_EVT, &SubscribeParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTST_TagCurrentLine( pars_p, "Unable to subscribe device event");
        return(TRUE);
    }

    STOS_TaskDelay((VSYNC_COUNT_TIME * TickPerSecond));


    /* Unsubscribe to VTG event */
    ErrCode = STEVT_UnsubscribeDeviceEvent(EvtHandle, DeviceName, STVTG_VSYNC_EVT);
    if(ErrCode != ST_NO_ERROR)
    {
        STTST_TagCurrentLine( pars_p, "Unable to unsubscribe device event");
        return(TRUE);
    }



    /* Free handle */
    ErrCode = STEVT_Close(EvtHandle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTST_TagCurrentLine( pars_p, "Unable to free handler");
        return(TRUE);
    }


    RetErr = FALSE;

    if(VSyncInfo.ClockLast >= VSyncInfo.ClockFirst)
    {
        ClockElapsed = VSyncInfo.ClockLast - VSyncInfo.ClockFirst;
    }
    else
    {
        ClockElapsed = 0xFFFF - VSyncInfo.ClockLast + VSyncInfo.ClockFirst;
    }
    ListenPeriodGot = (((unsigned long)ClockElapsed + (unsigned long)TickPerSecond/2) /(unsigned long)TickPerSecond);

    switch(ScanType)
    {
        case STGXOBJ_INTERLACED_SCAN:
            if((VSyncInfo.Top == 0) || (VSyncInfo.Bottom == 0))
            {
                RetErr = TRUE;
            }
            break;
        case STGXOBJ_PROGRESSIVE_SCAN:
            if((VSyncInfo.Bottom != 0) || (VSyncInfo.Top == 0))
            {
                RetErr = TRUE;
            }
            break;
        default:
            RetErr = TRUE;
            break;
    }

    VSyncAll = (unsigned long)VSyncInfo.Top + (unsigned long)VSyncInfo.Bottom - 1;
    if(ClockElapsed != 0)
    {
        FieldRateComputed = (1000000*VSyncAll + ListenPeriodGot/2)/ListenPeriodGot;
        FieldRateComputed = FieldRateComputed/1000;
    }
    else
    {
        FieldRateComputed = 0;
    }
    /* Check if over acceptable difference : F=N/T so dF/F=dN/N-dT/T, dT/T << 1, assume dN=+-1 (one Vsync max)*/
    /* so dF=F/N */
    DeltaF = (U32)(FieldRate / VSyncAll);
    if(FieldRate>FieldRateComputed)
    {
        if((FieldRate - FieldRateComputed) > DeltaF)
         {
           if((VSyncInfo.TopMissed!=0)||(VSyncInfo.BottomMissed!=0))
           {
                RetErr_1=TRUE;
           }
           else
           {
              RetErr_2=TRUE;
           }
         }
    }
    else
    {
        if((FieldRateComputed - FieldRate) > DeltaF)
         {
           if((VSyncInfo.TopMissed!=0)||(VSyncInfo.BottomMissed!=0))
           {
                RetErr_1=TRUE;
           }
           else
           {
              RetErr_2=TRUE;
           }
         }
    }

    STTST_AssignInteger("ERROR_VSYNC", RetErr_1, FALSE);
    STTST_AssignInteger("ERROR_MODE", RetErr_2, FALSE);
    STTST_AssignInteger("ERRORCODE", RetErr, FALSE);
    Test_Msg[0] = '\0';


   if (VSyncInfo.Error != NO_ERROR)
     {

         if (VSyncInfo.Error&VSYNC_DELAY)
                printf( "There is an error on VSYNC signal= VSYNC_DELAY \n");
        else if (VSyncInfo.Error&TWO_BOT_VSYNC)
                printf( "There is an error on VSYNC signal= TWO_BOT_VSYNC \n");
        else if (VSyncInfo.Error&TWO_TOP_VSYNC)
                printf( "There is an error on VSYNC signal= TWO_TOP_VSYNC \n");

     }

    if(RetErr)
    {
        strcat (Test_Msg , "Failed !!!\n");
        sprintf( Test_Msg, "%sDeltaF allowed= %dHz\n", Test_Msg, DeltaF);
        sprintf( Test_Msg, "%sPeriod based on nb of VSync received and expected field rate: %ldms",
            Test_Msg, (1000000*VSyncAll + (unsigned long)FieldRate/2) /(unsigned long)FieldRate);
    }
    else
    {
        strcat (Test_Msg , "Ok");
    }
    sprintf( Test_Msg, "%s\n", Test_Msg);
    sprintf( Test_Msg, "%sDeltaF allowed= %dHz\n", Test_Msg, DeltaF);
    sprintf( Test_Msg, "%s\n\tVsync Top=%d Bot=%d TopMissed=%d BottomMissed=%d T=%lds FR=%ld\n", Test_Msg,
             VSyncInfo.Top, VSyncInfo.Bottom, VSyncInfo.TopMissed, VSyncInfo.BottomMissed, ListenPeriodGot, FieldRateComputed);
    TEST_PRINT(Test_Msg);

    return(API_EnableError ? RetErr: FALSE);

 }
/*-------------------------------------------------------------------------
 * Function : VTG_NullPointersTest
 *            Bad parameter test : function call with null pointer
 *            (C code program because macro are not convenient for this test)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VTG_NullPointersTest(STTST_Parse_t *pars_p, char *result_sym_p)
{

    STVTG_InitParams_t VTGInitParams;
    STVTG_OpenParams_t VTGOpenParams;
    STVTG_TimingMode_t ModeVtg;
    STVTG_ModeParams_t ModeParams_t;
    STVTG_TermParams_t VTGTermParams;
    BOOL IsRetErr;
    U16 NbErr;
    ST_ErrorCode_t ErrCode;

    UNUSED_PARAMETER(pars_p);

    STTBX_Print(( "Call API functions with null pointers...\n" ));
    NbErr = 0;

    VTGInitParams.DeviceType = VTG_DEVICE_TYPE;
    strcpy(VTGInitParams.EvtHandlerName, STEVT_DEVICE_NAME);
    VTGInitParams.MaxOpen = 1;
    if(VTGInitParams.DeviceType == STVTG_DEVICE_TYPE_DENC){
        strcpy(VTGInitParams.Target.DencName, STDENC_DEVICE_NAME);
    }
    else
    {
#if defined (ST_7015)
        VTGInitParams.Target.VtgCell.BaseAddress_p   = (void*)VTGCMD_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7015_VTG1_OFFSET)
        {
            VTGInitParams.Target.VtgCell.InterruptEvent = ST7015_VTG1_INT;
        }
        else if (VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7015_VTG2_OFFSET)
        {
            VTGInitParams.Target.VtgCell.InterruptEvent = ST7015_VTG2_INT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        strcpy(VTGInitParams.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#elif defined (ST_7020)
        VTGInitParams.Target.VtgCell.BaseAddress_p   = (void*)VTGCMD_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7020_VTG1_OFFSET)
        {
            VTGInitParams.Target.VtgCell.InterruptEvent = ST7020_VTG1_INT;
        }
        else if (VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7020_VTG2_OFFSET)
        {
            VTGInitParams.Target.VtgCell.InterruptEvent = ST7020_VTG2_INT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        strcpy(VTGInitParams.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#elif defined (ST_7710)
        VTGInitParams.Target.VtgCell3.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG1_OFFSET)

        {
#ifdef WA_GNBvd35956
            /* Install DVP Interrupt number instead of VOS1 one.*/
            VTGInitParams.Target.VtgCell3.InterruptNumber = ST7710_DVP_INTERRUPT;
#else

            VTGInitParams.Target.VtgCell3.InterruptNumber = ST7710_VOS_0_INTERRUPT;
#endif

        }
        else if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG2_OFFSET)
        {
             VTGInitParams.Target.VtgCell3.InterruptNumber = ST7710_VOS_1_INTERRUPT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        VTGInitParams.Target.VtgCell3.InterruptLevel      = 4;
#ifdef STVTG_USE_CLKRV
        strcpy(VTGInitParams.Target.VtgCell3.ClkrvName , STCLKRV_DEVICE_NAME) ;
#endif

#elif defined (ST_7100)
        VTGInitParams.Target.VtgCell3.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG1_OFFSET)
        {
            VTGInitParams.Target.VtgCell3.InterruptNumber = ST7100_VTG_0_INTERRUPT;
        }
        else if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG2_OFFSET)
        {
            VTGInitParams.Target.VtgCell3.InterruptNumber = ST7100_VTG_1_INTERRUPT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        VTGInitParams.Target.VtgCell3.InterruptLevel      = 4;
#ifdef STVTG_USE_CLKRV
        strcpy(VTGInitParams.Target.VtgCell3.ClkrvName , STCLKRV_DEVICE_NAME) ;
#endif
#elif defined (ST_7109)
        VTGInitParams.Target.VtgCell3.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG1_OFFSET)
        {
            VTGInitParams.Target.VtgCell3.InterruptNumber = ST7109_VTG_0_INTERRUPT;
        }
        else if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG2_OFFSET)
        {
            VTGInitParams.Target.VtgCell3.InterruptNumber = ST7109_VTG_1_INTERRUPT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        VTGInitParams.Target.VtgCell3.InterruptLevel      = 4;
#ifdef STVTG_USE_CLKRV
        strcpy(VTGInitParams.Target.VtgCell3.ClkrvName , STCLKRV_DEVICE_NAME) ;
#endif
 #elif defined(ST_7200)||defined(ST_7111)||defined(ST_7105)
        VTGInitParams.Target.VtgCell3.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG1_BASE_ADDRESS)
        {
			#if defined(ST_7200)
            VTGInitParams.Target.VtgCell3.InterruptNumber =  ST7200_VTG_MAIN0_INTERRUPT;
			#elif defined(ST_7111)
			VTGInitParams.Target.VtgCell3.InterruptNumber =  ST7111_VTG_MAIN0_INTERRUPT;
			#elif defined(ST_7105)
			VTGInitParams.Target.VtgCell3.InterruptNumber =  ST7105_VTG_MAIN0_INTERRUPT;
			#endif
        }
        else if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG2_BASE_ADDRESS)
        {
			#if defined(ST_7200)
            VTGInitParams.Target.VtgCell3.InterruptNumber =  ST7200_VTG_AUX0_INTERRUPT;
			#elif defined(ST_7111)
			VTGInitParams.Target.VtgCell3.InterruptNumber =  ST7111_VTG_AUX0_INTERRUPT;
			#elif defined(ST_7105)
			VTGInitParams.Target.VtgCell3.InterruptNumber =  ST7105_VTG_AUX0_INTERRUPT;
			#endif
        }
        else if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG3_BASE_ADDRESS)
        {
			#if defined(ST_7200)
            VTGInitParams.Target.VtgCell3.InterruptNumber = ST7200_VTG_SD0_INTERRUPT;
			#elif defined(ST_7111)
			VTGInitParams.Target.VtgCell3.InterruptNumber = ST7111_VTG_SD0_INTERRUPT;
			#elif defined(ST_7105)
			VTGInitParams.Target.VtgCell3.InterruptNumber = ST7105_VTG_SD0_INTERRUPT;
			#endif
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        VTGInitParams.Target.VtgCell3.InterruptLevel      = 4;

#ifdef STVTG_USE_CLKRV
        strcpy(VTGInitParams.Target.VtgCell3.ClkrvName , STCLKRV_DEVICE_NAME) ;
#endif
#elif defined (ST_5528)
        VTGInitParams.Target.VtgCell2.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell2.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell2.InterruptNumber     = VTG_INTERRUPT_NUMBER;
        VTGInitParams.Target.VtgCell2.InterruptLevel      = 7;
#elif defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)||defined(ST_5162)
        VTGInitParams.Target.VtgCell2.BaseAddress_p       = (void*)VTGCMD_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell2.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell2.InterruptNumber     = VTG_INTERRUPT_NUMBER;
        VTGInitParams.Target.VtgCell2.InterruptLevel      = 0;
#elif defined (ST_GX1)
        VTGInitParams.Target.VtgCell.BaseAddress_p   = (void*)VTGCMD_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell.InterruptEvent = 0;
        strcpy(VTGInitParams.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#endif
    }
#ifdef STVTG_VSYNC_WITHOUT_VIDEO
    VTGInitParams.VideoBaseAddress_p       = (void*) VIDEO_BASE_ADDRESS;
    VTGInitParams.VideoDeviceBaseAddress_p = (void*) 0;
    VTGInitParams.VideoInterruptNumber     = VIDEO_INTERRUPT;
    VTGInitParams.VideoInterruptLevel      = VIDEO_INTERRUPT_LEVEL;
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */

    /* Test STVTG_Init with a NULL pointer parameter */
    ErrCode = STVTG_Init("", &VTGInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "  STVTG_Init(NULL,...) : unexpected return code=0x%0x !\n", ErrCode ));
        NbErr++;
    }

    /* Test STVTG_Init with a NULL pointer parameter */
    ErrCode = STVTG_Init(STVTG_DEVICE_NAME, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "  STVTG_Init(%s, NULL) : unexpected return code=0x%0x \n", STVTG_DEVICE_NAME, ErrCode ));
        NbErr++;
    }

    /* Init a device to for further tests needing an init device */
    ErrCode = STVTG_Init(STVTG_DEVICE_NAME, &VTGInitParams);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( "  STVTG_Init() : unexpected return code=0x%0x \n", ErrCode ));
        NbErr++;
    }
    else
    {
        /* Test STVTG_GetCapability with a NULL pointer parameter */
        ErrCode = STVTG_GetCapability(STVTG_DEVICE_NAME, NULL);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "  STVTG_GetCapability(%s, NULL,...) : unexpected return code=0x%0x \n", STVTG_DEVICE_NAME, ErrCode ));
            NbErr++;
        }

        /* Test STVTG_Open with a NULL pointer parameter */
        ErrCode = STVTG_Open(STVTG_DEVICE_NAME, NULL, &VTGHandle);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "  STVTG_Open(%s, NULL,...) : unexpected return code=0x%0x \n", STVTG_DEVICE_NAME, ErrCode ));
            NbErr++;
        }

        /* Test STVTG_Open with a NULL pointer parameter */
        ErrCode = STVTG_Open(STVTG_DEVICE_NAME, &VTGOpenParams, NULL);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "  STVTG_Open(%s, ...,NULL) : unexpected return code=0x%0x \n", STVTG_DEVICE_NAME, ErrCode ));
            NbErr++;
        }

        /* Open device to get a good handle for further tests */
        ErrCode = STVTG_Open(STVTG_DEVICE_NAME, &VTGOpenParams, &VTGHandle);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(( "  STVTG_Open() : unexpected return code=0x%0x \n", ErrCode ));
            NbErr++;
        }
        else
        {
            /* Test STVTG_GetMode with a NULL pointer parameter */
            ErrCode = STVTG_GetMode(VTGHandle, NULL, &ModeParams_t);
            if ( ErrCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVTG_GetMode(.., NULL, ..) : unexpected return code=0x%0x \n", ErrCode ));
                NbErr++;
            }
            /* Test STVTG_GetMode with a NULL pointer parameter */
            ErrCode = STVTG_GetMode(VTGHandle, &ModeVtg, NULL);
            if ( ErrCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVTG_GetMode(.., .., NULL) : unexpected return code=0x%0x \n", ErrCode ));
                NbErr++;
            }
            /* Test STVTG_GetSlaveModeParams with a NULL pointer parameter */
            ErrCode = STVTG_GetSlaveModeParams(VTGHandle, NULL);
            if ( ErrCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVTG_GetSlaveModeParams(.., NULL) : unexpected return code=0x%0x \n", ErrCode ));
                NbErr++;
            }
            /* Test STVTG_SetSlaveModeParams with a NULL pointer parameter */
            ErrCode = STVTG_SetSlaveModeParams(VTGHandle, NULL);
            if ( ErrCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVTG_SetSlaveModeParams(.., NULL) : unexpected return code=0x%0x \n", ErrCode ));
                NbErr++;
            }

            /* Close connection */
            ErrCode = STVTG_Close(VTGHandle);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(( "  STVTG_Close() : unexpected return code=0x%0x \n", ErrCode ));
        NbErr++;
            }
        }
        /* Test STVTG_Term with a NULL pointer parameter */
        ErrCode = STVTG_Term(STVTG_DEVICE_NAME, NULL);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "  STVTG_Term(%s, NULL) : unexpected return code=0x%0x \n", STVTG_DEVICE_NAME, ErrCode ));
            NbErr++;
        }
        /* Terminate device use */
        VTGTermParams.ForceTerminate = 0;
        ErrCode = STVTG_Term(STVTG_DEVICE_NAME, &VTGTermParams);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(( "  STVTG_Term() : unexpected return code=0x%0x \n", ErrCode ));
            NbErr++;
        }
    }

    /* cases of structures containing a null pointer */
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VTG_CELL_7015;
    VTGInitParams.Target.VtgCell.BaseAddress_p = NULL;
    ErrCode = STVTG_Init(STVTG_DEVICE_NAME, &VTGInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "  STVTG_Init(..,(BaseAddress_p = NULL)) : unexpected return code=0x%0x \n", ErrCode ));
        NbErr++;
    }
    VTGInitParams.Target.VtgCell.BaseAddress_p = (void *)VTGCMD_BASE_ADDRESS;

    /* test result */
    if ( NbErr == 0 )
    {
        STTBX_Print(( "### VTG_NullPointersTest() result : ok, ST_ERROR_BAD_PARAMETER returned each time as expected\n" ));
        IsRetErr = FALSE;
    }
    else
    {
        STTBX_Print(( "### VTG_NullPointersTest() result : failed ! ST_ERROR_BAD_PARAMETER not returned %d times\n", NbErr ));
        IsRetErr = TRUE;
    }

    assign_integer( result_sym_p, NbErr, FALSE);

    return(API_EnableError ? IsRetErr : FALSE );
} /* end of VTG_NullPointersTest() */

/*******************************************************************************
Name        : VTG_ReceiveEvtVSync
Description :
Parameters  :
Assumptions : Device is valid
Limitations :
Returns     : none
*******************************************************************************/
static void VTG_ReceiveEvtVSync(STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void *EventData,
                                void *SubscriberData_p)

{
    STVTG_VSYNC_t EventType;
    VTG_Vsync_t*  Vsync_p;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);


    EventType = *((const STVTG_VSYNC_t*) EventData);
    Vsync_p = ((VTG_Vsync_t*) SubscriberData_p);

    Vsync_p->VsyncTime.Time = STOS_time_now();

    if(!Vsync_p->VsyncTime.TimeRef_OK)
    {
       Vsync_p->VsyncTime.TimeRef = Vsync_p->VsyncTime.Time;
    }

    if(Vsync_p->VsyncTime.Time1_OK)
    {
      if(!Vsync_p->VsyncTime.Time2_OK)
       {
         Vsync_p->Value.VsyncDelay = Vsync_p->VsyncTime.Time - Vsync_p->VsyncTime.Time1;
       }
       else
       {
         Vsync_p->Value.VsyncDelay = Vsync_p->VsyncTime.Time - Vsync_p->VsyncTime.Time2;

       }
      Vsync_p->Value.VsyncDelay = Vsync_p->Value.VsyncDelay ;
      Vsync_p->VsyncTime.Time2_OK = TRUE;
      Vsync_p->VsyncTime.Time2 = Vsync_p->VsyncTime.Time;
      if (((U32) abs(Vsync_p->Value.VsyncDelay - Vsync_p->Value.DelayRef )) > Vsync_p->Value.DelayRef  )
              Vsync_p->Error |= VSYNC_DELAY ;              /* Wrong delay of Vsync*/

    }
    if( (Vsync_p->VsyncTime.TimeRef_OK) && (!Vsync_p->VsyncTime.Time1_OK))
     {
       Vsync_p->VsyncTime.Time1 = Vsync_p->VsyncTime.Time;
       Vsync_p->Value.DelayRef = Vsync_p->VsyncTime.Time1 - Vsync_p->VsyncTime.TimeRef;
       Vsync_p->Value.DelayRef = Vsync_p->Value.DelayRef ;
       Vsync_p->VsyncTime.Time1_OK = TRUE;

     }

     Vsync_p->VsyncTime.TimeRef_OK = TRUE;


    if(!Vsync_p->ClockStored)
    {

        Vsync_p->ClockStored = TRUE;
        Vsync_p->ClockFirst = STOS_time_now();

    }

    Vsync_p->ClockLast = STOS_time_now();

if (Vsync_p->ScanType_1 == STGXOBJ_PROGRESSIVE_SCAN)
{
  switch(EventType)
    {
        case STVTG_TOP:
            Vsync_p->Top++;

            break;
        case STVTG_BOTTOM:
            Vsync_p->Bottom++;
            break;
        default:
            break;
    }
}

if (Vsync_p->ScanType_1 == STGXOBJ_INTERLACED_SCAN )
{
    switch(EventType)
    {
        case STVTG_TOP:
            if(Vsync_p->Value.FirstVsync)
            {
                Vsync_p->Value.Top = TRUE;
                Vsync_p->Value.FirstVsync = FALSE;
                Vsync_p->Value.Bottom = TRUE;
            }
            if (Vsync_p->Value.Bottom)
            {
                Vsync_p->Value.Bottom = FALSE;
                Vsync_p->Value.Top = TRUE;
                Vsync_p->Top++;
            }
            else
            {
                Vsync_p->Error |= TWO_TOP_VSYNC ; /* two Vsync Top*/
                Vsync_p->BottomMissed++;
            }
            break;
        case STVTG_BOTTOM:
            if(Vsync_p->Value.FirstVsync)
            {
              Vsync_p->Value.Bottom = TRUE;
              Vsync_p->Value.FirstVsync = FALSE;
              Vsync_p->Value.Top = TRUE;
            }
            if (Vsync_p->Value.Top)
            {
                Vsync_p->Value.Bottom = TRUE;
                Vsync_p->Value.Top = FALSE;
                Vsync_p->Bottom++;

            }
            else
            {
                 Vsync_p->Error |= TWO_BOT_VSYNC;  /* two Vsync Bot */
                 Vsync_p->TopMissed++;

            }


            break;
        default:
            break;
    }
}
} /* end of VTG_ReceiveEvtVSync() */



/*-------------------------------------------------------------------------
 * Function : Test_CmdStart
 *            Definition of test register command
 * Input    :
 * Output   :
 * Return   : TRUE if success, FALSE if error
 * ----------------------------------------------------------------------*/
BOOL Test_CmdStart(void)
{
    BOOL RetErr = FALSE;

    RetErr |= STTST_RegisterCommand( "VTG_Event", VTG_Event, "<VTGName><ScanType><FieldRate> VTG event test");
    RetErr |= STTST_RegisterCommand( "VTG_NullPt", VTG_NullPointersTest, \
                                     "Call API functions with null pointers (bad param. test)");
#ifdef ST_OS20
    RetErr |= STTST_RegisterCommand( "VTG_StackUse" , (BOOL (*)(STTST_Parse_t*, char*))metrics_Stack_Test, "");
#endif

#ifdef STI7710_CUT2x
RetErr |= STTST_AssignString ("CHIP_CUT", "STI7710_CUT2x", TRUE);
#else
RetErr |= STTST_AssignString ("CHIP_CUT", "", TRUE);
#endif
    if (RetErr)
    {
        sprintf( Test_Msg,  "VTG_TestCommand() : macros registrations failure !\n");
    }
    else
    {
        sprintf( Test_Msg,  "VTG_TestCommand() Ok\n");
    }
    TEST_PRINT(Test_Msg);

    return(!RetErr);
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
#if defined(ST_OS21) || defined(ST_OSWINCE)
    printf ("\nBOOT ...\n");
#ifndef ST_OSWINCE
    setbuf(stdout, NULL);
#else
    _WCE_SetMsgLevel(0);
	SetFopenBasePath("/dvdgr-prj-stvtg/tests/src/objs/ST40");
#endif /* ST_OSWINCE*/
#endif
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

     os20_main(NULL);

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}
/* end of vtg_test.c */





