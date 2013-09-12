/*******************************************************************************

File name   : cluart.c

Description : UART configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
31 Jan 2002        Created                                           HSdLM
05 Jun 2002        Add 5516 support                                  HSdLM
11 Jun 2002        Add Open/Close                                    HSdLM
14 Jun 2002        5516 needs some pokes                             AN
14 Oct 2002        Add support for 5517 and 5578                     HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#ifdef ST_OS20
#include <debug.h>
#endif /* ST_OS20 */
#ifdef ST_OS21
#include <os21debug.h>
#endif /* ST_OS21 */
#include "testcfg.h"

#ifdef USE_UART

#define USE_AS_FRONTEND

#include "genadd.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stcommon.h"
#include "stsys.h"
#include "stuart.h"
#include "cluart.h"
#include "clpio.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Enum of ASC devices */
enum
{
    ASC_DEVICE_0,
    ASC_DEVICE_1,
    ASC_DEVICE_2,
    ASC_DEVICE_3,
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
    PIO_DEVICE_NOT_USED
};

/* Bit definition for an unused PIO bit */
#define PIO_BIT_NOT_USED        0

#ifndef BOARD71XX_WA_OSC_VALUE
#define BOARD71XX_WA_OSC_VALUE 30
#endif

/***************************************** BOARD wires **************************************/
/* The following associations are BOARD dependant and NOT CHIP dependant. But we keep the   */
/* check by CHIP because we suppose the consumer's boards have the same schematic.          */
/********************************************************************************************/
#if defined(ST_5510) || defined(ST_5512) || defined(ST_TP3)
#define ASC_0_TXD_BIT                 PIO_BIT_0
#define ASC_0_RXD_BIT                 PIO_BIT_1
#define ASC_0_TXD_DEV                 PIO_DEVICE_0
#define ASC_0_RXD_DEV                 PIO_DEVICE_0
#define ASC_0_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_TXD_BIT                 PIO_BIT_5
#define ASC_1_RXD_BIT                 PIO_BIT_6
#define ASC_1_TXD_DEV                 PIO_DEVICE_1
#define ASC_1_RXD_DEV                 PIO_DEVICE_1
#define ASC_1_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_TXD_BIT                 PIO_BIT_0
#define ASC_2_RXD_BIT                 PIO_BIT_1
#define ASC_2_TXD_DEV                 PIO_DEVICE_2
#define ASC_2_RXD_DEV                 PIO_DEVICE_2
#define ASC_2_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_2_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_2_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_TXD_BIT                 PIO_BIT_0
#define ASC_3_RXD_BIT                 PIO_BIT_1
#define ASC_3_TXD_DEV                 PIO_DEVICE_4
#define ASC_3_RXD_DEV                 PIO_DEVICE_4
#define ASC_3_RTS_BIT                 PIO_BIT_4
#define ASC_3_CTS_BIT                 PIO_BIT_3
#define ASC_3_RTS_DEV                 PIO_DEVICE_4
#define ASC_3_CTS_DEV                 PIO_DEVICE_4
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_2
#endif

#if defined(ST_5508)
#define ASC_0_TXD_BIT                 PIO_BIT_0
#define ASC_0_RXD_BIT                 PIO_BIT_0
#define ASC_0_TXD_DEV                 PIO_DEVICE_0
#define ASC_0_RXD_DEV                 PIO_DEVICE_0
#define ASC_0_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_TXD_BIT                 PIO_BIT_5
#define ASC_1_RXD_BIT                 PIO_BIT_1
#define ASC_1_TXD_DEV                 PIO_DEVICE_1
#define ASC_1_RXD_DEV                 PIO_DEVICE_2
#define ASC_1_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_TXD_BIT                 PIO_BIT_3
#define ASC_2_RXD_BIT                 PIO_BIT_4
#define ASC_2_TXD_DEV                 PIO_DEVICE_1
#define ASC_2_RXD_DEV                 PIO_DEVICE_1
#define ASC_2_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_2_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_2_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_TXD_BIT                 PIO_BIT_0
#define ASC_3_RXD_BIT                 PIO_BIT_0
#define ASC_3_TXD_DEV                 PIO_DEVICE_2
#define ASC_3_RXD_DEV                 PIO_DEVICE_2
#define ASC_3_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_3_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_3_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_3

