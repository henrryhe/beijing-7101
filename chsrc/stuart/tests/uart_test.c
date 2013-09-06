/*****************************************************************************
File Name   : uart_test.c

Description : Test harness for the UART driver.

Copyright (C) 2005 STMicroelectronics

Revision History :

    05/04/00    Added support for STi5508 testing.

    28/11/01    Add register test for new device type ISO7816

    25/08/03    Added support for 5528 testing.

    10/11/03    Added support for 5100 testing.

    30/01/04    Added support for 7710 testing.

    15/08/04    Added multithread-safe test.
                Ported on OS21.

    04/10/04    Added support for 5105 testing.

    13/12/04    Added support for 5700 testing.

    10/01/05    Added support for 7100 testing.

    14/01/05    Added support for 5301 testing.

    22/04/05    Added support for 8010 testing.

    27/10/05    Added support for 7109 testing.

    05/12/05    Added support for 5188 testing.

    05/01/06    Added support for 5525 testing.

    26/04/06    Added support for 5107 testing.

    28/02/07    Added support for 7200 testing.

Reference   :

ST API Definition "UART Driver API" DVD-API-22
*****************************************************************************/

#include <stdio.h>                      /* C libs */
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "stlite.h"                     /* System */
#include "stdevice.h"

#include "stuart.h"                     /* STAPI */
#include "stboot.h"
#include "stcommon.h"

#ifdef STTBX_PRINT
#include "sttbx.h"
#endif

#include "stsys.h"
#include "stos.h"

#ifdef STUART_DMA_SUPPORT
#include "stfdma.h"
#endif

#include "uart_test.h"                  /* Local */

#ifdef CODETEST                         /* Codetest definitions */
#include "codetest.h"
#endif

#ifdef OS40
#include <chorus.h>
#include <exec/p_boardInfo.H>
#include <exec/chContext.h>
#endif

#ifdef ST_OS21

#define STACK_SIZE  20 * 1024
#else
#define STACK_SIZE  4 * 1024
#endif

#define TIME_DELAY  ST_GetClocksPerSecond()/100

/*****************************************************************************
Types and definitions
*****************************************************************************/

/* Fifth UART defines for 5514 */
#if defined (ST_5514) /*||defined(ST_5516) */

#ifndef ASC_4_BASE_ADDRESS
#define ASC_4_BASE_ADDRESS    0x20114000
#endif

#ifndef ASC_4_INTERRUPT
#define ASC_4_INTERRUPT       35
#endif

#ifndef ASC_4_INTERRUPT_LEVEL
#define ASC_4_INTERRUPT_LEVEL  1
#endif

#endif

#if defined(ST_5100) /* These should have been in sti5100.h in chip vob */

#ifndef PIO_4_BASE_ADDRESS
#define PIO_4_BASE_ADDRESS              ST5100_PIO4_BASE_ADDRESS
#endif

#ifndef PIO_4_INTERRUPT
#define PIO_4_INTERRUPT                 ST5100_PIO4_INTERRUPT
#endif

#endif

#define NUMBER_FDMA             1
/* Defines number of devices */
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)

#define NUMBER_PIO              6
#define NUMBER_ASC              5

#elif defined(ST_5528)
#define NUMBER_PIO              8
#define NUMBER_ASC              5

#elif defined(ST_7710) || defined(ST_7100) || defined(ST_5301) || defined(ST_7109)
#define NUMBER_PIO              6
#define NUMBER_ASC              4

#elif defined(ST_7200)
#define NUMBER_PIO              8
#define NUMBER_ASC              4

#elif defined(ST_5105) || defined(ST_5107)
#define NUMBER_PIO              4
#define NUMBER_ASC              2

#elif defined(ST_5700)
#define NUMBER_PIO              6
#define NUMBER_ASC              1

#elif defined(ST_8010)
#define NUMBER_PIO              7
#define NUMBER_ASC              3 /* 2 for smartcard, 1 com */

#elif defined(ST_5188)
#define NUMBER_PIO              4
#define NUMBER_ASC              1

#elif defined(ST_5525)
#define NUMBER_PIO              9
#define NUMBER_ASC              4
#ifndef PIO_8_INTERRUPT_LEVEL  /* This should have been in include vob*/
#define PIO_8_INTERRUPT_LEVEL   1
#endif

#else /* 5100 */
#define NUMBER_PIO              5
#define NUMBER_ASC              4

#endif

/* Enum of ASC devices */
enum
{
    ASC_DEVICE_0,
    ASC_DEVICE_1,
    ASC_DEVICE_2,
    ASC_DEVICE_3,
    ASC_DEVICE_4,
    ASC_DEVICE_NOT_USED
};

/* Enum of PIO devices */
enum
{
    PIO_DEVICE_0,
    PIO_DEVICE_1,
    PIO_DEVICE_2,
    PIO_DEVICE_3,
    PIO_DEVICE_4,
    PIO_DEVICE_5,
    PIO_DEVICE_6,/* 5528 ,5525 only */
    PIO_DEVICE_7,/* 5528 ,5525 only */
    PIO_DEVICE_8,/*5525 only*/
    PIO_DEVICE_NOT_USED
};

/* Bit definition for an unused PIO bit */
#define PIO_BIT_NOT_USED        0

/* Define ASC device types */

#if defined(ST_5510) || defined(ST_5508) || defined(ST_TP3) || defined(ST_5188)

#define ASC_DEVICE_TYPE         STUART_16_BYTE_FIFO

#elif defined(ST_5512) || defined(ST_5514) || defined(ST_5516) || defined(ST_5517) \
   || defined(ST_5518) || defined(ST_GX1)  || defined(ST_5528) || defined(ST_5100) \
   || defined(ST_7710) || defined(ST_5105) || defined(ST_5700) ||  defined(ST_7100)\
   || defined(ST_5301) || defined(ST_8010) || defined(ST_7109) || defined(ST_5525) \
   || defined(ST_5107)  || defined(ST_7200)

#define ASC_DEVICE_TYPE         STUART_RTSCTS

#endif

#if defined(ST_5510) || defined(ST_5512)  || defined(ST_TP3)

#define ASC_DEVICE_DATA         ASC_DEVICE_3
#define ASC_DEVICE_MODEM        ASC_DEVICE_1
#define ASC_DEVICE_SC0          ASC_DEVICE_0
#define ASC_DEVICE_SC1          ASC_DEVICE_2

#elif defined(ST_5508) || defined(ST_5518)

#define ASC_DEVICE_DATA         ASC_DEVICE_2
#define ASC_DEVICE_MODEM        ASC_DEVICE_1
#define ASC_DEVICE_SC0          ASC_DEVICE_0
#define ASC_DEVICE_SC1          ASC_DEVICE_3

#elif defined(ST_GX1) || defined(ST_NGX1)

#define ASC_DEVICE_DATA         ASC_DEVICE_0
#define ASC_DEVICE_MODEM        ASC_DEVICE_1 /* not used */
#define ASC_DEVICE_SC0          ASC_DEVICE_NOT_USED
#define ASC_DEVICE_SC1          ASC_DEVICE_NOT_USED

#elif defined(ST_5528)

#define ASC_DEVICE_SC0          ASC_DEVICE_0
#define ASC_DEVICE_SC1          ASC_DEVICE_1
#define ASC_DEVICE_DATA         ASC_DEVICE_3
#define ASC_DEVICE_MODEM        ASC_DEVICE_4

#elif defined(ST_7710)

#define ASC_DEVICE_SC0          ASC_DEVICE_0
#define ASC_DEVICE_SC1          ASC_DEVICE_2
#define ASC_DEVICE_DATA         ASC_DEVICE_3
#define ASC_DEVICE_MODEM        ASC_DEVICE_1

#elif defined(ST_5100) || defined(ST_5301)

#define ASC_DEVICE_SC0          ASC_DEVICE_0
#define ASC_DEVICE_SC1          ASC_DEVICE_1
#define ASC_DEVICE_DATA         ASC_DEVICE_3
#define ASC_DEVICE_MODEM        ASC_DEVICE_1

#elif defined(ST_7200)
#define ASC_DEVICE_SC0          	ASC_DEVICE_0
#define ASC_DEVICE_SC1          	ASC_DEVICE_1
#define ASC_DEVICE_DATA         	ASC_DEVICE_2
#define ASC_DEVICE_MODEM        	ASC_DEVICE_3

#elif defined(ST_5105) || defined(ST_5107)

#define ASC_DEVICE_DATA         ASC_DEVICE_1
#define ASC_DEVICE_MODEM        ASC_DEVICE_1
#define ASC_DEVICE_SC0          ASC_DEVICE_0

#elif defined(ST_5700)

#define ASC_DEVICE_DATA         ASC_DEVICE_0
#define ASC_DEVICE_MODEM        ASC_DEVICE_0 /* theres' only 1 uart on 5700 */

#elif defined(ST_8010)

#define ASC_DEVICE_DATA         ASC_DEVICE_2
#define ASC_DEVICE_MODEM        ASC_DEVICE_2/* theres' only 1 uart on 8010*/

#elif defined(ST_5514)  || defined(ST_7100)  || defined(ST_7109)

#define ASC_DEVICE_SC0          ASC_DEVICE_0
#define ASC_DEVICE_SC1          ASC_DEVICE_1

#if defined (MEDIAREF)
#define ASC_DEVICE_DATA         ASC_DEVICE_4
#define ASC_DEVICE_MODEM        ASC_DEVICE_4 /* Same as DATA*/
#else
#define ASC_DEVICE_DATA         ASC_DEVICE_3
#define ASC_DEVICE_MODEM        ASC_DEVICE_2
#endif

#elif defined(ST_5516) || defined(ST_5517)

#define ASC_DEVICE_DATA         ASC_DEVICE_3 /* was 4 */ /* was 3 */
#define ASC_DEVICE_MODEM        ASC_DEVICE_2 /* was 2 */ /* was 4 */

#elif defined(ST_5188)

#define ASC_DEVICE_DATA         ASC_DEVICE_0
#define ASC_DEVICE_MODEM        ASC_DEVICE_0 /* theres' only 1 uart on 5188 */

#elif defined(ST_5525)

#define ASC_DEVICE_SC0          ASC_DEVICE_0
#define ASC_DEVICE_SC1          ASC_DEVICE_1
#define ASC_DEVICE_DATA         ASC_DEVICE_2
#define ASC_DEVICE_MODEM        ASC_DEVICE_3

#endif

/* Use default interrupt levels if running on ST40
   architecture
*/
#ifdef ARCHITECTURE_ST40

#undef  ASC_0_INTERRUPT_LEVEL
#undef  ASC_1_INTERRUPT_LEVEL

#define ASC_0_INTERRUPT_LEVEL   0
#define ASC_1_INTERRUPT_LEVEL   0

#endif

/* Port pins for PIO */

#if defined(ST_5510) || defined(ST_5512) || defined(ST_TP3)

#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_1
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_5
#define ASC_1_RXD_BIT   PIO_BIT_6
#define ASC_1_TXD_DEV   PIO_DEVICE_1
#define ASC_1_RXD_DEV   PIO_DEVICE_1
#define ASC_1_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_2_TXD_BIT   PIO_BIT_0
#define ASC_2_RXD_BIT   PIO_BIT_1
#define ASC_2_TXD_DEV   PIO_DEVICE_2
#define ASC_2_RXD_DEV   PIO_DEVICE_2
#define ASC_2_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_2_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_2_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_2_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_3_TXD_BIT   PIO_BIT_0
#define ASC_3_RXD_BIT   PIO_BIT_1
#define ASC_3_TXD_DEV   PIO_DEVICE_4
#define ASC_3_RXD_DEV   PIO_DEVICE_4
#define ASC_3_RTS_BIT   PIO_BIT_4
#define ASC_3_CTS_BIT   PIO_BIT_3
#define ASC_3_RTS_DEV   PIO_DEVICE_4
#define ASC_3_CTS_DEV   PIO_DEVICE_4

#elif defined(ST_5508)

#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_0
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_5
#define ASC_1_RXD_BIT   PIO_BIT_1
#define ASC_1_TXD_DEV   PIO_DEVICE_1
#define ASC_1_RXD_DEV   PIO_DEVICE_2
#define ASC_1_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_2_TXD_BIT   PIO_BIT_3
#define ASC_2_RXD_BIT   PIO_BIT_4
#define ASC_2_TXD_DEV   PIO_DEVICE_1
#define ASC_2_RXD_DEV   PIO_DEVICE_1
#define ASC_2_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_2_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_2_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_2_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_3_TXD_BIT   PIO_BIT_0
#define ASC_3_RXD_BIT   PIO_BIT_0
#define ASC_3_TXD_DEV   PIO_DEVICE_2
#define ASC_3_RXD_DEV   PIO_DEVICE_2
#define ASC_3_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_3_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_3_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_3_CTS_DEV   PIO_DEVICE_NOT_USED

#elif defined(ST_5518)

#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_0
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_5
#define ASC_1_RXD_BIT   PIO_BIT_1
#define ASC_1_TXD_DEV   PIO_DEVICE_1
#define ASC_1_RXD_DEV   PIO_DEVICE_2
#define ASC_1_RTS_BIT   PIO_BIT_4
#define ASC_1_CTS_BIT   PIO_BIT_6
#define ASC_1_RTS_DEV   PIO_DEVICE_3
#define ASC_1_CTS_DEV   PIO_DEVICE_3

#define ASC_2_TXD_BIT   PIO_BIT_3
#define ASC_2_RXD_BIT   PIO_BIT_4
#define ASC_2_TXD_DEV   PIO_DEVICE_1
#define ASC_2_RXD_DEV   PIO_DEVICE_1
#define ASC_2_RTS_BIT   PIO_BIT_5
#define ASC_2_CTS_BIT   PIO_BIT_7
#define ASC_2_RTS_DEV   PIO_DEVICE_3
#define ASC_2_CTS_DEV   PIO_DEVICE_3

#define ASC_3_TXD_BIT   PIO_BIT_0
#define ASC_3_RXD_BIT   PIO_BIT_0
#define ASC_3_TXD_DEV   PIO_DEVICE_2
#define ASC_3_RXD_DEV   PIO_DEVICE_2
#define ASC_3_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_3_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_3_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_3_CTS_DEV   PIO_DEVICE_NOT_USED

#elif defined (ST_GX1) || defined (ST_NGX1)

#define ASC_0_TXD_BIT   PIO_BIT_NOT_USED
#define ASC_0_RXD_BIT   PIO_BIT_NOT_USED
#define ASC_0_TXD_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_RXD_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_NOT_USED
#define ASC_1_RXD_BIT   PIO_BIT_NOT_USED
#define ASC_1_TXD_DEV   PIO_DEVICE_NOT_USED
#define ASC_1_RXD_DEV   PIO_DEVICE_NOT_USED
#define ASC_1_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_2_TXD_BIT   PIO_BIT_NOT_USED
#define ASC_2_RXD_BIT   PIO_BIT_NOT_USED
#define ASC_2_TXD_DEV   PIO_DEVICE_NOT_USED
#define ASC_2_RXD_DEV   PIO_DEVICE_NOT_USED
#define ASC_2_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_2_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_2_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_2_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_3_TXD_BIT   PIO_BIT_NOT_USED
#define ASC_3_RXD_BIT   PIO_BIT_NOT_USED
#define ASC_3_TXD_DEV   PIO_DEVICE_NOT_USED
#define ASC_3_RXD_DEV   PIO_DEVICE_NOT_USED
#define ASC_3_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_3_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_3_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_3_CTS_DEV   PIO_DEVICE_NOT_USED

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528)

#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_1
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_0
#define ASC_1_RXD_BIT   PIO_BIT_1
#define ASC_1_TXD_DEV   PIO_DEVICE_1
#define ASC_1_RXD_DEV   PIO_DEVICE_1
#define ASC_1_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV   PIO_DEVICE_NOT_USED


#define ASC_2_TXD_BIT   PIO_BIT_4
#define ASC_2_RXD_BIT   PIO_BIT_3
#define ASC_2_TXD_DEV   PIO_DEVICE_4
#define ASC_2_RXD_DEV   PIO_DEVICE_4
#define ASC_2_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_2_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_2_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_2_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_3_TXD_BIT   PIO_BIT_4
#define ASC_3_RXD_BIT   PIO_BIT_5
#define ASC_3_TXD_DEV   PIO_DEVICE_5
#define ASC_3_RXD_DEV   PIO_DEVICE_5
#define ASC_3_RTS_BIT   PIO_BIT_7
#define ASC_3_CTS_BIT   PIO_BIT_6
#define ASC_3_RTS_DEV   PIO_DEVICE_5
#define ASC_3_CTS_DEV   PIO_DEVICE_5

#if defined(ST_5528)
#define ASC_4_TXD_BIT   PIO_BIT_0
#define ASC_4_RXD_BIT   PIO_BIT_1
#define ASC_4_TXD_DEV   PIO_DEVICE_6
#define ASC_4_RXD_DEV   PIO_DEVICE_6
#define ASC_4_RTS_BIT   PIO_BIT_3
#define ASC_4_CTS_BIT   PIO_BIT_2
#define ASC_4_RTS_DEV   PIO_DEVICE_6
#define ASC_4_CTS_DEV   PIO_DEVICE_6
#else
#define ASC_4_TXD_BIT   PIO_BIT_1
#define ASC_4_RXD_BIT   PIO_BIT_2
#define ASC_4_TXD_DEV   PIO_DEVICE_2
#define ASC_4_RXD_DEV   PIO_DEVICE_2
#define ASC_4_RTS_BIT   PIO_BIT_0
#define ASC_4_CTS_BIT   PIO_BIT_3
#define ASC_4_RTS_DEV   PIO_DEVICE_2
#define ASC_4_CTS_DEV   PIO_DEVICE_2
#endif
#endif

#if defined(ST_5100) || defined(ST_5301)
#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_1
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_0
#define ASC_1_RXD_BIT   PIO_BIT_1
#define ASC_1_TXD_DEV   PIO_DEVICE_1
#define ASC_1_RXD_DEV   PIO_DEVICE_1
#define ASC_1_RTS_BIT   PIO_BIT_4
#define ASC_1_CTS_BIT   PIO_BIT_5
#define ASC_1_RTS_DEV   PIO_DEVICE_1
#define ASC_1_CTS_DEV   PIO_DEVICE_1

#define ASC_2_TXD_BIT   PIO_BIT_1
#define ASC_2_RXD_BIT   PIO_BIT_2
#define ASC_2_TXD_DEV   PIO_DEVICE_2
#define ASC_2_RXD_DEV   PIO_DEVICE_2
#define ASC_2_RTS_BIT   PIO_BIT_0
#define ASC_2_CTS_BIT   PIO_BIT_3
#define ASC_2_RTS_DEV   PIO_DEVICE_2
#define ASC_2_CTS_DEV   PIO_DEVICE_2

#define ASC_3_TXD_BIT   PIO_BIT_0
#define ASC_3_RXD_BIT   PIO_BIT_1
#define ASC_3_TXD_DEV   PIO_DEVICE_4
#define ASC_3_RXD_DEV   PIO_DEVICE_4
#define ASC_3_RTS_BIT   PIO_BIT_2
#define ASC_3_CTS_BIT   PIO_BIT_3
#define ASC_3_RTS_DEV   PIO_DEVICE_4
#define ASC_3_CTS_DEV   PIO_DEVICE_4

#endif

#if defined(ST_7710)

#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_1
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_0
#define ASC_1_RXD_BIT   PIO_BIT_1
#define ASC_1_TXD_DEV   PIO_DEVICE_1
#define ASC_1_RXD_DEV   PIO_DEVICE_1
#define ASC_1_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_2_TXD_BIT   PIO_BIT_3
#define ASC_2_RXD_BIT   PIO_BIT_2
#define ASC_2_TXD_DEV   PIO_DEVICE_4
#define ASC_2_RXD_DEV   PIO_DEVICE_4
#define ASC_2_RTS_BIT   PIO_BIT_5
#define ASC_2_CTS_BIT   PIO_BIT_4
#define ASC_2_RTS_DEV   PIO_DEVICE_4
#define ASC_2_CTS_DEV   PIO_DEVICE_4

#define ASC_3_TXD_BIT   PIO_BIT_0
#define ASC_3_RXD_BIT   PIO_BIT_1
#define ASC_3_TXD_DEV   PIO_DEVICE_5
#define ASC_3_RXD_DEV   PIO_DEVICE_5
#define ASC_3_RTS_BIT   PIO_BIT_3
#define ASC_3_CTS_BIT   PIO_BIT_2
#define ASC_3_RTS_DEV   PIO_DEVICE_5
#define ASC_3_CTS_DEV   PIO_DEVICE_5

#endif

#if defined(ST_5105) || defined(ST_5107)

#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_1
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_4
#define ASC_0_CTS_BIT   PIO_BIT_5
#define ASC_0_RTS_DEV   PIO_DEVICE_0
#define ASC_0_CTS_DEV   PIO_DEVICE_0

#define ASC_1_TXD_BIT   PIO_BIT_0
#define ASC_1_RXD_BIT   PIO_BIT_1
#define ASC_1_TXD_DEV   PIO_DEVICE_2
#define ASC_1_RXD_DEV   PIO_DEVICE_2
#define ASC_1_RTS_BIT   PIO_BIT_2
#define ASC_1_CTS_BIT   PIO_BIT_3
#define ASC_1_RTS_DEV   PIO_DEVICE_2
#define ASC_1_CTS_DEV   PIO_DEVICE_2

#endif

#if defined(ST_5700)

#define ASC_0_TXD_BIT   PIO_BIT_2
#define ASC_0_RXD_BIT   PIO_BIT_3
#define ASC_0_TXD_DEV   PIO_DEVICE_1
#define ASC_0_RXD_DEV   PIO_DEVICE_1
#define ASC_0_RTS_BIT   PIO_BIT_4
#define ASC_0_CTS_BIT   PIO_BIT_5
#define ASC_0_RTS_DEV   PIO_DEVICE_1
#define ASC_0_CTS_DEV   PIO_DEVICE_1

#endif

#if defined(ST_5188)

#define ASC_0_TXD_BIT   PIO_BIT_1
#define ASC_0_RXD_BIT   PIO_BIT_6
#define ASC_0_TXD_DEV   PIO_DEVICE_2
#define ASC_0_RXD_DEV   PIO_DEVICE_1
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#endif

#if defined(ST_8010)

#define ASC_0_TXD_BIT   PIO_BIT_4
#define ASC_0_RXD_BIT   PIO_BIT_5
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_3
#define ASC_1_RXD_BIT   PIO_BIT_4
#define ASC_1_TXD_DEV   PIO_DEVICE_1
#define ASC_1_RXD_DEV   PIO_DEVICE_1
#define ASC_1_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_2_TXD_BIT   PIO_BIT_3
#define ASC_2_RXD_BIT   PIO_BIT_4
#define ASC_2_TXD_DEV   PIO_DEVICE_1
#define ASC_2_RXD_DEV   PIO_DEVICE_1
#define ASC_2_RTS_BIT   PIO_BIT_4
#define ASC_2_CTS_BIT   PIO_BIT_5
#define ASC_2_RTS_DEV   PIO_DEVICE_2
#define ASC_2_CTS_DEV   PIO_DEVICE_2

#endif

#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)

#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_1
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_0
#define ASC_1_RXD_BIT   PIO_BIT_1
#define ASC_1_TXD_DEV   PIO_DEVICE_1
#define ASC_1_RXD_DEV   PIO_DEVICE_1
#define ASC_1_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV   PIO_DEVICE_NOT_USED

#if defined(ST_7100) || defined(ST_7109)

#define ASC_2_TXD_BIT   PIO_BIT_3
#define ASC_2_RXD_BIT   PIO_BIT_2
#define ASC_2_TXD_DEV   PIO_DEVICE_4
#define ASC_2_RXD_DEV   PIO_DEVICE_4
#define ASC_2_RTS_BIT   PIO_BIT_5
#define ASC_2_CTS_BIT   PIO_BIT_4
#define ASC_2_RTS_DEV   PIO_DEVICE_4
#define ASC_2_CTS_DEV   PIO_DEVICE_4

#define ASC_3_TXD_BIT   PIO_BIT_0
#define ASC_3_RXD_BIT   PIO_BIT_1
#define ASC_3_TXD_DEV   PIO_DEVICE_5
#define ASC_3_RXD_DEV   PIO_DEVICE_5
#define ASC_3_RTS_BIT   PIO_BIT_3
#define ASC_3_CTS_BIT   PIO_BIT_2
#define ASC_3_RTS_DEV   PIO_DEVICE_5
#define ASC_3_CTS_DEV   PIO_DEVICE_5

#elif defined(ST_7200)

#define ASC_2_TXD_BIT   PIO_BIT_3
#define ASC_2_RXD_BIT   PIO_BIT_2
#define ASC_2_TXD_DEV   PIO_DEVICE_4
#define ASC_2_RXD_DEV   PIO_DEVICE_4
#define ASC_2_RTS_BIT   PIO_BIT_5
#define ASC_2_CTS_BIT   PIO_BIT_4
#define ASC_2_RTS_DEV   PIO_DEVICE_4
#define ASC_2_CTS_DEV   PIO_DEVICE_4

#define ASC_3_TXD_BIT   PIO_BIT_4
#define ASC_3_RXD_BIT   PIO_BIT_3
#define ASC_3_TXD_DEV   PIO_DEVICE_5
#define ASC_3_RXD_DEV   PIO_DEVICE_5
#define ASC_3_RTS_BIT   PIO_BIT_6
#define ASC_3_CTS_BIT   PIO_BIT_5
#define ASC_3_RTS_DEV   PIO_DEVICE_5
#define ASC_3_CTS_DEV   PIO_DEVICE_5
#endif

#endif

#if defined(ST_5525)

#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_1
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_0
#define ASC_1_RXD_BIT   PIO_BIT_1
#define ASC_1_TXD_DEV   PIO_DEVICE_1
#define ASC_1_RXD_DEV   PIO_DEVICE_1
#define ASC_1_RTS_BIT   PIO_BIT_5
#define ASC_1_CTS_BIT   PIO_BIT_7
#define ASC_1_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_2_TXD_BIT   PIO_BIT_1
#define ASC_2_RXD_BIT   PIO_BIT_2
#define ASC_2_TXD_DEV   PIO_DEVICE_2
#define ASC_2_RXD_DEV   PIO_DEVICE_2
#define ASC_2_RTS_BIT   PIO_BIT_0
#define ASC_2_CTS_BIT   PIO_BIT_3
#define ASC_2_RTS_DEV   PIO_DEVICE_2
#define ASC_2_CTS_DEV   PIO_DEVICE_2

#define ASC_3_TXD_BIT   PIO_BIT_0
#define ASC_3_RXD_BIT   PIO_BIT_1
#define ASC_3_TXD_DEV   PIO_DEVICE_4
#define ASC_3_RXD_DEV   PIO_DEVICE_4
#define ASC_3_RTS_BIT   PIO_BIT_2
#define ASC_3_CTS_BIT   PIO_BIT_3
#define ASC_3_RTS_DEV   PIO_DEVICE_4
#define ASC_3_CTS_DEV   PIO_DEVICE_4

#endif

#if defined(ST_5516) || defined(ST_5517)

#define INTERCONNECT_BASE                 (U32)CFG_BASE_ADDRESS
#define INTERCONNECT_CONFIG_CONTROL_REG_A       0x00  /* 4 bytes access */
#define INTERCONNECT_CONFIG_CONTROL_REG_B       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_C       0x08
#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x0C
#define INTERCONNECT_MONITOR_CONTROL_REG_A      0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_E       0x28
#define CONFIG_MONITOR_A (INTERCONNECT_BASE + INTERCONNECT_MONITOR_CONTROL_REG_A)
#define CONFIG_CONTROL_A (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_A)
#define CONFIG_CONTROL_B (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_B)
#define CONFIG_CONTROL_C (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_C)
#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_E (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_E)

