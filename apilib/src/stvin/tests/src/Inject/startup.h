/*****************************************************************************

File name   : STARTUP.H

Description : common module for inclusion of STARTUP

COPYRIGHT (C) ST-Microelectronics 1998.

Date               Modification                 Name
----               ------------                 ----
7/30/98            Created                      Antonin CONTINSOUZAS

*****************************************************************************/

#ifndef __STARTUP_H
#define __STARTUP_H

/*#define VIN_1_0_0A4*/

/* Includes --------------------------------------------------------------- */
#include "testcfg.h" /* test specific dependency needs */

#include "stddefs.h" /* standard definitions                        */
#include "stuart.h"  /* do not remove */

/* ***********************************************************************
 *                        library inter dependencies
 * ***********************************************************************/

/* TRACE_UART needs USE_UART */
#if defined (TRACE_UART) && !defined (USE_UART)
#define USE_UART
#endif

/* Toolbox, STUART, STPWM, STCFG, STI2C needs PIO */
#if (defined (USE_TBX) || defined (USE_PWM) || defined (USE_UART) || defined (USE_STCFG) || defined (USE_I2C)) && !defined (USE_PIO)
#define USE_PIO
#endif

/* STGPDMA, STINTMR, STCLKRV need STEVT */
#if (defined (USE_GPDMA) || defined (USE_INTMR) || defined (USE_CLKRV)) && !defined (USE_EVT)
#define USE_EVT
#endif

/* STVTG, STVBI, STVOUT need STDENC */
#if (defined (USE_VTG) || defined (USE_VOUT) || defined (USE_VBI)) && !defined (USE_DENC)
#define USE_DENC
#endif

/*#########################################################################
 *                                  CONSTANTS
 *#########################################################################*/

/* Defines number of devices */
#define NUMBER_ASC              4

#if defined(ST_5514)
#define NUMBER_PIO              6
#else
#define NUMBER_PIO              5
#endif

#ifdef TRACE_UART
#define UART_BAUD_RATE        38400
#define TBX_SUPPORTED_DEVICE  STTBX_DEVICE_DCU | STTBX_DEVICE_UART
#else
#define UART_BAUD_RATE        9600
#define TBX_SUPPORTED_DEVICE  STTBX_DEVICE_DCU
#endif /* #ifdef TRACE_UART */

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

/* Define all devices */
#define STAVMEM_DEVICE_NAME           "AVMEM"
#define STBOOT_DEVICE_NAME            "BOOT"
#define STCLKRV_DEVICE_NAME           "CLKRV"
#define STDENC_DEVICE_NAME            "DENC"
#define STEVT_DEVICE_NAME             "EVT"
#define STGPDMA_DEVICE_NAME           "GPDMA"
#define STI2C_FRONT_DEVICE_NAME       "I2CFRONT"
#define STI2C_BACK_DEVICE_NAME        "I2CBACK"
#define STINTMR_DEVICE_NAME           "INTMR"
#define STLAYER_GFX_DEVICE_NAME       "LAYGFX"
#define STLAYER_VID_DEVICE_NAME       "LAYVID"
#define STLLI_DEVICE_NAME             "LLI"
#define STPIO_0_DEVICE_NAME           "PIO0"
#define STPIO_1_DEVICE_NAME           "PIO1"
#define STPIO_2_DEVICE_NAME           "PIO2"
#define STPIO_3_DEVICE_NAME           "PIO3"
#define STPIO_4_DEVICE_NAME           "PIO4"
#define STPIO_5_DEVICE_NAME           "PIO5"
#define STPTI_DEVICE_NAME             "PTI"
#define STPWM_DEVICE_NAME             "PWM"
#define STTBX_DEVICE_NAME             "TBX"
#define STTTX_DEVICE_NAME             "TTX"
#define STTST_DEVICE_NAME             "TST"
#define STTUNER_DEVICE_NAME           "TUNER"
#define STVBI_DEVICE_NAME             "VBI"
#define STVID_DEVICE_NAME             "VID"
#define STVMIX_DEVICE_NAME            "VMIX1"
#define STVOUT_DEVICE_NAME            "VOUT"
#define STVTG_DEVICE_NAME             "VTG"
#define STVIN_DEVICE_NAME             "VIN"