#elif defined(ST_5518) || defined(ST_5578)
#define ASC_0_TXD_BIT                 PIO_BIT_0
#define ASC_0_RXD_BIT                 PIO_BIT_0
#define ASC_0_TXD_DEV                 PIO_DEVICE_0
#define ASC_0_RXD_DEV                 PIO_DEVICE_0
#define ASC_0_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_TXD_BIT                 PIO_BIT_5
#define ASC_1_RXD_BIT                 PIO_BIT_1
#define ASC_1_TXD_DEV                 PIO_DEVICE_1
#define ASC_1_RXD_DEV                 PIO_DEVICE_2
#define ASC_1_RTS_BIT                 PIO_BIT_4
#define ASC_1_CTS_BIT                 PIO_BIT_6
#define ASC_1_RTS_DEV                 PIO_DEVICE_3
#define ASC_1_CTS_DEV                 PIO_DEVICE_3
#define ASC_2_TXD_BIT                 PIO_BIT_3
#define ASC_2_RXD_BIT                 PIO_BIT_4
#define ASC_2_TXD_DEV                 PIO_DEVICE_1
#define ASC_2_RXD_DEV                 PIO_DEVICE_1
#define ASC_2_RTS_BIT                 PIO_BIT_5
#define ASC_2_CTS_BIT                 PIO_BIT_7
#define ASC_2_RTS_DEV                 PIO_DEVICE_3
#define ASC_2_CTS_DEV                 PIO_DEVICE_3
#define ASC_3_TXD_BIT                 PIO_BIT_0
#define ASC_3_RXD_BIT                 PIO_BIT_0
#define ASC_3_TXD_DEV                 PIO_DEVICE_2
#define ASC_3_RXD_DEV                 PIO_DEVICE_2
#define ASC_3_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_3_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_3_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_3

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
#define ASC_0_TXD_BIT                 PIO_BIT_0
#define ASC_0_RXD_BIT                 PIO_BIT_1
#define ASC_0_TXD_DEV                 PIO_DEVICE_0
#define ASC_0_RXD_DEV                 PIO_DEVICE_0
#define ASC_0_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_TXD_BIT                 PIO_BIT_5
#define ASC_1_RXD_BIT                 PIO_BIT_6
#define ASC_1_TXD_DEV                 PIO_DEVICE_1
#define ASC_1_RXD_DEV                 PIO_DEVICE_1
#define ASC_1_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_TXD_BIT                 PIO_BIT_4
#define ASC_2_RXD_BIT                 PIO_BIT_3
#define ASC_2_TXD_DEV                 PIO_DEVICE_4
#define ASC_2_RXD_DEV                 PIO_DEVICE_4
#define ASC_2_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_2_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_2_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_TXD_BIT                 PIO_BIT_4
#define ASC_3_RXD_BIT                 PIO_BIT_5
#define ASC_3_TXD_DEV                 PIO_DEVICE_5
#define ASC_3_RXD_DEV                 PIO_DEVICE_5
#define ASC_3_RTS_BIT                 PIO_BIT_7
#define ASC_3_CTS_BIT                 PIO_BIT_6
#define ASC_3_RTS_DEV                 PIO_DEVICE_5
#define ASC_3_CTS_DEV                 PIO_DEVICE_5
#define ASC_4_TXD_BIT                 PIO_BIT_1
#define ASC_4_RXD_BIT                 PIO_BIT_2
#define ASC_4_TXD_DEV                 PIO_DEVICE_2
#define ASC_4_RXD_DEV                 PIO_DEVICE_2
#define ASC_4_RTS_BIT                 PIO_BIT_0
#define ASC_4_CTS_BIT                 PIO_BIT_3
#define ASC_4_RTS_DEV                 PIO_DEVICE_2
#define ASC_4_CTS_DEV                 PIO_DEVICE_2
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_1

#elif defined(ST_GX1)
#define ASC_0_TXD_BIT                 PIO_BIT_NOT_USED
#define ASC_0_RXD_BIT                 PIO_BIT_NOT_USED
#define ASC_0_TXD_DEV                 PIO_DEVICE_NOT_USED
#define ASC_0_RXD_DEV                 PIO_DEVICE_NOT_USED
#define ASC_0_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_TXD_BIT                 PIO_BIT_NOT_USED
#define ASC_1_RXD_BIT                 PIO_BIT_NOT_USED
#define ASC_1_TXD_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_RXD_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_TXD_BIT                 PIO_BIT_NOT_USED
#define ASC_2_RXD_BIT                 PIO_BIT_NOT_USED
#define ASC_2_TXD_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_RXD_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_2_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_2_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_TXD_BIT                 PIO_BIT_NOT_USED
#define ASC_3_RXD_BIT                 PIO_BIT_NOT_USED
#define ASC_3_TXD_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_RXD_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_3_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_3_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_DEVICE_SC0                ASC_DEVICE_NOT_USED
#define ASC_DEVICE_SC1                ASC_DEVICE_NOT_USED

#elif defined(ST_5528) || defined(ST_5100) || defined(ST_5301)|| defined(ST_5525)
#define ASC_0_TXD_BIT                 PIO_BIT_0
#define ASC_0_RXD_BIT                 PIO_BIT_1
#define ASC_0_TXD_DEV                 PIO_DEVICE_0
#define ASC_0_RXD_DEV                 PIO_DEVICE_0
#define ASC_0_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_TXD_BIT                 PIO_BIT_0
#define ASC_1_RXD_BIT                 PIO_BIT_1
#define ASC_1_TXD_DEV                 PIO_DEVICE_1
#define ASC_1_RXD_DEV                 PIO_DEVICE_1
#define ASC_1_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_TXD_BIT                 PIO_BIT_4
#define ASC_2_RXD_BIT                 PIO_BIT_3
#define ASC_2_TXD_DEV                 PIO_DEVICE_4
#define ASC_2_RXD_DEV                 PIO_DEVICE_4
#define ASC_2_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_2_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_2_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_2_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_TXD_BIT                 PIO_BIT_4
#define ASC_3_RXD_BIT                 PIO_BIT_5
#define ASC_3_TXD_DEV                 PIO_DEVICE_5
#define ASC_3_RXD_DEV                 PIO_DEVICE_5
#define ASC_3_RTS_BIT                 PIO_BIT_7
#define ASC_3_CTS_BIT                 PIO_BIT_6
#define ASC_3_RTS_DEV                 PIO_DEVICE_5
#define ASC_3_CTS_DEV                 PIO_DEVICE_5
#define ASC_4_TXD_BIT                 PIO_BIT_0
#define ASC_4_RXD_BIT                 PIO_BIT_1
#define ASC_4_TXD_DEV                 PIO_DEVICE_6
#define ASC_4_RXD_DEV                 PIO_DEVICE_6
#define ASC_4_RTS_BIT                 PIO_BIT_3
#define ASC_4_CTS_BIT                 PIO_BIT_2
#define ASC_4_RTS_DEV                 PIO_DEVICE_6
#define ASC_4_CTS_DEV                 PIO_DEVICE_6
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_1

