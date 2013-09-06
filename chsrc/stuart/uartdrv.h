/*****************************************************************************
File Name   : uartdrv.h

Description : UART devices access routines.

Copyright (C) 1999 STMicroelectronics

History     :

    01/12/99  Module created.

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __UARTDRV_H
#define __UARTDRV_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"                    /* STAPI includes */
#include "stuart.h"

#include "uartdat.h"                    /* Local includes */

/* Exported Constants ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

/* Interrupt service routine */
#ifdef ST_OS21
int UART_InterruptHandler(UART_ControlBlock_t *Uart_p);
#else
void UART_InterruptHandler(UART_ControlBlock_t *Uart_p);
#endif

/* Register manipulation */
BOOL UART_BaudRate(UART_ControlBlock_t *Uart_p,
                   U32 BaudRate,
                   BOOL Set);

/* Read helper routines */
#ifndef ST_OSWINCE
__inline U32 UART_GetDataFromSwFIFO(UART_ControlBlock_t *Uart_p,
                                    U8 *Buffer,
                                    U32 MinToRead,
                                    U32 MaxToRead,
                                    BOOL *Terminated,
                                    UART_Operation_t *ReadOperation);
#else
U32 UART_GetDataFromSwFIFO(UART_ControlBlock_t *Uart_p,
                                    U8 *Buffer,
                                    U32 MinToRead,
                                    U32 MaxToRead,
                                    BOOL *Terminated,
                                    UART_Operation_t *ReadOperation);
#endif


/* Write helper routines */
__inline U32 UART_WriteBlock(UART_ControlBlock_t *Uart_p,
                             U8 *Buffer,
                             U32 NumberToWrite);
__inline U32 UART_Write9BitBlock(UART_ControlBlock_t *Uart_p,
                                 U8 *Buffer,
                                 U32 NumberToWrite);

/* Parameter setting/checking */
void UART_SetProtocol(UART_ControlBlock_t *Uart_p,
                      STUART_Protocol_t *Protocol_p);
void UART_SetParams(UART_ControlBlock_t *Uart_p);
U8 UART_PIOBitFromBitMask(U8 BitMask);

/* Debug */
#ifdef STUART_DEBUG
void UART_DumpRegs(UART_ControlBlock_t *Uart_p);
#endif

#endif /* __UARTDRV_H */

/* End of uartdrv.h */
