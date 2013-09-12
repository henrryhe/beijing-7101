/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: session.c
 Description: HAL session functions for PTI
 
******************************************************************************/

/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX) 
#include <assert.h>
#include <string.h>
#include <stdio.h>
#endif /* #if !defined(ST_OSLINUX)  */

#include "stddefs.h"
#include "stdevice.h"
#if !defined(ST_OSLINUX) 
#include "sttbx.h"
#endif /* #if !defined(ST_OSLINUX)  */
#include "stpti.h"

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"

#include "memget.h"

#include "pti4.h"


/*------------------------------------------------------------------------------
Function Name : stptiHAL_PeekNextFreeSession
  Description : peek next session for the cell, tcram holds the session info
   Parameters : 
------------------------------------------------------------------------------*/
U16 stptiHAL_PeekNextFreeSession(FullHandle_t DeviceHandle)
{
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;

    int session;


    for (session = 0; session < TC_Params_p->TC_NumberOfSessions; session++)
    {
        TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[session];

        if ( STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionTSmergerTag) == SESSION_NOT_ALLOCATED )
        {
           STTBX_Print(("\n stptiHAL_PeekNextFreeSession: %d\n", session ));
           return( session );
       }
    }

    STTBX_Print(("\n stptiHAL_PeekNextFreeSession: SESSION_NOT_ALLOCATED\n"));
    return( SESSION_NOT_ALLOCATED );
}


/*------------------------------------------------------------------------------
Function Name : stptiHAL_GetNextFreeSession
  Description : get next session for the cell, tcram holds the session info
   Parameters :
------------------------------------------------------------------------------*/
U16 stptiHAL_GetNextFreeSession(FullHandle_t DeviceHandle)
{
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;

    U16 IndexIntoSession;


    /* If valid streamid check that StreamId not already in use on Device */
    if (( Device_p->StreamID == STPTI_STREAM_ID_NONE ) ||
        ( stptiHAL_GetSessionPtrForStreamId( DeviceHandle, Device_p->StreamID ) == NULL ))
    {
        IndexIntoSession = stptiHAL_PeekNextFreeSession( DeviceHandle );
        assert( IndexIntoSession != SESSION_NOT_ALLOCATED );

        TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[IndexIntoSession];
        STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionTSmergerTag, Device_p->StreamID);
        if(Device_p->PacketArrivalTimeSource == STPTI_ARRIVAL_TIME_SOURCE_TSMERGER)
        {
            STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionTSmergerTag, SESSION_USE_MERGER_FOR_STC);
        }
    
        Device_p->Session = IndexIntoSession;                   /* STPTI should remeber WHICH session on the tc it owns */

        STTBX_Print(("\n stptiHAL_GetNextFreeSession: %d\n", IndexIntoSession ));
    }
    else
    {
        /* Raise an error - StreamId is already in use */
        IndexIntoSession = SESSION_ILLEGAL_INDEX;
    }

    return( IndexIntoSession );
}


#if !defined ( STPTI_BSL_SUPPORT )
/*------------------------------------------------------------------------------
Function Name : stptiHAL_ReleaseSession
  Description : give up session allocation
   Parameters :
------------------------------------------------------------------------------*/
ST_ErrorCode_t stptiHAL_ReleaseSession(FullHandle_t DeviceHandle)
{
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;

    U16 IndexIntoSession = Device_p->Session;

    if( TC_Params_p->TC_SessionDataStart != NULL )
    {
        TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[IndexIntoSession];
        STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionTSmergerTag, SESSION_NOT_ALLOCATED);
        STTBX_Print(("\n stptiHAL_ReleaseSession: %d\n", IndexIntoSession ));
    }

    return( ST_NO_ERROR );
}


/*------------------------------------------------------------------------------
Function Name : stptiHAL_NumberOfSessions
  Description : give up session allocation
   Parameters :
------------------------------------------------------------------------------*/
int stptiHAL_NumberOfSessions(FullHandle_t DeviceHandle)
{
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;

    int session, sum = 0;


    for (session = 0; session < TC_Params_p->TC_NumberOfSessions; session++)
    {
        TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[session];

        if ( STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionTSmergerTag) != SESSION_NOT_ALLOCATED )
           sum++;
    }

    STTBX_Print(("\n stptiHAL_NumberOfSessions() = %d\n", sum ));
    return( sum );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/*------------------------------------------------------------------------------
Function Name : stptiHAL_GetSessionPtrForStreamId
  Description : Returns pointer to session(as void *) with specified
                stream id, or Null if none such exists (i.e. StreamId is
                available for use)
   Parameters :
------------------------------------------------------------------------------*/
void *stptiHAL_GetSessionPtrForStreamId(FullHandle_t PTIHandle, STPTI_StreamID_t StreamID)
{
    Device_t             *Device_p       = stptiMemGet_Device( PTIHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;
    int                   Session = 0;

    do
    {
        TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[Session];
    }
    while (( StreamID != (STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionTSmergerTag) & SESSION_MASK_STC_SOURCE )) &&
           ( ++Session < TC_Params_p->TC_NumberOfSessions ));

    /* So, did we find a match (did sessionindex get to terminate the loop)?
     * return NULL if it did
     */
    return (( Session < TC_Params_p->TC_NumberOfSessions ) ? (void *) TCSessionInfo_p : NULL );
}


/*------------------------------------------------------------------------------
Function Name : stptiHAL_GetSessionFromFullHandle
  Description : give up session allocation
   Parameters :
------------------------------------------------------------------------------*/
TCSessionInfo_t *stptiHAL_GetSessionFromFullHandle(FullHandle_t DeviceHandle)
{
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    U16 IndexIntoSession = Device_p->Session;


    return( &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[IndexIntoSession] );
}


/* EOF --------------------------------------------------------------------- */