#elif defined(ST_5105) || defined(ST_5107)
#define ASC_0_TXD_BIT                 PIO_BIT_0
#define ASC_0_RXD_BIT                 PIO_BIT_1
#define ASC_0_TXD_DEV                 PIO_DEVICE_0
#define ASC_0_RXD_DEV                 PIO_DEVICE_0
#define ASC_0_RTS_BIT                 PIO_BIT_4
#define ASC_0_CTS_BIT                 PIO_BIT_5
#define ASC_0_RTS_DEV                 PIO_DEVICE_0
#define ASC_0_CTS_DEV                 PIO_DEVICE_0
#define ASC_1_TXD_BIT                 PIO_BIT_0
#define ASC_1_RXD_BIT                 PIO_BIT_1
#define ASC_1_TXD_DEV                 PIO_DEVICE_2
#define ASC_1_RXD_DEV                 PIO_DEVICE_2
#define ASC_1_RTS_BIT                 PIO_BIT_2
#define ASC_1_CTS_BIT                 PIO_BIT_3
#define ASC_1_RTS_DEV                 PIO_DEVICE_2
#define ASC_1_CTS_DEV                 PIO_DEVICE_2

#elif defined(ST_5188)
#define ASC_0_TXD_BIT                 PIO_BIT_1
#define ASC_0_RXD_BIT                 PIO_BIT_6
#define ASC_0_TXD_DEV                 PIO_DEVICE_2
#define ASC_0_RXD_DEV                 PIO_DEVICE_1
#define ASC_0_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV                 PIO_DEVICE_NOT_USED


#elif defined(ST_7710)
#define ASC_0_TXD_BIT                 PIO_BIT_0
#define ASC_0_RXD_BIT                 PIO_BIT_1
#define ASC_0_TXD_DEV                 PIO_DEVICE_0
#define ASC_0_RXD_DEV                 PIO_DEVICE_0
#define ASC_0_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_TXD_BIT                 PIO_BIT_0
#define ASC_1_RXD_BIT                 PIO_BIT_1
#define ASC_1_TXD_DEV                 PIO_DEVICE_1
#define ASC_1_RXD_DEV                 PIO_DEVICE_1
#define ASC_1_RTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_CTS_BIT                 PIO_BIT_NOT_USED
#define ASC_1_RTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_1_CTS_DEV                 PIO_DEVICE_NOT_USED
#define ASC_3_TXD_BIT                 PIO_BIT_0
#define ASC_3_RXD_BIT                 PIO_BIT_1
#define ASC_3_TXD_DEV                 PIO_DEVICE_5
#define ASC_3_RXD_DEV                 PIO_DEVICE_5
#define ASC_3_RTS_BIT                 PIO_BIT_3
#define ASC_3_CTS_BIT                 PIO_BIT_2
#define ASC_3_RTS_DEV                 PIO_DEVICE_5
#define ASC_3_CTS_DEV                 PIO_DEVICE_5
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_1

#elif defined(ST_7100) || defined (ST_7109)
/* UART2 for testtool console, UART3 for debug traces */
#define ASC_2_TXD_BIT                 PIO_BIT_3
#define ASC_2_RXD_BIT                 PIO_BIT_2
#define ASC_2_TXD_DEV                 PIO_DEVICE_4
#define ASC_2_RXD_DEV                 PIO_DEVICE_4
#define ASC_2_RTS_BIT                 PIO_BIT_6
#define ASC_2_CTS_BIT                 PIO_BIT_4
#define ASC_2_RTS_DEV                 PIO_DEVICE_4
#define ASC_2_CTS_DEV                 PIO_DEVICE_4
#define ASC_3_TXD_BIT                 PIO_BIT_0
#define ASC_3_RXD_BIT                 PIO_BIT_1
#define ASC_3_TXD_DEV                 PIO_DEVICE_5
#define ASC_3_RXD_DEV                 PIO_DEVICE_5
#define ASC_3_RTS_BIT                 PIO_BIT_3
#define ASC_3_CTS_BIT                 PIO_BIT_2
#define ASC_3_RTS_DEV                 PIO_DEVICE_5
#define ASC_3_CTS_DEV                 PIO_DEVICE_5
#elif defined(ST_7200)
/* UART2 for testtool console, UART3 for debug traces */
#define ASC_2_TXD_BIT                 PIO_BIT_3
#define ASC_2_RXD_BIT                 PIO_BIT_2
#define ASC_2_TXD_DEV                 PIO_DEVICE_4
#define ASC_2_RXD_DEV                 PIO_DEVICE_4
#define ASC_2_RTS_BIT                 PIO_BIT_5
#define ASC_2_CTS_BIT                 PIO_BIT_4
#define ASC_2_RTS_DEV                 PIO_DEVICE_4
#define ASC_2_CTS_DEV                 PIO_DEVICE_4
#define ASC_3_TXD_BIT                 PIO_BIT_4
#define ASC_3_RXD_BIT                 PIO_BIT_3
#define ASC_3_TXD_DEV                 PIO_DEVICE_5
#define ASC_3_RXD_DEV                 PIO_DEVICE_5
#define ASC_3_RTS_BIT                 PIO_BIT_6
#define ASC_3_CTS_BIT                 PIO_BIT_5
#define ASC_3_RTS_DEV                 PIO_DEVICE_5
#define ASC_3_CTS_DEV                 PIO_DEVICE_5
#endif


