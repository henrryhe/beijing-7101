/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005
Source file name : stspimstest.c

SPI tests - master/slave tests.

The program initially polls the other processor for a 1 byte message, which
should contain a special value. This indicates that the other processor has
started running. When it gets this message, the main part of the test is
entered.

The program then enters a loop sending and receiving messages. Each time around
the loop, it sends message of a random length and content to the other
processor, as a master mode transfer, and then attempts to read a message in
response (of random length less than or equal to the message written). The
response message should have a specific content (see below). There is a
variable random delay between loop iterations.

In parallel with this, the program must respond to slave mode requests from
the other processor (which will be trying to do to us what we are doing to it).
These requests are signalled as events, and come in at interrupt level.

The test passes only if all messages are send and received without error,
and the content of all received messages is as expected.

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
#include "stcommon.h"
#include "stspi.h"
#include "stspitest.h"

extern ST_Partition_t *system_partition;

extern ST_DeviceName_t PIODevName[];
/* Enum of PIO devices */
enum
{
    PIO_DEVICE_0,
    PIO_DEVICE_1,
    PIO_DEVICE_2,
    PIO_DEVICE_3,
    PIO_DEVICE_4,
    PIO_DEVICE_5,
    PIO_DEVICE_6,
    PIO_DEVICE_7,
    PIO_DEVICE_NOT_USED
};

/* Private types/constants ----------------------------------------------- */
/* Parameters of SSC1,SSC2 */
#ifndef SSC_2_INTERRUPT_LEVEL
#define SSC_2_INTERRUPT_LEVEL           5
#endif

#ifndef PIO_4_INTERRUPT_LEVEL
#define PIO_4_INTERRUPT_LEVEL           2
#endif

#if defined(ST_5100) || defined(ST_5301) || defined(ST_8010)

#define SSC1_BASE_ADDRESS       SSC_1_BASE_ADDRESS
#define SSC1_INTERRUPT          SSC_1_INTERRUPT
#define SSC1_INTERRUPT_LEVEL    SSC_1_INTERRUPT_LEVEL

#elif defined(ST_7100) || defined(ST_7109)

#define SSC1_BASE_ADDRESS       SSC_0_BASE_ADDRESS
#define SSC1_INTERRUPT          SSC_0_INTERRUPT
#define SSC1_INTERRUPT_LEVEL    SSC_0_INTERRUPT_LEVEL

#endif

#if defined(ST_5100) || defined(ST_5301) || defined(ST_8010)

#ifndef SSC_2_INTERRUPT
#define SSC_2_INTERRUPT        ST5100_SSC2_INTERRUPT
#endif

#define SSC2_BASE_ADDRESS       SSC_2_BASE_ADDRESS
#define SSC2_INTERRUPT          SSC_2_INTERRUPT
#define SSC2_INTERRUPT_LEVEL    SSC_2_INTERRUPT_LEVEL

#elif defined(ST_7100) || defined(ST_7109)

#define SSC2_BASE_ADDRESS       SSC_1_BASE_ADDRESS
#define SSC2_INTERRUPT          SSC_1_INTERRUPT
#define SSC2_INTERRUPT_LEVEL    SSC_1_INTERRUPT_LEVEL

#endif

#if defined(ST_5100) || defined(ST_5301)

#define SSC1_MTSR_BIT           PIO_BIT_2
#define SSC1_MTSR_DEV           PIO_DEVICE_3

#define SSC1_MRST_BIT           PIO_BIT_0
#define SSC1_MRST_DEV           PIO_DEVICE_4

#define SSC1_SCL_BIT            PIO_BIT_3
#define SSC1_SCL_DEV            PIO_DEVICE_3


#define SSC2_MTSR_BIT           PIO_BIT_3
#define SSC2_MTSR_DEV           PIO_DEVICE_4

#define SSC2_MRST_BIT           PIO_BIT_1
#define SSC2_MRST_DEV           PIO_DEVICE_4

#define SSC2_SCL_BIT            PIO_BIT_2
#define SSC2_SCL_DEV            PIO_DEVICE_4

#define MASTER_CS_BIT           PIO_BIT_5
#define MASTER_CS_DEV           PIO_DEVICE_3

#define SLAVE_CS_BIT            PIO_BIT_7
#define SLAVE_CS_DEV            PIO_DEVICE_3

#elif defined(ST_7100) || defined(ST_7109)

#define SSC1_MTSR_BIT           PIO_BIT_1
#define SSC1_MTSR_DEV           PIO_DEVICE_2

#define SSC1_MRST_BIT           PIO_BIT_2
#define SSC1_MRST_DEV           PIO_DEVICE_2

#define SSC1_SCL_BIT            PIO_BIT_0
#define SSC1_SCL_DEV            PIO_DEVICE_2


#define SSC2_MTSR_BIT           PIO_BIT_1
#define SSC2_MTSR_DEV           PIO_DEVICE_3

#define SSC2_MRST_BIT           PIO_BIT_2
#define SSC2_MRST_DEV           PIO_DEVICE_3

