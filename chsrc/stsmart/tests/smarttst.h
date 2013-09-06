/*****************************************************************************
File Name   : smarttst.h

Description : STSMART test harness header file.

Copyright (C) 2006 STMicroelectronics

Revision History :

    10/04/00    Added device map information.

Reference   :

ST API Definition "SMART Driver API" DVD-API-06
*****************************************************************************/

#ifndef __SMART_TEST_H
#define __SMART_TEST_H

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Some chips have extra ASC functionality */
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || \
    defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || \
    defined(ST_5105) || defined(ST_7100) || defined(ST_5301) || \
    defined(ST_8010) || defined(ST_7109) || defined(ST_5525) || \
    defined(ST_5107) || defined(ST_7200)
#define UART_F_PRESENT
#endif

/* Debug message prefix string (always appended with a ':') */
#define SMART_DEBUG_PREFIX           "STSMART"

/* Zero out test result structure */
#define TEST_RESULT_ZERO    {0, 0}

#define MAXBLOCKLENGTH      3 + 254 + 2
#define MAXCARDRESPONSE     15
#define MAXSTRING           255

/*****************************************************************************
Device enumerations for SC, ASC and PIO
*****************************************************************************/

/* Enum of SC devices */
enum
{
    SC_DEVICE_0,
    SC_DEVICE_1,
    SC_DEVICE_NOT_USED
};

#if defined(ST_OSLINUX)
#define MAX_SC    2
#elif defined(ST_5105) || defined(ST_8010) || defined(ST_5107)
#define MAX_SC                  1       /* Number of smartcards */
#else
#define MAX_SC                  2       /* Number of smartcards */
#endif


/* Enum of ASC devices */
enum
{
    ASC_DEVICE_0,
    ASC_DEVICE_1,
    ASC_DEVICE_2,
    ASC_DEVICE_3,
    ASC_DEVICE_NOT_USED
};

#ifdef ST_OSLINUX
#define MAX_ASC                 2
#elif defined(ST_5107)
#define MAX_ASC                 2
#elif defined(ST_5105)
#define MAX_ASC                 1       /* Number of ASCs */
#elif defined(ST_8010) 
#define MAX_ASC                 3
#else
#define MAX_ASC                 4
#endif

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

/* Maximum total PIO devices we support */
#if defined(ST_5105) || defined(ST_5107)
#define MAX_PIO                  4
#else
#define MAX_PIO                  6
#endif

/* Now define the number of PIOs that are actually on the current
 * platform
 */
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5100) \
   || defined(ST_7710) || defined(ST_7100) || defined(ST_5301) || defined(ST_7109)
    #define MAX_ACTUAL_PIO          6
#elif defined(ST_5528) || defined(ST_5525) || defined(ST_7200)
    #define MAX_ACTUAL_PIO          8
#elif defined(ST_5105) || defined(ST_5107)
    #define MAX_ACTUAL_PIO          4
#elif defined(ST_8010)
    #define MAX_ACTUAL_PIO          7
#else
    #define MAX_ACTUAL_PIO          5
#endif

/* Enum of EVT devices */
enum
{
    EVT_DEVICE_0,
    EVT_DEVICE_1,
    EVT_DEVICE_NOT_USED
};

#if defined(ST_5100)
#ifndef PIO_4_INTERRUPT
#define PIO_4_INTERRUPT     ST5100_PIO4_INTERRUPT
#endif
#ifndef PIO_5_INTERRUPT
#define PIO_5_INTERRUPT     ST5100_PIO5_INTERRUPT
#endif
#ifndef PIO_4_BASE_ADDRESS
#define PIO_4_BASE_ADDRESS  ST5100_PIO4_BASE_ADDRESS
#endif
#ifndef PIO_5_BASE_ADDRESS
#define PIO_5_BASE_ADDRESS  ST5100_PIO5_BASE_ADDRESS
#endif
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

#elif defined(ST_5105) || defined(ST_5107)

