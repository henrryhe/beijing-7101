/*****************************************************************************
File Name   : ascregs.h

Description : ASC register definitions.

Copyright (C) 2001 STMicroelectronics

History     :

    29/11/99    Migrated ASC2 register definitions to separate header file.

                ASC3:   Added new ASCRetries register.

                        New control bits added to ASCControl; CtsEnable and
                        BaudMode.

                        New status bit added to ASCStatus register; Nacked.

   28/11/01    Added additional functionalty for device type ISO7816 based
               on UARTF Functional Specification May 2001

Reference   :

UART with FIFO SC Interface Silicon Specification (18Feb99)
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __ASCREGS_H
#define __ASCREGS_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"                    /* STAPI includes */

#define USE_STSYS 1
/* Exported Types --------------------------------------------------------- */

/* ASC register layout.  Each register is 32-bits wide */
typedef struct {
    U32 ASCnBaudRate;
    U32 ASCnTxBuffer;
    U32 ASCnRxBuffer;
    U32 ASCnControl;
    U32 ASCnIntEnable;
    U32 ASCnStatus;
    U32 ASCnGuardTime;
    U32 ASCnTimeout;
    U32 ASCnTxReset;
    U32 ASCnRxReset;
    U32 ASCnRetries;
} UART_ASCRegs_t;

/* Exported Constants ----------------------------------------------------- */

/* ASCnControl register */

/* -- 2:0 Mode
000     RESERVED
001     8-bit data
010     RESERVED
011     7-bit data
100     9-bit data
101     8-bit data + wake up bit
110     RESERVED
111     8-bit data + parity
*/
#define CONTROL_MODE            0x7     /* Bitmask for mode bits */
#define CONTROL_8BITS           0x1     /* 8 data bits */
#define CONTROL_7BITSPARITY     0x3     /* 7 data bits + parity */
#define CONTROL_9BITS           0x4     /* 9 data bits */
#define CONTROL_8BITSWAKEUP     0x5     /* 8 data bits + wakeup bit */
#define CONTROL_8BITSPARITY     0x7     /* 8 data bits + parity */

/* -- 4:3 StopBits
00      0.5 stop bits
01      1 stop bit
10      1.5 stop bits
11      2 stop bits
*/
#define CONTROL_STOPBITS        0x18    /* Bitmask for stopbits bits */
#define CONTROL_1_0_STOPBITS    0x8     /* 1 stop bit */
#define CONTROL_1_5_STOPBITS    0x10    /* 1.5 stop bits */
#define CONTROL_2_0_STOPBITS    0x18    /* 2 stop bits */

/* -- 5 ParityOdd
0       even parity
1       odd parity
*/
#define CONTROL_PARITY          0x20

/* -- 6 LoopBack
0       standard tx/rx
1       loop back enabled
*/

#define CONTROL_LOOPBACK        0x40

/* -- 7 Run
0       baudrate generator disabled
1       baudrate generator enabled
*/

#define CONTROL_RUN             0x80

/* -- 8 RxEnable
0       receiver disabled
1       receiver enabled
*/

#define CONTROL_RXENABLE        0x100

/* -- 9 SCEnable
0       smartcard mode disabled
1       smartcard mode enabled
*/

#define CONTROL_SMARTCARD       0x200

/* -- 10 FifoEnable
0       FIFO disabled
1       FIFO enabled
*/

#define CONTROL_FIFO            0x400

/* -- 11 CtsEnable
0       CTS ignored
1       CTS enabled
*/

#define CONTROL_CTSENABLE       0x800

/* -- 12 BaudMode
0       Baud counter decrements, tick when it reaches 0.
1       Baud counter added to itself, ticks on carry.
*/

#define CONTROL_BAUDMODE        0x1000

/* -- 13 NACKDisable
0       Nacking behaviour enabled (NACK signal generated)
1       Nacking behviour is disabled (No NACK signal)
*/

#define CONTROL_NACKDISABLE     0x2000



/* ASCnStatus register */
#define STATUS_RXBUFFULL        0x1
#define STATUS_TXEMPTY          0x2
#define STATUS_TXHALFEMPTY      0x4
#define STATUS_PARITYERROR      0x8
#define STATUS_FRAMEERROR       0x10
#define STATUS_OVERRUNERROR     0x20
#define STATUS_TIMEOUTNOTEMPTY  0x40
#define STATUS_TIMEOUTIDLE      0x80
#define STATUS_RXHALFFULL       0x100
#define STATUS_TXFULL           0x200
#define STATUS_NACKED           0x400