#define SSC2_SCL_BIT            PIO_BIT_0
#define SSC2_SCL_DEV            PIO_DEVICE_3

#define MASTER_CS_BIT           PIO_BIT_5
#define MASTER_CS_DEV           PIO_DEVICE_3

#define SLAVE_CS_BIT            PIO_BIT_7
#define SLAVE_CS_DEV            PIO_DEVICE_3

#elif defined(ST_8010)

#define SSC1_MTSR_BIT           0            //SPI1 is not through PIO
#define SSC1_MTSR_DEV           0            //pins.So no need to give
                                             //PIO dev name and Pins.
#define SSC1_MRST_BIT           0
#define SSC1_MRST_DEV           0

#define SSC1_SCL_BIT            0
#define SSC1_SCL_DEV            0

#define SSC2_MTSR_BIT           PIO_BIT_5
#define SSC2_MTSR_DEV           PIO_DEVICE_1

#define SSC2_MRST_BIT           PIO_BIT_6
#define SSC2_MRST_DEV           PIO_DEVICE_1

#define SSC2_SCL_BIT            PIO_BIT_7
#define SSC2_SCL_DEV            PIO_DEVICE_1

#define MASTER_CS_BIT           PIO_BIT_5
#define MASTER_CS_DEV           PIO_DEVICE_3

#define SLAVE_CS_BIT            PIO_BIT_7
#define SLAVE_CS_DEV            PIO_DEVICE_3

#endif

#define MAGIC_RESPONSE         0x2A
#define MESSAGE_COUNT          10
#define ERROR_ABORT_THRESHOLD  100
#define MAX_PHASE              10

int MagicResponse(U16 Val,U8 DataWidth)
{

   Val &= (~(0xFFFF << DataWidth));
   return(Val);
}

/* LUT terminator */
#define ST_ERROR_UNKNOWN            ((U32)-1)

#undef  VERBOSE
#define PRINTV                 if (Verbose) myprintf

typedef ST_ErrorCode_t (*SPIFunc_t) (STSPI_Handle_t, U16*, U32, U32, U32*);

/* Error messages */
typedef struct SPI_ErrorMessage_s
{
    ST_ErrorCode_t Error;
    char ErrorMsg[56];
} SPI_ErrorMessage_t;

#define UNKNOWN_ERROR            ((U32)-1)

static SPI_ErrorMessage_t SPI_ErrorLUT[] =
{
    { STSPI_ERROR_PIO, "STSPI_ERROR_PIO" },
    { STSPI_ERROR_EVT_REGISTER, "STSPI_ERROR_EVT_REGISTER" },

    /* STAPI */
    { ST_NO_ERROR, "ST_NO_ERROR" },
    { ST_ERROR_NO_MEMORY, "ST_ERROR_NO_MEMORY" },
    { ST_ERROR_INTERRUPT_INSTALL, "ST_ERROR_INTERRUPT_INSTALL" },
    { ST_ERROR_ALREADY_INITIALIZED, "ST_ERROR_DEVICE_INITIALIZED" },
    { ST_ERROR_UNKNOWN_DEVICE, "ST_ERROR_UNKNOWN_DEVICE" },
    { ST_ERROR_BAD_PARAMETER, "ST_ERROR_BAD_PARAMETER" },
    { ST_ERROR_OPEN_HANDLE, "ST_ERROR_OPEN_HANDLE" },
    { ST_ERROR_NO_FREE_HANDLES, "ST_ERROR_NO_FREE_HANDLES" },
    { ST_ERROR_INVALID_HANDLE, "ST_ERROR_INVALID_HANDLE" },
    { ST_ERROR_INTERRUPT_UNINSTALL, "ST_ERROR_INTERRUPT_UNINSTALL" },
    { ST_ERROR_TIMEOUT, "ST_ERROR_TIMEOUT" },
    { ST_ERROR_FEATURE_NOT_SUPPORTED, "ST_ERROR_FEATURE_NOT_SUPPORTED" },
    { ST_ERROR_DEVICE_BUSY, "ST_ERROR_DEVICE_BUSY" },
    { UNKNOWN_ERROR, "UNKNOWN ERROR" } /* Terminator */
};

/* Private variables ------------------------------------------------------ */

static U32     ErrorCount [MAX_PHASE]; /* Overall count of errors */
static U32     MessageNumber = 0;
static U32     TestPhase     = 0;
static BOOL    MessageNumPrinted = FALSE;
static BOOL    MasterFirst       = FALSE;
/* Set TRUE to generate extra output for debug etc. */
static BOOL    Verbose      =
#ifdef VERBOSE
                             TRUE;
#else
                             FALSE;
#endif

static U32 TicksPerSec;
/* Only one evt device required, since we do device registration/subscriber */
static ST_DeviceName_t   EVTDevName0 = "EVT0";
static STSPI_Handle_t    MasterHandle,SlaveHandle;


