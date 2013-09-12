/****************************************************************************

File Name   : stttxtst.c

Description : Teletext API Test Routines

Copyright (C) 2006, ST Microelectronics

References  :

$ClearCase (VOB: stttx)
ETS 300 706 May 1997 "Enhanced Teletext specification"
ttxt_api.fm "Teletext API" Reference DVD-API-003 Revision 3.4

****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef ST_OS21
#include <ostime.h>
#endif
#include "stlite.h"        /* MUST be included BEFORE stpti.h */
#endif

#include "stdevice.h"

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#include "stboot.h"
#else
#include <time.h>
#endif

#if defined(ST_OSLINUX)
#define STTBX_Print(x) printf x
#include "stos.h"
#else
#include "sttbx.h"
#include "stsys.h"
#include "stpio.h"
#endif
#include "sti2c.h"
#include "stddefs.h"
#include "stttx.h"
#include "stdenc.h"
#include "stevt.h"
#include "stcommon.h"
#include "testtool.h"
#include "stclkrv.h"
#include "stvtg.h"
#include "stvout.h"

#ifndef FEI_BASE_ADDRESS
 #define FEI_BASE_ADDRESS  0  /* For chips without FEI this will allow the test code to be
                                 compiled*/
#endif
 /* FEI Registers */

#define FEI_SOFT_RST                  (FEI_BASE_ADDRESS + 0x00000000 )
#define FEI_ENABLE                    (FEI_BASE_ADDRESS + 0x00000008 )
#define FEI_CONFIG                    (FEI_BASE_ADDRESS + 0x00000010 )
#define FEI_STATUS                    (FEI_BASE_ADDRESS + 0x00000018 )
#define FEI_STATUS_CLR                (FEI_BASE_ADDRESS + 0x00000020 )
#define FEI_OUT_DATA                  (FEI_BASE_ADDRESS + 0x00000028 )
#define FEI_OUT_ERROR                 (FEI_BASE_ADDRESS + 0x00000030 )
#define FEI_SECTOR_SIZE               (FEI_BASE_ADDRESS + 0x00000038 )

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
#include "stavmem.h"
#include "stgxobj.h"
#include "stvmix.h"
#endif

#ifdef ST_OS20
#include "debug.h"      /* For debugopen etc. */
#endif

/* Conditional includes */

#if !defined(STTTX_USE_DENC)
#include "stvbi.h"
#endif

#if defined(STTTX_USE_TUNER)
#include "sttuner.h"
#endif

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) \
    || defined(ST_5528)
#include "stcfg.h"
#include "stpwm.h"

#endif

#if defined(TTXSTPTI_TEST)
#if defined(ST_8010) || defined(ST_5188)
#undef TTXSTPTI_TEST
#define TTXSTDEMUX_TEST
#endif
#endif

/* Link crashes if pti_malloc_buffer is called without init */
#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || \
                                (defined(TTXDISK_TEST) && defined(ST_5518))
    #include "pti.h"
#elif defined(TTXSTPTI_TEST)
    #include "stpti.h"
#elif defined(TTXSTDEMUX_TEST)
    #include "stdemux.h"
    #include "stfdma.h"
#endif

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    #include "sttsmux.h"
#endif
#if defined(ST_5528) || defined (ST_5100) || defined (ST_7710) || defined(ST_7100) || defined(ST_5301) || defined(ST_7109)
    /* Replaces STTSMUX */
    #include "stmerge.h"
#endif

#if defined(ST_5100) ||  defined(ST_5528) || defined(ST_5105) || defined(ST_7710) || defined(ST_5188) || defined(ST_5107)|| defined(ST_7100) || defined(ST_7109)
#include "stvout.h"
#endif

/* Private Types ---------------------------------------------------------- */
#if !defined (ST_7710)
#pragma ST_device(TTX_REG_U8)
#pragma ST_device(TTX_REG_U32)
#endif

typedef volatile U8 TTX_REG_U8;

typedef volatile U32 TTX_REG_U32;

typedef struct
{
    U8  *DriverPesBuffer_p;
    U8  *PesBuffer_p;
    U32 TotalSize;
    U32 PesSize;
#ifdef ST_OS20
    semaphore_t DataSemaphore;
#endif
#if defined(ST_OS20) || defined(ST_OS21)/* linux changes */
    semaphore_t *DataSemaphore_p;
#endif
} InsertParams_t;

/* Private Constants ------------------------------------------------------ */

#define OPEN_HANDLE_WID         8                   /* Open bits width */
#define OPEN_HANDLE_MSK         ( ( 1 << OPEN_HANDLE_WID ) - 1)

#define SERVICE_DISPLAY_PAL 1

#define CLK_REC_TRACKING_INTERRUPT_LEVEL 7

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5107)
#define CLK_REC_TRACKING_INTERRUPT    DCO_CORRECTION_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_5100
#define AUD_PCM_CLK_FS_BASE_ADDRESS   NULL
#elif defined (ST_7710)
#define CLK_REC_TRACKING_INTERRUPT    ST7710_MPEG_CLK_REC_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_7710
#define AUD_PCM_CLK_FS_BASE_ADDRESS   ST7710_AUDIO_IF_BASE_ADDRESS
#elif defined (ST_7100)
#define CLK_REC_TRACKING_INTERRUPT    ST7100_MPEG_CLK_REC_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_7100
#define AUD_PCM_CLK_FS_BASE_ADDRESS   ST7100_AUDIO_IF_BASE_ADDRESS
#elif defined (ST_7109)
#define CLK_REC_TRACKING_INTERRUPT    ST7109_MPEG_CLK_REC_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_7100
#define AUD_PCM_CLK_FS_BASE_ADDRESS   ST7109_AUDIO_IF_BASE_ADDRESS
#endif

#ifdef ST_OSLINUX
#undef  AUD_PCM_CLK_FS_BASE_ADDRESS
#define AUD_PCM_CLK_FS_BASE_ADDRESS   NULL
#undef  CLK_REC_TRACKING_INTERRUPT
#define CLK_REC_TRACKING_INTERRUPT    0
#define LOCAL_PTI_ADDRESS             NULL
#define LOCAL_PTI_INTERRUPT           0
#define ncache_partition              NULL
#endif

#if defined (ST_5105) || defined(ST_5188) || defined(ST_5107)
#define STTTX_DEVICE_TYPE             STTTX_DEVICE_TYPE_5105
#elif defined (ST_7020)
#define STTTX_DEVICE_TYPE             STTTX_DEVICE_TYPE_7020
#else
#define STTTX_DEVICE_TYPE             STTTX_DEVICE_OUTPUT
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
#define TELETEXT_INTERRUPT            0 /*Interrupt number will not be effective as no interrupt
                                        installation in STTTX_Init for 5105 */
#endif

#if defined(ST_5508) || defined(ST_5518) || defined(mb5518)

#define PIO_FOR_SSC0_SDA        PIO_BIT_0
#define PIO_FOR_SSC0_SCL        PIO_BIT_1

#define PIO_FOR_SSC1_SDA        PIO_BIT_0
#define PIO_FOR_SSC1_SCL        PIO_BIT_1

#elif defined(ST_5510) || defined(ST_5512)

#define PIO_FOR_SSC0_SDA        PIO_BIT_0
#define PIO_FOR_SSC0_SCL        PIO_BIT_2

#define PIO_FOR_SSC1_SDA        PIO_BIT_0
#define PIO_FOR_SSC1_SCL        PIO_BIT_2

#else

/* ST_5514 || ST_5516 || ST_5517 || ST_5528 */
#define PIO_FOR_SSC0_SDA        PIO_BIT_0
#define PIO_FOR_SSC0_SCL        PIO_BIT_1

#define PIO_FOR_SSC1_SDA        PIO_BIT_2
#define PIO_FOR_SSC1_SCL        PIO_BIT_3

#endif

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) ||\
    defined(ST_5528) || defined(ST_5100) || defined(ST_5301)

#define     SSC_0_PIO_BASE              PIO_3_BASE_ADDRESS
#define     SSC_0_PIO_INTERRUPT         PIO_3_INTERRUPT
#define     SSC_0_PIO_INTERRUPT_LEVEL   PIO_3_INTERRUPT_LEVEL

#define     SSC_1_PIO_BASE              PIO_3_BASE_ADDRESS
#define     SSC_1_PIO_INTERRUPT         PIO_3_INTERRUPT
#define     SSC_1_PIO_INTERRUPT_LEVEL   PIO_3_INTERRUPT_LEVEL


#else

#define     SSC_0_PIO_BASE              PIO_1_BASE_ADDRESS
#define     SSC_0_PIO_INTERRUPT         PIO_1_INTERRUPT
#define     SSC_0_PIO_INTERRUPT_LEVEL   PIO_1_INTERRUPT_LEVEL

#endif

#if defined(mb5518)
#define     SSC_1_PIO_BASE              PIO_5_BASE_ADDRESS
#define     SSC_1_PIO_INTERRUPT         PIO_5_INTERRUPT
#define     SSC_1_PIO_INTERRUPT_LEVEL   PIO_5_INTERRUPT_LEVEL

#elif defined(ST_5518)

#define     SSC_1_PIO_BASE              PIO_2_BASE_ADDRESS
#define     SSC_1_PIO_INTERRUPT         PIO_2_INTERRUPT
#define     SSC_1_PIO_INTERRUPT_LEVEL   PIO_2_INTERRUPT_LEVEL

#else

#define     SSC_1_PIO_BASE              PIO_3_BASE_ADDRESS
#define     SSC_1_PIO_INTERRUPT         PIO_3_INTERRUPT
#define     SSC_1_PIO_INTERRUPT_LEVEL   PIO_3_INTERRUPT_LEVEL

#endif

/* These values seem to be common across all chips and eval boards. */
#define     TUNER_SSC_PIO_BASE          SSC_1_PIO_BASE
#define     TUNER_SSC_PIO_INTERRUPT     SSC_1_PIO_INTERRUPT
#define     TUNER_SSC_PIO_LEVEL         SSC_1_PIO_INTERRUPT_LEVEL
#define     PIO_FOR_TUNER_SSC_SDA       PIO_FOR_SSC1_SDA
#define     PIO_FOR_TUNER_SSC_SCL       PIO_FOR_SSC1_SCL
#define     TUNER_SSC_BASE              SSC_1_BASE_ADDRESS
#define     TUNER_SSC_INTERRUPT         SSC_1_INTERRUPT
#define     TUNER_SSC_INTERRUPT_LEVEL   SSC_1_INTERRUPT_LEVEL



/*** Defines for dealing with pti ***/
#if defined(ST_5514)

/* Specifically use PTIA */
#ifndef LOCAL_PTI_ADDRESS
#define LOCAL_PTI_ADDRESS               ST5514_PTIA_BASE_ADDRESS
#endif

#ifndef LOCAL_PTI_INTERRUPT
#define LOCAL_PTI_INTERRUPT             ST5514_PTIA_INTERRUPT
#endif

#ifndef LOCAL_STPTI_DEVICE
#define LOCAL_STPTI_DEVICE              STPTI_DEVICE_PTI_1
#endif

#elif defined(ST_5528)

/* Specifically use PTIA */
#ifndef LOCAL_PTI_ADDRESS
#define LOCAL_PTI_ADDRESS               ST5528_PTIA_BASE_ADDRESS
#define LOCAL2_PTI_ADDRESS               ST5528_PTIB_BASE_ADDRESS
#endif

#ifndef LOCAL_PTI_INTERRUPT
#define LOCAL_PTI_INTERRUPT             ST5528_PTIA_INTERRUPT
#define LOCAL2_PTI_INTERRUPT             ST5528_PTIB_INTERRUPT
#endif

#ifndef LOCAL_STPTI_DEVICE
#define LOCAL_STPTI_DEVICE              STPTI_DEVICE_PTI_4
#endif

#elif defined(ST_5100) || defined(ST_5105) || defined(ST_7710) || defined(ST_7100) || defined(ST_5301) || defined(ST_7109) || defined(ST_5107)

/* Specifically use PTIA */
#ifndef LOCAL_PTI_ADDRESS
#define LOCAL_PTI_ADDRESS               PTI_BASE_ADDRESS
#endif

#ifndef LOCAL_PTI_INTERRUPT
#define LOCAL_PTI_INTERRUPT             PTI_INTERRUPT
#endif

#ifndef LOCAL_STPTI_DEVICE
#define LOCAL_STPTI_DEVICE              STPTI_DEVICE_PTI_4
#endif

#else

/* Only one PTI */
#ifndef LOCAL_PTI_ADDRESS
#define LOCAL_PTI_ADDRESS               PTI_BASE_ADDRESS
#endif

#ifndef LOCAL_PTI_INTERRUPT
#define LOCAL_PTI_INTERRUPT             PTI_INTERRUPT
#endif

#ifndef LOCAL_STPTI_DEVICE
#define LOCAL_STPTI_DEVICE              STPTI_DEVICE_PTI_1
#endif

#endif /* ST_5516 || 5517 */

#if defined(mb231) ||  defined(mb282b) || defined(mb295) || defined(mb314) \
 || defined(mb361) || defined(mb382) || defined(mb290) || defined(mb376)
#define CLKRV_VCXO_CHANNEL  STPWM_CHANNEL_0
#elif defined(mb275) || defined(mb275_64)
#define CLKRV_VCXO_CHANNEL  STPWM_CHANNEL_1
#endif

#if defined (ST_7100)
#define VOUT_ADDRESS 		 ST7100_VOUT_BASE_ADDRESS
#elif defined(ST_7109)
#define VOUT_ADDRESS 		 ST7109_VOUT_BASE_ADDRESS
#endif

#if defined(DVD_TRANSPORT_STPTI4)
    #ifndef LOCAL_STPTI_LOADER
        #if defined(ST_7109) || defined(ST_5301)/* || defined(ST_7100)*/
        #define LOCAL_STPTI_LOADER       STPTI_DVBTCLoaderSL
        #elif defined(ST_5105) || defined(ST_5107)
        #define LOCAL_STPTI_LOADER       STPTI_DVBTCLoaderUL
        #elif defined(ST_7100) || defined(ST_5100)
        #define LOCAL_STPTI_LOADER       STPTI_DVBTCLoaderL
        #else
        #define LOCAL_STPTI_LOADER       STPTI_DVBTCLoader
        #endif
    #endif
#elif defined(DVD_TRANSPORT_STPTI)
    #ifndef LOCAL_STPTI_LOADER
        #define LOCAL_STPTI_LOADER       STPTI_DVBTC1LoaderAutoInputcount
    #endif
#endif

#if 0
#if defined(ST_5528)
#if defined (espresso)
static STPTI_StreamID_t STPTI_StreamID = STPTI_STREAM_ID_NOTAGS;
#else
static STPTI_StreamID_t STPTI_StreamID = STPTI_STREAM_ID_TSIN0;
#endif
#elif defined (ST_5100)
static STPTI_StreamID_t STPTI_StreamID = STPTI_STREAM_ID_TSIN1;
#endif
#endif

/* 4629 related base addresses */

#if defined(TTXDUAL_DISK_TEST)  || defined(TTXDUAL_STPTI_TEST)
#ifdef ST_OS20
#define ST4629_DENC_BASE_ADDRESS    (STI4629_BASE_ADDRESS + 0x100)
#define ST4629_TTX_CFG_ADDRESS      (STI4629_BASE_ADDRESS + 0x250)
#elif defined(ST_OS21)
#define ST4629_DENC_BASE_ADDRESS    (STI4629_ST40_BASE_ADDRESS + 0x100)
#define ST4629_TTX_CFG_ADDRESS      (STI4629_ST40_BASE_ADDRESS + 0x250)
#endif
#endif

/*** Other misc. defines ***/

const U8  HarnessRev[] = "3.1.4A2";

#ifndef STTTX_PES_FILE
    #define STTTX_PES_FILE "default.pes"
#endif

#ifndef STTTX_PESBUFFER_SIZE
    #define STTTX_PESBUFFER_SIZE 2000000
#endif

#define MAX_BANDS       3
#define BAND_US         0
#define BAND_EU_LO      1
#define BAND_EU_HI      2

#if defined(STTTX_DEBUG1)
U32 Pes_Lost=0;
extern U32 DmaWaCnt;
extern int StttxBufferSize;
#endif

/* Private Variables ------------------------------------------------------ */
#ifndef ST_OSLINUX
#if defined(TTXDUAL_DISK_TEST)  || defined(TTXDUAL_STPTI_TEST)

#if defined(espresso)
    STBOOT_DCache_Area_t BOOT_DCacheMap = {(U32 *) 0xC0000000, (U32 *) 0xC06FFFFF}; /* 7 MB  */
#else
    STBOOT_DCache_Area_t BOOT_DCacheMap = {NULL, NULL}; /* NA For MB376 From EMI  */
#endif

#else

/* Put here instead of where STBOOT is initialised because that way
 * STAVMEM init can use it as well.
 */
#if !defined(DISABLE_DCACHE)
#if defined CACHEABLE_BASE_ADDRESS
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS }, /* assumed ok */
        { NULL, NULL }
    };
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40200000, (U32 *)0x407FFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5301)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0xC0000000, (U32 *)0xC1FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_8010)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#else
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40080000, (U32 *)0x7FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#endif
#endif /* DISABLE_DCACHE */
#endif /* TTXDUAL_DISK_TEST */
#endif /* ST_OS_LINUX*/

static ST_ClockInfo_t ClockInfo;

ST_ErrorCode_t          retVal;

ST_DeviceName_t         DeviceNam;
#if defined(TTXDUAL_DISK_TEST) || defined(TTXDUAL_STPTI_TEST)
ST_DeviceName_t         DeviceNam4629;
#endif

STTTX_InitParams_t      InitParams_s;
STTTX_OpenParams_t      OpenParams_s;
STTTX_Request_t         FilterPars_s;
STTTX_TermParams_t      TermParams_s;

STTTX_Handle_t          VBIHandle;
#if defined(TTXDUAL_DISK_TEST)  || defined(TTXDUAL_STPTI_TEST)
STTTX_Handle_t          VBIHandle4629;
#endif

STTTX_Handle_t          STBHandle;
STTTX_Handle_t          STBHandle2;

STDENC_Handle_t         DENCHandle;
STVTG_Handle_t          VTGHandle;

#if defined(STTTX_USE_TUNER)
STTUNER_Handle_t        TUNERHandle;
#endif

STPIO_Handle_t          PIOVsyncHandle;
extern U32              VsyncCount;
extern U32              p3;
STTTX_Page_t OddPage,   EvenPage;
BOOL InsertTaskExit  =  FALSE;

/* Device names always present */
static ST_DeviceName_t EVTDeviceName  = "STEVT";
static ST_DeviceName_t VTGDeviceName  = "STVTG";
static ST_DeviceName_t DENCDeviceName = "STDENC";
static ST_DeviceName_t VBIDeviceName  = "STVBI";
#if defined(STTTX_SUBT_SYNC_ENABLE) || defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
static ST_DeviceName_t CLKRVDeviceName  = "STCLKRV";
#endif
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
static ST_DeviceName_t VMIXDeviceName = "STVMIX";
#endif

#if defined(TTXDUAL_DISK_TEST)  || defined(TTXDUAL_STPTI_TEST)
static ST_DeviceName_t DENCDeviceName4629 = "STDENC4629";
static ST_DeviceName_t VTGDeviceName4629  = "STVTG4629";
static ST_DeviceName_t VBIDeviceName4629  = "STVBI4629";
#endif

#if defined(TTXSTDEMUX_TEST)
STDEMUX_Signal_t      STBSignal;
STDEMUX_Slot_t        STBSlot;
STDEMUX_Slot_t        VBISlot;
STDEMUX_Slot_t        PCRSlot;
STDEMUX_Signal_t      STBSignal4629;
STDEMUX_Slot_t        STBSlot4629;
STDEMUX_Slot_t        VBISlot4629;

static ST_DeviceName_t STDEMUXDeviceName = "STDEMUX";
#endif /* TTXSTDEMUX_TEST */

#if defined(TTXSTPTI_TEST)
STPTI_Signal_t      STBSignal;
STPTI_Slot_t        STBSlot;
STPTI_Slot_t        VBISlot;
STPTI_Slot_t        PCRSlot;
STPTI_Signal_t      STBSignal4629;
STPTI_Slot_t        STBSlot4629;
STPTI_Slot_t        VBISlot4629;
static ST_DeviceName_t STPTIDeviceName = "STPTI";
#if defined(TTXDUAL_STPTI_TEST)
static ST_DeviceName_t STPTIDeviceName4629 = "STPTI4629";
#endif
#endif /* TTXSTPTI_TEST */

#ifdef STTTX_SUBT_SYNC_ENABLE
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528)
static ST_DeviceName_t PWMDeviceName  = "STPWM";
#endif
#endif

#if !defined(TTXDISK_TEST)
    #if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
        static ST_DeviceName_t TSMUXName = "STTSMUX";
    #endif
    #if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_7100) || defined(ST_5301) || defined(ST_7109)
        static ST_DeviceName_t TSMERGEName = "STMERGE";
    #endif
#endif

#if defined (ST_5100) || defined (ST_5528) || defined (ST_5105) || defined (ST_7710) ||\
         defined(ST_5188) || defined(ST_5107) || defined (ST_7100) || defined (ST_7109)
ST_DeviceName_t VOUTDeviceName = "VOUT";
#endif

static ST_DeviceName_t PioDeviceName = "PIO3";
/* List of PIO device names */

#if defined(TTXDUAL_DISK_TEST)  || defined(TTXDUAL_STPTI_TEST)
static ST_DeviceName_t PioDeviceNameTTX = "PIO6";
STPIO_OpenParams_t     PIO4629_1OpenParams;
STPIO_Handle_t         GlbPio4629_1;
#endif

#if defined(STTTX_DEBUG1)
static void TeletextNotifyPeslost(STEVT_CallbackProc_t Reason,
                                  STEVT_EventConstant_t Event,
                                  const void *EventData);
#endif

#if defined(TTXDUAL_DISK_TEST)  || defined(TTXDUAL_STPTI_TEST)
void TeletextEvtNotify(STEVT_CallReason_t Reason,
                       const ST_DeviceName_t RegistrantName,
                       STEVT_EventConstant_t Event,
                       const void *EventData,
                       const void *SubscriberData_p
                      );

STEVT_DeviceSubscribeParams_t DeviceSubscribeParams = {
        TeletextEvtNotify,
        NULL};

#endif



/* PIO Init parameters */
static STPIO_InitParams_t PioInitParams;

#if defined(STTTX_USE_TUNER)
/* List of device names */
static ST_DeviceName_t I2CDeviceName   = "front";
static ST_DeviceName_t TUNERDeviceName = "STTUNER";

/* I2C Init params */
static STI2C_InitParams_t I2CInitParams;

/* TUNER Init parameters */
static STTUNER_InitParams_t TunerInitParams;
static STTUNER_Band_t       BandList[10];
static STTUNER_BandList_t   Bands;
static STTUNER_Scan_t       ScanList[10];
static STTUNER_ScanList_t   Scans;

static STTUNER_EventType_t  LastEvent;
#endif

/* Global semaphores used in callbacks */
static semaphore_t *EventSemaphore_p;
static semaphore_t *FilterSemaphore_p;


/* PTI slots for teletext data */
#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || \
    (defined(TTXDISK_TEST) && defined(ST_5518))
slot_t vbi_slot;
slot_t stb_slot;
#endif

#if defined(mb314) && !defined(UNIFIED_MEMORY)
    #define                 NCACHE_MEMORY_SIZE          0x200000
#else
#if defined(TTXDUAL_DISK_TEST) ||  defined(TTXDUAL_STPTI_TEST)
    #define                 NCACHE_MEMORY_SIZE          0x400000
#else
    #define                 NCACHE_MEMORY_SIZE          0x800000
#endif
#endif

#if defined(mb314) && !defined(UNIFIED_MEMORY)
    #define                 SYSTEM_MEMORY_SIZE          0x800000
#else
#if defined(TTXDUAL_DISK_TEST) ||  defined(TTXDUAL_STPTI_TEST)
    #define                 SYSTEM_MEMORY_SIZE          0x500000
#else
    #define                 SYSTEM_MEMORY_SIZE          0x0200000
#endif
#endif

#ifdef ST_OS20

/* Allow room for OS20 segments in internal memory */
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1300];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section_noinit")

static unsigned char    ncache_memory [NCACHE_MEMORY_SIZE];
static partition_t      the_ncache_partition;
partition_t             *ncache_partition = &the_ncache_partition;
#pragma ST_section      ( ncache_memory , "ncache_section" )

static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
partition_t             *driver_partition = &the_system_partition;
#pragma ST_section      ( external_block, "system_section_noinit")

/* This is to avoid a linker warning */
static unsigned char    internal_block_init[1];
#pragma ST_section      ( internal_block_init, "internal_section")

static unsigned char    system_block_init[1];
#pragma ST_section      ( system_block_init, "system_section")


#if defined(ST_5528) || defined(ST_5100) || defined (ST_7710) || defined(ST_5105) || defined(ST_5188) ||\
         defined(ST_5107)
static U8               data_section[1];
#pragma ST_section      ( data_section,   "data_section")
#endif

#elif defined(ST_OS21)

static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;
static unsigned char    ncache_block[NCACHE_MEMORY_SIZE];
partition_t             *ncache_partition;

#endif

/* Memory partitions */
#if defined (ST_7020) || defined (ST_5105) || defined(ST_5188) || defined(ST_5107)
STAVMEM_PartitionHandle_t       AvmemPartitionHandle;
#endif

#if defined(ST_OS20) || defined(ST_OS21) /* Linux changes */
#define TEST_PARTITION_1        system_partition
#define TEST_PARTITION_2        ncache_partition
#else /* Linux */
#define TEST_PARTITION_1         NULL
#define TEST_PARTITION_2         NULL
#endif


/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

int main( int argc, char *argv[] );

#if defined(TTXDUAL_DISK_TEST) ||  defined(TTXDUAL_STPTI_TEST)
void GlobalInit_Dual(void);
#else
void GlobalInit(void);
#endif
ST_ErrorCode_t GlobalTerm(void);
void TeletextNotifyFunction(U32 Tag, STTTX_Page_t *p1, STTTX_Page_t *p2);
void NormalUsePti( void );
void NormalUseDisk( void );
void NormalUseStpti( void );
void  FEI_Setup(void);
ST_ErrorCode_t FdmaSetup(void);
void NormalUseStdemux( void );
void StackUsage(void);
void ErrantUse( void );
void ErrorReport( ST_ErrorCode_t *ErrorStore,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr );
void ShowConfig(void);
#if !defined(mb5518) && !defined(mb275) && !defined(mb376)
void Init641x(void);
#endif
#if !(defined(TTXDUAL_DISK_TEST) ||  defined(TTXDUAL_STPTI_TEST))
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) \
    || defined(ST_5528)
static ST_ErrorCode_t DoCfg(void);
#endif
#endif
#ifdef TTXINTERACTIVE_TEST
    BOOL TTX_InitCommand( void );
#ifdef STTTX_USE_TUNER
    BOOL TUNER_InitCommand(void);
#endif
    BOOL InitCommands(void);
    void InteractiveDriversInit( void );
    BOOL TST_Init(void);
#endif

#if defined(STTTX_USE_TUNER)
void TunerNotifyFunction(STTUNER_Handle_t Handle,
                         STTUNER_EventType_t EventType,
                         ST_ErrorCode_t Error);
ST_ErrorCode_t DoTunerScan(void);
void AwaitTunerEvent(STTUNER_EventType_t *EventType_p);
#endif

#if defined(TTXDISK_TEST)
static int InsertInit(U8 **PesBuffer_p, U32 *BufferUsed_p);
static void InsertMain( void *Data);
#if defined(TTXDUAL_DISK_TEST)
static int InsertInit4629(U8 **PesBuffer_p, U32 *BufferUsed_p);
static void InsertMain4629( void *Data);
#endif
#endif

#if defined(TTXSTPTI_TEST)
static ST_ErrorCode_t PTI_Init(void);
#elif defined(TTXSTDEMUX_TEST)
static ST_ErrorCode_t DEMUX_Init(void);
#endif

#if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_7100) || defined(ST_5301) || defined(ST_7109)
ST_ErrorCode_t ConfigureMerger(ST_ErrorCode_t  *StoredError);
/* Temporary until STMERGE */
ST_ErrorCode_t TSMERGER_SetTSIn0BypassMode(void);
#endif

#if defined(ST_5100) || defined(ST_5105) || defined(ST_7710) || defined(ST_5188) || defined(ST_5107)|| defined(ST_7100) || defined(ST_7109)
ST_ErrorCode_t VOUT_Setup(void);
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
static BOOL AVMEM_Init(void);
static BOOL VMIX_Init(void);
#endif
#if defined(ST_5528)
ST_ErrorCode_t VOUT_Setup(U32 SERVICE_Display);
#endif


#if defined(ST_7020) /* FDMA & AVMEM Init code */

#include "sti2c.h"
#include "stfdma.h"
#include "stavmem.h"
#include "stcfg.h"
#include "stintmr.h"

#if defined (ST_7020) && !defined(mb290) && !defined(mb295)
/* 7020 STEM board - force I2C usage to configure DAC */
#define USE_STI2C
#define db573 /* clearer #define for below */
#endif

#if defined (mb282b)
#define I2C_PIO_DEVICE_NAME             "PIO1"          /* I2C uses PIO1 */
#elif defined(mb314) || defined(mb361) || defined(mb382)
#define I2C_PIO_DEVICE_NAME             "PIO3"          /* I2C uses PIO3 */
#endif

#define CLCFG_DEVICE_TYPE   STCFG_DEVICE_5517
#define CLCFG_BASE_ADDRESS  ST5517_CFG_BASE_ADDRESS
#define CLCFG_PTI_FIRST     STCFG_PTI_A
#define CLCFG_PTI_LAST      STCFG_PTI_A

#if !defined (UNIFIED_MEMORY)
#define UNIFIED_MEMORY 0
#endif

#define STAVMEM_DEVICE_NAME             "AVMEM0"

static BOOL FDMA_Init(void)
{
    BOOL RetOk = TRUE;
    ST_ErrorCode_t ErrorCode;
    STFDMA_InitParams_t InitParams;

    InitParams.DeviceType          = STFDMA_DEVICE_FDMA_1;
    InitParams.DriverPartition_p   = TEST_PARTITION_1;
    InitParams.NCachePartition_p   = TEST_PARTITION_2;
    InitParams.BaseAddress_p       = (U32*) FDMA_BASE_ADDRESS;
    InitParams.InterruptNumber     = FDMA_INTERRUPT;
    InitParams.InterruptLevel      = 5;
    InitParams.NumberCallbackTasks = 2;
    InitParams.ClockTicksPerSecond = ST_GetClocksPerSecond();

    STTBX_Print(( "Calling STFDMA_Init() ....... " ));
    ErrorCode = STFDMA_Init("FDMA0", &InitParams );
    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("FAILED, error %x\n", ErrorCode));
        RetOk = FALSE;
    }
    else
    {
        STTBX_Print(("%s Initialized.\n", STFDMA_GetRevision()));
    }

    return RetOk;
}


static BOOL AVMEM_Init(void)
{
    BOOL RetOk = TRUE;
    ST_ErrorCode_t ErrorCode;
    STAVMEM_InitParams_t InitAvmem;
    STAVMEM_MemoryRange_t WorkIn[2];
    STAVMEM_CreatePartitionParams_t     AVMEMCreateParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;

    STTBX_Print(( "Calling STAVMEM_Init() ....... " ));

    /*Now Initialize AVMEM and create partition */
    WorkIn[0].StartAddr_p = (void *) SDRAM_BASE_ADDRESS;
    WorkIn[0].StopAddr_p  = (void *)(SDRAM_BASE_ADDRESS + SDRAM_SIZE - 1);

    InitAvmem.CPUPartition_p          = TEST_PARTITION_1;
    InitAvmem.NCachePartition_p       = TEST_PARTITION_2;
    InitAvmem.MaxPartitions           = 1;
    InitAvmem.MaxBlocks = 58;    /* video: 1 bits buffer + 4 frame buffers
                                    tests video: 2 dest buffer
                                    audio: 1 bits buffer
                                    osd : 50 */
    InitAvmem.MaxForbiddenRanges      = 2;
    InitAvmem.MaxForbiddenBorders     = 3;
    InitAvmem.MaxNumberOfMemoryMapRanges = 1;
#if defined BM_BASE_ADDRESS
    InitAvmem.BlockMoveDmaBaseAddr_p  = (void *) BM_BASE_ADDRESS;
#else
    InitAvmem.BlockMoveDmaBaseAddr_p  = NULL;
#endif
    InitAvmem.CacheBaseAddr_p         = (void *) CACHE_BASE_ADDRESS;
    InitAvmem.VideoBaseAddr_p         = (void *) VIDEO_BASE_ADDRESS;
    InitAvmem.SDRAMBaseAddr_p         = (void *) SDRAM_BASE_ADDRESS;
    InitAvmem.SDRAMSize               = SDRAM_SIZE;
    InitAvmem.NumberOfDCachedRanges   = (sizeof(DCacheMap) / sizeof(STBOOT_DCache_Area_t)) - 1;
    InitAvmem.DCachedRanges_p         = (STAVMEM_MemoryRange_t *) DCacheMap;

#if defined (USE_STGPDMA)
    strcpy(InitAvmem.GpdmaName, STGPDMA_NAME2);
#endif

    VirtualMapping.PhysicalAddressSeenFromCPU_p = (void *) SDRAM_BASE_ADDRESS;
    VirtualMapping.PhysicalAddressSeenFromDevice_p = (void *) 0;
    VirtualMapping.PhysicalAddressSeenFromDevice2_p= (void *)(S32)(SDRAM_BASE_ADDRESS - MB382_SDRAM_BASE_ADDRESS);

    VirtualMapping.VirtualBaseAddress_p = (void *) SDRAM_BASE_ADDRESS;
    VirtualMapping.VirtualSize = SDRAM_WINDOW_SIZE;
    VirtualMapping.VirtualWindowOffset = 0;
    VirtualMapping.VirtualWindowSize = SDRAM_WINDOW_SIZE;

    InitAvmem.DeviceType = STAVMEM_DEVICE_TYPE_VIRTUAL;
    InitAvmem.SharedMemoryVirtualMapping_p = &VirtualMapping;

    ErrorCode = STAVMEM_Init(STAVMEM_DEVICE_NAME, &InitAvmem);
    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report(( STTBX_REPORT_LEVEL_ERROR,
                       "STAVMEM_Init() returned: %x\n", ErrorCode ));
        RetOk = FALSE;
    }
    else
    {

        AVMEMCreateParams.NumberOfPartitionRanges = 1;
        AVMEMCreateParams.PartitionRanges_p = WorkIn;

        /* Open a partition */
        ErrorCode = STAVMEM_CreatePartition(STAVMEM_DEVICE_NAME,
                                            &AVMEMCreateParams,
                                            &AvmemPartitionHandle);
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report(( STTBX_REPORT_LEVEL_ERROR,
                           "STAVMEM_CreatePartition() returned: %x\n",
                           ErrorCode ));
            RetOk = FALSE;
        }
        else
        {
            /* Init and Open successed */
            STTBX_Print(( "%s Initialized.\n", STAVMEM_GetRevision() ));
        }
    }

    return(RetOk);

} /* end AVMEM_Init() */


