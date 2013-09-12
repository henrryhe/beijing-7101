/****************************************************************************

File Name   : startup.c

Description : Initialisation of ST... library

Copyright (C) 2000, ST Microelectronics

References  :

****************************************************************************/
/* Includes --------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "startup.h"

#include "stboot.h"
#include "stcommon.h"
#include "stdevice.h"
#include "stsys.h"
#include "sttbx.h"
#include "testtool.h"

#ifdef DUMP_REGISTERS
#include "dumpreg.h"
#endif

#ifdef TRACE_UART
#include "trace.h"
#endif

#ifdef USE_AVMEM
#include "stavmem.h"
#endif

#ifdef USE_CLKRV
#include "stclkrv.h"
#endif

#ifdef USE_EVT
#include "stevt.h"
#endif

#ifdef USE_GPDMA
#include "stgpdma.h"
#endif

#ifdef USE_I2C
#include "sti2c.h"
#endif

#ifdef USE_INTMR
#include "stintmr.h"
#endif

#ifdef USE_HDD
#include "hdd.h"
#endif

#ifdef USE_PIO
#include "stpio.h"
#endif

#ifdef USE_STCFG
#include "stcfg.h"
#endif

#ifdef USE_VIN
#include "stvin.h"
#endif

#ifdef USE_PWM
#include "stpwm.h"
#endif

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

/* Private variables (static) --------------------------------------------- */

static STBOOT_InitParams_t InitStboot;

/* Allow room for OS20 segments in internal memory */
static unsigned char    InternalBlock [ST20_INTERNAL_MEMORY_SIZE-1200];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      (InternalBlock, "internal_section")

static unsigned char    NCacheMemory [NCACHE_MEMORY_SIZE];
#pragma ST_section      (NCacheMemory , "ncache_section")
#endif /* #ifdef ARCHITECTURE_ST20 */

static unsigned char    ExternalBlock[SYSTEM_MEMORY_SIZE];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      (ExternalBlock, "system_section")
#endif /* #ifdef ARCHITECTURE_ST20 */

static ST_Partition_t   TheInternalPartition;
static ST_Partition_t   TheSystemPartition;
#ifdef ARCHITECTURE_ST20
static ST_Partition_t   TheNCachePartition;
#endif /* #ifdef ARCHITECTURE_ST20 */

#ifdef USE_PIO
static STPIO_InitParams_t PioInitParams[NUMBER_PIO];

static ST_DeviceName_t PioDeviceName[] =
{
    STPIO_0_DEVICE_NAME,
    STPIO_1_DEVICE_NAME,
    STPIO_2_DEVICE_NAME,
    STPIO_3_DEVICE_NAME,
    STPIO_4_DEVICE_NAME,
    STPIO_5_DEVICE_NAME,
    ""
};
#endif

/* Global variables ------------------------------------------------------- */

ST_Partition_t          *InternalPartition = &TheInternalPartition;
#ifdef ARCHITECTURE_ST20
ST_Partition_t          *NCachePartition = &TheNCachePartition;
ST_Partition_t          *ncache_partition = &TheNCachePartition;
#endif /* #ifdef ARCHITECTURE_ST20 */
ST_Partition_t          *SystemPartition = &TheSystemPartition;
ST_Partition_t          *DriverPartition = &TheSystemPartition;

ST_Partition_t          *system_partition = &TheSystemPartition;
ST_Partition_t          *internal_partition = &TheInternalPartition;

#ifdef USE_AVMEM
STAVMEM_PartitionHandle_t AvmemPartitionHandle;
#endif

#ifdef USE_CLKRV
STCLKRV_Handle_t          CLKRVHndl0;
#endif

#ifdef __FULL_DEBUG__
STBOOT_DCache_Area_t DCacheMap[] =
{
    { NULL, NULL }
};
#else
STBOOT_DCache_Area_t DCacheMap[] =
{
    { (U32 *)DCACHE_START, (U32 *)DCACHE_END }, /* Cacheable */
    { NULL, NULL }                              /* End */
};
#endif /* __FULL_DEBUG__ */

/* Extern functions prototypes -------------------------------------------- */
/* Test driver specific */
extern BOOL Test_CmdStart(void);

/* dependencies */
extern BOOL API_RegisterCmd(void);

#ifdef USE_AUDIO
extern BOOL AUD_RegisterCmd(void);
extern BOOL AUD_InitComponent(void);
#endif

#ifdef USE_CC
extern BOOL CC_RegisterCmd(void);
#endif

#ifdef USE_DENC
extern BOOL DENC_RegisterCmd(void);
#endif

#ifdef USE_EXTVIN
extern BOOL EXTVIN_RegisterCmd(void);
#endif

#ifdef USE_LAYER
extern BOOL LAYER_RegisterCmd(void);
#endif

#ifdef USE_PTI
extern BOOL PTI_InitComponent(void);
extern void PTI_TermComponent(void);
extern BOOL PTI_RegisterCmd(void);
#endif

#ifdef USE_TTX
extern BOOL TTX_RegisterCmd(void);
#endif

#ifdef USE_VBI
extern BOOL VBI_RegisterCmd(void);
#endif

#ifdef USE_VIDEO
extern BOOL VID_RegisterCmd(void);
#endif
#ifdef USE_VIDEO_2
extern BOOL VID_RegisterCmd2(void);
#endif

#ifdef USE_VIN
extern BOOL VIN_RegisterCmd(void);
#endif

#ifdef USE_VOUT
extern BOOL VOUT_RegisterCmd(void);
#endif

#ifdef USE_VMIX
extern BOOL VMIX_RegisterCmd(void);
#endif

#ifdef USE_VTG
extern BOOL VTG_RegisterCmd(void);
#endif


/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

#ifdef USE_AVMEM
static BOOL AVMEM_Init(void);
static void AVMEM_Term(void);
#endif

static BOOL BOOT_Init(void);
static void BOOT_Term(void);

#ifdef USE_STCFG
static BOOL CFG_Init(void);
/* no term() routine needed */
#endif

#ifdef USE_CLKRV
static BOOL CLKRV_Init(void);
static void CLKRV_Term(void);
#endif

#ifdef DUMP_REGISTERS
static BOOL DUMPREG_RegisterCmd(void);
#endif

static BOOL EVT_Init(void);
static void EVT_Term(void);

#ifdef USE_GPDMA
static BOOL GPDMA_Init(void);
static void GPDMA_Term(void);
#endif

#ifdef USE_I2C
static BOOL I2C_Init(void);
static void I2C_Term(void);
#endif

#ifdef USE_INTMR
static BOOL INTMR_Init(void);
static void INTMR_Term(void);
#endif

#ifdef USE_STV6410
static BOOL STV6410_Init(void);
/* no term() routine needed */
#endif

#ifdef USE_PIO
static BOOL PIO_Init(void);
static void PIO_Term(void);
#endif

#ifdef USE_PWM
static BOOL PWM_Init(void);
static void PWM_Term(void);
#endif

#ifdef USE_UART
static BOOL UART_Init(void);
static void UART_Term(void);
#endif

#ifdef USE_TBX
static BOOL TBX_Init(void);
static void TBX_Term(void);
#endif

static BOOL TST_Init(void);

static BOOL StartInitComponents(void);
static void StartTermComponents(void);
static BOOL StartSettingDevices(void);
static BOOL StartRegisterCmds(void);

int main( int argc, char *argv[] );


/* Functions -------------------------------------------------------------- */