/* ASCnIntEnable register */
#define IE_RXBUFFULL            0x1
#define IE_TXEMPTY              0x2
#define IE_TXHALFEMPTY          0x4
#define IE_PARITYERROR          0x8
#define IE_FRAMEERROR           0x10
#define IE_OVERRUNERROR         0x20
#define IE_TIMEOUTNOTEMPTY      0x40
#define IE_TIMEOUTIDLE          0x80
#define IE_RXHALFFULL           0x100

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Macros for setting the control mode */

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_ClearMode(base)        STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_MODE)))
#else
#define UART_ClearMode(base)        iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_MODE)) , base->ASCnControl); wmb()
#endif
#else
#define UART_ClearMode(base)        base->ASCnControl &= ~(U32)CONTROL_MODE
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_8Bits(base)            STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_MODE))); \
                                    STSYS_WriteRegDev32LE((&(base->ASCnControl)), (STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_8BITS)))
#else
#define UART_8Bits(base)            iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_MODE)) , base->ASCnControl); \
                                    iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_8BITS)), base->ASCnControl); wmb()
#endif
#else
#define UART_8Bits(base)            base->ASCnControl &= ~(U32)CONTROL_MODE; \
                                    base->ASCnControl |= CONTROL_8BITS
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_7BitsParity(base)      STSYS_WriteRegDev32LE((&(base->ASCnControl)), (STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_MODE))); \
                                    STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_7BITSPARITY)))
#else
#define UART_7BitsParity(base)      iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_MODE)) , base->ASCnControl); \
                                    iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_7BITSPARITY)), base->ASCnControl); wmb()
#endif
#else
#define UART_7BitsParity(base)      base->ASCnControl &= ~(U32)CONTROL_MODE; \
                                    base->ASCnControl |= CONTROL_7BITSPARITY
#endif



#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_9Bits(base)            STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_MODE))); \
                                    STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_9BITS)))
#else
#define UART_9Bits(base)            iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_MODE)) , base->ASCnControl); \
                                    iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_9BITS)), base->ASCnControl); wmb()
#endif
#else
#define UART_9Bits(base)            base->ASCnControl &= ~(U32)CONTROL_MODE; \
                                    base->ASCnControl |= CONTROL_9BITS
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_8BitsWakeup(base)      STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_MODE))); \
                                    STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_8BITSWAKEUP)))
#else
#define UART_8BitsWakeup(base)      iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_MODE)) , base->ASCnControl); \
                                    iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_8BITSWAKEUP)), base->ASCnControl); wmb()
#endif
#else
#define UART_8BitsWakeup(base)      base->ASCnControl &= ~(U32)CONTROL_MODE; \
                                    base->ASCnControl |= CONTROL_8BITSWAKEUP
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_8BitsParity(base)      STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_MODE))); \
                                    STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_8BITSPARITY)))
#else
#define UART_8BitsParity(base)      iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_MODE)) , base->ASCnControl); \
                                    iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_8BITSPARITY)), base->ASCnControl); wmb()
#endif
#else
#define UART_8BitsParity(base)      base->ASCnControl &= ~(U32)CONTROL_MODE; \
                                    base->ASCnControl |= CONTROL_8BITSPARITY
#endif

/* Macros for setting stop bits */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_0_5_StopBits(base)     STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_STOPBITS)))
#else
#define UART_0_5_StopBits(base)     iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_STOPBITS)) , base->ASCnControl); wmb()
#endif
#else
#define UART_0_5_StopBits(base)     base->ASCnControl &= ~(U32)CONTROL_STOPBITS
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_1_0_StopBits(base)     STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_STOPBITS))); \
                                    STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_1_0_STOPBITS)))
#else
#define UART_1_0_StopBits(base)     iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_STOPBITS)) , base->ASCnControl); \
                                    iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_1_0_STOPBITS)), base->ASCnControl); wmb()