/************************************** Link RS232 - UART ***********************************/
/* The following associations are BOARD dependant and NOT CHIP dependant. But we keep the   */
/* check by CHIP because we suppose the consumer's boards have the same schematic.          */
/********************************************************************************************/
#if defined(ST_5510) || defined(ST_5512) || defined(ST_5514) || defined(ST_TP3)
#define UART_BASEADDRESS              ASC_3_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_3_INTERRUPT
#define UART_IT_LEVEL                 ASC_3_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_3_RXD_DEV
#define UART_RXD_BITMASK              ASC_3_RXD_BIT
#define UART_TXD_PORTNAME_IDX         ASC_3_TXD_DEV
#define UART_TXD_BITMASK              ASC_3_TXD_BIT
#define UART_RTS_PORTNAME_IDX         ASC_3_RTS_DEV
#define UART_RTS_BITMASK              ASC_3_RTS_BIT
#define UART_CTS_PORTNAME_IDX         ASC_3_CTS_DEV
#define UART_CTS_BITMASK              ASC_3_CTS_BIT

#elif defined(ST_5516) || defined(ST_5517)
#ifdef RS232_A
#define UART_BASEADDRESS              ASC_3_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_3_INTERRUPT
#define UART_IT_LEVEL                 ASC_3_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_3_RXD_DEV
#define UART_RXD_BITMASK              ASC_3_RXD_BIT
#define UART_TXD_PORTNAME_IDX         ASC_3_TXD_DEV
#define UART_TXD_BITMASK              ASC_3_TXD_BIT
#define UART_RTS_PORTNAME_IDX         ASC_3_RTS_DEV
#define UART_RTS_BITMASK              ASC_3_RTS_BIT
#define UART_CTS_PORTNAME_IDX         ASC_3_CTS_DEV
#define UART_CTS_BITMASK              ASC_3_CTS_BIT
#endif /* RS232_A */

#ifdef RS232_B
#define UART_BASEADDRESS              ASC_2_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_2_INTERRUPT
#define UART_IT_LEVEL                 ASC_2_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_2_RXD_DEV
#define UART_RXD_BITMASK              ASC_2_RXD_BIT
#define UART_TXD_PORTNAME_IDX         ASC_2_TXD_DEV
#define UART_TXD_BITMASK              ASC_2_TXD_BIT
#define UART_RTS_PORTNAME_IDX         ASC_2_RTS_DEV
#define UART_RTS_BITMASK              ASC_2_RTS_BIT
#define UART_CTS_PORTNAME_IDX         ASC_2_CTS_DEV
#define UART_CTS_BITMASK              ASC_2_CTS_BIT
#endif /* RS232_B */

#define INTERCONNECT_BASE                       (U32)CFG_BASE_ADDRESS
#define INTERCONNECT_CONFIG_CONTROL_REG_C       0x08
#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_E       0x28

#define CONFIG_CONTROL_C (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_C)
#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_E (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_E)

#elif defined(ST_5508) || defined(ST_5518) || defined(ST_5578)
#define UART_BASEADDRESS              ASC_2_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_2_INTERRUPT
#define UART_IT_LEVEL                 ASC_2_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_2_RXD_DEV
#define UART_RXD_BITMASK              ASC_2_RXD_BIT;
#define UART_TXD_PORTNAME_IDX         ASC_2_TXD_DEV
#define UART_TXD_BITMASK              ASC_2_TXD_BIT;
#define UART_RTS_PORTNAME_IDX         ASC_2_RTS_DEV
#define UART_RTS_BITMASK              ASC_2_RTS_BIT;
#define UART_CTS_PORTNAME_IDX         ASC_2_CTS_DEV
#define UART_CTS_BITMASK              ASC_2_CTS_BIT;
#endif

#if defined(ST_GX1)
#define UART_BASEADDRESS              ASC_0_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_0_INTERRUPT
#define UART_IT_LEVEL                 0 /* ASC_0_INTERRUPT_LEVEL */ /* Use default interrupt levels if running on ST40 architecture */
#define UART_RXD_PORTNAME_IDX         ASC_0_RXD_DEV
#define UART_RXD_BITMASK              ASC_0_RXD_BIT;
#define UART_TXD_PORTNAME_IDX         ASC_0_TXD_DEV
#define UART_TXD_BITMASK              ASC_0_TXD_BIT;
#define UART_RTS_PORTNAME_IDX         ASC_0_RTS_DEV
#define UART_RTS_BITMASK              ASC_0_RTS_BIT;
#define UART_CTS_PORTNAME_IDX         ASC_0_CTS_DEV
#define UART_CTS_BITMASK              ASC_0_CTS_BIT;
#endif /* GX1 */

#if defined(ST_5528) || defined(ST_5100) || defined (ST_7710) || defined(ST_5301)|| defined(ST_5525)
#define UART_BASEADDRESS              ASC_3_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_3_INTERRUPT
#define UART_IT_LEVEL                 ASC_3_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_3_RXD_DEV
#define UART_RXD_BITMASK              ASC_3_RXD_BIT
#define UART_TXD_PORTNAME_IDX         ASC_3_TXD_DEV
#define UART_TXD_BITMASK              ASC_3_TXD_BIT
#define UART_RTS_PORTNAME_IDX         ASC_3_RTS_DEV
#define UART_RTS_BITMASK              ASC_3_RTS_BIT
#define UART_CTS_PORTNAME_IDX         ASC_3_CTS_DEV
#define UART_CTS_BITMASK              ASC_3_CTS_BIT
#endif /* ST_5528 || ST_5100 || ST_7710 || ST_5301 */

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
#define UART_BASEADDRESS              ASC_2_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_2_INTERRUPT
#define UART_IT_LEVEL                 ASC_2_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_2_RXD_DEV
#define UART_RXD_BITMASK              ASC_2_RXD_BIT
#define UART_TXD_PORTNAME_IDX         ASC_2_TXD_DEV
#define UART_TXD_BITMASK              ASC_2_TXD_BIT
#define UART_RTS_PORTNAME_IDX         ASC_2_RTS_DEV
#define UART_RTS_BITMASK              ASC_2_RTS_BIT
#define UART_CTS_PORTNAME_IDX         ASC_2_CTS_DEV
#define UART_CTS_BITMASK              ASC_2_CTS_BIT
#endif /* ST_7100 || ST_7109 || ST_7200*/