#ifdef USE_AVMEM
/*******************************************************************************
Name        : AVMEM_Init
Description : Initialise and open AVMEM
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL AVMEM_Init(void)
{
    BOOL RetOk = TRUE;
    STAVMEM_InitParams_t InitAvmem;
    STAVMEM_MemoryRange_t WorkIn[1];
    STAVMEM_CreatePartitionParams_t CreateParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;

    /* Now initialise AVMEM and open a handle */
    InitAvmem.MaxPartitions = 1;
    InitAvmem.MaxBlocks = 58;    /* video: 1 bits buffer + 4 frame buffers
                                    tests video: 2 dest buffer
                                    audio: 1 bits buffer
                                    osd : 50 */
    InitAvmem.MaxForbiddenRanges = 2 /* =0 without OSD */;   /* forbidden ranges not used */
    InitAvmem.MaxForbiddenBorders = 3 /* =1 without OSD */;  /* video will use one forbidden border */
    InitAvmem.CPUPartition_p = SystemPartition;
    InitAvmem.NumberOfMemoryMapRanges = 1;
    InitAvmem.MemoryMapRanges_p = &WorkIn[0];   /* One range: SDRAM */
    WorkIn[0].StartAddr_p = (void *) VIRTUAL_BASE_ADDRESS;
    WorkIn[0].StopAddr_p = (void *) (VIRTUAL_BASE_ADDRESS + VIRTUAL_WINDOW_SIZE - 1);
    InitAvmem.BlockMoveDmaBaseAddr_p = (void *) BM_BASE_ADDRESS;
    InitAvmem.CacheBaseAddr_p = (void *) CACHE_BASE_ADDRESS;
    InitAvmem.VideoBaseAddr_p = (void *) VIDEO_BASE_ADDRESS;
    InitAvmem.SDRAMBaseAddr_p = (void *) SDRAM_BASE_ADDRESS;
    InitAvmem.SDRAMSize = SDRAM_SIZE;
    InitAvmem.NumberOfDCachedRanges = (sizeof(DCacheMap) / sizeof(STBOOT_DCache_Area_t)) - 1;
    InitAvmem.DCachedRanges_p = (STAVMEM_MemoryRange_t *) DCacheMap;

    InitAvmem.DeviceType = STAVMEM_DEVICE_TYPE_VIRTUAL;

    VirtualMapping.PhysicalAddressSeenFromCPU_p    = (void *)PHYSICAL_ADDRESS_SEEN_FROM_CPU;
    VirtualMapping.PhysicalAddressSeenFromDevice_p = (void *)PHYSICAL_ADDRESS_SEEN_FROM_DEVICE;
    VirtualMapping.VirtualBaseAddress_p            = (void *)VIRTUAL_BASE_ADDRESS;
    VirtualMapping.VirtualSize         = VIRTUAL_SIZE;
    VirtualMapping.VirtualWindowOffset = 0;
    VirtualMapping.VirtualWindowSize   = VIRTUAL_WINDOW_SIZE;
    InitAvmem.SharedMemoryVirtualMapping_p = &VirtualMapping;
#ifdef USE_GPDMA
    strcpy(InitAvmem.GpdmaName, STGPDMA_DEVICE_NAME);
#endif /*#ifdef USE_GPDMA*/

    if (STAVMEM_Init(STAVMEM_DEVICE_NAME, &InitAvmem) != ST_NO_ERROR)
    {
        STTBX_Print(("AVMEM_Init() failed !\n"));
        RetOk = FALSE;
    }
    else
    {
        /* Open a partition */
        if (STAVMEM_CreatePartition(STAVMEM_DEVICE_NAME, &CreateParams, &AvmemPartitionHandle) != ST_NO_ERROR)
        {
            STTBX_Print(("AVMEM_CreatePartition() failed !\n"));
            RetOk = FALSE;
        }
        else
        {
            /* Init and Open successed */
            STTBX_Print(("STAVMEM initialized,\trevision=%s\n",STAVMEM_GetRevision() ));
        }
    }

    return(RetOk);

} /* end AVMEM_Init() */

/*******************************************************************************
Name        : AVMEM_Term
Description : Terminate AVMEM
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void AVMEM_Term(void)
{
    ST_ErrorCode_t                  ErrCode = ST_NO_ERROR;
    STAVMEM_TermParams_t            TermParams;
    STAVMEM_DeletePartitionParams_t DeleteParams;

    DeleteParams.ForceDelete =FALSE;
    ErrCode = STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &AvmemPartitionHandle);
    if (ErrCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("Warning !! Still allocated block on %s\n", STAVMEM_DEVICE_NAME));
        DeleteParams.ForceDelete = TRUE;
        STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &AvmemPartitionHandle);
    }
    TermParams.ForceTerminate = FALSE;
    ErrCode = STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("Warning !! Still open handle on %s\n", STAVMEM_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrCode = STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermParams);
    }
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error %d in %s STAVMEM Termination\n", ErrCode, STAVMEM_DEVICE_NAME));
    }
} /* end AVMEM_Term() */
#endif /* #ifdef USE_AVMEM */

/*******************************************************************************
Name        : BOOT_Init
Description : Initialise the BOOT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL BOOT_Init(void)
{
    /* Initialise STBOOT */
    InitStboot.CacheBaseAddress = (U32 *)CACHE_BASE_ADDRESS;
    InitStboot.SDRAMFrequency = SDRAM_FREQUENCY;
    InitStboot.DCacheMap = DCacheMap;
    InitStboot.MemorySize = BOOT_MEMORY_SIZE;
    InitStboot.BackendType.DeviceType = BOOT_BACKEND_TYPE;
#ifdef __FULL_DEBUG__
    InitStboot.ICacheEnabled = FALSE;
#else
    InitStboot.ICacheEnabled = TRUE;
#endif

    InitStboot.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    InitStboot.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

    if (STBOOT_Init(STBOOT_DEVICE_NAME, &InitStboot) != ST_NO_ERROR)
    {
        printf("STBOOT_Init() failed !\n");
        exit(0);
    }
    else
    {
        printf("STBOOT initialized, \trevision=%s\n", STBOOT_GetRevision());
    }
    return(TRUE);
}

/*******************************************************************************
Name        : BOOT_Term
Description : Terminate the BOOT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void BOOT_Term(void)
{
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    STBOOT_TermParams_t TermParams;

    ErrCode = STBOOT_Term( STBOOT_DEVICE_NAME, &TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error %d in %s STBOOT Termination\n", ErrCode, STBOOT_DEVICE_NAME));
    }
} /* end BOOT_Term() */