/* Data for slave mode operation */
static U16   RxBuffer [1024];
static U32   TxNotifyCount=0 , TxCompleteCount=0 , TxByteCount=0;
static U32   RxCompleteCount=0;
static U32   MasterRxCount=0,MasterTxCount=0,RxIndex=0, TxIndex=0;

/* Function prototypes ----------------------------------------------------- */

extern int MasterSlave (void);
void myprintf (char *format, ...);

/*---------------------------------------------------------------------------*/
/* Other utility functions for checking various things.                      */
/*---------------------------------------------------------------------------*/

/* Return the error from the lookup table */
static char *SPI_ErrorString(ST_ErrorCode_t Error)
{
    SPI_ErrorMessage_t *mp = NULL;

    /* Search lookup table for error code*/
    mp = SPI_ErrorLUT;
    while (mp->Error != UNKNOWN_ERROR)
    {
        if (mp->Error == Error)
        {
            break;
        }
        mp++;
    }

    /* Return error message string */
    return mp->ErrorMsg;
}

static U32 IncrementErrorCount (void)
{
    ErrorCount [TestPhase]++;
    return ErrorCount [TestPhase];
}

static void PrintMessageNum (void)
{
    if ( (!MessageNumPrinted) && (MessageNumber != 0) )
    {
        myprintf ("\n**** Message %u ****\n", MessageNumber);
        MessageNumPrinted = TRUE;
    }
}


static void PrintError (char *msg, U32 v1, U32 v2)
{
    (void)IncrementErrorCount();
    PrintMessageNum();
    myprintf ("ERROR: ");
    myprintf (msg, v1, v2);
    myprintf ("\n");
}


static BOOL CheckCode (char *msg,
                       ST_ErrorCode_t actualCode, ST_ErrorCode_t expectedCode)
{
    if (actualCode == expectedCode)
    {
        if (Verbose)
        {
             PrintMessageNum();
             myprintf ("%-30s: Got expected code %d\n", msg, expectedCode);
        }
        return TRUE;
    }
    else
    {
        PrintMessageNum();
        myprintf ("%-30s: ERROR: got code %s, expected %s\n", msg,
                  SPI_ErrorString(actualCode),
                  SPI_ErrorString(expectedCode) );
        (void)IncrementErrorCount();
        return FALSE;
    }
}


static BOOL CheckCodeOk (char *msg, ST_ErrorCode_t actualCode)
{
    return (CheckCode (msg, actualCode, ST_NO_ERROR));
}


void myprintf (char *format, ...)
{
    /* Local equivalent of printf, which can be defined to use STTBX or
       debug functions. */
    char s[256];
    va_list v;

    va_start (v, format);
    vsprintf (s, format, v);
#if !defined(STSPI_NO_TBX)
    STTBX_Print ((s));
#else
    printf(s);
#endif
    va_end (v);
}


static void PrintBuffer (U16 *Buffer, U32 Size, BOOL PrintFlag)
{
    /* Print contents of data buffer in hex, if PrintFlag is TRUE. */
    U32 i;
    char s[8], Line[128];

    if (PrintFlag)
    {
        Line[0] = 0;           /* empty string */
        for (i=0; i<Size; i++)
        {
            sprintf (s, "%02x ", (U8)Buffer[i]);
            strcat (Line, s);
            if ( (((i+1) % 16) == 0) || (i==Size-1) )
            {
                myprintf ("%s\n", Line);
                Line[0] = 0;
            }
        }
    }
}

#if 0
/* Estimate timeout required for an SPI transfer of size TransferSize */
static U32 GetSpiTimeout (U32 TransferSize)
{
    return (((TransferSize * 1000) / (STSPI_RATE_NORMAL / 40) + 2)*10);
}
#endif

static U32 MyRand (U32 Max)
{
    /* Return random number in range 0 .. Max-1 */
    return (rand() % Max);
}


/*--------------------------------------------------------------------------*/
/* Callbacks                                                                */
/*--------------------------------------------------------------------------*/

void TxNotify (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant_t Event, const void *EventData,
               const void *SubscriberData_p)
{
    TxNotifyCount++;
    TxIndex = 0;
}

void RxNotify (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant_t Event, const void *EventData,
               const void *SubscriberData_p)
{
    RxIndex = 0;
}

void TxComplete (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event, const void *EventData,
                 const void *SubscriberData_p)
{
    TxCompleteCount++;
}

void RxComplete (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event, const void *EventData,
                 const void *SubscriberData_p)
{
    RxCompleteCount++;
}

void TxByte (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
             STEVT_EventConstant_t Event, const void *EventData,
             const void *SubscriberData_p)
{
    if (TxNotifyCount == 1)
    {
        *(U8*)EventData = MAGIC_RESPONSE;   /* No message to reply to */
    }
    else
    {
        if (TxIndex==0)    /* 1st byte of response is last length recvd */
        {
            *(U8*)EventData = (U8) RxIndex;
        }
        else
        {
            *(U8*)EventData = (U8) (RxBuffer [TxIndex-1] + 1);
        }
        TxIndex++;
        TxByteCount++;
    }
}

