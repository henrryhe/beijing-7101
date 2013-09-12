/****************************************************************************

File Name   : ttxpage.c

Description : Example application that uses STTTX API to collect
              whole teletext pages.  Works with input stream from disk.

              This is code is not necessarily fully working or debugged
              but is provided to demonstrate how the STTTX API might
              be used.

Copyright (C) 2004, ST Microelectronics

References  :

****************************************************************************/


/* Includes -------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <ostime.h>
#include <string.h>
#include <debug.h>

#include "stlite.h"
#include "stdevice.h"
#include "stddefs.h"
#include "stttx.h"
#include "stttxdef.h"
#include "stpio.h"
#include "stdenc.h"
#include "stvtg.h"
#include "stevt.h"
#include "stboot.h"
#include "stcommon.h"
#include "sttbx.h"

#if !defined(STTTX_NO_TUNER)
#include "sttuner.h"
#endif

#if defined(mb282b) || defined(mb314) || defined(mb314_21)
#include "sti2c.h"
#endif

#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || (defined(TTXDISK_TEST) && defined(ST_5518))
#include "pti.h"
#elif defined(TTXSTPTI_TEST)
#include "stpti.h"
#endif

/* 5514 requires tsmux; it also requires STPTI */
#if defined(ST_5514) || defined(ST_5516) || defined (ST_5517)
    #include "sttsmux.h"
#endif

/* Private Constants ------------------------------------------------------ */

/*** Defines for dealing with pti ***/
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

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) \
    || defined(ST_5528) || defined (ST_5100)

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
#endif

#ifndef LOCAL_PTI_INTERRUPT
#define LOCAL_PTI_INTERRUPT             ST5528_PTIA_INTERRUPT
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
        #define LOCAL_STPTI_LOADER       STPTI_DVBTCLoader
    #endif
#elif defined(DVD_TRANSPORT_STPTI)
    #ifndef LOCAL_STPTI_LOADER
        #define LOCAL_STPTI_LOADER       STPTI_DVBTC1LoaderAutoInputcount
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
    semaphore_t DataSemaphore;
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
#define STTTX_PESBUFFER_SIZE 4000000
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
task_t CacheTask;
tdesc_t CacheTaskDescriptor;
U8 CacheTaskStack[1024 * 32];

/* Used for building cache of ASCII teletext pages */
STTTX_Page_t PageCache[NUMBER_CACHEABLE_PAGES];
STTTX_Request_t Request;

/*#if defined(TTXDISK_TEST)*/
InsertParams_t  VBIInsertParams;
U8              VBIInsertMainStack[4096];
task_t          IMVBITask;
tdesc_t         IMVBItdesc;
/*#endif*/

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
#if defined(TTXSTPTI_TEST)
static ST_DeviceName_t STPTIDeviceName = "STPTI";
#endif
#if defined(ST_5514) || defined(ST_5516)
static ST_DeviceName_t TSMUXName = "STTSMUX";
#endif

/* List of PIO device names */
static ST_DeviceName_t PioDeviceName = "I2C_PIO";

/* PIO Init parameters */
static STPIO_InitParams_t PioInitParams;



/* Global semaphores used in callbacks */
semaphore_t EventSemaphore;
semaphore_t FilterSemaphore;

#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST) || (defined(TTXDISK_TEST) && defined(ST_5518))
/* PTI slot for teletext data */
slot_t stb_slot;
#elif defined(TTXSTPTI_TEST)
STPTI_Handle_t  STPTIHandle;
STPTI_Slot_t    STBSlot;
STPTI_Buffer_t  STBBuffer;
STPTI_Signal_t  STBSignal;
#endif

#if defined(TTXDISK_TEST)
BOOL InsertTaskExit = FALSE;
#endif

/* Allow room for OS20 segments in internal memory */
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1200];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section")