#endif

#if defined(ST_5100)

#define INTERCONNECT_BASE                 (U32)0x20D00000

#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_F       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_G       0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_H       0x14

#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)

#endif

#if defined(ST_5105) || defined(ST_5107)

#define INTERCONNECT_BASE                 (U32)0x20402000
#define INTERCONNECT_CONFIG_CONTROL_REG_H      0x14
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)
#endif

#if defined(ST_7200)

#define SYSTEM_CONFIG_BASE_ADDRESS          	ST7200_CFG_BASE_ADDRESS
#define SYSTEM_CONFIG_REG_7			 	0x11c
#define SYSTEM_CONFIG_7					(SYSTEM_CONFIG_BASE_ADDRESS + SYSTEM_CONFIG_REG_7)
#endif



/* Macro for computing min of two numbers */
#ifdef min
#undef min
#endif

#define min(x,y) (x<y?x:y)

#define ST_ERROR_UNKNOWN ((U32)-1)

/* Global semaphore for some tests */
semaphore_t *WaitBlockedSemaphore_p;
ST_ErrorCode_t UartRxErrorCode, UartTxErrorCode;

/* Type used for passing information between tasks and main application */
typedef struct
{
    ST_ErrorCode_t Error;

#ifdef STUART_STATISTICS
    STUART_Statistics_t *Statistics_p;
#endif

    U32 ticks;
    U8  ASCNumber;
    ST_Partition_t  *Driverpartition;
    task_t 		     *Task_p;
} UART_TaskParams_t;

#if defined(TEST_MULTITHREADSAFE)
typedef struct
{
    STUART_Handle_t Handle;
    U8  ASCNumber;
    S32 ticks;
    BOOL IsBlocking;
 }UART_TraceTaskParams_t;
#endif

/* CLOCK macros */
#if defined(ARCHITECTURE_ST20) || defined(ARCHITECTURE_ST200)

#define CPU_CLOCK           ST_GetClockSpeed()
#define TICKS_PER_SECOND    ST_GetClocksPerSecond()
#define UART_CLOCK          ClockInfo.CommsBlock
#else /* architecture ST40 */

#define CPU_CLOCK           ST_GetClockSpeed()
#define TICKS_PER_SECOND    ST_GetClocksPerSecond()
#ifdef ST_GX1 /* Workaround for cut 1.0 of nGX1 - per_clk not correctly connected */
#define UART_CLOCK          boardInfo->peripheralClockFrequency
#else
#if defined(ST_7100)  || defined(ST_7109) 
#define UART_CLOCK          90000000 /* UART works with Comms Clock of 66.5Mhz for hyperterminal*/

#elif defined(ST_7200)
#define UART_CLOCK          100000000 

#else
#define UART_CLOCK          50000000 /* UART also works with Comms Clock of 133MHz & 200MHz */

#endif
#endif

#endif /*end of ARCHITECTURE */

/* Board options */

#if !(defined(mb275) || defined(mb275_64) || defined(mb317a) || \
      defined(mb5518) || defined(mb5518um)|| defined(mb317b) || defined(MEDIAREF) || defined(mb385) ||\
     defined(mb400) || defined(mb421) || defined(mb426) || defined(mb457) || defined(mb436))
#define TEST_TWO_PORTS
#endif

#define LOOPBACK_BAUDRATE 115200 /*38400*/

/*****************************************************************************
Global data
*****************************************************************************/

BOOL KillTransmitter = FALSE;

static ST_ClockInfo_t ClockInfo;

/* Test harness revision number */
static U8 Revision[] = "1.8.9A0";

/* Placeholder for device name */
static ST_DeviceName_t      UartDeviceName[] =
{
    "asc0",
    "asc1",
    "asc2",
    "asc3",
    "asc4",
    ""
};

static ST_DeviceName_t      PioDeviceName[] =
{
    "pio0",
    "pio1",
    "pio2",
    "pio3",
    "pio4",
    "pio5",
    "pio6", /*5528,5525*/
    "pio7", /*5528,5525*/
    "pio8", /*5525*/
    ""
};

#if defined(STUART_DMA_SUPPORT)
#define FDMA_INTERRUPT_NUMBER  FDMA_INTERRUPT
#define FDMA_INTERRUPT_LEVEL   14
static  ST_DeviceName_t        FDMADeviceName = "FDMA";
static  STFDMA_InitParams_t    FdmaInitParams[NUMBER_FDMA];
#endif
/* UART Init parameters */

static STUART_InitParams_t  UartInitParams[NUMBER_ASC];
static STPIO_InitParams_t   PioInitParams[NUMBER_PIO];

/* Declarations for memory partitions */

#ifndef ST_OS21

/* Allow room for OS20 segments in internal memory */
#define                 INTERNAL_MEMORY_SIZE        (ST20_INTERNAL_MEMORY_SIZE-1200)
static unsigned char    internal_block [INTERNAL_MEMORY_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( internal_block, "internal_section")
#endif

#define                 SYSTEM_MEMORY_SIZE          0x100000
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
static unsigned char    system_block[SYSTEM_MEMORY_SIZE];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( system_block, "system_section")
#endif

#define                  NCACHE_PARTITION_SIZE     0x800000 /*0x10*/
static unsigned char     ncache_block [NCACHE_PARTITION_SIZE];
static partition_t       ncache_partition;
partition_t              *the_ncache_partition = &ncache_partition;
#ifdef ARCHITECTURE_ST20
#pragma ST_section       (ncache_block,   "ncache_section")
#endif

/* This is to avoid a linker warning */
static unsigned char    internal_block_noinit[1];
#pragma ST_section      ( internal_block_noinit, "internal_section_noinit")

static unsigned char    system_block_noinit[1];
#pragma ST_section      ( system_block_noinit, "system_section_noinit")

#if defined(ST_7710) || defined(ST_5528) || defined(ST_5100) || defined(ST_5105) || defined(ST_5700) \
|| defined(ST_7100) || defined(ST_5301)  || defined(ST_7109) || defined(ST_5188) || defined(ST_5525) \
|| defined(ST_5107) || defined(ST_7200)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif

#else /* if defined ST_OS21 */

#define                 SYSTEM_MEMORY_SIZE          0x100500
#define                 NCACHE_PARTITION_SIZE       0x800000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;

static unsigned char    ncache_block [NCACHE_PARTITION_SIZE];
static partition_t      *the_ncache_partition;
#pragma ST_section      (ncache_block,   "ncache_section")

#endif /* ST_OS21 */

#if defined(ARCHITECTURE_ST40) && !defined(ST_OS21)
BoardInfo *boardInfo;
#endif

/*****************************************************************************
Function prototypes
*****************************************************************************/

/* Test routines */

#if defined(PRODUCER) || defined(CONSUMER) /* Multi-board back-to-back */

static UART_TestResult_t UART_TestProducerConsumer(UART_TestParams_t *);

#elif defined(SOAK)                     /* 24hr soak test */

static UART_TestResult_t UART_TestSoak(UART_TestParams_t *);

#elif defined(TERMINAL)                 /* Terminal mode */

static UART_TestResult_t UART_TestTerminal(UART_TestParams_t *);

#elif defined(INTERACTIVE)              /* Interactive debugging */

static UART_TestResult_t UART_TestErrors(UART_TestParams_t *);

#elif defined(RTSCTS)                   /* RTSCTS flow control */

static UART_TestResult_t UART_TestRTSCTS(UART_TestParams_t *);

#elif defined(XONXOFF) && defined(TEST_TWO_PORTS) /* XONXOFF flow control */

static UART_TestResult_t UART_TestXONXOFF(UART_TestParams_t *);

#elif defined(TEST_MULTITHREADSAFE)

static UART_TestResult_t UART_TestMultithreadsafe(UART_TestParams_t *);

#elif defined(STUFFING)

static UART_TestResult_t UART_TestStuffing(UART_TestParams_t *);

#elif defined(TEST_UARTDMA) && defined(STUART_DMA_SUPPORT)

static UART_TestResult_t UART_TestDMA(UART_TestParams_t *);

#else                                   /* Main tests */

static UART_TestResult_t UART_TestSimple(UART_TestParams_t *);
static UART_TestResult_t UART_TestSwirl(UART_TestParams_t *);
static UART_TestResult_t UART_TestInvalid(UART_TestParams_t *);
static UART_TestResult_t UART_TestMemoryLeak(UART_TestParams_t *);
static UART_TestResult_t UART_TestAbort(UART_TestParams_t *);
static UART_TestResult_t UART_TestFlush(UART_TestParams_t *);
static UART_TestResult_t UART_TestParams(UART_TestParams_t *);
static UART_TestResult_t UART_TestNonBlocking(UART_TestParams_t *);
static UART_TestResult_t UART_TestForceTerm(UART_TestParams_t *);
static UART_TestResult_t UART_TestPause(UART_TestParams_t *);
static UART_TestResult_t UART_PosixReadTest(UART_TestParams_t *);
static UART_TestResult_t UART_TestSmartCardRegConfig(UART_TestParams_t *TestParams);

#if defined(TEST_TWO_PORTS)
static UART_TestResult_t UART_CloseTest(UART_TestParams_t *TestParams);
static UART_TestResult_t UART_TestBackToBack(UART_TestParams_t *);
#endif

static UART_TestResult_t UART_TestPerformance(UART_TestParams_t *);
static UART_TestResult_t UART_TestTx1(UART_TestParams_t *);
static UART_TestResult_t UART_TestTx2(UART_TestParams_t *);
static UART_TestResult_t UART_Test9BitMode(UART_TestParams_t *);

#if defined(TEST_TWO_PORTS)
static UART_TestResult_t UART_TestSmart(UART_TestParams_t *TestParams);
#endif
#endif



/* Test table */
static UART_TestEntry_t UARTTestTable[] =
{

#if defined(PRODUCER) || defined(CONSUMER)

    {
        UART_TestProducerConsumer,
        "Multi-board back-to-back I/O",
        1,
        0                               /* Param not required */
    },

#elif defined(SOAK)
    {
        UART_TestSoak,
        "24hr loopback soak test",
        1,
        ASC_DEVICE_DATA
    },
#elif defined(TERMINAL)
    {
        UART_TestTerminal,
        "Terminal mode",
        1,
        ASC_DEVICE_DATA
    },
#elif defined(INTERACTIVE)
    {
        UART_TestErrors,
        "Interactively test receive errors",
        1,
        ASC_DEVICE_DATA
    },
#elif defined(RTSCTS)
    {
        UART_TestRTSCTS,
        "RTSCTS flow control",
        1,
        ASC_DEVICE_DATA
    },
#elif defined(XONXOFF) && defined(TEST_TWO_PORTS)

    {
        UART_TestXONXOFF,
        "XONXOFF flow control",
        1,
        ASC_DEVICE_DATA
    },
#elif defined(TEST_MULTITHREADSAFE)
    {
        UART_TestMultithreadsafe,
        "2 tasks writing on hyperterm in Blocking Mode",
        1,
        ASC_DEVICE_DATA,
        TRUE
    },
    {
        UART_TestMultithreadsafe,
        "2 tasks writing on hyperterm in Non-Blocking Mode",
        1,
        ASC_DEVICE_DATA,
        FALSE
    },
#elif defined(STUFFING)
    {
        UART_TestStuffing,
        "Send Stuffing when UART becomes idle",
        1,
        ASC_DEVICE_DATA

},
#elif defined(TEST_UARTDMA) && defined(STUART_DMA_SUPPORT)
    {
        UART_TestDMA,
        "high speed transfer using FDMA",
        1,
        ASC_DEVICE_DATA
    },

#else
    {
        UART_TestSimple,
        "A simple run through most of the API routines",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestSwirl,
        "Intensive IO looped back on a single UART",
        1,
        ASC_DEVICE_DATA
    },

#if defined(TEST_TWO_PORTS)
    {
        UART_TestBackToBack,
        "Multiple UARTs on one board",
        1,
        ASC_DEVICE_DATA
    },
#endif

    {
        UART_TestNonBlocking,
        "Test capability to call non-blocking API",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestTx1,
        "Back-to-back 1 byte transmits - blocking",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestTx2,
        "Back-to-back 1 byte transmits - non-blocking",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestInvalid,
        "Test driver with invalid params.",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestMemoryLeak,
        "Test memory leak.",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestAbort,
        "Test capability to abort read/write operations",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestFlush,
        "Test capability to flush FIFOs",
        1,
        ASC_DEVICE_DATA
    },

    {
        UART_TestSmartCardRegConfig,
        "Tests some of the smartcard reg config",
        1,
        0 /* Parameter not used */
    },
#if defined(TEST_TWO_PORTS)
    {
        UART_CloseTest,
        "Test STUART_Close() function",
        1,
        ASC_DEVICE_DATA
    },
#endif
    {
        UART_PosixReadTest,
        "Posix-like read function test",
        1,
        ASC_DEVICE_DATA
    },
#if defined(TEST_TWO_PORTS)
    {
        UART_TestSmart,
        "Test smartcard mode features",
        1,
        0                               /* Parameter not used */
    },
#endif
    {
        UART_Test9BitMode,
        "9 data bits no parity",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestPause,
        "Test capability to pause the driver",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestForceTerm,
        "Forcibly terminates the driver with IO pending",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestPerformance,
        "Timing tests for loopback IO",
        1,
        ASC_DEVICE_DATA
    },
    {
        UART_TestParams,
        "Comms modes testing",
        1,
        ASC_DEVICE_DATA
    },
#ifdef STUART_DEBUG
    {
        UART_TestTimer,
        "Test internal driver timer routines",
        1,
        ASC_DEVICE_DATA
    },
#endif /* STUART_DEBUG */


#endif

    { 0, "", 0, 0 }
};

/* Globally used function prototypes */
void ReadNotifyRoutine(STUART_Handle_t Handle, ST_ErrorCode_t error);
void WriteNotifyRoutine(STUART_Handle_t Handle, ST_ErrorCode_t error);
ST_ErrorCode_t UART_Loopback(U32 Iterations, U32 Blocks,
                             STUART_Handle_t Handle,
                             U32 RxTimeout,
                             U32 TxTimeout
                             );
/* Global errorcodes used in separate tasks */
static ST_ErrorCode_t ReadErrorCode;
static ST_ErrorCode_t WriteErrorCode;
#if defined(UART_MULTITHREADSAFE)
static ST_ErrorCode_t ErrorCode;
#endif

/*****************************************************************************
Main
*****************************************************************************/

/* DeviceConfig()
 *
 * This function performs any necessary top-level configuration
 * for the STUART test harness to function correctly.
 */
void DeviceConfig(void)
{
#if defined(ST_5514)
    /* For the STi5514 there are two registers we must configure to
     * allow the ASC2 to be used for RS232:
     *
     * IRB_ASC_CONTROL => Must be set for ASC2 TXD and RXD to
     *                    bypass the IRDA Data part of IRB module.
     * CFG_CONTROL_C   => Configure the PIO MUX for IRB operation
     *                    and not IrDA Control.
     */
    STSYS_WriteRegDev32LE(((U32)ST5514_IRB_BASE_ADDRESS + 0xD0), 1);
    STSYS_WriteRegDev32LE(((U32)ST5514_CFG_BASE_ADDRESS + 0x08), 0x10);

    /* akem added:
       CFG_CONTROL_D   => Permit ASC2 to send data to IRB, and wakeup
                          IRB with signal on PIO4 bit 3. */

    STSYS_WriteRegDev32LE(((U32)ST5514_CFG_BASE_ADDRESS + 0x0C), ((STSYS_ReadRegDev32LE(
        ST5514_CFG_BASE_ADDRESS + 0x0C)) |  0x1800000));


#elif defined(ST_5516) || defined(ST_5517)

    /* CONFIG REG E: Bit 16 (CONFIG_IRB_UART2RXD_BYPASS) 0: IRB to UART2RXD 1: PIO4[3] to UART2 directly */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_E, STSYS_ReadRegDev32LE(CONFIG_CONTROL_E) | 0x00010000 );

    /* [15] CONFIG_IRDACTRL_DIN_DISABLE Always set to 1*/
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_E, STSYS_ReadRegDev32LE(CONFIG_CONTROL_E) | 0x00008000 );

    /* [4] CONFIG_OTHER_ALT_IRD_NOTIRC_DOUT Always set this bit to 1*/
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_C, STSYS_ReadRegDev32LE(CONFIG_CONTROL_C) | 0x10 );

    /* wakeup enables */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_D, STSYS_ReadRegDev32LE(CONFIG_CONTROL_D) | 0x1800000 );

    /* CFG_CONTROL_D   => Select UART4 instead of MAFE on PIO2 */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_D, STSYS_ReadRegDev32LE(CONFIG_CONTROL_D) | 0x100000 );

    /* Bypass IrDA Control to allow ASC TX output */
    STSYS_WriteRegDev16LE(0x201150d0, 0x01);

#elif defined(ST_5100)

    STSYS_WriteRegDev32LE(CONFIG_CONTROL_D,STSYS_ReadRegDev32LE(CONFIG_CONTROL_D) | 0x01F00000 );

    /* For ASC0 & ASC1 */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_F,STSYS_ReadRegDev32LE(CONFIG_CONTROL_F) | 0x00303000 );

    /* For ASC2 */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, 0 );

    /* For ASC3 using PIO4 also set wakeup bit */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, 0 );

#elif defined(ST_5105) || defined(ST_5107)

    /*for using ASC0 instead of Smartcard */
   STSYS_WriteRegDev32LE(CONFIG_CONTROL_H,STSYS_ReadRegDev32LE(CONFIG_CONTROL_H) | 0x02000000 );

#elif defined(ST_8010)

    /* For enabling TX on ASC2 */
    STSYS_WriteRegDev32LE(0x5000301C,0x00000800 );

#elif defined (MEDIAREF)
    /* akem added:
       CFG_CONTROL_D   => Select UART4 instead of MAFE on PIO2 */
    STSYS_WriteRegDev32LE(((U32)ST5514_CFG_BASE_ADDRESS + 0x0C), ((STSYS_ReadRegDev32LE(
                            ST5514_CFG_BASE_ADDRESS + 0x0C)) |  0x100000));
#elif defined(ST_7200)
    /*for using ASC3 TXD pin*/
   STSYS_WriteRegDev32LE(SYSTEM_CONFIG_7,STSYS_ReadRegDev32LE(SYSTEM_CONFIG_7) | 0x00004000 );
#endif
}

void initialize_partitions(void)
{

#ifdef ST_OS21
    system_partition = partition_create_heap((U8*)external_block, sizeof(external_block));

#if defined(STUART_DMA_SUPPORT)
    the_ncache_partition = partition_create_heap(
                                    (void *)ST40_NOCACHE_NOTRANSLATE(ncache_block),
                                     sizeof(ncache_block));
#endif/*STUART_DMA_SUPPORT*/

#else

    partition_init_simple(&the_internal_partition,
                          (unsigned char*) internal_block,
                          INTERNAL_MEMORY_SIZE);
    partition_init_heap(&the_system_partition,
                        (unsigned char*)system_block,
                        SYSTEM_MEMORY_SIZE);

    partition_init_heap (&ncache_partition,ncache_block,
                         sizeof(ncache_block));


    internal_block_noinit[0] = 0;
    system_block_noinit[0]   = 0;

#if defined(ST_7710) || defined(ST_5528) || defined(ST_5100) || defined(ST_5105) || defined(ST_5700)\
|| defined(ST_7100) || defined(ST_5301)  || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)\
|| defined(ST_5107) || defined(ST_7200)

    data_section[0]          = 0;
#endif

#endif /* ST_OS21 */
}

void os20_main(void *ptr)
{
    UART_TestResult_t Total = TEST_RESULT_ZERO, Result;
    UART_TestEntry_t *Test_p;
    U32 Section = 1;
    UART_TestParams_t TestParams;
#ifdef STTBX_PRINT
    STTBX_InitParams_t TBXInitParams;
#endif

    STBOOT_InitParams_t BootParams;

#ifndef DISABLE_DCACHE

#if defined CACHEABLE_BASE_ADDRESS
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        {(U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS}, /* assumed ok */
        { NULL, NULL }
    };
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        {(U32 *)0x40200000, (U32 *)0x407FFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5528)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        {(U32 *) 0x407FFFFF, (U32 *) 0x7FFFFFFF},    /* Cacheable */
        { NULL, NULL}                                 /* End */
    };