/*#########################################################################
 *                         Board specific values
 *#########################################################################*/

#if defined(mb231)
#define BOARD_NAME                         "mb231"
#define SYSTEM_MEMORY_SIZE                 0x0040000
#define NCACHE_MEMORY_SIZE                 0x0080000
#define DCACHE_START                       0x40080000
#define DCACHE_END                         0x5FFFFFFF
#define BOOT_MEMORY_SIZE                   STBOOT_DRAM_MEMORY_SIZE_32_MBIT
#define CLKRV_VCXO_CHANNEL                 STPWM_CHANNEL_0
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS
#define VIRTUAL_SIZE                       SDRAM_SIZE
#define VIRTUAL_WINDOW_SIZE                SDRAM_SIZE

#elif defined(mb282b)
#define BOARD_NAME                         "mb282b"
#define SYSTEM_MEMORY_SIZE                 0x0D00000
#define NCACHE_MEMORY_SIZE                 0x0080000
#define DCACHE_START                       0x40080000
#define DCACHE_END                         0x5FFFFFFF
#define BOOT_MEMORY_SIZE                   STBOOT_DRAM_MEMORY_SIZE_64_MBIT
#define CLKRV_VCXO_CHANNEL                 STPWM_CHANNEL_0
#define STV6410_I2C_ADDRESS                0x94
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS
#define VIRTUAL_SIZE                       SDRAM_SIZE
#define VIRTUAL_WINDOW_SIZE                SDRAM_SIZE

#elif defined(mb275)
#define BOARD_NAME                         "mb275"
#define SYSTEM_MEMORY_SIZE                 0x0A00000
#define NCACHE_MEMORY_SIZE                 0x0080000
#define DCACHE_START                       0x40080000
#define DCACHE_END                         0x5FFFFFFF
#define BOOT_MEMORY_SIZE                   STBOOT_DRAM_MEMORY_SIZE_32_MBIT
#define CLKRV_VCXO_CHANNEL                 STPWM_CHANNEL_1
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS
#define VIRTUAL_SIZE                       SDRAM_SIZE
#define VIRTUAL_WINDOW_SIZE                SDRAM_SIZE

#elif defined(mb275_64)
#define BOARD_NAME                         "mb275_64"
#define SYSTEM_MEMORY_SIZE                 0x0A00000
#define NCACHE_MEMORY_SIZE                 0x0080000
#define DCACHE_START                       0x40080000
#define DCACHE_END                         0x5FFFFFFF
#define BOOT_MEMORY_SIZE                   STBOOT_DRAM_MEMORY_SIZE_64_MBIT
#define CLKRV_VCXO_CHANNEL                 STPWM_CHANNEL_1
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS
#define VIRTUAL_SIZE                       SDRAM_SIZE
#define VIRTUAL_WINDOW_SIZE                SDRAM_SIZE

#elif defined(mb295)
#define BOARD_NAME                         "mb295"
#define SYSTEM_MEMORY_SIZE                 0x0A00000
#define NCACHE_MEMORY_SIZE                 0x0080000
#define DCACHE_START                       0x40080000
#define DCACHE_END                         0x5FFFFFFF
#define BOOT_MEMORY_SIZE                   STBOOT_DRAM_MEMORY_SIZE_32_MBIT
#define CLKRV_VCXO_CHANNEL                 STPWM_CHANNEL_0
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_WINDOW_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS
#define VIRTUAL_SIZE                       SDRAM_SIZE
#define VIRTUAL_WINDOW_SIZE                SDRAM_SIZE

#elif defined(mb314)
#define BOARD_NAME                         "mb314"
#define SYSTEM_MEMORY_SIZE                 0x0A00000
#define NCACHE_MEMORY_SIZE                 0x0080000
#define DCACHE_START                       0x40200000
#define DCACHE_END                         0x5FFFFFFF
#define BOOT_MEMORY_SIZE                   STBOOT_DRAM_MEMORY_SIZE_64_MBIT
#define CLKRV_VCXO_CHANNEL                 STPWM_CHANNEL_0
#define STV6410_I2C_ADDRESS                0x96
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS
#define VIRTUAL_SIZE                       SDRAM_SIZE
#define VIRTUAL_WINDOW_SIZE                SDRAM_SIZE