void RxByte (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
             STEVT_EventConstant_t Event, const void *EventData,
             const void *SubscriberData_p)
{
    U8  Data = *(U8*)EventData;
    RxBuffer [RxIndex++] = Data;
}

/*--------------------------------------------------------------------------*/
/*  Event Handler initialisation                                            */
/*--------------------------------------------------------------------------*/

static BOOL EVT_Init (ST_DeviceName_t EVTDeviceName)
{
    STEVT_InitParams_t              EVTInitParams;

    EVTInitParams.ConnectMaxNum   = 10;
    EVTInitParams.EventMaxNum     = 40;
    EVTInitParams.SubscrMaxNum    = 40;
    EVTInitParams.MemoryPartition = system_partition;

    return (CheckCodeOk ("Initialising Event Handler",
                         STEVT_Init( EVTDeviceName, &EVTInitParams ) ));
}


static void EVT_Open (ST_DeviceName_t EVTDeviceName, STEVT_Handle_t *pHandle)
{
    STEVT_OpenParams_t              EVTOpenParams;
    STEVT_DeviceSubscribeParams_t   TxNotifySub,   RxNotifySub,
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

    (void)CheckCodeOk ("Open Event handler",
                       STEVT_Open (EVTDevName0, &EVTOpenParams, pHandle) );

    (void)CheckCodeOk ("Subscribe TxNotify",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-1",
                                   STSPI_TRANSMIT_NOTIFY_EVT,
                                   &TxNotifySub) );

    (void)CheckCodeOk ("Subscribe RxNotify",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-1",
                                   STSPI_RECEIVE_NOTIFY_EVT,
                                   &RxNotifySub) );

    (void)CheckCodeOk ("Subscribe TxByte",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-1",
                                   STSPI_TRANSMIT_BYTE_EVT,
                                   &TxByteSub) );

    (void)CheckCodeOk ("Subscribe RxByte",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-1",
                                   STSPI_RECEIVE_BYTE_EVT,
                                   &RxByteSub) );

    (void)CheckCodeOk ("Subscribe TxComplete",
                      STEVT_SubscribeDeviceEvent (*pHandle, "SPI-1",
                                   STSPI_TRANSMIT_COMPLETE_EVT,
                                   &TxCompleteSub) );

    (void)CheckCodeOk ("Subscribe RxComplete",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-1",
                                   STSPI_RECEIVE_COMPLETE_EVT,
                                   &RxCompleteSub) );

    (void)CheckCodeOk ("Subscribe TxNotify",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-2",
                                   STSPI_TRANSMIT_NOTIFY_EVT,
                                   &TxNotifySub) );

    (void)CheckCodeOk ("Subscribe RxNotify",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-2",
                                   STSPI_RECEIVE_NOTIFY_EVT,
                                   &RxNotifySub) );

    (void)CheckCodeOk ("Subscribe TxByte",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-2",
                                   STSPI_TRANSMIT_BYTE_EVT,
                                   &TxByteSub) );

    (void)CheckCodeOk ("Subscribe RxByte",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-2",
                                   STSPI_RECEIVE_BYTE_EVT,
                                   &RxByteSub) );

    (void)CheckCodeOk ("Subscribe TxComplete",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-2",
                                   STSPI_TRANSMIT_COMPLETE_EVT,
                                   &TxCompleteSub) );

    (void)CheckCodeOk ("Subscribe RxComplete",
                       STEVT_SubscribeDeviceEvent (*pHandle, "SPI-2",
                                   STSPI_RECEIVE_COMPLETE_EVT,
                                   &RxCompleteSub) );

}

/*--------------------------------------------------------------------------*/
/*  Test subroutines                                                        */
/*--------------------------------------------------------------------------*/

static BOOL WaitForResponse (U8 DataWidth)
{
    /* Synchronize with other processor, by repeatedly reading a single byte
       message from it. When a specific byte is successfully read, it can be
       safely assumed that the other processor is also at a synchronization
       point. This function is called on startup & at the end of each test. */

    ST_ErrorCode_t     Code;
    U32                ActLen, WaitCount=0;
    U16                Byte;

    myprintf ("Synchronizing with other processor....\n");

    while (1)
    {
        Byte = 0;
        Code = STSPI_Read(MasterHandle,&Byte, 1, 100, &ActLen);
        MasterFirst = (TxNotifyCount == 0);

        STSPI_Print(("Code = %s, Length = %d, Data = 0x%02x\n",
                      SPI_ErrorString(Code), ActLen, Byte));


        task_delay (TicksPerSec * 2);
        if ( (Code == ST_NO_ERROR) && (Byte == (U16)MagicResponse(MAGIC_RESPONSE,DataWidth)) )
        {
            myprintf ("\n*** Synchronization successful ***\n\n");
            return TRUE;
        }
        else
        {
            myprintf (".");
            PRINTV("%s\n", SPI_ErrorString(Code));
        }

        if (++WaitCount > 30)
        {
            (void)IncrementErrorCount();
            myprintf ("\nERROR: Giving up waiting for response\n");
            return FALSE;
        }
    }

}


