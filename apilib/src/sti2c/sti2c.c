/******************************************************************************

File name       : sti2c.c

Description     : Driver code for STAPI I2C interface

Reference       : 1)  STi5514 Datasheet
                  2)  Functional Specification rev A : ADCS 7184762
                  3)  The I2C-bus specification ver2.1   Philips Semiconductors
Revision History:

    [...]
    10Dec03     Updated for 5528 changes
    19March04   Updated for 5100 changes and resolution of DDTS 30111,300127,29417
    23Dec04     Updated for 5105 changes
    15Feb05     Updated for 7100 changes
    01Dec05     Updated for 5188 changes
    27Apr06     Updated for 5107 changes
    19Aug06     Updated for STOS support
    06Mar07     Updated for 7200 changes

COPYRIGHT (C) STMicroelectronics 2007

This driver supports both master mode, slave mode & mixed mode operation.

******************************************************************************/

/* Includes------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "stcommon.h"
#include "stddefs.h"
#ifndef  STI2C_MASTER_ONLY
#include "stevt.h"
#endif

#include "stos.h"
#include "sti2c.h"
#include "stlite.h"
#include "stpio.h"
#include "sttbx.h"
#include "stdevice.h"

/* Constants ----------------------------------------------------------------*/

#if defined(STI2C_DEBUG)
#define TRACEBUFSIZE        32 * 1024
BOOL DoOutput = TRUE;
#endif

#if defined(ST_7710) || defined(ST_5525)
#define NUM_SSC 4
#elif defined(ST_5100)||defined(ST_7100)||defined(ST_5301)||defined(ST_8010)||defined(ST_7109)
#define NUM_SSC 3
#elif defined(ST_7200)
#define NUM_SSC 5
#else
#define NUM_SSC 2 /*5105,5188,5107*/
#endif

#define MAGIC_NUM 0x08080808

/* Maximum number of times GenerateClocks should try clock-pulse */
#define I2C_SCL_MAXCLOCKS   15

/* DDTS 41388:Task_delay in STI2C_Lock too long. */
#ifndef STI2C_LOCK_DELAY
#define STI2C_LOCK_DELAY     100                 /* by default 100 ms */
#endif

/* --- SSC offsets in 32-bit chunks --- */
#define SSC_BAUD_RATE                  0
#define SSC_TX_BUFFER                  1
#define SSC_RX_BUFFER                  2
#define SSC_CONTROL                    3
#define SSC_INT_ENABLE                 4
#define SSC_STATUS                     5
#define SSC_I2C                        6
#define SSC_SLAD                       7
#define SSC_CLR                       32
#define SSC_GLITCH_WIDTH              64
#define SSC_PRE_SCALER                65
#define SSC_GLITCH_WIDTH_DATAOUT      66
#define SSC_PRE_SCALER_DATAOUT        67

/* --- The i2c control register bits ---  */
#define SSC_I2C_ENABLE           0x01
#define SSC_I2C_GENERATE_START   0x02
#define SSC_I2C_GENERATE_STOP    0x04
#define SSC_I2C_GENERATE_ACKS    0x08
#define SSC_I2C_10_BIT_ADDRESS   0x10
#define SSC_I2C_TXENABLED        0x20           /* Relevant to SSC3 only */
#define SSC_I2C_REPSTRTG         0x800          /* Relevant to SSC3 only */
#define SSC_I2C_FASTMODE         0x1000         /* Relevant to SSC3 only */

/* ---  The SSC control register bits --- */
#define SSC_CON_DATA_WIDTH_7     0x06
#define SSC_CON_DATA_WIDTH_8     0x07
#define SSC_CON_DATA_WIDTH_9     0x08
#define SSC_CON_HEAD_CONTROL     0x10
#define SSC_CON_CLOCK_PHASE      0x20
#define SSC_CON_CLOCK_POLARITY   0x40
#define SSC_CON_RESET            0x80
#define SSC_CON_MASTER_SELECT    0x100
#define SSC_CON_ENABLE           0x200
#define SSC_CON_LOOP_BACK        0x400
#define SSC_CON_ENB_TX_FIFO      0x800           /* Relevant to SSC4 only */
#define SSC_CON_ENB_RX_FIFO      0x1000          /* Relevant to SSC4 only */
#define SSC_CON_ENB_CLST_RX      0x2000          /* Relevant to SSC4 only */
/* ---  The status register bits --- */
#define SSC_STAT_RX_BUFFER_FULL  0x01
#define SSC_STAT_TX_BUFFER_EMPTY 0x02
#define SSC_STAT_TX_ERROR        0x04
#define SSC_STAT_RX_ERROR        0x08
#define SSC_STAT_PHASE_ERROR     0x10
#define SSC_STAT_STRETCH         0x20
#define SSC_STAT_AAS             0x40
#define SSC_STAT_STOP            0x80
#define SSC_STAT_ARBL            0x100
#define SSC_STAT_BUSY            0x200
#define SSC_STAT_NACK            0x400
#define SSC_STAT_REPSTRT         0x800
#define SSC_STAT_TX_HALF_EMPTY   0x1000           /* Relevant to SSC4 only */
#define SSC_STAT_TX_FULL         0x2000           /* Relevant to SSC4 only */
#define SSC_STAT_RX_HALF_FULL    0x4000           /* Relevant to SSC4 only */
/* --- For clearing status register bits (SSC3) --- */
#define SSC_CLR_AAS              0x40
#define SSC_CLR_STOP             0x80
#define SSC_CLR_ARBL             0x100
#define SSC_CLR_NACK             0x400
#define SSC_CLR_REPSTRT          0x800

/* ---  The interrupt enable register bits --- */
#define SSC_IEN_RX_BUFFER_FULL   0x01
#define SSC_IEN_TX_BUFFER_EMPTY  0x02
#define SSC_IEN_TX_ERROR         0x04
#define SSC_IEN_RX_ERROR         0x08
#define SSC_IEN_PHASE_ERROR      0x10
#define SSC_IEN_AAS              0x40
#define SSC_IEN_STOP             0x80
#define SSC_IEN_ARBL             0x100
#define SSC_IEN_NACK             0x400
#define SSC_IEN_REPSTRT          0x800
#define SSC_IEN_TX_HALF_EMPTY    0x1000           /* Relevant to SSC4 only */
#define SSC_IEN_TX_FULL          0x2000           /* Relevant to SSC4 only */
#define SSC_IEN_RX_HALF_FULL     0x4000           /* Relevant to SSC4 only */

#if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105) \
|| defined(ST_7100)  || defined(ST_5301) || defined(ST_5700) || defined(ST_8010) \
|| defined(ST_7109)  || defined(ST_5188) || defined(ST_5525) || defined(ST_5107) \
|| defined(ST_7200)

/* Timing registers; 5528+ only */
#define SSC_REP_START_HOLD_TIME     8   /* base + 0x20  */
#define SSC_START_HOLD_TIME         9   /* base + 0x24  */
#define SSC_REP_START_SETUP_TIME   10   /* base + 0x28  */
#define SSC_DATA_SETUP_TIME        11   /* base + 0x2c  */
#define SSC_STOP_SETUP_TIME        12   /* base + 0x30  */
#define SSC_BUS_FREE_TIME          13   /* base + 0x34  */
#endif

#define SSC_TX_FSTAT               14   /* base + 0x38  */
#define SSC_RX_FSTAT               15   /* base + 0x3c  */
#define SSC_AGFR_DATAOUT           42   /* base + 0x108 */
#define SSC_PRSC_DATAOUT           43   /* base + 0x10c */


/* --- Default values for SSC3_ for various registers --- */
#define SSC_STAT_MASK     (SSC_STAT_RX_BUFFER_FULL | SSC_STAT_TX_BUFFER_EMPTY | \
                           SSC_STAT_TX_ERROR | SSC_STAT_RX_ERROR | \
                           SSC_STAT_PHASE_ERROR | SSC_STAT_STRETCH | \
                           SSC_STAT_AAS | SSC_STAT_STOP | SSC_STAT_ARBL | \
                           SSC_STAT_BUSY | SSC_STAT_NACK | SSC_STAT_REPSTRT)

#define SSC_CLR_MASK      (SSC_CLR_AAS | SSC_CLR_STOP | SSC_CLR_ARBL | \
                           SSC_CLR_NACK | SSC_CLR_REPSTRT)

#define DEFAULT_I2C        SSC_I2C_ENABLE | SSC_I2C_GENERATE_ACKS


#define DEFAULT_CONTROL   (SSC_CON_DATA_WIDTH_9  | SSC_CON_HEAD_CONTROL | \
                           SSC_CON_CLOCK_PHASE | SSC_CON_CLOCK_POLARITY | \
                           SSC_CON_ENABLE )

#define DEFAULT_ENABLES   (SSC_IEN_AAS | SSC_IEN_TX_ERROR | \
                           SSC_IEN_STOP | SSC_IEN_REPSTRT )

/* --- misc constants & defines --- */

#define MILLISEC_PER_SEC            1000
#define MICROSEC_PER_SEC            1000000

#define ACKNOWLEDGE_BIT             0x01

#if defined(mb395)
#define STOP_BUSY_WAIT              5000
#else
#define STOP_BUSY_WAIT              1000
#endif

#define REP_START_HOLD_TIME_STD     4500 /* 6500*/
#define REP_START_HOLD_TIME_FAST    6500

#define REP_START_SETUP_TIME_STD    4500 /* 4900*/
#define REP_START_SETUP_TIME_FAST   800

#if defined(mb395)
#define START_HOLD_TIME_STD         5500
#else
#define START_HOLD_TIME_STD         4500
#endif
#define START_HOLD_TIME_FAST        800

#define DATA_SETUP_TIME_STD         300
#define DATA_SETUP_TIME_FAST        300

#if defined(mb395)
#define STOP_SETUP_TIME_STD         9000
#else
#define STOP_SETUP_TIME_STD         4200
#endif
#define STOP_SETUP_TIME_FAST        800

#define BUS_FREE_TIME_STD           5000
#define BUS_FREE_TIME_FAST          1500

#if defined(mb395)
#define GLITCH_WIDTH_CLOCKIN        1500
#define GLITCH_WIDTH_DATAOUT        1500   /* in nanosecs */
#else
#define GLITCH_WIDTH_CLOCKIN        500
#define GLITCH_WIDTH_DATAOUT        500   /* in nanosecs */
#endif

#define PRE_SCALER_FACTOR           0x01

/* Not only with SSC3, but one with timing registers to be written. */
#if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105) \
|| defined(ST_7100)  || defined(ST_5301) || defined(ST_5700) || defined(ST_8010) \
|| defined(ST_7109)  || defined(ST_5188) || defined(ST_5525) || defined(ST_5107) \
|| defined(ST_7200)
#define SSC3_WITH_TIMING_PRESENT
#endif

#if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)||defined(ST_7100) \
|| defined(ST_5301)||defined(ST_8010)||defined(ST_7109) || defined(ST_5188)\
|| defined(ST_5525) || defined(ST_5107)|| defined(ST_7200)
#define SSC4_PRESENT
#endif

#define PIO_PUSHPULL(PioReg,BitMask)  {  PioReg[PIO_C0_CLR] = BitMask;\
                                         PioReg[PIO_C1_SET] = BitMask;\
                                         PioReg[PIO_C2_CLR] = BitMask;  }
#define PIO_ALTBI(PioReg,BitMask)     {  PioReg[PIO_C0_SET] = BitMask;\
                                         PioReg[PIO_C1_SET] = BitMask;\
                                         PioReg[PIO_C2_SET] = BitMask;  }

/* Init block 0 PIO */
#define PIO_OUT_SET        1
#define PIO_OUT_CLR        2
#define PIO_C0_SET         9
#define PIO_C0_CLR         10
#define PIO_C1_SET         13
#define PIO_C1_CLR         14
#define PIO_C2_SET         17
#define PIO_C2_CLR         18

#define MASTER_STATE(p)      ( ((p)->State) >= STI2C_MASTER_ADDRESSING_WRITE )
#define SLAVE_STATE(p)       ( ((p)->State) <= STI2C_SLAVE_RECEIVE )

/* Typedefs -----------------------------------------------------------------*/
#if (!defined(PROCESSOR_C1)&& defined(ST_OS20))
#pragma ST_device (DU16)
#pragma ST_device (DU32)
#endif

typedef volatile U16       DU16;
typedef volatile U32       DU32;

/* Driver state - note order is important so we can do < & > comparisons */
typedef enum STI2C_DriverState_e
{
    STI2C_SLAVE_ADDRESSED              = 0,
    STI2C_SLAVE_TRANSMIT               = 1,
    STI2C_SLAVE_RECEIVE                = 2,
    STI2C_IDLE                         = 3,
    STI2C_MASTER_ADDRESSING_WRITE      = 4,
    STI2C_MASTER_WRITING               = 5,
    STI2C_MASTER_ADDRESSING_READ       = 6,
    STI2C_MASTER_READING               = 7
} STI2C_DriverState_t;

typedef struct i2c_param_block_s
{
    STI2C_InitParams_t         InitParams;
    STPIO_Handle_t             PIOHandle;
    ST_DeviceName_t            Name;
    BOOL                       LastTransferHadStop;
    U32                        OpenCnt;
    U16                        InterruptMask;
    STI2C_Handle_t             *LockedHandlePool_p;
    STI2C_Handle_t             LockedHandleCurrent;
    semaphore_t                *BusAccessSemaphore_p;
    semaphore_t                *IOSemaphore_p;
    STI2C_DriverState_t        State;
    U8                        *Buffer_p;
    S32                        BufferLen;
    S32                        BufferCnt;
    U32                        HandlerCondition;
#ifndef STI2C_MASTER_ONLY
    STEVT_Handle_t             EVTHandle;
    STEVT_EventID_t            TxNotifyId;
    STEVT_EventID_t            RxNotifyId;
    STEVT_EventID_t            TxByteId;
    STEVT_EventID_t            RxByteId;
    STEVT_EventID_t            TxCompleteId;
    STEVT_EventID_t            RxCompleteId;
    STEVT_EventID_t            BusStuckId;
    STI2C_DriverState_t        SlaveStatus;         /*used for 5516 Rxbufferfull AAS workaround*/
#endif
    U32                        *BaseAddressforSDA;  /* for direct PIO access */
    U32                        *BaseAddressforSCL;  /* for direct PIO access */
    STPIO_PIOBit_t             PIOforSCL;           /* for direct PIO access */
    STPIO_PIOBit_t             PIOforSDA;           /* for direct PIO access */
    U32                        DataToSend;
    struct i2c_open_block_s    *OpenBlockPtr;
    BOOL                       InUse;
} i2c_param_block_t;

static  i2c_param_block_t CtrlBlockArray[NUM_SSC];

typedef struct i2c_open_block_s
{
    U32                         MagicNumber;
    STI2C_OpenParams_t          OpenParams;
    i2c_param_block_t           *CtrlBlkPtr;
} i2c_open_block_t;


typedef enum i2c_context_e
{
    READING,
    WRITING
} i2c_context_t;

/* Externs ------------------------------------------------------------------*/
#if defined(STI2C_TRACE)
#define TRACE_PAGE_SIZE        50
#endif
#if defined(STI2C_DEBUG)
U8 TraceBuffer[TRACEBUFSIZE];
U32 TraceBufferPos;
#endif

/*extern*/
extern ST_ErrorCode_t STPIO_GetBaseAddress (ST_DeviceName_t DeviceName,U32 **BaseAddress);
/* Statics ------------------------------------------------------------------*/

static   const ST_Revision_t  g_Revision  = "STI2C-REL_2.4.1";

/* Used to ensure exclusive access to global control list */

static  semaphore_t         *g_DriverAtomic_p = NULL;
#if defined(STI2C_STSPI_SAME_SSC_SUPPORT)
ST_ErrorCode_t I2c_GetSSC(i2c_param_block_t *Params_p);
ST_ErrorCode_t I2c_FreeSSC(i2c_param_block_t *Params_p);
#endif
static void I2cStop (i2c_param_block_t *Params_p);
static void I2c_Deallocate(i2c_param_block_t    *CtrlBlkPtr_p);
static BOOL BusStuck (i2c_param_block_t    *InitBlock_p) ;
#if defined(SSC3_WITH_TIMING_PRESENT)
static void i2c_WriteTimingValues(i2c_param_block_t    *CtrlBlkPtr_p);
#endif
ST_ErrorCode_t STI2C_Reset (STI2C_Handle_t Handle);
STOS_INTERRUPT_DECLARE(I2cHandler, Param);
static ST_ErrorCode_t BusAccess ( STI2C_Handle_t    Handle,
                                  U8                *Buffer_p,
                                  U32               BufferLen,
                                  U32               TimeOutMS,
                                  U32               *ActLen_p,
                                  i2c_context_t     Context,
                                  BOOL              DoStop  );
#ifdef STI2C_MASTER_ONLY
static void I2cReset(i2c_param_block_t *Params_p);
#endif

static BOOL First_Init = TRUE;

#if defined(STI2C_DEBUG)
static  BOOL    InInterrupt = FALSE;
#endif

/* Debug facilities ---------------------------------------------------------*/
#if !defined(STI2C_NO_TBX)
#define STI2C_Print(x)          STTBX_Print(x);
#else
#define STI2C_Print(x)          printf x;
#endif

#ifdef STI2C_TRACE

#if (!defined(PROCESSOR_C1)&& defined(ST_OS20))
#define TIME_NOW GetHpClock()
#else
#define TIME_NOW STOS_time_now()
#endif

#endif
#ifdef STI2C_TRACE
/* Simple trace facility */
#define DBG_SIZE              10000
#define DBG_REC_SIZE          10
U32 I2cIntCount = 0;
U32 I2cErrCount = 0;
volatile U32 I2cIndex     = 0;
volatile U32 I2cLastIndex = 0;
U32 I2cDebug [DBG_SIZE * DBG_REC_SIZE];

#define I2C_TRACE(p,id,data)                                   \
    {                                                          \
        U32 t = I2cIndex*DBG_REC_SIZE;                         \
        if (++I2cIndex >= DBG_SIZE)                            \
        {                                                      \
            I2cIndex = 0;      /* wrap around */               \
        }                                                      \
        I2cDebug[t  ] = I2cIntCount;                           \
        I2cDebug[t+1] = (id);                                  \
        I2cDebug[t+2] = I2cErrCount;                           \
        I2cDebug[t+3] = (p)->State;                            \
        I2cDebug[t+4] = (p)->HandlerCondition;                 \
        I2cDebug[t+5] = (p)->InterruptMask;                    \
        I2cDebug[t+6] = (p)->BufferCnt;                        \
        I2cDebug[t+7] = (p)->BufferLen;                        \
        I2cDebug[t+8] = (data);                                \
        I2cDebug[t+9] = TIME_NOW;                              \
    }

#define I2C_TRACE_LOCK(p,id,data)                              \
    STOS_InterruptLock();                                          \
    I2C_TRACE(p,id,data);                                      \
    STOS_InterruptUnlock();

#define I2C_TRACE_ERR_INC            I2cErrCount++;
#define I2C_TRACE_INT_INC            I2cIntCount++;
#if (!defined(PROCESSOR_C1)&& defined(ST_OS20))
static U32 GetHpClock (void);
#pragma ST_inline (GetHpClock)
#endif
#else     /* not STI2C_TRACE */

#define I2C_TRACE(p,id,data)
#define I2C_TRACE_LOCK(p,id,data)
#define I2C_TRACE_ERR_INC
#define I2C_TRACE_INT_INC

#endif /* STI2C_TRACE */

/* Debug Functions ----------------------------------------------------------*/

#ifdef STI2C_TRACE
#if (!defined(PROCESSOR_C1)&& defined(ST_OS20))
static U32 GetHpClock (void)
{
    U32 temp;
    __optasm{ldc 0; ldclock; st temp;}
    return temp;
}
#endif
U32 I2cPrintTrace (S32 Start, S32 Number)
{
    U32 i, n, first, last, index=I2cIndex, result=0;

    if (Start<0)     /* special case - print from last print position */
    {
        first = I2cLastIndex;
        n = (index >= first) ? index-first : (index+DBG_SIZE)-first;
        if ((Number <= 0) || (Number > n))
        {
            last = index;
        }
        else
        {
            last   = (first+Number) % DBG_SIZE;
            result = (index>last) ? index-last : (index+DBG_SIZE)-last;
        }
        I2cLastIndex = last;
    }
    else
    {
        first = Start;
        last  = (Number <= 0) ? index : (first + Number) % DBG_SIZE;
    }

    n = (last>first) ? last-first : (last+DBG_SIZE)-first;
    if (Number > 0)
    {
        STI2C_Print(( "Current Index: %u   First: %u   Last: %u  (%u records)\n",
                      index, first, last, n ));
        STI2C_Print(( " Ints   Id  Errs State  ErrCode  IEn  Cnt  Len     Data       Time\n" ));
        i = first;
        while (i != last)
        {
            n = i*10;
            STI2C_Print(( "%5d %4x %5d %5d %8x %4x %4d %4d %8x %12u\n",
                   I2cDebug[n  ], I2cDebug[n+1], I2cDebug[n+2],
                   I2cDebug[n+3], I2cDebug[n+4], I2cDebug[n+5],
                   I2cDebug[n+6], I2cDebug[n+7], I2cDebug[n+8],
                   I2cDebug[n+9] ));
            if( ++i >= DBG_SIZE )
            {
                i = 0;
            }
        }
    }
    return result;
}
#ifdef STI2C_TRACE

static U8 GetNextChar (void)
{
    U8 s[80];
    long Length, i;

    while (1)
    {
        if ( (!(&s)) || (!(sizeof(s))) ) 
        {
            Length= -1;
        }
        else
        {
            gets((char *)(&s));
            Length=((long int)strlen((char *)(&s))+1);
        }
    
        for (i=0; i<Length; i++)
        {
            if (s[i] > ' ')
            {
                return (s[i]);
            }
        }
    }
    return 0;
}

static void QueryTrace (void)
{
    /* Print I2C trace buffer, if user enters 'y' */
    U8 c;

    STI2C_Print(("Print trace?\n"));
    while (1)
    {
        c = GetNextChar();
        if ((I2cPrintTrace (-1, (c=='y') ? TRACE_PAGE_SIZE : 0) == 0) ||
             (c != 'y'))
        {
            break;
        }
        STI2C_Print(("More?\n"));
    }
}

