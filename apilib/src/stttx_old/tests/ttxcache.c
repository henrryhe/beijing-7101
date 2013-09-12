/****************************************************************************

File Name   : ttxcache.c

Description : Example application that uses STTTX API to cache incoming
              teletext pages and displays there ASCII content.

              This is code is not necessarily fully working or debugged
              but is provided to demonstrate how the STTTX API might
              be used.

Copyright (C) 2006, ST Microelectronics

References  :

****************************************************************************/

#if defined(TTXPTI_TEST)
#undef STTTX_USE_LEGACY_PTI
#define STTTX_USE_LEGACY_PTI
#endif

#if defined(TTXSTPTI_TEST)
#undef STTTX_USE_LEGACY_PTI
#endif

/* Includes --------------------------------------------------------------- */

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#include <stdlib.h>
#include <stdio.h>
#ifndef ST_OS21
#include <ostime.h>
#else
#include "os21debug.h"
#endif
#include <string.h>
#include "stlite.h"
#include "stpio.h"
#endif

#include "stevt.h"
#include "stddefs.h"
#include "stvtg.h"
#include "stdenc.h"
#include "stttx.h"
#include "stdevice.h"
#include "stcommon.h"

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
#endif

#ifdef ST_OS20
#include "debug.h"      /* For debugopen etc. */
#endif

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

#ifndef ST_OSLINUX
#if defined(STTTX_USE_TUNER)
#include "sti2c.h"
#include "sttuner.h"
#else
#if defined(mb282b) || defined(mb314) || defined(mb314_21)
#include "sti2c.h"
#endif
#endif
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
#include "stavmem.h"
#include "stgxobj.h"
#include "stvout.h"
#include "stvmix.h"
#endif

#if defined(TTXSTPTI_TEST)
#if defined(ST_8010) || defined(ST_5188)
#undef TTXSTPTI_TEST
#define TTXSTDEMUX_TEST
#endif
#endif

#if defined TTXSTDEMUX_TEST
#include "stdemux.h"
#include "stfdma.h"
#elif defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || (defined(TTXDISK_TEST) && defined(ST_5518))
#include "pti.h"
#elif defined(TTXSTPTI_TEST)
#include "stpti.h"
#endif

/* 5514 requires tsmux; it also requires STPTI */
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    #include "sttsmux.h"
#endif

#if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_7100) || defined(ST_5301) || defined(ST_7109)
    /* Replaces STTSMUX */
    #include "stmerge.h"
#endif
#if defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
    #include "stclkrv.h"
#endif

#if !defined(STTTX_USE_DENC)
#include "stvbi.h"
#endif

/* Private Constants ------------------------------------------------------ */

#define OPEN_HANDLE_WID         8                   /* Open bits width */
#define OPEN_HANDLE_MSK         ( ( 1 << OPEN_HANDLE_WID ) - 1)
#define PAGE_NO_UNIT_POS        6                   /* LO hex nybble, hammed */
#define PAGE_NO_TENS_POS        7                   /* HO hex nybble, hammed */
#define SUBPAGE_UNIT_POS        8                   /* LO hex nybble, hammed */
#define SUBPAGE_TENS_POS        9
#define SUBPAGE_HUND_POS        10
#define SUBPAGE_THOU_POS        11
#define SUBCODE2_MASK           0x07
#define SUBCODE4_MASK           0x03
#define NYBBLE_SHIFT_CNT        4

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
#define TELETEXT_INTERRUPT 0 /*Interrupt number will not be effective as no interrupt
                               installation in STTTX_Init for 5105 */
#endif

#define CLK_REC_TRACKING_INTERRUPT_LEVEL 7

#if defined (ST_5100) || defined (ST_5105) || defined(ST_5301) || defined(ST_5107)
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
#endif

#ifndef LOCAL_PTI_INTERRUPT
#define LOCAL_PTI_INTERRUPT             ST5528_PTIA_INTERRUPT
#endif

#ifndef LOCAL_STPTI_DEVICE
#define LOCAL_STPTI_DEVICE              STPTI_DEVICE_PTI_4
#endif

#elif defined(ST_5100) || defined(ST_5105) || defined(ST_7710) || defined(ST_7100) || defined(ST_5301) ||\
         defined(ST_7109) || defined(ST_5107)

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


/*** Other misc. defines ***/

#if defined (mb314) || defined(mb314_21) || defined(mb361)
    #define BACK_I2C_BASE    SSC_0_BASE_ADDRESS
    #define FRONT_I2C_BASE   SSC_1_BASE_ADDRESS
    #define I2C_ADDRESS     0x96
#else
    #define I2C_ADDRESS     0x94
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

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) ||\
    defined(ST_5100) || defined(ST_5105) || defined(ST_5301) || defined(ST_5107)

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

#define MAX_BANDS       3
#define BAND_US         0
#define BAND_EU_LO      1
#define BAND_EU_HI      2

/* Private Types ---------------------------------------------------------- */

/* States for cache task state machine */
typedef enum
{
    CACHE_STATE_IDLE,
    CACHE_STATE_NEWPAGE,
    CACHE_STATE_DECODING,
    CACHE_STATE_COMPLETE
} TTX_CacheTaskState_t;

typedef struct
{
    U8  *DriverPesBuffer_p;
    U8  *PesBuffer_p;
    U32 TotalSize;
    U32 PesSize;
    semaphore_t *DataSemaphore_p;
} InsertParams_t;

/* Private Constants ------------------------------------------------------ */

/* Cache buffer sizes */
#define PAGE_BUFFER_SIZE        50
#define NUMBER_CACHEABLE_PAGES  800

/* Array offsets for I2C/PIO devices */
#define BACK_I2C        0
#define FRONT_I2C       1

/* ASCII keys */
#define DELETE_KEY      4

#ifndef STTTX_PES_FILE
#define STTTX_PES_FILE "default.pes"
#endif

#ifndef STTTX_PESBUFFER_SIZE
#define STTTX_PESBUFFER_SIZE 3000000
#endif

/* Private Variables ------------------------------------------------------ */

static ST_ClockInfo_t ClockInfo;


ST_ErrorCode_t          retVal;

ST_DeviceName_t         DeviceNam;
STTTX_InitParams_t      InitParams_s;
STTTX_OpenParams_t      OpenParams_s;
STTTX_Request_t         FilterPars_s;
STTTX_TermParams_t      TermParams_s;
STTTX_Handle_t          STBHandle;

/* Page buffers for odd/even fields, used for temporary storage of
 * raw teletext data.
 */
STTTX_Page_t            Odd[PAGE_BUFFER_SIZE];
STTTX_Page_t            Even[PAGE_BUFFER_SIZE];

/* Circular buffer for odd/even page storage */
STTTX_Page_t *OddPage = Odd;
STTTX_Page_t *EvenPage = Even;
STTTX_Page_t *OddPage_wr = Odd;
STTTX_Page_t *EvenPage_wr = Even;

/* Used for keep track of cache task's status */
TTX_CacheTaskState_t CacheTaskStatus;

#ifndef ST_OSLINUX
task_t  *CacheTask_p;
#else
task_t  CacheTask_p;
#endif

tdesc_t CacheTaskDescriptor;
void    *CacheTaskStack_p;



/* Used for building cache of ASCII teletext pages */
STTTX_Page_t PageCache[NUMBER_CACHEABLE_PAGES];
STTTX_Request_t Request;

#if defined(TTXDISK_TEST)
InsertParams_t  VBIInsertParams;
void            *VBIInsertMainStack_p;
#ifndef ST_OSLINUX
task_t          *IMVBITask_p;
#else
task_t          IMVBITask_p;
#endif
tdesc_t         IMVBItdesc;
#endif

