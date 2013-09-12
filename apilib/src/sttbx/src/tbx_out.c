/*******************************************************************************

File name   : tbx_out.c

Description : Source of Output feature of toolbox API

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
10 Mar 1999         Created                                          HG
11 Aug 1999         Change Semaphores structure (now one
                    semaphore for each I/O device)                   HG
                    Implement UART output                            HG
 8 Sep 1999         Exit debug function if debugger is
                    not connected (DEBUG_NOT_CONNECTED)              HG
19 Oct 1999         Change Semaphores structure (now two sem
                    for each I/O device: one In and one Out)         HG
23 Oct 1999         Added \r after each \n when outputting to
                    UART because of terminal alignments pbs          HG
11 Jan 2000         Use task_delay() of about 0.1s instead of 64us   HG
06 Apr 2000         Put output in a buffer; simplify stbx_Output()   FQ
24 Jul 2000         3 modes : no buffer, old. buffer, new buffer     FQ
16 Nov 2001         Fix DDTS GNBvd09866 and GNBvd09650               HS
28 Jan 2002         DDTS GNBvd10702 "Use ST_GetClocksPerSecond       HS
 *                  instead of Hardcoded value"
 *                  DDTS GNBvd04464 "Message buffering generates
 *                  erratic outputs"
 *                  Add STTBX_NO_UART compilation flag.
*******************************************************************************/
 #if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Includes ----------------------------------------------------------------- */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef ST_OS20
#include <debug.h>
#endif /* ST_OS20*/
#if defined(ST_OS21) && !defined(ST_OSWINCE)
#include <os21debug.h>
#endif /* ST_OS21 && !ST_OSWINCE*/

#include "stddefs.h"

#include "sttbx.h"

#include "tbx_out.h"
#include "tbx_init.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/*#define MAX_OUTPUT_TRYS 1000*/
/* #define STTBX_USE_UART2_ADD_CR */

#define TASK_DESCHEDULING_TIME    (sttbx_ClocksPerSecond / 10) /* 0.1s */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

extern BOOL sttbx_BuffersEnable ;
extern BOOL sttbx_KeepOldestWhenFull ;
extern BOOL sttbx_FlushingBuffers;
extern char *sttbx_OutputBegin_p[];
extern char *sttbx_OutputEnd_p[];
extern char *sttbx_OutputBuffer_p[];
extern BOOL sttbx_OutputBufferOverflow[];
extern BOOL sttbx_GiveUpFlushingBuffers[];
extern BOOL sttbx_FlushingBuffersAllowed[];
extern U32  sttbx_BufferSize[];
semaphore_t *sttbx_LockDCUIn_p;    /* DCU input is independant from output */
semaphore_t *sttbx_LockDCUOut_p;   /* DCU output is independant from input */
semaphore_t *sttbx_WakeUpFlushingBuffer_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

void sttbx_OutputDCUNoSemaphore(const char *Message_p);
void sttbx_OutputUartNoSemaphore(const char *Message_p);
void sttbx_OutputUart2NoSemaphore(const char *Message_p);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : sttbx_OutputDCUNoSemaphore
Description : Output message on DCU
Parameters  : Message (char *)
Assumptions : Message_p is not NULL
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_OutputDCUNoSemaphore(const char *Message_p)
{
    long int ErrorCode;
/*    int TryCount = 0;*/

    /* The while is to ensure the good execution of debugmessage in case
        other debug functions are used somewhere else at the same time.
        Try debugmessage until success...
        (debuggetmessage returns 0 on success, other values are errors.) */
    /* However, stop if the debugger is not connected. */
    /* However, stop after MAX_OUTPUT_TRYS times for security. */
#ifdef ST_OS20
    do
    {
#endif /* ST_OS20 */
        ErrorCode = debugmessage(Message_p);
#ifdef ST_OS20
        if ((ErrorCode != 0) && (ErrorCode != DEBUG_NOT_CONNECTED))
        {
/*            TryCount++;*/
            task_delay(TASK_DESCHEDULING_TIME);
        }
    } while ((ErrorCode != 0) && (ErrorCode != DEBUG_NOT_CONNECTED));
#endif /* ST_OS20 */
/*    } while ((ErrorCode != 0) && (TryCount < MAX_OUTPUT_TRYS));*/
}

