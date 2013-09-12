/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: stdcam.c
 Description: Provides HAL interface to transport controller CAM

******************************************************************************/


/* Includes ---------------------------------------------------------------- */

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

/* ------------------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>
#endif /* #endif !defined(ST_OSLINUX) */

#include "stddefs.h"
#include "stdevice.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX) */
#include "stpti.h"

#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"
#include "memget.h"

#include "pti4.h"
#include "tchal.h"


/* ------------------------------------------------------------------------- */


/* session.c */
extern TCSessionInfo_t *stptiHAL_GetSessionFromFullHandle(FullHandle_t DeviceHandle);


/* ------------------------------------------------------------------------- */


typedef enum FilterListState_e
{
    FL_EMPTY,   /* the area is *not valid* (never been used) */
    FL_IN_USE,  /* the area is in active use by a session */
    FL_FREE     /* the area is valid but not in use */
} FilterListState_t;


/*  A bit mask is used for the CAM allocation - 32 bits each for CAMs A & B */

typedef struct FilterList_s
{
    U32               CamA;             /* bitmask of filters that are available */
    U32               CamB;
    U32               AllocatedCamA;    /* bitmask of filters that are allocated, */
    U32               AllocatedCamB;
    FilterListState_t State;
} FilterList_t;


/* ------------------------------------------------------------------------- */


static void ClearAllHardwareCams( TCDevice_t *TC_Device );

static int NumberOfFreeFilters( FilterList_t *FilterList_p, int FilterMode );

static int NumberOfSetBitsInWord( U32 Value );

static ST_ErrorCode_t AllocateFilterMasksFromFreeList( FilterList_t *FreeFilterList_p, U32 *CamA, U32 *CamB, int FilterMode, int NumFilters );

static U32 GetFirstSetBit( U32 Value );

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
static void DumpFilterList( TCPrivateData_t *PrivateData_p, STPTI_TCParameters_t *TC_Params_p );
#endif

static U32 PositionOfFirstSetBit( U32 Value );

#ifdef STPTI_WORKAROUND_SFCAM   /* PMC 17/02/03 CAM workaround function */
static ST_ErrorCode_t CAMSFArbiter( FullHandle_t FilterHandle, U32 CAM_0, U32 CAM_1, U32 CAM_2, U32 CAM_3, U32 NotMatchData, TCSectionFilter_t *TC_SectionFilter_p );
#endif


/* ------------------------------------------------------------------------- */
/*                         Init/Term Functions                               */
/* ------------------------------------------------------------------------- */


ST_ErrorCode_t TcCam_Terminate( FullHandle_t DeviceHandle )
{
    Device_t             *Device        = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t      *PrivateData_p = &Device->TCPrivateData;
#if !defined(ST_OSLINUX)
    ST_Partition_t       *Partition_p   =  Device->DriverPartition_p;
#endif /* #endif !defined(ST_OSLINUX) */

    /* trust that this is the last session. */

    STTBX_Print(("--- TcCam_Terminate() ---\n"));

    if ( PrivateData_p->FilterList != NULL )
        memory_deallocate( Partition_p, PrivateData_p->FilterList );

    PrivateData_p->FilterList = NULL;

    return( ST_NO_ERROR );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t TcCam_Initialize( FullHandle_t DeviceHandle )
{
    Device_t             *Device_p      = stptiMemGet_Device(DeviceHandle);
    TCDevice_t           *TC_Device     = (TCDevice_t *)Device_p->TCDeviceAddress_p;
    TCPrivateData_t      *PrivateData_p = &Device_p->TCPrivateData;
#if !defined(ST_OSLINUX)
    ST_Partition_t       *Partition_p   =  Device_p->DriverPartition_p;
#endif /* #endif !defined(ST_OSLINUX) */
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    int a;


    STTBX_Print(("*** TcCam_Init ***\n"));

    ClearAllHardwareCams( TC_Device );

    PrivateData_p->FilterList = memory_allocate( Partition_p, sizeof( FilterList_t ) * TC_Params_p->TC_NumberOfSessions );
    if ( PrivateData_p->FilterList == NULL )
        return( ST_ERROR_NO_MEMORY );

    for( a = 0; a < TC_Params_p->TC_NumberOfSessions; a++ ) /* reset the data area to 'never been used' */
    {
        PrivateData_p->FilterList[a].CamA           = 0x00000000;
        PrivateData_p->FilterList[a].CamB           = 0x00000000;
        PrivateData_p->FilterList[a].AllocatedCamA  = 0x00000000;
        PrivateData_p->FilterList[a].AllocatedCamB  = 0x00000000;
        PrivateData_p->FilterList[a].State          = FL_EMPTY;
    }

    /* hardware limit of 64 blocks of 8 byte filters or 32 16 byte filters (2 lots of 8 byte filters) */
    PrivateData_p->FilterList[0].CamA  = 0xFFFFFFFF;
    PrivateData_p->FilterList[0].CamB  = 0xFFFFFFFF;
    PrivateData_p->FilterList[0].State = FL_FREE;


    return( ST_NO_ERROR );
}



/******************************************************************************
Function Name : TcCam_SetMode
  Description : Sets the Filters Mode and slot's Associated Filter Mode based
                on the high level FilterType enum.
   Parameters : 
   
   This function is not needed for STDCAM as the filter mode is set once per
   virtual PTI.
   
******************************************************************************/
ST_ErrorCode_t TcCam_SetMode(TCSectionFilterInfo_t *TCSectionFilterInfo_p, Slot_t *Slot_p, STPTI_FilterType_t FilterType)
{
    return(ST_NO_ERROR);
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t TcCam_FilterFreeSession( FullHandle_t DeviceHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Device_t        *Device_p        = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t *PrivateData_p   = &Device_p->TCPrivateData;
    TCSessionInfo_t *TCSessionInfo_p = stptiHAL_GetSessionFromFullHandle( DeviceHandle );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
#endif
    int session = Device_p->Session;


    STTBX_Print(("\n *** TcCam_FilterFreeSession ***\n"));

    STSYS_WriteRegDev32LE( (void*)&TCSessionInfo_p->SectionEnables_0_31, 0 );
    STSYS_WriteRegDev32LE( (void*)&TCSessionInfo_p->SectionEnables_32_63, 0 );
    
    STTBX_Print(("Freeing filter for session %d\n", session ));

    PrivateData_p->FilterList[session].AllocatedCamA  = 0x00000000;
    PrivateData_p->FilterList[session].AllocatedCamB  = 0x00000000;
    PrivateData_p->FilterList[session].State          = FL_FREE;    /* mark as free for use */

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    DumpFilterList( PrivateData_p, TC_Params_p );
#endif

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t TcCam_FilterAllocateSession( FullHandle_t DeviceHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Device_t             *Device_p      = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t      *PrivateData_p = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p = stptiHAL_GetSessionFromFullHandle( DeviceHandle );
    FilterList_t         *FilterList_p, TmpFilterList;

    int  a, b, numfilters, filtermode, freefilters;
    U32 CamA, CamB;

    STTBX_Print(("\n *** TcCam_FilterAllocateSession ***\n"));

    STTBX_Print(("TCSessionInfo_p = 0x%08x\n", (U32)TCSessionInfo_p ));
    STTBX_Print(("SectionFilterOperatingMode: 0x%08x\n", (U32)Device_p->SectionFilterOperatingMode ));
    {
        numfilters = Device_p->NumberOfSectionFilters;
        
        switch( Device_p->SectionFilterOperatingMode )
        {
            case  STPTI_FILTER_OPERATING_MODE_8x8:
            case STPTI_FILTER_OPERATING_MODE_16x8:
            case STPTI_FILTER_OPERATING_MODE_32x8:
            case STPTI_FILTER_OPERATING_MODE_64x8:
            case STPTI_FILTER_OPERATING_MODE_ANYx8:
                STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_FILTER_TYPE_SHORT );
                break;

            case  STPTI_FILTER_OPERATING_MODE_8x16:
            case STPTI_FILTER_OPERATING_MODE_16x16:
            case STPTI_FILTER_OPERATING_MODE_32x16:
                STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH );
                break;

            case STPTI_FILTER_OPERATING_MODE_NONE:
                /* invalid filter mode i.e. all bits set - TC_SESSION_INFO_FILTER_TYPE_FIELD */
                STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_FILTER_TYPE_FIELD );
                break;

            default:
                return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
        }
        
        STSYS_SetTCMask16LE ((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_DVB_PACKET_FORMAT );
    }

    filtermode = STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSectionParams)&TC_SESSION_INFO_FILTER_TYPE_FIELD;
    STTBX_Print(("SessionSectionParams: 0x%08x\n", filtermode ));

    for( a = 0; a < TC_Params_p->TC_NumberOfSessions; a++ ) /* loop around in search of a free area */
    {
        FilterList_p = &PrivateData_p->FilterList[a];

        STTBX_Print(("\nlooking at index %d...", a ));

        if ( FilterList_p->State == FL_FREE )    /* found a free area! */
        {
            assert( a == Device_p->Session );   /* session index must match filter index */

            STTBX_Print(("its Free.\n"));

            if ( Device_p->SectionFilterOperatingMode == STPTI_FILTER_OPERATING_MODE_NONE )
            {
                CamA = 0;   /*  If we have no filters for this session then bump the free data */
                CamB = 0;   /* to the next empty space so as to keep N == M for filterlist[N] and session[M]. */
            }
            else
            {
                freefilters = NumberOfFreeFilters( FilterList_p, filtermode );
                STTBX_Print(("Free filter test %d (available) < %d (wanted)?\n", freefilters, numfilters ));
                if ( freefilters < numfilters ) /* too small so try somewhere else */
                    continue;

                Error = AllocateFilterMasksFromFreeList( FilterList_p, &CamA, &CamB, filtermode, numfilters );
                if ( Error != ST_NO_ERROR )
                    return( Error );
            }

            TmpFilterList.CamA = FilterList_p->CamA;    /* keep a note of the remaining free filters */
            TmpFilterList.CamB = FilterList_p->CamB;

            FilterList_p->CamA  = CamA;                 /* occupy this entry and mark it as in use.  */
            FilterList_p->CamB  = CamB;
            FilterList_p->State = FL_IN_USE;

            assert( FilterList_p->AllocatedCamA == 0 ); /* should not be any allocated filters */
            assert( FilterList_p->AllocatedCamB == 0 );

            freefilters = NumberOfFreeFilters( &TmpFilterList, filtermode );
            STTBX_Print(("Free filters remaining: %d\n", freefilters ));

            /* find an empty entry in the list (if we can) to put the remaining free filters or if no filters requested then
             bump the free filters to the next empty area. */
            if ( freefilters > 0 )
            {
                for( b = 0; b < TC_Params_p->TC_NumberOfSessions; b++ ) /* scan all areas from the beginning */
                {
                   if ( PrivateData_p->FilterList[b].State == FL_EMPTY )  /* found an empty area */
                   {
                        PrivateData_p->FilterList[b].CamA  = TmpFilterList.CamA;
                        PrivateData_p->FilterList[b].CamB  = TmpFilterList.CamB;
                        PrivateData_p->FilterList[b].State = FL_FREE; /* mark as free for use */
                        break;  /* for(b) */
                   }
                }

                /* If we could not find a FL_EMPTY just forget the remainder */
            }

            /* do remaining filter setup stuff */
            
            /* Force CRC */
            STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_FORCECRCSTATE );
            if ( Device_p->DiscardOnCrcError == TRUE )
            {
                STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_DISCARDONCRCERROR );
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_DISCARDONCRCERROR );
            }

            STSYS_WriteRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_0_31, 0 ); /* nothing allocated yet, so nothing enabled from our FilterList_p[n] */
            STSYS_WriteRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_32_63, 0 );

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
            DumpFilterList( PrivateData_p, TC_Params_p );