#else /* not STI2C_TRACE */

/* Dummy functions to satisfy references */
static void QueryTrace (void) {}

#endif /* STI2C_TRACE */
#ifdef DURATION
static void PrintDuration (clock_t Start, clock_t Finish)
{
    STI2C_Print(("Start: %u  Finish: %u  Duration = %u (%u sec)\n",
              Start, Finish, Finish-Start, (Finish-Start)/ClocksPerSec));
}
#endif
#endif /* STI2C_TRACE */


/* Debug Functions ----------------------------------------------------------------*/

#if defined(STI2C_DEBUG)
void cleartrace(void)
{
    TraceBufferPos = 0;
    memset(TraceBuffer, 0, TraceBufferPos);
}

void dumptrace(char *filename)
{
    char myfilename[12];
    long int file;
    static int counter = 0;

    if (filename == NULL)
    {
        sprintf(myfilename, "trace%02i.dat", counter++);
        file = debugopen(myfilename, "wb");
    }
    else
    {
        file = debugopen(filename, "wb");
    }

    if (file != -1)
    {
        /* debugwrite returns # of bytes written */
        if (debugwrite(file, TraceBuffer, TraceBufferPos) == TraceBufferPos)
            cleartrace();
    }
    debugclose(file);
}

static void print(char *string)
{
    if (DoOutput == TRUE)
    {
        if (InInterrupt == FALSE)
        {
            STI2C_Print(("%s ", string));
        }
        else
        {
            if ((TraceBufferPos + strlen(string)) < (TRACEBUFSIZE))
            {
                memcpy(&TraceBuffer[TraceBufferPos], string, strlen(string));
                TraceBufferPos += strlen(string);
            }
        }
    }
}
#endif

#if defined(STI2C_DEBUG)
char *i2cbits(U16 ivalue)
{
    U16 mask = 32768;

    while (mask > 0)
    {
        U16 value = (mask & ivalue);
        mask >>= 1;
        if (value > 0)
        {
            switch (value)
            {
                case SSC_I2C_ENABLE:
                    print( "SSC_I2C_ENABLE ");
                    break;
                case SSC_I2C_GENERATE_START:
                    print( "SSC_I2C_GENERATE_START ");
                    break;
                case SSC_I2C_GENERATE_STOP:
                    print( "SSC_I2C_GENERATE_STOP ");
                    break;
                case SSC_I2C_GENERATE_ACKS:
                    print( "SSC_I2C_GENERATE_ACKS ");
                   break;
                case SSC_I2C_10_BIT_ADDRESS:
                    print( "SSC_I2C_10_BIT_ADDRESS ");
                    break;
                case SSC_I2C_TXENABLED:
                    print( "SSC_I2C_TXENABLED ");
                    break;
                case SSC_I2C_REPSTRTG:
                    print( "SSC_I2C_REPSTRTG "); break;
                case SSC_I2C_FASTMODE:
                    print( "SSC_I2C_FASTMODE "); break;
                default:
                    print( "Unknown value ");
                    break;
            }
        }
    }
    return NULL;
}

char *conbits(U16 ivalue)
{
    U16 mask = 32768;

    while (mask > 0)
    {
        U16 value = (mask & ivalue);
        mask >>= 1;
        if (value > 0)
        {
            switch (value)
            {
                case SSC_CON_DATA_WIDTH_7:
                    print( "SSC_CON_DATA_WIDTH_7 ");
                    break;
                case SSC_CON_DATA_WIDTH_8:
                    print( "SSC_CON_DATA_WIDTH_8 ");
                    break;
                case SSC_CON_DATA_WIDTH_9:
                    print( "SSC_CON_DATA_WIDTH_9 ");
                    break;
                case SSC_CON_HEAD_CONTROL:
                    print( "SSC_CON_HEAD_CONTROL ");
                    break;
                case SSC_CON_CLOCK_PHASE:
                    print( "SSC_CON_CLOCK_PHASE ");
                    break;
                case SSC_CON_CLOCK_POLARITY:
                    print( "SSC_CON_CLOCK_POLARITY ");
                    break;
                case SSC_CON_RESET:
                    print( "SSC_CON_RESET ");
                    break;
                case SSC_CON_MASTER_SELECT:
                    print( "SSC_CON_MASTER_SELECT ");
                    break;
                case SSC_CON_ENABLE:
                    print( "SSC_CON_ENABLE ");
                    break;
                case SSC_CON_LOOP_BACK:
                    print( "SSC_CON_LOOP_BACK ");
                    break;
                case SSC_CON_ENB_TX_FIFO:
                    print( "SSC_CON_ENB_TX_FIFO ");
                    break;
                case SSC_CON_ENB_RX_FIFO:
                    print( "SSC_CON_ENB_RX_FIFO ");
                    break;
                case SSC_CON_ENB_CLST_RX:
                    print( "SSC_CON_ENB_CLST_RX ");
                    break;
                default:
                    print( "Unknown value ");
                    break;
            }
        }
    }
    return NULL;
}

char *statbits(U16 ivalue)
{
    U16 mask = 32768;

    while (mask > 0)
    {
        U16 value = (mask & ivalue);
        mask >>= 1;
        if (value > 0)
        {
            switch (value)
            {
                case SSC_STAT_RX_BUFFER_FULL:
                    print( "SSC_STAT_RX_BUFFER_FULL ");
                    break;
                case SSC_STAT_TX_BUFFER_EMPTY:
                    print( "SSC_STAT_TX_BUFFER_EMPTY ");
                    break;
                case SSC_STAT_TX_ERROR:
                    print( "SSC_STAT_TX_ERROR ");
                    break;
                case SSC_STAT_RX_ERROR:
                    print( "SSC_STAT_RX_ERROR ");
                    break;
                case SSC_STAT_PHASE_ERROR:
                    print( "SSC_STAT_PHASE_ERROR ");
                    break;
                case SSC_STAT_STRETCH:
                    print( "SSC_STAT_STRETCH ");
                    break;
                case SSC_STAT_AAS:
                    print( "SSC_STAT_AAS ");
                    break;
                case SSC_STAT_STOP:
                    print( "SSC_STAT_STOP ");
                    break;
                case SSC_STAT_ARBL:
                    print( "SSC_STAT_ARBL ");
                    break;
                case SSC_STAT_BUSY:
                    print( "SSC_STAT_BUSY ");
                    break;
                case SSC_STAT_NACK:
                    print( "SSC_STAT_NACK ");
                    break;
                case SSC_STAT_REPSTRT:
                    print( "SSC_STAT_REPSTRT ");
                    break;
                case SSC_STAT_TX_HALF_EMPTY:
                    print( "SSC_STAT_TX_HALF_EMPTY ");
                case SSC_STAT_TX_FULL:
                    print( "SSC_STAT_TX_FULL ");
                case SSC_STAT_RX_HALF_FULL:
                    print( "SSC_STAT_RX_HALF_FULL ");
                default:
                    print( "Unknown value ");
                    break;
            }
        }
    }
    return NULL;
}

char *clrbits(U16 ivalue)
{
    U16 mask = 32768;

    while (mask > 0)
    {
        U16 value = (mask & ivalue);
        mask >>= 1;
        if (value > 0)
        {
            switch (value)
            {
                case SSC_CLR_AAS:
                    print( "SSC_CLR_AAS ");
                    break;
                case SSC_CLR_STOP:
                    print( "SSC_CLR_STOP ");
                    break;
                case SSC_CLR_ARBL:
                    print( "SSC_CLR_ARBL ");
                    break;
                case SSC_CLR_NACK:
                    print( "SSC_CLR_NACK ");
                    break;
                case SSC_CLR_REPSTRT:
                    print( "SSC_CLR_REPSTRT ");
                    break;
                default:
                    print( "Unknown value ");
                    break;
            }
        }
    }
    return NULL;
}

char *ienbits(U16 ivalue)
{
    U16 mask = 32768;

    while (mask > 0)
    {
        U16 value = (mask & ivalue);
        mask >>= 1;
        if (value > 0)
        {
            switch (value)
            {
                case SSC_IEN_RX_BUFFER_FULL:
                    print( "SSC_IEN_RX_BUFFER_FULL ");
                    break;
                case SSC_IEN_TX_BUFFER_EMPTY:
                    print( "SSC_IEN_TX_BUFFER_EMPTY ");
                    break;
                case SSC_IEN_RX_ERROR:
                    print( "SSC_IEN_RX_ERROR ");
                    break;
                case SSC_IEN_PHASE_ERROR:
                    print( "SSC_IEN_PHASE_ERROR ");
                    break;
                case SSC_IEN_AAS:
                    print( "SSC_IEN_AAS ");
                    break;
                case SSC_IEN_STOP:
                    print( "SSC_IEN_STOP ");
                    break;
                case SSC_IEN_ARBL:
                    print( "SSC_IEN_ARBL ");
                    break;
                case SSC_IEN_NACK:
                    print( "SSC_IEN_NACK ");
                    break;
                case SSC_IEN_REPSTRT:
                    print( "SSC_IEN_REPSTRT ");
                    break;
                case SSC_IEN_TX_HALF_EMPTY:
                    print( "SSC_IEN_TX_HALF_EMPTY ");
                case SSC_IEN_TX_FULL:
                    print( "SSC_IEN_TX_FULL ");
                case SSC_IEN_RX_HALF_FULL:
                    print( "SSC_IEN_RX_HALF_FULL ");
                default:
                    print( "Unknown value ");
                    break;
            }
        }
    }
    return NULL;
}

char *regname(U16 value)
{
    switch (value)
    {
        case SSC_BAUD_RATE:
            return "SSC_BAUD_RATE";
        case SSC_TX_BUFFER:
            return "SSC_TX_BUFFER";
        case SSC_RX_BUFFER:
            return "SSC_RX_BUFFER";
        case SSC_CONTROL:
            return "SSC_CONTROL";
        case SSC_INT_ENABLE:
            return "SSC_INT_ENABLE";
        case SSC_STATUS:
            return "SSC_STATUS";
        case SSC_I2C:
            return "SSC_I2C";
        case SSC_SLAD:
            return "SSC_SLAD";
        case SSC_CLR:
            return "SSC_CLR";
        case SSC_GLITCH_WIDTH:
            return "SSC_GLITCH_WIDTH";
        case SSC_PRE_SCALER:
            return "SSC_PRE_SCALER";
        case SSC_GLITCH_WIDTH_DATAOUT:
            return "SSC_GLITCH_WIDTH_DATAOUT";
        case SSC_PRE_SCALER_DATAOUT:
            return "SSC_PRE_SCALER_DATAOUT";
        default:
            return "Unknown (WTF?)";
    }

    return NULL;
}
#endif

#ifdef ST_OSWINCE
static void WriteReg(DU32 *I2cRegPtr, U16 Index, U16 Value)
#else
static __inline void WriteReg(DU32 *I2cRegPtr, U16 Index, U16 Value)
#endif
{
#if defined(STI2C_DEBUG)
    char tmp[30];

    if (DoOutput == TRUE)
    {
        if (InInterrupt == FALSE)
        {
            STI2C_Print(("Writing %04x to register %s (%08x)\n",
                        Value, regname(Index),
                        (U32)&I2cRegPtr[Index]));
            STI2C_Print(("\t"));
            switch (Index)
            {
                case SSC_CONTROL:
                    conbits(Value);
                    break;
                case SSC_INT_ENABLE:
                    ienbits(Value);
                    break;
                case SSC_STATUS:
                    statbits(Value);
                    break;
                case SSC_I2C:
                    i2cbits(Value);
                    break;
                case SSC_CLR:
                    clrbits(Value);
                    break;
                case SSC_TX_BUFFER:
                    STI2C_Print(("%i ", Value));
                    break;
            }
            STI2C_Print(("\n"));
        }
        else
        {
            sprintf(tmp, "%08x  ", (U32)I2cRegPtr);
            print(tmp);
            print("Write: ");
            print(regname(Index));
            print("\n");
            if (Value != 0)
            {
                switch (Index)
                {
                    case SSC_CONTROL:
                        conbits(Value);
                        break;
                    case SSC_INT_ENABLE:
                        ienbits(Value);
                        break;
                    case SSC_STATUS:
                        statbits(Value);
                        break;
                    case SSC_I2C:
                        i2cbits(Value);
                        break;
                    case SSC_CLR:
                        clrbits(Value);
                        break;
                    case SSC_SLAD:
                        sprintf(tmp, "%x", Value);
                        print(tmp);
                        break;
                    case SSC_TX_BUFFER:
                        sprintf(tmp, "%i", Value);
                        print(tmp);
                        break;
                }
            }
            print("\n");
        }
    }
#endif

    I2cRegPtr[Index] = Value;
}

#ifdef ST_OSWINCE
static U32 ReadReg(DU32 *I2cRegPtr, U16 Index)
#else
static __inline U32 ReadReg(DU32 *I2cRegPtr, U16 Index)
#endif
{
    U32 Value;
#if defined(STI2C_DEBUG)
    char tmp[6];
#endif

    Value = I2cRegPtr[Index];

#if defined(STI2C_DEBUG)
    if (DoOutput == TRUE)
    {
        if (InInterrupt == FALSE)
        {
            STI2C_Print(("Read %04x from register %s\n", Value, regname(Index)));
            STI2C_Print(("\t"));
            switch (Index)
            {
                case SSC_CONTROL:
                    conbits(Value);
                    break;
                case SSC_INT_ENABLE:
                    ienbits(Value);
                    break;
                case SSC_STATUS:
                    statbits(Value);
                    break;
                case SSC_I2C:
                    i2cbits(Value);
                    break;
                case SSC_CLR:
                    clrbits(Value);
                    break;
                case SSC_RX_BUFFER:
                    STI2C_Print(("%i ", Value));
                    break;
            }
            STI2C_Print(("\n"));
        }
        else
        {

            print("Read: ");
            print(regname(Index));
            print("\n");
            switch (Index)
            {
                case SSC_CONTROL:
                    conbits(Value);
                    break;
                case SSC_INT_ENABLE:
                    ienbits(Value);
                    break;
                case SSC_STATUS:
                    statbits(Value);
                    break;
                case SSC_I2C:
                    i2cbits(Value);
                    break;
                case SSC_CLR:
                    clrbits(Value);
                    break;
                case SSC_RX_BUFFER:
                    sprintf(tmp, "%i", Value);
                    print(tmp);
                    break;
            }
            print("\n");
        }
    }
#endif

    return Value;
}
#if defined(ST_8010) || defined(ST_5525)
/******************************************************************************
Name        : sti2c_Setup
Description :

Returns     :

******************************************************************************/
void sti2c_Setup( void )
{
#ifdef ST_8010
   /* configure SSC0 SCL/SDA pins */
   *(volatile U32*)(CFG_BASE_ADDRESS + 0x10 /*0x50003010*/) |= 0x07000000;
   *(volatile U32*)(CFG_BASE_ADDRESS + 0x14 /*0x50003014*/) |= 0xe0000;

   /* configure SSC1 SCL/SDA pins */
   *(volatile U32*)(EMI_BASE_ADDRESS + 0x28 /*0x47000028*/) |= 0x7f3c00;
#else
    /* configure SSC0 SCL/SDA pins */
    /* configure SSC1 SCL/SDA pins */
   *(volatile U32*)(0x19800024) |= 0x154;

#endif
}
#endif
/******************************************************************************
Name        : STI2C_GetRevision
Description : Returns a pointer to the string containing the version number of
              this code.
Parameters  : None
******************************************************************************/

ST_Revision_t STI2C_GetRevision (void)
{
    return g_Revision;
}

/******************************************************************************
Name        : I2cStop
Description : Generates an I2c stop condition on the bus through the I2c
              hardware.
Parameters  : Pointer to the I2C Device Control block.
******************************************************************************/

static void I2cStop (i2c_param_block_t *Params_p)
{
    DU32   *I2cRegPtr  = (DU32 *) Params_p->InitParams.BaseAddress;
    U32    I2CValue = 0;
    U32    Ctr;

    I2CValue = ReadReg(I2cRegPtr, SSC_I2C);
    I2CValue |= SSC_I2C_GENERATE_STOP;

    /*Disable the ACK generation but do't disable the xmitter
    as per SSC recomendation*/
    I2CValue &= ~SSC_I2C_GENERATE_ACKS;

    /* Tell the I2C control register to generate a a stop condition */
    WriteReg(I2cRegPtr, SSC_I2C, I2CValue);

    /* Wait for busy period to finish, then return SSC to default state.
       This is needed due to the 4.7us 'setup time' for STOP; there is also
       a ~4.7us time required *between* transfers. */
    Ctr = 0;
    while( (ReadReg(I2cRegPtr, SSC_STATUS) & SSC_STAT_BUSY) &&
           (Ctr < STOP_BUSY_WAIT) )
    {
        /* Timeout for safety */
        Ctr++;
        /*extra i2c delay (STOS_TaskDelay(10);)here was increasing the tuner/demod locking time
          Please see DDTS GNBvd31258 for more details.
        */
    }
    if (Ctr == STOP_BUSY_WAIT)
    {
        STI2C_Print(("Bus 0x%08x timed out waiting for busy; status: 0x%04x\n",
                (U32)I2cRegPtr, I2cRegPtr[SSC_STATUS] & 0xfff));
#if defined(STI2C_TRACE)
        QueryTrace();
#endif
    }
    /*This is the delay provided for Slave state handle
      the Bus Condition of SLAVE to clear the busy bit
     */
    STOS_TaskDelay(1);
    WriteReg(I2cRegPtr, SSC_TX_BUFFER, 0x1ff); /* disable clock stretch */

    WriteReg(I2cRegPtr, SSC_I2C, DEFAULT_I2C); /* Enable acks again */

}
/******************************************************************************
Name        : IsAlreadyInitialised
Description : Performs some tests prior to initialising a new control block
Parameters  : 1) A pointer to the name of the device about to be initalised
              2) A pointer to the parameter block of the device to be
                 initialised
******************************************************************************/

static BOOL IsAlreadyInitialised( const ST_DeviceName_t      Name,
                                  const STI2C_InitParams_t   *Params_p )
{
    BOOL  Match = FALSE;
    U32   CtrBlkIndex=0;

    while ((CtrBlkIndex < NUM_SSC) && (Match == FALSE))
    {
       if ((strcmp(CtrlBlockArray[CtrBlkIndex].Name, Name)) == 0 ||
           (CtrlBlockArray[CtrBlkIndex].InitParams.BaseAddress == Params_p->BaseAddress) ||
           (CtrlBlockArray[CtrBlkIndex].InitParams.InterruptNumber == Params_p->InterruptNumber )
          )
        {
            Match = TRUE;
        }
        else
        {
            CtrBlkIndex++;
        }
    }
     return( Match );
}


/******************************************************************************
Name        : GetPioPin
Description : Takes a PIO bitmask for one PIO pin and resolves it to a pin
              number between zero and seven. Returns UCHAR_MAX on error.
Parameters  : A bitmask for a PIO pin
******************************************************************************/

static S32 GetPioPin( U8 Value )
{
    S32 i = 0;

    while ( (Value != 1) && (i < 8) )
    {
        i++;
        Value >>= 1;
    }
    if ( i > 7 )
    {
        i = UCHAR_MAX;
    }
    return( i );
}


#ifndef STI2C_MASTER_ONLY
/******************************************************************************
Name        : RegisterEvents
Description : Called from STI2C_Init() when driver has to operate in slave
              mode. Opens the specified Event Handler & registers the
              events which can be notified in slave mode.
Parameters  :
******************************************************************************/

static ST_ErrorCode_t RegisterEvents (const ST_DeviceName_t  EvtHandlerName,
                                      i2c_param_block_t      *InitBlock_p)
{
    STEVT_OpenParams_t   EVTOpenParams;

    /* Keeps lint quiet. */
    EVTOpenParams.dummy = 0;

    /* First open the specified event handler */
    if (STEVT_Open (EvtHandlerName, &EVTOpenParams,
                    &InitBlock_p->EVTHandle) != ST_NO_ERROR)
    {
        return STI2C_ERROR_EVT_REGISTER;
    }

    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                        STI2C_TRANSMIT_NOTIFY_EVT,
                        &InitBlock_p->TxNotifyId) != ST_NO_ERROR)
    {
        return STI2C_ERROR_EVT_REGISTER;
    }

    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                        STI2C_RECEIVE_NOTIFY_EVT,
                        &InitBlock_p->RxNotifyId) != ST_NO_ERROR)
    {
        return STI2C_ERROR_EVT_REGISTER;
    }

    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                        STI2C_TRANSMIT_BYTE_EVT,
                        &InitBlock_p->TxByteId) != ST_NO_ERROR)
    {
        return STI2C_ERROR_EVT_REGISTER;
    }
    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                        STI2C_RECEIVE_BYTE_EVT,
                        &InitBlock_p->RxByteId) != ST_NO_ERROR)
    {
        return STI2C_ERROR_EVT_REGISTER;
    }
    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                        STI2C_TRANSMIT_COMPLETE_EVT,
                        &InitBlock_p->TxCompleteId) != ST_NO_ERROR)
    {
        return STI2C_ERROR_EVT_REGISTER;
    }
    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                        STI2C_RECEIVE_COMPLETE_EVT,
                        &InitBlock_p->RxCompleteId) != ST_NO_ERROR)
    {
        return STI2C_ERROR_EVT_REGISTER;
    }
    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                        STI2C_BUS_STUCK_EVT,
                        &InitBlock_p->BusStuckId) != ST_NO_ERROR)
    {
        return STI2C_ERROR_EVT_REGISTER;
    }
    return ST_NO_ERROR;
}
#endif /* STI2C_MASTER_ONLY */


static void I2cReset(i2c_param_block_t *Params_p)
{
    DU32   *I2cRegPtr  = (DU32 *) Params_p->InitParams.BaseAddress;

    /* Generate stop condition to reset bus */
    I2cRegPtr[SSC_CONTROL] = DEFAULT_CONTROL | SSC_CON_RESET;
    I2cRegPtr[SSC_CONTROL] = DEFAULT_CONTROL;
}

