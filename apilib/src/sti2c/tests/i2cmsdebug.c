/************************************************************************
COPYRIGHT (C) STMicroelectronics 1999

Source file name : i2cmsdebug.c

I2C tests - master/slave debug program.

************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "stlite.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stack.h"
#include "stpio.h"
#ifndef ST_OS21
#include "debug.h"
#endif
#include "sttbx.h"
#include "stevt.h"
#include "stboot.h"
#include "stcommon.h"
#include "sti2c.h"


/* Other Private types/constants ------------------------------------------ */

#if defined(ST_7710) || defined(ST_7100) || defined(ST_7109)

#define     SSC_0_PIO_BASE              PIO_2_BASE_ADDRESS
#define     SSC_0_PIO_INTERRUPT         PIO_2_INTERRUPT
#define     SSC_0_PIO_INTERRUPT_LEVEL   PIO_2_INTERRUPT_LEVEL

#elif defined(ST_5700)

#define     SSC_0_PIO_BASE              PIO_1_BASE_ADDRESS
#define     SSC_0_PIO_INTERRUPT         PIO_1_INTERRUPT
#define     SSC_0_PIO_INTERRUPT_LEVEL   PIO_1_INTERRUPT_LEVEL

#else

#define     SSC_0_PIO_BASE              PIO_3_BASE_ADDRESS
#define     SSC_0_PIO_INTERRUPT         PIO_3_INTERRUPT
#define     SSC_0_PIO_INTERRUPT_LEVEL   PIO_3_INTERRUPT_LEVEL

#endif

#if defined(ST_5510) || defined(ST_5512) || defined(ST_TP3)

#define PIO_FOR_SSC0_SDA                PIO_BIT_0
#define PIO_FOR_SSC0_SCL                PIO_BIT_2

#else

/* 08, 18, 14, 16, 17, 28, ... */
#define PIO_FOR_SSC0_SDA                PIO_BIT_0
#define PIO_FOR_SSC0_SCL                PIO_BIT_1

#endif

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            100000

#define TEST_PARTITION_1       &the_system_partition
#define TEST_PARTITION_2       &the_internal_partition

#define SLAVE_DEVICE_ID        0x42
#define EEPROM_DEVICE_ID       0xA0
#define EVENT_LOG_SIZE         8192
#define TRACE_PAGE_SIZE        50

/* PIO Offsets (8 bit registers, byte aligned) */
#define PIO_OUT                         0 
#define PIO_OUT_SET                     (1 << 2)
#define PIO_OUT_CLR                     (2 << 2)
#define PIO_IN                          (4 << 2)
#define PIO_C0                          (8 << 2)
#define PIO_C0_SET                      (9 << 2)
#define PIO_C0_CLR                      (10 << 2)
#define PIO_C1                          (12 << 2)
#define PIO_C1_SET                      (13 << 2)
#define PIO_C1_CLR                      (14 << 2)
#define PIO_C2                          (16 << 2)
#define PIO_C2_SET                      (17 << 2)
#define PIO_C2_CLR                      (18 << 2)
#define PIO_CMP                         (20 << 2)
#define PIO_CMP_SET                     (21 << 2)
#define PIO_CMP_CLR                     (22 << 2)
#define PIO_MASK                        (24 << 2)
#define PIO_MASK_SET                    (25 << 2)
#define PIO_MASK_CLR                    (26 << 2)

typedef enum Transfer_e
{
    WRITE_TRANSFER,
    WRITE_NO_STOP_TRANSFER,
    READ_TRANSFER,
    READ_NO_STOP_TRANSFER
} Transfer_t;

typedef ST_ErrorCode_t (*I2cFunc_t) (STI2C_Handle_t, U8*, U32, U32, U32*);


/* Private variables ------------------------------------------------------ */

/* Variables used in multiple task test sections ie handles and task  */
/* descriptors                                                               */

static U32     ErrorCount = 0;           /* Overall count of errors */
static BOOL    Quiet      = FALSE;       /* set to suppress confirmation
                                            messages on expected results */
static U32 I2cClockSpeed;
static ClocksPerSec;
static ST_DeviceName_t   EVTDevName0 = "EVT0";

/* Data for slave mode operation */
static U32 TxNotifyCount=0,   RxNotifyCount=0,   TxByteCount=0, RxByteCount=0,
           TxCompleteCount=0, RxCompleteCount=0;
