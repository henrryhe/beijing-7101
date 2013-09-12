/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: ramcam.c
 Description: Provides HAL interface to transport controller CAM

  Note that at this time a maximum of 48 filters per session are supported
 due to hardware limitations - metadata on first 48 filter matches only, although
 it can filter the maximum supported by the hardware.

  Note that like StdCam, RamCam TC_FilterIdent is an absolute value (0..max_filters)
 and refers to a specific CAM block & filter position (or at least until full multisession)

    Association/Enables mapping wrt SF_OPMASK
    =========================================

    NMM

    SectionAssociation0 CAM [A+B]0
    SectionAssociation1 CAM [A+B]1

    SectionAssociation2 CAM [A+B]2
    SectionAssociation3 CAM [A+B]3 (unused) (16+16+16=48 bits)

    SectionAssociation1.SectionAssociation0 => SectionEnables_0_31
    SectionAssociation3.SectionAssociation2 => SectionEnables_32_63


    SHORT

    SectionAssociation0 CAM A0
    SectionAssociation1 CAM A1 (top 8 bits unused) (16+8=24 bits)

    SectionAssociation2 CAM B0
    SectionAssociation3 CAM B1 (top 8 bits unused) (16+8=24 bits)

                                                   (24+24=48 bits)

    SectionAssociation1.SectionAssociation0 => SectionEnables_0_31
    SectionAssociation3.SectionAssociation2 => SectionEnables_32_63

******************************************************************************/
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
/* #define VIRTUAL_PTI_RAMCAM_DEBUG */
#endif

/* Includes ---------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>
#endif /* #endif !defined(ST_OSLINUX)  */
#include "stddefs.h"
#include "stdevice.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX)  */
#include "stpti.h"
#include "osx.h"
#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"
#include "memget.h"

#include "pti4.h"
#include "tchal.h"


/* ------------------------------------------------------------------------- */

/* MAGIC numbers for the CAM_CFG register */
#define CAM_CFG_8_FILTERS       0x0000  /* bits 5:3 (number of filters) */
#define CAM_CFG_16_FILTERS      0x0008
#define CAM_CFG_32_FILTERS      0x0010
#define CAM_CFG_64_FILTERS      0x0018
#define CAM_CFG_96_FILTERS      0x0020

#define INVALID_FILTER_INDEX ((U32)0xffffffff)

#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
#define BYTES_OF_METADATA_TC_OUTPUTS 16
#else
#define BYTES_OF_METADATA_TC_OUTPUTS 12
#endif

/* ------------------------------------------------------------------------- */

#define ENDIAN_U32_SWAP(value) (U32)( (((value)&0xFF000000)>>24)|(((value)&0x00FF0000)>>8)| \
                                      (((value)&0x0000FF00)<<8)|(((value)&0x000000FF)<<24) )

/* ------------------------------------------------------------------------- */


typedef enum FilterListState_e
{

    FL_IN_USE,  /* the area is in active use by a session */
    FL_FREE     /* the area is valid but not in use */
} FilterListState_t;


typedef struct SessionFilterList_s
{
    U8                FirstBlock;   /* a block is 4 8x8filters on 2 cams (A&B) i.e. 8 filters/block */
    U8                LastBlock;
    U8                FirstTCFilter;
    U8                LastTCFilter;
    FilterListState_t State;        /* allocation state for the WHOLE item */
} SessionFilterList_t;


typedef struct FilterList_s
{
    FilterListState_t       CamA[ SF_NUM_BLOCKS_PER_CAM ][4];   /* usage tracking: [blocks][filters] */
    FilterListState_t       CamB[ SF_NUM_BLOCKS_PER_CAM ][4];
    TCSectionFilterArrays_t ShadowSectionFilterArray;           /* save doing a rd-modify-wr cycle to TC CAMs */
    SessionFilterList_t    *SessionFilterList;                  /* block allocations per session */
    U16                     FilterMode[ (SF_BYTES_PER_CAM*2)/SF_FILTER_LENGTH/2 ];
} FilterList_t;



/* session.c  - does this really need to be declared this way?  JCC */
extern TCSessionInfo_t *stptiHAL_GetSessionFromFullHandle(FullHandle_t DeviceHandle);

static void ClearAllCams( TCSectionFilterArrays_t *SectionFilterArray_p );
static U32 AllocateFilter(FilterList_t *FilterList, SessionFilterList_t *SessionFilter, U16 TC_FilterMode);
static U16 ConvertFilterType(STPTI_FilterType_t FilterType);
static U32 ConvertNormalizedFilterIndexToAssociateBitmask( U32 NormalizedFilterIndex, BOOL IsCamA);
static ST_ErrorCode_t SetFilterAssociation( FullHandle_t DeviceHandle, TCSectionFilterInfo_t *TCSectionFilterInfo_p, U32 FilterIndex, BOOL DoAssociation );
static ST_ErrorCode_t WriteBlock(STPTI_TCParameters_t *TC_Params_p, U32 FilterIdent, TCCamIndex_t *ShadowIndex, TCCamIndex_t *TCCamIndex, TCSectionFilterArrays_t *TC_SectionFilterArrays_p, U32 NotMatchData);

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
static void DumpFilterList( TCPrivateData_t *PrivateData_p, STPTI_TCParameters_t *TC_Params_p );
static void DumpBlock( TCCamIndex_t *TCCamIndex_p );
#endif


/******************************************************************************
Function Name : TcCam_Initialize
  Description : Called when the first (virtual) PTI is init'ed (STPTI_Init())
                on a Physical PTI, before TcCam_FilterAllocateSession() below.
   Parameters : Device Handle of the virtual PTI
******************************************************************************/
ST_ErrorCode_t TcCam_Initialize( FullHandle_t DeviceHandle )
{
    Device_t                *Device_p      = stptiMemGet_Device(DeviceHandle);
    TCDevice_t              *TC_Device     = (TCDevice_t *)Device_p->TCDeviceAddress_p;
    TCPrivateData_t         *PrivateData_p = &Device_p->TCPrivateData;
    ST_Partition_t          *Partition_p   =  NULL;
    STPTI_TCParameters_t    *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSectionFilterArrays_t *TC_SectionFilterArrays_p = (TCSectionFilterArrays_t *) &TC_Device->TC_SectionFilterArrays;
    FilterList_t            *FilterList;
    SessionFilterList_t     *SessionFilterList;
    int session, block;

    STTBX_Print(("*** TcCam_Init (RAMCAM) ***\n"));

#if !defined(ST_OSLINUX)
    Partition_p   =  Device_p->DriverPartition_p;
#endif /* #endif !defined(ST_OSLINUX) */

    /* Prevent compiler warning with memory_allocate() for platforms which ignore the first parameter */
    Partition_p = Partition_p;

    ClearAllCams( TC_SectionFilterArrays_p );

    STTBX_Print(("TcCam_Init: allocating memory..."));

    PrivateData_p->FilterList = memory_allocate( Partition_p, sizeof( FilterList_t ) );
    if ( PrivateData_p->FilterList == NULL )
    {
        STTBX_Print(("FAIL!\n"));
        return( ST_ERROR_NO_MEMORY );
    }

    PrivateData_p->FilterList->SessionFilterList = memory_allocate( Partition_p, sizeof( SessionFilterList_t ) * TC_Params_p->TC_NumberOfSessions );
    if ( PrivateData_p->FilterList->SessionFilterList == NULL )
    {
        STTBX_Print(("FAIL!\n"));
        memory_deallocate( Partition_p, PrivateData_p->FilterList );
        return( ST_ERROR_NO_MEMORY );
    }
    STTBX_Print(("ok\n"));

    STTBX_Print(("TcCam_Init: reset shadow copy of CAMs..."));
    ClearAllCams( &PrivateData_p->FilterList->ShadowSectionFilterArray );
    STTBX_Print(("ok\n"));

    STTBX_Print(("TcCam_Init: reset usage tracking lists..."));

    FilterList = PrivateData_p->FilterList;

    for( block = 0; block < SF_NUM_BLOCKS_PER_CAM; block++)
    {
        FilterList->CamA[block][0] = FL_FREE;
        FilterList->CamA[block][1] = FL_FREE;
        FilterList->CamA[block][2] = FL_FREE;
        FilterList->CamA[block][3] = FL_FREE;

        FilterList->CamB[block][0] = FL_FREE;
        FilterList->CamB[block][1] = FL_FREE;
        FilterList->CamB[block][2] = FL_FREE;
        FilterList->CamB[block][3] = FL_FREE;
    }

    SessionFilterList = FilterList->SessionFilterList;

    for( session = 0; session < TC_Params_p->TC_NumberOfSessions; session++)
    {
        SessionFilterList[session].FirstBlock    = SF_NUM_BLOCKS_PER_CAM;                  /* set to an invalid value */
        SessionFilterList[session].LastBlock     = SF_NUM_BLOCKS_PER_CAM;                  /* set to an invalid value */
        SessionFilterList[session].FirstTCFilter = TC_Params_p->TC_NumberSectionFilters;   /* set to an invalid value */
        SessionFilterList[session].LastTCFilter  = TC_Params_p->TC_NumberSectionFilters;   /* set to an invalid value */
        SessionFilterList[session].State         = FL_FREE;
    }

    STTBX_Print(("%d filters...", TC_Params_p->TC_NumberSectionFilters ));
    STTBX_Print(("ok, %d 8x8 blocks per CAM.\n", SF_NUM_BLOCKS_PER_CAM));

    return( ST_NO_ERROR );
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : TcCam_Terminate
  Description : Called when the last Vitual PTI is term'ed (STPTI_term()) on a
                Physical PTI, after TcCam_FilterFreeSession() below.
   Parameters : Device Handle of the virtual PTI
******************************************************************************/
ST_ErrorCode_t TcCam_Terminate( FullHandle_t DeviceHandle )
{
    Device_t             *Device        = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t      *PrivateData_p = &Device->TCPrivateData;
    ST_Partition_t       *Partition_p   =  NULL;


    STTBX_Print(("--- TcCam_Terminate() ---\n"));

#if !defined(ST_OSLINUX)
    Partition_p   =  Device->DriverPartition_p;
#endif /* #if !defined(ST_OSLINUX) */

    /* Prevent compiler warning with memory_allocate() for platforms which ignore the first parameter */
    Partition_p = Partition_p;

    if ( PrivateData_p->FilterList != NULL )    /* trust that this is the last session! */
    {
        if ( PrivateData_p->FilterList->SessionFilterList != NULL )
        {
            memory_deallocate( Partition_p, PrivateData_p->FilterList->SessionFilterList );
        }

        memory_deallocate( Partition_p, PrivateData_p->FilterList );

        PrivateData_p->FilterList = NULL;
    }

    return( ST_NO_ERROR );
}

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/* -------------------------------------------------------------------------

    TcCam_FilterAllocateSession() & TcCam_FilterFreeSession()

    SessionFilterList is used to store information about where in the section
    filter CAMs the filter data & masks are...
    SessionFilterList[session].FirstBlock    (first filter CAM block for session)
    SessionFilterList[session].LastBlock      (last filter CAM block for session)
    SessionFilterList[session].FirstTCFilter    (first tc filter index for session)
    SessionFilterList[session].LastTCFilter      (last tc filter index for session)
    SessionFilterList[session].State = FL_FREE, FL_IN_USE

    There used to be a direct relationship between filter CAM block and the
    filter index used in the tc code.  This required moving filters around,
    however we shouldn't move sessions around as other (virtual) PTIs may be in
    use at the time.  Previous implementations moved sessions around, and this
    also had the the advantages of resolving fragmentation issues, but now we
    have to live with it!

   ------------------------------------------------------------------------- */


/******************************************************************************
Function Name : TcCam_FilterAllocateSession
  Description : Called when the a virtual PTI is init'ed (STPTI_Init())
   Parameters : Device Handle of the virtual PTI
******************************************************************************/
ST_ErrorCode_t TcCam_FilterAllocateSession( FullHandle_t DeviceHandle )
{
    Device_t             *Device_p                   = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t      *PrivateData_p              = &Device_p->TCPrivateData;
    TCSessionInfo_t      *TCSessionInfo_p            = stptiHAL_GetSessionFromFullHandle( DeviceHandle );
    FilterList_t         *FilterList                 = PrivateData_p->FilterList;
    STPTI_TCParameters_t *TC_Params_p                = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    SessionFilterList_t  *SessionFilterList          = FilterList->SessionFilterList;

    TCDevice_t              *TC_Device               = (TCDevice_t *)Device_p->TCDeviceAddress_p;
    TCSectionFilterArrays_t *TCSectionFilterArrays_p = (TCSectionFilterArrays_t *) &TC_Device->TC_SectionFilterArrays;

    U8 FirstBlock, LastBlock, FirstTCFilter=0, LastTCFilter=0;
    U8 numfilters;
    int session, numblocks, block, j, filtersperblock=SF_NUM_FILTERS_PER_BLOCK;
    U16 cam_Config, cam_first_filter;
    U32 BaseAddress, Offset;

    U8 BlockIdent;
    BOOL collision;

    STTBX_Print(("\n *** TcCam_FilterAllocateSession ***\n"));

    STTBX_Print(("TCSessionInfo_p = 0x%08x\n", (U32)TCSessionInfo_p ));
    STTBX_Print(("SectionFilterOperatingMode: 0x%08x\n", (U32)Device_p->SectionFilterOperatingMode ));

    /* Get the number of filters */
    numfilters = Device_p->NumberOfSectionFilters;

    {
        switch( Device_p->SectionFilterOperatingMode )
        {
            /* short (8 byte) filters */
            /* also used for DTV APG / MPT - if PTI4SL */
            case STPTI_FILTER_OPERATING_MODE_8x8:
            case STPTI_FILTER_OPERATING_MODE_16x8:
            case STPTI_FILTER_OPERATING_MODE_32x8:
            case STPTI_FILTER_OPERATING_MODE_48x8:
            case STPTI_FILTER_OPERATING_MODE_64x8:
            case STPTI_FILTER_OPERATING_MODE_ANYx8:
                STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_FILTER_TYPE_SHORT );
                filtersperblock=SF_NUM_FILTERS_PER_BLOCK;
                break;

            /* long (16 byte) filters */
            /* neg match mode (8+8 byte) filters for DVB */
            /* neg match mode (8+8 byte) filters for DTV APG - if not PTI4SL */
            /* neg match mode (8+8 byte) filters for DTV MPT (but we ignore the Negative Match part) - if not PTI4SL */
            case STPTI_FILTER_OPERATING_MODE_8x16:
            case STPTI_FILTER_OPERATING_MODE_16x16:
            case STPTI_FILTER_OPERATING_MODE_32x16:
            case STPTI_FILTER_OPERATING_MODE_48x16:
            case STPTI_FILTER_OPERATING_MODE_64x16:
                STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH );
                /* SF_NUM_FILTER_PER_BLOCK is in terms of 8byte short filters, we only have half that for 16byte modes */
                filtersperblock=SF_NUM_FILTERS_PER_BLOCK/2;
                break;

            case STPTI_FILTER_OPERATING_MODE_NONE:
                numfilters = 0;
                /* invalid filter mode i.e. all bits set */
                STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_FILTER_TYPE_FIELD );
                filtersperblock=SF_NUM_FILTERS_PER_BLOCK;
                break;

            default:
                return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
        }

        STSYS_SetTCMask16LE ((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_DVB_PACKET_FORMAT );
    }

    numblocks = (numfilters / filtersperblock );

    STTBX_Print(("Requested %d filters (%d blocks)\n", numfilters, numblocks ));

    session = Device_p->Session;

    if ( SessionFilterList[session].State == FL_FREE )
    {
        if ( numfilters == 0 )  /* STPTI_FILTER_OPERATING_MODE_NONE */
        {
            /* mark that no blocks are allocated to this session - this "marker" is used below when setting up SessionCAMFilterStartAddr */
            FirstBlock = SF_NUM_BLOCKS_PER_CAM;
            LastBlock  = SF_NUM_BLOCKS_PER_CAM;
        }
        else    /* numfilters > 0 */
        {
            /* search through RAMCAM allocation table to see if we have a suitable space */
            /* The allocation must be contiguous */
            for(FirstBlock=0;FirstBlock<SF_NUM_BLOCKS_PER_CAM;FirstBlock++)
            {
                LastBlock=FirstBlock+numblocks-1;
                collision=FALSE;
                for(j=0;j<TC_Params_p->TC_NumberOfSessions;j++)
                {
                    /* is this session allocated? */
                    if ( SessionFilterList[j].State != FL_FREE )
                    {
                        /* Does the start or end of the new blocks overlap the blocks already allocated for this session? */
                        /* Check to see if the blocks are after the end of previously allocated blocks, or if the blocks are */
                        /* before the previously allocated blocks, if this is so then there is NOT a collision (note inverted */
                        /* if statement). */
                        if (! ( (SessionFilterList[j].LastBlock < FirstBlock ) || ( LastBlock < SessionFilterList[j].FirstBlock ) ) )
                        {
                            collision=TRUE;
                            FirstBlock=SessionFilterList[j].LastBlock;          /* accelerate past the end of the colliding blocks (note for loop will +1) */
                            LastBlock=FirstBlock+numblocks-1;
                            break;                                              /* no point checking the other sessions */
                        }
                    }
                }
                if(!collision) break;
            }
            if(LastBlock>=SF_NUM_BLOCKS_PER_CAM)
            {
                STTBX_Print(("Not enough room to place filters in this TCSession (no space in CAMs).\n" ));
                return( ST_ERROR_NO_MEMORY );
            }

            /* search through TCFILTER allocation table to see if we have a suitable space.             */
            /* Again the allocation must be contiguous, but the sessions need not be in the same order  */
            /* as RAMCAM allocation. (this is all complicated by the fact that NMM uses twice the space */
            /* in the CAMS than filters) */
            for(FirstTCFilter=0;FirstTCFilter<TC_Params_p->TC_NumberSectionFilters;FirstTCFilter++)
            {
                LastTCFilter=FirstTCFilter+numfilters-1;
                collision=FALSE;
                for(j=0;j<TC_Params_p->TC_NumberOfSessions;j++)
                {
                    /* is this session allocated? */
                    if ( SessionFilterList[j].State != FL_FREE )
                    {
                        /* Does the start or end of the new filters overlap the filters already allocated for this session? */
                        /* Check to see if the filters are after the end of previously allocated filters, or if the filters are */
                        /* before the previously allocated filters, if this is so then there is NOT a collision (note inverted */
                        /* if statement). */
                        if (! ( (SessionFilterList[j].LastTCFilter < FirstTCFilter ) || ( LastTCFilter < SessionFilterList[j].FirstTCFilter ) ) )
                        {
                            collision=TRUE;
                            FirstTCFilter=SessionFilterList[j].LastTCFilter;    /* accelerate past the end of the colliding filters (note for loop will +1) */
                            LastTCFilter=FirstTCFilter+numfilters-1;
                            break;                                              /* no point checking the other sessions */
                        }
                    }
                }
                if(!collision) break;
            }
            if(LastTCFilter>=TC_Params_p->TC_NumberSectionFilters)
            {
                STTBX_Print(("Not enough room to place filters in this TCSession (no space in TC filters).\n" ));
                return( ST_ERROR_NO_MEMORY );
            }

        }

        SessionFilterList[session].FirstBlock    = FirstBlock;
        SessionFilterList[session].LastBlock     = LastBlock;
        SessionFilterList[session].FirstTCFilter = FirstTCFilter;
        SessionFilterList[session].LastTCFilter  = LastTCFilter;
        SessionFilterList[session].State         = FL_IN_USE;

        /* --- do remaining filter setup stuff --- */

        if ( Device_p->DiscardOnCrcError == TRUE )
        {
            STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_DISCARDONCRCERROR );
        }
        else
        {
            STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionSectionParams, TC_SESSION_INFO_DISCARDONCRCERROR );
        }

        STSYS_WriteRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_0_31,0); /* nothing allocated yet, so nothing enabled from our FilterList_p[n] */
        STSYS_WriteRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_32_63,0);
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        STSYS_WriteRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_64_95,0);
        STSYS_WriteRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_96_127,0);
