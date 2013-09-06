/*****************************************************************************
File Name   : uartdma.h

Description : UART devices access routines.

Copyright (C) 2006 STMicroelectronics

History     :

    24/6/06  Module created.

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __UARTDMA_H
#define __UARTDMA_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"                    /* STAPI includes */
#include "stfdma.h"

#include "uartdat.h"                    /* Local includes */

/* Exported Constants ----------------------------------------------------- */
#define UART_FDMA_ALIGN 127 /* 128 bytes aiignment */

/* Exported Types --------------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t UART_DMAInit(UART_ControlBlock_t *Uart_p);
void UART_DMATerm(UART_ControlBlock_t *Uart_p);
ST_ErrorCode_t UART_DMARead(UART_ControlBlock_t *Uart_p,
                                    U8 *Buffer,
                                    U32 MinToRead,
                                    U32 MaxToRead,
                                    U32 *NumberRead,
                                    U32 TimeOut);
ST_ErrorCode_t UART_DMAWrite(UART_ControlBlock_t *Uart_p,
                                    U8 *Buffer,
                                    U32 NumberToWrite,
                                    U32 *NumberWritten,
                                    U32 TimeOut);
ST_ErrorCode_t UART_DMAAbort(UART_ControlBlock_t *Uart_p, STUART_Direction_t Direction);
ST_ErrorCode_t UART_DMAFlushReceive(UART_ControlBlock_t *Uart_p);
ST_ErrorCode_t UART_StartDMARX(UART_ControlBlock_t *Uart_p, U8 *Buffer, U32 Length);
#endif /* __UARTDMA_H */

/* End of uartdma.h */