static U8    RxBuffer [4096];
static U32   EventLog [EVENT_LOG_SIZE];
static U32   EventIndex=0;
static const EventSize=3;
static U32   LastTxNotifyCount,        LastRxNotifyCount,
             LastTxByteCount,          LastRxByteCount,
             LastTxCompleteCount,      LastRxCompleteCount,
             CurrentTxNotifyCount=0,   CurrentRxNotifyCount=0,
             CurrentTxByteCount=0,     CurrentRxByteCount=0,
             CurrentTxCompleteCount=0, CurrentRxCompleteCount=0,
             LastEventIndex,           CurrentEventIndex=0;
static U32   LoopDelay = 0;
static U32   TxValue = 0, RxValue = 0;
static BOOL  RandomDelay = FALSE;
static U32   DelayMin, DelayRange;

#ifndef ST_OS21
/* Declarations for memory partitions */
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section_noinit")

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
#pragma ST_section      ( system_block,   "system_section_noinit")

#pragma ST_section      ( RxBuffer,       "ncache_section")

/* This is to avoid a linker warning */
static unsigned char    internal_block_init[1];
#pragma ST_section      ( internal_block_init, "internal_section")
 
static unsigned char    system_block_init[1];
#pragma ST_section      ( system_block_init, "system_section")

#if defined(ST_5528)|| defined(ST_5100)|| defined(ST_7710) ||defined(ST_5105)||defined(ST_7100) || defined(ST_7109) || defined(ST_5107)
static U8               data_section[1];
#pragma ST_section      ( data_section,   "data_section")
#endif

/* Temporary, for compatibility with old drivers (EVT) */
partition_t             *system_partition = &the_system_partition;

#else /* ST_OS21 */
#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition_p;
#endif /* ST_OS21 */



#ifdef STI2C_TRACE
extern U32 I2cPrintTrace (S32 Start, S32 Number);
#endif

extern ST_ErrorCode_t STI2C_Reset (STI2C_Handle_t);

/* Function prototypes ----------------------------------------------------- */

void DoTheTests(void);
#if (!defined(ST_OS21))
static U8 GetNextChar (void);
#endif
void myprintf (char *format, ...);


/* ------------------------------------------------------------------------- */

#if (!defined(ST_OS21))
static void QueryTrace (void)
{
#ifdef STI2C_TRACE
    /* Print I2C trace buffer, if user enters 'y' */
    U8 c;

    myprintf ("Print trace?\n");
    while (1)
    {
        c = GetNextChar();
        if ( (I2cPrintTrace (-1, (c=='y') ? TRACE_PAGE_SIZE : 0) == 0) ||
             (c != 'y') )
            break;
        myprintf ("More?\n");
    }
#endif
}

static void PrintTrace (S32 Start, S32 Number)
{
#ifdef STI2C_TRACE
    U32 r;

    if (Number<=0)
    {
        Number = 1000000;   /* a very large number! */
        if (Start <= 0)
            Start = -1;
    }

    while (1)
    {
        r = I2cPrintTrace (Start,
                (Number>TRACE_PAGE_SIZE) ? TRACE_PAGE_SIZE : Number);
        Number -= TRACE_PAGE_SIZE;
        if (Start==-1)
        {
            if (r==0)
                break;
        }
        else
        {
            if (Number <=0)
                break;
            Start += TRACE_PAGE_SIZE;
        }

        myprintf ("More?\n");
        if (GetNextChar() != 'y')
            break;
    }
#endif
}



static void PrintBuffer (U8 *Buffer, U32 Size)
{
    U32 i;
    char s[8], Line[128];

    Line[0] = 0;           /* empty string */
    for (i=0; i<Size; i++)
    {
        sprintf (s, "%02x ", Buffer[i]);
        strcat (Line, s);
        if ( (((i+1) % 16) == 0) || (i==Size-1) )
        {
            myprintf ("%s\n", Line);
            Line[0] = 0;
        }
    }
}


#if (!defined(PROCESSOR_C1))&&(!defined(ST_OS21))
static U32 GetHpClock (void)
{
    U32 temp;
    __optasm { ldc 0; ldclock; st temp; }
    return temp;
}
#endif


#if (!defined(ST_OS21))
static U8 GetNextChar (void)
{
    U8 s[80];
    long Length, i;

    while (1)
    {
        Length = debuggets (s, sizeof(s));
        for (i=0; i<Length; i++)
            if (s[i] > ' ')
                return (s[i]);
    }
    return 0;
}
#endif
#endif
static U32 GetI2cTimeout (U32 TransferSize)
{
    /* Estimate timeout required for an I2C transfer of size TransferSize */
    return (TransferSize * 1000) / (STI2C_RATE_FASTMODE / 40) + 2;
}

