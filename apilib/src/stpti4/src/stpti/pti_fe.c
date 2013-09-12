/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2007.  All rights reserved.

File Name:   pti_fe.c
Description: API functions to control Frontend features


******************************************************************************/
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif

/* Includes ---------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#endif /* #endif !defined(ST_OSLINUX)  */

#include "sttbx.h"

#include "stcommon.h"
#if defined(STAPIREF_COMPAT)
#else
/* STFAE - Inclusion of stpti.h should be done before to have STPTI_SUPPRESS_CACHING */
#include "stpti.h"
#endif /* STAPIREF_COMPAT */
#include "osx.h"
#include "validate.h"
#include "memget.h"
#if defined(STAPIREF_COMPAT)
#include "stpti.h"
#else
/* STFAE - Inclusion of stpti.h should be done before to have STPTI_SUPPRESS_CACHING */
#endif /* STAPIREF_COMPAT */

#if defined( STPTI_FRONTEND_HYBRID )

#include "frontend.h"

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* These parameters are determined by the hardware */
#define STPTI_MAX_PID_RAM        (0x400)
#define STPTI_PID_ALIGNMENT_SIZE (0x400)

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

static void *stpti_FrontendPIDBaseAdjust( void *Base_p );
static void *stpti_FrontendRAMBaseAdjust( void *Base_p );
static void stpti_FrontendClearSessionPids( FullHandle_t FullFrontendHandle );
static void stpti_FrontendDestroyClone( FullHandle_t FullFrontendHandle );
/* Functions --------------------------------------------------------------- */