#endif

        /* --- setup TC RAM CAM configuration --- */
        /* This improves performance of the section filter */

        if      ( numfilters <= 8 )    cam_Config = CAM_CFG_8_FILTERS;
        else if ( numfilters <= 16 )   cam_Config = CAM_CFG_16_FILTERS;
        else if ( numfilters <= 32 )   cam_Config = CAM_CFG_32_FILTERS;
        else if ( numfilters <= 64 )   cam_Config = CAM_CFG_64_FILTERS;
        else
        {
            /* a 'should not happen' error */
            return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
        }
        STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionCAMConfig, cam_Config );

        STTBX_Print(("      cam_Config: 0x%04x\n", cam_Config));

        /* --- setup TC RAM CAM base address --- */

        /* Note: SessionCAMFilterStartAddr is at bit position 7:1 (7 bits) */

        if ( SessionFilterList[session].FirstBlock == SF_NUM_BLOCKS_PER_CAM )
        {
            /* point to just past start of reserved area (0x304) */
            STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionCAMFilterStartAddr, (0xC1 << 1) ); /* 0x182 */
        }
        else
        {
            block = SessionFilterList[session].FirstBlock;

            BaseAddress = (U32)&TCSectionFilterArrays_p->CamA_Block[0];
            Offset      = (U32)&TCSectionFilterArrays_p->CamA_Block[block];

            cam_first_filter = ((Offset - BaseAddress) >> 2);   /* divide by 4 to get word offset */
            cam_first_filter = (cam_first_filter << 1);         /* LSH 1 for correct bit alignment on register */
            STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionCAMFilterStartAddr, cam_first_filter );
            STTBX_Print(("cam_first_filter: 0x%04x (block #%d)\n", cam_first_filter, block ));
        }

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        {
            STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
            DumpFilterList( PrivateData_p, TC_Params_p );
        }
        #endif

        /* all filters in session should be free*/

        for(BlockIdent=SessionFilterList[Device_p->Session].FirstBlock;
            BlockIdent<=SessionFilterList[Device_p->Session].LastBlock;BlockIdent++)
        {

            FilterList->CamA[BlockIdent][0] = FL_FREE;
            FilterList->CamA[BlockIdent][1] = FL_FREE;
            FilterList->CamA[BlockIdent][2] = FL_FREE;
            FilterList->CamA[BlockIdent][3] = FL_FREE;

            FilterList->CamB[BlockIdent][0] = FL_FREE;
            FilterList->CamB[BlockIdent][1] = FL_FREE;
            FilterList->CamB[BlockIdent][2] = FL_FREE;
            FilterList->CamB[BlockIdent][3] = FL_FREE;
        }

        return( ST_NO_ERROR );

    }   /* if (FL_FREE) */

    STTBX_Print(("cannot allocate!\n"));
    return( ST_ERROR_NO_MEMORY );
}

/******************************************************************************
Function Name : TcCam_SetMode
  Description : Sets the Filters Mode and slot's Associated Filter Mode based
                on the high level FilterType enum.
   Parameters : 
******************************************************************************/
ST_ErrorCode_t TcCam_SetMode(TCSectionFilterInfo_t *TCSectionFilterInfo_p, Slot_t *Slot_p, STPTI_FilterType_t FilterType)
{
    U16 FilterMode = ConvertFilterType(FilterType);

    STTBX_Print(("TcCam_SetMode() - Setting filter type %d to 0x%04x\n", FilterType, FilterMode));

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionFilterMode, FilterMode );
#endif
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionState, 0 );
    Slot_p->AssociatedFilterMode = FilterMode;
   
    return(ST_NO_ERROR);
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : TcCam_FilterFreeSession
  Description : Called when the a virtual PTI is term'ed (STPTI_Term())
   Parameters : Device Handle of the virtual PTI
******************************************************************************/
ST_ErrorCode_t TcCam_FilterFreeSession( FullHandle_t DeviceHandle )
{
    Device_t                *Device_p            = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t         *PrivateData_p       = &Device_p->TCPrivateData;
    FilterList_t            *FilterList          = PrivateData_p->FilterList;
    TCSessionInfo_t         *TCSessionInfo_p     = stptiHAL_GetSessionFromFullHandle( DeviceHandle );
    STPTI_TCParameters_t    *TC_Params_p         = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    SessionFilterList_t     *SessionFilterList;

    int session = Device_p->Session;
    int block;
    U8 FirstBlock, LastBlock;

    if(FilterList != NULL)
    {
        SessionFilterList = FilterList->SessionFilterList;

        STTBX_Print(("\n *** TcCam_FilterFreeSession ***\n"));
        STSYS_WriteRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_0_31,0);
        STSYS_WriteRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_32_63,0);
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        STSYS_WriteRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_64_95,0);
        STSYS_WriteRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_96_127,0);
#endif

        FirstBlock = SessionFilterList[session].FirstBlock;
        LastBlock  = SessionFilterList[session].LastBlock;

        if ( FirstBlock == SF_NUM_BLOCKS_PER_CAM )   /* STPTI_FILTER_OPERATING_MODE_NONE */
        {
            STTBX_Print(("TcCam_FilterFreeSession: STPTI_FILTER_OPERATING_MODE_NONE (no filter to free)\n"));
        }
        else
        {
            STTBX_Print(("TcCam_FilterFreeSession: Freeing filters for session %d (block #%d to #%d)\n", session, FirstBlock, LastBlock ));

            /* clean usage list for the blocks allocated to this session */
            for ( block = FirstBlock; block <= LastBlock; block++)
            {
                STTBX_Print(("Freeing block #%d...", block ));
                FilterList->CamA[block][0] = FL_FREE;   /* mark these filters as being free for use */
                FilterList->CamA[block][1] = FL_FREE;
                FilterList->CamA[block][2] = FL_FREE;
                FilterList->CamA[block][3] = FL_FREE;

                FilterList->CamB[block][0] = FL_FREE;
                FilterList->CamB[block][1] = FL_FREE;
                FilterList->CamB[block][2] = FL_FREE;
                FilterList->CamB[block][3] = FL_FREE;

                STTBX_Print(("ok\n"));

            }
        }

        STTBX_Print(("TcCam_FilterFreeSession: freeing blocks allocated to session.\n"));

        SessionFilterList[session].FirstBlock    = SF_NUM_BLOCKS_PER_CAM;                   /* set to an invalid value */
        SessionFilterList[session].LastBlock     = SF_NUM_BLOCKS_PER_CAM;                   /* set to an invalid value */
        SessionFilterList[session].FirstTCFilter = TC_Params_p->TC_NumberSectionFilters;    /* set to an invalid value */
        SessionFilterList[session].LastTCFilter  = TC_Params_p->TC_NumberSectionFilters;    /* set to an invalid value */
        SessionFilterList[session].State         = FL_FREE;

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        {
            STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *)&PrivateData_p->TC_Params;
            DumpFilterList( PrivateData_p, TC_Params_p );
        }
        #endif
    } 

    return( ST_NO_ERROR );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */





/* ------------------------------------------------------------------------- */
/*                           Inner API Functions                             */
/* ------------------------------------------------------------------------- */

/******************************************************************************
Function Name : TcCam_GetMetadataSizeInBytes
  Description : Returns Size of the Meta Data in bytes
   Parameters : None

Metadata is o/p as U16s: A0 A1 A2 B0 B1 B2.

******************************************************************************/
int TcCam_GetMetadataSizeInBytes( void )
{
    return( BYTES_OF_METADATA_TC_OUTPUTS );
}


/******************************************************************************
Function Name : TcCam_GetFilterOffset
  Description : Returns the offset into tc filter handles for a given session.
   Parameters : FullHandle for the Filter (or device)
******************************************************************************/
U32 TcCam_GetFilterOffset( FullHandle_t FullHandle )
{
    Device_t                *TCSessionsDevice_p       = stptiMemGet_Device(FullHandle);
    TCPrivateData_t         *PrivateData_p            = &TCSessionsDevice_p->TCPrivateData;
    FilterList_t            *FilterList               = PrivateData_p->FilterList;
    SessionFilterList_t     *SessionFilterList        = FilterList->SessionFilterList;

    return(SessionFilterList[TCSessionsDevice_p->Session].FirstTCFilter);

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

    U32 camstatus;
    U32 filter = 0;
    U32 MatchCount = 0;
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    U32 FilterOffset = 0;
#else
    U32 FilterOffset = TcCam_GetFilterOffset(FullBufferHandle);
#endif
    U32 mask, CombinedMatches, i;
    U16 filtermode=0;
    
    FullHandle_t     SlotListHandle   = ( stptiMemGet_Buffer( FullBufferHandle ))->SlotListHandle;
    U32              NumberOfSlots    = stptiMemGet_List( SlotListHandle )->MaxHandles;
    FullHandle_t     FullSlotHandle;

    U32 MatchedFilterMask[4] = {0};

    /* RAMCAM */
#if defined ( STPTI_ARCHITECTURE_PTI4_LITE )
    MatchedFilterMask[0] |= (U32) MetaData[0];              /* CAMA filters 0..7       */
    MatchedFilterMask[0] |= (U32) (MetaData[1] << 8);       /* CAMA filters 8..15      */
    MatchedFilterMask[0] |= (U32) (MetaData[2] << 16);      /* CAMA filters 16..23     */
    MatchedFilterMask[0] |= (U32) (MetaData[3] << 24);      /* CAMA filters 24..31     */
    MatchedFilterMask[1] |= (U32) MetaData[6];              /* CAMB filters 0..7       */
    MatchedFilterMask[1] |= (U32) (MetaData[7] << 8);       /* CAMB filters 8..15      */
    MatchedFilterMask[1] |= (U32) (MetaData[8] << 16);      /* CAMB filters 16..23     */
    MatchedFilterMask[1] |= (U32) (MetaData[9] << 24);      /* CAMB filters 24..31     */
    
#else
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1)
    MatchedFilterMask[0] |= (U32) MetaData[0];              /* CAMA filters 0..7       */
    MatchedFilterMask[0] |= (U32) (MetaData[1] << 8);       /* CAMA filters 8..15      */
    MatchedFilterMask[0] |= (U32) (MetaData[2] << 16);      /* CAMA filters 16..23     */
    MatchedFilterMask[0] |= (U32) (MetaData[3] << 24);      /* CAMA filters 24..31     */
    MatchedFilterMask[2] |= (U32) MetaData[4];              /* CAMA filters 32..39     */
    MatchedFilterMask[2] |= (U32) (MetaData[5] << 8);       /* CAMA filters 40..47     */
    MatchedFilterMask[1] |= (U32) MetaData[6];              /* CAMB filters 0..7       */
    MatchedFilterMask[1] |= (U32) (MetaData[7] << 8);       /* CAMB filters 8..15      */
    MatchedFilterMask[1] |= (U32) (MetaData[8] << 16);      /* CAMB filters 16..23     */
    MatchedFilterMask[1] |= (U32) (MetaData[9] << 24);      /* CAMB filters 24..31     */
    MatchedFilterMask[3] |= (U32) MetaData[10];             /* CAMB filters 32..39     */
    MatchedFilterMask[3] |= (U32) (MetaData[11] << 8);      /* CAMB filters 40..47     */

#else
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
    MatchedFilterMask[0] |= (U32) MetaData[0];              /* CAMA filters 0..7       */
    MatchedFilterMask[0] |= (U32) (MetaData[1] << 8);       /* CAMA filters 8..15      */
    MatchedFilterMask[0] |= (U32) (MetaData[2] << 16);      /* CAMA filters 16..23     */
    MatchedFilterMask[0] |= (U32) (MetaData[3] << 24);      /* CAMA filters 24..31     */
    MatchedFilterMask[2] |= (U32) MetaData[4];              /* CAMA filters 32..39     */
    MatchedFilterMask[2] |= (U32) (MetaData[5] << 8);       /* CAMA filters 40..47     */
    MatchedFilterMask[2] |= (U32) (MetaData[6] << 16);      /* CAMA filters 48..55     */
    MatchedFilterMask[2] |= (U32) (MetaData[7] << 24);      /* CAMA filters 56..63     */
    MatchedFilterMask[1] |= (U32) MetaData[8];              /* CAMB filters 0..7       */
    MatchedFilterMask[1] |= (U32) (MetaData[9] << 8);       /* CAMB filters 8..15      */
    MatchedFilterMask[1] |= (U32) (MetaData[10] << 16);     /* CAMB filters 16..23     */
    MatchedFilterMask[1] |= (U32) (MetaData[11] << 24);     /* CAMB filters 24..31     */
    MatchedFilterMask[3] |= (U32) MetaData[12];             /* CAMB filters 32..39     */
    MatchedFilterMask[3] |= (U32) (MetaData[13] << 8);      /* CAMB filters 40..47     */
    MatchedFilterMask[3] |= (U32) (MetaData[14] << 16);     /* CAMB filters 48..55     */
    MatchedFilterMask[3] |= (U32) (MetaData[15] << 24);     /* CAMB filters 56..63     */
    
#endif
#endif
#endif
    
    if(DSS_MPT_Mode)
    {
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
        /* for MPT frames we are using Positive Negative Match Mode, but the Negative Match part   */
        /* has a cleared mask, so never matches.  We invert it so that it appears to have matched */
        /* for stptiHAL_MaskConvertToList(). */
        MatchedFilterMask[1] = ~MatchedFilterMask[1];
#endif
    }
    
    for(i=0;i<NumberOfSlots;i++)
    {
        FullSlotHandle.word = ( &stptiMemGet_List( SlotListHandle )->Handle )[i];
        filtermode = stptiMemGet_Slot(FullSlotHandle)->AssociatedFilterMode;
        if(filtermode != 0) break;      /* Stop when we find a filter something */
    }

    STTBX_Print(("stptiHAL_MaskConvertToList() - %d associated slots, filtermode is 0x%04x\n", (unsigned int) NumberOfSlots, (unsigned int) filtermode));
    STTBX_Print(("Mask is 0x%08x 0x%08x\n", (unsigned int) MatchedFilterMask[0], (unsigned int) MatchedFilterMask[1]));
    
    if((filtermode==TC_SESSION_INFO_FILTER_TYPE_SHORT) || (filtermode==TC_SESSION_INFO_FILTER_TYPE_LONG))
    {
        /* For SHORT/LONG filters we allocate 4 in CAMA, 4 in CAMB, 4 in CAMA, 4 in CAMB... */
        /* I should mention here that the LONG filter allocation is different from that in STDCAM, */
        /* where it was allocated more like NMM.  This due to a the changes in the h/w blocks. */
        /* Note that 7109 and 7200 report filter numbers up to 95 and 127 respectively */
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        for (camstatus = 0; camstatus < 4; camstatus++)
#else
        for (camstatus = 0; camstatus < 2; camstatus++)
#endif
        {
            if(camstatus==0)      /* CAM A starts with filter 0 */
            {
                filter=0;
            }
            else if(camstatus == 1)           /* CAM B starts with filter 4*/
            {
                filter=SF_CAMLINE_WIDTH;
            }
            else if(camstatus == 2)         /* Top section of CAM A starts with filter 64 */
            {
                filter = 64;
            }
            else                            /* Top section of CAM B starts with filter 68 */
            {
                filter = 64 + SF_CAMLINE_WIDTH;
            }
            for (mask = 1; (mask != 0); mask <<= 1)
            {
                if (MatchedFilterMask[camstatus] & mask)
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
                filter++;
                if(filter%SF_CAMLINE_WIDTH==0)      /* skip 4 (SF_CAMLINE_WIDTH) filters every 4 (SF_CAMLINE_WIDTH) filters */
                {
                    /* CAM_A 0,1,2,3, 8,9,10,11,   16... */
                    /* CAM_B 4,5,6,7, 12,13,14,15, 20... */
                    filter=filter+SF_CAMLINE_WIDTH;
                }
            }
        }
    }
    else if(filtermode==TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH)
    {
        /* For NMM filters we allocate one half in CAMA and the other in CAMB, both need to have matched */
        CombinedMatches = MatchedFilterMask[0] & ~MatchedFilterMask[1];
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
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
        CombinedMatches = MatchedFilterMask[2] & ~MatchedFilterMask[3];
        for (mask = 1, filter = 32; (mask != 0); ++filter, mask <<= 1)
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
#endif /* defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE) */
    }
    *NumOfFilterMatches_p = MatchCount; /* Load up number of matched filters */
    STTBX_Print(("Found %d matches.\n", (unsigned int) MatchCount ));
}



/* ------------------------------------------------------------------------- */


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHAL_DVBFilterEnableSection
  Description : stpti_TCEnableSectionFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t TcCam_EnableFilter( FullHandle_t FilterHandle, BOOL enable )
{
    U32               FilterIdent   = stptiMemGet_Filter(FilterHandle)->TC_FilterIdent;
    TCSessionInfo_t  *TCSessionInfo_p = stptiHAL_GetSessionFromFullHandle( FilterHandle );
    Device_t         *Device_p      = stptiMemGet_Device( FilterHandle );
    TCPrivateData_t  *PrivateData_p = &Device_p->TCPrivateData;
    FilterList_t     *FilterList    = PrivateData_p->FilterList;
    U16               TC_FilterMode = FilterList->FilterMode[FilterIdent];
    
    TcCam_SetFilterEnableState( TCSessionInfo_p,  FilterIdent, TC_FilterMode, enable);

    return( ST_NO_ERROR );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : TcCam_SetFilterEnableState
  Description : Enable or disables a specific filter for a session.
   Parameters :
******************************************************************************/
void TcCam_SetFilterEnableState( TCSessionInfo_t *TCSessionInfo_p, U32 FilterIndex, U16 TC_FilterMode, BOOL EnableFilter )
{
    Device_t *TCSessionsDevice_p=NULL, *Device_p;

    BOOL       IsCamA = FALSE;
    /*skr changed from U16 7/21/2004 5:45PM*/
    U32 AssociateFilterMask, NormalizedFilterIndex;
    U8  ShiftValue, Loop, TCSession=0;
    U8 Block, Filter;

    STTBX_Print(("\n *** TcCam_SetFilterEnableState Filter #%d ", FilterIndex ));

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    if ( EnableFilter )
    {
        STTBX_Print(("ENABLE ***\n"));
    }
    else
    {
        STTBX_Print(("DISABLE ***\n"));
    }
#endif

    STTBX_Print((" --- before ---\n"));
    STTBX_Print((" SectionEnables_0_31  0x%08x\n", STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SectionEnables_0_31) )); /* CAM A */
    STTBX_Print(("SectionEnables_32_63  0x%08x\n", STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SectionEnables_32_63) )); /* CAM A (nmm) or CAM B (short) */
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    STTBX_Print(("SectionEnables_64_95  0x%08x\n", STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SectionEnables_64_95) )); /* CAM A */
    STTBX_Print(("SectionEnables_96_127 0x%08x\n", STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SectionEnables_96_127) )); /* CAM A (nmm) or CAM B (short) */