#endif

            return( ST_NO_ERROR );
        }

    }   /* for(a) */

    STTBX_Print(("cannot allocate!\n"));

    return( ST_ERROR_NO_MEMORY );
}


/* ------------------------------------------------------------------------- */
/*                           Inner API Functions                             */
/* ------------------------------------------------------------------------- */


U32 TcCam_RelativeMetaBitMaskPosition(FullHandle_t FullFilterHandle){
    return stptiMemGet_Filter(FullFilterHandle)->TC_FilterIdent;
}


int TcCam_GetMetadataSizeInBytes( void )
{
    return( 8 );    /* 64 bits, one for each 8 byte filter. */
}

/***************************************************************
Name: TcCam_GetFilterOffset
Description: Returns the bit offset into match_result register
Parameters: FullBufferHandle (not used in stdcam)
****************************************************************/
U32 TcCam_GetFilterOffset( FullHandle_t FullBufferHandle )
{
    FullBufferHandle = FullBufferHandle;        /* prevent compiler complaint */
    return( 0 );
}




/******************************************************************************
Function Name : stptiHAL_MaskConvertToList
  Description : Returns the Matched Filter List
   Parameters : 
   
******************************************************************************/
void stptiHAL_MaskConvertToList(  U8* MetaData,
                                  STPTI_Filter_t MatchedFilterList[],
                                  U32 MaxLengthofFilterList,
                                  BOOL CRCValid,
                                  U32 *NumOfFilterMatches_p,
                                  TCPrivateData_t *PrivateData_p,
                                  FullHandle_t FullBufferHandle,
                                  BOOL DSS_MPT_Mode )
{

    U32 i;
    U32 filter = 0;
    U32 MatchCount = 0;
    U32 FilterOffset = TcCam_GetFilterOffset(FullBufferHandle);
    U32 CombinedMatches, mask;

    U32 filtermode;
    TCSessionInfo_t         *TCSessionInfo_p     = stptiHAL_GetSessionFromFullHandle( FullBufferHandle );

    U32 MatchedFilterMask[2];

    /* STDCAM */
    MatchedFilterMask[0]  = (U32) MetaData[0];              /* CAMA filters 0..7       */
    MatchedFilterMask[0] |= (U32) (MetaData[1] << 8);       /* CAMA filters 8..15      */
    MatchedFilterMask[0] |= (U32) (MetaData[2] << 16);      /* CAMA filters 16..23     */
    MatchedFilterMask[0] |= (U32) (MetaData[3] << 24);      /* CAMA filters 24..31     */
    MatchedFilterMask[1]  = (U32) MetaData[4];              /* CAMB filters 0..7       */
    MatchedFilterMask[1] |= (U32) (MetaData[5] << 8);       /* CAMB filters 8..15      */
    MatchedFilterMask[1] |= (U32) (MetaData[6] << 16);      /* CAMB filters 16..23     */
    MatchedFilterMask[1] |= (U32) (MetaData[7] << 24);      /* CAMB filters 24..31     */

    if(DSS_MPT_Mode)
    {
        /* for MPT frames we are using Positive Negative Match Mode, but the Negative Match part   */
        /* has a cleared mask, so never matches.  We invert it so that it appears to have matched */
        /* for stptiHAL_MaskConvertToList(). */
        MatchedFilterMask[1] = ~MatchedFilterMask[1];
    }

    filtermode = STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSectionParams )&TC_SESSION_INFO_FILTER_TYPE_FIELD;

    if(filtermode==TC_SESSION_INFO_FILTER_TYPE_SHORT)
    {
        /* For SHORT filters we allocate firstly in CAMA until full, then start CAMB */
        for (i = 0; i < 2; ++i)
        {
            for (mask = 1; (mask != 0); ++filter, mask <<= 1)
            {
                if (MatchedFilterMask[i] & mask)
                {
                    FullHandle_t FilterHandle;

                    /* found one that matched */

                    FilterHandle.word = PrivateData_p->SectionFilterHandles_p[FilterOffset+filter];

                    if ( (CRCValid) || !( stptiMemGet_Filter(FilterHandle)->DiscardOnCrcError) )
                    {
                        if (MatchCount < MaxLengthofFilterList)
                        {
                            /* Room for it in list... so add it in */
                            MatchedFilterList[MatchCount] = FilterHandle.word;
                        }
                        ++MatchCount;
                    }
                }
            }
        }
    }
    else if((filtermode==TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH)||(filtermode==TC_SESSION_INFO_FILTER_TYPE_LONG))
    {
        /* For LONG filters we allocate one half in CAMA and the other in CAMB, both need to have matched */
        if(filtermode == TC_SESSION_INFO_FILTER_TYPE_LONG)
        {
            CombinedMatches = MatchedFilterMask[0] & MatchedFilterMask[1];
        }
        else    /* For Negative filters complement Mask[1] */
        {
            CombinedMatches = MatchedFilterMask[0] & ~MatchedFilterMask[1]; 
        }
        for (mask = 1, filter = 0; (mask != 0); ++filter, mask <<= 1)
        {
            if (CombinedMatches & mask) /* found one that matched */
            {
                FullHandle_t FilterHandle;

                FilterHandle.word = PrivateData_p->SectionFilterHandles_p[FilterOffset+filter];

                if ( (CRCValid) || !( stptiMemGet_Filter(FilterHandle)->DiscardOnCrcError) )
                {
                    if (MatchCount < MaxLengthofFilterList)
                    {
	                    MatchedFilterList[MatchCount] = FilterHandle.word;  /* Room for it in list... so add it in */
	                }
                    ++MatchCount;
                }
            }
        }
    }
    *NumOfFilterMatches_p = MatchCount; /* Load up number of matched filters */
    
}