#elif defined(mb317)
#define BOARD_NAME                         "mb317"
#define SYSTEM_MEMORY_SIZE                 0x0200000
#define NCACHE_MEMORY_SIZE                 0x0080000
#define DCACHE_START                       0x40080000
#define DCACHE_END                         0x5FFFFFFF
#define BOOT_MEMORY_SIZE                   SDRAM_SIZE;
#define LMI_BASE_ADDRESS                   0xA8000000
#define RAM_SIZE                           0x04000000  /* 64 MBytes */
#define AVMEM_BASE_ADDRESS                 (LMI_BASE_ADDRESS + (RAM_SIZE>>1))
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  AVMEM_BASE_ADDRESS
#define VIRTUAL_BASE_ADDRESS               AVMEM_BASE_ADDRESS
#define VIRTUAL_SIZE                       RAM_SIZE
#define VIRTUAL_WINDOW_SIZE                RAM_SIZE

#endif


/*#########################################################################
 *                         Chip specific values
 *#########################################################################*/

#if defined(ST_5510)
#define CHIP_NAME                     "STi5510"
#define BOOT_BACKEND_TYPE             STBOOT_DEVICE_5510;
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
#define ASC_DEVICE_DATA               ASC_DEVICE_3
#define ASC_DEVICE_MODEM              ASC_DEVICE_1
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_2
#define ASC_DEVICE_TYPE               STUART_16_BYTE_FIFO
#define STUART_DEVICE_NAME            "ASC3"
#define UART_BASEADDRESS              ASC_3_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_3_INTERRUPT
#define UART_IT_LEVEL                 ASC_3_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_3_RXD_DEV
#define UART_RXD_BITMASK              ASC_3_RXD_BIT;
#define UART_TXD_PORTNAME_IDX         ASC_3_TXD_DEV
#define UART_TXD_BITMASK              ASC_3_TXD_BIT;
#define UART_RTS_PORTNAME_IDX         ASC_3_RTS_DEV
#define UART_RTS_BITMASK              ASC_3_RTS_BIT;
#define UART_CTS_PORTNAME_IDX         ASC_3_CTS_DEV
#define UART_CTS_BITMASK              ASC_3_CTS_BIT;
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#define PIO_FOR_PWM0_PORTNAME         STPIO_1_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_3
#define PIO_FOR_PWM1_PORTNAME         ""
#define PIO_FOR_PWM1_BITMASK          0
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0

#elif defined(ST_5512)
#define CHIP_NAME                     "STi5512"
#define BOOT_BACKEND_TYPE             STBOOT_DEVICE_5512;
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
#define ASC_DEVICE_DATA               ASC_DEVICE_3
#define ASC_DEVICE_MODEM              ASC_DEVICE_1
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_2
#define ASC_DEVICE_TYPE               STUART_RTSCTS
#define STUART_DEVICE_NAME            "ASC3"
#define UART_BASEADDRESS              ASC_3_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_3_INTERRUPT
#define UART_IT_LEVEL                 ASC_3_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_3_RXD_DEV
#define UART_RXD_BITMASK              ASC_3_RXD_BIT;
#define UART_TXD_PORTNAME_IDX         ASC_3_TXD_DEV
#define UART_TXD_BITMASK              ASC_3_TXD_BIT;
#define UART_RTS_PORTNAME_IDX         ASC_3_RTS_DEV
#define UART_RTS_BITMASK              ASC_3_RTS_BIT;
#define UART_CTS_PORTNAME_IDX         ASC_3_CTS_DEV
#define UART_CTS_BITMASK              ASC_3_CTS_BIT;
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#define PIO_FOR_PWM0_PORTNAME         STPIO_1_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_3
#define PIO_FOR_PWM1_PORTNAME         ""
#define PIO_FOR_PWM1_BITMASK          0
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0


#elif defined(ST_5508)
#define CHIP_NAME                     "STi5508"
#define BOOT_BACKEND_TYPE             STBOOT_DEVICE_5508;
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
#define ASC_DEVICE_DATA               ASC_DEVICE_2
#define ASC_DEVICE_MODEM              ASC_DEVICE_1
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_3
#define ASC_DEVICE_TYPE               STUART_16_BYTE_FIFO
#define STUART_DEVICE_NAME            "ASC2"
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
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#define PIO_FOR_PWM0_PORTNAME         ""
#define PIO_FOR_PWM0_BITMASK          0
#define PIO_FOR_PWM1_PORTNAME         ""
#define PIO_FOR_PWM1_BITMASK          0 /* PIO_BIT_4 */
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0