/* Interrupt handler in which actual transfer take Place */
STOS_INTERRUPT_DECLARE(I2cHandler, Param)
{
    BOOL JustDoneRepeatedStart = FALSE;
    i2c_param_block_t   *CtrlBlk_p  = Param;
    DU32                *I2cRegPtr  = (DU32*) CtrlBlk_p->InitParams.BaseAddress;
    U16                  IntEnables = CtrlBlk_p->InterruptMask;
    U16                  Status,StatusErrors;
    U16                  RxData,NumberToRead,LoopCount;
    U32                  TxData,I2cValues,Loop,BytesLeft;
#ifndef STI2C_MASTER_ONLY
    U32                  Temp;
#endif
#if defined(STI2C_DEBUG)
    char tmp[50];

    InInterrupt = TRUE;
    print("Interrupt start\n");

    sprintf(tmp, "%08x %i\n", (U32)CtrlBlk_p->InitParams.BaseAddress, STOS_time_now());
    print(tmp);
#endif
    I2C_TRACE_INT_INC;

    /* Turn off interrupts while in ISR; IntEnables should be in sync
     * with the register contents already.
     */
    I2cRegPtr[SSC_INT_ENABLE] = 0;

    /* Get h/w status for status bit */
    Status = I2cRegPtr[SSC_STATUS];
    /* Mask off undefined bits (changes for each SSC version) */	
    Status &= SSC_STAT_MASK;
    I2C_TRACE(CtrlBlk_p,0xA00,Status);

    /* clear the bits that were read as being set. */
    I2cRegPtr[SSC_CLR] = Status & SSC_CLR_MASK;
    /*
    check if FIFO is enabled if enabled then R/W up to 8 bytes from FIFO
    else R/W only 1 byte
    */
    if (CtrlBlk_p->InitParams.FifoEnabled)
    {
        LoopCount = 8;
    }
    else
    {
        LoopCount = 1;
    }
#if defined(STI2C_DEBUG)
    sprintf(tmp, "%08x 0x%03x\n", (U32)CtrlBlk_p->InitParams.BaseAddress, Status & 0xfff);
    print(tmp);
    sprintf(tmp, "%08x 0x%03x\n", (U32)CtrlBlk_p->InitParams.BaseAddress, I2cRegPtr[SSC_I2C]);
    print(tmp);
#endif
    /* check for stop condition*/

    if (Status & SSC_STAT_STOP)
    {
        WriteReg(I2cRegPtr, SSC_CLR, SSC_CLR_STOP);

#ifndef STI2C_MASTER_ONLY
        if (SLAVE_STATE(CtrlBlk_p))
        {
            if (CtrlBlk_p->State ==STI2C_SLAVE_RECEIVE)
            {
                /* receive remaining bytes from master*/
                NumberToRead = I2cRegPtr[SSC_RX_FSTAT];
                for (Loop=0; Loop<NumberToRead; Loop++)
                {
                    /* Read a byte, so notify the user */
                    I2C_TRACE(CtrlBlk_p,0x40,RxData);
                    RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x01ff;
                    RxData >>= 1;
                    STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->RxByteId, &RxData);
                }
                STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->RxCompleteId, NULL);
            }

            if (CtrlBlk_p->State ==STI2C_SLAVE_TRANSMIT)
            {

                if (Status & SSC_STAT_NACK)
                {
                    I2cRegPtr[SSC_I2C] = (I2cRegPtr[SSC_I2C] & ~SSC_I2C_TXENABLED);
                }
                I2C_TRACE(CtrlBlk_p,0x33,0);
                /* In case of 5514 if Slave has received STOP,
                    so ensure buffer is left in  'high' state */
#if defined(ST_5514)
                I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;
#endif
                STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxCompleteId, NULL);
            }
            /* transfer is completed so disable the FIFO*/
            if (CtrlBlk_p->InitParams.FifoEnabled)
            {
                I2cRegPtr[SSC_CONTROL] &= ~SSC_CON_ENB_TX_FIFO;
                I2cRegPtr[SSC_CONTROL] &= ~SSC_CON_ENB_RX_FIFO;
            }
            CtrlBlk_p->State = STI2C_IDLE;
            IntEnables = DEFAULT_ENABLES;
            WriteReg(I2cRegPtr, SSC_TX_BUFFER, 0x1ff); /* disable clock stretch */
            WriteReg(I2cRegPtr, SSC_I2C, DEFAULT_I2C); /* Enable acks again */

            /* Reenable interrupts */
            I2cRegPtr[SSC_INT_ENABLE] = IntEnables;
            CtrlBlk_p->InterruptMask = IntEnables;
            I2C_TRACE(CtrlBlk_p,0xFFFF,IntEnables);
        }
        else
        {
            IntEnables = DEFAULT_ENABLES;
            I2cRegPtr[SSC_INT_ENABLE] = IntEnables; /*DRSDEBUG this is added to fix ack issue*/
            CtrlBlk_p->SlaveStatus = STI2C_IDLE; 
        }
#endif
        STOS_INTERRUPT_EXIT(STOS_SUCCESS);
    }

#ifndef STI2C_MASTER_ONLY
    /*Check for repeat start when in Slave mode (i.e. other master has
    started another transfer to us without doing a stop. If so, switch to
    Idle state so it gets treated like an ordinary AAS event.
    */

    if ((Status & SSC_STAT_AAS) && (SLAVE_STATE(CtrlBlk_p)))
    {
        I2C_TRACE(CtrlBlk_p,0x01,0);
        CtrlBlk_p->State = STI2C_IDLE;
    }

#endif /* STI2C_MASTER_ONLY */

    /* Check for Arbitration loss if yes go to ideal condn*/
    if (Status & SSC_STAT_ARBL)
    {
        CtrlBlk_p->State = STI2C_IDLE;
        CtrlBlk_p->HandlerCondition = STI2C_ERROR_BUS_IN_USE;
        /* Clear the BRG so it doesn't interfere with master.  */
        WriteReg(I2cRegPtr, SSC_BAUD_RATE, 0);
        I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;
        I2cReset(CtrlBlk_p);
        I2C_TRACE(CtrlBlk_p,0xF02,0);
        STOS_SemaphoreSignal( CtrlBlk_p->IOSemaphore_p );
    }


    if (Status & SSC_STAT_AAS)
    {
        /* check we should be in idle state*/
        if (CtrlBlk_p->State != STI2C_IDLE)
        {
#if defined(STI2C_DEBUG)
            debugbreak();
#endif
        }
    }

    /* check for phase error Currently this check has been skipped as we get this error during normal transfer*/
    /*StatusErrors= SSC_STAT_PHASE_ERROR;*/
    StatusErrors=0;
    if (MASTER_STATE(CtrlBlk_p)&& (Status & StatusErrors))
    {
        CtrlBlk_p->State = STI2C_IDLE;
        CtrlBlk_p->HandlerCondition = STI2C_ERROR_STATUS;
        /* Clear the BRG so it doesn't interfere with master.  */
        WriteReg(I2cRegPtr, SSC_BAUD_RATE, 0);
        I2C_TRACE(CtrlBlk_p,0xF04,0);
        IntEnables &= ~SSC_IEN_TX_ERROR;
        STOS_SemaphoreSignal( CtrlBlk_p->IOSemaphore_p );
    }
    /*check for  repeated start condition*/
    if(Status & SSC_STAT_REPSTRT)
    {
#ifndef STI2C_MASTER_ONLY
        if (SLAVE_STATE(CtrlBlk_p))
        {
            if(CtrlBlk_p->State ==STI2C_SLAVE_RECEIVE)
            {
                NumberToRead = I2cRegPtr[SSC_RX_FSTAT];
                for (Loop=0; Loop< NumberToRead; Loop++)
                {
                    /* Read a byte, so notify the user */
                    I2C_TRACE(CtrlBlk_p,0x40,RxData);
                    RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x01ff;
                    RxData >>= 1;
                    STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->RxByteId, &RxData);
                }
                /*Master wants the new transfer so notify user to complete current transfer*/
                STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->RxCompleteId, NULL);
            }
            /* transfer is completed so disable the FIFO*/
            if (CtrlBlk_p->InitParams.FifoEnabled)
            {
                I2cRegPtr[SSC_CONTROL] &= ~SSC_CON_ENB_TX_FIFO;
                I2cRegPtr[SSC_CONTROL] &= ~SSC_CON_ENB_RX_FIFO;
            }
            CtrlBlk_p->State = STI2C_IDLE;
            IntEnables       = DEFAULT_ENABLES;
            WriteReg(I2cRegPtr, SSC_I2C, DEFAULT_I2C);
            WriteReg(I2cRegPtr, SSC_TX_BUFFER, 0x1ff);


            /* Reenable interrupts */
            I2cRegPtr[SSC_INT_ENABLE] = IntEnables;
            CtrlBlk_p->InterruptMask  = IntEnables;
            I2C_TRACE(CtrlBlk_p,0xFFFF,IntEnables);
        }
        else if(CtrlBlk_p->State== STI2C_IDLE)
        {
            IntEnables       = DEFAULT_ENABLES;
            I2cRegPtr[SSC_INT_ENABLE] = IntEnables;
            CtrlBlk_p->InterruptMask  = IntEnables;
        }
        else
#endif
        {
            I2cRegPtr[SSC_TX_BUFFER] = CtrlBlk_p->DataToSend;

            I2C_TRACE(CtrlBlk_p,0xF06,CtrlBlk_p->DataToSend);
            IntEnables |= SSC_IEN_TX_ERROR;
            IntEnables  &= ~SSC_IEN_REPSTRT;
            CtrlBlk_p->InterruptMask=IntEnables;
            /* Ensure transmit bit is in i2c */
            I2cRegPtr[SSC_I2C] &= ~SSC_I2C_REPSTRTG;
            I2cRegPtr[SSC_I2C] |= SSC_I2C_TXENABLED;
            JustDoneRepeatedStart = TRUE;
        }
    }

    switch (CtrlBlk_p->State)
    {
        case STI2C_MASTER_ADDRESSING_WRITE:

            if (!JustDoneRepeatedStart)
            {
                if (Status & SSC_STAT_TX_ERROR)
                {
                    if (Status & SSC_STAT_NACK)
                    {

#if defined(STI2C_DEBUG)
                        print("Invalid ack\n");
#endif
                        /* Invalid ack so raise an error and terminate transfer */
                        CtrlBlk_p->HandlerCondition  = STI2C_ERROR_ADDRESS_ACK;
                        RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x1ff;
                        /*move to ideal state*/
                        CtrlBlk_p->State             = STI2C_IDLE;
                        IntEnables &= ~SSC_IEN_TX_ERROR;
                        I2C_TRACE_ERR_INC;
                        I2C_TRACE(CtrlBlk_p,0xF50,RxData);
                        STOS_SemaphoreSignal( CtrlBlk_p->IOSemaphore_p );
                    }
                    else   /* good ack */
                    {
#if defined(STI2C_DEBUG)
                        print("Started writing\n");
#endif

                        /* Set state to Writing & put 1st char into tx buffer */

                        CtrlBlk_p->State      = STI2C_MASTER_WRITING;
                        /* enable the FIFO if required*/
                        if (CtrlBlk_p->InitParams.FifoEnabled)
                        {
                            I2cRegPtr[SSC_CONTROL] |= SSC_CON_ENB_TX_FIFO;
                            I2cRegPtr[SSC_CONTROL] |= SSC_CON_ENB_RX_FIFO;
                        }
                        for(Loop=0; Loop<LoopCount && (CtrlBlk_p->BufferCnt < CtrlBlk_p->BufferLen); Loop++)
                        {
                            /* Read the rxdata just to clear the Rx buffer*/
                            RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x1ff;
                            TxData = ((*(CtrlBlk_p->Buffer_p++)) << 1) | ACKNOWLEDGE_BIT;
                            I2cRegPtr[SSC_TX_BUFFER] = (U16)TxData;
                            CtrlBlk_p->BufferCnt++;
                            I2C_TRACE(CtrlBlk_p,0x50,TxData);
                        }
                    }
                }
            }
            break;

        case STI2C_MASTER_WRITING:

            if (Status & SSC_STAT_TX_ERROR)
            {
                if (Status & SSC_STAT_NACK)
                {
#if defined(STI2C_DEBUG)
                   print("Bad ack (master writing)\n");
#endif
                    RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x1ff;
                    /* Invalid ack so raise an error and terminate transfer */
                    CtrlBlk_p->State             = STI2C_IDLE;
                    CtrlBlk_p->HandlerCondition  = STI2C_ERROR_WRITE_ACK;
                    IntEnables &= ~SSC_IEN_TX_ERROR;
                    I2C_TRACE_ERR_INC;
                    I2C_TRACE(CtrlBlk_p,0xF60,RxData);
                    STOS_SemaphoreSignal( CtrlBlk_p->IOSemaphore_p );
                }
                else   /* good ack */
                {
                    /* If the last byte has been transmitted then tidy up,
                        else buffer the next byte
                     */
                    if ( CtrlBlk_p->BufferCnt >= CtrlBlk_p->BufferLen )
                    {
#if defined(STI2C_DEBUG)
                        print("Finished writing\n");
#endif
                        RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x1ff;
                        CtrlBlk_p->State = STI2C_IDLE;
                        I2C_TRACE(CtrlBlk_p,0x60,RxData);
                        CtrlBlk_p->HandlerCondition  = ST_NO_ERROR;
                        /* transfer is completed so disable the FIFO*/
                        if (CtrlBlk_p->InitParams.FifoEnabled)
                        {
                            I2cRegPtr[SSC_CONTROL] &= ~SSC_CON_ENB_TX_FIFO;
                            I2cRegPtr[SSC_CONTROL] &= ~SSC_CON_ENB_RX_FIFO;
                        }
                        IntEnables &= ~SSC_IEN_TX_ERROR;
                        STOS_SemaphoreSignal( CtrlBlk_p->IOSemaphore_p );
                    }
                    else
                    {

                        for(Loop=0; Loop<LoopCount && (CtrlBlk_p->BufferCnt < CtrlBlk_p->BufferLen); Loop++)
                        {
#if defined(STI2C_DEBUG)
                            print("Placed next byte\n");
#endif
                            RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x1ff;
                            /* Put the next byte into the buffer byte and transmit */
                            TxData = ((*(CtrlBlk_p->Buffer_p++)) << 1) | ACKNOWLEDGE_BIT;

                            /* This work around is needed ONLY WHEN the demodulator is accessed.*/
#if defined (STI2C_DEMOD_ACCESSED)
                            if((Status & SSC_STAT_STRETCH)&& ((U16)TxData & 0x100))
                            {
                                DU32 *PioReg = CtrlBlk_p->BaseAddressforSDA;
                                STPIO_Set( CtrlBlk_p->PIOHandle,CtrlBlk_p->PIOforSDA.BitMask);
                                I2cRegPtr[SSC_TX_BUFFER] = (U16)TxData;

                                while ((ReadReg(I2cRegPtr, SSC_STATUS) & SSC_STAT_TX_BUFFER_EMPTY) == 0)
                                {
                                    /* Do nothing */
                                }
                                STPIO_Clear(CtrlBlk_p->PIOHandle,CtrlBlk_p->PIOforSDA.BitMask);
                            }
                            else
                            {
                                I2cRegPtr[SSC_TX_BUFFER] = (U16)TxData;
                            }
#else
                            I2cRegPtr[SSC_TX_BUFFER] = (U16)TxData;
#endif
                            CtrlBlk_p->BufferCnt++;
                            I2C_TRACE(CtrlBlk_p,0x61,TxData);
                        }
                    }
                }
            }
            break;

        case STI2C_MASTER_ADDRESSING_READ:

            if (!JustDoneRepeatedStart)
            {
                if(Status & SSC_STAT_TX_ERROR)
                {
                    if(Status & SSC_STAT_NACK)
                    {
#if defined(STI2C_DEBUG)
                        print("Invalid ack\n");
#endif
                        /* Invalid ack so raise an error and terminate transfer */
                        CtrlBlk_p->State             = STI2C_IDLE;
                        CtrlBlk_p->HandlerCondition  = STI2C_ERROR_ADDRESS_ACK;
                        I2C_TRACE_ERR_INC;
                        I2C_TRACE(CtrlBlk_p,0xF70,RxData);
                        IntEnables &= ~SSC_IEN_TX_ERROR;
                        STOS_SemaphoreSignal( CtrlBlk_p->IOSemaphore_p );
                    }
                    else
                    {
                        /* Read the rxdata just to clear the Rx buffer*/
                        RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x1ff;
                        /* good ack, so set state to Reading  */
                        if ((Status & SSC_STAT_REPSTRT) == 0)
                        {
                            /* A master-receiver will not ACK the last byte
                            of a transfer in order to stop the slave-transmitter
                            */
                            CtrlBlk_p->State = STI2C_MASTER_READING;
                            if (CtrlBlk_p->InitParams.FifoEnabled)
                            {
                                I2cRegPtr[SSC_CONTROL] |= SSC_CON_ENB_TX_FIFO;
                                I2cRegPtr[SSC_CONTROL] |= SSC_CON_ENB_RX_FIFO;
                            }
                            if ( CtrlBlk_p->BufferCnt+1 == CtrlBlk_p->BufferLen )
                            {
                                /* Disable transmitter */
                                I2cValues = I2cRegPtr[SSC_I2C] & 0xfff;
                                I2cValues= I2cValues & ~SSC_I2C_GENERATE_ACKS;
                                I2cValues= I2cValues &~SSC_I2C_TXENABLED;
                                WriteReg(I2cRegPtr, SSC_I2C, I2cValues);
                                /* Release the clock stretch*/
                                I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;
                                I2C_TRACE(CtrlBlk_p,0x70,RxData);
                            }
                            else   /* More than 1 byte to read, so generate acks */
                            {
                                I2cValues = I2cRegPtr[SSC_I2C] & 0xfff;
                                I2cValues |= SSC_I2C_GENERATE_ACKS;
                                I2cValues= I2cValues &~SSC_I2C_TXENABLED;
                                WriteReg(I2cRegPtr, SSC_I2C, I2cValues);
                                /* Release the clock stretch*/
                                /* Incase of FIFO enabled read only n-1 bytes from here
                                   for nth byte ack should be disabled*/
                                for(Loop=0;Loop<LoopCount && CtrlBlk_p->BufferCnt < CtrlBlk_p->BufferLen-1;Loop++)
                                {
                                    I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;
                                    I2C_TRACE(CtrlBlk_p,0x71,RxData);
                                }
                            }
                        }
                    }
                }
            }
            break;

        case STI2C_MASTER_READING:

            if (Status & SSC_STAT_TX_ERROR)   /* Have we got a char? */
            {
                if (!(Status & SSC_STAT_NACK))
                {
                    /* Incase of FIFO enabled read only n-1 bytes from here */
                    for(Loop=0; Loop<LoopCount && (CtrlBlk_p->BufferCnt < CtrlBlk_p->BufferLen-1); Loop++)
                    {
                        /* Buffer the received char & increment recvd char count */
                        RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x1ff;
                        *(CtrlBlk_p->Buffer_p++) = RxData >> 1;
                        CtrlBlk_p->BufferCnt++;
                    }
                }
                else
                {
                    /* read nth byte from here */
                    RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x1ff;
                    *(CtrlBlk_p->Buffer_p++) = RxData >> 1;
                    CtrlBlk_p->BufferCnt++;
                }

                /* If last byte has been read, clean up & don't acknowledge */
                if ( CtrlBlk_p->BufferCnt >= CtrlBlk_p->BufferLen )
                {
                    /* transfer is completed so disable the FIFO*/
                    if (CtrlBlk_p->InitParams.FifoEnabled)
                    {
                        I2cRegPtr[SSC_CONTROL] &= ~SSC_CON_ENB_TX_FIFO;
                        I2cRegPtr[SSC_CONTROL] &= ~SSC_CON_ENB_RX_FIFO;
                    }
                    CtrlBlk_p->State = STI2C_IDLE;
                    I2C_TRACE(CtrlBlk_p,0x80,RxData);
                    IntEnables &= ~SSC_IEN_TX_ERROR;
                    STOS_SemaphoreSignal( CtrlBlk_p->IOSemaphore_p );
                }
                else
                {
                    /* Turn off acks after penultimate char has been read */
                    if ( CtrlBlk_p->BufferCnt+1 == CtrlBlk_p->BufferLen )
                    {
                        I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;
                        I2cRegPtr[SSC_I2C]=SSC_I2C_ENABLE;
                        I2C_TRACE(CtrlBlk_p,0x81,RxData);
                    }
                    else
                    {
                        BytesLeft = CtrlBlk_p->BufferLen - CtrlBlk_p->BufferCnt;
                        /* Incase of FIFO enabled read only n-1 bytes from here
                           for nth byte ack should be disabled*/
                        for (Loop=0; Loop<LoopCount && Loop<BytesLeft-1; Loop++)
                        {
                            I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;
                            I2C_TRACE(CtrlBlk_p,0x82,RxData);
                        }
                    }
                }
            }
            break;

#ifndef STI2C_MASTER_ONLY

        case STI2C_IDLE:

            /*If Addressed As Slave, we need to go into Slave Mode.
            If we have received a char, this is the address, so the bottom
            bit indicates whether transfer is read or write.
            */
            if (Status & (SSC_STAT_AAS))
            {
                if ((Status & SSC_STAT_TX_ERROR) || (CtrlBlk_p->SlaveStatus != STI2C_IDLE))
                {
                    RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x01ff;
                    if ((RxData & 0x02)|| (CtrlBlk_p->SlaveStatus==STI2C_SLAVE_TRANSMIT))
                    {
                        /* R/W=1
                        Move to slave transmitter state,enable 'ack' bit*/
                        CtrlBlk_p->State = STI2C_SLAVE_TRANSMIT;
                        /*Notify user for start of transmission*/
                        STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxNotifyId,
                                          NULL);
                        /* Enable the transmitter */
                        Temp = I2cRegPtr[SSC_I2C] & 0xfff;
                        Temp |= SSC_I2C_TXENABLED;
                        I2cRegPtr[SSC_I2C] = Temp;
                        if (CtrlBlk_p->InitParams.FifoEnabled)
                        {
                            I2cRegPtr[SSC_CONTROL] |= SSC_CON_ENB_TX_FIFO;
                            I2cRegPtr[SSC_CONTROL] |= SSC_CON_ENB_RX_FIFO;
                        }
                        for(Loop=0; Loop<LoopCount; Loop++)
                        {
                            /* notify user,get char to send */
                            STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxByteId,
                                         (void *)&TxData);
                            /* shift TXdata left by one bit to provide LSB to
                            the receiver to use it as ack*/
                            TxData = ((TxData & 0xFF) << 1) | 1;
#if defined(STI2C_SLAVE_STRETCH)
                            if (Status & SSC_STAT_STRETCH)
                            {
                                DU32 *PioReg = CtrlBlk_p->BaseAddressforSCL;
                                PioReg[PIO_OUT_CLR] = CtrlBlk_p->PIOforSCL.BitMask;
                                PIO_PUSHPULL(PioReg, CtrlBlk_p->PIOforSCL.BitMask);
                                /* Load TX buffer */
                                I2cRegPtr[SSC_TX_BUFFER] = (U16)TxData;
                                I2C_TRACE(CtrlBlk_p,0x99,TxData);
                                PIO_ALTBI(PioReg, CtrlBlk_p->PIOforSCL.BitMask);
                                PioReg[PIO_OUT_SET] = CtrlBlk_p->PIOforSCL.BitMask;
                            }
                            else
#endif /* STI2C_SLAVE_STRETCH */
                            {
                                I2cRegPtr[SSC_TX_BUFFER] = (U16)TxData;
                            }
                            I2C_TRACE(CtrlBlk_p,0x02,TxData);
                        }
                    }
                    else
                    {
                        /* R/W=0
                        Move to slave Receiver state, notify user, turn on acks */
                        CtrlBlk_p->State = STI2C_SLAVE_RECEIVE;
                        STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->RxNotifyId,
                                      NULL);
                        if (CtrlBlk_p->InitParams.FifoEnabled)
                        {
                            I2cRegPtr[SSC_CONTROL] |= SSC_CON_ENB_TX_FIFO;
                            I2cRegPtr[SSC_CONTROL] |= SSC_CON_ENB_RX_FIFO;
                        }
                        /* Enable the Ack generation */
                        for (Loop=0;Loop<LoopCount;Loop++)
                        {
                            I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;
                            I2C_TRACE(CtrlBlk_p,0x03,RxData);
                        }
                    }
                    CtrlBlk_p->SlaveStatus = STI2C_IDLE ;
                }
                else
                {
                    /* Nothing received - switch to slave mode, but we
                    don't yet know if read or write. */
                    CtrlBlk_p->State = STI2C_SLAVE_ADDRESSED;
                    I2C_TRACE(CtrlBlk_p,0x04,0);
                }
            }
            else
            {
                RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x01ff;
                if (Status & SSC_STAT_STRETCH)
                {
                    I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;
                }
                I2C_TRACE(CtrlBlk_p,0x05,RxData);
                if (RxData & 0x02)
                {
                    /* R/W=1
                    Move to slave transmitter state,notify user,get 1st
                    char to send add 'ack' bit & put in tx buffer. */
                    CtrlBlk_p->SlaveStatus = STI2C_SLAVE_TRANSMIT;
                    I2C_TRACE(CtrlBlk_p,0x100,TxData);

                }
                else
                {
                    /* R/W=0
                    Move to slave Receiver state, notify user, turn on acks */
                    CtrlBlk_p->SlaveStatus = STI2C_SLAVE_RECEIVE;
                    I2C_TRACE(CtrlBlk_p,0x101,TxData);

                }

            }
            break;

        case STI2C_SLAVE_TRANSMIT:

            if (Status & SSC_STAT_TX_ERROR)
            {
                if (Status & SSC_STAT_NACK)
                {
                    if (Status & SSC_STAT_RX_BUFFER_FULL)
                    {
                        for(Loop=0;Loop<LoopCount;Loop++)
                        {
                            RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x01ff;
                        }
                    }
                    else
                    {
                        NumberToRead = I2cRegPtr[SSC_RX_FSTAT];
                        for (Loop=0; Loop<NumberToRead; Loop++)
                        {
                            I2C_TRACE(CtrlBlk_p,0x40,RxData);
                            RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x01ff;
                        }
                    }
                    /*Master wants to stop the T/F so Disable transmitter
                    bus will return to idle state after stop condition is generated*/
                    CtrlBlk_p->HandlerCondition  = ST_NO_ERROR;
                    I2cRegPtr[SSC_I2C] = (I2cRegPtr[SSC_I2C] & ~SSC_I2C_TXENABLED);
                    STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxCompleteId,(void *)&TxData);
                    /*state will be changed on receiving stop  interrupt*/
                    I2C_TRACE(CtrlBlk_p,0x30,RxData);
                }
                else
                {
                    if (Status & SSC_STAT_RX_BUFFER_FULL)
                    {
                        for(Loop=0;Loop<LoopCount;Loop++)
                        {
                            RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x01ff;
                        }
                    }
                    IntEnables |= SSC_IEN_NACK;
                    for(Loop=0;Loop<LoopCount;Loop++)
                    {
                        /* not last byte, so get next byte & send it */
                        STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxByteId,
                                     (void *)&TxData);
                        TxData = ((TxData & 0xFF) << 1) | 1;
#if defined(STI2C_SLAVE_STRETCH)
                        if (Status & SSC_STAT_STRETCH)
                        {
                            DU32 *PioReg = CtrlBlk_p->BaseAddressforSCL;

                            PioReg[PIO_OUT_CLR] = CtrlBlk_p->PIOforSCL.BitMask;
                            PIO_PUSHPULL(PioReg, CtrlBlk_p->PIOforSCL.BitMask);

                            /* Load TX buffer */
                            I2cRegPtr[SSC_TX_BUFFER] = (U16)TxData;

                            I2C_TRACE(CtrlBlk_p,0x100,TxData);
                            PIO_ALTBI(PioReg, CtrlBlk_p->PIOforSCL.BitMask);
                            PioReg[PIO_OUT_SET] = CtrlBlk_p->PIOforSCL.BitMask;
                        }
                        else
#endif /* STI2C_SLAVE_STRETCH */
                        {
                            /* Not stretching, or no workaround-enabled */
                            I2cRegPtr[SSC_TX_BUFFER] = (U16)TxData;
                        }
                        I2C_TRACE(CtrlBlk_p,0x31,TxData);
                    }
                }
            }
            else
            {
                I2C_TRACE(CtrlBlk_p,0x32,RxData);
            }
            break;

        case STI2C_SLAVE_RECEIVE:

            if (Status & SSC_STAT_TX_ERROR)
            {
                if (Status & SSC_STAT_RX_BUFFER_FULL)
                {
                    for(Loop=0;Loop<LoopCount;Loop++)
                    {
                        /* Read a byte, so notify the user */
                        I2C_TRACE(CtrlBlk_p,0x40,RxData);
                        RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x01ff;
                        RxData >>= 1;
                        STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->RxByteId, &RxData);
                        I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;
                    }
                }
            }
            else
            {
                I2C_TRACE(CtrlBlk_p,0x41,RxData);
            }
            break;

    case STI2C_SLAVE_ADDRESSED:
            /* We have entered slave state, but don't yet know if R or W.
               This state is  only needed because of a h/w bug, which
               results in AAS being set before 'Rx buffer full'. */

            if (Status & SSC_STAT_TX_ERROR)
            {
                RxData = I2cRegPtr[SSC_RX_BUFFER] & 0x01ff;
                if (RxData & 0x02)
                {
                    /* Start of slave Transmitter - notify user*/
                    CtrlBlk_p->State = STI2C_SLAVE_TRANSMIT;
                    STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxNotifyId,
                                      NULL);
                    {
                        U32 temp = I2cRegPtr[SSC_I2C] & 0xfff;

                        /* Arm the transmitter, and set
                           fast mode if needed */
                        temp |= SSC_I2C_TXENABLED;
                        if (CtrlBlk_p->InitParams.BaudRate == STI2C_RATE_FASTMODE)
                             temp |= SSC_I2C_FASTMODE;

                        I2cRegPtr[SSC_I2C] = temp;
                    }
                    for( Loop=0;Loop<LoopCount;Loop++)
                    {
                        /* notify user,get char to send */
                        STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxByteId,
                                     (void *)&TxData);
                        TxData = ((TxData & 0xFF) << 1) | 1;

#if defined(STI2C_SLAVE_STRETCH)
                        if (Status & SSC_STAT_STRETCH)
                        {
                            DU32 *PioReg = CtrlBlk_p->BaseAddressforSCL;

                            PioReg[PIO_OUT_CLR] = CtrlBlk_p->PIOforSCL.BitMask;
                            PIO_PUSHPULL(PioReg, CtrlBlk_p->PIOforSCL.BitMask);

                            /* Load TX buffer */
                            I2cRegPtr[SSC_TX_BUFFER] = (U16)TxData;

                            I2C_TRACE(CtrlBlk_p,0x101,TxData);
                            PIO_ALTBI(PioReg, CtrlBlk_p->PIOforSCL.BitMask);
                            PioReg[PIO_OUT_SET] = CtrlBlk_p->PIOforSCL.BitMask;
                        }
                        else
#endif /* STI2C_SLAVE_STRETCH */
                        {
                            /* Not stretching, or no workaround-enabled */
                            I2cRegPtr[SSC_TX_BUFFER] = (U16)TxData;
                        }
                        I2C_TRACE(CtrlBlk_p,0x20,TxData);
                    }
                }
                else
                {   /* Start of slave Receiver - notify user */
                    CtrlBlk_p->State = STI2C_SLAVE_RECEIVE;
                    I2C_TRACE(CtrlBlk_p,0x21,0);
                    STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->RxNotifyId,
                                  NULL);
                    for( Loop=0;Loop<LoopCount;Loop++)
                    {
                        I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;
                    }
                    Temp = SSC_I2C_ENABLE | SSC_I2C_GENERATE_ACKS;

                    /* Need tx enabled here? */
                    if (CtrlBlk_p->InitParams.BaudRate == STI2C_RATE_FASTMODE)
                    {
                        Temp |= SSC_I2C_FASTMODE;
                    }
                    I2cRegPtr[SSC_I2C] = Temp;
                }
            }
            else
            {
                I2C_TRACE(CtrlBlk_p,0x22,RxData);
            }
            break;