#else
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        {(U32 *)0x40080000, (U32 *)0x7FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#endif
#endif

    U32 error, i;
#ifdef CODETEST
    /* Codetest initialization */
    codetest_init();
#endif

    /* Create memory partitions */
    initialize_partitions();

#if defined DISABLE_ICACHE
    BootParams.ICacheEnabled             = FALSE;
#else
    BootParams.ICacheEnabled             = TRUE;
#endif

#if defined DISABLE_DCACHE
    BootParams.DCacheMap                 = NULL;
#else
    BootParams.DCacheMap                 = DCacheMap;
#endif

    BootParams.CacheBaseAddress = (U32 *)CACHE_BASE_ADDRESS;
    BootParams.SDRAMFrequency = SDRAM_FREQUENCY;
    BootParams.DCacheMap = NULL;
    BootParams.BackendType.DeviceType = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
#ifdef ST_OS21
    BootParams.TimeslicingEnabled = TRUE;
#endif

    /* Constant, defined in include; set automatically for board configuration */
    BootParams.MemorySize = (STBOOT_DramMemorySize_t)SDRAM_SIZE;

    /* Initialize the boot driver */
    error = STBOOT_Init( "boot", &BootParams );
    if (error != ST_NO_ERROR)
    {
        printf("Unable to intialize boot - %d\n", error);
        return;
    }

#ifdef STTBX_PRINT
    /* Initialize the toolbox */
    TBXInitParams.SupportedDevices    = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;
    TBXInitParams.CPUPartition_p      = system_partition;
    STTBX_Init( "tbx", &TBXInitParams );
#endif

    /* Setup clock info */
    ST_GetClockInfo(&ClockInfo);

    /* Setup PIO global parameters */
    PioInitParams[0].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[0].BaseAddress     = (U32 *)PIO_0_BASE_ADDRESS;
    PioInitParams[0].InterruptNumber = PIO_0_INTERRUPT;
    PioInitParams[0].InterruptLevel  = PIO_0_INTERRUPT_LEVEL;

    PioInitParams[1].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[1].BaseAddress     = (U32 *)PIO_1_BASE_ADDRESS;
    PioInitParams[1].InterruptNumber = PIO_1_INTERRUPT;
    PioInitParams[1].InterruptLevel  = PIO_1_INTERRUPT_LEVEL;

    PioInitParams[2].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[2].BaseAddress     = (U32 *)PIO_2_BASE_ADDRESS;
    PioInitParams[2].InterruptNumber = PIO_2_INTERRUPT;
    PioInitParams[2].InterruptLevel  = PIO_2_INTERRUPT_LEVEL;

    PioInitParams[3].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[3].BaseAddress     = (U32 *)PIO_3_BASE_ADDRESS;
    PioInitParams[3].InterruptNumber = PIO_3_INTERRUPT;
    PioInitParams[3].InterruptLevel  = PIO_3_INTERRUPT_LEVEL;

#if (NUMBER_PIO > 4)
    PioInitParams[4].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[4].BaseAddress     = (U32 *)PIO_4_BASE_ADDRESS;
    PioInitParams[4].InterruptNumber = PIO_4_INTERRUPT;
    PioInitParams[4].InterruptLevel  = PIO_4_INTERRUPT_LEVEL;
#endif

#if (NUMBER_PIO > 5)
    PioInitParams[5].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[5].BaseAddress     = (U32 *)PIO_5_BASE_ADDRESS;
    PioInitParams[5].InterruptNumber = PIO_5_INTERRUPT;
    PioInitParams[5].InterruptLevel  = PIO_5_INTERRUPT_LEVEL;
#endif

#if (NUMBER_PIO > 6)
    PioInitParams[6].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[6].BaseAddress     = (U32 *)PIO_6_BASE_ADDRESS;
    PioInitParams[6].InterruptNumber = PIO_6_INTERRUPT;
    PioInitParams[6].InterruptLevel  = PIO_6_INTERRUPT_LEVEL;
#endif

#if (NUMBER_PIO > 7)
    PioInitParams[7].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[7].BaseAddress     = (U32 *)PIO_7_BASE_ADDRESS;
    PioInitParams[7].InterruptNumber = PIO_7_INTERRUPT;
    PioInitParams[7].InterruptLevel  = PIO_7_INTERRUPT_LEVEL;
#endif

#if (NUMBER_PIO > 8)
    PioInitParams[8].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[8].BaseAddress     = (U32 *)PIO_8_BASE_ADDRESS;
    PioInitParams[8].InterruptNumber = PIO_8_INTERRUPT;
    PioInitParams[8].InterruptLevel  = PIO_8_INTERRUPT_LEVEL;
#endif

    /* Initialize all pio devices */
    UART_DebugMessage("Initializing pio devices...");
    for (i = 0; i < NUMBER_PIO; i++)
    {
        error = STPIO_Init(PioDeviceName[i], &PioInitParams[i]);
        UART_DebugError("STPIO_Init()", error);
    }
#if defined(STUART_DMA_SUPPORT)
     /* Initialize fdma device */
    FdmaInitParams[0].DeviceType            = STFDMA_DEVICE_FDMA_2;
    FdmaInitParams[0].DriverPartition_p     =(ST_Partition_t *)system_partition;
    FdmaInitParams[0].NCachePartition_p     =(ST_Partition_t *)the_ncache_partition;
    FdmaInitParams[0].BaseAddress_p         = (U32 *)FDMA_BASE_ADDRESS;
    FdmaInitParams[0].InterruptNumber       = FDMA_INTERRUPT_NUMBER;
    FdmaInitParams[0].InterruptLevel        = FDMA_INTERRUPT_LEVEL;        /* Random */
    FdmaInitParams[0].NumberCallbackTasks   = 1;
    FdmaInitParams[0].ClockTicksPerSecond   = ST_GetClocksPerSecond();
    FdmaInitParams[0].FDMABlock             = STFDMA_1;

    UART_DebugMessage("Initializing fdma device...");

    for (i = 0; i < NUMBER_FDMA; i++)
    {
        error = STFDMA_Init(FDMADeviceName, &FdmaInitParams[i]);
        UART_DebugError("STFDMA_Init()", error);
    }
#endif/*STUART_DMA_SUPPORT*/
    /* Global setup of uart init. params */
    for (i = 0; i < NUMBER_ASC; i++)
    {
        /* Zero port definitions */
        UartInitParams[i].RXD.PortName[0] = '\0';
        UartInitParams[i].TXD.PortName[0] = '\0';
        UartInitParams[i].RTS.PortName[0] = '\0';
        UartInitParams[i].CTS.PortName[0] = '\0';
        UartInitParams[i].RXD.PortName[0] = '\0';
        UartInitParams[i].TXD.PortName[0] = '\0';
    }
       /* Set-up init params for each UART */
    UartInitParams[0].UARTType        = ASC_DEVICE_TYPE;
    UartInitParams[0].DriverPartition = (ST_Partition_t *)system_partition;
#ifdef STUART_DMA_SUPPORT
    UartInitParams[0].NCachePartition =  NULL;
#endif
    UartInitParams[0].BaseAddress     = (U32 *)ASC_0_BASE_ADDRESS;
    UartInitParams[0].InterruptNumber = ASC_0_INTERRUPT;
    UartInitParams[0].InterruptLevel  = ASC_0_INTERRUPT_LEVEL;
    UartInitParams[0].ClockFrequency  = UART_CLOCK;
    UartInitParams[0].SwFIFOEnable    = TRUE;
    UartInitParams[0].FIFOLength      = 4096;
    strcpy(UartInitParams[0].RXD.PortName, PioDeviceName[ASC_0_RXD_DEV]);
    UartInitParams[0].RXD.BitMask     = ASC_0_RXD_BIT;
    strcpy(UartInitParams[0].TXD.PortName, PioDeviceName[ASC_0_TXD_DEV]);
    UartInitParams[0].TXD.BitMask     = ASC_0_TXD_BIT;
    strcpy(UartInitParams[0].RTS.PortName, PioDeviceName[ASC_0_RTS_DEV]);
    UartInitParams[0].RTS.BitMask     = ASC_0_RTS_BIT;
    strcpy(UartInitParams[0].CTS.PortName, PioDeviceName[ASC_0_CTS_DEV]);
    UartInitParams[0].CTS.BitMask     = ASC_0_CTS_BIT;
    UartInitParams[0].DefaultParams   = NULL;
#ifdef STUART_DMA_SUPPORT
    UartInitParams[0].RXDMARequestSignal = STFDMA_REQUEST_SIGNAL_NONE;
    UartInitParams[0].TXDMARequestSignal = STFDMA_REQUEST_SIGNAL_NONE;
#endif

#if (NUMBER_ASC > 1)

    UartInitParams[1].UARTType        = ASC_DEVICE_TYPE;
    UartInitParams[1].DriverPartition = (ST_Partition_t *)system_partition;
#ifdef STUART_DMA_SUPPORT
    UartInitParams[1].NCachePartition =  NULL;
#endif
    UartInitParams[1].BaseAddress     = (U32 *)ASC_1_BASE_ADDRESS;
    UartInitParams[1].InterruptNumber = ASC_1_INTERRUPT;
    UartInitParams[1].InterruptLevel  = ASC_1_INTERRUPT_LEVEL;
    UartInitParams[1].ClockFrequency  = UART_CLOCK;
    UartInitParams[1].SwFIFOEnable    = TRUE;
    UartInitParams[1].FIFOLength      = 4096;
    strcpy(UartInitParams[1].RXD.PortName, PioDeviceName[ASC_1_RXD_DEV]);
    UartInitParams[1].RXD.BitMask     = ASC_1_RXD_BIT;
    strcpy(UartInitParams[1].TXD.PortName, PioDeviceName[ASC_1_TXD_DEV]);
    UartInitParams[1].TXD.BitMask     = ASC_1_TXD_BIT;
    strcpy(UartInitParams[1].RTS.PortName, PioDeviceName[ASC_1_RTS_DEV]);
    UartInitParams[1].RTS.BitMask     = ASC_1_RTS_BIT;
    strcpy(UartInitParams[1].CTS.PortName, PioDeviceName[ASC_1_CTS_DEV]);
    UartInitParams[1].CTS.BitMask     = ASC_1_CTS_BIT;
    UartInitParams[1].DefaultParams   = NULL;
#ifdef STUART_DMA_SUPPORT
    UartInitParams[1].RXDMARequestSignal = STFDMA_REQUEST_SIGNAL_NONE;
    UartInitParams[1].TXDMARequestSignal = STFDMA_REQUEST_SIGNAL_NONE;
#endif
#endif

#if (NUMBER_ASC > 2)

    UartInitParams[2].UARTType        = ASC_DEVICE_TYPE;
    UartInitParams[2].DriverPartition = (ST_Partition_t *)system_partition;
#ifdef STUART_DMA_SUPPORT
    UartInitParams[2].NCachePartition =  NULL;
#endif
    UartInitParams[2].BaseAddress     = (U32 *)ASC_2_BASE_ADDRESS;
    UartInitParams[2].InterruptNumber = ASC_2_INTERRUPT;
    UartInitParams[2].InterruptLevel  = ASC_2_INTERRUPT_LEVEL;
    UartInitParams[2].ClockFrequency  = UART_CLOCK;
    UartInitParams[2].SwFIFOEnable    = TRUE;
    UartInitParams[2].FIFOLength      = 4096;
    strcpy(UartInitParams[2].RXD.PortName, PioDeviceName[ASC_2_RXD_DEV]);
    UartInitParams[2].RXD.BitMask     = ASC_2_RXD_BIT;
    strcpy(UartInitParams[2].TXD.PortName, PioDeviceName[ASC_2_TXD_DEV]);
    UartInitParams[2].TXD.BitMask     = ASC_2_TXD_BIT;
    strcpy(UartInitParams[2].RTS.PortName, PioDeviceName[ASC_2_RTS_DEV]);
    UartInitParams[2].RTS.BitMask     = ASC_2_RTS_BIT;
    strcpy(UartInitParams[2].CTS.PortName, PioDeviceName[ASC_2_CTS_DEV]);
    UartInitParams[2].CTS.BitMask     = ASC_2_CTS_BIT;
    UartInitParams[2].DefaultParams   = NULL;
#ifdef STUART_DMA_SUPPORT
    UartInitParams[2].RXDMARequestSignal = STFDMA_REQUEST_SIGNAL_NONE;
    UartInitParams[2].TXDMARequestSignal = STFDMA_REQUEST_SIGNAL_NONE;
#endif
#endif

#if (NUMBER_ASC > 3)

    UartInitParams[3].UARTType        = ASC_DEVICE_TYPE;
    UartInitParams[3].DriverPartition = (ST_Partition_t *)system_partition;
#ifdef STUART_DMA_SUPPORT
    UartInitParams[3].NCachePartition =  NULL;
#endif
    UartInitParams[3].BaseAddress     = (U32 *)ASC_3_BASE_ADDRESS;
    UartInitParams[3].InterruptNumber = ASC_3_INTERRUPT;
    UartInitParams[3].InterruptLevel  = ASC_3_INTERRUPT_LEVEL;
    UartInitParams[3].ClockFrequency  = UART_CLOCK;
    UartInitParams[3].SwFIFOEnable    = TRUE;
    UartInitParams[3].FIFOLength      = 4096;
    strcpy(UartInitParams[3].RXD.PortName, PioDeviceName[ASC_3_RXD_DEV]);
    UartInitParams[3].RXD.BitMask     = ASC_3_RXD_BIT;
    strcpy(UartInitParams[3].TXD.PortName, PioDeviceName[ASC_3_TXD_DEV]);
    UartInitParams[3].TXD.BitMask     = ASC_3_TXD_BIT;
    strcpy(UartInitParams[3].RTS.PortName, PioDeviceName[ASC_3_RTS_DEV]);
    UartInitParams[3].RTS.BitMask     = ASC_3_RTS_BIT;
    strcpy(UartInitParams[3].CTS.PortName, PioDeviceName[ASC_3_CTS_DEV]);
    UartInitParams[3].CTS.BitMask     = ASC_3_CTS_BIT;
    UartInitParams[3].DefaultParams   = NULL;
#ifdef  STUART_DMA_SUPPORT
    UartInitParams[3].RXDMARequestSignal = STFDMA_REQUEST_SIGNAL_NONE;
    UartInitParams[3].TXDMARequestSignal = STFDMA_REQUEST_SIGNAL_NONE;
#endif
#endif

#if (NUMBER_ASC > 4)

    UartInitParams[4].UARTType        = ASC_DEVICE_TYPE;
    UartInitParams[4].DriverPartition = (ST_Partition_t *)system_partition;
#ifdef STUART_DMA_SUPPORT
    UartInitParams[4].NCachePartition =  NULL;
#endif
    UartInitParams[4].BaseAddress     = (U32 *)ASC_4_BASE_ADDRESS;
    UartInitParams[4].InterruptNumber = ASC_4_INTERRUPT;
    UartInitParams[4].InterruptLevel  = ASC_4_INTERRUPT_LEVEL;
    UartInitParams[4].ClockFrequency  = UART_CLOCK;
    UartInitParams[4].SwFIFOEnable    = TRUE;
    UartInitParams[4].FIFOLength      = 4096;
    strcpy(UartInitParams[4].RXD.PortName, PioDeviceName[ASC_4_RXD_DEV]);
    UartInitParams[4].RXD.BitMask     = ASC_4_RXD_BIT;
    strcpy(UartInitParams[4].TXD.PortName, PioDeviceName[ASC_4_TXD_DEV]);
    UartInitParams[4].TXD.BitMask     = ASC_4_TXD_BIT;
    strcpy(UartInitParams[4].RTS.PortName, PioDeviceName[ASC_4_RTS_DEV]);
    UartInitParams[4].RTS.BitMask     = ASC_4_RTS_BIT;
    strcpy(UartInitParams[4].CTS.PortName, PioDeviceName[ASC_4_CTS_DEV]);
    UartInitParams[4].CTS.BitMask     = ASC_4_CTS_BIT;
    UartInitParams[4].DefaultParams   = NULL;
#ifdef STUART_DMA_SUPPORT
    UartInitParams[4].RXDMARequestSignal = STFDMA_REQUEST_SIGNAL_NONE;
    UartInitParams[4].TXDMARequestSignal = STFDMA_REQUEST_SIGNAL_NONE
#endif
#endif


    /* Perform any device-specific top-level configuration */
    DeviceConfig();

    UART_DebugMessage("**************************************************");
    UART_DebugMessage("                STUART Test Harness               ");
    UART_DebugPrintf(("Driver Revision: %s\n", STUART_GetRevision()));
    UART_DebugPrintf(("Test Harness Revision: %s\n", Revision));
    UART_DebugPrintf(("Clock Speed: %d\n", CPU_CLOCK));
    UART_DebugPrintf(("Clock/sec: %d\n", ST_GetClocksPerSecond()));
    UART_DebugPrintf(("Comms Speed: %d\n", UART_CLOCK));

    Test_p = UARTTestTable;

    while (Test_p->TestFunction != NULL)
    {
        while (Test_p->RepeatCount > 0)
        {
            UART_DebugMessage("**************************************************");
            UART_DebugPrintf(("SECTION %d - %s\n", Section,
                              Test_p->TestInfo));
            UART_DebugMessage("**************************************************");

            /* Set up parameters for test function */
            TestParams.Ref = Section;
            TestParams.ASCNumber = Test_p->ASCNumber;
#if defined(TEST_MULTITHREADSAFE)
            TestParams.IsBlocking = Test_p->IsBlocking;
#endif
            /* Call test */
            Result = Test_p->TestFunction(&TestParams);

            /* Update counters */
            Total.NumberPassed += Result.NumberPassed;
            Total.NumberFailed += Result.NumberFailed;
            Test_p->RepeatCount--;
        }

        Test_p++;
        Section++;
    }

    /* Output running analysis */
    UART_DebugMessage("**************************************************");
    UART_DebugPrintf(("PASSED: %d\n", Total.NumberPassed));
    UART_DebugPrintf(("FAILED: %d\n", Total.NumberFailed));
    UART_DebugMessage("**************************************************");
}

#if defined(ST_GX1) || defined(ST_NGX1)

#define SYSCONF_BASE            0xFB190000
#define SYSCONF_SYS_CON1        (SYSCONF_BASE + 0x10)
#define SYSCONF_SYS_CON2        (SYSCONF_BASE + 0x18)
#define SYS_CONF1_LSW           SYSCONF_SYS_CON1
#define SYS_CONF1_MSW           (SYSCONF_SYS_CON1 + 0x04)
#define DEV_WRITE_U32(x, y)     *(volatile U32 *)x = (U32)y

#endif

int main(int argc, char *argv[])
{

#if defined(ARCHITECTURE_ST40) && !defined(ST_OS21)
    boardInfo = (BoardInfo*)chorusContext->ctxBoardInfo;
    setbuf(stdout, NULL);

    printf( "Peripheral bus speed     = %10d Hz (%d MHz)\n",
            boardInfo->peripheralClockFrequency,
            boardInfo->peripheralClockFrequency / 1000000 );

    printf( "System clock frequency   = %10d Hz (%d MHz)\n",
            boardInfo->systemClockFrequency,
            boardInfo->systemClockFrequency / 1000000 );

    printf( "CPU core clock frequency = %10d Hz (%d MHz)\n",
            boardInfo->coreClockFrequency,
            boardInfo->coreClockFrequency / 1000000 );

    /* Configure GX1 bridge (should be added to STBOOT) */
#if defined(ST_GX1) || defined(ST_NGX1)

    /* Reset GX1 comms bridge */
    DEV_WRITE_U32(SYS_CONF1_LSW, (1<<25) );
    DEV_WRITE_U32(SYS_CONF1_MSW, (1<<0) );
    DEV_WRITE_U32(SYS_CONF1_LSW, ((U32)((U32)1<<25)|(U32)((U32)1<<31)));
    DEV_WRITE_U32(SYS_CONF1_MSW, ((1<<0)|(1<<6)) );
    DEV_WRITE_U32(SYS_CONF1_LSW, (1<<25) );
    DEV_WRITE_U32(SYS_CONF1_MSW, (1<<0) );

    /* ASC0 does not use PIO.  The pins are shared and must be
     * configured in the SYSCONF area to enable them for ASC0
     * usage.  Note also that the MB317 board allows COM3 port
     * to be connected to either ASC0 or ASC1 -- the test
     * harness assumes ASC0 is used (default).
     */
    /*DEV_WRITE_U32(SYSCONF_SYS_CON2, (1<<27)); - for ASC1 */
    DEV_WRITE_U32(SYSCONF_SYS_CON2, (1<<23));

#endif

    OS20_main(argc, argv, os20_main);

#else
    os20_main(NULL);
#endif

    exit(0);
}

/****************************************************************************
* Loopback transfer function
****************************************************************************/
ST_ErrorCode_t UART_Loopback(U32 Iterations, U32 Blocks,
                             STUART_Handle_t Handle,
                             U32 RxTimeout,
                             U32 TxTimeout
                             )
{
    U32 i, j;
    U32 NumberRead;
    U32 NumberWritten;
    U8 *RxBuf;
    char *SwirlBuf;
    ST_ErrorCode_t error = ST_NO_ERROR;

    RxBuf    = STOS_MemoryAllocate(system_partition, 8*1024);
    SwirlBuf = STOS_MemoryAllocate(system_partition, 8*1024);

    if (RxBuf == NULL || SwirlBuf == NULL)
        goto loop_end;

    /* Build swirl buffer */
    for (j = 0; j < Blocks;)
    {
        for (i = ' '; i <= 'z'; i++)
        {
            SwirlBuf[i-' '] = i;
        }
        j += (i - ' ');
    }

    for (i = 0; i < Iterations; i++)
    {
        /* Try a read/write cycle */
        error = STUART_Write(Handle,
                             (U8 *)SwirlBuf,
                             Blocks,
                             &NumberWritten,
                             TxTimeout
                            );

        if ((error != ST_NO_ERROR) || (NumberWritten != Blocks))
        {
            UART_DebugError("STUART_Write()", error);
            goto loop_end;
        }

        error = STUART_Read(Handle,
                            RxBuf,
                            Blocks,
                            &NumberRead,
                            RxTimeout
                           );

        if ((error != ST_NO_ERROR) || (NumberRead != Blocks))
        {
            UART_DebugError("STUART_Read()", error);
            UART_DebugPrintf(("Iterations = %d, NumberRead = %d\n",
                              i, NumberRead));
            goto loop_end;
        }

        if (strncmp((char *)RxBuf,
                    (char *)SwirlBuf,
                    Blocks) != 0)
        {
            UART_DebugPrintf(("Receive error = '%s'.\n", RxBuf));
            goto loop_end;
        }

#if TEST_VERBOSE
        STTBX_Print(("%s",RxBuf));
#endif

    }

#if TEST_VERBOSE
    STTBX_Print(("\n"));
#endif

loop_end:
    if (RxBuf)
    {
        STOS_MemoryDeallocate(system_partition, RxBuf);
    }
    if (SwirlBuf)
    {
        STOS_MemoryDeallocate(system_partition, SwirlBuf);
    }

    return error;
}

/*****************************************************************************
 * Loopback that includes a small delay, to see if the XON-XOFF flow control
 * stops over-runs.
 *****************************************************************************/
ST_ErrorCode_t UART_Loopback_xon(U32 Iterations, U32 Blocks,
                                 STUART_Handle_t Handle,
                                 U32 RxTimeout,
                                 U32 TxTimeout
                                )
{
    U32 i, j;
    U32 NumberRead;
    U32 NumberWritten;
    U8 RxBuf[256] = { 0 };
    char SwirlBuf[256] = { 0 };
    ST_ErrorCode_t error;

    /* Build swirl buffer */
    for (i = ' '; i <= 'z'; i++)
    {
        SwirlBuf[i-' '] = i;
    }

    for (i = 0; i < Iterations; i++)
    {
        for (j = 0; j < strlen(SwirlBuf); j+=Blocks)
        {
            /* Try a read/write cycle */
            error = STUART_Write(Handle,
                                 (U8 *)&SwirlBuf[j],
                                 min(Blocks,(strlen(SwirlBuf)-j)),
                                 &NumberWritten,
                                 TxTimeout
                                );

            if (error != ST_NO_ERROR)
            {
                UART_DebugError("STUART_Write()", error);
                goto loop_xon_end;
            }

            /* delay a little bit, to see if the XON/XOFF stuff kicks in */
            STOS_TaskDelay(10*TIME_DELAY);

            error = STUART_Read(Handle,
                                RxBuf,
                                min(Blocks,(strlen(SwirlBuf)-j)),
                                &NumberRead,
                                RxTimeout
                               );

            if (strncmp((char *)RxBuf,
                        (char *)&SwirlBuf[j],
                        min(Blocks,(strlen(SwirlBuf)-j))) != 0)
            {
                UART_DebugPrintf(("Receive error = '%s'.\n", RxBuf));
                return ST_NO_ERROR;
            }

            if (error != ST_NO_ERROR)
            {
                UART_DebugError("STUART_Read()", error);
                goto loop_xon_end;
            }
        }
#if TEST_VERBOSE
        STTBX_Print(("%s",RxBuf));
#endif
    }
#if TEST_VERBOSE
    STTBX_Print(("\n"));
#endif

loop_xon_end:
    return error;
}

/****************************************************************************
* Producer-Consumer test mode
****************************************************************************/

#if defined(PRODUCER) || defined(CONSUMER)

static void UART_ProducerConsumerTask(UART_TaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup PIO */
#ifdef PRODUCER
        ASCNumber = ASC_DEVICE_DATA;
#else
        ASCNumber = ASC_DEVICE_MODEM;
#endif /* PRODUCER */

        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
            goto pc_end;
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_NO_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_2_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 9600;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_NO_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_2_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 9600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
            goto pc_end;
    }

    /* Timeout Params */
#if defined(ST_5528)
    RxTimeout = 100000;
    TxTimeout = 10000;
#else
    RxTimeout = 10000;
    TxTimeout = 1000;
#endif

    UART_DebugMessage("Starting multi-board test...");

    while(Params_p->ticks > 0)
    {
        U32 NumberWritten;
        U32 NumberRead;

#ifdef PRODUCER
        char TestMessage[] = "0123456789ABCDEF0123456789ABCDEF";
        char Ack[12] = {0};

        /* Try a read/write cycle */
        error = STUART_Write(Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );

        if (error != ST_NO_ERROR)
        {
            UART_DebugError("STUART_Write()", error);
            break;
        }

        /* Since we have no flow control, the only way of ensuring
         * it's safe to send the next block of data is by waiting
         * for an acknowledge string.
         */

        error = STUART_Read(Handle,
                            (U8 *)Ack,
                            3,
                            &NumberRead,
                            RxTimeout
                           );

        if (error != ST_NO_ERROR ||
            NumberRead != 3 || strcmp("ACK",Ack) != 0)
        {
            UART_DebugError("No ACK received.", error);
            break;
        }

#else
        char TestMessage[35] = {0};

        /* Try a read/write cycle */
        error = STUART_Read(Handle,
                            (U8 *)TestMessage,
                            32,
                            &NumberRead,
                            RxTimeout
                           );
        if (error != ST_NO_ERROR)
        {
            UART_DebugError("STUART_Read()", error);
            break;
        }

        if (strcmp(TestMessage, "0123456789ABCDEF0123456789ABCDEF") != 0)
        {
            UART_DebugPrintf(("Bad data = '%s'\n", TestMessage));
            break;
        }

        error = STUART_Write(Handle,
                             (U8 *)"ACK",
                             3,
                             &NumberWritten,
                             TxTimeout
                            );

        if (error != ST_NO_ERROR)
        {
            UART_DebugError("STUART_Write()", error);
            break;
        }

#endif /* PRODUCER */
        Params_p->ticks--;
        if ((Params_p->ticks%1000) == 0)
        {
            UART_DebugPrintf(("%d iterations remaining.\n", Params_p->ticks));
        }
    }

    /* Get final statistics */
    Params_p->Error = error;
#ifdef STUART_STATISTICS
    error = STUART_GetStatistics(Handle, Params_p->Statistics_p);
    UART_DebugError("STUART_GetStatistics()", error);
#endif

    /* Tidy up */
pc_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }
}


static UART_TestResult_t UART_TestProducerConsumer(UART_TestParams_t *TestParams)
{
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    BOOL Error;
#ifdef STUART_STATISTICS
    STUART_Statistics_t Stats;
#endif
    UART_TaskParams_t Params;
    Params.Driverpartition = (ST_Partition_t *)system_partition;
	
    /* Create tasks */
    Params.ticks = 1000;
#ifdef STUART_STATISTICS
    Params.Statistics_p = &Stats;
#endif

    Error =  STOS_TaskCreate   ((void(*)(void *))UART_ProducerConsumerTask,
									  			&Params,
									  			Params.Driverpartition,
									  			STACK_SIZE,
									  			NULL,
									  			Params.Driverpartition,
									  			&Params.Task_p,
									  			NULL, 
									  			MAX_USER_PRIORITY,
									  			"PCTask", 
									  			(task_flags_t)0);

     /* Ensure task was created ok */
    if (Error  == ST_NO_ERROR)
    {
        /* Wait for each task before deleting it */
        if (STOS_TaskWait(&Params.Task_p, TIMEOUT_INFINITY) == 0)
        {
            /* Now it should be safe to delete the tasks */
            STOS_TaskDelete ( Params.Task_p,
						  Params.Driverpartition, 
						  NULL,
						  Params.Driverpartition);
        }

#ifdef STUART_STATISTICS
        UART_DebugPrintf(("NumberBytesReceived = %d\n", Stats.NumberBytesReceived));
        UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Stats.NumberBytesTransmitted));
        UART_DebugPrintf(("NumberOverrunErrors = %d\n", Stats.NumberOverrunErrors));
        UART_DebugPrintf(("NumberFramingErrors = %d\n", Stats.NumberFramingErrors));
        UART_DebugPrintf(("NumberParityErrors = %d\n", Stats.NumberParityErrors));
#endif /* STUART_STATISTICS */

        if (Params.Error == ST_NO_ERROR)
        {
            UART_TestPassed(Result);
        }
        else
        {
            UART_DebugError("PCTask", Params.Error);
            UART_TestFailed(Result, "Error during processing");
        }
    }
    else
    {
        UART_TestFailed(Result, "Unable to create task");
    }

    return Result;
}

#elif defined(SOAK)

/****************************************************************************
* Soak test mode
****************************************************************************/

static UART_TestResult_t UART_TestSoak(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* STUART_Init() */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto soak_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 115200;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 115200;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto soak_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 1000;


    UART_DebugPrintf(("%d.0: Soak test\n", TestParams->Ref));
    UART_DebugMessage("Purpose: 24hr heavy driver load.");
    {

#ifdef ARCHITECTURE_ST200
        U32 ElapsedClock, StartClock, NextClock, EndClock;
#else
        clock_t ElapsedClock, StartClock, NextClock, EndClock;
#endif

        StartClock = STOS_time_now();
        EndClock = StartClock + (24*60*60*TICKS_PER_SECOND);
        NextClock = StartClock + (60*60*TICKS_PER_SECOND);

        /* Must run for at least 24hrs */
        UART_DebugMessage("Started soak test...");
        do
        {
            /* Perform loopback and data check */
            UART_Loopback(255, 1500, Handle, RxTimeout, TxTimeout);

            ElapsedClock = STOS_time_minus(STOS_time_now(), StartClock);

            if (ElapsedClock >= NextClock)
            {
                UART_DebugPrintf(("%d hours passed.\n",
                                  (U16)(NextClock/(60*60*TICKS_PER_SECOND))));
                NextClock += (60*60*TICKS_PER_SECOND);

#ifdef STUART_STATISTICS
                /* STUART_GetStatistics() */
                {
                    STUART_Statistics_t Statistics;

                    error = STUART_GetStatistics(Handle, &Statistics);
                    UART_DebugError("STUART_GetStatistics()", error);
                    UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
                    UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
                    UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
                    UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
                    UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));

                    /* Check tx bytes == rx bytes */
                    if (Statistics.NumberBytesReceived !=
                        Statistics.NumberBytesTransmitted)
                    {
                        UART_TestFailed(Result, "Failed to read all transmitted bytes");
                    }
                    else
                    {
                        UART_TestPassed(Result);
                    }
                }
#endif /* STUART_STATISTICS */
            }
        } while (ElapsedClock <= EndClock);
    }

soak_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;
}

#elif defined(TERMINAL)

/****************************************************************************
* Terminal mode
****************************************************************************/

static UART_TestResult_t UART_TestTerminal(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);

        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto terminal_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
#ifdef UARTTST_HWFLOW
        UartDefaultParams.RxMode.FlowControl = STUART_RTSCTS_FLOW_CONTROL;
#else
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
#endif
        UartDefaultParams.RxMode.BaudRate = 9600;
        UartDefaultParams.RxMode.TermString = (ST_String_t)"\r\0";
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl =
            UartDefaultParams.RxMode.FlowControl;
        UartDefaultParams.TxMode.BaudRate = 9600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto terminal_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 10;
    TxTimeout = 100;


    UART_DebugPrintf(("%d.0: Terminal\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Test discontinuous delivery of data.");
    {
        U32 NumberRead;
        U32 NumberWritten;
        U8 RxBuf[1024] = { 0 };
        U8 CmdBuf[1024] = { 0 };
        U8 Mode = 0;

        UART_DebugMessage("Terminal started.");
        for (;;)
        {
            error = STUART_Read(Handle,
                                RxBuf,
                                100,
                                &NumberRead,
                                RxTimeout
                               );

            /* Should timeout every .5secs */

            if((error != ST_NO_ERROR) && (error != ST_ERROR_TIMEOUT))
            {
                UART_DebugPrintf(("STUART_Read() %d, NumberRead = %u",
                                 error, NumberRead));
            }

            if (NumberRead > 0)
            {
                RxBuf[NumberRead] = 0;

                /* Mode 0 => echo back to terminal,
                 * Mode 1 => echo locally
                 */
                if (Mode == 0)
                {
                    error = STUART_Write(Handle,
                                         (U8 *)RxBuf,
                                         NumberRead,
                                         &NumberWritten,
                                         TxTimeout);
                    if(error != ST_NO_ERROR)
                    {
                        UART_DebugError("STUART_Write()", error);
                    }
                }
                else
                {
                    char *ptr;
                    for (ptr = (char *)RxBuf; *ptr; ptr++)
                        if (*ptr=='\r') *ptr = '\n';
#if TEST_VERBOSE
                    STTBX_Print(("%s",(char *)RxBuf));
#endif
                }

                /* Ensure there's enough room to copy data to cmd buffer,
                 * otherwise throw away what we have so far and start
                 * again.
                 */
                if ((1024-strlen((char *)CmdBuf))<=strlen((char *)RxBuf))
                    CmdBuf[0] = 0;

                /* Append data to command buffer */
                strcat((char *)CmdBuf,(char *)RxBuf);

                /* If an end-of-line is received, then check for a valid
                 * command.
                 */
                if (CmdBuf[strlen((char *)CmdBuf)-1]=='\r' ||
                    CmdBuf[strlen((char *)CmdBuf)-1]=='\n')
                {
                    CmdBuf[strlen((char *)CmdBuf)-1]=0;
                    if (strcmp("quit",(char *)CmdBuf) == 0)
                    {
                        break;          /* End test */
                    }
                    if (strcmp("mode",(char *)CmdBuf) == 0)
                    {
                        Mode = (Mode==1)?0:1; /* Switch mode */
                    }

                    if (strcmp("stats",(char *)CmdBuf) == 0)
                    {
                        /* Output current stats */
#ifdef STUART_STATISTICS
                        STUART_Statistics_t Statistics;

                        error = STUART_GetStatistics(Handle, &Statistics);
                        UART_DebugError("STUART_GetStatistics()", error);
                        UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
                        UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
                        UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
                        UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
                        UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));
#endif /* STUART_STATISTICS */
                    }

                    /* Restart buffer for a new command */
                    CmdBuf[0] = 0;
                }
            }
        }
    }
    UART_DebugMessage("User has quit.");
    if (error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Expected ST_NO_ERROR");
    }
    else
    {
        UART_TestPassed(Result);
    }

    /* Tidy up */
