/*******************************************************************************

File name   : tbx_buff.c

Description : Source of data output management

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
06 Apr 2000         Creation                                         FQ
24 Jul 2000         3 modes: no buffer, old. buffer, new buffer      FQ
28 Jan 2002         DDTS GNBvd10702 "Use ST_GetClocksPerSecond       HS
 *                  instead of Hardcoded value"
 *                  DDTS GNBvd04464 "Message buffering generates
 *                  erratic outputs"
 *                  Add STTBX_NO_UART compilation flag.
*******************************************************************************/
#if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef ST_OS20
#include <debug.h>
#endif /* ST_OS20 */
#if defined(ST_OS21) && !defined(ST_OSWINCE)
#include <os21debug.h>
#endif /* ST_OS21 && !ST_OSWINCE*/

#include "stddefs.h"
#include "stcommon.h"
#include "sttbx.h"
#include "tbx_init.h"


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#ifndef STTBX_FLUSH_OUTPUT_BUFFERS_TASK_PRIORITY
#define STTBX_FLUSH_OUTPUT_BUFFERS_TASK_PRIORITY MIN_USER_PRIORITY
#endif

#define TASK_STACK_SIZE                         5*1024

#define TASK_OUTPUTBUFFERS_TIME   (sttbx_ClocksPerSecond / 50) /* 0.02s = waiting time for output task */
#define TASK_DESCHEDULING_TIME    (sttbx_ClocksPerSecond / 10) /* 0.1s */

/* Private Variables (static)------------------------------------------------ */
static BOOL FlushOutputStarted = FALSE;

static BOOL StopFlushingOutputTask;
static task_t * OutputTask_p;                 /* "STTBX_FlushOutputBuffers" task */
static char MsgBuffOverflow[] = "\n\n**** STTBX WARNING **** Ouput buffer overflow: some data have been lost.\n" ;

/* Global Variables --------------------------------------------------------- */

BOOL sttbx_BuffersEnable;          /* put output data in buffers if true */
BOOL sttbx_KeepOldestWhenFull;     /* if sttbx_BuffersEnable, keep old data when buffer full */

/* Handles on opened drivers for I/O devices which require some */
/*#ifndef STTBX_NO_UART
extern STUART_Handle_t sttbx_GlobalUartHandle;
extern STUART_Handle_t sttbx_GlobalUart2Handle;
#endif */
#if 0 /* Not supported yet */
extern STI1284_Handle_t GlobalI1284Handle;
#endif /* Not supported yet */

char *sttbx_OutputBegin_p[NUMBER_OF_OUTPUTS];
char *sttbx_OutputEnd_p[NUMBER_OF_OUTPUTS];
char *sttbx_OutputBuffer_p[NUMBER_OF_OUTPUTS];
U32   sttbx_BufferSize[] = {STTBX_DCU_OUTPUT_BUFFER_SIZE, STTBX_UART_OUTPUT_BUFFER_SIZE, STTBX_UART2_OUTPUT_BUFFER_SIZE};

BOOL sttbx_OutputBufferOverflow[NUMBER_OF_OUTPUTS];
BOOL sttbx_GiveUpFlushingBuffers[NUMBER_OF_OUTPUTS];
BOOL sttbx_FlushingBuffersAllowed[NUMBER_OF_OUTPUTS];
BOOL sttbx_FlushingBuffers;
semaphore_t *sttbx_WakeUpFlushingBuffer_p;

/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
ST_ErrorCode_t sttbx_InitOutputBuffers(sttbx_CallingDevice_t *Device_p);
ST_ErrorCode_t sttbx_DeleteOutputBuffers(sttbx_CallingDevice_t *Device_p);
ST_ErrorCode_t sttbx_RunOutputBuffers(sttbx_CallingDevice_t *Device_p);
ST_ErrorCode_t sttbx_TermOutputBuffers(sttbx_CallingDevice_t *Device_p);

static char * FlushDCUBuffer (char *Min_p, char *Max_p);
static char * FlushUARTBuffer(char *Min_p, char *Max_p);
static char * FlushUART2Buffer(char *Min_p, char *Max_p);