#endif /* STI2C_MASTER_ONLY */
    default:
            I2C_TRACE_ERR_INC;
            I2C_TRACE(CtrlBlk_p,0xF05,RxData);
            break;
    }

    /* Reenable interrupts */
    I2cRegPtr[SSC_INT_ENABLE] = IntEnables;

#if defined(STI2C_DEBUG)
    sprintf(tmp, "%08x State %i", (U32)I2cRegPtr, CtrlBlk_p->State);
    print(tmp);
    InInterrupt = FALSE;
#endif

    STOS_INTERRUPT_EXIT(STOS_SUCCESS);
}

/*****************************************************************************
STI2C_Init()
Description:
    Initialises an i2c device and its control structure.
Parameters  : 1) A pointer to the devicename
              2) A pointer to an initialisation parameter block
Return Value:
    ST_NO_ERROR,                       no errors.
    ST_ERROR_ALREADY_INITIALIZED       Another user has already initialized the device
    ST_ERROR_BAD_PARAMETER             One or more of the parameters was invalid
    ST_ERROR_NO_MEMORY                 Unable to allocate memory for internal data structures
    STI2C_ERROR_LINE_STATE             The data and clock signals are held low
    ST_ERROR_INTERRUPT_INSTALL         Error installing interrupts for the APIs internal interrupt handler
    STI2C_ERROR_PIO                    Error accessing specified PIO port (see below)
    STI2C_ERROR_EVT_REGISTER           Error registering for events
    STI2C_ERROR_NO_FREE_SSC            Error if all avaible SSCs have been already initialised
    ST_ERROR_FEATURE_NOT_SUPPORTED     Error while trying to initialise in slave mode if driver
                                       has been built in STI2C_MASTER_ONLY mode.
See Also:
    STI2C_Close()
    STI2C_Open()
*****************************************************************************/

ST_ErrorCode_t STI2C_Init(    const ST_DeviceName_t      Name,
                              const STI2C_InitParams_t   *InitParams_p)
{
    DU32                 *I2cRegPtr  = NULL;
    ST_ErrorCode_t       ReturnCode  = ST_NO_ERROR;
    i2c_param_block_t    *InitBlock_p = NULL;
    U8                   PIOValue,Loop=0;
#if !defined (STI2C_STSPI_SAME_SSC_SUPPORT)
    U32                  DeadData;
    S32                  IntReturn;
#endif
    S32                  PIOPin1, PIOPin2;
    STPIO_OpenParams_t   PIOOpenParams;
    ST_ErrorCode_t       PIOReturnCode;
    STPIO_Handle_t       PIOHandle;
    U32                  OpenBlkIndex=0,CtrBlkIndex=0;
    char IntName[] = "STI2C";

   /* Do Parameters checking only if STI2C_NO_PARAM_CHECK is disabled*/

#ifndef STI2C_NO_PARAM_CHECK
    if ((Name == 0)||(strlen(Name) >= ST_MAX_DEVICE_NAME )   ||
        (InitParams_p == NULL) || (InitParams_p->ClockFrequency == 0)||
        (InitParams_p->MaxHandles == 0))
    {
        return(ST_ERROR_BAD_PARAMETER) ;
    }
    /*Check partition has been specified */
    if (InitParams_p->DriverPartition == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
    /* Extra checks for Slave mode... */
    if ( InitParams_p->MasterSlave != STI2C_MASTER )
    {
#ifdef STI2C_MASTER_ONLY
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
        /* Check for valid slave address  */
        if (InitParams_p->SlaveAddress < 2)
        {
            return ST_ERROR_BAD_PARAMETER;
        }
#endif /* STI2C_MASTER_ONLY */
    }

#endif

#if defined(ST_8010)
    if ( InitParams_p->BaseAddress == (U32*)SSC_2_BASE_ADDRESS)
#endif
    {
        /* Check PIO Pin values are valid */
        PIOPin1 = GetPioPin (InitParams_p->PIOforSDA.BitMask);
        PIOPin2 = GetPioPin (InitParams_p->PIOforSCL.BitMask);
        if ( (PIOPin1 >= UCHAR_MAX) || (PIOPin2 >= UCHAR_MAX) )
        {
            return ST_ERROR_BAD_PARAMETER;
        }
    }
    STOS_TaskLock();
    if (First_Init == TRUE)
    {
        /*  Initialised the global semaphore */
        g_DriverAtomic_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 1);

        for (CtrBlkIndex = 0; CtrBlkIndex <  NUM_SSC; CtrBlkIndex++)
        {
            CtrlBlockArray[CtrBlkIndex].InUse = 0;
        }

        First_Init = FALSE;
    }
    STOS_TaskUnlock();

    /* Take this to protect global control list */
    STOS_SemaphoreWait(g_DriverAtomic_p);

    if (IsAlreadyInitialised (Name, InitParams_p) == TRUE)
    {
        /* release driver atomic */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return ST_ERROR_ALREADY_INITIALIZED;
    }

    /* Find a free control block structure*/
    for (CtrBlkIndex = 0; CtrBlkIndex <  NUM_SSC; CtrBlkIndex ++ )
    {
        if (CtrlBlockArray[CtrBlkIndex].InUse==0)
        {
            InitBlock_p = &CtrlBlockArray[CtrBlkIndex];
            break;
        }
    }
    /* If no free control block has been found,return error*/
    if (InitBlock_p == NULL)
    {
        /* release driver atomic */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return STI2C_ERROR_NO_FREE_SSC;
    }

    strcpy( InitBlock_p->Name, Name );

    /* Set up the control structures */
    memcpy(&InitBlock_p->InitParams, InitParams_p, sizeof(STI2C_InitParams_t));
    InitBlock_p->OpenCnt                = 0;
    InitBlock_p->LastTransferHadStop    = TRUE;
    InitBlock_p->InterruptMask          = DEFAULT_ENABLES;
    InitBlock_p->State                  = STI2C_IDLE;
    InitBlock_p->Buffer_p               = NULL;
    InitBlock_p->BufferLen              = 0;
    InitBlock_p->BufferCnt              = 0;
    CtrlBlockArray[CtrBlkIndex].InUse   = 1;
    InitBlock_p->PIOforSDA.BitMask      = InitParams_p->PIOforSDA.BitMask;
    InitBlock_p->PIOforSCL.BitMask      = InitParams_p->PIOforSCL.BitMask;
    InitBlock_p->InitParams.GlitchWidth = InitParams_p->GlitchWidth;

    /* Enable the FIFO if required */
#if defined(SSC4_PRESENT)
    InitBlock_p->InitParams.FifoEnabled = InitParams_p->FifoEnabled;
#else
    InitBlock_p->InitParams.FifoEnabled = 0;
#endif

#ifndef STI2C_MASTER_ONLY
    InitBlock_p->InitParams.SlaveAddress = InitParams_p->SlaveAddress;
    InitBlock_p->SlaveStatus             = STI2C_IDLE;
    /* Extra setup for Slave mode.... */
    if ( InitParams_p->MasterSlave != STI2C_MASTER )
    {
        ReturnCode = RegisterEvents (InitParams_p->EvtHandlerName, InitBlock_p);
    }

#endif

/*-----------------------------------------------------------------------*/
    /*Initialise the semaphores*/
    InitBlock_p->IOSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);

    InitBlock_p->BusAccessSemaphore_p = STOS_SemaphoreCreatePriorityTimeOut(NULL, 1);

    /* Reset I2C Control block */
    I2cRegPtr = (DU32*) InitBlock_p->InitParams.BaseAddress;

    /* Turn off interrupts while programming block */
    I2cRegPtr[SSC_INT_ENABLE] = 0;

    if (ReturnCode == ST_NO_ERROR)
    {
        /* Get physical addresses of PIO ports so we can do s/w
           START & STOP sequences without using PIO API (speed) */
#if defined(ST_8010) || defined(ST_5525)
        if (( InitParams_p->BaseAddress == (U32*)SSC_0_BASE_ADDRESS)
            || ( InitParams_p->BaseAddress == (U32*)SSC_1_BASE_ADDRESS))
        {
            sti2c_Setup();
        }
        else if ( (InitParams_p->BaseAddress == (U32*)SSC_2_BASE_ADDRESS)
#if defined(ST_5525)
               || (InitParams_p->BaseAddress == (U32*)SSC_3_BASE_ADDRESS)
#endif
        )
#endif
        {
            PIOReturnCode = STPIO_GetBaseAddress (InitBlock_p->InitParams.PIOforSDA.PortName,
                                                  &InitBlock_p->BaseAddressforSDA);
            if (PIOReturnCode != ST_NO_ERROR)
            {
                ReturnCode = STI2C_ERROR_PIO;
            }

            PIOReturnCode = STPIO_GetBaseAddress (InitBlock_p->InitParams.PIOforSCL.PortName,
                                                 &InitBlock_p->BaseAddressforSCL);
            if (PIOReturnCode != ST_NO_ERROR)
            {
                ReturnCode = STI2C_ERROR_PIO;
            }
            /* Initialise PIO for setup */
            PIOOpenParams.IntHandler            =  NULL;
            PIOOpenParams.BitConfigure[PIOPin1] =  STPIO_BIT_OUTPUT_HIGH;
            PIOOpenParams.BitConfigure[PIOPin2] =  STPIO_BIT_OUTPUT_HIGH;
            PIOOpenParams.ReservedBits          =  InitBlock_p->InitParams.PIOforSDA.BitMask |
                                                   InitBlock_p->InitParams.PIOforSCL.BitMask;

            PIOReturnCode =  STPIO_Open ( InitBlock_p->InitParams.PIOforSDA.PortName,
                                            &PIOOpenParams, &PIOHandle );
            if (PIOReturnCode != ST_NO_ERROR)
            {
                ReturnCode = STI2C_ERROR_PIO;
            }
            PIOReturnCode = STPIO_Read(PIOHandle,&PIOValue);
            if (PIOReturnCode != ST_NO_ERROR)
            {
                ReturnCode = STI2C_ERROR_PIO;
            }
            while(((!(PIOValue & InitBlock_p->InitParams.PIOforSDA.BitMask))
                 ||(!(PIOValue & InitBlock_p->InitParams.PIOforSCL.BitMask)))
                 && (Loop < 50))
            {
                STPIO_Read(PIOHandle,&PIOValue);
                Loop++;
            }
            if (PIOValue & PIOOpenParams.ReservedBits)
            {
                PIOOpenParams.BitConfigure[PIOPin1] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
                PIOOpenParams.BitConfigure[PIOPin2] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
                PIOReturnCode = STPIO_SetConfig (PIOHandle,PIOOpenParams.BitConfigure);
                /* Make sure the PIO_SetConfig worked */
                if ( PIOReturnCode == ST_NO_ERROR )
                {
                    /* Copy PIO handle into control structure */
                    InitBlock_p->PIOHandle = PIOHandle;
                    /* Test bus but allow initialisation to continue if it fails */
                    if ( BusStuck( InitBlock_p ) )
                    {
                        ReturnCode = STI2C_ERROR_LINE_STATE;
                    }
                }
                else
                {
                    ReturnCode = STI2C_ERROR_PIO;
                }
            }
            else
            {
                ReturnCode = STI2C_ERROR_PIO;
            }
        }
        if ( ReturnCode == ST_NO_ERROR )
        {
#if !defined (STI2C_STSPI_SAME_SSC_SUPPORT)
            /* Try to register and enable the interrupt handler */
            IntReturn = STOS_InterruptInstall ( InitBlock_p->InitParams.InterruptNumber,
                                                InitBlock_p->InitParams.InterruptLevel,
                                                I2cHandler,IntName, InitBlock_p );

            if (IntReturn == 0)
            {
                /* STOS_InterruptEnable() will be redundant after change will be performed in STBOOT */
                IntReturn =STOS_InterruptEnable(InitBlock_p->InitParams.InterruptNumber, InitBlock_p->InitParams.InterruptLevel);

                if ( IntReturn == 0 )
#endif
                {
                    /* Add the correct number of open blocks to the list */
                    InitBlock_p->OpenBlockPtr  =
                            (i2c_open_block_t*)memory_allocate( InitParams_p->DriverPartition,
                            (sizeof(i2c_open_block_t)*(InitParams_p->MaxHandles)));

                    /* Allocate memory for locked handle pool */
                    InitBlock_p->LockedHandlePool_p  =
                            (STI2C_Handle_t*)memory_allocate( InitParams_p->DriverPartition,
                            (sizeof(STI2C_Handle_t)*(InitParams_p->MaxHandles)));

                    /* If pointer value not NULL the allocate suceeded */
                    if ( (InitBlock_p->OpenBlockPtr != 0) && (InitBlock_p->LockedHandlePool_p != 0) )
                    {
                        /* Set block values to defaults */
                        for (OpenBlkIndex = 0 ; OpenBlkIndex < InitParams_p->MaxHandles;
                             OpenBlkIndex++)
                        {
                            InitBlock_p->OpenBlockPtr[OpenBlkIndex].MagicNumber  = 0;
                            InitBlock_p->OpenBlockPtr[OpenBlkIndex].OpenParams.I2cAddress  = 0;
                            InitBlock_p->OpenBlockPtr[OpenBlkIndex].CtrlBlkPtr   = InitBlock_p;
                            InitBlock_p->LockedHandlePool_p[OpenBlkIndex] = 0;
                        }
#if !defined (STI2C_STSPI_SAME_SSC_SUPPORT)
                        /* perform bus reset */
                        I2cReset(InitBlock_p);
                        I2cRegPtr[SSC_I2C] = DEFAULT_I2C;
#endif
#if !defined(STI2C_MASTER_ONLY)
                        /* Set Slave Address, if appropriate */
                        if ( InitParams_p->MasterSlave != STI2C_MASTER )
                        {
                            WriteReg(I2cRegPtr, SSC_SLAD,
                                    (InitParams_p->SlaveAddress >> 1) & 0x7f);
                        }
#endif
#if !defined (STI2C_STSPI_SAME_SSC_SUPPORT)
                        /* Set Tx data high to avoid bus corruption */
                        WriteReg(I2cRegPtr, SSC_TX_BUFFER, 0x1ff);

                        WriteReg(I2cRegPtr, SSC_BAUD_RATE,
                                 InitBlock_p->InitParams.ClockFrequency / (15 * InitBlock_p->InitParams.BaudRate));
#if defined(SSC3_WITH_TIMING_PRESENT)
                        WriteReg(I2cRegPtr, SSC_GLITCH_WIDTH,(int)((GLITCH_WIDTH_DATAOUT*(InitBlock_p->InitParams.ClockFrequency / 10000000))
                                                              /(PRE_SCALER_FACTOR*1000)));
                        WriteReg(I2cRegPtr, SSC_PRE_SCALER,PRE_SCALER_FACTOR);
                        WriteReg(I2cRegPtr, SSC_GLITCH_WIDTH_DATAOUT,(int)(GLITCH_WIDTH_DATAOUT/100));
                        WriteReg(I2cRegPtr, SSC_PRE_SCALER_DATAOUT,(int)(InitBlock_p->InitParams.ClockFrequency / 10000000));
                        /* Calculate and write the appropriate
                           values into the I2C timing registers. */
                        i2c_WriteTimingValues(InitBlock_p);
                        I2cRegPtr[SSC_I2C] = DEFAULT_I2C;
#else
                        WriteReg(I2cRegPtr, SSC_GLITCH_WIDTH,0x00);
                        WriteReg(I2cRegPtr, SSC_PRE_SCALER,(int)(InitBlock_p->InitParams.ClockFrequency / 10000000));
#endif
                        /* Read i2c buffer to clear spurious data */
                        DeadData  = ReadReg(I2cRegPtr, SSC_RX_BUFFER);

                        /* enable interrupts from I2C registers */
                        WriteReg(I2cRegPtr, SSC_INT_ENABLE,
                                 InitBlock_p->InterruptMask);

                        InitBlock_p->LastTransferHadStop = TRUE;
#endif
                    }
                    else       /* Allocate open block failed */
                    {
                        ReturnCode = ST_ERROR_NO_MEMORY;
                    }
                }
#if !defined (STI2C_STSPI_SAME_SSC_SUPPORT)
                else      /* Interrupt enable failed */
                {
                    ReturnCode = ST_ERROR_BAD_PARAMETER;
                }
            }
            else
            {
                ReturnCode = ST_ERROR_INTERRUPT_INSTALL;
            }
#endif
        }
    }
    if(ReturnCode != ST_NO_ERROR)
    {
        /* Reset the devices */
        I2cRegPtr = (DU32*)InitBlock_p->InitParams.BaseAddress;
        I2cRegPtr[SSC_INT_ENABLE] = 0;
        I2cRegPtr[SSC_I2C]        = 0;
        I2cRegPtr[SSC_CONTROL]    = 0;
        /*deallocate the open blocks*/
        I2c_Deallocate( InitBlock_p);
        /* clear the data in the control block*/
        memset (&CtrlBlockArray[CtrBlkIndex],0,sizeof(i2c_param_block_t ));
        CtrlBlockArray[CtrBlkIndex].InUse = 0;
    }
    /* Release global semaphore */
    STOS_SemaphoreSignal(g_DriverAtomic_p);

    return ( ReturnCode );
}