static BOOL SendMessagesAndCheckResponses (U8 DataWidth)
{
    /* Writes messages to the other processor of random size and content,
       then reads and checks a response to each. */

    ST_ErrorCode_t       Code;
    BOOL                 ErrorThisTime;
    U32                  ActLen, WriteLength, ReadLength;
    U32                  j;
    U16                  WriteBuffer[1024], ReadBuffer[1024]= {0};
    SPIFunc_t            WriteFunc, ReadFunc;

    myprintf ("Writing & Reading %u messages to other processor...\n",
              MESSAGE_COUNT);

    WriteFunc = (SPIFunc_t) STSPI_Write;
    ReadFunc  = STSPI_Read;

    for (MessageNumber = 1; MessageNumber <= MESSAGE_COUNT; MessageNumber++)
    {
        MessageNumPrinted = FALSE;
        if (Verbose)
        {
            PrintMessageNum();
        }
        ErrorThisTime = FALSE;
        ActLen = 0;

        /* Setup message of random length & contents & send to other device */
        WriteLength = 1 + MyRand(3);
        WriteBuffer[0] = MagicResponse(0x06,DataWidth);
        for (j=1; j<= WriteLength; j++)
        {
            WriteBuffer[j] = MagicResponse(MyRand(8),DataWidth);
         }

        PRINTV ("Sending %u bytes\n", WriteLength);

        Code = WriteFunc (MasterHandle, WriteBuffer, WriteLength+1,
                          1000000, &ActLen);
        if ( CheckCodeOk ("SPI Write", Code ) )
        {
            MasterTxCount += ActLen;
            if (ActLen != WriteLength + 1)
            {
                PrintError ("Write length = %u, actual length = %u",
                            WriteLength+1, ActLen);

            }
            else   /* length written ok */
            {
                /* Now read response of random length & check it.
                   Byte 0 should contain length of last message sent to it.
                   Bytes N should contain value of byte(N-1) + 1.  */

                ActLen = 0;
                ReadLength = 1 + MyRand (WriteLength);

                Code = ReadFunc (MasterHandle, ReadBuffer, ReadLength,
                                 /*GetSpiTimeout (ReadLength)*/1000000, &ActLen);
                if (CheckCodeOk ("SPI Read", Code))
                {
                    MasterRxCount += ActLen;
                    PRINTV ("Read %u bytes\n", ActLen);
                    if (ActLen != ReadLength)
                    {
                        ErrorThisTime = TRUE;
                        PrintError ("Read length = %u, actual length = %u",
                                     ReadLength, ActLen);

                    }
                    /* Check length byte */
                    else if (ReadBuffer[0] != MagicResponse(WriteLength,DataWidth))
                    {
                        ErrorThisTime = TRUE;
                        PrintError ("Byte 0 contains incorrect length! (%u)",
                                     (U32) ReadBuffer[0], 0);


                    }
                    else     /* Check other bytes received */
                    {
                        for (j=1; j<ReadLength; j++)
                        {
                            if (ReadBuffer[j] != (MagicResponse(WriteBuffer[j],DataWidth) + 1))
                            {
                                ErrorThisTime = TRUE;
                                PrintError ("Byte %u of data incorrect", j, 0);
                                if (ReadBuffer[j] == 0xFF)
                                {
                                    myprintf ("(Failure consistent with h/w bug)\n");
                                }

                                break;
                            }
                        }

                        if (j >= ReadLength)
                        {
                            PRINTV ("*** Message %u matches ***\n",
                                    MessageNumber);
                        }
                    }
                }

            }

            if (Verbose || ErrorThisTime)
            {
                myprintf ("Data written:\n");
                PrintBuffer (&WriteBuffer[1], WriteLength, TRUE);
                myprintf ("Data read:\n");
                PrintBuffer (ReadBuffer, ActLen, TRUE);
            }
        }

        if (ErrorCount [TestPhase] >= ERROR_ABORT_THRESHOLD)
        {
            myprintf ("\n*** Too many errors, aborting ***\n\n");
            return FALSE;
        }

        /* Random delay between iterations */
        task_delay (20 + MyRand(100));
    }

    return TRUE;
}



/**************************************************************************
       THIS FUNCTION WILL CALL ALL THE TEST ROUTINES...
**************************************************************************/


