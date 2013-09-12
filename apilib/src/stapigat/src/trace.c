
/*****************************************************************************

File name   :  trace.c

Description :  allows to trace some debug informations on second
               uart of eval board. Calls can be made from interrupt
               and multitasking. Principle based on polling task
               synchronized on buffer to print != 0

COPYRIGHT (C) ST-Microelectronics 1999-2002.

Date               Modification                 Name
----               ------------                 ----
 10/22/99            Created                    FR and SB
 16 Sep 2004         Add support for ST40/OS21  MH

*****************************************************************************/
/* --- includes ----------------------------------------------------------- */
#include <string.h>
#include <stdarg.h>
#ifdef ST_OS21
#include <os21/semaphore.h>
#endif /* ST_OS21 */
#ifdef ST_OS20
#include <semaphor.h>
#endif /* ST_OS20 */
#include <stdlib.h>
#include <stdio.h>
#include "testcfg.h"

#ifdef TRACE_UART

#include "stddefs.h"

#include "stuart.h"

#ifdef ST_OSLINUX
#include "compat.h"
/*#else*/
#endif

#include "sttbx.h"
#include "testtool.h"

#include "trace.h"
#include "cluart.h"


/* --- local defines ------------------------------------------------------ */

#define TRACE_THRESHOLD 80

#ifdef ST_OS20
#define TimerHigh(Value) __asm{ldc 0; ldclock; st Value;}
    #define TRACE_BUFFER    2048
#define TRACE_WORKSPACE 256
#endif  /* ST_OS20 */


#ifdef ST_OS21
#define TimerHigh(Value)    (Value) = time_now()
    #define TRACE_BUFFER    2048
#define TRACE_WORKSPACE (16*1024)
#endif /* ST_OS21 */

#ifdef ST_OSLINUX
    #define TimerHigh(Value) Value=0
    #define TRACE_BUFFER    2047
    #define TRACE_WORKSPACE 256
#endif

/* The timer value according to the architecture is either on 32 or on 64 bits */
#if defined(ARCHITECTURE_ST20)
#define TIMER U32
#elif defined(ARCHITECTURE_ST40)
#define TIMER long long
#elif defined(ARCHITECTURE_ST200)
#define TIMER U32 /* TO BE CHECKED */
#endif /* ARCHITECTURE_ST20 */

/* --- global variables --------------------------------------------------- */

/* --- local variables ---------------------------------------------------- */
static char *          trace_base;
static semaphore_t     *trace_lock_p;
static U32             trace_start, trace_end, trace_read, trace_write, trace_limit;
static BOOL            trace_printing;

/* static char *          trace_base2; */
/* static semaphore_t     trace_lock2_p; */
/* static U32             trace_start2, trace_end2, trace_read2, trace_write2, trace_limit2; */
/* static BOOL            trace_printing2; */

static BOOL            trace_on = FALSE;
#if !defined(PROCESSOR_C1)
static U32             time_latch;
#endif

/* --- local enumerations ------------------------------------------------- */

/* --- prototypes of local functions and static --------------------------- */
static BOOL                 TRACEWrite ( parse_t *pars_p, char *result );
static BOOL                 TRACEOn    ( parse_t *pars_p, char *result );
static BOOL                 TRACEOff   ( parse_t *pars_p, char *result );
static void                 tracetask ( void );
/* static void                 tracetask2( void ); */

/* --- functions in this driver ------------------------------------------- */
/* =======================================================================
   Name:        TraceInit
   Description: Creation of polling task, and allocation of print buffer

   ======================================================================== */
BOOL            TraceInit ( void )
{
#ifdef ST_OSLINUX
    pthread_t			trace_task;
	pthread_attr_t 		trace_taskAttribute;
#else
    task_t * trace_task;
#endif

    trace_lock_p = semaphore_create_fifo (0);
    trace_printing = FALSE;

#ifdef ST_OSLINUX
	pthread_attr_init(&trace_taskAttribute);
	if (pthread_create(&trace_task,
	                   &trace_taskAttribute,
					   (void *) tracetask,
                       (void *) NULL))
	{
        STTBX_Print (("Unable to create trace buffer task...\n"));
        return ( TRUE );
	}
#else
    trace_task = task_create ((void (*)(void *)) tracetask,
                             (void *) NULL ,
                             (TRACE_WORKSPACE),
                             0,
                             "trace_buffer",
                             task_flags_no_min_stack_size );
    if ( trace_task == NULL )
    {
        STTBX_Print (("Unable to create trace buffer task...\n"));
        return ( TRUE );
    }
#endif

    trace_base = malloc ( TRACE_BUFFER + 1 );
    if ( trace_base == NULL )
    {
        STTBX_Print (("Unable to allocate buffer for debug trace\n"));
    }
    trace_start = ( U32 ) trace_base;
    trace_end   = ( U32 ) trace_start + TRACE_BUFFER - TRACE_THRESHOLD;
    trace_limit = ( U32 ) trace_end + TRACE_THRESHOLD;
    trace_read = trace_write = trace_start;
#ifndef INTRUSIVE_TRACES
    STTBX_Print(("trace buffer activated\n"));
#else
    STTBX_Print(("intrusive trace buffer activated\n"));
#endif /* INTRUSIVE_TRACES */    
    
    trace_on = TRUE;


    /*
    trace_lock2_p = semaphore_create_fifo (0 );
    trace_printing2 = FALSE;
    trace_task = task_create ((void (*)(void *)) tracetask2,
                             (void *) NULL ,
                             (TRACE_WORKSPACE),
                             0,
                             "trace_buffer2",
                             0);
    if ( trace_task == NULL )
    {
    STTBX_Print (("Unable to create trace buffer task.2..\n"));
        return ( TRUE );
    }
    trace_base2 = malloc ( TRACE_BUFFER );
    if ( trace_base2 == NULL )
    {
        STTBX_Print (("Unable to allocate buffer for debug trace2\n"));
    }
    trace_start2 = ( U32 ) trace_base2;
    trace_end2   = ( U32 ) trace_start2 + TRACE_BUFFER - TRACE_THRESHOLD;
    trace_limit2 = ( U32 ) trace_end2 + TRACE_THRESHOLD;
    trace_read2  = trace_write2 = trace_start2;
    STTBX_Print(("trace buffer 2 activated\n"));
    */
    return (FALSE);

}

