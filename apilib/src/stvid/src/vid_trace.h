/*******************************************************************************

File name   : vid_trace.h

Description : Activation/Deactivation of Debug Traces in stvid

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
25 Oct 2006        Created                                           OL
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VID_TRACE_H
#define __VID_TRACE_H


/* Includes ----------------------------------------------------------------- */

#if defined  TRACE_UART || defined VIDEO_DEBUG
    #include "../tests/src/trace.h"
#endif

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#ifdef ST_OSWINCE
    #define trace_debug     printf
#else
    #define trace_debug     trace
#endif

#ifdef TRACE_UART
     /* Traces activated */
    #define TraceOn(x)             trace_debug x
    #define TraceOff(x)

    #define TraceValOn(x)        TraceVal x
    #define TraceValOff(x)

    #define TraceStateOn(x)    TraceState x
    #define TraceStateOff(x)

    #define TraceMsgOn(x)       TraceMessage x
    #define TraceMsgOff(x)

#else  /* !TRACE_UART */
    /* Traces not activated */
    #define TraceOn(x)
    #define TraceOff(x)

    #define TraceValOn(x)
    #define TraceValOff(x)

    #define TraceStateOn(x)
    #define TraceStateOff(x)

    #define TraceMsgOn(x)
    #define TraceMsgOff(x)

#endif /* TRACE_UART */


#if defined STVID_DEBUG_TAKE_RELEASE || defined TRACE_UART
    #define TraceTakeRelease(x)             trace_debug x
#else
    #define TraceTakeRelease(x)
#endif

#if defined  TRACE_UART || defined VIDEO_DEBUG
    #define TrError(x)                trace x
#else
    #define TrError(x)
#endif

/* Exported Functions ------------------------------------------------------- */



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VID_TRACE_H */

/* End of vid_trace.h */
