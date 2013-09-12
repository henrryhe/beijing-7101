/*****************************************************************************

File Name  : uart.h

Description: UART header

COPYRIGHT (C) 2005 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __UART_H
#define __UART_H

/* Includes --------------------------------------------------------------- */

#include "stpio.h"
#include "stuart.h"

/* Exported Types ------------------------------------------------------ */
/* Exported Constants -------------------------------------------------- */

/* Exported Variables -------------------------------------------------- */
STUART_Handle_t UART_Handle;

/* Exported Macros ----------------------------------------------------- */

#define ASC_DEVICE_TYPE STUART_RTSCTS

#if defined(ST_5100)
#define ASC_3_TXD_BIT   PIO_BIT_0
#define ASC_3_RXD_BIT   PIO_BIT_1
#define ASC_3_TXD_DEV   PIO_DEVICE_4
#define ASC_3_RXD_DEV   PIO_DEVICE_4
#define ASC_3_RTS_BIT   PIO_BIT_2
#define ASC_3_CTS_BIT   PIO_BIT_3
#define ASC_3_RTS_DEV   PIO_DEVICE_4
#define ASC_3_CTS_DEV   PIO_DEVICE_4

#elif defined(ST_5528)
#define ASC_3_TXD_BIT   PIO_BIT_4
#define ASC_3_RXD_BIT   PIO_BIT_5
#define ASC_3_RTS_BIT   PIO_BIT_7
#define ASC_3_CTS_BIT   PIO_BIT_6
#define ASC_3_TXD_DEV   PIO_DEVICE_5
#define ASC_3_RXD_DEV   PIO_DEVICE_5
#define ASC_3_RTS_DEV   PIO_DEVICE_5
#define ASC_3_CTS_DEV   PIO_DEVICE_5

#elif defined(ST_7710) || defined(ST_7100)
#define ASC_3_TXD_BIT   PIO_BIT_0
#define ASC_3_RXD_BIT   PIO_BIT_1
#define ASC_3_TXD_DEV   PIO_DEVICE_5
#define ASC_3_RXD_DEV   PIO_DEVICE_5
#define ASC_3_RTS_BIT   PIO_BIT_3
#define ASC_3_CTS_BIT   PIO_BIT_2
#define ASC_3_RTS_DEV   PIO_DEVICE_5
#define ASC_3_CTS_DEV   PIO_DEVICE_5

#elif defined(ST_5105)
#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_1
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_4
#define ASC_0_CTS_BIT   PIO_BIT_5
#define ASC_0_RTS_DEV   PIO_DEVICE_0
#define ASC_0_CTS_DEV   PIO_DEVICE_0

#endif

/* common uart identities */
#if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_7100)
#define DATA_UART_DEV              ASC_DEVICE_3
#define DATA_UART_BASE_ADDRESS     ASC_3_BASE_ADDRESS
#define DATA_UART_INTERRUPT        ASC_3_INTERRUPT
#define DATA_UART_INTERRUPT_LEVEL  ASC_3_INTERRUPT_LEVEL
#define DATA_UART_RXD_BIT          ASC_3_RXD_BIT
#define DATA_UART_RXD_DEV          ASC_3_RXD_DEV
#define DATA_UART_TXD_BIT          ASC_3_TXD_BIT
#define DATA_UART_TXD_DEV          ASC_3_TXD_DEV
#define DATA_UART_CTS_BIT          ASC_3_CTS_BIT
#define DATA_UART_CTS_DEV          ASC_3_CTS_DEV
#define DATA_UART_RTS_BIT          ASC_3_RTS_BIT
#define DATA_UART_RTS_DEV          ASC_3_RTS_DEV
#else /* 5105 */
#define DATA_UART_DEV              ASC_DEVICE_0
#define DATA_UART_BASE_ADDRESS     ASC_0_BASE_ADDRESS
#define DATA_UART_INTERRUPT        ASC_0_INTERRUPT
#define DATA_UART_INTERRUPT_LEVEL  ASC_0_INTERRUPT_LEVEL
#define DATA_UART_RXD_BIT          ASC_0_RXD_BIT
#define DATA_UART_RXD_DEV          ASC_0_RXD_DEV
#define DATA_UART_TXD_BIT          ASC_0_TXD_BIT
#define DATA_UART_TXD_DEV          ASC_0_TXD_DEV
#define DATA_UART_CTS_BIT          ASC_0_CTS_BIT
#define DATA_UART_CTS_DEV          ASC_0_CTS_DEV
#define DATA_UART_RTS_BIT          ASC_0_RTS_BIT
#define DATA_UART_RTS_DEV          ASC_0_RTS_DEV
#endif


#endif  /* __UART_H */

/* EOF --------------------------------------------------------------------- */
