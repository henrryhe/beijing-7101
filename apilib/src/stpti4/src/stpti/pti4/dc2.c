/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: dc2.c
 Description: 

******************************************************************************/

#include <string.h>

#include "stddefs.h"
#include "stdevice.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"

#include "memget.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"


#ifndef STPTI_DC2_SUPPORT
 #error Incorrect build options!
#endif


#define DC2_FILTER_INITIAL_STATE_ENABLED    0x0001
#define DC2_FILTER_REPEATED                 0x0002


/******************************************************************************
Function Name : stptiHAL_DC2FilterAllocateMulticast16
  Description : stpti_TCGetFreeDC2Multicast16Filter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DC2FilterAllocateMulticast16(FullHandle_t DeviceHandle, U32 *FilterIdent)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) & PrivateData_p->TC_Params;

    U32 filter;


    for (filter = 0; filter < TC_Params_p->TC_NumberPesFilters; filter++)
    {
        if ( PrivateData_p->PesFilterHandles_p[filter] == STPTI_NullHandle() )
        {
            *FilterIdent = filter;
            return( ST_NO_ERROR );
        }
    }

    return( STPTI_ERROR_NO_FREE_FILTERS );
} 


/******************************************************************************
Function Name : stptiHAL_DC2FilterInitialiseMulticast16
  Description : stpti_TCInitialiseDC2Multicast16Filter
   Parameters :
******************************************************************************/
ST_ErrorCode_t  stptiHAL_DC2FilterInitialiseMulticast16(FullHandle_t DeviceHandle, U32 FilterIdent, STPTI_Filter_t FilterHandle)
{
    stptiMemGet_Device(DeviceHandle)->TCPrivateData.PesFilterHandles_p[FilterIdent] = FilterHandle;

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DC2FilterAssociateMulticast16
  Description : stpti_TCAssociateDC2Multicast16Filter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DC2FilterAssociateMulticast16(FullHandle_t SlotHandle, FullHandle_t FilterHandle)
{
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    U32              SlotIdent     = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo_p  =       &((TCMainInfo_t *)  TC_Params_p->TC_MainInfoStart)[SlotIdent];


    if ( MainInfo_p->MIBF3.SectionPesFilter_p == TC_INVALID_LINK )
    {
        U32 FilterIdent = stptiMemGet_Filter(FilterHandle)->TC_FilterIdent;
        TCPESFilter_t *TCPESFilter_p = &((TCPESFilter_t *) TC_Params_p->TC_PESFilterStart)[FilterIdent];

		WRITE_TCMainInfo(MainInfo_p, 3, SectionPesFilter_p, ((U8 *)TCPESFilter_p - (U8 *)TC_Params_p->TC_DataStart + (U8 *)0x8000));

        return( ST_NO_ERROR );
   }

    return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
}


/******************************************************************************
Function Name : stptiHAL_DC2FilterSetMulticast16
  Description : stpti_TCSetUpDC2Multicast16Filter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DC2FilterSetMulticast16(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData_p)
{
    U32 FilterType = 0;    

    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FilterHandle);
    U32              FilterIdent   = stptiMemGet_Filter(FilterHandle)->TC_FilterIdent;

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCPESFilter_t        *TCPESFilter_p =      &((TCPESFilter_t *)  TC_Params_p->TC_PESFilterStart)[FilterIdent];
        
    STPTI_CRITICAL_SECTION_BEGIN {

    TCPESFilter_p->FlagsData |= FilterType;
    TCPESFilter_p->FlagsMask  = FilterData_p->u.DC2Multicast16Filter.FilterBytes0;
     
    if  (FilterData_p->FilterRepeatMode == STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_REPEATED)
        TCPESFilter_p->FlagsData |= DC2_FILTER_REPEATED; 

    if  (FilterData_p->InitialStateEnabled == TRUE)
        TCPESFilter_p->FlagsData |= DC2_FILTER_INITIAL_STATE_ENABLED;
    
    } STPTI_CRITICAL_SECTION_END;

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DC2FilterSetMulticast32
  Description : stpti_TCSetUpDC2Multicast32Filter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DC2FilterSetMulticast32(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData_p)
{
    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DC2FilterSetMulticast48
  Description : stpti_TCSetUpDC2Multicast48Filter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DC2FilterSetMulticast48(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData_p)
{
    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DC2FilterDisassociateMulticast
  Description : stpti_TCDisassociateDC2MulticastFilter
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_DC2FilterDisassociateMulticast(FullHandle_t SlotHandle, FullHandle_t FilterHandle)
{
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    U32              SlotIdent     = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo_p  =       &((TCMainInfo_t *)  TC_Params_p->TC_MainInfoStart)[SlotIdent];

	WRITE_TCMainInfo(MainInfo_p, 3, SectionPesFilter_p, TC_INVALID_LINK);

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DC2FilterEnableMulticast
  Description : stpti_TCEnableDC2MulticastFilter
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_DC2FilterEnableMulticast(FullHandle_t FilterHandle)
{
    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DC2FilterDisableMulticast
  Description : stpti_TCDisableDC2MulticastFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DC2FilterDisableMulticast(FullHandle_t FilterHandle)
{
    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DC2FilterDeallocateMulticast
  Description : stpti_TCTerminateDC2MulticastFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DC2FilterDeallocateMulticast(FullHandle_t DeviceHandle, U32 FilterIdent)
{
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);

    PrivateData_p->PesFilterHandles_p[FilterIdent] = STPTI_NullHandle();

    return( ST_NO_ERROR );
}


/* EOF */