#endif

    /* Work though all the virtual devices to find the virtual device corresponding to */
    /* this session. */
    for(Loop=0;Loop<stpti_GetNumberOfExistingDevices();Loop++)
    {
        Device_p = stpti_GetDevice(Loop);
        /* Ignore unused (deallocated) devices */
        if(Device_p!=NULL)
        {
            if((TCSessionInfo_t *)stptiHAL_GetSessionFromFullHandle(Device_p->Handle) == TCSessionInfo_p)
            {
                TCSessionsDevice_p = Device_p;
                TCSession  = TCSessionsDevice_p->Session;
                break;
            }
        }
    }

#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    ShiftValue = 0;             /* For secure lite use absolute offsetting */
#else
    /* Set ShiftValue to the offset into the filter bank for this session */
    ShiftValue = TcCam_GetFilterOffset( TCSessionsDevice_p->Handle );
#endif
    if ( ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_SHORT ) || ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_LONG ) )
    {
        STTBX_Print((" (short)\n"));

        Block = ( FilterIndex / SF_NUM_FILTERS_PER_BLOCK );
        Filter = ( FilterIndex % SF_NUM_FILTERS_PER_BLOCK );

        if ( Filter < SF_CAMLINE_WIDTH )
        {
            IsCamA = TRUE;
        }

        /* FilterIndex is absolute 0.. index into the filter bank */
        /* ShiftValue is the starting filter for this session */

        /* We want an AssociateFilterMask which takes into account the funny ramcam mapping    */
        /* ConvertNormalizedFilterIndexToAssociateBitmask(0x12345678,TRUE) = 0x00002468   CAMA */
        /* ConvertNormalizedFilterIndexToAssociateBitmask(0x12345678,FALSE) = 0x00001357  CAMB */

        /* We have to deal with the fact that NormalisedFilterIndex is only 32bits */
        if ( (FilterIndex - ShiftValue) < 32 )
		{
        	NormalizedFilterIndex = (1 << (FilterIndex - ShiftValue));
        	AssociateFilterMask = ConvertNormalizedFilterIndexToAssociateBitmask( NormalizedFilterIndex, IsCamA);
        }
#if !defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        else
        {
        	NormalizedFilterIndex = (1 << (FilterIndex -32 - ShiftValue));
        	AssociateFilterMask = ConvertNormalizedFilterIndexToAssociateBitmask( NormalizedFilterIndex, IsCamA);
        	AssociateFilterMask <<=16;
        }
#else
        else if ( (FilterIndex - ShiftValue) < 64 )
        {
        	NormalizedFilterIndex = (1 << (FilterIndex -32 - ShiftValue));
        	AssociateFilterMask = ConvertNormalizedFilterIndexToAssociateBitmask( NormalizedFilterIndex, IsCamA);
        	AssociateFilterMask <<=16;
        }
        else if ( (FilterIndex - ShiftValue) < 96 )
        {
        	NormalizedFilterIndex = (1 << (FilterIndex -64 - ShiftValue));
        	AssociateFilterMask = ConvertNormalizedFilterIndexToAssociateBitmask( NormalizedFilterIndex, IsCamA);
        }
        else
        {
        	NormalizedFilterIndex = (1 << (FilterIndex -96 - ShiftValue));
        	AssociateFilterMask = ConvertNormalizedFilterIndexToAssociateBitmask( NormalizedFilterIndex, IsCamA);
        	AssociateFilterMask <<=16;
        }
#endif

        if ( IsCamA )
        {
            if ( EnableFilter )
            {
                if((FilterIndex - ShiftValue) < 64)
                {
                    STSYS_SetRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_0_31, AssociateFilterMask);
                }
                else
                {
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
                    STSYS_SetRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_64_95, AssociateFilterMask);
#endif
                }
            }
            else
            {
                if((FilterIndex - ShiftValue) < 64)
                {
                    STSYS_ClearRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_0_31, AssociateFilterMask);
                }
                else
                {
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
                    STSYS_ClearRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_64_95, AssociateFilterMask);
#endif
                }
            }
        }
        else  /* Cam B */
        {
            if ( EnableFilter )
            {
                if((FilterIndex - ShiftValue) < 64)
                {
                    STSYS_SetRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_32_63, AssociateFilterMask);
                }
                else
                {
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
                    STSYS_SetRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_96_127, AssociateFilterMask);
#endif
                }
            }
            else
            {
                if((FilterIndex - ShiftValue) < 64)
                {
                    STSYS_ClearRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_32_63, AssociateFilterMask);
                }
                else
                {
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
                    STSYS_ClearRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_96_127, AssociateFilterMask);
#endif
                }
            }
        }

    }
    else    /* NM mode, again this is simpler than short mode */
    {
        STTBX_Print((" (nmm)\n"));

        NormalizedFilterIndex = FilterIndex - ShiftValue;
        if ( EnableFilter )
        {
            if (NormalizedFilterIndex < 32)
            {
                STSYS_SetRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_0_31,  (1 << NormalizedFilterIndex ));
            }
            else
            {
                STSYS_SetRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_32_63, (1 << (NormalizedFilterIndex - 32)) );
            }
        }
        else
        {
            if (NormalizedFilterIndex < 32)
            {
                STSYS_ClearRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_0_31,  (1 << NormalizedFilterIndex ));
            }
            else
            {
                STSYS_ClearRegMask32LE((void*)&TCSessionInfo_p->SectionEnables_32_63,  (1 << (NormalizedFilterIndex - 32) ));
            }
        }

   }

    STTBX_Print((" -- after ---\n"));
    STTBX_Print((" SectionEnables_0_31  0x%08x\n", STSYS_ReadRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_0_31) ));
    STTBX_Print(("SectionEnables_32_63  0x%08x\n", STSYS_ReadRegDev32LE((void*)&TCSessionInfo_p->SectionEnables_32_63) ));
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    STTBX_Print(("SectionEnables_64_95  0x%08x\n", STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SectionEnables_64_95) )); /* CAM A */
    STTBX_Print(("SectionEnables_96_127 0x%08x\n", STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SectionEnables_96_127) )); /* CAM A (nmm) or CAM B (short) */
#endif
}



#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : TcCam_ClearSectionInfoAssociations
  Description : Clears Filter Associations.
   Parameters :

  SectionAssociation specifies which filters are enabled for a slot (MainInfo)
  up to a maximum of 64 (only 48 can be matched though)

******************************************************************************/
void TcCam_ClearSectionInfoAssociations( TCSectionFilterInfo_t *TCSectionFilterInfo_p )
{
    STTBX_Print(("\n *** TcCam_ClearSectionInfoAssociations ***\n"));
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0, 0 );
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1, 0 );
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2, 0 );
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3, 0 );
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation4, 0 );
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation5, 0 );
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation6, 0 );
    STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionAssociation7, 0 );
#endif
}


/******************************************************************************
Function Name : TcCam_SectionInfoNothingIsAssociated
  Description : Returns TRUE if no filters are associated with buffers.
   Parameters :
******************************************************************************/
BOOL TcCam_SectionInfoNothingIsAssociated( TCSectionFilterInfo_t *TCSectionFilterInfo_p )
{
    STTBX_Print(("\n *** TcCam_SectionInfoNothingIsAssociated: "));

    if ( (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0) == 0) &&
         (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1) == 0) &&
         (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2) == 0) &&
         (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3) == 0) 
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        &&
         (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation4) == 0) &&
         (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation5) == 0) &&
         (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation6) == 0) &&
         (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation7) == 0)
#endif
        )
    {
        STTBX_Print(("TRUE\n"));
        return( TRUE );
    }

    STTBX_Print(("FALSE\n"));
    return( FALSE );
}


/******************************************************************************
Function Name : TcCam_DisassociateFilterFromSectionInfo
  Description : Disassociate Filter
   Parameters :
******************************************************************************/
ST_ErrorCode_t TcCam_DisassociateFilterFromSectionInfo(FullHandle_t DeviceHandle, TCSectionFilterInfo_t *TCSectionFilterInfo_p, U32 FilterIndex )
{
    STTBX_Print(("\n *** TcCam_DisassociateFilterFromSectionInfo (filter #%d, SFInfo: 0x%08x)\n", FilterIndex, (U32)TCSectionFilterInfo_p ));

    return( SetFilterAssociation(DeviceHandle, TCSectionFilterInfo_p, FilterIndex, FALSE) );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : TcCam_AssociateFilterWithSectionInfo
  Description : Associate Filter
   Parameters :
******************************************************************************/
ST_ErrorCode_t TcCam_AssociateFilterWithSectionInfo( FullHandle_t DeviceHandle, TCSectionFilterInfo_t *TCSectionFilterInfo_p, U32 FilterIndex )
{
    STTBX_Print(("\n *** TcCam_AssociateFilterWithSectionInfo (filter #%d, SFInfo: 0x%08x)\n", FilterIndex, (U32)TCSectionFilterInfo_p ));

    return( SetFilterAssociation(DeviceHandle, TCSectionFilterInfo_p, FilterIndex, TRUE) );
}

#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : TcCam_FilterDeallocate
  Description : Deallocate Filter (section)
   Parameters :
******************************************************************************/
ST_ErrorCode_t TcCam_FilterDeallocate( FullHandle_t DeviceHandle, U32 FilterIndex )
{
    Device_t             *Device_p        = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p   = &Device_p->TCPrivateData;
    FilterList_t         *FilterList      = PrivateData_p->FilterList;
    SessionFilterList_t  *SessionFilterList = &FilterList->SessionFilterList[ Device_p->Session ];
    
    U16 TC_FilterMode = FilterList->FilterMode[FilterIndex];
    U8 Block, Filter;

    STTBX_Print(("\n *** TcCam_FilterDeallocate (Device=0x0%08x, filter #%d, session #%d) 0x%04x ***\n", Device_p->TCDeviceAddress_p, FilterIndex, Device_p->Session, TC_FilterMode ));

    FilterList->FilterMode[FilterIndex] = 0;
    
    if ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_SHORT )
    {
        /* remember: include session->FirstBlock to calcs for full multi-session */

        Block = ( (FilterIndex-SessionFilterList->FirstTCFilter) / SF_NUM_FILTERS_PER_BLOCK );
        Block += SessionFilterList->FirstBlock;
        Filter = ( (FilterIndex-SessionFilterList->FirstTCFilter) % SF_NUM_FILTERS_PER_BLOCK );

        STTBX_Print(("Block #%d...", Block ));

        if ( Filter < SF_CAMLINE_WIDTH )
        {
            FilterList->CamA[Block][Filter] = FL_FREE;
            STTBX_Print(("deallocated CAM_A filter #%d\n", Filter));
        }
        else
        {
            Filter = Filter - SF_CAMLINE_WIDTH;
            FilterList->CamB[Block][Filter] = FL_FREE;
            STTBX_Print(("deallocated CAM_B filter #%d\n", Filter));
        }

    }
    else if ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_LONG )
    {
        /* remember: include session->FirstBlock to calcs for full multi-session */

        Block = ( (FilterIndex-SessionFilterList->FirstTCFilter) / SF_NUM_FILTERS_PER_BLOCK );
        Block += SessionFilterList->FirstBlock;
        Filter = ( (FilterIndex-SessionFilterList->FirstTCFilter) % SF_NUM_FILTERS_PER_BLOCK );

        STTBX_Print(("Block #%d...", Block ));

        if ( Filter < SF_CAMLINE_WIDTH )
        {
            FilterList->CamA[Block][Filter] = FL_FREE;
            FilterList->CamA[Block+1][Filter] = FL_FREE;
            STTBX_Print(("deallocated CAM_A filter #%d\n", Filter));
        }
        else
        {
            Filter = Filter - SF_CAMLINE_WIDTH;
            FilterList->CamB[Block][Filter] = FL_FREE;
            FilterList->CamB[Block+1][Filter] = FL_FREE;
            STTBX_Print(("deallocated CAM_B filter #%d\n", Filter));
        }

    }
    else if ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH )
    {
        /* remember: include session->FirstBlock to calcs for full multi-session */

        Block = ( (FilterIndex-SessionFilterList->FirstTCFilter) / (SF_NUM_FILTERS_PER_BLOCK/2) );
        Block += SessionFilterList->FirstBlock;
        Filter = ( (FilterIndex-SessionFilterList->FirstTCFilter) % (SF_NUM_FILTERS_PER_BLOCK/2) );

        STTBX_Print(("Block #%d...", Block ));

        if ( Filter < SF_CAMLINE_WIDTH )
        {
            FilterList->CamA[Block][Filter] = FL_FREE;
            FilterList->CamB[Block][Filter] = FL_FREE;

            STTBX_Print(("deallocated CAM_A & CAM_B filter #%d\n", Filter));
        }
        else
        {
            return( STPTI_ERROR_INVALID_FILTER_HANDLE );
        }
    }
    else
    {
        return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
    }

    return( ST_NO_ERROR );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/******************************************************************************
Function Name : TcCam_FilterAllocate
  Description : Allocate Filter (section)
   Parameters :
******************************************************************************/
ST_ErrorCode_t TcCam_FilterAllocate( FullHandle_t DeviceHandle, STPTI_FilterType_t FilterType, U32 *FilterIndex )
{
    Device_t             *Device_p          = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p     = &Device_p->TCPrivateData;

    FilterList_t         *FilterList        = PrivateData_p->FilterList;
    SessionFilterList_t  *SessionFilterList = &FilterList->SessionFilterList[ Device_p->Session ];

    U32 FilterNum;
    U16 TC_FilterMode = ConvertFilterType(FilterType);
    
    if( TC_FilterMode == 0 ) return( STPTI_ERROR_INVALID_FILTER_TYPE );
    
    FilterNum = AllocateFilter( FilterList, SessionFilterList, TC_FilterMode );

    if ( FilterNum  == INVALID_FILTER_INDEX )
        return( STPTI_ERROR_NO_FREE_FILTERS );

    /* return (by parameter) absolute filter index */
    *FilterIndex = FilterNum;
    
    STTBX_Print(("\n *** TcCam_FilterAllocate (Device=0x0%08x, filter #%d, session #%d) 0x%04x ***\n", Device_p->TCDeviceAddress_p, FilterNum, Device_p->Session, TC_FilterMode ));
    
    return( ST_NO_ERROR );
}

    
    

