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
#include "sttbx.h"
#include "stpti.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_indx.h"
#include "pti_hal.h"

#include "memget.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"

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


#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
/******************************************************************************
Function Name : stptiHAL_AllocateSCDFilter
  Description : Allocates a TC Start Code Detector Filter, and returns an index
                to it.  Will return 0xFFFF if it cannot allocate one.
   Parameters : IndexHandle - the host's index reference for the object which
                              will manage the start code detector filter.
******************************************************************************/
U16 stptiHAL_AllocateSCDFilter(FullHandle_t IndexHandle)
{
    TCPrivateData_t        *PrivateData_p = stptiMemGet_PrivateData(IndexHandle);
    STPTI_TCParameters_t     *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    U16 scd_filter_index;
    U32 *SCDTableAllocation_p = (U32 *) &(PrivateData_p->SCDTableAllocation);
    U16 TCSCD_Filter_Offset = 0;
    TCSCDFilterEntry_t *SCDFilter_p;

    for(scd_filter_index=0;scd_filter_index<TC_Params_p->TC_NumberSCDFilters;scd_filter_index++)
    {
        if( (*SCDTableAllocation_p&(1<<scd_filter_index)) == 0 ) break;
        TCSCD_Filter_Offset += sizeof(TCSCDFilterEntry_t);
    }
    if( scd_filter_index >= TC_Params_p->TC_NumberSCDFilters ) return( 0xFFFF );

    /* Now have a free SCD Filter index lets allocate this SCD filter */

    /* Update host tables to keep track of the allocation */
    stptiMemGet_Index(IndexHandle)->TC_SCDFilterIdent = scd_filter_index;
    *SCDTableAllocation_p |= 1<<scd_filter_index;
    STTBX_Print(("Allocated SCD Filter = %d at offset 0x%04x\n", scd_filter_index, TCSCD_Filter_Offset));
    STTBX_Print(("SCDTableAllocation = 0x%08x\n", *SCDTableAllocation_p));

    /* Initialise the NextIndex to self reference (i.e. default to not being in a chain of indexes) */
    SCDFilter_p = &((TCSCDFilterEntry_t *)TC_Params_p->TC_SCDFilterTableStart)[scd_filter_index];
    STSYS_WriteTCReg16LE((void*) &(SCDFilter_p->SCD_SlotStartCode), (TCSCD_Filter_Offset<<8) );       /* SCD_SlotStartCode is shared between StartCode (not set yet) and SCD_NextIndex */

    return( scd_filter_index );
}
#endif