#define INTERCONNECT_BASE                        (U32)0x20402000
#define INTERCONNECT_MONITOR_CONTROL_REG_G       0x0030  /* 4 bytes access */
#define INTERCONNECT_CONFIG_CONTROL_REG_F        0x000C
#define INTERCONNECT_CONFIG_CONTROL_REG_E        0x0008
#define INTERCONNECT_CONFIG_CONTROL_REG_H        0x0014
#define CONFIG_MONITOR_G ((U32)0x20402000 + INTERCONNECT_MONITOR_CONTROL_REG_G)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_E (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_E)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)

#elif defined(ST_7100) || defined(ST_7109)
#define INTERCONNECT_BASE                 (U32)0xB9001000
#define SYS_CONFIG_7                      0x11C
#define SYSTEM_CONFIG_7 (INTERCONNECT_BASE + SYS_CONFIG_7)

#elif defined(ST_7200)
#define INTERCONNECT_BASE 		ST7200_CFG_BASE_ADDRESS
#define SYS_CONFIG_7 			0x11C
#define SYSTEM_CONFIG_7 			(INTERCONNECT_BASE + SYS_CONFIG_7)
#endif


#define MAX_EVT                 MAX_SC

/*****************************************************************************
Definitions for PIO
*****************************************************************************/

/* Bit definition for an unused PIO bit */
#define PIO_BIT_NOT_USED        0

/*****************************************************************************
Definitions for ASC
*****************************************************************************/

/* Define ASC device type, based on ASC functionality */
#if defined(UART_F_PRESENT)
#define ASC_DEVICE_TYPE         STUART_ISO7816
#else
#define ASC_DEVICE_TYPE         STUART_16_BYTE_FIFO
#endif

/* Useful aliase names for tests */

#if defined(mb275)

#define SC_DEFAULT_SLOT         SC_DEVICE_1

/* There is no slot 0 so we set it to 1 */
#define SC_SLOT_0               SC_DEVICE_1
#define SC_SLOT_1               SC_DEVICE_1

#else

#define SC_DEFAULT_SLOT         SC_DEVICE_0
#define SC_SLOT_0               SC_DEVICE_0
#define SC_SLOT_1               SC_DEVICE_1

#endif

/* Port pins for ASC */

#if defined(ST_5508) || defined(ST_5518)

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

#elif defined(ST_8010)

#define ASC_0_TXD_BIT   PIO_BIT_4
#define ASC_0_RXD_BIT   PIO_BIT_5
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#define ASC_1_TXD_BIT   PIO_BIT_0 /* wrong values */
#define ASC_1_RXD_BIT   PIO_BIT_1 /* wrong values */
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

#elif defined(ST_5107)

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
#define ASC_1_TXD_DEV   PIO_DEVICE_2
#define ASC_1_RXD_DEV   PIO_DEVICE_2
#define ASC_1_RTS_BIT   PIO_BIT_2
#define ASC_1_CTS_BIT   PIO_BIT_3
#define ASC_1_RTS_DEV   PIO_DEVICE_2
#define ASC_1_CTS_DEV   PIO_DEVICE_2

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || \
      defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || \
      defined(ST_7100) || defined(ST_5301) || defined(ST_7109) || \
      defined(ST_5525) || defined(ST_7200)
      
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

#if defined(ST_5100) || defined(ST_5301) || defined(ST_5525)

#define ASC_2_TXD_BIT   PIO_BIT_1
#define ASC_2_RXD_BIT   PIO_BIT_2
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
#define ASC_3_RTS_BIT   PIO_BIT_2
#define ASC_3_CTS_BIT   PIO_BIT_3
#define ASC_3_RTS_DEV   PIO_DEVICE_4
#define ASC_3_CTS_DEV   PIO_DEVICE_4

#elif defined(ST_7100) || defined(ST_7109)

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
#define ASC_3_RTS_BIT   PIO_BIT_5
#define ASC_3_CTS_BIT   PIO_BIT_6
#define ASC_3_RTS_DEV   PIO_DEVICE_5
#define ASC_3_CTS_DEV   PIO_DEVICE_5