/* ------------------------------------------------------------------------- */

/******************************************************************************
Function Name : stptiHAL_DVBFilterEnableSection
  Description : stpti_TCEnableSectionFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t TcCam_EnableFilter( FullHandle_t FilterHandle, BOOL enable )
{
    U32               FilterIdent   = stptiMemGet_Filter(FilterHandle)->TC_FilterIdent;
    TCSessionInfo_t  *TCSessionInfo_p = stptiHAL_GetSessionFromFullHandle( FilterHandle );
    
    /* FilterMode is not used in stdcam's TcCam_SetFilterEnableState */
    TcCam_SetFilterEnableState( TCSessionInfo_p,  FilterIdent, 0, enable);

    return( ST_NO_ERROR );
}


void TcCam_SetFilterEnableState( TCSessionInfo_t *TCSessionInfo_p, U32 FilterIndex, U16 TC_FilterMode, BOOL EnableFilter )
{
    if ( EnableFilter )
    {
        if (FilterIndex < 32)
            TCSessionInfo_p->SectionEnables_0_31 |= (1 << FilterIndex);
        else
            TCSessionInfo_p->SectionEnables_32_63 |= (1 << (FilterIndex - 32));
    }
    else
    {
        if (FilterIndex < 32)
            TCSessionInfo_p->SectionEnables_0_31 &= ~(1 << FilterIndex);
        else
            TCSessionInfo_p->SectionEnables_32_63 &= ~(1 << (FilterIndex - 32));
    }
}


/* ------------------------------------------------------------------------- */


void TcCam_ClearSectionInfoAssociations( TCSectionFilterInfo_t *TCSectionFilterInfo_p )
{
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0, 0 );
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1, 0 );
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2, 0 );
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3, 0 );
}


/* ------------------------------------------------------------------------- */


BOOL TcCam_SectionInfoNothingIsAssociated( TCSectionFilterInfo_t *TCSectionFilterInfo_p )
{
    if ( (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0) == 0) &&
         (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1) == 0) &&
         (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2) == 0) &&
         (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3) == 0) )
        return( TRUE );

    return( FALSE );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t TcCam_DisassociateFilterFromSectionInfo(FullHandle_t DeviceHandle, TCSectionFilterInfo_t *TCSectionFilterInfo_p, U32 FilterIndex )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (FilterIndex < 16)
    {
        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0, (1 << FilterIndex) );
    }
    else if (FilterIndex < 32)
    {
        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1, (1 << (FilterIndex - 16)) );
    }
    else if (FilterIndex < 48)
    {
        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2, (1 << (FilterIndex - 32)) );
    }
    else
    {
        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3, (1 << (FilterIndex - 48)) );
    }

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t TcCam_AssociateFilterWithSectionInfo(FullHandle_t DeviceHandle, TCSectionFilterInfo_t *TCSectionFilterInfo_p, U32 FilterIndex )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (FilterIndex < 16)
    {
        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0) & (1 << FilterIndex))
        {
            Error = STPTI_ERROR_FILTER_ALREADY_ASSOCIATED;
        }
        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0, (1 << FilterIndex) );
    }
    else if (FilterIndex < 32)
    {
        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1) & (1 << (FilterIndex - 16)))
        {
            Error = STPTI_ERROR_FILTER_ALREADY_ASSOCIATED;
        }
        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1, (1 << (FilterIndex - 16)) );
    }
    else if (FilterIndex < 48)
    {
        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2) & (1 << (FilterIndex - 32)))
        {
            Error = STPTI_ERROR_FILTER_ALREADY_ASSOCIATED;
        }
        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2, (1 << (FilterIndex - 32)) );
    }
    else    /* FilterIndex <= 64 */
    {
        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3) & (1 << (FilterIndex - 48)))
        {
            Error = STPTI_ERROR_FILTER_ALREADY_ASSOCIATED;
        }
        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3, (1 << (FilterIndex - 48)) );
    }

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t TcCam_FilterDeallocate( FullHandle_t DeviceHandle, U32 FilterIndex )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Device_t        *Device_p        = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t *PrivateData_p   = &Device_p->TCPrivateData;
    TCSessionInfo_t *TCSessionInfo_p = stptiHAL_GetSessionFromFullHandle( DeviceHandle );
    FilterList_t    *FilterList_p    = &PrivateData_p->FilterList[Device_p->Session];

    if ( (STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSectionParams) & TC_SESSION_INFO_FILTER_TYPE_FIELD) == TC_SESSION_INFO_FILTER_TYPE_SHORT)
    {
        if ( FilterIndex >= 64 )
            return( STPTI_ERROR_INVALID_FILTER_HANDLE );

        if ( FilterIndex < 32 )
        {
             FilterList_p->AllocatedCamA &= ~(1 << FilterIndex);
        }
        else    /*  32 >= FilterIndex < 64 */
        {
             FilterList_p->AllocatedCamB &= ~(1 << (FilterIndex - 32));
        }
    }
    else if ( (STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSectionParams) & TC_SESSION_INFO_FILTER_TYPE_FIELD) == TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH)
    {
        if ( FilterIndex >= 32 )
            return( STPTI_ERROR_INVALID_FILTER_HANDLE );

        FilterList_p->AllocatedCamA &= ~(1 << FilterIndex);
        FilterList_p->AllocatedCamB &= ~(1 << FilterIndex);
    }
    else
    {
        Error = STPTI_ERROR_INVALID_FILTER_OPERATING_MODE;
    }

    return ( Error );
}