/* =======================================================================
   Name:        TraceDebug
   Description: Registration of debug commands for trace buffer

   ======================================================================== */
BOOL            TraceDebug (void)
{
    register_command ( "TRACEWRITE", TRACEWrite, "<string> write string to the trace buffer");
    register_command ( "TRACEON"   , TRACEOn   , "Restarts trace process");
    register_command ( "TRACEOFF"  , TRACEOff   ,"stop trace process");

    return ( FALSE );
}
/* =======================================================================
   Name:        TRACEWrite
   Description: debug function to transfer a buffer to the trace command

   ======================================================================== */
static BOOL         TRACEWrite ( parse_t *pars_p, char *result )
{
    char            local_string[TRACE_THRESHOLD];
    cget_string (pars_p, "", (char *) local_string , TRACE_THRESHOLD - 1 );
    trace ( "%s" , local_string );
    return ( FALSE );
}

/* ========================================================================
   Name:        TRACEOn
   Description: resumes the trace process

   ======================================================================== */
static BOOL                 TRACEOn    ( parse_t *pars_p, char *result )
{
    /* if ( io_trace_uart == NULL ) */
    /* { */
        /* STTBX_Print(("no trace process activated!\n")); */
        /* return ( TRUE ); */
    /* } */
    trace_on = TRUE;
    return ( FALSE );
}

/* ========================================================================
   Name:        TRACEOff
   Description: testtool command to stop trace printing

   ======================================================================== */
static BOOL                 TRACEOff   ( parse_t *pars_p, char *result )
{
    trace_on = FALSE;
    return ( FALSE );
}

/* =======================================================================
   Name:        tracetask
   Description: task that stays in a loop, and waits until data to print

   ======================================================================== */
void            tracetask ( void )
{
    U32 to_print;
    S32 Written;

    while ( 1 )
    {
        trace_printing = FALSE;
        semaphore_wait (trace_lock_p);
        trace_printing = TRUE;
        /* display buffer from trace_read to the limit */

        while ( trace_read != trace_write )
        {
            if ( trace_read > trace_write )
            {
                /* the buffer has wrapped around, so we display the end first */
                to_print = trace_limit - trace_read;

#ifdef ST_OSLINUX
				printf("%s", trace_read);
#else
                /* uart_write  (io_trace_uart, (U8 *) trace_read , (S32) to_print , &Written ); */
                STUART_Write(TraceUartHandle,
                            (U8 *) trace_read,
                            to_print,
                            (U32 *)(&Written),
                            5000
                            );
#endif  /* ST_OSLINUX */

                trace_read = trace_start;
            }
            /* then the general case */
            to_print = trace_write - trace_read;

#ifdef ST_OSLINUX
			printf("%s", trace_read);
#else
            /* uart_write (io_trace_uart, (U8 *) trace_read , (S32) to_print , &Written ); */
            STUART_Write(TraceUartHandle,
                            (U8 *) trace_read,
                            to_print,
                            (U32 *)(&Written),
                            5000
                            );
#endif  /* ST_OSLINUX */

            trace_read += to_print;
        }
    }
    /* never exits ( in theory */
}

/* =======================================================================
   Name:        tracetask2
   Description: task that stays in a loop, and waits until data to print

   ======================================================================== */
/*
void            tracetask2 ( void )
{
    U32 to_print;
    S32 Written;
    while ( 1 )
    {
        trace_printing2 = FALSE;
        semaphore_wait ( trace_lock2_p );
        trace_printing2 = TRUE;

        while ( trace_read2 != trace_write2 )
        {
            if ( trace_read2 > trace_write2 )
            {

                to_print = trace_limit2 - trace_read2;
                uart_write (io_trace_uart2, (U8 *) trace_read2 , (S32) to_print , &Written );
                trace_read2 = trace_start2;
            }

            to_print = trace_write2 - trace_read2;
            uart_write (io_trace_uart2, (U8 *) trace_read2 , (S32) to_print , &Written );
            trace_read2 += to_print;
        }
    }

}
*/
/* =======================================================================
   Name:        trace
   Description: local function that stores data to print into
                the print buffer and unlocks the task to print if not
                running
   ======================================================================== */
