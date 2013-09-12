/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_dvb.c
 Description: dvb specific functions.

******************************************************************************/


/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "stcommon.h"
#endif /* #endif !defined(ST_OSLINUX) */
#include "stdevice.h"
#include "stpti.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX) */

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_list.h"
#include "pti_loc.h"
#include "pti_mem.h"
#include "pti_hal.h"
#include "validate.h"

#include "memget.h"


/* Functions --------------------------------------------------------------- */


#ifndef STPTI_NullHandle
extern STPTI_Handle_t STPTI_NullHandle(void);
#endif


#if !defined ( STPTI_BSL_SUPPORT )

/* ------------------------------------------------------------------------- */


/******************************************************************************

Function Name : STPTI_BufferExtractSectionData
  Description :
   Parameters :-
   
        STPTI_Buffer_t      BufferHandle          : Handle of Buffer containing section
        
        STPTI_Filter_t *    MatchedFilterList     : A pointer to previously allocated space 
                                                    where a list of filters that this section 
                                                    matched may be placed
                                                    
        U16                 MaxLengthofFilterList : Maximum number of filters that may be palced
                                                    in list ( i.e ) the size of the allocated 
                                                    memory in units of sizeof(STPTI_Filter_t)
                                                    
        U16 *               NumOfFilterMatches    : A pointer to a location there the number of 
                                                    filters that the section matched is to be placed.
                                                    If this is less than 'MaxLengthofFilterList'
                                                    then it will be the number of filters in the list, 
                                                    otherwise the list will contain only the first 
                                                    'MaxLengthofFilterList' of the total matched
                                                    
        BOOL *              CRCValid              : A pointer to a location where a BOOL is written 
                                                    that will indicate if the section's CRC was valid. 
                                                    Only works with Manual mode TC code, With Automatic 
                                                    Mode TC code then TRUE will always be returned. 
                                                    
        U32 *               SectionHeader         : A pointer to a location where the first 3 bytes of 
                                                    the section header will be placed ( MS byte set to zero )
                                                    
******************************************************************************/
ST_ErrorCode_t STPTI_BufferExtractSectionData(STPTI_Buffer_t BufferHandle, STPTI_Filter_t MatchedFilterList[],
                                              U16 MaxLengthofFilterList, U16 *NumOfFilterMatches_p, BOOL *CRCValid_p,
                                              U32 *SectionHeader_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferExtractSectionData( FullBufferHandle, MatchedFilterList, MaxLengthofFilterList, NumOfFilterMatches_p, CRCValid_p, SectionHeader_p );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DVBBufferExtractSectionData( FullBufferHandle, MatchedFilterList, MaxLengthofFilterList, NumOfFilterMatches_p, CRCValid_p, SectionHeader_p );
    }    

    STOS_SemaphoreSignal(stpti_MemoryLock);    
    return( Error );
}