#else

#define ASC_2_TXD_BIT   PIO_BIT_4
#define ASC_2_RXD_BIT   PIO_BIT_3
#define ASC_2_TXD_DEV   PIO_DEVICE_4
#define ASC_2_RXD_DEV   PIO_DEVICE_4
#define ASC_2_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_2_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_2_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_2_CTS_DEV   PIO_DEVICE_NOT_USED

#if defined(ST_7710)
#define ASC_3_TXD_BIT   PIO_BIT_0
#define ASC_3_RXD_BIT   PIO_BIT_1
#define ASC_3_RTS_BIT   PIO_BIT_3
#define ASC_3_CTS_BIT   PIO_BIT_2
#else
#define ASC_3_TXD_BIT   PIO_BIT_4
#define ASC_3_RXD_BIT   PIO_BIT_5
#define ASC_3_RTS_BIT   PIO_BIT_7
#define ASC_3_CTS_BIT   PIO_BIT_6
#endif

#define ASC_3_TXD_DEV   PIO_DEVICE_5
#define ASC_3_RXD_DEV   PIO_DEVICE_5
#define ASC_3_RTS_DEV   PIO_DEVICE_5
#define ASC_3_CTS_DEV   PIO_DEVICE_5

#endif /* inner #ifdef */

#elif defined(ST_5105)

#define ASC_0_TXD_BIT   PIO_BIT_0
#define ASC_0_RXD_BIT   PIO_BIT_1
#define ASC_0_TXD_DEV   PIO_DEVICE_0
#define ASC_0_RXD_DEV   PIO_DEVICE_0
#define ASC_0_RTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_CTS_BIT   PIO_BIT_NOT_USED
#define ASC_0_RTS_DEV   PIO_DEVICE_NOT_USED
#define ASC_0_CTS_DEV   PIO_DEVICE_NOT_USED

#else /* defined(ST_5510) || defined(ST_5512) || defined(ST_TP3) */

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
#define ASC_3_RTS_BIT   PIO_BIT_3
#define ASC_3_CTS_BIT   PIO_BIT_4
#define ASC_3_RTS_DEV   PIO_DEVICE_4
#define ASC_3_CTS_DEV   PIO_DEVICE_4

#endif

/*****************************************************************************
Definitions for smartcard
*****************************************************************************/

/* SC Device Type */

#define SC_DEVICE_TYPE  STSMART_ISO

/* CLOCK macros */
#if defined(ARCHITECTURE_ST20) || (defined(ST_OS21) && defined(ST_5528))
#define CLOCK_CPU           ClockInfo.C2
#define TICKS_PER_SECOND    ST_GetClocksPerSecond()
#define CLOCK_COMMS         ClockInfo.CommsBlock

#elif defined(ARCHITECTURE_ST200)
#define CLOCK_CPU           ClockInfo.ST200
#define TICKS_PER_SECOND    ST_GetClocksPerSecond()
#define CLOCK_COMMS         ClockInfo.CommsBlock

#elif defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
#define CLOCK_CPU           ClockInfo.ST40
#define TICKS_PER_SECOND    ST_GetClocksPerSecond()
#define CLOCK_COMMS         ClockInfo.CommsBlock

#else
#define CLOCK_CPU           ST_GetClockSpeed()
#define TICKS_PER_SECOND    ST_GetClocksPerSecond()
#if defined(ST_GX1) /* Workaround for cut 1.0 of nGX1 - per_clk not correctly connected */
#define CLOCK_COMMS         boardInfo->peripheralClockFrequency
#else
#define CLOCK_COMMS         50000000
#endif
#endif

#define CLOCK_SC            ClockInfo.SmartCard


/* Resources for SC  */

#if defined(ST_5508) || defined(ST_5518)

