/******************************************************************************

File Name  : nginit.h

Description: ST Net header

COPYRIGHT (C) 2007 STMicroelectronics

******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __NGINIT_H
#define __NGINIT_H

/* C++ support ------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ---------------------------------------------------------------- */
#include "stddefs.h"
#include "stos.h"

 #include <osplus.h>

 #include <ngip.h>
 #include <ngtcp.h>
 #include <ngudp.h>

#if defined(ENABLE_TRACE)
 #include "trace.h"
#endif

/* Exported Constants ------------------------------------------------------ */



/* Exported Variables ------------------------------------------------------ */

#define ETHERNET_DEVICE_NAME "eth0"

#define STNETi_RECEIVER_TASK_MQ_SIZE    (4)
extern NGsockaddr            NG_SocketAddr;


BOOL NG_Start;

/* Exported Macros --------------------------------------------------------- */
#if defined(ENABLE_TRACE)
 #define STNETi_TraceDBG1(x)     TraceBuffer(x)
#else
#define STNETi_TraceDBG1(x)      {}
#endif

/* Exported Types ---------------------------------------------------------- */

message_queue_t    *STNETi_TaskMQ_p;



/* C++ support ------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif
#endif /* __NGINIT_H */

/* EOF --------------------------------------------------------------------- */