/*******************************************************************************
Name        : Board_Init
Description : mb282b: Initialize the STV6410 composant by the I2C port
              7020 STEM: Setup DAC through I2C port
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL Board_Init(void)
{
    STI2C_InitParams_t  I2cInitParams;
    STI2C_TermParams_t  I2cTermParams;
    ST_ErrorCode_t      Err;
#define CPU_50_MHZ      50000000
    BOOL                RetOk = TRUE;
    ST_DeviceName_t     DevName;


    STTBX_Print(("STI2C_GetRevision() returns: %s\n",(char*)STI2C_GetRevision() ));
    /* initialize PIO_1 already done */
    /* Setup InitParams structure */
    strcpy( DevName, "I2C-0" );
#ifdef mb282b /* SSC0 */
    I2cInitParams.BaseAddress           = (U32*)SSC_0_BASE_ADDRESS;
    I2cInitParams.InterruptNumber       = SSC_0_INTERRUPT;
    I2cInitParams.InterruptLevel        = SSC_0_INTERRUPT_LEVEL;
    I2cInitParams.ClockFrequency        = CPU_50_MHZ;     /*ST_GetClockSpeed() for tuenr*/
    I2cInitParams.PIOforSDA.BitMask     = PIO_BIT_0;
    I2cInitParams.PIOforSCL.BitMask     = PIO_BIT_2;
#else /* SSC1 -> STEM IO */
    I2cInitParams.BaseAddress           = (U32*)SSC_1_BASE_ADDRESS;
    I2cInitParams.InterruptNumber       = SSC_1_INTERRUPT;
    I2cInitParams.InterruptLevel        = SSC_1_INTERRUPT_LEVEL;
    I2cInitParams.ClockFrequency        = ST_GetClockSpeed();
    I2cInitParams.PIOforSDA.BitMask     = PIO_BIT_2;
    I2cInitParams.PIOforSCL.BitMask     = PIO_BIT_3;
#endif
    I2cInitParams.BaudRate              = STI2C_RATE_FASTMODE /*STI2C_TIMEOUT_INFINITY*/;
    I2cInitParams.MasterSlave           = STI2C_MASTER;
    I2cInitParams.MaxHandles            = 4;
    I2cInitParams.DriverPartition       = TEST_PARTITION_1;
    strcpy( I2cInitParams.PIOforSDA.PortName, I2C_PIO_DEVICE_NAME);
    strcpy( I2cInitParams.PIOforSCL.PortName, I2C_PIO_DEVICE_NAME);

    /* initialize the I2C device driver */
    Err = STI2C_Init (DevName, &I2cInitParams);

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot initialize I2C device driver\n"));
        RetOk = FALSE;
    }
    else
    {
#ifdef mb282b /* SCART switch setup */
        STI2C_Handle_t      I2cHandle;
        STI2C_OpenParams_t  I2cOpenParams;
        #define TEST_BUFFER_SIZE 32
        U8  Buffer[TEST_BUFFER_SIZE];
        U32 ActLen;
        U8  index;

        /* Setup OpenParams structure here */
        I2cOpenParams.I2cAddress        = 0x94; /* address of remote I2C device */
        I2cOpenParams.AddressType       = STI2C_ADDRESS_7_BITS;
        I2cOpenParams.BusAccessTimeOut  = 100000;

        Err = STI2C_Open (DevName, &I2cOpenParams, &I2cHandle);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot open I2C device driver\n"));
            RetOk = FALSE;
        }
        else
        {
            /* Write to a device */
            for (index = 0; index < 32; index+=1)
            {
                Buffer[index]=0x0;
            }
            Buffer[0] = 0x00;
            Buffer[1] = 0x05;
            Buffer[2] = 0x01;
            Buffer[3] = 0x05;
            Buffer[4] = 0x02;
            Buffer[5] = 0x00;
            Buffer[6] = 0x03;
            Buffer[7] = 0x68;
            Buffer[8] = 0x04;
            Buffer[9] = 0x53;
            Buffer[10] = 0x05;
            Buffer[11] = 0x00;
            Buffer[12] = 0x06;
            Buffer[13] = 0x00;
            Buffer[14] = 0x07;
            Buffer[15] = 0x00;
            Buffer[16] = 0x08;
            Buffer[17] = 0x03;
            for (index = 0; index < 18; index+=2)
            {
                if (STI2C_Write ( I2cHandle, &Buffer[index], 2, STI2C_TIMEOUT_INFINITY, &ActLen) != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "failed to write to the I2C device.\n"));
                    RetOk = FALSE;
                }
            }

            if (STI2C_Close (I2cHandle) != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "failed to close the I2C driver.\n"));
                RetOk = FALSE;
            }
        }

#elif defined(db573) /* Setup 7020 STEM DAC through I2C PIO port PCF8575 */

        /* STBOOT has to use I2C to reset the 7020,
          and sets up the DAC mode we want (I2S) */

        Err = STBOOT_Init7020STEM(DevName, SDRAM_FREQUENCY);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STBOOT_Init7020STEM returned error 0x%08X\n", Err));
            RetOk = FALSE;
        }
#endif
    }

    I2cTermParams.ForceTerminate = FALSE;
    if (STI2C_Term (DevName, &I2cTermParams) != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "failed to terminate the I2C driver.\n"));
        RetOk = FALSE;
    }

    if ( RetOk == FALSE )
    {
#ifdef mb282b
        STTBX_Report((  STTBX_REPORT_LEVEL_ERROR,
                       "Configuration of the STV6410 componant failed !\n" ));
#elif defined(db573)
        STTBX_Report((  STTBX_REPORT_LEVEL_ERROR,
                       "Configuration of the 7020 STEM failed !\n" ));
#endif
    }
    else
    {
#ifdef mb282b
        STTBX_Print(( "Configuration of the STV6410 componant : ok\n" ));
#elif defined(db573)
        STTBX_Print(( "Configuration of the 7020 STEM : ok\n" ));
#endif
    }
    return(RetOk);

} /* end Board_Init() */

BOOL CFG_Init(void)
{
    STCFG_InitParams_t  CfgInit;
    STCFG_Handle_t      CFGHandle;
    U8                  Index;
    char                Msg[150];
    BOOL                RetOk = TRUE;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    CfgInit.DeviceType    = CLCFG_DEVICE_TYPE;
    CfgInit.Partition_p   = TEST_PARTITION_1;
    CfgInit.BaseAddress_p = (U32*)CLCFG_BASE_ADDRESS;
    if (CfgInit.BaseAddress_p == NULL)
    {
       debugmessage("***ERROR: Invalid device base address OR simulator memory allocation error\n\n");
       return(FALSE);
    }

    ErrorCode = STCFG_Init("cfg", &CfgInit);
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STCFG_Open("cfg", NULL, &CFGHandle);
        if (ErrorCode == ST_NO_ERROR)
        {
#if defined (ST_5528) && defined (ST_7020)
            /* Enable Exteranl interrupt (STi5528 and STEM 7020) */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                              STCFG_EXTINT_0, STCFG_EXTINT_INPUT);
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                              STCFG_EXTINT_1, STCFG_EXTINT_INPUT);
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                              STCFG_EXTINT_2, STCFG_EXTINT_INPUT);
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                              STCFG_EXTINT_3, STCFG_EXTINT_INPUT);

                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to enable Video DAC in STCFG \n");
                }
            }
#else /* else of (ST_5528) && defined (ST_7020) */

            /* CDREQ configuration for PTIs A, B and C */
            for (Index = CLCFG_PTI_FIRST; Index <= CLCFG_PTI_LAST; Index++)
            {
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_PTI_CDREQ_CONNECT, Index, STCFG_PTI_CDIN_0, STCFG_CDREQ_VIDEO_INT);
                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to set PTI CDREQ connect in STCFG \n" );
                    break;
                }
#ifdef PCR_CONNECT
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_PTI_PCR_CONNECT, Index, STCFG_PCR_27MHZ_A);
                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to set PTI PCR connect in STCFG \n");
                    break;
                }
#endif /* PCR_CONNECT */
            }
            /* Enable video DACs */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_VIDEO_ENABLE, STCFG_VIDEO_DAC_ENABLE);
                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to enable Video DAC in STCFG \n");
                }
            }

            /* Power down audio DACs */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_AUDIO_ENABLE, STCFG_AUDIO_DAC_POWER_OFF);
                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to power down audio DACs in STCFG \n");
                }
            }

            /* Enable PIO4<4> as IrDA Data not IRDA Control */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_PIO_ENABLE, STCFG_PIO4_BIT4_IRB_NOT_IRDA);
                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to enable PIO4<4> as IrDA Data not IRDA Control in STCFG \n");
                }
            }
#endif /* not defined (ST_5528) && defined (ST_7020) */

            ErrorCode = STCFG_Close(CFGHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                debugmessage(" Unable to close opened STCFG handle \n");
            }

        }
        else
        {
           debugmessage(" Unable to open STCFG \n");
        }
    }
    else
    {
        debugmessage(" Unable to initialize STCFG \n");
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        sprintf(Msg,"STCFG initialized,\trevision=%-21.21s\n", STCFG_GetRevision());
        RetOk = TRUE;
    }
    else
    {
        sprintf(Msg,"Error: STCFG_Init() failed ! Error 0x%0x\n", ErrorCode);
        RetOk = FALSE;
    }
    debugmessage(Msg);

    return(RetOk);
} /* end CFG_Init() */



#define STINTMR_DEVICE_NAME "intmr"

#if defined (mb290)
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7020
#define INTMR_BASE_ADDRESS            (STI7020_BASE_ADDRESS + ST7020_CFG_OFFSET)
#define INTMR_INTERRUPT_NUMBER        ST5514_EXTERNAL1_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         EXTERNAL_1_INTERRUPT_LEVEL

#elif defined (mb376) /* this means db573 7020 STEM board */

#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7020
#define INTMR_BASE_ADDRESS            (STI7020_BASE_ADDRESS + ST7020_CFG_OFFSET)
#define INTMR_INTERRUPT_NUMBER        ST5528_EXTERNAL2_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         3

#elif (defined (mb382) || defined (mb314)) /* this means db573 7020 STEM board */
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7020
#define INTMR_BASE_ADDRESS            (STI7020_BASE_ADDRESS + ST7020_CFG_OFFSET)
    #if defined(mb314)
    #define INTMR_INTERRUPT_NUMBER        ST5514_EXTERNAL0_INTERRUPT
    #else /* defined(mb382) */
    #define INTMR_INTERRUPT_NUMBER        ST5517_EXTERNAL0_INTERRUPT
    #endif
#define INTMR_INTERRUPT_LEVEL         EXTERNAL_0_INTERRUPT_LEVEL

#elif defined (ST_7015)
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7015
#define INTMR_BASE_ADDRESS            (STI7015_BASE_ADDRESS + ST7015_CFG_OFFSET)
#define INTMR_INTERRUPT_NUMBER        ST20TP3_EXTERNAL1_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         EXTERNAL_1_INTERRUPT_LEVEL

#elif defined (ST_7020)
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7015
#define INTMR_BASE_ADDRESS            (STI7020_BASE_ADDRESS + ST7020_CFG_OFFSET)
#define INTMR_INTERRUPT_NUMBER        ST20TP3_EXTERNAL1_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         EXTERNAL_1_INTERRUPT_LEVEL

#elif defined (ST_GX1)
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_GX1
#define INTMR_BASE_ADDRESS            (S32)(0xBE080000+0x28)      /* GX1_INTREQ08 */
#define INTMR_INTERRUPT_NUMBER        kbimST40_VideoSync
#define INTMR_INTERRUPT_LEVEL         0
#endif



/*******************************************************************************
Name        : INTMR_Init
Description : Initialise STINTMR
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL INTMR_Init(void)
{
    BOOL RetOk = TRUE;
    ST_ErrorCode_t       ErrCode = ST_NO_ERROR;
    STINTMR_InitParams_t InitStIntMr;
#ifdef ST_GX1
    U32 i;
#endif /* #ifdef ST_GX1 */

    /* Initialise STINTMR */
    InitStIntMr.DeviceType          = INTMR_DEVICE_TYPE;
    InitStIntMr.Partition_p         = TEST_PARTITION_1;
    InitStIntMr.BaseAddress_p       = (void *)INTMR_BASE_ADDRESS;
    InitStIntMr.InterruptNumbers[0] = INTMR_INTERRUPT_NUMBER;
    InitStIntMr.InterruptLevels[0]  = INTMR_INTERRUPT_LEVEL;
#if defined(ST_7015) || defined(ST_7020)
    /* The following lines are needed only for 7015 (backward compatibility reason) !! */
    InitStIntMr.InterruptNumber     = INTMR_INTERRUPT_NUMBER;
    InitStIntMr.InterruptLevel      = INTMR_INTERRUPT_LEVEL;
#endif /* ST_7015/20*/

    InitStIntMr.NumberOfInterrupts  = 1;
    InitStIntMr.NumberOfEvents      = 3;

    strcpy( InitStIntMr.EvtDeviceName, EVTDeviceName);

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

            /* Setting default interrupt mask */
#if defined(ST_7015)
            /* Setting interrupt mask for VTG1  */
            ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, ST7015_VTG1_INT, 1<<ST7015_VTG1_INT );
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",ST7015_VTG1_INT,ErrCode));
            }
            /* Setting interrupt mask for VTG2  */
            ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, ST7015_VTG2_INT, 1<<ST7015_VTG2_INT );
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",ST7015_VTG2_INT,ErrCode));
            }
#elif defined(ST_7020)
            /* Setting interrupt mask for VTG1  */
            ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, ST7020_VTG1_INT, 1<<ST7020_VTG1_INT );
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",ST7020_VTG1_INT,ErrCode));
            }
            /* Setting interrupt mask for VTG2  */
            ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, ST7020_VTG2_INT, 1<<ST7020_VTG2_INT );
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",ST7020_VTG2_INT,ErrCode));
            }
#elif defined (ST_GX1)
            for(i=0;i<3;i++)
            {
                /* Setting interrupt mask for VTG=0, DVP1=1 & DVP2=2 */
                ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, i, (0x10000000)<<i);
                if(ErrCode != ST_NO_ERROR)
                {
                    STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",i,ErrCode));
                }
            }
#endif /* ST_GX1 */
        }
    }

    return(RetOk);
} /* end INTMR_Init() */



#endif/* ST_7020 */

/* Functions -------------------------------------------------------------- */

/****************************************************************************
Name         : main()

Description  : Calls the specific test functions

Parameters   : int argc, char *argv[]

Return Value : int

See Also     : None
 ****************************************************************************/

int main( int argc, char *argv[] )
{
    ST_ErrorCode_t          retVal = 0;
#ifndef ST_OSLINUX
    STTBX_InitParams_t      TBXInitParams;
    STBOOT_InitParams_t     BootParams;
    STBOOT_TermParams_t     BootTerm = { FALSE };
#endif

#if defined(STTTX_DEBUG1)
    STEVT_OpenParams_t      EVTOpenParams;
    STEVT_SubscribeParams_t EVTSubscribeParams;
    STEVT_Handle_t          EVTHandle;
#endif
#ifndef ST_OSLINUX
#ifdef ST_OS21

    /* Create memory partitions */
    system_partition   =  partition_create_heap((U8*)external_block, sizeof(external_block));
#ifdef ARCHITECTURE_ST200
    ncache_partition   =  partition_create_heap((U8*)(ncache_block),sizeof(ncache_block));
#else
    ncache_partition   =  partition_create_heap((U8*)ST40_NOCACHE_NOTRANSLATE(ncache_block),
                                                                         sizeof(ncache_block));
#endif

#elif defined(ST_OS20)
        /* Create memory partitions */
    partition_init_simple(&the_internal_partition,
                          (U8*)internal_block,
                          sizeof(internal_block)
                         );
    partition_init_heap(&the_system_partition,
                        (U8*)external_block,
                        sizeof(external_block));
    partition_init_heap(&the_ncache_partition,
                        ncache_memory,
                        sizeof(ncache_memory));

    /* to avoid linker warnings - start */
    internal_block_init[0] = 0;
    system_block_init[0] = 0;

#if defined(ST_5528) || defined(ST_5100) || defined (ST_7710) || defined(ST_5105) || defined(ST_5188) ||\
         defined(ST_5107)
    data_section[0] = 0;
#endif

#endif

#if defined(DISABLE_DCACHE) || defined(ST_5301)
    BootParams.DCacheMap                 = NULL;
#else

#if defined(TTXDUAL_DISK_TEST)  || defined(TTXDUAL_STPTI_TEST)
    BootParams.DCacheMap                 = &BOOT_DCacheMap;
#else
    BootParams.DCacheMap                 = DCacheMap;
#endif

#endif

#ifdef DISABLE_ICACHE
    BootParams.ICacheEnabled             = FALSE;
#else
    BootParams.ICacheEnabled             = TRUE;
#endif

#ifdef ST_OS21
    BootParams.TimeslicingEnabled        = TRUE;
#endif
    BootParams.CacheBaseAddress          = (U32 *)CACHE_BASE_ADDRESS;
    BootParams.SDRAMFrequency            = SDRAM_FREQUENCY;
    BootParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.MemorySize                = SDRAM_SIZE;
    retVal = STBOOT_Init( "boot", &BootParams );

    if (retVal != ST_NO_ERROR)
    {
        printf("Error: STBOOT failed to initialize !\n");
        return 0;
    }

    /* Initialize the toolbox */
    TBXInitParams.SupportedDevices       = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice    = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice     = STTBX_DEVICE_DCU;
    TBXInitParams.CPUPartition_p         = TEST_PARTITION_1;
    STTBX_Init( "tbx", &TBXInitParams );

#ifdef STTTX_NOSTEMBOARD
    /* Will probably change for other boards */
#if defined (mb314)
    /* Poke epld to select Tuner connectors as TS source NOT STEM */
    STTBX_Print(("MB314 EPLD Poke for Tuner connector select\n"));
    *(volatile U32*)(0x7e0c0000) = 1;
#endif
#endif
#endif  /* ST_OSLINUX */

#if defined(TTXSTPTI_TEST) || defined(TTXSTDEMUX_TEST)
    VBISlot = 0;
#endif

#if defined(TTXDUAL_DISK_TEST) || defined(TTXDUAL_STPTI_TEST)
    GlobalInit_Dual();
#else
    GlobalInit();
#endif

    ShowConfig();

    /* revision string available before STTTX Initialized */
    STTBX_Print(("Harness Software Revision is %s\n\n", HarnessRev));
    STTBX_Print(("----------------------------------------------------------\n"));

#if defined(STTTX_DEBUG1)
    EVTSubscribeParams.NotifyCallback = (STEVT_CallbackProc_t)TeletextNotifyPeslost;

    STTBX_Print(("Subscribe to STTTX_EVENT_PES_LOST\n"));
    STTBX_Print(("EVTOpen=%x\n",STEVT_Open(EVTDeviceName,&EVTOpenParams,&EVTHandle)));
    STTBX_Print(("EVTSubscribeResult=%x\n",STEVT_Subscribe(EVTHandle,STTTX_EVENT_PES_LOST, &EVTSubscribeParams)));
    STTBX_Print(("EVTSubscribeResult=%x\n",STEVT_Subscribe(EVTHandle,STTTX_EVENT_PES_NOT_CONSUMED, &EVTSubscribeParams)));
    STTBX_Print(("EVTSubscribeResult=%x\n",STEVT_Subscribe(EVTHandle,STTTX_EVENT_PES_INVALID, &EVTSubscribeParams)));
    STTBX_Print(("----------------------------------------------------------\n"));
#endif
#if defined(TTXINTERACTIVE_TEST)
    if (TST_Init())
    {
        printf("TST_Init() failed\n");
        return (0);
    }
    InitCommands();                              /* Initialise all of the registered tests */
    STTST_SetMode(STTST_INTERACTIVE_MODE);       /* set to interactive mode */

    InteractiveDriversInit();
    STTST_Start();                               /* Launch the Command interpretor */

    STTBX_Print(("\nTEST END. \n"));
#elif defined(TTXPTI_TEST) || defined(TTXLINK_TEST)
    NormalUsePti();
    ErrantUse();
#elif defined(TTXSTPTI_TEST)
    NormalUseStpti();
#if 0
    ErrantUse();
#endif
#elif defined(TTXSTDEMUX_TEST)
    NormalUseStdemux();
    ErrantUse();
#else
    /*StackUsage();*/
    NormalUseDisk();
    ErrantUse();
#endif

    GlobalTerm();
#ifndef ST_OSLINUX
    STBOOT_Term( "boot", &BootTerm );
#endif

    return retVal;
}

#if defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
void CLKRV_Init()
{
    static STCLKRV_InitParams_t    CLKRVInitPars;

    /* Initialize our native Clock Recovery Driver */

    CLKRVInitPars.PCRMaxGlitch      = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitPars.PCRDriftThres     = 300;
    CLKRVInitPars.MinSampleThres    = 10;
    CLKRVInitPars.MaxWindowSize     = 50;
    CLKRVInitPars.Partition_p       = TEST_PARTITION_1;
    CLKRVInitPars.DeviceType        = CLK_REC_DEVICE_TYPE;
    CLKRVInitPars.FSBaseAddress_p   = (U32 *)CKG_BASE_ADDRESS;
#if !defined(ST_7100) && !defined(ST_7109)
    CLKRVInitPars.ADSCBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
    CLKRVInitPars.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
    CLKRVInitPars.InterruptNumber   = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitPars.InterruptLevel    = 7;

    strcpy( CLKRVInitPars.EVTDeviceName, EVTDeviceName );
#ifdef STTTX_SUBT_SYNC_ENABLE
    strcpy( CLKRVInitPars.PTIDeviceName, STPTIDeviceName );
    strcpy( CLKRVInitPars.PCREvtHandlerName, EVTDeviceName);
#endif
    STTBX_Print(( "Calling STCLKRV_Init()....\n" ));
    if ( STCLKRV_Init( CLKRVDeviceName, &CLKRVInitPars ) != ST_NO_ERROR)
    STTBX_Print(( "Error!!! - STCLKRV_Init()....\n" ));
}
#endif

#if defined(ST_5105)
#define COMPO_BASE_ADDRESS     (ST5105_BLITTER_BASE_ADDRESS + 0xA00)
#define GDMA_BASE_ADDRESS      (ST5105_GDMA1_BASE_ADDR)
#elif defined(ST_5107)
#define COMPO_BASE_ADDRESS     (ST5107_BLITTER_BASE_ADDRESS + 0xA00)
#define GDMA_BASE_ADDRESS      (ST5107_GDMA1_BASE_ADDR)
#elif defined(ST_5188)
#define COMPO_BASE_ADDRESS     (ST5188_BLITTER_BASE_ADDRESS + 0xA00)
#define GDMA_BASE_ADDRESS      (ST5188_GDMA1_BASE_ADDR)
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
static BOOL AVMEM_Init(void)
{
    BOOL RetOk = TRUE;
    ST_ErrorCode_t ErrorCode;
    STAVMEM_InitParams_t InitAvmem;
    STAVMEM_MemoryRange_t WorkIn[2];
    STAVMEM_CreatePartitionParams_t     AVMEMCreateParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;

    STTBX_Print(( "Calling STAVMEM_Init() ....... " ));

    /*Now Initialize AVMEM and create partition */
    WorkIn[0].StartAddr_p = (void *) AVMEM_BASE_ADDRESS;
    WorkIn[0].StopAddr_p  = (void *)(AVMEM_BASE_ADDRESS + AVMEM_SIZE - 1);

    InitAvmem.CPUPartition_p          = TEST_PARTITION_1;
    InitAvmem.NCachePartition_p       = TEST_PARTITION_2;
    InitAvmem.MaxPartitions           = 1;
    InitAvmem.MaxBlocks               = 58;    /* video: 1 bits buffer + 4 frame buffers
                                                  tests video: 2 dest buffer
                                                  audio: 1 bits buffer
                                                  osd : 50  */
    InitAvmem.MaxForbiddenRanges         = 2;
    InitAvmem.MaxForbiddenBorders        = 3;
    InitAvmem.MaxNumberOfMemoryMapRanges = 1;
    InitAvmem.BlockMoveDmaBaseAddr_p     = NULL;
    InitAvmem.CacheBaseAddr_p            = (void *) CACHE_BASE_ADDRESS;
    InitAvmem.VideoBaseAddr_p            = (void *) VIDEO_BASE_ADDRESS;
    InitAvmem.SDRAMBaseAddr_p            = (void *) AVMEM_BASE_ADDRESS;
    InitAvmem.SDRAMSize                  = AVMEM_SIZE;
    InitAvmem.NumberOfDCachedRanges      = 0/*(sizeof(DCacheMap) / sizeof(STBOOT_DCache_Area_t)) - 1*/;
    InitAvmem.DCachedRanges_p            = (STAVMEM_MemoryRange_t *)NULL /*DCacheMap*/;
    InitAvmem.OptimisedMemAccessStrategy_p  = NULL;
#if defined (USE_STGPDMA)
    strcpy(InitAvmem.GpdmaName, STGPDMA_NAME2);
#endif

    VirtualMapping.PhysicalAddressSeenFromCPU_p = (void *) AVMEM_BASE_ADDRESS;
    VirtualMapping.PhysicalAddressSeenFromDevice_p = (void *) AVMEM_BASE_ADDRESS;
    VirtualMapping.PhysicalAddressSeenFromDevice2_p= (void *)AVMEM_BASE_ADDRESS;

    VirtualMapping.VirtualBaseAddress_p = (void *) AVMEM_BASE_ADDRESS;
    VirtualMapping.VirtualSize = AVMEM_SIZE;
    VirtualMapping.VirtualWindowOffset = 0;
    VirtualMapping.VirtualWindowSize = AVMEM_SIZE;

    InitAvmem.DeviceType = STAVMEM_DEVICE_TYPE_VIRTUAL;
    InitAvmem.SharedMemoryVirtualMapping_p = &VirtualMapping;

    ErrorCode = STAVMEM_Init("STAVMEM", &InitAvmem);
    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report(( STTBX_REPORT_LEVEL_ERROR,
                       "STAVMEM_Init() returned: %x\n", ErrorCode ));
        RetOk = FALSE;
    }
    else
    {

        AVMEMCreateParams.NumberOfPartitionRanges = 1;
        AVMEMCreateParams.PartitionRanges_p = WorkIn;

        /* Open a partition */
        ErrorCode = STAVMEM_CreatePartition("STAVMEM",
                                            &AVMEMCreateParams,
                                            &AvmemPartitionHandle);
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report(( STTBX_REPORT_LEVEL_ERROR,
                           "STAVMEM_CreatePartition() returned: %x\n",
                           ErrorCode ));
            RetOk = FALSE;
        }
        else
        {
            /* Init and Open successed */
            STTBX_Print(( "%s Initialized.\n", STAVMEM_GetRevision() ));
        }
    }

    return(RetOk);

} /* end AVMEM_Init() */
static BOOL VMIX_Init()
{
    BOOL RetOk = TRUE;
    STVMIX_InitParams_t STVMIX_InitParams;
    ST_ErrorCode_t      ErrCode;
    STGXOBJ_Bitmap_t    CacheBitmap;
    ST_DeviceName_t         VoutDeviceNames[3];
    STAVMEM_AllocBlockParams_t      AllocBlockParams;
    STAVMEM_BlockHandle_t           VMIX_AVMEMHandle;


    STVMIX_InitParams.CPUPartition_p      = TEST_PARTITION_2;
    STVMIX_InitParams.AVMEM_Partition     = AvmemPartitionHandle;
    STVMIX_InitParams.AVMEM_Partition2     = AvmemPartitionHandle;
    STVMIX_InitParams.BaseAddress_p       = (void *)0;
    STVMIX_InitParams.DeviceBaseAddress_p = (void *)0;
#if defined(ST_5105) || defined(ST_5107)
    STVMIX_InitParams.DeviceType          = STVMIX_COMPOSITOR;
#else
    STVMIX_InitParams.DeviceType          = STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422;
#endif
    STVMIX_InitParams.MaxOpen             = 1;
    STVMIX_InitParams.MaxLayer            = 1;

    STVMIX_InitParams.RegisterInfo.CompoBaseAddress_p = (void*)COMPO_BASE_ADDRESS;
    STVMIX_InitParams.RegisterInfo.GdmaBaseAddress_p  = (void*)GDMA_BASE_ADDRESS;
    STVMIX_InitParams.RegisterInfo.VoutBaseAddress_p  = (void*)VOUT_BASE_ADDRESS;

    AllocBlockParams.PartitionHandle          = AvmemPartitionHandle;
    AllocBlockParams.Alignment                = 8;
    AllocBlockParams.Size                     = 16*1024;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    ErrCode= STAVMEM_AllocBlock(&AllocBlockParams,&VMIX_AVMEMHandle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report(( STTBX_REPORT_LEVEL_ERROR,
                       "STAVMEM_AllocBlock() returned: %x\n",
                        ErrorCode ));
        RetOk = FALSE;
    }
    else
    {
        ErrCode = STAVMEM_GetBlockAddress(VMIX_AVMEMHandle,&CacheBitmap.Data1_p);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report(( STTBX_REPORT_LEVEL_ERROR,
                           "STAVMEM_GetBlockAddress() returned: %x\n",
                            ErrorCode ));
            RetOk = FALSE;
        }
        else
        {
            CacheBitmap.ColorType                     = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
            CacheBitmap.ColorSpaceConversion          = STGXOBJ_ITU_R_BT601;
            CacheBitmap.Size1                         = AllocBlockParams.Size;

            STVMIX_InitParams.CacheBitmap_p           = &CacheBitmap;

            strcpy(VoutDeviceNames[0], VOUTDeviceName );
            strcpy(VoutDeviceNames[1], "" );
            strcpy(VoutDeviceNames[2], "" );

            STVMIX_InitParams.OutputArray_p       = VoutDeviceNames;

            strcpy(STVMIX_InitParams.VTGName, VTGDeviceName );
            strcpy(STVMIX_InitParams.EvtHandlerName, EVTDeviceName );

            STTBX_Print(("VMIX_Setup(%s)=", VMIXDeviceName ));
            ErrCode = STVMIX_Init(VMIXDeviceName, &STVMIX_InitParams);
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Print(("%s\n", ErrCode ));
                RetOk = FALSE;
            }
            else
            {
                STTBX_Print(("%s\n", STVMIX_GetRevision() ));
            }
        }
    }
    return(RetOk);
}
#endif

#if !(defined(TTXDUAL_DISK_TEST) || defined(TTXDUAL_STPTI_TEST))
void GlobalInit(void)
{
    ST_ErrorCode_t        StoredError = ST_NO_ERROR;
    ST_ErrorCode_t        ReturnError;
    STDENC_InitParams_t   DENCInitParams;
    STDENC_OpenParams_t   DENCOpenParams;
    STDENC_EncodingMode_t DENCEncodingMode;
    STVTG_InitParams_t    VTGInitParams;
    STVTG_OpenParams_t    VTGOpenParams;
    STEVT_InitParams_t    EVTInitParams;
#ifdef STTTX_SUBT_SYNC_ENABLE
#if defined (ST_5514)|| defined (ST_5516)|| defined (ST_5517) ||defined (ST_5528)
    STPWM_InitParams_t    PWMInitParams;
#endif
#ifndef ST_7710
    STCLKRV_InitParams_t  CLKRVInitParams;
#endif
#endif

#if !defined(STTTX_USE_DENC)
    STVBI_InitParams_t    VBIInitParams;
#endif

#if defined(STTTX_USE_TUNER)
    STTUNER_OpenParams_t  TUNEROpenParams;
#endif

#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || (defined(TTXDISK_TEST) && defined(ST_5518))
    boolean               PtiReturn;
#endif

    EventSemaphore_p  = STOS_SemaphoreCreateFifo(NULL, 0);
    FilterSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);

    ST_GetClockInfo(&ClockInfo);