#define SC_0_EXTCLK_BIT PIO_BIT_2
#define SC_0_EXTCLK_DEV PIO_DEVICE_1
#define SC_0_CLK_BIT    PIO_BIT_3
#define SC_0_CLK_DEV    PIO_DEVICE_0
#define SC_0_VPP_BIT    PIO_BIT_NOT_USED
#define SC_0_VPP_DEV    PIO_DEVICE_NOT_USED
#define SC_0_VCC_BIT    PIO_BIT_5
#define SC_0_VCC_DEV    PIO_DEVICE_0
#define SC_0_RST_BIT    PIO_BIT_4
#define SC_0_RST_DEV    PIO_DEVICE_0
#define SC_0_DETECT_BIT PIO_BIT_7
#define SC_0_DETECT_DEV PIO_DEVICE_0
#define SC_0_ASC_DEV    ASC_DEVICE_0
#define SC_0_CLK_SRC    STSMART_CPU_CLOCK
#define SC_0_CLK_SPD    CLOCK_COMMS

#define SC_1_EXTCLK_BIT PIO_BIT_NOT_USED
#define SC_1_EXTCLK_DEV PIO_DEVICE_NOT_USED
#define SC_1_CLK_BIT    PIO_BIT_3
#define SC_1_CLK_DEV    PIO_DEVICE_2
#define SC_1_VPP_BIT    PIO_BIT_NOT_USED
#define SC_1_VPP_DEV    PIO_DEVICE_NOT_USED
#define SC_1_VCC_BIT    PIO_BIT_5
#define SC_1_VCC_DEV    PIO_DEVICE_2
#define SC_1_RST_BIT    PIO_BIT_4
#define SC_1_RST_DEV    PIO_DEVICE_2
#define SC_1_DETECT_BIT PIO_BIT_7
#define SC_1_DETECT_DEV PIO_DEVICE_2
#define SC_1_ASC_DEV    ASC_DEVICE_3
#define SC_1_CLK_SRC    STSMART_CPU_CLOCK
#define SC_1_CLK_SPD    CLOCK_COMMS

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || \
      defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || \
      defined(ST_7100) || defined(ST_5301) || defined(ST_7109) || \
      defined(ST_5525) || defined(ST_7200)

#define SC_0_EXTCLK_BIT PIO_BIT_2
#define SC_0_EXTCLK_DEV PIO_DEVICE_0
#define SC_0_CLK_BIT    PIO_BIT_3
#define SC_0_CLK_DEV    PIO_DEVICE_0
#define SC_0_VPP_BIT    PIO_BIT_NOT_USED
#define SC_0_VPP_DEV    PIO_DEVICE_NOT_USED
#define SC_0_VCC_BIT    PIO_BIT_5
#define SC_0_VCC_DEV    PIO_DEVICE_0
#define SC_0_RST_BIT    PIO_BIT_4
#define SC_0_RST_DEV    PIO_DEVICE_0
#define SC_0_DETECT_BIT PIO_BIT_7
#define SC_0_DETECT_DEV PIO_DEVICE_0
#define SC_0_ASC_DEV    ASC_DEVICE_0
#define SC_0_CLK_SRC    STSMART_CPU_CLOCK
#define SC_0_CLK_SPD    CLOCK_COMMS

#define SC_1_EXTCLK_BIT PIO_BIT_2
#define SC_1_EXTCLK_DEV PIO_DEVICE_1
#define SC_1_CLK_BIT    PIO_BIT_3
#define SC_1_CLK_DEV    PIO_DEVICE_1
#define SC_1_VPP_BIT    PIO_BIT_NOT_USED
#define SC_1_VPP_DEV    PIO_DEVICE_NOT_USED
#define SC_1_VCC_BIT    PIO_BIT_5
#define SC_1_VCC_DEV    PIO_DEVICE_1
#define SC_1_RST_BIT    PIO_BIT_4
#define SC_1_RST_DEV    PIO_DEVICE_1
#define SC_1_DETECT_BIT PIO_BIT_7
#define SC_1_DETECT_DEV PIO_DEVICE_1
#define SC_1_ASC_DEV    ASC_DEVICE_1
#define SC_1_CLK_SRC    STSMART_CPU_CLOCK
#define SC_1_CLK_SPD    CLOCK_COMMS