/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */


ST_ErrorCode_t TcCam_FilterAllocate( FullHandle_t DeviceHandle, STPTI_FilterType_t FilterType, U32 *FilterIndex )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Device_t        *Device_p        = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t *PrivateData_p   = &Device_p->TCPrivateData;
    TCSessionInfo_t *TCSessionInfo_p = stptiHAL_GetSessionFromFullHandle( DeviceHandle );
    FilterList_t    *FilterList_p = &PrivateData_p->FilterList[Device_p->Session];

    U32 CamA, CamB;
    U32 FreeA, FreeB;
    

    STTBX_Print(("\n *** TcCam_FilterAllocate (session %d) ***\n", Device_p->Session ));

    FilterList_p = &PrivateData_p->FilterList[Device_p->Session];

    CamA = FilterList_p->CamA & ~FilterList_p->AllocatedCamA;   /* mask off any used filters */
    CamB = FilterList_p->CamB & ~FilterList_p->AllocatedCamB;

    FreeA = PositionOfFirstSetBit( CamA );  /* one of these will be our FilterIndex */
    FreeB = PositionOfFirstSetBit( CamB );

    STTBX_Print(("  Cam: 0x%08x 0x%08x \n",  FilterList_p->CamA,  FilterList_p->CamB ));
    STTBX_Print(("Alloc: 0x%08x 0x%08x \n",  FilterList_p->AllocatedCamA,  FilterList_p->AllocatedCamB ));
    STTBX_Print((" Free: 0x%08x 0x%08x \n", CamA, CamB ));
    STTBX_Print(("FreeA: %d FreeB: %d \n", FreeA, FreeB));

    if ( (STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSectionParams) & TC_SESSION_INFO_FILTER_TYPE_FIELD) == TC_SESSION_INFO_FILTER_TYPE_SHORT)
    {
        if ( FreeA < 32 )   /* allocate a filter from this CAM... */
        {
            *FilterIndex = FreeA;
             FilterList_p->AllocatedCamA |= (1 << FreeA);
        }
        else if  ( FreeB < 32 )   /* ...or the other if Cam A is full */
        {
            *FilterIndex = FreeB + 32;
             FilterList_p->AllocatedCamB |= (1 << FreeB);
        }
        else
        {
            Error = STPTI_ERROR_NO_FREE_FILTERS;
        }
    }
    else if ( (STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSectionParams) & TC_SESSION_INFO_FILTER_TYPE_FIELD) == TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH)
    {
        assert ( FreeA == FreeB );  /* paranoid check */

        if ( FreeA < 32 )  /* space for this CAM */
        {
            *FilterIndex = FreeA;
             FilterList_p->AllocatedCamA |= (1 << FreeA);   /* 16 bit filters remember! */
             FilterList_p->AllocatedCamB |= (1 << FreeA);
        }
        else
        {
            Error = STPTI_ERROR_NO_FREE_FILTERS;
        }
    }
    else
    {
            Error = STPTI_ERROR_INVALID_FILTER_OPERATING_MODE;
    }

    /* NB: FilterIndex - the absolute index into filter #/bit number for TC & ST20) is used
      for enable/disbale etc, the AllocatedCamA/B is just for housekeeping */

    STTBX_Print(("  New: 0x%08x 0x%08x \n",  FilterList_p->AllocatedCamA,  FilterList_p->AllocatedCamB ));
    STTBX_Print(("Filter #%d, error=%d\n", *FilterIndex, Error ));

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t TcCam_FilterSetSection(FullHandle_t FilterHandle, STPTI_FilterData_t *FilterData_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    U32 FilterIdent = stptiMemGet_Filter(FilterHandle)->TC_FilterIdent;

    TCDevice_t              *TC_Device                = (TCDevice_t *)stptiMemGet_Device(FilterHandle)->TCDeviceAddress_p;
    TCSectionFilterArrays_t *TC_SectionFilterArrays_p = (TCSectionFilterArrays_t *) &TC_Device->TC_SectionFilterArrays;
    TCSessionInfo_t         *TCSessionInfo_p          = stptiHAL_GetSessionFromFullHandle( FilterHandle );
    TCSectionFilter_t       *TC_SectionFilter_p;
    U32 NotMatchData;


    switch ( stptiMemGet_Device(FilterHandle)->SectionFilterOperatingMode )
    {
        case STPTI_FILTER_OPERATING_MODE_8x8:           /* 8 byte filters */
        case STPTI_FILTER_OPERATING_MODE_16x8:
        case STPTI_FILTER_OPERATING_MODE_32x8:
        case STPTI_FILTER_OPERATING_MODE_64x8:
        case STPTI_FILTER_OPERATING_MODE_ANYx8:        
        {
            U32 AlignedFilterDataMS = 0;
            U32 AlignedFilterDataLS = 0;
            U32 AlignedFilterMaskMS = 0;
            U32 AlignedFilterMaskLS = 0;
        
            /* Sort out the data alignment now rather than later to minimise the time spent in an interrupt lock */

            /* Filter Data (8 bytes) */
            AlignedFilterDataMS |= (FilterData_p->FilterBytes_p[0] << 24);
            AlignedFilterDataMS |= (FilterData_p->FilterBytes_p[1] << 16);
            AlignedFilterDataMS |= (FilterData_p->FilterBytes_p[2] << 8);
            AlignedFilterDataMS |= (FilterData_p->FilterBytes_p[3]);

            AlignedFilterDataLS |= (FilterData_p->FilterBytes_p[4] << 24);
            AlignedFilterDataLS |= (FilterData_p->FilterBytes_p[5] << 16);
            AlignedFilterDataLS |= (FilterData_p->FilterBytes_p[6] << 8);
            AlignedFilterDataLS |= (FilterData_p->FilterBytes_p[7]);

            /* Filter Mask (8 bytes) */
            AlignedFilterMaskMS |= (FilterData_p->FilterMasks_p[0] << 24);
            AlignedFilterMaskMS |= (FilterData_p->FilterMasks_p[1] << 16);
            AlignedFilterMaskMS |= (FilterData_p->FilterMasks_p[2] << 8);
            AlignedFilterMaskMS |= (FilterData_p->FilterMasks_p[3]);

            AlignedFilterMaskLS |= (FilterData_p->FilterMasks_p[4] << 24);
            AlignedFilterMaskLS |= (FilterData_p->FilterMasks_p[5] << 16);
            AlignedFilterMaskLS |= (FilterData_p->FilterMasks_p[6] << 8);
            AlignedFilterMaskLS |= (FilterData_p->FilterMasks_p[7]);

#ifdef STPTI_WORKAROUND_SFCAM   /* PMC 17/02/03 Apply S/W CAM=SF arbiter workaround */
            STPTI_CRITICAL_SECTION_BEGIN {

                if (FilterIdent < 32)
                {
                    TCSessionInfo_p->SectionEnables_0_31 &= ~(1 << FilterIdent);            /* Disable Filter */

                    TC_SectionFilter_p = &TC_SectionFilterArrays_p->FilterA[FilterIdent];   /* set up filterA data and masks */

                    /* Set-up NotMatchMode */
                    NotMatchData = (NOT_MATCH_ENABLE | (((AlignedFilterDataMS >> 1) & 0x1f) & NOT_MATCH_DATA_MASK));

                    if (!(FilterData_p->u.SectionFilter.NotMatchMode))
                        NotMatchData = 0;

                    Error = CAMSFArbiter(FilterHandle, AlignedFilterDataLS, AlignedFilterMaskLS, AlignedFilterDataMS, AlignedFilterMaskMS, NotMatchData, TC_SectionFilter_p);
                    if (FilterData_p->InitialStateEnabled)      /* Re-enable filter, if initial state is enabled */
                        TCSessionInfo_p->SectionEnables_0_31 |= (1 << FilterIdent);
                }
                else    /* FilterIdent >= 32 */
                {
                    TCSessionInfo_p->SectionEnables_32_63 &= ~(1 << (FilterIdent - 32));   /* Disable Filter */

                    TC_SectionFilter_p = &TC_SectionFilterArrays_p->FilterB[FilterIdent - 32];  /* set up filterB data and masks */

                    if (FilterData_p->u.SectionFilter.NotMatchMode)
                        Error = STPTI_ERROR_INVALID_FILTER_DATA;

                    /* param=0 As NotMatchMode not allowed on CAM B */
                    Error = CAMSFArbiter(FilterHandle, AlignedFilterDataLS, AlignedFilterMaskLS, AlignedFilterDataMS, AlignedFilterMaskMS, 0, TC_SectionFilter_p);
                    if (FilterData_p->InitialStateEnabled)  /* Re-enable filter, if initial state is enabled */
                        TCSessionInfo_p->SectionEnables_32_63 |= (1 << (FilterIdent - 32));
                }

            } STPTI_CRITICAL_SECTION_END;

#else
            
            STPTI_CRITICAL_SECTION_BEGIN {

                if (FilterIdent < 32)
                {
                    TCSessionInfo_p->SectionEnables_0_31 &= ~(1 << FilterIdent);            /* Disable Filter */

                    TC_SectionFilter_p = &TC_SectionFilterArrays_p->FilterA[FilterIdent];   /* set up filterA data and masks */

                    TC_SectionFilter_p->SFFilterDataLS = AlignedFilterDataLS;
                    TC_SectionFilter_p->SFFilterMaskLS = AlignedFilterMaskLS;
                    TC_SectionFilter_p->SFFilterDataMS = AlignedFilterDataMS;
                    TC_SectionFilter_p->SFFilterMaskMS = AlignedFilterMaskMS;

                    NotMatchData = (AlignedFilterDataMS >> 1);  /* Set-up NotMatchMode */

                    if (FilterData_p->u.SectionFilter.NotMatchMode)
                      TC_SectionFilterArrays_p->NotFilter[FilterIdent] = (NOT_MATCH_ENABLE | (NotMatchData & NOT_MATCH_DATA_MASK));
                    else
                        TC_SectionFilterArrays_p->NotFilter[FilterIdent] = 0;
                    if (FilterData_p->InitialStateEnabled)      /* Re-enable filter, if initial state is enabled */
                        TCSessionInfo_p->SectionEnables_0_31 |= (1 << FilterIdent);
                }
                else    /* FilterIdent >= 32 */
                {
                    TCSessionInfo_p->SectionEnables_32_63 &= ~(1 << (FilterIdent - 32));   /* Disable Filter */

                    TC_SectionFilter_p = &TC_SectionFilterArrays_p->FilterB[FilterIdent - 32];  /* set up filterB data and masks */

                    TC_SectionFilter_p->SFFilterDataLS = AlignedFilterDataLS;
                    TC_SectionFilter_p->SFFilterMaskLS = AlignedFilterMaskLS;
                    TC_SectionFilter_p->SFFilterDataMS = AlignedFilterDataMS;
                    TC_SectionFilter_p->SFFilterMaskMS = AlignedFilterMaskMS;

                    if (FilterData_p->u.SectionFilter.NotMatchMode) /* NotMatchMode not allowed on CAM B */
                        Error = STPTI_ERROR_INVALID_FILTER_DATA;

                    if (FilterData_p->InitialStateEnabled)  /* Re-enable filter, if initial state is enabled */
                        TCSessionInfo_p->SectionEnables_32_63 |= (1 << (FilterIdent - 32));
                }

            } STPTI_CRITICAL_SECTION_END;

#endif
        }
        break;


        case STPTI_FILTER_OPERATING_MODE_8x16:
        case STPTI_FILTER_OPERATING_MODE_16x16:
        case STPTI_FILTER_OPERATING_MODE_32x16: /* 8+8 byte filters (neg match mode ONLY) */
        {
            U32 AlignedFilterADataMS = 0;
            U32 AlignedFilterADataLS = 0;
            U32 AlignedFilterBDataMS = 0;
            U32 AlignedFilterBDataLS = 0;
            U32 AlignedFilterAMaskMS = 0;
            U32 AlignedFilterAMaskLS = 0;
            U32 AlignedFilterBMaskMS = 0;
            U32 AlignedFilterBMaskLS = 0;


            /* Filter Data */
            AlignedFilterADataMS |= (FilterData_p->FilterBytes_p[0] << 24);
            AlignedFilterADataMS |= (FilterData_p->FilterBytes_p[1] << 16);
            AlignedFilterADataMS |= (FilterData_p->FilterBytes_p[2] << 8);
            AlignedFilterADataMS |= (FilterData_p->FilterBytes_p[3]);

            AlignedFilterADataLS |= (FilterData_p->FilterBytes_p[4] << 24);
            AlignedFilterADataLS |= (FilterData_p->FilterBytes_p[5] << 16);
            AlignedFilterADataLS |= (FilterData_p->FilterBytes_p[6] << 8);
            AlignedFilterADataLS |= (FilterData_p->FilterBytes_p[7]);

            switch( FilterData_p->FilterType )
            {
                case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE:
                    AlignedFilterBDataMS = AlignedFilterADataMS;
                    AlignedFilterBDataLS = AlignedFilterADataLS;
                    break;

                default:
                    Error = STPTI_ERROR_INVALID_FILTER_TYPE;
                    break;
            }


            /* Filter Mask */

            if (Error == ST_NO_ERROR)   /* it is STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE */
            {
                U8 *ModePattern_p = FilterData_p->u.SectionFilter.ModePattern_p;

                if( ModePattern_p == NULL )
                {
                    Error = STPTI_ERROR_INVALID_FILTER_DATA;
                    break;
                }    

                AlignedFilterAMaskMS |= ((FilterData_p->FilterMasks_p[0] & ModePattern_p[0]) << 24);
                AlignedFilterAMaskMS |= ((FilterData_p->FilterMasks_p[1] & ModePattern_p[1]) << 16);
                AlignedFilterAMaskMS |= ((FilterData_p->FilterMasks_p[2] & ModePattern_p[2]) << 8);
                AlignedFilterAMaskMS |= ((FilterData_p->FilterMasks_p[3] & ModePattern_p[3]));

                AlignedFilterAMaskLS |= ((FilterData_p->FilterMasks_p[4] & ModePattern_p[4]) << 24);
                AlignedFilterAMaskLS |= ((FilterData_p->FilterMasks_p[5] & ModePattern_p[5]) << 16);
                AlignedFilterAMaskLS |= ((FilterData_p->FilterMasks_p[6] & ModePattern_p[6]) << 8);
                AlignedFilterAMaskLS |= ((FilterData_p->FilterMasks_p[7] & ModePattern_p[7]));

                AlignedFilterBMaskMS |= ((FilterData_p->FilterMasks_p[0] & ~ModePattern_p[0]) << 24);
                AlignedFilterBMaskMS |= ((FilterData_p->FilterMasks_p[1] & ~ModePattern_p[1]) << 16);
                AlignedFilterBMaskMS |= ((FilterData_p->FilterMasks_p[2] & ~ModePattern_p[2]) << 8);
                AlignedFilterBMaskMS |= ((FilterData_p->FilterMasks_p[3] & ~ModePattern_p[3]));

                AlignedFilterBMaskLS |= ((FilterData_p->FilterMasks_p[4] & ~ModePattern_p[4]) << 24);
                AlignedFilterBMaskLS |= ((FilterData_p->FilterMasks_p[5] & ~ModePattern_p[5]) << 16);
                AlignedFilterBMaskLS |= ((FilterData_p->FilterMasks_p[6] & ~ModePattern_p[6]) << 8);
                AlignedFilterBMaskLS |= ((FilterData_p->FilterMasks_p[7] & ~ModePattern_p[7]));

                /* Now, if the Mask for CAM B ends up as zero then we should disable the CAM B filter,
                   This is 'almost' impossible from the ST 20 but we can set the filter to look for a
                   Table ID ( first byte of section filter ) of 0xFF which can never exist, so the
                   filter will never match. Thisis equivalent to disabling it. While we are setting the
                   first byte to 0xff we may as well set them ALL to this value */

                if ((AlignedFilterBMaskLS == 0) && (AlignedFilterBMaskMS == 0))
                {
                    AlignedFilterBMaskLS = 0xffffffff;
                    AlignedFilterBMaskMS = 0xffffffff;

                    AlignedFilterBDataLS = 0xffffffff;
                    AlignedFilterBDataMS = 0xffffffff;
                }

#ifdef STPTI_WORKAROUND_SFCAM   /* PMC 17/02/03 Apply S/W CAM=SF arbiter workaround */
                STPTI_CRITICAL_SECTION_BEGIN {

                    TCSessionInfo_p->SectionEnables_0_31 &= ~(1 << FilterIdent);    /* Disable Filter */

                    TC_SectionFilter_p = &TC_SectionFilterArrays_p->FilterA[FilterIdent];   /* set up filterA data and masks */

                    /* Set-up NotMatchMode */
                    NotMatchData = (NOT_MATCH_ENABLE | (((AlignedFilterADataMS >> 1) & 0x1f) & NOT_MATCH_DATA_MASK));

                    if (!(FilterData_p->u.SectionFilter.NotMatchMode))
                        NotMatchData = 0;

                    Error = CAMSFArbiter(FilterHandle, AlignedFilterADataLS, AlignedFilterAMaskLS, AlignedFilterADataMS, AlignedFilterAMaskMS, NotMatchData, TC_SectionFilter_p);
                    TC_SectionFilter_p = &TC_SectionFilterArrays_p->FilterB[FilterIdent];   /* set up filterB data and masks */


                    Error = CAMSFArbiter(FilterHandle, AlignedFilterBDataLS, AlignedFilterBMaskLS, AlignedFilterBDataMS, AlignedFilterBMaskMS, 0,  TC_SectionFilter_p);

                    if (FilterData_p->InitialStateEnabled)  /* Re-enable filter, if initial state is enabled */
                        TCSessionInfo_p->SectionEnables_0_31 |= (1 << FilterIdent);

                } STPTI_CRITICAL_SECTION_END;
#else
                STPTI_CRITICAL_SECTION_BEGIN {

                    TCSessionInfo_p->SectionEnables_0_31 &= ~(1 << FilterIdent);    /* Disable Filter */

                    TC_SectionFilter_p = &TC_SectionFilterArrays_p->FilterA[FilterIdent];   /* set up filterA data and masks */

                    TC_SectionFilter_p->SFFilterDataLS = AlignedFilterADataLS;
                    TC_SectionFilter_p->SFFilterMaskLS = AlignedFilterAMaskLS;
                    TC_SectionFilter_p->SFFilterDataMS = AlignedFilterADataMS;
                    TC_SectionFilter_p->SFFilterMaskMS = AlignedFilterAMaskMS;
                    TC_SectionFilter_p = &TC_SectionFilterArrays_p->FilterB[FilterIdent];   /* set up filterB data and masks */

                    TC_SectionFilter_p->SFFilterDataLS = AlignedFilterBDataLS;
                    TC_SectionFilter_p->SFFilterMaskLS = AlignedFilterBMaskLS;
                    TC_SectionFilter_p->SFFilterDataMS = AlignedFilterBDataMS;
                    TC_SectionFilter_p->SFFilterMaskMS = AlignedFilterBMaskMS;

                    NotMatchData = (AlignedFilterADataMS >> 1); /* Set-up NotMatchMode */

                    if (FilterData_p->u.SectionFilter.NotMatchMode)
                        TC_SectionFilterArrays_p->NotFilter[FilterIdent] = (NOT_MATCH_ENABLE | (NotMatchData & NOT_MATCH_DATA_MASK));
                    else
                        TC_SectionFilterArrays_p->NotFilter[FilterIdent] = 0;

                    if (FilterData_p->InitialStateEnabled)  /* Re-enable filter, if initial state is enabled */
                        TCSessionInfo_p->SectionEnables_0_31 |= (1 << FilterIdent);

                } STPTI_CRITICAL_SECTION_END;
#endif
                        
                        
            }   /* (Error == ST_NO_ERROR) */
        }
        break;

    default:
        Error = STPTI_ERROR_INVALID_FILTER_OPERATING_MODE;
        break;
    }

    return Error;
}