/* 8/4 hamming table used for computing value of hammed bytes */
static U8 InvHamming8_4Tab[256] =
        { 0x01,0xFF,0xFF,0x08,0xFF,0x0C,0x04,0xFF,
          0xFF,0x08,0x08,0x08,0x06,0xFF,0xFF,0x08,
          0xFF,0x0A,0x02,0xFF,0x06,0xFF,0xFF,0x0F,
          0x06,0xFF,0xFF,0x08,0x06,0x06,0x06,0xFF,
          0xFF,0x0A,0x04,0xFF,0x04,0xFF,0x04,0x04,
          0x00,0xFF,0xFF,0x08,0xFF,0x0D,0x04,0xFF,
          0x0A,0x0A,0xFF,0x0A,0xFF,0x0A,0x04,0xFF,
          0xFF,0x0A,0x03,0xFF,0x06,0xFF,0xFF,0x0E,
          0x01,0x01,0x01,0xFF,0x01,0xFF,0xFF,0x0F,
          0x01,0xFF,0xFF,0x08,0xFF,0x0D,0x05,0xFF,
          0x01,0xFF,0xFF,0x0F,0xFF,0x0F,0x0F,0x0F,
          0xFF,0x0B,0x03,0xFF,0x06,0xFF,0xFF,0x0F,
          0x01,0xFF,0xFF,0x09,0xFF,0x0D,0x04,0xFF,
          0xFF,0x0D,0x03,0xFF,0x0D,0x0D,0xFF,0x0D,
          0xFF,0x0A,0x03,0xFF,0x07,0xFF,0xFF,0x0F,
          0x03,0xFF,0x03,0x03,0xFF,0x0D,0x03,0xFF,
          0xFF,0x0C,0x02,0xFF,0x0C,0x0C,0xFF,0x0C,
          0x00,0xFF,0xFF,0x08,0xFF,0x0C,0x05,0xFF,
          0x02,0xFF,0x02,0x02,0xFF,0x0C,0x02,0xFF,
          0xFF,0x0B,0x02,0xFF,0x06,0xFF,0xFF,0x0E,
          0x00,0xFF,0xFF,0x09,0xFF,0x0C,0x04,0xFF,
          0x00,0x00,0x00,0xFF,0x00,0xFF,0xFF,0x0E,
          0xFF,0x0A,0x02,0xFF,0x07,0xFF,0xFF,0x0E,
          0x00,0xFF,0xFF,0x0E,0xFF,0x0E,0x0E,0x0E,
          0x01,0xFF,0xFF,0x09,0xFF,0x0C,0x05,0xFF,
          0xFF,0x0B,0x05,0xFF,0x05,0xFF,0x05,0x05,
          0xFF,0x0B,0x02,0xFF,0x07,0xFF,0xFF,0x0F,
          0x0B,0x0B,0xFF,0x0B,0xFF,0x0B,0x05,0xFF,
          0xFF,0x09,0x09,0x09,0x07,0xFF,0xFF,0x09,
          0x00,0xFF,0xFF,0x09,0xFF,0x0D,0x05,0xFF,
          0x07,0xFF,0xFF,0x09,0x07,0x07,0x07,0xFF,
          0xFF,0x0B,0x03,0xFF,0x07,0xFF,0xFF,0x0E };

long int keybuff;

/* DENC/VTG/TUNER initialization */
STDENC_Handle_t DENCHandle;
STVTG_Handle_t VTGHandle;

#if defined(STTTX_USE_TUNER)
STTUNER_Handle_t TUNERHandle;

/* Tuner device name -- used for all tests */
static ST_DeviceName_t TUNERDeviceName = "STTUNER";
#endif

static ST_DeviceName_t EVTDeviceName = "STEVT";
static ST_DeviceName_t VTGDeviceName = "STVTG";
static ST_DeviceName_t DENCDeviceName = "STDENC";
static ST_DeviceName_t STVBIDeviceName = "STVBI";
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
static ST_DeviceName_t VOUTDeviceName = "VOUT";
static ST_DeviceName_t VMIXDeviceName = "STVMIX";
#endif
#if defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
static ST_DeviceName_t CLKRVDeviceName  = "STCLKRV";
#endif

#if defined(TTXSTDEMUX_TEST)
static ST_DeviceName_t STDEMUXDeviceName = "STDEMUX";
#endif
#if defined(TTXSTPTI_TEST)
static ST_DeviceName_t STPTIDeviceName = "STPTI";
#if defined(ST_5514) || defined(ST_5516) || defined (ST_5517)
static ST_DeviceName_t TSMUXName = "STTSMUX";
#endif
#if defined(ST_5528) || defined (ST_5100) || defined (ST_7710) || defined(ST_7100) ||\
    defined(ST_5301) || defined(ST_7109)
static ST_DeviceName_t TSMERGEName = "STMERGE";
#endif
#endif/* TTXSTPTI_TEST */

#if defined(TTXPTI_TEST)
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
static ST_DeviceName_t TSMUXName = "STTSMUX";
#endif
#endif

#ifndef ST_OSLINUX
/* List of PIO device names */
static ST_DeviceName_t PioDeviceName = "I2C_PIO";

/* PIO Init parameters */
static STPIO_InitParams_t PioInitParams;
#endif

#if defined(STTTX_USE_TUNER)
/* List of I2C device names */
static ST_DeviceName_t I2CDeviceName = "front";

/* I2C Init params */
static STI2C_InitParams_t I2CInitParams;

/* TUNER Init parameters */
static STTUNER_InitParams_t TunerInitParams;
static STTUNER_Band_t BandList[10];
static STTUNER_BandList_t Bands;
static STTUNER_Scan_t ScanList[10];
static STTUNER_ScanList_t Scans;

static STTUNER_EventType_t LastEvent;
#endif

/* Global semaphores used in callbacks */
semaphore_t *EventSemaphore_p;
semaphore_t *FilterSemaphore_p;

#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || (defined(TTXDISK_TEST) && defined(ST_5518))
/* PTI slot for teletext data */
slot_t stb_slot;
#endif

#if defined(TTXSTPTI_TEST)
STPTI_Handle_t  STPTIHandle;
STPTI_Slot_t    STBSlot;
STPTI_Buffer_t  STBBuffer;
STPTI_Signal_t  STBSignal;
#endif

#if defined(TTXSTDEMUX_TEST)
STDEMUX_Handle_t  STDEMUXHandle;
STDEMUX_Slot_t    STBSlot;
STDEMUX_Buffer_t  STBBuffer;
STDEMUX_Signal_t  STBSignal;
STDEMUX_StreamParams_t    STDEMUX_StreamParams;
#endif

#if defined(TTXDISK_TEST)
BOOL InsertTaskExit = FALSE;
#endif
#if defined (ST_7020) || defined (ST_5105) || defined(ST_5188) || defined(ST_5107)
STAVMEM_PartitionHandle_t       AvmemPartitionHandle;
#endif
#if defined(mb314) && !defined(UNIFIED_MEMORY)
#define                 SYSTEM_MEMORY_SIZE          0x800000
#else
#define                 SYSTEM_MEMORY_SIZE          0x400000
#endif

#if defined(mb314) && !defined(UNIFIED_MEMORY)
#define                 NCACHE_MEMORY_SIZE          0x200000
#else
#define                 NCACHE_MEMORY_SIZE          0x800000
#endif

#ifdef ST_OS20
#define STACK_TASK_SIZE     6*1024
#else
#define STACK_TASK_SIZE     32*1024
#endif

#ifdef ST_OS20
#define IMVBI_TASK_SIZE         4096
#else
#define IMVBI_TASK_SIZE         32*1024
#endif
#ifdef ST_OS20
#define IMVBI_TASK_PARTITION    TEST_PARTITION_2
#else
#define IMVBI_TASK_PARTITION    TEST_PARTITION_1
#endif

/*#ifdef ST_OS20
#define STACK_TASK_PARTITION    TEST_PARTITION_2
#else
#define STACK_TASK_PARTITION    TEST_PARTITION_1
#endif*/

#ifdef ST_OS20
/* Allow room for OS20 segments in internal memory */
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1200];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section")


static unsigned char    ncache_memory [NCACHE_MEMORY_SIZE];
static partition_t      the_ncache_partition;
partition_t             *ncache_partition = &the_ncache_partition;
#pragma ST_section      ( ncache_memory , "ncache_section" )

static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
partition_t             *driver_partition = &the_system_partition;
#pragma ST_section      ( external_block, "system_section")

/* This is to avoid a linker warning */
static unsigned char    internal_block_noinit[1];
#pragma ST_section      ( internal_block_noinit, "internal_section_noinit")

static unsigned char    system_block_noinit[1];
#pragma ST_section      ( system_block_noinit, "system_section_noinit")

#if defined(ST_5528) || defined(ST_5100) || defined (ST_7710) || defined(ST_5105) || defined(ST_5188) ||\
         defined(ST_5107)
static U8               data_section[1];
#pragma ST_section      ( data_section,   "data_section")
#endif

#elif defined(ST_OS21) /* ST_OS21 */

static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;
static unsigned char    ncache_block[NCACHE_MEMORY_SIZE];
partition_t             *ncache_partition;

#endif /* ST_OS21 */

/* Memory partitions */
#if defined(ST_OS20) || defined(ST_OS21) /* Linux changes */
#define TEST_PARTITION_1        system_partition
#define TEST_PARTITION_2        ncache_partition
#else   /*Linux*/
#define TEST_PARTITION_1        NULL
#define TEST_PARTITION_2        NULL
#endif

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

void ErrorReport( ST_ErrorCode_t *ErrorStore,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr );
void TeletextCacheTask(void *pDummy);
U32 ReadU32(void);
void GlobalInit(void);
void GlobalTerm(void);
void  FEI_Setup(void);
ST_ErrorCode_t FdmaSetup(void);
U32 TeletextStartSTB(U32 Pid);
void TeletextStop(void);
void AwaitFilterHit(void);
U8 ReverseBits(U8 b);
void DisplayCompletePage(STTTX_Page_t *DisplayPage);
void CacheNotifyFunction( U32 Tag, STTTX_Page_t *p1, STTTX_Page_t *p2 );
BOOL IsEndOfPage(STTTX_Page_t *pOdd, STTTX_Page_t *pEven, BOOL SerialMode,
                 U8 Magazine, U8 Page);