#elif defined(ST_5105) || defined(ST_5107)
#define SC_0_EXTCLK_BIT PIO_BIT_2
#define SC_0_EXTCLK_DEV PIO_DEVICE_0
#define SC_0_CLK_BIT    PIO_BIT_3
#define SC_0_CLK_DEV    PIO_DEVICE_0
#define SC_0_VPP_BIT    PIO_BIT_NOT_USED
#define SC_0_VPP_DEV    PIO_DEVICE_NOT_USED
#define SC_0_VCC_BIT    PIO_BIT_5
#define SC_0_VCC_DEV    PIO_DEVICE_0
#define SC_0_RST_BIT    PIO_BIT_4
#define SC_0_RST_DEV    PIO_DEVICE_0
#define SC_0_DETECT_BIT PIO_BIT_7
#define SC_0_DETECT_DEV PIO_DEVICE_0
#define SC_0_ASC_DEV    ASC_DEVICE_0
#define SC_0_CLK_SRC    STSMART_CPU_CLOCK
#define SC_0_CLK_SPD    CLOCK_COMMS

#elif defined(ST_8010)

#define SC_0_EXTCLK_BIT PIO_BIT_2
#define SC_0_EXTCLK_DEV PIO_DEVICE_0
#define SC_0_CLK_BIT    PIO_BIT_3
#define SC_0_CLK_DEV    PIO_DEVICE_0
#define SC_0_VPP_BIT    PIO_BIT_NOT_USED
#define SC_0_VPP_DEV    PIO_DEVICE_NOT_USED
#define SC_0_VCC_BIT    PIO_BIT_1
#define SC_0_VCC_DEV    PIO_DEVICE_0
#define SC_0_RST_BIT    PIO_BIT_0
#define SC_0_RST_DEV    PIO_DEVICE_0
#define SC_0_DETECT_BIT PIO_BIT_7
#define SC_0_DETECT_DEV PIO_DEVICE_0
#define SC_0_ASC_DEV    ASC_DEVICE_0
#define SC_0_CLK_SRC    STSMART_CPU_CLOCK
#define SC_0_CLK_SPD    CLOCK_COMMS

#else /*  defined(ST_5510) || defined(ST_5512) || defined(ST_TP3) */

#define SC_0_EXTCLK_BIT PIO_BIT_2
#define SC_0_EXTCLK_DEV PIO_DEVICE_2
#define SC_0_CLK_BIT    PIO_BIT_3
#define SC_0_CLK_DEV    PIO_DEVICE_2
#define SC_0_VPP_BIT    PIO_BIT_NOT_USED
#define SC_0_VPP_DEV    PIO_DEVICE_NOT_USED
#define SC_0_VCC_BIT    PIO_BIT_5
#define SC_0_VCC_DEV    PIO_DEVICE_2
#define SC_0_RST_BIT    PIO_BIT_4
#define SC_0_RST_DEV    PIO_DEVICE_2
#define SC_0_DETECT_BIT PIO_BIT_7
#define SC_0_DETECT_DEV PIO_DEVICE_2
#define SC_0_ASC_DEV    ASC_DEVICE_2
#define SC_0_CLK_SRC    STSMART_CPU_CLOCK
#define SC_0_CLK_SPD    CLOCK_COMMS