#ifdef USE_STCFG
/*****************************************************************************
Name        : CFG_Init
Description : Initialise STCFG
Parameters  : CFG device name
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*****************************************************************************/
static BOOL CFG_Init(void)
{
    STCFG_InitParams_t  CfgInit;
    STCFG_Handle_t      CFGHandle;
    U8                  index;
    ST_ErrorCode_t      Error = ST_NO_ERROR;

    CfgInit.DeviceType = STCFG_DEVICE_5514;
    CfgInit.Partition_p = SystemPartition;
    CfgInit.BaseAddress_p = (U32*) ST5514_CFG_BASE_ADDRESS;
    if (CfgInit.BaseAddress_p == NULL)
    {
       STTBX_Print(("***ERROR: Invalid device base address OR simulator memory allocation error\n\n"));
       return(false);
    }

    Error = STCFG_Init("cfg", &CfgInit);
    if (Error == ST_NO_ERROR)
    {
        Error = STCFG_Open("cfg", NULL, &CFGHandle);
        if (Error == ST_NO_ERROR)
        {
            /* CDREQ configuration for PTIs A, B and C */
            for (index = STCFG_PTI_A; index <= STCFG_PTI_C; index++)
            {
                Error |= STCFG_CallCmd(
                        CFGHandle,
                        STCFG_CMD_PTI_CDREQ_CONNECT,
                        index,
                        STCFG_PTI_CDIN_0,
                        STCFG_CDREQ_VIDEO_INT);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print((" Unable to set PTI CDREQ connect in STCFG \n" ));
                }
                Error |= STCFG_CallCmd(
                        CFGHandle,
                        STCFG_CMD_PTI_PCR_CONNECT,
                        index,
                        STCFG_PCR_27MHZ_A);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print((" Unable to set PTI PCR connect in STCFG \n"));
                }
            }

            /* Enable video DACs */
            if (Error == ST_NO_ERROR)
            {
                Error = STCFG_CallCmd(
                        CFGHandle,
                        STCFG_CMD_VIDEO_ENABLE,
                        STCFG_VIDEO_DAC_ENABLE);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print((" Unable to enable Video DAC in STCFG \n"));
                }
            }

            /* Power down audio DACs */
            if (Error == ST_NO_ERROR)
            {
                Error = STCFG_CallCmd(
                        CFGHandle,
                        STCFG_CMD_AUDIO_ENABLE,
                        STCFG_AUDIO_DAC_POWER_OFF);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print((" Unable to power down audio DACs in STCFG \n"));
                }
            }

            /* Enable PIO4<4> as IrDA Data not IRDA Control */
            if (Error == ST_NO_ERROR)
            {
                Error = STCFG_CallCmd(
                        CFGHandle,
                        STCFG_CMD_PIO_ENABLE,
                        STCFG_PIO4_BIT4_IRB_NOT_IRDA);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print((" Unable to enable PIO4<4> as IrDA Data not IRDA Control in STCFG \n"));
                }
            }
        }
        else
        {
           STTBX_Print((" Unable to open STCFG \n"));
        }
    }
    else
    {
        STTBX_Print((" Unable to initialise STCFG \n"));
    }

    if (Error == ST_NO_ERROR)
    {
        STTBX_Print(("STCFG    : %-21.21s ", STCFG_GetRevision()));
        STTBX_Print(("Initialized\n"));
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}
#endif /* #ifdef USE_STCFG */

#ifdef USE_CLKRV
/*******************************************************************************
Name        : CLKRV_Init
Description : Initialise the STCLKRV driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL CLKRV_Init(void)
{
    BOOL RetOk = TRUE;

    STCLKRV_InitParams_t stclkrv_InitParams_s;
    STCLKRV_OpenParams_t stclkrv_OpenParams;

    /* Initialization & Open code fragments */
    /* assumed that all dependent drivers are already Initialised */
    strcpy(stclkrv_InitParams_s.PWMDeviceName, STPWM_DEVICE_NAME); /* same as Init */
    stclkrv_InitParams_s.VCXOChannel = CLKRV_VCXO_CHANNEL;
    stclkrv_InitParams_s.InitialMark = STCLKRV_INIT_MARK_VALUE;
    stclkrv_InitParams_s.MinimumMark = STCLKRV_MIN_MARK_VALUE;
    stclkrv_InitParams_s.MaximumMark = STCLKRV_MAX_MARK_VALUE;
    stclkrv_InitParams_s.ErrorThreshold = STCLKRV_ERROR_THRESHOLD;
    stclkrv_InitParams_s.PCRMaxGlitch = STCLKRV_PCR_MAX_GLITCH;
    stclkrv_InitParams_s.PCRDriftThres = STCLKRV_PCR_DRIFT_THRES;
    stclkrv_InitParams_s.TicksPerMark = STCLKRV_TICKS_PER_MARK;
    stclkrv_InitParams_s.SampleNum = STCLKRV_NB_PCR_TO_SAMPLE;
    stclkrv_InitParams_s.StateChangeNum = STCLKRV_NB_PCR_BEFORE_SLOW_TRACK;
    stclkrv_InitParams_s.FreqDiffThres = STCLKRV_PCR_STC_FREQ_DIFF_THRES;
    strcpy( stclkrv_InitParams_s.EVTDeviceName, STEVT_DEVICE_NAME);
    stclkrv_InitParams_s.DeviceType = STCLKRV_DEVICE_PWMOUT;
    stclkrv_InitParams_s.Partition_p = SystemPartition;
    strcpy( stclkrv_InitParams_s.PCREvtHandlerName, STEVT_DEVICE_NAME);
    if( STCLKRV_Init(STCLKRV_DEVICE_NAME, &stclkrv_InitParams_s) != ST_NO_ERROR )
    {
        RetOk = FALSE;
        STTBX_Print(("CLKRV_Init() failed !\n"));
    }
    else
    {
        if( STCLKRV_Open(STCLKRV_DEVICE_NAME, &stclkrv_OpenParams, &CLKRVHndl0) != ST_NO_ERROR)
        {
            STTBX_Print(("CLKRV_Open() failed !\n"));
            RetOk = FALSE;
        }
        else
        {
            STTBX_Print(("STCLKRV initialized, \trevision=%s\n", STCLKRV_GetRevision() ));
        }
    }
    return(RetOk);

} /* end CLKRV_Init() */

/*******************************************************************************
Name        : CLKRV_Term
Description : Terminate the STCLKRV driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void CLKRV_Term(void)
{
    ST_ErrorCode_t       ErrCode = ST_NO_ERROR;
    STCLKRV_TermParams_t TermParams;

    STCLKRV_Close(CLKRVHndl0);
    TermParams.ForceTerminate = FALSE;
    ErrCode = STCLKRV_Term(STCLKRV_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("Warning !! Still open handle on %s\n", STCLKRV_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrCode = STCLKRV_Term(STCLKRV_DEVICE_NAME, &TermParams);
    }
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error %d in %s STCLKRV Termination\n", ErrCode, STCLKRV_DEVICE_NAME));
    }
} /* end CLKRV_Term() */
#endif /* #ifdef USE_CLKRV */

#ifdef USE_EXTVIN
/*******************************************************************************
Name        : ConfigurePWM
Description : Configure the PWM to have chroma
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL ConfigurePWM(void)
{
    STPWM_OpenParams_t OpenParams;
    U32 MarkValue, ReturnMark;
    BOOL RetOk = TRUE;
    ST_ErrorCode_t ErrorCode;
    STPWM_Handle_t PWMHandle2;

    STTBX_Print(("Configuring PWM: "));

    /* open PWM channel 2 */
    OpenParams.C4.ChannelNumber = STPWM_CHANNEL_2;
    ErrorCode = STPWM_Open(STPWM_DEVICE_NAME, &OpenParams, &PWMHandle2);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* error handling */
        STTBX_Print(("PWM 2 Open failed ! Error %x\n", ErrorCode));
        RetOk = FALSE;
    }
    else
    {
        STTBX_Print(("Open 2 Ok, "));

        /* Set PWM Mark to Space Ratio */ MarkValue = STPWM_MARK_MAX / 2;
        ErrorCode = STPWM_SetRatio(PWMHandle2, MarkValue);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* error handling */
            STTBX_Print(("PWM 2 SetRatio failed ! Error %x\n", ErrorCode));
            RetOk = FALSE;
        }
        else
        {
            ErrorCode = STPWM_GetRatio( PWMHandle2, &ReturnMark);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* error handling */
                STTBX_Print(("PWM 2 GetRatio failed ! Error %x\n", ErrorCode));
                RetOk = FALSE;
            }
            else
            {
                STTBX_Print(("PWM 2 Mark Value %d.\n", ReturnMark));
            }
        } /* SetRatio OK */
    } /* Open() OK */
    return(RetOk);
}
#endif /* #ifdef USE_EXTVIN */

