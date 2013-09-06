/*****************************************************************************
File Name   : scio.h

Description : Smartcard I/O routines.

Copyright (C) 2006 STMicroelectronics

History     :

    30/05/00    New IO routines created to encapsulate all communications
                with the STUART driver.  This simplifies the
                main driver code e.g., all echo'd bytes are read back,
                and the bit convention is respected at all times.

    31/08/00    Added SetNAK() to allow t=1 compliance where NAKs are
                not permitted by the smartcard.

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __SCIO_H
#define __SCIO_H

/* Includes --------------------------------------------------------------- */

#include "scio_p.h"                     /* Private IO data */

#ifdef ST_OSLINUX
#include "compat.h"
#endif
/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported for user */
typedef SMART_IO_Params_t *SMART_IO_Handle_t;

typedef enum SMART_IO_BitConvention_e
{
    DIRECT,
    INVERSE
} SMART_IO_BitConvention_t;

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t SMART_IO_Init(const ST_DeviceName_t DeviceName,
                             ST_Partition_t *DriverPartition,
                             STSMART_Device_t DeviceType,
                             SMART_IO_Handle_t *Handle_p);
ST_ErrorCode_t SMART_IO_Term(SMART_IO_Handle_t Handle);
ST_ErrorCode_t SMART_IO_Rx(SMART_IO_Handle_t Handle,
                           U8 *Buf,
                           U32 NumberToRead,
                           U32 *NumberRead,
                           U32 Timeout,
                           void (*RxCallback)(ST_ErrorCode_t, void *),
                           void *Arg
                         );

ST_ErrorCode_t SMART_IO_Tx(SMART_IO_Handle_t Handle,
                           U8 *Buf,
                           U32 NumberToWrite,
                           U32 *NumberWritten,
                           U32 Timeout,
                           void (*TxCallback)(ST_ErrorCode_t, void *),
                           void *Arg
                         );

ST_ErrorCode_t SMART_IO_Abort(SMART_IO_Handle_t Handle);
ST_ErrorCode_t SMART_IO_Flush(SMART_IO_Handle_t Handle);
ST_ErrorCode_t SMART_IO_NackDisable(SMART_IO_Handle_t Handle);
ST_ErrorCode_t SMART_IO_NackEnable(SMART_IO_Handle_t Handle);
ST_ErrorCode_t SMART_IO_SetBitCoding(SMART_IO_Handle_t Handle,
                                     SMART_IO_BitConvention_t BitCon);
ST_ErrorCode_t SMART_IO_SetBitRate(SMART_IO_Handle_t Handle,
                                   U32 BitRate);
ST_ErrorCode_t SMART_IO_SetRetries(SMART_IO_Handle_t Handle,
                                   U8 Retries);
ST_ErrorCode_t SMART_IO_SetGuardTime(SMART_IO_Handle_t Handle,
                                     U16 N);

ST_ErrorCode_t SMART_IO_SetNAK(SMART_IO_Handle_t Handle,
                               BOOL Enable);

#endif /* __SCIO_H */

/* End of scio.h */