static char * (*FlushBuffer[])(char *Min_p, char *Max_p) = {FlushDCUBuffer, FlushUARTBuffer, FlushUART2Buffer};

#ifndef STTBX_NO_UART
static char * GenericFlushUARTBuffer(U8 UARTNb, char *Min_p, char *Max_p);
#endif /* #ifndef STTBX_NO_UART */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : sttbx_InitOutputBuffers
Description : output buffers creation
Parameters  : device pointer
Assumptions : creation not already done
Limitations :
Returns     : ST_NO_ERROR if successfull
*******************************************************************************/
ST_ErrorCode_t sttbx_InitOutputBuffers(sttbx_CallingDevice_t *Device_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    U8 Output;

    if (Device_p == NULL)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    if (Device_p->CPUPartition_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    sttbx_BuffersEnable = FALSE; /* for compatility with previous aplications */
    StopFlushingOutputTask = FALSE;
    sttbx_KeepOldestWhenFull = TRUE;

    /* Allocate buffers for output data */
    /* Each buffer is managed as a circular buffer */
    /* The length of each message is variable */

    for (Output = 0; Output < NUMBER_OF_OUTPUTS; Output ++)
    {
        sttbx_OutputBufferOverflow[Output] = FALSE;
        sttbx_GiveUpFlushingBuffers[Output] = FALSE;
        sttbx_FlushingBuffersAllowed[Output] = TRUE;
        sttbx_OutputBuffer_p[Output] = memory_allocate(Device_p->CPUPartition_p, sttbx_BufferSize[Output]);
        if (sttbx_OutputBuffer_p[Output] == NULL)
        {
            ErrCode = ST_ERROR_NO_MEMORY;
            break;
        }
        else
        {
            sttbx_OutputBegin_p[Output] = sttbx_OutputBuffer_p[Output];
            sttbx_OutputEnd_p[Output]   = sttbx_OutputBuffer_p[Output];
        }
    }
    return(ErrCode);
}

/*******************************************************************************
Name        : sttbx_DeleteOutputBuffers
Description : output buffers deletion
Parameters  : device pointer
Assumptions : creation already done
Limitations : remaining data in buffers will be lost
Returns     : ST_NO_ERROR if successfull
*******************************************************************************/
ST_ErrorCode_t sttbx_DeleteOutputBuffers(sttbx_CallingDevice_t *Device_p)
{
    U8 Output;

    if (Device_p == NULL)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    for (Output = 0; Output < NUMBER_OF_OUTPUTS; Output ++)
    {
        memory_deallocate(Device_p->CPUPartition_p, sttbx_OutputBuffer_p[Output]);
        sttbx_OutputBuffer_p[Output] = NULL;
        sttbx_OutputBegin_p[Output]  = NULL;
        sttbx_OutputEnd_p[Output]    = NULL;
    }
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : FlushDCUBuffer
Description : output DCU data
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if successfull
*******************************************************************************/
static char * FlushDCUBuffer(char *Min_p, char *Max_p)
{
    S32 StringLength=1;
    long int ErrDebug ;

    while ( (Min_p < Max_p) && (StringLength > 0))
    {
        /* print each string terminated by null */
        StringLength = strlen(Min_p);
#ifdef ST_OS20

        do
        {
            /* if buffer overflow in keep newest mode, then stop outputting, will re-start to new Begin */
            if (sttbx_GiveUpFlushingBuffers[DCU_OUTPUT])
            {
                return(NULL); /* yes, inside while() !  */
            }

            ErrDebug = debugmessage(Min_p);

            if ((ErrDebug != 0) && (ErrDebug != DEBUG_NOT_CONNECTED))
            {
                /* conflict with another debugxxxx() function : try again */
                task_delay(TASK_DESCHEDULING_TIME);
            }
        } while ((ErrDebug != 0) && (ErrDebug != DEBUG_NOT_CONNECTED));
#endif /* ST_OS20 */

#ifdef ST_OS21
        UNUSED_PARAMETER(ErrDebug);
        printf(Min_p);
#endif
        Min_p = Min_p + StringLength + 1 ; /* skip the null */
    }
    return(Max_p);
}

#ifndef STTBX_NO_UART
/*******************************************************************************
Name        : GenericFlushUARTBuffer
Description : output UART data
Parameters  :
Assumptions : UARTNb = 1 or 2 only
Limitations :
Returns     : Max_p
*******************************************************************************/
static char * GenericFlushUARTBuffer(U8 UARTNb, char *Min_p, char *Max_p)
{
    ST_ErrorCode_t ErrCode;
    S32 StringLength;
    U32 Index;
    U8 Output;
    U32 UARTNumberWritten;
    STUART_Handle_t UARTHandle;
    char CarriageReturn[2] ;

    CarriageReturn[0] = '\r' ;
    CarriageReturn[1] = '\0' ;

    if (UARTNb == 1)
    {
        UARTHandle = sttbx_GlobalUartHandle;
        Output = UART_OUTPUT;
    }
    else /* UARTNb = 2 */
    {
        UARTHandle = sttbx_GlobalUart2Handle;
        Output = UART2_OUTPUT;
    }

    /* output strings ended by \n */
    Index = 0;
    StringLength = 0;
    while (Min_p + Index < Max_p)
    {
        StringLength ++;
        if (Min_p[Index] != '\n')
        {
            Index++;
        }
        else  /* newline detected ... */
        {
            /* output current characters */
            do
            {
                /* if buffer overflow in keep newest mode, then stop outputting, will re-start to new Begin */
                if (sttbx_GiveUpFlushingBuffers[Output])
                {
                    return(NULL); /* yes, inside while() !  */
                }
                ErrCode = STUART_Write(UARTHandle, (U8 *)Min_p,
                                    StringLength, &UARTNumberWritten, 0);
                if ((ErrCode != ST_NO_ERROR) && (ErrCode != ST_ERROR_INVALID_HANDLE))
                {
                    /* write not done or uart not ready (buffer full): try again */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
            } while ((ErrCode != ST_NO_ERROR) && (ErrCode != ST_ERROR_INVALID_HANDLE));
            Min_p = Min_p + StringLength ;

            /* output a carriage return */
            StringLength = 1;
            do
            {
                ErrCode = STUART_Write(UARTHandle, (U8 *)CarriageReturn,
                                    StringLength, &UARTNumberWritten, 0);
                if ((ErrCode != ST_NO_ERROR) && (ErrCode != ST_ERROR_INVALID_HANDLE))
                {
                    /* write not done or uart not ready (buffer full): try again */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
            } while ((ErrCode != ST_NO_ERROR) && (ErrCode != ST_ERROR_INVALID_HANDLE));
            StringLength = 0;
            Index = 0;
        }
    } /* end while */

    /* output remaining characters */
    if (Min_p < Max_p)
    {
        StringLength = Max_p - Min_p + 1;
        do
        {
            /* if buffer overflow in keep newest mode, then stop outputting, will re-start to new Begin */
            if (sttbx_GiveUpFlushingBuffers[Output])
            {
                return(NULL); /* yes, inside while() !  */
            }
            ErrCode = STUART_Write(UARTHandle, (U8 *)Min_p,
                                StringLength, &UARTNumberWritten, 0);
            if ((ErrCode != ST_NO_ERROR) && (ErrCode != ST_ERROR_INVALID_HANDLE))
            {
                /* write not done or uart not ready (buffer full): try again */
                task_delay(TASK_DESCHEDULING_TIME);
            }
        } while ((ErrCode != ST_NO_ERROR) && (ErrCode != ST_ERROR_INVALID_HANDLE));
    }
    return(Max_p);
}
#endif /* #ifndef STTBX_NO_UART  */

/*******************************************************************************
Name        : FlushUARTBuffer
Description : output UART data
Parameters  :
Assumptions :
Limitations :
Returns     : Max_p
*******************************************************************************/
static char * FlushUARTBuffer(char *Min_p, char *Max_p)
{
#ifndef STTBX_NO_UART
    return(GenericFlushUARTBuffer(1, Min_p, Max_p));
#else
    UNUSED_PARAMETER(Min_p);
    UNUSED_PARAMETER(Max_p);
    return(NULL); /* only for compilation */
#endif /* #ifndef STTBX_NO_UART */
}

/*******************************************************************************
Name        : FlushUART2Buffer
Description : output UART2 data
Parameters  :
Assumptions :
Limitations :
Returns     : Max_p
*******************************************************************************/
static char * FlushUART2Buffer(char *Min_p, char *Max_p)
{
#ifndef STTBX_NO_UART
    return(GenericFlushUARTBuffer(2, Min_p, Max_p));
#else
    UNUSED_PARAMETER(Min_p);
    UNUSED_PARAMETER(Max_p);
    return(NULL); /* only for compilation */
#endif /* #ifndef STTBX_NO_UART */
}

/*******************************************************************************
Name        : FlushOutputBuffers
Description : output the data when output buffers are filled (task)
Parameters  :
Assumptions : output buffers created, task launched, DCU/UART/... opened
Limitations :
Returns     : none
*******************************************************************************/
static void FlushOutputBuffers(void)
{
    char *CurrentBegin_p, *CurrentEnd_p, *Max_p;
    U8 Output;

    /* Infinite loop, until Toolbox termination */
    while ( !StopFlushingOutputTask )
    {
        sttbx_FlushingBuffers = FALSE;
        semaphore_wait(sttbx_WakeUpFlushingBuffer_p);
        sttbx_FlushingBuffers = TRUE;

        /* -------------------------------------------------
        output all the bufferized data:
        case 1: buffer is ...XXXXXX...  then print XXXXXX
        case 2: buffer is YYYY....XXXY  then print XXXYYYYY
        string termination: not null for UART, null for DCU
        ---------------------------------------------------- */

        for (Output = 0; Output < NUMBER_OF_OUTPUTS; Output ++)
        {
            if (sttbx_OutputBufferOverflow[Output])
            {
                FlushBuffer[Output](MsgBuffOverflow, MsgBuffOverflow + strlen(MsgBuffOverflow));
                sttbx_OutputBufferOverflow[Output] = FALSE;
            }
        }

        for (Output = 0; Output < NUMBER_OF_OUTPUTS; Output ++)
        {
            while (sttbx_OutputBegin_p[Output] != sttbx_OutputEnd_p[Output])
            {
                CurrentBegin_p = sttbx_OutputBegin_p[Output];
                CurrentEnd_p   = sttbx_OutputEnd_p[Output];

                if (CurrentBegin_p < CurrentEnd_p)
                {
                    /* case 1 */
                    CurrentBegin_p = FlushBuffer[Output](CurrentBegin_p, CurrentEnd_p);
                }
                else if (CurrentBegin_p > CurrentEnd_p)
                {
                    /* case 2 */
                    Max_p = sttbx_OutputBuffer_p[Output] + sttbx_BufferSize[Output] - 1;
                    CurrentBegin_p = FlushBuffer[Output](CurrentBegin_p, Max_p);
                    if (CurrentBegin_p != NULL)
                    {
                        CurrentBegin_p = FlushBuffer[Output](sttbx_OutputBuffer_p[Output], CurrentEnd_p);
                    }
                }
                /* check if we are not in the case 'buffer full in keep newest mode' */
                if ((!sttbx_GiveUpFlushingBuffers[Output]) && (CurrentBegin_p != NULL))
                {
                    /* most frequent case  */
                    sttbx_OutputBegin_p[Output] = CurrentBegin_p;
                }
                else
                {
                    /* we are in the case 'buffer full in keep newest mode' */
                    if (sttbx_FlushingBuffersAllowed[Output])
                    {
                        /* if keeping new data, overlap old data */
                        sttbx_OutputBegin_p[Output] = sttbx_OutputBuffer_p[Output];
                    }
                    else
                    {
                        /* sttbx_OutputEnd_p[Output] no yet updated, new string not yet copied ...
                         * so defer flusing buffer by setting begin = end */
                        sttbx_OutputBegin_p[Output] = sttbx_OutputEnd_p[Output];
                    }
                }
                if (sttbx_FlushingBuffersAllowed[Output])
                {
                    sttbx_GiveUpFlushingBuffers[Output] = FALSE;
                }
            } /* end end while (begin != end) */
        } /* end for (Output ... */
    } /* while ( !StopFlushingOutputTask ) */
} /* end of task FlushOutputBuffers() */

/*******************************************************************************
Name        : sttbx_RunOutputBuffers
Description :
Parameters  : device pointer
Assumptions : output buffers already created
Limitations :
Returns     : ST_NO_ERROR if successfull
*******************************************************************************/
ST_ErrorCode_t sttbx_RunOutputBuffers(sttbx_CallingDevice_t *Device_p)
{
    ST_ErrorCode_t ErrCode;

    ErrCode = ST_NO_ERROR;
    UNUSED_PARAMETER(Device_p);
    if (!FlushOutputStarted)
    {
        OutputTask_p = task_create((void(*)(void*))FlushOutputBuffers,
                            NULL,
                            TASK_STACK_SIZE,
                            STTBX_FLUSH_OUTPUT_BUFFERS_TASK_PRIORITY,
                            "STTBX_FlushOutputBuffers",
                            task_flags_no_min_stack_size /* low level flags */);

        if (OutputTask_p == NULL)
        {
#ifdef STTBX_PRINT
            debugmessage("STTBX failed to start the task 'STTBX_FlushOutputBuffers' !\n");
#endif
            ErrCode = ST_ERROR_BAD_PARAMETER; /* failure: bad priority or stack too small */
        }
        else
        {
#ifdef STTBX_PRINT
            debugmessage("STTBX had launched the task 'STTBX_FlushOutputBuffers'\n");
#endif
            FlushOutputStarted = TRUE;
        }
    }
    return(ErrCode);
}

/*******************************************************************************
Name        : STTBX_SetBuffersConfig
Description :
Parameters  : pointer to STTBX_BuffersConfigParams_t
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if successfull
*******************************************************************************/
ST_ErrorCode_t STTBX_SetBuffersConfig(const STTBX_BuffersConfigParams_t * const BuffersConfigParam_p)
{
    if ( BuffersConfigParam_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    sttbx_BuffersEnable = BuffersConfigParam_p->Enable ;
    sttbx_KeepOldestWhenFull = BuffersConfigParam_p->KeepOldestWhenFull ;

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : sttbx_TermOutputBuffers
Description :
Parameters  : device pointer
Assumptions : task sttbx_RunOutputBuffers already started
Limitations :
Returns     : ST_NO_ERROR if successfull
*******************************************************************************/
ST_ErrorCode_t sttbx_TermOutputBuffers(sttbx_CallingDevice_t *Device_p)
{
    task_t *TaskList_p;
    ST_ErrorCode_t ErrCode;
    int TaskStatus;

    ErrCode = ST_NO_ERROR;
    UNUSED_PARAMETER(Device_p);
    /* Tell the task to stop now */
    StopFlushingOutputTask = TRUE ;
    semaphore_signal(sttbx_WakeUpFlushingBuffer_p);

    /* Wait the end of the task */
    TaskList_p = OutputTask_p;

    TaskStatus = task_wait(&TaskList_p, 1, TIMEOUT_INFINITY);
    if (TaskStatus == -1)
    {
#ifdef STTBX_PRINT
        debugmessage("STTBX failed to wait the end of the task 'STTBX_FlushOutputBuffers' !\n");
#endif
        ErrCode = ST_ERROR_DEVICE_BUSY;
    }

    /* Delete the task */
    TaskStatus = task_delete(OutputTask_p);
    if (TaskStatus == -1)
    {
#ifdef STTBX_PRINT
        debugmessage("STTBX failed to delete the task 'STTBX_FlushOutputBuffers' !\n");
#endif
        ErrCode = ST_ERROR_DEVICE_BUSY;
    }
    else
    {
        FlushOutputStarted = FALSE;
    }
    task_delay(TASK_DESCHEDULING_TIME);

    return(ErrCode);
}
#endif  /* #ifndef ST_OSLINUX */

/* End of tbx_buff.c */