#ifdef DUMP_REGISTERS
/*-------------------------------------------------------------------------
* Function : DUMPREG_RegisterCmd
*            Init Testtool commands for DUMP registers * Input    : N/A
* Output   : N/A
* Return   : BOOL (FALSE if OK)
*
----------------------------------------------------------------------*/
static BOOL DUMPREG_RegisterCmd(void)
{
    BOOL RetErr;
    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand("REG_LIST",DUMPREG_ListReg,"<RegisterName> List register(s)");
    RetErr |= STTST_RegisterCommand("REG_READ",DUMPREG_Read,"<RegisterName | RegisterAddress> Read register(s) content");
    RetErr |= STTST_RegisterCommand("REG_WRITE",DUMPREG_Write,"<RegisterName> <Value> Write value in register(s)");
    RetErr |= STTST_RegisterCommand("REG_DUMP",DUMPREG_Dump,"<RegisterName> <FileName> Dump register(s) contents in file");
    if ( RetErr == TRUE )
    {
        STTBX_Print(("DUMPREG_RegisterCmd() : failed !\n" ));
    }
    else
    {
        STTBX_Print(("DUMPREG_RegisterCmd() : ok\n"));
    }

    return(!(RetErr));
} /* DUMPREG_InitCommand () */
#endif /* #ifdef DUMP_REGISTERS */


#ifdef USE_EVT
/*******************************************************************************
Name        : EVT_Init
Description : Initialise the EVT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL EVT_Init(void)
{
    BOOL RetOk = TRUE;
    STEVT_InitParams_t EVTInitParams;

    EVTInitParams.MemoryPartition = SystemPartition;
    EVTInitParams.EventMaxNum = 100;
    EVTInitParams.ConnectMaxNum = 80;
    EVTInitParams.SubscrMaxNum = 100;
    if ((STEVT_Init(STEVT_DEVICE_NAME, &EVTInitParams)) != ST_NO_ERROR)
    {
        RetOk = FALSE;
        STTBX_Print(("EVT Init failed !\n"));
    }

    if (RetOk == TRUE)
    {
        STTBX_Print(("EVT initialized, \trevision=%s\n", STEVT_GetRevision() ));
    }
    return(RetOk);
} /* end EVT_Init() */

/*******************************************************************************
Name        : EVT_Term
Description : Terminate the EVT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void EVT_Term(void)
{
    ST_ErrorCode_t     ErrCode = ST_NO_ERROR;
    STEVT_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;
    ErrCode = STEVT_Term(STEVT_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("Warning !! Still open handle on %s\n", STEVT_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrCode = STEVT_Term(STEVT_DEVICE_NAME, &TermParams);
    }
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error %d in %s STEVT Termination\n", ErrCode, STEVT_DEVICE_NAME));
    }
} /* end EVT_Term() */
#endif /* #ifdef USE_EVT */

#ifdef USE_GPDMA
/*******************************************************************************
Name        : GPDMA_Init
Description : Initialize the GPDMA driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL GPDMA_Init(void)
{
    BOOL RetOk = TRUE;
    STGPDMA_InitParams_t InitParams;

    InitParams.DeviceType = STGPDMA_DEVICE_GPDMAC;
    strcpy( InitParams.EVTDeviceName, STEVT_DEVICE_NAME);
    InitParams.DriverPartition = DriverPartition;
    InitParams.BaseAddress = (U32 *) GPDMA_BASE_ADDRESS;
    InitParams.InterruptNumber = GPDMA2_INTERRUPT;
    InitParams.InterruptLevel = 5;
    InitParams.ChannelNumber = 2;
    InitParams.FreeListSize = 2;
    InitParams.MaxHandles = 2;
    InitParams.MaxTransfers = 4;

    if(STGPDMA_Init(STGPDMA_DEVICE_NAME, &InitParams ) != ST_NO_ERROR)
    {
        STTBX_Print(("GPDMA Init failed !\n"));
        RetOk = FALSE;
    }
    else
    {
        STTBX_Print(("GPDMA initialized, \trevision=%s\n", STGPDMA_GetRevision() ));
    }

    return(RetOk);
} /* end of GPDMA_Init() */

/*******************************************************************************
Name        : GPDMA_Term
Description : Terminate the GPDMA driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void GPDMA_Term(void)
{
    ST_ErrorCode_t       ErrCode = ST_NO_ERROR;
    STGPDMA_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;
    ErrCode = STGPDMA_Term(STGPDMA_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("Warning !! Still open handle on %s\n", STGPDMA_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrCode = STGPDMA_Term(STGPDMA_DEVICE_NAME, &TermParams);
    }
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error %d in %s STGPDMA Termination\n", ErrCode, STGPDMA_DEVICE_NAME));
    }
} /* end GPDMA_Term() */
#endif /*#ifdef USE_GPDMA*/


#ifdef USE_I2C
/*******************************************************************************
Name        : I2C_Init
Description : Initialise the I2C driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL I2C_Init(void)
{
    BOOL RetOk = TRUE;

    STI2C_InitParams_t I2CInitParams;

    I2CInitParams.BaseAddress = (U32 *)BACK_I2C_BASE;
    strcpy(I2CInitParams.PIOforSDA.PortName, PIO_FOR_SDA_PORTNAME);
    strcpy(I2CInitParams.PIOforSCL.PortName, PIO_FOR_SCL_PORTNAME);
    I2CInitParams.PIOforSDA.BitMask = PIO_FOR_SDA_BITMASK;
    I2CInitParams.PIOforSCL.BitMask = PIO_FOR_SCL_BITMASK;
    I2CInitParams.InterruptNumber   = SSC_0_INTERRUPT;
    I2CInitParams.InterruptLevel    = SSC_0_INTERRUPT_LEVEL;
    I2CInitParams.MasterSlave       = STI2C_MASTER;
    I2CInitParams.BaudRate          = STI2C_RATE_NORMAL;      /* Fast is STI2C_RATE_FASTMODE */
    I2CInitParams.ClockFrequency    = ST_GetClockSpeed();
    I2CInitParams.MaxHandles        = 4;
    I2CInitParams.DriverPartition   = DriverPartition;
    strcpy(I2CInitParams.EvtHandlerName, STEVT_DEVICE_NAME);
    if ((STI2C_Init(STI2C_BACK_DEVICE_NAME, &I2CInitParams)) != ST_NO_ERROR)
    {
        RetOk = FALSE;
        STTBX_Print(("I2C Back Init failed !\n"));
    }
    else{
        STTBX_Print(("I2C back initialized, \trevision=%s\n", STI2C_GetRevision() ));
    }
    return(RetOk);
} /* end I2C_Init() */

/*******************************************************************************
Name        : I2C_Term
Description : Terminate the I2C driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void I2C_Term(void)
{
    ST_ErrorCode_t     ErrCode = ST_NO_ERROR;
    STI2C_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;
    ErrCode = STI2C_Term(STI2C_BACK_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("Warning !! Still open handle on %s\n", STI2C_BACK_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrCode = STI2C_Term(STI2C_BACK_DEVICE_NAME, &TermParams);
    }
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error %d in %s STI2C Termination\n", ErrCode, STI2C_BACK_DEVICE_NAME));
    }
} /* end I2C_Term() */
#endif /* #ifdef USE_I2C */

