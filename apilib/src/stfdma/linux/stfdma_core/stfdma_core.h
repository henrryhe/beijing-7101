/*****************************************************************************
 *
 *  Module      : fdma
 *  Date        : 29-02-2005
 *  Author      : WALKERM
 *  Description :
 *
 *****************************************************************************/


#ifndef FDMA_H
#define FDMA_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */

#include "stfdma.h"

/*** IOCtl defines ***/

typedef struct _StartGenericTransferParams
{
  STFDMA_TransferGenericParams_t  TransferParams;
  STFDMA_TransferId_t             TransferId;
} StartGenericTransferParams;

typedef struct _StartTransferParams
{
  STFDMA_TransferParams_t  TransferParams;
  STFDMA_TransferId_t      TransferId;
} StartTransferParams;

typedef struct _STFDMA_ContextGetSCListParams
{
  STFDMA_ContextId_t    ContextId;
  STFDMA_SCEntry_t*     SCList;
  U32                   Size;
  BOOL                  Overflow;
} STFDMA_ContextGetSCListParams;

typedef struct _STFDMA_ContextSetSCListParams
{
  STFDMA_ContextId_t    ContextId;
  STFDMA_SCEntry_t*     SCList;
  U32                   Size;
} STFDMA_ContextSetSCListParams;

typedef struct _STFDMA_ContextGetSCStateParams
{
  STFDMA_ContextId_t    ContextId;
  STFDMA_SCState_t      State;
  STFDMA_SCRange_t      Range;
} STFDMA_ContextGetSCStateParams;

typedef STFDMA_ContextGetSCStateParams STFDMA_ContextSetSCStateParams;

typedef struct _STFDMA_ContextSetESBufferParams
{
  STFDMA_ContextId_t    ContextId;
  void*                 Buffer;
  U32                   Size;
} STFDMA_ContextSetESBufferParams;

typedef struct _STFDMA_ContextSetESReadPtrParams
{
  STFDMA_ContextId_t    ContextId;
  void*                 Read;
} STFDMA_ContextSetESReadPtrParams;

typedef struct _STFDMA_ContextSetESWritePtrParams
{
  STFDMA_ContextId_t    ContextId;
  void*                 Write;
} STFDMA_ContextSetESWritePtrParams;

typedef STFDMA_ContextSetESReadPtrParams STFDMA_ContextGetESReadPtrParams;

typedef struct _STFDMA_ContextGetESWritePtrParams
{
  STFDMA_ContextId_t    ContextId;
  void*                 Write;
  BOOL                  Overflow;
} STFDMA_ContextGetESWritePtrParams;

typedef struct _STFDMA_GetTransferStatusParams
{
    STFDMA_TransferId_t     TransferId;
    STFDMA_TransferStatus_t TransferStatus;
} STFDMA_GetTransferStatusParams;

typedef struct _STFDMA_LockChannelInPoolParams
{
    STFDMA_Pool_t       Pool;
    STFDMA_ChannelId_t  ChannelId;
    U32 DeviceNo;
} STFDMA_LockChannelInPoolParams;

typedef struct _STFDMA_LockChannelParams
{
    STFDMA_ChannelId_t  ChannelId;
    U32 DeviceNo;
} STFDMA_LockChannelParams;

typedef struct _STFDMA_SetAddDataRegionParameterParams
{
    STFDMA_Device_t DeviceNo;
    STFDMA_AdditionalDataRegion_t RegionNo;
    STFDMA_AdditionalDataRegionParameter_t ADRParameter;
    U32 Value;
} STFDMA_SetAddDataRegionParameterParams;

typedef STFDMA_LockChannelParams STFDMA_UnlockChannelParams;

#define FDMA_TYPE   0XFF     /* Unique module id - See 'ioctl-number.txt' */

#define IO_STFDMA_StartGenericTransfer      _IOWR(FDMA_TYPE,  4, StartGenericTransferParams)
#define IO_STFDMA_StartTransfer             _IOWR(FDMA_TYPE,  5, StartTransferParams)
#define IO_STFDMA_ResumeTransfer            _IOW (FDMA_TYPE,  6, STFDMA_TransferId_t)
#define IO_STFDMA_FlushTransfer             _IOW (FDMA_TYPE,  7, STFDMA_TransferId_t)
#define IO_STFDMA_AbortTransfer             _IOWR(FDMA_TYPE,  8, STFDMA_TransferId_t)
#define IO_STFDMA_AllocateContext           _IOR (FDMA_TYPE,  9, STFDMA_ContextId_t)
#define IO_STFDMA_DeallocateContext         _IOW (FDMA_TYPE, 10, STFDMA_ContextId_t)
#define IO_STFDMA_ContextGetSCList          _IOWR(FDMA_TYPE, 11, STFDMA_ContextGetSCListParams)
#define IO_STFDMA_ContextSetSCList          _IOWR(FDMA_TYPE, 12, STFDMA_ContextSetSCListParams)
#define IO_STFDMA_ContextGetSCState         _IOWR(FDMA_TYPE, 13, STFDMA_ContextGetSCStateParams)
#define IO_STFDMA_ContextSetSCState         _IOW (FDMA_TYPE, 14, STFDMA_ContextSetSCStateParams)
#define IO_STFDMA_ContextSetESBuffer        _IOW (FDMA_TYPE, 15, STFDMA_ContextSetESBufferParams)
#define IO_STFDMA_ContextSetESReadPtr       _IOWR(FDMA_TYPE, 16, STFDMA_ContextSetESReadPtrParams)
#define IO_STFDMA_ContextSetESWritePtr      _IOWR(FDMA_TYPE, 17, STFDMA_ContextSetESWritePtrParams)
#define IO_STFDMA_ContextGetESReadPtr       _IOWR(FDMA_TYPE, 18, STFDMA_ContextGetESReadPtrParams)
#define IO_STFDMA_ContextGetESWritePtr      _IOWR(FDMA_TYPE, 19, STFDMA_ContextGetESWritePtrParams)
#define IO_STFDMA_GetTransferStatus         _IOWR(FDMA_TYPE, 20, STFDMA_GetTransferStatusParams)
#define IO_STFDMA_LockChannelInPool         _IOWR(FDMA_TYPE, 21, STFDMA_LockChannelInPoolParams)
#define IO_STFDMA_LockChannel               _IOWR(FDMA_TYPE, 22, STFDMA_LockChannelParams)
#define IO_STFDMA_UnlockChannel             _IOW (FDMA_TYPE, 23, STFDMA_UnlockChannelParams)
#define IO_STFDMA_GetRevision               _IOR (FDMA_TYPE, 24, ST_Revision_t)
#define IO_STFDMA_SetAddDataRegionParameter _IOW (FDMA_TYPE, 25, STFDMA_SetAddDataRegionParameterParams)

#define FDMA_CMD0   _IO  (FDMA_TYPE, 0)         /* icoctl() - no argument */
#define FDMA_CMD1   _IOR (FDMA_TYPE, 1, int)    /* icoctl() - read an int argument */
#define FDMA_CMD2   _IOW (FDMA_TYPE, 2, long)   /* icoctl() - write a long argument */
#define FDMA_CMD3   _IOWR(FDMA_TYPE, 3, double) /* icoctl() - read/write a double argument */

#endif
