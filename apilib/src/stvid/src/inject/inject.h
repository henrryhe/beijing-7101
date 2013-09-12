/*******************************************************************************

File name   : inject.h

Description : Injection controller header file, ontop fdma driver.

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
 Jun 2003            Created                                      Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __INJECT_H
#define __INJECT_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */

#include "stavmem.h"

#include "stfdma.h"

#include "vid_com.h"

/* Exported Constant -------------------------------------------------------- */

#define BAD_INJECT_NUM              0xffffffff
#define ALIGNEMENT_FOR_TRANSFERS    0xffffff80

/* Exported Type ------------------------------------------------------------ */

typedef void  (*VIDINJ_TransferDoneFct_t) (U32 UserIdent,void * ES_Write_p,
                                              void * SCListWrite_p,
                                              void * SCListLoop_p
#if defined(DVD_SECURED_CHIP)
                                              , void * ESCopy_Write_p
#endif /* DVD_SECURED_CHIP */
);

typedef void * VIDINJ_Handle_t;

typedef struct VIDINJ_GetInjectParams_s
{
    VIDINJ_TransferDoneFct_t    TransferDoneFct;
    U32                         UserIdent;
    STAVMEM_PartitionHandle_t   AvmemPartition;
    ST_Partition_t *            CPUPartition;
} VIDINJ_GetInjectParams_t; /* init-params - like */

typedef struct VIDINJ_ParserRanges_s
{
    STFDMA_SCState_t            RangeConfig;
    STFDMA_SCRange_t            RangeId;
} VIDINJ_ParserRanges_t;

typedef struct VIDINJ_InitParams_s
{
    ST_Partition_t *    CPUPartition_p;
    STVID_DeviceType_t  DeviceType;
    BOOL                FDMASecureTransferMode; /* 0=non-secure, 1=secure */
} VIDINJ_InitParams_t;

/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t VIDINJ_Init(const VIDINJ_InitParams_t * const InitParams_p, VIDINJ_Handle_t * const InjecterHandle_p);
ST_ErrorCode_t VIDINJ_Term(const VIDINJ_Handle_t InjecterHandle);

void VIDINJ_EnterCriticalSection(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum);
void VIDINJ_LeaveCriticalSection(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum);

void VIDINJ_GetRemainingDataToInject(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, U32* RemainingData);
void VIDINJ_InjectStartCode(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const U32 SCValue, const void* SCAdd);
void VIDINJ_SoftReset(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum);
void VIDINJ_TransferReset(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum);
void VIDINJ_TransferFlush(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const BOOL DiscontinuityStartCode, U8 * const Pattern, const U8 Size);
U32  VIDINJ_TransferGetInject(const VIDINJ_Handle_t InjecterHandle, VIDINJ_GetInjectParams_t * const Params_p);
void VIDINJ_TransferReleaseInject(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum);
void VIDINJ_TransferStop(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum);
void VIDINJ_TransferStart(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const BOOL IsRealTime);
void VIDINJ_TransferLimits(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                           void * const ES_Start_p,       void * const ES_Stop_p,
                           void * const SCListStart_p,    void * const SCListStop_p,
                           void * const InputStart_p,     void * const InputStop_p
#if defined(DVD_SECURED_CHIP)
                           , void * const ESCopy_Start_p,   void * const ESCopy_Stop_p
#endif /* DVD_SECURED_CHIP */
                           );
ST_ErrorCode_t VIDINJ_Transfer_SetDataInputInterface(const VIDINJ_Handle_t InjecterHandle,
        const U32 InjectNum,
        ST_ErrorCode_t (*GetWriteAddress)(void *  const Handle,
                                          void ** const Address_p),
        void (*InformReadAddress)(void * const Handle, void * const Address),
        void * const Handle );
void VIDINJ_TransferSetESRead(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, void * const Read );

#if defined(DVD_SECURED_CHIP)
void VIDINJ_TransferSetESCopyBufferRead(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, void * const Read);
#endif /* DVD_SECURED_CHIP */

void VIDINJ_TransferSetSCListRead(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, void * const Read );
void VIDINJ_SetSCRanges(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const U32 NbParserRanges, VIDINJ_ParserRanges_t ParserRanges[]);
ST_ErrorCode_t VIDINJ_ReallocateFDMANodes(const VIDINJ_Handle_t InjecterHandle, const U32 InjecterIndex, const STVID_SetupParams_t * const SetupParams_p);

#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t VIDINJ_GetStatistics(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                                STVID_Statistics_t * const Statistics_p);
ST_ErrorCode_t VIDINJ_ResetStatistics(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                                const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
ST_ErrorCode_t VIDINJ_GetStatus(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                                STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */


#ifdef ST_OSLINUX
ST_ErrorCode_t StubInject_SetStreamSize(U32 Size);
ST_ErrorCode_t StubInject_GetStreamSize(U32 * Size_p);
ST_ErrorCode_t StubInject_SetBBInfo(void * BaseAddress_p, U32 Size);
ST_ErrorCode_t StubInject_GetBBInfo(void ** BaseAddress_pp, U32 * Size_p);
#endif /* ST_OSLINUX */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __INJECT_H */
