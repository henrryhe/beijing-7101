/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: index.c
 Description: see indexing API..

******************************************************************************/


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <string.h>
#endif /* #endif !defined(ST_OSLINUX) */

#if defined(ST_OSLINUX)
#include <linux/dma-mapping.h> /* dma_alloc_coherent etc. */
#endif /* #endif defined(ST_OSLINUX) */

#include "stddefs.h"
#include "stdevice.h"

#include "stpti.h"
#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_indx.h"
#include "pti_hal.h"

#include "memget.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"


#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX) */


/* ------------------------------------------------------------------------- */


#ifdef STPTI_NO_INDEX_SUPPORT
#error Incorrect build options!
#endif


#define REQUEST_FIRST_RECORDED_PACKET   0x01
#define START_INDEXING                  0x02


/******************************************************************************
Function Name :stptiHAL_IndexAllocate
  Description : stpti_TCGetFreeIndex
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexAllocate(FullHandle_t DeviceHandle, U32 *IndexIdent)
{
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32 Index;


    for (Index = 0; Index < TC_Params_p->TC_NumberIndexs; Index++)
    {
        if ( PrivateData_p->IndexHandles_p[Index] == STPTI_NullHandle() )
        {
            *IndexIdent = Index;
            return( ST_NO_ERROR );
        }
    }

    return( STPTI_ERROR_INDEX_NONE_FREE );
}


/******************************************************************************
Function Name : stptiHAL_IndexInitialise
  Description : stpti_TCInitialiseIndex
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexInitialise(FullHandle_t IndexHandle)
{
    /* Mark this newly created index as not having a MPEG Start Code Detector filter */
    stptiMemGet_Index(IndexHandle)->TC_SCDFilterIdent = TC_SCD_FILTER_INDEX_NOT_SET;
    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_IndexDeallocate
  Description : stpti_TCTerminateIndex
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexDeallocate(FullHandle_t IndexHandle)
{
    U16 SCDIndexIdent;
    TCPrivateData_t        *PrivateData_p = stptiMemGet_PrivateData(IndexHandle);
    U32 *SCDTableAllocation_p = (U32 *) &(PrivateData_p->SCDTableAllocation);
    
    /* we need to deallocate MPEG Start Code Detector Filter if one is allocated */
    SCDIndexIdent = stptiMemGet_Index(IndexHandle)->TC_SCDFilterIdent;      /* obtain the index into the SCD Filter table */
    if( SCDIndexIdent != TC_SCD_FILTER_INDEX_NOT_SET )
    {
        /* mark the entry in the SCD Filter table as not set (both for SCD filter allocation, and in the hosts index data) */
        *SCDTableAllocation_p &= ~(1<<SCDIndexIdent);                       
        stptiMemGet_Index(IndexHandle)->TC_SCDFilterIdent = TC_SCD_FILTER_INDEX_NOT_SET;
        STTBX_Print(("Deallocated SCD Filter = %d\n", SCDIndexIdent ));
        STTBX_Print(("SCDTableAllocation = 0x%08x\n", *SCDTableAllocation_p));
    }    

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_IndexSet
  Description : stpti_TCIndexSet
   Parameters :
     Here we just need to set up the IndexMask in mainInfo for the releated slot
    and set the required indexing bits into the mask
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexSet(FullHandle_t IndexHandle)
{
    TCPrivateData_t        *PrivateData_p = stptiMemGet_PrivateData(IndexHandle);
    STPTI_IndexDefinition_t IndexDef      = stptiMemGet_Index(IndexHandle)->IndexMask;
 
    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t *MainInfo_p;
    U32 SlotIdent;
    FullHandle_t Handle;
    U32 MatchedBits = 0;

    for (SlotIdent = 0; SlotIdent < TC_Params_p->TC_NumberSlots; ++SlotIdent)
    {
        Handle.word = PrivateData_p->IndexHandles_p[SlotIdent];

        if ( Handle.word == IndexHandle.word )
        {
            break;
        }
    }
    
    if (SlotIdent >= TC_Params_p->TC_NumberSlots)
        return( STPTI_ERROR_INDEX_NONE_FREE );

    MainInfo_p = &((TCMainInfo_t *)TC_Params_p->TC_MainInfoStart)[SlotIdent];

    /* Set the IndexMask for the found MainInfo the Bit positions are the same
     as that used in StatusBlock */ 

    if (IndexDef.s.PayloadUnitStartIndicator)    MatchedBits |= STATUS_FLAGS_PUSI_FLAG;
    if (IndexDef.s.ScramblingChangeToClear)      MatchedBits |= STATUS_FLAGS_SCRAMBLE_CHANGE;
    if (IndexDef.s.ScramblingChangeToEven)       MatchedBits |= STATUS_FLAGS_SCRAMBLE_CHANGE;
    if (IndexDef.s.ScramblingChangeToOdd)        MatchedBits |= STATUS_FLAGS_SCRAMBLE_CHANGE;
    if (IndexDef.s.DiscontinuityIndicator)       MatchedBits |= STATUS_FLAGS_DISCONTINUITY_INDICATOR;
    if (IndexDef.s.RandomAccessIndicator)        MatchedBits |= STATUS_FLAGS_RANDOM_ACCESS_INDICATOR;
    if (IndexDef.s.PriorityIndicator)            MatchedBits |= STATUS_FLAGS_PRIORITY_INDICATOR;
    if (IndexDef.s.PCRFlag)                      MatchedBits |= STATUS_FLAGS_PCR_FLAG;
    if (IndexDef.s.OPCRFlag)                     MatchedBits |= STATUS_FLAGS_OPCR_FLAG;
    if (IndexDef.s.SplicingPointFlag)            MatchedBits |= STATUS_FLAGS_SPLICING_POINT_FLAG;
    if (IndexDef.s.TransportPrivateDataFlag)     MatchedBits |= STATUS_FLAGS_PRIVATE_DATA_FLAG;
    if (IndexDef.s.AdaptationFieldExtensionFlag) MatchedBits |= STATUS_FLAGS_ADAPTATION_EXTENSION_FLAG;

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    /* Start Code detection is only available on PTI4SL.  This is because of memory limitations on other devices */
    if ( IndexDef.s.MPEGStartCode )
    {
        TCSCDFilterEntry_t *SCDFilter_p;
        U16 TCSCD_Filter_Offset;
        U16 scd_filter_index      = stptiMemGet_Index(IndexHandle)->TC_SCDFilterIdent;
        U32 *SCDTableAllocation_p = (U32 *) &(PrivateData_p->SCDTableAllocation);
        U16 MPEGStartCode         = (U16) stptiMemGet_Index(IndexHandle)->MPEGStartCode;
        U16 MPEGStartCodeMask     = (U16) (stptiMemGet_Index(IndexHandle)->MPEGStartCodeMode & 0xFFFF);

        /* Or in underlying video protocol - MPEG2 or H264 */
        MPEGStartCode            |= (U16) ((stptiMemGet_Index(IndexHandle)->MPEGStartCodeMode & STPTI_MPEG_START_CODE_H264) >> 16);

        if( scd_filter_index == TC_SCD_FILTER_INDEX_NOT_SET )
        {
            /* not allocated a SCD Filter let go and find one */
            TCSCD_Filter_Offset = 0;
            for(scd_filter_index=0;scd_filter_index<TC_Params_p->TC_NumberSCDFilters;scd_filter_index++)
            {
                if( ((*SCDTableAllocation_p<<scd_filter_index)&1) == 0 ) break;
                TCSCD_Filter_Offset += sizeof(TCSCDFilterEntry_t);
            }
            if( scd_filter_index >= TC_Params_p->TC_NumberSCDFilters ) return( STPTI_ERROR_INDEX_NONE_FREE );

            /* Now have a free SCD Filter index lets allocate this SCD filter */
            
            /* We do a RMW as StartCodeIndexing_p is also reused for watch and record (this is the index's offset into the SCD table) */
            STSYS_ClearTCMask16LE( (void*)&MainInfo_p->StartCodeIndexing_p, TC_MAIN_INFO_STARTCODE_DETECTION_OFFSET_MASK );
            STSYS_SetTCMask16LE( (void*)&MainInfo_p->StartCodeIndexing_p, TCSCD_Filter_Offset );
            STTBX_Print(("Set TC MainInfo StartCodeIndexing Pointer at address... %p\n", &MainInfo_p->StartCodeIndexing_p));
            
            /* Update host tables to keep track of the allocation */
            stptiMemGet_Index(IndexHandle)->TC_SCDFilterIdent = scd_filter_index;
            *SCDTableAllocation_p |= 1<<scd_filter_index;
            STTBX_Print(("Allocated SCD Filter = %d at offset 0x%04x\n", scd_filter_index, TCSCD_Filter_Offset));
            STTBX_Print(("SCDTableAllocation = 0x%08x\n", *SCDTableAllocation_p));
        }

        /* Now have a SCD Filter index to use now lets initialise the filter */
        STTBX_Print(("Set TC TC_SCDFilterTableStart at address... %p\n", &SCDFilter_p->SCD_SlotState));
        SCDFilter_p = &((TCSCDFilterEntry_t *)TC_Params_p->TC_SCDFilterTableStart)[scd_filter_index];
        STSYS_WriteTCReg16LE((void*)&SCDFilter_p->SCD_SlotState, 0 );
        STSYS_WriteTCReg16LE((void*)&SCDFilter_p->SCD_SlotStartCode, MPEGStartCode ); 
        STSYS_WriteTCReg16LE((void*)&SCDFilter_p->SCD_SlotMask, MPEGStartCodeMask ); 

        MatchedBits |= STATUS_FLAGS_START_CODE_FLAG;
    }
    else
    {
        U16 SCDIndexIdent;
        TCPrivateData_t        *PrivateData_p = stptiMemGet_PrivateData(IndexHandle);
        U32 *SCDTableAllocation_p = (U32 *) &(PrivateData_p->SCDTableAllocation);

        /* we deallocate MPEG Start Code Detector Filter if one is allocated */
        SCDIndexIdent = stptiMemGet_Index(IndexHandle)->TC_SCDFilterIdent;      /* obtain the index into the SCD Filter table */
        if( SCDIndexIdent != TC_SCD_FILTER_INDEX_NOT_SET )
        {
            /* mark the entry in the SCD Filter table as not set (both for SCD filter allocation, and in the hosts index data) */
            *SCDTableAllocation_p &= ~(1<<SCDIndexIdent);                       
            stptiMemGet_Index(IndexHandle)->TC_SCDFilterIdent = TC_SCD_FILTER_INDEX_NOT_SET;
            STTBX_Print(("Deallocated SCD Filter = %d\n", SCDIndexIdent ));
            STTBX_Print(("SCDTableAllocation = 0x%08x\n", *SCDTableAllocation_p));
        }    
    }
#endif
    

    STSYS_WriteTCReg16LE((void*)&MainInfo_p->IndexMask, (MatchedBits >> 16) ); 

    return( ST_NO_ERROR );
}



/******************************************************************************
Function Name : stptiHAL_IndexReset
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexReset(FullHandle_t IndexHandle)
{
    TCPrivateData_t        *PrivateData_p = stptiMemGet_PrivateData(IndexHandle);
 
    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t *MainInfo_p;
    U32 SlotIdent;
    FullHandle_t Handle;

    for (SlotIdent = 0; SlotIdent < TC_Params_p->TC_NumberSlots; ++SlotIdent)
    {
        Handle.word = PrivateData_p->IndexHandles_p[SlotIdent];

        if ( Handle.word == IndexHandle.word )
        {
            break;
        }
    }
    
    if (SlotIdent >= TC_Params_p->TC_NumberSlots)
        return( STPTI_ERROR_INDEX_NONE_FREE );

    MainInfo_p = &((TCMainInfo_t *)TC_Params_p->TC_MainInfoStart)[SlotIdent];

    STSYS_WriteTCReg16LE((void*)&MainInfo_p->IndexMask, 0 ); 

    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name : stptiHAL_IndexAssociate
  Description : stpti_TCAssociateIndexWithSlot
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexAssociate(FullHandle_t IndexHandle, FullHandle_t SlotHandle)
{
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    U32              SlotIdent     = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    

    PrivateData_p->IndexHandles_p[SlotIdent] = IndexHandle.word;

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_IndexDisassociate
  Description : stpti_TCDisassociateIndexFromSlot
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexDisassociate(FullHandle_t IndexHandle, FullHandle_t SlotHandle)
{
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    U32              SlotIdent     = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
  

    if ( PrivateData_p->IndexHandles_p[SlotIdent] != STPTI_NullHandle() )
    {
        PrivateData_p->IndexHandles_p[SlotIdent] = STPTI_NullHandle();
        return( ST_NO_ERROR );
    }

    return( STPTI_ERROR_INDEX_NOT_ASSOCIATED );
}


/******************************************************************************
Function Name : stptiHAL_IndexStart
  Description : stpti_TCIndexStart
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexStart(FullHandle_t Handle)
{
    /* The Handle passes is a DeviceHandle */    
    Device_t        *Device_p        = stptiMemGet_Device(Handle);   
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(Handle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t   *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ];

    STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask1,(REQUEST_FIRST_RECORDED_PACKET));
    STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask1,(START_INDEXING)); 
    

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_IndexStop
  Description : stpti_TCIndexStop
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexStop(FullHandle_t Handle)
{
    Device_t        *Device_p        = stptiMemGet_Device(Handle);       
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(Handle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t   *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ];

    STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask1,(REQUEST_FIRST_RECORDED_PACKET));
    STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask1,(START_INDEXING)); 
    return( ST_NO_ERROR );
}



/* EOF  -------------------------------------------------------------------- */