#ifndef STTBX_NO_UART
/*******************************************************************************
Name        : sttbx_OutputUartNoSemaphore
Description : Output message on UART
Parameters  : Message (char *)
Assumptions : Message_p is not NULL
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_OutputUartNoSemaphore(const char *Message_p)
{
    U32 UARTNumberWritten;
    ST_ErrorCode_t Err;

    do
    {
        /* No time-out to output character */
        Err = STUART_Write(sttbx_GlobalUartHandle, (U8 *) Message_p, strlen(Message_p), &UARTNumberWritten, 0);
        if ((Err != ST_NO_ERROR) && (Err != ST_ERROR_INVALID_HANDLE))
        {
            task_delay(TASK_DESCHEDULING_TIME);
        }
        /* Exit if ST_NO_ERROR: success
            OR ST_ERROR_INVALID_HANDLE: UART not initialised: just pass out... */
    } while ((Err != ST_NO_ERROR) && (Err != ST_ERROR_INVALID_HANDLE));
}

/*******************************************************************************
Name        : sttbx_OutputUart2NoSemaphore
Description : Output message on UART2
Parameters  : Message (char *)
Assumptions : Message_p is not NULL
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_OutputUart2NoSemaphore(const char *Message_p)
{
    U32 UARTNumberWritten;
    ST_ErrorCode_t Err;

    do
    {
        /* No time-out to output character */
        Err = STUART_Write(sttbx_GlobalUart2Handle, (U8 *) Message_p, strlen(Message_p), &UARTNumberWritten, 0);
        if ((Err != ST_NO_ERROR) && (Err != ST_ERROR_INVALID_HANDLE))
        {
            task_delay(TASK_DESCHEDULING_TIME);
        }
        /* Exit if ST_NO_ERROR: success
            OR ST_ERROR_INVALID_HANDLE: UART not initialised: just pass out... */
    } while ((Err != ST_NO_ERROR) && (Err != ST_ERROR_INVALID_HANDLE));
}

/*******************************************************************************
Name        : sttbx_OutputUartNoSemaphoreAddCarriageReturn
Description : Output message on UART adding \r after each \n
Parameters  : Message (char *)
Assumptions : Message_p is not NULL
Limitations :
Returns     : nothing
*******************************************************************************/
static void sttbx_OutputUartNoSemaphoreAddCarriageReturn(const char *Message_p)
{
    char *NextLineEnd_p, *PartialStr_p;

    NextLineEnd_p = (char *) Message_p - 1;
    do
    {
        PartialStr_p = NextLineEnd_p + 1;
        NextLineEnd_p = strchr(PartialStr_p, '\n');
        if (NextLineEnd_p != NULL)
        {
            *NextLineEnd_p = '\0';
        }
        if (*PartialStr_p != '\0')
        {
            sttbx_OutputUartNoSemaphore(PartialStr_p);
        }
        if (NextLineEnd_p != NULL)
        {
            sttbx_OutputUartNoSemaphore("\r\n");
            *NextLineEnd_p = '\n';
        }
    } while ((NextLineEnd_p != NULL) && (*(NextLineEnd_p + 1) != '\0'));
}


#ifdef STTBX_USE_UART2_ADD_CR
/*******************************************************************************
Name        : sttbx_OutputUart2NoSemaphoreAddCarriageReturn
Description : Output message on UART adding \r after each \n
Parameters  : Message (char *)
Assumptions : Message_p is not NULL
Limitations :
Returns     : nothing
*******************************************************************************/
static void sttbx_OutputUart2NoSemaphoreAddCarriageReturn(const char *Message_p)
{
    char *NextLineEnd_p, *PartialStr_p;

    NextLineEnd_p = (char *) Message_p - 1;
    do
    {
        PartialStr_p = NextLineEnd_p + 1;
        NextLineEnd_p = strchr(PartialStr_p, '\n');
        if (NextLineEnd_p != NULL)
        {
            *NextLineEnd_p = '\0';
        }
        if (*PartialStr_p != '\0')
        {
            sttbx_OutputUart2NoSemaphore(PartialStr_p);
        }
        if (NextLineEnd_p != NULL)
        {
            sttbx_OutputUart2NoSemaphore("\r\n");
            *NextLineEnd_p = '\n';
        }
    } while ((NextLineEnd_p != NULL) && (*(NextLineEnd_p + 1) != '\0'));
}
#endif /* #ifdef STTBX_USE_UART2_ADD_CR */
#endif /* #ifndef STTBX_NO_UART  */