/******************************************************************************
Function Name : TcCam_FilterSetSection
  Description : Setup filter contents (data/mask) for Sections
   Parameters :
******************************************************************************/
ST_ErrorCode_t TcCam_FilterSetSection(FullHandle_t FilterHandle, STPTI_FilterData_t *FilterData_p)
{
    U32                      FilterIdent              = stptiMemGet_Filter(FilterHandle)->TC_FilterIdent;
    Device_t                *Device_p                 = stptiMemGet_Device( FilterHandle );
    TCPrivateData_t         *PrivateData_p            = &Device_p->TCPrivateData;
    STPTI_TCParameters_t    *TC_Params_p              = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    FilterList_t            *FilterList               = PrivateData_p->FilterList;
    TCDevice_t              *TC_Device                = (TCDevice_t *)Device_p->TCDeviceAddress_p;
    TCSectionFilterArrays_t *TC_SectionFilterArrays_p = (TCSectionFilterArrays_t *)&TC_Device->TC_SectionFilterArrays;
    TCSessionInfo_t         *TCSessionInfo_p          = stptiHAL_GetSessionFromFullHandle( FilterHandle );
    U16                      TC_FilterMode            = FilterList->FilterMode[FilterIdent];
    U8                      *ModePattern_p            = FilterData_p->u.SectionFilter.ModePattern_p;
    SessionFilterList_t     *SessionFilterList        = &FilterList->SessionFilterList[ Device_p->Session ];

    U32 NotMatchData, AlignedFilterADataMS = 0;
    U8 Block, Filter, i, AccMask;
    TCCamIndex_t *TCCamIndex, *ShadowIndex, *TCCamIndexA, *ShadowIndexA, *TCCamIndexB, *ShadowIndexB;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTBX_Print(("*** TcCam_FilterSetSection FilterIdent #%d, ", FilterIdent));

    /* --- Set-up NotMatchMode --- */

    if ( FilterData_p->u.SectionFilter.NotMatchMode )
    {
        AlignedFilterADataMS  = (FilterData_p->FilterBytes_p[0] << 24);
        AlignedFilterADataMS |= (FilterData_p->FilterBytes_p[1] << 16);
        AlignedFilterADataMS |= (FilterData_p->FilterBytes_p[2] <<  8);
        AlignedFilterADataMS |= (FilterData_p->FilterBytes_p[3]      );

        NotMatchData = (NOT_MATCH_ENABLE | (((AlignedFilterADataMS >> 1) & 0x1f) & NOT_MATCH_DATA_MASK));
    }
    else
    {
        NotMatchData = 0;
    }

    if ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_SHORT )
    {
        /* remember: include session->FirstBlock to calcs for full multi-session */

        Block  = ( (FilterIdent-SessionFilterList->FirstTCFilter) / SF_NUM_FILTERS_PER_BLOCK );
        Block += SessionFilterList->FirstBlock;
        Filter = ( (FilterIdent-SessionFilterList->FirstTCFilter) % SF_NUM_FILTERS_PER_BLOCK );

        STTBX_Print(("SHORT mode, Block #%d Filter #%d. ", Block, Filter ));

        if ( Filter < SF_CAMLINE_WIDTH )
        {
            STTBX_Print(("CAM_A\n"));
            TCCamIndex  = &TC_SectionFilterArrays_p->CamA_Block[Block];
            ShadowIndex = &FilterList->ShadowSectionFilterArray.CamA_Block[Block];
        }
        else
        {
            STTBX_Print(("CAM_B\n"));
            Filter = Filter - SF_CAMLINE_WIDTH;
            TCCamIndex = &TC_SectionFilterArrays_p->CamB_Block[Block];
            ShadowIndex = &FilterList->ShadowSectionFilterArray.CamB_Block[Block];
        }

        for (i = 0; i < SF_FILTER_LENGTH; i++)
        {
            ShadowIndex->Index[i].Data.Element.Filter[Filter] = FilterData_p->FilterBytes_p[i];
            ShadowIndex->Index[i].Mask.Element.Filter[Filter] = FilterData_p->FilterMasks_p[i];
        }

        if(NotMatchData != 0)
        {
            ShadowIndex->Index[3].Mask.Element.Filter[Filter] &= NOT_MATCH_MASK_MASK;      /* Clear mask bits 1-5 for NotMatch */
        }

        TcCam_SetFilterEnableState( TCSessionInfo_p, FilterIdent, TC_FilterMode, FALSE );

        Error = WriteBlock( TC_Params_p, FilterIdent, ShadowIndex, TCCamIndex, TC_SectionFilterArrays_p, NotMatchData );
        if(Error != ST_NO_ERROR)
            return(Error);

        if (FilterData_p->InitialStateEnabled)      /* Re-enable filter, if initial state is enabled */
        {
             TcCam_SetFilterEnableState( TCSessionInfo_p, FilterIdent, TC_FilterMode, TRUE );
        }

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        DumpBlock( TCCamIndex );
        DumpBlock( ShadowIndex );
#endif

    }
    else if ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_LONG )
    {
        /* remember: include session->FirstBlock to calcs for full multi-session */

        Block  = ( (FilterIdent-SessionFilterList->FirstTCFilter) / SF_NUM_FILTERS_PER_BLOCK );
        Block += SessionFilterList->FirstBlock;
        Filter = ( (FilterIdent-SessionFilterList->FirstTCFilter) % SF_NUM_FILTERS_PER_BLOCK );

        STTBX_Print(("LONG mode, Block #%d&%d Filter #%d. ", Block, Block+1, Filter ));

        if ( Filter < SF_CAMLINE_WIDTH )
        {
            STTBX_Print(("CAM_A\n"));
            TCCamIndexA  = &TC_SectionFilterArrays_p->CamA_Block[Block];
            TCCamIndexB  = &TC_SectionFilterArrays_p->CamA_Block[Block+1];
            ShadowIndexA = &FilterList->ShadowSectionFilterArray.CamA_Block[Block];
            ShadowIndexB = &FilterList->ShadowSectionFilterArray.CamA_Block[Block+1];
        }
        else
        {
            STTBX_Print(("CAM_B\n"));
            Filter = Filter - SF_CAMLINE_WIDTH;
            TCCamIndexA  = &TC_SectionFilterArrays_p->CamB_Block[Block];
            TCCamIndexB  = &TC_SectionFilterArrays_p->CamB_Block[Block+1];
            ShadowIndexA = &FilterList->ShadowSectionFilterArray.CamB_Block[Block];
            ShadowIndexB = &FilterList->ShadowSectionFilterArray.CamB_Block[Block+1];
        }

        for (i = 0; i < SF_FILTER_LENGTH; i++)
        {
            ShadowIndexA->Index[i].Data.Element.Filter[Filter] = FilterData_p->FilterBytes_p[i];
            ShadowIndexA->Index[i].Mask.Element.Filter[Filter] = FilterData_p->FilterMasks_p[i];
        }

        for (i = 0; i < SF_FILTER_LENGTH; i++)
        {
            ShadowIndexB->Index[i].Data.Element.Filter[Filter] = FilterData_p->FilterBytes_p[i+SF_FILTER_LENGTH];
            ShadowIndexB->Index[i].Mask.Element.Filter[Filter] = FilterData_p->FilterMasks_p[i+SF_FILTER_LENGTH];
        }

        /* We only mark the first filter space as enabled/disabled, as the enable for the second one will be ignored */
        TcCam_SetFilterEnableState( TCSessionInfo_p, FilterIdent, TC_FilterMode, FALSE );

        Error = WriteBlock( TC_Params_p, FilterIdent, ShadowIndexA, TCCamIndexA, TC_SectionFilterArrays_p, NotMatchData );
        if(Error != ST_NO_ERROR)
            return(Error);
        Error = WriteBlock( TC_Params_p, FilterIdent, ShadowIndexB, TCCamIndexB, TC_SectionFilterArrays_p, NotMatchData );
        if(Error != ST_NO_ERROR)
            return(Error);

        /* We only mark the first filter space as enabled, as the enable for the second one will be ignored */
        if (FilterData_p->InitialStateEnabled)      /* Re-enable filter, if initial state is enabled */
        {
             TcCam_SetFilterEnableState( TCSessionInfo_p, FilterIdent, TC_FilterMode, TRUE );
        }

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        DumpBlock( TCCamIndex );
        DumpBlock( ShadowIndex );
#endif

    }
    else if ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH )
    {
        Block  = ( (FilterIdent-SessionFilterList->FirstTCFilter) / (SF_NUM_FILTERS_PER_BLOCK/2) );
        Block += SessionFilterList->FirstBlock;
        Filter = ( (FilterIdent-SessionFilterList->FirstTCFilter) % (SF_NUM_FILTERS_PER_BLOCK/2) );
        
        STTBX_Print(("NEG MATCH mode, Block #%d Filter #%d. ", Block, Filter ));

        if ( Filter < SF_CAMLINE_WIDTH )
        {

            TCCamIndexA  = &TC_SectionFilterArrays_p->CamA_Block[Block];
            ShadowIndexA = &FilterList->ShadowSectionFilterArray.CamA_Block[Block];

            TCCamIndexB  = &TC_SectionFilterArrays_p->CamB_Block[Block];
            ShadowIndexB = &FilterList->ShadowSectionFilterArray.CamB_Block[Block];

            AccMask = 0;

            for (i = 0; i < SF_FILTER_LENGTH; i++)
            {
                ShadowIndexA->Index[i].Data.Element.Filter[Filter] = FilterData_p->FilterBytes_p[i];
                ShadowIndexB->Index[i].Data.Element.Filter[Filter] = FilterData_p->FilterBytes_p[i];

                ShadowIndexA->Index[i].Mask.Element.Filter[Filter] = FilterData_p->FilterMasks_p[i] &  ModePattern_p[i];
                ShadowIndexB->Index[i].Mask.Element.Filter[Filter] = FilterData_p->FilterMasks_p[i] & ~ModePattern_p[i];

                AccMask |= ShadowIndexB->Index[i].Mask.Element.Filter[Filter];
            }

            /*  Now, if the Mask for CAM B ends up as zero then we should disable the CAM B filter,
               This is 'almost' impossible from the ST 20 but we can set the filter to look for a
               Table ID ( first byte of section filter ) of 0xFF which can never exist, so the
               filter will never match. This is equivalent to disabling it. While we are setting the
               first byte to 0xff we may as well set them ALL to this value */

            if ( AccMask == 0 )
            {
                for (i = 0; i < SF_FILTER_LENGTH; i++)
                {
                    ShadowIndexB->Index[i].Data.Element.Filter[Filter] = 0xFF;
                    ShadowIndexB->Index[i].Mask.Element.Filter[Filter] = 0xFF;
                }
            }

            TcCam_SetFilterEnableState( TCSessionInfo_p, FilterIdent, TC_FilterMode, FALSE );

            Error = WriteBlock( TC_Params_p, FilterIdent, ShadowIndexB, TCCamIndexB, TC_SectionFilterArrays_p, 0 );
            if(Error != ST_NO_ERROR)
                return(Error);

            Error = WriteBlock( TC_Params_p, FilterIdent, ShadowIndexA, TCCamIndexA, TC_SectionFilterArrays_p, NotMatchData );
            if(Error != ST_NO_ERROR)
                return(Error);

            if (FilterData_p->InitialStateEnabled)      /* Re-enable filter, if initial state is enabled */
            {
                 TcCam_SetFilterEnableState( TCSessionInfo_p, FilterIdent, TC_FilterMode, TRUE );
            }

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
            DumpBlock( TCCamIndexA );
            DumpBlock( TCCamIndexB );
            DumpBlock( ShadowIndexA );
            DumpBlock( ShadowIndexB );
#endif
        }
        else
        {
            return( STPTI_ERROR_INVALID_FILTER_HANDLE );
        }
    }
    else
    {
        return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
    }


    return( ST_NO_ERROR );
}