static void GetPIOConfig (U8 *pC0, U8 *pC1, U8 *pC2)
{
#if (!defined(PROCESSOR_C1))&&(!defined(ST_OS21)) 
    #pragma ST_device (PioReg)
#endif    
    U8  *PioReg = (U8*)SSC_0_PIO_BASE;

    *pC0 = PioReg[PIO_C0];
    *pC1 = PioReg[PIO_C1];
    *pC2 = PioReg[PIO_C2];
}

static void PrintPIOConfig (void)
{
    U8 C0, C1, C2;
    GetPIOConfig (&C0, &C1, &C2);
    myprintf ("PIO config: C0 = 0x%02x, C1 = 0x%02x, C2 = 0x%02x\n",
              C0, C1, C2);
}

 
static void LogEvent (STEVT_EventConstant_t Event, U32 Data)
{
    interrupt_lock();
    EventLog [EventIndex]   = (U32) Event;
    EventLog [EventIndex+1] = Data;
#if (!defined(PROCESSOR_C1))&&(!defined(ST_OS21))
    EventLog [EventIndex+2] = GetHpClock();
#endif
    EventIndex += EventSize;
    if (EventIndex >= EVENT_LOG_SIZE-EventSize)
        EventIndex = 0;   /* wrap around */
    interrupt_unlock();
}
void myprintf (char *format, ...)
{
    /* Just a shorthand for STTBX_Output... */
    char s[256];
    va_list v;

    va_start (v, format);
    vsprintf (s, format, v);
#ifndef STI2C_NO_TBX
    STTBX_Print ((s));
#else
    printf(s);
#endif    
    va_end (v);
}