/*****************************************************************************
STI2C_Open
Description:
    Create and return a handle to an initialised i2c device.
Parameters  : 1) A pointer to the name of a device control block
              2) A pointer to parameter block
              3) A pointer to an empty handle
Return Value:
    ST_NO_ERROR,                       no errors.
    ST_ERROR_BAD_PARAMETER             One or more of the parameters was invalid
    ST_ERROR_UNKNOWN_DEVICE            Invalid device name
    ST_ERROR_FEATURE_NOT_SUPPORTED     Error while trying to initialise in slave mode if driver
                                       has been built in STI2C_MASTER_ONLY mode.
    ST_ERROR_NO_FREE_HANDLES           The maximum number of allocated handles are
                                       in use; cannot allocate another handle

See Also:
    STI2C_Close()
    STI2C_Init()
    STI2C_Read()
*****************************************************************************/

ST_ErrorCode_t STI2C_Open(    const ST_DeviceName_t      Name,
                              const STI2C_OpenParams_t   *OpenParams_p,
                              STI2C_Handle_t             *Handle)
{
    i2c_param_block_t    *CtrlBlkPtr_p    = NULL;
    i2c_open_block_t     *OpenBlkPtr_p    = NULL;
    U32               CtrBlkIndex=0,OpenBlkIndex=0;
    STOS_TaskLock();

    if(Handle != NULL)
    {
        *Handle = 0;
    }
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    STOS_TaskUnlock();

#ifndef STI2C_NO_PARAM_CHECK
    if ((Name == 0) ||(strlen(Name) >= ST_MAX_DEVICE_NAME ) ||
        (OpenParams_p == NULL) ||  (Handle == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (OpenParams_p->AddressType != STI2C_ADDRESS_7_BITS)
    {
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
#endif

    /* Take the  global semaphore to make safe access to global control list */
    STOS_SemaphoreWait(g_DriverAtomic_p);

    /* Walk the static array of Ctrl blocks until we find the right name */

    while ( CtrBlkIndex  <  NUM_SSC)
    {
       if ( strcmp( CtrlBlockArray[CtrBlkIndex].Name, Name ) == 0 )
       {
            CtrlBlkPtr_p= &CtrlBlockArray[CtrBlkIndex];
            break;
       }
       else
       {
            CtrBlkIndex++;
       }
    }

     /* If no match was found, this device name has not been init'ed */
    if ( CtrlBlkPtr_p  == NULL )
    {
        /* release driver atomic */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Take bus access. Note that we still have the global semaphore. */
    STOS_SemaphoreWait(CtrlBlkPtr_p->BusAccessSemaphore_p);
    /* Release the global semaphore so that other func call can take place*/
    STOS_SemaphoreSignal(g_DriverAtomic_p);

    /* Check OpenCnt is less than MaxHandles */
    if ( CtrlBlkPtr_p ->OpenCnt >=CtrlBlkPtr_p->InitParams.MaxHandles )
    {
        /* release semaphores */
        STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
        return ST_ERROR_NO_FREE_HANDLES;
    }
    /* Search a free Open Block to assign to this Handle */
    while ((OpenBlkIndex < CtrlBlkPtr_p->InitParams.MaxHandles))
    {
       if (CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].MagicNumber == 0)
        {
            break;
        }
        else
        {
            OpenBlkIndex ++;
        }
    }

    if (OpenBlkIndex == CtrlBlkPtr_p ->InitParams.MaxHandles)
    {
        /* release semaphores */
        STOS_SemaphoreSignal(CtrlBlkPtr_p ->BusAccessSemaphore_p);
#ifndef STI2C_NO_TBX
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                     "STI2C: !! Got \"no free handles\" after checking open count !!\n"));
#endif
        return ST_ERROR_NO_FREE_HANDLES;    /* Should never get this! */
    }

    OpenBlkPtr_p = &(CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex]);

    /* Increment the handle count in the control block */
    CtrlBlkPtr_p ->OpenCnt++;

    /* Store the open parameters */
    /* Bottom bit is R/W bit - remove for now */
    OpenBlkPtr_p->OpenParams.I2cAddress        = OpenParams_p->I2cAddress & 0xFFFE;
    OpenBlkPtr_p->OpenParams.AddressType       = OpenParams_p->AddressType;
    OpenBlkPtr_p->OpenParams.BusAccessTimeOut  = OpenParams_p->BusAccessTimeOut;
#ifdef STI2C_ADAPTIVE_BAUDRATE
    if ( (OpenParams_p->BaudRate == STI2C_RATE_NORMAL) || (OpenParams_p->BaudRate == STI2C_RATE_FASTMODE) )
    {
        OpenBlkPtr_p->OpenParams.BaudRate      = OpenParams_p->BaudRate;
    }
    else
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    OpenBlkPtr_p->OpenParams.BaudRate          = CtrlBlkPtr_p->InitParams.BaudRate;
#endif
    /*Place the driver Id as magic value to mark this block as in-use and valid*/
    OpenBlkPtr_p->MagicNumber                  = MAGIC_NUM;

    /* Set the handle pointer for the calling function */
    /* Handle value is expected to be the memory address of associated open block*/
    *Handle =  (STI2C_Handle_t)  OpenBlkPtr_p;

    /* Release the access semaphore */
    STOS_SemaphoreSignal( CtrlBlkPtr_p->BusAccessSemaphore_p );

    return ST_NO_ERROR;
}
/*****************************************************************************
STI2C_Close
Description:
    Closes an open handle to an i2c device.
Parameters  : 1) The handle to be closed

Return Value:
    ST_NO_ERROR,                       no errors.
    ST_ERROR_BAD_PARAMETER             One or more of the parameters was invalid
    ST_ERROR_UNKNOWN_DEVICE            Invalid device name
    ST_ERROR_FEATURE_NOT_SUPPORTED     Error while trying to initialise in slave mode if driver
                                       has been built in STI2C_MASTER_ONLY mode.
    ST_ERROR_NO_FREE_HANDLES           The maximum number of allocated handles are
                                       in use; cannot allocate another handle

See Also:
    STI2C_Open()
*****************************************************************************/
ST_ErrorCode_t STI2C_Close( STI2C_Handle_t Handle)
{
    i2c_param_block_t    *CtrlBlkPtr_p = 0;
    i2c_open_block_t     *OpenBlkPtr_p = 0;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;
    U32                  HandleIndex=0;

    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    if ((i2c_open_block_t*)Handle == NULL)
    {
        STOS_TaskUnlock();
        return ST_ERROR_INVALID_HANDLE;
    }
    STOS_TaskUnlock();

    /* Take global atomic */
    STOS_SemaphoreWait(g_DriverAtomic_p);
    OpenBlkPtr_p = (i2c_open_block_t*)Handle;

    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
        /* Release Busaccess Semaphore */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return ST_ERROR_INVALID_HANDLE;
    }
    CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;
    /* Check if this handle is in the Locked Handles pool */
    for(HandleIndex=0; HandleIndex<CtrlBlkPtr_p->InitParams.MaxHandles; HandleIndex++)
    {
        if (CtrlBlkPtr_p->LockedHandlePool_p[HandleIndex] == Handle)
        {
            /* Release Busaccess Semaphore */
            STOS_SemaphoreSignal(g_DriverAtomic_p);
            return STI2C_ERROR_BUS_IN_USE;
        }
    }
    /* Check if this handle is currently in use */
    if (CtrlBlkPtr_p->LockedHandleCurrent == Handle)
    {
        /* Release Busaccess Semaphore */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return STI2C_ERROR_BUS_IN_USE;
    }
    /* Take Bus access to protect the open list  */
    STOS_SemaphoreWait(CtrlBlkPtr_p->BusAccessSemaphore_p);

    /* Decrement the control structure open count */
    CtrlBlkPtr_p->OpenCnt--;
    /* Clear the entries in the open block  */
    OpenBlkPtr_p->OpenParams.I2cAddress    = 0;
    /* clear the magic value to mark it availabe for reuse*/
    OpenBlkPtr_p->MagicNumber              = 0;
    /* Release global Semaphore */
    STOS_SemaphoreSignal(g_DriverAtomic_p);
    /* Release Busaccess Semaphore */
    STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
    return( ReturnCode );
}

/******************************************************************************
Name        : BusStuck
Description : Tests if the I2C bus is in the correct state. The result
              depends on whether the last transfer had a Stop.
Parameters  : A pointer to the control structure for the i2c device
Returns     : result.
******************************************************************************/

static BOOL BusStuck (i2c_param_block_t    *InitBlock_p)
{
#ifdef STI2C_SW_START_STOP
    return FALSE;
#else
    DU32 *I2cRegPtr = (DU32 *) InitBlock_p->InitParams.BaseAddress;
    U16   Status    = I2cRegPtr[SSC_STATUS];

    I2C_TRACE_LOCK(InitBlock_p,0xC000,Status);
    Status &= SSC_STAT_BUSY;
    /* If last transfer had STOP, bus should be free. If not, should be busy.*/
    if (InitBlock_p->LastTransferHadStop)
    {
        return (Status ? TRUE : FALSE);
    }
    else
    {
        return (Status ? FALSE : TRUE);
    }
#endif /*STI2C_SW_START_STOP*/
}
/******************************************************************************
Name        : ConvertMStoTicks
Description : Converts a timeout in milliseconds to the appropiate number of
              clock ticks. Returns a pointer to the result. Takes care of
              special case 'infinity'. Also takes care of overflow by treating
              as infinity.

Parameters  : 1) The number of milliseconds to convert to ticks
              2) The number of clock ticks per second
              3) Pointer to result (not used if returns infinity)

Returns     : Pointer to result. This will either be pResult or
              TIMEOUT_INFINITY.
******************************************************************************/
static clock_t * ConvertMStoTicks (U32 TimeOutMS, U32 TicksPerSec,
                                   clock_t *pResult_p)
{
    int      Div = 0;
    clock_t  Timeout;

    /* Deal with Infinity as special case */
    if (TimeOutMS == STI2C_TIMEOUT_INFINITY)
    {
        return (clock_t *)(TIMEOUT_INFINITY);
    }
    /* Make sure the conversion will not overflow */
    while (TimeOutMS > INT_MAX/TicksPerSec)
    {
        Div++;
        TimeOutMS /= 10;
    }

    /* Convert from ms to clock ticks, with rounding */
    Timeout = (TimeOutMS * TicksPerSec + 500) / 1000;

    /* Now multiply up again, but if overflow giving infinity */
    while (Div > 0)
    {
        if (Timeout > INT_MAX/10)
        {
            return (clock_t *)(TIMEOUT_INFINITY);
        }
        Div--;
        Timeout *= 10;
    }

    *pResult_p = Timeout;
    return pResult_p;
}

/*****************************************************************************
STI2C_Term
Description:
    Puts an init()ed i2c device into a stable shut down state and
    deletes its associated control structures.
Parameters  : 1) A pointer to the name of a device to be shut down
              2) A pointer to the termination parameter block
Return Value:
    ST_NO_ERROR,                       no errors.
    ST_ERROR_INTERRUPT_UNINSTALL       Error Uninstalling interrupts for the APIs internal interrupt handler
    ST_ERROR_UNKNOWN_DEVICE            Device has not been initialized
    ST_ERROR_OPEN_HANDLE               Could not terminate driver; not all handles have been closed

See Also:
    STI2C_Close()
    STI2C_Init()
    STI2C_Open()
*****************************************************************************/

ST_ErrorCode_t STI2C_Term(    const ST_DeviceName_t      Name,
                              const STI2C_TermParams_t  *TermParams_p)
{
    DU32                 *I2cRegPtr   = NULL;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;
    i2c_param_block_t    *CtrlBlkPtr_p  = NULL;
    U32                  OpenBlkIndex=0,CtrBlkIndex=0;


#ifndef STI2C_NO_PARAM_CHECK
    if ((Name == 0)|| (strlen(Name) >= ST_MAX_DEVICE_NAME )  ||
        (TermParams_p == NULL) )
    {
        return(ST_ERROR_BAD_PARAMETER) ;
    }
#endif

    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    STOS_TaskUnlock();

    /* Take the  global semaphore to make safe access to global control list */
    STOS_SemaphoreWait(g_DriverAtomic_p);
    /* Walk the static array of Ctrl blocks until we find the right name */
    while (CtrBlkIndex  <  NUM_SSC)
    {
        if (strcmp( CtrlBlockArray[CtrBlkIndex].Name, Name) == 0)
        {
            CtrlBlkPtr_p= &CtrlBlockArray[CtrBlkIndex];
            break;
        }
        else
        {
            CtrBlkIndex++;
        }
    }
    /* If no match was found, this device name has not been init'ed */
    if (CtrlBlkPtr_p == NULL)
    {
        /* release driver atomic */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    /* Take bus access. Note that we still have the global semaphore.
       This is done  to ensure that nobody else is waiting on it when it gets deleted */
    STOS_SemaphoreWait(CtrlBlkPtr_p ->BusAccessSemaphore_p);
    /*Check the opencount */
    if ((CtrlBlkPtr_p->OpenCnt != 0) && (TermParams_p->ForceTerminate == FALSE))
    {
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        STOS_SemaphoreSignal(CtrlBlkPtr_p ->BusAccessSemaphore_p);
        return ST_ERROR_OPEN_HANDLE;
    }
    /* Do Force termination of the block if required */
    while ((OpenBlkIndex < CtrlBlkPtr_p->InitParams.MaxHandles) && (CtrlBlkPtr_p->OpenCnt != 0))
    {
        if (CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].MagicNumber == MAGIC_NUM)
        {
            /* Decrement the control structure open count */
            CtrlBlkPtr_p->OpenCnt--;
            /* Make LockedHandlePool 0 */
            CtrlBlkPtr_p->LockedHandlePool_p[OpenBlkIndex] = 0;
            /* Clear the entries in the open block  */
            CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].OpenParams.I2cAddress    = 0;
            /* clear the magic value to mark it available for reuse */
            CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].MagicNumber              = 0;
        }
        else
        {
            OpenBlkIndex++;
        }
    }

    I2cRegPtr = (DU32*) CtrlBlkPtr_p->InitParams.BaseAddress;
#if !defined (STI2C_STSPI_SAME_SSC_SUPPORT)
    /* Disable the Interrupt */
    I2cRegPtr[SSC_INT_ENABLE] = 0;
    /* Uninstall the interrupt handler for this port. Only disable
     * interrupt if no other handlers connected on the same level.
     * Note this needs to be MT-safe.
     */
    STOS_InterruptLock();
    if (STOS_InterruptUninstall(CtrlBlkPtr_p->InitParams.InterruptNumber,
                                CtrlBlkPtr_p->InitParams.InterruptLevel,
                                CtrlBlkPtr_p ) != 0)
    {
        ReturnCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }
    if (STOS_InterruptDisable(CtrlBlkPtr_p->InitParams.InterruptNumber,
                                CtrlBlkPtr_p->InitParams.InterruptLevel)!=0)
    {
        ReturnCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }

    STOS_InterruptUnlock();       /* end of interrupt uninstallation */
#endif
    /* Deallocate all open blocks associated with this device */
    memory_deallocate( CtrlBlkPtr_p->InitParams.DriverPartition, CtrlBlkPtr_p->OpenBlockPtr);
    memory_deallocate( CtrlBlkPtr_p->InitParams.DriverPartition, CtrlBlkPtr_p->LockedHandlePool_p);

    /* Reset the devices */
    I2cRegPtr[SSC_I2C] = 0;
    I2cRegPtr[SSC_CONTROL] = 0;
    /* deallocate the semaphores */
    STOS_SemaphoreDelete(NULL, CtrlBlkPtr_p->IOSemaphore_p );
    STOS_SemaphoreDelete(NULL, CtrlBlkPtr_p->BusAccessSemaphore_p );

    if((CtrlBlkPtr_p->InitParams.PIOforSDA.PortName!= NULL) ||
           (CtrlBlkPtr_p->InitParams.PIOforSDA.PortName!= NULL))
    {

#ifndef STI2C_NO_PIO
        STPIO_Close (CtrlBlkPtr_p->PIOHandle);
#endif

    }
#ifndef STI2C_MASTER_ONLY
    if (CtrlBlkPtr_p->InitParams.MasterSlave != STI2C_MASTER)
    {
        ReturnCode = STEVT_Close ( CtrlBlkPtr_p->EVTHandle );
    }
#endif /* STI2C_MASTER_ONLY */
    /* clear the data in the control block*/
    memset (&CtrlBlockArray[CtrBlkIndex],0,sizeof(i2c_param_block_t ));
    /* We've now finished with the atomic operations */
    STOS_SemaphoreSignal(g_DriverAtomic_p);

    return( ReturnCode );
}

/******************************************************************************
Name        : I2c_Deallocate
Description : Deallocate
Parameters  : A pointer to the control structure for the i2c device
Return      : None
******************************************************************************/