#if defined(ST_5105) || defined(ST_5107)
#define UART_BASEADDRESS              ASC_1_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_1_INTERRUPT
#define UART_IT_LEVEL                 ASC_1_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_1_RXD_DEV
#define UART_RXD_BITMASK              ASC_1_RXD_BIT
#define UART_TXD_PORTNAME_IDX         ASC_1_TXD_DEV
#define UART_TXD_BITMASK              ASC_1_TXD_BIT
#define UART_RTS_PORTNAME_IDX         ASC_1_RTS_DEV
#define UART_RTS_BITMASK              ASC_1_RTS_BIT
#define UART_CTS_PORTNAME_IDX         ASC_1_CTS_DEV
#define UART_CTS_BITMASK              ASC_1_CTS_BIT
#endif /* ST_5105 ST_5107 */

#if defined(ST_5188)
#define UART_BASEADDRESS              ASC_0_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_0_INTERRUPT
#define UART_IT_LEVEL                 ASC_0_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_0_RXD_DEV
#define UART_RXD_BITMASK              ASC_0_RXD_BIT
#define UART_TXD_PORTNAME_IDX         ASC_0_TXD_DEV
#define UART_TXD_BITMASK              ASC_0_TXD_BIT
#define UART_RTS_PORTNAME_IDX         ASC_0_RTS_DEV
#define UART_RTS_BITMASK              ASC_0_RTS_BIT
#define UART_CTS_PORTNAME_IDX         ASC_0_CTS_DEV
#define UART_CTS_BITMASK              ASC_0_CTS_BIT
#endif /* ST_5188 */



/************************************************************/

#if defined(ST_5508) || defined(ST_5510) || defined(ST_TP3)|| defined(ST_5188)
#define ASC_DEVICE_TYPE               STUART_16_BYTE_FIFO
#elif defined(ST_5512) || defined(ST_5518) || defined(ST_5578) || \
      defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || \
      defined(ST_GX1)  || defined(ST_5528) || defined(ST_5100) || \
      defined(ST_7710) || defined(ST_5105) || defined(ST_7100) || \
      defined(ST_5301) || defined(ST_7109) || defined(ST_5525) || \
      defined(ST_5107) || defined(ST_7200)
#define ASC_DEVICE_TYPE               STUART_RTSCTS
#endif

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

#ifdef TRACE_UART
STUART_Handle_t TraceUartHandle;
#endif

extern ST_Partition_t *DriverPartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/* CL: Temporary addition to overcome bad comms clock settings in board vob for mb411  */
#if defined(ST_7100) || defined(ST_7109)
typedef struct
{
	U32 sh4_clk;                      /* en Hz */
	U32 sh4_ic_clk;
	U32 sh4_per_clk;
	U32 slim_clk;
	U32 mpeg2_clk;
	U32 lx_dh_clk;
	U32 lx_aud_clk;
	U32 lmi2x_sys_clk;
	U32 lmi2x_vid_clk;
	U32 ic_clk;
	U32 icdiv2_emi_clk;
} CkgA_Clock_t;

/* Private Function prototypes ---------------------------------------------- */
int ST_CkgA_GetClock(CkgA_Clock_t *Clock);

/* ========================================================================
   Name:        ST_CkgA_GetClock
   Description: Get CKGA clocks frequencies function

   ======================================================================== */