/******************************************************************************
Function Name : STPTI_BufferExtractPartialPesPacketData
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferExtractPartialPesPacketData(STPTI_Buffer_t BufferHandle, BOOL *PayloadUnitStart_p,
                                                       BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p,
                                                       U16 *DataLength_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferExtractPartialPesPacketData( FullBufferHandle, PayloadUnitStart_p, CCDiscontinuity_p, ContinuityCount_p, DataLength_p );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DVBBufferExtractPartialPesPacketData( FullBufferHandle, PayloadUnitStart_p, CCDiscontinuity_p, ContinuityCount_p, DataLength_p);
    }    

    STOS_SemaphoreSignal(stpti_MemoryLock);    
    return( Error );
}


/******************************************************************************
Function Name : STPTI_BufferExtractPesPacketData
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferExtractPesPacketData(STPTI_Buffer_t BufferHandle, U8 *PesFlags_p, U8 *TrickModeFlags_p,
                                                U32 *PESPacketLength_p, STPTI_TimeStamp_t * PTSValue_p, STPTI_TimeStamp_t * DTSValue_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferExtractPesPacketData( FullBufferHandle, PesFlags_p, TrickModeFlags_p, PESPacketLength_p, PTSValue_p, DTSValue_p );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DVBBufferExtractPesPacketData( FullBufferHandle, PesFlags_p, TrickModeFlags_p, PESPacketLength_p, PTSValue_p, DTSValue_p );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);    
    return( Error );
}



/******************************************************************************
Function Name : STPTI_BufferReadPartialPesPacket
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferReadPartialPesPacket(STPTI_Buffer_t BufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                                U8 *Destination1_p, U32 DestinationSize1, BOOL *PayloadUnitStart_p,
                                                BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p, U32 *DataSize_p,
                                                STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);    
    FullBufferHandle.word = BufferHandle;

    if (NULL != DataSize_p) *DataSize_p = 0;

    Error = stptiValidate_BufferReadPartialPesPacket( FullBufferHandle,Destination0_p, DestinationSize0, Destination1_p, DestinationSize1, PayloadUnitStart_p, CCDiscontinuity_p, ContinuityCount_p, DataSize_p, DmaOrMemcpy );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DVBBufferReadPartialPesPacket( FullBufferHandle, Destination0_p, DestinationSize0, Destination1_p, DestinationSize1, PayloadUnitStart_p, CCDiscontinuity_p, ContinuityCount_p, DataSize_p, DmaOrMemcpy );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);    
    return( Error );
}


/******************************************************************************
Function Name : STPTI_BufferReadPes
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferReadPes(STPTI_Buffer_t BufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                   U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);    
    FullBufferHandle.word = BufferHandle;
   
    if (NULL != DataSize_p) *DataSize_p = 0;

    Error = stptiValidate_BufferReadPes( FullBufferHandle, Destination0_p, DestinationSize0, Destination1_p, DestinationSize1, DataSize_p, DmaOrMemcpy);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DVBBufferReadPes( FullBufferHandle, Destination0_p, DestinationSize0, Destination1_p, DestinationSize1, DataSize_p, DmaOrMemcpy );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);    
    return( Error );
}


/******************************************************************************
Function Name : STPTI_FilterSetMatchAction
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_FilterSetMatchAction(STPTI_Filter_t FilterHandle, STPTI_Filter_t FiltersToEnable[],
                                          U16 NumOfFiltersToEnable)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullFilterHandle;
    FullHandle_t MatchFilterListHandle;
    U32 filter;
  
    
    STOS_SemaphoreWait(stpti_MemoryLock);    
    FullFilterHandle.word = FilterHandle;    

    Error = stptiValidate_FilterSetMatchAction(  FullFilterHandle,  FiltersToEnable, NumOfFiltersToEnable);
    if (Error == ST_NO_ERROR)
    {
        MatchFilterListHandle = (stptiMemGet_Filter(FullFilterHandle))->MatchFilterListHandle;

        if ( MatchFilterListHandle.word != STPTI_NullHandle() )
        {
            /* match action already exists .... remove existing list */
            stpti_MemDealloc(&(stptiMemGet_Session(MatchFilterListHandle)->MemCtl), MatchFilterListHandle.member.Object);
            (stptiMemGet_Filter(FullFilterHandle))->MatchFilterListHandle.word = STPTI_NullHandle();
        }

        /* create and load match action list  */
        Error = stpti_CreateListObject(FullFilterHandle, &MatchFilterListHandle);

        (stptiMemGet_Filter(FullFilterHandle))->MatchFilterListHandle = MatchFilterListHandle;

        for (filter = 0; filter < NumOfFiltersToEnable; ++filter)
        {
            FullHandle_t TmpFilterHandle;

            TmpFilterHandle.word = FiltersToEnable[filter];

            Error = stptiValidate_FilterHandleCheck( TmpFilterHandle );
            if (Error == ST_NO_ERROR)
            {
                Error = stpti_AddObjectToList(MatchFilterListHandle, TmpFilterHandle);
                if (Error != ST_NO_ERROR)
                    break;
            }
            else
            {
                break;
            }
        }

        if (Error == ST_NO_ERROR)
            Error = stptiHAL_DVBFilterSetMatchAction( FullFilterHandle );
    }


    STOS_SemaphoreSignal(stpti_MemoryLock);    
    return( Error );
}