/* ------------------------------------------------------------------------- */
/*                            Local Functions                                */
/* ------------------------------------------------------------------------- */

#if defined( STPTI_WORKAROUND_SFCAM )
/******************************************************************************
Function Name : WriteBlock
  Description : Write a FilterBlock into the CAMs.
   Parameters :

   This is separated out due to the sensitive nature of loading the CAMs.  In
   the past lots of workarounds have been required.  Thankfully later chips are
   alot more "friendly".

******************************************************************************/
static ST_ErrorCode_t WriteBlock( STPTI_TCParameters_t *TC_Params_p, U32 FilterIdent, TCCamIndex_t *ShadowIndex,
                                   TCCamIndex_t *TCCamIndex, TCSectionFilterArrays_t *TC_SectionFilterArrays_p, U32 NotMatchData )
{
    U32 i;
    ST_ErrorCode_t CAM_status = ST_NO_ERROR;
    TCGlobalInfo_t *TC_Global_p = (TCGlobalInfo_t *) TC_Params_p->TC_GlobalDataStart;
    U32 Value=0, Start=0, Now=0;
    U16 Mask;

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1)
    U32 FilterNotMatchOffset;
   
    /* 7109 ONLY  *** Calculate Filter not match offset for 7109 due to CAM A/B swap after every 4 filters          */
    /* 7109 ONLY  *** Section Not Match filters are in blocks for CAMA and CAMB with offsets of 0x800 and 0x8c0     */
    /* 7109 ONLY  *** We need to allocate Not Match filters 0-3 to CAMA, 4-7 to CAMB, 8-11 to CAMA etc...           */
    /* 7109 ONLY  *** This means the bottom 2 bits of the filter number are correct addresses within each CAM block */
    /* 7109 ONLY  *** The next bit (2) decides between CAM A and CAM B, so it adds an offset of 48 words to give    */
    /* 7109 ONLY  *** 196 bytes between CAM start addresses                                                         */
    /* 7109 ONLY  *** The remaining bits (3-6) of the filter number shift right into the gap left by bit 2          */
    /* 7109 ONLY  *** This generates address offsets which work for 7109 ONLY                                       */
    /* 7109 ONLY  *** NOTE that 7200 will require different re-mapping as the NotMatch Offsets have changed again   */
    
    FilterNotMatchOffset = FilterIdent & 0x03;                    /* Use bottom (0,1) bits of ident as they are */
    if(FilterIdent & 0x04)
        FilterNotMatchOffset += TC_NUMBER_OF_HARDWARE_NOT_FILTERS/2; /* If bit 2 is set, add offset to CAM B block */
    FilterNotMatchOffset += (FilterIdent & 0x78)>>1;              /* Bits 3-6 shift right to fill gap left by bit 2! */
#endif

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
    U32 FilterNotMatchOffset;
    U32 FilterNotMatchModify;

    /* 7200      *** Filter offsets similar to 7109 with CAM A/B swap after every 4 filters, but then shares        */
    /* 7200      *** NMM memory allocation between CAMA and CAMB in LSWords and MSWord of the same 32 bit word      */
    /* 7200      *** This requires a read-modify-write with masking of lower or upper 16 bits for each CAM          */
    
    FilterNotMatchOffset = FilterIdent & 0x03;                      /* Use bottom 2 bits of ident as they are  */
    FilterNotMatchOffset += (FilterIdent & 0x78)>>1;                /* Bits 3-6 shift right to fill gap left by bit 2! */
                                                                    /* Bit 2 is used later to determine upper or lower word */

#endif


    /* Ensure that while we are inhibiting the TC, we are not pre-empted or interrupted */

    STOS_TaskLock();

    STPTI_CRITICAL_SECTION_BEGIN {

        /*  Make request to write CAM registers */
        Mask = STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit);
        Mask |= TC_GLOBAL_DATA_CAM_ARBITER_INHIBIT_REQUEST;
        STSYS_WriteTCReg16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit, Mask );

        /* Need to delay for a few clock cycles.  This is to ensure the following scenario is avoided:

         1. TC reads inhibit as being clear.

         2. The ST20 sets the inhibit and immediately reads the idle value
         and determines the TC is idle.

         3. In the meantime, the TC, having determined that it is not inhibited,
         exits the idle loop and could process a partial packet just in time for
         the ST20 to write CAM and lock itself up. */

        /* Time for TC to ld inhibit and st BUSY flag = (5 * STBUS cycle time).
           Assume STBUS = CPU / 2 and wait 12 CPU cycles */
        /* TN - Added extra delay to allow for Pipeline stalls (TC takes about 10 * STBUS cycles
            instead of 5) and extra pipeline stall introduced when bit RMW used for IDLE instead
            of Word access */

        __ASM_12_NOPS;
        __ASM_12_NOPS;

        Start = time_now();

        /* Poll for idle to be free. Return if free or timed-out If not free in TEN packet periods (of a 20Mbit stream), then drop out */
        do
        {
            /* Until we determine otherwise, assume we're unable to access the CAM */
            CAM_status = STPTI_ERROR_UNABLE_TO_ENABLE_FILTERS;

            Value = STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_DATA_CAM_ARBITER_IDLE_CLEAR;

            if ( Value == (U32)TC_GLOBAL_DATA_CAM_ARBITER_IDLE_CLEAR )   /* Check if CAM available now if so, then break  */
            {
                CAM_status = ST_NO_ERROR;
                break;
            }
                /* Get current time to check if we have timed out... */
            Now = time_now();

        }
        while(( Now - Start ) < (( TC_ONE_PACKET_TIME * 10 ) * TICKS_PER_SECOND ) / 1000000 );

        if( CAM_status == ST_NO_ERROR )  /* Actually set the cam registers */
        {

            for( i = 0; i < SF_FILTER_LENGTH; i++ )
            {
                TCCamIndex->Index[i].Data.Word = ENDIAN_U32_SWAP( ShadowIndex->Index[i].Data.Word );
                TCCamIndex->Index[i].Mask.Word = ENDIAN_U32_SWAP( ShadowIndex->Index[i].Mask.Word );
            }

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1)
            TC_SectionFilterArrays_p->NotFilter[FilterNotMatchOffset] = NotMatchData;
#else
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
            if(FilterIdent & 0x04)          /* CAM B, write to Most Sig. 16 bits */
            {
                FilterNotMatchModify = TC_SectionFilterArrays_p->NotFilter[FilterNotMatchOffset] & 0x0000ffff;
                FilterNotMatchModify |= (NotMatchData<<16) & 0xffff0000;
            }
            else                            /* CAM A, write to Least Sig 16 bits */
            {
                FilterNotMatchModify = TC_SectionFilterArrays_p->NotFilter[FilterNotMatchOffset] & 0xffff0000;
                FilterNotMatchModify |= NotMatchData & 0x0000ffff;
            }
            TC_SectionFilterArrays_p->NotFilter[FilterNotMatchOffset] = FilterNotMatchModify;
#else
            if( FilterIdent < 32 )   /* only first 32 filters are enabled for this (only first session should be able to access this!) */
            {
                TC_SectionFilterArrays_p->NotFilter[FilterIdent] = NotMatchData;
            }
#endif
#endif

            STTBX_Print(("Offset %x, NotMatchData[%d] = 0%02x\n", FilterNotMatchOffset, FilterIdent, NotMatchData));
        }

        /*  Clear request to write CAM registers */
        Mask = STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit);
        Mask &= ~TC_GLOBAL_DATA_CAM_ARBITER_INHIBIT_REQUEST;
        STSYS_WriteTCReg16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit, Mask );

    } STPTI_CRITICAL_SECTION_END;

    STOS_TaskUnlock();
    
    return( CAM_status );
}

#else /* STPTI_WORKAROUND_SFCAM not defined */

/******************************************************************************
Function Name : WriteBlock
  Description : Write a FilterBlock into the CAMs.
   Parameters :

   This is separated out due to the sensitive nature of loading the CAMs.  In
   the past lots of workarounds have been required.  Thankfully later chips are
   alot more "friendly".

******************************************************************************/
static ST_ErrorCode_t WriteBlock( STPTI_TCParameters_t *TC_Params_p, U32 FilterIdent, TCCamIndex_t *ShadowIndex,
                                   TCCamIndex_t *TCCamIndex, TCSectionFilterArrays_t *TC_SectionFilterArrays_p, U32 NotMatchData )
{
    U32 i;
    ST_ErrorCode_t CAM_status = ST_NO_ERROR;

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1)
    U32 FilterNotMatchOffset;
   
    /* 7109 ONLY  *** Calculate Filter not match offset for 7109 due to CAM A/B swap after every 4 filters          */
    /* 7109 ONLY  *** Section Not Match filters are in blocks for CAMA and CAMB with offsets of 0x800 and 0x900     */
    /* 7109 ONLY  *** We need to allocate Not Match filters 0-3 to CAMA, 4-7 to CAMB, 8-11 to CAMA etc...           */
    /* 7109 ONLY  *** This means the bottom 2 bits of the filter number are correct addresses within each CAM block */
    /* 7109 ONLY  *** The next bit (2) decides between CAM A and CAM B, so it shifts to bit position 6 to give      */
    /* 7109 ONLY  *** an offset of 64 long words, 256 bytes between CAMs                                            */
    /* 7109 ONLY  *** The remaining bits (3-6) of the filter number shift right into the gap left by bit 2          */
    /* 7109 ONLY  *** This generates address offsets which work for 7109 ONLY                                       */
    /* 7109 ONLY  *** NOTE that 7200 will require different re-mapping as the NotMatch Offsets have changed again   */
    
    FilterNotMatchOffset = FilterIdent & 0x03;                    /* Use bottom (0,1) bits of ident as they are */
    if(FilterIdent & 0x04)
        FilterNotMatchOffset += TC_NUMBER_OF_HARDWARE_NOT_FILTERS/2; /* If bit 2 is set, add offset to CAM B block */
    FilterNotMatchOffset += (FilterIdent & 0x78)>>1;              /* Bits 3-6 shift right to fill gap left by bit 2! */
#endif

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
    U32 FilterNotMatchOffset;
    U32 FilterNotMatchModify;

    /* 7200      *** Filter offsets similar to 7109 with CAM A/B swap after every 4 filters, but then shares        */
    /* 7200      *** NMM memory allocation between CAMA and CAMB in LSWords and MSWord of the same 32 bit word      */
    /* 7200      *** This requires a read-modify-write with masking of lower or upper 16 bits for each CAM          */
    
    FilterNotMatchOffset = FilterIdent & 0x03;                      /* Use bottom 2 bits of ident as they are  */
    FilterNotMatchOffset += (FilterIdent & 0x78)>>1;                /* Bits 3-6 shift right to fill gap left by bit 2! */
                                                                    /* Bit 2 is used later to determine upper or lower word */

#endif

    if( CAM_status == ST_NO_ERROR )  /* Actually set the cam registers */
    {
        STOS_TaskLock();
        
        STPTI_CRITICAL_SECTION_BEGIN {

            for( i = 0; i < SF_FILTER_LENGTH; i++ )
            {
                TCCamIndex->Index[i].Data.Word = ENDIAN_U32_SWAP( ShadowIndex->Index[i].Data.Word );
                TCCamIndex->Index[i].Mask.Word = ENDIAN_U32_SWAP( ShadowIndex->Index[i].Mask.Word );
            }

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1)
            TC_SectionFilterArrays_p->NotFilter[FilterNotMatchOffset] = NotMatchData;
#else
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
            if(FilterIdent & 0x04)          /* CAM B, write to Most Sig. 16 bits */
            {
                FilterNotMatchModify = TC_SectionFilterArrays_p->NotFilter[FilterNotMatchOffset] & 0x0000ffff;
                FilterNotMatchModify |= (NotMatchData<<16) & 0xffff0000;
            }
            else                            /* CAM A, write to Least Sig 16 bits */
            {
                FilterNotMatchModify = TC_SectionFilterArrays_p->NotFilter[FilterNotMatchOffset] & 0xffff0000;
                FilterNotMatchModify |= NotMatchData & 0x0000ffff;
            }
            TC_SectionFilterArrays_p->NotFilter[FilterNotMatchOffset] = FilterNotMatchModify;
#else
            if( FilterIdent < 32 )   /* only first 32 filters are enabled for this (only first session should be able to access this!) */
            {
                TC_SectionFilterArrays_p->NotFilter[FilterIdent] = NotMatchData;
            }
#endif
#endif

        } STPTI_CRITICAL_SECTION_END;

        STOS_TaskUnlock();
        
        STTBX_Print(("NotMatchData[%d] = %d 0%02x\n", FilterIdent, NotMatchData, NotMatchData));
    }

    return( CAM_status );
}
#endif





/* ------------------------------------------------------------------------- */