int ST_CkgA_GetClock(CkgA_Clock_t *Clock)
{
/* PLL1 control */
	#define PLL1_CTRL           0x008
	#define PLL1_CTRL2          0x00c
	#define PLL1_LOCK_STATUS    0x010
	#define PLL1_CLK1_CTRL      0x014
	#define PLL1_CLK2_CTRL      0x018
	#define PLL1_CLK3_CTRL      0x01c
	#define PLL1_CLK4_CTRL      0x020
	/* PLL2 control */
	#define PLL2_CTRL           0x024


	U32 data;
    int mdiv, ndiv, pdiv;
    int pll1frq, infrq, clkfrq = 0;

	/* PLL1 clocks */
#if defined(ST_7100)
    data = STSYS_ReadRegDev32LE( ST7100_CKG_A_BASE_ADDRESS + PLL1_CTRL );
#elif defined(ST_7109)
    data = STSYS_ReadRegDev32LE( ST7109_CKG_A_BASE_ADDRESS + PLL1_CTRL );
#endif
    mdiv = (data&0xFF);
    ndiv =((data>>8)&0xFF);
    pdiv =((data>>16)&0x7);
    pll1frq = (((2*BOARD71XX_WA_OSC_VALUE*ndiv)/mdiv) / (1 << pdiv))*1000000;
    infrq = pll1frq / 2;

	/* SH4 clock */
#if defined(ST_7100)
	data = STSYS_ReadRegDev32LE( ST7100_CKG_A_BASE_ADDRESS + PLL1_CLK1_CTRL );
#elif defined(ST_7109)
    data = STSYS_ReadRegDev32LE( ST7109_CKG_A_BASE_ADDRESS + PLL1_CLK1_CTRL );
#endif
	data = data & 0x7;
	switch (data)
	{
		case 0 :
			clkfrq = infrq;
			break;
		case 1 :
			clkfrq = infrq / 2;
			break;
		case 2 :
			clkfrq = infrq / 3;
			break;
		case 3 :
			clkfrq = infrq / 4;
			break;
		case 4 :
			clkfrq = infrq / 6;
			break;
		case 5 :
			clkfrq = infrq / 8;
			break;
		case 6 :
			clkfrq = infrq;
			break;
		case 7 :
			clkfrq = infrq;
			break;
		default :
			break;
	}
	Clock->sh4_clk = clkfrq;

	/* SH4_INTERCO clock */
#if defined(ST_7100)
	data = STSYS_ReadRegDev32LE( ST7100_CKG_A_BASE_ADDRESS + PLL1_CLK2_CTRL );
#elif defined(ST_7109)
    data = STSYS_ReadRegDev32LE( ST7109_CKG_A_BASE_ADDRESS + PLL1_CLK2_CTRL );
#endif
	data = data & 0x7;
	switch (data)
	{
		case 0 :
			clkfrq = infrq;
			break;
		case 1 :
			clkfrq = infrq / 2;
			break;
		case 2 :
			clkfrq = infrq / 3;
			break;
		case 3 :
			clkfrq = infrq / 4;
			break;
		case 4 :
			clkfrq = infrq / 6;
			break;
		case 5 :
			clkfrq = infrq / 8;
			break;
		case 6 :
			clkfrq = infrq;
			break;
		case 7 :
			clkfrq = infrq;
			break;
		default :
			break;
	}
	Clock->sh4_ic_clk = clkfrq;

	/* SH4_PERIPHERAL clock */
#if defined(ST_7100)
	data = STSYS_ReadRegDev32LE( ST7100_CKG_A_BASE_ADDRESS + PLL1_CLK3_CTRL );
#elif defined(ST_7109)
	data = STSYS_ReadRegDev32LE( ST7109_CKG_A_BASE_ADDRESS + PLL1_CLK3_CTRL );
#endif
	data = data & 0x7;
	switch (data)
	{
		case 0 :
			clkfrq = infrq;
			break;
		case 1 :
			clkfrq = infrq / 2;
			break;
		case 2 :
			clkfrq = infrq / 3;
			break;
		case 3 :
			clkfrq = infrq / 4;
			break;
		case 4 :
			clkfrq = infrq / 6;
			break;
		case 5 :
			clkfrq = infrq / 8;
			break;
		case 6 :
			clkfrq = infrq;
			break;
		case 7 :
			clkfrq = infrq;
			break;
		default :
			break;
	}
	Clock->sh4_per_clk = clkfrq;

	/* SLIM and MPEG2 clocks */
#if defined(ST_7100)
	data = STSYS_ReadRegDev32LE( ST7100_CKG_A_BASE_ADDRESS + PLL1_CLK4_CTRL );
#elif defined(ST_7109)
	data = STSYS_ReadRegDev32LE( ST7109_CKG_A_BASE_ADDRESS + PLL1_CLK4_CTRL );
#endif
	data = data & 0x7;
	switch (data)
	{
		case 0 :
			clkfrq = infrq;
			break;
		case 1 :
			clkfrq = infrq / 2;
			break;
		case 2 :
			clkfrq = infrq / 3;
			break;
		case 3 :
			clkfrq = infrq / 4;
			break;
		case 4 :
			clkfrq = infrq / 6;
			break;
		case 5 :
			clkfrq = infrq / 8;
			break;
		case 6 :
			clkfrq = infrq;
			break;
		case 7 :
			clkfrq = infrq;
			break;
		default :
			break;
	}
	Clock->slim_clk = clkfrq;
	Clock->mpeg2_clk = clkfrq;

	/* PLL2 clocks */
#if defined(ST_7100)
	data = STSYS_ReadRegDev32LE( ST7100_CKG_A_BASE_ADDRESS + PLL2_CTRL );
#elif defined(ST_7109)
	data = STSYS_ReadRegDev32LE( ST7109_CKG_A_BASE_ADDRESS + PLL2_CTRL );
#endif
	mdiv = (data&0xFF);
	ndiv=((data>>8)&0xFF);
	pdiv =((data>>16)&0x7);
    pll1frq = (((2*BOARD71XX_WA_OSC_VALUE*ndiv)/mdiv) / (1 << pdiv))*1000000;

	/* LX Delta Phi clock */
	Clock->lx_dh_clk = pll1frq;

	/* LX Audio clock */
	Clock->lx_aud_clk = pll1frq;

	/* LMI2X sys clock */
	Clock->lmi2x_sys_clk = pll1frq;

	/* LMI2X vid clock */
	Clock->lmi2x_vid_clk = pll1frq;

	/* STBUS clock */
	Clock->ic_clk = pll1frq / 2;

	/* EMI clock */
	Clock->icdiv2_emi_clk = pll1frq / 4;

	return 0;
}
#endif /* ST_7100 || ST_7109 */