void CachePageData(STTTX_Page_t *pOdd, STTTX_Page_t *pEven, U8 Page);
BOOL GetPageDetails(STTTX_Page_t *pOddPage, STTTX_Page_t *pEvenPage,
                    U8 *CurrentMagazine, U8 *CurrentPage,
                    U32 *CurrentSubpage, BOOL *bSerial);

void TeletextStartCacheFilter(void);
void TeletextGetPage(U32 PageNumber);
void TeletextListPages(void);
void TeletextCacheTask(void *pDummy);
void ErrorReport( ST_ErrorCode_t *ErrorStore,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr );

#if defined(STTTX_USE_TUNER)
void TunerNotifyFunction(STTUNER_Handle_t Handle, STTUNER_EventType_t EventType,ST_ErrorCode_t Error);
void AwaitTunerEvent(STTUNER_EventType_t *EventType_p);
#endif

#if defined(TTXDISK_TEST)
static int InsertInit(U8 **PesBuffer_p, U32 *BufferUsed_p);
static void InsertMain( void *Data);
#endif

#if defined(TTXSTPTI_TEST)
static ST_ErrorCode_t PTI_Init(void);
ST_ErrorCode_t ConfigureMerger(ST_ErrorCode_t  *StoredError);
#endif
#if defined(TTXSTDEMUX_TEST)
static ST_ErrorCode_t DEMUX_Init( U32 DemuxNum );
ST_ErrorCode_t ConfigureMerger(ST_ErrorCode_t  *StoredError);
#endif
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
ST_ErrorCode_t VOUT_Setup(void);
static BOOL AVMEM_Init(void);
static BOOL VMIX_Init(void);
#endif
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
#ifndef ST_OSLINUX
    STTBX_InitParams_t TBXInitParams;
    STBOOT_InitParams_t BootParams;
    STBOOT_TermParams_t BootTerm = { FALSE };
    
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
#endif
    /* Create memory partitions */
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

    internal_block_noinit[0] = 0;
    system_block_noinit[0] = 0;

/* Avoid compiler warnings */
#if defined(ST_5528) || defined (ST_5100) || defined (ST_7710) || defined(ST_5105) || defined(ST_5188) ||\
         defined(ST_5107)
    data_section[0] = 0;
#endif
#endif

#if defined(DISABLE_ICACHE)
    BootParams.ICacheEnabled = FALSE;
#else
    BootParams.ICacheEnabled = TRUE;
#endif
#if defined(DISABLE_DCACHE)
    BootParams.DCacheMap = NULL;
#else
    BootParams.DCacheMap = DCacheMap;
#endif
    BootParams.CacheBaseAddress = (U32 *)CACHE_BASE_ADDRESS;
    BootParams.SDRAMFrequency = SDRAM_FREQUENCY;
    BootParams.BackendType.DeviceType = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.MemorySize = SDRAM_SIZE;
    STBOOT_Init( "boot", &BootParams );

    /* Initialize the toolbox */
    TBXInitParams.SupportedDevices = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.CPUPartition_p = TEST_PARTITION_1;
    STTBX_Init( "tbx", &TBXInitParams );

#ifdef STTTX_NOSTEMBOARD
    /* Will probably change for other boards */
#if defined (mb314)
    /* Poke epld to select Tuner connectors as TS source NOT STEM */
    STTBX_Print(("MB314 EPLD Poke for Tuner connector select\n"));
    *(volatile U32*)(0x7e0c0000) = 1;
#endif
#endif
#endif /* ST_OSLINUX */

/*    task_priority_set (NULL, MIN_USER_PRIORITY);*/

    GlobalInit();
    if (TeletextStartSTB(STB_PID) != 0) {
        STTBX_Print(("Error starting up teletext for STB\n"));
    }
    else
    {
        TeletextStartCacheFilter();

        for (;;)
        {
            U32 p;
            STTBX_Print(("Display Page = "));
            p = ReadU32();
            if (p >= 100 && p <= 799)
            {
                TeletextGetPage(p);
            }
#if defined(TTXSTPTI_TEST)
            else if (p == 800)
            {
                U16 U16Count;
                U32 U32Count;

                /* Print counts */
                STPTI_SlotPacketCount(STBSlot, &U16Count);
                STTBX_Print(("Slot packet count: %i\n", U16Count));
                STPTI_GetInputPacketCount(STPTIDeviceName, &U16Count);
                STTBX_Print(("Input packet count: %i\n", U16Count));
                STPTI_BufferTestForData(STBBuffer, &U32Count);
                STTBX_Print(("Buffer data level: %i\n", U32Count));
            }
#endif
            else
            {
              STTBX_Print(("Invalid page - "));
              TeletextListPages();
            }
            STOS_TaskDelay(ST_GetClocksPerSecond()/10);
        }
    }
#ifndef ST_OSLINUX
    STBOOT_Term( "boot", &BootTerm );
#endif
    return 0;
}


U32 ReadU32( void )
{
    U32 Val = 0;
    BOOL Finished = FALSE;
    /*int Status;*/

    while ( ! Finished )
    {
       /* keybuff = getchar();
        keybuff &= 0x000000FF;
        Status = 1;
        while(Status==0)    STOS_TaskDelay(ST_GetClocksPerSecond()/50);*/
        while ( debugpollkey( &keybuff ) == 0 ) STOS_TaskDelay(ST_GetClocksPerSecond()/50);
        STTBX_Print(("%c", (char) keybuff));
        if ( keybuff >= '0' && keybuff <= '9' )
            Val = (Val * 10) + ( (U32)keybuff - '0');
        else if ( keybuff == '\n' || keybuff == '\r' )
            Finished = TRUE;
        else if ( keybuff == DELETE_KEY )
            Val = Val / 10;
    }

    return Val;
}

void GlobalInit(void)
{
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError;
    STDENC_InitParams_t DENCInitParams;
    STDENC_OpenParams_t DENCOpenParams;
    STDENC_EncodingMode_t DENCEncodingMode;
    STVTG_InitParams_t VTGInitParams;
    STVTG_OpenParams_t VTGOpenParams;
    STEVT_InitParams_t EVTInitParams;
#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || (defined(TTXDISK_TEST) && defined(ST_5518))
    boolean         PtiReturn;
#endif
#if defined(TTXSTPTI_TEST)
    STPTI_OpenParams_t  STPTIOpenParams;
#endif
#if defined(TTXSTDEMUX_TEST)
    STDEMUX_OpenParams_t  STDEMUXOpenParams;
#endif
#if !defined(STTTX_USE_DENC)
    STVBI_InitParams_t VBIInitParams;
#endif
#if defined(STTTX_USE_TUNER)
    STTUNER_OpenParams_t TUNEROpenParams;
#endif

    EventSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);
    FilterSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

    ST_GetClockInfo (&ClockInfo);

    /* Initialize PIO and I2C */