int main (void)
{
    ST_ErrorCode_t        Code;
    STBOOT_InitParams_t   InitParams;
    STBOOT_TermParams_t   TermParams;
    ST_DeviceName_t       Name = "DEV0";
    STBOOT_DCache_Area_t  Cache[] = { {(U32*)0x40080000, (U32*)0x4FFFFFFF},
                                      { NULL, NULL} };
#ifndef STI2C_NO_TBX
    STTBX_InitParams_t    TBX_InitParams;
#endif    
    ST_ClockInfo_t ClockInfo;

#ifdef ST_OS21
    /* Create memory partitions */
    system_partition_p   =  partition_create_heap((U8*)external_block, sizeof(external_block));
#else /* ifndef ST_OS21 */
    partition_init_heap (&the_internal_partition, internal_block,
                         sizeof(internal_block));
    partition_init_heap (&the_system_partition,   system_block,
                         sizeof(system_block));

    /* Avoid compiler warnings */
    internal_block_init[0] = 0;
    system_block_init[0] = 0;

#if defined(ST_5528)|| defined(ST_5100)|| defined(ST_7710)||defined(ST_5105)||defined(ST_7100)|| defined(ST_7109) || defined(ST_5107)
    data_section[0] = 0;
#endif
#endif

    InitParams.ICacheEnabled             = TRUE;
    InitParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    InitParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    InitParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    InitParams.CacheBaseAddress          = (U32*) CACHE_BASE_ADDRESS;
    InitParams.DCacheMap                 = Cache;
    InitParams.MemorySize                = (STBOOT_DramMemorySize_t)SDRAM_SIZE;
    InitParams.SDRAMFrequency            = SDRAM_FREQUENCY;

    if ((Code = STBOOT_Init (Name, &InitParams)) != ST_NO_ERROR)
        printf ("ERROR: STBOOT_Init returned code %d\n", Code);
    else
    {
        ClocksPerSec = ST_GetClocksPerSecond();

        ST_GetClockInfo(&ClockInfo);
        I2cClockSpeed = ClockInfo.CommsBlock;
#ifndef STI2C_NO_TBX
#if defined(ST_OS21)    
       TBX_InitParams.CPUPartition_p       =  (ST_Partition_t *)system_partition_p;
#else   
       TBX_InitParams.CPUPartition_p       = &the_system_partition;
#endif 
        TBX_InitParams.SupportedDevices    = STTBX_DEVICE_DCU;
        TBX_InitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
        TBX_InitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;

        if ((Code = STTBX_Init( "TBX0", &TBX_InitParams )) != ST_NO_ERROR )
            printf ("Error: STTBX_Init = 0x%08X\n", Code );
        else
        {
            myprintf("STTBX_Init() ok\n");
        }
#endif
        DoTheTests();
        STBOOT_Term (Name, &TermParams);
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
/* Other utility functions for checking various things.                      */
/*---------------------------------------------------------------------------*/

static BOOL CheckCode (ST_ErrorCode_t actualCode, ST_ErrorCode_t expectedCode)
{
    if (actualCode == expectedCode)
    {
        if (!Quiet)
             myprintf (" Got expected code %d\n", expectedCode);
        return TRUE;
    }
    else
    {
        myprintf (" ERROR: got code %d, expected %d\n", actualCode,
                  expectedCode);
        ErrorCount++;
        return FALSE;
    }
}


static BOOL CheckCodeOk (ST_ErrorCode_t actualCode)
{
    return CheckCode (actualCode, ST_NO_ERROR);
}

/*--------------------------------------------------------------------------*/
/* Callbacks                                                                */
/*--------------------------------------------------------------------------*/

void TxNotify (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant_t Event, const void *EventData,
               const void *SubscriberData_p)
{
    TxNotifyCount++;
    LogEvent (Event, 0);
}

void RxNotify (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant_t Event, const void *EventData,
               const void *SubscriberData_p)
{
    RxNotifyCount++;
    LogEvent (Event, 0);
}

void TxComplete (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event, const void *EventData,
                 const void *SubscriberData_p)
{
    TxCompleteCount++;
    LogEvent (Event, 0);
}

void RxComplete (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event, const void *EventData,
                 const void *SubscriberData_p)
{
    RxCompleteCount++;
    LogEvent (Event, 0);
}

void TxByte (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
             STEVT_EventConstant_t Event, const void *EventData,
             const void *SubscriberData_p)
{
    TxByteCount++;
    *(U8*)EventData = (U8)TxByteCount;
    LogEvent (Event, TxByteCount);
}

void RxByte (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
             STEVT_EventConstant_t Event, const void *EventData,
             const void *SubscriberData_p)
{
    U8  Data = *(U8*)EventData;
    RxBuffer [RxByteCount % sizeof(RxBuffer)] = Data;
    RxByteCount++;
    LogEvent (Event, (U32)Data);
}

/*--------------------------------------------------------------------------*/
/*  Event Handler initialisation                                            */
/*--------------------------------------------------------------------------*/

static BOOL EVT_Init (ST_DeviceName_t EVTDeviceName)
{
    STEVT_InitParams_t      EVTInitParams;

    EVTInitParams.ConnectMaxNum   =  5;
    EVTInitParams.EventMaxNum     = 20;
    EVTInitParams.SubscrMaxNum    = 20;
#if defined(ST_OS21)    
    EVTInitParams.MemoryPartition       = (ST_Partition_t *)system_partition_p;
#else    
    EVTInitParams.MemoryPartition       = TEST_PARTITION_1;
#endif      

    myprintf ("Initialising Event Handler '%s'...  ", EVTDeviceName);
    return CheckCodeOk ( STEVT_Init( EVTDeviceName, &EVTInitParams ) );
}

static void EVT_Open (ST_DeviceName_t EVTDeviceName, STEVT_Handle_t *pHandle)
{
    STEVT_OpenParams_t      EVTOpenParams;
    STEVT_DeviceSubscribeParams_t TxNotifySub,   RxNotifySub,   
                            TxByteSub, RxByteSub,
                            TxCompleteSub, RxCompleteSub;

    TxNotifySub.NotifyCallback     = TxNotify;
    TxNotifySub.SubscriberData_p   = NULL;
    RxNotifySub.NotifyCallback     = RxNotify;
    RxNotifySub.SubscriberData_p   = NULL;
    TxByteSub.NotifyCallback       = TxByte;
    TxByteSub.SubscriberData_p     = NULL;
    RxByteSub.NotifyCallback       = RxByte;
    RxByteSub.SubscriberData_p     = NULL;
    TxCompleteSub.NotifyCallback   = TxComplete;
    TxCompleteSub.SubscriberData_p = NULL;
    RxCompleteSub.NotifyCallback   = RxComplete;
    RxCompleteSub.SubscriberData_p = NULL;

    myprintf ("Open Event handler              :");
    CheckCodeOk (STEVT_Open (EVTDevName0, &EVTOpenParams, pHandle) );

    myprintf ("Subscribe TxNotify              :");
    CheckCodeOk (STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                  STI2C_TRANSMIT_NOTIFY_EVT,
                                  &TxNotifySub) );

    myprintf ("Subscribe RxNotify              :");
    CheckCodeOk (STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                  STI2C_RECEIVE_NOTIFY_EVT,
                                  &RxNotifySub) );

    myprintf ("Subscribe TxByte                :");
    CheckCodeOk (STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                  STI2C_TRANSMIT_BYTE_EVT,
                                  &TxByteSub) );

    myprintf ("Subscribe RxByte                :");
    CheckCodeOk (STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                  STI2C_RECEIVE_BYTE_EVT,
                                  &RxByteSub) );

    myprintf ("Subscribe TxComplete            :");
    CheckCodeOk (STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                  STI2C_TRANSMIT_COMPLETE_EVT,
                                  &TxCompleteSub) );

    myprintf ("Subscribe RxComplete            :");
    CheckCodeOk (STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                  STI2C_RECEIVE_COMPLETE_EVT,
                                  &RxCompleteSub) );
}

