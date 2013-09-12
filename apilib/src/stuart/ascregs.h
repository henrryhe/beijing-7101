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
#define UART_ClearMode(base)        base->ASCnControl &= ~(U32)CONTROL_MODE
#define UART_8Bits(base)            base->ASCnControl &= ~(U32)CONTROL_MODE; \
                                    base->ASCnControl |= CONTROL_8BITS
#define UART_7BitsParity(base)      base->ASCnControl &= ~(U32)CONTROL_MODE; \
                                    base->ASCnControl |= CONTROL_7BITSPARITY
#define UART_9Bits(base)            base->ASCnControl &= ~(U32)CONTROL_MODE; \
                                    base->ASCnControl |= CONTROL_9BITS
#define UART_8BitsWakeup(base)      base->ASCnControl &= ~(U32)CONTROL_MODE; \
                                    base->ASCnControl |= CONTROL_8BITSWAKEUP
#define UART_8BitsParity(base)      base->ASCnControl &= ~(U32)CONTROL_MODE; \
                                    base->ASCnControl |= CONTROL_8BITSPARITY

/* Macros for setting stop bits */
#define UART_0_5_StopBits(base)     base->ASCnControl &= ~(U32)CONTROL_STOPBITS
#define UART_1_0_StopBits(base)     base->ASCnControl &= \
                                                     ~(U32)CONTROL_STOPBITS; \
                                    base->ASCnControl |= CONTROL_1_0_STOPBITS
#define UART_1_5_StopBits(base)     base->ASCnControl &= \
                                                     ~(U32)CONTROL_STOPBITS; \
                                    base->ASCnControl |= CONTROL_1_5_STOPBITS
#define UART_2_0_StopBits(base)     base->ASCnControl &= \
                                                     ~(U32)CONTROL_STOPBITS; \
                                    base->ASCnControl |= CONTROL_2_0_STOPBITS

/* Macros for setting parity */
#define UART_ParityOdd(base)        base->ASCnControl |= CONTROL_PARITY
#define UART_ParityEven(base)       base->ASCnControl &= ~(U32)CONTROL_PARITY

/* Macros for setting loopback mode */
#define UART_LoopBackEnable(base)   base->ASCnControl |= CONTROL_LOOPBACK
#define UART_LoopBackDisable(base)  base->ASCnControl &= ~(U32)CONTROL_LOOPBACK

/* Macros for setting run mode */
#define UART_RunEnable(base)        base->ASCnControl |= CONTROL_RUN
#define UART_RunDisable(base)       base->ASCnControl &= ~(U32)CONTROL_RUN

/* Macros for setting receiver enable */
#define UART_ReceiverEnable(base)   base->ASCnControl |= CONTROL_RXENABLE
#define UART_ReceiverDisable(base)  base->ASCnControl &= ~(U32)CONTROL_RXENABLE

/* Macros for setting smartcard mode */
#define UART_SmartCardEnable(base)  base->ASCnControl |= CONTROL_SMARTCARD
#define UART_SmartCardDisable(base) base->ASCnControl &= ~(U32)CONTROL_SMARTCARD

/* Macros for setting FIFO enable */
#define UART_FifoEnable(base)       base->ASCnControl |= CONTROL_FIFO
#define UART_FifoDisable(base)      base->ASCnControl &= ~(U32)CONTROL_FIFO

/* Macros for setting CTS enable */
#define UART_CTSEnable(base)        base->ASCnControl |= CONTROL_CTSENABLE
#define UART_CTSDisable(base)       base->ASCnControl &= ~(U32)CONTROL_CTSENABLE

/* Macros for setting BaudMode */
#define UART_BaudTickOnZero(base)   base->ASCnControl &= ~(U32)CONTROL_BAUDMODE
#define UART_BaudTickOnCarry(base)  base->ASCnControl |= CONTROL_BAUDMODE

/* FIFO reset */
#define UART_ResetTxFIFO(base)      base->ASCnTxReset = 0xFFFFFFFF
#define UART_ResetRxFIFO(base)      base->ASCnRxReset = 0xFFFFFFFF

/* Macros for setting retries */
#define UART_SetRetries(base, v)    base->ASCnRetries = (v)

/* Macros for enabling/disabling interrupts */
#define UART_EnableInts(base, v)    base->ASCnIntEnable |= (v)
#define UART_DisableInts(base, v)   base->ASCnIntEnable &= ~(U32)(v)
#define UART_GlobalDisableInts(base)   base->ASCnIntEnable = 0

/* Macro for checking TxEmpty status */
#define UART_IsTxEmpty(base)        ((base->ASCnStatus & STATUS_TXEMPTY) != 0)

/* Macro for checking RxFull status */
#define UART_IsRxFull(base)        ((base->ASCnStatus & STATUS_RXBUFFULL) != 0)

/* Macro for setting guardtime */
#define UART_SetGuardTime(base, v)  base->ASCnGuardTime = (v)

/* Macro for setting timeout */
#define UART_SetTimeout(base, v)    base->ASCnTimeout = (v)

/* Macro for setting NACKDisable */
#define UART_NACKDisable(base)    base->ASCnControl |= CONTROL_NACKDISABLE
#define UART_NACKEnable(base)     base->ASCnControl &= ~(U32)CONTROL_NACKDISABLE

#endif /*__ASCREGS_H */

/* End of ascregs.h */