#ifndef ST_OSLINUX
    PioInitParams.BaseAddress = (U32 *)TUNER_SSC_PIO_BASE;
    PioInitParams.InterruptNumber = TUNER_SSC_PIO_INTERRUPT;
    PioInitParams.InterruptLevel = TUNER_SSC_PIO_LEVEL;
    PioInitParams.DriverPartition = TEST_PARTITION_1;

    STTBX_Print(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init(PioDeviceName,
                             &PioInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    /* Initialize EVT */
    EVTInitParams.MemoryPartition = (ST_Partition_t *)TEST_PARTITION_1;
    EVTInitParams.EventMaxNum = 20;
    EVTInitParams.ConnectMaxNum = 20;
    EVTInitParams.SubscrMaxNum = 20;
    EVTInitParams.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() ................\n" ));
    ReturnError = STEVT_Init(EVTDeviceName, &EVTInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(STTTX_USE_TUNER)
    I2CInitParams.BaseAddress = (U32 *)TUNER_SSC_BASE;
    strcpy(I2CInitParams.PIOforSDA.PortName, PioDeviceName);
    I2CInitParams.PIOforSDA.BitMask = PIO_FOR_TUNER_SSC_SDA;
    strcpy(I2CInitParams.PIOforSCL.PortName, PioDeviceName);
    I2CInitParams.PIOforSCL.BitMask = PIO_FOR_TUNER_SSC_SCL;
    I2CInitParams.InterruptNumber = TUNER_SSC_INTERRUPT;
    I2CInitParams.InterruptLevel = TUNER_SSC_INTERRUPT_LEVEL;
    I2CInitParams.MasterSlave = STI2C_MASTER;
    I2CInitParams.BaudRate = STI2C_RATE_NORMAL;
    I2CInitParams.ClockFrequency = ClockInfo.CommsBlock;
    I2CInitParams.MaxHandles = 5;
    I2CInitParams.DriverPartition = TEST_PARTITION_1;

    STTBX_Print(( "Calling STI2C_Init() ................\n" ));
    ReturnError = STI2C_Init(I2CDeviceName,
                             &I2CInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Setup Tuner */
#ifdef STTTX_OLD_TUNER
    STTBX_Print(( "Calling STTUNER_Init() ................\n" ));
    TunerInitParams.Device = STTUNER_DEVICE_SATELLITE;
    TunerInitParams.DemodType = TEST_DEMOD_TYPE;
    TunerInitParams.TunerType = TEST_TUNER_TYPE;
    TunerInitParams.EVTDeviceName[0] = 0;
    strcpy(TunerInitParams.I2CDemodDevice, I2CDeviceName);
    strcpy(TunerInitParams.I2CTunerDevice, I2CDeviceName);
    TunerInitParams.DriverPartition = TEST_PARTITION_1;
    TunerInitParams.I2CDemodAddress = DEMOD_I2C;
    TunerInitParams.I2CTunerAddress = TUNER_I2C;
    TunerInitParams.ExternalClock = 4000000;
    TunerInitParams.BandListMax = 10;
    TunerInitParams.ScanListMax = 10;
    TunerInitParams.SignalListMax = 10;
    TunerInitParams.TSOutputMode = STTUNER_TS_MODE_PARALLEL;
    TunerInitParams.SerialClockSource = STTUNER_SCLK_DEFAULT;
    TunerInitParams.FECMode = STTUNER_FEC_MODE_DVB;
    TunerInitParams.Repeater = FALSE;
#else
    STTBX_Print(( "Calling STTUNER_Init() ................\n" ));
    TunerInitParams.Device = STTUNER_DEVICE_SATELLITE;
    TunerInitParams.DemodType = TEST_DEMOD_TYPE;
    TunerInitParams.TunerType = TEST_TUNER_TYPE;
    strcpy(TunerInitParams.EVT_DeviceName, EVTDeviceName);
    strcpy(TunerInitParams.EVT_RegistrantName, TUNERDeviceName);
    TunerInitParams.Device = STTUNER_DEVICE_SATELLITE;

    TunerInitParams.Max_BandList      = 10;
    TunerInitParams.Max_ThresholdList = 10;
    TunerInitParams.Max_ScanList      = 10;
    TunerInitParams.DriverPartition = TEST_PARTITION_1;

    /* sat drivers */
    TunerInitParams.TunerType         = STTUNER_TUNER_HZ1184;
    TunerInitParams.TunerIO.Route     = STTUNER_IO_REPEATER;
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
    TunerInitParams.TSOutputMode      = STTUNER_TS_MODE_PARALLEL;
    TunerInitParams.SerialClockSource = STTUNER_SCLK_DEFAULT;
    TunerInitParams.SerialDataMode    = STTUNER_SDAT_DEFAULT;
    TunerInitParams.FECMode           = STTUNER_FEC_MODE_DVB;

    /* front I2C bus */
    strcpy(TunerInitParams.DemodIO.DriverName, I2CDeviceName);
    strcpy(TunerInitParams.TunerIO.DriverName, I2CDeviceName);
    strcpy(TunerInitParams.LnbIO.DriverName, I2CDeviceName);
#endif

    ReturnError = STTUNER_Init(TUNERDeviceName, &TunerInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#if defined (ST_7710) || defined(ST_7100) || defined(ST_7109)
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
    /* Initialize the DENC */
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
    /* Think it should be true, anyway. -- Paul */
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
#elif defined (ST_5528) || defined (ST_5100) || defined(ST_5105) || defined (ST_5301) || defined(ST_5188) ||\
         defined(ST_5107)
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VFE2;

    VTGInitParams.Target.VtgCell2.BaseAddress_p       = (void*)VTG_BASE_ADDRESS;
    VTGInitParams.Target.VtgCell2.DeviceBaseAddress_p = (void*)0;
    VTGInitParams.Target.VtgCell2.InterruptNumber     = VTG_VSYNC_INTERRUPT;

#if defined(ST_5100) || defined(ST_5105) || defined(ST_5301) || defined(ST_5188) || defined(ST_5107)
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

#elif defined(ST_7100)
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VTG_CELL_7100;

    VTGInitParams.Target.VtgCell3.BaseAddress_p       = (void*)ST7100_VTG1_BASE_ADDRESS;
    VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)0;
    VTGInitParams.Target.VtgCell3.InterruptNumber     = ST7100_VTG_0_INTERRUPT;
    VTGInitParams.Target.VtgCell3.InterruptLevel      = 4;
    strcpy(VTGInitParams.Target.VtgCell3.ClkrvName, CLKRVDeviceName);

#elif defined(ST_7109)
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VTG_CELL_7100;

    VTGInitParams.Target.VtgCell3.BaseAddress_p       = (void*)ST7109_VTG1_BASE_ADDRESS;
    VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)0;
    VTGInitParams.Target.VtgCell3.InterruptNumber     = ST7109_VTG_0_INTERRUPT;
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

    STTBX_Print(( "Calling STVTG_SetMode() ................\n" ));
    ReturnError = STVTG_SetMode(VTGHandle, STVTG_TIMING_MODE_576I50000_13500);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#ifndef ST_OSLINUX
#if defined(ST_7100)
    *(volatile U32*)(ST7100_VOS_BASE_ADDRESS+0x70) = 0x01;
#elif defined(ST_7109)
    *(volatile U32*)(ST7109_VOS_BASE_ADDRESS+0x70) = 0x01;
#endif
#endif /*ST_OSLINUX*/

#if defined(STTTX_USE_TUNER)
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

#if 0
    Scans.NumElements = 2;
    ScanList[0].SymbolRate   = 27500000;
    ScanList[0].Polarization = STTUNER_PLR_ALL;
    ScanList[0].FECRates     = STTUNER_FEC_ALL;
    ScanList[0].Modulation   = STTUNER_MOD_QPSK;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
    ScanList[0].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[0].Band         = 2;

    ScanList[1].SymbolRate   = 22000000;
    ScanList[1].Polarization = STTUNER_PLR_ALL;
    ScanList[1].FECRates     = STTUNER_FEC_ALL;
    ScanList[1].Modulation   = STTUNER_MOD_QPSK;
    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
    ScanList[1].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[1].Band         = 2;
#else
    ScanList[0].SymbolRate   = 20000000;           /* dummy parameters */
    ScanList[0].Polarization = STTUNER_PLR_ALL;
    ScanList[0].FECRates     = STTUNER_FEC_ALL;
    ScanList[0].Modulation   = STTUNER_MOD_QPSK;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
    ScanList[0].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[0].Band         = BAND_US;

    Scans.NumElements  = 1;
#endif

    STTBX_Print(( "Calling STTUNER_SetScanList() ................\n" ));
    ReturnError = STTUNER_SetScanList(TUNERHandle, &Scans);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    VOUT_Setup();
    AVMEM_Init();
    VMIX_Init();
#endif

#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || (defined(TTXDISK_TEST) && defined(ST_5518))
    STTBX_Print(( "Calling pti_init() ................\n" ));
    PtiReturn = pti_init(
        associate_descramblers_with_slots );
    STTBX_Print(( PtiReturn ? "pti_init() failure\n" :
                        "pti_init() success\n" ));

    /* Allocate slot stb data */
    pti_allocate_dynamic_slot(&stb_slot);
#endif

#if defined(TTXSTPTI_TEST)
    /* Set up STPTI */

    STTBX_Print(("Calling STPTI_Init()...\n"));

    PTI_Init();
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    if (ReturnError == ST_NO_ERROR)
    {
        STPTIOpenParams.DriverPartition_p = TEST_PARTITION_1;
        STPTIOpenParams.NonCachedPartition_p = TEST_PARTITION_2;
        STTBX_Print(("Calling STPTI_Open()...\n"));
        ReturnError = STPTI_Open(STPTIDeviceName, &STPTIOpenParams, &STPTIHandle);
    }
#endif

#if defined(TTXSTDEMUX_TEST)
    /* Set up STDEMUX */
    FEI_Setup();
    FdmaSetup();
    STTBX_Print(("Calling STDEMUX_Init()...\n"));

    ReturnError = DEMUX_Init(1);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    if (ReturnError == ST_NO_ERROR)
    {
        STDEMUXOpenParams.DriverPartition_p = TEST_PARTITION_1;
        STDEMUXOpenParams.NonCachedPartition_p = TEST_PARTITION_2;
        ReturnError = STDEMUX_Open(STDEMUXDeviceName, &STDEMUXOpenParams, &STDEMUXHandle);
    }
#endif

#if !defined(STTTX_USE_DENC)
    /* Initialize the STVBI driver */
    STTBX_Print(( "Calling STVBI_Init() ................\n" ));
    VBIInitParams.DeviceType = STVBI_DEVICE_TYPE_DENC;
    VBIInitParams.MaxOpen = 1;
    strcpy(VBIInitParams.Target.DencName, DENCDeviceName);
    ReturnError = STVBI_Init(STVBIDeviceName, &VBIInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Revision string available before STTTX Initialized */

  /*  STTBX_Print(( "Using STTTX revision: %s\n", STTTX_GetRevision()));*/

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
    strcpy(InitParams_s.VBIDeviceName, STVBIDeviceName);
    strcpy(InitParams_s.EVTDeviceName, EVTDeviceName);
    strcpy(InitParams_s.DENCDeviceName, DENCDeviceName);
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    strcpy(InitParams_s.VMIXDeviceName, VMIXDeviceName);
#endif
    STTBX_Print(( "Calling STTTX_Init() ................\n" ));
    ReturnError = STTTX_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    printf("BEFORE TASK CREATE\n");
#ifndef ST_OSLINUX   
    if(STOS_TaskCreate( TeletextCacheTask,
                                    NULL,
                                    TEST_PARTITION_1,
                                    STACK_TASK_SIZE,
                                    &CacheTaskStack_p,
                                    TEST_PARTITION_1,
                                    &CacheTask_p,
                                    &CacheTaskDescriptor,
                                    MAX_USER_PRIORITY,
                                    "Teletext Cache",
                                    0)!= ST_NO_ERROR)
#else                                    
    if (pthread_create(&CacheTask_p, NULL, (void*)TeletextCacheTask, NULL))
#endif    
	{
        STTBX_Print(("Error trying to spawn cache task\n"));
    }
}

U32 TeletextStartSTB( U32 Pid )
{
#if defined(STTTX_USE_TUNER)
    STTUNER_EventType_t TunerEvent;
    STTUNER_Scan_t Scan;
#endif
    ST_ErrorCode_t  ReturnError = ST_NO_ERROR;
    ST_ErrorCode_t  StoredError;
    STTTX_SourceParams_t SourceParams;

#if defined(STTTX_USE_TUNER)

#if defined(STTTX_OLD_TUNER)
    /* Setup scan structure for acquiring transponder */
    Scan.SymbolRate = 27500000;
    Scan.Polarization = POLARITY;
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

    /* Lock to a frequency ----------------- */
    /* Scan to required frequency */
    STTBX_Print(("STTUNER_SetFrequency(%s,%d)=",
                TUNERDeviceName, FREQ_MHZ * 1000));
    ReturnError = STTUNER_SetFrequency(TUNERHandle,
                                       (FREQ_MHZ * 1000), &Scan, 0);
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
    }
#endif
    OpenParams_s.Type = STTTX_STB;
    STTBX_Print(( "Calling STTTX_Open() STB Type .......\n" ));
    ReturnError = STTTX_Open( DeviceNam, &OpenParams_s, &STBHandle );
    STTBX_Print(("OpenIndex returned as %d\n", STBHandle & OPEN_HANDLE_MSK ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST)
    /* Call SetStreamID prior to Start with a valid PID */
    STTBX_Print(( "Calling STTTX_SetStreamID() PID %d ...\n", Pid ));
    ReturnError = STTTX_SetStreamID( STBHandle, Pid );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#if defined(TTXSTPTI_TEST)
    /* Set up slot, signal, buffer */
    {
        STPTI_SlotData_t SlotData;

        SlotData.SlotType = STPTI_SLOT_TYPE_PES;
        SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
        SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
        SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
        SlotData.SlotFlags.StoreLastTSHeader = FALSE;
        SlotData.SlotFlags.InsertSequenceError = FALSE;
        SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
        SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
        SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
#if defined (ST_5528)  || defined (ST_5100) || defined (ST_7710) || defined (ST_5105) ||\
    defined (ST_7100)  || defined (ST_5301) || defined(ST_7109) || defined(ST_5107)
        SlotData.SlotFlags.SoftwareCDFifo = FALSE;
#endif
        STTBX_Print(("Allocating STPTI buffer...\n"));
        ReturnError = STPTI_BufferAllocate(STPTIHandle, STTTX_BUFFER_SIZE, 1,
                            &STBBuffer);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STTBX_Print(("Allocating STPTI slot...\n"));
        ReturnError = STPTI_SlotAllocate(STPTIHandle, &STBSlot,
                                           &SlotData);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STTBX_Print(("Allocating STPTI signal...\n"));
        ReturnError = STPTI_SignalAllocate(STPTIHandle, &STBSignal);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STTBX_Print(("Calling STPTI_SlotLinkToBuffer()...\n"));
        ReturnError = STPTI_SlotLinkToBuffer(STBSlot, STBBuffer);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STTBX_Print(("Calling STPTI_SignalAssociateBuffer()...\n"));
        ReturnError = STPTI_SignalAssociateBuffer(STBSignal, STBBuffer);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STTBX_Print(("Calling STPTI_SlotSetPid %i\n", Pid));
        ReturnError = STPTI_SlotSetPid(STBSlot, Pid);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }
#endif

#if defined(TTXSTDEMUX_TEST)
    /* Set up slot, signal, buffer */
    {
        STDEMUX_SlotData_t SlotData;

        SlotData.SlotType = STDEMUX_SLOT_TYPE_PES;
        SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
        SlotData.SlotFlags.DiscardOnCrcError = FALSE;
    #if defined(ST_8010) || defined(ST_5188)
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;
    #endif

        STTBX_Print(("Allocating STDEMUX buffer...\n"));
        ReturnError = STDEMUX_BufferAllocate(STDEMUXHandle, STTTX_BUFFER_SIZE, 1,
                            &STBBuffer);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STTBX_Print(("Allocating STDEMUX slot...\n"));
        ReturnError = STDEMUX_SlotAllocate(STDEMUXHandle, &STBSlot,
                                           &SlotData);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STTBX_Print(("Allocating STDEMUX signal...\n"));
        ReturnError = STDEMUX_SignalAllocate(STDEMUXHandle, &STBSignal);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STTBX_Print(("Calling STDEMUX_SlotLinkToBuffer()...\n"));
        ReturnError = STDEMUX_SlotLinkToBuffer(STBSlot, STBBuffer);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STTBX_Print(("Calling STDEMUX_SignalAssociateBuffer()...\n"));
        ReturnError = STDEMUX_SignalAssociateBuffer(STBSignal, STBBuffer);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STTBX_Print(("Calling STDEMUX_SlotSetParams %i\n", STDEMUX_StreamParams.member.Pid));
        STDEMUX_StreamParams.member.Pid = STB_PID;
        ReturnError = STDEMUX_SlotSetParams(STBSlot, &STDEMUX_StreamParams);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }
#endif

#if defined(TTXSTPTI_TEST) || defined(TTXPTI_TEST)
#if defined(ST_5514) || defined(ST_5516) || defined (ST_5517)
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

    /* don't know why tsmux always initialized */
#if defined(ST_5528) || defined (ST_5100) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301) || defined(ST_7109)
    {
        ST_ErrorCode_t Error;

        Error = ConfigureMerger(&StoredError);
        ErrorReport(&StoredError, Error, ST_NO_ERROR);
    }
#endif
#endif


#if defined(TTXSTPTI_TEST)
    {
        ST_ErrorCode_t Error;
        U16 PacketCount;

        STTBX_Print(("Sync packet count : "));
        STOS_TaskDelay(ST_GetClocksPerSecond());
        Error = STPTI_GetInputPacketCount(STPTIDeviceName, &PacketCount);
        STTBX_Print(("%u \n",PacketCount));
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
#endif /* ST_5514 */

    STTBX_Print(( "Calling STTTX_SetDataIdentifier() ...\n" ));
    ReturnError = STTTX_SetDataIdentifier( STBHandle, 0x10 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST)
    SourceParams.DataSource = STTTX_SOURCE_PTI_SLOT;
    SourceParams.Source_u.PTISlot_s.Slot = stb_slot;
#elif defined(TTXSTPTI_TEST)
    SourceParams.DataSource = STTTX_SOURCE_STPTI_SIGNAL;
    SourceParams.Source_u.STPTISignal_s.Signal = STBSignal;
#elif defined(TTXSTDEMUX_TEST)
    SourceParams.DataSource = STTTX_SOURCE_STDEMUX_SIGNAL;
    SourceParams.Source_u.STDEMUXSignal_s.Signal = STBSignal;
#else

    VBIInsertParams.DataSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* Do the insert setup */
    if (InsertInit(&VBIInsertParams.PesBuffer_p, &VBIInsertParams.TotalSize) != 0)
    {
        /* error, can't continue. */
        return -1;
    }
    VBIInsertParams.DriverPesBuffer_p = VBIInsertParams.PesBuffer_p;

    SourceParams.DataSource = STTTX_SOURCE_USER_BUFFER;
    SourceParams.Source_u.UserBuf_s.DataReady_p = VBIInsertParams.DataSemaphore_p;
    SourceParams.Source_u.UserBuf_s.PesBuf_p = &VBIInsertParams.DriverPesBuffer_p;
    SourceParams.Source_u.UserBuf_s.BufferSize = &VBIInsertParams.PesSize;
#endif
    STTBX_Print(( "Calling STTTX_SetSource() ...............\n" ));
    ReturnError = STTTX_SetSource(STBHandle, &SourceParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXDISK_TEST)
    /* Spawn off the VBI insert task */
#ifndef ST_OSLINUX    
   if(STOS_TaskCreate( (void(*)(void *))InsertMain,
                                    (void *)&VBIInsertParams,
                                    TEST_PARTITION_1,
                                    IMVBI_TASK_SIZE,
                                    &VBIInsertMainStack_p,
                                    TEST_PARTITION_1,
                                    &IMVBITask_p,
                                    &IMVBItdesc,
                                    MIN_USER_PRIORITY,
                                    "Packet insertion",
                                    0)!= ST_NO_ERROR) 
#else
    if (pthread_create(&IMVBITask_p, NULL, (void*)InsertMain, NULL))
#endif    
    {
        STTBX_Print(("Error trying to spawn VBI InsertMain task\n"));
    }
    
#endif

    STTBX_Print(( "Calling STTTX_Start() ...............\n" ));
    ReturnError = STTTX_Start( STBHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXPTI_TEST)
    {
        U16 count = 0;

        pti_slot_packet_count(stb_slot, &count);
        STTBX_Print(("Slot packet count: %u\n", count));
        STOS_TaskDelay(ST_GetClocksPerSecond());
        pti_slot_packet_count(stb_slot, &count);
        STTBX_Print(("Slot packet count: %u\n", count));
    }
#endif

    return (ReturnError == ST_NO_ERROR)?0:-1;
}

void TeletextStop(void)
{
    U32 i;
    ST_ErrorCode_t  ReturnError = ST_NO_ERROR;
    ST_ErrorCode_t  StoredError;

    /* Call Stop */
    STTBX_Print(( "Calling STTTX_Stop() ................\n" ));
    ReturnError = STTTX_Stop( STBHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STTTX_Close() ................\n" ));
    ReturnError = STTTX_Close( STBHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Reset cached pages */
    for (i = 0; i < 800; i++)
        PageCache[i].ValidLines = 0;
}

void GlobalTerm(void)
{
    ST_ErrorCode_t  ReturnError = ST_NO_ERROR;
    ST_ErrorCode_t  StoredError;

    /* Force a Termination, Closing the open channel */

    TermParams_s.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STTTX_Term(), forced Closure \n" ));
    ReturnError = STTTX_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* STVTG_Term() */
    {
        STVTG_TermParams_t TermParams;
        TermParams.ForceTerminate = TRUE;
        STVTG_Term(VTGDeviceName, (STVTG_TermParams_t *)&TermParams);
    }

    /* STDENC_Term() */
    {
        STDENC_TermParams_t TermParams;
        TermParams.ForceTerminate = TRUE;
        STDENC_Term(DENCDeviceName, (STDENC_TermParams_t *)&TermParams);
    }

    /* STEVT_Term() */
    {
        STEVT_TermParams_t TermParams;
        TermParams.ForceTerminate = TRUE;
        STEVT_Term(EVTDeviceName, (STEVT_TermParams_t *)&TermParams);
    }

#if defined(STTTX_USE_TUNER)
    /* STTUNER_Term() */
    {
        STTUNER_TermParams_t TermParams;
        TermParams.ForceTerminate = TRUE;
        STTUNER_Term(TUNERDeviceName, &TermParams);
    }

    /* STI2C_Term() */
    {
        STI2C_TermParams_t TermParams;
        TermParams.ForceTerminate = FALSE;
        STI2C_Term(I2CDeviceName, &TermParams);
    }
#endif

    /* STPIO_Term() */
#ifndef ST_OSLINUX    
    {
        STPIO_TermParams_t TermParams;
        TermParams.ForceTerminate = FALSE;
        STPIO_Term(PioDeviceName, &TermParams);
    }
#endif

#if defined(TTXSTPTI_TEST)
    {
        STPTI_TermParams_t TermParams;

        TermParams.ForceTermination = FALSE;
        STPTI_Term(STPTIDeviceName, &TermParams);
    }
#endif

    STOS_SemaphoreDelete(NULL,FilterSemaphore_p);
    STOS_SemaphoreDelete(NULL,EventSemaphore_p);
}

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

void AwaitFilterHit(void)
{
    STOS_SemaphoreWait(FilterSemaphore_p);
}

U8 ReverseBits(U8 b)
{
    U8 n = 0, i;

    for (i = 0; i < 8; i++)
        if ( (b & (1 << i)) != 0)
            n |= ( 0x80 >> i );

    return n;
}

void DisplayCompletePage(STTTX_Page_t *DisplayPage)
{
    U32 i,j;

    STTBX_Print(("\nValidLines: %08x\n", DisplayPage->ValidLines));
    for (i = 1; i < 26; i++)
    {
        if ((DisplayPage->ValidLines & (1 << i)) != 0)
        {
            for (j = 0; DisplayPage->Lines[i][j] != '\0'; j++)
                STTBX_Print(("%c", DisplayPage->Lines[i][j]));
        }
        else
            STTBX_Print(("\n"));
    }
}

void CacheNotifyFunction( U32 Tag, STTTX_Page_t *p1, STTTX_Page_t *p2 )
{
    /* Increment buffer counter ready for next page */
    OddPage_wr++;
    EvenPage_wr++;

    if (OddPage_wr >= &Odd[PAGE_BUFFER_SIZE])     /* Buffer wrapped? */
    {
        OddPage_wr = Odd;
        EvenPage_wr = Even;
    }

    /* Check for buffer overflow */
    if (OddPage_wr == OddPage)
    {
        STTBX_Print(("+"));
        STOS_SemaphoreSignal(FilterSemaphore_p);
        return;
    }

    if (CacheTaskStatus != CACHE_STATE_COMPLETE)
    {
        Request.RequestType = STTTX_FILTER_ANY;
        Request.OddPage = OddPage_wr;
        Request.EvenPage = EvenPage_wr;
        STTTX_SetRequest( STBHandle, &Request );

        /* Signal cache task of new packet */
        STOS_SemaphoreSignal(FilterSemaphore_p);
    }
}

BOOL IsEndOfPage(STTTX_Page_t *pOdd, STTTX_Page_t *pEven, BOOL SerialMode, U8 Magazine, U8 Page)
{
    U8 CurMag, CurPage;
    U32 CurSp;
    BOOL CurSerMode;

    if ( GetPageDetails(pOdd, pEven, &CurMag, &CurPage, &CurSp, &CurSerMode) )
    {
        if ( Page != CurPage )
            return TRUE;
    }

    return FALSE;
}

void CachePageData(STTTX_Page_t *pOdd, STTTX_Page_t *pEven, U8 Page)
{
    S32 i, j, CacheIndex;
    STTTX_Page_t *pCache;

    for (i = 0; i < STTTX_MAXLINES; i++)
    {
        if ( ( OddPage->ValidLines & (1 << i) ) != 0 )
        {
            S32 PacketNo, PcktAddr1, PcktAddr2;

            PcktAddr1 = InvHamming8_4Tab[OddPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[OddPage->Lines[i][5]];
            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );

       /*     STTBX_Print(("\n\nOddPage PacketNo: (%d) and i = %d\n", PacketNo,i));*/
            if (PacketNo > 0 && PacketNo < 26)
            {
                CacheIndex = ((PcktAddr1 & 0x07) * 100) +
                             (((Page >> 4) & 0x0F) * 10) +
                             (Page & 0x0F) - 100;

                if (CacheIndex >= 0)
                {
                    /*STTBX_Print(("OddPage CacheIndex:..................... %d\n", CacheIndex));*/
                    pCache = &PageCache[CacheIndex];

                    pCache->ValidLines |= (1 << PacketNo);
                    /*for (j = 0; j < 35; j++)
                    {
                    	STTBX_Print(("%x ",OddPage->Lines[i][j]));
                    }*/
                    for (j = 6; j < 46; j++)
                    {

                        U8 c = ReverseBits(OddPage->Lines[i][j]) & 0x7F;
                        c = (c < 32 || c > 126) ? 32 : c;
                        pCache->Lines[PacketNo][j-6] = c;
                       /* STTBX_Print(("%c", c));*/
                    }
                    pCache->Lines[PacketNo][j-6] = '\n';
                    pCache->Lines[PacketNo][j-5] = 0;
                }
            }
        }

        if ( ( EvenPage->ValidLines & (1 << i) ) != 0 )
        {
            S32 PacketNo, PcktAddr1, PcktAddr2;

            PcktAddr1 = InvHamming8_4Tab[EvenPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[EvenPage->Lines[i][5]];

            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );

            /*STTBX_Print(("\n\nEvenPage PacketNo: (%d) and i = %d\n", PacketNo,i));*/
            if (PacketNo > 0 && PacketNo < 26)
            {
                CacheIndex = ((PcktAddr1 & 0x07) * 100) +
                             (((Page >> 4) & 0x0F) * 10) +
                             (Page & 0x0F) - 100;

                if (CacheIndex >= 0)
                {
                    /*STTBX_Print(("\EvenPage CacheIndex:................... %d\n", CacheIndex));*/
                    pCache = &PageCache[CacheIndex];

                    pCache->ValidLines |= (1 << PacketNo);
                    /*for (j = 0; j < 35; j++)
                    {
                    	STTBX_Print(("%x ",EvenPage->Lines[i][j]));
                    }*/
                    for (j = 6; j < 46; j++)
                    {

                        U8 c = ReverseBits(EvenPage->Lines[i][j]) & 0x7F;
                        c = (c < 32 || c > 126) ? 32 : c;
                        pCache->Lines[PacketNo][j-6] = c;
                       /* STTBX_Print(("%c", c));*/
                    }
                    pCache->Lines[PacketNo][j-6] = '\n';
                    pCache->Lines[PacketNo][j-5] = 0;
                }
            }
        }
    }
    /*STTBX_Print(("\nOddPage->ValidLines: %08x\n", OddPage->ValidLines));
    STTBX_Print(("\nEvenPage->ValidLines: %08x\n",EvenPage->ValidLines));*/
}

BOOL GetPageDetails(STTTX_Page_t *pOddPage, STTTX_Page_t *pEvenPage,
                    U8 *CurrentMagazine, U8 *CurrentPage,
                    U32 *CurrentSubpage, BOOL *bSerial)
{
    U32 PacketNo, PcktAddr1, PcktAddr2;
    U32 i;
    U8 *CurrLine;

    for (i = 0; i < STTTX_MAXLINES; i++)
    {
        PacketNo = -1;

        if ( ( pOddPage->ValidLines & (1 << i) ) != 0 )
        {

            PcktAddr1 = InvHamming8_4Tab[pOddPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[pOddPage->Lines[i][5]];

            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );
            CurrLine = pOddPage->Lines[i];
        }

        if ( ( pEvenPage->ValidLines & (1 << i) ) != 0 && PacketNo != 0 )
        {


            PcktAddr1 = InvHamming8_4Tab[pEvenPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[pEvenPage->Lines[i][5]];

            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );
            CurrLine = pEvenPage->Lines[i];
        }

        if (PacketNo == 0)
        {
            U8 PageNumb1, PageNumb2, SubPCode1, SubPCode2, SubPCode3,
            SubPCode4;

            /* Get page number */

            PageNumb1 = InvHamming8_4Tab[       /* LO nybble */
                        CurrLine[PAGE_NO_UNIT_POS]];
            PageNumb2 = InvHamming8_4Tab[       /* HO nybble */
                        CurrLine[PAGE_NO_TENS_POS]];
            *CurrentPage = PageNumb1 |
                           ( PageNumb2 <<
                             NYBBLE_SHIFT_CNT );

            /* Ensure this page is not used for time filling */
            if (*CurrentPage > 0x99)
                continue;               /* Try next line */

            /* Get magazine number */
            *CurrentMagazine = PcktAddr1 & 0x07;

            /* Get transmission mode */
            *bSerial = (CurrLine[13] & 0x02) ? TRUE : FALSE;

            /* Get subpage number */

            SubPCode4 =                     /* HI nybble */
                       InvHamming8_4Tab[CurrLine[SUBPAGE_THOU_POS]];
            SubPCode3 =
                       InvHamming8_4Tab[CurrLine[SUBPAGE_HUND_POS]];
            SubPCode2 =
                       InvHamming8_4Tab[CurrLine[SUBPAGE_TENS_POS]];
            SubPCode1 =                     /* LO nybble */
                       InvHamming8_4Tab[CurrLine[SUBPAGE_UNIT_POS]];
            *CurrentSubpage = ( SubPCode4 &
                                SUBCODE4_MASK ) <<
                              NYBBLE_SHIFT_CNT;
            *CurrentSubpage = ( SubPCode3 |
                                *CurrentSubpage ) <<
                              NYBBLE_SHIFT_CNT;
            *CurrentSubpage = ( ( SubPCode2 &
                                  SUBCODE2_MASK ) |
                                *CurrentSubpage ) <<
                              NYBBLE_SHIFT_CNT;
            *CurrentSubpage |= SubPCode1;

            return TRUE;
        }
    }

    return FALSE;
}

void CachePageData2(void)
{
    S32 i, j, CacheIndex;
    STTTX_Page_t *pCache;
    U8 Page;
    U8 *CurrLine;

    for (i = 0; i < STTTX_MAXLINES; i++)
    {
        if ( ( OddPage->ValidLines & (1 << i) ) != 0 )
        {
            S32 PacketNo, PcktAddr1, PcktAddr2;

            PcktAddr1 = InvHamming8_4Tab[OddPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[OddPage->Lines[i][5]];
            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );
            CurrLine = OddPage->Lines[i];


            if (PacketNo == 0)
            {
                U8 PageNumb1, PageNumb2;

                /* Get page number */
                PageNumb1 = InvHamming8_4Tab[       /* LO nybble */
                           CurrLine[PAGE_NO_UNIT_POS]];
                PageNumb2 = InvHamming8_4Tab[       /* HO nybble */
                            CurrLine[PAGE_NO_TENS_POS]];
                Page = PageNumb1 |
                               ( PageNumb2 <<
                                 NYBBLE_SHIFT_CNT );

                /* Ensure this page is not used for time filling */
                if (Page > 0x99)
                    continue;               /* Try next line */

                /* Get magazine number */

                /* Get transmission mode */

                /* Get subpage number */

                continue;
            }

            if (PacketNo > 0 && PacketNo < 26)
            {
                CacheIndex = ((PcktAddr1 & 0x07) * 100) +
                             (((Page >> 4) & 0x0F) * 10) +
                             (Page & 0x0F) - 100;

                if (CacheIndex >= 0)
                {
                    pCache = &PageCache[CacheIndex];

                    pCache->ValidLines |= (1 << PacketNo);
                    for (j = 6; j < 46; j++)
                    {
                        U8 c = ReverseBits(OddPage->Lines[i][j]) & 0x7F;
                        c = (c < 32 || c > 126) ? 32 : c;
                        pCache->Lines[PacketNo][j-6] = c;
                    }
                    pCache->Lines[PacketNo][j-6] = '\n';
                    pCache->Lines[PacketNo][j-5] = 0;
                }
            }
        }
    }

    for (i = 0; i < STTTX_MAXLINES; i++)
    {
        if ( ( EvenPage->ValidLines & (1 << i) ) != 0 )
        {
            S32 PacketNo, PcktAddr1, PcktAddr2;

            PcktAddr1 = InvHamming8_4Tab[EvenPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[EvenPage->Lines[i][5]];
            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );
            CurrLine = EvenPage->Lines[i];


            if (PacketNo == 0)
            {
                U8 PageNumb1, PageNumb2;

                /* Get page number */
                PageNumb1 = InvHamming8_4Tab[       /* LO nybble */
                           CurrLine[PAGE_NO_UNIT_POS]];
                PageNumb2 = InvHamming8_4Tab[       /* HO nybble */
                            CurrLine[PAGE_NO_TENS_POS]];
                Page = PageNumb1 |
                               ( PageNumb2 <<
                                 NYBBLE_SHIFT_CNT );

                /* Ensure this page is not used for time filling */
                if (Page > 0x99)
                    continue;               /* Try next line */

                /* Get magazine number */

                /* Get transmission mode */

                /* Get subpage number */

                continue;
            }

            if (PacketNo > 0 && PacketNo < 26)
            {
                CacheIndex = ((PcktAddr1 & 0x07) * 100) +
                             (((Page >> 4) & 0x0F) * 10) +
                             (Page & 0x0F) - 100;

                if (CacheIndex >= 0)
                {
                    pCache = &PageCache[CacheIndex];

                    pCache->ValidLines |= (1 << PacketNo);
                    for (j = 6; j < 46; j++)
                    {
                        U8 c = ReverseBits(EvenPage->Lines[i][j]) & 0x7F;
                        c = (c < 32 || c > 126) ? 32 : c;
                        pCache->Lines[PacketNo][j-6] = c;
                    }
                    pCache->Lines[PacketNo][j-6] = '\n';
                    pCache->Lines[PacketNo][j-5] = 0;
                }
            }
        }
    }
}


void TeletextStartCacheFilter(void)
{
    /* Setup up initial request for data */
    Request.RequestTag = 0;
    Request.RequestType = STTTX_FILTER_ANY;
    Request.NotifyFunction = CacheNotifyFunction;
    Request.OddPage = OddPage_wr;
    Request.EvenPage = EvenPage_wr;
    Request.MagazineNumber = 1;
    Request.PageNumber = 0;
    STTTX_SetRequest(STBHandle, &Request);
}

void TeletextGetPage(U32 PageNumber)
{
    if (PageCache[PageNumber-100].ValidLines != 0)
        DisplayCompletePage(&PageCache[PageNumber-100]);
    else
        STTBX_Print(("No data yet\n"));
}

void TeletextListPages(void)
{
    U32 i;

    for (i = 0; i < 800; i++)
    {
        if (PageCache[i].ValidLines != 0)
        {
            STTBX_Print(("%u ", i+100));
        }
    }
}

void TeletextCacheTask(void *pDummy)
{
    U8 CurrentPage = 0xFF;
    U8 CurrentMagazine = 0;
    U32 CurrentSubpage = 0;
    BOOL SerialMode, NewPage;

    CacheTaskStatus = CACHE_STATE_DECODING;
    STOS_TaskEnter (NULL);
    for (;;)
    {
        AwaitFilterHit();

        NewPage = GetPageDetails(OddPage, EvenPage,
                                 &CurrentMagazine, &CurrentPage,
                                 &CurrentSubpage, &SerialMode);

        /* Is new page details valid */
        if ( CurrentPage <= 0x99 )
        {
            if (!NewPage)
                CachePageData(OddPage, EvenPage, CurrentPage);
        }

        /* Increment consumer pointers */
        OddPage++;
        EvenPage++;

        /* Check for read pointer wrap */
        if( OddPage >= &Odd[PAGE_BUFFER_SIZE])
        {
            OddPage = Odd;
            EvenPage = Even;
        }
    }
    STOS_TaskExit (NULL);
}
void TeletextCacheTask2(void *pDummy)
{

    CacheTaskStatus = CACHE_STATE_DECODING;

    for (;;)
    {
        AwaitFilterHit();

        /* CachePageData(); */

        /* Increment consumer pointers */
        OddPage++;
        EvenPage++;

        /* Check for read pointer wrap */
        if( OddPage >= &Odd[PAGE_BUFFER_SIZE])
        {
            OddPage = Odd;
            EvenPage = Even;
        }
    }
}

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

#if defined(TTXDISK_TEST)
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
#elif defined(ST_OS20)
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
    U32 BufferPosition = 0;
    InsertParams_t *Params = (InsertParams_t *)Data;

    STOS_TaskEnter (NULL);
    /* Yes, I'm aware it will never exit. In this harness, that's okay. */
    while (InsertTaskExit == FALSE)
    {
        /* Copy PES size into Params->PesSize, move through buffer */
        memcpy(&Params->PesSize, &Params->PesBuffer_p[BufferPosition],
               sizeof(Params->PesSize));
        BufferPosition += sizeof(Params->PesSize);

        /* Point linearbuff at the PesBuffer_p, and then move the marker
         * forward again
         */
        Params->DriverPesBuffer_p = &Params->PesBuffer_p[BufferPosition];
        BufferPosition += Params->PesSize;

        /* Just let the user know if we've had to wrap */
        if ((BufferPosition >= Params->TotalSize) &&
            (Params->PesBuffer_p != NULL))
        {
            STTBX_Print(("\nWrapping to start of buffer (for next packet)\n"));
            BufferPosition = 0;
        }

        /* Don't want to flood the thing with data */
        STOS_TaskDelay(ST_GetClocksPerSecond() / 25);

        /* And off we go */
        STOS_SemaphoreSignal(Params->DataSemaphore_p);
    }
    STOS_TaskExit (NULL);
}
#endif

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

    STSYS_WriteRegDev32LE (FEI_SECTOR_SIZE, 0xBC);

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
#if defined(TTXSTDEMUX_TEST)
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
static ST_ErrorCode_t DEMUX_Init(U32 DemuxNum)
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
    InitParams.DemuxProcessPriority = 8;
    InitParams.DemuxMessageQueueSize = 256;
    InitParams.InputPacketSize = 192;
    InitParams.OutputModeBlocking = FALSE;
    InitParams.SyncLock = 1;
    InitParams.SyncDrop = 0;

    InitParams.NumOfPcktsInPesBuffer = 1000;
    InitParams.NumOfPcktsInRawBuffer =0 ;
    InitParams.NumOfPcktsInSecBuffer =0 ;

    /* Initialise stdemux driver */
    STTBX_Print(("STDEMUX_Init(%s)", STDEMUXDeviceName));
    error = STDEMUX_Init(STDEMUXDeviceName, &InitParams );
    ErrorReport( &StoredError, error, ST_NO_ERROR );

    return (error);

}
#endif
/* end DEMUX_Init */
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
    InitParams.Device = LOCAL_STPTI_DEVICE;
    InitParams.TCDeviceAddress_p = (STPTI_DevicePtr_t )LOCAL_PTI_ADDRESS;
#ifndef ST_OSLINUX    
    InitParams.TCLoader_p = LOCAL_STPTI_LOADER;
#endif
    InitParams.TCCodes = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
    InitParams.DescramblerAssociation =
                        STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    InitParams.Partition_p = TEST_PARTITION_1;
    InitParams.NCachePartition_p = TEST_PARTITION_2;

    strcpy(InitParams.EventHandlerName, EVTDeviceName);

    InitParams.EventProcessPriority = MIN_USER_PRIORITY;
    InitParams.InterruptProcessPriority = MAX_USER_PRIORITY;
    InitParams.InterruptNumber = LOCAL_PTI_INTERRUPT;
#ifndef ST_OSLINUX
    InitParams.InterruptLevel = 6;
#else
    InitParams.InterruptLevel = 0;
#endif 
    InitParams.SyncLock = 0;
    InitParams.SyncDrop = 0;
    InitParams.SectionFilterOperatingMode =
                        STPTI_FILTER_OPERATING_MODE_8x8;

#if defined(STPTI_CAROUSEL_SUPPORT)
    InitParams.CarouselProcessPriority = 0;
    InitParams.CarouselEntryType = STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE;
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
#ifdef ST_5301
    InitParams.StreamID = STPTI_STREAM_ID_TSIN1;
#else
    InitParams.StreamID = 0x80;
#endif
    InitParams.NumberOfSlots = 10;
    InitParams.AlternateOutputLatency = 42;
#endif
    InitParams.PacketArrivalTimeSource  = STPTI_ARRIVAL_TIME_SOURCE_PTI;
    /* Initialise stpti driver */
    STTBX_Print(("STPTI_Init=(%s)\n", STPTIDeviceName));
    error = STPTI_Init(STPTIDeviceName, &InitParams );
    ErrorReport( &StoredError, error, ST_NO_ERROR );
    return (error);


} /* end PTI_Init */

#if defined(ST_5528) || defined (ST_5100) || defined (ST_7710) || defined (ST_7100) || defined(ST_7109)
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

    STTBX_Print(("Calling STMERGE_Init()... \n"));
    error = STMERGE_Init(TSMERGEName, &InitParams);
    ErrorReport(StoredError, error, ST_NO_ERROR);
    error = STMERGE_ConnectGeneric(STMERGE_TSIN_0,STMERGE_PTI_0,STMERGE_BYPASS_GENERIC);
    ErrorReport(StoredError, error, ST_NO_ERROR);

    return error;

}
#elif defined (ST_5301)
ST_ErrorCode_t ConfigureMerger(ST_ErrorCode_t  *StoredError)
{
    ST_ErrorCode_t error;

    /* STMERGE in  BYPASS MODE  */
    STMERGE_InitParams_t     InitParams;
    STMERGE_ObjectConfig_t   SetParams;

    InitParams.DeviceType = STMERGE_DEVICE_1;
    InitParams.DriverPartition_p = TEST_PARTITION_1;
    InitParams.BaseAddress_p = (U32 *)TSMERGE_BASE_ADDRESS;
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

    /* Connect TSIN1 withPTI0 */
    error = STMERGE_Connect(STMERGE_TSIN_1,STMERGE_PTI_0);
    ErrorReport(StoredError, error, ST_NO_ERROR);

    return error;

}
#endif
#endif /* TTXSTPTI_TEST */

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

    VOUTInitParams.CPUPartition_p  = TEST_PARTITION_1;
    VOUTInitParams.MaxOpen         = 3;

    VOUTInitParams.DeviceType      = STVOUT_DEVICE_TYPE_V13;

    VOUTInitParams.OutputType      = STVOUT_OUTPUT_ANALOG_CVBS;

    memset((char*)&VOUTOpenParams,0,sizeof(STVOUT_OpenParams_t));

    switch ( VOUTInitParams.DeviceType )
    {
        case STVOUT_DEVICE_TYPE_DENC:
            strcpy( VOUTInitParams.Target.DencName, DENCDeviceName);
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
    STVMIX_InitParams.AVMEM_Partition2    = AvmemPartitionHandle;
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
/* End of ttxcache.c module */