terminal_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;
}


#elif defined(TEST_MULTITHREADSAFE)

ST_ErrorCode_t ErrorCode;
BOOL KILL_TASK1 = FALSE;
BOOL KILL_TASK2 = FALSE;
S32 uart_write_busy_cnt1 = 0;
S32 uart_write_busy_cnt2 = 0;

void NotifyRoutine(STUART_Handle_t Handle, ST_ErrorCode_t error)
{
    ErrorCode = error;
    STOS_SemaphoreSignal(WaitBlockedSemaphore_p);
}

/****************************************************************************
* Outputs on Terminal not on console
****************************************************************************/

static void UART_Task1(UART_TraceTaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    U32 TxTimeout = 1000;
    char TestMessage[] = "1111111111";

    UART_DebugMessage("Starting Task1...");

    while(Params_p->ticks > 0)
    {
        U32 NumberWritten;
        error = STUART_Write(Params_p->Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );

        if ( error == ST_NO_ERROR)
        {
            if (Params_p->IsBlocking == FALSE)
            {
                /* Wait for both callbacks to complete */
                STOS_SemaphoreWait(WaitBlockedSemaphore_p);
            }
        }
        else if (error == ST_ERROR_DEVICE_BUSY)
        {
           STOS_InterruptLock();
           uart_write_busy_cnt1++;
           STOS_InterruptUnlock();
           Params_p->ticks--;
           continue;
        }

        if (Params_p->IsBlocking == FALSE)
        {
            if (ErrorCode != ST_NO_ERROR)
            {
                UART_DebugError("STUART_Write()", ErrorCode);
                KILL_TASK1 = TRUE;
                break;
            }
        }
        else
        {
       	    if ((error != ST_NO_ERROR) || (NumberWritten != strlen(TestMessage)))
            {
                UART_DebugError("STUART_Write()", error);
                KILL_TASK1 = TRUE;
                break;
            }
        }

        Params_p->ticks--;
    }

    if (Params_p->IsBlocking == FALSE)
    {
        UART_DebugPrintf(("UART write Busy cnt in TASK1 = %d \n",uart_write_busy_cnt1));
    }

}


static void UART_Task2(UART_TraceTaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    U32 TxTimeout = 1000;
    char TestMessage[] = "0000000000";

    UART_DebugMessage("Starting Task2...");

    while(Params_p->ticks > 0)
    {
        U32 NumberWritten;

        error = STUART_Write(Params_p->Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );

        if ( error == ST_NO_ERROR)
        {
            if (Params_p->IsBlocking == FALSE)
            {
                /* Wait for both callbacks to complete */
                STOS_SemaphoreWait(WaitBlockedSemaphore_p);
            }
        }
        else if (error == ST_ERROR_DEVICE_BUSY)
        {
           STOS_InterruptLock();
           uart_write_busy_cnt2++;
           STOS_InterruptUnlock();
           Params_p->ticks--;
           continue;
        }

        if (Params_p->IsBlocking == FALSE)
        {
            if (ErrorCode != ST_NO_ERROR)
            {
                UART_DebugError("STUART_Write()", ErrorCode);
                KILL_TASK2 =TRUE;
                break;
            }
        }
        else
        {
       	    if ((error != ST_NO_ERROR) || (NumberWritten != strlen(TestMessage)))
            {
                UART_DebugError("STUART_Write()", error);
                KILL_TASK2 =TRUE;
                break;
            }
        }

        Params_p->ticks--;
    }
    if (Params_p->IsBlocking == FALSE)
    {
        UART_DebugPrintf(("UART write Busy cnt in TASK2 = %d \n",uart_write_busy_cnt2));
    }
}

static UART_TestResult_t UART_TestMultithreadsafe(UART_TestParams_t *TestParams)
{
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    UART_TraceTaskParams_t Task1, Task2;
    task_t *Task1_p, *Task2_p;
    ST_ErrorCode_t error;
    
       

    /* Init */
    {
    	if (TestParams->IsBlocking == FALSE)/* non blocking */
    	{
    	    /* Create semaphores */
            WaitBlockedSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);
        }

        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            goto trace_end;
        }

    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 9600;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 9600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        if (TestParams->IsBlocking == FALSE) /* non blocking */
        {
            UartDefaultParams.TxMode.NotifyFunction = NotifyRoutine;
        }
        else
        {
            UartDefaultParams.TxMode.NotifyFunction = NULL;
        }

        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            goto trace_end;
        }

    }
    
    /* Create tasks */
    Task1.ticks = 100;
    Task2.ticks = 100;
    Task1.Handle = Handle;
    Task2.Handle = Handle;
    Task1.IsBlocking = TestParams->IsBlocking;
    Task2.IsBlocking = TestParams->IsBlocking;
       
    UART_DebugPrintf(("%d.0:Terminal \n", TestParams->Ref));
    UART_DebugMessage("Purpose: Continuous output on Hyperterminal");
    UART_DebugMessage("Creating tasks...");

    Task1_p = task_create((void(*)(void *))UART_Task1,
                           &Task1,
                           STACK_SIZE,
                           MAX_USER_PRIORITY,
                           "Task1",
                           (task_flags_t)0);

    if (Task1_p == NULL)
    {
        UART_TestFailed(Result, "Unable to create task");
        return Result;
    }
    Task2_p = task_create((void(*)(void *))UART_Task2,
                          &Task2,
                          STACK_SIZE,
                          MAX_USER_PRIORITY,
                          "Task2",
                          (task_flags_t)0);

    if (Task2_p == NULL)
    {
        UART_TestFailed(Result, "Unable to create task");
        return Result;
    }

    UART_DebugMessage("Waiting for tasks to complete...");

    /* Wait for each task before deleting it */
    if (task_wait(&Task1_p,1,TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        task_delete(Task1_p);
    }

    if (task_wait(&Task2_p,1,TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        task_delete(Task2_p);
    }


    if ( KILL_TASK1 == TRUE || KILL_TASK2 == TRUE )
    {
        UART_TestFailed(Result, "Error during processing");
    }
    else
    {
        UART_TestPassed(Result);
    }
    
    /* Tidy up */
trace_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    if (TestParams->IsBlocking == FALSE)/* non blocking */
    {
        STOS_SemaphoreDelete(NULL,WaitBlockedSemaphore_p);
    }
    return Result;
}

#elif defined(RTSCTS)

/* copied from src/api.c */
static U8 PIOBitFromBitMask(U8 BitMask)
{
    U8 Count = 0;                       /* Initially assume bit zero */

    /* Shift bit along until the bitmask is set to one */
    while (BitMask != 1 && Count < 7)
    {
        BitMask >>= 1;
        Count++;
    }

    return Count;                       /* Should contain the bit [0-7] */
}

static UART_TestResult_t UART_TestRTSCTS(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    ST_DeviceName_t RTSPortName;
    STPIO_OpenParams_t PioOpenParams;
    STPIO_Handle_t PioHandle;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 NumberRead;
    U8 RxBuf[1024];

    UART_DebugMessage("Check STUART_Init accepts CTS without RTS.");

    /* Initialise without RTS (keep CTS to prove we can specify one without
      the other - DDTS 15048) */
    {
        strcpy(RTSPortName, UartInitParams[ASCNumber].RTS.PortName);
        UartInitParams[ASCNumber].RTS.PortName[0] = '\0';

        /* Disable software FIFO - making overflow happen quicker */
        UartInitParams[ASCNumber].SwFIFOEnable = FALSE;

        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
    }

    /* Confirm we can open the RTS PIO pin ourselves,
      and set it so the PC will always transmit */
    {
        UART_DebugMessage("Check we can STPIO_Open the RTS pin directly.");

        PioOpenParams.BitConfigure[
                PIOBitFromBitMask(UartInitParams[ASCNumber].RTS.BitMask)
                ] = STPIO_BIT_OUTPUT;
        PioOpenParams.ReservedBits = UartInitParams[ASCNumber].RTS.BitMask;
        PioOpenParams.IntHandler = NULL;

        error = STPIO_Open(RTSPortName, &PioOpenParams, &PioHandle);
        UART_DebugError("STPIO_Open()", error);

        if(error == ST_NO_ERROR)
        {
            /* empirically, STPIO_Clear is needed to enable transmission rather
              than STPIO_Set ... there's an inverter in there somewhere! */
            error = STPIO_Clear(PioHandle, UartInitParams[ASCNumber].RTS.BitMask);
            UART_DebugError("STPIO_Set()", error);
        }
    }

    /* STUART_Open() */
    if (error == ST_NO_ERROR)
    {
        /* Open params setup (actually setting STUART_NO_FLOW_CONTROL
          makes no difference to whether we send RTS) */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_NO_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 57600;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_NO_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 57600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;

        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
    }

    UART_DebugPrintf(("%d.0: Without flow control\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Ensure data overflow occurs without RTSCTS.");

    if (error == ST_NO_ERROR)           /* Read overflow */
    {
        /* Read the data - should overflow */

        UART_DebugMessage("Commence a large transfer from the host...");
        error = STUART_Read(Handle, RxBuf, 1, &NumberRead, 0);

        UART_DebugMessage("Reading data...");
        error = STUART_Read(Handle, RxBuf, 1024, &NumberRead, 0);
        UART_DebugError("STUART_Read()", error);
        UART_DebugPrintf(("NumberRead = %d\n", NumberRead));
    }

    if (error == STUART_ERROR_OVERRUN)
    {
        UART_TestPassed(Result);
    }
    else
    {
        UART_TestFailed(Result, "Expected STUART_ERROR_OVERFLOW");
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = TRUE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    UART_DebugPrintf(("%d.1: With RTSCTS control\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Ensure overflow doesn't happen.");

    /* Reinitialise with RTSCTS, checking we have to Close the RTS pin first */
    {
        UART_DebugMessage("Check STUART_Init with RTS requires us to Close the pin.");

        strcpy(UartInitParams[ASCNumber].RTS.PortName, RTSPortName);

        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != STUART_ERROR_PIO)
        {
            UART_DebugMessage("Expected STUART_ERROR_PIO.");
        }

        STPIO_Close(PioHandle);

        UART_DebugMessage("Check STUART_Init with succeeds now pin is closed.");

        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
    }

    if (error == ST_NO_ERROR)
    {
        /* Open params setup */
        UartDefaultParams.RxMode.FlowControl = STUART_RTSCTS_FLOW_CONTROL;
        UartDefaultParams.TxMode.FlowControl = STUART_RTSCTS_FLOW_CONTROL;

        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
    }

    if (error == ST_NO_ERROR)
    {
        /* Wait for any remaining bytes to trickle through from last test */
        while (STUART_Read(Handle, RxBuf, 1, &NumberRead, 1000) != ST_ERROR_TIMEOUT)
            ;

        STUART_Flush(Handle);

        /* Read the data - should never overflow */
        UART_DebugMessage("Commence a large transfer from the host"
                          " WITH hardware flow control...");

        error = STUART_Read(Handle, RxBuf, 1, &NumberRead, 0);
        UART_DebugMessage("Reading data...");

        if (error == ST_NO_ERROR)
        {
            error = STUART_Read(Handle, RxBuf, 1024, &NumberRead, 0);
            UART_DebugError("STUART_Read()", error);
        }
        UART_DebugPrintf(("NumberRead = %d\n", NumberRead));

#if 0
        {
            U32 NumberWritten = 0;

            /* Pump some bytes out, and see behaviour
              of Close if cable is unplugged */

            error = STUART_Write(Handle, (U8*)"RTSCTS\n", 7,
                                 &NumberWritten, 100);
            UART_DebugError("STUART_Write()", error);
            UART_DebugPrintf(("NumberWritten = %d\n", NumberWritten));
        }
#endif
    }

    if (error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Expected ST_NO_ERROR");
    }
    else
    {
        UART_TestPassed(Result);
    }

    /* Tidy up tests */

    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;                      /* Return test result pass/fail count */
}

#elif defined(XONXOFF) && defined(TEST_TWO_PORTS)

/****************************************************************************
* Loopback functions for use in XON/XOFF testing
****************************************************************************/

static void UART_ProducerTask2(UART_TaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = ASC_DEVICE_MODEM;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
            goto producer_end;
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 115200;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 115200;

        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
            goto producer_end;
    }

    /* Timeout Params */
    RxTimeout = 5000;
    TxTimeout = 1000;

    UART_DebugMessage("Starting producer task...");

    /* Wait for consumer to commence */
    STOS_SemaphoreWait(WaitBlockedSemaphore_p);

    while(Params_p->ticks > 0)
    {
        U32 NumberWritten;
        char TestMessage[] = "0123456789ABCDEF";

        if (KillTransmitter)
        {
            break;
        }

        error = STUART_Write(Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );

        if (error != ST_NO_ERROR)
        {
            UART_DebugError("STUART_Write()", error);
            break;
        }

        Params_p->ticks--;
    }

    /* Get final statistics */
    Params_p->Error = error;
#ifdef STUART_STATISTICS
    error = STUART_GetStatistics(Handle, Params_p->Statistics_p);
    UART_DebugError("STUART_GetStatistics()", error);
#endif

    /* Tidy up */
producer_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }
}

static void UART_ConsumerTask2(UART_TaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = ASC_DEVICE_DATA;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
            goto consumer_end;
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 115200;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 115200;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
            goto consumer_end;
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 1000;


    UART_DebugMessage("Starting consumer task...");

    /* Allow producer to commence */
    STOS_SemaphoreSignal(WaitBlockedSemaphore_p);

    while(Params_p->ticks > 0)
    {
        U32 NumberRead;
        char TestMessage[35] = {0};

        /* Artificially induce a small delay in processing */
        STOS_TaskDelay(TIME_DELAY);

        /* Try a read */
        error = STUART_Read(Handle,
                            (U8 *)TestMessage,
                            16,
                            &NumberRead,
                            RxTimeout
                           );
        if (error != ST_NO_ERROR)
        {
            UART_DebugError("STUART_Read()", error);
            break;
        }

        if (strcmp(TestMessage, "0123456789ABCDEF") != 0)
        {
            UART_DebugPrintf(("Bad data = 0x%02x\n", TestMessage[0]));
            break;
        }

        Params_p->ticks--;
    }

    /* Unless any remaining bytes are read, the OVERRUN flag
     * will continue to be set. Therefore, we need to
     * do another read.
     */
    STUART_Flush(Handle);
    if (error != ST_NO_ERROR)
    {
        U32 NumberRead = 1;
        char TempBuffer[1024] = {0};
        ST_ErrorCode_t error;

        KillTransmitter = TRUE;
        while (NumberRead != 0)
        {
            error = STUART_Read(Handle,
                                (U8 *)TempBuffer,
                                1024,
                                &NumberRead,
                                RxTimeout
                               );
        }
    }

    /* Get final statistics */
    Params_p->Error = error;
#ifdef STUART_STATISTICS
    error = STUART_GetStatistics(Handle, Params_p->Statistics_p);
    UART_DebugError("STUART_GetStatistics()", error);
#endif

    /* Tidy up */
consumer_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }
}

static void UART_ProducerTask3(UART_TaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = ASC_DEVICE_MODEM;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup Uart */
        UartInitParams->UARTType = STUART_NO_HW_FIFO;
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
            goto producer3_end;
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_SW_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 115200;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_SW_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 115200;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            goto producer3_end;
        }
    }

    /* Timeout Params */
#if defined(ST_7710)
    RxTimeout = 0;
    TxTimeout = 0;
#else
    RxTimeout = 50000; /* 50ms */
    TxTimeout = 10000; /* 10ms */
#endif

    UART_DebugMessage("Starting producer task...");

    /* Wait for consumer to commence */
    STOS_SemaphoreWait(WaitBlockedSemaphore_p);

    while(Params_p->ticks > 0)
    {
        U32 NumberWritten;
        char TestMessage[] = "0123456789ABCDEF";

        error = STUART_Write(Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );

        if (error != ST_NO_ERROR)
        {
            UART_DebugError("STUART_Write()", error);
            break;
        }

        Params_p->ticks--;
    }

    /* Get final statistics */
    Params_p->Error = error;
#ifdef STUART_STATISTICS
    error = STUART_GetStatistics(Handle, Params_p->Statistics_p);
    UART_DebugError("STUART_GetStatistics()", error);
#endif

    /* Tidy up */
producer3_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }
}

static void UART_ConsumerTask3(UART_TaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = ASC_DEVICE_DATA;
    U32 RxTimeout, TxTimeout;
    /* Init */
    {
        /* Setup Uart */
        UartInitParams->UARTType = STUART_NO_HW_FIFO;
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);

        if (error != ST_NO_ERROR)
            goto consumer3_end;
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_SW_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 115200;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_SW_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 115200;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
            goto consumer3_end;
    }

    /* Timeout Params */
#if defined(ST_7109) 
    RxTimeout = 50000; /* 50ms */
    TxTimeout = 10000; /* 10ms */
#else
    RxTimeout = 1000;
    TxTimeout = 1000;
#endif

    UART_DebugMessage("Starting consumer task...");

    /* Allow producer to commence */
    STOS_SemaphoreSignal(WaitBlockedSemaphore_p);

    while(Params_p->ticks > 0)
    {
        U32 NumberRead;

        char TestMessage[35] = {0};

        /* Artificially induce a small delay in processing */
        STOS_TaskDelay(TIME_DELAY);

        /* Try a write  */
        error = STUART_Read(Handle,
                            (U8 *)TestMessage,
                            16,
                            &NumberRead,
                            RxTimeout
                           );
        if (error != ST_NO_ERROR)
        {
            UART_DebugError("STUART_Read()", error);
            break;
        }

        if (strcmp(TestMessage, "0123456789ABCDEF") != 0)
        {
            UART_DebugPrintf(("Bad data = 0x%02x\n", TestMessage[0]));
            break;
        }

        Params_p->ticks--;
    }

    /* Get final statistics */
    Params_p->Error = error;
#ifdef STUART_STATISTICS
    error = STUART_GetStatistics(Handle, Params_p->Statistics_p);
    UART_DebugError("STUART_GetStatistics()", error);
#endif

    /* Tidy up */
consumer3_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }
}