/*--------------------------------------------------------------------------*/
/*  Other functions                                                         */
/*--------------------------------------------------------------------------*/
#ifndef ST_OS21
void PrintEvents (void)
{
    U32 i, n;
    char  s[8], Line[128];

    LastTxNotifyCount   = CurrentTxNotifyCount;
    LastRxNotifyCount   = CurrentRxNotifyCount;
    LastTxByteCount     = CurrentTxByteCount;
    LastRxByteCount     = CurrentRxByteCount;
    LastTxCompleteCount = CurrentTxCompleteCount;
    LastRxCompleteCount = CurrentRxCompleteCount;
    LastEventIndex      = CurrentEventIndex;

    interrupt_lock();
    CurrentTxNotifyCount   = TxNotifyCount;
    CurrentRxNotifyCount   = RxNotifyCount;
    CurrentTxByteCount     = TxByteCount;
    CurrentRxByteCount     = RxByteCount;
    CurrentTxCompleteCount = TxCompleteCount;
    CurrentRxCompleteCount = RxCompleteCount;
    CurrentEventIndex      = EventIndex;
    interrupt_unlock();

    if (CurrentEventIndex != LastEventIndex)
    {
        myprintf ("EventIndex :     %6u -> %6u\n",
                   LastEventIndex, CurrentEventIndex);
        myprintf ("TxNotifyCount:   %6u -> %6u\n",
                   LastTxNotifyCount, CurrentTxNotifyCount);
        myprintf ("RxNotifyCount:   %6u -> %6u\n",
                   LastRxNotifyCount, CurrentRxNotifyCount);
        myprintf ("TxByteCount:     %6u -> %6u\n",
                   LastTxByteCount, CurrentTxByteCount);
        myprintf ("RxByteCount:     %6u -> %6u\n",
                   LastRxByteCount, CurrentRxByteCount);
        myprintf ("TxCompleteCount: %6u -> %6u\n",
                   LastTxCompleteCount, CurrentTxCompleteCount);
        myprintf ("RxCompleteCount: %6u -> %6u\n",
                   LastRxCompleteCount, CurrentRxCompleteCount);

        if (LastRxByteCount != CurrentRxByteCount)
        {
            myprintf ("Received %u bytes:\n",
                      ((CurrentRxByteCount+sizeof(RxBuffer))-LastRxByteCount)
                        % sizeof(RxBuffer)  );
            n=0;
            Line[0] = 0;               /* empty string */
            i = LastRxByteCount % sizeof(RxBuffer);
            while (i != (CurrentRxByteCount % sizeof(RxBuffer)))
            {
                sprintf (s, "%02x ", RxBuffer[i]);
                strcat (Line, s);
                if (++n % 16 == 0)
                {
                    myprintf ("%s\n", Line);
                    Line[0] = 0;
                }
                if (++i >= sizeof(RxBuffer))
                {
                    myprintf ("Wrapping round after %u chars\n", n);
                    i = 0;   /* wrap around */
                }
            }
            if (Line[0] != 0)
                myprintf ("%s\n", Line);   /* print incomplete line */
        }

        myprintf ("Print log?\n");
        if (GetNextChar() == 'y')
        {
            myprintf ("Event Log:\n   Event   Data       Time\n");
            i = LastEventIndex;
            while (i != CurrentEventIndex)
            {
                myprintf ("0x%06x   0x%02x %10u\n",
                           EventLog[i], EventLog[i+1], EventLog[i+2]);
                i += EventSize;
                if (i >= EVENT_LOG_SIZE-EventSize)
                    i = 0;                
            }
        }
    }

    else
        myprintf ("No new events\n");
}