#ifdef USE_INTMR
/*******************************************************************************
Name        : INTMR_Init
Description : Initialise STINTMR
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL INTMR_Init(void)
{
    BOOL RetOk = TRUE;

    ST_ErrorCode_t       ErrCode = ST_NO_ERROR;
    STINTMR_InitParams_t InitStIntMr;

    /* Initialise STINTMR */
    InitStIntMr.DeviceType      = INTMR_DEVICE_TYPE;
    InitStIntMr.Partition_p     = SystemPartition;
    InitStIntMr.BaseAddress_p   = (void *)INTMR_BASE_ADDRESS;
    InitStIntMr.InterruptNumber = INTMR_INTERRUPT_NUMBER;
    InitStIntMr.InterruptLevel  = INTMR_INTERRUPT_LEVEL;
    strcpy( InitStIntMr.EvtDeviceName, STEVT_DEVICE_NAME);

    if (STINTMR_Init(STINTMR_DEVICE_NAME, &InitStIntMr) != ST_NO_ERROR)
    {
        STTBX_Print(("STINTMR failed to initialise !\n"));
        RetOk = FALSE;
    }
    else

    {
        STTBX_Print(("STINTMR initialized, \trevision=%s", STINTMR_GetRevision() ));
        ErrCode = STINTMR_Enable(STINTMR_DEVICE_NAME);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print((", but failed to enable interrupts !\n"));
            RetOk = FALSE;
        }
        else
        {
            STTBX_Print((" and enabled.\n"));
#if defined (ST_GX1)
            {
                U32 i;
                for(i=0;i<3;i++)
                {
                    /* Setting interrupt mask for VTG(0), DVP1(1) & DVP2(2) */
                    ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, i, (0x10000000)<<i);
                    if(ErrCode != ST_NO_ERROR)
                    {
                        STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",i,ErrCode));

                    }
                }
            }
#endif /* ST_GX1 */
        }
    }

    return(RetOk);
} /* end INTMR_Init() */

/*******************************************************************************
Name        : INTMR_Term
Description : Terminate STINTMR
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void INTMR_Term(void)
{
    ST_ErrorCode_t       ErrCode = ST_NO_ERROR;
    STINTMR_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;
    ErrCode = STINTMR_Term(STINTMR_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("Warning !! Still open handle on %s\n", STINTMR_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrCode = STINTMR_Term(STINTMR_DEVICE_NAME, &TermParams);
    }
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error %d in %s STINTMR Termination\n", ErrCode, STINTMR_DEVICE_NAME));
    }
} /* end INTMR_Term() */
#endif /* #ifdef USE_INTMR */

#ifdef USE_STV6410
/*******************************************************************************
Name        : STV6410_Init
Description : Set up the STV6410 composant by the I2C port.
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL STV6410_Init(void)
{
    STI2C_Handle_t     I2cHandle;
    STI2C_OpenParams_t I2cOpenParams;
    ST_ErrorCode_t     ErrCode = ST_NO_ERROR;
#define TEST_BUFFER_SIZE 32

    U8      Buffer[TEST_BUFFER_SIZE];
    U32     ActLen;
    BOOL RetOk = TRUE;
    U8 index;

    /* Setup OpenParams structure here */
    I2cOpenParams.I2cAddress        = STV6410_I2C_ADDRESS;          /* address of remote I2C device */
    I2cOpenParams.AddressType       = STI2C_ADDRESS_7_BITS;
    I2cOpenParams.BusAccessTimeOut  = 100000;

    ErrCode = STI2C_Open (STI2C_BACK_DEVICE_NAME, &I2cOpenParams, &I2cHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STI2C_Open(%s) failed !\n", STI2C_BACK_DEVICE_NAME));
        RetOk = FALSE;
    }
    else
    {
        /* Write to a device */
        for (index = 0 ; index < 32 ; index+=1)
        {
            Buffer[index]=0x0;
        }
        Buffer[0] = 0x00;  Buffer[1] = 0x05;  Buffer[2] = 0x01;  Buffer[3] = 0x05;
        Buffer[4] = 0x02;  Buffer[5] = 0x00;  Buffer[6] = 0x03;  Buffer[7] = 0x68;
        Buffer[8] = 0x04;  Buffer[9] = 0x53;  Buffer[10] = 0x05; Buffer[11] = 0x00;
        Buffer[12] = 0x06; Buffer[13] = 0x00; Buffer[14] = 0x07; Buffer[15] = 0x00;
        Buffer[16] = 0x08; Buffer[17] = 0x03;
        for (index = 0 ; index < 18 ; index+=2)
        {
            if (STI2C_Write ( I2cHandle, &Buffer[index], 2, STI2C_TIMEOUT_INFINITY, &ActLen) != ST_NO_ERROR)
            {
                STTBX_Print(("STI2C_Write() failed !\n"));
                RetOk = FALSE;
            }
        }

        if (STI2C_Close (I2cHandle) != ST_NO_ERROR)
        {
            STTBX_Print(("STI2C_Close() failed !\n"));
            RetOk = FALSE;
        }
    }

    if ( RetOk == FALSE )
    {
        STTBX_Print(( "STV6410 configuration failed !\n" ));
    }
    else
    {
        STTBX_Print(( "STV6410 initialized \n" ));
    }
    return(RetOk);

} /* end STV6410_Init() */
#endif /* #ifdef USE_STV6410 */

#ifdef USE_PIO
/*******************************************************************************
Name        : PIO_Init
Description : Initialise the PIO driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL PIO_Init(void)
{
    BOOL RetOk = TRUE;
    U8 i;

    /* Setup PIO global parameters */
    PioInitParams[0].BaseAddress = (U32 *)PIO_0_BASE_ADDRESS;
    PioInitParams[0].InterruptNumber    = PIO_0_INTERRUPT;
    PioInitParams[0].InterruptLevel     = PIO_0_INTERRUPT_LEVEL;

    PioInitParams[1].BaseAddress = (U32 *)PIO_1_BASE_ADDRESS;
    PioInitParams[1].InterruptNumber    = PIO_1_INTERRUPT;
    PioInitParams[1].InterruptLevel     = PIO_1_INTERRUPT_LEVEL;

    PioInitParams[2].BaseAddress = (U32 *)PIO_2_BASE_ADDRESS;
    PioInitParams[2].InterruptNumber    = PIO_2_INTERRUPT;
    PioInitParams[2].InterruptLevel     = PIO_2_INTERRUPT_LEVEL;

    PioInitParams[3].BaseAddress = (U32 *)PIO_3_BASE_ADDRESS;
    PioInitParams[3].InterruptNumber    = PIO_3_INTERRUPT;
    PioInitParams[3].InterruptLevel     = PIO_3_INTERRUPT_LEVEL;

    PioInitParams[4].BaseAddress = (U32 *)PIO_4_BASE_ADDRESS;
    PioInitParams[4].InterruptNumber    = PIO_4_INTERRUPT;
    PioInitParams[4].InterruptLevel     = PIO_4_INTERRUPT_LEVEL;

#if (NUMBER_PIO > 5)
    PioInitParams[5].BaseAddress = (U32 *)PIO_5_BASE_ADDRESS;
    PioInitParams[5].InterruptNumber    = PIO_5_INTERRUPT;
    PioInitParams[5].InterruptLevel     = PIO_5_INTERRUPT_LEVEL;