/* ------------------------------------------------------------------------- */
/*                            Local Functions                                */
/* ------------------------------------------------------------------------- */


static ST_ErrorCode_t AllocateFilterMasksFromFreeList( FilterList_t *FreeFilterList_p, U32 *CamA, U32 *CamB, int FilterMode, int NumFilters )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 bitmask_a, bitmask_b, acc_bitmask_a = 0, acc_bitmask_b = 0;
    int a, countto;


    STTBX_Print(("\n *** GetAllocatableFilterMasks ***\n"));

    countto = NumFilters;

    if ( FilterMode == TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH )
    {
        STTBX_Print(("NMM filter: %dx16\n", NumFilters >> 1 ));
    }
    else if ( FilterMode == TC_SESSION_INFO_FILTER_TYPE_SHORT )
    {
        STTBX_Print(("short filter: %dx8\n",NumFilters ));
        countto = (countto >> 1); /* divide by two */
    }
    else
    {
        STTBX_Print(("Incorrect filter mode '0x%08x'\n", FilterMode ));
        return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
    }

    /* beacuse the allocation is done in equal blocks between CAMs A & B we can get away
    with GetFirstSetBit() doing the right thing. */

    for( a = 0; a < countto; a++)
    {
        bitmask_a = GetFirstSetBit( FreeFilterList_p->CamA );   /* allocate the bits from each CAM */
        bitmask_b = GetFirstSetBit( FreeFilterList_p->CamB );

        if (( bitmask_a == 0 ) || ( bitmask_b == 0 ) || ( bitmask_a != bitmask_b ) )
        {
            STTBX_Print(("STPTI_ERROR_NO_FREE_FILTERS!\n"));
            return( STPTI_ERROR_NO_FREE_FILTERS );
        }

        acc_bitmask_a |= bitmask_a;         /* add the bits to the grand total we need to allocate */
        acc_bitmask_b |= bitmask_b;

        FreeFilterList_p->CamA = FreeFilterList_p->CamA & ~bitmask_a;    /* mark filters as taken (clear bit) */
        FreeFilterList_p->CamB = FreeFilterList_p->CamB & ~bitmask_b;
    }

    *CamA = acc_bitmask_a;  /* return what can be allocated */
    *CamB = acc_bitmask_b;

    STTBX_Print((" allocA: 0x%08x allocB: 0x%08x\n", acc_bitmask_a, acc_bitmask_b ));

    return( Error );

}