static UART_TestResult_t UART_TestXONXOFF(UART_TestParams_t *TestParams)
{
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    BOOL Error;
#ifdef STUART_STATISTICS
    STUART_Statistics_t ProducerStats, ConsumerStats;
    STUART_Statistics_t ProducerStats2, ConsumerStats2;
#endif
    UART_TaskParams_t ProducerParams, ConsumerParams;
   
    ConsumerParams.Driverpartition = (ST_Partition_t*)system_partition;
    ProducerParams.Driverpartition = (ST_Partition_t*)system_partition;	
	
    WaitBlockedSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* Create tasks */
    ProducerParams.ticks = 1000;
#ifdef STUART_STATISTICS
    ProducerParams.Statistics_p = &ProducerStats;
#endif
    ConsumerParams.ticks = 1000;
#ifdef STUART_STATISTICS
    ConsumerParams.Statistics_p = &ConsumerStats;
#endif

    KillTransmitter = FALSE;

    UART_DebugPrintf(("%d.0: Multi-task back-to-back\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To ensure overflow happens without flow control.");
    UART_DebugMessage("Creating tasks...");

    Error =  STOS_TaskCreate   ((void(*)(void *))UART_ConsumerTask2,
							&ConsumerParams,
							ConsumerParams.Driverpartition,
							STACK_SIZE,
							NULL,
							ConsumerParams.Driverpartition,
							&ConsumerParams.Task_p,
							NULL, 
							MAX_USER_PRIORITY,
							"Consumer", 
							(task_flags_t)0);
    
    
    if  (Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Unable to create task");
        return Result;
    }

    Error =  STOS_TaskCreate   ((void(*)(void *))UART_ProducerTask2,
							&ProducerParams,
							ProducerParams.Driverpartition,
							STACK_SIZE,
							NULL,
							ProducerParams.Driverpartition,
							&ProducerParams.Task_p,
							NULL, 
							MAX_USER_PRIORITY,
							"Producer", 
							(task_flags_t)0);
    if  (Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Unable to create task");
        return Result;
    }

    UART_DebugMessage("Waiting for tasks to complete...");

     /* Wait for each task before deleting it */
    if (STOS_TaskWait(&ProducerParams.Task_p, TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        STOS_TaskDelete( ProducerParams.Task_p,
				 	    ProducerParams.Driverpartition, 
					    NULL,
					    ProducerParams.Driverpartition);
    }

    /* Wait for each task before deleting it */
    if (STOS_TaskWait(&ConsumerParams.Task_p, TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        STOS_TaskDelete( ConsumerParams.Task_p,
				 	    ConsumerParams.Driverpartition, 
					    NULL,
					    ConsumerParams.Driverpartition);
    }
  
   
#ifdef STUART_STATISTICS
    UART_DebugMessage("Producer stats...");
    UART_DebugPrintf(("NumberBytesReceived = %d\n", ProducerStats.NumberBytesReceived));
    UART_DebugPrintf(("NumberBytesTransmitted = %d\n", ProducerStats.NumberBytesTransmitted));
    UART_DebugPrintf(("NumberOverrunErrors = %d\n", ProducerStats.NumberOverrunErrors));
    UART_DebugPrintf(("NumberFramingErrors = %d\n", ProducerStats.NumberFramingErrors));
    UART_DebugPrintf(("NumberParityErrors = %d\n", ProducerStats.NumberParityErrors));

    UART_DebugMessage("Consumer stats...");
    UART_DebugPrintf(("NumberBytesReceived = %d\n", ConsumerStats.NumberBytesReceived));
    UART_DebugPrintf(("NumberBytesTransmitted = %d\n", ConsumerStats.NumberBytesTransmitted));
    UART_DebugPrintf(("NumberOverrunErrors = %d\n", ConsumerStats.NumberOverrunErrors));
    UART_DebugPrintf(("NumberFramingErrors = %d\n", ConsumerStats.NumberFramingErrors));
    UART_DebugPrintf(("NumberParityErrors = %d\n", ConsumerStats.NumberParityErrors));
#endif /* STUART_STATISTICS */

    if (ConsumerParams.Error == ST_NO_ERROR ||
        ProducerParams.Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Error during processing");
    }
    else
    {
        UART_TestPassed(Result);
    }

    UART_DebugPrintf(("%d.0: Multi-task back-to-back\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To ensure overflow doesn't occur with software flow control.");
    UART_DebugMessage("Creating tasks...");

    /* Create tasks */
    ProducerParams.ticks = 1000;
#ifdef STUART_STATISTICS
    ProducerParams.Statistics_p = &ProducerStats2;
#endif
    ConsumerParams.ticks = 1000;
#ifdef STUART_STATISTICS
    ConsumerParams.Statistics_p = &ConsumerStats2;
#endif

    ProducerParams.Error = ST_NO_ERROR;
    ConsumerParams.Error = ST_NO_ERROR;
    ProducerParams.ASCNumber = ASC_DEVICE_MODEM;
    ConsumerParams.ASCNumber = ASC_DEVICE_DATA;

    KillTransmitter = FALSE;

    STOS_SemaphoreDelete(NULL,WaitBlockedSemaphore_p);
    WaitBlockedSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

   Error =  STOS_TaskCreate   ((void(*)(void *))UART_ConsumerTask3,
							&ConsumerParams,
							ConsumerParams.Driverpartition,
							STACK_SIZE,
							NULL,
							ConsumerParams.Driverpartition,
							&ConsumerParams.Task_p,
							NULL, 
							MAX_USER_PRIORITY,
							"Consumer", 
							(task_flags_t)0);
    
    if  (Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Unable to create task");
        return Result;
    }

    Error =  STOS_TaskCreate   ((void(*)(void *))UART_ProducerTask3,
							&ProducerParams,
							ProducerParams.Driverpartition,
							STACK_SIZE,
							NULL,
							ProducerParams.Driverpartition,
							&ProducerParams.Task_p,
							NULL, 
							MAX_USER_PRIORITY,
							"Producer", 
							(task_flags_t)0);
     if  (Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Unable to create task");
        return Result;
    }

    UART_DebugMessage("Waiting for tasks to complete...");

     /* Wait for each task before deleting it */
    if (STOS_TaskWait(&ProducerParams.Task_p, TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        STOS_TaskDelete( ProducerParams.Task_p,
				 	    ProducerParams.Driverpartition, 
					    NULL,
					    ProducerParams.Driverpartition);
    }
  
    
    /* Wait for each task before deleting it */
    if (STOS_TaskWait(&ConsumerParams.Task_p, TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        STOS_TaskDelete( ConsumerParams.Task_p,
				 	    ConsumerParams.Driverpartition, 
					    NULL,
					    ConsumerParams.Driverpartition);
    }

    STOS_SemaphoreDelete(NULL,WaitBlockedSemaphore_p);

#ifdef STUART_STATISTICS
    UART_DebugMessage("Producer stats...");
    UART_DebugPrintf(("NumberBytesReceived = %d\n", ProducerStats2.NumberBytesReceived));
    UART_DebugPrintf(("NumberBytesTransmitted = %d\n", ProducerStats2.NumberBytesTransmitted));
    UART_DebugPrintf(("NumberOverrunErrors = %d\n", ProducerStats2.NumberOverrunErrors));
    UART_DebugPrintf(("NumberFramingErrors = %d\n", ProducerStats2.NumberFramingErrors));
    UART_DebugPrintf(("NumberParityErrors = %d\n", ProducerStats2.NumberParityErrors));

    UART_DebugMessage("Consumer stats...");
    UART_DebugPrintf(("NumberBytesReceived = %d\n", ConsumerStats2.NumberBytesReceived));
    UART_DebugPrintf(("NumberBytesTransmitted = %d\n", ConsumerStats2.NumberBytesTransmitted));
    UART_DebugPrintf(("NumberOverrunErrors = %d\n", ConsumerStats2.NumberOverrunErrors));
    UART_DebugPrintf(("NumberFramingErrors = %d\n", ConsumerStats2.NumberFramingErrors));
    UART_DebugPrintf(("NumberParityErrors = %d\n", ConsumerStats2.NumberParityErrors));
#endif /* STUART_STATISTICS */

    if (ConsumerParams.Error != ST_NO_ERROR ||
        ProducerParams.Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Error during processing");
    }
    else
    {
        UART_TestPassed(Result);
    }

    return Result;
}

#elif defined(STUFFING)

static UART_TestResult_t UART_TestStuffing(UART_TestParams_t *TestParams)
{


    ST_ErrorCode_t error = ST_NO_ERROR;
    STUART_Handle_t Handle = 0;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 TxTimeout;

    /* STUART_Init() */
    UART_DebugPrintf(("%d.0: STUART_Init()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be initialized.");
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize ASC_DEVICE_3 device");
            goto api_end;
        }
        UART_TestPassed(Result);

    }

    /* STUART_Open() */
    UART_DebugPrintf(("%d.1: STUART_Open()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be opened.");
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 9600;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 9600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto api_end;
        }
        UART_TestPassed(Result);

     }

    /* Timeout Params */
    TxTimeout = 1000;

    /* Enable Stuffing */
    UART_DebugPrintf(("%d.2: STUART_EnableStuffing\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To ensure stuffing is turned ON.");
    {
        U32 NumberWritten;
        char TestMessage[] = "0123456789ABCDEF0123456789ABCDEF";
        U8 StuffByte = 0x7F;
        int ticks = 5000;


        error = STUART_EnableStuffing(Handle,StuffByte);
        UART_DebugError("STUART_EnableStuffing()", error);

        /* Try a read/write cycle */
        error = STUART_Write(Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );


        UART_DebugError("STUART_Write()", error);
        UART_DebugPrintf(("Written %d bytes.\n",NumberWritten));
        /* Check all bytes were successfully written */
        if (NumberWritten != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to write all bytes");
            goto api_end;
        }

        /* wait for sometime & observe on hyperterminal if the
           stuffing is sent or not */
        while(ticks)
        {
            STOS_TaskDelay(TIME_DELAY);
            ticks--;
        }
        UART_TestPassed(Result);

    }

    /* Disable Stuffing */
    UART_DebugPrintf(("%d.3: STUART_DisableStuffing\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To ensure stuffing is turned OFF.");
    {
        U32 NumberWritten;
        char TestMessage[] = "0123456789ABCDEF0123456789ABCDEF";
        int ticks = 5000;

        error = STUART_DisableStuffing(Handle);
        UART_DebugError("STUART_DisableStuffing()", error);

        /* Try a read/write cycle */
        error = STUART_Write(Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );


        UART_DebugError("STUART_Write()", error);
        UART_DebugPrintf(("Written %d bytes.\n",NumberWritten));
        /* Check all bytes were successfully written */
        if (NumberWritten != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to write all bytes");
            goto api_end;
        }

        /* wait for sometime & observe on hyperterminal whether the
           stuffing is still sent */
        while(ticks)
        {
            STOS_TaskDelay(TIME_DELAY);
            ticks--;
        }
        UART_TestPassed(Result);

    }

    /* Enable Stuffing second time to be sure of its consistent performance */
    UART_DebugPrintf(("%d.4: STUART_EnableStuffing\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Stuffing is turned ON second time");
    {
        U32 NumberWritten;
        char TestMessage[] = "0123456789ABCDEF0123456789ABCDEF";
        U8 StuffByte = 0x7F;
        int ticks = 5000;

        error = STUART_EnableStuffing(Handle,StuffByte);
        UART_DebugError("STUART_EnableStuffing()", error);

        /* Try a read/write cycle */
        error = STUART_Write(Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );


        UART_DebugError("STUART_Write()", error);
        UART_DebugPrintf(("Written %d bytes.\n",NumberWritten));
        /* Check all bytes were successfully written */
        if (NumberWritten != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to write all bytes");
            goto api_end;
        }

        /* wait for sometime & observe on hyperterminal if the
           stuffing is sent or not */
        while(ticks)
        {
            STOS_TaskDelay(TIME_DELAY);
            ticks--;
        }
        UART_TestPassed(Result);

    }

    /* Disable Stuffing */
    UART_DebugPrintf(("%d.5: STUART_DisableStuffing\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Stuffing is turned OFF second time");
    {
        U32 NumberWritten;
        char TestMessage[] = "0123456789ABCDEF0123456789ABCDEF";
        int ticks = 5000;

        error = STUART_DisableStuffing(Handle);
        UART_DebugError("STUART_DisableStuffing()", error);

        /* Try a read/write cycle */
        error = STUART_Write(Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );


        UART_DebugError("STUART_Write()", error);
        UART_DebugPrintf(("Written %d bytes.\n",NumberWritten));
        /* Check all bytes were successfully written */
        if (NumberWritten != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to write all bytes");
            goto api_end;
        }

        /* wait for sometime & observe on hyperterminal whether the
           stuffing is still sent */
        while(ticks)
        {
            STOS_TaskDelay(TIME_DELAY);
            ticks--;
        }
        UART_TestPassed(Result);

    }

api_end:
    /* STUART_Close() */
    UART_DebugPrintf(("%d.6: STUART_Close()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be closed.");
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result, "Unable to close UART");
        }
        else
        {
            UART_TestPassed(Result);
        }

    }

    /* STUART_Term() */
    UART_DebugPrintf(("%d.7: STUART_Term()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be terminated.");
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to terminate UART");
        }
        else
        {
            UART_TestPassed(Result);
        }

    }

    return Result;
}

#elif defined(TEST_UARTDMA) && defined(STUART_DMA_SUPPORT)
static UART_TestResult_t UART_TestDMA(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle = 0;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;

    U32 RxTimeout, TxTimeout;
    U8 *DataBuffer1 = NULL, *store1;
    U8 *DataBuffer2 = NULL, *store2;
    U8 *DMADataBuffer1 = NULL;
    U8 *DMADataBuffer2 = NULL;
    U32 NumberRead;
    U32 NumberWritten;
    U32 Count = 0;
    U32 ByteCount = 4096;
    U32 i,j;
    clock_t ReadStartTime=0,ReadEndTime=0,WriteStartTime=0,WriteEndTime=0;

    /* Allocate from system partition region */
    DataBuffer1 = STOS_MemoryAllocate(system_partition,ByteCount);
    DataBuffer2 = STOS_MemoryAllocate(system_partition,ByteCount);

    /* Allocate from Ncache partition region */
    store1 = STOS_MemoryAllocate(the_ncache_partition,ByteCount + 127);
    store2 = STOS_MemoryAllocate(the_ncache_partition,ByteCount + 127);
    DMADataBuffer1 = (U8 *)(((U32)store1 + 127) & 0xffffff00);
    DMADataBuffer2 = (U8 *)(((U32)store2 + 127) & 0xffffff00);

    for (i = 0; i < ByteCount; i++)
    {
        DMADataBuffer1[i] = (U8)i;
        DMADataBuffer2[i] = 0x00;
        DataBuffer1[i]    = (U8)i;
        DataBuffer2[i]    = 0x00;
    }

    /* STUART_Init() */
    UART_DebugPrintf(("%d.0: STUART_Init()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be initialized\n");
    UART_DebugMessage("Purpose: to ensure performance improvement in using uart with DMA");
    UART_DebugMessage("Purpose:UART \n");
    {
        {
            /* Setup Uart */
            error = STUART_Init(UartDeviceName[ASCNumber],
                                &UartInitParams[ASCNumber]);
            UART_DebugError("STUART_Init()", error);
            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result,"Unable to initialize UART device");
                goto api_end;
            }
            UART_TestPassed(Result);
        }

        /* STUART_Open() */
        UART_DebugPrintf(("%d.1: STUART_Open()\n", TestParams->Ref));
        UART_DebugMessage("Purpose: to ensure a UART device can be opened.");
        {
            /* Open params setup */
            UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
            UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
            UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
            UartDefaultParams.RxMode.BaudRate = 38400;
            UartDefaultParams.RxMode.TermString = NULL;
            UartDefaultParams.RxMode.NotifyFunction = NULL;
            UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
            UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
            UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
            UartDefaultParams.TxMode.BaudRate = 38400;
            UartDefaultParams.TxMode.TermString = NULL;
            UartDefaultParams.TxMode.NotifyFunction = NULL;
            UartDefaultParams.SmartCardModeEnabled = FALSE;
            UartDefaultParams.GuardTime = 0;
            UartOpenParams.LoopBackEnabled = TRUE;

            UartOpenParams.FlushIOBuffers = TRUE;
            UartOpenParams.DefaultParams = &UartDefaultParams;
            error = STUART_Open(UartDeviceName[ASCNumber],
                                &UartOpenParams, &Handle);
            UART_DebugError("STUART_Open()", error);
            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result,"Unable to open UART device");
                goto api_end;
            }

            UART_TestPassed(Result);
        }

        RxTimeout = 1000;
        TxTimeout = 1000;

        /* Loopback */
        UART_DebugPrintf(("%d.2: Loopback\n", TestParams->Ref));
        UART_DebugMessage("Purpose: to ensure a UART can write & read data.");
        {
            for(i = 256; i < ByteCount ; i <<= 1)
            {

                 WriteStartTime = STOS_time_now();
                 /* Try a read/write cycle */
                 error = STUART_Write(Handle,
                                     (U8 *)DataBuffer1,
                                      i,
                                     &NumberWritten,
                                      TxTimeout
                                    );

                 WriteEndTime = STOS_time_now();
                 UART_DebugError("STUART_Write()", error);
                 UART_DebugPrintf(("Written %d bytes in %d ticks\n",i,STOS_time_minus(WriteEndTime,WriteStartTime)));

                 /* Check all bytes were successfully written */
                 if (NumberWritten != i)
                 {
                     UART_TestFailed(Result, "Failed to write all bytes");
                     goto api_end;
                 }

                 ReadStartTime = STOS_time_now();
                 error = STUART_Read(Handle,
                                     DataBuffer2,
                                     i,
                                     &NumberRead,
                                     RxTimeout
                                    );

                 ReadEndTime = STOS_time_now();
                 UART_DebugError("STUART_Read()", error);
                 UART_DebugPrintf(("Read %d bytes in %d ticks\n\n",i,STOS_time_minus(ReadEndTime,ReadStartTime)));

                /* Check all bytes were successfully read */
                if (NumberRead != i)
                {
                    UART_TestFailed(Result, "Failed to read all bytes");
                    goto api_end;
                }
                if (memcmp (DataBuffer1,DataBuffer2, i) == 0)
                {
                    UART_DebugPrintf(("Data read matches data written!\n"));

                }
                else
                {
                        /* Find out first different character */
                    for (j = 0; j < i; j++)
                    {
                      if (DataBuffer1[j] != DataBuffer2[j])
                      {
                            UART_DebugPrintf(("Data Mismatch at postition %d\n",j));
                            break;
                      }
                    }
                }
                ReadStartTime = 0;
                ReadEndTime   = 0;
                WriteStartTime= 0;
                WriteEndTime  = 0;

              }/*for*/

              UART_TestPassed(Result);
        }
#ifdef STUART_STATISTICS
        /* STUART_GetStatistics() */
        {
            STUART_Statistics_t Statistics;

            error = STUART_GetStatistics(Handle, &Statistics);
            UART_DebugError("STUART_GetStatistics()", error);
            UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
            UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
            UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
            UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
            UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));
        }
#endif /* STUART_STATISTICS */

api_end:

         /* STUART_Close() */
        UART_DebugPrintf(("%d.3: STUART_Close()\n", TestParams->Ref));
        UART_DebugMessage("Purpose: to ensure a UART device can be closed.");
        {
            error = STUART_Close(Handle);
            UART_DebugError("STUART_Close()", error);
            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result, "Unable to close UART");
            }
            else
            {
                UART_TestPassed(Result);
            }
        }

        /* STUART_Term() */
        UART_DebugPrintf(("%d.4: STUART_Term()\n", TestParams->Ref));
        UART_DebugMessage("Purpose: to ensure a UART device can be terminated.");
        {
            STUART_TermParams_t UartTermParams;
            UartTermParams.ForceTerminate = FALSE;

            error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
            UART_DebugError("STUART_Term()", error);
            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result,"Unable to terminate UART");
            }
            else
            {
                UART_TestPassed(Result);
            }

         }
    }/*UART*/
    UART_DebugMessage("*********************************************************\n");
    UART_DebugMessage("Purpose:UART using DMA\n");
    {
        {
            /* Setup Uart */
            UartInitParams[ASCNumber].SwFIFOEnable        = TRUE;
            UartInitParams[ASCNumber].UARTType            = STUART_DMA;
            UartInitParams[ASCNumber].FIFOLength          = 131072;
            UartInitParams[ASCNumber].NCachePartition     = the_ncache_partition;
            UartInitParams[ASCNumber].DefaultParams       = NULL;
            UartInitParams[ASCNumber].RXDMARequestSignal  = STFDMA_REQUEST_SIGNAL_UART3_RX;
            UartInitParams[ASCNumber].TXDMARequestSignal  = STFDMA_REQUEST_SIGNAL_UART3_TX;
            /* Setup Uart */
            error = STUART_Init(UartDeviceName[ASCNumber],
                                &UartInitParams[ASCNumber]);
            UART_DebugError("STUART_Init()", error);
            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result,"Unable to initialize UART device");
                goto api2_end;
            }
            UART_TestPassed(Result);
        }

        /* STUART_Open() */
        UART_DebugPrintf(("%d.1: STUART_Open()\n", TestParams->Ref));
        UART_DebugMessage("Purpose: to ensure a UART device can be opened.");
        {
            /* Open params setup */
            UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
            UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
            UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
            UartDefaultParams.RxMode.BaudRate = STUART_BAUDRATE_MAX;
            UartDefaultParams.RxMode.TermString = NULL;
            UartDefaultParams.RxMode.NotifyFunction = NULL;
            UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
            UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
            UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
            UartDefaultParams.TxMode.BaudRate = STUART_BAUDRATE_MAX;
            UartDefaultParams.TxMode.TermString = NULL;
            UartDefaultParams.TxMode.NotifyFunction = NULL;
            UartDefaultParams.SmartCardModeEnabled = FALSE;
            UartDefaultParams.GuardTime = 0;
            UartOpenParams.LoopBackEnabled = TRUE;

            UartOpenParams.FlushIOBuffers = TRUE;
            UartOpenParams.DefaultParams = &UartDefaultParams;
            error = STUART_Open(UartDeviceName[ASCNumber],
                                &UartOpenParams, &Handle);
            UART_DebugError("STUART_Open()", error);
            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result,"Unable to open UART device");
                goto api2_end;
            }

            UART_TestPassed(Result);
        }

        RxTimeout = 1000;
        TxTimeout = 1000;

        /* Loopback */
        UART_DebugPrintf(("%d.2: Loopback\n", TestParams->Ref));
        UART_DebugMessage("Purpose: to ensure a UART can write & read data");
        {

            for(i = 256; i < ByteCount ; i <<= 1)
            {
                 WriteStartTime = STOS_time_now();
                 /* Try a read/write cycle */
                 error = STUART_Write(Handle,
                                     (U8 *)DMADataBuffer1,
                                      i,
                                      &NumberWritten,
                                      TxTimeout
                                    );

                 WriteEndTime = STOS_time_now();
                 UART_DebugError("STUART_Write()", error);
                 UART_DebugPrintf(("Written %d bytes in %d ticks\n",i,STOS_time_minus(WriteEndTime,WriteStartTime)));

                 /* Check all bytes were successfully written */
                 if (NumberWritten != i)
                 {
                     UART_TestFailed(Result, "Failed to write all bytes");
                     goto api2_end;
                 }

                 ReadStartTime = STOS_time_now();
                 error = STUART_Read(Handle,
                                     DMADataBuffer2,
                                     i,
                                     &NumberRead,
                                     RxTimeout
                                    );
                 ReadEndTime = STOS_time_now();
                 UART_DebugError("STUART_Read()", error);
                 UART_DebugPrintf(("Read %d bytes in %d ticks\n\n",i,STOS_time_minus(ReadEndTime,ReadStartTime)));

                /* Check all bytes were successfully read */
                if (NumberRead != i)
                {
                    UART_TestFailed(Result, "Failed to read all bytes");
                    goto api2_end;
                }
                if (memcmp (DMADataBuffer1,DMADataBuffer2, i) == 0)
                {
                    UART_DebugPrintf(("Data read matches data written!\n"));

                }
                else
                {
                        /* Find out first different character */
                    for (j = 0; j < i; j++)
                    {
                      if (DMADataBuffer1[j] != DMADataBuffer2[j])
                      {
                            UART_DebugPrintf(("Data Mismatch at postition %d\n",j));
                            break;
                      }
                    }
                }
                ReadStartTime = 0;
                ReadEndTime   = 0;
                WriteStartTime= 0;
                WriteEndTime  = 0;
              }/*for*/

              UART_TestPassed(Result);
        }
#ifdef STUART_STATISTICS
        /* STUART_GetStatistics() */
        {
            STUART_Statistics_t Statistics;

            error = STUART_GetStatistics(Handle, &Statistics);
            UART_DebugError("STUART_GetStatistics()", error);
            UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
            UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
            UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
            UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
            UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));
        }
#endif /* STUART_STATISTICS */

api2_end:
        if(store1)
        {
            STOS_MemoryDeallocate(the_ncache_partition, store1);
        }
        if(store2)
        {
            STOS_MemoryDeallocate(the_ncache_partition, store2);
        }
        if (DataBuffer1)
        {
            STOS_MemoryDeallocate(system_partition, DataBuffer1);
        }
        if (DataBuffer2)
        {
            STOS_MemoryDeallocate(system_partition, DataBuffer2);
        }
        /* STUART_Close() */
        UART_DebugPrintf(("%d.3: STUART_Close()\n", TestParams->Ref));
        UART_DebugMessage("Purpose: to ensure a UART device can be closed.");
        {
            error = STUART_Close(Handle);
            UART_DebugError("STUART_Close()", error);
            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result, "Unable to close UART");
            }
            else
            {
                UART_TestPassed(Result);
            }

        }

        /* STUART_Term() */
        UART_DebugPrintf(("%d.4: STUART_Term()\n", TestParams->Ref));
        UART_DebugMessage("Purpose: to ensure a UART device can be terminated.");
        {
            STUART_TermParams_t UartTermParams;
            UartTermParams.ForceTerminate = FALSE;

            error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
            UART_DebugError("STUART_Term()", error);
            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result,"Unable to terminate UART");
            }
            else
            {
                UART_TestPassed(Result);
            }

         }
    }
    return Result;
}

#else

/****************************************************************************
* Automated test harness
****************************************************************************/

/* Nacked byte test - test smartcard mode */

#if defined(TEST_TWO_PORTS)
static UART_TestResult_t UART_TestSmart(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t TxHandle = 0, RxHandle = 0;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 TxASCNumber = ASC_DEVICE_MODEM;
    U8 RxASCNumber = ASC_DEVICE_DATA;
    UART_TestResult_t Result = TEST_RESULT_ZERO;

    /* Sender Init */
    {
        STUART_Device_t DeviceType;

        /* Setup Uart - note that we can't use the FIFOs */
        DeviceType = UartInitParams[TxASCNumber].UARTType;
        UartInitParams[TxASCNumber].UARTType = STUART_NO_HW_FIFO;

        error = STUART_Init(UartDeviceName[TxASCNumber],
                            &UartInitParams[TxASCNumber]);
        UART_DebugError("STUART_Init()", error);

        /* Restore previous setting */
        UartInitParams[TxASCNumber].UARTType = DeviceType;
    }

    /* Receiver Init */
    {
        error = STUART_Init(UartDeviceName[RxASCNumber],
                            &UartInitParams[RxASCNumber]);
        UART_DebugError("STUART_Init()", error);
    }

    /* Sender STUART_Open() */
    if (error == ST_NO_ERROR)
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_5;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
#if defined(ST_7109)
        UartDefaultParams.RxMode.BaudRate = 9600;
#else
        UartDefaultParams.RxMode.BaudRate = 300;
#endif
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_5;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
#if defined(ST_7109)
        UartDefaultParams.TxMode.BaudRate = 9600;
#else
        UartDefaultParams.TxMode.BaudRate = 300;
#endif
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = WriteNotifyRoutine;
        UartDefaultParams.SmartCardModeEnabled = TRUE;
        UartDefaultParams.NACKSignalDisabled = FALSE;
        UartDefaultParams.GuardTime = 2; /* Must be at least 2 */
        UartDefaultParams.Retries = 3;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[TxASCNumber],
                            &UartOpenParams, &TxHandle);
        UART_DebugError("STUART_Open()", error);
    }

    /* Receiver STUART_Open() */
    if (error == ST_NO_ERROR)
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_0_5;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 300;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_0_5;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 300;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[RxASCNumber],
                            &UartOpenParams, &RxHandle);
        UART_DebugError("STUART_Open()", error);
    }

    UART_DebugPrintf(("%d.0: Retries test\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Ensure NACKed bytes are 'retried' SC-mode.");

    if (error == ST_NO_ERROR)
    {
        U32 NumberNacks, NumberWritten;
        U8 TxBuf[20];

        /* Create notify semaphore */
        WaitBlockedSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

        /* Simulate sending data to the smartcard (non-blocking) */
        error = STUART_Write(TxHandle, TxBuf, 20, &NumberWritten, TIME_DELAY/10);

        /* Simulate smartcard receiving data and nacking every time - we need to
         * send at least Retries+1 bytes i.e., the initial nack and a nack for
         * each retry.
         */
        if (error == ST_NO_ERROR)
        {
#if defined(ST_OS21)
            STUART_Write(RxHandle, TxBuf, 20, &NumberNacks, 0);
#else
            STUART_Write(RxHandle, TxBuf, 20, &NumberNacks, TIME_DELAY);
#endif
        }

        /* Wait for sender to complete transfer of smartcard command */
        STOS_SemaphoreWait(WaitBlockedSemaphore_p);
        STOS_SemaphoreDelete(NULL,WaitBlockedSemaphore_p);

        /* Check error code */
        UART_DebugError("Sender STUART_Write()", WriteErrorCode);
        if (WriteErrorCode == STUART_ERROR_RETRIES)
        {
            UART_TestPassed(Result);
        }
        else
        {
            UART_TestFailed(Result, "Expected STUART_ERROR_RETRIES");
        }
    }

    /* Tidy up test */

    /* STUART_Close() */
    {
        error = STUART_Close(TxHandle);
        UART_DebugError("STUART_Close()", error);
        error = STUART_Close(RxHandle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[TxASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
        error = STUART_Term(UartDeviceName[RxASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;
}
#endif

/* Simple test */

static UART_TestResult_t UART_TestTx1(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* STUART_Init() */
    UART_DebugPrintf(("%d.0: STUART_Init()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be initialized.");
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto api_end;
        }
        UART_TestPassed(Result);
    }

    /* STUART_Open() */
    UART_DebugPrintf(("%d.1: STUART_Open()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be opened.");
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 9600;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 9600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto api_end;
        }
        UART_TestPassed(Result);
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 1000;

    /* Loopback */
    UART_DebugPrintf(("%d.2: Loopback\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART can write & read data.");
    {
        U32 NumberRead;
        U32 NumberWritten = 0;
        U8 RxBuf[100] = { 0 };
        char TestMessage[] = "0123456789ABCDEF0123456789ABCDEF";
        U8 i;

        UART_DebugPrintf(("Sending %d bytes: '%s'.\n",
                          strlen(TestMessage),
                          TestMessage));

        /* Try a read/write cycle */
        for (i = 0; i < strlen(TestMessage); i++)
        {
            U32 n;
            error = STUART_Write(Handle,
                                 (U8 *)(TestMessage+i),
                                 1,
                                 &n,
                                 TxTimeout
                                );
            NumberWritten += n;
        }

        UART_DebugError("STUART_Write()", error);
        UART_DebugPrintf(("Written %d bytes.\n", NumberWritten));

        /* Check all bytes were successfully written */
        if (NumberWritten != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to write all bytes");
            goto api_end;
        }

        error = STUART_Read(Handle,
                            RxBuf,
                            strlen(TestMessage),
                            &NumberRead,
                            RxTimeout
                           );
        UART_DebugError("STUART_Read()", error);
        UART_DebugPrintf(("Read %d bytes = '%s'.\n",NumberRead, RxBuf));

        /* Check all bytes were successfully read */
        if (NumberRead != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to read all bytes");
            goto api_end;
        }
        UART_TestPassed(Result);
    }

#ifdef STUART_STATISTICS
    /* STUART_GetStatistics() */
    {
        STUART_Statistics_t Statistics;

        error = STUART_GetStatistics(Handle, &Statistics);
        UART_DebugError("STUART_GetStatistics()", error);
        UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
        UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
        UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
        UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
        UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));
    }
#endif /* STUART_STATISTICS */

api_end:
    /* STUART_Close() */
    UART_DebugPrintf(("%d.3: STUART_Close()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be closed.");
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result, "Unable to close UART");
        }
        else
        {
            UART_TestPassed(Result);
        }

    }

    /* STUART_Term() */
    UART_DebugPrintf(("%d.4: STUART_Term()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be terminated.");
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result, "Unable to terminate UART");
        }
        else
        {
            UART_TestPassed(Result);
        }

    }

    return Result;
}

static UART_TestResult_t UART_TestTx2(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    WaitBlockedSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* STUART_Init() */
    UART_DebugPrintf(("%d.0: STUART_Init()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be initialized.");
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto api_end;
        }
        UART_TestPassed(Result);
    }

    /* STUART_Open() */
    UART_DebugPrintf(("%d.1: STUART_Open()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be opened.");
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 9600;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = ReadNotifyRoutine;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 9600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = WriteNotifyRoutine;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto api_end;
        }
        UART_TestPassed(Result);
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 1000;

    /* Loopback */
    UART_DebugPrintf(("%d.2: Loopback\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART can write & read data.");
    {
        U32 NumberRead = 0;
        U32 NumberWritten = 0;
        U8 RxBuf[100] = { 0 };
        char TestMessage[] = "0123456789ABCDEF0123456789ABCDEF";
        U8 i;
        U32 n;

        UART_DebugPrintf(("Sending %d bytes: '%s'.\n",
                          strlen(TestMessage),
                          TestMessage));

        /* Try a write cycle */
        for (i = 0; i < strlen(TestMessage) &&
                 error == ST_NO_ERROR; i++)
        {
            error = STUART_Write(Handle,
                                 (U8 *)&TestMessage[i],
                                 1,
                                 &n,
                                 TxTimeout
                                );
            if (error == ST_NO_ERROR)
            {
                STOS_SemaphoreWait(WaitBlockedSemaphore_p);
                NumberWritten += n;
                if (WriteErrorCode != ST_NO_ERROR)
                    error = WriteErrorCode;
            }
        }

        UART_DebugError("STUART_Write()", error);
        UART_DebugPrintf(("Written %d bytes.\n", NumberWritten));

        /* Check all bytes were successfully written */
        if (NumberWritten != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to write all bytes");
            goto api_end;
        }

        for (i = 0; i < strlen(TestMessage) &&
                 error == ST_NO_ERROR; i++)
        {
            error = STUART_Read(Handle,
                                RxBuf+i,
                                1,
                                &n,
                                RxTimeout
                               );
            if (error == ST_NO_ERROR)
            {
                STOS_SemaphoreWait(WaitBlockedSemaphore_p);
                NumberRead += n;
                if (ReadErrorCode != ST_NO_ERROR)
                    error = ReadErrorCode;
            }
        }

        UART_DebugError("STUART_Read()", error);
        UART_DebugPrintf(("Read %d bytes = '%s'.\n",NumberRead, RxBuf));

        /* Check all bytes were successfully read */
        if (NumberRead != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to read all bytes");
            goto api_end;
        }
        UART_TestPassed(Result);
    }

#ifdef STUART_STATISTICS
    /* STUART_GetStatistics() */
    {
        STUART_Statistics_t Statistics;

        error = STUART_GetStatistics(Handle, &Statistics);
        UART_DebugError("STUART_GetStatistics()", error);
        UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
        UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
        UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
        UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
        UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));
    }
#endif /* STUART_STATISTICS */

api_end:

    STOS_SemaphoreDelete(NULL,WaitBlockedSemaphore_p);

    /* STUART_Close() */
    UART_DebugPrintf(("%d.3: STUART_Close()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be closed.");
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result, "Unable to close UART");
        }
        else
        {
            UART_TestPassed(Result);
        }

    }

    /* STUART_Term() */
    UART_DebugPrintf(("%d.4: STUART_Term()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be terminated.");
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result, "Unable to terminate UART");
        }
        else
        {
            UART_TestPassed(Result);
        }

    }

    return Result;
}