/******************************************************************************
Function Name : ClearAllCams
  Description : Clear the CAMs used in TcCam_Initialize() to put them into a
                known (and inactive / nonmatching) state.
   Parameters :

  The TC should *NOT* be running when this function is being called.

******************************************************************************/
static void ClearAllCams( TCSectionFilterArrays_t *SectionFilterArray_p )
{
    int block, index;


    STTBX_Print(("TcCam_ClearAllCams(0x%x)...", (U32)SectionFilterArray_p ));

    for( block = 0; block < SF_NUM_BLOCKS_PER_CAM; block++ )
    {
        for( index = 0; index < SF_FILTER_LENGTH; index++ )
        {
            SectionFilterArray_p->CamA_Block[block].Index[index].Data.Word = 0xffffffff;
            SectionFilterArray_p->CamA_Block[block].Index[index].Mask.Word = 0xffffffff;

            SectionFilterArray_p->CamB_Block[block].Index[index].Data.Word = 0xffffffff;
            SectionFilterArray_p->CamB_Block[block].Index[index].Mask.Word = 0xffffffff;
        }
    }

#if !defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    for( index = 0; index < TC_NUMBER_OF_HARDWARE_NOT_FILTERS; index++ )
    {
        SectionFilterArray_p->NotFilter[index] = 0;
    }
#else
    for( index = 0; index < TC_NUMBER_OF_HARDWARE_NOT_FILTERS/2; index++ )
    {
    /* NOTE - 7109 ONLY - CamA and CamB NotMatch Filters are in blocks with offsets 0x800 and 0x8c0 respectively */
    /* Each filter is a U32, so offset to the CAMB from CAMA is 48words (192bytes = 48 * 4) */
        SectionFilterArray_p->NotFilter[index] = 0;
        SectionFilterArray_p->NotFilter[index+TC_NUMBER_OF_HARDWARE_NOT_FILTERS/2] = 0;      /* Get the CamB filters on 7109 */
    }
#endif

    STTBX_Print(("ok\n"));
}


/******************************************************************************
Function Name : AllocateFilter
  Description : Generic Allocate Filter Routine
   Parameters :

   The return value is the relative index (for the session) of a free filter
   
   Note, we work through the CAMs in pairs of blocks to reduce the risk of 
   fragmentation when mixing short filters with long filters.
   
******************************************************************************/
static U32 AllocateFilter(FilterList_t *FilterList, SessionFilterList_t *SessionFilter, U16 TC_FilterMode)
{
    U32 block_pair, block, filter, filterindex, i, blocks_to_check_per_pair=2;
    
    STTBX_Print(("\n *** AllocateFilter ***   Blocks%d-%d", SessionFilter->FirstBlock, SessionFilter->LastBlock));

    /* For long mode filtering we must allocate only on even block numbers 0,2,4... because it uses 2 blocks per filter */
    if ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_LONG ) blocks_to_check_per_pair=1;
    
    for ( block_pair = SessionFilter->FirstBlock; block_pair <= SessionFilter->LastBlock; block_pair+=2 )
    {
        /* CAM A */
        STTBX_Print(( "\nCamA: " ));
        
        for( filter = 0; filter < SF_CAMLINE_WIDTH; filter++ )
        {
            for(i=0;i<blocks_to_check_per_pair;i++)
            {
                block = block_pair+i;
                if(block > SessionFilter->LastBlock ) break;
                
                STTBX_Print(("B%dF%d... ", block, filter ));
                
                if ( FilterList->CamA[block][filter] == FL_FREE)
                {
                    STTBX_Print(("Free..."));

                    FilterList->CamA[block][filter] = FL_IN_USE;

                    if ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH )
                    {
                        STTBX_Print(("X2..."));
                        if (FilterList->CamB[block][filter] == FL_IN_USE)
                        {
                            FilterList->CamA[block][filter] = FL_FREE;       /* Free up the first filter half we just allocated */
                            continue;             /* second filter space is not free */
                        }
                        FilterList->CamB[block][filter] = FL_IN_USE;
                        filterindex = ((block-SessionFilter->FirstBlock) * (SF_NUM_FILTERS_PER_BLOCK/2)) + filter;
                    }
                    else if ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_LONG )
                    {
                        STTBX_Print(("X2..."));
                        if( block == SessionFilter->LastBlock ) 
                        {
                            FilterList->CamA[block][filter] = FL_FREE;       /* Free up the first filter half we just allocated */
                            return( INVALID_FILTER_INDEX ); /* Cannot allocate on last block as we need successive blocks */
                        }
                        if (FilterList->CamA[block+1][filter] == FL_IN_USE)
                        {
                            FilterList->CamA[block][filter] = FL_FREE;       /* Free up the first filter half we just allocated */
                            continue;           /* second filter space is not free */
                        }
                        FilterList->CamA[block+1][filter] = FL_IN_USE;
                        filterindex = ((block-SessionFilter->FirstBlock) * SF_NUM_FILTERS_PER_BLOCK) + filter;
                    }
                    else
                    {
                        filterindex = ((block-SessionFilter->FirstBlock) * SF_NUM_FILTERS_PER_BLOCK) + filter;
                    }

                    filterindex += SessionFilter->FirstTCFilter;
                    FilterList->FilterMode[filterindex] = TC_FilterMode;

                    STTBX_Print(("index #%d\n", filterindex ));
                    return( filterindex );
                }
            }
        }

        /* CAM B */
        STTBX_Print(( "\nCamB: " ));

        if ( ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_SHORT ) || ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_LONG ) )
        {
            for( filter = 0; filter < SF_CAMLINE_WIDTH; filter++ )
            {
                for(i=0;i<blocks_to_check_per_pair;i++)
                {
                    block = block_pair+i;
                    if(block > SessionFilter->LastBlock ) break;

                    STTBX_Print(("B%dF%d... ", block, filter ));
                
                    if ( FilterList->CamB[block][filter] == FL_FREE)
                    {
                        STTBX_Print(("Free..."));

                        FilterList->CamB[block][filter] = FL_IN_USE;

                        if ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_LONG )
                        {
                            STTBX_Print(("X2..."));
                            if( block == SessionFilter->LastBlock ) 
                            {
                                FilterList->CamB[block][filter] = FL_FREE;       /* Free up the first filter half we just allocated */
                                return( INVALID_FILTER_INDEX ); /* Cannot allocate on last block as we need successive blocks */
                            }
                            if (FilterList->CamB[block+1][filter] == FL_IN_USE) 
                            {
                                FilterList->CamB[block][filter] = FL_FREE;       /* Free up the first filter half we just allocated */
                                continue;           /* second filter space is not free */
                            }
                            FilterList->CamB[block+1][filter] = FL_IN_USE;
                            filterindex = ((block-SessionFilter->FirstBlock) * SF_NUM_FILTERS_PER_BLOCK) + ( filter + SF_CAMLINE_WIDTH );
                        }
                        else
                        {
                            filterindex = ((block-SessionFilter->FirstBlock) * SF_NUM_FILTERS_PER_BLOCK) + ( filter + SF_CAMLINE_WIDTH );
                        }

                        filterindex += SessionFilter->FirstTCFilter;
                        FilterList->FilterMode[filterindex] = TC_FilterMode;

                        STTBX_Print(("index #%d\n", filterindex ));
                        return( filterindex );
                    }
                }
            }
        }
    }

    STTBX_Print(("\nFail: All filters in use!\n"));
    return( INVALID_FILTER_INDEX );
}


/******************************************************************************
Function Name : ConvertFilterType
  Description : Return the Filter Mode from the Filter Type
   Parameters :

******************************************************************************/
U16 ConvertFilterType(STPTI_FilterType_t FilterType)
{
    switch(FilterType)
    {
         
        case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE:
            return( TC_SESSION_INFO_FILTER_TYPE_LONG );
            
        case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE:
            return( TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH );
            
        case STPTI_FILTER_TYPE_SECTION_FILTER:
        case STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE:
        case STPTI_FILTER_TYPE_EMM_FILTER:
        case STPTI_FILTER_TYPE_ECM_FILTER:
            return( TC_SESSION_INFO_FILTER_TYPE_SHORT );

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
            return( TC_SESSION_INFO_FILTER_TYPE_SHORT );
#else
            return( TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH );
#endif
            
                        
        default:
            return( 0 );
    }
}


/******************************************************************************
Function Name : TcCam_RelativeMetaBitMaskPosition
  Description : Returns the relative bit position for the filter of the meta bit mask.
   Parameters : 
******************************************************************************/
U32 TcCam_RelativeMetaBitMaskPosition(FullHandle_t FullFilterHandle)
{

    Filter_t *Filter_p;
    FilterList_t *FilterList;
    U32 RelativeFilterIdent;
    U8  BitPosition;
    U16 TC_FilterMode;
    
    Filter_p = stptiMemGet_Filter(FullFilterHandle);
    RelativeFilterIdent = (Filter_p->TC_FilterIdent) - TcCam_GetFilterOffset(FullFilterHandle);
    
    FilterList = stptiMemGet_PrivateData(FullFilterHandle)->FilterList;
    TC_FilterMode = FilterList->FilterMode[Filter_p->TC_FilterIdent];
    
    if ((TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_SHORT) || (TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_LONG))
    {
        BitPosition = (RelativeFilterIdent % SF_CAMLINE_WIDTH) + ((RelativeFilterIdent / SF_FILTER_LENGTH) * SF_CAMLINE_WIDTH);
    }
    else
    {
        BitPosition = RelativeFilterIdent;
    }

    return BitPosition;
}


/******************************************************************************
Function Name : ConvertNormalizedFilterIndexToAssociateBitmask
  Description : Convert the filter index allocation pattern (msb)0xB1.A1.B0.A0(lsb/1st filter)
                to 0xB1.B0 or 0xA1.A0 where A|B[0|1] is 4 bits (4 filter 'enables')
   Parameters :

   e.g.
   ConvertNormalizedFilterIndexToAssociateBitmask(0x12345678,TRUE)  = 0x00002468  (CAMA)
   ConvertNormalizedFilterIndexToAssociateBitmask(0x12345678,FALSE) = 0x00001357  (CAMB)

******************************************************************************/
static U32 ConvertNormalizedFilterIndexToAssociateBitmask( U32 NormalizedFilterIndex, BOOL IsCamA)
{
    if ( IsCamA )
    {
        return( ((NormalizedFilterIndex & 0x0000000F)) | ((NormalizedFilterIndex & 0x00000F00) >>  4) |
                 ((NormalizedFilterIndex & 0x000F0000) >>  8) | ((NormalizedFilterIndex & 0x0F000000) >>  12));
    }
    else
    {
         return( ((NormalizedFilterIndex & 0x000000F0) >> 4) | ((NormalizedFilterIndex & 0x0000F000) >>  8) |
                 ((NormalizedFilterIndex & 0x00F00000) >>  12) | ((NormalizedFilterIndex & 0xF0000000) >>  16));
    }
}