#define SC_1_EXTCLK_BIT PIO_BIT_2
#define SC_1_EXTCLK_DEV PIO_DEVICE_0
#define SC_1_CLK_BIT    PIO_BIT_3
#define SC_1_CLK_DEV    PIO_DEVICE_0
#define SC_1_VPP_BIT    PIO_BIT_NOT_USED
#define SC_1_VPP_DEV    PIO_DEVICE_NOT_USED
#define SC_1_VCC_BIT    PIO_BIT_5
#define SC_1_VCC_DEV    PIO_DEVICE_0
#define SC_1_RST_BIT    PIO_BIT_4
#define SC_1_RST_DEV    PIO_DEVICE_0
#define SC_1_DETECT_BIT PIO_BIT_7
#define SC_1_DETECT_DEV PIO_DEVICE_0
#define SC_1_ASC_DEV    ASC_DEVICE_0
#define SC_1_CLK_SRC    STSMART_CPU_CLOCK
#define SC_1_CLK_SPD    CLOCK_COMMS

#endif

/* Cache settings */

#ifndef ICACHE
#define ICACHE  TRUE
#endif

#ifndef DCACHE
#define DCACHE  TRUE
#endif

#if (DCACHE == TRUE)

#if defined(CACHEABLE_BASE_ADDRESS)

#define DCACHE_START    CACHEABLE_BASE_ADDRESS
#define DCACHE_END      CACHEABLE_STOP_ADDRESS

#else

#define DCACHE_START    0x40080000
#define DCACHE_END      0x4FFFFFFF

#endif

#else

#define DCACHE_START    NULL
#define DCACHE_END      NULL

#endif

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Pass/fail counts */
typedef struct
{
    U32     NumberPassed;
    U32     NumberFailed;
} SMART_TestResult_t;

/* Error messages */
typedef struct
{
    ST_ErrorCode_t  Error;
    char            ErrorMsg[64];
} SMART_ErrorMessage;

/* Test parameters -- passed to every test routine */
typedef struct
{
    U32     Ref;
    U8      Slot;
} SMART_TestParams_t;

/* Defines a test harness function */
struct SMART_TestEntry_s
{
    SMART_TestResult_t  (*TestFunction)(SMART_TestParams_t *);
    char                TestInfo[50];
    U32                 RepeatCount;
    U8 Slot;
};

typedef struct SMART_TestEntry_s SMART_TestEntry_t;

typedef struct {
    U8  TestName[50];
    BOOL SuccessExpected;
    U8  Command[MAXBLOCKLENGTH];
    U8  CommandLength;
    U8  Expected[MAXCARDRESPONSE][MAXBLOCKLENGTH];
    U8  Responses[MAXCARDRESPONSE][MAXBLOCKLENGTH];
    BOOL GenerateLRC[MAXCARDRESPONSE];
    U8  ResponsesPresent;
} T1Test_t;

/* Exported Macros -------------------------------------------------------- */

/* Debug output */
#if !defined(DISABLE_TBX)
#define STSMART_Print(x)          STTBX_Print(x);
#else
#define STSMART_Print(x)          printf x;
#endif
#define SMART_DebugPrintf(args)      STSMART_Print(("%s: ", SMART_DEBUG_PREFIX)); STSMART_Print(args)
#define SMART_DebugError(msg,err)    STSMART_Print(("%s: %s = %s\n", SMART_DEBUG_PREFIX, msg, SMART_ErrorString(err)))
#define SMART_DebugMessage(msg)      STSMART_Print(("%s: %s\n", SMART_DEBUG_PREFIX, msg))

/* Test success indicator */
#define SMART_TestPassed(x) x.NumberPassed++; SMART_DebugPrintf(("Result: **** PASS ****\n"))
#define SMART_TestFailed(x,msg,reason) x.NumberFailed++; SMART_DebugPrintf(("Result: !!!! FAIL !!!! %s (%s)\n", msg, reason))

/* Exported Variables ----------------------------------------------------- */

/* Test harness revision number */
const U8 Revision[] = "3.7.2";

/* Exported Functions ----------------------------------------------------- */

static char *SMART_ErrorString(ST_ErrorCode_t Error);

ST_ErrorCode_t GetBlock(U8 *Buffer, U32 *Length);
ST_ErrorCode_t PutBlock(U8 *Buffer, U32 Length);

#endif /* __SMART_TEST_H */

/* End of header */