#ifdef INTERACTIVE
static UART_TestResult_t UART_TestErrors(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U32 RxTimeout;
    U8 ASCNumber = TestParams->ASCNumber;

    /* STUART_Init() */
    UART_DebugPrintf(("%d.0: STUART_Init()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure UART can be initialized.");
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto end_overflow;
        }
        UART_TestPassed(Result);
    }

    /* STUART_Open() */
    UART_DebugPrintf(("%d.1: STUART_Open()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure UART device can be opened.");
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_2_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 57600;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_2_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 57600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto end_overflow;
        }
        UART_TestPassed(Result);
    }

    /* Timeout Params */
    RxTimeout = 10000;

    /* Check received data */
    UART_DebugPrintf(("%d.2: Receive data on UART.\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Check data received on UART.");
    {
        U16 i;
        U8 Buf;
        U32 NumberRead;

        UART_DebugMessage("Checking data received....");

        for (i = 0; i < 256; i++)
        {
            /* Try a read/write cycle */
            error = STUART_Read(Handle,
                                &Buf,
                                1,
                                &NumberRead,
                                RxTimeout
                               );
            UART_DebugError("STUART_Read()", error);

            if (NumberRead == 1)
            {
                UART_DebugPrintf(("--- 0x%02x [ %c ]\n", Buf, Buf));
            }
            else if (error == ST_ERROR_TIMEOUT)
            {
                break;
            }
        }

        UART_TestPassed(Result);
    }

#ifdef STUART_STATISTICS
    /* STUART_GetStatistics() */
    {
        STUART_Statistics_t Statistics;

        error = STUART_GetStatistics(Handle, &Statistics);
        UART_DebugError("STUART_GetStatistics()", error);
        UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
        UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
        UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
        UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
        UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));
    }
#endif /* STUART_STATISTICS */

end_overflow:

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = TRUE;
        STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
    }

    return Result;
}
#endif

static UART_TestResult_t UART_Test9BitMode(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* STUART_Init() */
    UART_DebugPrintf(("%d.0: STUART_Init()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be initialized.");
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto end_9bit;
        }
        UART_TestPassed(Result);
    }

    /* STUART_Open() */
    UART_DebugPrintf(("%d.1: STUART_Open()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be opened.");
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_9BITS_NO_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 38400;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_9BITS_NO_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 38400;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto end_9bit;
        }
        UART_TestPassed(Result);
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 1000;

    /* Loopback */
    UART_DebugPrintf(("%d.2: 9 bit mode loopback\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART can write & read data in 9-bit mode.");
    {
        U32 NumberRead;
        U32 NumberWritten;
        U16 i, j;
        U8 RxBuf[2];

        /* The 9th data bit is tested by transmitting 8-bit and 9-bit numbers i.e.,
         * 0 <= x < 256 => 8 bit range,
         * 256 <= x < 512 => 9 bit range.
         */
        UART_DebugMessage("Sending incrementing 9-bit counter....");

        for (i = 0; i < 512; i++)
        {
            /* Try a read/write cycle */
            error = STUART_Write(Handle,
                                 (U8 *)&i,
                                 2,
                                 &NumberWritten,
                                 TxTimeout
                                );

            /* Check all bytes were successfully written */
            if (NumberWritten != 2 || error != ST_NO_ERROR)
            {
                break;
            }

            error = STUART_Read(Handle,
                                RxBuf,
                                2,
                                &NumberRead,
                                RxTimeout
                               );

            if (NumberRead != 2 || error != ST_NO_ERROR)
            {
                break;
            }

            /* Calculate received counter value */
            j = RxBuf[1];
            j <<= 8;
            j |= RxBuf[0];

            /* Ensure received counter is same as transmitted counter value */
            if (j != i)
            {
                break;
            }
        }

        /* Ensure counter completed */
        if (i == 512)
        {
            UART_TestPassed(Result);
        }
        else
        {
            char buf[50];
            sprintf(buf, "Count failed at %d", i);
            UART_TestFailed(Result, buf);
        }
    }

#ifdef STUART_STATISTICS
    /* STUART_GetStatistics() */
    {
        STUART_Statistics_t Statistics;

        error = STUART_GetStatistics(Handle, &Statistics);
        UART_DebugError("STUART_GetStatistics()", error);
        UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
        UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
        UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
        UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
        UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));
    }
#endif /* STUART_STATISTICS */

end_9bit:
    /* STUART_Close() */
    UART_DebugPrintf(("%d.3: STUART_Close()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be closed.");
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result, "Unable to close UART");
        }
        else
        {
            UART_TestPassed(Result);
        }

    }

    /* STUART_Term() */
    UART_DebugPrintf(("%d.4: STUART_Term()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be terminated.");
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result, "Unable to terminate UART");
        }
        else
        {
            UART_TestPassed(Result);
        }

    }

    return Result;
}

/****************************************************************************
* Posix-like read function test
****************************************************************************/

UART_TestResult_t UART_PosixReadTest(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeOut, TxTimeOut;
    U8 OutBuff[32] = "Posix-like read test";
    U8 InBuff[32] = "                    ";
    U8 TermString[10] = "Adiorrr";
    U32 NumberWritten;
    U32 NumberRead;
    U32 TicksPerSecond = ST_GetClocksPerSecond();

    /* STUART_Init() */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 115200;
        UartDefaultParams.RxMode.TermString = (char *)TermString;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 115200;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = TRUE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
        }
    }

    RxTimeOut = 1000;
    TxTimeOut = 1000;
    /* Timeout Params */


    UART_DebugPrintf(("%d.0: Test with less than the minimum number of characters.\n", TestParams->Ref));
    error = STUART_Write(Handle, OutBuff, 5, &NumberWritten, TxTimeOut);
    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_Write()", error);
        UART_TestFailed(Result,"Unable to write to the UART device");
    }

    /* Wait to ensure we have all the characters transmitted */
    STOS_TaskDelay (TicksPerSecond/100);

    error = STUART_PosixRead(Handle, InBuff, 10, 20, &NumberRead, RxTimeOut);

    if (error != ST_ERROR_TIMEOUT)
    {
        UART_DebugError("STUART_PosixRead()", error);
        UART_DebugPrintf(("Expected %s\n",UART_ErrorString(ST_ERROR_TIMEOUT)));
        UART_TestFailed(Result,"Unable to read from the UART device");
    }

    if (NumberRead != 5)
    {
        UART_TestFailed(Result,"Bad number of characters read");
        UART_DebugPrintf(("Expected %i, received %i\n",5,NumberRead));
    }

    if ((error == ST_ERROR_TIMEOUT) && (NumberRead == 5))
    {
        UART_TestPassed(Result);
    }

    UART_DebugPrintf(("%d.1: Number of characters between min and max.\n", TestParams->Ref));
    error = STUART_Write(Handle, OutBuff, 15, &NumberWritten, TxTimeOut);
    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_Write()", error);
        UART_TestFailed(Result,"Unable to write to the UART device");
    }

    /* Wait to ensure we have all the characters transmitted */
    STOS_TaskDelay (TicksPerSecond/100);

    error = STUART_PosixRead(Handle, InBuff, 10, 20, &NumberRead, RxTimeOut);

    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_PosixRead()", error);
        UART_DebugPrintf(("Expected %s\n",UART_ErrorString(ST_NO_ERROR)));
        UART_TestFailed(Result,"Unable to read from the UART device");
    }

    if (NumberRead != 15)
    {
        UART_TestFailed(Result,"Bad number of characters read");
        UART_DebugPrintf(("Expected %i, received %i\n",15,NumberRead));
    }

    if ((error == ST_NO_ERROR) && (NumberRead == 15))
    {
        UART_TestPassed(Result);
    }

    UART_DebugPrintf(("%d.2: Number of characters bigger than max.\n", TestParams->Ref));
    error = STUART_Write(Handle, OutBuff, 25, &NumberWritten, TxTimeOut);
    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_Write()", error);
        UART_TestFailed(Result,"Unable to write to the UART device");
    }

    /* Wait to ensure we have all the characters transmitted */
    STOS_TaskDelay (TicksPerSecond/100);

    error = STUART_PosixRead(Handle, InBuff, 10, 20, &NumberRead, RxTimeOut);

    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_PosixRead()", error);
        UART_DebugPrintf(("Expected %s\n",UART_ErrorString(ST_NO_ERROR)));
        UART_TestFailed(Result,"Unable to read from the UART device");
    }

    if (NumberRead != 20)
    {
        UART_TestFailed(Result,"Bad number of characters read");
        UART_DebugPrintf(("Expected %i, received %i\n",20,NumberRead));
    }

    if ((error == ST_NO_ERROR) && (NumberRead == 20))
    {
        UART_TestPassed(Result);
    }

    error = STUART_PosixRead(Handle, InBuff, 10, 20, &NumberRead, RxTimeOut);

    UART_DebugPrintf(("%d.3: Termination string.\n", TestParams->Ref));
    error = STUART_Write(Handle, OutBuff, 5, &NumberWritten, TxTimeOut);
    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_Write()", error);
        UART_TestFailed(Result,"Unable to write to the UART device");
    }
    error = STUART_Write(Handle, TermString, 9, &NumberWritten, TxTimeOut);
    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_Write()", error);
        UART_TestFailed(Result,"Unable to write to the UART device");
    }
    error = STUART_Write(Handle, OutBuff, 5, &NumberWritten, TxTimeOut);
    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_Write()", error);
        UART_TestFailed(Result,"Unable to write to the UART device");
    }

    /* Wait to ensure we have all the characters transmitted */
    STOS_TaskDelay (TicksPerSecond/100);

    error = STUART_PosixRead(Handle, InBuff, 15, 20, &NumberRead, RxTimeOut);

    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_PosixRead()", error);
        UART_DebugPrintf(("Expected %s\n",UART_ErrorString(ST_NO_ERROR)));
        UART_TestFailed(Result,"Unable to read from the UART device");
    }

    if (NumberRead != 12)
    {
        UART_TestFailed(Result,"Bad number of characters read");
        UART_DebugPrintf(("Expected %i, received %i\n",12,NumberRead));
    }

    if ((error == ST_NO_ERROR) && (NumberRead == 12))
    {
        UART_TestPassed(Result);
    }

    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;
}

/****************************************************************************
* Test that the STUART_Close() function allow the last bytes in the hardware
 FIFO to be transmitted
****************************************************************************/

/****************************************************************************
* Test that the STUART_Close() function allow the last bytes in the hardware
 FIFO to be transmitted
****************************************************************************/

UART_TestResult_t UART_CloseTest(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t TxHandle, RxHandle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 TxASCNumber = ASC_DEVICE_DATA;
    U8 RxASCNumber = ASC_DEVICE_MODEM;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    U8 RxBuff[30] = "                    ";
    U8 TxBuff[30] = "abcdefghijklmnopqrst";
    U32 RxTimeOut, TxTimeOut;
    U32 NumberWritten;
    U32 NumberRead;

    /* Sender Init */
    {
        STUART_Device_t DeviceType;

        /* Setup Uart - note that we can't use the FIFOs */
        DeviceType = UartInitParams[TxASCNumber].UARTType;
        UartInitParams[TxASCNumber].UARTType = STUART_NO_HW_FIFO;
	 
        UART_DebugError("STUART_Init()", error);

        /* Restore previous setting */
        UartInitParams[TxASCNumber].UARTType = DeviceType;
    }
        error = STUART_Init(UartDeviceName[TxASCNumber],
                            &UartInitParams[TxASCNumber]);

    /* Receiver Init */
    {
        error = STUART_Init(UartDeviceName[RxASCNumber],
                            &UartInitParams[RxASCNumber]);
        UART_DebugError("STUART_Init()", error);
    }

    /* Sender STUART_Open() */
    if (error == ST_NO_ERROR)
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
#if defined(ST_7100) || defined(ST_7109) 
        UartDefaultParams.RxMode.BaudRate = 300;
#else
        UartDefaultParams.RxMode.BaudRate = 75;
#endif
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
#if defined(ST_7100) || defined(ST_7109) 
        UartDefaultParams.TxMode.BaudRate = 300;
#else
        UartDefaultParams.TxMode.BaudRate = 75;
#endif
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[TxASCNumber],
                            &UartOpenParams, &TxHandle);
        UART_DebugError("STUART_Open()", error);
    }

    /* Receiver STUART_Open() */
    if (error == ST_NO_ERROR)
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
#if defined(ST_7100) || defined(ST_7109) 
        UartDefaultParams.RxMode.BaudRate = 300;
#else
        UartDefaultParams.RxMode.BaudRate = 75;
#endif
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
#if defined(ST_7100)  || defined(ST_7109)
        UartDefaultParams.TxMode.BaudRate = 300;
#else
        UartDefaultParams.TxMode.BaudRate = 75;
#endif
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[RxASCNumber],
                            &UartOpenParams, &RxHandle);
        UART_DebugError("STUART_Open()", error);
    }

    /* Timeout Params */
#if defined(ST_7710) || defined(ST_7100)  || defined(ST_7109) /* need more time since timer is too fast */
    RxTimeOut = 0;
    TxTimeOut = 0;
#else
    RxTimeOut = 10000;
    TxTimeOut = 10000;
#endif


    UART_DebugPrintf(("%d.0: Test write and read\n", TestParams->Ref));
    error = STUART_Write(TxHandle, TxBuff, 16, &NumberWritten, TxTimeOut);
    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_Write()", error);
        UART_TestFailed(Result,"Unable to write to the UART device");
    }

    error = STUART_Close(TxHandle);
    UART_DebugError("STUART_Close()", error);

    error = STUART_Read(RxHandle, RxBuff, 16, &NumberRead, RxTimeOut);

    if (error != ST_NO_ERROR)
    {
        UART_DebugError("STUART_Read()", error);
        UART_DebugPrintf(("Expected %s\n",UART_ErrorString(ST_NO_ERROR)));
        UART_TestFailed(Result,"Unable to read from the UART device");
    }

    if (NumberRead != 16)
    {
        UART_TestFailed(Result,"Bad number of characters read");
        UART_DebugPrintf(("Expected %i, received %i\n",16,NumberRead));
    }

    if ((error == ST_NO_ERROR) && (NumberRead == 16))
    {
        UART_TestPassed(Result);
    }

    UART_DebugPrintf(("Received: '%s'\n",RxBuff));


    /* Tidy up test */

    /* STUART_Close() */
    {
        error = STUART_Close(RxHandle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[TxASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
        error = STUART_Term(UartDeviceName[RxASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;
}
static UART_TestResult_t UART_TestSimple(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle = 0;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* STUART_Init() */
    UART_DebugPrintf(("%d.0: STUART_Init()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be initialized.");
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto api_end;
        }
        UART_TestPassed(Result);
    }

    /* STUART_Open() */
    UART_DebugPrintf(("%d.1: STUART_Open()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be opened.");
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 38400;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 38400;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto api_end;
        }

        UART_TestPassed(Result);
    }

    RxTimeout = 1000;
    TxTimeout = 1000;

    /* Loopback */
    UART_DebugPrintf(("%d.2: Loopback\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART can write & read data.");
    {
        U32 NumberRead;
        U32 NumberWritten;
        U8 RxBuf[100] = { 0 };
        char TestMessage[] = "0123456789ABCDEF0123456789ABCDEF";
        UART_DebugPrintf(("Sending %d bytes: '%s'.\n",
                          strlen(TestMessage),
                          TestMessage));

        /* Try a read/write cycle */
        error = STUART_Write(Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );


        UART_DebugError("STUART_Write()", error);
        UART_DebugPrintf(("Written %d bytes.\n",NumberWritten));

        /* Check all bytes were successfully written */
        if (NumberWritten != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to write all bytes");
            goto api_end;
        }

        error = STUART_Read(Handle,
                            RxBuf,
                            strlen(TestMessage),
                            &NumberRead,
                            RxTimeout
                           );

        UART_DebugError("STUART_Read()", error);
        UART_DebugPrintf(("Read %d bytes = '%s'.\n",NumberRead, RxBuf));

        /* Check all bytes were successfully read */
        if (NumberRead != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to read all bytes");
            goto api_end;
        }

        UART_TestPassed(Result);
    }

#ifdef STUART_STATISTICS
    /* STUART_GetStatistics() */
    {
        STUART_Statistics_t Statistics;

        error = STUART_GetStatistics(Handle, &Statistics);
        UART_DebugError("STUART_GetStatistics()", error);
        UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
        UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
        UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
        UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
        UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));
    }
#endif /* STUART_STATISTICS */

api_end:
    /* STUART_Close() */
    UART_DebugPrintf(("%d.3: STUART_Close()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be closed.");
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result, "Unable to close UART");
        }
        else
        {
            UART_TestPassed(Result);
        }

    }

    /* STUART_Term() */
    UART_DebugPrintf(("%d.4: STUART_Term()\n", TestParams->Ref));
    UART_DebugMessage("Purpose: to ensure a UART device can be terminated.");
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to terminate UART");
        }
        else
        {
            UART_TestPassed(Result);
        }

    }

    return Result;
}

static UART_TestResult_t UART_TestInvalid(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto handle_end;
        }
    }

    /* Init twice */
    UART_DebugPrintf(("%d.0: Init UART twice\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To ensure multiple inits fail.");
    {
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_ERROR_ALREADY_INITIALIZED)
        {
            UART_TestFailed(Result,"Expected ST_ERROR_ALREADY_INITIALIZED");
            goto handle_end;
        }
        UART_TestPassed(Result);
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 38400;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 38400;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto handle_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 1000;


#if 0 /* This test is no more valid, second Open with same device name will return
         same handle - DDTS 35847 */
    /* Try multiple open on the same device */
    UART_DebugPrintf(("%d.2: Open UART twice\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To ensure multiple opens fail.");
    {
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_ERROR_NO_FREE_HANDLES)
        {
            UART_TestFailed(Result,"Expected ST_ERROR_NO_FREE_HANDLES");
            goto handle_end;
        }
        else
        {
            UART_TestPassed(Result);
        }
    }
#endif

    UART_DebugPrintf(("%d.3: Open uninitialized UART\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To ensure uninit'ed opens fail.");
    {
        error = STUART_Open("plop", &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_ERROR_UNKNOWN_DEVICE)
        {
            UART_TestFailed(Result,"Expected ST_ERROR_UNKNOWN_DEVICE");
        }
        else
        {
            UART_TestPassed(Result);
        }
    }

    /* Invalid handle */
    UART_DebugPrintf(("%d.4: Read/Write invalid handle\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check these calls detect an invalid handle.");
    {
        U32 NumberRead;
        U32 NumberWritten;
        U8 RxBuf[100] = { 0 };
        char TestMessage[] = "0123456789ABCDEF0123456789ABCDEF";

        /* Try a read/write cycle */
        error = STUART_Write(0,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );
        UART_DebugError("STUART_Write()", error);

        if (error != ST_ERROR_INVALID_HANDLE)
        {
            UART_TestFailed(Result,"Should be ST_ERROR_INVALID_HANDLE");
            goto handle_end;
        }

        error = STUART_Read(0,
                            RxBuf,
                            strlen(TestMessage),
                            &NumberRead,
                            RxTimeout
                           );
        UART_DebugError("STUART_Read()", error);

        if (error != ST_ERROR_INVALID_HANDLE)
        {
            UART_TestFailed(Result,"Should be ST_ERROR_INVALID_HANDLE");
            goto handle_end;
        }
        UART_TestPassed(Result);
    }

    /* Invalid handle: Close */
    UART_DebugPrintf(("%d.5: Close invalid handle\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check call detects an invalid handle.");
    {
        error = STUART_Close(0);
        UART_DebugError("STUART_Close()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            UART_TestFailed(Result,"Should be ST_ERROR_INVALID_HANDLE");
            goto handle_end;
        }
        UART_TestPassed(Result);
    }

    /* Invalid handle: Get/Set Params */
    UART_DebugPrintf(("%d.6: Get/Set Params invalid handle\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check calls detect an invalid handle.");
    {
        error = STUART_GetParams(0, NULL);
        UART_DebugError("STUART_GetParams()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            UART_TestFailed(Result,"Should be ST_ERROR_INVALID_HANDLE");
            goto handle_end;
        }

        error = STUART_SetParams(0, NULL);
        UART_DebugError("STUART_SetParams()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            UART_TestFailed(Result,"Should be ST_ERROR_INVALID_HANDLE");
            goto handle_end;
        }
        UART_TestPassed(Result);
    }

    /* Invalid handle: Pause */
    UART_DebugPrintf(("%d.7: Pause invalid handle\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check call detects an invalid handle.");
    {
        error = STUART_Pause(0, STUART_DIRECTION_BOTH);
        UART_DebugError("STUART_Pause()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            UART_TestFailed(Result,"Should be ST_ERROR_INVALID_HANDLE");
            goto handle_end;
        }
        UART_TestPassed(Result);
    }

    /* Invalid handle: Abort */
    UART_DebugPrintf(("%d.8: Abort invalid handle\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check call detects an invalid handle.");
    {
        error = STUART_Abort(0, STUART_DIRECTION_BOTH);
        UART_DebugError("STUART_Abort()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            UART_TestFailed(Result,"Should be ST_ERROR_INVALID_HANDLE");
            goto handle_end;
        }
        UART_TestPassed(Result);
    }

    /* Invalid handle: Flush */
    UART_DebugPrintf(("%d.9: Flush invalid handle\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check call detects an invalid handle.");
    {
        error = STUART_Flush(0);
        UART_DebugError("STUART_Flush()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            UART_TestFailed(Result,"Should be ST_ERROR_INVALID_HANDLE");
            goto handle_end;
        }
        UART_TestPassed(Result);
    }

    UART_DebugPrintf(("%d.10: Close twice\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Ensure device can only be closed once.");
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to close device");
            goto handle_end;
        }
        UART_DebugMessage("Attempting close with same handle...");
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            UART_TestFailed(Result,"Expected ST_ERROR_INVALID_HANDLE");
            goto handle_end;
        }
        UART_TestPassed(Result);
    }

    /* Tidy up */
handle_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;
        error = STUART_Term(UartDeviceName[ASCNumber],
                            &UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;
}


/***************************************************************************
                 Test Memomy Leak
****************************************************************************/
static UART_TestResult_t UART_TestMemoryLeak(UART_TestParams_t *TestParams)
{

    ST_ErrorCode_t error = ST_NO_ERROR;
    U8 ASCNumber = TestParams->ASCNumber;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    partition_status_t  pstatus;
    U32   PrevMem,AfterMem;

    if (error == partition_status(system_partition, &pstatus, 0))
    {

        PrevMem = pstatus.partition_status_free;

        /* Init */
        {
            /* Setup Uart */
            error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
            UART_DebugError("STUART_Init()", error);

       }

        if (error == ST_NO_ERROR)
        {

            /* STUART_Term() */
            {
                STUART_TermParams_t UartTermParams;
                UartTermParams.ForceTerminate = FALSE;

                error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
                UART_DebugError("STUART_Term()", error);
            }
            if (error == ST_NO_ERROR)
            {
                if (error == partition_status(system_partition, &pstatus, 0))
                {
                    AfterMem = pstatus.partition_status_free;
                    if (PrevMem == AfterMem)
                    {
                        UART_TestPassed(Result);
                    }
                    else
                    {
                        UART_TestFailed(Result,"Memory Leak Failed");
                    }
                }
                else
                {
                    UART_TestFailed(Result,"partition_status FAILED");
                }
            }
            else
            {
                UART_TestFailed(Result,"Unable to terminate UART device");
            }

        }
        else
        {
            UART_TestFailed(Result,"Unable to initialise UART device");
        }
    }
    else
    {
        UART_TestFailed(Result,"partition_status FAILED");
    }

    return Result;
}

/* Outputs a swirl pattern across the screen */
static UART_TestResult_t UART_TestSwirl(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* STUART_Init() */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto swirl_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = LOOPBACK_BAUDRATE;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = LOOPBACK_BAUDRATE;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto swirl_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 1000;


    /* Loopback */
    UART_DebugPrintf(("%d.0: Swirl test\n", TestParams->Ref));
    UART_DebugMessage("Purpose: test the driver under intense load.");
    {
        error = UART_Loopback(255, 128, Handle, RxTimeout, TxTimeout);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Data error");
            goto swirl_end;
        }
    }
    UART_TestPassed(Result);

#ifdef STUART_STATISTICS
    /* STUART_GetStatistics() */
    {
        STUART_Statistics_t Statistics;

        error = STUART_GetStatistics(Handle, &Statistics);
        UART_DebugError("STUART_GetStatistics()", error);
        UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
        UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
        UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
        UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
        UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));

        /* Check tx bytes == rx bytes */
        if (Statistics.NumberBytesReceived !=
            Statistics.NumberBytesTransmitted)
        {
            UART_TestFailed(Result, "Failed to read all transmitted bytes");
        }
        else
        {
            UART_TestPassed(Result);
        }
    }
#endif /* STUART_STATISTICS */

swirl_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;
}



/*
  SmartCardConfig Test
  - Simple test to set values to NACKDisable, GuardTime and SCEnble to ensure
    1) nothing was disturbed when new features where added,
    2) Features accessible for defined device type and not for others
    3) the expected value can be set to the desired register/bit.
    NOTE: Not a functional test, just register comparison with expected value.
*/
static UART_TestResult_t UART_TestSmartCardRegConfig(UART_TestParams_t *TestParams)
{
#define CONTROL_REG_OFFSET      0x03
#define GUARDTIME_REG_OFFSET    0x06
#define BIT_CONTROL_NACKDISABLE 0x2000
#define BIT_CONTROL_SCENABLE    0x200
#define BIT_CONTROL_HWFIFODISABLE    0x400
#define BIT_CONTROL_RXENABLE   0x100
    ST_ErrorCode_t error;
    STUART_Handle_t Handle = 0;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    STUART_TermParams_t UartTermParams;
    U8 TestASCNumber;
    STUART_Device_t DeviceType;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STSYS_DU32 RefControl, RefGuardTime, CompControl, CompGuardTime;
    BOOL CompDiff = FALSE;

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) \
|| defined(ST_5100) || defined(ST_7710) || defined(ST_5525) /* test ISO7816 device type */

    U8 TxBuf[1];
    ST_ErrorCode_t error_a;
    U32 NumberWritten;

    /* Setup UART for ISO7816 device type */
    UART_DebugMessage("Initialising a STUART_ISO7816 type device");

    UART_DebugMessage("Starting with known good, default parameters");

    TestASCNumber = ASC_DEVICE_DATA;

    /* Default params setup */
    UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    UartDefaultParams.RxMode.StopBits = STUART_STOP_1_5;
    UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
    UartDefaultParams.RxMode.BaudRate = 300;
    UartDefaultParams.RxMode.TermString = NULL;
    UartDefaultParams.RxMode.NotifyFunction = NULL;
    UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    UartDefaultParams.TxMode.StopBits = STUART_STOP_1_5;
    UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
    UartDefaultParams.TxMode.BaudRate = 300;
    UartDefaultParams.TxMode.TermString = NULL;
    UartDefaultParams.TxMode.NotifyFunction = NULL;
    UartDefaultParams.SmartCardModeEnabled = TRUE;
    UartDefaultParams.GuardTime = 2;
    UartDefaultParams.Retries = 3;
    UartDefaultParams.NACKSignalDisabled = FALSE;
    UartDefaultParams.HWFifoDisabled = FALSE;

    /* Init params adjustment */
    DeviceType = UartInitParams[TestASCNumber].UARTType;
    UartInitParams[TestASCNumber].UARTType = STUART_ISO7816;

    /* Init with non-null default params */
    UartInitParams[TestASCNumber].DefaultParams = &UartDefaultParams;
    error = STUART_Init(UartDeviceName[TestASCNumber],
                        &UartInitParams[TestASCNumber]);
    UART_DebugError("STUART_Init()", error);

    /* Restore previous setting */
    UartInitParams[TestASCNumber].UARTType = DeviceType;
    UartInitParams[TestASCNumber].DefaultParams = NULL;

    /* All ok, so open driver */
    if (error == ST_NO_ERROR)
    {
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[TestASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
    }


    /* Obtain test reference values */
    if (error == ST_NO_ERROR)
    {

        RefControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                         + CONTROL_REG_OFFSET) & 0x0003FFF);
        RefGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress) +
                           GUARDTIME_REG_OFFSET)& 0x00001FF);

        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);


    }

    /* RegTest a: Should change the value of NACKDisable and GuardTime */
    if (error == ST_NO_ERROR)
    {
        UART_DebugMessage("Attempting to set NACKDisable TRUE, Guardtime 255");
        UartDefaultParams.NACKSignalDisabled = TRUE;
        UartDefaultParams.GuardTime = 255;

        error = STUART_Open(UartDeviceName[TestASCNumber], &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);

        if (error == ST_NO_ERROR)
        {
            CompControl = (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                              + CONTROL_REG_OFFSET) & 0x0003FFF);
            CompGuardTime = (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                + GUARDTIME_REG_OFFSET )& 0x00001FF);

            if (CompControl != (RefControl | BIT_CONTROL_NACKDISABLE))
            {
                UART_DebugMessage("****Comparison failed!!");
                UART_DebugPrintf(("Control: %x\n",CompControl));
                CompDiff = TRUE;
            }

            if (CompGuardTime != UartDefaultParams.GuardTime)
            {
                UART_DebugMessage("****Comparison failed!!");
                UART_DebugPrintf(("GuardTime: %d \n",CompGuardTime));
                CompDiff = TRUE;
            }

            RefGuardTime = CompGuardTime;

            error = STUART_Close(Handle);
            UART_DebugError("STUART_Close()", error);
        }
    }


    /* RegTest b: Should change the value NACKDisable and GuardTime */
    if (error == ST_NO_ERROR)
    {
        UART_DebugMessage("Attempting to set NACKDisable FALSE, Guardtime 255");
        UartDefaultParams.NACKSignalDisabled = FALSE;
        UartDefaultParams.GuardTime = 255;

        error = STUART_Open(UartDeviceName[TestASCNumber], &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);

        if (error == ST_NO_ERROR)
        {
            CompControl = (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                              + CONTROL_REG_OFFSET) & 0x0003FFF);
            CompGuardTime = (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                + GUARDTIME_REG_OFFSET)& 0x00001FF);

            if (CompControl != RefControl)
            {
                UART_DebugMessage("****Comparison failed!!");
                UART_DebugPrintf(("Control: %x\n",CompControl));
                UART_DebugPrintf(("Expected: %x\n",RefControl));
                CompDiff = TRUE;

            }
            if (CompGuardTime != UartDefaultParams.GuardTime)
            {
                UART_DebugMessage("****Comparison failed!!");
                UART_DebugPrintf(("GuardTime: %d \n",CompGuardTime));
                CompDiff = TRUE;
            }

            RefGuardTime = CompGuardTime;

            error = STUART_Close(Handle);
            UART_DebugError("STUART_Close()", error);
        }
    }

    /* RegTest c: Should change HWFifoDisabled */
    if (error == ST_NO_ERROR)
    {
        UART_DebugMessage("Attempting to set HWFifoDisable TRUE");
        UartDefaultParams.HWFifoDisabled = TRUE;
        error = STUART_Open(UartDeviceName[TestASCNumber], &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);

        if (error == ST_NO_ERROR)
        {
            CompControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                              + CONTROL_REG_OFFSET) & 0x0003FFF);
            CompGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                + GUARDTIME_REG_OFFSET)& 0x00001FF);

            if (CompControl != (RefControl & (~BIT_CONTROL_HWFIFODISABLE)))
            {
                UART_DebugMessage("****Comparison failed!!");
                UART_DebugPrintf(("Control: %x\n",CompControl));
                CompDiff = TRUE;
            }
            if (CompGuardTime != RefGuardTime)
            {
                UART_DebugMessage("*****Comparison failed!!");
                UART_DebugPrintf(("GuardTime: %d \n",CompGuardTime));
                CompDiff = TRUE;
            }

            error = STUART_Close(Handle);
            UART_DebugError("STUART_Close()", error);
        }
    }

    /* RegTest d: Should only change HWFifoDisabled */
    if (error == ST_NO_ERROR)
    {
        UART_DebugMessage("Attempting to set HWFifoDisable FALSE");
        UartDefaultParams.HWFifoDisabled = FALSE;
        error = STUART_Open(UartDeviceName[TestASCNumber], &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);

        if (error == ST_NO_ERROR)
        {
            CompControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                              + CONTROL_REG_OFFSET) & 0x0003FFF);
            CompGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                + GUARDTIME_REG_OFFSET)& 0x00001FF);

            if (CompControl != (RefControl | (BIT_CONTROL_HWFIFODISABLE)))
            {
                UART_DebugMessage("****Comparison failed!!");
                UART_DebugPrintf(("Control: %x\n",CompControl));
                CompDiff = TRUE;
            }
            if (CompGuardTime != RefGuardTime)
            {
                UART_DebugMessage("*****Comparison failed!!");
                UART_DebugPrintf(("GuardTime: %d \n",CompGuardTime));
                CompDiff = TRUE;
            }

            error = STUART_Close(Handle);
            UART_DebugError("STUART_Close()", error);
        }
    }


    /* RegTest e: Should only change value of SmartCardEnabled. Not other changes */
    if (error == ST_NO_ERROR)
    {
        UART_DebugMessage("Attempting to set SCEnable FALSE, NACKDisable TRUE, Guardtime 2");
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.NACKSignalDisabled = TRUE;
        UartDefaultParams.GuardTime = 2;

        error = STUART_Open(UartDeviceName[TestASCNumber], &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);

        if (error == ST_NO_ERROR)
        {
            CompControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                              + CONTROL_REG_OFFSET) & 0x0003FFF);
            CompGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                + GUARDTIME_REG_OFFSET)& 0x00001FF);

            if (CompControl != (RefControl & (~BIT_CONTROL_SCENABLE)))
            {
                UART_DebugMessage("****Comparison failed!!");
                UART_DebugPrintf(("Control: %x\n",CompControl));
                CompDiff = TRUE;
            }
            if (CompGuardTime != RefGuardTime)
            {
                UART_DebugMessage("*****Comparison failed!!");
                UART_DebugPrintf(("GuardTime: %d \n",CompGuardTime));
                CompDiff = TRUE;
            }

            error = STUART_Close(Handle);
            UART_DebugError("STUART_Close()", error);
        }
    }


    /* RegTest f: Should change NACKDisable and GuardTime with STUART_SetParams call */
    if (error == ST_NO_ERROR)
    {
        UART_DebugMessage("Opening with SCEnable TRUE, NACKDisable FALSE, Guardtime 2");
        UartDefaultParams.SmartCardModeEnabled = TRUE;
        UartDefaultParams.NACKSignalDisabled = FALSE;
        UartDefaultParams.GuardTime = 2;
        error = STUART_Open(UartDeviceName[TestASCNumber], &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);

        if (error == ST_NO_ERROR)
        {
            RefControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                              + CONTROL_REG_OFFSET) & 0x0003FFF);
            RefGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                + GUARDTIME_REG_OFFSET)& 0x00001FF);

            UART_DebugMessage("SetParams NACKDisable TRUE, Guardtime 255");
            UartDefaultParams.NACKSignalDisabled = TRUE;
            UartDefaultParams.GuardTime = 255;
            error = STUART_SetParams(Handle,&UartDefaultParams);
            UART_DebugError("STUART_SetParams()", error);

            if (error == ST_NO_ERROR)
            {
                /* Perform a write to action SetParams */
                error = STUART_Write(Handle, TxBuf, 1, &NumberWritten, 1000);
                UART_DebugError("STUART_Write()", error);
            }

            if (error ==ST_NO_ERROR)
            {
                CompControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                              + CONTROL_REG_OFFSET) & 0x0003FFF);
                CompGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                                + GUARDTIME_REG_OFFSET) & 0x00001FF);

                if (CompControl != (RefControl | BIT_CONTROL_NACKDISABLE))
                {
                    UART_DebugMessage("****Comparison failed!!");
                    UART_DebugPrintf(("Control: %x\n",CompControl));
                    UART_DebugPrintf(("Expected: %x\n",RefControl));
                    CompDiff = TRUE;
                }
                if (CompGuardTime != UartDefaultParams.GuardTime)
                {
                    UART_DebugMessage("*****Comparison failed!!");
                    UART_DebugPrintf(("GuardTime: %d \n",CompGuardTime));
                    CompDiff = TRUE;
                }
            }

            error = STUART_Close(Handle);
            UART_DebugError("STUART_Close()", error);
        }
    }

    /* Tidy up: Leave NACK disable in FALSE and GuardTime at 2 */
    UART_DebugMessage("Tidying up UART control regsiter");
    UartDefaultParams.SmartCardModeEnabled = TRUE;
    UartDefaultParams.NACKSignalDisabled = FALSE;
    UartDefaultParams.GuardTime = 2;
    error_a = STUART_Open(UartDeviceName[TestASCNumber], &UartOpenParams, &Handle);
    UART_DebugError("STUART_Open()", error_a);
    error_a = STUART_Close(Handle);
    UART_DebugError("STUART_Close()", error_a);
    UartTermParams.ForceTerminate = TRUE;
    error_a = STUART_Term(UartDeviceName[TestASCNumber],&UartTermParams);
    UART_DebugError("STUART_Term()", error_a);