void DoTransfer (Transfer_t Type, STI2C_Handle_t Handle,
                 S32 Size, S32 RepeatCount, U8 *Buffer, U32 NumInBuffer)
{
    U32        i, j, ActLen;
    BOOL       AbortOnFail = TRUE, Finished = FALSE;
    I2cFunc_t  Func;
    U8         LocalBuffer[1024];

    if (Size > sizeof(LocalBuffer))
    {
        myprintf ("ERROR: Specified transfer size too large (max %u bytes)\n",
                   sizeof(LocalBuffer));
        return;
    }

    if (RepeatCount <= 1)
        RepeatCount = 1;
    else
    {
        myprintf ("Abort on fail?\n");
        AbortOnFail = (GetNextChar()=='y');
        Quiet = TRUE;
    }
    myprintf ("Transferring %d bytes %d times:\n", Size, RepeatCount);

    if ((Type == WRITE_TRANSFER) || (Type == WRITE_NO_STOP_TRANSFER))
    {
        for (i=0; i<NumInBuffer; i++)
            LocalBuffer[i] = Buffer[i];
        for (i=NumInBuffer; i<Size; i++, TxValue+=2)
            LocalBuffer[i] = (U8)TxValue;
        PrintBuffer (LocalBuffer, Size);
        if (Type == WRITE_NO_STOP_TRANSFER)
        {
            myprintf ("Write No Stop returns    :");
            Func = (I2cFunc_t) STI2C_WriteNoStop;
        }
        else
        {
            myprintf ("Write returns            :");
            Func = (I2cFunc_t) STI2C_Write;
        }
    }

    else    /* read transfer */
    {
        for (i=0; i<Size; i++)
            LocalBuffer[i] = 0xAA;   /* so we can see if anything is read */

        if (Type == READ_NO_STOP_TRANSFER)
        {
            myprintf ("Read No Stop returns     :");
            Func = STI2C_ReadNoStop;
        }
        else
        {
            myprintf ("Read returns             :");
            Func = STI2C_Read;
        }
    }

    for (i=0; i<RepeatCount; i++)
    {
        if (CheckCodeOk (Func (Handle, LocalBuffer, Size, GetI2cTimeout(Size),
                               &ActLen)) == FALSE)
        {
            if (AbortOnFail)
                Finished = TRUE;
            else
            {
                /* Attempt recovery */
                task_delay (20);
                STI2C_Write (Handle, LocalBuffer, 0, 1, &ActLen);
                task_delay (20);
                STI2C_Reset (Handle);
                task_delay (20);
            }
        }
        else if (ActLen != Size)
        {
            RxValue += ActLen;
            myprintf ("ERROR: Specified size = %d, actual size = %d\n",
                       Size, ActLen);
            if (AbortOnFail)
                Finished = TRUE;
        }

        /* Check received bytes for reads */
        else if ((Type == READ_TRANSFER) || (Type == READ_NO_STOP_TRANSFER))
        {
            if (NumInBuffer > 0)
            {
                RxValue += Size;
                for (j=0; j<NumInBuffer; j++)
                {
                    if (Buffer[j] != LocalBuffer[j])
                    {
                        myprintf ("ERROR: byte %u = 0x%02x, expected = 0x%02x\n",
                                   j, LocalBuffer[j], Buffer[j]);
                        if (AbortOnFail)
                        {
                            Finished = TRUE;
                            break;
                        }
                    }
                }
            }
            else   /* check recevied bytes against incrementing count */
            {
                for (j=0; j<Size; j++)
                {
                    if (LocalBuffer[j] != (U8)++RxValue)
                    {
                        myprintf ("ERROR: byte %u = 0x%02x, expected = 0x%02x\n",
                                   j, LocalBuffer[j], (U8) RxValue);
                        if (AbortOnFail)
                        {
                            Finished = TRUE;
                        }
                    }
                }
            }
        }

        if (Finished)
            break;
        task_delay (RandomDelay ? (DelayMin + rand() % DelayRange)
                                : LoopDelay);
    }   /* end of repeat loop */

    if (i<RepeatCount)
        myprintf ("\nAborted after %u transfers", i+1);
    myprintf ("\n%d bytes actually transferred\n", ActLen);
    if ((Type == READ_TRANSFER) || (Type == READ_NO_STOP_TRANSFER))
        PrintBuffer (LocalBuffer, ActLen);
    QueryTrace();
}

#endif