/*******************************************************************************
Name        : UART_Init
Description : Initialise the UART drivers
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL UART_Init(void)
{
#if defined(ST_7100) || defined(ST_7109)
    CkgA_Clock_t CkgAClock;
 #endif
    BOOL RetOk = TRUE;
    char Msg[256];
    ST_ErrorCode_t ErrorCode;
    STUART_Params_t UartDefaultParams;
    STUART_InitParams_t UartInitParams;
    ST_ClockInfo_t ClockInfo;

    ST_GetClockInfo(&ClockInfo);

    memset(&UartInitParams, 0, sizeof(UartInitParams));
    UartInitParams.RXD.PortName[0] = '\0';
    UartInitParams.TXD.PortName[0] = '\0';
    UartInitParams.RTS.PortName[0] = '\0';
    UartInitParams.CTS.PortName[0] = '\0';

    UartInitParams.UARTType        = ASC_DEVICE_TYPE;
    UartInitParams.DriverPartition = DriverPartition_p;
    UartInitParams.BaseAddress     = (U32 *)UART_BASEADDRESS;
    UartInitParams.InterruptNumber = UART_IT_NUMBER;
    UartInitParams.InterruptLevel  = UART_IT_LEVEL;
    UartInitParams.ClockFrequency  = ClockInfo.CommsBlock;
    UartInitParams.SwFIFOEnable    = TRUE;
    UartInitParams.FIFOLength      = 4096;
    strcpy(UartInitParams.RXD.PortName, PioDeviceName[UART_RXD_PORTNAME_IDX]);
    UartInitParams.RXD.BitMask = UART_RXD_BITMASK;
    strcpy(UartInitParams.TXD.PortName, PioDeviceName[UART_TXD_PORTNAME_IDX]);
    UartInitParams.TXD.BitMask = UART_TXD_BITMASK;
    strcpy(UartInitParams.RTS.PortName, PioDeviceName[UART_RTS_PORTNAME_IDX]);
    UartInitParams.RTS.BitMask = UART_RTS_BITMASK;
    strcpy(UartInitParams.CTS.PortName, PioDeviceName[UART_CTS_PORTNAME_IDX]);
    UartInitParams.CTS.BitMask = UART_CTS_BITMASK;

    UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
    UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
    UartDefaultParams.RxMode.BaudRate = UART_BAUD_RATE;
    UartDefaultParams.RxMode.TermString = NULL;
    UartDefaultParams.RxMode.TimeOut = 1; /* As short as possible */
    UartDefaultParams.RxMode.NotifyFunction = NULL; /* No callback */

    UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
    UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
    UartDefaultParams.TxMode.BaudRate = UART_BAUD_RATE;
    UartDefaultParams.TxMode.TermString = NULL;
    UartDefaultParams.TxMode.TimeOut = 0; /* No time-out */
    UartDefaultParams.TxMode.NotifyFunction = NULL; /* No callback */

    UartDefaultParams.SmartCardModeEnabled = FALSE;
    UartDefaultParams.GuardTime = 0;

    UartInitParams.DefaultParams = &UartDefaultParams;

#if defined(ST_7100) || defined(ST_7109)
/* CL quick fix to get UART working properly on mb411 */
    ST_CkgA_GetClock(&CkgAClock);
    UartInitParams.ClockFrequency  = CkgAClock.icdiv2_emi_clk;
#endif
#if defined(ST_7200)
		/*COMMS clock clk_ic_100*/
		/* Need to call Getclock function to determine if 100MHz or 133MHz */
		UartInitParams.ClockFrequency  = 100000000;
#endif
    ErrorCode = STUART_Init(STUART_DEVICE_NAME, &UartInitParams);
    if ( ErrorCode != ST_NO_ERROR)
    {
        /* error handling */
        RetOk = FALSE;
        sprintf(Msg,"Error: UART Init failed ! Error 0x%0x\n", ErrorCode);
        debugmessage(Msg);
    }
    else
    {
        sprintf(Msg,"STUART initialized, \trevision=%s\n", STUART_GetRevision() );
        debugmessage(Msg);
    }
#if (defined(ST_7100) || defined(ST_7109)|| defined(ST_7200)) && defined(TRACE_UART)
    /* Testtool on ASC2, traces on ASC3 required. Reuse same config 115200 bauds 8bits/Even */
    UartInitParams.BaseAddress     = (U32 *)ASC_3_BASE_ADDRESS;
    UartInitParams.InterruptNumber = ASC_3_INTERRUPT;
    UartInitParams.InterruptLevel  = ASC_3_INTERRUPT_LEVEL;
    strcpy(UartInitParams.RXD.PortName, PioDeviceName[ASC_3_RXD_DEV]);
    UartInitParams.RXD.BitMask = ASC_3_RXD_BIT;
    strcpy(UartInitParams.TXD.PortName, PioDeviceName[ASC_3_TXD_DEV]);
    UartInitParams.TXD.BitMask = ASC_3_TXD_BIT;
    strcpy(UartInitParams.RTS.PortName, PioDeviceName[ASC_3_RTS_DEV]);
    UartInitParams.RTS.BitMask = ASC_3_RTS_BIT;
    strcpy(UartInitParams.CTS.PortName, PioDeviceName[ASC_3_CTS_DEV]);
    UartInitParams.CTS.BitMask = ASC_3_CTS_BIT;
    ErrorCode = STUART_Init(TRACE_UART_DEVICE, &UartInitParams);
    if ( ErrorCode != ST_NO_ERROR)
    {
        /* error handling */
        RetOk = FALSE;
        sprintf(Msg,"Error: UART Init for debug traces failed ! Error 0x%0x\n", ErrorCode);
        debugmessage(Msg);
    }
    else
    {
        sprintf(Msg,"STUART ASC3 for debug traces initialized, \trevision=%s\n", STUART_GetRevision() );
        debugmessage(Msg);
    }
#endif