#endif /* ST_5514/16/17/28 */

    UART_DebugMessage("Initialising a NON STUART_ISO7816 type device");

    UART_DebugMessage("Starting with known good parameters");
    TestASCNumber = ASC_DEVICE_MODEM;
    /* Default params setup */
    UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    UartDefaultParams.RxMode.StopBits = STUART_STOP_1_5;
    UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
    UartDefaultParams.RxMode.BaudRate = 300;
    UartDefaultParams.RxMode.TermString = NULL;
    UartDefaultParams.RxMode.NotifyFunction = NULL;
    UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    UartDefaultParams.TxMode.StopBits = STUART_STOP_1_5;
    UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
    UartDefaultParams.TxMode.BaudRate = 300;
    UartDefaultParams.TxMode.TermString = NULL;
    UartDefaultParams.TxMode.NotifyFunction = NULL;
    UartDefaultParams.SmartCardModeEnabled = TRUE;
    UartDefaultParams.GuardTime = 2;
    UartDefaultParams.Retries = 3;
    UartDefaultParams.NACKSignalDisabled = FALSE;
    UartDefaultParams.HWFifoDisabled = FALSE;

    /* Init params adjustment */
    DeviceType = UartInitParams[TestASCNumber].UARTType;
    UartInitParams[TestASCNumber].UARTType = STUART_RTSCTS;

    /* Init with non-null default params */
    UartInitParams[TestASCNumber].DefaultParams = &UartDefaultParams;
    error = STUART_Init(UartDeviceName[TestASCNumber],
                        &UartInitParams[TestASCNumber]);
    UART_DebugError("STUART_Init()", error);

    /* Restore previous setting */
    UartInitParams[TestASCNumber].UARTType = DeviceType;
    UartInitParams[TestASCNumber].DefaultParams = NULL;

    /* All ok, so open driver */
    if (error == ST_NO_ERROR)
    {
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[TestASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
    }

    /* Obtain test reference values */
    if (error == ST_NO_ERROR)
    {
        RefControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                         + CONTROL_REG_OFFSET) & 0x0003FFF);
        RefGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress) +
                           GUARDTIME_REG_OFFSET)& 0x00001FF);

        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);

    }


    /* RegTest g: Nothing should change. BAD_PARAMETER error expected */
    if (error == ST_NO_ERROR)
    {
        UART_DebugMessage("Attempting to set NACK TRUE, GuardTime to 256");
        UartDefaultParams.NACKSignalDisabled = TRUE;
        UartDefaultParams.GuardTime = 256;

        error = STUART_Open(UartDeviceName[TestASCNumber], &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);

        if (error == ST_ERROR_BAD_PARAMETER)
        {
            /* BAD_PARAMETER is expected, clear error flag for success */
            error = ST_NO_ERROR;
        }
    }

    /* RegTest h: Nothing should change. NO_ERROR  expected */
    if (error == ST_NO_ERROR)
    {
        UART_DebugMessage("Attempting to set HWFifoDisabled TRUE");
        UartDefaultParams.NACKSignalDisabled = FALSE;
        UartDefaultParams.GuardTime = 2;
        UartDefaultParams.HWFifoDisabled = TRUE;

        error = STUART_Open(UartDeviceName[TestASCNumber], &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);

        if (error == ST_NO_ERROR)
        {
            error = ST_NO_ERROR;
            CompControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                              + CONTROL_REG_OFFSET) & 0x0003FFF);
            CompGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                + GUARDTIME_REG_OFFSET)& 0x00001FF);

            if (CompControl != RefControl)
            {
                UART_DebugMessage("****Comparison failed!!");
                UART_DebugPrintf(("Control: %x\n",CompControl));
                UART_DebugPrintf(("Expected: %x\n",RefControl));
                CompDiff = TRUE;
            }
            if (CompGuardTime != RefGuardTime)
            {
                UART_DebugMessage("****Comparison failed!!");
                UART_DebugPrintf(("GuardTime: %d \n",CompGuardTime));
                CompDiff = TRUE;
            }

            error = STUART_Close(Handle);
            UART_DebugError("STUART_Close()", error);
        }
    }



    /* RegTest i: Expect NOT_SUPPORTED  from SetParams function */
    if (error == ST_NO_ERROR)
    {
        UART_DebugMessage("Opening with SCEnable TRUE, NACKDisable FALSE, Guardtime 2");
        UartDefaultParams.SmartCardModeEnabled = TRUE;
        UartDefaultParams.NACKSignalDisabled = FALSE;
        UartDefaultParams.GuardTime = 2;
        error = STUART_Open(UartDeviceName[TestASCNumber], &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);

        if (error == ST_NO_ERROR)
        {
            RefControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                              + CONTROL_REG_OFFSET) & 0x0003FFF);
            RefGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                + GUARDTIME_REG_OFFSET)& 0x00001FF);

            UART_DebugMessage("Attempting SetParams NACKDisabled TRUE, GuardTime 1");
            UartDefaultParams.NACKSignalDisabled = TRUE;
            UartDefaultParams.GuardTime = 1;
            error = STUART_SetParams(Handle,&UartDefaultParams);
            UART_DebugError("STUART_SetParams()", error);

            if (error == ST_ERROR_BAD_PARAMETER)
            {
                error = ST_NO_ERROR;

                CompControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                              + CONTROL_REG_OFFSET) & 0x0003FFF);
                CompGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                                + GUARDTIME_REG_OFFSET)& 0x00001FF);

                if (CompControl != RefControl)
                {
                    UART_DebugMessage("****Comparison failed!!");
                    UART_DebugPrintf(("Control: %x\n",CompControl));
                    UART_DebugPrintf(("Expected: %x\n",RefControl));
                    CompDiff = TRUE;
                }
                if (CompGuardTime != RefGuardTime)
                {
                    UART_DebugMessage("*****Comparison failed!!");
                    UART_DebugPrintf(("GuardTime: %d \n",CompGuardTime));
                    CompDiff = TRUE;
                }
            }

            UART_DebugMessage("Attempting SetParams NACKDisable FALSE, GuardTime 255");
            UartDefaultParams.NACKSignalDisabled = FALSE;
            UartDefaultParams.GuardTime = 255;
            error = STUART_SetParams(Handle,&UartDefaultParams);
            UART_DebugError("STUART_SetParams()", error);

            if (error == ST_ERROR_BAD_PARAMETER)
            {
                error = ST_NO_ERROR;

                CompControl =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                              + CONTROL_REG_OFFSET) & 0x0003FFF);
                CompGuardTime =  (STSYS_DU32)(*((UartInitParams[TestASCNumber].BaseAddress)
                                                + GUARDTIME_REG_OFFSET)& 0x00001FF);

                if (CompControl != RefControl)
                {
                    UART_DebugMessage("****Comparison failed!!");
                    UART_DebugPrintf(("Control: %x\n",CompControl));
                    UART_DebugPrintf(("Expected: %x\n",RefControl));
                    CompDiff = TRUE;
                }
                if (CompGuardTime != RefGuardTime)
                {
                    UART_DebugMessage("*****Comparison failed!!");
                    UART_DebugPrintf(("GuardTime: %d \n",CompGuardTime));
                    CompDiff = TRUE;
                }
            }

            error = STUART_Close(Handle);
            UART_DebugError("STUART_Close()", error);
        }
    }

    /* Terminate driver */
    if (error == ST_NO_ERROR)
    {
        UartTermParams.ForceTerminate = TRUE;
        error = STUART_Term(UartDeviceName[TestASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    if (error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "API funtion call returned unexpected error code");
    }
    else
    {
        if (CompDiff == TRUE)
        {
            UART_TestFailed(Result, "Register had unexepected values");
        }
        else
        {
            UART_TestPassed(Result);
        }
    }

    return Result;
}

/* Abort test */

void AbortRxNotifyRoutine(STUART_Handle_t Handle, ST_ErrorCode_t error)
{
    UART_DebugError("AbortRxNotifyRoutine() error code", error);
    UartRxErrorCode = error;
    STOS_SemaphoreSignal(WaitBlockedSemaphore_p);
}

void AbortTxNotifyRoutine(STUART_Handle_t Handle, ST_ErrorCode_t error)
{
    UART_DebugError("AbortTxNotifyRoutine() error code", error);
    UartTxErrorCode = error;
    STOS_SemaphoreSignal(WaitBlockedSemaphore_p);
}

static UART_TestResult_t UART_TestAbort(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* Create semaphores */
    WaitBlockedSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto abort_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 600;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = AbortRxNotifyRoutine;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = AbortTxNotifyRoutine;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto abort_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 0;
    TxTimeout = 0;

    UART_DebugPrintf(("%d.0: Read abort\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check a read can be aborted.");
    {
        U32 NumberRead;
        U8 RxBuf[100] = { 0 };
        error = STUART_Read(Handle,
                            RxBuf,
                            1,
                            &NumberRead,
                            RxTimeout
                           );
        UART_DebugError("STUART_Read()", error);
        UART_DebugMessage("Now attempting abort...");
        error = STUART_Abort(Handle, STUART_DIRECTION_RECEIVE);
        UART_DebugError("STUART_Abort()", error);
        STOS_SemaphoreWait(WaitBlockedSemaphore_p);
        UART_DebugError("STUART_Read() callback", UartRxErrorCode);

        if (UartRxErrorCode != STUART_ERROR_ABORT)
        {
            UART_TestFailed(Result,"Should be STUART_ERROR_ABORT");
            goto abort_end;
        }
        UART_TestPassed(Result);
    }

    UART_DebugPrintf(("%d.1: Write abort\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check a write can be aborted.");
    {
        U32 NumberWritten;
        U8 TxBuf[255] = { 0 };
        error = STUART_Write(Handle,
                             TxBuf,
                             255,
                             &NumberWritten,
                             TxTimeout
                            );
        UART_DebugError("STUART_Write()", error);
        UART_DebugMessage("Now attempting abort...");
        error = STUART_Abort(Handle, STUART_DIRECTION_TRANSMIT);
        UART_DebugError("STUART_Abort()", error);
        STOS_SemaphoreWait(WaitBlockedSemaphore_p);
        UART_DebugError("STUART_Write() callback", UartTxErrorCode);

        if (UartTxErrorCode != STUART_ERROR_ABORT)
        {
            UART_TestFailed(Result,"Should be STUART_ERROR_ABORT");
            goto abort_end;
        }
        UART_TestPassed(Result);
    }

    /* Tidy up */

abort_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    STOS_SemaphoreDelete(NULL,WaitBlockedSemaphore_p);

    return Result;
}

/* Flush test */
static UART_TestResult_t UART_TestFlush(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto flush_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 9600;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 9600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto flush_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 1000;

    UART_DebugPrintf(("%d.0: Flush\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check FIFOs are flushed.");
    {
        U32 NumberRead;
        U32 NumberWritten;
        U8 RxBuf[100] = { 0 };
        char TestMessage[] = "0123456789ABCDEF0123456789ABCDEF";

        UART_DebugPrintf(("Sending %d bytes: '%s' to loopback to FIFO.\n",
                          strlen(TestMessage),
                          TestMessage));

#ifdef STUART_STATISTICS
        /* STUART_GetStatistics() */
        {
            STUART_Statistics_t Statistics;

            error = STUART_GetStatistics(Handle, &Statistics);
            UART_DebugError("STUART_GetStatistics()", error);
            UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
            UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
            UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
            UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
            UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));
        }
#endif /* STUART_STATISTICS */

        /* Try a read/write cycle */
        error = STUART_Write(Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );
        UART_DebugError("STUART_Write()", error);
        UART_DebugPrintf(("Written %d bytes.\n", NumberWritten));

        /* Check all bytes were successfully written */
        if (NumberWritten != strlen(TestMessage))
        {
            UART_TestFailed(Result, "Failed to write all bytes");
            goto flush_end;
        }

        UART_DebugMessage("Ensuring data has been received...");
#ifdef STUART_STATISTICS
        /* STUART_GetStatistics() */
        {
            STUART_Statistics_t Statistics;

            error = STUART_GetStatistics(Handle, &Statistics);
            UART_DebugError("STUART_GetStatistics()", error);
            UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
            UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
            UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
            UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
            UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));

            if (Statistics.NumberBytesReceived != NumberWritten )
            {
                UART_TestFailed(Result, "Incorrect bytes in FIFO");
                goto flush_end;
            }
        }
#endif /* STUART_STATISTICS */

        UART_DebugMessage("Now flushing FIFOs...");
        error = STUART_Flush(Handle);
        UART_DebugError("STUART_Flush()", error);
        UART_DebugMessage("Now trying to read back data...");
        error = STUART_Read(Handle,
                            RxBuf,
                            strlen(TestMessage),
                            &NumberRead,
                            RxTimeout
                           );
        UART_DebugError("STUART_Read()", error);
        UART_DebugPrintf(("Read %d bytes.\n", NumberRead));

        /* Check no bytes were read and a timeout occured */
        if (NumberRead != 0 || error != ST_ERROR_TIMEOUT)
        {
            UART_TestFailed(Result, "FIFOs failed to flush");
            goto flush_end;
        }
        UART_TestPassed(Result);
    }

    /* Tidy up */
flush_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;
}

static UART_TestResult_t UART_TestParams(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    U8 i;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto params_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 38400;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 38400;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto params_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 6000;
    TxTimeout = 0;

    UART_DebugPrintf(("%d.0: Baudrate\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Ensure commonly used baudrates work correctly.");
    {
#if defined(ST_7100)  || defined(ST_7109)
        U32 BaudRates[] = {
            300, 600, 1200, 2400, 4800, 9600, 19200, 38400,
            96000, 115200, 625000, 0
        };
#else
        U32 BaudRates[] = {
            75, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400,
            96000, 115200, 625000, 0
        };
#endif
        U32 *bp;

        bp = BaudRates;
        while(*bp != 0)
        {
            U32 ActualBaudRate;

            UartDefaultParams.RxMode.BaudRate =
                   UartDefaultParams.TxMode.BaudRate = *bp;
            UART_DebugPrintf(("Setting baud to %d...\n", *bp));
            error = STUART_SetParams(Handle, &UartDefaultParams);
            UART_DebugError("STUART_SetParams()", error);
            STUART_Flush(Handle);

            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result, "Unable to set baud rate");
                bp++;
                continue;
            }

            error = UART_Loopback(5, 128, Handle, RxTimeout, TxTimeout);
            STUART_GetActualBaudRate(Handle, &ActualBaudRate);
            UART_DebugPrintf(("Actual baudrate was = %d\n", ActualBaudRate));

            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result, "Data error");
                bp++;
                continue;
            }

            /* Ensure buffers are flushed for next test */
            STUART_Flush(Handle);

            bp++;
        }
    }
    UART_TestPassed(Result);

    /* Run remaining tests at 38400 */
    UartDefaultParams.RxMode.BaudRate =
               UartDefaultParams.TxMode.BaudRate = 38400;

    UART_DebugPrintf(("%d.1: DataBits\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Ensure all data modes work correctly.");
    {
        STUART_DataBits_t DataBits[] =
        {
            STUART_7BITS_ODD_PARITY,
            STUART_7BITS_EVEN_PARITY,
            STUART_8BITS_NO_PARITY,
            STUART_8BITS_ODD_PARITY,
            STUART_8BITS_EVEN_PARITY,
            /*STUART_8BITS_PLUS_WAKEUP,
            STUART_9BITS_NO_PARITY,*/
            (STUART_DataBits_t)-1
        };
        /*char ModeNames[][] =*/
        char *ModeNames[] =
        {
            "STUART_7BITS_ODD_PARITY ",
            "STUART_7BITS_EVEN_PARITY",
            "STUART_8BITS_NO_PARITY  ",
            "STUART_8BITS_ODD_PARITY ",
            "STUART_8BITS_EVEN_PARITY",
            "STUART_8BITS_PLUS_WAKEUP",
            "STUART_9BITS_NO_PARITY  "
        };
        STUART_DataBits_t *ptr;

        ptr = DataBits;
        i = 0;
        while(*ptr != -1)
        {
            UartDefaultParams.RxMode.DataBits = *ptr;
            UartDefaultParams.TxMode.DataBits = *ptr;
            UART_DebugPrintf(("Setting mode to %s...\n",
                              ModeNames[i++]));
            error = STUART_SetParams(Handle, &UartDefaultParams);
            UART_DebugError("STUART_SetParams()", error);
            STUART_Flush(Handle);

            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result, "Unable to set mode");
                ptr++;
                continue;
            }

            error = UART_Loopback(64, 8, Handle, RxTimeout, TxTimeout);

            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result, "Data error");
                ptr++;
                continue;
            }

            ptr++;
        }
    }
    UART_TestPassed(Result);

    UART_DebugPrintf(("%d.2: Stop bits\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Ensure all stop-bit modes work correctly.");
    {
        STUART_StopBits_t StopBits[] =
        {
            STUART_STOP_0_5,
            STUART_STOP_1_0,
            STUART_STOP_1_5,
            STUART_STOP_2_0,
            (STUART_StopBits_t)-1
        };
        /*char StopBitNames[][] =*/
        char *StopBitNames[] =
        {
            "STUART_STOP_0_5",
            "STUART_STOP_1_0",
            "STUART_STOP_1_5",
            "STUART_STOP_2_0"
        };
        STUART_StopBits_t *ptr;

        ptr = StopBits;
        i = 0;
        while(*ptr != -1)
        {
            UartDefaultParams.RxMode.StopBits =
                   UartDefaultParams.TxMode.StopBits = *ptr;
            UART_DebugPrintf(("Setting stop-bits to %s...\n",
                              StopBitNames[i++]));
            error = STUART_SetParams(Handle, &UartDefaultParams);
            UART_DebugError("STUART_SetParams()", error);
            STUART_Flush(Handle);

            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result, "Unable to set mode");
                ptr++;
                continue;
            }

            error = UART_Loopback(5, 128, Handle, RxTimeout, TxTimeout);

            if (error != ST_NO_ERROR)
            {
                UART_TestFailed(Result, "Data error");
                ptr++;
                continue;
            }

            ptr++;
        }
    }
    UART_TestPassed(Result);

    UART_DebugPrintf(("%d.3: Termination String\n", TestParams->Ref));
    UART_DebugMessage("Purpose: Ensure read calls returns with term string.");
    {
        U8 TxBuf[] = "0123456789ABCDEF,0123456789ABCDEF";
        U8 RxBuf[255] = {0};
        ST_String_t TermString = (ST_String_t)",\0";
        U32 NumberWritten, NumberRead;

        UART_DebugMessage("Setting termination string = ',\\0'");
        UartDefaultParams.RxMode.TermString = TermString;
        error = STUART_SetParams(Handle, &UartDefaultParams);
        UART_DebugError("STUART_SetParams()", error);
        STUART_Flush(Handle);

        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result, "Unable to set term string");
        }
        else
        {
            U32 NumberRemaining;

            UART_DebugPrintf(("Sending TxBuf = '%s'...\n", TxBuf));
            error = STUART_Write(Handle,
                                 TxBuf,
                                 strlen((char *)TxBuf),
                                 &NumberWritten,
                                 TxTimeout
                                );
            UART_DebugError("STUART_Write()", error);
            NumberRemaining = NumberWritten;

            if (error == ST_NO_ERROR)
            {
                while (NumberRemaining > 0)
                {
                    error = STUART_Read(Handle,
                                        RxBuf,
                                        NumberRemaining,
                                        &NumberRead,
                                        RxTimeout
                                       );
                    UART_DebugError("STUART_Read()", error);
                    RxBuf[NumberRead] = '\0';
                    UART_DebugPrintf(("Read = '%s' (%d)\n", RxBuf, NumberRead));
                    NumberRemaining -= NumberRead;
                }
            }
        }
    }

    /* Tidy up */