/* ------------------------------------------------------------------------- */


static U32 GetFirstSetBit( U32 Value )
{
    U32 a;

    for (a = 1; a != 0; a = (a << 1))
    {
        if ( a & Value )
        {
            return( a );
        }
    }

    return( 0 );
}


/* ------------------------------------------------------------------------- */


static U32 PositionOfFirstSetBit( U32 Value )   /* returns 0..31 or a value of 32 which indicates no bits set */
{
    U32 a, position = 0;

    for (a = 1; a != 0; a = (a << 1))
    {
        if ( a & Value )
            break;

        position++;
    }

    return( position );
}


/* ------------------------------------------------------------------------- */


static int NumberOfFreeFilters( FilterList_t *FilterList_p, int FilterMode )
{
    int freefilters = 0;

    /* if we are in short mode then count CAMs A & B else if neg match then we AND
     CAM A & B together so that only set bits in both CAMs at the _same_ bit
     position will be indicated as being free. */

    STTBX_Print(("\n *** NumberOfFreeFilters ***\n"));

    STTBX_Print(("CamA: 0x%08x CamB: 0x%08x\n", FilterList_p->CamA, FilterList_p->CamB ));

    if ( FilterMode == TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH )
    {
        freefilters = NumberOfSetBitsInWord( FilterList_p->CamA & FilterList_p->CamB );
    }
    else if (( FilterMode == TC_SESSION_INFO_FILTER_TYPE_SHORT ) || ( FilterMode == TC_SESSION_INFO_FILTER_TYPE_FIELD ))
    {
        freefilters = NumberOfSetBitsInWord( FilterList_p->CamA ) + NumberOfSetBitsInWord( FilterList_p->CamB );
    }
    else
    {
        STTBX_Print(("Incorrect filter mode '0x%08x'\n", FilterMode ));
    }

    return( freefilters );
}