#endif
#else
#define UART_1_0_StopBits(base)     base->ASCnControl &= \
                                                     ~(U32)CONTROL_STOPBITS; \
                                    base->ASCnControl |= CONTROL_1_0_STOPBITS
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_1_5_StopBits(base)     STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_STOPBITS))); \
                                    STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_1_5_STOPBITS)))
#else
#define UART_1_5_StopBits(base)     iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_STOPBITS)) , base->ASCnControl); \
                                    iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_1_5_STOPBITS)), base->ASCnControl); wmb()
#endif
#else
#define UART_1_5_StopBits(base)     base->ASCnControl &= \
                                                     ~(U32)CONTROL_STOPBITS; \
                                    base->ASCnControl |= CONTROL_1_5_STOPBITS
#endif
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_2_0_StopBits(base)     STSYS_WriteRegDev32LE((void*)(&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_STOPBITS))); \
                                    STSYS_WriteRegDev32LE((void*)(&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_2_0_STOPBITS)))
#else
#define UART_2_0_StopBits(base)     iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_STOPBITS)) , base->ASCnControl); \
                                    iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_2_0_STOPBITS)), base->ASCnControl); wmb()
#endif
#else
#define UART_2_0_StopBits(base)     base->ASCnControl &= \
                                                     ~(U32)CONTROL_STOPBITS; \
                                    base->ASCnControl |= CONTROL_2_0_STOPBITS
#endif

/* Macros for setting parity */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_ParityOdd(base)        STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_PARITY)))
#else
#define UART_ParityOdd(base)        iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_PARITY)) , base->ASCnControl); wmb()
#endif
#else
#define UART_ParityOdd(base)        base->ASCnControl |= CONTROL_PARITY
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_ParityEven(base)       STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_PARITY)))
#else
#define UART_ParityEven(base)       iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_PARITY)) , base->ASCnControl); wmb()
#endif
#else
#define UART_ParityEven(base)       base->ASCnControl &= ~(U32)CONTROL_PARITY
#endif

/* Macros for setting loopback mode */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_LoopBackEnable(base)   STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_LOOPBACK)))
#else
#define UART_LoopBackEnable(base)   iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_LOOPBACK)) , base->ASCnControl); wmb()
#endif
#else
#define UART_LoopBackEnable(base)   base->ASCnControl |= CONTROL_LOOPBACK
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_LoopBackDisable(base)  STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_LOOPBACK)))
#else
#define UART_LoopBackDisable(base)  iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_LOOPBACK)) , base->ASCnControl ); wmb()
#endif
#else
#define UART_LoopBackDisable(base)  base->ASCnControl &= ~(U32)CONTROL_LOOPBACK
#endif

/* Macros for setting run mode */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_RunEnable(base)        STSYS_WriteRegDev32LE((&(base->ASCnControl)), (STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_RUN)))
#else
#define UART_RunEnable(base)        iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_RUN)) , base->ASCnControl); wmb()
#endif
#else
#define UART_RunEnable(base)        base->ASCnControl |= CONTROL_RUN
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_RunDisable(base)       STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_RUN)))
#else
#define UART_RunDisable(base)       iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_RUN)) , base->ASCnControl); wmb()
#endif
#else
#define UART_RunDisable(base)       base->ASCnControl &= ~(U32)CONTROL_RUN
#endif

/* Macros for setting receiver enable */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_ReceiverEnable(base)   STSYS_WriteRegDev32LE((&(base->ASCnControl)), (STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_RXENABLE)))
#else
#define UART_ReceiverEnable(base)   iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_RXENABLE)) , base->ASCnControl); wmb()
#endif
#else
#define UART_ReceiverEnable(base)   base->ASCnControl |= CONTROL_RXENABLE
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_ReceiverDisable(base)  STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_RXENABLE)))
#else
#define UART_ReceiverDisable(base)  iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_RXENABLE)) , base->ASCnControl); wmb()
#endif
#else
#define UART_ReceiverDisable(base)  base->ASCnControl &= ~(U32)CONTROL_RXENABLE
#endif