#if defined(ST_5514)
    /* For the STi5514 there are two registers we must configure to
     * allow the ASC2 to be used for RS232:
     *
     * IRB_ASC_CONTROL => Must be set for ASC2 TXD and RXD to
     *                    bypass the IRDA Data part of IRB module.
     * CFG_CONTROL_C   => Configure the PIO MUX for IRB operation
     *                    and not IrDA Control.
     */
    STSYS_WriteRegDev32LE(((U32)IRB_BASE_ADDRESS + 0xD0), STSYS_ReadRegDev32LE(((U32)IRB_BASE_ADDRESS + 0xD0)) | 1);
    STSYS_WriteRegDev32LE(((U32)CFG_BASE_ADDRESS + 0x08), STSYS_ReadRegDev32LE(((U32)CFG_BASE_ADDRESS + 0x08)) | 0x10);

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
#elif defined(ST_5301) || defined(ST_5100) || defined(ST_5525)
    /* Configure PIOs */
    STSYS_WriteRegDev8(PIO_4_BASE_ADDRESS+0x20, 0x0);
    STSYS_WriteRegDev8(PIO_4_BASE_ADDRESS+0x30, 0x1);
    STSYS_WriteRegDev8(PIO_4_BASE_ADDRESS+0x40, 0x1);
#endif


    return(RetOk);

} /* end UART_Init() */


#ifdef TRACE_UART
/*******************************************************************************
Name        : UART_Open
Description : Open a connection
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL UART_Open(void)
{
    BOOL RetOk = TRUE;
    char Msg[256];
    ST_ErrorCode_t ErrorCode;
    STUART_OpenParams_t UartOpenParams;

    UartOpenParams.LoopBackEnabled = FALSE;   /* NO internal loopback */
    UartOpenParams.FlushIOBuffers  = TRUE;    /* Flush before open */
    UartOpenParams.DefaultParams   = NULL;    /* Don't overwrite default parameters set at init */
#if defined(ST_7100) || defined (ST_7109) || defined(ST_7200)
    /* Testtool on STUART_DEVICE_NAME (ASC2), traces on TRACE_UART_DEVICE (ASC3) */
    ErrorCode = STUART_Open(TRACE_UART_DEVICE, &UartOpenParams, &TraceUartHandle);
#else
    ErrorCode = STUART_Open(STUART_DEVICE_NAME, &UartOpenParams, &TraceUartHandle);
#endif
    if ( ErrorCode != ST_NO_ERROR)
    {
        /* error handling */
        RetOk = FALSE;
        sprintf(Msg,"Error: UART Open failed ! Error 0x%0x\n", ErrorCode);
        debugmessage(Msg);
    }
    else
    {
        debugmessage("STUART Opened\n");
    }
    return(RetOk);

} /* end UART_Open() */

/*******************************************************************************
Name        : UART_Close
Description : Close the UART connection
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void UART_Close(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    char Msg[256];

    ErrorCode = STUART_Close(TraceUartHandle);
    if ( ErrorCode != ST_NO_ERROR)
    {
        /* error handling */
        sprintf(Msg,"Error: trace UART closing failed ! Error 0x%0x\n", ErrorCode);
        debugmessage(Msg);
    }
    else
    {
        debugmessage("trace UART closed\n");
    }
} /* end UART_Close() */

#endif /* #ifdef TRACE_UART */

/*******************************************************************************
Name        : UART_Term
Description : Terminate the UART driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void UART_Term(void)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STUART_TermParams_t TermParams;
    char Msg[256];

    TermParams.ForceTerminate = FALSE;

    ErrorCode = STUART_Term(STUART_DEVICE_NAME, &TermParams);
    if (ErrorCode == ST_ERROR_OPEN_HANDLE)
    {
        sprintf(Msg,"STUART_Term(%s): Warning !! Still open handle\n", STUART_DEVICE_NAME);
        debugmessage(Msg);
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STUART_Term(STUART_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        sprintf(Msg,"STUART_Term(%s): ok\n", STUART_DEVICE_NAME);
    }
    else
    {
        sprintf(Msg,"STUART_Term(%s): error 0x%0x\n", STUART_DEVICE_NAME, ErrorCode);
    }
    debugmessage(Msg);
} /* end UART_Term() */


#ifdef TEST_UART_DRIVER
/*******************************************************************************
Name        : Test_Uart_Driver_fn
Description : just makes an open+write+close of STUART to test it
Parameters  : none
Assumptions : STUART initialized
Limitations :
Returns     : none
*******************************************************************************/
void Test_Uart_Driver_fn(void)
{
    STUART_OpenParams_t UartOpenParams;
    STUART_Handle_t UartHandle;
    char UartTab[2];
    U32 UARTNumberWritten;
    ST_ErrorCode_t ErrorCode;

    ErrorCode = ST_NO_ERROR;
    strcpy(UartTab, "");

    /* Open UART if support is required */
    UartOpenParams.LoopBackEnabled = FALSE; /* NO internal loopback */
    UartOpenParams.FlushIOBuffers  = TRUE;   /* Flush before open */
    UartOpenParams.DefaultParams   = NULL;    /* Don't overwrite default parameters set at init */
    ErrorCode = STUART_Open(STUART_DEVICE_NAME, &UartOpenParams, &UartHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        debugmessage("UART failed to open !\n");
    }
    else
    {
        debugmessage("UART opened just to check it works.\n");

        ErrorCode = STUART_Write(UartHandle, (U8 *) UartTab, 0, &UARTNumberWritten, 0);
        if ((ErrorCode != ST_NO_ERROR) && (ErrorCode != ST_ERROR_INVALID_HANDLE))
        {
            printf("Could not write on UART (ErrorCode = 0x%0x) !\n", ErrorCode);
        }

        ErrorCode = STUART_Close(UartHandle);
        if(ErrorCode != ST_NO_ERROR)
        {
            /* error handling: UART failed to open in toolbox ! */
            debugmessage("UART failed to close !\n");
        }
        else
        {
            debugmessage("UART closed\n");
        }
    }
} /* end of Test_Uart_Driver_fn() */
#endif /* #ifdef TEST_UART_DRIVER */

#endif /* USE_UART */
/* End of cluart.c */