/* ------------------------------------------------------------------------- */


static int NumberOfSetBitsInWord( U32 Value )
{
    U32 a, count = 0;

    for (a = 1; a != 0; a = (a << 1))
    {
        if ( a & Value ) count++;
    }

    return( count );
}


/* ------------------------------------------------------------------------- */


/* nb: cam should be initalized while TC is in reset or paused */

static void ClearAllHardwareCams( TCDevice_t *TC_Device )
{
    TCSectionFilterArrays_t *TC_SectionFilterArrays_p = (TCSectionFilterArrays_t *) &TC_Device->TC_SectionFilterArrays;
    int i;


    STTBX_Print(("TcCam_ClearAllFilters()..."));

    for (i = 0; i < TC_NUMBER_OF_HARDWARE_SECTION_FILTERS; ++i)
    {
        TCSectionFilter_t *TC_SectionFilter = &TC_SectionFilterArrays_p->FilterA[i];
        STSYS_WriteRegDev32LE( (void*)(&TC_SectionFilter->SFFilterDataLS), 0 );
        STSYS_WriteRegDev32LE( (void*)(&TC_SectionFilter->SFFilterMaskLS), 0 );
        STSYS_WriteRegDev32LE( (void*)(&TC_SectionFilter->SFFilterDataMS), 0 );
        STSYS_WriteRegDev32LE( (void*)(&TC_SectionFilter->SFFilterMaskMS), 0 );
    }

    for (i = 0; i < TC_NUMBER_OF_HARDWARE_SECTION_FILTERS; ++i)
    {
        TCSectionFilter_t *TC_SectionFilter = &TC_SectionFilterArrays_p->FilterB[i];
        STSYS_WriteRegDev32LE( (void*)(&TC_SectionFilter->SFFilterDataLS), 0 );
        STSYS_WriteRegDev32LE( (void*)(&TC_SectionFilter->SFFilterMaskLS), 0 );
        STSYS_WriteRegDev32LE( (void*)(&TC_SectionFilter->SFFilterDataMS), 0 );
        STSYS_WriteRegDev32LE( (void*)(&TC_SectionFilter->SFFilterMaskMS), 0 );
    }

    for (i = 0; i < TC_NUMBER_OF_HARDWARE_SECTION_FILTERS; ++i)
    {
        STSYS_WriteRegDev32LE( (void*)(&TC_SectionFilterArrays_p->NotFilter[i]), 0);
    }

    STTBX_Print(("ok\n"));
}


/* ------------------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
static void DumpFilterList( TCPrivateData_t *PrivateData_p, STPTI_TCParameters_t *TC_Params_p )
{
    int a;

    for( a = 0; a < TC_Params_p->TC_NumberOfSessions; a++ )
    {
        STTBX_Print(("index %d\n", a));
        STTBX_Print(("CamA: 0x%08x\n",  PrivateData_p->FilterList[a].CamA));
        STTBX_Print(("CamB: 0x%08x\n",  PrivateData_p->FilterList[a].CamB));
        STTBX_Print(("AllocCamA: 0x%08x\n",  PrivateData_p->FilterList[a].AllocatedCamA));
        STTBX_Print(("AllocCamB: 0x%08x\n",  PrivateData_p->FilterList[a].AllocatedCamB));
        STTBX_Print(("  State: " ));

        switch( PrivateData_p->FilterList[a].State )
        {
            case FL_EMPTY:
                STTBX_Print(("EMPTY\n"));
                break;

            case FL_IN_USE:
                STTBX_Print(("IN_USE\n"));
                break;

            case FL_FREE:
                STTBX_Print(("FREE\n"));
                break;

            default:
                STTBX_Print(("???\n"));
                break;

        }
    }
}
#endif


/* ------------------------------------------------------------------------- */