/* Macros for setting smartcard mode */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_SmartCardEnable(base)  STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_SMARTCARD)))
#else
#define UART_SmartCardEnable(base)  iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_SMARTCARD)) , base->ASCnControl)
#endif
#else
#define UART_SmartCardEnable(base)  base->ASCnControl |= CONTROL_SMARTCARD
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_SmartCardDisable(base) STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_SMARTCARD)))
#else
#define UART_SmartCardDisable(base) iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_SMARTCARD)) , base->ASCnControl)
#endif
#else
#define UART_SmartCardDisable(base) base->ASCnControl &= ~(U32)CONTROL_SMARTCARD
#endif

/* Macros for setting FIFO enable */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_FifoEnable(base)       STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_FIFO)))
#else
#define UART_FifoEnable(base)       iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_FIFO)) , base->ASCnControl);wmb()
#endif
#else
#define UART_FifoEnable(base)       base->ASCnControl |= CONTROL_FIFO
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_FifoDisable(base)      STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_FIFO)))
#else
#define UART_FifoDisable(base)      iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_FIFO)) , base->ASCnControl); wmb()
#endif
#else
#define UART_FifoDisable(base)      base->ASCnControl &= ~(U32)CONTROL_FIFO
#endif

/* Macros for setting CTS enable */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_CTSEnable(base)        STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_CTSENABLE)))
#else
#define UART_CTSEnable(base)        iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_CTSENABLE)) , base->ASCnControl); wmb()
#endif
#else
#define UART_CTSEnable(base)        base->ASCnControl |= CONTROL_CTSENABLE
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_CTSDisable(base)       STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_CTSENABLE)))
#else
#define UART_CTSDisable(base)       iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_CTSENABLE)) , base->ASCnControl); wmb()
#endif
#else
#define UART_CTSDisable(base)       base->ASCnControl &= ~(U32)CONTROL_CTSENABLE
#endif

/* Macros for setting BaudMode */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_BaudTickOnZero(base)   STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_BAUDMODE)))
#else
#define UART_BaudTickOnZero(base)   iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_BAUDMODE)) , base->ASCnControl); wmb()
#endif
#else
#define UART_BaudTickOnZero(base)   base->ASCnControl &= ~(U32)CONTROL_BAUDMODE
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_BaudTickOnCarry(base)  STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|((U32)CONTROL_BAUDMODE)))
#else
#define UART_BaudTickOnCarry(base)  iowrite32((ioread32(base->ASCnControl)|((U32)CONTROL_BAUDMODE)) , base->ASCnControl); wmb()
#endif
#else
#define UART_BaudTickOnCarry(base)  base->ASCnControl |= CONTROL_BAUDMODE
#endif

/* FIFO reset */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_ResetTxFIFO(base)      STSYS_WriteRegDev32LE((&(base->ASCnTxReset)), 0xFFFFFFFF)
#else
#define UART_ResetTxFIFO(base)      iowrite32(0xFFFFFFFF , base->ASCnTxReset); wmb()
#endif
#else
#define UART_ResetTxFIFO(base)      base->ASCnTxReset = 0xFFFFFFFF
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_ResetRxFIFO(base)      STSYS_WriteRegDev32LE((&(base->ASCnRxReset)), 0xFFFFFFFF)
#else
#define UART_ResetRxFIFO(base)      iowrite32(0xFFFFFFFF , base->ASCnRxReset); wmb()
#endif
#else
#define UART_ResetRxFIFO(base)      base->ASCnRxReset = 0xFFFFFFFF
#endif

/* Macros for setting retries */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_SetRetries(base, v)    STSYS_WriteRegDev32LE((&(base->ASCnRetries)),(v))
#else
#define UART_SetRetries(base, v)    iowrite32((v) , base->ASCnRetries); wmb()
#endif
#else
#define UART_SetRetries(base, v)    base->ASCnRetries = (v)
#endif