#elif defined(ST_5518)
#define CHIP_NAME                     "STi5518"
#define BOOT_BACKEND_TYPE             STBOOT_DEVICE_5518;
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
#define ASC_DEVICE_DATA               ASC_DEVICE_2
#define ASC_DEVICE_MODEM              ASC_DEVICE_1
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_3
#define ASC_DEVICE_TYPE               STUART_RTSCTS
#define STUART_DEVICE_NAME            "ASC2"
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
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#define PIO_FOR_PWM0_PORTNAME         ""
#define PIO_FOR_PWM0_BITMASK          0
#define PIO_FOR_PWM1_PORTNAME         ""
#define PIO_FOR_PWM1_BITMASK          0 /* PIO_BIT_4 */
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0

#elif defined(ST_5514)
#define CHIP_NAME                     "STi5514"
#define BOOT_BACKEND_TYPE             STBOOT_DEVICE_5514;
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
#define ASC_DEVICE_DATA               ASC_DEVICE_3
#define ASC_DEVICE_MODEM              ASC_DEVICE_2
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_1
#define ASC_DEVICE_TYPE               STUART_RTSCTS
#define STUART_DEVICE_NAME            "ASC3"
#define UART_BASEADDRESS              ASC_3_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_3_INTERRUPT
#define UART_IT_LEVEL                 ASC_3_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_3_RXD_DEV
#define UART_RXD_BITMASK              ASC_3_RXD_BIT;
#define UART_TXD_PORTNAME_IDX         ASC_3_TXD_DEV
#define UART_TXD_BITMASK              ASC_3_TXD_BIT;
#define UART_RTS_PORTNAME_IDX         ASC_3_RTS_DEV
#define UART_RTS_BITMASK              ASC_3_RTS_BIT;
#define UART_CTS_PORTNAME_IDX         ASC_3_CTS_DEV
#define UART_CTS_BITMASK              ASC_3_CTS_BIT;
#define PIO_FOR_SDA_PORTNAME          STPIO_3_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_3_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_1
#define BACK_I2C_BASE                 SSC_0_BASE_ADDRESS
#define PIO_FOR_PWM0_PORTNAME         "PIO2"
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_7
#define PIO_FOR_PWM1_PORTNAME         "PIO4"
#define PIO_FOR_PWM1_BITMASK          PIO_BIT_7
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0

#elif defined(ST_7015)
#define CHIP_NAME                     "STi7015"
#define BOOT_BACKEND_TYPE             STBOOT_DEVICE_TP3;
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
#define ASC_DEVICE_DATA               ASC_DEVICE_3
#define ASC_DEVICE_MODEM              ASC_DEVICE_1
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_2
#define ASC_DEVICE_TYPE               STUART_16_BYTE_FIFO
#define STUART_DEVICE_NAME            "ASC3"
#define UART_BASEADDRESS              ASC_3_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_3_INTERRUPT
#define UART_IT_LEVEL                 ASC_3_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_3_RXD_DEV
#define UART_RXD_BITMASK              ASC_3_RXD_BIT;
#define UART_TXD_PORTNAME_IDX         ASC_3_TXD_DEV
#define UART_TXD_BITMASK              ASC_3_TXD_BIT;
#define UART_RTS_PORTNAME_IDX         ASC_3_RTS_DEV
#define UART_RTS_BITMASK              ASC_3_RTS_BIT;
#define UART_CTS_PORTNAME_IDX         ASC_3_CTS_DEV
#define UART_CTS_BITMASK              ASC_3_CTS_BIT;
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#define PIO_FOR_PWM0_PORTNAME         STPIO_1_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_3
#define PIO_FOR_PWM1_PORTNAME         STPIO_1_DEVICE_NAME
#define PIO_FOR_PWM1_BITMASK          PIO_BIT_4
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7015
#define INTMR_BASE_ADDRESS            (STI7015_BASE_ADDRESS + ST7015_CFG_OFFSET)
#define INTMR_INTERRUPT_NUMBER        ST20TP3_EXTERNAL1_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         BACKEND_INTERRUPT_LEVEL