/******************************************************************************
Function Name : STPTI_FiltersFlush
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_FiltersFlush(STPTI_Buffer_t BufferHandle, STPTI_Filter_t Filters[], U16 NumOfFiltersToFlush)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;
    

    STOS_SemaphoreWait(stpti_MemoryLock);    
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_FiltersFlush( FullBufferHandle, Filters, NumOfFiltersToFlush );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DVBFiltersFlush( FullBufferHandle, Filters, NumOfFiltersToFlush );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);    
    return( Error );
}


/******************************************************************************
Function Name : STPTI_GetPacketErrorCount
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_GetPacketErrorCount(ST_DeviceName_t DeviceName, U32 *Count_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;    

    Error = stptiValidate_GetPacketErrorCount( DeviceName, Count_p );
    if (Error == ST_NO_ERROR)    
    {
	    STOS_SemaphoreWait(stpti_MemoryLock);    
	
        Error = stpti_GetDeviceHandleFromDeviceName( DeviceName, &DeviceHandle, TRUE );
        if (Error == ST_NO_ERROR)
        {
            Error = stptiHAL_DVBGetPacketErrorCount( DeviceHandle, Count_p );
        }

    	STOS_SemaphoreSignal(stpti_MemoryLock);    	
    }

    return( Error );
}

/******************************************************************************
Function Name : STPTI_ModifyGlobalFilterState
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_ModifyGlobalFilterState(STPTI_Filter_t FiltersToDisable[], U16 NumOfFiltersToDisable,
                                             STPTI_Filter_t FiltersToEnable[], U16 NumOfFiltersToEnable)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t TmpFilterHandle;
    U32 filter;

    
    STOS_SemaphoreWait(stpti_MemoryLock);
    
    Error = stptiValidate_ModifyGlobalFilterState( FiltersToDisable, NumOfFiltersToDisable, FiltersToEnable, NumOfFiltersToEnable);
    if (Error == ST_NO_ERROR)
    {
        for (filter = 0; filter < NumOfFiltersToDisable; ++filter)  /* Do filter disabling first */
        {
            TmpFilterHandle.word = FiltersToDisable[filter];

            (stptiMemGet_Filter(TmpFilterHandle))->Enabled = FALSE;

                switch ((stptiMemGet_Filter(TmpFilterHandle))->Type)
                {  
                    case STPTI_FILTER_TYPE_SECTION_FILTER:
                    case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE:
                    case STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE: 
                    case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE:                     
                        Error = stptiHAL_DVBFilterDisableSection(TmpFilterHandle);
                        break;

                    case STPTI_FILTER_TYPE_TSHEADER_FILTER:
                        Error = stptiHAL_DVBFilterDisableTransport(TmpFilterHandle);
                        break;

                    case STPTI_FILTER_TYPE_PES_FILTER:
                        Error = stptiHAL_DVBFilterDisablePES(TmpFilterHandle);
                        break;

#ifdef STPTI_DC2_SUPPORT
                    case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
                    case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
                    case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
                        Error = stptiHAL_DC2FilterDisableMulticast(TmpFilterHandle);
                        break;
#endif                                                    
                    default:
                        Error = ST_ERROR_BAD_PARAMETER; /* taken care of by stptiValidate_ModifyGlobalFilterState() */
                        break;
                }


        }

        if (Error == ST_NO_ERROR)
        {
            for (filter = 0; filter < NumOfFiltersToEnable; ++filter)   /* Now filter enabling */
            {
                TmpFilterHandle.word = FiltersToEnable[filter];

                (stptiMemGet_Filter(TmpFilterHandle))->Enabled = TRUE;

                    switch ((stptiMemGet_Filter(TmpFilterHandle))->Type)
                    {     
                        case STPTI_FILTER_TYPE_SECTION_FILTER:                       
                        case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE:
                        case STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE: 
                        case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE:   
                            Error = stptiHAL_DVBFilterEnableSection(TmpFilterHandle);
                            break;

                        case STPTI_FILTER_TYPE_TSHEADER_FILTER:
                            Error = stptiHAL_DVBFilterEnableTransport(TmpFilterHandle);
                            break;

                        case STPTI_FILTER_TYPE_PES_FILTER:
                            Error = stptiHAL_DVBFilterEnablePES(TmpFilterHandle);
                            break;                                          

#ifdef STPTI_DC2_SUPPORT
                        case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
                        case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
                        case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
                            Error = stptiHAL_DC2FilterEnableMulticast(TmpFilterHandle);
                            break;
#endif                            
                        default:
                            Error = ST_ERROR_BAD_PARAMETER;   /* taken care of by stptiValidate_ModifyGlobalFilterState() */
                            break;

                    }
            }
        }


    }   /* if(stptiValidate_ModifyGlobalFilterState) */

    STOS_SemaphoreSignal(stpti_MemoryLock);    
    return( Error );
}

/* STPTI_AlternateOutputSetDefaultAction moved from here to pti_extended.c*/
/******************************************************************************
Function Name : STPTI_SlotDescramblingControl
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SlotDescramblingControl(STPTI_Slot_t SlotHandle, BOOL EnableDescramblingControl)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_SlotDescramblingControl( FullSlotHandle );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DVBSlotDescramblingControl( FullSlotHandle, EnableDescramblingControl );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_SlotSetAlternateOutputAction
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SlotSetAlternateOutputAction(STPTI_Slot_t SlotHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_SlotSetAlternateOutputAction( FullSlotHandle, Method, Tag);
    if (Error == ST_NO_ERROR)
    {
        /* Mark this slot as NOT default action and load it's Alternate output parameters */
        (stptiMemGet_Slot(FullSlotHandle))->UseDefaultAltOut    = FALSE;
        (stptiMemGet_Slot(FullSlotHandle))->AlternateOutputType = Method;
        (stptiMemGet_Slot(FullSlotHandle))->AlternateOutputTag  = Tag;

        Error = stptiHAL_DVBSlotSetAlternateOutputAction(FullSlotHandle, Method);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);    
    return( Error );
}

#endif /* #if !defined (STPTI_BSL_SUPPORT )*/



/* --- EOF  --- */