/* Macros for enabling/disabling interrupts */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_EnableInts(base, v)    STSYS_WriteRegDev32LE((&(base->ASCnIntEnable)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnIntEnable)))|(v)))
#else
#define UART_EnableInts(base, v)    iowrite32((ioread32(base->ASCnIntEnable)|(v)) , base->ASCnIntEnable); wmb()
#endif
#else
#define UART_EnableInts(base, v)    base->ASCnIntEnable |= (v)
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_DisableInts(base, v)   STSYS_WriteRegDev32LE((&(base->ASCnIntEnable)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnIntEnable)))&(~(U32)(v)))) 
#else
#define UART_DisableInts(base, v)   iowrite32((ioread32(base->ASCnIntEnable)&(~(U32)(v))) , base->ASCnIntEnable); wmb() 
#endif
#else
#define UART_DisableInts(base, v)   base->ASCnIntEnable &= ~(U32)(v)
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_GlobalDisableInts(base)   STSYS_WriteRegDev32LE((&(base->ASCnIntEnable)),0)
#else
#define UART_GlobalDisableInts(base)   iowrite32(0 , base->ASCnIntEnable); wmb()
#endif
#else
#define UART_GlobalDisableInts(base)   base->ASCnIntEnable = 0
#endif

/* Macro for checking TxEmpty status */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_IsTxEmpty(base)        (((STSYS_ReadRegDev32LE((void*)(&(base->ASCnStatus)))&STATUS_TXEMPTY) != 0))
#else
#define UART_IsTxEmpty(base)        (((ioread32(base->ASCnStatus)&STATUS_TXEMPTY) != 0))
#endif
#else
#define UART_IsTxEmpty(base)        ((base->ASCnStatus & STATUS_TXEMPTY) != 0)
#endif

/* Macro for setting guardtime */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_SetGuardTime(base, v)  STSYS_WriteRegDev32LE((&(base->ASCnGuardTime)), (v))
#else
#define UART_SetGuardTime(base, v)  iowrite32((v) , base->ASCnGuardTime); wmb()
#endif
#else
#define UART_SetGuardTime(base, v)  base->ASCnGuardTime = (v)
#endif

/* Macro for setting timeout */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_SetTimeout(base, v)    STSYS_WriteRegDev32LE((&(base->ASCnTimeout)), (v))
#else
#define UART_SetTimeout(base, v)    iowrite32((v) , base->ASCnTimeout); wmb()
#endif
#else
#define UART_SetTimeout(base, v)    base->ASCnTimeout = (v)
#endif

/* Macro for setting NACKDisable */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_NACKDisable(base)    STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))|(~(U32)CONTROL_NACKDISABLE)))
#else
#define UART_NACKDisable(base)    iowrite32((ioread32(base->ASCnControl)|(~(U32)CONTROL_NACKDISABLE)) , base->ASCnControl); wmb()
#endif
#else
#define UART_NACKDisable(base)    base->ASCnControl |= CONTROL_NACKDISABLE
#endif

#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define UART_NACKEnable(base)     STSYS_WriteRegDev32LE((&(base->ASCnControl)),(STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))&(~(U32)CONTROL_NACKDISABLE)))
#else
#define UART_NACKEnable(base)     iowrite32((ioread32(base->ASCnControl)&(~(U32)CONTROL_NACKDISABLE)) , base->ASCnControl); wmb()
#endif
#else
#define UART_NACKEnable(base)     base->ASCnControl &= ~(U32)CONTROL_NACKDISABLE
#endif

#ifdef ST_OSLINUX
#define UART_Status(base)         STSYS_ReadRegDev32LE((void*)(&(base->ASCnStatus)))
#define UART_ReadBaudrate(base)   STSYS_ReadRegDev32LE((void*)(&(base->ASCnBaudRate)))
#define UART_WriteBaudRate(base,v)   STSYS_WriteRegDev32LE((void*)(&(base->ASCnBaudRate)),v)
#define UART_ReadTxBuf(base)      STSYS_ReadRegDev32LE((void*)(&(base->ASCnTxBuffer)))
#define UART_WriteTxBuf(base, v)  STSYS_WriteRegDev32LE((void*)(&(base->ASCnTxBuffer)), v)
#define UART_ReadRxBuf(base)      STSYS_ReadRegDev32LE((void*)(&(base->ASCnRxBuffer)))
#define UART_ReadControl(base)    STSYS_ReadRegDev32LE((void*)(&(base->ASCnControl)))
#define UART_ReadIntEnable(base)  STSYS_ReadRegDev32LE((void*)(&(base->ASCnIntEnable)))
#define UART_ReadGuardTime(base)  STSYS_ReadRegDev32LE((void*)(&(base->ASCnGuardTime)))
#endif

#endif /*__ASCREGS_H */

/* End of ascregs.h */