/******************************************************************************
Function Name : SetFilterAssociation
  Description : Generic Assocate Filter Routine
   Parameters :
******************************************************************************/
static ST_ErrorCode_t SetFilterAssociation( FullHandle_t DeviceHandle, TCSectionFilterInfo_t *TCSectionFilterInfo_p, U32 FilterIndex, BOOL DoAssociation )
{
    Device_t             *Device_p        = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p   = &Device_p->TCPrivateData;
    FilterList_t         *FilterList      = PrivateData_p->FilterList;
    U16                   TC_FilterMode   = FilterList->FilterMode[FilterIndex];

    U32 AssociateFilterMask, NormalizedFilterIndex;
    U8  ShiftValue, Block, Filter;
    BOOL IsCamA = FALSE;

    STTBX_Print((" --- before ---\n"));
    STTBX_Print((" SectionAssociation0  0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0)));
    STTBX_Print((" SectionAssociation1  0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1)));
    STTBX_Print((" SectionAssociation2  0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2)));
    STTBX_Print((" SectionAssociation3  0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3)));
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    STTBX_Print((" SectionAssociation4  0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation4)));
    STTBX_Print((" SectionAssociation5  0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation5)));
    STTBX_Print((" SectionAssociation6  0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation6)));
    STTBX_Print((" SectionAssociation7  0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation7)));
#endif
    STTBX_Print((" -- after ---"));

#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    ShiftValue = 0;             /* For secure lite use absolute offsetting */
#else
    /* Set ShiftValue to the offset into the filter bank for this session */
    ShiftValue = TcCam_GetFilterOffset( DeviceHandle );
#endif
    if ( ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_SHORT ) || ( TC_FilterMode == TC_SESSION_INFO_FILTER_TYPE_LONG ) )
    {
        STTBX_Print((" (short)\n"));

        /* For short and long filtering modes each bit form both cams represent an individual filter, the allocations
        is 4 from A (block N), then 4 from B (block N), 4 from A (block N+1) etc. so that the filter index
        indicates a different cam every 4 filters. The SectionAssociation (U16) TC elements that enable filtering
        are structured as lower 16 bits for cam A, lower 16 bits for cam B, then upper 16 bits for cam A, and upper
        16 bits for cam B. For the filter index to be mapped to this it has to transformed:

         A normalized FilterIndex is a set of 16 filters, 8 from cam A & 8 from cam B. The function
        ConvertNormalizedFilterIndexToAssociateBitmask() transforms this to a U8 depending upon which CAM
        the FilterIndex refers to. This U8 can then be applied to the required cam. */

        Block = (FilterIndex / SF_NUM_FILTERS_PER_BLOCK );
        Filter = ( FilterIndex % SF_NUM_FILTERS_PER_BLOCK );

        if ( Filter < SF_CAMLINE_WIDTH )
        {
            IsCamA = TRUE;
        }

        /* FilterIndex is absolute 0.. index into the filter bank */
        /* ShiftValue is the starting filter for this session */

        /* We want an AssociateFilterMask which takes into account the funny ramcam mapping    */
        /* ConvertNormalizedFilterIndexToAssociateBitmask(0x12345678,TRUE) = 0x00002468   CAMA */
        /* ConvertNormalizedFilterIndexToAssociateBitmask(0x12345678,FALSE) = 0x00001357  CAMB */

        /* We have to deal with the fact that NormalisedFilterIndex is only 32bits */
        if ( (FilterIndex - ShiftValue) < 32 )
		{
        	NormalizedFilterIndex = (1 << (FilterIndex - ShiftValue));
        	AssociateFilterMask = ConvertNormalizedFilterIndexToAssociateBitmask( NormalizedFilterIndex, IsCamA);
        }
#if !defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        else
        {
        	NormalizedFilterIndex = (1 << (FilterIndex -32 - ShiftValue));
        	AssociateFilterMask = ConvertNormalizedFilterIndexToAssociateBitmask( NormalizedFilterIndex, IsCamA);
        	AssociateFilterMask <<=16;
        }

#else
        else if ( (FilterIndex - ShiftValue) < 64 )
        {
        	NormalizedFilterIndex = (1 << (FilterIndex -32 - ShiftValue));
        	AssociateFilterMask = ConvertNormalizedFilterIndexToAssociateBitmask( NormalizedFilterIndex, IsCamA);
        	AssociateFilterMask <<=16;
        }
        else if ( (FilterIndex - ShiftValue) < 96 )
        {
        	NormalizedFilterIndex = (1 << (FilterIndex -64 - ShiftValue));
        	AssociateFilterMask = ConvertNormalizedFilterIndexToAssociateBitmask( NormalizedFilterIndex, IsCamA);
        }
        else
        {
        	NormalizedFilterIndex = (1 << (FilterIndex -96 - ShiftValue));
        	AssociateFilterMask = ConvertNormalizedFilterIndexToAssociateBitmask( NormalizedFilterIndex, IsCamA);
        	AssociateFilterMask <<=16;
        }
#endif

        if ( IsCamA )
        {
            if ( (FilterIndex - ShiftValue) < 64 )
            {
                if ( DoAssociation )
                {
                    if( AssociateFilterMask & 0xFFFF0000 )
                    {
                        AssociateFilterMask = AssociateFilterMask >> 16;

                        /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation1 |= AssociateFilterMask; */
                        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1) & (AssociateFilterMask))
                        {
                            return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                        }
                        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1, (AssociateFilterMask) );
                    }
                    else
                    {
                        /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation0 |= AssociateFilterMask; */
                        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0) & (AssociateFilterMask))
                        {
                            return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                        }
                        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0, (AssociateFilterMask) );
                    }

                }
                else
                {
                    if(AssociateFilterMask&0xFFFF0000)
                    {
                        AssociateFilterMask = AssociateFilterMask >> 16;
                        /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation1 &= ~AssociateFilterMask; */
                        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1, (AssociateFilterMask) );
                    }
                    else
                    {
                        /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation0 &= ~AssociateFilterMask; */
                        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0, (AssociateFilterMask) );
                    }

                }
            }
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
            else            /* Filters > 64 */
            {
                if ( DoAssociation )
                {
                    if( AssociateFilterMask & 0xFFFF0000 )
                    {
                        AssociateFilterMask = AssociateFilterMask >> 16;

                        /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation1 |= AssociateFilterMask; */
                        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation5) & (AssociateFilterMask))
                        {
                            return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                        }
                        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation5, (AssociateFilterMask) );
                    }
                    else
                    {
                        /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation0 |= AssociateFilterMask; */
                        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation4) & (AssociateFilterMask))
                        {
                            return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                        }
                        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation4, (AssociateFilterMask) );
                    }

                }
                else
                {
                    if(AssociateFilterMask&0xFFFF0000)
                    {
                        AssociateFilterMask = AssociateFilterMask >> 16;
                        /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation1 &= ~AssociateFilterMask; */
                        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation5, (AssociateFilterMask) );
                    }
                    else
                    {
                        /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation0 &= ~AssociateFilterMask; */
                        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation4, (AssociateFilterMask) );
                    }

                }
            }
#endif            
        }
        else  /* Cam B */
        {
            if ( (FilterIndex - ShiftValue) < 64 )
            {
                if ( DoAssociation )
                {
                    if(AssociateFilterMask&0xFFFF0000)
                    {
                        AssociateFilterMask = AssociateFilterMask >> 16;
                        /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation3 |= AssociateFilterMask; */
                        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3) & (AssociateFilterMask))
                        {
                            return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                        }
                        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3, (AssociateFilterMask) );
                    }
                    else
                    {
                        /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation2 |= AssociateFilterMask; */
                        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2) & (AssociateFilterMask))
                        {
                            return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                        }
                        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2, (AssociateFilterMask) );
                    }

                }
                else        /* Disassociate */
                {
                    if(AssociateFilterMask&0xFFFF0000)
                    {
                        AssociateFilterMask = AssociateFilterMask >> 16;
                        /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation3 &= ~AssociateFilterMask; */
                        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3, (AssociateFilterMask) );
                    }
                    else
                    {
                        /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation2 &= ~AssociateFilterMask; */
                        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2, (AssociateFilterMask) );
                    }

                }
            }
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
            else           /* Filters > 64 */
            {
                if ( DoAssociation )
                {
                    if(AssociateFilterMask&0xFFFF0000)
                    {
                        AssociateFilterMask = AssociateFilterMask >> 16;
                        /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation3 |= AssociateFilterMask; */
                        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation7) & (AssociateFilterMask))
                        {
                            return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                        }
                        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation7, (AssociateFilterMask) );
                    }
                    else
                    {
                        /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation2 |= AssociateFilterMask; */
                        if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation6) & (AssociateFilterMask))
                        {
                            return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                        }
                        STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation6, (AssociateFilterMask) );
                    }

                }
                else        /* Disassociate */
                {
                    if(AssociateFilterMask&0xFFFF0000)
                    {
                        AssociateFilterMask = AssociateFilterMask >> 16;
                        /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation3 &= ~AssociateFilterMask; */
                        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation7, (AssociateFilterMask) );
                    }
                    else
                    {
                        /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation2 &= ~AssociateFilterMask; */
                        STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation6, (AssociateFilterMask) );
                    }

                }
            }
#endif
        }

        STTBX_Print((" SectionAssociation0 CamA_0 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0) ));
        STTBX_Print((" SectionAssociation1 CamA_1 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1) ));
        STTBX_Print((" SectionAssociation2 CamB_0 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2) ));
        STTBX_Print((" SectionAssociation3 CamB_1 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3) ));
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        STTBX_Print((" SectionAssociation4 CamA_2 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation4) ));
        STTBX_Print((" SectionAssociation5 CamA_3 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation5) ));
        STTBX_Print((" SectionAssociation6 CamB_2 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation6) ));
        STTBX_Print((" SectionAssociation7 CamB_3 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation7) ));
#endif
    }
    else    /* NM mode */
    {
        STTBX_Print((" (nmm)\n"));

        /* NMM is much simpler than short mode, as each filter is a pair from each CAM at the _SAME_ location
          then the associations are linear and do not have the 'ABAB' allocation pattern but 'NNNNN' where
          N refers to both cam A & B */
        NormalizedFilterIndex = FilterIndex - ShiftValue;
        if (NormalizedFilterIndex < 16)
        {
            if ( DoAssociation )
	        {
                /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation0 |= (1 << NormalizedFilterIndex); */
                if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0) & (1 << NormalizedFilterIndex))
                {
                    return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                }
                STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0, (1 << NormalizedFilterIndex) );
            }
	        else        /* Disassociate */
            {
	            /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation0 &= ~(1 << NormalizedFilterIndex); */
                STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0, (1 << NormalizedFilterIndex) );
	        }
        }
        else if (NormalizedFilterIndex < 32)
        {
            if ( DoAssociation )
	        {
                /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation1 |= (1 << (NormalizedFilterIndex - 16)); */
                if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1) & (1 << (NormalizedFilterIndex - 16)))
                {
                    return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                }
                STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1, (1 << (NormalizedFilterIndex - 16)) );
	        }
            else        /* Disassociate */
	        {
                /* TCSectionFilterInfo_p->SFIBF1.SectionAssociation1 &= ~(1 << (NormalizedFilterIndex - 16)); */
                STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1, (1 << (NormalizedFilterIndex - 16)) );
	        }
        }
        else if (NormalizedFilterIndex < 48)
        {
            if ( DoAssociation )
	        {
                /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation2 |= (1 << (NormalizedFilterIndex - 32)); */
                if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2) & (1 << (NormalizedFilterIndex - 32)))
                {
                    return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                }
                STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2, (1 << (NormalizedFilterIndex - 32)) );
	        }
            else        /* Disassociate */
	        {
                /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation2 &= ~(1 << (NormalizedFilterIndex - 32)); */
                STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2, (1 << (NormalizedFilterIndex - 32)) );
	        }
        }
        else
        {
            if ( DoAssociation )
	        {
                /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation3 |= (1 << (NormalizedFilterIndex - 48)); */
                if (STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3) & (1 << (NormalizedFilterIndex - 48)))
                {
                    return( STPTI_ERROR_FILTER_ALREADY_ASSOCIATED );
                }
                STSYS_SetTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3, (1 << (NormalizedFilterIndex - 48)) );
	        }
            else        /* Disassociate */
	        {
                /* TCSectionFilterInfo_p->SFIBF2.SectionAssociation3 &= ~(1 << (NormalizedFilterIndex - 48)); */
                STSYS_ClearTCMask16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3, (1 << (NormalizedFilterIndex - 48)) );
	        }
        }
        STTBX_Print((" SectionAssociation0 CamAB_0 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation0) ));
        STTBX_Print((" SectionAssociation1 CamAB_1 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation1) ));
        STTBX_Print((" SectionAssociation2 CamAB_0 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation2) ));
        STTBX_Print((" SectionAssociation3 CamAB_1 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation3) ));
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        STTBX_Print((" SectionAssociation4 CamAB_2 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation4)));
        STTBX_Print((" SectionAssociation5 CamAB_3 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation5)));
        STTBX_Print((" SectionAssociation6 CamAB_2 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation6)));
        STTBX_Print((" SectionAssociation7 CamAB_3 0x%04x\n", STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionAssociation7)));
#endif
    }

    STTBX_Print((" ------------\n"));

    return( ST_NO_ERROR );

}

#if !defined ( STPTI_BSL_SUPPORT )
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
/******************************************************************************
Function Name : DumpFilterList
  Description : Dumps out the Filter State for each Session
   Parameters :
******************************************************************************/
static void DumpFilterList( TCPrivateData_t *PrivateData_p, STPTI_TCParameters_t *TC_Params_p )
{
    int a;
    FilterList_t        *FilterList        = PrivateData_p->FilterList;
    SessionFilterList_t *SessionFilterList = FilterList->SessionFilterList;

    for( a = 0; a < TC_Params_p->TC_NumberOfSessions; a++ )
    {
        STTBX_Print((" Session %d: ", a));

        switch(  SessionFilterList[a].State )
        {
            case FL_IN_USE:
                STTBX_Print(("IN_USE "));
                break;

            case FL_FREE:
                STTBX_Print(("FREE   "));
                break;

            default:
                STTBX_Print(("???    "));
                break;

        }
        STTBX_Print(("CAM(%02d,%02d) ", SessionFilterList[a].FirstBlock, SessionFilterList[a].LastBlock ));
        STTBX_Print(("TCSF(%02d,%02d)\n", SessionFilterList[a].FirstTCFilter, SessionFilterList[a].LastTCFilter ));
    }
}


/******************************************************************************
Function Name : DumpBlock
  Description : Dumps out a filter block
   Parameters :
******************************************************************************/
static void DumpBlock( TCCamIndex_t *TCCamIndex_p )
{
#ifdef VIRTUAL_PTI_RAMCAM_DEBUG
    int i;


    STTBX_Print(("\n--- Filter Block (0x%08x) ---\n", (U32)TCCamIndex_p ));

    for( i = 0; i < SF_FILTER_LENGTH; i++ )
    {
        STTBX_Print(("Byte %d  0x%08x\n", i, TCCamIndex_p->Index[i].Data.Word ));
    }

    STTBX_Print(("     -----\n"));

    for( i = 0; i < SF_FILTER_LENGTH; i++ )
    {
        STTBX_Print(("Mask %d  0x%08x\n", i, TCCamIndex_p->Index[i].Mask.Word ));
    }

    STTBX_Print(("--------------------\n"));
#endif
}
#endif

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/* EOF  -------------------------------------------------------------------- */