#endif

    /* Initialize all pio devices */
    for (i = 0; i < NUMBER_PIO; i++)
    {
        PioInitParams[i].DriverPartition = DriverPartition;
        if (STPIO_Init(PioDeviceName[i], &PioInitParams[i]) != ST_NO_ERROR)
        {
            RetOk = FALSE;
            printf("%s Init failed !\n", PioDeviceName[i]);
        }
    }
    if (RetOk == TRUE)
    {
        printf("PIO 0-%d initialized,\trevision=%s\n", NUMBER_PIO-1, STPIO_GetRevision());
    }
    return(RetOk);
} /* end PIO_Init() */

/*******************************************************************************
Name        : PIO_Term
Description : Terminate the PIO driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void PIO_Term(void)
{
    ST_ErrorCode_t     ErrCode = ST_NO_ERROR;
    STPIO_TermParams_t TermParams;
    U8 i;

    for (i = 0; i < NUMBER_PIO; i++)
    {
        TermParams.ForceTerminate = FALSE;
        ErrCode = STPIO_Term(PioDeviceName[i], &TermParams);
        if (ErrCode == ST_ERROR_OPEN_HANDLE)
        {
            printf("Warning !! Still open handle on %s\n", PioDeviceName[i]);
            TermParams.ForceTerminate = TRUE;
            ErrCode = STPIO_Term(PioDeviceName[i], &TermParams);
        }
        if (ErrCode != ST_NO_ERROR)
        {
            printf("Error %d in %s STPIO Termination\n", ErrCode, PioDeviceName[i]);
        }
    }
} /* end PIO_Term() */
#endif /* #ifdef USE_PIO */

#ifdef USE_PWM
/*******************************************************************************
Name        : PWM_Init
Description : Initialise the PWM driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL PWM_Init(void)
{
    BOOL RetOk = TRUE;
    STPWM_InitParams_t PWMInitParams_s;

    /* Initialize the sub-ordinate PWM Driver
       for Channel 0 only (VCXO fine control) */
    PWMInitParams_s.Device = STPWM_C4;
    PWMInitParams_s.C4.BaseAddress = (U32 *) PWM_BASE_ADDRESS;
    PWMInitParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN;
    strcpy(PWMInitParams_s.C4.PIOforPWM0.PortName, PIO_FOR_PWM0_PORTNAME);
    PWMInitParams_s.C4.PIOforPWM0.BitMask        = PIO_FOR_PWM0_BITMASK;
    strcpy(PWMInitParams_s.C4.PIOforPWM1.PortName, PIO_FOR_PWM1_PORTNAME);
    PWMInitParams_s.C4.PIOforPWM1.BitMask        = PIO_FOR_PWM1_BITMASK;
    strcpy(PWMInitParams_s.C4.PIOforPWM2.PortName, PIO_FOR_PWM2_PORTNAME);
    PWMInitParams_s.C4.PIOforPWM2.BitMask        = PIO_FOR_PWM2_BITMASK;

    if ((STPWM_Init(STPWM_DEVICE_NAME, &PWMInitParams_s)) != ST_NO_ERROR)
    {
        RetOk = FALSE;
        STTBX_Print(("STPWM_Init() failed !\n"));
    }
    else
    {
        STTBX_Print(("STPWM initialized, \trevision=%s\n", STPWM_GetRevision() ));
    }
    return(RetOk);
} /* end PWM_Init() */

/*******************************************************************************
Name        : PWM_Term
Description : Terminate the PWM driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void PWM_Term(void)
{
    ST_ErrorCode_t     ErrCode = ST_NO_ERROR;
    STPWM_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;
    ErrCode = STPWM_Term(STPWM_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("Warning !! Still open handle on %s\n", STPWM_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrCode = STPWM_Term(STPWM_DEVICE_NAME, &TermParams);
    }
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error %d in %s STPWM Termination\n", ErrCode, STPWM_DEVICE_NAME));
    }
} /* end PWM_Term() */
#endif /* #ifdef USE_PWM */

#ifdef USE_UART
/*******************************************************************************
Name        : UART_Init
Description : Initialise the UART driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL UART_Init(void)
{
    BOOL RetOk = TRUE;
    ST_ErrorCode_t Error;
    STUART_InitParams_t UartInitParams;
    STUART_Params_t UartDefaultParams;
#ifdef TRACE_UART
    STUART_OpenParams_t UartOpenParams;
#endif /* #ifdef TRACE_UART */

    /* Initialise UART */
    memset(&UartInitParams, 0, sizeof(UartInitParams));
    UartInitParams.RXD.PortName[0] = '\0';
    UartInitParams.TXD.PortName[0] = '\0';
    UartInitParams.RTS.PortName[0] = '\0';
    UartInitParams.CTS.PortName[0] = '\0';

    UartInitParams.UARTType        = ASC_DEVICE_TYPE;
    UartInitParams.DriverPartition = DriverPartition;
    UartInitParams.BaseAddress     = (U32 *)UART_BASEADDRESS;
    UartInitParams.InterruptNumber = UART_IT_NUMBER;
    UartInitParams.InterruptLevel  = UART_IT_LEVEL;
    UartInitParams.ClockFrequency  = ST_GetClockSpeed();
    UartInitParams.SwFIFOEnable    = TRUE;
    UartInitParams.FIFOLength      = 4096;
    strcpy(UartInitParams.RXD.PortName, PioDeviceName[UART_RXD_PORTNAME_IDX]);
    UartInitParams.RXD.BitMask = UART_RXD_BITMASK;
    strcpy(UartInitParams.TXD.PortName, PioDeviceName[UART_TXD_PORTNAME_IDX]);
    UartInitParams.TXD.BitMask = UART_TXD_BITMASK;
    strcpy(UartInitParams.RTS.PortName, PioDeviceName[UART_RTS_PORTNAME_IDX]);
    UartInitParams.RTS.BitMask = UART_RTS_BITMASK;
    strcpy(UartInitParams.CTS.PortName, PioDeviceName[UART_CTS_PORTNAME_IDX]);
    UartInitParams.CTS.BitMask = UART_CTS_BITMASK;

    /* set default values */
    UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
    UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
    UartDefaultParams.RxMode.BaudRate = UART_BAUD_RATE;
    UartDefaultParams.TxMode.BaudRate = UART_BAUD_RATE;
    UartDefaultParams.RxMode.TermString = NULL;
    UartDefaultParams.RxMode.TimeOut = 1; /* As short as possible */
    UartDefaultParams.RxMode.NotifyFunction = NULL; /* No callback */
    UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
    UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
    UartDefaultParams.TxMode.TermString = NULL;
    UartDefaultParams.TxMode.TimeOut = 0; /* No time-out */
    UartDefaultParams.TxMode.NotifyFunction = NULL; /* No callback */
    UartDefaultParams.SmartCardModeEnabled = FALSE;
    UartDefaultParams.GuardTime = 0;
    UartInitParams.DefaultParams = &UartDefaultParams;

    Error = STUART_Init(STUART_DEVICE_NAME, &UartInitParams);
    if (Error != ST_NO_ERROR)
    {
        /* error handling */
        RetOk = FALSE;
        printf("UART Init failed ! Error %x\n", Error);
    }
    else
    {
        printf("STUART initialized, \trevision=%s\n", STUART_GetRevision() );
    }
#ifdef TRACE_UART
    UartOpenParams.LoopBackEnabled = FALSE;
    UartOpenParams.FlushIOBuffers = TRUE;
    UartOpenParams.DefaultParams = NULL;    /* Don't overwrite default parameters set at init */

    Error = STUART_Open(STUART_DEVICE_NAME, &UartOpenParams, &TraceUartHandle);
    if (Error != ST_NO_ERROR)
    {
        /* error handling */
        RetOk = FALSE;
        printf("UART Open failed ! Error %x\n", Error);
    }
