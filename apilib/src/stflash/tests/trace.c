/*****************************************************************************

File name   :  trace.c

Description :  allows to trace some debug informations on second
               uart of eval board. Calls can be made from interrupt
               and multitasking. Principle based on polling task
               synchronized on buffer to print != 0

COPYRIGHT (C) ST-Microelectronics 2003.

*****************************************************************************/

/* --- includes ----------------------------------------------------------- */

#include <string.h>
#include <stdarg.h>
#ifndef ST_OS21
#include <semaphor.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "stddefs.h"
#include "sttbx.h"

#include "trace.h"
#include "uart.h"

/* Private Types ------------------------------------------------------ */

/* Private Constants -------------------------------------------------- */

/* Private Variables -------------------------------------------------- */

static char *          trace_base;

#ifdef ST_OS21
static semaphore_t     *trace_lock_p;
#else
static semaphore_t     trace_lock;
static semaphore_t     *trace_lock_p;
#endif

static U32             trace_start, trace_end, trace_read, trace_write, trace_limit;
static BOOL            trace_printing;

static BOOL            trace_on = FALSE;
static U32             time_latch;

/* Private Macros ----------------------------------------------------- */

#if defined(ST_OS21)
#define TRACE_WORKSPACE 20*1024
#else
#define TRACE_WORKSPACE 256
#endif
#define TRACE_BUFFER    20*1024

#if defined(PROCESSOR_C1) || defined(ST_OS21)
#define TimerHigh(Value) /* NOP */
#else
#define TimerHigh(Value) __optasm{ldc 0; ldclock; st Value;}
#endif


#ifdef DEBUG_TRACE
    #define TRACE_Print(a) STFLASH_Print a
#else
    #define TRACE_Print(a)
#endif

/* Private Function prototypes ---------------------------------------- */

static void                 tracetask ( void );

/* Functions ---------------------------------------------------------- */

/* =======================================================================
   Name:        TraceInit
   Description: Creation of polling task, and allocation of print buffer

   ======================================================================== */
BOOL  TraceInit ( void )
{
    task_t * trace_task_p;
    ST_ErrorCode_t ErrCode;
    STUART_OpenParams_t UartOpenParams;

    trace_base = malloc(TRACE_BUFFER);
    if ( trace_base == NULL )
    {
        STFLASH_Print (("Unable to allocate buffer for debug trace\n"));
        return TRUE;
    }
    trace_start = ( U32 ) trace_base;
    trace_end   = ( U32 ) trace_start + TRACE_BUFFER - 80;
    trace_limit = ( U32 ) trace_end + 80;
    trace_read = trace_write = trace_start;

    UartOpenParams.LoopBackEnabled = FALSE;
    UartOpenParams.FlushIOBuffers = TRUE;
    UartOpenParams.DefaultParams = NULL;
    ErrCode = STUART_Open("asc",
                          &UartOpenParams,
                          &UART_Handle);
    if (ErrCode != ST_NO_ERROR)
    {
        STFLASH_Print(("Failed to open uart device"));
        free(trace_base);
        return TRUE;
    }

#ifdef ST_OS21
    trace_lock_p = semaphore_create_fifo(0);
#else            
    trace_lock_p = &trace_lock;
    semaphore_init_fifo_timeout(trace_lock_p, 0);
#endif
    
    trace_printing = FALSE;
    
    STFLASH_Print(("\nCreating trace task\n"));    
    trace_task_p = task_create ((void (*)(void *)) tracetask,
                             (void *) NULL ,
                             (TRACE_WORKSPACE),
                             0,
                             "trace_buffer",
                             0);
    if ( trace_task_p == NULL )
    {
        STFLASH_Print (("Unable to create trace buffer task...\n"));
        free(trace_base);
        ErrCode=STUART_Close(UART_Handle);
        return TRUE;
    }

    trace_on = TRUE;
    trace("\n*********** trace buffer activated ****************\n");

    return (FALSE);
}

/* =======================================================================
   Name:        tracetask
   Description: task that stays in a loop, and waits until data to print

   ======================================================================== */
void  tracetask ( void )
{
    U32 to_print;
    S32 Written;
    ST_ErrorCode_t ErrCode=ST_NO_ERROR;
    
    while ( 1 )
    {
        trace_printing = FALSE;
        
        semaphore_wait ( trace_lock_p );
        trace_printing = TRUE;
        /* display buffer from trace_read to the limit */

        while ( trace_read != trace_write )
        {
            if ( trace_read > trace_write )
            {
                /* the buffer has wrapped around, so we display the end first */
                to_print = trace_limit - trace_read;

                ErrCode = STUART_Write(UART_Handle,
                             (U8 *) trace_read,
                             to_print,
                             (U32 *)(&Written),
                             5000 );

                trace_read = trace_start;
            }
            /* then the general case */
            to_print = trace_write - trace_read;

            ErrCode = STUART_Write(UART_Handle,
                         (U8 *) trace_read,
                         to_print,
                         (U32 *)(&Written),
                         5000 );
            if(ErrCode != ST_NO_ERROR)
            {
                /*Do Nothing,but some failure information could be printed*/
            }

            trace_read += to_print;
        }
    }
}

/* =======================================================================
   Name:        trace
   Description: local function that stores data to print into
                the print buffer and unlocks the task to print if not
                running
   ======================================================================== */
void  trace (const char *format,...)
{
    va_list         list;

    va_start (list, format);
    if ( trace_on == TRUE )
    {
        trace_write += vsprintf ((char*) trace_write , format, list);
        if ( trace_write > ( trace_end - 80 ) )
        { /* if end of the buffer almost reached, wrap around */
            trace_limit = trace_write;
            trace_write = trace_start;
        }
        if ( trace_printing == FALSE )
        {
            semaphore_signal ( trace_lock_p );
        }
    }
    va_end (list);
}

/* ========================================================================
   Name:        tracestarttime
   Description: utility function to latch time start ( high priority timer )

   ======================================================================== */
void           tracestarttime ( void )
{
    TimerHigh ( time_latch );
}

/* ========================================================================
   Name:        tracelatchtime
   Description: utility function to latch time.

   ======================================================================== */
void  tracelatchtime ( void )
{
    U32     now;
    BOOL    History;

    TimerHigh ( now );
    History = trace_on;
    if ( trace_on == FALSE )
    {
        trace_on = TRUE;
    }
    trace ("<-%d->", now - time_latch );
    trace_on = History;
}

/* ------------------------------- End of file ---------------------------- */