#ifndef ST_OSLINUX
    /* Initialize PIO and I2C */
    PioInitParams.BaseAddress     = (U32 *)TUNER_SSC_PIO_BASE;
    PioInitParams.InterruptNumber = TUNER_SSC_PIO_INTERRUPT;
    PioInitParams.InterruptLevel  = TUNER_SSC_PIO_LEVEL;
    PioInitParams.DriverPartition = TEST_PARTITION_1;

    STTBX_Print(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init(PioDeviceName,
                             &PioInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    /* Initialize EVT */
    EVTInitParams.MemoryPartition = (ST_Partition_t *)TEST_PARTITION_1;
    EVTInitParams.EventMaxNum     = 40;
    EVTInitParams.ConnectMaxNum   = 40;
    EVTInitParams.SubscrMaxNum    = 40;
    EVTInitParams.MemorySizeFlag  = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() ................\n" ));
    ReturnError = STEVT_Init(EVTDeviceName, &EVTInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#ifdef STTTX_SUBT_SYNC_ENABLE

#if defined (ST_5514)|| defined(ST_5516)|| defined(ST_5517) ||defined(ST_5528)
#ifndef ST_OSLINUX
    PioInitParams.BaseAddress     = (U32 *)PIO_2_BASE_ADDRESS;
    PioInitParams.InterruptNumber = PIO_2_INTERRUPT;
    PioInitParams.InterruptLevel  = PIO_2_INTERRUPT_LEVEL;
    PioInitParams.DriverPartition = TEST_PARTITION_1;
    STTBX_Print(( "Calling STPIO_Init() for PWM................\n" ));
    ReturnError = STPIO_Init("PIO2",
                             &PioInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    PioInitParams.BaseAddress     = (U32 *)PIO_4_BASE_ADDRESS;
    PioInitParams.InterruptNumber = PIO_4_INTERRUPT;
    PioInitParams.InterruptLevel  = PIO_4_INTERRUPT_LEVEL;
    PioInitParams.DriverPartition = TEST_PARTITION_1;
    STTBX_Print(( "Calling STPIO_Init()................\n" ));
    ReturnError = STPIO_Init("PIO4",
                             &PioInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    /* Initialize PWM */
    PWMInitParams.Device                = STPWM_C4;
    PWMInitParams.C4.BaseAddress        = (U32 *) PWM_BASE_ADDRESS;
    PWMInitParams.C4.PrescaleFactor     = STPWM_PRESCALE_MIN;

    strcpy(PWMInitParams.C4.PIOforPWM0.PortName,"PIO2");
    PWMInitParams.C4.PIOforPWM0.BitMask = PIO_BIT_7;
    strcpy(PWMInitParams.C4.PIOforPWM1.PortName,"PIO3");
    PWMInitParams.C4.PIOforPWM1.BitMask = PIO_BIT_7;
    strcpy(PWMInitParams.C4.PIOforPWM2.PortName,"PIO4");
    PWMInitParams.C4.PIOforPWM2.BitMask = PIO_BIT_7;

    STTBX_Print(( "Calling STPWM_Init() ................\n" ));
    ReturnError = STPWM_Init(PWMDeviceName, &PWMInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
#endif

#if defined (ST_7020)
    INTMR_Init();
    FDMA_Init();

#ifdef ST_5301
    *(volatile U32*)(0x20D00004) = 0x18000;
#endif

#ifdef ST_7710
    *(volatile U32*)(0x20103cdc) = 0x80;
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    *(volatile U32*)(0x20402000) = 0x00;
    *(volatile U32*)(0x20402004) = 0x00;
#endif
    AVMEM_Init();
    Board_Init();
#endif

#if defined(STTTX_USE_TUNER)

    I2CInitParams.BaseAddress       = (U32 *)TUNER_SSC_BASE;
    strcpy(I2CInitParams.PIOforSDA.PortName, PioDeviceName);
    I2CInitParams.PIOforSDA.BitMask = PIO_FOR_TUNER_SSC_SDA;
    strcpy(I2CInitParams.PIOforSCL.PortName, PioDeviceName);
    I2CInitParams.PIOforSCL.BitMask = PIO_FOR_TUNER_SSC_SCL;
    I2CInitParams.InterruptNumber   = TUNER_SSC_INTERRUPT;
    I2CInitParams.InterruptLevel    = TUNER_SSC_INTERRUPT_LEVEL;
    I2CInitParams.MasterSlave       = STI2C_MASTER;
    I2CInitParams.BaudRate          = STI2C_RATE_NORMAL;
    I2CInitParams.ClockFrequency    = ClockInfo.CommsBlock;
    I2CInitParams.MaxHandles        = 5;
    I2CInitParams.DriverPartition   = TEST_PARTITION_1;

    STTBX_Print(( "Calling STI2C_Init() ................\n" ));
    ReturnError = STI2C_Init(I2CDeviceName,
                             &I2CInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STTUNER_Init() ................\n" ));
    TunerInitParams.Device    = STTUNER_DEVICE_SATELLITE;
    TunerInitParams.DemodType = TEST_DEMOD_TYPE;
    TunerInitParams.TunerType = TEST_TUNER_TYPE;
    strcpy(TunerInitParams.EVT_DeviceName, EVTDeviceName);
    strcpy(TunerInitParams.EVT_RegistrantName, TUNERDeviceName);
    TunerInitParams.Device    = STTUNER_DEVICE_SATELLITE;

    TunerInitParams.Max_BandList      = 10;
    TunerInitParams.Max_ThresholdList = 10;
    TunerInitParams.Max_ScanList      = 10;
    TunerInitParams.DriverPartition = TEST_PARTITION_1;

    /* sat drivers */
    TunerInitParams.TunerType         = STTUNER_TUNER_HZ1184;
#if defined(mb5518) || defined(mb5518um)
    TunerInitParams.TunerIO.Route     = STTUNER_IO_DIRECT;
#else
    TunerInitParams.TunerIO.Route     = STTUNER_IO_REPEATER;
#endif
    TunerInitParams.TunerIO.Driver    = STTUNER_IODRV_I2C;
    TunerInitParams.TunerIO.Address   = 0xC0;

    TunerInitParams.LnbType           = STTUNER_LNB_STEM;
    TunerInitParams.LnbIO.Route       = STTUNER_IO_DIRECT;
    TunerInitParams.LnbIO.Driver      = STTUNER_IODRV_I2C;
    /* Address should really be probed, but this will do for now */
    /* TunerInitParams.LnbIO.Address     = LnbAddress; */
    TunerInitParams.LnbIO.Address     = 0x44;

    TunerInitParams.DemodType         = STTUNER_DEMOD_STV0299;
    TunerInitParams.DemodIO.Route     = STTUNER_IO_DIRECT;
    TunerInitParams.DemodIO.Driver    = STTUNER_IODRV_I2C;
    TunerInitParams.DemodIO.Address   = 0xD0;

    TunerInitParams.ExternalClock     = 4000000;
#if defined(mb5518) || defined(mb5518um)
    TunerInitParams.TSOutputMode      = STTUNER_TS_MODE_SERIAL;
    TunerInitParams.SerialClockSource = STTUNER_SCLK_VCODIV6;
    TunerInitParams.SerialDataMode    = STTUNER_SDAT_DEFAULT;
    TunerInitParams.FECMode           = STTUNER_FEC_MODE_DVB;
#else
    TunerInitParams.TSOutputMode      = STTUNER_TS_MODE_PARALLEL;
    TunerInitParams.SerialClockSource = STTUNER_SCLK_DEFAULT;
    TunerInitParams.SerialDataMode    = STTUNER_SDAT_DEFAULT;
    TunerInitParams.FECMode           = STTUNER_FEC_MODE_DVB;
#endif

    /* front I2C bus */
    strcpy(TunerInitParams.DemodIO.DriverName, I2CDeviceName);
    strcpy(TunerInitParams.TunerIO.DriverName, I2CDeviceName);
    strcpy(TunerInitParams.LnbIO.DriverName, I2CDeviceName);

    ReturnError = STTUNER_Init(TUNERDeviceName,
                               &TunerInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Setup open params for tuner */
    TUNEROpenParams.NotifyFunction = TunerNotifyFunction;

    /* Open params setup */
    STTBX_Print(( "Calling STTUNER_Open() ................\n" ));
    ReturnError = STTUNER_Open(TUNERDeviceName,
                               (const STTUNER_OpenParams_t *)&TUNEROpenParams,
                               &TUNERHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    /* Configure tuner band list */
    Bands.BandList = BandList;
    Bands.NumElements = MAX_BANDS;
    BandList[BAND_US].BandStart     = 12200000;
    BandList[BAND_US].BandEnd       = 12700000;
    BandList[BAND_US].LO            = 11250000;

    BandList[BAND_EU_LO].BandStart  = 10600000;
    BandList[BAND_EU_LO].BandEnd    = 11700000;
    BandList[BAND_EU_LO].LO         =  9750000;

    BandList[BAND_EU_HI].BandStart  = 11700000;
    BandList[BAND_EU_HI].BandEnd    = 12750000;
    BandList[BAND_EU_HI].LO         = 10600000;
#if !defined(STTTX_OLD_TUNER)
    BandList[BAND_US].DownLink      = STTUNER_DOWNLINK_Ku;
    BandList[BAND_EU_LO].DownLink   = STTUNER_DOWNLINK_Ku;
    BandList[BAND_EU_HI].DownLink   = STTUNER_DOWNLINK_Ku;
#endif

    STTBX_Print(( "Calling STTUNER_SetBandList() ................\n" ));
    ReturnError = STTUNER_SetBandList(TUNERHandle, &Bands);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    Scans.ScanList = ScanList;

    ScanList[0].SymbolRate   = 20000000;           /* dummy parameters */
    ScanList[0].Polarization = STTUNER_PLR_ALL;
    ScanList[0].FECRates     = STTUNER_FEC_ALL;
    ScanList[0].Modulation   = STTUNER_MOD_QPSK;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
    ScanList[0].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[0].Band         = BAND_US;

    Scans.NumElements  = 1;

    STTBX_Print(( "Calling STTUNER_SetScanList() ................\n" ));
    ReturnError = STTUNER_SetScanList(TUNERHandle, &Scans);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    {
        STTUNER_Scan_t Scan;
        STTUNER_EventType_t TunerEvent;

        Scan.IQMode             = STTUNER_IQ_MODE_AUTO;
        Scan.Band               = BAND_EU_HI;

        Scan.LNBToneState       = STTUNER_LNB_TONE_22KHZ;
        Scan.Polarization       = POLARITY;
        Scan.FECRates           = FEC;
        Scan.SymbolRate         = 27500000;
        Scan.Modulation         = STTUNER_MOD_QPSK;
        Scan.AGC                = STTUNER_AGC_ENABLE;

        STTBX_Print(("STTUNER_SetFrequency(%s,%d)=",
                    TUNERDeviceName, FREQ_MHZ * 1000));
        ReturnError = STTUNER_SetFrequency(TUNERHandle,
                                           FREQ_MHZ * 1000, &Scan, 0);

        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        AwaitTunerEvent(&TunerEvent);

        if (TunerEvent == STTUNER_EV_LOCKED)
        {
            STTBX_Print(( "Tuner locked to frequency.\n\n" ));
        }
        else
        {
            STTBX_Print(( "Tuner failed to lock.\n\n" ));
            STTBX_Print(("Tuner event: "));
            switch (TunerEvent)
            {
                case STTUNER_EV_NO_OPERATION:
                    STTBX_Print(("STTUNER_EV_NO_OPERATION"));
                    break;
                case STTUNER_EV_LOCKED:
                    STTBX_Print(("STTUNER_EV_LOCKED"));
                    break;
                case STTUNER_EV_UNLOCKED:
                    STTBX_Print(("STTUNER_EV_UNLOCKED"));
                    break;
                case STTUNER_EV_SCAN_FAILED:
                    STTBX_Print(("STTUNER_EV_SCAN_FAILED"));
                    break;
                case STTUNER_EV_TIMEOUT:
                    STTBX_Print(("STTUNER_EV_TIMEOUT"));
                    break;
                case STTUNER_EV_SIGNAL_CHANGE:
                    STTBX_Print(("STTUNER_EV_SIGNAL_CHANGE"));
                    break;
                case STTUNER_EV_SCAN_NEXT:
                    STTBX_Print(("STTUNER_EV_SCAN_NEXT"));
                    break;
            }
            STTBX_Print(("\n"));
        }
    }
#endif

#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || \
    (defined(TTXDISK_TEST) && defined(ST_5518))
    STTBX_Print(( "Calling pti_init() ................\n" ));
    PtiReturn = pti_init(associate_descramblers_with_slots);
    STTBX_Print(( PtiReturn ? "pti_init() failure\n" :
                        "pti_init() success\n" ));

    /* Allocate slots for stb / vbi data */
    pti_allocate_dynamic_slot(&vbi_slot);
    pti_allocate_dynamic_slot(&stb_slot);
#elif defined(TTXSTPTI_TEST)
    STTBX_Print(("Calling STPTI_Init()...\n"));

    ReturnError = PTI_Init();
#elif defined(TTXSTDEMUX_TEST)
    FEI_Setup();
    FdmaSetup();
    DEMUX_Init();
#endif

#if defined (ST_7710) || defined (ST_7100) || defined(ST_7109)
    CLKRV_Init();
#endif

    /* Initialize the DENC */
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5107)
    DENCInitParams.DeviceType = STDENC_DEVICE_TYPE_V13;
#elif defined (ST_7020)
    DENCInitParams.DeviceType = STDENC_DEVICE_TYPE_7020;
#else /* 7710 too */
    DENCInitParams.DeviceType = STDENC_DEVICE_TYPE_DENC;
#endif

    DENCInitParams.AccessType = STDENC_ACCESS_TYPE_EMI;

#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7020) ||\
         defined (ST_7100) || defined (ST_5301) || defined(ST_7109) || defined(ST_5188) || defined(ST_5107)
    DENCInitParams.STDENC_Access.EMI.Width = STDENC_EMI_ACCESS_WIDTH_32_BITS;
#else
    DENCInitParams.STDENC_Access.EMI.Width = STDENC_EMI_ACCESS_WIDTH_8_BITS;
#endif

#if defined (ST_7020)
    DENCInitParams.STDENC_Access.EMI.BaseAddress_p = (void *)0x00008200;
    DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)0x60000000;
#else
    DENCInitParams.STDENC_Access.EMI.BaseAddress_p = (void *)DENC_BASE_ADDRESS;
    DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = 0;
#endif

    DENCInitParams.MaxOpen = 5;

    STTBX_Print(( "Calling STDENC_Init() ................\n" ));
    ReturnError = STDENC_Init(DENCDeviceName, &DENCInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Open and configure DENC device */
    STTBX_Print(( "Calling STDENC_Open() ................\n" ));
    ReturnError = STDENC_Open(DENCDeviceName, &DENCOpenParams, &DENCHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STDENC_SetEncodingMode() ................\n" ));
    DENCEncodingMode.Mode = STDENC_MODE_PALBDGHI;
    DENCEncodingMode.Option.Pal.SquarePixel = FALSE;
    DENCEncodingMode.Option.Pal.Interlaced = TRUE;
    ReturnError = STDENC_SetEncodingMode(DENCHandle,
                                         &DENCEncodingMode);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize the VTG */
    /* Simulation of Vsync events on 5518, 5514, 5516, 5517 */
    /* On 5528 and 5100 Vsync are genrated by VTG driver    */

#if defined (ST_7020)
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VTG_CELL_7020;

    VTGInitParams.Target.VtgCell.BaseAddress_p =       (void*)0x00008000;
    VTGInitParams.Target.VtgCell.DeviceBaseAddress_p = (void*)0x60000000;
    VTGInitParams.Target.VtgCell.InterruptEvent = 8;
    strcpy(VTGInitParams.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#elif defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5107)
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VFE2;
    VTGInitParams.Target.VtgCell2.BaseAddress_p       = (void*)VTG_BASE_ADDRESS;
    VTGInitParams.Target.VtgCell2.DeviceBaseAddress_p = (void*)0;
    VTGInitParams.Target.VtgCell2.InterruptNumber     = VTG_VSYNC_INTERRUPT;

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5107)
    VTGInitParams.Target.VtgCell2.InterruptLevel      = 0;
#else
    VTGInitParams.Target.VtgCell2.InterruptLevel      = 7;
#endif

#elif defined (ST_7710)
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VTG_CELL_7710;

    VTGInitParams.Target.VtgCell3.BaseAddress_p       = (void*)ST7710_VTG2_BASE_ADDRESS;
    VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)0;
    VTGInitParams.Target.VtgCell3.InterruptNumber     = 35;
    VTGInitParams.Target.VtgCell3.InterruptLevel      = 4;
    strcpy(VTGInitParams.Target.VtgCell3.ClkrvName, CLKRVDeviceName);

#elif defined (ST_7100)
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VTG_CELL_7100;

    VTGInitParams.Target.VtgCell3.BaseAddress_p       = (void*)ST7100_VTG2_BASE_ADDRESS;
    VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)0;
    VTGInitParams.Target.VtgCell3.InterruptNumber     = ST7100_VTG_1_INTERRUPT;
    VTGInitParams.Target.VtgCell3.InterruptLevel      = 4;
    strcpy(VTGInitParams.Target.VtgCell3.ClkrvName, CLKRVDeviceName);

#elif defined (ST_7109)
   VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VTG_CELL_7100;

   VTGInitParams.Target.VtgCell3.BaseAddress_p       = (void*)ST7109_VTG2_BASE_ADDRESS;
   VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)0;
   VTGInitParams.Target.VtgCell3.InterruptNumber     = ST7109_VTG_1_INTERRUPT;
   VTGInitParams.Target.VtgCell3.InterruptLevel      = 4;
   strcpy(VTGInitParams.Target.VtgCell3.ClkrvName, CLKRVDeviceName);

#else
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_DENC;
#endif

#ifdef STVTG_VSYNC_WITHOUT_VIDEO
    VTGInitParams.VideoBaseAddress_p       = (void*) VIDEO_BASE_ADDRESS;
    VTGInitParams.VideoDeviceBaseAddress_p = (void*) 0;
    VTGInitParams.VideoInterruptNumber     = VIDEO_INTERRUPT;
    VTGInitParams.VideoInterruptLevel      = VIDEO_INTERRUPT_LEVEL;
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */


    VTGInitParams.MaxOpen = 2;
    strcpy(VTGInitParams.EvtHandlerName, EVTDeviceName);
    if(VTGInitParams.DeviceType == STVTG_DEVICE_TYPE_DENC)
    {
        strcpy(VTGInitParams.Target.DencName, DENCDeviceName);
    }


    STTBX_Print(( "Calling STVTG_Init() ................\n" ));
    ReturnError = STVTG_Init(VTGDeviceName, &VTGInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    /* Open and configure the VTG */
    STTBX_Print(( "Calling STVTG_Open() ................\n" ));
    ReturnError = STVTG_Open(VTGDeviceName, &VTGOpenParams, &VTGHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Note - calling this function will disrupt anything which happens
     * to be being displayed at the time. Since TTX hasn't started yet,
     * though, this doesn't really bother us.
     */
    STTBX_Print(( "Calling STVTG_SetMode() ................\n" ));
    ReturnError = STVTG_SetMode(VTGHandle, STVTG_TIMING_MODE_576I50000_13500);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (ST_7020)
#define DSPCFG_DAC             0x200C       /* Dac configuration */
#define DSPCFG_ANA             0x0008       /* Main analog output config */
#define DSPCFG_DAC_ENABLE_SD   0x00020000   /* Enable SD DAC out */
#define DSPCFG_ANA_RGB         0x00000001   /* Enable RGB output */
#define ttx_Write32(a, v)     STSYS_WriteRegDev32LE((a), (v))


    printf ("%0x, %0x, %0x, %0x \n ",STI7020_BASE_ADDRESS, ST7020_DSPCFG_OFFSET, DSPCFG_DAC, DSPCFG_DAC_ENABLE_SD  );
    printf ("%0x, %0x \n ",DSPCFG_ANA, DSPCFG_ANA_RGB );

    ttx_Write32((U32)STI7020_BASE_ADDRESS + ST7020_DSPCFG_OFFSET + DSPCFG_DAC, DSPCFG_DAC_ENABLE_SD);
    ttx_Write32((U32)STI7020_BASE_ADDRESS + ST7020_DSPCFG_OFFSET + DSPCFG_ANA, DSPCFG_ANA_RGB);

    /* DENC_CFG = 0 */
    *(volatile U32*)(0x60008220) = 0x0;
#endif /* ST_7020 */

#ifdef ST_5528
        *(volatile U32*)(ST5528_VOUT_BASE_ADDRESS+0x04) = 0x0;
        *(volatile U32*)(ST5528_VOUT_BASE_ADDRESS+0x08) = 0x0;
        *(volatile U32*)(ST5528_DENC_BASE_ADDRESS+0x17c) = 0x17;
#endif

#if defined(ST_5100) || defined(ST_5105) || defined(ST_5188) || defined(ST_5107)|| defined(ST_7100) || defined(ST_7109)
    VOUT_Setup();
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    AVMEM_Init();
    VMIX_Init();
#endif


#ifdef STTTX_SUBT_SYNC_ENABLE

#if defined (ST_5514)|| defined(ST_5516)|| defined(ST_5517) ||defined(ST_5528)
    /* Initialize CLKRV */
    CLKRVInitParams.VCXOChannel       = CLKRV_VCXO_CHANNEL;
    CLKRVInitParams.InitialMark       = STCLKRV_INIT_MARK_VALUE;
    CLKRVInitParams.MinimumMark       = STCLKRV_MIN_MARK_VALUE;
    CLKRVInitParams.MaximumMark       = STCLKRV_MAX_MARK_VALUE;
    CLKRVInitParams.PCRMaxGlitch      = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitParams.PCRDriftThres     = STCLKRV_PCR_DRIFT_THRES;
    CLKRVInitParams.TicksPerMark      = STCLKRV_TICKS_PER_MARK;
    CLKRVInitParams.MinSampleThres    = STCLKRV_MIN_SAMPLE_THRESHOLD;
    CLKRVInitParams.MaxWindowSize     = STCLKRV_MAX_WINDOW_SIZE;
    CLKRVInitParams.DeviceType        = STCLKRV_DEVICE_PWMOUT;
    CLKRVInitParams.Partition_p       = TEST_PARTITION_1;
    strcpy( CLKRVInitParams.EVTDeviceName, EVTDeviceName);
    strcpy( CLKRVInitParams.PCREvtHandlerName, EVTDeviceName);
    strcpy( CLKRVInitParams.PWMDeviceName, PWMDeviceName);
    strcpy( CLKRVInitParams.PTIDeviceName, STPTIDeviceName);

    STTBX_Print(( "Calling STCLKRV_Init() ................\n" ));
    ReturnError = STCLKRV_Init(CLKRVDeviceName, &CLKRVInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#elif defined(ST_5100) || defined(ST_5105) || defined(ST_5301) || defined(ST_5107)
    CLKRVInitParams.PCRMaxGlitch      = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitParams.PCRDriftThres     = 300;
    CLKRVInitParams.MinSampleThres    = 10;
    CLKRVInitParams.MaxWindowSize     = 50;
    CLKRVInitParams.Partition_p       = TEST_PARTITION_1;
    CLKRVInitParams.DeviceType        = CLK_REC_DEVICE_TYPE;
    CLKRVInitParams.FSBaseAddress_p   = (U32 *)CKG_BASE_ADDRESS;
    CLKRVInitParams.ADSCBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
    CLKRVInitParams.InterruptNumber   = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitParams.InterruptLevel    = 7;

    strcpy( CLKRVInitParams.EVTDeviceName, EVTDeviceName );
    strcpy( CLKRVInitParams.PTIDeviceName, STPTIDeviceName );
    strcpy( CLKRVInitParams.PCREvtHandlerName, EVTDeviceName);
    STTBX_Print(( "Calling STCLKRV_Init() ................\n" ));
    ReturnError = STCLKRV_Init(CLKRVDeviceName, &CLKRVInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif /* defined (ST_5514)|| defined(ST_5516)|| defined(ST_5517) ||defined(ST_5528) */

#endif /* STTTX_SUBT_SYNC_ENABLE */

#if !defined(mb5518) && !defined(mb275) && !defined(mb376) && !defined(mb390) && !defined(mb391) \
    && !defined(mb400) && !defined(mb411) && !defined(mb457) && !defined(mb436) && !defined(DTT5107)
    Init641x();
#endif

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) \
|| defined(ST_5528)
    DoCfg();
#endif

#if !defined(STTTX_USE_DENC)
    /* Initialize the STVBI driver */
    STTBX_Print(( "Calling STVBI_Init() ................\n" ));
    VBIInitParams.DeviceType = STVBI_DEVICE_TYPE_DENC;
    VBIInitParams.MaxOpen = 1;
    strcpy(VBIInitParams.Target.DencName, DENCDeviceName);
    ReturnError = STVBI_Init(VBIDeviceName, &VBIInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Revision string available before STTTX Initialized */
}
#endif
/* global Init for dual test cases */

#if defined(TTXDUAL_DISK_TEST) || defined(TTXDUAL_STPTI_TEST)
void GlobalInit_Dual(void)
{
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError;
    STDENC_InitParams_t DENCInitParams;
    STDENC_OpenParams_t DENCOpenParams;
    STDENC_EncodingMode_t   DENCEncodingMode;
    STVTG_InitParams_t VTGInitParams;
    STVTG_OpenParams_t VTGOpenParams;
    STEVT_InitParams_t EVTInitParams;
#if !defined(STTTX_USE_DENC)
    STVBI_InitParams_t VBIInitParams;
#endif

#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || (defined(TTXDISK_TEST) && defined(ST_5518))
    boolean         PtiReturn;
#endif

    STVTG_SlaveModeParams_t  Slave_ModeParams;

    STEVT_OpenParams_t EVTOpenParams;
    STEVT_Handle_t EVTHandle;
    memset((char*)&Slave_ModeParams,0,sizeof(STVTG_SlaveModeParams_t));

    EventSemaphore_p  = STOS_SemaphoreCreateFifo(NULL, 0);
    FilterSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);

    ST_GetClockInfo(&ClockInfo);


    /* Initialize PIO and I2C */
    PioInitParams.BaseAddress = (U32 *)TUNER_SSC_PIO_BASE;
    PioInitParams.InterruptNumber = TUNER_SSC_PIO_INTERRUPT;
    PioInitParams.InterruptLevel = TUNER_SSC_PIO_LEVEL;
    PioInitParams.DriverPartition = TEST_PARTITION_1;

    STTBX_Print(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init(PioDeviceName,
                             &PioInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Initialize PIO and I2C */
    PioInitParams.BaseAddress = (U32 *)ST5528_PIO6_BASE_ADDRESS;
    PioInitParams.InterruptNumber = ST5528_PIO6_INTERRUPT;
    PioInitParams.InterruptLevel = PIO_6_INTERRUPT_LEVEL;
    PioInitParams.DriverPartition = TEST_PARTITION_1;

    STTBX_Print(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init(PioDeviceNameTTX,
                             &PioInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    PIO4629_1OpenParams.ReservedBits     = 0xC0;
    PIO4629_1OpenParams.BitConfigure[7]  = STPIO_BIT_ALTERNATE_OUTPUT;
    PIO4629_1OpenParams.BitConfigure[6]  = STPIO_BIT_INPUT;

    ReturnError = STPIO_Open( PioDeviceNameTTX,&PIO4629_1OpenParams, &GlbPio4629_1);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize EVT */
    EVTInitParams.MemoryPartition = (ST_Partition_t *)TEST_PARTITION_1;
    EVTInitParams.EventMaxNum = 20;
    EVTInitParams.ConnectMaxNum = 20;
    EVTInitParams.SubscrMaxNum = 20;
    EVTInitParams.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() ................\n" ));
    ReturnError = STEVT_Init(EVTDeviceName, &EVTInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* 5528 VTG Init */

    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VFE2;
    VTGInitParams.Target.VtgCell2.BaseAddress_p = (void *)VTG_BASE_ADDRESS;
    VTGInitParams.Target.VtgCell2.DeviceBaseAddress_p = (void *)0;
    VTGInitParams.Target.VtgCell2.InterruptNumber = VTG_VSYNC_INTERRUPT;
    VTGInitParams.Target.VtgCell2.InterruptLevel = 7;
    strcpy(VTGInitParams.EvtHandlerName, EVTDeviceName);
    VTGInitParams.MaxOpen = 2;

    STTBX_Print(( "Calling STVTG_Init ................\n" ));
    ReturnError = STVTG_Init(VTGDeviceName, &VTGInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Open and configure the VTG */
    STTBX_Print(( "Calling STVTG_Open ................\n" ));
    ReturnError = STVTG_Open(VTGDeviceName, &VTGOpenParams, &VTGHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Note - calling this function will disrupt anything which happens
     * to be being displayed at the time. Since TTX hasn't started yet,
     * though, this doesn't really bother us.
     */
    STTBX_Print(( "Calling STVTG_SetMode ................\n" ));
    ReturnError = STVTG_SetMode(VTGHandle, STVTG_TIMING_MODE_576I50000_13500);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    /* DENC Init */
    DENCInitParams.DeviceType = STDENC_DEVICE_TYPE_DENC;
    DENCInitParams.STDENC_Access.EMI.Width = STDENC_EMI_ACCESS_WIDTH_32_BITS;
    DENCInitParams.AccessType = STDENC_ACCESS_TYPE_EMI;
    DENCInitParams.STDENC_Access.EMI.BaseAddress_p = (void *)DENC_BASE_ADDRESS;
    DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = 0;
    DENCInitParams.MaxOpen = 5;

    STTBX_Print(( "Calling STDENC_Init(5528) ................\n" ));
    ReturnError = STDENC_Init(DENCDeviceName, &DENCInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open and configure 5528 DENC device */
    STTBX_Print(( "Calling STDENC_Open(5528) ................\n" ));
    ReturnError = STDENC_Open(DENCDeviceName, &DENCOpenParams, &DENCHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STDENC_SetEncodingMode(5528) ................\n" ));
    DENCEncodingMode.Mode = STDENC_MODE_PALBDGHI;
    DENCEncodingMode.Option.Pal.SquarePixel = FALSE;
    DENCEncodingMode.Option.Pal.Interlaced = TRUE;
    ReturnError = STDENC_SetEncodingMode(DENCHandle,
                                         &DENCEncodingMode);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    /* This register controls the Teletext interface. on 4629 */
#if defined (espresso)
    *(volatile U8*)(ST4629_TTX_CFG_ADDRESS) |= 0x01;
#elif defined (mb376)
    *(volatile U8*)(ST4629_TTX_CFG_ADDRESS) |= 0x01;
#endif

    /* 4629 DENC Init */
    DENCInitParams.DeviceType = STDENC_DEVICE_TYPE_4629;
    DENCInitParams.STDENC_Access.EMI.Width = STDENC_EMI_ACCESS_WIDTH_8_BITS;
    DENCInitParams.AccessType = STDENC_ACCESS_TYPE_EMI;
    DENCInitParams.STDENC_Access.EMI.BaseAddress_p = (void *)ST4629_DENC_BASE_ADDRESS;
    DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)0;
    DENCInitParams.MaxOpen = 5;

    STTBX_Print(( "Calling STDENC_Init(4629) ................\n" ));
    ReturnError = STDENC_Init(DENCDeviceName4629, &DENCInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open and configure 4629 DENC device */
    STTBX_Print(( "Calling STDENC_Open(4629) ................\n" ));
    ReturnError = STDENC_Open(DENCDeviceName4629, &DENCOpenParams, &DENCHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STDENC_SetEncodingMode(4629) ................\n" ));
    DENCEncodingMode.Mode = STDENC_MODE_PALBDGHI;
    DENCEncodingMode.Option.Pal.SquarePixel = FALSE;
    DENCEncodingMode.Option.Pal.Interlaced = TRUE;
    ReturnError = STDENC_SetEncodingMode(DENCHandle,
                                         &DENCEncodingMode);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Simulation of Vsync events on 5518, 5514, 5516, 5517 */
    /* On 5528 and 5100 Vsync are genrated by VTG driver    */
    /* Initialize the VTG */

    /* 4629 VTG Init */
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_DENC;
    strcpy(VTGInitParams.EvtHandlerName, EVTDeviceName); /* Same name as main VTG */
    VTGInitParams.MaxOpen = 2;
    strcpy( VTGInitParams.Target.DencName, DENCDeviceName4629);

    STTBX_Print(( "Calling STVTG_Init(4629) ................\n" ));
    ReturnError = STVTG_Init(VTGDeviceName4629, &VTGInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open and configure the VTG 4629 */
    STTBX_Print(( "Calling STVTG_Open(4629) ................\n" ));
    ReturnError = STVTG_Open(VTGDeviceName4629, &VTGOpenParams, &VTGHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    Slave_ModeParams.SlaveMode = STVTG_SLAVE_MODE_H_AND_V;
    Slave_ModeParams.Target.VfeSlaveOf = STVTG_DENC_SLAVE_OF_SYNC_IN_DATA;

    STTBX_Print(( "Calling STVTG_SetSlaveModeParams(4629) ................\n" ));
    ReturnError = STVTG_SetSlaveModeParams(VTGHandle,&Slave_ModeParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STVTG_SetMode(4629) ................\n" ));
    ReturnError = STVTG_SetMode(VTGHandle, STVTG_TIMING_MODE_SLAVE);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /***************************************
        TO BE REMOVED SOMEDAY
    ****************************************/


    VOUT_Setup(SERVICE_DISPLAY_PAL);
    /* Enable denc in YC mode from RGB */
    /**(volatile U32*)(0xb901117c) = 7;*/
    *(volatile U32*)(ST5528_DENC_BASE_ADDRESS+0x17c) = 0x17;

#if defined(TTXSTPTI_TEST)
    PTI_Init();
#elif defined(TTXSTDEMUX_TEST)
    FEI_Setup();
    FdmaSetup();
    DEMUX_Init();
#endif


#if !defined(STTTX_USE_DENC)

    /* Initialize the STVBI driver 5528 */
    STTBX_Print(( "Calling STVBI_Init(5528) ................\n" ));
    VBIInitParams.DeviceType = STVBI_DEVICE_TYPE_DENC;
    VBIInitParams.MaxOpen = 1;
    strcpy(VBIInitParams.Target.DencName, DENCDeviceName);
    ReturnError = STVBI_Init(VBIDeviceName, &VBIInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize the STVBI driver 4629 */
    STTBX_Print(( "Calling STVBI_Init(4629) ................\n" ));
    VBIInitParams.DeviceType = STVBI_DEVICE_TYPE_DENC;
    VBIInitParams.MaxOpen = 1;
    strcpy(VBIInitParams.Target.DencName, DENCDeviceName4629);
    ReturnError = STVBI_Init(VBIDeviceName4629, &VBIInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#endif

    ReturnError = STEVT_Open("STEVT", &EVTOpenParams, &EVTHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                            "STTTX",
                            STTTX_EVENT_PACKET_CONSUMED,
                            &DeviceSubscribeParams
                           );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                            "STTTX",
                            STTTX_EVENT_PACKET_DISCARDED,
                            &DeviceSubscribeParams
                           );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                            "STTTX_AUX",
                            STTTX_EVENT_PACKET_CONSUMED,
                            &DeviceSubscribeParams
                           );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                            "STTTX_AUX",
                            STTTX_EVENT_PACKET_DISCARDED,
                            &DeviceSubscribeParams
                           );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Revision string available before STTTX Initialized */
}
#endif

ST_ErrorCode_t GlobalTerm(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    /* STVTG_Term() */
    {
        STVTG_TermParams_t TermParams;
        TermParams.ForceTerminate = TRUE;
        error = STVTG_Term(VTGDeviceName, (STVTG_TermParams_t *)&TermParams);
    }
    /* STDENC_Term() */
    {
        STDENC_TermParams_t TermParams;
        TermParams.ForceTerminate = TRUE;
        error = STDENC_Term(DENCDeviceName, (STDENC_TermParams_t *)&TermParams);
    }
    /* STEVT_Term() */
    {
        STEVT_TermParams_t TermParams;
        TermParams.ForceTerminate = TRUE;
        error = STEVT_Term(EVTDeviceName, (STEVT_TermParams_t *)&TermParams);
    }

#if defined(TTXSTPTI_TEST)
    {
        STPTI_TermParams_t TermParams;
        TermParams.ForceTermination = TRUE;
        error = STPTI_Term(STPTIDeviceName, &TermParams);
    }
#elif defined(TTXSTDEMUX_TEST)
    {
        STDEMUX_TermParams_t TermParams;
        TermParams.ForceTermination = TRUE;

        error = STDEMUX_Term(STDEMUXDeviceName, &TermParams);
    }
#endif

#if defined(STTTX_USE_TUNER)
    /* STTUNER_Term() */
    {
        STTUNER_TermParams_t TermParams;
        TermParams.ForceTerminate = TRUE;

        error = STTUNER_Term(TUNERDeviceName, &TermParams);
    }

    /* STI2C_Term() */
    {
        STI2C_TermParams_t TermParams;
        TermParams.ForceTerminate = FALSE;

        error = STI2C_Term(I2CDeviceName, &TermParams);
    }
#endif
#ifndef ST_OSLINUX
    /* STPIO_Term() */
    {
        STPIO_TermParams_t TermParams;
        TermParams.ForceTerminate = FALSE;

        error = STPIO_Term(PioDeviceName, &TermParams);
    }
#endif
    STOS_SemaphoreDelete(NULL,EventSemaphore_p);
    STOS_SemaphoreDelete(NULL,FilterSemaphore_p);
    return error;
}

#ifndef ST_OSLINUX
void test_overhead(void *dummy) { }

void test_init(void *dummy)
{
    ST_ErrorCode_t error, stored;
    STTTX_InitParams_t InitParams_s;

    strcpy( DeviceNam, "STTTX" );
    InitParams_s.DriverPartition = (ST_Partition_t *)TEST_PARTITION_2;
    InitParams_s.DeviceType = STTTX_DEVICE_TYPE;
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;
    InitParams_s.InterruptNumber = TELETEXT_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 4;
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.VBIDeviceName, VBIDeviceName);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName);
#ifdef STTTX_SUBT_SYNC_ENABLE
    strcpy(InitParams_s.CLKRVDeviceName, CLKRVDeviceName);
#endif
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    strcpy(InitParams_s.VMIXDeviceName, VMIXDeviceName);
#endif
    error = STTTX_Init("STTTX", &InitParams_s);
    if (error != ST_NO_ERROR)
        ErrorReport(&stored, error, ST_NO_ERROR);
}

void test_typical(void *dummy)
{
    ST_ErrorCode_t          error, stored;
    InsertParams_t          VBIInsertParams;

    STTTX_OpenParams_t      OpenParams;
    STTTX_Handle_t          Handle;
    STTTX_Request_t         Request;
    STTTX_SourceParams_t    SourceParams;

    OpenParams.Type = STTTX_STB;
    error = STTTX_Open("STTTX", &OpenParams, &Handle);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Opening handle...\n"));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }

    VBIInsertParams.DriverPesBuffer_p = VBIInsertParams.PesBuffer_p;

    /* Set the source */

    VBIInsertParams.DataSemaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);
    SourceParams.DataSource                     = STTTX_SOURCE_USER_BUFFER;
#if defined(ST_OS20) || defined(ST_OS21)/* linux changes */
    SourceParams.Source_u.UserBuf_s.DataReady_p = VBIInsertParams.DataSemaphore_p;
#endif
    SourceParams.Source_u.UserBuf_s.PesBuf_p    = &VBIInsertParams.DriverPesBuffer_p;
    SourceParams.Source_u.UserBuf_s.BufferSize  = &VBIInsertParams.PesSize;
    error = STTTX_SetSource(Handle, &SourceParams);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Setting source...\n"));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }

    /* call Start */
    error = STTTX_Start( Handle );
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Starting...\n"));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }

    /* Filter on any packet */
    Request.RequestTag = 1;
    Request.RequestType = STTTX_FILTER_ANY;
    Request.NotifyFunction = TeletextNotifyFunction;
    Request.OddPage = &OddPage;
    Request.EvenPage = &EvenPage;
    error = STTTX_SetRequest( Handle, &Request );
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Calling setrequest...\n"));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }

    error = STTTX_Stop(Handle);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Stopping...\n"));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }

    /* Close the open Teletext channel explicitly */
    error = STTTX_Close( Handle );
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Closing handle...\n"));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }
#if defined(ST_OS20) || defined(ST_OS21)/* linux changes */
    STOS_SemaphoreDelete(NULL,VBIInsertParams.DataSemaphore_p);
#endif
}

void test_term(void *dummy)
{
    STTTX_TermParams_t TermParams;

    TermParams.ForceTerminate = TRUE;
    STTTX_Term("STTTX", &TermParams);
}

void DataSpaceUsed()
{
    ST_ErrorCode_t error, stored;
    STTTX_InitParams_t InitParams_s;
    STTTX_TermParams_t TermParams;
    U32 address1, address2;
    U32 Used;
    U32 *temporary;

    /* Find first address */
    temporary = memory_allocate(TEST_PARTITION_1, sizeof(U32));
    address1 = (U32)temporary;
    memory_deallocate(TEST_PARTITION_1, temporary);

    strcpy( DeviceNam, "STTTX" );
    /* Might as well be system partition */
    InitParams_s.DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
    InitParams_s.DeviceType = STTTX_DEVICE_TYPE;
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;
    InitParams_s.InterruptNumber = TELETEXT_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 1;
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.VBIDeviceName, VBIDeviceName);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName);
#ifdef STTTX_SUBT_SYNC_ENABLE
    strcpy(InitParams_s.CLKRVDeviceName, CLKRVDeviceName);
#endif
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    strcpy(InitParams_s.VMIXDeviceName, VMIXDeviceName);
#endif
#ifdef ST_7020
    InitParams_s.AVMEMPartition = AvmemPartitionHandle;
#endif
    error = STTTX_Init("STTTX", &InitParams_s);
    if (error != ST_NO_ERROR)
        ErrorReport(&stored, error, ST_NO_ERROR);

    /* Find address now */
    temporary = memory_allocate(TEST_PARTITION_1, sizeof(U32));
    if (temporary != NULL)
        address2 = (U32)temporary;
    else
        STTBX_Print(("Error allocating temporary marker\n"));
    memory_deallocate(TEST_PARTITION_1, temporary);
    Used = address1 - address2;
    STTBX_Print(("Space used for one STB object, one request: %i\n", Used));

    /* Term, and init with another STB object */
    TermParams.ForceTerminate = FALSE;
    STTTX_Term("STTTX", &TermParams);
    InitParams_s.NumSTBObjects = 2;
    error = STTTX_Init("STTTX", &InitParams_s);
    if (error != ST_NO_ERROR)
        ErrorReport(&stored, error, ST_NO_ERROR);

    /* Check address now */
    temporary = memory_allocate(TEST_PARTITION_1, sizeof(U32));
    if (temporary != NULL)
        address2 = (U32)temporary;
    else
        STTBX_Print(("Error allocating temporary marker\n"));
    memory_deallocate(TEST_PARTITION_1, temporary);
    Used = address1 - address2;
    STTBX_Print(("Space used for two STB objects, one request: %i\n", Used));

    /* Term, and init with another STB object */
    TermParams.ForceTerminate = FALSE;
    STTTX_Term("STTTX", &TermParams);
    InitParams_s.NumSTBObjects = 3;
    error = STTTX_Init("STTTX", &InitParams_s);
    if (error != ST_NO_ERROR)
        ErrorReport(&stored, error, ST_NO_ERROR);

    /* Check address now */
    temporary = memory_allocate(TEST_PARTITION_1, sizeof(U32));
    if (temporary != NULL)
        address2 = (U32)temporary;
    else
        STTBX_Print(("Error allocating temporary marker\n"));
    memory_deallocate(TEST_PARTITION_1, temporary);
    Used = address1 - address2;
    STTBX_Print(("Space used for three STB objects, one request: %i\n", Used));

    /* Term, and init with another request */
    STTTX_Term("STTTX", &TermParams);
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 2;
    error = STTTX_Init("STTTX", &InitParams_s);
    if (error != ST_NO_ERROR)
        ErrorReport(&stored, error, ST_NO_ERROR);

    /* Check address now */
    temporary = memory_allocate(TEST_PARTITION_1, sizeof(U32));
    if (temporary != NULL)
        address2 = (U32)temporary;
    else
        STTBX_Print(("Error allocating temporary marker\n"));
    memory_deallocate(TEST_PARTITION_1, temporary);
    Used = address1 - address2;
    STTBX_Print(("Space used for one STB object, two requests: %i\n", Used));

    /* Term, and init with another request */
    STTTX_Term("STTTX", &TermParams);
    InitParams_s.NumRequestsAllowed = 3;
    error = STTTX_Init("STTTX", &InitParams_s);
    if (error != ST_NO_ERROR)
        ErrorReport(&stored, error, ST_NO_ERROR);

    /* Check address now */
    temporary = memory_allocate(TEST_PARTITION_1, sizeof(U32));
    if (temporary != NULL)
        address2 = (U32)temporary;
    else
        STTBX_Print(("Error allocating temporary marker\n"));
    memory_deallocate(TEST_PARTITION_1, temporary);
    Used = address1 - address2;
    STTBX_Print(("Space used for one STB object, three requests: %i\n", Used));

    /* Term */
    STTTX_Term("STTTX", &TermParams);
}

void StackUsage()
{
#ifdef ST_OS20
    task_t          task;
    tdesc_t         tdesc;
#endif
    task_t          *task_p;
    task_status_t   status;
    char            stack[64 * 1024];

    void (*func_table[])(void *) = {
                    test_overhead,
                    test_init,
                    test_typical,
                    test_term,
                    NULL
                };
    void (*func)(void *);
    partition_status_t before, after;
    partition_status_flags_t flags/* = partition_status_flags_dummy*/;
    int ret;
    U8 loop;

    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing Stack Usage Test Function ....\n" ));
    STTBX_Print(( "==========================================================\n" ));

    printf("Getting 'before' partition status: \n");
    ret = partition_status(TEST_PARTITION_2, &before, flags);
    if (ret != 0)
        printf("error!! in partition_status \n");

#ifdef ST_OS20
    task_p = &task;
#endif

    for (loop = 0; func_table[loop] != NULL; loop++)
    {
        func = func_table[loop];

#ifdef ST_OS21
        task_p = task_create(func, NULL, 24 * 1024, MAX_USER_PRIORITY,
                             "stack_test", 0);
#elif defined(ST_OS20)
        task_init(func, NULL, stack, 16 * 1024,
                  task_p, &tdesc, MAX_USER_PRIORITY,
                  "stack_test", (task_flags_t)0);
#endif
        STOS_TaskWait(&task_p, TIMEOUT_INFINITY);

        task_status(task_p, &status, task_status_flags_stack_used);

        printf("Function %i, stack = %d\n", loop, status.task_stack_used);

        task_delete(task_p);
    }

    printf("Getting 'after' partition status: \n");
    partition_status(TEST_PARTITION_2, &after, flags);
    if (ret != 0)
        printf("error!! in partition_status \n");

    if (before.partition_status_free != after.partition_status_free)
    {
        STTBX_Print(("!!! Memory leak - started with %i bytes free, ended with %i bytes !!!\n",
                    before.partition_status_free,
                    after.partition_status_free));
    }
    else
    {
        STTBX_Print(("No leak - memory free (before and after): %i\n",
                    before.partition_status_free));
    }

    DataSpaceUsed();
}
#endif

#if defined(STTTX_USE_TUNER)
void TunerNotifyFunction(STTUNER_Handle_t Handle,
                         STTUNER_EventType_t EventType, ST_ErrorCode_t Error)
{
    LastEvent = EventType;
    STOS_SemaphoreSignal(EventSemaphore_p);
}

void AwaitTunerEvent(STTUNER_EventType_t *EventType_p)
{
    STOS_SemaphoreWait(EventSemaphore_p);
    *EventType_p = LastEvent;
}
#endif

void TeletextNotifyFunction( U32 Tag, STTTX_Page_t *p1, STTTX_Page_t *p2 )
{
    STOS_SemaphoreSignal(FilterSemaphore_p);
}

void AwaitFilterHit(void)
{
    STOS_SemaphoreWait(FilterSemaphore_p);
}
#if defined(STTTX_DEBUG1)
void TeletextNotifyPeslost(STEVT_CallbackProc_t Reason, STEVT_EventConstant_t Event, const void *EventData)
{
    switch (Event)                      /* Output event */
    {
        case STTTX_EVENT_PES_LOST:
            STTBX_Print(("Event = STTTX_EVENT_PES_LOST\n"));
            break;
        case STTTX_EVENT_PES_NOT_CONSUMED:
            STTBX_Print(("Event = STTTX_EVENT_PES_NOT_CONSUMED\n"));
            break;
        case STTTX_EVENT_PES_INVALID:
            STTBX_Print(("Event = STTTX_EVENT_PES_INVALID\n"));
            break;
    }
}
#endif

#if defined(TTXDUAL_DISK_TEST)  || defined(TTXDUAL_STPTI_TEST)

U32 pes_available_count_5528 = 0;
U32 pkt_consumed_count_5528  = 0;
U32 pkt_discarded_count_5528 = 0;
U32 pes_available_count_4629 = 0;
U32 pkt_consumed_count_4629  = 0;
U32 pkt_discarded_count_4629 = 0;


void TeletextEvtNotify(STEVT_CallReason_t Reason,
                       const ST_DeviceName_t RegistrantName,
                       STEVT_EventConstant_t Event,
                       const void *EventData,
                       const void *SubscriberData_p
                      )
{


    if (!strcmp("STTTX",RegistrantName))
    {
        switch ( Event )
        {
            case STTTX_EVENT_PACKET_CONSUMED:
                pkt_consumed_count_5528++;
                break;
            case STTTX_EVENT_PACKET_DISCARDED:
                pkt_discarded_count_5528++;
                break;
            default:
                break;
        }

    }
    else if (!strcmp("STTTX_AUX",RegistrantName))
    {
        switch ( Event )
        {
            case STTTX_EVENT_PACKET_CONSUMED:
                pkt_consumed_count_4629++;
                break;
            case STTTX_EVENT_PACKET_DISCARDED:
                pkt_discarded_count_4629++;
                break;
            default:
                break;
        }
    }

}

#endif


#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST)
/****************************************************************************
Name         : NormalUsePti()

Description  : Performs all the API interface calls in the correct
               sequence, with legal parameters, exercising the message
               queue creation, interrupt install and thread creation
               followed by their deletion/removal. Uses old PTI driver.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors detected
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel
               STTTX_ERROR_INTERRUPT_INSTALL   Interrupt (de)installation failed
               STTTX_ERROR_TASK_CREATION       Failure of a thread to initialize

See Also     : STTTX_ErrorCode_t
 ****************************************************************************/

void NormalUsePti( void )
{
    U32             Pid;
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError;
    ST_Revision_t   RevisionStr;
    STTTX_SourceParams_t SourceParams;
#if defined(STTTX_USE_TUNER)
    STTTX_Request_t Request;
    STTUNER_EventType_t TunerEvent;
    STTUNER_Scan_t Scan;
#endif

    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing NormalUse Test Function ....\n" ));
    STTBX_Print(( "==========================================================\n" ));


    /* Initialize our native Teletext Driver */
    strcpy( DeviceNam, "STTTX" );
    InitParams_s.DriverPartition = (ST_Partition_t *)TEST_PARTITION_2;
    InitParams_s.DeviceType = STTTX_DEVICE_TYPE;
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;
    InitParams_s.InterruptNumber = TELETEXT_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 4;
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.VBIDeviceName, VBIDeviceName);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName);
#ifdef STTTX_SUBT_SYNC_ENABLE
    strcpy(InitParams_s.CLKRVDeviceName, CLKRVDeviceName);
#endif
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));


    STTBX_Print(( "Calling STTTX_Init() ................\n" ));
    ReturnError = STTTX_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    {
        STTSMUX_InitParams_t  TSMUXInitParams;
        STTSMUX_OpenParams_t  TSMUXOpenParams;
        STTSMUX_Handle_t      TSMUXHandle;
        ST_ErrorCode_t        Error;

        /* Initialise TSMUX device */
        STTBX_Print(("Initialising TSMUX\n\n"));
        TSMUXInitParams.DeviceType = STTSMUX_DEVICE_GENERIC;
        TSMUXInitParams.DriverPartition_p = TEST_PARTITION_1;
        TSMUXInitParams.BaseAddress_p = (U32*)TSMUX_BASE_ADDRESS;

        if (TSMUXInitParams.BaseAddress_p == NULL)
        {
            STTBX_Print(("***ERROR: Invalid device base address OR simulator memory allocation error\n\n"));
        }

        TSMUXInitParams.ClockFrequency = ClockInfo.STBus;
        TSMUXInitParams.MaxHandles = 1;
        STTBX_Print(("Initialising TSMUX driver (at addr 0x%x)\n",(U32)TSMUXInitParams.BaseAddress_p));
        STTBX_Print(("Calling STTSMUX_Init\n"));
        Error = STTSMUX_Init(TSMUXName,&TSMUXInitParams);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* Open tsmux */
        STTBX_Print(("Opening TSMUX for use\n\n"));
        STTBX_Print(("Calling STTSMUX_Open\n"));
        Error = STTSMUX_Open(TSMUXName,&TSMUXOpenParams,&TSMUXHandle);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* Setup TSIN for stream */
        STTBX_Print(("Configure TSIN 1 use\n\n"));
        STTBX_Print(("Calling STTSMUX_SetTSMode\n"));
        Error = STTSMUX_SetTSMode(TSMUXHandle, STTSMUX_TSIN_1,
                                  STTSMUX_TSMODE_PARALLEL);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("Calling STTSMUX_SetSyncMode\n"));
        Error = STTSMUX_SetSyncMode(TSMUXHandle, STTSMUX_TSIN_1,
                                    STTSMUX_SYNCMODE_SYNCHRONOUS);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* Setup TSIN for stream */
        STTBX_Print(("Configure TSIN 2 use\n\n"));
        STTBX_Print(("Calling STTSMUX_SetTSMode\n"));
        Error = STTSMUX_SetTSMode(TSMUXHandle, STTSMUX_TSIN_2,
                                  STTSMUX_TSMODE_PARALLEL);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("Calling STTSMUX_SetSyncMode\n"));
        Error = STTSMUX_SetSyncMode(TSMUXHandle, STTSMUX_TSIN_2,
                                    STTSMUX_SYNCMODE_SYNCHRONOUS);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* TSIN 2 -> PTI B */
#if defined(TTXPTI_TEST) && defined(ST_5514)
        STTBX_Print(("Connecting TSIN 2 to PTI B\n\n"));
        STTBX_Print(("Calling STTSMUX_Connect PTI B\n"));
        Error = STTSMUX_Connect(TSMUXHandle,STTSMUX_TSIN_2, STTSMUX_PTI_B);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
        STTBX_Print(("Connecting TSIN 2 to PTI A\n\n"));
        STTBX_Print(("Calling STTSMUX_Connect PTI A\n"));
        Error = STTSMUX_Connect(TSMUXHandle,STTSMUX_TSIN_2, STTSMUX_PTI_A);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif
    }
#endif /* 5514 || 5516 || 5517 */

    /* Open a PID channel for VBI injection */
    OpenParams_s.Type = STTTX_VBI;
    OpenParams_s.Slot = vbi_slot;
    STTBX_Print(( "Calling STTTX_Open() VBI Type .......\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &VBIHandle );
    STTBX_Print(("OpenIndex returned as %d\n", VBIHandle & OPEN_HANDLE_MSK ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set the source */
    STTBX_Print(( "Calling STTTX_SetSource() ...............\n" ));
    SourceParams.DataSource = STTTX_SOURCE_PTI_SLOT;
    SourceParams.Source_u.PTISlot_s.Slot = vbi_slot;
    ReturnError = STTTX_SetSource(VBIHandle, &SourceParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* call SetStreamID prior to Start with a valid PID */
    Pid = VBI_PID;
    STTBX_Print(( "Calling STTTX_SetStreamID() PID %d ...\n", Pid ));
    ReturnError = STTTX_SetStreamID( VBIHandle, Pid );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* call Start */
    STTBX_Print(( "Calling STTTX_Start() ...............\n" ));
    ReturnError = STTTX_Start( VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXPTI_TEST) || defined (TTXLINK_TEST)
    {
        U16 count = 0;

        pti_slot_packet_count(vbi_slot, &count);
        STTBX_Print(("Slot packet count: %u\n", count));

        STOS_TaskDelay(ST_GetClocksPerSecond());

        pti_slot_packet_count(vbi_slot, &count);
        STTBX_Print(("Slot packet count: %u\n", count));
    }
#endif

#if defined(STTTX_USE_TUNER)
    /* Open a PID channel, STB instead of VBI */
    OpenParams_s.Type = STTTX_STB;
    OpenParams_s.Slot = stb_slot;
    STTBX_Print(( "Calling STTTX_Open(), STB post ReInit\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &STBHandle );
    STTBX_Print(("OpenIndex returned as %d\n", STBHandle & OPEN_HANDLE_MSK ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set the source - should work without this too. */
    STTBX_Print(( "Calling STTTX_SetSource() ...............\n" ));
    SourceParams.DataSource = STTTX_SOURCE_PTI_SLOT;
    SourceParams.Source_u.PTISlot_s.Slot = stb_slot;
    ReturnError = STTTX_SetSource(STBHandle, &SourceParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* call SetStreamID prior to Start with a valid PID */
    Pid = STB_PID;
    STTBX_Print(( "Calling STTTX_SetStreamID(), PID %d ..\n", Pid ));
    ReturnError = STTTX_SetStreamID( STBHandle, Pid );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STTTX_Start() ...............\n" ));
    ReturnError = STTTX_Start( STBHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Filter on any packet */
    STTBX_Print(("Setting up STTTX_FILTER_ANY ...............\n"));
    Request.RequestTag = 1;
    Request.RequestType = STTTX_FILTER_ANY;
    Request.NotifyFunction = TeletextNotifyFunction;
    Request.OddPage = &OddPage;
    Request.EvenPage = &EvenPage;
    STTBX_Print(( "Calling STTTX_SetRequest() ...............\n" ));
    ReturnError = STTTX_SetRequest( STBHandle, &Request );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    STTBX_Print(( "Waiting for filter hit ...............\n" ));
    AwaitFilterHit();
    STTBX_Print(("Done.\n"));

    /* Filter on X/0 packet */
    STTBX_Print(("Setting up STTTX_FILTER_PACKET_0 ...............\n"));
    Request.RequestTag = 1;
    Request.RequestType = STTTX_FILTER_PACKET_0;
    Request.NotifyFunction = TeletextNotifyFunction;
    Request.OddPage = &OddPage;
    Request.EvenPage = &EvenPage;
    STTBX_Print(( "Calling STTTX_SetRequest() ...............\n" ));
    ReturnError = STTTX_SetRequest( STBHandle, &Request );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    STTBX_Print(( "Waiting for filter hit ...............\n" ));
    AwaitFilterHit();
    STTBX_Print(("Done.\n"));

    /* Filter on X/30 packet */
    STTBX_Print(("Setting up STTTX_FILTER_PACKET_30 ...............\n"));
    Request.RequestTag = 1;
    Request.RequestType = STTTX_FILTER_PACKET_30;
    Request.NotifyFunction = TeletextNotifyFunction;
    Request.OddPage = &OddPage;
    Request.EvenPage = &EvenPage;
    STTBX_Print(( "Calling STTTX_SetRequest() ...............\n" ));
    ReturnError = STTTX_SetRequest( STBHandle, &Request );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    STTBX_Print(( "Waiting for filter hit ...............\n" ));
    AwaitFilterHit();
    STTBX_Print(("Done.\n"));

    /* Filter on a page */
    STTBX_Print(("Setting up filter for page 100 ...............\n"));
    Request.RequestTag = 1;
    Request.RequestType = STTTX_FILTER_MAGAZINE | STTTX_FILTER_PAGE;
    Request.NotifyFunction = TeletextNotifyFunction;
    Request.OddPage = &OddPage;
    Request.EvenPage = &EvenPage;
    Request.MagazineNumber = 1;
    Request.PageNumber = 0x00;

    STTBX_Print(( "Calling STTTX_SetRequest() ...............\n" ));
    ReturnError = STTTX_SetRequest( STBHandle, &Request );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    STTBX_Print(( "Waiting for filter hit ...............\n" ));
    AwaitFilterHit();
    STTBX_Print(("Done.\n"));

    STTBX_Print(( "Swapping STB and VBI pids ...............\n" ));
    STTBX_Print(( "Calling STTTX_Stop() ...............\n" ));
    ReturnError = STTTX_Stop( STBHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    STTBX_Print(( "Calling STTTX_SetStreamID() ...............\n" ));
    ReturnError = STTTX_SetStreamID( VBIHandle, STB_PID );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    STTBX_Print(( "Calling STTTX_SetStreamID() ...............\n" ));
    ReturnError = STTTX_SetStreamID( STBHandle, VBI_PID );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    /* Delay for a reasonable period to allow VBI teletext to be seen
     * by the caller.
     */
    STTBX_Print(("Waiting for VBI test timeout to end ...............\n"));

    STOS_TaskDelay(TTX_VBI_PERIOD*ST_GetClocksPerSecond());
    STOS_TaskDelay(TTX_VBI_PERIOD*ST_GetClocksPerSecond());

    /* Close the open Teletext channel explicitly */

    STTBX_Print(( "Calling STTTX_Close() ...............\n" ));
    ReturnError = STTTX_Close( VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(STTTX_USE_TUNER)
    STTBX_Print(( "Calling STTTX_Close() ...............\n" ));
    ReturnError = STTTX_Close( STBHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Unforced Teletext Termination */

    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STTTX_Term() unforced .......\n" ));
    ReturnError = STTTX_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* note no pti termination call - none exists at time of writing */
    /* STPTI term does, though I can't see a way to safely use it.   */

    /* NormalUse Tests completed */

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
}
#endif

#if defined(TTXSTPTI_TEST)
/****************************************************************************
Name         : NormalUseStpti()

Description  : Performs all the API interface calls in the correct
               sequence, with legal parameters, exercising the message
               queue creation, interrupt install and thread creation
               followed by their deletion/removal. Uses old PTI driver.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors detected
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel
               STTTX_ERROR_INTERRUPT_INSTALL   Interrupt (de)installation failed
               STTTX_ERROR_TASK_CREATION       Failure of a thread to initialize

See Also     : STTTX_ErrorCode_t
 ****************************************************************************/

void NormalUseStpti( void )
{
    STPTI_Pid_t             Pid;
    ST_ErrorCode_t          StoredError = ST_NO_ERROR;
    ST_ErrorCode_t          ReturnError;
    ST_Revision_t           RevisionStr;
    STTTX_SourceParams_t    SourceParams;

#if defined(STTTX_USE_TUNER)
    STTTX_Request_t         Request;
#endif

    STPTI_OpenParams_t      STPTI_OpenParams;
    STPTI_Handle_t          STPTI_Handle;

    U32                     BufferSize;

    STPTI_Buffer_t          VBIBuffer, STBBuffer;
    STPTI_Signal_t          VBISignal, STBSignal;
    STPTI_SlotData_t        SlotData;
#ifdef STTTX_SUBT_SYNC_ENABLE
    STCLKRV_OpenParams_t    CLKRVOpenParams;
    STCLKRV_SourceParams_t  PCRSrc;
    STCLKRV_Handle_t        CLKRV_Handle;
    U16                     SyncTime;
#endif
    const U32               TicksPerSec = ST_GetClocksPerSecond();
#if defined (TTXDUAL_STPTI_TEST)
    STPTI_Handle_t          STPTI_Handle4629;
    STPTI_Buffer_t          VBIBuffer4629, STBBuffer4629;
    STPTI_Signal_t          VBISignal4629, STBSignal4629;
    STPTI_SlotData_t        SlotData4629;
#endif

    STTBX_Print(("TicksPerSec=%u\n",TicksPerSec));

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing NormalUse Test Function ....\n" ));
    STTBX_Print(( "==========================================================\n" ));

    /* revision string available before STTTX Initialized */

    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));

#if defined(STTTX_USE_TUNER)
    STTBX_Print(("Setting tuner frequency...\n"));
    ReturnError = DoTunerScan();
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Open session with STPTI */
    STTBX_Print(("Calling STPTI_Open()...\n"));
    STPTI_OpenParams.DriverPartition_p = TEST_PARTITION_1;
    STPTI_OpenParams.NonCachedPartition_p = TEST_PARTITION_2;
    ReturnError = STPTI_Open(STPTIDeviceName, &STPTI_OpenParams,
                             &STPTI_Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_Open(second)...\n"));
    ReturnError = STPTI_Open(STPTIDeviceName4629, &STPTI_OpenParams,
                             &STPTI_Handle4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif


    /* Get slots */
    SlotData.SlotType = STPTI_SLOT_TYPE_PES;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
#if defined (ST_5528) || defined (ST_5100) || defined (ST_7710) || defined (ST_5105) ||\
    defined (ST_7100) || defined (ST_5301) || defined (ST_7109) || defined(ST_5107)
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;
#endif

#if defined (TTXDUAL_STPTI_TEST)
    /* Get slots */
    SlotData4629.SlotType = STPTI_SLOT_TYPE_PES;
    SlotData4629.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData4629.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData4629.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData4629.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData4629.SlotFlags.InsertSequenceError = FALSE;
    SlotData4629.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData4629.SlotFlags.ForcePesLengthToZero = FALSE;
    SlotData4629.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
#if defined (ST_5528)
    SlotData4629.SlotFlags.SoftwareCDFifo = FALSE;
#endif
#endif


#if !defined(NO_VBI)
    STTBX_Print(("Calling STPTI_SlotAllocate()...\n"));
    ReturnError = STPTI_SlotAllocate(STPTI_Handle, &VBISlot, &SlotData);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SlotAllocate(4629)...\n"));
    ReturnError = STPTI_SlotAllocate(STPTI_Handle4629, &VBISlot4629, &SlotData4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#endif
    STTBX_Print(("Calling STPTI_SlotAllocate()...\n"));
    ReturnError = STPTI_SlotAllocate(STPTI_Handle, &STBSlot, &SlotData);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    ReturnError = STPTI_SlotClearPid(STBSlot);

#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SlotAllocate(4629)...\n"));
    ReturnError = STPTI_SlotAllocate(STPTI_Handle4629, &STBSlot4629, &SlotData4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    ReturnError = STPTI_SlotClearPid(STBSlot4629);
#endif

#ifdef STTTX_SUBT_SYNC_ENABLE
    /* Allocate slot type */
    SlotData.SlotType = STPTI_SLOT_TYPE_PCR;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
#if defined (ST_5528) || defined (ST_5100) || defined (ST_7710) || defined (ST_5105) ||\
    defined (ST_7100) || defined (ST_5301) || defined (ST_7109) || defined(ST_5107)
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;
#endif

    STTBX_Print(( "Calling STPTI_SlotAllocate() ..........\n" ));
    ReturnError = STPTI_SlotAllocate(STPTI_Handle, &PCRSlot, &SlotData);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    /* Get buffers */

    BufferSize = STTTX_BUFFER_SIZE;
#if !defined(NO_VBI)
    STTBX_Print(("Calling STPTI_BufferAllocate()...\n"));
    ReturnError = STPTI_BufferAllocate(STPTI_Handle, BufferSize, 1, &VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_BufferAllocate(4629)...\n"));
    ReturnError = STPTI_BufferAllocate(STPTI_Handle4629, BufferSize, 1, &VBIBuffer4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
#endif
    STTBX_Print(("Calling STPTI_BufferAllocate()...\n"));
    ReturnError = STPTI_BufferAllocate(STPTI_Handle, BufferSize, 1, &STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_BufferAllocate(4629)...\n"));
    ReturnError = STPTI_BufferAllocate(STPTI_Handle4629, BufferSize, 1, &STBBuffer4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Get signals */
#if !defined(NO_VBI)
    STTBX_Print(("Calling STPTI_SignalAllocate()...\n"));
    ReturnError = STPTI_SignalAllocate(STPTI_Handle, &VBISignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SignalAllocate(4629)...\n"));
    ReturnError = STPTI_SignalAllocate(STPTI_Handle4629, &VBISignal4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
#endif

    STTBX_Print(("Calling STPTI_SignalAllocate()...\n"));
    ReturnError = STPTI_SignalAllocate(STPTI_Handle, &STBSignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SignalAllocate(4629)...\n"));
    ReturnError = STPTI_SignalAllocate(STPTI_Handle4629, &STBSignal4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Link them */
#if !defined(NO_VBI)
    STTBX_Print(("Calling STPTI_SlotLinkToBuffer()...\n"));
    ReturnError = STPTI_SlotLinkToBuffer(VBISlot, VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SlotLinkToBuffer(4629)...\n"));
    ReturnError = STPTI_SlotLinkToBuffer(VBISlot4629, VBIBuffer4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
#endif
    STTBX_Print(("Calling STPTI_SlotLinkToBuffer()...\n"));
    ReturnError = STPTI_SlotLinkToBuffer(STBSlot, STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SlotLinkToBuffer(4629)...\n"));
    ReturnError = STPTI_SlotLinkToBuffer(STBSlot4629, STBBuffer4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
#if !defined(NO_VBI)
    STTBX_Print(("Calling STPTI_SignalAssociateBuffer()...\n"));
    ReturnError = STPTI_SignalAssociateBuffer(VBISignal, VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SignalAssociateBuffer(4629)...\n"));
    ReturnError = STPTI_SignalAssociateBuffer(VBISignal4629, VBIBuffer4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
#endif
    STTBX_Print(("Calling STPTI_SignalAssociateBuffer()...\n"));
    ReturnError = STPTI_SignalAssociateBuffer(STBSignal, STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SignalAssociateBuffer(4629)...\n"));
    ReturnError = STPTI_SignalAssociateBuffer(STBSignal4629, STBBuffer4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)

    {
        STTSMUX_InitParams_t  TSMUXInitParams;
        STTSMUX_OpenParams_t  TSMUXOpenParams;
        STTSMUX_Handle_t      TSMUXHandle;
        ST_ErrorCode_t        Error;

        /* Initialise TSMUX device */
        STTBX_Print(("Initialising TSMUX\n\n"));
        TSMUXInitParams.DeviceType = STTSMUX_DEVICE_GENERIC;
        TSMUXInitParams.DriverPartition_p = TEST_PARTITION_1;
        TSMUXInitParams.BaseAddress_p = (U32*)TSMUX_BASE_ADDRESS;

        if (TSMUXInitParams.BaseAddress_p == NULL)
        {
            STTBX_Print(("***ERROR: Invalid device base address OR simulator memory allocation error\n\n"));
        }

        TSMUXInitParams.ClockFrequency = ClockInfo.STBus;
        TSMUXInitParams.MaxHandles = 1;
        STTBX_Print(("Initialising TSMUX driver (at addr 0x%x)\n",(U32)TSMUXInitParams.BaseAddress_p));
        STTBX_Print(("Calling STTSMUX_Init\n"));
        Error = STTSMUX_Init(TSMUXName,&TSMUXInitParams);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* Open tsmux */
        STTBX_Print(("Opening TSMUX for use\n\n"));
        STTBX_Print(("Calling STTSMUX_Open\n"));
        Error = STTSMUX_Open(TSMUXName,&TSMUXOpenParams,&TSMUXHandle);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* Setup TSIN for stream */
        STTBX_Print(("Configure TSIN 1 use\n\n"));
        STTBX_Print(("Calling STTSMUX_SetTSMode\n"));
        Error = STTSMUX_SetTSMode(TSMUXHandle, STTSMUX_TSIN_1,
                                  STTSMUX_TSMODE_PARALLEL);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("Calling STTSMUX_SetSyncMode\n"));
        Error = STTSMUX_SetSyncMode(TSMUXHandle, STTSMUX_TSIN_1,
                                    STTSMUX_SYNCMODE_SYNCHRONOUS);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* Setup TSIN for stream */
        STTBX_Print(("Configure TSIN 2 use\n\n"));
        STTBX_Print(("Calling STTSMUX_SetTSMode\n"));
        Error = STTSMUX_SetTSMode(TSMUXHandle, STTSMUX_TSIN_2,
                                  STTSMUX_TSMODE_PARALLEL);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("Calling STTSMUX_SetSyncMode\n"));
        Error = STTSMUX_SetSyncMode(TSMUXHandle, STTSMUX_TSIN_2,
                                    STTSMUX_SYNCMODE_SYNCHRONOUS);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* TSIN 2 -> PTI B */
#if defined(TTXPTI_TEST)
        STTBX_Print(("Connecting TSIN 2 to PTI B\n\n"));
        STTBX_Print(("Calling STTSMUX_Connect PTI B\n"));
        Error = STTSMUX_Connect(TSMUXHandle,STTSMUX_TSIN_2, STTSMUX_PTI_B);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
        STTBX_Print(("Connecting TSIN 2 to PTI A\n\n"));
        STTBX_Print(("Calling STTSMUX_Connect PTI A\n"));
        Error = STTSMUX_Connect(TSMUXHandle,STTSMUX_TSIN_2, STTSMUX_PTI_A);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif
    }
#endif /* 5514 || 5516 || 5517 */

#if defined(ST_5528) || defined (ST_5100) || defined (ST_7710) || defined(ST_7100) ||\
    defined (ST_5301) || defined (ST_7109)
    {
        ST_ErrorCode_t Error;
        Error = ConfigureMerger(&StoredError);
        ErrorReport(&StoredError, Error, ST_NO_ERROR);
    }
#endif

    /* Initialize our native Teletext Driver */

    /* Initialize our native Teletext Driver 5528 */
    strcpy( DeviceNam, "STTTX" );
    InitParams_s.DriverPartition = (ST_Partition_t *)TEST_PARTITION_2;
    InitParams_s.DeviceType = STTTX_DEVICE_TYPE;
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;
    InitParams_s.InterruptNumber = TELETEXT_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 4;
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.VBIDeviceName, VBIDeviceName);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName);
#ifdef STTTX_SUBT_SYNC_ENABLE
    strcpy(InitParams_s.CLKRVDeviceName, CLKRVDeviceName);
#endif
#if defined(ST_5105) || defined(ST_5107)
    strcpy(InitParams_s.VMIXDeviceName, VMIXDeviceName);
#endif
#ifdef ST_7020
    InitParams_s.AVMEMPartition = AvmemPartitionHandle;
#endif
    STTBX_Print(( "Calling STTTX_Init ................\n" ));
    ReturnError = STTTX_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (TTXDUAL_STPTI_TEST)
    strcpy( DeviceNam4629, "STTTX_AUX" );
    InitParams_s.DriverPartition = (ST_Partition_t *)TEST_PARTITION_2;
    InitParams_s.BaseAddress = (U32*)TTXT1_BASE_ADDRESS;
    InitParams_s.InterruptNumber = ST5528_TTX1_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 4;
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.VBIDeviceName, VBIDeviceName4629);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName4629);


    STTBX_Print(( "Calling STTTX_Init(4629) ................\n" ));
    ReturnError = STTTX_Init( DeviceNam4629, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#if !defined(NO_VBI)
    /* Open a PID channel for VBI injection */
    OpenParams_s.Type = STTTX_VBI;
    STTBX_Print(( "Calling STTTX_Open() VBI Type .......\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &VBIHandle );
    STTBX_Print(("OpenIndex returned as %d\n", VBIHandle & OPEN_HANDLE_MSK ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(( "Calling STTTX_Open(4629) VBI Type .......\n" ));
    ReturnError = STTTX_Open( DeviceNam4629, &OpenParams_s, &VBIHandle4629 );
    STTBX_Print(("OpenIndex returned as %d\n", VBIHandle4629 & OPEN_HANDLE_MSK ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Set the source */
    STTBX_Print(( "Calling STTTX_SetSource() ...............\n" ));
    SourceParams.DataSource = STTTX_SOURCE_STPTI_SIGNAL;
    SourceParams.Source_u.STPTISignal_s.Signal = VBISignal;
    ReturnError = STTTX_SetSource(VBIHandle, &SourceParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(( "Calling STTTX_SetSource(4629) ...............\n" ));
    SourceParams.DataSource = STTTX_SOURCE_STPTI_SIGNAL;
    SourceParams.Source_u.STPTISignal_s.Signal = VBISignal4629;
    ReturnError = STTTX_SetSource(VBIHandle4629, &SourceParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* call SetStreamID prior to Start with a valid PID */
    Pid = VBI_PID;
    STTBX_Print(( "Calling STPTI_SlotSetPid(), VBI PID %d ..\n", Pid ));
    ReturnError = STPTI_SlotSetPid(VBISlot, Pid);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXSTPTI_TEST)
    {
        U16                 PacketCount=0;
        ST_ErrorCode_t      Error;
        STTBX_Print(("Input packet count : "));
        STOS_TaskDelay(ST_GetClocksPerSecond());
        Error = STPTI_GetInputPacketCount(STPTIDeviceName, &PacketCount);
        STTBX_Print(("%u \n",PacketCount));
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        PacketCount = 0;
        Error = STPTI_SlotPacketCount(VBISlot,&PacketCount);
        STTBX_Print(("Slot count %u \n",PacketCount));
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
#endif

#ifdef STTTX_SUBT_SYNC_ENABLE
    Pid = PCR_PID;
    STTBX_Print(( "Calling STPTI_SlotSetPid(), PCR PID %d ..\n", Pid ));
    ReturnError = STPTI_SlotSetPid(PCRSlot, Pid);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Calling STCLKRV_Open() ..............\n" ));
    ReturnError = STCLKRV_Open( CLKRVDeviceName, &CLKRVOpenParams, &CLKRV_Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

   /* Set Slot ID for STPTI communication */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource()...\n"));
    PCRSrc.Source_u.STPTI_s.Slot = PCRSlot;
    ReturnError = STCLKRV_SetPCRSource(CLKRV_Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    STTBX_Print(( "Calling STTTX_SetSyncOffset() ...............\n" ));
    SyncTime = 80;
    ReturnError = STTTX_SetSyncOffset( VBIHandle, SyncTime );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#if defined (TTXDUAL_STPTI_TEST)
    Pid = VBI_PID;
    STTBX_Print(( "Calling STPTI_SlotSetPid(), VBI PID %d ..\n", Pid ));
    ReturnError = STPTI_SlotSetPid(VBISlot4629, Pid);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* call Start */
    STTBX_Print(( "Calling STTTX_Start() ...............\n" ));
    ReturnError = STTTX_Start( VBIHandle );
#ifndef ST_OSLINUX
#ifndef ARCHITECTURE_ST200
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
#endif

#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(( "Calling STTTX_Start(4629) ...............\n" ));
    ReturnError = STTTX_Start( VBIHandle4629 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#endif /* NO_VBI */

    /* Delay for a reasonable period to allow VBI teletext to be seen
     * by the caller.
     */
/*    task_priority_set (NULL, MIN_USER_PRIORITY);*/
#ifndef ST_OSLINUX
#ifndef ARCHITECTURE_ST200
    STTBX_Print(("Waiting for VBI test timeout to end ...............\n"));
#endif
#endif

    STOS_TaskDelay(TTX_VBI_PERIOD*ST_GetClocksPerSecond());
    STOS_TaskDelay(TTX_VBI_PERIOD*ST_GetClocksPerSecond());
#if defined(STTTX_DEBUG1)

    STTBX_Print(("\nWORK AROUND Count = %d \n",DmaWaCnt));
    STTBX_Print(("\nPES LOST Count=%d\n",Pes_Lost));
#endif /* STTTX_DEBUG1 */

    /* Close the open Teletext channel */
    STTBX_Print(( "Calling STTTX_Close() VBI ...............\n" ));
    ReturnError = STTTX_Close( VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(( "Calling STTTX_Close(4629) VBI ...............\n" ));
    ReturnError = STTTX_Close( VBIHandle4629 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    /* Unforced Teletext Termination */

    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STTTX_Term() unforced .......\n" ));
    ReturnError = STTTX_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(( "Calling STTTX_Term(4629) unforced .......\n" ));
    ReturnError = STTTX_Term( DeviceNam4629, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    /* NormalUse Tests completed */

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Close the Clock Recovery Driver */
#ifdef STTTX_SUBT_SYNC_ENABLE
    STTBX_Print(( "Calling STCLKRV_Close() ..............\n" ));
    ReturnError = STCLKRV_Close( CLKRV_Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Break links */
    STTBX_Print(("Calling STPTI_SlotUnLink()...\n"));
    ReturnError = STPTI_SlotUnLink(VBISlot);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SlotUnLink(4629)...\n"));
    ReturnError = STPTI_SlotUnLink(VBISlot4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(("Calling STPTI_SlotUnLink()...\n"));
    ReturnError = STPTI_SlotUnLink(STBSlot);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SlotUnLink(4629)...\n"));
    ReturnError = STPTI_SlotUnLink(STBSlot4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(("Calling STPTI_SignalDisassociateBuffer()...\n"));
    ReturnError = STPTI_SignalDisassociateBuffer(VBISignal, VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SignalDisassociateBuffer(4629)...\n"));
    ReturnError = STPTI_SignalDisassociateBuffer(VBISignal4629, VBIBuffer4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STPTI_SignalDisassociateBuffer()...\n"));
    ReturnError = STPTI_SignalDisassociateBuffer(STBSignal, STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Free slots */
#ifdef STTTX_SUBT_SYNC_ENABLE
    STTBX_Print(( "Calling STPTI_SlotDeallocate() ...\n" ));
    ReturnError = STPTI_SlotDeallocate(PCRSlot);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(("Calling STPTI_SlotDeallocate()...\n"));
    ReturnError = STPTI_SlotDeallocate(VBISlot);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_SlotDeallocate(4629)...\n"));
    ReturnError = STPTI_SlotDeallocate(VBISlot4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(("Calling STPTI_SlotDeallocate()...\n"));
    ReturnError = STPTI_SlotDeallocate(STBSlot);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Free buffers */
    STTBX_Print(("Calling STPTI_BufferDeallocate()...\n"));
    ReturnError = STPTI_BufferDeallocate(VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    STTBX_Print(("Calling STPTI_BufferDeallocate(4629)...\n"));
    ReturnError = STPTI_BufferDeallocate(VBIBuffer4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(("Calling STPTI_BufferDeallocate()...\n"));
    ReturnError = STPTI_BufferDeallocate(STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Free signals */
    STTBX_Print(("Calling STPTI_SignalDeallocate()...\n"));
    ReturnError = STPTI_SignalDeallocate(VBISignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    /* Free signals */
    STTBX_Print(("Calling STPTI_SignalDeallocate()...\n"));
    ReturnError = STPTI_SignalDeallocate(VBISignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(("Calling STPTI_SignalDeallocate()...\n"));
    ReturnError = STPTI_SignalDeallocate(STBSignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Close STPTI session */
    STTBX_Print(("Calling STPTI_Close()...\n"));
    ReturnError = STPTI_Close(STPTI_Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if defined (TTXDUAL_STPTI_TEST)
    /* Close STPTI session */
    STTBX_Print(("Calling STPTI_Close()...\n"));
    ReturnError = STPTI_Close(STPTI_Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

}
#endif

#if defined(TTXSTDEMUX_TEST)


/****************************************************************************
Name         : NormalUseStdemux()

Description  : Performs all the API interface calls in the correct
               sequence, with legal parameters, exercising the message
               queue creation, interrupt install and thread creation
               followed by their deletion/removal. Uses old PTI driver.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors detected
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel
               STTTX_ERROR_INTERRUPT_INSTALL   Interrupt (de)installation failed
               STTTX_ERROR_TASK_CREATION       Failure of a thread to initialize

See Also     : STTTX_ErrorCode_t
 ****************************************************************************/

void NormalUseStdemux( void )
{
    ST_ErrorCode_t          StoredError = ST_NO_ERROR;
    ST_ErrorCode_t          ReturnError;
    ST_Revision_t           RevisionStr;
    STTTX_SourceParams_t    SourceParams;

    STDEMUX_OpenParams_t      STDEMUX_OpenParams;
    STDEMUX_StreamParams_t    STDEMUX_StreamParams;
    STDEMUX_Handle_t          STDEMUX_Handle;

    U32                     BufferSize;

    STDEMUX_Buffer_t          VBIBuffer, STBBuffer;
    STDEMUX_Signal_t          VBISignal, STBSignal;
    STDEMUX_SlotData_t        SlotData;
#ifdef STTTX_SUBT_SYNC_ENABLE
    STCLKRV_OpenParams_t    CLKRVOpenParams;
    STCLKRV_SourceParams_t  PCRSrc;
    STCLKRV_Handle_t        CLKRV_Handle;
    U16                     SyncTime;
#endif
    const U32               TicksPerSec = ST_GetClocksPerSecond();

    STTBX_Print(("TicksPerSec=%u\n",TicksPerSec));

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing NormalUse Test Function ....\n" ));
    STTBX_Print(( "==========================================================\n" ));

    /* revision string available before STTTX Initialized */

    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));

    /* Open session with STDEMUX */
    STTBX_Print(("Calling STDEMUX_Open()...\n"));
    STDEMUX_OpenParams.DriverPartition_p = TEST_PARTITION_1;
    STDEMUX_OpenParams.NonCachedPartition_p = TEST_PARTITION_2;
    ReturnError = STDEMUX_Open(STDEMUXDeviceName, &STDEMUX_OpenParams,
                             &STDEMUX_Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Get slots */
    SlotData.SlotType = STDEMUX_SLOT_TYPE_PES;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.DiscardOnCrcError = FALSE;
    #if defined(ST_8010) || defined(ST_5188)
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;
    #endif

#if !defined(NO_VBI)
    STTBX_Print(("Calling STDEMUX_SlotAllocate()...\n"));
    ReturnError = STDEMUX_SlotAllocate(STDEMUX_Handle, &VBISlot, &SlotData);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STDEMUX_SlotAllocate()...\n"));
    ReturnError = STDEMUX_SlotAllocate(STDEMUX_Handle, &STBSlot, &SlotData);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    ReturnError = STDEMUX_SlotClearParams(STBSlot);

#ifdef STTTX_SUBT_SYNC_ENABLE
    /* Allocate slot type */
    SlotData.SlotType = STDEMUX_SLOT_TYPE_PCR;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.DiscardOnCrcError = FALSE;
#if defined(ST_8010) || defined(ST_5188)
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;
#endif

    STTBX_Print(( "Calling STDEMUX_SlotAllocate() ..........\n" ));
    ReturnError = STDEMUX_SlotAllocate(STDEMUX_Handle, &PCRSlot, &SlotData);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    /* Get buffers */

    BufferSize = STTTX_BUFFER_SIZE;
#if !defined(NO_VBI)
    STTBX_Print(("Calling STDEMUX_BufferAllocate()...\n"));
    ReturnError = STDEMUX_BufferAllocate(STDEMUX_Handle, BufferSize, 1, &VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STDEMUX_BufferAllocate()...\n"));
    ReturnError = STDEMUX_BufferAllocate(STDEMUX_Handle, BufferSize, 1, &STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Get signals */
#if !defined(NO_VBI)
    STTBX_Print(("Calling STDEMUX_SignalAllocate()...\n"));
    ReturnError = STDEMUX_SignalAllocate(STDEMUX_Handle, &VBISignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(("Calling STDEMUX_SignalAllocate()...\n"));
    ReturnError = STDEMUX_SignalAllocate(STDEMUX_Handle, &STBSignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Link them */
#if !defined(NO_VBI)
    STTBX_Print(("Calling STDEMUX_SlotLinkToBuffer()...\n"));
    ReturnError = STDEMUX_SlotLinkToBuffer(VBISlot, VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STDEMUX_SlotLinkToBuffer()...\n"));
    ReturnError = STDEMUX_SlotLinkToBuffer(STBSlot, STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if !defined(NO_VBI)
    STTBX_Print(("Calling STDEMUX_SignalAssociateBuffer()...\n"));
    ReturnError = STDEMUX_SignalAssociateBuffer(VBISignal, VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STDEMUX_SignalAssociateBuffer()...\n"));
    ReturnError = STDEMUX_SignalAssociateBuffer(STBSignal, STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize our native Teletext Driver */

    /* Initialize our native Teletext Driver 5528 */
    /* Initialize our native Teletext Driver */
    strcpy( DeviceNam, "STTTX" );
    InitParams_s.DriverPartition = (ST_Partition_t *)TEST_PARTITION_2;
    InitParams_s.DeviceType = STTTX_DEVICE_TYPE;
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;
    InitParams_s.InterruptNumber = TELETEXT_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 4;
    strcpy(InitParams_s.VBIDeviceName, VBIDeviceName);
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName);
#if defined(ST_5105) || defined(ST_5188)
    strcpy(InitParams_s.VMIXDeviceName, VMIXDeviceName);
#endif

    STTBX_Print(( "Calling STTTX_Init ................\n" ));
    ReturnError = STTTX_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if !defined(NO_VBI)
    /* Open a PID channel for VBI injection */
    OpenParams_s.Type = STTTX_VBI;
    STTBX_Print(( "Calling STTTX_Open() VBI Type .......\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &VBIHandle );
    STTBX_Print(("OpenIndex returned as %d\n", VBIHandle & OPEN_HANDLE_MSK ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set the source */
    STTBX_Print(( "Calling STTTX_SetSource() ...............\n" ));
    SourceParams.DataSource = STTTX_SOURCE_STDEMUX_SIGNAL;
    SourceParams.Source_u.STDEMUXSignal_s.Signal = VBISignal;
    ReturnError = STTTX_SetSource(VBIHandle, &SourceParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* call SetStreamID prior to Start with a valid PID */
    STDEMUX_StreamParams.member.Pid = VBI_PID;
    STTBX_Print(( "Calling STDEMUX_SlotSetParams(), VBI PID %d ..\n", STDEMUX_StreamParams.member.Pid ));
    ReturnError = STDEMUX_SlotSetParams(VBISlot, &STDEMUX_StreamParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    {
        U32                 PacketCount;
        ST_ErrorCode_t      Error;

        STTBX_Print(("Input packet count : "));

        STOS_TaskDelay(ST_GetClocksPerSecond());

        Error = STDEMUX_GetInputPacketCount(STDEMUXDeviceName, &PacketCount);
        STTBX_Print(("%u \n",PacketCount));
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        PacketCount = 0;
        Error = STDEMUX_SlotPacketCount(VBISlot,&PacketCount);
        STTBX_Print(("Slot count %u \n",PacketCount));
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

    }

#ifdef STTTX_SUBT_SYNC_ENABLE
    STDEMUX_StreamParams.member.Pid = PCR_PID;
    STTBX_Print(( "Calling STDEMUX_SlotSetParams(), PCR PID %d ..\n", STDEMUX_StreamParams.member.Pid ));
    ReturnError = STDEMUX_SlotSetParams(PCRSlot, &STDEMUX_StreamParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Calling STCLKRV_Open() ..............\n" ));
    ReturnError = STCLKRV_Open( CLKRVDeviceName, &CLKRVOpenParams, &CLKRV_Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

   /* Set Slot ID for STDEMUX communication */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource()...\n"));
    PCRSrc.Source_u.STDEMUX_s.Slot = PCRSlot;
    ReturnError = STCLKRV_SetPCRSource(CLKRV_Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    STTBX_Print(( "Calling STTTX_SetSyncOffset() ...............\n" ));
    SyncTime = 80;
    ReturnError = STTTX_SetSyncOffset( VBIHandle, SyncTime );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* call Start */
    STTBX_Print(( "Calling STTTX_Start() ...............\n" ));
    ReturnError = STTTX_Start( VBIHandle );
#ifndef ST_OSLINUX
#ifndef ARCHITECTURE_ST200
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
#endif

#endif /* NO_VBI */

    /* Delay for a reasonable period to allow VBI teletext to be seen
     * by the caller.
     */
    task_priority_set (NULL, MIN_USER_PRIORITY);
#ifndef ST_OSLINUX
#ifndef ARCHITECTURE_ST200
    STTBX_Print(("Waiting for VBI test timeout to end ...............\n"));
#endif
#endif

        STOS_TaskDelay(TTX_VBI_PERIOD*ST_GetClocksPerSecond());
        STOS_TaskDelay(TTX_VBI_PERIOD*ST_GetClocksPerSecond());

#if defined(STTTX_DEBUG1)

    STTBX_Print(("\nWORK AROUND Count = %d \n",DmaWaCnt));
    STTBX_Print(("\nPES LOST Count=%d\n",Pes_Lost));

#endif /* STTTX_DEBUG1 */

    /* Close the open Teletext channel */
    STTBX_Print(( "Calling STTTX_Close() VBI ...............\n" ));
    ReturnError = STTTX_Close( VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Teletext Termination */

    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STTTX_Term() unforced .......\n" ));
    ReturnError = STTTX_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Close the Clock Recovery Driver */
#ifdef STTTX_SUBT_SYNC_ENABLE
    STTBX_Print(( "Calling STCLKRV_Close() ..............\n" ));
    ReturnError = STCLKRV_Close( CLKRV_Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Break links */
    STTBX_Print(("Calling STDEMUX_SlotUnLink()...\n"));
    ReturnError = STDEMUX_SlotUnLink(VBISlot);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Calling STDEMUX_SlotUnLink()...\n"));
    ReturnError = STDEMUX_SlotUnLink(STBSlot);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Calling STDEMUX_SignalDisassociateBuffer()...\n"));
    ReturnError = STDEMUX_SignalDisassociateBuffer(VBISignal, VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Calling STDEMUX_SignalDisassociateBuffer()...\n"));
    ReturnError = STDEMUX_SignalDisassociateBuffer(STBSignal, STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Free slots */
#ifdef STTTX_SUBT_SYNC_ENABLE
    STTBX_Print(( "Calling STDEMUX_SlotDeallocate() ...\n" ));
    ReturnError = STDEMUX_SlotDeallocate(PCRSlot);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(("Calling STDEMUX_SlotDeallocate()...\n"));
    ReturnError = STDEMUX_SlotDeallocate(VBISlot);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Calling STDEMUX_SlotDeallocate()...\n"));
    ReturnError = STDEMUX_SlotDeallocate(STBSlot);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Free buffers */
    STTBX_Print(("Calling STDEMUX_BufferDeallocate()...\n"));
    ReturnError = STDEMUX_BufferDeallocate(VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Calling STDEMUX_BufferDeallocate()...\n"));
    ReturnError = STDEMUX_BufferDeallocate(STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Free signals */
    STTBX_Print(("Calling STDEMUX_SignalDeallocate()...\n"));
    ReturnError = STDEMUX_SignalDeallocate(VBISignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Calling STDEMUX_SignalDeallocate()...\n"));
    ReturnError = STDEMUX_SignalDeallocate(STBSignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Close STDEMUX session */
    STTBX_Print(("Calling STDEMUX_Close()...\n"));
    ReturnError = STDEMUX_Close(STDEMUX_Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

}
#endif

#if defined(TTXDISK_TEST)

/****************************************************************************
Name         : NormalUseDisk()

Description  : Performs all the API interface calls in the correct
               sequence, with legal parameters, exercising the message
               queue creation, interrupt install and thread creation
               followed by their deletion/removal. Uses old PTI driver.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors detected
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel
               STTTX_ERROR_INTERRUPT_INSTALL   Interrupt (de)installation failed
               STTTX_ERROR_TASK_CREATION       Failure of a thread to initialize

See Also     : STTTX_ErrorCode_t
 ****************************************************************************/
void NormalUseDisk( void )
{
    U8              VBIInsertMainStack[2048];
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError;
    ST_Revision_t   RevisionStr;
    InsertParams_t  VBIInsertParams;
    STTTX_SourceParams_t  SourceParams;
#ifdef ST_OS20
    task_t          IMVBITask;
    tdesc_t         IMVBItdesc;
#endif
    task_t          *IMVBITask_p;
#if defined(TTXDUAL_DISK_TEST)
    U8              VBIInsertMainStack4629[2048];
    InsertParams_t  VBIInsertParams4629;
#ifdef ST_OS20
    task_t          IMVBITask4629;
    tdesc_t         IMVBItdesc4629;
#endif
    task_t          *IMVBITask4629_p;
#endif

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing NormalUse Test Function ....\n" ));
    STTBX_Print(( "==========================================================\n" ));

    /* revision string available before STTTX Initialized */

    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));

    /* Initialize our native Teletext Driver 5528 */
    strcpy( DeviceNam, "STTTX" );
    InitParams_s.DriverPartition = (ST_Partition_t *)TEST_PARTITION_2;
    InitParams_s.DeviceType = STTTX_DEVICE_TYPE;
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;
    InitParams_s.InterruptNumber = TELETEXT_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 4;
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.VBIDeviceName, VBIDeviceName);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName);
#ifdef STTTX_SUBT_SYNC_ENABLE
    strcpy(InitParams_s.CLKRVDeviceName, CLKRVDeviceName);
#endif
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    strcpy(InitParams_s.VMIXDeviceName, VMIXDeviceName);
#endif
#ifdef ST_7020
    InitParams_s.AVMEMPartition = AvmemPartitionHandle;
#endif
    STTBX_Print(( "Calling STTTX_Init ................\n" ));
    ReturnError = STTTX_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    VBIInsertParams.DataSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

#if defined(TTXDUAL_DISK_TEST)
    /* Initialize our native Teletext Driver 4629 */
    strcpy( DeviceNam4629, "STTTX_AUX" );
    InitParams_s.DriverPartition = (ST_Partition_t *)TEST_PARTITION_2;
    InitParams_s.BaseAddress = (U32*)TTXT1_BASE_ADDRESS;
    InitParams_s.InterruptNumber = ST5528_TTX1_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 4;
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.VBIDeviceName, VBIDeviceName4629);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName4629);

    STTBX_Print(( "Calling STTTX_Init(4629) ................\n" ));
    ReturnError = STTTX_Init( DeviceNam4629, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    VBIInsertParams4629.DataSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);
#endif

    /* Open a PID channel for VBI injection */
    OpenParams_s.Type = STTTX_VBI;
    STTBX_Print(( "Calling STTTX_Open VBI Type .......\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &VBIHandle );
    STTBX_Print(("OpenIndex returned as %d\n", VBIHandle & OPEN_HANDLE_MSK ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXDUAL_DISK_TEST)
    OpenParams_s.Type = STTTX_VBI;
    STTBX_Print(( "Calling STTTX_Open(4629) VBI Type .......\n" ));
    ReturnError = STTTX_Open( DeviceNam4629, &OpenParams_s, &VBIHandle4629 );
    STTBX_Print(("OpenIndex returned as %d\n", VBIHandle & OPEN_HANDLE_MSK ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Do the insert setup */
    if (InsertInit(&VBIInsertParams.PesBuffer_p, &VBIInsertParams.TotalSize) != 0)
    {
        /* Unforced Teletext Termination */
        TermParams_s.ForceTerminate = TRUE;
        STTBX_Print(( "Calling STTTX_Term forced .......\n" ));
        ReturnError = STTTX_Term( DeviceNam, &TermParams_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "Calling STTTX_Term forced .......\n" ));
        ReturnError = STTTX_Term( DeviceNam, &TermParams_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        return;
    }
    VBIInsertParams.DriverPesBuffer_p = VBIInsertParams.PesBuffer_p;

#if defined(TTXDUAL_DISK_TEST)
    /* Do the insert setup */
    if (InsertInit4629(&VBIInsertParams4629.PesBuffer_p, &VBIInsertParams4629.TotalSize) != 0)
    {
        /* Unforced Teletext Termination */
        TermParams_s.ForceTerminate = TRUE;
        STTBX_Print(( "Calling STTTX_Term(4629) forced .......\n" ));
        ReturnError = STTTX_Term( DeviceNam4629, &TermParams_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "Calling STTTX_Term(4629) forced .......\n" ));
        ReturnError = STTTX_Term( DeviceNam4629, &TermParams_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        return;
    }
    VBIInsertParams4629.DriverPesBuffer_p = VBIInsertParams.PesBuffer_p;
#endif

    /* Set the source */
    STTBX_Print(( "Calling STTTX_SetSource...............\n" ));
    SourceParams.DataSource = STTTX_SOURCE_USER_BUFFER;
#if defined(ST_OS20) || defined(ST_OS21)/* linux changes */
    SourceParams.Source_u.UserBuf_s.DataReady_p = VBIInsertParams.DataSemaphore_p;
#endif
    SourceParams.Source_u.UserBuf_s.PesBuf_p = &VBIInsertParams.DriverPesBuffer_p;
    SourceParams.Source_u.UserBuf_s.BufferSize = &VBIInsertParams.PesSize;
    ReturnError = STTTX_SetSource(VBIHandle, &SourceParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXDUAL_DISK_TEST)
    /* Set the source */
    STTBX_Print(( "Calling STTTX_SetSource(4629) ...............\n" ));
    SourceParams.DataSource = STTTX_SOURCE_USER_BUFFER;
#if defined(ST_OS20) || defined(ST_OS21)/* linux changes */
    SourceParams.Source_u.UserBuf_s.DataReady_p = VBIInsertParams4629.DataSemaphore_p;
#endif
    SourceParams.Source_u.UserBuf_s.PesBuf_p = &VBIInsertParams4629.DriverPesBuffer_p;
    SourceParams.Source_u.UserBuf_s.BufferSize = &VBIInsertParams4629.PesSize;
    ReturnError = STTTX_SetSource(VBIHandle4629, &SourceParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#ifdef ST_OS21
    IMVBITask_p = task_create( (void(*)(void *))InsertMain,
                               (void *)&VBIInsertParams,
                               16*1024,(int)MIN_USER_PRIORITY,
                               "VBI packet insertion",0);
#elif defined(ST_OS20)
    task_init( (void(*)(void *))InsertMain,
                       (void *)&VBIInsertParams,
                       VBIInsertMainStack, 2048,
                       &IMVBITask, &IMVBItdesc,
                       MIN_USER_PRIORITY,
                       "VBI packet insertion",(task_flags_t) 0);
    IMVBITask_p = &IMVBITask;
#endif

    /* Spawn off the VBI insert task */
    if (IMVBITask_p == NULL)
    {
        STTBX_Print(("Error trying to spawn VBI InsertMain task\n"));
    }

#if defined(TTXDUAL_DISK_TEST)

#ifdef ST_OS21
    IMVBITask4629_p = task_create( (void(*)(void *))InsertMain4629,
                                   (void *)&VBIInsertParams4629,
                                   16*1024,(int)MIN_USER_PRIORITY,
                                   "VBI packet  4629 insertion",0);
#elif defined(ST_OS20)
    task_init( (void(*)(void *))InsertMain4629,
                       (void *)&VBIInsertParams4629,
                       VBIInsertMainStack4629, 2048,
                       &IMVBITask4629, &IMVBItdesc4629,
                       MIN_USER_PRIORITY,
                       "VBI packet  4629 insertion",(task_flags_t) 0);
    IMVBITask4629_p = &IMVBITask4629;
#endif
    /* Spawn off the VBI insert task */
    if(IMVBITask4629_p == NULL)
    {
        STTBX_Print(("Error trying to spawn VBI InsertMain task\n"));
    }
#endif

    /* call Start */
    STTBX_Print(( "Calling STTTX_Start...............\n" ));
    ReturnError = STTTX_Start( VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXDUAL_DISK_TEST)
    /* call Start */
    STTBX_Print(( "Calling STTTX_Start(4629) ...............\n" ));
    ReturnError = STTTX_Start( VBIHandle4629 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Delay for a reasonable period to allow VBI teletext to be seen
     * by the caller.
     */
    task_priority_set (NULL, MIN_USER_PRIORITY);
#ifndef ST_OSLINUX
#ifndef ARCHITECTURE_ST200
    STTBX_Print(("Waiting for VBI test timeout to end ...............\n"));
#endif
#endif

    STOS_TaskDelay(TTX_VBI_PERIOD*ST_GetClocksPerSecond());
    STOS_TaskDelay(TTX_VBI_PERIOD*ST_GetClocksPerSecond());

    STTBX_Print(("Calling STTTX_Stop()\n"));
    ReturnError = STTTX_Stop(VBIHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXDUAL_DISK_TEST)
    STTBX_Print(("Calling STTTX_Stop(4629)\n"));
    ReturnError = STTTX_Stop(VBIHandle4629);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#if defined(TTXDUAL_DISK_TEST)
    STTBX_Print(("\n5528 Event information\n"));
    STTBX_Print(("pkt_consumed_count_5528 = %u\n", pkt_consumed_count_5528 ));
    STTBX_Print(("pkt_discarded_count_5528 = %u\n", pkt_discarded_count_5528 ));
    STTBX_Print(("\n4629 Event information\n"));
    STTBX_Print(("pkt_consumed_count_4629 = %u\n", pkt_consumed_count_4629 ));
    STTBX_Print(("pkt_discarded_count_4629 = %u\n", pkt_discarded_count_4629 ));
#endif

    /* Close the open Teletext channel explicitly */

    STTBX_Print(( "Calling STTTX_Close ...............\n" ));
    ReturnError = STTTX_Close( VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXDUAL_DISK_TEST)
    STTBX_Print(( "Calling STTTX_Close(4629) ...............\n" ));
    ReturnError = STTTX_Close( VBIHandle4629 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Kill the insert task */
    InsertTaskExit = TRUE;

    STOS_TaskDelay(ST_GetClocksPerSecond() / 25);
    /* Wait. */
    STOS_TaskWait(&IMVBITask_p, TIMEOUT_INFINITY );
    /* And destroy. */
    task_delete(IMVBITask_p);

#if defined(TTXDUAL_DISK_TEST)
    STOS_TaskWait(&IMVBITask4629_p, TIMEOUT_INFINITY );
    /* And destroy. */
    task_delete(IMVBITask4629_p);
#endif
#if defined(ST_OS20) || defined(ST_OS21)/* linux changes */
    STOS_SemaphoreDelete(NULL,VBIInsertParams.DataSemaphore_p);
#endif
#if defined(TTXDUAL_DISK_TEST)
#if defined(ST_OS20) || defined(ST_OS21)/* linux changes */
    STOS_SemaphoreDelete(NULL,VBIInsertParams4629.DataSemaphore_p);
#endif
#endif

    /* Unforced Teletext Termination */
    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STTTX_Term unforced .......\n" ));
    ReturnError = STTTX_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXDUAL_DISK_TEST)
    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STTTX_Term(4629) unforced .......\n" ));
    ReturnError = STTTX_Term( DeviceNam4629, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* NormalUse Tests completed */
    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
}

/****************************************************************************
Name         : InsertInit

Description  : Gets memory for pes buffer for capture/insert mode
               If inserting, loads file into buffer

Parameters   : PesBuffer_p   pointer to byte-array for PES packets
               BufferUsed_p  pointer to U32 for amount of buffer used

Return Value : 0        success
               -1       failure (no memory)
               -2       failure (file-related)

See Also     : Nothing.
****************************************************************************/
static int InsertInit(U8 **PesBuffer_p, U32 *BufferUsed_p)
{
    static long int FileSize;
    /* Get memory */
    *PesBuffer_p = (U8 *)memory_allocate(
                    (partition_t *)TEST_PARTITION_1, STTTX_PESBUFFER_SIZE );
    if (*PesBuffer_p == NULL)
    {
        STTBX_Print(("Unable to allocate memory for PesBuffer_p!\n"));
        return -1;
    }

    /* If we're inserting, then load the data from the file */
    {

#ifdef ST_OS21
        FILE *file_p = fopen(STTTX_PES_FILE, "rb");
        if (file_p == NULL)
#elif defined(ST_OS20)
        S32 file = (S32)debugopen(STTTX_PES_FILE, "rb");
        if (file == -1)
#endif
        {
            STTBX_Print(("Error trying to open file\n"));
            return -2;
        }
        else
        {
            if (PesBuffer_p != NULL)
            {
#ifdef ST_OS21
                fseek(file_p,0,SEEK_END);
                FileSize = (U32) ftell(file_p);
                rewind(file_p);
#elif defined(ST_OS20)
                FileSize = debugfilesize(file);
#endif
                if (FileSize < STTTX_PESBUFFER_SIZE)
                {
                    STTBX_Print(("Reading from file...\n"));
#ifdef ST_OS21
                    *BufferUsed_p = (U32)fread(*PesBuffer_p, sizeof(U8),FileSize,file_p);
#elif defined(ST_OS20)
                    *BufferUsed_p = (U32)debugread(file, *PesBuffer_p,(int) FileSize);
#endif
                }
                else
                {
                    STTBX_Print(("WARNING! PES file is larger than buffer!\n"));
                    STTBX_Print(("Reading as much as possible, but we are very likely to hit problems later!"));
#ifdef ST_OS21
                    *BufferUsed_p = (U32)fread(*PesBuffer_p, sizeof(U8),STTTX_PESBUFFER_SIZE,file_p);
#elif defined(ST_OS20)
                    *BufferUsed_p = (U32)debugread(file, *PesBuffer_p, STTTX_PESBUFFER_SIZE);
#endif
                }
                STTBX_Print(("Read %i bytes from file\n", *BufferUsed_p));
#if defined(ST_OS21)
                fclose(file_p);
#else
                debugclose(file);
#endif
            }
            else
            {
                STTBX_Print(("Unable to read from file; no buffer\n"));
                *BufferUsed_p = 0;
                return -2;
            }
        }
    }

    return 0;
}

#if defined(TTXDUAL_DISK_TEST)
static int InsertInit4629(U8 **PesBuffer_p, U32 *BufferUsed_p)
{
    static long int FileSize;
    /* Get memory */
    *PesBuffer_p = (U8 *)memory_allocate(
                    (partition_t *)TEST_PARTITION_1, STTTX_PESBUFFER_SIZE );
    if (*PesBuffer_p == NULL)
    {
        STTBX_Print(("Unable to allocate memory for PesBuffer_p!\n"));
        return -1;
    }

    {
    /* If we're inserting, then load the data from the file */
#ifdef ST_OS21
        FILE *file_p = fopen(STTTX_PES_FILE, "rb");
        if (file_p == NULL)
#elif defined(ST_OS20)
        S32 file = (S32)debugopen(STTTX_PES_FILE, "rb");
        if (file == -1)
#endif
        {
            STTBX_Print(("Error trying to open file\n"));
            return -2;
        }
        else
        {
            if (PesBuffer_p != NULL)
            {
#ifdef ST_OS21
                fseek(file_p,0,SEEK_END);
                FileSize = (U32) ftell(file_p);
                rewind(file_p);
#elif defined(ST_OS20)
                FileSize = debugfilesize(file);
#endif
                if (FileSize < STTTX_PESBUFFER_SIZE)
                {
                    STTBX_Print(("Reading from file...\n"));
#ifdef ST_OS21
                    *BufferUsed_p = (U32)fread(*PesBuffer_p, sizeof(U8),FileSize,file_p);
#elif defined(ST_OS20)
                    *BufferUsed_p = (U32)debugread(file, *PesBuffer_p, (int)FileSize);
#endif
                }
                else
                {
                    STTBX_Print(("WARNING! PES file is larger than buffer!\n"));
                    STTBX_Print(("Reading as much as possible, but we are very likely to hit problems later!"));
#ifdef ST_OS21
                    *BufferUsed_p = (U32)fread(*PesBuffer_p, sizeof(U8),STTTX_PESBUFFER_SIZE,file_p);
#elif defined(ST_OS20)
                    *BufferUsed_p = (U32)debugread(file, *PesBuffer_p, (int)STTTX_PESBUFFER_SIZE);
#endif
                }
                STTBX_Print(("Read %i bytes from file\n", *BufferUsed_p));
#if defined(ST_OS21)
                fclose(file_p);
#else
                debugclose(file);
#endif
            }
            else
            {
                STTBX_Print(("Unable to read from file; no buffer\n"));
                *BufferUsed_p = 0;
                return -2;
            }
        }
    }

    return 0;
}
#endif



/****************************************************************************
Name         : InsertMain

Description  : Does the work of inserting a packet

Parameters   : ThisElem         STB object
               CurrPes          structure detailing current pes packet (this is
                                 set or read accordingly)
               PesBuffer_p      pointer to byte-array for PES packets
               buffer_position  where in the PesBuffer_p we are at present
               BufferUsed       total bytes in the buffer (not the overall size)
               BytesWritten     the size of the PES packet (read or set
                                 accordingly)
               CaptureAction    called twice during capture-mode; this is which
                                 action we should do at this call

Return Value : void

See Also     : Nothing.
****************************************************************************/
static void InsertMain( void *Data)
{
    static U32 BufferPosition = 0;
    InsertParams_t *Params = (InsertParams_t *)Data;
    U32 Tenths[10], TenthPos, i;

    for (i = 0; i < 10; i++)
        Tenths[i] = (Params->TotalSize / 10) * i;
    TenthPos = 0;

    while (InsertTaskExit == FALSE)
    {
        /* Copy PES size into Params->PesSize, move through buffer */
        memcpy(&Params->PesSize, &Params->PesBuffer_p[BufferPosition],
               sizeof(Params->PesSize));
        BufferPosition += sizeof(Params->PesSize);
        if (BufferPosition > Tenths[TenthPos])
        {
            TenthPos = (TenthPos + 1) % 10;
        }

        /* Point linearbuff at the PesBuffer_p, and then move the marker
         * forward again
         */
        Params->DriverPesBuffer_p = &Params->PesBuffer_p[BufferPosition];
        BufferPosition += Params->PesSize;

        /* Just let the user know if we've had to wrap */
        if ((BufferPosition >= Params->TotalSize) &&
            (Params->PesBuffer_p != NULL))
        {
#ifndef ST_OSLINUX
#ifndef ARCHITECTURE_ST200
            STTBX_Print(("Wrapping to start of buffer (for next packet)\n"));
#endif
#endif
            BufferPosition = 0;
        }

        /* Don't want to flood the thing with data */

        STOS_TaskDelay(ST_GetClocksPerSecond() / 25);

        /* And off we go */
#if defined(ST_OS20) || defined(ST_OS21)/* linux changes */
        STOS_SemaphoreSignal(Params->DataSemaphore_p);
#endif
    }
}
#if defined(TTXDUAL_DISK_TEST)
static void InsertMain4629( void *Data)
{
    static U32 BufferPosition = 0;
    InsertParams_t *Params = (InsertParams_t *)Data;
    U32 Tenths[10], TenthPos, i;

    for (i = 0; i < 10; i++)
        Tenths[i] = (Params->TotalSize / 10) * i;
    TenthPos = 0;

    while (InsertTaskExit == FALSE)
    {
        /* Copy PES size into Params->PesSize, move through buffer */
        memcpy(&Params->PesSize, &Params->PesBuffer_p[BufferPosition],
               sizeof(Params->PesSize));
        BufferPosition += sizeof(Params->PesSize);
        if (BufferPosition > Tenths[TenthPos])
        {
            TenthPos = (TenthPos + 1) % 10;
        }

        /* Point linearbuff at the PesBuffer_p, and then move the marker
         * forward again
         */
        Params->DriverPesBuffer_p = &Params->PesBuffer_p[BufferPosition];
        BufferPosition += Params->PesSize;

        /* Just let the user know if we've had to wrap */
        if ((BufferPosition >= Params->TotalSize) &&
            (Params->PesBuffer_p != NULL))
        {
            STTBX_Print(("Wrapping to start of buffer (for next packet)\n"));
            BufferPosition = 0;
        }

        /* Don't want to flood the thing with data */

        STOS_TaskDelay(ST_GetClocksPerSecond() / 25);

        /* And off we go */
#if defined(ST_OS20) || defined(ST_OS21)/* linux changes */
        STOS_SemaphoreSignal(Params->DataSemaphore_p);
#endif
    }
}
#endif
#endif/* #ifdef NormauseDisk */
/****************************************************************************
Name         : ErrantUse()

Description  : Performs API interface calls, generally mis-sequenced,
               or with illegal parameters.  This is intended to
               exercise the error checking built into the routines.
               Note that errors should be raised during this test,
               but these MUST be the expected ones.  If this is the
               case, an overall pass will be reported.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors occurred
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel
               STCLKRV_ERROR_PCR_UNAVAILABLE   Valid PCR can't be returned
               STCLKRV_ERROR_CALLBACK_OPEN     RemoveEvent() not called

See Also     : STCL_ErrorCode_t
 ****************************************************************************/

void ErrantUse( void )
{
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError;
    U8              Dummy;
    STTTX_SourceParams_t SourceParams;
#if defined(TTXDISK_TEST)
    semaphore_t     *TestSemaphore_p;
    U32             DataBuffer;     /* Fake data buffer for source */
    U8              *DataBuffer_p = (U8 *)&DataBuffer;
    U32             DataBufferSize = 4;
#endif

    STTBX_Print(( "\n" ));
    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing ErrantUse Test Function ..\n" ));
    STTBX_Print(( "==========================================================\n" ));

    /* Note that the pti could not be terminated at time of writing, so
       the ErrantUse test MUST follow the NormalUse test, re-using the
       prior pti init.  When pti becomes STPTI (i.e. API compliant), pti
       Term and Init calls will need to be added across Normal/ErrantUse  */

    /* call Term without prior Init call... */
    TermParams_s.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STTTX_Term() forced w/o Init ..\n" ));
    ReturnError = STTTX_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    /* set up Init parameters correctly initially */
    strcpy( DeviceNam, "1234" );
    InitParams_s.DriverPartition = (ST_Partition_t *)TEST_PARTITION_2;
    InitParams_s.DeviceType = STTTX_DEVICE_TYPE;
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;
    InitParams_s.InterruptNumber = TELETEXT_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 2;
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.VBIDeviceName, VBIDeviceName);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName);
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    strcpy(InitParams_s.VMIXDeviceName, VMIXDeviceName);
#endif
    /* deliberately spike Init calls with a single invalid parameter... */
#if 0
    STTBX_Print(( "Calling STTTX_Init(), with NULL Name ..\n" ));
    ReturnError = STTTX_Init( NULL, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    STTBX_Print(( "Calling STTTX_Init(), with NUL Name ...\n" ));
    ReturnError = STTTX_Init( "", &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    STTBX_Print(( "Calling STTTX_Init(), with long Name ..\n" ));
    ReturnError = STTTX_Init( "TooLongDeviceNam", &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    InitParams_s.NumSTBObjects = STTTX_MAX_STB_OPEN + 1; /* too high */
    STTBX_Print(( "Calling STTTX_Init(), NumSTBObjects too high \n" ));
    ReturnError = STTTX_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    InitParams_s.NumSTBObjects = 1;

    InitParams_s.BaseAddress = 0;                   /* NULL */
#ifndef ST_7020
    STTBX_Print(( "Calling STTTX_Init(), NULL BaseAddress \n" ));
    ReturnError = STTTX_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
#endif
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;

    /* STTTX_Init has not been called with success yet, so the following
       otherwise legal Open call OUGHT to fail                           */

    OpenParams_s.Type = STTTX_VBI;
    STTBX_Print(( "Calling STTTX_Open() VBI w/o valid Init\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );
#endif
    /* make a valid Init call */
    STTBX_Print(( "Calling STTTX_Init(), with no errors \n" ));
    ReturnError = STTTX_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* call Close with no prior good Open call. Using white-box
       knowledge, all Handle checks are performed by a common static
       function; thus only one call requires exercising.  The check
       routine has to be used on every API call between Open and
       Term as it returns the data structure pointer & object offset.
       The Handle should be reported as invalid....                      */
#if 0
    STTBX_Print(( "Calling STTTX_Close() w/o good Open .\n" ));
    ReturnError = STTTX_Close( VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_INVALID_HANDLE );

    /* call Open with parameter errors...  */

    STTBX_Print(( "Calling STTTX_Open() with NULL Name .\n" ));
    ReturnError = STTTX_Open( NULL, &OpenParams_s, &VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    STTBX_Print(( "Calling STTTX_Open(), NULL &Handle ..\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, NULL );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    STTBX_Print(( "Calling STTTX_Open(), different Name \n" ));
    ReturnError = STTTX_Open( "Different", &OpenParams_s, &VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    STTBX_Print(( "Calling STTTX_Open(), NUL DeviceName .\n" ));
    ReturnError = STTTX_Open( "", &OpenParams_s, &VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );
#endif
    /* clean VBI Open */

    STTBX_Print(( "Calling STTTX_Open(), VBI, no errors .\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &VBIHandle );
    STTBX_Print(("OpenIndex returned as %d\n", VBIHandle & OPEN_HANDLE_MSK ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set source */
#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST)
    SourceParams.DataSource = STTTX_SOURCE_PTI_SLOT;
#elif defined(TTXSTPTI_TEST)
    SourceParams.DataSource = STTTX_SOURCE_STPTI_SIGNAL;
#elif defined(TTXSTDEMUX_TEST)
    SourceParams.DataSource = STTTX_SOURCE_STDEMUX_SIGNAL;
#elif defined(TTXDISK_TEST)
    TestSemaphore_p = STOS_SemaphoreCreateFifo(NULL,1);
    SourceParams.DataSource = STTTX_SOURCE_USER_BUFFER;
    SourceParams.Source_u.UserBuf_s.DataReady_p = TestSemaphore_p;
    SourceParams.Source_u.UserBuf_s.PesBuf_p = &DataBuffer_p;
    SourceParams.Source_u.UserBuf_s.BufferSize = &DataBufferSize;
#endif
    STTBX_Print(( "Calling STTTX_SetSource(), VBI .....\n" ));
    ReturnError = STTTX_SetSource( VBIHandle, &SourceParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* try a second Open with VBI Type */
#if 0
    STTBX_Print(( "Calling STTTX_Open(), VBI already Open\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &VBIHandle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_NO_FREE_HANDLES );

    /* call unforced Term with Open Handle */

    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STTTX_Term() with open handle \n" ));
    ReturnError = STTTX_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_OPEN_HANDLE );

    /* Set the source */
#ifdef ST_OS20
    STTBX_Print(( "Calling STTTX_SetSource(), invalid source params\n" ));
    ReturnError = STTTX_SetSource(VBIHandle, NULL);
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
#endif
    /* open STB objects as these are needed for filter testing,
       try for two with only one specified in Initialization! */

    OpenParams_s.Type = STTTX_STB;
    STTBX_Print(( "Calling STTTX_Open(), STB, no errors .\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &STBHandle );
    STTBX_Print(("OpenIndex returned as %d\n", STBHandle & OPEN_HANDLE_MSK ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STTTX_Open(), second STB .....\n" ));
    scanf("%c",&Dummy);
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &STBHandle2 );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_NO_FREE_HANDLES );

    /* Set source */
#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST)
    SourceParams.DataSource = STTTX_SOURCE_PTI_SLOT;
#elif defined(TTXSTPTI_TEST)
    SourceParams.DataSource = STTTX_SOURCE_STPTI_SIGNAL;
#elif defined(TTXSTDEMUX_TEST)
    SourceParams.DataSource = STTTX_SOURCE_STDEMUX_SIGNAL;
#elif defined(TTXDISK_TEST)
    SourceParams.DataSource = STTTX_SOURCE_USER_BUFFER;
    SourceParams.Source_u.UserBuf_s.DataReady_p = TestSemaphore_p;
    SourceParams.Source_u.UserBuf_s.PesBuf_p = &DataBuffer_p;
    SourceParams.Source_u.UserBuf_s.BufferSize = &DataBufferSize;
#endif
    STTBX_Print(( "Calling STTTX_SetSource(), STB .....\n" ));
    ReturnError = STTTX_SetSource( STBHandle, &SourceParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set up Filter Requests, try for three with only two specified */

    FilterPars_s.RequestTag = 0;
    FilterPars_s.RequestType = STTTX_FILTER_MAGAZINE | STTTX_FILTER_PAGE;
    FilterPars_s.MagazineNumber = 0;
    FilterPars_s.PageNumber = 0;
    FilterPars_s.PageSubCode = 0;
    FilterPars_s.OddPage = (STTTX_Page_t *)NULL;
    FilterPars_s.EvenPage = (STTTX_Page_t *)NULL;
    FilterPars_s.NotifyFunction = NULL;
    STTBX_Print(( "Calling STTTX_SetRequest(), first call\n" ));
    ReturnError = STTTX_SetRequest( STBHandle, &FilterPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    FilterPars_s.RequestTag = 1;
    STTBX_Print(( "Calling STTTX_SetRequest(), second ...\n" ));
    ReturnError = STTTX_SetRequest( STBHandle, &FilterPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    FilterPars_s.RequestTag = 2;
    STTBX_Print(( "Calling STTTX_SetRequest(), third call\n" ));
    ReturnError = STTTX_SetRequest( STBHandle, &FilterPars_s );
    ErrorReport( &StoredError, ReturnError, STTTX_ERROR_NO_MORE_REQUESTS );

    /* test that a tag overwrite can be performed with a full queue */
    FilterPars_s.RequestTag = 1;
    FilterPars_s.MagazineNumber = 5;                /* token change */
    STTBX_Print(( "Calling STTTX_SetRequest(), overwrite \n" ));
    ReturnError = STTTX_SetRequest( STBHandle, &FilterPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* cancel one of the requests and check another can be set */

    FilterPars_s.RequestTag = 0;
    STTBX_Print(( "Calling STTTX_ClearRequest(), Tag %d \n",
                                    FilterPars_s.RequestTag  ));
    ReturnError = STTTX_ClearRequest( STBHandle, FilterPars_s.RequestTag );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    FilterPars_s.RequestTag = 3;
    FilterPars_s.MagazineNumber = 7;                /* token further change */
    STTBX_Print(( "Calling STTTX_SetRequest(), to fill ..\n" ));
    ReturnError = STTTX_SetRequest( STBHandle, &FilterPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    /* finally a forced Term call to clean up */

    TermParams_s.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STTTX_Term(), forced ........\n" ));
    scanf("%c",&Dummy);
    printf("Entering in Terminate function");
    ReturnError = STTTX_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXDISK_TEST)
    STOS_SemaphoreDelete(NULL,TestSemaphore_p);
#endif

    STTBX_Print(( "Overall test result is\n" ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
}


/****************************************************************************
Name         : ErrorReport()

Description  : Expands the returned error code to a message string,
               followed by the expected code/message if different.

Parameters   : ST_ErrorCode_t pointer to an error store, the latest
               ST_ErrorCode_t error code returned, followed by the
               ST_ErrorCode_t expected error code.

Return Value : none

See Also     : STLLI_ErrorCode_t
 ****************************************************************************/

void ErrorReport( ST_ErrorCode_t *ErrorStore,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr )
{
    ST_ErrorCode_t  Temp = ErrorGiven;
    U32             i;

    for( i = 0; i < 2; i++ )
    {
        switch( Temp )
        {
        case ST_NO_ERROR:
            STTBX_Print(( "ST_NO_ERROR - Successful Return\n" ));
            break;

        case ST_ERROR_ALREADY_INITIALIZED:
            STTBX_Print(( "ST_ERROR_ALREADY_INITIALIZED - Prior Init call w/o Term\n" ));
            break;

        case ST_ERROR_UNKNOWN_DEVICE:
            STTBX_Print(( "ST_ERROR_UNKNOWN_DEVICE - Init call must precede access\n" ));
            break;

        case ST_ERROR_INVALID_HANDLE:
            STTBX_Print(( "ST_ERROR_INVALID_HANDLE - Handle number not recognized\n" ));
            break;

        case ST_ERROR_OPEN_HANDLE:
            STTBX_Print(( "ST_ERROR_OPEN_HANDLE - unforced Term with handle open\n" ));
            break;

        case ST_ERROR_BAD_PARAMETER:
            STTBX_Print(( "ST_ERROR_BAD_PARAMETER - Parameter(s) out of valid range\n" ));
            break;

        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            STTBX_Print(( "ST_ERROR_FEATURE_NOT_SUPPORTED - Device mismatch\n" ));
            break;

        case ST_ERROR_NO_FREE_HANDLES:
            STTBX_Print(( "ST_ERROR_NO_FREE_HANDLES - object slot not free\n" ));
            break;

        case ST_ERROR_INTERRUPT_INSTALL:
            STTBX_Print(( "ST_ERROR_INTERRUPT_INSTALL - ISR not installed\n" ));
            break;

        case ST_ERROR_INTERRUPT_UNINSTALL:
            STTBX_Print(( "ST_ERROR_INTERRUPT_UNINSTALL - ISR not un-installed\n" ));
            break;

        case ST_ERROR_TIMEOUT:
            STTBX_Print(("ST_ERROR_TIMEOUT\n"));
            break;

        case ST_ERROR_DEVICE_BUSY:
            STTBX_Print(("ST_ERROR_DEVICE_BUSY\n"));
            break;

        case STDENC_ERROR_I2C:
            STTBX_Print(("STDENC_ERROR_I2C\n"));
            break;

        case STDENC_ERROR_INVALID_ENCODING_MODE:
            STTBX_Print(("STDENC_ERROR_INVALID_ENCODING_MODE\n"));
            break;

        case STTTX_ERROR_TASK_CREATION:
            STTBX_Print(( "STTTX_ERROR_TASK_CREATION - threads can't be created\n" ));
            break;

        case STTTX_ERROR_PID_NOT_SET:
            STTBX_Print(( "STTTX_ERROR_PID_NOT_SET - no SetStreamID call\n" ));
            break;

        case STTTX_ERROR_NO_MORE_REQUESTS:
            STTBX_Print(( "STTTX_ERROR_NO_MORE_REQUESTS - no free request slots\n" ));
            break;

        case STTTX_ERROR_CANT_FILTER:
            STTBX_Print(( "STTTX_ERROR_CANT_FILTER - no filter possible for VBI\n" ));
            break;

        case STTTX_ERROR_EVT_REGISTER:
            STTBX_Print(("STTTX_ERROR_EVT_REGISTER - error registering events\n"));
            break;

        case STTTX_ERROR_STVBI_SETUP:
            STTBX_Print(("STTTX_ERROR_STVBI_SETUP - error setting up STVBI\n"));
            break;

#if defined(STTTX_USE_TUNER) && !defined(STTTX_OLD_TUNER)
        case STTUNER_ERROR_HWFAIL:
            STTBX_Print(("STTUNER_ERROR_HWFAIL\n"));
            break;

        case STTUNER_ERROR_EVT_REGISTER:
            STTBX_Print(("STTUNER_ERROR_EVT_REGISTER\n"));
            break;

        case STTUNER_ERROR_UNSPECIFIED:
            STTBX_Print(("STTUNER_ERROR_UNSPECIFIED\n"));
            break;
#endif

#if defined(TTXSTDEMUX_TEST)
        case STDEMUX_ERROR_NOT_INITIALISED:
             STTBX_Print(("STDEMUX_ERROR_NOT_INITIALISED\n"));
             break;
        case STDEMUX_ERROR_BUFFER_NOT_LINKED:
             STTBX_Print(("STDEMUX_ERROR_BUFFER_NOT_LINKED\n"));
             break;
        case STDEMUX_ERROR_BUFFER_READ_FAILED:
             STTBX_Print(("STDEMUX_ERROR_BUFFER_READ_FAILED\n"));
             break;
        case STDEMUX_ERROR_CHANNEL_ALREADY_INITIALISED:
             STTBX_Print(("STDEMUX_ERROR_CHANNEL_ALREADY_INITIALISED\n"));
             break;
        case STDEMUX_ERROR_CORRUPT_DATA_IN_BUFFER:
             STTBX_Print(("STDEMUX_ERROR_CORRUPT_DATA_IN_BUFFER\n"));
             break;
        case STDEMUX_ERROR_DISCARD_ON_CRC_ERROR_ONLY_ON_SECTION_SLOT:
             STTBX_Print(("STDEMUX_ERROR_DISCARD_ON_CRC_ERROR_ONLY_ON_SECTION_SLOT\n"));
             break;
        case STDEMUX_ERROR_INPUT_FDMA_ABORT_FAILED:
             STTBX_Print(("STDEMUX_ERROR_INPUT_FDMA_ABORT_FAILED\n"));
             break;
        case STDEMUX_ERROR_INCOMPATIBLE_TYPES:
             STTBX_Print(("STDEMUX_ERROR_INCOMPATIBLE_TYPES\n"));
             break;
        case STDEMUX_ERROR_INCOMPLETE_PES_IN_BUFFER:
             STTBX_Print(("STDEMUX_ERROR_INCOMPLETE_PES_IN_BUFFER\n"));
             break;
        case STDEMUX_ERROR_INCOMPLETE_SECTION_IN_BUFFER:
             STTBX_Print(("STDEMUX_ERROR_INCOMPLETE_SECTION_IN_BUFFER\n"));
             break;
        case STDEMUX_ERROR_INTERNAL_ALL_MESSED_UP:
             STTBX_Print(("STDEMUX_ERROR_INTERNAL_ALL_MESSED_UP\n"));
             break;
        case STDEMUX_ERROR_INTERNAL_NOT_EXPLICITLY_SET:
             STTBX_Print(("STDEMUX_ERROR_INTERNAL_NOT_EXPLICITLY_SET\n"));
             break;
        case STDEMUX_ERROR_INTERNAL_OBJECT_ALLOCATED:
             STTBX_Print(("STDEMUX_ERROR_INTERNAL_OBJECT_ALLOCATED\n"));
             break;
        case STDEMUX_ERROR_INTERNAL_OBJECT_ASSOCIATED:
             STTBX_Print(("STDEMUX_ERROR_INTERNAL_OBJECT_ASSOCIATED\n"));
             break;
        case STDEMUX_ERROR_INTERNAL_OBJECT_NOT_FOUND:
             STTBX_Print(("STDEMUX_ERROR_INTERNAL_OBJECT_NOT_FOUND\n"));
             break;
        case STDEMUX_ERROR_INVALID_BUFFER_HANDLE:
             STTBX_Print(("STDEMUX_ERROR_INVALID_BUFFER_HANDLE\n"));
             break;
        case STDEMUX_ERROR_INVALID_CD_FIFO_ADDRESS:
             STTBX_Print(("STDEMUX_ERROR_INVALID_CD_FIFO_ADDRESS\n"));
             break;
        case STDEMUX_ERROR_INVALID_DEVICE_HANDLE:
             STTBX_Print(("STDEMUX_ERROR_INVALID_DEVICE_HANDLE\n"));
             break;
        case STDEMUX_ERROR_INVALID_FILTER_REPEAT_MODE:
             STTBX_Print(("STDEMUX_ERROR_INVALID_FILTER_REPEAT_MODE\n"));
             break;
        case STDEMUX_ERROR_INVALID_FILTER_HANDLE:
             STTBX_Print(("STDEMUX_ERROR_INVALID_FILTER_HANDLE\n"));
             break;
        case STDEMUX_ERROR_INVALID_FILTER_OPERATING_MODE:
             STTBX_Print(("STDEMUX_ERROR_INVALID_FILTER_OPERATING_MODE\n"));
             break;
        case STDEMUX_ERROR_INVALID_FILTER_TYPE:
             STTBX_Print(("STDEMUX_ERROR_INVALID_FILTER_TYPE\n"));
             break;
        case STDEMUX_ERROR_INVALID_PID:
             STTBX_Print(("STDEMUX_ERROR_INVALID_PID\n"));
             break;
        case STDEMUX_ERROR_INVALID_SIGNAL_HANDLE:
             STTBX_Print(("STDEMUX_ERROR_INVALID_SIGNAL_HANDLE\n"));
             break;
        case STDEMUX_ERROR_INVALID_SLOT_HANDLE:
             STTBX_Print(("STDEMUX_ERROR_INVALID_SLOT_HANDLE\n"));
             break;
        case STDEMUX_ERROR_INVALID_SLOT_TYPE:
             STTBX_Print(("STDEMUX_ERROR_INVALID_SLOT_TYPE\n"));
             break;
        case STDEMUX_ERROR_INVALID_SUPPORTED_TC_CODE:
             STTBX_Print(("STDEMUX_ERROR_INVALID_SUPPORTED_TC_CODE\n"));
             break;
        case STDEMUX_ERROR_NO_FREE_DMAS:
             STTBX_Print(("STDEMUX_ERROR_NO_FREE_DMAS\n"));
             break;
        case STDEMUX_ERROR_NO_FREE_FILTERS:
             STTBX_Print(("STDEMUX_ERROR_NO_FREE_FILTERS\n"));
             break;
        case STDEMUX_ERROR_NO_FREE_SLOTS:
             STTBX_Print(("STDEMUX_ERROR_NO_FREE_SLOTS\n"));
             break;
        case STDEMUX_ERROR_NO_PACKET:
             STTBX_Print(("STDEMUX_ERROR_NO_PACKET\n"));
             break;
        case STDEMUX_ERROR_NOT_A_MANUAL_BUFFER:
             STTBX_Print(("STDEMUX_ERROR_NOT_A_MANUAL_BUFFER\n"));
             break;
        case STDEMUX_ERROR_NOT_ON_SAME_DEVICE:
             STTBX_Print(("STDEMUX_ERROR_NOT_ON_SAME_DEVICE\n"));
             break;
        case STDEMUX_ERROR_ONLY_ONE_SIGNAL_PER_BUFFER:
             STTBX_Print(("STDEMUX_ERROR_ONLY_ONE_SIGNAL_PER_BUFFER\n"));
             break;
        case STDEMUX_ERROR_PARSER_NOT_SET:
             STTBX_Print(("STDEMUX_ERROR_PARSER_NOT_SET\n"));
             break;
        case STDEMUX_ERROR_PES_START_EVENT_ONLY_ON_PES_SLOT:
             STTBX_Print(("STDEMUX_ERROR_PES_START_EVENT_ONLY_ON_PES_SLOT\n"));
             break;
        case STDEMUX_ERROR_PID_ALREADY_COLLECTED:
             STTBX_Print(("STDEMUX_ERROR_PID_ALREADY_COLLECTED\n"));
             break;
        case STDEMUX_ERROR_SIGNAL_ABORTED:
             STTBX_Print(("STDEMUX_ERROR_SIGNAL_ABORTED\n"));
             break;
        case STDEMUX_ERROR_SIGNAL_EVERY_PACKET_ONLY_ON_PES_SLOT:
             STTBX_Print(("STDEMUX_ERROR_SIGNAL_EVERY_PACKET_ONLY_ON_PES_SLOT\n"));
             break;
        case STDEMUX_ERROR_SIGNAL_NOT_ASSOCIATED:
             STTBX_Print(("STDEMUX_ERROR_SIGNAL_NOT_ASSOCIATED\n"));
             break;
        case STDEMUX_ERROR_SLOT_ALREADY_LINKED:
             STTBX_Print(("STDEMUX_ERROR_SLOT_ALREADY_LINKED\n"));
             break;
        case STDEMUX_ERROR_SLOT_NOT_LINKED:
             STTBX_Print(("STDEMUX_ERROR_SLOT_NOT_LINKED\n"));
             break;
        case STDEMUX_ERROR_SLOT_NOT_RAW_MODE:
             STTBX_Print(("STDEMUX_ERROR_SLOT_NOT_RAW_MODE\n"));
             break;
        case STDEMUX_ERROR_SLOT_NOT_SIGNAL_EVERY_PACKET:
             STTBX_Print(("STDEMUX_ERROR_SLOT_NOT_SIGNAL_EVERY_PACKET\n"));
             break;
#endif

#if defined(TTXSTPTI_TEST)
        case STPTI_ERROR_ALREADY_WAITING_ON_SLOT:
            STTBX_Print(("STPTI_ERROR_ALREADY_WAITING_ON_SLOT\n"));
            break;

        case STPTI_ERROR_ALT_OUT_ALREADY_IN_USE:
            STTBX_Print(("STPTI_ERROR_ALT_OUT_ALREADY_IN_USE\n"));
            break;

        case STPTI_ERROR_ALT_OUT_TYPE_NOT_SUPPORTED:
            STTBX_Print(("STPTI_ERROR_ALT_OUT_TYPE_NOT_SUPPORTED\n"));
            break;

        case STPTI_ERROR_BUFFER_NOT_LINKED:
            STTBX_Print(("STPTI_ERROR_BUFFER_NOT_LINKED\n"));
            break;

        case STPTI_ERROR_COLLECT_FOR_ALT_OUT_ONLY_ON_NULL_SLOT:
            STTBX_Print(("STPTI_ERROR_COLLECT_FOR_ALT_OUT_ONLY_ON_NULL_SLOT\n"));
            break;

        case STPTI_ERROR_CORRUPT_DATA_IN_BUFFER:
            STTBX_Print(("STPTI_ERROR_CORRUPT_DATA_IN_BUFFER\n"));
            break;

        case STPTI_ERROR_DESCRAMBLER_ALREADY_ASSOCIATED:
            STTBX_Print(("STPTI_ERROR_DESCRAMBLER_ALREADY_ASSOCIATED\n"));
            break;

        case STPTI_ERROR_DESCRAMBLER_NOT_ASSOCIATED:
            STTBX_Print(("STPTI_ERROR_DESCRAMBLER_NOT_ASSOCIATED\n"));
            break;

        case STPTI_ERROR_DESCRAMBLER_TYPE_NOT_SUPPORTED:
            STTBX_Print(("STPTI_ERROR_DESCRAMBLER_TYPE_NOT_SUPPORTED\n"));
            break;

        case STPTI_ERROR_DMA_UNAVAILABLE:
            STTBX_Print(("STPTI_ERROR_DMA_UNAVAILABLE\n"));
            break;

        case STPTI_ERROR_EVENT_QUEUE_EMPTY:
            STTBX_Print(("STPTI_ERROR_EVENT_QUEUE_EMPTY\n"));
            break;

        case STPTI_ERROR_EVENT_QUEUE_FULL:
            STTBX_Print(("STPTI_ERROR_EVENT_QUEUE_FULL\n"));
            break;

        case STPTI_ERROR_FLUSH_FILTERS_NOT_SUPPORTED:
            STTBX_Print(("STPTI_ERROR_FLUSH_FILTERS_NOT_SUPPORTED\n"));
            break;

        case STPTI_ERROR_FUNCTION_NOT_SUPPORTED:
            STTBX_Print(("STPTI_ERROR_FUNCTION_NOT_SUPPORTED\n"));
            break;

        case STPTI_ERROR_INCOMPLETE_PES_IN_BUFFER:
            STTBX_Print(("STPTI_ERROR_INCOMPLETE_PES_IN_BUFFER\n"));
            break;

        case STPTI_ERROR_INCOMPLETE_SECTION_IN_BUFFER:
            STTBX_Print(("STPTI_ERROR_INCOMPLETE_SECTION_IN_BUFFER\n"));
            break;

        case STPTI_ERROR_INTERRUPT_QUEUE_EMPTY:
            STTBX_Print(("STPTI_ERROR_INTERRUPT_QUEUE_EMPTY\n"));
            break;

        case STPTI_ERROR_INTERRUPT_QUEUE_FULL:
            STTBX_Print(("STPTI_ERROR_INTERRUPT_QUEUE_FULL\n"));
            break;

        case STPTI_ERROR_INVALID_ALLOW_OUTPUT_TYPE:
            STTBX_Print(("STPTI_ERROR_INVALID_ALLOW_OUTPUT_TYPE\n"));
            break;

        case STPTI_ERROR_INVALID_ALTERNATE_OUTPUT_TYPE:
            STTBX_Print(("STPTI_ERROR_INVALID_ALTERNATE_OUTPUT_TYPE\n"));
            break;

        case STPTI_ERROR_INVALID_BUFFER_HANDLE:
            STTBX_Print(("STPTI_ERROR_INVALID_BUFFER_HANDLE\n"));
            break;

        case STPTI_ERROR_INVALID_CD_FIFO_ADDRESS:
            STTBX_Print(("STPTI_ERROR_INVALID_CD_FIFO_ADDRESS\n"));
            break;

        case STPTI_ERROR_INVALID_DESCRAMBLER_ASSOCIATION:
            STTBX_Print(("STPTI_ERROR_INVALID_DESCRAMBLER_ASSOCIATION\n"));
            break;

        case STPTI_ERROR_INVALID_DESCRAMBLER_HANDLE:
            STTBX_Print(("STPTI_ERROR_INVALID_DESCRAMBLER_HANDLE\n"));
            break;

        case STPTI_ERROR_INVALID_DEVICE:
            STTBX_Print(("STPTI_ERROR_INVALID_DEVICE\n"));
            break;

        case STPTI_ERROR_INVALID_FILTER_DATA:
            STTBX_Print(("STPTI_ERROR_INVALID_FILTER_DATA\n"));
            break;

        case STPTI_ERROR_INVALID_FILTER_HANDLE:
            STTBX_Print(("STPTI_ERROR_INVALID_FILTER_HANDLE\n"));
            break;

        case STPTI_ERROR_INVALID_FILTER_REPEAT_MODE:
            STTBX_Print(("STPTI_ERROR_INVALID_FILTER_REPEAT_MODE\n"));
            break;

        case STPTI_ERROR_INVALID_FILTER_TYPE:
            STTBX_Print(("STPTI_ERROR_INVALID_FILTER_TYPE\n"));
            break;

        case STPTI_ERROR_INVALID_INJECTION_TYPE:
            STTBX_Print(("STPTI_ERROR_INVALID_INJECTION_TYPE\n"));
            break;

        case STPTI_ERROR_INVALID_KEY_USAGE:
            STTBX_Print(("STPTI_ERROR_INVALID_KEY_USAGE\n"));
            break;

        case STPTI_ERROR_INVALID_LOADER_OPTIONS:
            STTBX_Print(("STPTI_ERROR_INVALID_LOADER_OPTIONS\n"));
            break;

        case STPTI_ERROR_INVALID_PARITY:
            STTBX_Print(("STPTI_ERROR_INVALID_PARITY\n"));
            break;

        case STPTI_ERROR_INVALID_PES_START_CODE:
            STTBX_Print(("STPTI_ERROR_INVALID_PES_START_CODE\n"));
            break;

        case STPTI_ERROR_INVALID_PID:
            STTBX_Print(("STPTI_ERROR_INVALID_PID\n"));
            break;

        case STPTI_ERROR_INVALID_SESSION_HANDLE:
            STTBX_Print(("STPTI_ERROR_INVALID_SESSION_HANDLE\n"));
            break;

        case STPTI_ERROR_INVALID_SIGNAL_HANDLE:
            STTBX_Print(("STPTI_ERROR_INVALID_SIGNAL_HANDLE\n"));
            break;

        case STPTI_ERROR_INVALID_SLOT_HANDLE:
            STTBX_Print(("STPTI_ERROR_INVALID_SLOT_HANDLE\n"));
            break;

        case STPTI_ERROR_INVALID_SLOT_TYPE:
            STTBX_Print(("STPTI_ERROR_INVALID_SLOT_TYPE\n"));
            break;

        case STPTI_ERROR_INVALID_SUPPORTED_TC_CODE:
            STTBX_Print(("STPTI_ERROR_INVALID_SUPPORTED_TC_CODE\n"));
            break;

        case STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION:
            STTBX_Print(("STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION\n"));
            break;

        case STPTI_ERROR_NOT_ON_SAME_DEVICE:
            STTBX_Print(("STPTI_ERROR_NOT_ON_SAME_DEVICE\n"));
            break;

        case STPTI_ERROR_NO_FREE_DESCRAMBLERS:
            STTBX_Print(("STPTI_ERROR_NO_FREE_DESCRAMBLERS\n"));
            break;

        case STPTI_ERROR_NO_FREE_DMAS:
            STTBX_Print(("STPTI_ERROR_NO_FREE_DMAS\n"));
            break;

        case STPTI_ERROR_NO_FREE_FILTERS:
            STTBX_Print(("STPTI_ERROR_NO_FREE_FILTERS\n"));
            break;

        case STPTI_ERROR_NO_FREE_SLOTS:
            STTBX_Print(("STPTI_ERROR_NO_FREE_SLOTS\n"));
            break;

        case STPTI_ERROR_NO_PACKET:
            STTBX_Print(("STPTI_ERROR_NO_PACKET\n"));
            break;

        case STPTI_ERROR_ONLY_ONE_SIGNAL_PER_BUFFER:
            STTBX_Print(("STPTI_ERROR_ONLY_ONE_SIGNAL_PER_BUFFER\n"));
            break;

        case STPTI_ERROR_ONLY_ONE_SIGNAL_PER_SLOT:
            STTBX_Print(("STPTI_ERROR_ONLY_ONE_SIGNAL_PER_SLOT\n"));
            break;

        case STPTI_ERROR_PID_ALREADY_COLLECTED:
            STTBX_Print(("STPTI_ERROR_PID_ALREADY_COLLECTED\n"));
            break;

        case STPTI_ERROR_SET_MATCH_ACTION_NOT_SUPPORTED:
            STTBX_Print(("STPTI_ERROR_SET_MATCH_ACTION_NOT_SUPPORTED\n"));
            break;

        case STPTI_ERROR_SIGNAL_EVERY_PACKET_ONLY_ON_PES_SLOT:
            STTBX_Print(("STPTI_ERROR_SIGNAL_EVERY_PACKET_ONLY_ON_PES_SLOT\n"));
            break;

        case STPTI_ERROR_SLOT_FLAG_NOT_SUPPORTED:
            STTBX_Print(("STPTI_ERROR_SLOT_FLAG_NOT_SUPPORTED\n"));
            break;

        case STPTI_ERROR_SLOT_NOT_ASSOCIATED:
            STTBX_Print(("STPTI_ERROR_SLOT_NOT_ASSOCIATED\n"));
            break;

        case STPTI_ERROR_SLOT_NOT_RAW_MODE:
            STTBX_Print(("STPTI_ERROR_SLOT_NOT_RAW_MODE\n"));
            break;

        case STPTI_ERROR_SLOT_NOT_SIGNAL_EVERY_PACKET:
            STTBX_Print(("STPTI_ERROR_SLOT_NOT_SIGNAL_EVERY_PACKET\n"));
            break;

        case STPTI_ERROR_STORE_LAST_HEADER_ONLY_ON_RAW_SLOT:
            STTBX_Print(("STPTI_ERROR_STORE_LAST_HEADER_ONLY_ON_RAW_SLOT\n"));
            break;

        case STPTI_ERROR_UNABLE_TO_ENABLE_FILTERS:
            STTBX_Print(("STPTI_ERROR_UNABLE_TO_ENABLE_FILTERS\n"));
            break;

        case STPTI_ERROR_USER_BUFFER_NOT_ALIGNED:
            STTBX_Print(("STPTI_ERROR_USER_BUFFER_NOT_ALIGNED\n"));
            break;

        case STPTI_ERROR_WILDCARD_PID_ALREADY_SET:
            STTBX_Print(("STPTI_ERROR_WILDCARD_PID_ALREADY_SET\n"));
            break;

        case STPTI_ERROR_WILDCARD_PID_NOT_SUPPORTED:
            STTBX_Print(("STPTI_ERROR_WILDCARD_PID_NOT_SUPPORTED\n"));
            break;
#endif
        case STVTG_ERROR_INVALID_MODE:
            STTBX_Print(("STVTG_ERROR_INVALID_MODE\n"));
            break;

        case STVTG_ERROR_EVT_REGISTER:
            STTBX_Print(("STVTG_ERROR_EVT_REGISTER\n"));
            break;

        case STVTG_ERROR_EVT_UNREGISTER:
            STTBX_Print(("STVTG_ERROR_EVT_UNREGISTER\n"));
            break;

        case STVTG_ERROR_EVT_SUBSCRIBE:
            STTBX_Print(("STVTG_ERROR_EVT_SUBSCRIBE\n"));
            break;

        case STVTG_ERROR_DENC_OPEN:
            STTBX_Print(("STVTG_ERROR_DENC_OPEN\n"));
            break;

        case STVTG_ERROR_DENC_ACCESS:
            STTBX_Print(("STVTG_ERROR_DENC_ACCESS\n"));
            break;

        case STI2C_ERROR_LINE_STATE:
            STTBX_Print(("STI2C_ERROR_LINE_STATE\n"));
            break;

        case STI2C_ERROR_STATUS:
            STTBX_Print(("STI2C_ERROR_STATUS\n"));
            break;

        case STI2C_ERROR_ADDRESS_ACK:
            STTBX_Print(("STI2C_ERROR_ADDRESS_ACK\n"));
            break;

        case STI2C_ERROR_WRITE_ACK:
            STTBX_Print(("STI2C_ERROR_WRITE_ACK\n"));
            break;

        case STI2C_ERROR_NO_DATA:
            STTBX_Print(("STI2C_ERROR_NO_DATA\n"));
            break;

        case STI2C_ERROR_PIO:
            STTBX_Print(("STI2C_ERROR_PIO\n"));
            break;

        case STI2C_ERROR_BUS_IN_USE:
            STTBX_Print(("STI2C_ERROR_BUS_IN_USE\n"));
            break;

        case STI2C_ERROR_EVT_REGISTER:
            STTBX_Print(("STI2C_ERROR_EVT_REGISTER\\n"));
            break;

        default:
            STTBX_Print(( "*** Unrecognised return code (%i) ***\n", Temp ));
        }

        if( ErrorGiven == ExpectedErr )
        {
            break;
        }
        else
        {
            if( i == 0 )                            /* first pass? */
            {
                Temp = ExpectedErr;
                STTBX_Print(( "==============> Mismatch, expected error was:\n" ));
            }
        }
    }

    STTBX_Print(( "----------------------------------------------------------\n" ));

    /* if ErrorGiven does not match ExpectedErr, update
       ErrorStore only if no previous error was noted    */
    if( ErrorGiven != ExpectedErr )
    {
        if( *ErrorStore == ST_NO_ERROR )
        {
            *ErrorStore = ErrorGiven;
        }
    }
}

#if !defined(mb5518) && !defined(mb275) && !defined(mb376) && !defined(mb390) && !defined(mb391) &&\
    !defined(mb400)  && !defined(mb411) && !defined(mb457) && !defined(mb421) && !defined(mb436) && !defined(DTT5107)
void Init641x(void)
{
    U32                 ActualLen;
    STI2C_Handle_t      Handle;
    STI2C_InitParams_t  I2CInitParams;
    STI2C_OpenParams_t  I2COpenParams;
    STI2C_TermParams_t  I2CTermParams;
    STPIO_InitParams_t  PIOInitParams;
    STPIO_TermParams_t  PIOTermParams;
    ST_ErrorCode_t      error, StoredError;

    /* Here, we make use of the 'auto-increment' mode supported by the
       STV641x family, supplying further bytes that go to sucessively
       higher register addresses (0, 1, 2, 3, ...). All registers default
       to 0 at reset. */

#if defined(mb314) || defined(mb282b)
    U8 ScartName[] = "6410";
    U8 DataBuf[] =
    {
        0,  /* subaddress */
        0x5,
        0x5,
        0x0,
        0x68,
        0x53,
        0x0,
        0x0,
        0x0,
        0x3
    };
#elif defined(mb361) || defined(mb382) || defined (ST_5100)
    U8 ScartName[] = "6413";
    /* STV6412a or STV6413 part, which has 7 registers numbered 0 to 6: */
    U8 DataBuf[] =
    {
        0, /* subaddress - start at the bottom */
      0x00, /* 0: TV Audio Out: stereo, soft vol change, default level/gain */
      0x01, /* 1: Audio Select: from encoder inputs, vcr out muted */
      0x09, /* 2: TV/VCR Video Select: TV Y/CVBS_ENC & R/C_ENC, chroma muted,
                                       VCR out muted */
      0x85, /* 3: RGB & Fast Blanking: FB high, RGB_ENC selected, +6dB gain,
                                       no vid boost, RGB outputs active */
      0x00, /* 4: RF & Misc control: red not chroma for TV, cvbs to rf,
                                    'C_Gate' (?) high, C_VCR out high impedence,
                                     normal slow blanking, no interrupt */
      0x03, /* 5: Slow blanking & inputs control: avg level clamps, no aud
                                     boost, 'input mode' slow blanking */
      0x00,  /* 6: Standby modes: all inputs active and outputs on */
    };
#elif  defined(espresso)
    U8 ScartName[] = "6412a";
    /* STV6412a or STV6413 part, which has 7 registers numbered 0 to 6: */

    U8 DataBuf[] =
    {
        0, /* subaddress - start at the bottom */
      0x11, /* 0: TV Audio Out: stereo, soft vol change, default level/gain */
      0x11, /* 1: Audio Select: from encoder inputs, vcr out muted */
      0x84, /* 2: TV/VCR Video Select: TV Y/CVBS_ENC & R/C_ENC, chroma muted,
                                       VCR out muted */
      0x18, /* 3: RGB & Fast Blanking: FB high, RGB_ENC selected, +6dB gain,
                                       no vid boost, RGB outputs active */
      0x30, /* 4: RF & Misc control: red not chroma for TV, cvbs to rf,
                                    'C_Gate' (?) high, C_VCR out high impedence,
                                     normal slow blanking, no interrupt */
      0x88, /* 5: Slow blanking & inputs control: avg level clamps, no aud
                                     boost, 'input mode' slow blanking */
    };

#endif

    ST_GetClockInfo (&ClockInfo);


    PIOInitParams.BaseAddress = (U32 *)SSC_0_PIO_BASE;
    PIOInitParams.InterruptNumber = SSC_0_PIO_INTERRUPT;
    PIOInitParams.InterruptLevel = SSC_0_PIO_INTERRUPT_LEVEL;
    PIOInitParams.DriverPartition = TEST_PARTITION_1;


    STTBX_Print(("Calling STPIO_Init() ...\n"));
    error = STPIO_Init("SSC_0_PIO", &PIOInitParams);
    ErrorReport(&StoredError, error, ST_NO_ERROR);

    STTBX_Print(("Initialising %s using %s\n",
                ScartName, STI2C_GetRevision()));

    I2CInitParams.BaseAddress = (U32 *)SSC_0_BASE_ADDRESS;
    I2CInitParams.PIOforSDA.BitMask = PIO_FOR_SSC0_SDA;
    I2CInitParams.PIOforSCL.BitMask = PIO_FOR_SSC0_SCL;
    strcpy(I2CInitParams.PIOforSDA.PortName, "SSC_0_PIO");
    strcpy(I2CInitParams.PIOforSCL.PortName, "SSC_0_PIO");
    I2CInitParams.InterruptNumber = SSC_0_INTERRUPT;
    I2CInitParams.InterruptLevel = SSC_0_INTERRUPT_LEVEL;
    I2CInitParams.MasterSlave = STI2C_MASTER;

    I2CInitParams.BaudRate = STI2C_RATE_NORMAL;
    I2CInitParams.ClockFrequency = ClockInfo.CommsBlock;
    I2CInitParams.MaxHandles = 1;
    I2CInitParams.DriverPartition = TEST_PARTITION_1;
#if !defined(ST_5518)
    I2CInitParams.GlitchWidth = 0;
#endif

    STTBX_Print(("Calling STI2C_Init() ................\n"));
    error = STI2C_Init("backbus", &I2CInitParams);
    ErrorReport(&StoredError, error, ST_NO_ERROR);

    if (error == ST_NO_ERROR)
    {
        /* Initialize the I2C interface */
#if defined(mb282b)
        I2COpenParams.I2cAddress       = 0x94; /* ADDress select pin tied high */
#else
        I2COpenParams.I2cAddress       = 0x96; /* ADDress select pin tied high */
#endif
        I2COpenParams.AddressType      = STI2C_ADDRESS_7_BITS;
        I2COpenParams.BusAccessTimeOut = 100000;

        STTBX_Print(("Calling STI2C_Open() ................\n"));
        error = STI2C_Open("backbus", &I2COpenParams, &Handle);
        ErrorReport( &StoredError, error, ST_NO_ERROR );


        STTBX_Print(("Calling STI2C_Write() ...............\n"));
        error = STI2C_Write( Handle, DataBuf, sizeof(DataBuf),
                           10000, &ActualLen);
        ErrorReport( &StoredError, error, ST_NO_ERROR );
        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("ERROR!! STI2C_Write - wrote %d of %d bytes\n",
                        ActualLen, sizeof(DataBuf)));
        }
        else
        {
            STTBX_Print(("STI2C_Write - wrote %d of %d bytes\n",
                        ActualLen, sizeof(DataBuf)));
        }

        I2CTermParams.ForceTerminate = TRUE;
        STTBX_Print(("Calling STI2C_Term() ...\n"));
        error = STI2C_Term("backbus", &I2CTermParams);
        ErrorReport(&StoredError, error, ST_NO_ERROR);
    }

    PIOTermParams.ForceTerminate = TRUE;
    STTBX_Print(("Calling STPIO_Term() ...\n"));
    error = STPIO_Term("SSC_0_PIO", &PIOTermParams);
    ErrorReport(&StoredError, error, ST_NO_ERROR);
}
#endif

#if !(defined(TTXDUAL_DISK_TEST) ||  defined(TTXDUAL_STPTI_TEST))
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) ||\
    defined(ST_5528) || defined(espresso)
static ST_ErrorCode_t DoCfg(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR, StoredError;
    STCFG_InitParams_t  InitParams;
    STCFG_OpenParams_t  OpenParams;
    STCFG_Handle_t      Handle;
    STCFG_TermParams_t  TermParams;

#if defined(ST_5514)
    InitParams.DeviceType = STCFG_DEVICE_5514;
#elif defined(ST_5516)
    InitParams.DeviceType = STCFG_DEVICE_5516;
#elif defined(ST_5517)
    InitParams.DeviceType = STCFG_DEVICE_5517;
#elif defined(ST_5528)
    InitParams.DeviceType = STCFG_DEVICE_5528;
#endif
    InitParams.Partition_p = TEST_PARTITION_1;
    InitParams.BaseAddress_p = (U32 *)CFG_BASE_ADDRESS;

    STTBX_Print(("Calling STCFG_Init() ... \n"));
    error = STCFG_Init("ttxcfg", &InitParams);
    ErrorReport(&StoredError, error, ST_NO_ERROR);

    if (error == ST_NO_ERROR)
    {
        STTBX_Print(("Calling STCFG_Open() ... \n"));
        error = STCFG_Open("ttxcfg", &OpenParams, &Handle);
        ErrorReport(&StoredError, error, ST_NO_ERROR);

#if defined (ST_5528)

 /* Question will VOUT by 5528 now by default ??? */
        /* setup OUT put stage registers */

        /**(volatile U32*)(ST5528_VOUT_BASE_ADDRESS+0x00) = 0x0;
        *(volatile U32*)(ST5528_VOUT_BASE_ADDRESS+0x04) = 0x0;
        *(volatile U32*)(ST5528_VOUT_BASE_ADDRESS+0x08) = 0x0;
        *(volatile U32*)(ST5528_DENC_BASE_ADDRESS+0x17c)=   7;*/
        /**(volatile U32*)(0x19011C00) = 0x0;
        *(volatile U32*)(0x19011C04) = 0x0;
        *(volatile U32*)(0x19011C08) = 0x0;*/

        /* Enable denc in YC mode from RGB */
        /**(volatile U32*)(0x1901117c) = 7;*/
#elif defined (ST_5100)
        /* TBD */
        VOUT_Setup();
#else
        if (error == ST_NO_ERROR)
        {
            STTBX_Print(("Calling STCFG_CallCmd() ...\n"));
            error = STCFG_CallCmd(Handle, STCFG_CMD_VIDEO_ENABLE,
                                  STCFG_VIDEO_DAC_ENABLE);
            ErrorReport(&StoredError, error, ST_NO_ERROR);
        }

        {
            /* Select TXTDATAOUT for PIO4,1 */
            TTX_REG_U32 *Reg = (TTX_REG_U32 *)((U32)CFG_BASE_ADDRESS + 0x08);
            *Reg |= 0x0008;
        }
#endif

        TermParams.ForceTerminate = TRUE;
        STTBX_Print(("Calling STCFG_Term() ... \n"));
        error = STCFG_Term("ttxcfg", &TermParams);
        ErrorReport(&StoredError, error, ST_NO_ERROR);
    }

    return error;
}
#endif /* END of DoCfg ()*/
#endif /* !(defined(TTXDUAL_DISK_TEST) ||  defined(TTXDUAL_STPTI_TEST))*/

#if defined (ST_5100) || defined (ST_5105)  || defined(ST_5188) || defined(ST_5107) || defined(ST_7100) || defined(ST_7109)
/*-------------------------------------------------------------------------
 * Function : VOUT_Setup
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t VOUT_Setup(void)
{
    ST_ErrorCode_t               ST_ErrorCode;
    STVOUT_InitParams_t          VOUTInitParams;
    STVOUT_OpenParams_t          VOUTOpenParams;
    STVOUT_Handle_t              VOUT_Handle = 0;
#ifndef ST_OSLINUX
    VOUTInitParams.CPUPartition_p  = TEST_PARTITION_1;
#else
    VOUTInitParams.CPUPartition_p  =(int *) 0xFFFFFFFF;	
#endif
    VOUTInitParams.MaxOpen         = 3;
    
#if defined (ST_5100) || defined (ST_5105)  || defined(ST_5188) || defined(ST_5107)
    VOUTInitParams.DeviceType      = STVOUT_DEVICE_TYPE_V13;
#else
    VOUTInitParams.DeviceType      = STVOUT_DEVICE_TYPE_7100; 
#endif

    VOUTInitParams.OutputType      = STVOUT_OUTPUT_ANALOG_CVBS;

    memset((char*)&VOUTOpenParams,0,sizeof(STVOUT_OpenParams_t));

    switch ( VOUTInitParams.DeviceType )
    {
        case STVOUT_DEVICE_TYPE_DENC:
            strcpy( VOUTInitParams.Target.DencName, DENCDeviceName);
            break;

        case STVOUT_DEVICE_TYPE_7100:
        #if defined (ST_7100) || defined (ST_7109)
            VOUTInitParams.Target.DualTriDacCell.BaseAddress_p = (void*)VOUT_ADDRESS;
        #endif
            VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)0;
            VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
            VOUTInitParams.Target.DualTriDacCell.Format =STVOUT_SD_MODE;
            memset(VOUTInitParams.Target.DualTriDacCell.DencName, 0, sizeof(ST_DeviceName_t));
            strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, DENCDeviceName);
            switch (VOUTInitParams.OutputType)
            {
                case STVOUT_OUTPUT_ANALOG_RGB :
                case STVOUT_OUTPUT_ANALOG_YUV :
                    VOUTInitParams.Target.DualTriDacCell.DacSelect = STVOUT_DAC_4 | STVOUT_DAC_5 | STVOUT_DAC_6;
                    break;
                case STVOUT_OUTPUT_ANALOG_YC :
                    VOUTInitParams.Target.DualTriDacCell.DacSelect = STVOUT_DAC_1| STVOUT_DAC_2;
                    break;
                case STVOUT_OUTPUT_ANALOG_CVBS :
                    VOUTInitParams.Target.DualTriDacCell.DacSelect = STVOUT_DAC_3;
                    break;
                default:
                break;    
            }
            break;
        case STVOUT_DEVICE_TYPE_V13:
            VOUTInitParams.Target.DualTriDacCell.BaseAddress_p = (void*)VOUT_BASE_ADDRESS;
            VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)0;
            strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, DENCDeviceName);
            switch (VOUTInitParams.OutputType)
            {
                case STVOUT_OUTPUT_ANALOG_RGB :
                case STVOUT_OUTPUT_ANALOG_YUV :
                    VOUTInitParams.Target.DualTriDacCell.DacSelect = STVOUT_DAC_1 | STVOUT_DAC_2 | STVOUT_DAC_3;
                    break;
                case STVOUT_OUTPUT_ANALOG_YC :
                    VOUTInitParams.Target.DualTriDacCell.DacSelect = STVOUT_DAC_4 | STVOUT_DAC_5;
                    break;
                default:
                case STVOUT_OUTPUT_ANALOG_CVBS :
                    VOUTInitParams.Target.DualTriDacCell.DacSelect = STVOUT_DAC_6;
                    break;
            }
            break;
        default :
            break;
    }
    STTBX_Print(("VOUT_Setup(%s)=", VOUTDeviceName ));
    ST_ErrorCode = STVOUT_Init(VOUTDeviceName, &VOUTInitParams);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("%u\n",ST_ErrorCode));
        return( ST_ErrorCode );
    }
    STTBX_Print(("%s\n", STVOUT_GetRevision() ));

    ST_ErrorCode = STVOUT_Open(VOUTDeviceName, &VOUTOpenParams, &VOUT_Handle);
    STTBX_Print(("VOUT_Open %u\n", ST_ErrorCode));
    if (ST_ErrorCode != ST_NO_ERROR) return(ST_ErrorCode);

    ST_ErrorCode = STVOUT_Enable(VOUT_Handle);
    if (ST_ErrorCode != ST_NO_ERROR) return(ST_ErrorCode);

    return( ST_ErrorCode );
}

#elif defined (espresso) || defined(mb376)

#if defined(TTXDUAL_DISK_TEST) || defined(TTXDUAL_STPTI_TEST)
#define VOUT_MAX_OPEN 3

#define VOUT_DEVICE_TYPE            STVOUT_DEVICE_TYPE_5528
#define VOUT_BASE_ADDRESS           ST5528_VOUT_BASE_ADDRESS
#define VOUT_DENCBASE_ADDRESS       ST5528_DENC_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS    0
#define VOUT_OUTPUT_TYPE            STVOUT_OUTPUT_ANALOG_CVBS

#define AUXVOUT_DEVICE_TYPE         STVOUT_DEVICE_TYPE_4629
#define AUXVOUT_BASE_ADDRESS        VOUT_BASE_ADDRESS
#define AUXVOUT_DENCBASE_ADDRESS    ST4629_DENC_BASE_ADDRESS
#define AUXVOUT_OUTPUT_TYPE         STVOUT_OUTPUT_ANALOG_CVBS

#define DIGVOUT_DEVICE_TYPE         STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT
#define DIGVOUT_BASE_ADDRESS        ST5528_DVO_BASE_ADDRESS
#define DIGVOUT_DEVICE_BASE_ADDRESS NULL
#define DIGVOUT_OUTPUT_TYPE         STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED

ST_DeviceName_t VOUT_DeviceName[3] = {"VOUT","DIGVOUT","AUXVOUT"};
STVOUT_Handle_t VOUT_Handle[3];


ST_ErrorCode_t VOUT_Setup(U32 SERVICE_Display)
{
    ST_ErrorCode_t               ST_ErrorCode;
    STVOUT_InitParams_t          VOUTInitParams;
    STVOUT_OpenParams_t          VOUTOpenParams;
    STVOUT_OutputParams_t        VOUTOutputParams;

    U32                    Count =0;
    STVOUT_DeviceType_t    Vout_Device_Type[3] = {STVOUT_DEVICE_TYPE_5528, STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT, STVOUT_DEVICE_TYPE_4629};
    STVOUT_OutputType_t    Vout_Output_Type[3] = {STVOUT_OUTPUT_ANALOG_CVBS, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, STVOUT_OUTPUT_ANALOG_CVBS};

    STTBX_Print(("%s\n", STVOUT_GetRevision() ));

    for(Count =0 ; Count < 3 ; Count++)
    {

        memset((char*)&VOUTOpenParams,0,sizeof(STVOUT_OpenParams_t));
        memset((char*)&VOUTInitParams,0,sizeof(STVOUT_InitParams_t));
        VOUTInitParams.CPUPartition_p  = TEST_PARTITION_1;
        VOUTInitParams.MaxOpen         = VOUT_MAX_OPEN;
        VOUTInitParams.DeviceType      = Vout_Device_Type[Count];
        VOUTInitParams.OutputType      = Vout_Output_Type[Count];

        switch ( VOUTInitParams.DeviceType )
        {
            case STVOUT_DEVICE_TYPE_DENC:
                strcpy( VOUTInitParams.Target.DencName, DENCDeviceName);
                break;

            case STVOUT_DEVICE_TYPE_5528:
                /* Incorrect settings */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, DENCDeviceName);
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)0;
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)ST5528_VOUT_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_6;
                break;

            case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :

                VOUTInitParams.OutputType = STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED;
                VOUTInitParams.Target.DVOCell.DeviceBaseAddress_p = (void *)0;
                VOUTInitParams.Target.DVOCell.BaseAddress_p = (void *)ST5528_DVO_BASE_ADDRESS;
                break;

            case STVOUT_DEVICE_TYPE_4629 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, DENCDeviceName4629);
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)ST4629_DENC_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)0;
                VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
                break;

            default :
                break;
        }

        STTBX_Print(("VOUT_Setup(%s)=", VOUT_DeviceName[Count] ));

        ST_ErrorCode = STVOUT_Init(VOUT_DeviceName[Count], &VOUTInitParams);
        if (ST_ErrorCode != ST_NO_ERROR)
        {
            STTBX_Print(("%u\n", (ST_ErrorCode) ));
            return( ST_ErrorCode );
        }

        ST_ErrorCode = STVOUT_Open(VOUT_DeviceName[Count], &VOUTOpenParams, &VOUT_Handle[Count]);
        STTBX_Print(("VOUT_Open %u\n",(ST_ErrorCode) ));
        if (ST_ErrorCode != ST_NO_ERROR)
        {
            return(ST_ErrorCode);
        }

        if(Count == 1)
        {
            VOUTOutputParams.Digital.EmbeddedType      = TRUE;
            VOUTOutputParams.Digital.SyncroInChroma    = TRUE;
            VOUTOutputParams.Digital.EmbeddedSystem    = STVOUT_EMBEDDED_SYSTEM_625_50;
            VOUTOutputParams.Digital.ColorSpace        = STVOUT_ITU_R_601;
            ST_ErrorCode = STVOUT_SetOutputParams(VOUT_Handle[1], &VOUTOutputParams);
            if(ST_ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STVOUT_SetOutputParams failed \n"));
                return ST_ErrorCode;
            }
        }
        ST_ErrorCode = STVOUT_Enable(VOUT_Handle[Count]);
        if (ST_ErrorCode != ST_NO_ERROR)
        {
             return(ST_ErrorCode);
        }
    } /* for loop ends here */
    return( ST_ErrorCode );
}


#endif
#endif /*#if defined (ST_5100)*/


#ifdef TTXINTERACTIVE_TEST
BOOL InitCommands (void)
{
    #if !defined(STTTX_USE_TUNER)
        return (TTX_InitCommand());
    #else
        return (TUNER_InitCommand() & TTX_InitCommand());
    #endif
}

void InteractiveDriversInit( void )
{
    U32             Pid;
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError, ErrCode;
    ST_Revision_t   RevisionStr;
    STTTX_SourceParams_t SourceParams;
#if defined(STTTX_USE_TUNER)
    STTTX_Request_t Request;
    STTUNER_EventType_t TunerEvent;
    STTUNER_Scan_t Scan;
#endif

    STPTI_OpenParams_t  STPTI_OpenParams;
    STPTI_Handle_t      STPTI_Handle;

    U32 BufferSize;
    STPTI_Buffer_t      VBIBuffer, STBBuffer;
    STPTI_Signal_t      VBISignal;/*, STBSignal;*/
    STPTI_SlotData_t    SlotData;

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing Interactive Test ....\n" ));
    STTBX_Print(( "==========================================================\n" ));

    /* revision string available before STTTX Initialized */

    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));

    /* Initialize our native Teletext Driver */
    strcpy( DeviceNam, "STTTX" );
    InitParams_s.DriverPartition = (ST_Partition_t *)TEST_PARTITION_2;
    InitParams_s.DeviceType = STTTX_DEVICE_TYPE;
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;
    InitParams_s.InterruptNumber = TELETEXT_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 2;
    InitParams_s.NumRequestsAllowed = 4;
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.VBIDeviceName, VBIDeviceName);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName);
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    strcpy(InitParams_s.VMIXDeviceName, VMIXDeviceName);
#endif
    STTBX_Print(( "Calling STTTX_Init() ................\n" ));

    ReturnError = STTTX_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(STTTX_USE_TUNER)
    Scan.SymbolRate = 27500000;
    Scan.Polarization = POLARITY;
    Scan.FECRates = FEC;
    Scan.Modulation = STTUNER_MOD_QPSK;
    Scan.AGC = STTUNER_AGC_ENABLE;
    Scan.LNBToneState = STTUNER_LNB_TONE_22KHZ;
    Scan.Band = 2;
    Scan.IQMode = STTUNER_IQ_MODE_AUTO;

    /* Scan to required frequency */
    STTBX_Print(( "Calling STTUNER_SetFrequency() ..............\n" ));

    ReturnError = STTUNER_SetFrequency(TUNERHandle,
                        (FREQ_MHZ * 1000), &Scan, 0);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    AwaitTunerEvent(&TunerEvent);

    if (TunerEvent == STTUNER_EV_LOCKED)
        STTBX_Print(( "Tuner locked to frequency.\n\n" ));
    else
        STTBX_Print(( "Tuner failed to lock.\n\n" ));

    {
        STTUNER_Scan_t  ScanParams;

        ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;
        ScanParams.Polarization = STTUNER_PLR_ALL;
        ScanParams.FECRates     = STTUNER_FEC_ALL;
        ScanParams.Modulation   = STTUNER_MOD_QPSK;
        ScanParams.AGC          = STTUNER_AGC_ENABLE;

        /* Scans for Europe */
        ScanParams.Band         = 1;
        ScanParams.LNBToneState = STTUNER_LNB_TONE_OFF;
        ScanParams.SymbolRate   = 22000;
        ErrCode = TUNER_SetScanList( TRUE, &ScanParams );

        ScanParams.Band         = 1;
        ScanParams.LNBToneState = STTUNER_LNB_TONE_OFF;
        ScanParams.SymbolRate   = 27500;
        ErrCode |= TUNER_SetScanList( FALSE, &ScanParams );

        ScanParams.Band         = 2;
        ScanParams.LNBToneState = STTUNER_LNB_TONE_22KHZ;
        ScanParams.SymbolRate   = 22000;
        ErrCode = TUNER_SetScanList( FALSE, &ScanParams );

        ScanParams.Band         = 2;
        ScanParams.LNBToneState = STTUNER_LNB_TONE_22KHZ;
        ScanParams.SymbolRate   = 27500;
        ErrCode |= TUNER_SetScanList( FALSE, &ScanParams );

        /* Scan for USA */
        ScanParams.Band         = 0;
        ScanParams.LNBToneState = STTUNER_LNB_TONE_22KHZ;
        ScanParams.SymbolRate   = 20000;
        ErrCode |= TUNER_SetScanList( FALSE, &ScanParams );
    }
#endif

#if defined(TTXSTPTI_TEST)
    /* Open session with STPTI */
    STTBX_Print(("Calling STPTI_Open()...\n"));
    STPTI_OpenParams.DriverPartition_p = TEST_PARTITION_1;
    STPTI_OpenParams.NonCachedPartition_p = TEST_PARTITION_2;
    ReturnError = STPTI_Open(STPTIDeviceName, &STPTI_OpenParams,
                             &STPTI_Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Get slots */
    SlotData.SlotType = STPTI_SLOT_TYPE_PES;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;

#if !defined(NO_VBI)
    STTBX_Print(("Calling STPTI_SlotAllocate()...\n"));
    ReturnError = STPTI_SlotAllocate(STPTI_Handle, &VBISlot, &SlotData);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STPTI_SlotAllocate()...\n"));
    ReturnError = STPTI_SlotAllocate(STPTI_Handle, &STBSlot, &SlotData);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    ReturnError = STPTI_SlotClearPid(STBSlot);

    /* Get buffers */
    BufferSize = STTTX_BUFFER_SIZE;
#if !defined(NO_VBI)
    STTBX_Print(("Calling STPTI_BufferAllocate()...\n"));
    ReturnError = STPTI_BufferAllocate(STPTI_Handle, BufferSize, 1, &VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STPTI_BufferAllocate()...\n"));
    ReturnError = STPTI_BufferAllocate(STPTI_Handle, BufferSize, 1, &STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Get signals */
#if !defined(NO_VBI)
    STTBX_Print(("Calling STPTI_SignalAllocate()...\n"));
    ReturnError = STPTI_SignalAllocate(STPTI_Handle, &VBISignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STPTI_SignalAllocate()...\n"));
    ReturnError = STPTI_SignalAllocate(STPTI_Handle, &STBSignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Link them */
#if !defined(NO_VBI)
    STTBX_Print(("Calling STPTI_SlotLinkToBuffer()...\n"));
    ReturnError = STPTI_SlotLinkToBuffer(VBISlot, VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STPTI_SlotLinkToBuffer()...\n"));
    ReturnError = STPTI_SlotLinkToBuffer(STBSlot, STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if !defined(NO_VBI)
    STTBX_Print(("Calling STPTI_SignalAssociateBuffer()...\n"));
    ReturnError = STPTI_SignalAssociateBuffer(VBISignal, VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STPTI_SignalAssociateBuffer()...\n"));
    ReturnError = STPTI_SignalAssociateBuffer(STBSignal, STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(ST_5514)
    {
        STTSMUX_InitParams_t  TSMUXInitParams;
        STTSMUX_OpenParams_t  TSMUXOpenParams;
        STTSMUX_Handle_t      TSMUXHandle;
        ST_ErrorCode_t        Error;
        U16                   PacketCount;

        /* Initialise TSMUX device */
        STTBX_Print(("Initialising TSMUX\n\n"));
        TSMUXInitParams.DeviceType = STTSMUX_DEVICE_GENERIC;
        TSMUXInitParams.DriverPartition_p = TEST_PARTITION_1;
        TSMUXInitParams.BaseAddress_p = (U32*)TSMUX_BASE_ADDRESS;

        if (TSMUXInitParams.BaseAddress_p == NULL)
        {
            STTBX_Print(("***ERROR: Invalid device base address OR simulator memory allocation error\n\n"));
        }

        TSMUXInitParams.ClockFrequency = ClockInfo.STBus;
        TSMUXInitParams.MaxHandles = 1;
        STTBX_Print(("Initialising TSMUX driver (at addr 0x%x)\n",(U32)TSMUXInitParams.BaseAddress_p));
        STTBX_Print(("Calling STTSMUX_Init\n"));
        Error = STTSMUX_Init(TSMUXName,&TSMUXInitParams);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* Open tsmux */
        STTBX_Print(("Opening TSMUX for use\n\n"));
        STTBX_Print(("Calling STTSMUX_Open\n"));
        Error = STTSMUX_Open(TSMUXName,&TSMUXOpenParams,&TSMUXHandle);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* Setup TSIN for stream */
        STTBX_Print(("Configure TSIN 1 use\n\n"));
        STTBX_Print(("Calling STTSMUX_SetTSMode\n"));
        Error = STTSMUX_SetTSMode(TSMUXHandle, STTSMUX_TSIN_1, STTSMUX_TSMODE_PARALLEL);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("Calling STTSMUX_SetSyncMode\n"));
        Error = STTSMUX_SetSyncMode(TSMUXHandle, STTSMUX_TSIN_1, STTSMUX_SYNCMODE_SYNCHRONOUS);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* TSIN 1 -> PTI A */
        STTBX_Print(("Connecting TSIN 1 to PTI A\n\n"));
        STTBX_Print(("...connecting TSIN 2 to other PTIs to invalidate them\n"));
        STTBX_Print(("Calling STTSMUX_Connect PTI A\n"));
        Error = STTSMUX_Connect(TSMUXHandle,STTSMUX_TSIN_1, STTSMUX_PTI_A);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Calling STTSMUX_Connect PTI B\n"));
        Error = STTSMUX_Connect(TSMUXHandle,STTSMUX_TSIN_2, STTSMUX_PTI_B);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Calling STTSMUX_Connect PTI C\n"));
        Error = STTSMUX_Connect(TSMUXHandle,STTSMUX_TSIN_0, STTSMUX_PTI_C);
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Input packet count : "));
        STOS_TaskDelay(10000);
        Error = STPTI_GetInputPacketCount(STPTIDeviceName, &PacketCount);
        STTBX_Print(("%u \n",PacketCount));
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
#endif
#elif defined(TTXSTDEMUX_TEST)
    /* Open session with STDEMUX */
    STTBX_Print(("Calling STDEMUX_Open()...\n"));
    STDEMUX_OpenParams.DriverPartition_p = TEST_PARTITION_1;
    STDEMUX_OpenParams.NonCachedPartition_p = TEST_PARTITION_2;
    ReturnError = STDEMUX_Open(STDEMUXDeviceName, &STDEMUX_OpenParams,
                             &STDEMUX_Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Get slots */
    SlotData.SlotType = STDEMUX_SLOT_TYPE_PES;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.DiscardOnCrcError = FALSE;

#if defined(ST_8010) || defined(ST_5188)
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;
#endif

#if !defined(NO_VBI)
    STTBX_Print(("Calling STDEMUX_SlotAllocate()...\n"));
    ReturnError = STDEMUX_SlotAllocate(STDEMUX_Handle, &VBISlot, &SlotData);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STDEMUX_SlotAllocate()...\n"));
    ReturnError = STDEMUX_SlotAllocate(STDEMUX_Handle, &STBSlot, &SlotData);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    ReturnError = STDEMUX_SlotClearPid(STBSlot);

    /* Get buffers */
    BufferSize = STTTX_BUFFER_SIZE;
#if !defined(NO_VBI)
    STTBX_Print(("Calling STDEMUX_BufferAllocate()...\n"));
    ReturnError = STDEMUX_BufferAllocate(STDEMUX_Handle, BufferSize, 1, &VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STDEMUX_BufferAllocate()...\n"));
    ReturnError = STDEMUX_BufferAllocate(STDEMUX_Handle, BufferSize, 1, &STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Get signals */
#if !defined(NO_VBI)
    STTBX_Print(("Calling STDEMUX_SignalAllocate()...\n"));
    ReturnError = STDEMUX_SignalAllocate(STDEMUX_Handle, &VBISignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STDEMUX_SignalAllocate()...\n"));
    ReturnError = STDEMUX_SignalAllocate(STDEMUX_Handle, &STBSignal);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Link them */
#if !defined(NO_VBI)
    STTBX_Print(("Calling STDEMUX_SlotLinkToBuffer()...\n"));
    ReturnError = STDEMUX_SlotLinkToBuffer(VBISlot, VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STDEMUX_SlotLinkToBuffer()...\n"));
    ReturnError = STDEMUX_SlotLinkToBuffer(STBSlot, STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if !defined(NO_VBI)
    STTBX_Print(("Calling STDEMUX_SignalAssociateBuffer()...\n"));
    ReturnError = STDEMUX_SignalAssociateBuffer(VBISignal, VBIBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    STTBX_Print(("Calling STDEMUX_SignalAssociateBuffer()...\n"));
    ReturnError = STDEMUX_SignalAssociateBuffer(STBSignal, STBBuffer);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif /* defined(TTXSTPTI_TEST) */
} /* InteractiveDriversInit() */

/*-------------------------------------------------------------------------
 * Function : TST_Init
 *            STTST Init function
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TST_Init(void)
{
    STTST_InitParams_t STTST_InitParams;
    BOOL error;

    STTST_InitParams.CPUPartition_p = TEST_PARTITION_1;
    STTST_InitParams.NbMaxOfSymbols = 300;
    /* Set input file name array to all zeros */
    memset(STTST_InitParams.InputFileName, 0, sizeof(char) * max_tok_len);

/*    strcpy(STTST_InitParams.InputFileName, "stpti.com"); */

    if (STTST_Init(&STTST_InitParams) == ST_NO_ERROR)
    {
        printf("STTST_Init()=OK\n");
        error = FALSE;
    }
    else
    {
        printf("STTST_Init()=FAILED\n");
        error = TRUE;
    }
    return error;
}
#endif
#if defined(TTXSTPTI_TEST)
/*-------------------------------------------------------------------------
 * Function : PTI_Init
 *            PTI Init function
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t PTI_Init(void)
{
    ST_ErrorCode_t error, StoredError;
    STPTI_InitParams_t InitParams;

    /* Set up the parameters */
    InitParams.Device                   = LOCAL_STPTI_DEVICE;
    InitParams.TCDeviceAddress_p        = (STPTI_DevicePtr_t )LOCAL_PTI_ADDRESS;
#ifndef ST_OSLINUX
    InitParams.TCLoader_p               = LOCAL_STPTI_LOADER;
#endif
    InitParams.TCCodes                  = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
    InitParams.DescramblerAssociation   = STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    InitParams.Partition_p              = TEST_PARTITION_1;
    InitParams.NCachePartition_p        = TEST_PARTITION_2;

    strcpy(InitParams.EventHandlerName, EVTDeviceName);

    InitParams.EventProcessPriority     = MIN_USER_PRIORITY;
    InitParams.InterruptProcessPriority = MAX_USER_PRIORITY;
    InitParams.InterruptNumber          = LOCAL_PTI_INTERRUPT;
#ifndef ST_OSLINUX
    InitParams.InterruptLevel           = 6;
#else
    InitParams.InterruptLevel           = PTI_INTERRUPT_LEVEL; /* 0 TBR */
#endif
    InitParams.SyncLock = 0;
    InitParams.SyncDrop = 0;
    InitParams.SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_8x8;

#if defined(STPTI_CAROUSEL_SUPPORT)
    InitParams.CarouselProcessPriority  = MIN_USER_PRIORITY;
    InitParams.CarouselEntryType        = STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE;
#endif

#if defined (ST_7100) || defined (ST_7109)
#if !defined(STPTI_NO_INDEX_SUPPORT)
    InitParams.IndexAssociation         = STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    InitParams.IndexProcessPriority     = 2;
#endif
#else
#if defined(STPTI_INDEX_SUPPORT)
    InitParams.IndexAssociation         = STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    InitParams.IndexProcessPriority     = 2;
#endif
#endif

#if defined(DVD_TRANSPORT_STPTI4)
#ifndef ST_5301
    InitParams.StreamID                 = 0x80;
#else
    InitParams.StreamID                 = STPTI_STREAM_ID_TSIN1;
#endif
    InitParams.NumberOfSlots            = 10;
    InitParams.AlternateOutputLatency   = 42;
#endif
    InitParams.PacketArrivalTimeSource  = STPTI_ARRIVAL_TIME_SOURCE_PTI;
    /* Initialise stpti driver */
    STTBX_Print(("STPTI_Init(%s)=", STPTIDeviceName));
    error = STPTI_Init(STPTIDeviceName, &InitParams );
    ErrorReport( &StoredError, error, ST_NO_ERROR );
#if defined(TTXDUAL_STPTI_TEST)
    InitParams.TCDeviceAddress_p = (STPTI_DevicePtr_t )LOCAL2_PTI_ADDRESS;
    InitParams.InterruptNumber = LOCAL2_PTI_INTERRUPT;
    /* Initialise stpti driver */
    STTBX_Print(("STPTI_Init(%s)=", STPTIDeviceName4629));
    error = STPTI_Init(STPTIDeviceName4629, &InitParams );
    ErrorReport( &StoredError, error, ST_NO_ERROR );
#endif

    return (error);

}
/* end PTI_Init */
#endif

#if defined(STTTX_USE_TUNER)
ST_ErrorCode_t DoTunerScan(void)
{
    ST_ErrorCode_t StoredError = ST_NO_ERROR, ReturnError;
    STTUNER_Scan_t Scan;
    STTUNER_EventType_t TunerEvent;

#if defined(STTTX_OLD_TUNER)
    /* Setup scan structure for acquiring transponder */
    Scan.SymbolRate = 27500000;
    /* Scan.Polarization = POLARITY; */
    Scan.Polarization = STTUNER_PLR_VERTICAL;
    Scan.FECRates = FEC;
    Scan.Modulation = STTUNER_MOD_QPSK;
    Scan.AGC = STTUNER_AGC_ENABLE;
    Scan.LNBToneState = STTUNER_LNB_TONE_22KHZ;
    Scan.Band = 2;
    Scan.IQMode = STTUNER_IQ_MODE_AUTO;

    STTBX_Print(("Calling STTUNER_Scan() from %i to %i\n",
                (FREQ_MHZ - 1) * 1000, (FREQ_MHZ + 1) * 1000));

    /* frequency limits for scanning */
    ReturnError = STTUNER_Scan(TUNERHandle, (FREQ_MHZ - 1) * 1000,
                                    (FREQ_MHZ + 1) * 1000,
                                    1000, 30000);
#else
    Scan.IQMode             = STTUNER_IQ_MODE_AUTO;
    Scan.Band               = BAND_EU_HI;

    Scan.LNBToneState       = STTUNER_LNB_TONE_22KHZ;
    Scan.Polarization       = POLARITY;
    Scan.FECRates           = FEC;
    Scan.SymbolRate         = 27500000;
    Scan.Modulation         = STTUNER_MOD_QPSK;
    Scan.AGC                = STTUNER_AGC_ENABLE;

    STTBX_Print(("STTUNER_SetFrequency(%s,%d)=",
                TUNERDeviceName, FREQ_MHZ * 1000));
    ReturnError = STTUNER_SetFrequency(TUNERHandle, FREQ_MHZ * 1000, &Scan, 0);
#endif

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    AwaitTunerEvent(&TunerEvent);

    if (TunerEvent == STTUNER_EV_LOCKED)
    {
        STTBX_Print(( "Tuner locked to frequency.\n\n" ));
    }
    else
    {
        STTBX_Print(( "Tuner failed to lock.\n\n" ));
        STTBX_Print(("Tuner event: "));
        switch (TunerEvent)
        {
            case STTUNER_EV_NO_OPERATION:
                STTBX_Print(("STTUNER_EV_NO_OPERATION"));
                break;
            case STTUNER_EV_LOCKED:
                STTBX_Print(("STTUNER_EV_LOCKED"));
                break;
            case STTUNER_EV_UNLOCKED:
                STTBX_Print(("STTUNER_EV_UNLOCKED"));
                break;
            case STTUNER_EV_SCAN_FAILED:
                STTBX_Print(("STTUNER_EV_SCAN_FAILED"));
                break;
            case STTUNER_EV_TIMEOUT:
                STTBX_Print(("STTUNER_EV_TIMEOUT"));
                break;
            case STTUNER_EV_SIGNAL_CHANGE:
                STTBX_Print(("STTUNER_EV_SIGNAL_CHANGE"));
                break;
            case STTUNER_EV_SCAN_NEXT:
                STTBX_Print(("STTUNER_EV_SCAN_NEXT"));
                break;
        }
        STTBX_Print(("\n"));
    }

    return StoredError;
}
#endif

#if defined(TTXSTPTI_TEST)
#if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109)
ST_ErrorCode_t ConfigureMerger(ST_ErrorCode_t  *StoredError)
{
    ST_ErrorCode_t error;

    /* STMERGE in  BYPASS MODE  */
    STMERGE_InitParams_t InitParams;

    InitParams.DeviceType = STMERGE_DEVICE_1;
    InitParams.DriverPartition_p = TEST_PARTITION_1;
#ifndef ST_OSLINUX
    InitParams.BaseAddress_p = (U32 *)TSMERGE_BASE_ADDRESS;
#else
    InitParams.BaseAddress_p = NULL;
#endif
    InitParams.MergeMemoryMap_p = STMERGE_DEFAULT_MERGE_MEMORY_MAP;
    InitParams.Mode = STMERGE_BYPASS_TSIN_TO_PTI_ONLY;
#ifdef STMERGE_INTERRUPT_SUPPORT
    InitParams.InterruptNumber = 0;
    InitParams.InterruptLevel  = 0;
#endif
    STTBX_Print(("Calling STMERGE_Init()... \n"));
    error = STMERGE_Init(TSMERGEName, &InitParams);
    ErrorReport(StoredError, error, ST_NO_ERROR);

    error = STMERGE_ConnectGeneric(STMERGE_TSIN_0,STMERGE_PTI_0,STMERGE_BYPASS_GENERIC);
    ErrorReport(StoredError, error, ST_NO_ERROR);

    return error;

}
#elif defined(ST_5301)
ST_ErrorCode_t ConfigureMerger(ST_ErrorCode_t  *StoredError)
{
    ST_ErrorCode_t error;

    /* STMERGE in  BYPASS MODE  */
    STMERGE_InitParams_t     InitParams;
    STMERGE_ObjectConfig_t   SetParams;

    InitParams.DeviceType = STMERGE_DEVICE_1;
    InitParams.DriverPartition_p = TEST_PARTITION_1;
    InitParams.MergeMemoryMap_p = STMERGE_DEFAULT_MERGE_MEMORY_MAP;
    InitParams.Mode = STMERGE_NORMAL_OPERATION;

    STTBX_Print(("Calling STMERGE_Init()... \n"));
    error = STMERGE_Init(TSMERGEName, &InitParams);
    ErrorReport(StoredError, error, ST_NO_ERROR);

    SetParams.Priority  = STMERGE_PRIORITY_HIGHEST;
    SetParams.SyncLock  = 0;
    SetParams.SyncDrop  = 0;
    SetParams.SOPSymbol = 0x47;

    SetParams.u.TSIN.SerialNotParallel  = FALSE; /* FALSE in Parallel Mode */
    SetParams.u.TSIN.InvertByteClk      = FALSE;
    SetParams.u.TSIN.ByteAlignSOPSymbol = FALSE; /* FALSE for Parallel Mode */

    SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
    SetParams.u.TSIN.SyncNotAsync       = TRUE;
    SetParams.u.TSIN.ReplaceSOPSymbol   = FALSE;

    /* Set params and Get Params function checking */
    STTBX_Print(("SetParams..\n"));

    /* Set Params for TSIN1 */
    error = STMERGE_SetParams(STMERGE_TSIN_1,&SetParams);
    ErrorReport(StoredError, error, ST_NO_ERROR);

    /* Connect TSIN1 with PTI0 */
    error = STMERGE_Connect(STMERGE_TSIN_1,STMERGE_PTI_0);
    ErrorReport(StoredError, error, ST_NO_ERROR);

    return error;

}
#endif
#endif


/* Somewhat messy function to print out the system configuration */
void ShowConfig(void)
{
#if defined(DISABLE_ICACHE)
    char *icache = "disabled";
#else
    char *icache = "enabled";
#endif
#if defined(DISABLE_DCACHE)
    char *dcache = "disabled";
#else
    char *dcache = "enabled";
#endif
#if defined(UNIFIED_MEMORY)
    char *memory = "unified";
#else
    char *memory = "standard";
#endif
#if defined(STTTX_USE_DENC)
    char *output = "STDENC";
#else
    char *output = "STVBI";
#endif
#if defined(DVD_TRANSPORT_PTI)
    char *transport = "PTI";
#elif defined(DVD_TRANSPORT_LINK)
    char *transport = "LINK";
#elif defined(DVD_TRANSPORT_STPTI4)
    char *transport = "STPTI4";
#elif defined(DVD_TRANSPORT_STPTI)
    char *transport = "STPTI";
#elif defined(DVD_TRANSPORT_DEMUX)
    char *transport = "STDEMUX";
#else
    char *transport = "unknown (what?)";
#endif

    STTBX_Print(("\nCPU running at %iMHz\n", ST_GetClockSpeed() / 1000000));
    STTBX_Print(("Instruction cache %s", icache));
    STTBX_Print(("; data cache %s\n", dcache));
    STTBX_Print(("Running in %s memory\n", memory));
    STTBX_Print(("Output is via %s; ", output));
    STTBX_Print(("transport is %s\n", transport));
#if defined(DEBUG)
    STTBX_Print(("Debug is enabled\n"));
#endif
#if defined(STTTX_DEBUG)
    STTBX_Print(("Teletext-specific debug is enabled\n"));
#endif
}

#if defined(ST_5528)
/* Temporary, until STMERGE is available */
ST_ErrorCode_t TSMERGER_ClearAll(BOOL *FirstTime)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0008/4)) = 0x00; /*Stream_conf_0*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0000/4)) = 0x00; /*Stream_Sync_0*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0028/4)) = 0x00;  /*Stream_conf_1*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0020/4)) = 0x00; /*Stream_Sync_1*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0048/4)) = 0x00;  /*Stream_conf_2*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0040/4)) = 0x00; /*Stream_Sync_2*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0068/4)) = 0x00;  /*Stream_conf_3*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0060/4)) = 0x00; /*Stream_Sync_3*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0088/4)) = 0x00;  /*Stream_conf_4*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0080/4)) = 0x00; /*Stream_Sync_4*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x00A8/4)) = 0x00;  /*Stream_conf_5*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x00A0/4)) = 0x00; /*Stream_Sync_5*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x00C8/4)) = 0x00;  /*Stream_conf_6*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x00C0/4)) = 0x00; /*Stream_Sync_6*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x00E8/4)) = 0x00;  /*Stream_conf_7*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x00E0/4)) = 0x00; /*Stream_Sync_7*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0200/4)) = 0x00; /*PTI0_destination*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0208/4)) = 0x00; /*PTI1_destination*/
    *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x03f0/4)) = 0x00; /*BYPASS mode*/
    *FirstTime = TRUE;

    return Error;
}


/******************************************************************************
Function Name : TSMergerSetTSIn0BypassMode()
  Description : Links TSIn0 to both PTIs in bypass mode
   Parameters :
        Notes :  Taken from STPTI test * harness.
******************************************************************************/
ST_ErrorCode_t TSMERGER_SetTSIn0BypassMode(void)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    static BOOL FirstTime = TRUE;

    if(FirstTime)
    {
        TSMERGER_ClearAll(&FirstTime);

        /*Reset the cell*/
        *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x03f8/4)) = 0x00006;
        STOS_TaskDelay(10);
        *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x03f8/4)) = 0x00000;

        /*This is the first time, so we'll set up the TSMerger RAM spaces*/
        *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0000/4)) = 0x00000; /*stream_conf_0 (TSin0,tagging off)*/
        *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0020/4)) = 0x00300; /*stream_conf_1 (TSin1,tagging off)*/
        *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0040/4)) = 0x00600; /*stream_conf_2 (TSin2,tagging off)*/
        *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0060/4)) = 0x00900; /*stream_conf_3 (TSin3,tagging off)*/
        *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0080/4)) = 0x00c00; /*stream_conf_4 (TSin4,tagging off)*/
        *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x00A0/4)) = 0x00f00; /*stream_conf_5 (1394,tagging off)*/
        *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x00C0/4)) = 0x01200; /*stream_conf_6 (SWTS0,tagging off)*/
        *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x00E0/4)) = 0x01500; /*stream_conf_7 (SWTS1,tagging off)*/

        FirstTime = FALSE;
    }


   *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x03f0/4)) = 0x02; /*BYPASS mode*/
   *((STSYS_DU32*)ST5528_TSMERGE_BASE_ADDRESS + (0x0000/4)) |= 0xF0087; /* 0xF0603 stream_conf_2(TSin0, tagging off, Synced, serial input)*/

    return Error;

}


#endif

#if defined(TTXSTDEMUX_TEST)
/* Functions -------------------------------------------------------------- */
/****************************************************************************
Name         : FEC_Setup
Description  : Setup FEI for FEC transfer
Parameters   :
Return Value :
****************************************************************************/
void  FEI_Setup(void)
{
#if defined (ST_5188)
    STSYS_WriteRegDev32LE (0x20402000, 0x00200000);  /*for 5188 */

    STSYS_WriteRegDev32LE (FEI_SOFT_RST, 0x00);

        STSYS_WriteRegDev32LE (FEI_CONFIG, 0x5E); /*Bit 6 set */

        STSYS_WriteRegDev32LE (FEI_SECTOR_SIZE, 0x00);

        STSYS_WriteRegDev32LE (FEI_SOFT_RST, 0x01);

#if 0
    {
        /*used for debugging FEI*/
        U32 count =0;
        while(0)
        {
            /* used to check if data available on
               FEI */
            if(!(count)&&(!(count%16)))
            {
                STTBX_Print(("\n"));
            }
            STTBX_Print(("%02x ", *(volatile U32 *)FEI_OUT_DATA));
        }
    }
#endif

#endif
}

/*-------------------------------------------------------------------------
 * Function : STAPP_FdmaSetup
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t FdmaSetup(void)
{
    ST_ErrorCode_t       ST_ErrorCode;
    STFDMA_InitParams_t  STFDMA_InitParams;

    STFDMA_InitParams.DeviceType          = STFDMA_DEVICE_FDMA_2;
    STFDMA_InitParams.DriverPartition_p   = TEST_PARTITION_1;
    STFDMA_InitParams.NCachePartition_p   = TEST_PARTITION_2;
    STFDMA_InitParams.BaseAddress_p       = (void *)FDMA_BASE_ADDRESS;
    STFDMA_InitParams.InterruptNumber     = FDMA_INTERRUPT;
    STFDMA_InitParams.InterruptLevel      = 14;
    STFDMA_InitParams.NumberCallbackTasks = 1;
    STFDMA_InitParams.FDMABlock           = STFDMA_1;
    STFDMA_InitParams.ClockTicksPerSecond = ST_GetClocksPerSecond();

    ST_ErrorCode = STFDMA_Init("FDMA0", &STFDMA_InitParams);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        return( ST_ErrorCode );
    }
    STTBX_Print(("%s\n", STFDMA_GetRevision() ));

    return( ST_ErrorCode );
}

/*-------------------------------------------------------------------------
 * Function : DEMUX_Init
 *            DEMUX Init function
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t DEMUX_Init(void)
{
    ST_ErrorCode_t error, StoredError;
    STDEMUX_InitParams_t InitParams;

    /* Set up the parameters */
    InitParams.Device = STDEMUX_DEVICE_CHANNEL_FEC1;
    InitParams.Parser = STDEMUX_PARSER_TYPE_TS;
    InitParams.Partition_p = TEST_PARTITION_1;
    InitParams.NCachePartition_p = TEST_PARTITION_2;

    strcpy(InitParams.EventHandlerName, EVTDeviceName);

    InitParams.EventProcessPriority = MIN_USER_PRIORITY;
    InitParams.DemuxProcessPriority = 13/*(MAX_USER_PRIORITY)/2*/;
    InitParams.DemuxMessageQueueSize = 256;
    InitParams.InputPacketSize = 192;
    InitParams.OutputModeBlocking = FALSE;
    InitParams.SyncLock = 1;
    InitParams.SyncDrop = 0;
    InitParams.InputMetaData = STDEMUX_INPUT_METADATA_PRE;

    InitParams.NumOfPcktsInPesBuffer = 200;
    InitParams.NumOfPcktsInRawBuffer = 0;
    InitParams.NumOfPcktsInSecBuffer = 0;

    /* Initialise stdemux driver */
    STTBX_Print(("STDEMUX_Init(%s)\n", STDEMUXDeviceName));
    error = STDEMUX_Init(STDEMUXDeviceName, &InitParams );
    ErrorReport( &StoredError, error, ST_NO_ERROR );

    return (error);

}
#endif
/* End of stttxtst.c module */