#endif /* #ifdef TRACE_UART */
    return(RetOk);
} /* end UART_Init() */


/*******************************************************************************
Name        : UART_Term
Description : Terminate the UART driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void UART_Term(void)
{
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    STUART_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;

#ifdef TRACE_UART
    ErrCode = STUART_Close(TraceUartHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        /* error handling */
        RetOk = FALSE;
        printf("UART Close failed ! Error %x\n", ErrCode);
        TermParams.ForceTerminate = TRUE;
    }
#endif /* #ifdef TRACE_UART */

    ErrCode = STUART_Term(STUART_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_ERROR_OPEN_HANDLE)
    {
        printf("Warning !! Still open handle on %s\n", STUART_DEVICE_NAME);
        TermParams.ForceTerminate = TRUE;
        ErrCode = STUART_Term(STUART_DEVICE_NAME, &TermParams);
    }
    if (ErrCode != ST_NO_ERROR)
    {
        printf("Error %d in %s STUART Termination\n", ErrCode, STUART_DEVICE_NAME);
    }
} /* end UART_Term() */
#endif /* #ifdef USE_UART */


#ifdef USE_TBX
/*******************************************************************************
Name        : TBX_Init
Description : Initialise the STTBX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL TBX_Init(void)
{
    STTBX_InitParams_t  STTBXInitParams;
    STTBX_BuffersConfigParams_t STTBXBuffParams;
    BOOL RetOk = TRUE;

    STTBXInitParams.SupportedDevices = TBX_SUPPORTED_DEVICE ;
    STTBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    STTBXInitParams.DefaultInputDevice = STTBX_DEVICE_DCU;
    strcpy(STTBXInitParams.UartDeviceName,STUART_DEVICE_NAME);
    STTBXInitParams.CPUPartition_p = SystemPartition;

    STTBXBuffParams.KeepOldestWhenFull = TRUE;

    if ((STTBX_Init(STTBX_DEVICE_NAME, &STTBXInitParams)) == ST_NO_ERROR)
    {
        STTBX_Print(("STTBX initialized, \trevision=%s\n",STTBX_GetRevision()));
        STTBXBuffParams.Enable = FALSE;
        STTBX_SetBuffersConfig (&STTBXBuffParams);
    }
    else
    {
        printf("STTBX_Init() failed !\n");
        RetOk = FALSE;
    }
    return(RetOk);
} /* End of TBX_Init() */

/*******************************************************************************
Name        : TBX_Term
Description : Initialise the STTBX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void TBX_Term(void)
{
    ST_ErrorCode_t     ErrCode = ST_NO_ERROR;
    STTBX_TermParams_t TermParams;

    ErrCode = STTBX_Term( STTBX_DEVICE_NAME, &TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error %d in %s STTBX Termination\n", ErrCode, STTBX_DEVICE_NAME));
    }
} /* end TBX_Term() */
#endif /* #ifdef USE_TBX */

/*******************************************************************************
Name        : TST_Init
Description : Initialise the TST driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL TST_Init(void)
{
    BOOL RetOk = TRUE;
    STTST_InitParams_t Testtool_InitParams;

    memset (Testtool_InitParams.InputFileName, 0, sizeof(Testtool_InitParams.InputFileName));
    Testtool_InitParams.CPUPartition_p = SystemPartition;
    Testtool_InitParams.NbMaxOfSymbols = 1000;
    strcpy( Testtool_InitParams.InputFileName, "../../../../scripts/inject.com" );
    if ((STTST_Init(&Testtool_InitParams)) == ST_NO_ERROR)
    {
        STTBX_Print(("STTST initialized, \trevision=%s\n",STTST_GetRevision()));
    }
    else
    {
        STTBX_Print(("STTST_Init() failed !\n"));
        RetOk = FALSE;
    }
    return(RetOk);
} /* End of TST_Init() */


/*-------------------------------------------------------------------------
 * Function : StartInitComponents
 * Input    :
 * Output   :
 * Return   : TRUE if success, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL StartInitComponents(void)
{
    BOOL RetOk;

    printf ( "----------------------------------------------------------------------\n");

    RetOk = BOOT_Init();

#ifdef USE_STCFG
    if( RetOk )
        RetOk = CFG_Init();
#endif

#ifdef USE_PIO
    if( RetOk )
        RetOk = PIO_Init();
#endif

#ifdef USE_UART
    if( RetOk )
        RetOk = UART_Init();
#endif /* #ifdef USE_UART */

#ifdef USE_TBX
    if( RetOk )
        RetOk = TBX_Init();
#endif

    if( RetOk )
        RetOk = TST_Init();

#ifdef USE_EVT
    if( RetOk )
        RetOk = EVT_Init();
#endif

#ifdef USE_INTMR
    if( RetOk)
        RetOk = INTMR_Init();
#endif

#ifdef USE_GPDMA
    if( RetOk)
        RetOk = GPDMA_Init();
#endif

#ifdef USE_AVMEM
    if( RetOk )
        RetOk = AVMEM_Init();
#endif

#ifdef USE_PWM
    if( RetOk )
        RetOk = PWM_Init();
#endif

#ifdef USE_I2C
    if( RetOk )
        RetOk = I2C_Init();
#endif

#ifdef USE_CLKRV
    if( RetOk )
        RetOk = CLKRV_Init();
#endif

#ifdef USE_STV6410
    if( RetOk )
        RetOk = STV6410_Init();
#endif

#ifdef USE_PTI
    if( RetOk )
        RetOk = PTI_InitComponent();
#endif

#ifdef USE_AUDIO
    if( RetOk )
        RetOk = AUD_InitComponent();
#endif

    return( RetOk );
} /* end of StartInitComponents() */

/*-------------------------------------------------------------------------
 * Function : StartTermComponents
 * Input    :
 * Output   :
 * Return   : TRUE if success, else FALSE
 * ----------------------------------------------------------------------*/
static void StartTermComponents(void)
{
#ifdef USE_PTI
    PTI_TermComponent();
#endif

#ifdef USE_CLKRV
    CLKRV_Term();
#endif

#ifdef USE_PWM
    PWM_Term();
#endif

#ifdef USE_AVMEM
    AVMEM_Term();
#endif

#ifdef USE_GPDMA
    GPDMA_Term();
#endif

#ifdef USE_I2C
    I2C_Term();
#endif

#ifdef USE_INTMR
    INTMR_Term();
#endif

#ifdef USE_EVT
    EVT_Term();
#endif

    STTST_Term();

#ifdef USE_TBX
    TBX_Term();
#endif

#ifdef USE_UART
    UART_Term();
#endif

#ifdef USE_PIO
    PIO_Term();
#endif

    BOOT_Term();
} /* end StartTermComponents() */

/*-------------------------------------------------------------------------
 * Function : StartSettingDevices
 *            Set some devices
 * Input    :
 * Output   :
 * Return   : TRUE if success, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL StartSettingDevices(void)
{
    BOOL RetOk;

    RetOk = TRUE;
    STTBX_Print(( "----------------------------------------------------------------------\n" ));
    STTBX_Print(( "Device setting...\n" ));

#ifdef USE_HDD
    RetOk = HDD_Init();
    if ( !RetOk )
    {
        STTBX_Print(("HDD not available !\n"));
    }
    else
    {
        RetOk = HDD_CmdStart();
        if ( !RetOk )
        {
            STTBX_Print(("HDD_CmdStart() failed !\n"));
        }
    }
#endif /* #ifdef USE_HDD */