void            trace (const char *format,...)
{
    va_list         list;

 #ifdef INTRUSIVE_TRACES
    /* Lock the current context to avoid having interlaed traces 
        from different contexts, that couldn't be understood */
    if(task_context(NULL, NULL) == task_context_task)
    {
        task_lock();
        interrupt_lock();
    }
    else
    {
        interrupt_lock();
    }
#endif /* INTRUSIVE_TRACES */  
    va_start (list, format);
    if (trace_on)
    {
        trace_write += vsprintf ((char*) trace_write , format, list);

#ifdef ST_OSLINUX
		*((char*) trace_write) = '\0';
#endif

        if ( trace_write > ( trace_end - TRACE_THRESHOLD ) )
        { /* if end of the buffer almost reached, wrap around */
            trace_limit = trace_write;
            trace_write = trace_start;
        }
        if (!trace_printing)
        {
            semaphore_signal (trace_lock_p);
        }
    }
    va_end (list);
#ifdef INTRUSIVE_TRACES
    /* Unlock the current context */
    if(task_context(NULL, NULL) == task_context_task)
    {
        interrupt_unlock();
        task_unlock();
    }
    else
    {
        interrupt_unlock();
    }
#endif /* INTRUSIVE_TRACES */      
}

/* =======================================================================
   Name:        TraceVal
   Description: Function used to send on the UART the value of a signed
                variable, formatted for the Visual Debugger utility.
   Usage        TraceValue("<variable_name>",<variable_value>);
   Example      TraceValue("i",i);
   ======================================================================== */
void TraceVal(const char *VariableName_p, S32 Value)
{
    TIMER timer = time_now();
    U32 timer32 = (U32)timer;
   
    trace("Val:%s:%d:%lu\r\n", VariableName_p, Value, timer32);
}

/* =======================================================================
   Name:        TraceState
   Description: Function used to send on the UART the value of a multiple
                variable, formatted for the Visual Debugger utility.
   Usage        TraceState ("<variable_name>","<state_name>");
   Examples :   TraceState ("VSync","Top");
                TraceState ("VSync","Bot");
   ======================================================================== */
void TraceState(const char *VariableName_p, const char *format,...) 
{
    char Message[256];
    TIMER timer = time_now();
    U32 timer32 = (U32)timer;

    va_list         list;
    va_start (list, format);
    vsprintf ( Message , format, list);
    va_end (list);
    
    trace("Stt:%s:%s:%lu\r\n", VariableName_p, Message, timer32);
}

/* =======================================================================
   Name:        TraceMessage
   Description: Function used to send on the UART a characters string, 
                formatted for the Visual Debugger utility.
   Usage        TraceMessage("<variable_name>","<message>");
   Examples     TraceMessage ("Injection","Start");
                TraceMessage ("Injection","Stop");
   ======================================================================== */
void TraceMessage(const char *VariableName_p, const char *format,...)
{   
    char Message[256];
    TIMER timer = time_now();
    U32 timer32 = (U32)timer;

    va_list         list;
    va_start (list, format);
    vsprintf ( Message , format, list);
    va_end (list);

    trace("Msg:%s:%s:%lu\r\n", VariableName_p, Message, timer32);
}

/* =======================================================================
   Name:        trace2
   Description: local function that stores data to print into
                the print buffer and unlocks the task to print if not
                running
   ======================================================================== */
/*
void            trace2 (const char *format,...)
{
    va_list         list;

    va_start (list, format);
    if (trace_on)
    {
        if ( io_trace_uart2 != NULL )
        {
            trace_write2 += vsprintf ((char*) trace_write2 , format, list);
            if ( trace_write2 > ( trace_end2 - TRACE_THRESHOLD ) )
            {
                trace_limit2 = trace_write2;
                trace_write2 = trace_start2;
            }
            if (!trace_printing2)
            {
                semaphore_signal ( trace_lock2_p );
            }
         }
         else
         {
            trace_write += vsprintf ((char*) trace_write , format, list);
            if ( trace_write > ( trace_end - TRACE_THRESHOLD ) )
            {
                trace_limit = trace_write;
                trace_write = trace_start;
            }
            if (!trace_printing)
            {
                semaphore_signal ( trace_lock_p );
            }
        }
    }
    va_end (list);
}
*/

#if !defined(PROCESSOR_C1)
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
void            tracelatchtime ( void )
{
    U32     now;
    BOOL    History;

    TimerHigh ( now );
    History = trace_on;
    if (!trace_on)
    {
        trace_on = TRUE;
    }
    trace ("<-%d->", now - time_latch );
    trace_on = History;
}
#endif /* PROCESSOR_C1 */

#endif /* TRACE_UART */
/* ------------------------------- End of file ---------------------------- */

