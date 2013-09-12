/*****************************************************************************
File Name   : scio_p.h

Description : Private data for smartcard I/O routines.

Copyright (C) 2006 STMicroelectronics

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __SCIOP_H
#define __SCIOP_H

/* Includes --------------------------------------------------------------- */

#include "stuart.h"

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

typedef struct SMART_IO_Params_s
{
    STUART_Handle_t UARTHandle;         /* Private handle to UART */
    STUART_Params_t UARTParams;         /* Parameter of UART */
    ST_Partition_t  *MemoryPartition;   /* Where driver memory is alloc'ed */
    U8              *Buf;               /* Current buffer pointer */
    U32             *ActualSize;        /* Transfer size pointer */
    BOOL            InverseConvention;  /* Bit convention */
    void (*Callback)(ST_ErrorCode_t,    /* User callback function */
                     void *);
    void            *CallbackArg;       /* User callback argument */
    BOOL            TransferIsRead;     /* Is this a read or write op? */
    BOOL            RxParamsFlush;
    BOOL            TxParamsFlush;
} SMART_IO_Params_t;

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

#endif /* __SCIOP_H */

/* End of scio_p.h */