#if defined(mb314) && !defined(UNIFIED_MEMORY)
#define                 NCACHE_MEMORY_SIZE          0x200000
#else
#define                 NCACHE_MEMORY_SIZE          0x80000
#endif
static unsigned char    ncache_memory [NCACHE_MEMORY_SIZE];
static partition_t      the_ncache_partition;
partition_t             *ncache_partition = &the_ncache_partition;
#pragma ST_section      ( ncache_memory , "ncache_section" )

#if defined(mb314) && !defined(UNIFIED_MEMORY)
#define                 SYSTEM_MEMORY_SIZE          0x800000
#else
#define                 SYSTEM_MEMORY_SIZE          0x400000
#endif
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

#if defined(ST_5528) || defined(ST_5100)
static U8               data_section[1];
#pragma ST_section      ( data_section,   "data_section")
#endif

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

void ErrorReport( ST_ErrorCode_t *ErrorStore,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr );
U32 ReadU32(void);
void GlobalInit(void);
U32 TeletextStartSTB(U32 Pid);
void AwaitFilterHit(void);
void CacheNotifyFunction( U32 Tag, STTTX_Page_t *p1, STTTX_Page_t *p2 );
BOOL IsEndOfPage(STTTX_Page_t *pOdd, STTTX_Page_t *pEven, BOOL SerialMode,
                 U8 Magazine, U8 Page);
void CachePageData(STTTX_Page_t *pOdd, STTTX_Page_t *pEven, U8 Page);

void TeletextStartCacheFilter(void);
void TeletextGetPage(U32 PageNumber);
void TeletextListPages(void);
void ErrorReport( ST_ErrorCode_t *ErrorStore,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr );

void DisplayPageDetails(STTTX_Page_t *pOddPage, STTTX_Page_t *pEvenPage);
void DisplayCompletePage(STTTX_Page_t *DisplayPage);


#if defined(TTXDISK_TEST)
static int InsertInit(U8 **PesBuffer_p, U32 *BufferUsed_p);
static void InsertMain( void *Data);
#endif

#if defined(TTXSTPTI_TEST)
static ST_ErrorCode_t PTI_Init(void);
#endif

#if defined(ST_5528) || defined (ST_5100)
ST_ErrorCode_t ConfigureMerger(ST_ErrorCode_t  *StoredError);
/* Temporary until STMERGE */
ST_ErrorCode_t TSMERGER_SetTSIn0BypassMode(void);
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
    STTBX_InitParams_t TBXInitParams;
    STBOOT_InitParams_t BootParams;
    STBOOT_TermParams_t BootTerm = { FALSE };

    /* Somewhat simplistic map, but since DMAs aren't really used in
     * this test harness that's okay.
     */
#if defined(DISABLE_DCACHE)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { NULL, NULL }
    };
#elif defined(mb314)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40200000, (U32 *)0x5FFFFFFF }, /* Cacheable */
        { NULL, NULL }                            /* End */
    };
#else
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40080000, (U32 *)0x4FFFFFFF }, /* Cacheable */
        { NULL, NULL }                            /* End */
    };
#endif

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

    internal_block_noinit[0] = 0;
    system_block_noinit[0]   = 0;

/* Avoid compiler warnings */
#if defined(ST_5528) || defined (ST_5100)
    data_section[0] = 0;
#endif

#if defined(DISABLE_ICACHE)
    BootParams.ICacheEnabled = FALSE;
#else
    BootParams.ICacheEnabled = TRUE;
#endif
    BootParams.DCacheMap = DCacheMap;
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
    TBXInitParams.CPUPartition_p = &the_system_partition;
    STTBX_Init( "tbx", &TBXInitParams );

#ifdef STTTX_NOSTEMBOARD
    /* Will probably change for other boards */
#if defined (mb314)
    /* Poke epld to select Tuner connectors as TS source NOT STEM */
    STTBX_Print(("MB314 EPLD Poke for Tuner connector select\n"));
    *(volatile U32*)(0x7e0c0000) = 1;
#endif
#endif

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

            else
            {
              STTBX_Print(("Invalid page - "));
              TeletextListPages();
            }
            task_delay(ST_GetClocksPerSecond()/10);
        }
    }

    STBOOT_Term( "boot", &BootTerm );
}