#ifdef USE_EXTVIN
    if(RetOk)
    {
        /* Configure PWM to have chroma */
        RetOk = ConfigurePWM();
        if (!(RetOk))
        {
            STTBX_Print(("PWM not configured !\n"));
        }
    }
#endif /* mb295 */
    return( RetOk );
} /* end of StartSettingDevices() */

/*-------------------------------------------------------------------------
 * Function : StartRegisterCmds
 *            Create register commands needed for the test
 * Input    :
 * Output   :
 * Return   : TRUE if success, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL StartRegisterCmds(void)
{
    BOOL RetOk = TRUE;
#ifdef TRACE_UART
    ST_ErrorCode_t ErrorCode;
#endif

    STTBX_Print(( "----------------------------------------------------------------------\n" ));
    STTBX_Print(( "Testtool commands registrations...\n" ));

#ifdef DUMP_REGISTERS
    if (RetOk)
        RetOk = DUMPREG_RegisterCmd();
    if( (DUMPREG_Init()) != ST_NO_ERROR)
        RetOk = FALSE;
#endif

/*    if( RetOk )
        RetOk = Test_CmdStart();*/
    if (RetOk)
        RetOk = API_RegisterCmd();

#ifdef USE_PTI
    if( RetOk )
        RetOk = PTI_RegisterCmd();
#endif

#ifdef USE_DENC
    if (RetOk)
        RetOk = DENC_RegisterCmd();
#endif

#ifdef USE_VTG
    if (RetOk)
        RetOk = VTG_RegisterCmd();
#endif

#ifdef USE_VOUT
    if (RetOk)
        RetOk = VOUT_RegisterCmd();
#endif

#ifdef USE_LAYER
    if (RetOk)
        RetOk = LAYER_RegisterCmd();
#endif

#ifdef USE_VMIX
    if (RetOk)
        RetOk = VMIX_RegisterCmd();
#endif

#ifdef USE_AUDIO
    if (RetOk)
        RetOk = AUD_RegisterCmd();
#endif

#ifdef USE_VIDEO
    if (RetOk)
        RetOk = VID_RegisterCmd();
#endif
#ifdef USE_VIDEO_2
    if (RetOk)
        RetOk = VID_RegisterCmd2();
#endif

#ifdef USE_VIN
    if (RetOk)
        RetOk = VIN_RegisterCmd();
#endif

#ifdef USE_EXTVIN
    if (RetOk)
        RetOk = EXTVIN_RegisterCmd();
#endif

#ifdef USE_VBI
    if (RetOk)
        RetOk = VBI_RegisterCmd();
#endif

#ifdef USE_TTX
    if (RetOk)
        RetOk = TTX_RegisterCmd();
#endif

#ifdef USE_CC
    if (RetOk)
        RetOk = CC_RegisterCmd();
#endif

    STTBX_Print(( "----------------------------------------------------------------------\n" ));
#ifdef TRACE_UART
    if (RetOk)
    {
        /* Uart Traces implementation task inits and debug */
        ErrorCode = TraceInit();
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Print(("TraceInit() failed !"));
        }
        /* Testtool's commands for traces */
        ErrorCode = TraceDebug();
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Print(("TraceDebug() failed !"));
        }
    }
#endif
    return(RetOk);
} /* end StartRegisterCmds() */


/*#########################################################################
 *                                 MAIN
 * #######################################################################*/

/*-------------------------------------------------------------------------
 * Function : os20_main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/

void os20_main(void *ptr)
{
    BOOL RetOk;
    BOOL IsDCacheOn;

    /* Create memory partitions */
#ifdef ST_OS20
    partition_init_simple(&TheInternalPartition, (U8*) InternalBlock, sizeof(InternalBlock));
#ifdef ARCHITECTURE_ST20
    partition_init_heap(&TheNCachePartition, (U8*)NCacheMemory, sizeof(NCacheMemory));
#endif
    partition_init_heap(&TheSystemPartition, (U8*) ExternalBlock, sizeof(ExternalBlock));
#endif
#ifdef ST_OS21
    NCachePartition_p = partition_create_heap((void *) NCacheMemory, NCACHE_SIZE);
    SystemPartition_p = partition_create_heap((void *) ExternalBlock, SYSTEM_SIZE);
#endif
    printf("Partition initialized\n");

    RetOk = StartInitComponents();
    if( RetOk )
        RetOk = StartSettingDevices();
    if( RetOk )
        RetOk = StartRegisterCmds();

#if defined(ST_GX1) || defined(ST_NGX1)
    DEV_WRITE_U32(0xbe080068,0x70000000  ); /* Bug OS40, need to change the clear to accept DVP IT */
#endif /* #if defined(ST_GX1) || defined(ST_NGX1) */

#if defined (ST_5514)
    /* Need to program MPEG CONTROL register to
       account for 5514 being in x1 mode */
    STSYS_WriteRegDev32LE(0x5000, 0x70000);
#endif

    if (!RetOk)
    {
        STTBX_Print(("Errors reported during system initialisation\n"));
    }
    else
    {
        STTBX_Print(("No errors during initialisation\n"));
    }
    IsDCacheOn = !( DCacheMap[0].StartAddress==NULL && DCacheMap[0].EndAddress==NULL );
    STTBX_Print(("======================================================================\n"));
    STTBX_Print(("                          Test Application \n"));
    STTBX_Print(("                       %s - %s \n", __DATE__, __TIME__  ));
    STTBX_Print(("                   Chip %s on board %s\n", CHIP_NAME, BOARD_NAME ));
    STTBX_Print(("                    CPU Clock Speed = %d Hz\n", ST_GetClockSpeed() ));
    STTBX_Print(("                     ICache = %s DCache = %s\n", InitStboot.ICacheEnabled == TRUE ? "ON":"OFF", \
                                                                   IsDCacheOn ? "ON":"OFF"));
    STTBX_Print(("======================================================================\n"));
    STTBX_Print(("Hit return to enter Testtool or backspace to exit\n"));

    /* Lowest priority for task root testool */
    task_priority_set( task_id(),MIN_USER_PRIORITY);

    STTST_Start();

    /* Termination */
    StartTermComponents();
} /* end os20_main */

/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
#ifdef ST_OS21
    printf ("\nBOOT ...\n");

    setbuf(stdout, NULL);

    /* Configure GX1 bridge (should be added to STBOOT) */
    #if defined(ST_GX1) || defined(ST_NGX1)

    /* Reset GX1 comms bridge */
    DEV_WRITE_U32(SYS_CONF1_LSW, (1<<25) );
    DEV_WRITE_U32(SYS_CONF1_MSW, (1<<0) );
    DEV_WRITE_U32(SYS_CONF1_LSW, ((U32)((U32)1<<25)|(U32)((U32)1<<31)));
    DEV_WRITE_U32(SYS_CONF1_MSW, ((1<<0)|(1<<6)) );
    DEV_WRITE_U32(SYS_CONF1_LSW, (1<<25) );
    DEV_WRITE_U32(SYS_CONF1_MSW, (1<<0) );

    /* ASC0 does not use PIO.  The pins are shared and must be
     * configured in the SYSCONF area to enable them for ASC0
     * usage.  Note also that the MB317 board allows COM3 port
     * to be connected to either ASC0 or ASC3 -- the test
     * harness assumes ASC0 is used (default).
     */
    DEV_WRITE_U32(SYSCONF_SYS_CON2, (1<<23));

    #endif
    OS20_main(NULL);
#endif
#ifdef ST_OS20
    os20_main(NULL);
#endif

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}

/* End of startup.c module */