#elif defined(ST_7020)
#define CHIP_NAME                     "STi7020"
#define BOOT_BACKEND_TYPE             STBOOT_DEVICE_TP3;
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
#define ASC_DEVICE_DATA               ASC_DEVICE_3
#define ASC_DEVICE_MODEM              ASC_DEVICE_1
#define ASC_DEVICE_SC0                ASC_DEVICE_0
#define ASC_DEVICE_SC1                ASC_DEVICE_2
#define ASC_DEVICE_TYPE               STUART_16_BYTE_FIFO
#define STUART_DEVICE_NAME            "ASC3"
#define UART_BASEADDRESS              ASC_3_BASE_ADDRESS
#define UART_IT_NUMBER                ASC_3_INTERRUPT
#define UART_IT_LEVEL                 ASC_3_INTERRUPT_LEVEL
#define UART_RXD_PORTNAME_IDX         ASC_3_RXD_DEV
#define UART_RXD_BITMASK              ASC_3_RXD_BIT;
#define UART_TXD_PORTNAME_IDX         ASC_3_TXD_DEV
#define UART_TXD_BITMASK              ASC_3_TXD_BIT;
#define UART_RTS_PORTNAME_IDX         ASC_3_RTS_DEV
#define UART_RTS_BITMASK              ASC_3_RTS_BIT;
#define UART_CTS_PORTNAME_IDX         ASC_3_CTS_DEV
#define UART_CTS_BITMASK              ASC_3_CTS_BIT;
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#define PIO_FOR_PWM0_PORTNAME         STPIO_1_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_3
#define PIO_FOR_PWM1_PORTNAME         STPIO_1_DEVICE_NAME
#define PIO_FOR_PWM1_BITMASK          PIO_BIT_4
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7020
#define INTMR_BASE_ADDRESS            (STI7020_BASE_ADDRESS + ST7020_CFG_OFFSET)
#define INTMR_INTERRUPT_NUMBER        ST20TP3_EXTERNAL1_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         BACKEND_INTERRUPT_LEVEL

#elif defined(ST_GX1)
#ifdef ST_GX1
#define CHIP_NAME                     "ST40GX1"
#endif
#define BOOT_BACKEND_TYPE             STBOOT_DEVICE_UNKNOWN
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
#define ASC_DEVICE_DATA               ASC_DEVICE_0
#define ASC_DEVICE_MODEM              ASC_DEVICE_1
#define ASC_DEVICE_SC0                ASC_DEVICE_NOT_USED
#define ASC_DEVICE_SC1                ASC_DEVICE_NOT_USED
#define ASC_DEVICE_TYPE               STUART_RTSCTS
#define STUART_DEVICE_NAME            "ASC0"
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
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#define SYSCONF_BASE                  0xFB190000
#define SYSCONF_SYS_CON1              (SYSCONF_BASE + 0x10)
#define SYSCONF_SYS_CON2              (SYSCONF_BASE + 0x18)
#define SYS_CONF1_LSW                 SYSCONF_SYS_CON1
#define SYS_CONF1_MSW                 (SYSCONF_SYS_CON1 + 0x04)
#define DEV_WRITE_U32(x, y)           *(volatile U32 *)x = (U32)y
#define SDRAM_BASE_ADDRESS            0 /* for STAVMEM : SDRAMBaseAddr_p not used in virtual */
#define BM_BASE_ADDRESS               0 /* for STAVMEM : no block move DMA copy method on GX1, so address not used */
#define VIDEO_BASE_ADDRESS            0 /* for STAVMEM : no MPEG1D/2D copy method on GX1, so address not used */
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_GX1_VIDEO_SYNC
#define INTMR_BASE_ADDRESS            (S32)(0xBE080000)
#define INTMR_INTERRUPT_NUMBER        44
#define INTMR_INTERRUPT_LEVEL         0   /* TBD with an accurate level */
#endif


/* Exported Macros and Defines -------------------------------------------- */
#define STRING_DEVICE_LENGTH   80
#define RETURN_PARAMS_LENGTH   50

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

#endif /* #ifndef __STARTUP_H */