U32 ReadU32( void )
{
    U32 Val = 0;
    BOOL Finished = FALSE;

    while ( ! Finished )
    {
        while ( debugpollkey( &keybuff ) == 0 ) task_delay(ST_GetClocksPerSecond()/50);
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
    InitParams.TCLoader_p = LOCAL_STPTI_LOADER;
    InitParams.TCCodes = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
    InitParams.DescramblerAssociation =
                        STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_PIDS;
    InitParams.Partition_p = system_partition;
    InitParams.NCachePartition_p = ncache_partition;

    strcpy(InitParams.EventHandlerName, EVTDeviceName);

    InitParams.EventProcessPriority = MIN_USER_PRIORITY;
    InitParams.InterruptProcessPriority = MAX_USER_PRIORITY;
    InitParams.InterruptNumber = LOCAL_PTI_INTERRUPT;
    InitParams.InterruptLevel = 6;
    InitParams.SyncLock = 1;
    InitParams.SyncDrop = 3;
    InitParams.SectionFilterOperatingMode =
                        STPTI_FILTER_OPERATING_MODE_32x8;

#if defined(STPTI_CAROUSEL_SUPPORT)
    InitParams.CarouselProcessPriority = 0;
    InitParams.CarouselEntryType = STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE;
#endif

#if defined(STPTI_INDEX_SUPPORT)
    InitParams.IndexAssociation = STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    InitParams.IndexProcessPriority = 0;
#endif

#if defined(DVD_TRANSPORT_STPTI4)
    InitParams.StreamID = STPTI_STREAM_ID_NOTAGS;
    InitParams.NumberOfSlots = 10;
    InitParams.AlternateOutputLatency = 1;
#endif

    /* Initialise stpti driver */
    STTBX_Print(("STPTI_Init(%s)=", STPTIDeviceName));
    error = STPTI_Init(STPTIDeviceName, &InitParams );
    ErrorReport( &StoredError, error, ST_NO_ERROR );

    return (error);

}
/* end PTI_Init */

#if defined(ST_5528) || defined (ST_5100)
ST_ErrorCode_t ConfigureMerger(ST_ErrorCode_t  *StoredError)
{
    ST_ErrorCode_t error;

    /* STMERGE in  BYPASS MODE  */
    STMERGE_InitParams_t InitParams;

    InitParams.DeviceType = STMERGE_DEVICE_1;
    InitParams.DriverPartition_p = system_partition;
    InitParams.BaseAddress_p = (U32 *)TSMERGE_BASE_ADDRESS;
    InitParams.MergeMemoryMap_p = STMERGE_DEFAULT_MERGE_MEMORY_MAP;
    InitParams.Mode = STMERGE_NORMAL_OPERATION;

    InitParams.Mode = STMERGE_BYPASS_TSIN_TO_PTI_ONLY;

    STTBX_Print(("Calling STMERGE_Init()... \n"));
    error = STMERGE_Init(TSMERGEName, &InitParams);
    ErrorReport(StoredError, error, ST_NO_ERROR);

    return error;

}
#endif /* defined(ST_5528) */


#endif


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

#if !defined(STTTX_USE_DENC)
    STVBI_InitParams_t VBIInitParams;
#endif

    ST_GetClockInfo (&ClockInfo);

    /* Initialize PIO and I2C */

    PioInitParams.BaseAddress = (U32 *)TUNER_SSC_PIO_BASE;
    PioInitParams.InterruptNumber = TUNER_SSC_PIO_INTERRUPT;
    PioInitParams.InterruptLevel = TUNER_SSC_PIO_LEVEL;
    PioInitParams.DriverPartition = system_partition;

    STTBX_Print(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init(PioDeviceName,
                             &PioInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize EVT */
    EVTInitParams.MemoryPartition = (ST_Partition_t *)system_partition;
    EVTInitParams.EventMaxNum = 20;
    EVTInitParams.ConnectMaxNum = 20;
    EVTInitParams.SubscrMaxNum = 20;
    EVTInitParams.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() ................\n" ));
    ReturnError = STEVT_Init(EVTDeviceName, &EVTInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );



/* Initialize the DENC */
#if defined (ST_5100)
    DENCInitParams.DeviceType = STDENC_DEVICE_TYPE_V13;
#else
    DENCInitParams.DeviceType = STDENC_DEVICE_TYPE_DENC;
#endif
#if defined (ST_5528) || defined (ST_5100)
    DENCInitParams.STDENC_Access.EMI.Width = STDENC_EMI_ACCESS_WIDTH_32_BITS;
#else
    DENCInitParams.STDENC_Access.EMI.Width = STDENC_EMI_ACCESS_WIDTH_8_BITS;
#endif
    DENCInitParams.AccessType = STDENC_ACCESS_TYPE_EMI;
    DENCInitParams.STDENC_Access.EMI.BaseAddress_p = (void *)DENC_BASE_ADDRESS;
    DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = 0;
    DENCInitParams.MaxOpen = 5;

    STTBX_Print(( "Calling STDENC_Init() ................\n" ));
    ReturnError = STDENC_Init(DENCDeviceName, &DENCInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Initialize the VTG */

    /* Simulation of Vsync events on 5518, 5514, 5516, 5517 */
    /* On 5528 and 5100 Vsync are genrated by VTG driver    */


#if defined(ST_5528) || defined (ST_5100)
    /* Parameters obtained from Adel Chaouch, 28/10/03 */
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VFE2;
    VTGInitParams.Target.VtgCell2.BaseAddress_p = (void *)VTG_BASE_ADDRESS;
    VTGInitParams.Target.VtgCell2.DeviceBaseAddress_p = (void *)0;
    VTGInitParams.Target.VtgCell2.InterruptNumber = VTG_VSYNC_INTERRUPT;
#if defined (ST_5100)
    VTGInitParams.Target.VtgCell2.InterruptLevel = 0;   /* random number */
#else
    VTGInitParams.Target.VtgCell2.InterruptLevel = 7;   /* random number */
#endif

#else
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_DENC;
    strcpy(VTGInitParams.Target.DencName, DENCDeviceName);
#if !defined (ST_5528) && !defined(ST_5100)
#ifdef STVTG_VSYNC_WITHOUT_VIDEO
    VTGInitParams.VideoBaseAddress_p       = (void*) VIDEO_BASE_ADDRESS;
    VTGInitParams.VideoDeviceBaseAddress_p = (void*) 0;
    VTGInitParams.VideoInterruptNumber     = VIDEO_INTERRUPT;
    VTGInitParams.VideoInterruptLevel      = VIDEO_INTERRUPT_LEVEL;
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */
#endif
#endif
    strcpy(VTGInitParams.EvtHandlerName, EVTDeviceName);
    VTGInitParams.MaxOpen = 2;

    STTBX_Print(( "Calling STVTG_Init() ................\n" ));
    ReturnError = STVTG_Init(VTGDeviceName, &VTGInitParams);
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

    /* Open and configure the VTG */
    STTBX_Print(( "Calling STVTG_Open() ................\n" ));
    ReturnError = STVTG_Open(VTGDeviceName, &VTGOpenParams, &VTGHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STVTG_SetMode() ................\n" ));
    ReturnError = STVTG_SetMode(VTGHandle, STVTG_TIMING_MODE_576I50000_13500);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );



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

    PTI_Init();

    if (ReturnError == ST_NO_ERROR)
    {
        STPTIOpenParams.DriverPartition_p = system_partition;
        STPTIOpenParams.NonCachedPartition_p = ncache_partition;
        ReturnError = STPTI_Open(STPTIDeviceName,
                                 &STPTIOpenParams, &STPTIHandle);
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

    STTBX_Print(( "Using STTTX revision: %s\n", STTTX_GetRevision()));

    /* Initialize our native Teletext Driver */
    strcpy( DeviceNam, "STTTX" );
    InitParams_s.DriverPartition = (ST_Partition_t *)ncache_partition;
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;
    InitParams_s.InterruptNumber = TELETEXT_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 4;
    strcpy(InitParams_s.VBIDeviceName, STVBIDeviceName);
    strcpy(InitParams_s.EVTDeviceName, "STEVT");

    STTBX_Print(( "Calling STTTX_Init() ................\n" ));
    ReturnError = STTTX_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
}

U32 TeletextStartSTB( U32 Pid )
{

    ST_ErrorCode_t  ReturnError = ST_NO_ERROR;
    ST_ErrorCode_t  StoredError;
    STTTX_SourceParams_t SourceParams;



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
#elif defined(TTXSTPTI_TEST)
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
#if defined (ST_5528)
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

#if defined(ST_5514) || defined(ST_5516) || defined (ST_5517)
    {
        STTSMUX_InitParams_t  TSMUXInitParams;
        STTSMUX_OpenParams_t  TSMUXOpenParams;
        STTSMUX_Handle_t      TSMUXHandle;
        ST_ErrorCode_t        Error;

        /* Initialise TSMUX device */
        STTBX_Print(("Initialising TSMUX\n\n"));
        TSMUXInitParams.DeviceType = STTSMUX_DEVICE_GENERIC;
        TSMUXInitParams.DriverPartition_p = system_partition;
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
#endif /* 5514 || 5516 */

#if defined(TTXSTPTI_TEST)
#endif

#if defined(TTXSTPTI_TEST)
    {
        U16                   PacketCount;
        ST_ErrorCode_t        Error;

        STTBX_Print(("Sync packet count : "));
        task_delay(ST_GetClocksPerSecond());
        Error = STPTI_GetInputPacketCount(STPTIDeviceName, &PacketCount);
        STTBX_Print(("%u \n",PacketCount));
        ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
#endif /* ST_5514 */


    STTBX_Print(( "Calling STTTX_SetDataIdentifier() ...\n" ));
    ReturnError = STTTX_SetDataIdentifier( STBHandle, 0x10 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    semaphore_init_fifo(&VBIInsertParams.DataSemaphore, 0);

#if defined(TTXPTI_TEST) || defined(TTXLINK_TEST)
    SourceParams.DataSource = STTTX_SOURCE_PTI_SLOT;
    SourceParams.Source_u.PTISlot_s.Slot = stb_slot;
#elif defined(TTXSTPTI_TEST)
    SourceParams.DataSource = STTTX_SOURCE_STPTI_SIGNAL;
    SourceParams.Source_u.STPTISignal_s.Signal = STBSignal;
#else
    /* Do the insert setup */
    if (InsertInit(&VBIInsertParams.PesBuffer_p, &VBIInsertParams.TotalSize) != 0)
    {
        /* error, can't continue. */
        return -1;
    }
    VBIInsertParams.DriverPesBuffer_p = VBIInsertParams.PesBuffer_p;

    SourceParams.DataSource = STTTX_SOURCE_USER_BUFFER;
    SourceParams.Source_u.UserBuf_s.DataReady_p = &VBIInsertParams.DataSemaphore;
    SourceParams.Source_u.UserBuf_s.PesBuf_p = &VBIInsertParams.DriverPesBuffer_p;
    SourceParams.Source_u.UserBuf_s.BufferSize = &VBIInsertParams.PesSize;
#endif


    STTBX_Print(( "Calling STTTX_SetSource() ...............\n" ));
    ReturnError = STTTX_SetSource(STBHandle, &SourceParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(TTXDISK_TEST)
    /* Start off the VBI insert task */
    if (task_init( (void(*)(void *))InsertMain,
                    (void *)&VBIInsertParams,
                    VBIInsertMainStack, 4096,
                    &IMVBITask, &IMVBItdesc,
                    MIN_USER_PRIORITY,
                    "Packet insertion", 0) != 0)
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
        task_delay(ST_GetClocksPerSecond());
        pti_slot_packet_count(stb_slot, &count);
        STTBX_Print(("Slot packet count: %u\n", count));
    }
#endif

    return (ReturnError == ST_NO_ERROR)?0:-1;
}



/* Called when a request is met */
void CacheNotifyFunction( U32 Tag, STTTX_Page_t *p1, STTTX_Page_t *p2 )
{
    ST_ErrorCode_t error = ST_NO_ERROR, stored = ST_NO_ERROR;

    /* check which request has been notified */
    if (Tag == 0)
    {
        STTBX_Print(("\n\nNew Page.\nEven Lines...."));
        DisplayPageDetails(NULL, p2);

        STTBX_Print(("\n\nOdd Lines...."));
        DisplayPageDetails(p1, NULL);

        /* Resets request */
        if (CacheTaskStatus != CACHE_STATE_COMPLETE)
        {
            Request.RequestTag     = 0;
            Request.RequestType    =  STTTX_FILTER_PAGE_COMPLETE_ODD |
                                      STTTX_FILTER_PAGE_COMPLETE_EVEN;
            Request.MagazineNumber = 1;
            Request.PageNumber     = 0;
            Request.PageSubCode    = 0;

            Request.OddPage = Odd;
            Request.EvenPage = Even;

            STTBX_Print(("Setting new request; page 100/0, odd or even\n"));
            error = STTTX_SetRequest( STBHandle, &Request );
            if (error != ST_NO_ERROR)
            {
                ErrorReport(&stored, error, ST_NO_ERROR);
            }
        }
    }
}


/* Outputs the results of the filter on screen */
void DisplayPageDetails(STTTX_Page_t *pOddPage, STTTX_Page_t *pEvenPage)
{
    U32  PacketNo=0, PcktAddr1=0, PcktAddr2=0;
    U32  i;
    U8  *CurrLine=0;
    U8   CurrentMagazine=0;
    U8   CurrentPage=0;
    U32  CurrentSubpage=0;
    BOOL bSerial;

    for (i = 0; i < STTTX_MAXLINES; i++)
    {
        PacketNo = -1;

        /* checks the validity of each line */
        /* works out packet number at each line by combining byte 4 and 5 */
        if ((pOddPage != NULL) &&
            ((pOddPage->ValidLines & (1 << i) ) != 0) &&
            (PacketNo != 0))
        {
            PcktAddr1 = InvHamming8_4Tab[pOddPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[pOddPage->Lines[i][5]];
            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );
            CurrLine = pOddPage->Lines[i];
            STTBX_Print(("\n[Odd ]"));
        }

        if ((pEvenPage != NULL) &&
            ((pEvenPage->ValidLines & (1 << i)) != 0) &&
            (PacketNo != 0))
        {
            PcktAddr1 = InvHamming8_4Tab[pEvenPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[pEvenPage->Lines[i][5]];
            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );
            CurrLine = pEvenPage->Lines[i];
            STTBX_Print(("\n[Even]"));
        }

        if (PacketNo != -1)
        {
            STTBX_Print((" Line no %d Pckt no %d mag %d ",
                        i, PacketNo, (PcktAddr1 & MAGAZINE_MASK)));
        }

        /********** this should be == 0 but i turned it of for now */
        /* XXX What is this? PW */
        if (PacketNo == 0)
        {
            U8 PageNumb1, PageNumb2, SubPCode1, SubPCode2, SubPCode3,
            SubPCode4;

            /* Get page number - LO nybble */
            PageNumb1 = InvHamming8_4Tab[CurrLine[PAGE_NO_UNIT_POS]];
            /* HI nybble */
            PageNumb2 = InvHamming8_4Tab[CurrLine[PAGE_NO_TENS_POS]];
            CurrentPage = PageNumb1 | (PageNumb2 << NYBBLE_SHIFT_CNT);

            STTBX_Print((" page no %d", CurrentPage));

            /* Ensure this header packet is not used for time filling */
            if (CurrentPage > 0x99)
                continue;               /* Try next line */

            /* Get magazine number */
            CurrentMagazine = PcktAddr1 & MAGAZINE_MASK;

            /* Get transmission mode */
            bSerial = (CurrLine[13] & 0x02) ? TRUE : FALSE;

            /* Get subpage number - HI nybble */
            SubPCode4 = InvHamming8_4Tab[CurrLine[SUBPAGE_THOU_POS]];
            SubPCode3 = InvHamming8_4Tab[CurrLine[SUBPAGE_HUND_POS]];
            SubPCode2 = InvHamming8_4Tab[CurrLine[SUBPAGE_TENS_POS]];
            /* LO nybble */
            SubPCode1 = InvHamming8_4Tab[CurrLine[SUBPAGE_UNIT_POS]];
            CurrentSubpage = (SubPCode4 & SUBCODE4_MASK) << NYBBLE_SHIFT_CNT;
            CurrentSubpage = (SubPCode3 | CurrentSubpage) << NYBBLE_SHIFT_CNT;
            CurrentSubpage = ((SubPCode2 & SUBCODE2_MASK) | CurrentSubpage) <<
                                  NYBBLE_SHIFT_CNT;
            CurrentSubpage |= SubPCode1;
        }
    }
}


void TeletextStartCacheFilter(void)
{
    /* Setup up initial request for data */
    Request.RequestTag      = 0;
    Request.NotifyFunction  = CacheNotifyFunction;
    Request.OddPage         = Odd;
    Request.EvenPage        = Even;
    Request.MagazineNumber  = 1;
    Request.PageNumber      = 0;
    Request.RequestType     = STTTX_FILTER_ANY;
    STTTX_SetRequest(STBHandle, &Request);
}


void TeletextGetPage(U32 PageNumber)
{
    if (PageCache[PageNumber-100].ValidLines != 0)
        DisplayCompletePage(&PageCache[PageNumber-100]);
    else
        debugmessage("No data yet\n");
}


void DisplayCompletePage(STTTX_Page_t *DisplayPage)
{
    U32 i;

    STTBX_Print(("\nValidLines: %08x\n", DisplayPage->ValidLines));
    for (i = 1; i < 26; i++)
    {
        if ((DisplayPage->ValidLines & (1 << i)) != 0)
            debugmessage((char *)DisplayPage->Lines[i]);
        else
            debugmessage("\n");
    }
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
    /* Get memory */
    *PesBuffer_p = (U8 *)memory_allocate(
                    (partition_t *)system_partition, STTTX_PESBUFFER_SIZE );
    if (*PesBuffer_p == NULL)
    {
        debugmessage("Unable to allocate memory for PesBuffer_p!\n");
        return -1;
    }

    /* If we're inserting, then load the data from the file */
    {
        S32 file = (S32)debugopen(STTTX_PES_FILE, "rb");
        if (file == -1)
        {
            debugmessage("Error trying to open file\n");
            return -2;
        }
        else
        {
            if (PesBuffer_p != NULL)
            {
                if (debugfilesize(file) < STTTX_PESBUFFER_SIZE)
                {
                    debugmessage("Reading from file...\n");
                    *BufferUsed_p = (U32)debugread(file, *PesBuffer_p, (int)debugfilesize(file));
                }
                else
                {
                    debugmessage("WARNING! PES file is larger than buffer!\n");
                    debugmessage("Reading as much as possible, but we are very likely to hit problems later!");
                    *BufferUsed_p = (U32)debugread(file, *PesBuffer_p, STTTX_PESBUFFER_SIZE);
                }
                STTBX_Print(("Read %i bytes from file\n", *BufferUsed_p));
                debugclose(file);
            }
            else
            {
                debugmessage("Unable to read from file; no buffer\n");
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

    STTBX_Print (("\nST_GetClocksPerSecond = %u\n",ST_GetClocksPerSecond()));
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
            STTBX_Print(("Wrapping to start of buffer (for next packet)\n"));
            BufferPosition = 0;
        }

        /* Don't want to flood the thing with data */
        task_delay(ST_GetClocksPerSecond() / 25);

        /* And off we go */
        semaphore_signal(&Params->DataSemaphore);
    }
}
#endif


/* End of ttxcache.c module */
