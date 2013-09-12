/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_ioctl_lib.c
 *  Date        : 17-04-2005
 *  Author      : STIEGLITZP
 *  Description : Implementation for calling STAPI ioctl functions
 *
 *****************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "stpti.h"

#include "stpti4_ioctl.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/
static int fd=-1; /* not open */
static int UseCount=0; /* Number of initialised devices */

static char RevisionString[32];/* Strings must not be longer that 31 bytes with NULL*/

static int LoaderVariant = -1;

/*** METHODS ****************************************************************/

ST_ErrorCode_t STPTI_Init(ST_DeviceName_t DeviceName, const STPTI_InitParams_t * InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if( !UseCount ){ /* First time? */
        char *devpath;

        devpath = getenv("STPTI4_IOCTL_DEV_PATH");  /* get the path for the device */

        if( devpath ){
            fd = open( devpath, O_RDWR );  /* open it */
            if( fd == -1 ){
                perror(strerror(errno));
                fprintf(stderr,"Error trying to open %s\n",devpath);
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }

        }
        else{
            fprintf(stderr,"The dev path enviroment variable is not defined: STPTI4_IOCTL_DEV_PATH\n");
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    UseCount++;

    if( ErrorCode == ST_NO_ERROR ){
        STPTI_Ioctl_Init_t UserParams;

        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
        UserParams.InitParams = *InitParams_p;
        UserParams.ForceLoader = LoaderVariant;

        /* IOCTL the command */
        if (ioctl(fd, STPTI_IOC_INIT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STPTI4_IOCTL_Init():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }

    return ErrorCode;
}

ST_ErrorCode_t STPTI_Open(ST_DeviceName_t DeviceName, const STPTI_OpenParams_t * OpenParams_p, STPTI_Handle_t * Handle_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STPTI_Ioctl_Open_t UserParams;

    if( Handle_p != NULL ){
        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
        UserParams.OpenParams = *OpenParams_p;

        /* IOCTL the command */
        if (ioctl(fd, STPTI_IOC_OPEN, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STPTI_IOCTL_OPEN():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* get the handle. */
            *Handle_p = UserParams.Handle;
            /* and retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    else
        ErrorCode = ST_ERROR_BAD_PARAMETER;

    return ErrorCode;
}

ST_ErrorCode_t STPTI_Close(STPTI_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STPTI_Ioctl_Close_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STPTI_IOC_CLOSE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STPTI_IOC_CLOSE():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STPTI_Term(ST_DeviceName_t DeviceName, const STPTI_TermParams_t * TermParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STPTI_Ioctl_Term_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.TermParams = *TermParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STPTI_IOC_TERM, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STPTI_IOCTL_Term():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }


    if( UseCount ){
        UseCount--;
        if( !UseCount ){ /* Last Termination */
            close(fd);
            fd = -1;
        }
    }
    return ErrorCode;
}

ST_ErrorCode_t STPTI_SlotAllocate(STPTI_Handle_t SessionHandle, STPTI_Slot_t * SlotHandle_p, STPTI_SlotData_t * SlotData_p)
{
    STPTI_Ioctl_SlotAllocate_t UserParams;

    if( SlotHandle_p != NULL ){

        UserParams.SessionHandle = SessionHandle;
        UserParams.SlotData = *SlotData_p;

        if( ioctl( fd, STPTI_IOC_SLOTALLOCATE, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }
        *SlotHandle_p = UserParams.SlotHandle;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SlotDeallocate(STPTI_Slot_t SlotHandle)
{
    STPTI_Ioctl_SlotDeallocate_t UserParams;

    UserParams.SlotHandle = SlotHandle;

    if( ioctl( fd, STPTI_IOC_SLOTDEALLOCATE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SlotSetPid(STPTI_Slot_t SlotHandle, STPTI_Pid_t Pid)
{
    STPTI_Ioctl_SlotSetPid_t UserParams;

    UserParams.SlotHandle = SlotHandle;
    UserParams.Pid = Pid;

    if( ioctl( fd, STPTI_IOC_SLOTSETPID, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SlotClearPid(STPTI_Slot_t SlotHandle)
{
    STPTI_Ioctl_SlotClearPid_t UserParams;

    UserParams.SlotHandle = SlotHandle;

    if( ioctl( fd, STPTI_IOC_SLOTCLEARPID, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferAllocate( STPTI_Handle_t SessionHandle,
                                     U32 RequiredSize,
                                     U32 NumberOfPacketsInMultiPacket,
                                     STPTI_Buffer_t * BufferHandle )
{
    STPTI_Ioctl_BufferAllocate_t UserParams;

    if( BufferHandle != NULL ){
        UserParams.SessionHandle = SessionHandle;
        UserParams.RequiredSize = RequiredSize;
        UserParams.NumberOfPacketsInMultiPacket = NumberOfPacketsInMultiPacket;

        if( ioctl( fd, STPTI_IOC_BUFFERALLOCATE, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *BufferHandle = UserParams.BufferHandle;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferDeallocate( STPTI_Buffer_t BufferHandle )
{
    STPTI_Ioctl_BufferDeallocate_t UserParams;

    UserParams.BufferHandle = BufferHandle;

    if( ioctl( fd, STPTI_IOC_BUFFERDEALLOCATE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;

}

ST_ErrorCode_t STPTI_SlotLinkToBuffer(STPTI_Slot_t Slot, STPTI_Buffer_t BufferHandle)
{
    STPTI_Ioctl_SlotLinkToBuffer_t UserParams;

    UserParams.BufferHandle = BufferHandle;
    UserParams.Slot = Slot;

    if( ioctl( fd, STPTI_IOC_SLOTLINKTOBUFFER, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;

}

ST_ErrorCode_t STPTI_SlotUnLink(STPTI_Slot_t Slot)
{
    STPTI_Ioctl_SlotUnLink_t UserParams;

    UserParams.Slot = Slot;

    if( ioctl( fd, STPTI_IOC_SLOTUNLINK, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;

}

ST_ErrorCode_t STPTI_BufferLinkToCdFifo(STPTI_Buffer_t BufferHandle, STPTI_DMAParams_t * CdFifoParams)
{
    STPTI_Ioctl_BufferLinkToCdFifo_t UserParams;

    UserParams.BufferHandle = BufferHandle;
    UserParams.CdFifoParams = *CdFifoParams;

    if( ioctl( fd, STPTI_IOC_BUFFERLINKTOCDFIFO, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferUnLink( STPTI_Buffer_t BufferHandle )
{
    STPTI_Ioctl_BufferUnLink_t UserParams;

    UserParams.BufferHandle = BufferHandle;

    if( ioctl( fd, STPTI_IOC_BUFFERUNLINK, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_EnableErrorEvent( ST_DeviceName_t DeviceName,
                                       STPTI_Event_t EventName )
{
    STPTI_Ioctl_ErrorEvent_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.EventName = EventName;

    if( ioctl( fd, STPTI_IOC_ENABLEERROREVENT, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_DisableErrorEvent( ST_DeviceName_t DeviceName,
                                       STPTI_Event_t EventName )
{
    STPTI_Ioctl_ErrorEvent_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.EventName = EventName;

    if( ioctl( fd, STPTI_IOC_DISABLEERROREVENT, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

/* This is specifically done this way so that it is difficult to call the
   functions from users space.
*/
ST_ErrorCode_t STPTI_GetSWCDFifoCfg(void** GetWritePtrFn, void** SetReadPtrFn)
{
    STPTI_Ioctl_GetSWCDFifoCfg_t UserParams;

    UserParams.GetWritePtrFn = GetWritePtrFn;
    UserParams.SetReadPtrFn = SetReadPtrFn;

    if( ioctl( fd, STPTI_IOC_GETSWCDFIFOCFG, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferAllocateManual( STPTI_Handle_t SessionHandle,
                                           U8 *Base_p,
                                           U32 RequiredSize,
                                           U32 NumberOfPacketsInMultiPacket,
                                           STPTI_Buffer_t * BufferHandle )
{
    STPTI_Ioctl_BufferAllocateManual_t UserParams;

    if( BufferHandle != NULL ){
        UserParams.SessionHandle = SessionHandle;
        UserParams.Base_p = Base_p;
        UserParams.RequiredSize = RequiredSize;
        UserParams.NumberOfPacketsInMultiPacket = NumberOfPacketsInMultiPacket;

        if( ioctl( fd, STPTI_IOC_BUFFERALLOCATEMANUAL, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *BufferHandle = UserParams.BufferHandle;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferAllocateManualUser( STPTI_Handle_t SessionHandle,
                                               U8 *Base_p,
                                               U32 RequiredSize,
                                               U32 NumberOfPacketsInMultiPacket,
                                               STPTI_Buffer_t * BufferHandle )
{
    STPTI_Ioctl_BufferAllocateManual_t UserParams;

    if( BufferHandle != NULL ){
        UserParams.SessionHandle = SessionHandle;
        UserParams.Base_p = Base_p;
        UserParams.RequiredSize = RequiredSize;
        UserParams.NumberOfPacketsInMultiPacket = NumberOfPacketsInMultiPacket;

        if( ioctl( fd, STPTI_IOC_BUFFERALLOCATEMANUALUSER, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *BufferHandle = UserParams.BufferHandle;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferSetOverflowControl( STPTI_Buffer_t BufferHandle,
                                               BOOL IgnoreOverflow)
{
    STPTI_Ioctl_BufferSetOverflowControl_t UserParams;

    UserParams.BufferHandle = BufferHandle;
    UserParams.IgnoreOverflow = IgnoreOverflow;

    if( ioctl( fd, STPTI_IOC_BUFFERSETOVERFLOWCONTROL, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_GetInputPacketCount(ST_DeviceName_t DeviceName, U16 *Count)
{
    STPTI_Ioctl_GetInputPacketCount_t UserParams;

    if( fd != -1 ){/* Is device initialised? */
        if( Count != NULL ){/* Test for good return pointer */

            memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

            if( ioctl( fd, STPTI_IOC_GETINPUTPACKETCOUNT, &UserParams ) == -1 ){
                perror(strerror(errno));
                UserParams.Error = ST_ERROR_BAD_PARAMETER;
            }
            else
                *Count = UserParams.Count;
        }
        else
            UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }
    else
        UserParams.Error = ST_ERROR_UNKNOWN_DEVICE; /* Not initialised */

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SlotPacketCount(STPTI_Slot_t SlotHandle, U16 *Count)
{
    STPTI_Ioctl_SlotPacketCount_t UserParams;

    if( Count != NULL ){/* Test for good return pointer */
        UserParams.SlotHandle = SlotHandle;

        if( ioctl( fd, STPTI_IOC_SLOTPACKETCOUNT, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *Count = UserParams.Count;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

/****************************************************************************
Name         : STPTI_GetRevision

Description  : Returns revision string of driver

Parameters   :

Return Value :
****************************************************************************/
/* Returns NULL if failed to get revision */
ST_Revision_t STPTI_GetRevision(void)
{
    int local_fd; /* Don't care whether previously opened or not */
    char *devpath;

    devpath = getenv("STPTI4_IOCTL_DEV_PATH");  /* get the path for the device */

    if( devpath ){
        local_fd = open( devpath, O_RDWR );  /* open it */
        if( local_fd == -1 ){
            perror(strerror(errno));
            return (NULL);
        }

    }
    else{
        fprintf(stderr,"The dev path enviroment variable is not defined: STPTI_IOCTL_DEV_PATH\n");
        return (NULL);
    }

    if( ioctl( local_fd, STPTI_IOC_GETREVISION, RevisionString ) == -1 ){
        printf("STPTI_GetRevision(): Failed\n");
        perror(strerror(errno));
        close( local_fd );
        return (NULL);
    }
    close( local_fd );

    return RevisionString;
} /* STPTI_GetRevision */

ST_ErrorCode_t STPTI_SubscribeEvent(STPTI_Handle_t SessionHandle, STPTI_Event_t Event_ID )
{
    STPTI_Ioctl_SubscribeEvent_t UserParams;

    UserParams.SessionHandle = SessionHandle;
    UserParams.Event_ID = Event_ID;

    if( ioctl( fd, STPTI_IOC_SUBSCRIBEEVENT, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_UnsubscribeEvent(STPTI_Handle_t SessionHandle, STPTI_Event_t Event_ID )
{
    STPTI_Ioctl_UnsubscribeEvent_t UserParams;

    UserParams.SessionHandle = SessionHandle;
    UserParams.Event_ID = Event_ID;

    if( ioctl( fd, STPTI_IOC_UNSUBSCRIBEEVENT, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SendEvent(STPTI_Handle_t SessionHandle, STPTI_Event_t Event_ID, STPTI_EventData_t *EventData )
{
    STPTI_Ioctl_SendEvent_t UserParams;

    UserParams.SessionHandle = SessionHandle;
    UserParams.Event_ID = Event_ID;
    UserParams.EventData = *EventData;

    if( ioctl( fd, STPTI_IOC_SENDEVENT, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    UserParams.Error = ST_NO_ERROR;
    return UserParams.Error;
}

/* Return event in location pointed to by  Event_ID */
ST_ErrorCode_t STPTI_WaitForEvent(STPTI_Handle_t SessionHandle, STPTI_Event_t *Event_ID, STPTI_EventData_t *EventData )
{
    STPTI_Ioctl_WaitForEvent_t UserParams;

    UserParams.SessionHandle = SessionHandle;

    if( ioctl( fd, STPTI_IOC_WAITFOREVENT, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    *Event_ID = UserParams.Event_ID;
    *EventData = UserParams.EventData;

    return UserParams.Error;
}

/* This is not really a STPTI function so it is not necessary to
   call STPTI_Init() first */
/* This function is used to replace ResetAllPti() used by harness.*/
void STPTI_TEST_ResetAllPti()
{
    char *devpath;
    int local_fd = -1;

    devpath = getenv("STPTI4_IOCTL_DEV_PATH");  /* get the path for the device */

    if( devpath ){
        local_fd = open( devpath, O_RDWR );  /* open it */
        if( local_fd == -1 )
            perror(strerror(errno));
    }
    else{
        fprintf(stderr,"The dev path enviroment variable is not defined: TSMERGER_IOCTL_DEV_PATH\n");
    }

    if( ioctl( local_fd, STPTI_IOC_TEST_RESETALLPTI, NULL ) == -1 ){
        perror(strerror(errno));
    }

    close( local_fd );
}

/*
void STPTI_TEST_ResetAllPti();
*/
ST_ErrorCode_t STPTI_SignalAllocate(STPTI_Handle_t SessionHandle, STPTI_Signal_t * SignalHandle_p)
{
    STPTI_Ioctl_SignalAllocate_t UserParams;

    if( SignalHandle_p != NULL ){/* Test for good return pointer */
        UserParams.SessionHandle = SessionHandle;

        if( ioctl( fd, STPTI_IOC_SIGNALALLOCATE, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *SignalHandle_p = UserParams.SignalHandle;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SignalAssociateBuffer(STPTI_Signal_t SignalHandle, STPTI_Buffer_t BufferHandle)
{
    STPTI_Ioctl_SignalAssociateBuffer_t UserParams;

    UserParams.SignalHandle = SignalHandle;
    UserParams.BufferHandle = BufferHandle;

    if( ioctl( fd, STPTI_IOC_SIGNALASSOCIATEBUFFER, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SignalDeallocate(STPTI_Signal_t SignalHandle)
{
    STPTI_Ioctl_SignalDeallocate_t UserParams;

    UserParams.SignalHandle = SignalHandle;

    if( ioctl( fd, STPTI_IOC_SIGNALDEALLOCATE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SignalDisassociateBuffer(STPTI_Signal_t SignalHandle, STPTI_Buffer_t BufferHandle)
{
    STPTI_Ioctl_SignalDisassociateBuffer_t UserParams;

    UserParams.SignalHandle = SignalHandle;
    UserParams.BufferHandle = BufferHandle;

    if( ioctl( fd, STPTI_IOC_SIGNALDISASSOCIATEBUFFER, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferLevelSignalDisable( STPTI_Buffer_t BufferHandle )
{
    STPTI_Ioctl_BufferLevelSignalDisable_t UserParams;

    UserParams.BufferHandle = BufferHandle;

    if( ioctl( fd, STPTI_IOC_BUFFERLEVELSIGNALDISABLE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferLevelSignalEnable( STPTI_Buffer_t BufferHandle, U32 BufferLevelThreshold )
{
    STPTI_Ioctl_BufferLevelSignalEnable_t UserParams;

    UserParams.BufferHandle = BufferHandle;
    UserParams.BufferLevelThreshold = BufferLevelThreshold;

    if( ioctl( fd, STPTI_IOC_BUFFERLEVELSIGNALENABLE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SignalWaitBuffer(STPTI_Signal_t SignalHandle, STPTI_Buffer_t * BufferHandle_p, U32 TimeoutMS)
{
    STPTI_Ioctl_SignalWaitBuffer_t UserParams;

    if( BufferHandle_p != NULL ){/* Test for good return pointer */
        UserParams.SignalHandle = SignalHandle;
        UserParams.TimeoutMS = TimeoutMS;

        if( ioctl( fd, STPTI_IOC_SIGNALWAITBUFFER, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *BufferHandle_p = UserParams.BufferHandle;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SignalAbort( STPTI_Signal_t SignalHandle )
{
    STPTI_Ioctl_SignalAbort_t UserParams;

    UserParams.SignalHandle = SignalHandle;

    if( ioctl( fd, STPTI_IOC_SIGNALABORT, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SlotQuery( STPTI_Slot_t SlotHandle,
                                BOOL *PacketsSeen,
                                BOOL *TransportScrambledPacketsSeen,
                                BOOL *PESScrambledPacketsSeen,
                                STPTI_Pid_t *Pid )
{
    STPTI_Ioctl_SlotQuery_t UserParams;

    if( (PacketsSeen != NULL) &&
        (TransportScrambledPacketsSeen != NULL) &&
        (PESScrambledPacketsSeen != NULL) &&
        (Pid != NULL) ){/* Test for good return pointer */

        UserParams.SlotHandle = SlotHandle;

        if( ioctl( fd, STPTI_IOC_SLOTQUERY, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *PacketsSeen = UserParams.PacketsSeen;
        *TransportScrambledPacketsSeen = UserParams.TransportScrambledPacketsSeen;
        *PESScrambledPacketsSeen = UserParams.PESScrambledPacketsSeen;
        *Pid = UserParams.Pid;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferFlush( STPTI_Buffer_t BufferHandle )
{
    STPTI_Ioctl_BufferFlush_t UserParams;

    UserParams.BufferHandle = BufferHandle;

    if( ioctl( fd, STPTI_IOC_BUFFERFLUSH, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

/*
   This function returns the number of bytes in the buffer.
   It is contradictory to its name.
   The function will fail if the buffer is not linked to a slot.
*/
ST_ErrorCode_t STPTI_BufferGetFreeLength(STPTI_Buffer_t BufferHandle,
                                         U32 *FreeLength_p)
{
    STPTI_Ioctl_BufferGetFreeLength_t UserParams;

    if( FreeLength_p != NULL){
        UserParams.BufferHandle = BufferHandle;

        if( ioctl( fd, STPTI_IOC_BUFFERGETFREELENGTH, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *FreeLength_p = UserParams.FreeLength;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferReadPes(STPTI_Buffer_t BufferHandle,
                                   U8 *Destination0_p,
                                   U32 DestinationSize0,
                                   U8 *Destination1_p,
                                   U32 DestinationSize1,
                                   U32 *DataSize_p,
                                   STPTI_Copy_t DmaOrMemcpy)
{
    STPTI_Ioctl_BufferReadPes_t UserParams;

    if( DataSize_p != NULL){
        UserParams.BufferHandle = BufferHandle;              /* In */
        UserParams.Destination0_p = Destination0_p;          /* In */
        UserParams.DestinationSize0 = DestinationSize0;      /* In */
        UserParams.Destination1_p = Destination1_p;          /* In */
        UserParams.DestinationSize1 = DestinationSize1;      /* In */
        UserParams.DmaOrMemcpy = DmaOrMemcpy;                /* In */

        if( ioctl( fd, STPTI_IOC_BUFFERREADPES, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *DataSize_p = UserParams.DataSize;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferTestForData(STPTI_Buffer_t BufferHandle, U32 *BytesInBuffer)
{
    STPTI_Ioctl_BufferTestForData_t UserParams;

    if( BytesInBuffer != NULL){
        UserParams.BufferHandle = BufferHandle;              /* In */

        if( ioctl( fd, STPTI_IOC_BUFFERTESTFORDATA, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *BytesInBuffer = UserParams.BytesInBuffer;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

/* Returns -1 if error else a valid filedescriptor */
int OpenSTPTIDriver()
{
    char *devpath;
    int ret_fd = -1;
    devpath = getenv("STPTI4_IOCTL_DEV_PATH");  /* get the path for the device */
    if( devpath ){
        ret_fd = open( devpath, O_RDWR );  /* open it */
        if( ret_fd == -1 )
            perror(strerror(errno));
    }
    else{
        fprintf(stderr,"The dev path enviroment variable is not defined: STPTI_IOCTL_DEV_PATH\n");
    }

    return( ret_fd );
}

ST_ErrorCode_t STPTI_GetCapability(ST_DeviceName_t DeviceName, STPTI_Capability_t *DeviceCapability)
{
    STPTI_Ioctl_GetCapability_t UserParams;
    int local_fd; /* Don't care whether previously opened or not */
    ST_ErrorCode_t Error = ST_ERROR_BAD_PARAMETER;

    local_fd = OpenSTPTIDriver();

    if( local_fd != -1 ){
        if( DeviceCapability != NULL){
            memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

            if( ioctl( local_fd, STPTI_IOC_GETCAPABILITY, &UserParams ) == -1 ){
                perror(strerror(errno));
            }
            else{
                *DeviceCapability = UserParams.DeviceCapability;
                Error = UserParams.Error;
            }
        }
    }

    close( local_fd );

    return Error;
}

ST_ErrorCode_t STPTI_FilterAllocate(STPTI_Handle_t SessionHandle,
                                    STPTI_FilterType_t FilterType,
                                    STPTI_Filter_t * FilterObject )
{
    STPTI_Ioctl_FilterAllocate_t UserParams;

    if( FilterObject != NULL){
        UserParams.SessionHandle = SessionHandle;
        UserParams.FilterType    = FilterType;

        if( ioctl( fd, STPTI_IOC_FILTERALLOCATE, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *FilterObject = UserParams.FilterObject;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_FilterAssociate(STPTI_Filter_t FilterHandle, STPTI_Slot_t SlotHandle)
{
    STPTI_Ioctl_FilterAssociate_t UserParams;

    UserParams.FilterHandle = FilterHandle;
    UserParams.SlotHandle = SlotHandle;

    if( ioctl( fd, STPTI_IOC_FILTERASSOCIATE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_FilterDeallocate(STPTI_Filter_t FilterHandle)
{
    STPTI_Ioctl_FilterDeallocate_t UserParams;

    UserParams.FilterHandle = FilterHandle;

    if( ioctl( fd, STPTI_IOC_FILTERDEALLOCATE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_FilterDisassociate(STPTI_Filter_t FilterHandle, STPTI_Slot_t SlotHandle)
{
    STPTI_Ioctl_FilterDisassociate_t UserParams;

    UserParams.FilterHandle = FilterHandle;
    UserParams.SlotHandle = SlotHandle;

    if( ioctl( fd, STPTI_IOC_FILTERDISASSOCIATE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_FilterSet(STPTI_Filter_t FilterHandle, STPTI_FilterData_t *FilterData_p)
{
    STPTI_Ioctl_FilterSet_t UserParams;
    ST_ErrorCode_t Error = ST_ERROR_BAD_PARAMETER;

    if( FilterData_p != NULL ){
        UserParams.FilterHandle = FilterHandle;
        UserParams.FilterData   = *FilterData_p;

        if( ioctl( fd, STPTI_IOC_FILTERSET, &UserParams ) == -1 ){
            perror(strerror(errno));
        }
        else{
            Error = UserParams.Error;
        }
    }

    return Error;
}

/* Only Memcpy supported - STPTI_COPY_TRANSFER_BY_MEMCPY */
ST_ErrorCode_t STPTI_BufferReadNBytes(STPTI_Buffer_t BufferHandle,
                                   U8 *Destination0_p,
                                   U32 DestinationSize0,
                                   U8 *Destination1_p,
                                   U32 DestinationSize1,
                                   U32 *DataSize_p,
                                   STPTI_Copy_t DmaOrMemcpy,
                                   U32 BytesToCopy)
{
    STPTI_Ioctl_BufferReadNBytes_t UserParams;

    if( DataSize_p != NULL){
        UserParams.BufferHandle = BufferHandle;              /* In */
        UserParams.Destination0_p = Destination0_p;          /* In */
        UserParams.DestinationSize0 = DestinationSize0;      /* In */
        UserParams.Destination1_p = Destination1_p;          /* In */
        UserParams.DestinationSize1 = DestinationSize1;      /* In */
        UserParams.DmaOrMemcpy = DmaOrMemcpy;                /* In */
        UserParams.BytesToCopy = BytesToCopy;

        if( ioctl( fd, STPTI_IOC_BUFFERREADNBYTES, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *DataSize_p = UserParams.DataSize;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

/******************************************************************************
Function Name : STPTI_SlotSetCCControl
  Description :
   Parameters : SetOnOff - when TRUE   CC checking will be disabled.
                         - when FALSE  CC checking will be enabled.
******************************************************************************/
ST_ErrorCode_t STPTI_SlotSetCCControl(STPTI_Slot_t SlotHandle, BOOL SetOnOff)
{
    STPTI_Ioctl_SlotSetCCControl_t UserParams;

    UserParams.SlotHandle = SlotHandle;
    UserParams.SetOnOff = SetOnOff;

    if( ioctl( fd, STPTI_IOC_SLOTSETCCCONTROL, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

/******************************************************************************
Function Name : STPTI_BufferReadSection
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferReadSection( STPTI_Buffer_t BufferHandle,
                                        STPTI_Filter_t MatchedFilterList[],
                                        U32 MaxLengthofFilterList, /* In filters */
                                        U32 *NumOfFilterMatches_p,
                                        BOOL *CRCValid_p,
                                        U8 *Destination0_p,
                                        U32 DestinationSize0,
                                        U8 *Destination1_p,
                                        U32 DestinationSize1,
                                        U32 *DataSize_p,
                                        STPTI_Copy_t DmaOrMemcpy )
{
    STPTI_Ioctl_BufferReadSection_t UserParams;
    ST_ErrorCode_t Error;

    if (MaxLengthofFilterList == 0)
	MatchedFilterList = NULL;

    if( (DataSize_p != NULL) &&
	(NumOfFilterMatches_p != NULL) &&
	(CRCValid_p != NULL) && 	   
	((MatchedFilterList == NULL && MaxLengthofFilterList == 0) ||
	(MatchedFilterList != NULL && MaxLengthofFilterList > 0))){
    	UserParams.MatchedFilterList = MatchedFilterList;
        UserParams.MaxLengthofFilterList = MaxLengthofFilterList;
        UserParams.BufferHandle = BufferHandle;
        UserParams.Destination0_p = Destination0_p;
        UserParams.DestinationSize0 = DestinationSize0;
        UserParams.Destination1_p = Destination1_p;
        UserParams.DestinationSize1 = DestinationSize1;
        UserParams.DmaOrMemcpy = DmaOrMemcpy;

        if( ioctl( fd, STPTI_IOC_BUFFERREADSECTION, &UserParams ) == -1 ){
            perror(strerror(errno));
            Error = ST_ERROR_BAD_PARAMETER;
        }
        else{
            *DataSize_p = UserParams.DataSize;
            *CRCValid_p = UserParams.CRCValid;
            *NumOfFilterMatches_p = UserParams.NumOfFilterMatches;
            Error = UserParams.Error;
        }
    }
    else
        Error = ST_ERROR_BAD_PARAMETER;

    return Error;
}

ST_ErrorCode_t STPTI_WaitEventList( STPTI_Handle_t SessionHandle,
                                  U32 NbEvents,
                                  U32 *Events_p,
                                  U32 *Event_That_Has_Occurred_p,
                                  U32 EventDataSize,
                                  void *EventData_p )
{
    ST_ErrorCode_t Error;
    STPTI_Ioctl_WaitEventList_t *UserParams_p;

    if( (Events_p != NULL) ||
        (Event_That_Has_Occurred_p != NULL) ||
        (EventData_p != NULL) ){

        UserParams_p = (STPTI_Ioctl_WaitEventList_t *)malloc(sizeof(STPTI_Ioctl_WaitEventList_t) +
                                                             (sizeof(U32)*NbEvents) +
                                                             EventDataSize );
        UserParams_p->SessionHandle = SessionHandle;
        UserParams_p->NbEvents = NbEvents;
        UserParams_p->EventDataSize = EventDataSize;

        memcpy(UserParams_p->FuncData, Events_p, sizeof(U32)*NbEvents );

        if( ioctl( fd, STPTI_IOC_WAITEVENTLIST, UserParams_p ) == -1 ){
            perror(strerror(errno));
            Error = ST_ERROR_BAD_PARAMETER;
        }
        else{
            *Event_That_Has_Occurred_p = UserParams_p->Event_That_Has_Occurred;
            memcpy(EventData_p, UserParams_p->FuncData+sizeof(U32)*NbEvents, EventDataSize );
            Error = UserParams_p->Error;
        }

        Error = UserParams_p->Error;
        free(UserParams_p);
    }
    else
        Error = ST_ERROR_BAD_PARAMETER;

    return Error;
}

/******************************************************************************
Function Name : STPTI_DescramblerAllocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerAllocate(STPTI_Handle_t Handle, STPTI_Descrambler_t *DescramblerObject_p, STPTI_DescramblerType_t DescramblerType)
{
    STPTI_DescAllocate_t UserParams;

    if (DescramblerObject_p != NULL) {
      UserParams.PtiHandle       = Handle;
      UserParams.DescramblerType = DescramblerType;
      if( ioctl( fd, STPTI_IOC_DESCALLOCATE, &UserParams ) == -1){
          perror(strerror(errno));
          return( ST_ERROR_BAD_PARAMETER );
      }
      else
      {
        *DescramblerObject_p = UserParams.DescramblerObject;
      }
    }else {
      return ST_ERROR_BAD_PARAMETER;
    }
    return UserParams.Error;
}

/******************************************************************************
Function Name : STPTI_DescramblerAssociate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerAssociate(STPTI_Descrambler_t DescramblerHandle, STPTI_SlotOrPid_t SlotOrPid)
{
    STPTI_DescAssociate_t UserParams;

    UserParams.DescramblerHandle = DescramblerHandle;
    UserParams.SlotOrPid         = SlotOrPid;
    if( ioctl( fd, STPTI_IOC_DESCASSOCIATE, &UserParams ) == -1){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }
    return UserParams.Error;
}

/******************************************************************************
Function Name : STPTI_DescramblerDeallocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerDeallocate(STPTI_Descrambler_t DescramblerHandle)
{
    STPTI_DescDeAllocate_t UserParams;

    UserParams.DescramblerHandle = DescramblerHandle;
    if( ioctl( fd, STPTI_IOC_DESCDEALLOCATE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }
    return UserParams.Error;
}

/******************************************************************************
Function Name : STPTI_DescramblerDisassociate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerDisassociate(STPTI_Descrambler_t DescramblerHandle, STPTI_SlotOrPid_t SlotOrPid)
{
    STPTI_DescDisAssociate_t UserParams;

    UserParams.DescramblerHandle = DescramblerHandle;
    UserParams.SlotOrPid         = SlotOrPid;
    if( ioctl( fd, STPTI_IOC_DESCDISASSOCIATE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }
    return UserParams.Error;
}

/******************************************************************************
Function Name : STPTI_DescramblerSet
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerSet(STPTI_Descrambler_t DescramblerHandle, STPTI_KeyParity_t Parity, STPTI_KeyUsage_t Usage, U8 *Data_p)
{
    STPTI_DescSetKeys_t UserParams;

    if (Data_p != NULL) {
      UserParams.DescramblerHandle = DescramblerHandle;
      UserParams.Parity            = Parity;
      UserParams.Usage             = Usage;
      UserParams.UserKeyPtr        = Data_p;

      if( ioctl( fd, STPTI_IOC_DESCSETKEYS, &UserParams ) == -1 ){
          perror(strerror(errno));
          return( ST_ERROR_BAD_PARAMETER );
      }
    }else {
      return ST_ERROR_BAD_PARAMETER;
    }
    return UserParams.Error;
}

/******************************************************************************
Function Name : STPTI_GetPacketArrivalTime
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_GetPacketArrivalTime(STPTI_Handle_t Handle, STPTI_TimeStamp_t *ArrivalTime_p, U16 *ArrivalTimeExtension_p)
{
    STPTI_GetArrivalTime_t UserParams;

    if ((ArrivalTime_p != NULL) &&
        (ArrivalTimeExtension_p != NULL)) {
      UserParams.Handle                 = Handle;
      if( ioctl( fd, STPTI_IOC_GETARRIVALTIME, &UserParams ) == -1 ){
          perror(strerror(errno));
          return( ST_ERROR_BAD_PARAMETER );
      }
      else
      {
        *ArrivalTime_p          = UserParams.ArrivalTime;
        *ArrivalTimeExtension_p = UserParams.ArrivalTimeExtension;
      }
    }
    else {
      return ST_ERROR_BAD_PARAMETER;
    }
    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SlotDescramblingControl(STPTI_Slot_t SlotHandle, BOOL EnableDescramblingControl)
{
    STPTI_Ioctl_SlotDescramblingControl_t UserParams;

    UserParams.SlotHandle = SlotHandle;
    UserParams.EnableDescramblingControl = EnableDescramblingControl;

    if( ioctl( fd, STPTI_IOC_SLOTDESCRAMBLINGCONTROL, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SlotSetAlternateOutputAction(STPTI_Slot_t SlotHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag)
{
    STPTI_Ioctl_SlotSetAlternateOutputAction_t UserParams;

    UserParams.SlotHandle = SlotHandle;
    UserParams.Method = Method;
    UserParams.Tag = Tag;

    if( ioctl( fd, STPTI_IOC_SLOTSETALTERNATEOUTPUTACTION, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SlotState( STPTI_Slot_t SlotHandle,
                                U32 *SlotCount_p,
                                STPTI_ScrambleState_t *ScrambleState_p,
                                STPTI_Pid_t *Pid_p )
{
    ST_ErrorCode_t retval = ST_ERROR_BAD_PARAMETER;
    STPTI_Ioctl_SlotState_t UserParams;

    if ((SlotCount_p != NULL) &&
        (ScrambleState_p != NULL) &&
        (Pid_p != NULL)) {

        UserParams.SlotHandle = SlotHandle;

        if( ioctl( fd, STPTI_IOC_SLOTSTATE, &UserParams ) == -1 ){
            perror(strerror(errno));
        }
        else{
            *SlotCount_p = UserParams.SlotCount;
            *ScrambleState_p = UserParams.ScrambleState;
            *Pid_p = UserParams.Pid;

            retval = UserParams.Error;
        }
    }

    return retval;
}

ST_ErrorCode_t STPTI_AlternateOutputSetDefaultAction(STPTI_Slot_t SlotHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag)
{
    STPTI_Ioctl_AlternateOutputSetDefaultAction_t UserParams;

    UserParams.SlotHandle = SlotHandle;
    UserParams.Method = Method;
    UserParams.Tag = Tag;

    if( ioctl( fd, STPTI_IOC_ALTERNATEOUTPUTSETDEFAULTACTION, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_HardwarePause( ST_DeviceName_t DeviceName )
{
    STPTI_Ioctl_HardwareData_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

    if( ioctl( fd, STPTI_IOC_HARWAREPAUSE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_HardwareReset( ST_DeviceName_t DeviceName )
{
    STPTI_Ioctl_HardwareData_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

    if( ioctl( fd, STPTI_IOC_HARWARERESET, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_HardwareResume( ST_DeviceName_t DeviceName )
{
    STPTI_Ioctl_HardwareData_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

    if( ioctl( fd, STPTI_IOC_HARWARERESUME, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_GetPresentationSTC(ST_DeviceName_t DeviceName, STPTI_Timer_t Timer, STPTI_TimeStamp_t *TimeStamp_p)
{
    ST_ErrorCode_t retval = ST_ERROR_BAD_PARAMETER;
    STPTI_Ioctl_GetPresentationSTC_t UserParams;

    if (TimeStamp_p != NULL){

        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
        UserParams.Timer = Timer;

        if( ioctl( fd, STPTI_IOC_GETPRESENTATIONSTC, &UserParams ) == -1 ){
            perror(strerror(errno));
        }

        *TimeStamp_p = UserParams.TimeStamp;

        retval = UserParams.Error;
    }
    return retval;
}

ST_ErrorCode_t STPTI_BufferSetMultiPacket(STPTI_Buffer_t BufferHandle, U32 NumberOfPacketsInMultiPacket)
{
    STPTI_Ioctl_BufferSetMultiPacket_t UserParams;

    UserParams.BufferHandle = BufferHandle;
    UserParams.NumberOfPacketsInMultiPacket = NumberOfPacketsInMultiPacket;

    if( ioctl( fd, STPTI_IOC_BUFFERSETMULTIPACKET, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;

}

/* Only Memcpy supported - STPTI_COPY_TRANSFER_BY_MEMCPY */
ST_ErrorCode_t STPTI_BufferRead(STPTI_Buffer_t BufferHandle,
                                   U8 *Destination0_p,
                                   U32 DestinationSize0,
                                   U8 *Destination1_p,
                                   U32 DestinationSize1,
                                   U32 *DataSize_p,
                                   STPTI_Copy_t DmaOrMemcpy)
{
    STPTI_Ioctl_BufferRead_t UserParams;

    if( DataSize_p != NULL){
        UserParams.BufferHandle = BufferHandle;              /* In */
        UserParams.Destination0_p = Destination0_p;          /* In */
        UserParams.DestinationSize0 = DestinationSize0;      /* In */
        UserParams.Destination1_p = Destination1_p;          /* In */
        UserParams.DestinationSize1 = DestinationSize1;      /* In */
        UserParams.DmaOrMemcpy = DmaOrMemcpy;                /* In */

        if( ioctl( fd, STPTI_IOC_BUFFERREAD, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *DataSize_p = UserParams.DataSize;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

/* Only Memcpy supported - STPTI_COPY_TRANSFER_BY_MEMCPY */
ST_ErrorCode_t STPTI_BufferExtractData(STPTI_Buffer_t BufferHandle,
                                       U32 Offset,
                                       U32 NumBytesToExtract,
                                       U8 * Destination0_p,
                                       U32 DestinationSize0,
                                       U8 * Destination1_p,
                                       U32 DestinationSize1,
                                       U32 * DataSize_p,
                                       STPTI_Copy_t DmaOrMemcpy)
{
    STPTI_Ioctl_BufferExtractData_t UserParams;

    if( DataSize_p != NULL){
        UserParams.BufferHandle = BufferHandle;              /* In */
        UserParams.Offset = Offset;                          /* In */
        UserParams.NumBytesToExtract = NumBytesToExtract;    /* In */
        UserParams.Destination0_p = Destination0_p;          /* In */
        UserParams.DestinationSize0 = DestinationSize0;      /* In */
        UserParams.Destination1_p = Destination1_p;          /* In */
        UserParams.DestinationSize1 = DestinationSize1;      /* In */
        UserParams.DmaOrMemcpy = DmaOrMemcpy;                /* In */

        if( ioctl( fd, STPTI_IOC_BUFFEREXTRACTDATA, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *DataSize_p = UserParams.DataSize;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferExtractPartialPesPacketData(STPTI_Buffer_t BufferHandle,
                                                       BOOL *PayloadUnitStart_p,
                                                       BOOL *CCDiscontinuity_p,
                                                       U8 *ContinuityCount_p,
                                                       U16 *DataLength_p)
{
    STPTI_Ioctl_BufferExtractPartialPesPacketData_t UserParams;

    if( (PayloadUnitStart_p != NULL) &&
        (CCDiscontinuity_p != NULL) &&
        (ContinuityCount_p != NULL) &&
        (DataLength_p != NULL) ){
        UserParams.BufferHandle = BufferHandle;              /* In */

        if( ioctl( fd, STPTI_IOC_BUFFEREXTRACTPARTIALPESPACKETDATA, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *PayloadUnitStart_p = UserParams.PayloadUnitStart;
        *CCDiscontinuity_p = UserParams.CCDiscontinuity;
        *ContinuityCount_p = UserParams.ContinuityCount;
        *DataLength_p = UserParams.DataLength;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

/* Only Memcpy supported - STPTI_COPY_TRANSFER_BY_MEMCPY */
ST_ErrorCode_t STPTI_BufferExtractPesPacketData(STPTI_Buffer_t BufferHandle,
                                                U8 *PesFlags_p,
                                                U8 *TrickModeFlags_p,
                                                U32 *PESPacketLength_p,
                                                STPTI_TimeStamp_t * PTSValue_p,
                                                STPTI_TimeStamp_t * DTSValue_p)
{
    STPTI_Ioctl_BufferExtractPesPacketData_t UserParams;

    if( (PesFlags_p != NULL) &&
        (TrickModeFlags_p != NULL) &&
        (PESPacketLength_p != NULL) &&
        (PTSValue_p != NULL) &&
        (DTSValue_p != NULL) ){
        UserParams.BufferHandle = BufferHandle;              /* In */

        if( ioctl( fd, STPTI_IOC_BUFFEREXTRACTPESPACKETDATA, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *PesFlags_p = UserParams.PesFlags;
        *TrickModeFlags_p = UserParams.TrickModeFlags;
        *PESPacketLength_p = UserParams.PESPacketLength;
        *PTSValue_p = UserParams.PTSValue;
        *DTSValue_p = UserParams.DTSValue;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

/* Only Memcpy supported - STPTI_COPY_TRANSFER_BY_MEMCPY */
ST_ErrorCode_t STPTI_BufferExtractSectionData(STPTI_Buffer_t BufferHandle,
                                              STPTI_Filter_t MatchedFilterList[],
                                              U16 MaxLengthofFilterList,
                                              U16 *NumOfFilterMatches_p,
                                              BOOL *CRCValid_p,
                                              U32 *SectionHeader_p)
{
    STPTI_Ioctl_BufferExtractSectionData_t UserParams;
    ST_ErrorCode_t Error;

    if( (SectionHeader_p != NULL) &&
        (NumOfFilterMatches_p != NULL) &&
        (CRCValid_p != NULL) &&
        (MatchedFilterList != NULL) )
    {
        UserParams.MaxLengthofFilterList = MaxLengthofFilterList;
        UserParams.BufferHandle = BufferHandle;
        UserParams.MatchedFilterList = MatchedFilterList;

        if( ioctl( fd, STPTI_IOC_BUFFEREXTRACTSECTIONDATA, &UserParams ) == -1 ){
            perror(strerror(errno));
            Error = ST_ERROR_BAD_PARAMETER;
        }
        else{
            *CRCValid_p = UserParams.CRCValid;
            *NumOfFilterMatches_p = UserParams.NumOfFilterMatches;
            *SectionHeader_p = UserParams.SectionHeader;
            Error = UserParams.Error;
        }
    }
    else
        Error = ST_ERROR_BAD_PARAMETER;

    return Error;
}

ST_ErrorCode_t STPTI_BufferExtractTSHeaderData(STPTI_Buffer_t BufferHandle, U32 *TSHeader_p)
{
    STPTI_Ioctl_BufferExtractTSHeaderData_t UserParams;

    if( TSHeader_p != NULL ){
        UserParams.BufferHandle = BufferHandle;              /* In */

        if( ioctl( fd, STPTI_IOC_BUFFEREXTRACTTSHEADERDATA, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *TSHeader_p = UserParams.TSHeader;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferReadPartialPesPacket(STPTI_Buffer_t BufferHandle,
                                                U8 *Destination0_p,
                                                U32 DestinationSize0,
                                                U8 *Destination1_p,
                                                U32 DestinationSize1,
                                                BOOL *PayloadUnitStart_p,
                                                BOOL *CCDiscontinuity_p,
                                                U8 *ContinuityCount_p,
                                                U32 *DataSize_p,
                                                STPTI_Copy_t DmaOrMemcpy)
{
    STPTI_Ioctl_BufferReadPartialPesPacket_t UserParams;

    if( (PayloadUnitStart_p != NULL) &&
        (CCDiscontinuity_p != NULL) &&
        (ContinuityCount_p != NULL) &&
        (DataSize_p != NULL) ){
        UserParams.BufferHandle = BufferHandle;              /* In */
        UserParams.Destination0_p = Destination0_p;          /* In */
        UserParams.DestinationSize0 = DestinationSize0;      /* In */
        UserParams.Destination1_p = Destination1_p;          /* In */
        UserParams.DestinationSize1 = DestinationSize1;      /* In */
        UserParams.DmaOrMemcpy = DmaOrMemcpy;                /* In */

        if( ioctl( fd, STPTI_IOC_BUFFERREADPARTIALPESPACKET, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *PayloadUnitStart_p = UserParams.PayloadUnitStart;
        *CCDiscontinuity_p = UserParams.CCDiscontinuity;
        *ContinuityCount_p = UserParams.ContinuityCount;
        *DataSize_p = UserParams.DataSize;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}


/* Only Memcpy supported - STPTI_COPY_TRANSFER_BY_MEMCPY */
ST_ErrorCode_t STPTI_BufferReadTSPacket(STPTI_Buffer_t BufferHandle,
                                        U8 *Destination0_p,
                                        U32 DestinationSize0,
                                        U8 *Destination1_p,
                                        U32 DestinationSize1,
                                        U32 *DataSize_p,
                                        STPTI_Copy_t DmaOrMemcpy)
{
    /* Share STPTI_Ioctl_BufferRead_t structure */
    STPTI_Ioctl_BufferRead_t UserParams;

    if( DataSize_p != NULL){
        UserParams.BufferHandle = BufferHandle;              /* In */
        UserParams.Destination0_p = Destination0_p;          /* In */
        UserParams.DestinationSize0 = DestinationSize0;      /* In */
        UserParams.Destination1_p = Destination1_p;          /* In */
        UserParams.DestinationSize1 = DestinationSize1;      /* In */
        UserParams.DmaOrMemcpy = DmaOrMemcpy;                /* In */

        if( ioctl( fd, STPTI_IOC_BUFFERREADTSPACKET, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *DataSize_p = UserParams.DataSize;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_FilterSetMatchAction(STPTI_Filter_t FilterHandle,
                                          STPTI_Filter_t FiltersToEnable[],
                                          U16 NumOfFiltersToEnable)
{
    STPTI_Ioctl_FilterSetMatchAction_t *UserParams_p;
    ST_ErrorCode_t Error;

    if( FiltersToEnable != NULL )
    {
        UserParams_p = (STPTI_Ioctl_FilterSetMatchAction_t *)malloc(sizeof(STPTI_Ioctl_FilterSetMatchAction_t)+sizeof(STPTI_Filter_t)*NumOfFiltersToEnable);

        UserParams_p->NumOfFiltersToEnable = NumOfFiltersToEnable;
        UserParams_p->FilterHandle = FilterHandle;

        memcpy(UserParams_p->FiltersToEnable,FiltersToEnable,sizeof(STPTI_Filter_t)*NumOfFiltersToEnable);

        if( ioctl( fd, STPTI_IOC_FILTERSETMATCHACTION, UserParams_p ) == -1 ){
            perror(strerror(errno));
            Error = ST_ERROR_BAD_PARAMETER;
        }
        else{
            Error = UserParams_p->Error;
        }
        free(UserParams_p);
    }
    else
        Error = ST_ERROR_BAD_PARAMETER;

    return Error;
}

ST_ErrorCode_t STPTI_FiltersFlush(STPTI_Buffer_t BufferHandle,
                                  STPTI_Filter_t Filters[],
                                  U16 NumOfFiltersToFlush)
{
    STPTI_Ioctl_FiltersFlush_t *UserParams_p;
    ST_ErrorCode_t Error;

    if( Filters != NULL )
    {
        UserParams_p = (STPTI_Ioctl_FiltersFlush_t *)malloc(sizeof(STPTI_Ioctl_FiltersFlush_t)+sizeof(STPTI_Filter_t)*NumOfFiltersToFlush);

        UserParams_p->NumOfFiltersToFlush = NumOfFiltersToFlush;
        UserParams_p->BufferHandle = BufferHandle;

        memcpy(UserParams_p->Filters,Filters,sizeof(STPTI_Filter_t)*NumOfFiltersToFlush);

        if( ioctl( fd, STPTI_IOC_FILTERSFLUSH, UserParams_p ) == -1 ){
            perror(strerror(errno));
            Error = ST_ERROR_BAD_PARAMETER;
        }
        else{
            Error = UserParams_p->Error;
        }
        free(UserParams_p);
    }
    else
        Error = ST_ERROR_BAD_PARAMETER;

    return Error;
}

ST_ErrorCode_t STPTI_ModifyGlobalFilterState(STPTI_Filter_t FiltersToDisable[],
                                             U16 NumOfFiltersToDisable,
                                             STPTI_Filter_t FiltersToEnable[],
                                             U16 NumOfFiltersToEnable)
{
    STPTI_Ioctl_ModifyGlobalFilterState_t *UserParams_p;
    ST_ErrorCode_t Error;

    if( (FiltersToDisable != NULL) &&
        (FiltersToEnable != NULL) )
    {
        UserParams_p = (STPTI_Ioctl_ModifyGlobalFilterState_t *)
            malloc(sizeof(STPTI_Ioctl_ModifyGlobalFilterState_t)+
                   sizeof(STPTI_Filter_t)*(NumOfFiltersToDisable+NumOfFiltersToEnable));

        UserParams_p->NumOfFiltersToDisable = NumOfFiltersToDisable;
        UserParams_p->NumOfFiltersToEnable = NumOfFiltersToEnable;

        memcpy(UserParams_p->Filters,
               FiltersToDisable,
               sizeof(STPTI_Filter_t)*NumOfFiltersToDisable);
        memcpy(UserParams_p->Filters+NumOfFiltersToDisable,
               FiltersToEnable,sizeof(STPTI_Filter_t)*NumOfFiltersToEnable);

        if( ioctl( fd, STPTI_IOC_MODIFYGLOBALFILTERSTATE, UserParams_p ) == -1 ){
            perror(strerror(errno));
            Error = ST_ERROR_BAD_PARAMETER;
        }
        else{
            Error = UserParams_p->Error;
        }
        free(UserParams_p);
    }
    else
        Error = ST_ERROR_BAD_PARAMETER;

    return Error;
}

ST_ErrorCode_t STPTI_GetPacketErrorCount(ST_DeviceName_t DeviceName, U32 *Count)
{
    STPTI_Ioctl_GetPacketErrorCount_t UserParams;

    if( fd != -1 ){ /* Test for device initialised. */
        if( Count != NULL ){/* Test for good return pointer */

            memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

            if( ioctl( fd, STPTI_IOC_GETPACKETERRORCOUNT, &UserParams ) == -1 ){
                perror(strerror(errno));
                return( ST_ERROR_BAD_PARAMETER );
            }

            *Count = UserParams.Count;
        }
        else
            UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }
    else
        UserParams.Error = ST_ERROR_UNKNOWN_DEVICE;/* Not initialised */

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_PidQuery(ST_DeviceName_t DeviceName, STPTI_Pid_t Pid, STPTI_Slot_t *Slot_p)
{
    STPTI_Ioctl_PidQuery_t UserParams;

    if( fd != -1 ){/* Is device initialised? */
        if( Slot_p != NULL ){/* Test for good return pointer */
            UserParams.Pid = Pid;
            memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

            if( ioctl( fd, STPTI_IOC_PIDQUERY, &UserParams ) == -1 ){
                perror(strerror(errno));
                return( ST_ERROR_BAD_PARAMETER );
            }

            *Slot_p = UserParams.Slot;
        }
        else
            UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }
    else
        UserParams.Error = ST_ERROR_UNKNOWN_DEVICE; /* Not Initialised */

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_GetCurrentPTITimer(ST_DeviceName_t DeviceName,
                                        STPTI_TimeStamp_t *TimeStamp_p)
{
    STPTI_Ioctl_GetCurrentPTITimer_t UserParams;

    if( fd != -1 ){/* Is device initialised? */
        if( TimeStamp_p != NULL ){/* Test for good return pointer */

            memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

            if( ioctl( fd, STPTI_IOC_GETCURRENTPTITIMER, &UserParams ) == -1 ){
                perror(strerror(errno));
                UserParams.Error = ST_ERROR_BAD_PARAMETER;
            }
            else
                *TimeStamp_p = UserParams.TimeStamp;
        }
        else
            UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }
    else
        UserParams.Error = ST_ERROR_UNKNOWN_DEVICE; /* Not initialised */

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_GetGlobalPacketCount(ST_DeviceName_t DeviceName, U32 *Count)
{
    STPTI_Ioctl_GetGlobalPacketCount_t UserParams;

    if( fd != -1 ){/* Is device initialised? */
        if( Count != NULL ){/* Test for good return pointer */

            memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

            if( ioctl( fd, STPTI_IOC_GETGLOBALPACKETCOUNT, &UserParams ) == -1 ){
                perror(strerror(errno));
                UserParams.Error = ST_ERROR_BAD_PARAMETER;
            }
            else
                *Count = UserParams.Count;
        }
        else
            UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }
    else
        UserParams.Error = ST_ERROR_UNKNOWN_DEVICE; /* Not initialised */

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_GetBuffersFromSlot(STPTI_Slot_t SlotHandle, STPTI_Buffer_t *BufferHandle_p, STPTI_Buffer_t *RecordBufferHandle_p)
{
    STPTI_Ioctl_GetBuffersFromSlot_t UserParams;

    if( fd != -1 ){/* Is device initialised? */
        if( (BufferHandle_p != NULL) &&
            (RecordBufferHandle_p != NULL) ){/* Test for good return pointer */

            UserParams.SlotHandle = SlotHandle;

            if( ioctl( fd, STPTI_IOC_GETBUFFERSFROMSLOT, &UserParams ) == -1 ){
                perror(strerror(errno));
                UserParams.Error = ST_ERROR_BAD_PARAMETER;
            }
            else
                *BufferHandle_p = UserParams.BufferHandle;
                *RecordBufferHandle_p = UserParams.RecordBufferHandle;
        }
        else
            UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }
    else
        UserParams.Error = ST_ERROR_UNKNOWN_DEVICE; /* Not initialised */

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SetDiscardParams(ST_DeviceName_t DeviceName, U8 NumberOfDiscardBytes)
{
    STPTI_Ioctl_SetDiscardParams_t UserParams;

    if( fd != -1 ){/* Is device initialised? */

        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

        UserParams.NumberOfDiscardBytes = NumberOfDiscardBytes;

        if( ioctl( fd, STPTI_IOC_SETDISCARDPARAMS, &UserParams ) == -1 ){
            perror(strerror(errno));
            UserParams.Error = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
        UserParams.Error = ST_ERROR_UNKNOWN_DEVICE; /* Not initialised */

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SetStreamID(ST_DeviceName_t Device, STPTI_StreamID_t StreamID)
{
    STPTI_Ioctl_SetStreamID_t UserParams;

    if( fd != -1 ){/* Is device initialised? */

        memcpy(UserParams.Device, Device, sizeof(ST_DeviceName_t));

        UserParams.StreamID = StreamID;

        if( ioctl( fd, STPTI_IOC_SETSTREAMID, &UserParams ) == -1 ){
            perror(strerror(errno));
            UserParams.Error = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
        UserParams.Error = ST_ERROR_UNKNOWN_DEVICE; /* Not initialised */

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_HardwareFIFOLevels(ST_DeviceName_t DeviceName, BOOL *Overflow, U16 *InputLevel, U16 *AltLevel, U16 *HeaderLevel)
{
    STPTI_Ioctl_HardwareFIFOLevels_t UserParams;

    if ( Overflow == NULL || InputLevel == NULL || AltLevel == NULL || HeaderLevel == NULL ){
            UserParams.Error = ST_ERROR_BAD_PARAMETER;
            return UserParams.Error;
    }

    if( fd != -1 ){/* Is device initialised? */

        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

        UserParams.Overflow = *Overflow;
        UserParams.InputLevel = *InputLevel;
        UserParams.AltLevel = *AltLevel;
        UserParams.HeaderLevel = *HeaderLevel;

        if( ioctl( fd, STPTI_IOC_HARDWAREFIFOLEVELS, &UserParams ) == -1 ){
            perror(strerror(errno));
            UserParams.Error = ST_ERROR_BAD_PARAMETER;
        }
        else {
            *Overflow = UserParams.Overflow;
            *InputLevel = UserParams.InputLevel;
            *AltLevel = UserParams.AltLevel;
            *HeaderLevel = UserParams.HeaderLevel;
        }
    }
    else
        UserParams.Error = ST_ERROR_UNKNOWN_DEVICE; /* Not initialised */

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_EnableScramblingEvents(STPTI_Slot_t SlotHandle)
{
    STPTI_Ioctl_EnableScramblingEvents_t UserParams;

    UserParams.SlotHandle = SlotHandle;

    if( ioctl( fd, STPTI_IOC_ENABLESCRAMBLINGEVENTS, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

/* Added Indexing Under Linux */
#ifndef STPTI_NO_INDEX_SUPPORT
ST_ErrorCode_t STPTI_IndexAllocate(STPTI_Handle_t SessionHandle, STPTI_Index_t *IndexHandle_p)
{
    STPTI_Ioctl_IndexAllocate_t UserParams;

    if( IndexHandle_p != NULL )
    {
        UserParams.SessionHandle = SessionHandle;

        if( ioctl( fd, STPTI_IOC_INDEXALLOCATE, &UserParams ) == -1 )
        {
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }
        *IndexHandle_p = UserParams.IndexHandle;

    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_IndexStart( ST_DeviceName_t DeviceName )
{
    STPTI_Ioctl_IndexStart_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

    if( ioctl( fd, STPTI_IOC_INDEXSTART, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_IndexStop( ST_DeviceName_t DeviceName )
{
    STPTI_Ioctl_IndexStop_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

    if( ioctl( fd, STPTI_IOC_INDEXSTOP, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_IndexReset(STPTI_Index_t IndexHandle)
{
    STPTI_Ioctl_IndexReset_t UserParams;

    UserParams.IndexHandle = IndexHandle;

    if( ioctl( fd, STPTI_IOC_INDEXRESET, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}



ST_ErrorCode_t STPTI_IndexDisassociate(STPTI_Index_t IndexHandle, STPTI_SlotOrPid_t SlotOrPid)
{
    STPTI_Ioctl_IndexDisassociate_t UserParams;

    UserParams.IndexHandle = IndexHandle;
    UserParams.SlotOrPid   = SlotOrPid;

    if( ioctl( fd, STPTI_IOC_INDEXDISASSOCIATE, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}



ST_ErrorCode_t STPTI_IndexAssociate(STPTI_Index_t IndexHandle, STPTI_SlotOrPid_t SlotOrPid)
{
    STPTI_Ioctl_IndexAssociate_t UserParams;

    UserParams.IndexHandle = IndexHandle;
    UserParams.SlotOrPid   = SlotOrPid;

    if( ioctl( fd, STPTI_IOC_INDEXASSOCIATE, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_IndexDeallocate(STPTI_Index_t IndexHandle)
{
    STPTI_Ioctl_IndexDeallocate_t UserParams;

    UserParams.IndexHandle = IndexHandle;

    if( ioctl( fd, STPTI_IOC_INDEXDEALLOCATE, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}
ST_ErrorCode_t STPTI_IndexSet(STPTI_Index_t IndexHandle, STPTI_IndexDefinition_t IndexMask, U32 MPEGStartCode, U32 MPEGStartCodeMode)
{
    STPTI_Ioctl_IndexSet_t  UserParams;

    UserParams.IndexHandle          = IndexHandle;
    UserParams.IndexMask            = IndexMask;
    UserParams.MPEGStartCode        = MPEGStartCode;
    UserParams.MPEGStartCodeMode    = MPEGStartCodeMode;

    if( ioctl( fd, STPTI_IOC_INDEXSET, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_IndexChain(STPTI_Index_t *IndexHandles, U32 NumberOfHandles)
{
    STPTI_Ioctl_IndexChain_t  UserParams;
    int i;
    
    if( NumberOfHandles > STPTI_INDEXCHAIN_MAX_HANDLES )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    
    for(i=0;i<NumberOfHandles;i++)
    {
        UserParams.IndexHandles[i] = IndexHandles[i];
    }
            
    UserParams.NumberOfHandles      = NumberOfHandles;

    if( ioctl( fd, STPTI_IOC_INDEXCHAIN, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

#endif /* STPTI_NO_INDEX_SUPPORT */





ST_ErrorCode_t STPTI_SlotLinkToRecordBuffer(STPTI_Slot_t Slot, STPTI_Buffer_t Buffer, BOOL DescrambleTS)
{
    STPTI_Ioctl_SlotLinkToRecordBuffer_t UserParams;

    UserParams.Slot = Slot;
    UserParams.Buffer = Buffer;
    UserParams.DescrambleTS = DescrambleTS;

    if( ioctl( fd, STPTI_IOC_SLOTLINKTORECORDBUFFER, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_SlotUnLinkRecordBuffer( STPTI_Slot_t SlotHandle )
{
    STPTI_Ioctl_SlotUnLinkRecordBuffer_t UserParams;

    UserParams.SlotHandle = SlotHandle;

    if( ioctl( fd, STPTI_IOC_SLOTUNLINKRECORDBUFFER, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_SlotSetCorruptionParams(STPTI_Slot_t SlotHandle, U8 Offset, U8 Value )
{
    STPTI_Ioctl_SlotSetCorruptionParams_t UserParams;

    UserParams.SlotHandle = SlotHandle;
    UserParams.Offset = Offset;
    UserParams.Value = Value;

    if( ioctl( fd, STPTI_IOC_SLOTSETCORRUPTIONPARAMS, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}



ST_ErrorCode_t STPTI_TEST_ForceLoader(int Variant)
{
    LoaderVariant=Variant;
    return(ST_NO_ERROR);
}


ST_ErrorCode_t STPTI_SlotUpdatePid(STPTI_Slot_t SlotHandle, STPTI_Pid_t Pid)
{
    STPTI_Ioctl_SlotUpdatePid_t UserParams;

    UserParams.SlotHandle = SlotHandle;
    UserParams.Pid = Pid;

    if( ioctl( fd, STPTI_IOC_SLOTUPDATEPID, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_DmaMalloc(U32 Size, void **Buffer_p, BOOL UseBigPhysArea)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    void *buf;

    /* This allocates and maps in one go */
    buf = mmap(*Buffer_p, Size, PROT_READ | PROT_WRITE , MAP_SHARED, fd,
                UseBigPhysArea ? (off_t)0x10000000 : 0);

    if (buf == MAP_FAILED)
        Error = ST_ERROR_NO_MEMORY;
    else
        *Buffer_p = buf;

    return Error;
}

ST_ErrorCode_t STPTI_DmaFree(void *Buffer_p)
{
    STPTI_Ioctl_DmaFree_t UserParams;

    UserParams.Buffer_p = Buffer_p;

    if( ioctl( fd, STPTI_IOC_DMAFREE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferSetReadPointer(STPTI_Buffer_t BufferHandle, void* Read_p)
{
    STPTI_Ioctl_BufferSetReadPointer_t UserParams;

    UserParams.BufferHandle = BufferHandle;
    UserParams.Read_p = Read_p;

    if( ioctl( fd, STPTI_IOC_BUFFERSETREADPOINTER, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferGetWritePointer(STPTI_Buffer_t BufferHandle, void **Write_p)
{
    STPTI_Ioctl_BufferGetWritePointer_t UserParams;

    UserParams.BufferHandle = BufferHandle;

    if( ioctl( fd, STPTI_IOC_BUFFERGETWRITEPOINTER, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    *Write_p = UserParams.Write_p;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_BufferPacketCount(STPTI_Buffer_t BufferHandle, U32 *Count_p)
{
    STPTI_Ioctl_BufferPacketCount_t UserParams;

    if( Count_p != NULL)
    {
        UserParams.BufferHandle = BufferHandle;
        *Count_p = 0;

        if( ioctl( fd, STPTI_IOC_BUFFERPACKETCOUNT, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }
        *Count_p=UserParams.Count;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_DisableScramblingEvents(STPTI_Slot_t SlotHandle)
{
    STPTI_Ioctl_DisableScramblingEvents_t UserParams;

    UserParams.SlotHandle = SlotHandle;

    if( ioctl( fd, STPTI_IOC_DISABLESCRAMBLINGEVENTS, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_ModifySyncLockAndDrop(ST_DeviceName_t DeviceName, U8 SyncLock, U8 SyncDrop)
{
    STPTI_Ioctl_ModifySyncLockAndDrop_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.SyncLock = SyncLock;
    UserParams.SyncDrop = SyncDrop;

    if( ioctl( fd, STPTI_IOC_MODIFYSYNCLOCKANDDROP, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_SetSystemKey(ST_DeviceName_t DeviceName, U8 *Data)
{
    STPTI_Ioctl_SetSystemKey_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.Data = Data;

    if( ioctl( fd, STPTI_IOC_SETSYSTEMKEY, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_DumpInputTS(STPTI_Buffer_t BufferHandle, U16 bytes_to_capture_per_packet)
{
    STPTI_Ioctl_DumpInputTS_t UserParams;

    UserParams.BufferHandle = BufferHandle;
    UserParams.bytes_to_capture_per_packet = bytes_to_capture_per_packet;

    if( ioctl( fd, STPTI_IOC_DUMPINPUTTS, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

#if defined( STPTI_FRONTEND_HYBRID )

ST_ErrorCode_t STPTI_FrontendAllocate( STPTI_Handle_t PTIHandle, STPTI_Frontend_t * FrontendHandle_p, STPTI_FrontendType_t FrontendType )
{
    STPTI_Ioctl_FrontendAllocate_t UserParams;

    if( FrontendHandle_p != NULL)
    {
        UserParams.PTIHandle    = PTIHandle;
        UserParams.FrontendType = FrontendType;

        if( ioctl( fd, STPTI_IOC_FRONTENDALLOCATE, &UserParams ) == -1 )
        {
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *FrontendHandle_p = UserParams.FrontendHandle;
    }
    else
    {
        UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_FrontendDeallocate( STPTI_Frontend_t FrontendHandle )
{
    STPTI_Ioctl_FrontendDeallocate_t UserParams;

    UserParams.FrontendHandle = FrontendHandle;

    if( ioctl( fd, STPTI_IOC_FRONTENDDEALLOCATE, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_FrontendGetParams( STPTI_Frontend_t FrontendHandle, STPTI_FrontendType_t * FrontendType_p, STPTI_FrontendParams_t * FrontendParams_p )
{
    STPTI_Ioctl_FrontendGetParams_t UserParams;

    if(( FrontendType_p != NULL ) && ( FrontendParams_p != NULL ))
    {
        UserParams.FrontendHandle = FrontendHandle;

        if( ioctl( fd, STPTI_IOC_FRONTENDGETPARAMS, &UserParams ) == -1 )
        {
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *FrontendType_p   = UserParams.FrontendType;
        *FrontendParams_p = UserParams.FrontendParams;
    }
    else
    {
        UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_FrontendGetStatus( STPTI_Frontend_t FrontendHandle, STPTI_FrontendStatus_t * FrontendStatus_p )
{
    STPTI_Ioctl_FrontendGetStatus_t UserParams;

    if( FrontendStatus_p != NULL )
    {
        UserParams.FrontendHandle = FrontendHandle;

        if( ioctl( fd, STPTI_IOC_FRONTENDGETSTATUS, &UserParams ) == -1 )
        {
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }

        *FrontendStatus_p = UserParams.FrontendStatus;
    }
    else
    {
        UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_FrontendInjectData( STPTI_Frontend_t FrontendHandle, STPTI_FrontendSWTSNode_t *FrontendSWTSNode_p, U32 NumberOfSWTSNodes )
{
    STPTI_Ioctl_FrontendInjectData_t UserParams;

    if( FrontendSWTSNode_p != NULL )
    {
        UserParams.FrontendHandle       = FrontendHandle;
        UserParams.FrontendSWTSNode_p   = FrontendSWTSNode_p;
        UserParams.NumberOfSWTSNodes    = NumberOfSWTSNodes;

        if( ioctl( fd, STPTI_IOC_FRONTENDINJECTDATA, &UserParams ) == -1 )
        {
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }
    }
    else
    {
        UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_FrontendLinkToPTI( STPTI_Frontend_t FrontendHandle, STPTI_Handle_t PTIHandle )
{
    STPTI_Ioctl_FrontendLinkToPTI_t UserParams;

    UserParams.FrontendHandle       = FrontendHandle;
    UserParams.PTIHandle            = PTIHandle;

    if( ioctl( fd, STPTI_IOC_FRONTENDLINKTOPTI, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}



ST_ErrorCode_t STPTI_FrontendReset( STPTI_Frontend_t FrontendHandle )
{
    STPTI_Ioctl_FrontendReset_t UserParams;

    UserParams.FrontendHandle       = FrontendHandle;

    if( ioctl( fd, STPTI_IOC_FRONTENDRESET, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_FrontendSetParams( STPTI_Frontend_t FrontendHandle, STPTI_FrontendParams_t * FrontendParams_p )
{
    STPTI_Ioctl_FrontendSetParams_t UserParams;

    if( FrontendParams_p != NULL )
    {
        UserParams.FrontendHandle = FrontendHandle;
        UserParams.FrontendParams = *FrontendParams_p;

        if( ioctl( fd, STPTI_IOC_FRONTENDSETPARAMS, &UserParams ) == -1 )
        {
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }
    }
    else
    {
        UserParams.Error = ST_ERROR_BAD_PARAMETER;
    }

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_FrontendUnLink( STPTI_Frontend_t FrontendHandle )
{
    STPTI_Ioctl_FrontendUnLink_t UserParams;

    UserParams.FrontendHandle       = FrontendHandle;

    if( ioctl( fd, STPTI_IOC_FRONTENDUNLINK, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */


#ifdef STPTI_CAROUSEL_SUPPORT
ST_ErrorCode_t STPTI_CarouselAllocate(STPTI_Handle_t Handle, STPTI_Carousel_t * Carousel_p)
{
    STPTI_Ioctl_CarouselAllocate_t UserParams;

    if( Carousel_p != NULL ){
        UserParams.SessionHandle = Handle;

        if( ioctl( fd, STPTI_IOC_CAROUSELALLOCATE, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }
        *Carousel_p = UserParams.CarouselHandle;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselDeallocate(STPTI_Carousel_t CarouselHandle)
{
    STPTI_Ioctl_CarouselDeallocate_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;

    if( ioctl( fd, STPTI_IOC_CAROUSELDEALLOCATE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselEntryAllocate(STPTI_Handle_t Handle, STPTI_CarouselEntry_t * CarouselEntryHandle_p)
{
    STPTI_Ioctl_CarouselEntryAllocate_t UserParams;

    if( CarouselEntryHandle_p != NULL ){
        UserParams.SessionHandle = Handle;
        UserParams.CarouselEntryHandle = *CarouselEntryHandle_p;

        if( ioctl( fd, STPTI_IOC_CAROUSELENTRYALLOCATE, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }
        *CarouselEntryHandle_p = UserParams.CarouselEntryHandle;
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselEntryDeallocate(STPTI_CarouselEntry_t CarouselEntryHandle)
{
    STPTI_Ioctl_CarouselEntryDeallocate_t UserParams;

    UserParams.CarouselEntryHandle = CarouselEntryHandle;

    if( ioctl( fd, STPTI_IOC_CAROUSELENTRYDEALLOCATE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselEntryLoad(STPTI_CarouselEntry_t CarouselEntry, STPTI_TSPacket_t Packet, U8 FromByte)
{
    STPTI_Ioctl_CarouselEntryLoad_t UserParams;

    if( Packet != NULL)
    {
        UserParams.CarouselEntryHandle = CarouselEntry;
        UserParams.FromByte = FromByte;

        memcpy ( UserParams.Packet, Packet, sizeof (STPTI_TSPacket_t));

        if( ioctl( fd, STPTI_IOC_CAROUSELENTRYLOAD, &UserParams ) == -1 ){
            perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
        }
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselGetCurrentEntry(STPTI_Carousel_t CarouselHandle, STPTI_CarouselEntry_t *Entry_p, U32 *PrivateData_p)
{
    STPTI_Ioctl_CarouselGetCurrentEntry_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;

    if( ioctl( fd, STPTI_IOC_CAROUSELGETCURRENTENTRY, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }
    if (NULL != Entry_p)       *Entry_p       = UserParams.Entry;
    if (NULL != PrivateData_p) *PrivateData_p = UserParams.PrivateData;

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselInsertEntry(STPTI_Carousel_t CarouselHandle, STPTI_AlternateOutputTag_t AlternateOutputTag, STPTI_InjectionType_t InjectionType, STPTI_CarouselEntry_t Entry)
{
    STPTI_Ioctl_CarouselInsertEntry_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;
    UserParams.AlternateOutputTag = AlternateOutputTag;
    UserParams.InjectionType = InjectionType;
    UserParams.Entry = Entry;

    if( ioctl( fd, STPTI_IOC_CAROUSELINSERTENTRY, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselInsertTimedEntry(STPTI_Carousel_t CarouselHandle, U8 *TSPacket_p, U8 CCValue, 
                                              STPTI_InjectionType_t InjectionType, STPTI_TimeStamp_t MinOutputTime,
                                              STPTI_TimeStamp_t MaxOutputTime, BOOL EventOnOutput, BOOL EventOnTimeout,
                                              U32 PrivateData, STPTI_CarouselEntry_t Entry, U8 FromByte)
{
    STPTI_Ioctl_CarouselInsertTimedEntry_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;
    UserParams.TSPacket_p     = TSPacket_p;
    UserParams.CCValue        = CCValue;
    UserParams.InjectionType  = InjectionType;
    UserParams.MinOutputTime  = MinOutputTime;
    UserParams.MaxOutputTime  = MaxOutputTime;
    UserParams.EventOnOutput  = EventOnOutput;
    UserParams.EventOnTimeout = EventOnTimeout;
    UserParams.PrivateData    = PrivateData;
    UserParams.EventOnOutput  = EventOnOutput;
    UserParams.Entry          = Entry;
    UserParams.FromByte       = FromByte;

    if( TSPacket_p != NULL)
    {
        if( ioctl( fd, STPTI_IOC_CAROUSELINSERTTIMEDENTRY, &UserParams ) == -1 ){
            perror(strerror(errno));
            return( ST_ERROR_BAD_PARAMETER );
        }
    }
    else
        UserParams.Error = ST_ERROR_BAD_PARAMETER;
        
    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselLinkToBuffer(STPTI_Carousel_t CarouselHandle, STPTI_Buffer_t BufferHandle)
{
   STPTI_Ioctl_CarouselLinkToBuffer_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;
    UserParams.BufferHandle = BufferHandle;
    
    if( ioctl( fd, STPTI_IOC_CAROUSELLINKTOBUFFER, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselLock(STPTI_Carousel_t CarouselHandle)
{
   STPTI_Ioctl_CarouselLock_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;

    if( ioctl( fd, STPTI_IOC_CAROUSELLOCK, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselRemoveEntry(STPTI_Carousel_t CarouselHandle, STPTI_CarouselEntry_t Entry)
{
   STPTI_Ioctl_CarouselRemoveEntry_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;
    UserParams.Entry = Entry;

    if( ioctl( fd, STPTI_IOC_CAROUSELREMOVEENTRY, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselSetAllowOutput(STPTI_Carousel_t CarouselHandle, STPTI_AllowOutput_t OutputAllowed)
{
   STPTI_Ioctl_CarouselSetAllowOutput_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;
    UserParams.OutputAllowed = OutputAllowed;

    if( ioctl( fd, STPTI_IOC_CAROUSELSETALLOWOUTPUT, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselSetBurstMode(STPTI_Carousel_t CarouselHandle, U32 PacketTimeMs, U32 CycleTimeMs)
{
   STPTI_Ioctl_CarouselSetBurstMode_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;
    UserParams.PacketTimeMs = PacketTimeMs;
    UserParams.CycleTimeMs = CycleTimeMs;

    if( ioctl( fd, STPTI_IOC_CAROUSELSETBURSTMODE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselUnlinkBuffer(STPTI_Carousel_t CarouselHandle, STPTI_Buffer_t BufferHandle)
{
   STPTI_Ioctl_CarouselUnlinkBuffer_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;
    UserParams.BufferHandle = BufferHandle;

    if( ioctl( fd, STPTI_IOC_CAROUSELUNLINKBUFFER, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_CarouselUnLock(STPTI_Carousel_t CarouselHandle)
{
   STPTI_Ioctl_CarouselUnLock_t UserParams;

    UserParams.CarouselHandle = CarouselHandle;

    if( ioctl( fd, STPTI_IOC_CAROUSELUNLOCK, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

#endif /* #ifdef STPTI_CAROUSEL_SUPPORT */


ST_ErrorCode_t STPTI_ObjectEnableFeature(STPTI_Handle_t ObjectHandle, STPTI_FeatureInfo_t FeatureInfo)
{
   STPTI_Ioctl_ObjectEnableFeature_t UserParams;

    UserParams.ObjectHandle = ObjectHandle;
    UserParams.FeatureInfo = FeatureInfo;

    if( ioctl( fd, STPTI_IOC_OBJECTENABLEFEATURE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_ObjectDisableFeature(STPTI_Handle_t ObjectHandle, STPTI_Feature_t Feature)
{
   STPTI_Ioctl_ObjectDisableFeature_t UserParams;

    UserParams.ObjectHandle = ObjectHandle;
    UserParams.Feature = Feature;

    if( ioctl( fd, STPTI_IOC_OBJECTDISABLEFEATURE, &UserParams ) == -1 ){
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}

ST_ErrorCode_t STPTI_PrintDebug( ST_DeviceName_t DeviceName )
{
    STPTI_Ioctl_PrintDebug_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));

    if( ioctl( fd, STPTI_IOC_PRINTDEBUG, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}


ST_ErrorCode_t STPTI_Debug(ST_DeviceName_t DeviceName, char *dbg_class, char *string, int string_size)
{
    STPTI_Ioctl_Debug_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    strcpy(UserParams.dbg_class, dbg_class);
    UserParams.string = string;
    UserParams.string_size = string_size;

    if( ioctl( fd, STPTI_IOC_DEBUG, &UserParams ) == -1 )
    {
        perror(strerror(errno));
        return( ST_ERROR_BAD_PARAMETER );
    }

    return UserParams.Error;
}