static void I2c_Deallocate(  i2c_param_block_t    *CtrlBlkPtr_p)
{
    U32                  OpenBlkIndex=0;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;

    /* Clear the data in the open blocks that are currently being used  */
    while((OpenBlkIndex < CtrlBlkPtr_p->InitParams.MaxHandles)&&
          (CtrlBlkPtr_p->OpenCnt != 0))
    {
        if (CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].MagicNumber  == MAGIC_NUM)
        {
            /* Decrement the control structure open count */
            CtrlBlkPtr_p->OpenCnt--;
            /* Clear the entries in the open block  */
            CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].OpenParams.I2cAddress    = 0;
            CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].CtrlBlkPtr               = NULL;
            /* clear the magic value to mark it availabe for reuse*/
            CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].MagicNumber              = 0;
        }
        else
        {
            OpenBlkIndex++;
        }
     }
     /* Uninstall the interrupt handler */
    STOS_InterruptLock();
    STOS_InterruptUninstall(CtrlBlkPtr_p->InitParams.InterruptNumber,
                            CtrlBlkPtr_p->InitParams.InterruptLevel,
                            CtrlBlkPtr_p );
    if (STOS_InterruptDisable(CtrlBlkPtr_p->InitParams.InterruptNumber,
                          CtrlBlkPtr_p->InitParams.InterruptLevel )!=0)
    {
        ReturnCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }

    STOS_InterruptUnlock();       /* end of interrupt uninstallation */
    /* Deallocate all open blocks associated with this device */
    memory_deallocate( CtrlBlkPtr_p->InitParams.DriverPartition, CtrlBlkPtr_p->OpenBlockPtr);

    /* deallocate the semaphores */
    STOS_SemaphoreDelete(NULL, CtrlBlkPtr_p->IOSemaphore_p );
    STOS_SemaphoreDelete(NULL, CtrlBlkPtr_p->BusAccessSemaphore_p );
    /* Close the PIO Handle  */
#ifndef STI2C_NO_PIO
    if((CtrlBlkPtr_p->InitParams.PIOforSDA.PortName!= NULL) ||
           (CtrlBlkPtr_p->InitParams.PIOforSDA.PortName!= NULL))
    {
        STPIO_Close (CtrlBlkPtr_p->PIOHandle );
    }
#endif
    /* Close the Event Handler */
#ifndef STI2C_MASTER_ONLY
    if (CtrlBlkPtr_p->InitParams.MasterSlave != STI2C_MASTER)
    {
        STEVT_Close ( CtrlBlkPtr_p->EVTHandle );
    }
#endif /* STI2C_MASTER_ONLY */
}
/*****************************************************************************
STI2C_GetParams
Description:
    Returns the current operating parameters
Parameters  : 1) A handle to the i2c device being read from
              2) A pointer for returning the current parameters
Return Value:
    ST_NO_ERROR,                 no errors.
    ST_ERROR_BAD_PARAMETER       One or more of the parameters was invalid
    ST_ERROR_UNKNOWN_DEVICE      Device has not been initialized
    ST_ERROR_INVALID_HANDLE      Device handle invalid.

See Also:
    STI2C_SetParams()
*****************************************************************************/
ST_ErrorCode_t STI2C_GetParams( STI2C_Handle_t Handle,
                                STI2C_Params_t *GetParams_p )
{

    i2c_open_block_t   *OpenBlkPtr_p;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
#ifndef STI2C_NO_PARAM_CHECK
    if (GetParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER) ;
    }
#endif
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    STOS_TaskUnlock();

    /* Take global atomic */
    STOS_SemaphoreWait(g_DriverAtomic_p);
    /* Set up the pointers to the open block  */
    OpenBlkPtr_p = (i2c_open_block_t*)Handle;
    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
        /* Release Busaccess Semaphore */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Copy the parameters from open block to user defined structure*/
    memcpy(GetParams_p, &OpenBlkPtr_p->OpenParams, sizeof(STI2C_OpenParams_t));
    /* Release the access semaphore */
    STOS_SemaphoreSignal(g_DriverAtomic_p);
    return( ErrorCode );
   }
/*****************************************************************************
STI2C_SetParams
Description:
    Returns the current operating parameters
Parameters  : 1) A handle to the i2c device being read from
              2) Pointer to the I2C parameter data structure
Return Value:
    ST_NO_ERROR,                 no errors.
    ST_ERROR_BAD_PARAMETER       One or more of the parameters was invalid
    ST_ERROR_UNKNOWN_DEVICE      Device has not been initialized
    ST_ERROR_INVALID_HANDLE      Device handle invalid.

See Also:
   STI2C_GetParams()
*****************************************************************************/
ST_ErrorCode_t STI2C_SetParams( STI2C_Handle_t       Handle,
                                const STI2C_Params_t *SetParams_p )
{

    i2c_open_block_t   *OpenBlkPtr_p = NULL;
    i2c_param_block_t  *CtrlBlkPtr_p = NULL;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
#ifndef STI2C_NO_PARAM_CHECK
    if ((SetParams_p == NULL) || (SetParams_p->AddressType != STI2C_ADDRESS_7_BITS))
    {
        return(ST_ERROR_BAD_PARAMETER) ;
    }

#endif
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    STOS_TaskUnlock();

    /* Take global atomic */
    STOS_SemaphoreWait(g_DriverAtomic_p);
    /* Set up the pointers to the open block  */
    OpenBlkPtr_p = (i2c_open_block_t*)Handle;
    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
         /* Release Busaccess Semaphore */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return ST_ERROR_INVALID_HANDLE;
    }
    /* Take CtrlBlkPtr and I2cRegptr */
    CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;

    /* Copy the parameters from user defined structure to open block  */
    /*memcpy( &OpenBlkPtr_p->OpenParams,SetParams_p, sizeof(STI2C_OpenParams_t));*/
    OpenBlkPtr_p->OpenParams.I2cAddress        = SetParams_p->I2cAddress;
    OpenBlkPtr_p->OpenParams.AddressType       = SetParams_p->AddressType;
    OpenBlkPtr_p->OpenParams.BusAccessTimeOut  = SetParams_p->BusAccessTimeOut;
#ifdef STI2C_ADAPTIVE_BAUDRATE
    if ( (SetParams_p->BaudRate == STI2C_RATE_NORMAL) || (SetParams_p->BaudRate == STI2C_RATE_FASTMODE) )
    {
        OpenBlkPtr_p->OpenParams.BaudRate      = SetParams_p->BaudRate;
    }
    else
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    OpenBlkPtr_p->OpenParams.BaudRate          = CtrlBlkPtr_p->InitParams.BaudRate;
#endif
    /* Release the access semaphore */
    STOS_SemaphoreSignal(g_DriverAtomic_p);
    return( ErrorCode );

}
/*****************************************************************************
STI2C_Read
Description:
    Called to initiate a read on the i2c bus
Parameters  : 1) A handle to the i2c device being read from
              2) A pointer to the buffer being read into
              3) The number of bytes to read
              4) The timeout for the whole transfer
              5) A pointer to an empty U32 which will have the actual number
                 of bytes read when the read completes.
Return Value:
    ST_NO_ERROR,                 no errors.
    ST_ERROR_INVALID_HANDLE      Device handle invalid.
    STI2C_ERROR_LINE_STATE       The data and clock signals are held low
    ST_ERROR_DEVICE_BUSY         The I2C device is in use by another application
    ST_ERROR_TIMEOUT             Time-out on read operation
    STI2C_ERROR_STATUS           Error in device status register
    STI2C_ERROR_ADDRESS_ACK      No acknowledge from remote device to addressing
    ST_ERROR_UNKNOWN_DEVICE      Device has not been initialized
    STI2C_ERROR_BUS_IN_USE       The I2C bus is in use by another Master device

See Also:
   STI2C_Write()
*****************************************************************************/

ST_ErrorCode_t STI2C_Read (   STI2C_Handle_t Handle,
                              U8             *Buffer_p,
                              U32            MaxLen,
                              U32            Timeout,
                              U32            *ActLen_p)
{

    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    ErrorCode = BusAccess (Handle, Buffer_p, MaxLen,
                           Timeout, ActLen_p, READING, TRUE);

    return( ErrorCode );

}
/*****************************************************************************
STI2C_Write
Description:
    Called to initiate a write on the i2c bus
Parameters  : 1) A handle to the i2c device being written to
              2) A pointer to the buffer being written to
              3) The number of bytes to write
              4) The timeout for the whole transfer
              5) A pointer to an empty U32 which will have the actual number
                 of bytes written when the write completes.
Return Value:
    ST_NO_ERROR,                 no errors.
    ST_ERROR_INVALID_HANDLE      Device handle invalid.
    STI2C_ERROR_LINE_STATE       The data and clock signals are held low
    ST_ERROR_DEVICE_BUSY         The I2C device is in use by another application
    ST_ERROR_TIMEOUT             Time-out on read operation
    STI2C_ERROR_STATUS           Error in device status register
    STI2C_ERROR_ADDRESS_ACK      No acknowledge from remote device to addressing
    STI2C_ERROR_WRITE_ACK        No acknowledge from remote device to data write
    ST_ERROR_UNKNOWN_DEVICE      Device has not been initialized
    STI2C_ERROR_BUS_IN_USE       The I2C bus is in use by another Master device

See Also:
   STI2C_Write()
*****************************************************************************/

ST_ErrorCode_t STI2C_Write(   STI2C_Handle_t Handle,
                              const U8       *Buffer_p,
                              U32            NumberToWrite,
                              U32            Timeout,
                              U32            *ActLen_p)
{

    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    ErrorCode = BusAccess (Handle,(U8 *)Buffer_p, NumberToWrite,
                            Timeout, ActLen_p, WRITING, TRUE);
    return( ErrorCode );
 }
/*****************************************************************************
STI2C_ReadNoStop
Description:
              Called to initiate a read on the i2c bus, with no STOP.
              After a successful call to this routine the bus is locked
              to the Handle used until either STI2C_Read() or STI2C_Write()
              are called.
Parameters  : 1) A handle to the i2c device being read from
              2) A pointer to the buffer being read into
              3) The number of bytes to read
              4) The timeout for the whole transfer
              5) A pointer to an empty U32 which will have the actual number
                 of bytes read when the read completes.
Return Value:
    ST_NO_ERROR,                 no errors.
    ST_ERROR_INVALID_HANDLE      Device handle invalid.
    STI2C_ERROR_LINE_STATE       The data and clock signals are held low
    ST_ERROR_DEVICE_BUSY         The I2C device is in use by another application
    ST_ERROR_TIMEOUT             Time-out on read operation
    STI2C_ERROR_STATUS           Error in device status register
    STI2C_ERROR_ADDRESS_ACK      No acknowledge from remote device to addressing
    ST_ERROR_UNKNOWN_DEVICE      Device has not been initialized
    STI2C_ERROR_BUS_IN_USE       The I2C bus is in use by another Master device

See Also:
   STI2C_Read()
   STI2C_Write()
*****************************************************************************/
ST_ErrorCode_t STI2C_ReadNoStop (    STI2C_Handle_t Handle,
                                     U8             *Buffer_p,
                                     U32            MaxLen,
                                     U32            Timeout,
                                     U32            *ActLen_p)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
#ifndef STI2C_NO_PARAM_CHECK
    if ((Buffer_p == NULL) || (ActLen_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

#endif
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    ErrorCode = BusAccess (Handle, Buffer_p, MaxLen,
                            Timeout, ActLen_p, READING, FALSE);

    return( ErrorCode );

}

/*****************************************************************************
STI2C_WriteNoStop
Description:
              Called to initiate a write on the i2c bus, with no STOP.
              After a successful call to this routine the bus is locked
              to the Handle used until either STI2C_Read() or STI2C_Write()
              are called.
Parameters  : 1) A handle to the i2c device being written to
              2) A pointer to the buffer being written to
              3) The number of bytes to write
              4) The timeout for the whole transfer
              5) A pointer to an empty U32 which will have the actual number
                 of bytes written when the write completes.
Return Value:
    ST_NO_ERROR,                 no errors.
    ST_ERROR_INVALID_HANDLE      Device handle invalid.
    STI2C_ERROR_LINE_STATE       The data and clock signals are held low
    ST_ERROR_DEVICE_BUSY         The I2C device is in use by another application
    ST_ERROR_TIMEOUT             Time-out on read operation
    STI2C_ERROR_STATUS           Error in device status register
    STI2C_ERROR_ADDRESS_ACK      No acknowledge from remote device to addressing
    STI2C_ERROR_WRITE_ACK        No acknowledge from remote device to data write
    ST_ERROR_UNKNOWN_DEVICE      Device has not been initialized
    STI2C_ERROR_BUS_IN_USE       The I2C bus is in use by another Master device

See Also:
   STI2C_Read()
   STI2C_Write()
*****************************************************************************/

ST_ErrorCode_t STI2C_WriteNoStop (   STI2C_Handle_t    Handle,
                                     const U8          *Buffer_p,
                                     U32               NumberToWrite,
                                     U32               Timeout,
                                     U32               *ActLen_p)
{

    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    ErrorCode = BusAccess (Handle,(U8 *)Buffer_p, NumberToWrite,
                           Timeout, ActLen_p, WRITING, FALSE);
    return( ErrorCode );
}

/************************************************************
BusAccess
Description: This function will initiates the transference
             of data across the bus.
************************************************************/
static ST_ErrorCode_t BusAccess( STI2C_Handle_t     Handle,
                                  U8                *Buffer_p,
                                  U32               BufferLen,
                                  U32               TimeOutMS,
                                  U32               *ActLen_p,
                                  i2c_context_t     Context,
                                  BOOL              DoStop  )
{

    const U32   TicksPerSec = ST_GetClocksPerSecond();

    DU32                *I2cRegPtr;
    /* BusTimeOut varible have been added to use
       relative time out in STOS_TaskDelay. Please
       see DDTS GNBvd31571 for more details
    */
    clock_t             BusTimeOut, *pBusTimeOut_p;
    clock_t             *pTransferTimeOut,TimeOut;
    ST_ErrorCode_t      ReturnCode  = ST_NO_ERROR;
    i2c_param_block_t   *CtrlBlkPtr_p;
    i2c_open_block_t    *OpenBlkPtr_p;
    S32                 SemReturn;
    U32                 Address,Status,TxData,I2cValues,Ctr=0;
#ifdef STI2C_DEBUG_BUS_STATE
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    ST_ErrorCode_t      PIOReturnCode = ST_NO_ERROR;
    STPIO_BitConfig_t   BitConfig[8];
    U8                  PIOValueOld,PIOValueNew;
#endif
    U32                 HandleIndex=0;

    if ((Buffer_p == NULL) || (ActLen_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER) ;
    }
    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    STOS_TaskUnlock();

    /* Set up the pointers to the open block  */
    OpenBlkPtr_p = (i2c_open_block_t*)Handle;
    TimeOut=OpenBlkPtr_p->OpenParams.BusAccessTimeOut;
    /* Set bus access timeout */
    pBusTimeOut_p = ConvertMStoTicks (TimeOut, TicksPerSec,
                                     &BusTimeOut );

    if ((clock_t *)pBusTimeOut_p != TIMEOUT_INFINITY)
    {
        *pBusTimeOut_p = STOS_time_plus (*pBusTimeOut_p, STOS_time_now());
    }
    /* Take global atomic */
    SemReturn = STOS_SemaphoreWaitTimeOut(g_DriverAtomic_p,(clock_t *)pBusTimeOut_p);
    if( SemReturn != 0 )
    {
        return ST_ERROR_TIMEOUT;
    }
    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
        /* Return error */
        return ST_ERROR_INVALID_HANDLE;
    }
    /* Take CtrlBlkPtr and I2cRegptr */
    CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;
    I2cRegPtr = (DU32*) CtrlBlkPtr_p->InitParams.BaseAddress;
    Status = I2cRegPtr[SSC_STATUS];
    /* Check bus access has not been claimed already */
    SemReturn = STOS_SemaphoreWaitTimeOut( CtrlBlkPtr_p->BusAccessSemaphore_p,
                                            (clock_t *)pBusTimeOut_p );
    if (SemReturn != 0)
    {
        STOS_SemaphoreSignal(g_DriverAtomic_p);
#if defined(STI2C_TRACE)
        QueryTrace();
#endif

        /* Failed to claim bus access */
        return ST_ERROR_DEVICE_BUSY;
    }
    else
    {
        /* check if the bus has been locked to this handle previously by using lock function*/
        for(HandleIndex=0; HandleIndex<CtrlBlkPtr_p->InitParams.MaxHandles; HandleIndex++)
        {
            if((CtrlBlkPtr_p->LockedHandlePool_p[HandleIndex] == Handle)
                &&(CtrlBlkPtr_p->LastTransferHadStop == TRUE))
            {
                CtrlBlkPtr_p->LockedHandleCurrent = Handle;
                break;
            }
        }
        /* Check if we already locked the bus, or it's free (0) */
        if ( ((CtrlBlkPtr_p->LockedHandlePool_p[0] == 0)&&(CtrlBlkPtr_p->LastTransferHadStop == TRUE))
           || (CtrlBlkPtr_p->LockedHandleCurrent == Handle))
        {
            /* SUCCESS: I2C bus is now locked to this handle */
            CtrlBlkPtr_p->LockedHandleCurrent = Handle;
        }
        else
        {
            /* Locked by someone else; set code, release semaphore */
            STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
            /* Release global Semaphore so that other driver function can run */
            STOS_SemaphoreSignal(g_DriverAtomic_p);
            return ST_ERROR_DEVICE_BUSY;
        }
    }
    /* Release global Semaphore so that other driver function can run */
    STOS_SemaphoreSignal(g_DriverAtomic_p);

    /* Set I2C Address from open Block*/
    Address = OpenBlkPtr_p->OpenParams.I2cAddress;
    I2C_TRACE_LOCK(CtrlBlkPtr_p,0xB000,Address);
    /* Turn off I2C interrupts while we set up control structure */
    WriteReg(I2cRegPtr, SSC_INT_ENABLE, 0);
    /* Tidy up the IOSemaphore as it may be out of sync. Note the routine
       can exit with spurious events still on the semaphore!
    */
    while (STOS_SemaphoreWaitTimeOut (CtrlBlkPtr_p->IOSemaphore_p,
                                       TIMEOUT_IMMEDIATE) == 0)
    {}
    if (BusStuck (CtrlBlkPtr_p))
    {
        I2C_TRACE_LOCK(CtrlBlkPtr_p,0xBF20,0);
        /* Spurious pulses were setting the busy bit on 5528 board , soft reset is done
        to clear the busy bit.This may not work in multi-master environment.*/
        while( BusStuck (CtrlBlkPtr_p)  && (Ctr < STOP_BUSY_WAIT) )
        {
            Ctr++;
        }
        WriteReg(I2cRegPtr, SSC_TX_BUFFER, 0x1ff);
        WriteReg(I2cRegPtr, SSC_CONTROL, SSC_CON_RESET);
        if (BusStuck (CtrlBlkPtr_p))
        {
            /* BusTimeout exceeded device still busy return error */
            STI2C_Print(("\nEntered in bus stuck\n"));
#ifndef STI2C_MASTER_ONLY
            STEVT_Notify (CtrlBlkPtr_p->EVTHandle, CtrlBlkPtr_p->BusStuckId, NULL);
#endif
            I2C_TRACE_LOCK(CtrlBlkPtr_p,0xBF21,0);
            CtrlBlkPtr_p->LockedHandleCurrent = 0;
            STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);

#ifndef STI2C_DEBUG_BUS_STATE
            return ST_ERROR_DEVICE_BUSY;
#else
            BitConfig[CtrlBlkPtr_p->InitParams.PIOforSDA.BitMask] = STPIO_BIT_BIDIRECTIONAL;
            BitConfig[CtrlBlkPtr_p->InitParams.PIOforSCL.BitMask] = STPIO_BIT_BIDIRECTIONAL;

            PIOReturnCode = STPIO_SetConfig (CtrlBlkPtr_p->PIOHandle,BitConfig);

            PIOReturnCode = STPIO_Read(CtrlBlkPtr_p->PIOHandle,&PIOValueNew);
            if(PIOReturnCode!=ST_NO_ERROR)
            {
                ReturnCode = STI2C_ERROR_PIO;
            }
            else
            {
                if(!(PIOValueNew & CtrlBlkPtr_p->InitParams.PIOforSDA.BitMask))
                {
                    ReturnCode = STI2C_ERROR_SDA_LOW_SLAVE;
                }
                else
                {
                    if(!(PIOValueNew & CtrlBlkPtr_p->InitParams.PIOforSCL.BitMask))
                    {
                        ReturnCode = STI2C_ERROR_SCL_LOW_SLAVE;
                    }
                    else
                    {
                        ReturnCode = STI2C_ERROR_FALSE_START;
                    }
                }
            }
            BitConfig[CtrlBlkPtr_p->InitParams.PIOforSDA.BitMask] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
            BitConfig[CtrlBlkPtr_p->InitParams.PIOforSCL.BitMask] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;

            PIOReturnCode = STPIO_SetConfig (CtrlBlkPtr_p->PIOHandle,BitConfig);
            return(ReturnCode);
#endif /* End of BUS STUCK*/
        }
    }

    /* If in Slave state, reenable interrupts & abort this attempt */
    if (CtrlBlkPtr_p->State != STI2C_IDLE)
    {
        WriteReg(I2cRegPtr, SSC_INT_ENABLE,
        CtrlBlkPtr_p->InterruptMask);
        I2C_TRACE_LOCK(CtrlBlkPtr_p,0xBF10,0);
        CtrlBlkPtr_p->LockedHandleCurrent = 0;
        STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
        return STI2C_ERROR_BUS_IN_USE;
    }
#if defined(STI2C_STSPI_SAME_SSC_SUPPORT)
    if(CtrlBlkPtr_p->LastTransferHadStop == TRUE)
    /* Get SSC to start operation */
    {
        ReturnCode = I2c_GetSSC(CtrlBlkPtr_p);
        if(ReturnCode != ST_NO_ERROR)
        {
            CtrlBlkPtr_p->LockedHandleCurrent = 0;
            STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
            return ReturnCode;
        }
    }
#endif
    CtrlBlkPtr_p->InitParams.BaudRate = OpenBlkPtr_p->OpenParams.BaudRate;
#if defined(SSC3_WITH_TIMING_PRESENT)
    /* Program I2C timing register values as per baudrate */
    i2c_WriteTimingValues(CtrlBlkPtr_p);
