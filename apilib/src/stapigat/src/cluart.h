/*******************************************************************************

File name   : cluart.h

Description : UART configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
31 Jan 2002        Created                                           HSdLM
05 Jun 2002        Add 5516 support                                  HSdLM
11 Jun 2002        Add Open/Close                                    HSdLM
14 Jun 2002        RS233_A is not working on mb361a                  AN
14 Oct 2002        Add support for 5517                              HSdLM
30 Oct 2002        Ability to choose UART_BAUD_RATE                  HSdLM
05 May 2003        Ability to choose RS232_A or B                    HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLUART_H
#define __CLUART_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "testcfg.h"
#ifdef TRACE_UART
#include "stuart.h"
#endif

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#ifndef UART_BAUD_RATE
#define UART_BAUD_RATE        115200
#endif /* UART_BAUD_RATE */

#if !(defined(RS232_A) || defined(RS232_B)) /* can be forced in testcfg.h */
/* default value if user has not set it */
#if defined(mb361) || defined(mb382) || defined(mb390)
#define RS232_B /* advised as RS232_A is not working on mb361a / mb382 */
#else
#define RS232_A
#endif /* mb361 || mb382 */
#endif /* #if !(defined(RS232_A) || defined(RS232_B)) */

/* STUART_DEVICE_NAME */
#if ((defined(ST_5516) || defined(ST_5517)) && defined(RS232_A))
#define STUART_DEVICE_NAME   "ASC4"
#elif defined(ST_5510) || defined(ST_5512) || defined(ST_5514) || defined(ST_TP3)
#define STUART_DEVICE_NAME   "ASC3"
#elif defined(ST_5508) || defined(ST_5518) || \
      ((defined(ST_5516) || defined(ST_5517)) && defined(RS232_B))
#define STUART_DEVICE_NAME   "ASC2"
#elif defined(ST_GX1) || defined(ST_5188)
#define STUART_DEVICE_NAME   "ASC0"
#elif defined(ST_5528) || defined(ST_5100) || defined(ST_5301)
#define STUART_DEVICE_NAME   "ASC3"
#elif defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
/* Testtool on ASC2, traces on ASC3 if required */
#define STUART_DEVICE_NAME   "ASC2"
#ifdef TRACE_UART
#define TRACE_UART_DEVICE    "ASC3"
#endif
#else
#define STUART_DEVICE_NAME   "UART"
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

#ifdef TRACE_UART
extern STUART_Handle_t TraceUartHandle;
#endif

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

BOOL UART_Init(void);
void UART_Term(void);
void Test_Uart_Driver_fn(void);

#ifdef TRACE_UART
BOOL UART_Open(void);
void UART_Close(void);
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLUART_H */

/* End of cluart.h */