int MasterSlave (void)
{
    /* Declarations ---------------------------------------------------------*/

    ST_DeviceName_t      DevName;
    STSPI_InitParams_t   SPIInitParams;
    STSPI_TermParams_t   SPITermParams;
    STSPI_OpenParams_t   SPIOpenParams;
    STSPI_Params_t       SPISetParams;
    STEVT_TermParams_t   EVTTermParams;
    STEVT_Handle_t       EVTHandle;
    U32                  j;
    ST_ClockInfo_t       ClockInfo;


    ST_GetClockInfo(&ClockInfo);
    STSPI_Print(("Comms block frequency: %d\n", ClockInfo.CommsBlock));

    /* Initialisation ------------------------------------------------------*/

    strcpy( DevName, "SPI-1" );
    SPIInitParams.BaseAddress           = (U32*)SSC1_BASE_ADDRESS;
    SPIInitParams.InterruptNumber       = SSC1_INTERRUPT;
    SPIInitParams.InterruptLevel        = SSC1_INTERRUPT_LEVEL;
    SPIInitParams.MasterSlave           = STSPI_SLAVE;
    SPIInitParams.PIOforSCL.BitMask     = SSC1_SCL_BIT;
    SPIInitParams.PIOforMTSR.BitMask    = SSC1_MTSR_BIT;
    SPIInitParams.PIOforMRST.BitMask    = SSC1_MRST_BIT;
    SPIInitParams.MaxHandles            = 2;
    SPIInitParams.DriverPartition       = system_partition;
    SPIInitParams.BusAccessTimeout      = 20;
    SPIInitParams.DefaultParams         = NULL;
    SPIInitParams.ClockFrequency        = ClockInfo.CommsBlock;
    strcpy( SPIInitParams.PIOforSCL.PortName, PIODevName[SSC1_SCL_DEV] );
    strcpy( SPIInitParams.PIOforMTSR.PortName, PIODevName[SSC1_MTSR_DEV] );
    strcpy( SPIInitParams.PIOforMRST.PortName, PIODevName[SSC1_MRST_DEV] );
    strcpy( SPIInitParams.EvtHandlerName, EVTDevName0);

    SPIOpenParams.PIOforCS.BitMask      = SLAVE_CS_BIT;
    strcpy( SPIOpenParams.PIOforCS.PortName, PIODevName[SLAVE_CS_DEV] );
    SPITermParams.ForceTerminate        = TRUE;


    /* Program --------------------------------------------------------------*/

    myprintf ("\nSPI DRIVER TEST SUITE - MASTER/SLAVE\n\n");

    srand (time_now());
    for (j=0; j<MAX_PHASE; j++)
    {
        ErrorCount[j] = 0;
    }

    /* Set up Event Handler */
    (void)EVT_Init (EVTDevName0);

    /* Initialized SSC1 */
    {
        char TmpString[80];

        sprintf(TmpString, "Initializing %s", DevName);
        (void)CheckCodeOk( TmpString, STSPI_Init (DevName, &SPIInitParams) );
        sprintf(TmpString, "Opening %s", DevName);
        (void)CheckCodeOk( TmpString, STSPI_Open (DevName, &SPIOpenParams,  &SlaveHandle) );
    }


    /* Need to initialize SSC2 as well */
    strcpy( DevName, "SPI-2" );
    SPIInitParams.BaseAddress           = (U32*)SSC2_BASE_ADDRESS;
    SPIInitParams.InterruptNumber       = SSC2_INTERRUPT;
    SPIInitParams.InterruptLevel        = SSC2_INTERRUPT_LEVEL;
    SPIInitParams.PIOforMTSR.BitMask    = SSC2_MTSR_BIT;
    SPIInitParams.PIOforMRST.BitMask    = SSC2_MRST_BIT;
    SPIInitParams.PIOforSCL.BitMask     = SSC2_SCL_BIT;
    SPIInitParams.MasterSlave           = STSPI_MASTER;
    SPIInitParams.ClockFrequency        = ClockInfo.CommsBlock;
    SPIInitParams.BusAccessTimeout      = 20;
    SPIInitParams.DefaultParams         = NULL;
    strcpy( SPIInitParams.PIOforSCL.PortName,  PIODevName[SSC2_SCL_DEV] );
    strcpy( SPIInitParams.PIOforMTSR.PortName, PIODevName[SSC2_MTSR_DEV] );
    strcpy( SPIInitParams.PIOforMRST.PortName, PIODevName[SSC2_MRST_DEV] );


    SPIOpenParams.PIOforCS.BitMask      = MASTER_CS_BIT;
    strcpy( SPIOpenParams.PIOforCS.PortName, PIODevName[MASTER_CS_DEV] );

    /* Initialize and Open SSC1 */
    {
        char TmpString[80];

        sprintf(TmpString, "Initializing %s", DevName);
        (void)CheckCodeOk( TmpString, STSPI_Init (DevName, &SPIInitParams) );
        sprintf(TmpString, "Opening %s", DevName);
        (void)CheckCodeOk( TmpString, STSPI_Open (DevName, &SPIOpenParams,  &MasterHandle) );

    }


    EVT_Open (EVTDevName0, &EVTHandle);

    /* Abort if any errors during initialisation (phase 0) */
    if (ErrorCount[0] > 0)
    {
        return(1);
    }

    TestPhase = 1;

    if (!WaitForResponse(8))
    {
        return(1);
    }

    /* Got initial response, start tests */
    myprintf ("******** PHASE 1 : Master writes & reads to Slave **********\n\n");

    if (!SendMessagesAndCheckResponses(8))
    {
        return(1);
    }


    myprintf ("Test phase 1 complete, %u errors detected\n\n",
              ErrorCount [TestPhase]);
    TxNotifyCount = 0;

    myprintf ("************* PHASE 2 : Performance test *******************\n\n");
    TestPhase = 2;

    myprintf ("----------------- FASTMODE --------------------------------- \n\n");
    SPISetParams.BaudRate    = STSPI_RATE_FASTMODE;
    SPISetParams.MSBFirst    = TRUE;
    SPISetParams.ClkPhase    = TRUE;
    SPISetParams.Polarity    = TRUE;
    SPISetParams.DataWidth   = 8;
    SPISetParams.GlitchWidth = 0;

    (void)CheckCodeOk ("Changing to FASTMODE", STSPI_SetParams(MasterHandle,&SPISetParams));
    (void)CheckCodeOk ("Changing to FASTMODE", STSPI_SetParams(SlaveHandle,&SPISetParams));

    /* Synchronize with other processor ready for next phase */
    if (!WaitForResponse(SPISetParams.DataWidth))
    {
        return(1);
    }

    if (!SendMessagesAndCheckResponses(SPISetParams.DataWidth))
    {
        return(1);
    }

    myprintf ("Tested in FASTMODE, %u errors detected\n\n",
               ErrorCount [TestPhase]);
    TxNotifyCount = 0;

    myprintf ("----------------- DataWidths ------------------------------- \n\n");

    SPISetParams.BaudRate    = STSPI_RATE_NORMAL;
    SPISetParams.MSBFirst    = TRUE;
    SPISetParams.DataWidth   = 8;
    SPISetParams.GlitchWidth = 0;
    SPISetParams.ClkPhase    = TRUE; /* Ph = 1 */
    SPISetParams.Polarity    = TRUE; /* Po = 1 */
    (void)CheckCodeOk ("Datawidth =8", STSPI_SetParams(MasterHandle,&SPISetParams));
    (void)CheckCodeOk ("Datawidth =8", STSPI_SetParams(SlaveHandle,&SPISetParams));

    /* Synchronize with other processor ready for next phase */
    if (!WaitForResponse(SPISetParams.DataWidth))
    {
        return(1);
    }

    if (!SendMessagesAndCheckResponses(SPISetParams.DataWidth))
    {
        return(1);
    }

    myprintf ("Tested with DataWidth = 8, %u errors detected\n\n",
               ErrorCount [TestPhase]);

    TxNotifyCount = 0;

#if !defined(ST_7100) && !defined(ST_7109)
    SPISetParams.BaudRate    = STSPI_RATE_NORMAL;
    SPISetParams.MSBFirst    = TRUE;
    SPISetParams.DataWidth   = 9;
    SPISetParams.GlitchWidth = 0;
    SPISetParams.ClkPhase    = TRUE; /* Ph = 1 */
    SPISetParams.Polarity    = TRUE; /* Po = 1 */
    (void)CheckCodeOk ("Datawidth =9", STSPI_SetParams(MasterHandle,&SPISetParams));
    (void)CheckCodeOk ("Datawidth =9", STSPI_SetParams(SlaveHandle,&SPISetParams));

    /* Synchronize with other processor ready for next phase */
    if (!WaitForResponse(SPISetParams.DataWidth))
    {
        return(1);
    }

    if (!SendMessagesAndCheckResponses(SPISetParams.DataWidth))
    {
        return(1);
    }

    myprintf ("Tested with DataWidth = 9, %u errors detected\n\n",
               ErrorCount [TestPhase]);


    TxNotifyCount = 0;

    SPISetParams.DataWidth   = 11;
    (void)CheckCodeOk ("Datawidth =11", STSPI_SetParams(MasterHandle,&SPISetParams));
    (void)CheckCodeOk ("Datawidth =11", STSPI_SetParams(SlaveHandle,&SPISetParams));

    /* Synchronize with other processor ready for next phase */
    if (!WaitForResponse(SPISetParams.DataWidth))
    {
        return(1);
    }

    if (!SendMessagesAndCheckResponses(SPISetParams.DataWidth))
    {
        return(1);
    }

    myprintf ("Tested with DataWidth = 11, %u errors detected\n\n",
               ErrorCount [TestPhase]);

    TxNotifyCount = 0;

    SPISetParams.DataWidth   = 2;
    (void)CheckCodeOk ("Datawidth =2", STSPI_SetParams(MasterHandle,&SPISetParams));
    (void)CheckCodeOk ("Datawidth =2", STSPI_SetParams(SlaveHandle,&SPISetParams));

    /* Synchronize with other processor ready for next phase */
    if (!WaitForResponse(SPISetParams.DataWidth))
    {
        return(1);
    }

    if (!SendMessagesAndCheckResponses(SPISetParams.DataWidth))
    {
        return(1);
    }

    myprintf ("Tested with DataWidth = 2, %u errors detected\n\n",
               ErrorCount [TestPhase]);
#endif
    TxNotifyCount = 0;

    myprintf ("********* PHASE 3 : Ditto, Master & Slave switched *********\n\n");
    TestPhase = 3;

    (void)CheckCodeOk ("Terminating Master",STSPI_Term("SPI-2",&SPITermParams) );
    (void)CheckCodeOk ("Terminating Slave",STSPI_Term("SPI-1",&SPITermParams) );

    EVTTermParams.ForceTerminate        = TRUE;
    STEVT_Term(EVTDevName0,&EVTTermParams);
    EVT_Init (EVTDevName0);

    /* Change Master to Slave - SSC2 becomes SLAVE */
    strcpy( DevName, "SPI-2" );
    SPIInitParams.BaseAddress           = (U32*)SSC2_BASE_ADDRESS;
    SPIInitParams.InterruptNumber       = SSC2_INTERRUPT;
    SPIInitParams.InterruptLevel        = SSC2_INTERRUPT_LEVEL;
    SPIInitParams.PIOforMTSR.BitMask    = SSC2_MTSR_BIT;
    SPIInitParams.PIOforMRST.BitMask    = SSC2_MRST_BIT;
    SPIInitParams.PIOforSCL.BitMask     = SSC2_SCL_BIT;
    SPIInitParams.MasterSlave           = STSPI_SLAVE;
    SPIInitParams.ClockFrequency        = ClockInfo.CommsBlock;
    SPIInitParams.BusAccessTimeout      = 20;
    SPIInitParams.DefaultParams         = NULL;

    strcpy( SPIInitParams.EvtHandlerName, EVTDevName0);
    strcpy( SPIInitParams.PIOforSCL.PortName,  PIODevName[SSC2_SCL_DEV] );
    strcpy( SPIInitParams.PIOforMTSR.PortName, PIODevName[SSC2_MTSR_DEV] );
    strcpy( SPIInitParams.PIOforMRST.PortName, PIODevName[SSC2_MRST_DEV] );

    SPIOpenParams.PIOforCS.BitMask      = SLAVE_CS_BIT;
    strcpy( SPIOpenParams.PIOforCS.PortName, PIODevName[SLAVE_CS_DEV] );

    (void)CheckCodeOk ("Initialising Slave",STSPI_Init(DevName, &SPIInitParams));
    (void)CheckCodeOk ("Obtaining SlaveHandle",STSPI_Open(DevName,&SPIOpenParams, &SlaveHandle));

    /* Change Slave to Master - SSC1 now becomes MASTER */
    strcpy( DevName, "SPI-1" );
    SPIInitParams.BaseAddress           = (U32*)SSC1_BASE_ADDRESS;
    SPIInitParams.InterruptNumber       = SSC1_INTERRUPT;
    SPIInitParams.InterruptLevel        = SSC1_INTERRUPT_LEVEL;
    SPIInitParams.MasterSlave           = STSPI_MASTER;
    SPIInitParams.PIOforSCL.BitMask     = SSC1_SCL_BIT;
    SPIInitParams.PIOforMTSR.BitMask    = SSC1_MTSR_BIT;
    SPIInitParams.PIOforMRST.BitMask    = SSC1_MRST_BIT;
    SPIInitParams.MaxHandles            = 2;
    SPIInitParams.DriverPartition       = system_partition;
    SPIInitParams.BusAccessTimeout      = 20;
    SPIInitParams.DefaultParams         = NULL;
    SPIInitParams.ClockFrequency        = ClockInfo.CommsBlock;
    strcpy( SPIInitParams.PIOforSCL.PortName, PIODevName[SSC1_SCL_DEV] );
    strcpy( SPIInitParams.PIOforMTSR.PortName, PIODevName[SSC1_MTSR_DEV] );
    strcpy( SPIInitParams.PIOforMRST.PortName, PIODevName[SSC1_MRST_DEV] );

    SPIOpenParams.PIOforCS.BitMask      = MASTER_CS_BIT;
    strcpy( SPIOpenParams.PIOforCS.PortName, PIODevName[MASTER_CS_DEV] );

    (void)CheckCodeOk ("Initialising Master",STSPI_Init(DevName, &SPIInitParams));
    (void)CheckCodeOk ("Obtain MasterHandle",STSPI_Open(DevName, &SPIOpenParams, &MasterHandle));

    EVT_Open (EVTDevName0, &EVTHandle);

    /* Synchronize with other processor ready for next phase */
    if (!WaitForResponse(8))
    {
        return(1);
    }


    if (!SendMessagesAndCheckResponses(8))
    {
        return(1);
    }

    myprintf ("Test phase 3 complete, %u errors detected\n\n",
              ErrorCount [TestPhase]);
    TxNotifyCount = 0;

    (void)CheckCodeOk ("Terminating Master",STSPI_Term("SPI-1",&SPITermParams) );
    (void)CheckCodeOk ("Terminating Slave",STSPI_Term("SPI-2",&SPITermParams) );

    STSPI_Print(( "Master Slave test result is " ));
    STSPI_Print(( "%d errors\n", ErrorCount[TestPhase] ));
    return(ErrorCount[TestPhase]);
}
/* EOF */