#endif
    WriteReg(I2cRegPtr, SSC_I2C,
           SSC_I2C_GENERATE_ACKS | SSC_I2C_ENABLE);
    if (CtrlBlkPtr_p->InitParams.BaudRate == STI2C_RATE_FASTMODE)
    {
        WriteReg(I2cRegPtr, SSC_I2C,
                 I2cRegPtr[SSC_I2C] | SSC_I2C_FASTMODE);
    }
    /* Setup the control structure */
    CtrlBlkPtr_p->Buffer_p          = Buffer_p;
    CtrlBlkPtr_p->BufferLen         = BufferLen;
    CtrlBlkPtr_p->BufferCnt         = 0;
    CtrlBlkPtr_p->HandlerCondition  = ST_NO_ERROR;
    if (Context == WRITING)
    {
        CtrlBlkPtr_p->State = STI2C_MASTER_ADDRESSING_WRITE;
        /* put ack in LSB of tx char */
        TxData = (Address << 1) | 1;
    }
    else  /* Context == READING */
    {
        CtrlBlkPtr_p->State = STI2C_MASTER_ADDRESSING_READ;
        /* put ack in LSB of tx char & set bottom bit
           of address for Read */
        TxData = (Address << 1) | 3;
    }
    I2C_TRACE_LOCK(CtrlBlkPtr_p,0xB010, I2cRegPtr[SSC_CONTROL]);

    /* Do a quick reset */
    if (CtrlBlkPtr_p->LastTransferHadStop)
    {
        /* Enable Master mode bearing in mind the soft reset restriction*/
        WriteReg(I2cRegPtr, SSC_CONTROL,
          DEFAULT_CONTROL | SSC_CON_MASTER_SELECT | SSC_CON_RESET);
        WriteReg(I2cRegPtr, SSC_CONTROL,
          DEFAULT_CONTROL | SSC_CON_MASTER_SELECT);
    }
    /* Clear all status bits, and make sure control has
      master-select  */
    WriteReg(I2cRegPtr, SSC_CLR, SSC_CLR_MASK);

    WriteReg(I2cRegPtr, SSC_CONTROL,
               I2cRegPtr[SSC_CONTROL] | SSC_CON_MASTER_SELECT);
    STOS_TaskLock();
    I2C_TRACE_LOCK(CtrlBlkPtr_p,0xB030,TxData);
     /* Enable Appropriate I2C interrupts */
    CtrlBlkPtr_p->InterruptMask = SSC_IEN_TX_ERROR  |
                  SSC_IEN_STOP | SSC_IEN_AAS | SSC_IEN_ARBL;

    /* If doing a repeated start, enable the repeatstart interrupt */
    if (CtrlBlkPtr_p->LastTransferHadStop == FALSE)
    {
        CtrlBlkPtr_p->InterruptMask |= SSC_IEN_REPSTRT;
        CtrlBlkPtr_p->InterruptMask &= ~SSC_IEN_TX_ERROR;
    }
    WriteReg(I2cRegPtr, SSC_INT_ENABLE,
              CtrlBlkPtr_p->InterruptMask);
    /* Set the baud-rate to use */
    WriteReg(I2cRegPtr, SSC_BAUD_RATE,
             CtrlBlkPtr_p->InitParams.ClockFrequency / (2 * CtrlBlkPtr_p->InitParams.BaudRate));

    if (CtrlBlkPtr_p->LastTransferHadStop == TRUE)
    {
        WriteReg(I2cRegPtr, SSC_TX_BUFFER, TxData);
        Status = ReadReg(I2cRegPtr, SSC_STATUS);
        I2C_TRACE_LOCK(CtrlBlkPtr_p,0xB040,Status);
    }
    else
    {
        /* put first byte in control block varible this
        will be loaded by ISR after the repeated start
        occours*/
        CtrlBlkPtr_p->DataToSend = TxData;
    }
    /* Convert transfer timeout in ms to ticks */
    pTransferTimeOut = ConvertMStoTicks (TimeOutMS, TicksPerSec,
                                          &TimeOut );

    /* Derive timeout, if specified timeout not infinity */
    if ((clock_t *)pTransferTimeOut != TIMEOUT_INFINITY)
    {
        *pTransferTimeOut = STOS_time_plus (*pTransferTimeOut, STOS_time_now());
    }

    /* Calculate and set values for SSCI2C */
    I2cValues = I2cRegPtr[SSC_I2C] & 0xfff;
    I2cValues |= SSC_I2C_ENABLE |
                  SSC_I2C_GENERATE_ACKS |
                   SSC_I2C_TXENABLED;
    /* generate the start condition on the bus*/
    if (CtrlBlkPtr_p->LastTransferHadStop == FALSE)
    {
        I2cValues |= SSC_I2C_REPSTRTG;
    }
    else
    {
        I2cValues |= SSC_I2C_GENERATE_START;
    }

    /* This kicks off the transfer */
    WriteReg(I2cRegPtr, SSC_I2C, I2cValues);
    I2C_TRACE_LOCK(CtrlBlkPtr_p,0xB020,I2cValues);
    STOS_TaskUnlock();

    /* Now wait for transfer to complete */
    SemReturn = STOS_SemaphoreWaitTimeOut(
                 CtrlBlkPtr_p->IOSemaphore_p,(clock_t *)pTransferTimeOut);

    /* Return to default Enable interrupts i.e. strip off ARBL, RXFULL etc. */

    CtrlBlkPtr_p->InterruptMask  =   DEFAULT_ENABLES ;
    CtrlBlkPtr_p->InterruptMask &=   ~SSC_IEN_TX_ERROR;

    WriteReg(I2cRegPtr, SSC_INT_ENABLE,CtrlBlkPtr_p->InterruptMask);

    I2C_TRACE_LOCK(CtrlBlkPtr_p,0xB070,SemReturn);

    /* Test semaphore for timeout */
    if (SemReturn != 0)
    {
       /* Force the driver to ideal mode*/
        CtrlBlkPtr_p->State = STI2C_IDLE;
        ReturnCode = ST_ERROR_TIMEOUT;
    }

    /* If a handler error has occurred return it */
    if (( CtrlBlkPtr_p->HandlerCondition != ST_NO_ERROR ) &&
        (ReturnCode == ST_NO_ERROR))
    {
        ReturnCode = CtrlBlkPtr_p->HandlerCondition;
    }

    /* Check if stop condition is to be generated or not */
    if (DoStop)
    {
        I2cStop(CtrlBlkPtr_p);
#ifdef STI2C_DEBUG_BUS_STATE
        if (ReadReg(I2cRegPtr, SSC_STATUS) & SSC_STAT_BUSY)
        {
            PIOReturnCode = STPIO_Read(CtrlBlkPtr_p->PIOHandle,&PIOValueOld);

            BitConfig[CtrlBlkPtr_p->InitParams.PIOforSDA.BitMask] = STPIO_BIT_BIDIRECTIONAL;
            BitConfig[CtrlBlkPtr_p->InitParams.PIOforSCL.BitMask] = STPIO_BIT_BIDIRECTIONAL;

            PIOReturnCode = STPIO_SetConfig (CtrlBlkPtr_p->PIOHandle,BitConfig);

            PIOReturnCode = STPIO_Read(CtrlBlkPtr_p->PIOHandle,&PIOValueNew);
            if (PIOReturnCode!=ST_NO_ERROR)
            {
                ErrorCode = STI2C_ERROR_PIO;
            }
            else
            {
                if (!(PIOValueNew & CtrlBlkPtr_p->InitParams.PIOforSDA.BitMask))
                {
                    ErrorCode = STI2C_ERROR_SDA_LOW_SLAVE;
                }
                else
                {
                    if (!(PIOValueNew & CtrlBlkPtr_p->InitParams.PIOforSCL.BitMask))
                    {
                        ErrorCode = STI2C_ERROR_SCL_LOW_SLAVE;
                    }
                    else
                    {
                        if (!(PIOValueOld & CtrlBlkPtr_p->InitParams.PIOforSDA.BitMask))
                        {
                            ErrorCode = STI2C_ERROR_SDA_LOW_MASTER;
                        }
                        else
                        {
                            if (!(PIOValueOld & CtrlBlkPtr_p->InitParams.PIOforSCL.BitMask))
                            {
                                ErrorCode = STI2C_ERROR_SCL_LOW_MASTER;
                            }
                        }
                    }
                }
            }
            /* Restore the bus to its original state */
            BitConfig[CtrlBlkPtr_p->InitParams.PIOforSDA.BitMask] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
            BitConfig[CtrlBlkPtr_p->InitParams.PIOforSCL.BitMask] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;

            PIOReturnCode = STPIO_SetConfig (CtrlBlkPtr_p->PIOHandle,BitConfig);
        }
        if ((ReturnCode == ST_NO_ERROR)||(ReturnCode == ST_ERROR_TIMEOUT))
        {
            ReturnCode = ErrorCode;
        }
#endif
        CtrlBlkPtr_p->InterruptMask = DEFAULT_ENABLES;
        WriteReg(I2cRegPtr, SSC_INT_ENABLE,CtrlBlkPtr_p->InterruptMask);

        CtrlBlkPtr_p->LockedHandleCurrent = 0;
        /* Clear the BRG so it doesn't interfere with master.*/
        WriteReg(I2cRegPtr, SSC_BAUD_RATE, 0);
        CtrlBlkPtr_p->LastTransferHadStop = TRUE;
        WriteReg(I2cRegPtr, SSC_CONTROL,
        I2cRegPtr[SSC_CONTROL] & ~SSC_CON_MASTER_SELECT);
    }
    else
    {
        if(ReturnCode != ST_NO_ERROR)
        {
            I2cStop(CtrlBlkPtr_p);
            CtrlBlkPtr_p->LockedHandleCurrent = 0;
            CtrlBlkPtr_p->LastTransferHadStop = TRUE;
            CtrlBlkPtr_p->InterruptMask = DEFAULT_ENABLES;
            WriteReg(I2cRegPtr, SSC_INT_ENABLE,CtrlBlkPtr_p->InterruptMask);
            /* Clear the BRG so it doesn't interfere with master.*/
            WriteReg(I2cRegPtr, SSC_BAUD_RATE, 0);
            WriteReg(I2cRegPtr, SSC_CONTROL,
            I2cRegPtr[SSC_CONTROL] & ~SSC_CON_MASTER_SELECT);
        }
        else
        {
            CtrlBlkPtr_p->LastTransferHadStop = FALSE;
        }
    }
    I2C_TRACE_LOCK(CtrlBlkPtr_p,0xB081,SemReturn);

    /* See how many bytes were transferred */
    *ActLen_p = CtrlBlkPtr_p->BufferCnt;
#if defined(STI2C_STSPI_SAME_SSC_SUPPORT)
    if(CtrlBlkPtr_p->LastTransferHadStop == TRUE)
    /* Free SSC to stop operation */
    {
        I2c_FreeSSC(CtrlBlkPtr_p);
    }
#endif
    STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);

    /*
    This delay is required to remove any dead lock for BusAccessSemaphore
    as without this delay STI2C driver was causing some problem with
    Multi-instance of TUNER driver
    */
    STOS_TaskDelay(0);

    return(ReturnCode);
}

/*****************************************************************************
STI2C_Lock
Description:
              Tries to lock the bus that the handle is on. If the bus is
              currently in use, WaitForLock determines whether the caller
              would like to wait for the lock.

Parameters  :
              Handle          The handle trying to lock the bus
              WaitForLock     Whether the caller would like to wait for the lock
Return code :
              ST_NO_ERROR
              ST_ERROR_INVALID_HANDLE     - Invalid handle
              ST_ERROR_DEVICE_BUSY        - Device in use, caller not blocking

See Also:
   STI2C_ReadNoStop()
   STI2C_Unlock()
   STI2C_WriteNoStop()
*****************************************************************************/

ST_ErrorCode_t STI2C_Lock(STI2C_Handle_t    Handle,
                          BOOL              WaitForLock)

{
    U32                  SemReturn;
    i2c_param_block_t    *CtrlBlkPtr_p = NULL;
    i2c_open_block_t     *OpenBlkPtr_p = NULL;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;

    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    STOS_TaskUnlock();
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    while(1)
    {
        /* Take global atomic */
        STOS_SemaphoreWait(g_DriverAtomic_p);
        OpenBlkPtr_p = (i2c_open_block_t*)Handle;

        if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
        {
            /* Release Busaccess Semaphore */
            STOS_SemaphoreSignal(g_DriverAtomic_p);
            return ST_ERROR_INVALID_HANDLE;
        }
        CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;
        /* Test if I2C bus has already been locked */
        if (WaitForLock == TRUE)
        {
            SemReturn = STOS_SemaphoreWait(CtrlBlkPtr_p->BusAccessSemaphore_p);
        }
        else
        {
            SemReturn = STOS_SemaphoreWaitTimeOut(CtrlBlkPtr_p->BusAccessSemaphore_p,
                                                    TIMEOUT_IMMEDIATE);
        }
        if( SemReturn == 0 )
        {
            if ( ((CtrlBlkPtr_p->LockedHandlePool_p[0] == 0) && (CtrlBlkPtr_p->LockedHandleCurrent == 0)) ||
                 ((CtrlBlkPtr_p->LockedHandlePool_p[0] == Handle) && (CtrlBlkPtr_p->LockedHandlePool_p[1] == 0)) )
            {
                CtrlBlkPtr_p->LockedHandlePool_p[0] = Handle;
                ReturnCode = ST_NO_ERROR;
            }
            else
            {
                ReturnCode = ST_ERROR_DEVICE_BUSY;
            }
            /* Release the BusAccess semaphore */
            STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
            /* changed global sem signal location
               please see GNBvd33415 for more details*/
            STOS_SemaphoreSignal(g_DriverAtomic_p);
        }
        else
        {
            /* Release global Semaphore */
            STOS_SemaphoreSignal(g_DriverAtomic_p);
            ReturnCode = ST_ERROR_DEVICE_BUSY;
        }
        if ((WaitForLock == FALSE) ||
            (CtrlBlkPtr_p->LockedHandlePool_p[0] == Handle))
        {
            break;
        }

        /* Try every tenth of a second */
        STOS_TaskDelay( (ST_GetClocksPerSecond()/1000) * STI2C_LOCK_DELAY);

    }
    return(ReturnCode);
}
/*****************************************************************************
STI2C_Unlock
Description:
             Reverses a call to STI2C_Lock().

Parameters  :
              The handle releasing the bus
Return code :
              ST_NO_ERROR
              ST_ERROR_INVALID_HANDLE     - Invalid handle
              ST_ERROR_DEVICE_BUSY        - Device in use, caller not blocking
              ST_ERROR_UNKNOWN_DEVICE     - Device has not been initialized
              ST_ERROR_BAD_PARAMETER      - The caller is not the owner of the lock.
See Also:
   STI2C_ReadNoStop()
   STI2C_Unlock()
   STI2C_WriteNoStop()
*****************************************************************************/

ST_ErrorCode_t STI2C_Unlock(STI2C_Handle_t Handle)
{
    i2c_param_block_t    *CtrlBlkPtr_p = NULL;
    i2c_open_block_t     *OpenBlkPtr_p = NULL;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;

    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    STOS_TaskUnlock();
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Take global atomic */
    STOS_SemaphoreWait(g_DriverAtomic_p);
    OpenBlkPtr_p = (i2c_open_block_t*)Handle;

    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
        /* Release Busaccess Semaphore */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return ST_ERROR_INVALID_HANDLE;
    }
    CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;
    /* Test if I2C bus has already been locked */
    STOS_SemaphoreWait(CtrlBlkPtr_p->BusAccessSemaphore_p);

    /* We got the handle lock */
    if( CtrlBlkPtr_p->LockedHandlePool_p[0] == Handle )
    {
        /* SUCCESS: This handle already has the bus locked so release */
        CtrlBlkPtr_p->LockedHandlePool_p[0] = 0;
        ReturnCode = ST_NO_ERROR;
    }
    else
    {
        ReturnCode = ST_ERROR_BAD_PARAMETER;
    }
    /* Release the BusAccess semaphore */
    STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
    /* changed global sem signal location
    please see GNBvd38020 for more details
    */
    /* Release global Semaphore so that other function call can take place */
    STOS_SemaphoreSignal(g_DriverAtomic_p);
    return(ReturnCode);

}

/*****************************************************************************
STI2C_LockMultiple
Description:
              Tries to lock the bus that the handle is on. If the bus is
              currently in use, WaitForLock determines whether the caller
              would like to wait for the lock.

Parameters  :
              Handle          The handle trying to lock the bus
              WaitForLock     Whether the caller would like to wait for the lock
Return code :
              ST_NO_ERROR
              ST_ERROR_INVALID_HANDLE     - Invalid handle
              ST_ERROR_DEVICE_BUSY        - Device in use, caller not blocking

See Also:
   STI2C_ReadNoStop()
   STI2C_Unlock()
   STI2C_WriteNoStop()
*****************************************************************************/

ST_ErrorCode_t STI2C_LockMultiple(STI2C_Handle_t    *HandlePool_p,
                                  U32               NumberOfHandles,
                                  BOOL              WaitForLock)
{
    U32                  SemReturn;
    i2c_param_block_t    *CtrlBlkPtr_p = NULL;
    i2c_open_block_t     *OpenBlkPtr_p = NULL;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;
    U32                  HandleIndex=0;

    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    for(HandleIndex=0;HandleIndex<NumberOfHandles;HandleIndex++)
    {
        if ((i2c_open_block_t*)HandlePool_p[HandleIndex] == NULL)
        {
            STOS_TaskUnlock();
            return ST_ERROR_INVALID_HANDLE;
        }
    }
    STOS_TaskUnlock();

    while(1)
    {
        /* Take global atomic */
        STOS_SemaphoreWait(g_DriverAtomic_p);
        OpenBlkPtr_p = (i2c_open_block_t*)HandlePool_p[0];

        for(HandleIndex=0;HandleIndex<NumberOfHandles;HandleIndex++)
        {
            if (((i2c_open_block_t*)(HandlePool_p[HandleIndex]))->MagicNumber != MAGIC_NUM)
            {
                /* Release Busaccess Semaphore */
                STOS_SemaphoreSignal(g_DriverAtomic_p);
                return ST_ERROR_INVALID_HANDLE;
            }
            /* Handles should be on the same bus */
            if (OpenBlkPtr_p->CtrlBlkPtr != ((i2c_open_block_t*)HandlePool_p[HandleIndex])->CtrlBlkPtr)
            {
                /* Release Busaccess Semaphore */
                STOS_SemaphoreSignal(g_DriverAtomic_p);
                return ST_ERROR_INVALID_HANDLE;
            }
        }

        CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;
        /* Test if I2C bus has already been locked */
        if (WaitForLock == TRUE)
        {
            SemReturn = STOS_SemaphoreWait(CtrlBlkPtr_p->BusAccessSemaphore_p);
        }
        else
        {
            SemReturn = STOS_SemaphoreWaitTimeOut(CtrlBlkPtr_p->BusAccessSemaphore_p,
                                                    TIMEOUT_IMMEDIATE);
        }
        if( SemReturn == 0 )
        {
            for(HandleIndex=0;HandleIndex<NumberOfHandles;HandleIndex++)
            {
                if ((CtrlBlkPtr_p->LockedHandlePool_p[HandleIndex] == 0)
                 && (CtrlBlkPtr_p->LastTransferHadStop == TRUE))
                {
                    CtrlBlkPtr_p->LockedHandlePool_p[HandleIndex] = HandlePool_p[HandleIndex];
                    ReturnCode = ST_NO_ERROR;
                }
                else
                {
                    ReturnCode = ST_ERROR_DEVICE_BUSY;
                    break;
                }
            }
            /* Release the BusAccess semaphore */
            STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
            /* changed global sem signal location
               please see GNBvd33415 for more details*/
            STOS_SemaphoreSignal(g_DriverAtomic_p);
        }
        else
        {
            /* Release global Semaphore */
            STOS_SemaphoreSignal(g_DriverAtomic_p);
            ReturnCode = ST_ERROR_DEVICE_BUSY;
        }
        if ((WaitForLock == FALSE) ||
            (memcmp(CtrlBlkPtr_p->LockedHandlePool_p,HandlePool_p,NumberOfHandles*sizeof(STI2C_Handle_t))==0))
        {
            break;
        }

        /* Try every tenth of a second */
        STOS_TaskDelay( (ST_GetClocksPerSecond()/1000) * STI2C_LOCK_DELAY);

    }
    return(ReturnCode);
}
/*****************************************************************************
STI2C_UnlockMultiple
Description:
             Reverses a call to STI2C_Lock().

Parameters  :
              The handle releasing the bus
Return code :
              ST_NO_ERROR
              ST_ERROR_INVALID_HANDLE     - Invalid handle
              ST_ERROR_DEVICE_BUSY        - Device in use, caller not blocking
              ST_ERROR_UNKNOWN_DEVICE     - Device has not been initialized
              ST_ERROR_BAD_PARAMETER      - The caller is not the owner of the lock.
See Also:
   STI2C_ReadNoStop()
   STI2C_Unlock()
   STI2C_WriteNoStop()
*****************************************************************************/