/******************************************************************************
Function Name : STPTI_FrontendAllocate
  Description : Allocate a 'Frontend' object
   Parameters : PTIHandle        - Session handle to associcate object to
                FrontendHandle_p - Pointer to hold returned object handle
                FrontendType     - TSIN or SWTS channel?
******************************************************************************/
ST_ErrorCode_t STPTI_FrontendAllocate( STPTI_Handle_t PTIHandle,
                                       STPTI_Frontend_t * FrontendHandle_p,
                                       STPTI_FrontendType_t FrontendType)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t   FullSessionHandle;
    FullHandle_t   FullFrontendHandle;

    Frontend_t     TmpFrontend;

    /* Obtain exclusive access to PTI memory resources */
    STOS_SemaphoreWait( stpti_MemoryLock );

    STTBX_Print(("Calling  STPTI_FrontendAllocate\n"));

    FullSessionHandle.word = PTIHandle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_FrontendAllocate( FullSessionHandle, FrontendHandle_p, FrontendType );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    if ( Error == ST_NO_ERROR )
    {
            Error = stpti_CreateObjectHandle( FullSessionHandle,
                                              OBJECT_TYPE_FRONTEND,
                                              sizeof( Frontend_t ),
                                              &FullFrontendHandle );
        /* load default frontend configuration */
                TmpFrontend.OwnerSession             = PTIHandle;
                TmpFrontend.Handle                   = FullFrontendHandle.word;
                TmpFrontend.Type                     = FrontendType;
                TmpFrontend.IsAssociatedWithHW       = FALSE;
                TmpFrontend.PIDTableStart_p          = NULL;
                TmpFrontend.PIDTableRealStart_p      = NULL;
                TmpFrontend.PIDFilterEnabled         = TRUE;
                TmpFrontend.SessionListHandle.word   = STPTI_NullHandle();
                TmpFrontend.CloneFrontendHandle.word = STPTI_NullHandle();
                TmpFrontend.RAMStart_p               = NULL;
                TmpFrontend.RAMRealStart_p           = NULL;
                TmpFrontend.AllocatedNumOfPkts       = 0;
                TmpFrontend.WildCardPidCount         = 0;
        TmpFrontend.UserParams.PktLength     = STPTI_FRONTEND_PACKET_LENGTH_DVB;

        if ( STPTI_FRONTEND_TYPE_TSIN == TmpFrontend.Type )
        {
            U8 *           PidTable_p = NULL;

            STTBX_Print(("STPTI_FrontendAllocate - Allocating TSIN object\n"));
            PidTable_p = STOS_MemoryAllocate( stptiMemGet_Session( FullSessionHandle )->DriverPartition_p,
                                              STPTI_MAX_PID_RAM + STPTI_PID_ALIGNMENT_SIZE -1 );
            if ( PidTable_p != NULL )
            {
                /*
                 *  We need to allocate memory for the STFE PID block to use for pid searching
                 *  Data must be aligned to the size of the PID table. On the 7200  PID width is 13 bits
                 *  which means 8192 locations of 1 bit. i.e. a size of 1024 bytes of memory required.
                 */

                /*
                 *  Create the PID table in the partition allocated for it when the
                 *  session was opened. Overallocate to allow for alignment issues.
                 */
                    TmpFrontend.PIDTableStart_p                     = (U8 *) stpti_FrontendPIDBaseAdjust( PidTable_p );
                    TmpFrontend.PIDTableRealStart_p                 = PidTable_p;
                    TmpFrontend.UserParams.u.TSIN.SyncLDEnable      = FALSE;
                    TmpFrontend.UserParams.u.TSIN.SerialNotParallel = FALSE;
                    TmpFrontend.UserParams.u.TSIN.AsyncNotSync      = FALSE;
                    TmpFrontend.UserParams.u.TSIN.AlignByteSOP      = FALSE;
                    TmpFrontend.UserParams.u.TSIN.InvertTSClk       = FALSE;
                    TmpFrontend.UserParams.u.TSIN.IgnoreErrorInByte = FALSE;
                    TmpFrontend.UserParams.u.TSIN.IgnoreErrorInPkt  = FALSE;
                    TmpFrontend.UserParams.u.TSIN.IgnoreErrorAtSOP  = FALSE;
                    TmpFrontend.UserParams.u.TSIN.InputBlockEnable  = FALSE;
                    TmpFrontend.UserParams.u.TSIN.MemoryPktNum      = 0;
                    TmpFrontend.UserParams.u.TSIN.ClkRvSrc          = STPTI_FRONTEND_CLK_RCV0;
                }
                else
                {
                Error = ST_ERROR_NO_MEMORY;
            }
        }
        else if ( STPTI_FRONTEND_TYPE_SWTS == TmpFrontend.Type )
        {
            STTBX_Print(("STPTI_FrontendAllocate - Allocating SWTS object\n"));
            TmpFrontend.UserParams.u.SWTS.PaceBitRate      = 0;
        }

        if ( Error == ST_NO_ERROR )
        {
            memcpy( ( U8 * ) stptiMemGet_Frontend( FullFrontendHandle ), ( U8 * ) &TmpFrontend, sizeof( Frontend_t ));
#if !defined ( STPTI_BSL_SUPPORT )
                Error = stpti_FrontendHandleCheck( FullFrontendHandle );
#endif
            if ( ST_NO_ERROR == Error )
            {
                *FrontendHandle_p = FullFrontendHandle.word;
            }
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

/******************************************************************************
Function Name : STPTI_FrontendDeallocate
  Description : Deallocate `Frontend` object
   Parameters : FrontendHandle
******************************************************************************/
ST_ErrorCode_t STPTI_FrontendDeallocate( STPTI_Frontend_t FrontendHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullFrontendHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullFrontendHandle.word = FrontendHandle;

    Error = stptiValidate_FrontendHandle( FullFrontendHandle );
    if ( Error == ST_NO_ERROR )
    {
        if ( (stptiMemGet_Frontend( FullFrontendHandle ))->Type == STPTI_FRONTEND_TYPE_TSIN )
        {
            stpti_FrontendDestroyClone ( FullFrontendHandle );
        }

        Error = stpti_DeallocateFrontend( FullFrontendHandle, TRUE );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

/******************************************************************************
Function Name : STPTI_FrontendGetParams
  Description : Copy the current settings for the selected Frontend input to
                to provided memory pointer
   Parameters : FrontendHandle
                FrontendType_p
                FrontendParams_p
******************************************************************************/
ST_ErrorCode_t STPTI_FrontendGetParams ( STPTI_Frontend_t FrontendHandle,
                                         STPTI_FrontendType_t * FrontendType_p,
                                         STPTI_FrontendParams_t * FrontendParams_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullFrontendHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    FullFrontendHandle.word = FrontendHandle;
#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_FrontendGetParams( FullFrontendHandle, FrontendType_p, FrontendParams_p );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if( Error == ST_NO_ERROR )
    {
        memcpy ( FrontendParams_p, (void *)&(stptiMemGet_Frontend( FullFrontendHandle ))->UserParams, sizeof( STPTI_FrontendParams_t ) );
        *FrontendType_p = (stptiMemGet_Frontend( FullFrontendHandle ))->Type;
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}

/******************************************************************************
Function Name : STPTI_FrontendGetStatus
  Description : Return status information for the selected frontend object
                This function will error if not used with a TSIN type object
   Parameters : FrontendHandle
                FrontendStatus_p
******************************************************************************/
ST_ErrorCode_t STPTI_FrontendGetStatus ( STPTI_Frontend_t FrontendHandle, STPTI_FrontendStatus_t * FrontendStatus_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t   FullFrontendHandle;
    FullHandle_t   FullSessionHandle;
    FullHandle_t   FullSessionListHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    FullFrontendHandle.word = FrontendHandle;
#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_FrontendGetStatus( FullFrontendHandle, FrontendStatus_p );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if( Error == ST_NO_ERROR )
    {
        Frontend_t * Frontend_p = stptiMemGet_Frontend( FullFrontendHandle );

        FullSessionListHandle = Frontend_p->SessionListHandle;
        if ( STPTI_FRONTEND_TYPE_TSIN != Frontend_p->Type )
        {
            Error = STPTI_ERROR_INVALID_FRONTEND_TYPE;
        }
        else if ( FullSessionListHandle.word == STPTI_NullHandle() )
        {
            Error = STPTI_ERROR_FRONTEND_NOT_LINKED;
        }
        else
        {
            /* Does not matter if the frontend has been cloned as we can figure out the channel from the streamID */
            FullSessionHandle.word = ( &stptiMemGet_List( FullSessionListHandle )->Handle )[0];
            Error = stptiHAL_FrontendGetStatus( FullFrontendHandle, FrontendStatus_p, (stptiMemGet_Device(FullSessionHandle))->StreamID );
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}

/*****************************************************************************************************
Function Name : STPTI_FrontendInjectData
  Description : Injection of data to a SWTS frontend object
                This function will error if not used with a SWTS type object
   Parameters : FrontendHandle
                STPTI_FrontendSWTSNode_p - a pointer to an array of STPTI_FrontendSWTSNode_t
                NumberOfSWTSNodes - the number of elements in the array of STPTI_FrontendSWTSNode_t
******************************************************************************************************/
ST_ErrorCode_t STPTI_FrontendInjectData( STPTI_Frontend_t FrontendHandle, STPTI_FrontendSWTSNode_t *FrontendSWTSNode_p, U32 NumberOfSWTSNodes )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t   FullFrontendHandle;
    FullHandle_t   FullSessionHandle;
    FullHandle_t   FullSessionListHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    FullFrontendHandle.word = FrontendHandle;
#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_FrontendInjectData( FullFrontendHandle, FrontendSWTSNode_p ); /* PJW Do we need to check the number of nodes? */
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if( Error == ST_NO_ERROR )
    {
        Frontend_t * Frontend_p = stptiMemGet_Frontend( FullFrontendHandle );

        FullSessionListHandle = Frontend_p->SessionListHandle;
        if ( STPTI_FRONTEND_TYPE_SWTS != Frontend_p->Type )
        {
            Error = STPTI_ERROR_INVALID_FRONTEND_TYPE;
            /* Release the semaphore so the other children can play - will change later see comment */
            STOS_SemaphoreSignal(stpti_MemoryLock);

        }
        else if ( FullSessionListHandle.word == STPTI_NullHandle() )
        {
            Error = STPTI_ERROR_FRONTEND_NOT_LINKED;
            /* Release the semaphore so the other children can play - will change later see comment */
            STOS_SemaphoreSignal(stpti_MemoryLock);
        }
        else
        {

            /* Does not matter if the frontend has been cloned as we can figure out the channel from the streamID */
            FullSessionHandle.word = ( &stptiMemGet_List( FullSessionListHandle )->Handle )[0];

            /* 2do We probably need to release the stpti_MemoryLock here and will need a new semaphore for each SWTS channel which will be locked here too
               these semaphores will need to be taken before a SWTS */

            /* Release the semaphore so the other children can play - will change later see comment */
            STOS_SemaphoreSignal(stpti_MemoryLock);

            Error = stptiHAL_FrontendInjectData( FullFrontendHandle, FrontendSWTSNode_p, NumberOfSWTSNodes, (stptiMemGet_Device(FullSessionHandle))->StreamID );
            /* Release the SWTS semaphore here */
        }
    }
    else
    {
        STOS_SemaphoreSignal(stpti_MemoryLock);
    }
    return Error;
}

/******************************************************************************
Function Name : STPTI_FrontendLinkToPTI
  Description : Associate the Frontend object to a PTI session
   Parameters : FrontendHandle
                PTIHandle
******************************************************************************/
ST_ErrorCode_t STPTI_FrontendLinkToPTI( STPTI_Frontend_t FrontendHandle, STPTI_Handle_t PTIHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t   FullFrontendHandle;
    FullHandle_t   FullSessionHandle;
    FullHandle_t   SessionListHandle;
    FullHandle_t   FrontendListHandle;
    FullHandle_t   FullPrimaryFrontendHandle;
    BOOL           Clone = FALSE;
    STPTI_FrontendType_t     FrontendType;

    STOS_SemaphoreWait( stpti_MemoryLock );

    STTBX_Print(("Calling STPTI_FrontendLinkToPTI\n"));

    FullFrontendHandle.word = FrontendHandle;
    FullSessionHandle.word  = PTIHandle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_FrontendLinkToPTI( FullFrontendHandle, FullSessionHandle );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    if ( Error == ST_NO_ERROR )
    {
        Device_t   *DeviceStruct_p;
        U32         DeviceNum;

        FrontendType   =(stptiMemGet_Frontend( FullFrontendHandle ))->Type;
        DeviceStruct_p = stptiMemGet_Device(FullSessionHandle);
        /* If Frontend already has a session linked to it then error */
        SessionListHandle = (stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle;
        if ( (SessionListHandle.word != STPTI_NullHandle()) || (STPTI_NullHandle() != DeviceStruct_p->FrontendListHandle.word))
        {
            Error = STPTI_ERROR_FRONTEND_ALREADY_LINKED;
        }
        else if(  ((FrontendType == STPTI_FRONTEND_TYPE_TSIN) && ((DeviceStruct_p->StreamID >> 8)& STPTI_STREAM_IDTYPE_SWTS)) ||
                  ((FrontendType == STPTI_FRONTEND_TYPE_SWTS) && ((DeviceStruct_p->StreamID >> 8)& STPTI_STREAM_IDTYPE_TSIN)) )
        {
            STTBX_Print(("STPTI_FrontendLinkToPTI invalid Type - Device %x Frontend %x\n",DeviceStruct_p->StreamID, FrontendType ));
            Error = STPTI_ERROR_INVALID_FRONTEND_TYPE;
        }
        /* We need to check the packetlength against the PTI type before linking to a pti session. If they don't match then we should error */
        else if ( (STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB == DeviceStruct_p->TCCodes) && (STPTI_FRONTEND_PACKET_LENGTH_DVB != (stptiMemGet_Frontend( FullFrontendHandle ))->UserParams.PktLength) )
        {
            STTBX_Print(("STPTI_FrontendLinkToPTI Incompatible PktLength setting PTI = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB\n"));
            Error = ST_ERROR_BAD_PARAMETER;
        }
        if (ST_NO_ERROR == Error)
        {
            /* Check to see if there is another frontend associated a session using the same stream ID */
            for ( DeviceNum = 0; DeviceNum < stpti_GetNumberOfExistingDevices(); ++DeviceNum )
            {
                Device_t *Device_p = stpti_GetDevice( DeviceNum );

                if ( (Device_p != NULL) && (DeviceStruct_p != Device_p))
                {

                    if ( ((Device_p->StreamID & 0xFF7F) == (DeviceStruct_p->StreamID & 0xFF7F) ) && (STPTI_NullHandle() != Device_p->FrontendListHandle.word) )
                    {
                        if (FrontendType == STPTI_FRONTEND_TYPE_TSIN)
                        {
                            /* Another frontend is linked to the same stream ID so we must clone the input*/
                            Clone = TRUE;
                            FullPrimaryFrontendHandle.word = ( &stptiMemGet_List( Device_p->FrontendListHandle )->Handle )[0];
                        }
                        else
                        {
                            /* Clones not allowed for STPTI_FRONTEND_TYPE_SWTS */
                            Error = STPTI_ERROR_FRONTEND_ALREADY_LINKED;
                        }
                        break;
                    }
                }
            }

            if ( Error == ST_NO_ERROR )
            {
                if (FrontendType == STPTI_FRONTEND_TYPE_TSIN)
                {
                    STTBX_Print(("STPTI_FrontendLinkToPTI - Linking for TSIN type\n"));
                    if ( TRUE == Clone )
                    {
                        /* Associate the two frontend objects */
                        (stptiMemGet_Frontend( FullFrontendHandle ))->CloneFrontendHandle            = FullPrimaryFrontendHandle;
                        (stptiMemGet_Frontend( FullPrimaryFrontendHandle ))->CloneFrontendHandle     = FullFrontendHandle;

                        (stptiMemGet_Frontend( FullFrontendHandle ))->IsAssociatedWithHW             = FALSE;

                        /* Configure the destination register of the TSGDMA */
                        Error = stptiHAL_FrontendCloneInput( FullFrontendHandle, FullPrimaryFrontendHandle );
                    }
                    else
                    {
                        (stptiMemGet_Frontend( FullFrontendHandle ))->IsAssociatedWithHW             = TRUE;

                    /* If we have setup the parameters with STPTI_Frontend_SetParams() then configure the hardware.
                       It is possible that the user has set the params but set InputBlockEnable false this is still
                       OK as the SetParams API will configure when re-enabled */
                        if ( TRUE == ( stptiMemGet_Frontend( FullFrontendHandle ))->UserParams.u.TSIN.InputBlockEnable )
                        {
                            Error = stptiHAL_TSINLinkToPTI( FullFrontendHandle, FullSessionHandle );
                            if (ST_NO_ERROR == Error)
                            {
                               stptiHAL_SyncFrontendPids( FullFrontendHandle, FullSessionHandle );
                            }
                        }
                    }
                }
                else
                {
                    STTBX_Print(("STPTI_FrontendLinkToPTI - Linking for SWTS type\n"));
                    stptiHAL_SWTSLinkToPTI();
                    stptiHAL_FrontendSWTSSetPace( FullSessionHandle, ( stptiMemGet_Frontend( FullFrontendHandle ))->UserParams.u.SWTS.PaceBitRate );
                }

                /* Now associate the objects together */
                if ( Error == ST_NO_ERROR )
                {
                    SessionListHandle = ( stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle;

                    if ( SessionListHandle.word == STPTI_NullHandle())
                    {
                        Error = stpti_CreateListObject( FullFrontendHandle, &SessionListHandle );
                        if ( Error == ST_NO_ERROR )
                        {
                            ( stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle = SessionListHandle;
                        }
                    }

                    if ( Error == ST_NO_ERROR )
                    {
                        FrontendListHandle = ( stptiMemGet_Device( FullSessionHandle ))->FrontendListHandle;

                        if ( FrontendListHandle.word == STPTI_NullHandle())
                        {
                            Error = stpti_CreateListObject( FullSessionHandle, &FrontendListHandle );
                            if ( Error == ST_NO_ERROR )
                            {
                                (stptiMemGet_Device( FullSessionHandle ))->FrontendListHandle = FrontendListHandle;
                            }
                        }
                    }

                    if ( Error == ST_NO_ERROR )
                    {
                      Error = stpti_AssociateObjects( FrontendListHandle, FullSessionHandle, SessionListHandle, FullFrontendHandle );
                    }
                }
                /* Make sure we sync the PID table */
                if ( (TRUE == Clone) && (ST_NO_ERROR == Error) && (FrontendType == STPTI_FRONTEND_TYPE_TSIN))
                {
                    stptiHAL_SyncFrontendPids( FullFrontendHandle, FullSessionHandle );
                }
            }
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

/******************************************************************************
Function Name : STPTI_FrontendReset
  Description : Reset the selected Frontend object input block.
                Frontend object must be associated with a valid PTI session
   Parameters : STPTI_Frontend_GetStatus
******************************************************************************/
ST_ErrorCode_t STPTI_FrontendReset( STPTI_Frontend_t FrontendHandle )
{
    ST_ErrorCode_t    Error = ST_NO_ERROR;
    FullHandle_t      FullFrontendHandle;
    FullHandle_t      FullSessionHandle;
    FullHandle_t      SessionListHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullFrontendHandle.word = FrontendHandle;

    Error = stptiValidate_FrontendHandle( FullFrontendHandle );
    if ( Error == ST_NO_ERROR )
    {
        SessionListHandle = ( stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle;

        if ( (stptiMemGet_Frontend( FullFrontendHandle ))->Type != STPTI_FRONTEND_TYPE_TSIN )
        {
            Error = STPTI_ERROR_INVALID_FRONTEND_TYPE;
        }
        else if ( SessionListHandle.word == STPTI_NullHandle() )
        {
            Error = STPTI_ERROR_FRONTEND_NOT_LINKED;
        }
        else
        {
            /* Does not matter if the frontend has been cloned as we can figure out the channel from the streamID */
            FullSessionHandle.word = ( &stptiMemGet_List( SessionListHandle )->Handle )[0];
            Error = stptiHAL_FrontendReset( FullFrontendHandle,  (stptiMemGet_Device(FullSessionHandle))->StreamID );
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name : STPTI_FrontendSetParams
  Description : Config the Frontend Object for stream input data
                Also will allocate the memory requested for the DMA transfer
   Parameters : FrontendHandle
                FrontendParams_p
******************************************************************************/
ST_ErrorCode_t STPTI_FrontendSetParams ( STPTI_Frontend_t FrontendHandle, STPTI_FrontendParams_t * FrontendParams_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t   FullFrontendHandle;
    FullHandle_t   FullSessionHandle;
    FullHandle_t   FullSessionListHandle;
    U8 *           OldRamToFree = NULL;
    Device_t *     Device_p;

    STOS_SemaphoreWait(stpti_MemoryLock);

    STTBX_Print(("Calling STPTI_FrontendSetParams\n"));

    FullFrontendHandle.word = FrontendHandle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_FrontendSetParams( FullFrontendHandle, FrontendParams_p );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    if( Error == ST_NO_ERROR )
    {
        U8 * RAMBuffer_p;
        U32  ActualNumOfPkts = 0;
        U32  AdjustedPktLen  = 0;
        Frontend_t * Frontend_p = stptiMemGet_Frontend( FullFrontendHandle );

        Device_p = stptiMemGet_Device(FullFrontendHandle);

        /* We need to check the packetlength against the PTI type if linked to a pti session. If they don't match then we should error */
        if (STPTI_NullHandle() != Frontend_p->SessionListHandle.word)
        {
            STTBX_Print(("STPTI_FrontendSetParams Checking PktLength against sessions type\n"));
            if ( (STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB == Device_p->TCCodes) && (STPTI_FRONTEND_PACKET_LENGTH_DVB != FrontendParams_p->PktLength) )
            {
                Error = ST_ERROR_BAD_PARAMETER;
            }
        }

        if ( ST_NO_ERROR == Error )
        {

        /* Allocate memory for the DMA as required if TSIN input */
        if ( (STPTI_FRONTEND_TYPE_TSIN == Frontend_p->Type) && (Frontend_p->AllocatedNumOfPkts !=  FrontendParams_p->u.TSIN.MemoryPktNum) )
        {
            /* DMA transfers in 32 byte blocks. Thus packet size must be a multiple of 8 bytes and buffer size a multiple of 4 packets. */
            /* For DVB                       = 188 bytes packet + 8 bytes Tag + 4 bytes = 200 */

            STTBX_Print(("STPTI_FrontendSetParams attempting to allocate memory for %u packets\n", FrontendParams_p->u.TSIN.MemoryPktNum));

            /* Free the previously allocated memory if necessary */
            if ( Frontend_p->AllocatedNumOfPkts)
            {
                STTBX_Print(("STPTI_FrontendSetParams freeing memory for existing %u packets\n", Frontend_p->AllocatedNumOfPkts ));
                    OldRamToFree = Frontend_p->RAMRealStart_p;
                Frontend_p->RAMStart_p              = NULL;
                Frontend_p->RAMRealStart_p          = NULL;
                Frontend_p->AllocatedNumOfPkts      = 0;
            }

            if ( FrontendParams_p->u.TSIN.MemoryPktNum % 8 )
            {
                /* Round up to a multiple of 8 packets - Datasheet suggests a multiple of 4 packets which is fine if you allocate twice for Base and NextBase i.e. MOD32
                   but as we divide the memory allocated by 2 for the buffer pointers the total memory needs to MOD 64 = 0 */
                ActualNumOfPkts = (FrontendParams_p->u.TSIN.MemoryPktNum +   (8 - (FrontendParams_p->u.TSIN.MemoryPktNum % 8)));
            }
            else
            {
                ActualNumOfPkts = FrontendParams_p->u.TSIN.MemoryPktNum;
            }

            if ( FrontendParams_p->PktLength % 8 )
            {
                /* Adjust the pkt len for a multiple of 8 bytes */
                AdjustedPktLen = (FrontendParams_p->PktLength  + 8 + (8 - (FrontendParams_p->PktLength  % 8)));
            }
            else
            {
                AdjustedPktLen = (FrontendParams_p->PktLength  + 8);
            }

            STTBX_Print(("Actual Num of TS packets Allocated: %04u\n", ActualNumOfPkts));
            STTBX_Print(("Adjusted TS packet Len for STFE   : %04u\n", AdjustedPktLen));

            /* Memory should be aligned to 32 bytes for bus addresses so overallocate & then re-align */
            RAMBuffer_p = STOS_MemoryAllocate( stptiMemGet_Session( FullFrontendHandle )->DriverPartition_p,
                                               (ActualNumOfPkts * AdjustedPktLen)
                                               +  STPTI_BUFFER_ALIGN_MULTIPLE - 1 );
            if ( NULL == RAMBuffer_p )
            {
                Error = ST_ERROR_NO_MEMORY;
            }
            else
            {
                /* Now store the pointers  */
                Frontend_p->RAMStart_p                = (U8*) stpti_FrontendRAMBaseAdjust( RAMBuffer_p );
                Frontend_p->RAMRealStart_p            = RAMBuffer_p;
                Frontend_p->AllocatedNumOfPkts        = ActualNumOfPkts;
                FrontendParams_p->u.TSIN.MemoryPktNum = ActualNumOfPkts;
#if defined (STTBX_PRINT)
                if ( ((U32)(Frontend_p->RAMStart_p)) % STPTI_BUFFER_ALIGN_MULTIPLE )
                {
                    STTBX_Print(("STPTI_FrontendSetParams - Ram Buffer address not %u byte aligned 0x%08x\n", STPTI_BUFFER_ALIGN_MULTIPLE, (U32)(Frontend_p->RAMStart_p) ));
                }
#endif
            }
        }
        else if (STPTI_FRONTEND_TYPE_TSIN == Frontend_p->Type)
        {
            STTBX_Print(("STPTI_FrontendSetParams packet number unchanged no need to re-allocate memory\n"));
        }

        if (ST_NO_ERROR == Error)
        {
            FullSessionListHandle = (stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle;

            if (STPTI_FRONTEND_TYPE_TSIN == Frontend_p->Type)
            {
                if ( (STPTI_NullHandle() != (stptiMemGet_Frontend( FullFrontendHandle ))->CloneFrontendHandle.word) &&
                     (TRUE == (stptiMemGet_Frontend( (stptiMemGet_Frontend( FullFrontendHandle ))->CloneFrontendHandle ))->IsAssociatedWithHW) )
                {
                    /* This frontend object is a clone so configure the primary object's settings instead by replacing the Frontend handle */
                    FullFrontendHandle = (stptiMemGet_Frontend( FullFrontendHandle ))->CloneFrontendHandle;
                }

                /* Update the stored configuration */
                memcpy ( (void*)&((stptiMemGet_Frontend( FullFrontendHandle ))->UserParams) ,FrontendParams_p, sizeof ( STPTI_FrontendParams_t ));

                /* If the Frontend is linked then setup the hardware otherwise just copy the settings */
                if ( (FullSessionListHandle.word != STPTI_NullHandle()) && (TRUE == FrontendParams_p->u.TSIN.InputBlockEnable))
                {
                    FullSessionHandle.word = ( &stptiMemGet_List( FullSessionListHandle )->Handle )[0];

                    stptiHAL_FrontendSetParams( FullFrontendHandle, FullSessionHandle );
                    stptiHAL_SyncFrontendPids ( FullFrontendHandle, FullSessionHandle );
                }
            }
            else if (STPTI_FRONTEND_TYPE_SWTS == Frontend_p->Type)
            {
                /* Update the stored configuration */
                memcpy ( (void*)&((stptiMemGet_Frontend( FullFrontendHandle ))->UserParams) ,FrontendParams_p, sizeof ( STPTI_FrontendParams_t ));

                /* If the Frontend is linked then update the pacing rate */
                if (FullSessionListHandle.word != STPTI_NullHandle())
                {
                    FullSessionHandle.word = ( &stptiMemGet_List( FullSessionListHandle )->Handle )[0];
                    stptiHAL_FrontendSWTSSetPace( FullSessionHandle, FrontendParams_p->u.SWTS.PaceBitRate );
                }
            }
        }
        }
    }

    if  (NULL != OldRamToFree )
    {
        STOS_MemoryDeallocate( stptiMemGet_Session( FullFrontendHandle )->DriverPartition_p, OldRamToFree );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}

/******************************************************************************
Function Name : STPTI_FrontendUnLink
  Description : Disassociate the Frontend object from its PTI session which
                will disable the STFE Input block if its the only object linked
                to the IB block
   Parameters : FrontendHandle
******************************************************************************/
ST_ErrorCode_t STPTI_FrontendUnLink ( STPTI_Frontend_t FrontendHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t   FullFrontendHandle;
    FullHandle_t   FullSessionHandle;
    FullHandle_t   FullSessionListHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullFrontendHandle.word = FrontendHandle;

    Error = stptiValidate_FrontendHandle( FullFrontendHandle );
    if ( Error == ST_NO_ERROR )
    {
        FullSessionListHandle = ( stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle;
        if ( FullSessionListHandle.word == STPTI_NullHandle() )
        {
            Error = STPTI_ERROR_FRONTEND_NOT_LINKED;
        }
        /* If this is a primary cloned input (TSIN only) then convert the other to the primary */
        else
        {
            stpti_FrontendDestroyClone( FullFrontendHandle );
        }

        if ( Error == ST_NO_ERROR )
        {
            /* Now disable the Frontend or SWTS channel */
            FullSessionHandle.word = ( &stptiMemGet_List( FullSessionListHandle )->Handle )[0];
            Error = stpti_FrontendDisassociatePTI( FullFrontendHandle, FullSessionHandle, FALSE );
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

/******************************************************************************
Function Name :stpti_FrontendPIDBaseAdjust
  Description :
   Parameters :
******************************************************************************/
static void *stpti_FrontendPIDBaseAdjust( void *Base_p )
{
    return ( void * ) ((( U32 ) Base_p + STPTI_PID_ALIGNMENT_SIZE - 1 ) &
                                     ( ~( STPTI_PID_ALIGNMENT_SIZE - 1 )));
}

/******************************************************************************
Function Name :stpti_BufferBaseAdjust
  Description :
   Parameters :
******************************************************************************/
static void *stpti_FrontendRAMBaseAdjust( void *Base_p )
{
    return ( void * ) ((( U32 ) Base_p + STPTI_BUFFER_ALIGN_MULTIPLE - 1 ) &
                                     ( ~( STPTI_BUFFER_ALIGN_MULTIPLE - 1 )));
}


/******************************************************************************
Function Name :stpti_FrontendClearSessionPids
  Description : Clear all the PIDs on the stfe channel for the PTI session.
                Used when unlinking a session
   Parameters :
******************************************************************************/
static void stpti_FrontendClearSessionPids( FullHandle_t FullFrontendHandle )
{
    FullHandle_t SlotListHandle = stptiMemGet_Session( FullFrontendHandle )->AllocatedList[OBJECT_TYPE_SLOT];
    FullHandle_t SlotHandle;
    U16          MaxSlots       = stptiMemGet_List( SlotListHandle )->MaxHandles;
    U16          SlotIndex      = 0;

    /*
     * Iterate over all allocated slots until we run out of Slots.
     */
    while ( SlotIndex < MaxSlots )
    {
        SlotHandle.word = ( &stptiMemGet_List( SlotListHandle )->Handle )[SlotIndex];

        if ( SlotHandle.word != STPTI_NullHandle() )
        {
            stptiHAL_FrontendClearPid( SlotHandle, (stptiMemGet_Slot( SlotHandle ))->Pid );
        }
        ++SlotIndex;
    }
}


/******************************************************************************
Function Name : stpti_FrontendDestroyClone
  Description : When a frontend is cloned break links and update remaining frontend

   Parameters : FullFrontendHandle
******************************************************************************/
static void stpti_FrontendDestroyClone( FullHandle_t FullFrontendHandle )
{
    FullHandle_t   FullSessionHandle;
    FullHandle_t   FullSessionListHandle;
    U32            ResyncPIDs = FALSE;

    FullSessionListHandle = ( stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle;
    if ( FullSessionListHandle.word != STPTI_NullHandle() )
    {
        /* If this is a primary cloned input (TSIN only) then convert the secondary to the primary */
        if ( (TRUE == (stptiMemGet_Frontend(FullFrontendHandle))->IsAssociatedWithHW) &&
                  (STPTI_NullHandle() != (stptiMemGet_Frontend( FullFrontendHandle ))->CloneFrontendHandle.word) )
        {
            U8 * TmpMemory_p;
            U32 Tmp;
            Frontend_t * PrimaryFrontend_p   = stptiMemGet_Frontend( FullFrontendHandle );
            Frontend_t * SecondaryFrontend_p = stptiMemGet_Frontend( (stptiMemGet_Frontend( FullFrontendHandle ))->CloneFrontendHandle );

            if ( (!(PrimaryFrontend_p->WildCardPidCount)) && (PrimaryFrontend_p->PIDFilterEnabled = TRUE) )
            {
                stpti_FrontendClearSessionPids( FullFrontendHandle );
            }
            if ( (PrimaryFrontend_p->WildCardPidCount) && !(SecondaryFrontend_p->WildCardPidCount))
            {
                ResyncPIDs = TRUE;
            }

            /* Switch primary status to secondary frontend */
            PrimaryFrontend_p->IsAssociatedWithHW   = FALSE;
            SecondaryFrontend_p->IsAssociatedWithHW = TRUE;

            /* Adopt the settings */
            memcpy ( (void*)&(SecondaryFrontend_p->UserParams),(void*)&(PrimaryFrontend_p->UserParams), sizeof ( STPTI_FrontendParams_t ) );

            /* Clear the links */
            PrimaryFrontend_p->CloneFrontendHandle.word   = STPTI_NullHandle();
            SecondaryFrontend_p->CloneFrontendHandle.word = STPTI_NullHandle();

            /* Switch the memory pointers associated with the structures */
            TmpMemory_p = SecondaryFrontend_p->PIDTableStart_p;
            SecondaryFrontend_p->PIDTableStart_p = PrimaryFrontend_p->PIDTableStart_p;
            PrimaryFrontend_p->PIDTableStart_p = TmpMemory_p;

            TmpMemory_p = SecondaryFrontend_p->PIDTableRealStart_p;
            SecondaryFrontend_p->PIDTableRealStart_p = PrimaryFrontend_p->PIDTableRealStart_p;
            PrimaryFrontend_p->PIDTableRealStart_p = TmpMemory_p;

            TmpMemory_p = SecondaryFrontend_p->RAMStart_p;
            SecondaryFrontend_p->PIDTableRealStart_p = PrimaryFrontend_p->RAMStart_p;
            PrimaryFrontend_p->RAMStart_p = TmpMemory_p;

            TmpMemory_p = SecondaryFrontend_p->RAMRealStart_p;
            SecondaryFrontend_p->RAMRealStart_p = PrimaryFrontend_p->RAMRealStart_p;
            PrimaryFrontend_p->RAMRealStart_p = TmpMemory_p;

            SecondaryFrontend_p->PIDFilterEnabled = PrimaryFrontend_p->PIDFilterEnabled;
            PrimaryFrontend_p->PIDFilterEnabled = TRUE;

            Tmp = SecondaryFrontend_p->AllocatedNumOfPkts;
            SecondaryFrontend_p->AllocatedNumOfPkts = PrimaryFrontend_p->AllocatedNumOfPkts;
            PrimaryFrontend_p->AllocatedNumOfPkts = Tmp;

            if ( (PrimaryFrontend_p->WildCardPidCount) && (SecondaryFrontend_p->PIDFilterEnabled = FALSE))
            {
                SecondaryFrontend_p->PIDFilterEnabled = TRUE;
            }
            PrimaryFrontend_p->WildCardPidCount = 0;

            /* We need to clear any PIDs on associated with the new primary session */
            if ( (TRUE == ( SecondaryFrontend_p->UserParams.u.TSIN.InputBlockEnable )) && (ResyncPIDs == TRUE) )
            {
                FullSessionHandle.word = ( &stptiMemGet_List( FullSessionListHandle )->Handle )[0];
                stptiHAL_SyncFrontendPids( FullFrontendHandle, FullSessionHandle );
            }

        }
        /* If this is a secondary cloned input (TSIN) break the association */
        else if ( (FALSE == (stptiMemGet_Frontend(FullFrontendHandle))->IsAssociatedWithHW) &&
                  (STPTI_NullHandle() != (stptiMemGet_Frontend( FullFrontendHandle ))->CloneFrontendHandle.word) )
        {
            Frontend_t * PrimaryFrontend_p   = stptiMemGet_Frontend( (stptiMemGet_Frontend( FullFrontendHandle ))->CloneFrontendHandle );
            Frontend_t * SecondaryFrontend_p = stptiMemGet_Frontend( FullFrontendHandle );

            if ( (!(SecondaryFrontend_p->WildCardPidCount)) && (PrimaryFrontend_p->PIDFilterEnabled = TRUE))
            {
                stpti_FrontendClearSessionPids( FullFrontendHandle );
            }
            if ( (SecondaryFrontend_p->WildCardPidCount) && !(PrimaryFrontend_p->WildCardPidCount))
            {
                ResyncPIDs = TRUE;
            }

            /* Clear the link */
            PrimaryFrontend_p->CloneFrontendHandle.word   = STPTI_NullHandle();
            SecondaryFrontend_p->CloneFrontendHandle.word = STPTI_NullHandle();

            SecondaryFrontend_p->PIDFilterEnabled = TRUE;
            SecondaryFrontend_p->WildCardPidCount = 0;

            /* We need to clear any PIDs on associated with the new primary session */
            if ( (TRUE == ( PrimaryFrontend_p->UserParams.u.TSIN.InputBlockEnable )) && (ResyncPIDs == TRUE) )
            {
                FullSessionHandle.word = ( &stptiMemGet_List( FullSessionListHandle )->Handle )[0];
                stptiHAL_SyncFrontendPids( FullFrontendHandle, FullSessionHandle );
            }

        }
    }
}

#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */

/* End of pti_fe.c --------------------------------------------------------- */