void DoTheTests(void)
{
    /* Declarations ---------------------------------------------------------*/

    ST_DeviceName_t      DevName;
    STI2C_InitParams_t   I2cInitParams;
    STI2C_TermParams_t   I2cTermParams;
    STPIO_InitParams_t   PIOInitParams;
    STPIO_TermParams_t   PIOTermParams;
    ST_DeviceName_t      PIODevName           = "PIO1";
    STI2C_OpenParams_t   I2cOpenParams1, I2cOpenParams2;
    STI2C_Handle_t       *pHandle, Handle1, Handle2;
    STEVT_Handle_t       EVTHandle;
#if (!defined(PROCESSOR_C1))&&(!defined(ST_OS21))    
    U32                  ActLen;
#endif
    U32 b[16];
    S32                  i, n, NumValues, Value, Value2;
    U8                   c, Buffer[256];
    BOOL                 Finished=FALSE;
    char                 s[256];

    /* Initialisation ------------------------------------------------------*/

    strcpy( DevName, "I2C-0" );
    I2cInitParams.BaseAddress           = (U32*)SSC_0_BASE_ADDRESS;
    I2cInitParams.InterruptNumber       = SSC_0_INTERRUPT;
    I2cInitParams.InterruptLevel        = SSC_0_INTERRUPT_LEVEL;
    I2cInitParams.BaudRate              = STI2C_RATE_FASTMODE;
    I2cInitParams.MasterSlave           = STI2C_MASTER_OR_SLAVE;
    I2cInitParams.ClockFrequency        = I2cClockSpeed;
    I2cInitParams.PIOforSDA.BitMask     = PIO_BIT_0;
    I2cInitParams.PIOforSCL.BitMask     = PIO_BIT_2;
    I2cInitParams.MaxHandles            = 4;
#if defined(ST_OS21)    
    I2cInitParams.DriverPartition       = (ST_Partition_t *)system_partition_p;
#else    
    I2cInitParams.DriverPartition       = TEST_PARTITION_1;
#endif   
    strcpy( I2cInitParams.PIOforSDA.PortName, PIODevName );
    strcpy( I2cInitParams.PIOforSCL.PortName, PIODevName );
    I2cInitParams.SlaveAddress          = SLAVE_DEVICE_ID;
    strcpy( I2cInitParams.EvtHandlerName, EVTDevName0);

    PIOInitParams.BaseAddress           = (U32*)SSC_0_PIO_BASE;
    PIOInitParams.InterruptNumber       = SSC_0_PIO_INTERRUPT;
    PIOInitParams.InterruptLevel        = SSC_0_PIO_INTERRUPT_LEVEL;
#if defined(ST_OS21)    
    PIOInitParams.DriverPartition       = (ST_Partition_t *)system_partition_p;
#else    
    PIOInitParams.DriverPartition       = TEST_PARTITION_1;
#endif   

    I2cOpenParams1.BusAccessTimeOut      = 50;
    I2cOpenParams1.AddressType           = STI2C_ADDRESS_7_BITS;
    I2cOpenParams1.I2cAddress            = SLAVE_DEVICE_ID;
    I2cOpenParams2.BusAccessTimeOut      = 50;
    I2cOpenParams2.AddressType           = STI2C_ADDRESS_7_BITS;
    I2cOpenParams2.I2cAddress            = EEPROM_DEVICE_ID;

    I2cTermParams.ForceTerminate        = TRUE;
    PIOTermParams.ForceTerminate        = TRUE;

    /* Program --------------------------------------------------------------*/

    myprintf ("\nI2C DRIVER TEST SUITE - MASTER/SLAVE\n\n");

    srand (time_now());
    PrintPIOConfig();

    /* Set up the PIO */
    myprintf ("Initializing PIO for back I2C  :");
    CheckCodeOk (STPIO_Init( PIODevName, &PIOInitParams ) );

    PrintPIOConfig();

    /* Set up Event Handler */
    EVT_Init (EVTDevName0);

    myprintf ("Initializing I2C (Master Mode) :");
    CheckCodeOk (STI2C_Init (DevName, &I2cInitParams) );

    myprintf ("Open (Address 0x%02x)            :", I2cOpenParams1.I2cAddress);
    CheckCodeOk (STI2C_Open( DevName, &I2cOpenParams1, &Handle1) );
    myprintf ("Open (Address 0x%02x)            :", I2cOpenParams2.I2cAddress);
    CheckCodeOk (STI2C_Open( DevName, &I2cOpenParams2, &Handle2) );

    pHandle = &Handle1;
    EVT_Open (EVTDevName0, &EVTHandle);

    PrintPIOConfig();

    /* Main loop -----------------------------------------------------------*/

    while (!Finished)
    {
        Value = Value2 = 0;
        Quiet  = FALSE;

        /* Command format is: <c> <v1> <v2> <b0> <b1> ... <b7>
           Where :   <c>         is the command character
                     <v1> <v2>   are decimal numeric parameters. In the case of
                                 read & write, 1st parameter is transfer size,
                                 2nd parameter is repeat count.
                     <b0-7>      For writes only, are (optional) hex byte
                                 values to send.   */

        myprintf ("Command?\n");

#ifndef ST_OS21     
        n = (S32) debuggets (s, sizeof(s));
#endif         
        if (n<2)
            continue;
        NumValues = sscanf (s, "%c %d %d %2x %2x %2x %2x %2x %2x %2x %2x",
                   &c, &Value, &Value2,
                   &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &b[6], &b[7]) - 3;
        if (NumValues < 0)
            NumValues = 0;
        for (i=0; i<NumValues; i++)
            Buffer[i] = (U8) b[i];

        /* Determine user command & execute it */

        switch (c)
        {
        case 'h':                            /* Set handle */
            if (Value==1)
            {
                pHandle = &Handle1;
                myprintf ("Setting to Handle1 (Other Processor)\n");
            }
            else if (Value==2)
            {
                pHandle = &Handle2;
                myprintf ("Setting to Handle2 (Address %u)\n",
                           EEPROM_DEVICE_ID);
            }
            else
                myprintf ("Invalid or missing handle number\n");
            break;

        case 'd':                            /* Set loop delay */
            if (Value2 <= Value)
            {
                myprintf ("Changing LoopDelay from %u to %u\n",
                          LoopDelay, Value);
                LoopDelay = Value;
                RandomDelay = FALSE;
            }
            else
            {
                myprintf ("Delay changed to random value between %u & %u\n",
                           Value, Value2);
                RandomDelay = TRUE;
                DelayMin   = Value;
                DelayRange = Value2 - Value + 1;
                srand (time_now());
            }
            break;

        case 'w':                            /* Write */
#ifndef ST_OS21           
            DoTransfer (WRITE_TRANSFER, *pHandle, Value, Value2,
                        Buffer, NumValues);
#endif                        
            break;

        case 'o':                            /* Write No Stop */
#ifndef ST_OS21                     
            DoTransfer (WRITE_NO_STOP_TRANSFER, *pHandle, Value, Value2,
                        Buffer, NumValues);
#endif             
            break;

        case 'r':                            /* Read */
#ifndef ST_OS21                     
            DoTransfer (READ_TRANSFER, *pHandle, Value, Value2,
                        Buffer, NumValues);
#endif             
            break;

        case 'i':                            /* Read No Stop */
#ifndef ST_OS21                     
            DoTransfer (READ_NO_STOP_TRANSFER, *pHandle, Value, Value2,
                        Buffer, NumValues);
#endif             
            break;

        case 's':                            /* Sync with other processor */
#if (!defined(PROCESSOR_C1))&&(!defined(ST_OS21))
            myprintf ("Writing time message   :");
            Value = GetHpClock();
            CheckCodeOk (STI2C_Write (*pHandle, (U8*)&Value, 4, 2, &ActLen));
            myprintf ("%d bytes actually written, time sent= %u (0x%08x)\n",
                      ActLen, Value, Value);
            QueryTrace();
#endif
            break;

        case 'e':                            /* Print event info */
#ifndef ST_OS21               
            PrintEvents ( );
#endif            
            break;

        case 'p':                          /* Print trace info */
#ifndef ST_OS21               
            PrintTrace (Value, Value2);
#endif            
            break;

        case 't':                          /* Terminate & Reinitialize I2C */
            myprintf ("Reinitialize PIO?\n");
#ifndef ST_OS21            
            c = GetNextChar();
#endif               
            PrintPIOConfig();
            myprintf ("Terminating                     :");
            CheckCodeOk (STI2C_Term( DevName, &I2cTermParams ) );
            PrintPIOConfig();
            if (c == 'y')
            {
                myprintf ("Terminating  PIO                :");
                CheckCodeOk (STPIO_Term (PIODevName, &PIOTermParams) );
                PrintPIOConfig();
                myprintf ("Initialising PIO                :");
                CheckCodeOk (STPIO_Init ( PIODevName, &PIOInitParams ) );
                PrintPIOConfig();
            }
            myprintf ("Initializing I2C (Master Mode)  :");
            CheckCodeOk (STI2C_Init (DevName, &I2cInitParams) );
            PrintPIOConfig();
            myprintf ("Open (Address 0x%02x)             :",
                       I2cOpenParams1.I2cAddress);
            CheckCodeOk (STI2C_Open( DevName, &I2cOpenParams1, &Handle1) );
            myprintf ("Open (Address 0x%02x)             :",
                       I2cOpenParams2.I2cAddress);
            CheckCodeOk (STI2C_Open( DevName, &I2cOpenParams2, &Handle2) );
            break;

        case 'q':                         /* Quit */
            Finished = TRUE;
            break;

        case 'x':                         /* Set expected next recevied byte */
            myprintf ("Changing next rx value to 0x%02x (was 0x%02x)\n",
                      Value, RxValue);
            RxValue = (U8) Value;
            break;

        case 'z':                         /* Reset I2C */
            myprintf ("Reset I2C                       :");
            CheckCodeOk (STI2C_Reset (*pHandle));
            break;

        default:
            myprintf ("Invalid command - '%c'\n", c);
        }
    }       /* end while */

    myprintf ("Terminating          :");
    CheckCodeOk (STI2C_Term( DevName, &I2cTermParams ) );
}
/* EOF */