ST_ErrorCode_t STI2C_UnlockMultiple(STI2C_Handle_t *HandlePool_p,
                                    U32            NumberOfHandles)
{
    i2c_param_block_t    *CtrlBlkPtr_p = NULL;
    i2c_open_block_t     *OpenBlkPtr_p = NULL;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;
    U32                  HandleIndex=0;

    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    for(HandleIndex=0;HandleIndex<NumberOfHandles;HandleIndex++)
    {
        if ((i2c_open_block_t*)HandlePool_p[HandleIndex] == NULL)
        {
            STOS_TaskUnlock();
            return ST_ERROR_INVALID_HANDLE;
        }
    }
    STOS_TaskUnlock();

    /* Take global atomic */
    STOS_SemaphoreWait(g_DriverAtomic_p);
    OpenBlkPtr_p = (i2c_open_block_t*)HandlePool_p[0];

    for(HandleIndex=0;HandleIndex<NumberOfHandles;HandleIndex++)
    {
        if (((i2c_open_block_t*)HandlePool_p[HandleIndex])->MagicNumber != MAGIC_NUM)
        {
            /* Release Busaccess Semaphore */
            STOS_SemaphoreSignal(g_DriverAtomic_p);
            return ST_ERROR_INVALID_HANDLE;
        }
        /* Handles should be on the same bus */
        if (OpenBlkPtr_p->CtrlBlkPtr != ((i2c_open_block_t*)HandlePool_p[HandleIndex])->CtrlBlkPtr)
        {
            /* Release Busaccess Semaphore */
            STOS_SemaphoreSignal(g_DriverAtomic_p);
            return ST_ERROR_INVALID_HANDLE;
        }
    }
    CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;
    /* Test if I2C bus has already been locked */
    STOS_SemaphoreWait(CtrlBlkPtr_p->BusAccessSemaphore_p);

    /* We got the handle lock */
    if( (memcmp(CtrlBlkPtr_p->LockedHandlePool_p,HandlePool_p,NumberOfHandles*sizeof(STI2C_Handle_t)) == 0) )
    {
        for(HandleIndex=0;HandleIndex<NumberOfHandles;HandleIndex++)
        {
            /* SUCCESS: This handle already has the bus locked so release */
            CtrlBlkPtr_p->LockedHandlePool_p[HandleIndex] = 0;
            ReturnCode = ST_NO_ERROR;
        }
    }
    else
    {
        ReturnCode = ST_ERROR_BAD_PARAMETER;
    }
    /* Release the BusAccess semaphore */
    STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
    /* changed global sem signal location
    please see GNBvd38020 for more details
    */
    /* Release global Semaphore so that other function call can take place */
    STOS_SemaphoreSignal(g_DriverAtomic_p);
    return(ReturnCode);

}

/******************************************************************************
Name        : STI2C_GenerateClocks
Description : This function may be called if the I2C instance is configured
              (solely) as master, and bus-in-use is detected. It attempts to
              free the bus by generating clock pulses.
              Note that calling this function if the 55xx chip is not the sole
              master will likely result in data corruption.
              See DDTS GNBvd10880 for details.
Parameters  :

Handle        - handle to I2C bus opened
SclCount_p    - SCL clock pulses generated to unblock the bus (maximum of 15)

Returns     :

              ST_NO_ERROR             - no error
              ST_ERROR_INVALID_HANDLE - invalid handle given
              ST_ERROR_BAD_PARAMETER  - function called but handle not configured as
                                        master (master_or_slave isn't enough)
              ST_ERROR_DEVICE_BUSY    - Device in use, caller not blocking
              ST_ERROR_UNKNOWN_DEVICE - Device has not been initialized
              STI2C_ERROR_PIO         - Error accessing specified PIO port (see below)

******************************************************************************/
ST_ErrorCode_t STI2C_GenerateClocks(STI2C_Handle_t Handle,
                                    U32 *SclCount_p)
{
    i2c_param_block_t    *CtrlBlkPtr_p = NULL;
    i2c_open_block_t     *OpenBlkPtr_p = NULL;
    U8                   SCLValue=0;
    STPIO_OpenParams_t   PIOOpenParams;
    S32                  PIOPin1, PIOPin2;
    ST_ErrorCode_t       PIOReturnCode;
    STPIO_Handle_t       PIOHandle=0;
    U8                   SDAValue;

#ifndef STI2C_NO_PARAM_CHECK
    if (SclCount_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER) ;
    }
#endif
    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    STOS_TaskUnlock();
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Take global atomic */
    STOS_SemaphoreWait(g_DriverAtomic_p);
    OpenBlkPtr_p = (i2c_open_block_t*)Handle;

    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
         /* Release Busaccess Semaphore */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return ST_ERROR_INVALID_HANDLE;
    }
    CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;
    /*Take bus access semaphore to acquire execluive use of bus*/
    STOS_SemaphoreWait(CtrlBlkPtr_p->BusAccessSemaphore_p);
    /* Release global Semaphore so that other function call can take place */
    STOS_SemaphoreSignal(g_DriverAtomic_p);

    /*Check if bus is in master only mode Potentially unsafe otherwise */
    if (CtrlBlkPtr_p->InitParams.MasterSlave != STI2C_MASTER)
    {
        /* Release the BusAccess semaphore */
        STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    PIOHandle=CtrlBlkPtr_p->PIOHandle;

    /* Check PIO Pin values are valid */
    PIOPin1 = GetPioPin (CtrlBlkPtr_p->InitParams.PIOforSDA.BitMask);
    PIOPin2 = GetPioPin (CtrlBlkPtr_p->InitParams.PIOforSCL.BitMask);

    /* set PIO ports in bidirectional-open drain mode */
    PIOOpenParams.BitConfigure[PIOPin1] =
                            STPIO_BIT_BIDIRECTIONAL;
    PIOOpenParams.BitConfigure[PIOPin2] =
                            STPIO_BIT_BIDIRECTIONAL;

    PIOReturnCode = STPIO_SetConfig( PIOHandle,
                                             PIOOpenParams.BitConfigure );
    if ( PIOReturnCode != ST_NO_ERROR )
    {
        /* Release the BusAccess semaphore */
        STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
        /* PIO SetConfig failed */
        return STI2C_ERROR_PIO;
    }

    /* do SCL clocks until SDA becomes high again */

    do
    {
        STOS_TaskDelay(0);
        /* Set clock low */
        SCLValue=0x00;
        PIOReturnCode = STPIO_Write(PIOHandle,
                                    CtrlBlkPtr_p->InitParams.PIOforSCL.BitMask
                                    & SCLValue
                                    );
        STOS_TaskDelay(0);
        /* Set clock high */
        SCLValue=0xFF;
        PIOReturnCode = STPIO_Write(PIOHandle,
                                    CtrlBlkPtr_p->InitParams.PIOforSCL.BitMask
                                    & SCLValue
                                    );

        (*SclCount_p)++;
        STPIO_Read(PIOHandle,&SDAValue);

    } while ((((SDAValue) & CtrlBlkPtr_p->InitParams.PIOforSDA.BitMask)
               != (CtrlBlkPtr_p->InitParams.PIOforSDA.BitMask)) &&
             (*SclCount_p < I2C_SCL_MAXCLOCKS));


    /* Change PIO mode for I2C usage */
    PIOOpenParams.BitConfigure[PIOPin1] =
                            STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
    PIOOpenParams.BitConfigure[PIOPin2] =
                            STPIO_BIT_ALTERNATE_BIDIRECTIONAL;

    PIOReturnCode = STPIO_SetConfig( PIOHandle,
                                             PIOOpenParams.BitConfigure );
    if ( PIOReturnCode != ST_NO_ERROR )
    {
        /* Release the BusAccess semaphore */
        STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
        /* PIO SetConfig failed */
        return STI2C_ERROR_PIO;
    }
    STOS_SemaphoreSignal(CtrlBlkPtr_p->BusAccessSemaphore_p);
    return ST_NO_ERROR;
}



/******************************************************************************
Name        : STI2C_Reset
Description : Resets the I2C h/w device associated with the given handle.
              Also sets the s/w state back to Idle. Does not reset the I2C bus.
              This is designed as a recovery aid when things go badly wrong.
              Note this function does not get exclusive access by claiming the
              bus access semaphore.

              Currently, this is a hidden function not visible in the API.

Parameters  : 1) The handle to a device being used
******************************************************************************/

ST_ErrorCode_t STI2C_Reset (STI2C_Handle_t Handle)
{
    DU32                *I2cRegPtr;
    i2c_param_block_t    *CtrlBlk_p = NULL;
    i2c_open_block_t     *OpenBlkPtr_p = NULL;

    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    STOS_TaskUnlock();
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Take global atomic */
    STOS_SemaphoreWait(g_DriverAtomic_p);
    OpenBlkPtr_p = (i2c_open_block_t*)Handle;

    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
         /* Release Busaccess Semaphore */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return ST_ERROR_INVALID_HANDLE;
    }

    CtrlBlk_p = OpenBlkPtr_p->CtrlBlkPtr;
    /*Take bus access semaphore to acquire execluive use of bus*/
    STOS_SemaphoreWait(CtrlBlk_p->BusAccessSemaphore_p);
    I2cRegPtr = (DU32*) CtrlBlk_p->InitParams.BaseAddress;
    I2C_TRACE_LOCK(CtrlBlk_p,0xD000,(U32)I2cRegPtr);
    CtrlBlk_p->State = STI2C_IDLE;
    I2cReset(CtrlBlk_p);
    if (OpenBlkPtr_p->OpenParams.BaudRate == STI2C_RATE_FASTMODE)
    {
        I2cRegPtr[SSC_CONTROL] = DEFAULT_CONTROL | SSC_I2C_FASTMODE;
    }
    else
    {
        I2cRegPtr[SSC_CONTROL] = DEFAULT_CONTROL;
    }
    /* Release global Semaphore so that other function call can take place */
    STOS_SemaphoreSignal(g_DriverAtomic_p);
    I2cRegPtr[SSC_I2C] = DEFAULT_I2C;

    /* Set Tx data high to avoid bus corruption */
    I2cRegPtr[SSC_TX_BUFFER] = 0x1ff;

    I2cRegPtr[SSC_INT_ENABLE] = DEFAULT_ENABLES;
    CtrlBlk_p->InterruptMask = DEFAULT_ENABLES;
    STOS_SemaphoreSignal(CtrlBlk_p->BusAccessSemaphore_p);
    return ST_NO_ERROR;
}

/******************************************************************************
Name        : STI2C_SetBaudRate
Description : This function has been added to change the baudrate of the I2C driver
              on the fly.Please see the DDTS GNBvd30949 for more details
Parameters  : Handle        - handle to I2C bus opened
              BaudRate      - value of Baud rate to be set on fly
Returns     :

              ST_NO_ERROR             - no error
              ST_ERROR_INVALID_HANDLE - invalid handle given
              ST_ERROR_UNKNOWN_DEVICE - Device has not been initialized
******************************************************************************/


ST_ErrorCode_t STI2C_SetBaudRate(STI2C_Handle_t Handle,U32 BaudRate)
{

    i2c_open_block_t     *OpenBlkPtr_p = NULL;
    STOS_TaskLock();
    if (TRUE == First_Init)
    {
        STOS_TaskUnlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    STOS_TaskUnlock();
    if ((i2c_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Take global atomic */
    STOS_SemaphoreWait(g_DriverAtomic_p);
    OpenBlkPtr_p = (i2c_open_block_t*)Handle;

    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
         /* Release Busaccess Semaphore */
        STOS_SemaphoreSignal(g_DriverAtomic_p);
        return ST_ERROR_INVALID_HANDLE;
    }

    OpenBlkPtr_p->OpenParams.BaudRate=BaudRate;

    /* Release global Semaphore so that other function call can take place */
    STOS_SemaphoreSignal(g_DriverAtomic_p);

    return ST_NO_ERROR;

}

#if defined(STI2C_STSPI_SAME_SSC_SUPPORT)
/*****************************************************************************
I2c_GetSSC
Description:
    Get the SSC Device by installing the interrupt handler.

******************************************************************************/

ST_ErrorCode_t I2c_GetSSC (i2c_param_block_t *Params_p)
{
    DU32                 *I2cRegPtr  = NULL;
    ST_ErrorCode_t       ReturnCode  = ST_NO_ERROR;
    i2c_param_block_t    *CtrlBlkPtr_p;
    S32                  IntReturn;
    U32                  DeadData;

    CtrlBlkPtr_p =       (i2c_param_block_t *)Params_p;
    I2cRegPtr = (DU32*) CtrlBlkPtr_p->InitParams.BaseAddress;

    STOS_InterruptLock();

    if(I2cRegPtr[SSC_STATUS] & SSC_STAT_BUSY)
    {
        STOS_InterruptUnlock();
        return ST_ERROR_DEVICE_BUSY;
    }
    else
    {
        /* Try to register and enable the interrupt handler */
        IntReturn = STOS_InterruptInstall ( CtrlBlkPtr_p->InitParams.InterruptNumber,
                                                CtrlBlkPtr_p->InitParams.InterruptLevel,
                                                I2cHandler,"STI2C", CtrlBlkPtr_p );
        if (IntReturn == 0)
        {
#ifndef ST_OS21
            /* interrupt_enable() will be redundant after change will be performed in STBOOT */
            IntReturn =interrupt_enable (CtrlBlkPtr_p->InitParams.InterruptLevel);
            if (IntReturn == 0)
#endif
            {
                IntReturn=interrupt_enable_number(CtrlBlkPtr_p->InitParams.InterruptNumber);
            }

            STOS_InterruptUnlock();

            if ( IntReturn != 0 )
              /* Interrupt enable failed */
            {
                ReturnCode = ST_ERROR_BAD_PARAMETER;
            }
            /* perform bus reset */
            I2cReset(CtrlBlkPtr_p);
            I2cRegPtr[SSC_I2C] = DEFAULT_I2C;

            /* Set Tx data high to avoid bus corruption */
            WriteReg(I2cRegPtr, SSC_TX_BUFFER, 0x1ff);

            WriteReg(I2cRegPtr, SSC_BAUD_RATE,
                     CtrlBlkPtr_p->InitParams.ClockFrequency / (15 * CtrlBlkPtr_p->InitParams.BaudRate));
#if defined(SSC3_WITH_TIMING_PRESENT)
            WriteReg(I2cRegPtr, SSC_GLITCH_WIDTH,(int)((GLITCH_WIDTH_DATAOUT*(CtrlBlkPtr_p->InitParams.ClockFrequency / 10000000))
                                                  /(PRE_SCALER_FACTOR*1000)));
            WriteReg(I2cRegPtr, SSC_PRE_SCALER,PRE_SCALER_FACTOR);
            WriteReg(I2cRegPtr, SSC_GLITCH_WIDTH_DATAOUT,(int)(GLITCH_WIDTH_DATAOUT/100));
            WriteReg(I2cRegPtr, SSC_PRE_SCALER_DATAOUT,(int)(CtrlBlkPtr_p->InitParams.ClockFrequency / 10000000));
            /* Calculate and write the appropriate
               values into the I2C timing registers. */
            I2cRegPtr[SSC_I2C] = DEFAULT_I2C;
#else
            WriteReg(I2cRegPtr, SSC_GLITCH_WIDTH,0x00);
            WriteReg(I2cRegPtr, SSC_PRE_SCALER,(int)(CtrlBlkPtr_p->InitParams.ClockFrequency / 10000000));
#endif
            /* Read i2c buffer to clear spurious data */
            DeadData  = ReadReg(I2cRegPtr, SSC_RX_BUFFER);

            /* enable interrupts from I2C registers */
            WriteReg(I2cRegPtr, SSC_INT_ENABLE,
                     CtrlBlkPtr_p->InterruptMask);

            CtrlBlkPtr_p->LastTransferHadStop = TRUE;
        }
        else
        {
            STOS_InterruptUnlock();
            ReturnCode = ST_ERROR_INTERRUPT_INSTALL;
        }
    }
    return ( ReturnCode );
}
#endif

#if defined(STI2C_STSPI_SAME_SSC_SUPPORT)
/****************************************************************************
I2c_FreeSSC
Description: It will free the SSC by Uninstalling its interrupt handler.
***************************************************************************/

ST_ErrorCode_t I2c_FreeSSC(i2c_param_block_t *Params_p)
{
    DU32                 *I2cRegPtr   = NULL;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;
    i2c_param_block_t    *CtrlBlkPtr_p;
    S32                  IntReturn;

   CtrlBlkPtr_p = (i2c_param_block_t *)Params_p;

    I2cRegPtr = (DU32*) CtrlBlkPtr_p->InitParams.BaseAddress;
    /* Disable the Interrupt */
    I2cRegPtr[SSC_INT_ENABLE] = 0;
    /* Uninstall the interrupt handler for this port. Only disable
     * interrupt if no other handlers connected on the same level.
     * Note this needs to be MT-safe.
     */
    STOS_InterruptLock();
    if(I2cRegPtr[SSC_STATUS] & SSC_STAT_BUSY)
    {
       STOS_InterruptUnlock();
       return ST_ERROR_DEVICE_BUSY;
    }
    else
    {
        IntReturn = STOS_InterruptUninstall(CtrlBlkPtr_p->InitParams.InterruptNumber,
                                CtrlBlkPtr_p->InitParams.InterruptLevel,
                                CtrlBlkPtr_p );
        if (IntReturn == 0)
        {
            /* interrupt_enable() will be redundant after change will be performed in STBOOT */
            IntReturn =STOS_InterruptDisable(CtrlBlkPtr_p->InitParams.InterruptNumber,
                                CtrlBlkPtr_p->InitParams.InterruptLevel);
            STOS_InterruptUnlock();

            if ( IntReturn != 0 )
            {
                ReturnCode = ST_ERROR_BAD_PARAMETER;
            }
            /* Reset the devices */
            I2cRegPtr[SSC_I2C] = 0;
            I2cRegPtr[SSC_CONTROL] = 0;
            I2cRegPtr[SSC_REP_START_HOLD_TIME]= 0x01;
            I2cRegPtr[SSC_START_HOLD_TIME] = 0x01;
            I2cRegPtr[SSC_REP_START_SETUP_TIME] = 0x01;
            I2cRegPtr[SSC_DATA_SETUP_TIME]=0x01;
            I2cRegPtr[SSC_STOP_SETUP_TIME]=0x01;
            I2cRegPtr[SSC_BUS_FREE_TIME]=0x01;
        }
        else
        {
            STOS_InterruptUnlock();
            ReturnCode = ST_ERROR_INTERRUPT_UNINSTALL;
        }
    }
    return( ReturnCode );
}
#endif

#if defined(SSC3_WITH_TIMING_PRESENT)
/******************************************************************************
Name        : i2c_WriteTimingRegisters
Description : This function writes the appropriate values to the timing
              registers present on STi5528 and upwards. The values vary
              depending on clockspeed and mode. See ADCS 7184762 for
              details.
Parameters  :

CtrlBlkPtr_p      - pointer to the I2c register block to program


Returns     : Nothing.

******************************************************************************/
void i2c_WriteTimingValues(i2c_param_block_t   *CtrlBlkPtr_p)
{
    U32 NSPerCyc;
    DU32 *I2cRegPtr = (DU32*) CtrlBlkPtr_p->InitParams.BaseAddress;

    /* Work out the number of nanoseconds per cycle. */
    NSPerCyc = 1000000000 / CtrlBlkPtr_p->InitParams.ClockFrequency;

    /* Numbers are in ns - ie 600 is 600ns => 0.6us. Numbers obtained
     * from ADCS 7184762, or the I2C specification v2.1
     * Default to slow-rate values. (Safer.)
     */
#ifndef ST_5528
    if (CtrlBlkPtr_p->InitParams.BaudRate == STI2C_RATE_FASTMODE)
    {
        I2cRegPtr[SSC_GLITCH_WIDTH]=0;
        I2cRegPtr[SSC_REP_START_HOLD_TIME]  = ( REP_START_HOLD_TIME_FAST  + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
        I2cRegPtr[SSC_REP_START_SETUP_TIME] = ( REP_START_SETUP_TIME_FAST + GLITCH_WIDTH_CLOCKIN ) / NSPerCyc;
        if(GLITCH_WIDTH_DATAOUT < 200)
        {
            I2cRegPtr[SSC_START_HOLD_TIME]  = ( START_HOLD_TIME_FAST + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
        }
        else
        {
            I2cRegPtr[SSC_START_HOLD_TIME]  = ( 5 * GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
        }
        I2cRegPtr[SSC_DATA_SETUP_TIME]      = ( DATA_SETUP_TIME_FAST + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
        I2cRegPtr[SSC_STOP_SETUP_TIME]      = ( STOP_SETUP_TIME_FAST + GLITCH_WIDTH_CLOCKIN ) / NSPerCyc;
        I2cRegPtr[SSC_BUS_FREE_TIME]        = ( BUS_FREE_TIME_FAST   + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;

    }
    else
    {
        I2cRegPtr[SSC_REP_START_HOLD_TIME]  = ( REP_START_HOLD_TIME_STD  + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
        I2cRegPtr[SSC_REP_START_SETUP_TIME] = ( REP_START_SETUP_TIME_STD + GLITCH_WIDTH_CLOCKIN ) / NSPerCyc;
        /* value of SSC_START_HOLD_TIME has been changed for H/W bug
        GNBvd3284 "I2C problem between STi5100 and NIM399 on MB390"
        */
        if(GLITCH_WIDTH_DATAOUT < 1200)
        {
            I2cRegPtr[SSC_START_HOLD_TIME]  = ( START_HOLD_TIME_STD + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
        }
        else
        {
            I2cRegPtr[SSC_START_HOLD_TIME]  = ( 5 * GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
        }
        I2cRegPtr[SSC_DATA_SETUP_TIME]      = ( DATA_SETUP_TIME_STD + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
        I2cRegPtr[SSC_STOP_SETUP_TIME]      = ( STOP_SETUP_TIME_STD + GLITCH_WIDTH_CLOCKIN ) / NSPerCyc;
        I2cRegPtr[SSC_BUS_FREE_TIME]        = ( BUS_FREE_TIME_STD   + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
    }
#else
    I2cRegPtr[SSC_REP_START_HOLD_TIME]  = ( 120000 + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
    I2cRegPtr[SSC_REP_START_SETUP_TIME] = ( 4600   + GLITCH_WIDTH_CLOCKIN ) / NSPerCyc;
    if(GLITCH_WIDTH_DATAOUT < 1200)
    {
        I2cRegPtr[SSC_START_HOLD_TIME]  = ( 4500 + GLITCH_WIDTH_DATAOUT)/ NSPerCyc;
    }
    else
    {
        I2cRegPtr[SSC_START_HOLD_TIME]  = ( 5 * GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
    }
    I2cRegPtr[SSC_DATA_SETUP_TIME]      = (  300 + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
    I2cRegPtr[SSC_STOP_SETUP_TIME]      = ( 4200 + GLITCH_WIDTH_CLOCKIN ) / NSPerCyc;
    I2cRegPtr[SSC_BUS_FREE_TIME]        = ( 5000 + GLITCH_WIDTH_DATAOUT ) / NSPerCyc;
#endif
}
#endif
/*-----------------------------------END-------------------------------------*/