/******************************************************************************
Function Name : stptiHAL_IndexSet
  Description : stpti_TCIndexSet
   Parameters :
    Here we just need to set up the IndexMask in mainInfo for the releated slot
    and set the required indexing bits into the mask
    
    stptiHAL_IndexSet is called directly by the user (index might not be 
    associated to a slot).  
    
    stptiHAL_IndexSet is also called when the user associates the index, as
    index information is found in MainInfo.
    
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexSet(FullHandle_t IndexHandle)
{
    TCPrivateData_t        *PrivateData_p = stptiMemGet_PrivateData(IndexHandle);
    STPTI_IndexDefinition_t IndexDef      = stptiMemGet_Index(IndexHandle)->IndexMask;
 
    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo_p = NULL;
    U32                   SlotIdent;
    Slot_t               *SlotStruct_p = NULL;     
    FullHandle_t          Handle;
    U32                   MatchedBits;
    BOOL                  PCRslotAssociated = FALSE;

    /* Need to reverse lookup the Slot for the index handle */
    for (SlotIdent = 0; SlotIdent < TC_Params_p->TC_NumberSlots; ++SlotIdent)
    {
        Handle.word = PrivateData_p->IndexHandles_p[SlotIdent];

        if ( Handle.word == IndexHandle.word )
        {
            break;
        }
    }
    if (SlotIdent < TC_Params_p->TC_NumberSlots)
    {
        MainInfo_p = &((TCMainInfo_t *)TC_Params_p->TC_MainInfoStart)[SlotIdent];
        Handle.word = PrivateData_p->SlotHandles_p[SlotIdent];
        if( stpti_SlotHandleCheck(Handle) == ST_NO_ERROR )
        {
            SlotStruct_p = stptiMemGet_Slot(Handle);
        }
    }
    
    if( ( MainInfo_p != NULL ) && (SlotStruct_p != NULL) )
    {
        if( SlotStruct_p->Type == STPTI_SLOT_TYPE_PCR || SlotStruct_p->PCRReturnHandle != STPTI_NullHandle())
        {
            /* We are either a PCR slot, or one is associated to this slot (i.e. on the same PID) */
            PCRslotAssociated = TRUE;
        }
        else
        {
            /* Not associated to a PCR slot, nor slot associated to a PCR slot */
            MatchedBits = 0;
        }
    }

    if( PCRslotAssociated )
    {
        /* We must preserve PCR and DISCOUNITUITY_INDICATOR events as they are handled by the PCR slot mechanism */
        MatchedBits =  STSYS_ReadRegDev16LE((void*)&MainInfo_p->IndexMask) <<16;
        MatchedBits &= STATUS_FLAGS_PCR_FLAG | STATUS_FLAGS_DISCONTINUITY_INDICATOR;
    }
    else
    {
        MatchedBits = 0;
    }

    /* Set the IndexMask for the found MainInfo the Bit positions are the same
     as that used in StatusBlock */ 

    if (IndexDef.s.PayloadUnitStartIndicator)                    MatchedBits |= STATUS_FLAGS_PUSI_FLAG;
    if (IndexDef.s.ScramblingChangeToClear)                      MatchedBits |= STATUS_FLAGS_SCRAMBLE_CHANGE;
    if (IndexDef.s.ScramblingChangeToEven)                       MatchedBits |= STATUS_FLAGS_SCRAMBLE_CHANGE;
    if (IndexDef.s.ScramblingChangeToOdd)                        MatchedBits |= STATUS_FLAGS_SCRAMBLE_CHANGE;
    if (!PCRslotAssociated && IndexDef.s.DiscontinuityIndicator) MatchedBits |= STATUS_FLAGS_DISCONTINUITY_INDICATOR;
    if (IndexDef.s.RandomAccessIndicator)                        MatchedBits |= STATUS_FLAGS_RANDOM_ACCESS_INDICATOR;
    if (IndexDef.s.PriorityIndicator)                            MatchedBits |= STATUS_FLAGS_PRIORITY_INDICATOR;
    if (!PCRslotAssociated && IndexDef.s.PCRFlag)                MatchedBits |= STATUS_FLAGS_PCR_FLAG;
    if (IndexDef.s.OPCRFlag)                                     MatchedBits |= STATUS_FLAGS_OPCR_FLAG;
    if (IndexDef.s.SplicingPointFlag)                            MatchedBits |= STATUS_FLAGS_SPLICING_POINT_FLAG;
    if (IndexDef.s.TransportPrivateDataFlag)                     MatchedBits |= STATUS_FLAGS_PRIVATE_DATA_FLAG;
    if (IndexDef.s.AdaptationFieldExtensionFlag)                 MatchedBits |= STATUS_FLAGS_ADAPTATION_EXTENSION_FLAG;

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)

    /* Start Code detection is only available on PTI4SL.  This is because of memory (and hardware) limitations on other devices */
    if ( IndexDef.s.MPEGStartCode )
    {
        TCSCDFilterEntry_t *SCDFilter_p;
        U16 scd_filter_index      = stptiMemGet_Index(IndexHandle)->TC_SCDFilterIdent;
        U16 TCSCD_Filter_Offset;
        
        U16 MPEGStartCode         = (U16) stptiMemGet_Index(IndexHandle)->MPEGStartCode;
        U16 MPEGStartCodeMask     = (U16) (stptiMemGet_Index(IndexHandle)->MPEGStartCodeMode & 0xFFFF);

        if( scd_filter_index == TC_SCD_FILTER_INDEX_NOT_SET )
        {
            /* not allocated a SCD Filter lets go and find one */
            scd_filter_index = stptiHAL_AllocateSCDFilter( IndexHandle );
            if( scd_filter_index == 0xFFFF ) return( STPTI_ERROR_INDEX_NONE_FREE );
        }

        /* Now have a SCD Filter index to use now lets initialise the filter */
        SCDFilter_p = &((TCSCDFilterEntry_t *)TC_Params_p->TC_SCDFilterTableStart)[scd_filter_index];

        STSYS_WriteTCReg16LE((void*)&SCDFilter_p->SCD_SlotMask, 0 );                     /* In case the user is changing this on-the-fly make the following fields irrelevent */
        
        STSYS_ClearTCMask16LE((void*)&SCDFilter_p->SCD_SlotStartCode, 0x00FF );          /* SCD_SlotStartCode is shared between StartCode and SCD_NextIndex */
        STSYS_SetTCMask16LE((void*)&SCDFilter_p->SCD_SlotStartCode, MPEGStartCode );

        STSYS_WriteTCReg16LE((void*)&SCDFilter_p->SCD_SlotState, 0 );
        STSYS_WriteTCReg16LE((void*)&SCDFilter_p->SCD_SlotMask, MPEGStartCodeMask ); 


        if( MainInfo_p != NULL )
        {
            /* Now make sure the start code filter index is linked to the slot */
            /* We do a RMW as StartCodeIndexing_p is also reused for watch and record (this is the index's offset into the SCD table) */
            TCSCD_Filter_Offset  = STSYS_ReadRegDev16LE( (void*)&MainInfo_p->StartCodeIndexing_p);
            TCSCD_Filter_Offset &= (~TC_MAIN_INFO_STARTCODE_DETECTION_OFFSET_MASK);
            TCSCD_Filter_Offset |= stptiMemGet_Index(IndexHandle)->TC_SCDFilterIdent * sizeof(TCSCDFilterEntry_t);
            STSYS_WriteTCReg16LE( (void*)&MainInfo_p->StartCodeIndexing_p, TCSCD_Filter_Offset );
        }

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
    

    if( MainInfo_p != NULL )
    {
        STSYS_WriteTCReg16LE((void*)&MainInfo_p->IndexMask, (MatchedBits >> 16) ); 
    }

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
Function Name : stptiHAL_IndexChain
  Description : 
   Parameters :
   
   Used for the Start code detector "state machine".  The detector looks for a
   particular start code, and when it finds one, reports it, and moves on a state,
   looking for a different start code.
   
   This is handled by having an array of indexes with start codes and chaining
   them together.
   
******************************************************************************/
ST_ErrorCode_t stptiHAL_IndexChain(FullHandle_t *IndexHandles, U32 NumberOfHandles)
{
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    TCPrivateData_t        *PrivateData_p = stptiMemGet_PrivateData(IndexHandles[0]);
    STPTI_TCParameters_t     *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    
    U32 i;
    U16 MPEGStartCode_NextIndex, TCSCD_Filter_Offset;
    U16 scd_filter_index, next_scd_filter_index;

    TCSCDFilterEntry_t *SCDFilter_p;
    
    /* Make sure all indexes have start code filters allocated, and if not allocate them */
    /* Any problems we should abort here, before we do any initialising */
    for(i=0;i<NumberOfHandles;i++)
    {
        scd_filter_index = stptiMemGet_Index(IndexHandles[(i+1)%NumberOfHandles])->TC_SCDFilterIdent;
        if( scd_filter_index == TC_SCD_FILTER_INDEX_NOT_SET )
        {
            /* not allocated a SCD Filter, lets go and find one */
            next_scd_filter_index = stptiHAL_AllocateSCDFilter( IndexHandles[(i+1)%NumberOfHandles] );
            if( next_scd_filter_index == 0xFFFF ) return( STPTI_ERROR_INDEX_NONE_FREE );
        }
    }

    /* We now have start code filters allocated, now chain them together */
    scd_filter_index = stptiMemGet_Index(IndexHandles[0])->TC_SCDFilterIdent;
    for(i=0;i<NumberOfHandles;i++)
    {
        next_scd_filter_index = stptiMemGet_Index(IndexHandles[(i+1)%NumberOfHandles])->TC_SCDFilterIdent;

        /* Get a pointer to the start code filter and its offset in tc memory */
        SCDFilter_p = &((TCSCDFilterEntry_t *)TC_Params_p->TC_SCDFilterTableStart)[scd_filter_index];
        TCSCD_Filter_Offset = next_scd_filter_index*sizeof(TCSCDFilterEntry_t);
        
        /* SCD_SlotStartCode is shared between StartCode (not set yet) and SCD_NextIndex */
        /* We set SCD_NextIndex to the next start code filter (a linked list affair) */
        MPEGStartCode_NextIndex  = STSYS_ReadRegDev16LE( (void*) &(SCDFilter_p->SCD_SlotStartCode) );
        MPEGStartCode_NextIndex &= 0x00FF;
        MPEGStartCode_NextIndex |= (TCSCD_Filter_Offset<<8);
        
        STSYS_WriteTCReg16LE((void*) &(SCDFilter_p->SCD_SlotStartCode), MPEGStartCode_NextIndex );  
        
        scd_filter_index = next_scd_filter_index;
    }
    
    return( ST_NO_ERROR );
#else
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
#endif
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