params_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;
}

static UART_TestResult_t UART_TestNonBlocking(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* Create semaphores */
    WaitBlockedSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto nonblock_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 38400;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = ReadNotifyRoutine;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 38400;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = WriteNotifyRoutine;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto nonblock_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 5000;
    TxTimeout = 0;

    UART_DebugPrintf(("%d.0: Non-blocking loopback\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check non-blocking read/write calls.");
    {
        U32 NumberRead, NumberWritten;
        U8 RxBuf[100] = { 0 };
        U8 TxBuf[] = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";

        error = STUART_Read(Handle,
                            RxBuf,
                            strlen((char *)TxBuf),
                            &NumberRead,
                            RxTimeout
                           );
        UART_DebugError("STUART_Read()", error);

        UART_DebugMessage("Now attempting transmit...");
        error = STUART_Write(Handle,
                             TxBuf,
                             strlen((char *)TxBuf),
                             &NumberWritten,
                             TxTimeout
                            );
#if TEST_VERBOSE
        UART_DebugError("STUART_Write()", error);
#endif

        /* Wait for both callbacks to complete */
        STOS_SemaphoreWait(WaitBlockedSemaphore_p);
        STOS_SemaphoreWait(WaitBlockedSemaphore_p);

        UART_DebugError("STUART_Read() callback", ReadErrorCode);
        UART_DebugError("STUART_Write() callback", WriteErrorCode);

        if (ReadErrorCode != ST_NO_ERROR || NumberRead != NumberWritten ||
            strcmp((char *)RxBuf, (char *)TxBuf) != 0)
        {
            UART_DebugPrintf(("Received = '%s'.\n", RxBuf));
            UART_TestFailed(Result,"Receive error");
        }
        else if (WriteErrorCode != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Transmit error");
        }
        else
        {
            UART_TestPassed(Result);
        }
    }

#ifdef STUART_STATISTICS
    /* STUART_GetStatistics() */
    {
        STUART_Statistics_t Statistics;

        error = STUART_GetStatistics(Handle, &Statistics);
        UART_DebugError("STUART_GetStatistics()", error);
        UART_DebugPrintf(("NumberBytesReceived = %d\n", Statistics.NumberBytesReceived));
        UART_DebugPrintf(("NumberBytesTransmitted = %d\n", Statistics.NumberBytesTransmitted));
        UART_DebugPrintf(("NumberOverrunErrors = %d\n", Statistics.NumberOverrunErrors));
        UART_DebugPrintf(("NumberFramingErrors = %d\n", Statistics.NumberFramingErrors));
        UART_DebugPrintf(("NumberParityErrors = %d\n", Statistics.NumberParityErrors));
    }
#endif /* STUART_STATISTICS */

    /* Tidy up */
nonblock_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    STOS_SemaphoreDelete(NULL,WaitBlockedSemaphore_p);

    return Result;
}

static UART_TestResult_t UART_TestForceTerm(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle = 0;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* Create semaphores */
    WaitBlockedSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto force_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 600;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = ReadNotifyRoutine;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 600;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = WriteNotifyRoutine;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto force_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 1000;

    UART_DebugPrintf(("%d.0: Terminate with IO pending\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check driver can be forcibly terminated.");
    {
        U32 NumberRead, NumberWritten;
        U8 RxBuf[100] = { 0 };
        U8 TxBuf[] = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";

        error = STUART_Read(Handle,
                            RxBuf,
                            strlen((char *)TxBuf),
                            &NumberRead,
                            RxTimeout
                           );
        UART_DebugError("STUART_Read()", error);

        error = STUART_Write(Handle,
                             TxBuf,
                             strlen((char *)TxBuf),
                             &NumberWritten,
                             TxTimeout
                            );
        UART_DebugError("STUART_Write()", error);

        /* Attempt to terminate the driver */
        {
            STUART_TermParams_t UartTermParams;
            UartTermParams.ForceTerminate = TRUE;

            error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
            UART_DebugError("STUART_Term()", error);
        }

        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Terminate failed");
            goto force_end;
        }

        /* Wait for both callbacks to complete */
        STOS_SemaphoreWait(WaitBlockedSemaphore_p);
        STOS_SemaphoreWait(WaitBlockedSemaphore_p);
        UART_DebugError("STUART_Read() callback", ReadErrorCode);
        UART_DebugError("STUART_Write() callback", WriteErrorCode);

        /* Check the terminate induced an abort on all IO */
        if (ReadErrorCode != STUART_ERROR_ABORT ||
            WriteErrorCode != STUART_ERROR_ABORT)
        {
            UART_TestFailed(Result,"Expected STUART_ERROR_ABORT");
        }
        else
        {
            UART_TestPassed(Result);
        }
    }

    /* Tidy up */
force_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    STOS_SemaphoreDelete(NULL,WaitBlockedSemaphore_p);

    return Result;
}

static UART_TestResult_t UART_TestPause(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    U32 RxTimeout, TxTimeout;

    /* Create semaphores */
    WaitBlockedSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to initialize UART device");
            goto pause_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 300;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = ReadNotifyRoutine;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 300;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = WriteNotifyRoutine;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to open UART device");
            goto pause_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 0;
    TxTimeout = 0;

    UART_DebugPrintf(("%d.0: Pause/Resume with IO pending\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To check driver can pause operations.");
    {
        U32 NumberRead, NumberWritten;
        U8 RxBuf[100] = { 0 };
        U8 TxBuf[] = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";

        error = STUART_Read(Handle,
                            RxBuf,
                            strlen((char *)TxBuf),
                            &NumberRead,
                            RxTimeout
                           );
        UART_DebugError("STUART_Read()", error);

        UART_DebugMessage("Now attempting transmit...");
        error = STUART_Write(Handle,
                             TxBuf,
                             strlen((char *)TxBuf),
                             &NumberWritten,
                             TxTimeout
                            );
        UART_DebugError("STUART_Write()", error);

        UART_DebugMessage("Now attempting pause...");
        /* Attempt to terminate the driver */
        error = STUART_Pause(Handle, STUART_DIRECTION_TRANSMIT);
        UART_DebugError("STUART_Pause()", error);

        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to pause driver");
            goto pause_end;
        }

        UART_DebugMessage("Delaying for 5s to be sure we're paused...");
        STOS_TaskDelay(TICKS_PER_SECOND*5);
        UART_DebugMessage("Now resuming (IO should complete ok)");
        error = STUART_Resume(Handle, STUART_DIRECTION_TRANSMIT);
        UART_DebugError("STUART_Resume()", error);

        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result,"Unable to resume driver");
            goto pause_end;
        }

        /* Wait for both callbacks to complete */
        STOS_SemaphoreWait(WaitBlockedSemaphore_p);
        STOS_SemaphoreWait(WaitBlockedSemaphore_p);

        UART_DebugError("STUART_Read() callback", ReadErrorCode);
        UART_DebugError("STUART_Write() callback", WriteErrorCode);

        /* Check operation was unaffected by the pause */
        if (ReadErrorCode != ST_NO_ERROR ||
            WriteErrorCode != ST_NO_ERROR ||
            strcmp((char *)RxBuf,(char *)TxBuf) != 0)
        {
            UART_TestFailed(Result, "IO should have completed ok");
        }
        else
        {
            UART_TestPassed(Result);
        }
    }

    /* Tidy up */
pause_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    STOS_SemaphoreDelete(NULL,WaitBlockedSemaphore_p);

    return Result;
}

#if defined(TEST_TWO_PORTS)

static void UART_ProducerTask(UART_TaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = ASC_DEVICE_MODEM;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
            goto producer_end;
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = LOOPBACK_BAUDRATE;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = LOOPBACK_BAUDRATE;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            goto producer_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 5000;
    TxTimeout = 1000;

    UART_DebugMessage("Starting producer task...");

    /* Wait for consumer to commence */
    STOS_SemaphoreWait(WaitBlockedSemaphore_p);

    while(Params_p->ticks > 0)
    {
        U32 NumberWritten;
        U32 NumberRead;
        char TestMessage[] = "0123456789ABCDEF";
        char Ack[12] = {0};

        /* Try a read/write cycle */
        error = STUART_Write(Handle,
                             (U8 *)TestMessage,
                             strlen(TestMessage),
                             &NumberWritten,
                             TxTimeout
                            );

        if (error != ST_NO_ERROR)
        {
            UART_DebugError("STUART_Write()", error);
            break;
        }


        /* Since we have no flow control, the only way of ensuring
         * it's safe to send the next block of data is by waiting
         * for an acknowledge string.
         */
        error = STUART_Read(Handle,
                            (U8 *)Ack,
                            3,
                            &NumberRead,
                            RxTimeout
                           );

        if (error != ST_NO_ERROR ||
            NumberRead != 3 || strcmp("ACK",Ack) != 0)
        {
            UART_DebugError("No ACK received.", error);
            break;
        }

        Params_p->ticks--;
    }

    /* Get final statistics */
    Params_p->Error = error;
#ifdef STUART_STATISTICS
    error = STUART_GetStatistics(Handle, Params_p->Statistics_p);
    UART_DebugError("STUART_GetStatistics()", error);
#endif

    /* Tidy up */
producer_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }
}

static void UART_ConsumerTask(UART_TaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = ASC_DEVICE_DATA;
    U32 RxTimeout, TxTimeout;
    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            goto consumer_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = LOOPBACK_BAUDRATE;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = LOOPBACK_BAUDRATE;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = FALSE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            goto consumer_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 5000;
    TxTimeout = 1000;

    UART_DebugMessage("Starting consumer task...");

    /* Allow producer to commence */
    STOS_SemaphoreSignal(WaitBlockedSemaphore_p);

    while(Params_p->ticks > 0)
    {
        U32 NumberWritten;
        U32 NumberRead;

        char TestMessage[35] = {0};

        /* Try a read/write cycle */
        error = STUART_Read(Handle,
                            (U8 *)TestMessage,
                            16,
                            &NumberRead,
                            RxTimeout
                           );

        if (error != ST_NO_ERROR)
        {
            UART_DebugError("STUART_Read()", error);
            break;
        }

        if (strcmp(TestMessage, "0123456789ABCDEF") != 0)
        {
            UART_DebugPrintf(("Bad data = 0x%02x\n", TestMessage[0]));
            break;
        }

        error = STUART_Write(Handle,
                             (U8 *)"ACK",
                             3,
                             &NumberWritten,
                             TxTimeout
                            );

        if (error != ST_NO_ERROR)
        {
            UART_DebugError("STUART_Write()", error);
            break;
        }


        Params_p->ticks--;
    }

    /* Get final statistics */
    Params_p->Error = error;
#ifdef STUART_STATISTICS
    error = STUART_GetStatistics(Handle, Params_p->Statistics_p);
    UART_DebugError("STUART_GetStatistics()", error);
#endif

    /* Tidy up */
consumer_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }
}

static void UART_Loopback1(UART_TaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = ASC_DEVICE_DATA;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
#if TEST_VERBOSE
        UART_DebugError("STUART_Init()", error);
#endif
        if (error != ST_NO_ERROR)
        {
            goto loopback1_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = LOOPBACK_BAUDRATE;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = LOOPBACK_BAUDRATE;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = TRUE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
#if TEST_VERBOSE
        UART_DebugError("STUART_Open()", error);
#endif
        if (error != ST_NO_ERROR)
        {
            goto loopback1_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 100;

#if TEST_VERBOSE
    UART_DebugMessage("Starting loopback 1 task...");
#endif
    error = UART_Loopback(Params_p->ticks, 128, Handle, RxTimeout, TxTimeout);

    /* Get final statistics */
    Params_p->Error = error;
#ifdef STUART_STATISTICS
    error = STUART_GetStatistics(Handle, Params_p->Statistics_p);
#if TEST_VERBOSE
    UART_DebugError("STUART_GetStatistics()", error);
#endif
#endif

    /* Tidy up */
loopback1_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
#if TEST_VERBOSE
        UART_DebugError("STUART_Close()", error);
#endif
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
#if TEST_VERBOSE
        UART_DebugError("STUART_Term()", error);
#endif
    }
}

static void UART_Loopback2(UART_TaskParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = ASC_DEVICE_MODEM;
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
#if TEST_VERBOSE
        UART_DebugError("STUART_Init()", error);
#endif
        if (error != ST_NO_ERROR)
        {
            goto loopback2_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = LOOPBACK_BAUDRATE;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = LOOPBACK_BAUDRATE;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
        UartOpenParams.LoopBackEnabled = TRUE;
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
#if TEST_VERBOSE
        UART_DebugError("STUART_Open()", error);
#endif
        if (error != ST_NO_ERROR)
        {
            goto loopback2_end;
        }
    }

    /* Timeout Params */
    RxTimeout = 1000;
    TxTimeout = 100;

#if TEST_VERBOSE
    UART_DebugMessage("Starting loopback 2 task...");
#endif
    error = UART_Loopback(Params_p->ticks, 128, Handle, RxTimeout, TxTimeout);

    /* Get final statistics */
    Params_p->Error = error;
#ifdef STUART_STATISTICS
    error = STUART_GetStatistics(Handle, Params_p->Statistics_p);
#if TEST_VERBOSE
    UART_DebugError("STUART_GetStatistics()", error);
#endif
#endif

    /* Tidy up */
loopback2_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
#if TEST_VERBOSE
        UART_DebugError("STUART_Close()", error);
#endif
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
#if TEST_VERBOSE
        UART_DebugError("STUART_Term()", error);
#endif
    }
}

static UART_TestResult_t UART_TestBackToBack(UART_TestParams_t *TestParams)
{
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    BOOL Error;
#ifdef STUART_STATISTICS
    STUART_Statistics_t ProducerStats, ConsumerStats;
#endif
    UART_TaskParams_t ProducerParams, ConsumerParams;
    
    ConsumerParams.Driverpartition = (ST_Partition_t*)system_partition;
    ProducerParams.Driverpartition = (ST_Partition_t*)system_partition;	

    /* Create tasks */
    ProducerParams.ticks = 1000;
#ifdef STUART_STATISTICS
    ProducerParams.Statistics_p = &ProducerStats;
#endif
    ConsumerParams.ticks = 1000;

#ifdef STUART_STATISTICS
    ConsumerParams.Statistics_p = &ConsumerStats;
#endif

    WaitBlockedSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);

    UART_DebugPrintf(("%d.0: Multi-task loopback\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To ensure more than one UART can be"
                      " handled by the driver.");
    UART_DebugMessage("Creating tasks...");

    Error =  STOS_TaskCreate   ((void(*)(void *))UART_Loopback1,
							&ProducerParams,
							ProducerParams.Driverpartition,
							STACK_SIZE,
							NULL,
							ProducerParams.Driverpartition,
							&ProducerParams.Task_p,
							NULL, 
							MAX_USER_PRIORITY,
							"Producer", 
							(task_flags_t)0);
    
    if  (Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Unable to create task");
        return Result;
    }

   Error =  STOS_TaskCreate   ((void(*)(void *))UART_Loopback2,
							&ConsumerParams,
							ConsumerParams.Driverpartition,
							STACK_SIZE,
							NULL,
							ConsumerParams.Driverpartition,
							&ConsumerParams.Task_p,
							NULL, 
							MAX_USER_PRIORITY,
							"Consumer", 
							(task_flags_t)0);

    if  (Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Unable to create task");
        return Result;
    }

#if TEST_VERBOSE
    UART_DebugMessage("Awaiting for tasks to complete...");
#endif

    /* Wait for each task before deleting it */
    if (STOS_TaskWait(&ProducerParams.Task_p, TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        STOS_TaskDelete( ProducerParams.Task_p,
				 	  ConsumerParams.Driverpartition, 
					  NULL,
					  ProducerParams.Driverpartition);
    }

     /* Wait for each task before deleting it */
    if (STOS_TaskWait(&ConsumerParams.Task_p, TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        STOS_TaskDelete( ConsumerParams.Task_p,
				 	  ConsumerParams.Driverpartition, 
					  NULL,
					  ConsumerParams.Driverpartition);
    }
    

#ifdef STUART_STATISTICS
    UART_DebugMessage("Loopback1 stats...");
    UART_DebugPrintf(("NumberBytesReceived = %d\n", ProducerStats.NumberBytesReceived));
    UART_DebugPrintf(("NumberBytesTransmitted = %d\n", ProducerStats.NumberBytesTransmitted));
    UART_DebugPrintf(("NumberOverrunErrors = %d\n", ProducerStats.NumberOverrunErrors));
    UART_DebugPrintf(("NumberFramingErrors = %d\n", ProducerStats.NumberFramingErrors));
    UART_DebugPrintf(("NumberParityErrors = %d\n", ProducerStats.NumberParityErrors));

    UART_DebugMessage("Loopback2 stats...");
    UART_DebugPrintf(("NumberBytesReceived = %d\n", ConsumerStats.NumberBytesReceived));
    UART_DebugPrintf(("NumberBytesTransmitted = %d\n", ConsumerStats.NumberBytesTransmitted));
    UART_DebugPrintf(("NumberOverrunErrors = %d\n", ConsumerStats.NumberOverrunErrors));
    UART_DebugPrintf(("NumberFramingErrors = %d\n", ConsumerStats.NumberFramingErrors));
    UART_DebugPrintf(("NumberParityErrors = %d\n", ConsumerStats.NumberParityErrors));
#endif /* STUART_STATISTICS */

    if (ConsumerParams.Error != ST_NO_ERROR ||
        ProducerParams.Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Error during processing");
    }
    else
    {
        UART_TestPassed(Result);
    }

    ProducerParams.ticks = 1000;
#ifdef STUART_STATISTICS
    ProducerParams.Statistics_p = &ProducerStats;
#endif
    ConsumerParams.ticks = 1000;
#ifdef STUART_STATISTICS
    ConsumerParams.Statistics_p = &ConsumerStats;
#endif

#ifdef TEST_TWO_PORTS
    UART_DebugPrintf(("%d.1: Multi-task back-to-back\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To ensure more than one UART can be"
                      " handled by the driver with external comms.");
    UART_DebugMessage("Creating tasks...");

   Error =  STOS_TaskCreate   ((void(*)(void *))UART_ConsumerTask,
							&ConsumerParams,
							ConsumerParams.Driverpartition,
							STACK_SIZE,
							NULL,
							ConsumerParams.Driverpartition,
							&ConsumerParams.Task_p,
							NULL, 
							MAX_USER_PRIORITY,
							"Consumer", 
							(task_flags_t)0);

    if  (Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Unable to create task");
        return Result;
    }
    Error =  STOS_TaskCreate   ((void(*)(void *))UART_ProducerTask,
							&ProducerParams,
							ProducerParams.Driverpartition,
							STACK_SIZE,
							NULL,
							ProducerParams.Driverpartition,
							&ProducerParams.Task_p,
							NULL, 
							MAX_USER_PRIORITY,
							"Producer", 
							(task_flags_t)0);

    if  (Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Unable to create task");
        return Result;
    }

    UART_DebugMessage("Waiting for tasks to complete...");

    /* Wait for each task before deleting it */
    if (STOS_TaskWait(&ProducerParams.Task_p, TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        STOS_TaskDelete( ProducerParams.Task_p,
				 	    ProducerParams.Driverpartition, 
					    NULL,
					    ProducerParams.Driverpartition);
    }
   
    /* Wait for each task before deleting it */
    if (STOS_TaskWait(&ConsumerParams.Task_p, TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        STOS_TaskDelete( ConsumerParams.Task_p,
				 	    ConsumerParams.Driverpartition, 
					    NULL,
					    ConsumerParams.Driverpartition);
    }
    
#ifdef STUART_STATISTICS
    UART_DebugMessage("Producer stats...");
    UART_DebugPrintf(("NumberBytesReceived = %d\n", ProducerStats.NumberBytesReceived));
    UART_DebugPrintf(("NumberBytesTransmitted = %d\n", ProducerStats.NumberBytesTransmitted));
    UART_DebugPrintf(("NumberOverrunErrors = %d\n", ProducerStats.NumberOverrunErrors));
    UART_DebugPrintf(("NumberFramingErrors = %d\n", ProducerStats.NumberFramingErrors));
    UART_DebugPrintf(("NumberParityErrors = %d\n", ProducerStats.NumberParityErrors));

    UART_DebugMessage("Consumer stats...");
    UART_DebugPrintf(("NumberBytesReceived = %d\n", ConsumerStats.NumberBytesReceived));
    UART_DebugPrintf(("NumberBytesTransmitted = %d\n", ConsumerStats.NumberBytesTransmitted));
    UART_DebugPrintf(("NumberOverrunErrors = %d\n", ConsumerStats.NumberOverrunErrors));
    UART_DebugPrintf(("NumberFramingErrors = %d\n", ConsumerStats.NumberFramingErrors));
    UART_DebugPrintf(("NumberParityErrors = %d\n", ConsumerStats.NumberParityErrors));
#endif /* STUART_STATISTICS */

    if (ConsumerParams.Error != ST_NO_ERROR ||
        ProducerParams.Error != ST_NO_ERROR)
    {
        UART_TestFailed(Result, "Error during processing");
    }
    else
    {
        UART_TestPassed(Result);
    }
#endif /* TEST_TWO_PORTS */

    STOS_SemaphoreDelete(NULL,WaitBlockedSemaphore_p);

    return Result;
}

#endif
static UART_TestResult_t UART_TestPerformance(UART_TestParams_t *TestParams)
{
    ST_ErrorCode_t error;
    STUART_Handle_t Handle;
    STUART_OpenParams_t UartOpenParams;
    STUART_Params_t UartDefaultParams;
    U8 ASCNumber = TestParams->ASCNumber;
    UART_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Blocks;
#ifdef STUART_STATISTICS
    STUART_Statistics_t Statistics;
#endif
    U32 RxTimeout, TxTimeout;

    /* Init */
    {
        /* Setup Uart */
        error = STUART_Init(UartDeviceName[ASCNumber],
                            &UartInitParams[ASCNumber]);
        UART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            goto perf_end;
        }
    }

    /* STUART_Open() */
    {
        /* Open params setup */
        UartDefaultParams.RxMode.DataBits = STUART_8BITS_NO_PARITY;
        UartDefaultParams.RxMode.StopBits = STUART_STOP_2_0;
        UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.RxMode.BaudRate = 38400;
        UartDefaultParams.RxMode.TermString = NULL;
        UartDefaultParams.RxMode.NotifyFunction = NULL;
        UartDefaultParams.TxMode.DataBits = STUART_8BITS_NO_PARITY;
        UartDefaultParams.TxMode.StopBits = STUART_STOP_2_0;
        UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        UartDefaultParams.TxMode.BaudRate = 38400;
        UartDefaultParams.TxMode.TermString = NULL;
        UartDefaultParams.TxMode.NotifyFunction = NULL;
        UartDefaultParams.SmartCardModeEnabled = FALSE;
        UartDefaultParams.GuardTime = 0;
#if EXTERNAL_LOOPBACK
        UartOpenParams.LoopBackEnabled = FALSE;
#else
        UartOpenParams.LoopBackEnabled = TRUE;
#endif
        UartOpenParams.FlushIOBuffers = TRUE;
        UartOpenParams.DefaultParams = &UartDefaultParams;
        error = STUART_Open(UartDeviceName[ASCNumber],
                            &UartOpenParams, &Handle);
        UART_DebugError("STUART_Open()", error);
        if (error != ST_NO_ERROR)
        {
            goto perf_end;
        }
    }

    RxTimeout = 1000;
    TxTimeout = 1000;

    UART_DebugPrintf(("%d.0: Timed loopback\n", TestParams->Ref));
    UART_DebugMessage("Purpose: To illustrate the effective bit-rate"
                      " during a loopback.");
    UART_DebugPrintf(("Params: BaudRate=%d FIFO=%dbytes\n",
                      UartDefaultParams.RxMode.BaudRate,
                      UartInitParams[ASCNumber].FIFOLength));

    for (Blocks = 1; Blocks <= 128; Blocks <<= 1)
    {

#if defined(ARCHITECTURE_ST200)
        U32 StartTime,TimeTaken;
#else
        clock_t StartTime,TimeTaken;
#endif

        /* Flush buffers before starting... */
        STUART_Flush(Handle);

        UART_DebugPrintf(("Transferring %d byte blocks...\n", Blocks));
        StartTime = STOS_time_now();
        error = UART_Loopback((16000/Blocks), Blocks, Handle, RxTimeout, TxTimeout);
        TimeTaken = STOS_time_minus(STOS_time_now(),StartTime);
        UART_DebugError("UART_Loopback()", error);

        if (error != ST_NO_ERROR)
        {
            UART_TestFailed(Result, "Error during loopback");
            break;
        }

        UART_DebugPrintf(("Time taken=%d ticks (%d ms)\n", TimeTaken,
                          TimeTaken/(TICKS_PER_SECOND/1000)));

        /* Get final statistics */
#ifdef STUART_STATISTICS
        error = STUART_GetStatistics(Handle, &Statistics);
        UART_DebugPrintf(("Effective data rate=%u bps\n",
                          (((U32)TICKS_PER_SECOND * (U32)160) /
                           (U32)TimeTaken) * (U32)1100));
#endif
    }

    if (error == ST_NO_ERROR)
    {
        UART_TestPassed(Result);
    }

    /* Tidy up */
perf_end:
    /* STUART_Close() */
    {
        error = STUART_Close(Handle);
        UART_DebugError("STUART_Close()", error);
    }

    /* STUART_Term() */
    {
        STUART_TermParams_t UartTermParams;
        UartTermParams.ForceTerminate = FALSE;

        error = STUART_Term(UartDeviceName[ASCNumber],&UartTermParams);
        UART_DebugError("STUART_Term()", error);
    }

    return Result;
}

#endif
/*****************************************************************************
Debug routines
*****************************************************************************/

/* Error message look up table */
static UART_ErrorMessage UART_ErrorLUT[] =
{
    /* STUART */
    { STUART_ERROR_OVERRUN,"STUART_ERROR_OVERRUN" },
    { STUART_ERROR_PARITY, "STUART_ERROR_PARITY" },
    { STUART_ERROR_FRAMING, "STUART_ERROR_FRAMING" },
    { STUART_ERROR_ABORT, "STUART_ERROR_ABORT" },
    { STUART_ERROR_RETRIES, "STUART_ERROR_RETRIES" },
    { STUART_ERROR_PIO, "STUART_ERROR_PIO" },

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
    { ST_ERROR_UNKNOWN, "ST_ERROR_UNKNOWN" } /* Terminator */
};

char *UART_ErrorString(ST_ErrorCode_t Error)
{
    UART_ErrorMessage *mp;

    mp = UART_ErrorLUT;
    while (mp->Error != ST_ERROR_UNKNOWN)
    {
        if (mp->Error == Error)
        {
            break;
        }
        mp++;
    }

    return mp->ErrorMsg;
}

void ReadNotifyRoutine(STUART_Handle_t Handle, ST_ErrorCode_t error)
{
    ReadErrorCode = error;
    STOS_SemaphoreSignal(WaitBlockedSemaphore_p);
}

void WriteNotifyRoutine(STUART_Handle_t Handle, ST_ErrorCode_t error)
{
    WriteErrorCode = error;
    STOS_SemaphoreSignal(WaitBlockedSemaphore_p);
}
/* End of module */

