/*****************************************************************************

File name   : trace.h

Description : visible functions available for development purpose

COPYRIGHT (C) ST-Microelectronics 2003

*****************************************************************************/
/* Define to prevent recursive inclusion */

#ifndef __TRACE_H
#define __TRACE_H

/* Includes ------------------------------------------------------------ */

/* Exported Types ------------------------------------------------------ */

/* Exported Constants -------------------------------------------------- */

/* Exported Variables -------------------------------------------------- */

extern void trace (const char *format,...);
extern BOOL TraceInit (void);
extern BOOL TraceDebug (void);

/* Exported Macros ----------------------------------------------------- */

#define TRACE_UART_DEVICE         DATA_UART_DEV

#ifdef ENABLE_TRACE
  #define TraceBuffer(x)  trace x
#else
  #define TraceBuffer(x)
#endif

/* Exported Functions -------------------------------------------------- */

#endif /* __TRACE_H */

/* EOF --------------------------------------------------------------------- */