/*******************************************************************************
Name        : sttbx_StringOutput
Description : Output message on device
Parameters  : device + <printf> like format
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_StringOutput(STTBX_Device_t Device, char *String_p)
{
    U32 StringSize;     /* size of new string to display */
    U32 RemainingSize;  /* remaining free size from end of last string to end of buffer, if begin before end  */
    U32 FreeSize;       /* sum of all free sizes, ie buffer size minus sum of old strings sizes */
    U8 Output;
    char *Dest_p, *CurrentBegin_p, *CurrentEnd_p;

    if (sttbx_BuffersEnable)
    {
        /* ---------------------------------------------------
        add YYYY in the circular output buffer
        case 1 : change XXXXXX...... to XXXXXXYYYY..
                        ^     ^
                 begin__|     |__end
        case 2 : change ....XXXXXX.. to YY..XXXXXXYY
        case 3 : change XXX......XXX to XXXYYYY..XXX
                           ^     ^
                      end__|     |__begin
        case 4 : change XXXXXXX..XXX to XXXXXXXXYYXX
        string termination : no null for UART, null for DCU
        output of bufferized strings : done by a parallel task
        ------------------------------------------------------ */

        StringSize = strlen(String_p);
        if (StringSize == 0)
        {
            return;
        }
        switch(Device)
        {
            case STTBX_DEVICE_DCU:
                Output      = DCU_OUTPUT;
                StringSize++; /* add a null */
                break;
#ifndef STTBX_NO_UART
            case STTBX_DEVICE_UART:
                Output      = UART_OUTPUT;
                break;
            case STTBX_DEVICE_UART2:
                Output      = UART2_OUTPUT;
                break;
#endif /* #ifndef STTBX_NO_UART  */
            default:
                return;
        }

        /* sttbx_OutputBegin_p[Output] can be modified by flushing buffer task, so take here a local copy
         * to work with the same value during following lines*/
        CurrentBegin_p = sttbx_OutputBegin_p[Output];
        /* sttbx_OutputEnd_p[Output] is used by flushing buffer task, so take here a local copy
         * to compute its new value step by step, then affect sttbx_OutputEnd_p[Output] with one instruction */
        CurrentEnd_p   = sttbx_OutputEnd_p[Output];

        Dest_p = CurrentEnd_p;
        RemainingSize = sttbx_BufferSize[Output] - (U32)(CurrentEnd_p - sttbx_OutputBuffer_p[Output]);
        if (CurrentEnd_p >= CurrentBegin_p)  /* cases 1 & 2 */
        {
            FreeSize = sttbx_BufferSize[Output] - (U32)(CurrentEnd_p - CurrentBegin_p) ;
        }
        else  /* cases 3 & 4 */
        {
            FreeSize = (U32)(CurrentBegin_p - CurrentEnd_p);
        }
        /* if not enough place (even for another null if DCU)... */
        if ( (sttbx_KeepOldestWhenFull) && (StringSize > FreeSize))
        {
            StringSize = FreeSize; /* keep old data, truncate new data */
        }
        if (StringSize <= RemainingSize)
        {
            CurrentEnd_p += StringSize;
        }
        else /* case 2 : split message into 2 strings */
        {
            CurrentEnd_p = sttbx_OutputBuffer_p[Output] + (StringSize - RemainingSize);
            if (Output == DCU_OUTPUT)
            {
                CurrentEnd_p += 1; /* add another null */
            }
        }

        if (StringSize >= FreeSize) /* if not enough place... */
        {
            sttbx_OutputBufferOverflow[Output] = TRUE;
            if (!sttbx_KeepOldestWhenFull) /* keep newest */
            {
                /* start on a new clean buffer */
                sttbx_FlushingBuffersAllowed[Output] = FALSE;
                sttbx_GiveUpFlushingBuffers[Output] = TRUE; /* stop flushing current data! */
                Dest_p = sttbx_OutputBuffer_p[Output];
                CurrentEnd_p = Dest_p + StringSize;
                RemainingSize = sttbx_BufferSize[Output] - StringSize;
            }
            else if ((sttbx_KeepOldestWhenFull) && (CurrentBegin_p == CurrentEnd_p))
            {
                CurrentEnd_p = CurrentBegin_p - 1;
                StringSize --; /* remove one char */
            }
        }
        if (StringSize <= RemainingSize)
        {
            strncpy(Dest_p, String_p, StringSize);
            if (Output == DCU_OUTPUT)
            {
                Dest_p[StringSize-1] = '\0'; /* replace last char by a null */
            }
        }
        else /* case 2 : split message into 2 strings */
        {
            if (Output == DCU_OUTPUT)
            {
                if (RemainingSize > 0)
                {
                    RemainingSize --; /* artefact to introduce null char */
                }
                sttbx_OutputBuffer_p[Output][sttbx_BufferSize[Output] - 1] = '\0';  /* replace last char by a null */
                sttbx_OutputBuffer_p[Output][StringSize - RemainingSize] = '\0';    /* replace last char by a null */
            }
            strncpy(Dest_p, String_p, RemainingSize);
            strncpy(sttbx_OutputBuffer_p[Output], String_p + RemainingSize, StringSize - RemainingSize );
        }
        sttbx_OutputEnd_p[Output] = CurrentEnd_p;
        sttbx_FlushingBuffersAllowed[Output] = TRUE; /* in case was FALSE, save a test... */
        if (!sttbx_FlushingBuffers)
        {
            semaphore_signal(sttbx_WakeUpFlushingBuffer_p);
        }
    }
    else /* if (sttbx_BuffersEnable) */
    {
        /* output the data now */

        /* Display the string */
        switch(Device)
        {
            case STTBX_DEVICE_DCU :
                SEM_WAIT(sttbx_LockDCUOut_p);
                sttbx_OutputDCUNoSemaphore(String_p);
                SEM_SIGNAL(sttbx_LockDCUOut_p);
                break;
#ifndef STTBX_NO_UART
            case STTBX_DEVICE_UART :
                SEM_WAIT(sttbx_LockUartOut_p);
/*              sttbx_OutputUartNoSemaphore(String_p);*/
                sttbx_OutputUartNoSemaphoreAddCarriageReturn(String_p);
                SEM_SIGNAL(sttbx_LockUartOut_p);
                break;
            case STTBX_DEVICE_UART2 :
                SEM_WAIT(sttbx_LockUart2Out_p);
                sttbx_OutputUart2NoSemaphore(String_p);
                SEM_SIGNAL(sttbx_LockUart2Out_p);
                break;
#endif /* #ifndef STTBX_NO_UART  */
#if 0 /* Not supported yet */
            case STTBX_DEVICE_I1284 :
                SEM_WAIT(sttbx_LockI1284_p);
                OutputI1284NoSemaphore(String_p);
                SEM_SIGNAL(sttbx_LockI1284_p);
                break;
#endif /* Not supported yet */
            case STTBX_DEVICE_NULL : /* Exit if no output device is selected */
                default :
                break; /* Unknown Output Device */
        }
    } /* end if enable...*/

    return;

}

/*******************************************************************************
Name        : sttbx_Output
Description : Output message on device
Parameters  : device + <printf> like format
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_Output(STTBX_Device_t Device, char *Format_p, ...)
{
    va_list Argument;
    U8 PrintTableIndex;

    /* Exit if no output device is selected */
    if (Device == STTBX_DEVICE_NULL)
    {
        return;
    }

    /* take new index */
    sttbx_NewPrintSlot(&PrintTableIndex);

    sttbx_PrintTable_p[PrintTableIndex]->String[0] = '\0';

    /* Translate arguments into a character buffer */
    va_start(Argument, Format_p);
    vsprintf(sttbx_PrintTable_p[PrintTableIndex]->String, Format_p, Argument);
    va_end(Argument);

    /* Display the string */
    sttbx_StringOutput(Device,sttbx_PrintTable_p[PrintTableIndex]->String);

    /* Free sttbx_PrintTable_p index used before leaving : */
    sttbx_FreePrintSlot(PrintTableIndex);
} /* end of sttbx_Output() */


#endif  /* #ifndef ST_OSLINUX */
/* End of tbx_out.c */