#ifdef STPTI_WORKAROUND_SFCAM

/* Addresses used to connect to TS_MUX device */

#if defined (ST_5528)
#define STPTI_TSMUX_SWTS_REGISTER TSMERGE_BASE_ADDRESS + 0x280
#define STPTI_TSMUX_SWTS_CONFIG   TSMERGE_BASE_ADDRESS + 0x288
#define STPTI_SWTS_REQUEST        0x80000000
#else
#if defined (ST_7710)
#define STPTI_TSMUX_SWTS_REGISTER TSMERGE2_BASE_ADDRESS + 0x280
#define STPTI_TSMUX_SWTS_CONFIG   TSMERGE_BASE_ADDRESS + 0x2e0
#define STPTI_SWTS_REQUEST        0x80000000
#endif
#endif
/*
#define STPTI_TSMUX_SWTS_REGISTER 0x20400028
#define STPTI_TSMUX_SWTS_CONFIG   0x20400030
#define STPTI_SWTS_REQUEST        0x400
*/

#if defined( ST_OS20 )
    #define ProcClockHigh(_value_)     __optasm{ldc 0; ldclock; st _value_;}
#else
#if defined( ST_OS21 )   || defined(ST_UCOS_SYSTEM)
    #define ProcClockHigh(_value_)     _value_ = time_now()
#endif
#endif

/******************************************************************************
Function Name : CAMSFArbiter
  Description : Need to replace the H/W CAM/TC arbitor with a nice s/w version
                due to bug in PTI3 cell.
   Parameters :
         NOTE : PMC 17/02/03 Required for GNBvd18811
******************************************************************************/

static ST_ErrorCode_t CAMSFArbiter( FullHandle_t FilterHandle, U32 CAM_0, U32 CAM_1, U32 CAM_2, U32 CAM_3, U32 NotMatchData, TCSectionFilter_t *TC_SectionFilter_p )
{
    ST_ErrorCode_t            CAM_status = STPTI_ERROR_UNABLE_TO_ENABLE_FILTERS;
    U32                       Test=0,Count=0, Value=0, Start=0, Now=0;
    U16                       Mask;

    TCDevice_t      *TC_Device     = (TCDevice_t *)  stptiMemGet_Device(FilterHandle)->TCDeviceAddress_p;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FilterHandle);
    U32              FilterIdent   = stptiMemGet_Filter(FilterHandle)->TC_FilterIdent;

    STPTI_TCParameters_t    *TC_Params_p              =    (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSectionFilterArrays_t *TC_SectionFilterArrays_p = (TCSectionFilterArrays_t *) &TC_Device->TC_SectionFilterArrays;
    TCGlobalInfo_t          *TC_Global_p              =          (TCGlobalInfo_t *)  TC_Params_p->TC_GlobalDataStart;


    /*  Make request to write CAM registers */
    Mask = STSYS_ReadRegDev16LE((void*)&(((TCGlobalInfo_t *)(TC_Params_p->TC_GlobalDataStart))->GlobalCAMArbiterInhibit));
    Mask |= TC_GLOBAL_DATA_CAM_ARBITER_INHIBIT_REQUEST;
    STSYS_WriteTCReg16LE((void*)&(((TCGlobalInfo_t *)(TC_Params_p->TC_GlobalDataStart))->GlobalCAMArbiterInhibit), Mask);
      /* Need to delay for a few clock cycles.  This is to ensure the
        following scenario is avoided:

         1. TC reads inhibit as being clear.

         2. The ST20 sets the inhibit and immediately reads the idle value
         and determines the TC is idle.

         3. In the meantime, the TC, having determined that it is not inhibited,
         exits the idle loop and could process a partial packet just in time for
         the ST20 to write CAM and lock itself up. */

      /* Time for TC to ld inhibit and st BUSY flag = (5 * STBUS cycle time). Assume
         STBUS = CPU / 2 and wait 12 CPU cycles */
        /* TN - Added extra delay to allow for Pipeline stalls (TC takes about 10 * STBUS cycles
            instead of 5) and extra pipeline stall introduced when bit RMW used for IDLE instead
            of Word access */

    __ASM_12_NOPS;
    __ASM_12_NOPS;

    /* Poll for idle to be free. Return if free or timed-out
      If not free in TWO packet periods, then drop out */

    ProcClockHigh( Start );

    do
    {
        /* Check if CAM available now if so, then break  */
        TCGlobalInfo_t *TC_Global_p = (TCGlobalInfo_t *) TC_Params_p->TC_GlobalDataStart;
        Value = STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_DATA_CAM_ARBITER_IDLE_CLEAR;
        if (Value == (U32) TC_GLOBAL_DATA_CAM_ARBITER_IDLE_CLEAR)
        {
            CAM_status = ST_NO_ERROR;
            break;
        }

        /* Delay a few no-ops to give TC time to process */
        __ASM_12_NOPS;

        /* Else, get current time and see if packet time * 2 elapsed
        Wait 2 packets max to be *very* sure its locked */
        ProcClockHigh( Now );
    }
    while( (Now - Start) < (TC_ONE_PACKET_TIME * 2));

    if (CAM_status == STPTI_ERROR_UNABLE_TO_ENABLE_FILTERS) /* If timed out, then flush with SWTS before continuing */
    {
        /* Section filter is hung with partial packet. Send one word to the SWTS and see if that does it. */

        for (Count=0; Count < (DVB_TS_PACKET_LENGTH/4); Count++)
        {
            *((STPTI_DevicePtr_t )STPTI_TSMUX_SWTS_REGISTER) =  0xffffffff; /* Send 0xFFFFFFFF value (actual value irrelevent) */

            do  /* Check for the SWTS fifo to become empty */
            {
              Test = ((*(STPTI_DevicePtr_t )STPTI_TSMUX_SWTS_CONFIG) & STPTI_SWTS_REQUEST);
            }
            while (Test == 0);
            Value = STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_DATA_CAM_ARBITER_IDLE_CLEAR;
            if (Value == ((U32) TC_GLOBAL_DATA_CAM_ARBITER_IDLE_CLEAR)) /* If Value clear, then SWTS write successful - so break */
            {
              CAM_status = ST_NO_ERROR;
              break;
            }
        }
    }

    if (CAM_status == ST_NO_ERROR)  /* Actually set the cam registers */
    {
      TC_SectionFilter_p->SFFilterDataLS = CAM_0;
      TC_SectionFilter_p->SFFilterMaskLS = CAM_1;
      TC_SectionFilter_p->SFFilterDataMS = CAM_2;
      TC_SectionFilter_p->SFFilterMaskMS = CAM_3;

      TC_SectionFilterArrays_p->NotFilter[FilterIdent] = NotMatchData;
    }

    /*  Clear request to write CAM registers */
    Mask = STSYS_ReadRegDev16LE((void*)&(((TCGlobalInfo_t *)(TC_Params_p->TC_GlobalDataStart))->GlobalCAMArbiterInhibit));
    Mask &= ~TC_GLOBAL_DATA_CAM_ARBITER_INHIBIT_REQUEST;
    STSYS_WriteTCReg16LE((void*)&(((TCGlobalInfo_t *)(TC_Params_p->TC_GlobalDataStart))->GlobalCAMArbiterInhibit), Mask);
    
    return( CAM_status );
}
#endif


/* EOF  -------------------------------------------------------------------- */
