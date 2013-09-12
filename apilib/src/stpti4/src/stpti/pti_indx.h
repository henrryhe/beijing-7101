/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

  File Name: pti_indx.h
Description: indexing support

******************************************************************************/


#ifdef STPTI_NO_INDEX_SUPPORT
 #error Incorrectly included file!
#endif


#ifndef __PTI_INDX_H
 #define __PTI_INDX_H


/* Includes ------------------------------------------------------------ */


#include "pti_loc.h"


/* Exported Types ------------------------------------------------------ */


typedef volatile struct
{
    STPTI_Handle_t          OwnerSession;
    STPTI_Signal_t          Handle;
    U32                     TC_IndexIdent;
    STPTI_IndexDefinition_t IndexMask;
    FullHandle_t            SlotListHandle;     /* Associations */
    STPTI_Pid_t             AssociatedPid;
    U16                     TC_SCDFilterIdent;  /* Index into table of Start Code Filters on tc */
    U8                      MPEGStartCode;
    U32                     MPEGStartCodeMode;  /* Mask used for SCD (which Start Codes to detect) */
} Index_t;

#define TC_SCD_FILTER_INDEX_NOT_SET 0xFFFF

/* Exported Function Prototypes ---------------------------------------- */

/* change parameters to pass partition for task_init GNBvd21068*/
ST_ErrorCode_t stpti_IndexQueueInit(ST_Partition_t *Partition_p);
ST_ErrorCode_t stpti_IndexQueueTerm(void);
ST_ErrorCode_t stpti_DirectQueueIndex(FullHandle_t Handle, STEVT_EventConstant_t Event, BOOL Dumped, BOOL Signalled, STPTI_EventData_t *EventData);
ST_ErrorCode_t stpti_IndexAssociateSlot(FullHandle_t IndexHandle, FullHandle_t SlotHandle);
ST_ErrorCode_t stpti_DeallocateIndex(FullHandle_t IndexHandle, BOOL Force);
ST_ErrorCode_t stpti_SlotDisassociateIndex(FullHandle_t SlotHandle, FullHandle_t IndexHandle, BOOL PreChecked);


#endif  /* __PTI_INDX_H */
