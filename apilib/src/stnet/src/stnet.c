/******************************************************************************

File name   : stnet.c

Description : STNET IP packets receiver transmitter

COPYRIGHT (C) 2007 STMicroelectronics

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stos.h"
#include "stnet.h"
#include "net.h"

#ifndef ST_OSLINUX
#include "sttbx.h"
#include "nginit.h"
#endif

/* Type specific APIs */
#include "net_ioctl.h"
#include "net_recvtrans.h"


/* Private Types ----------------------------------------------------------- */


/* Private Variables ------------------------------------------------------- */
static STNETi_Device_t  STNETi_Device[STNETi_MAX_DEVICES];
static BOOL FirstInitDone=FALSE;




/* Private functions prototypes -------------------------------------------- */


/* Global Variables -------------------------------------------------------- */
#ifndef ST_OSLINUX
NGsockaddr            NG_SocketAddr;
#endif
/* Public functions -------------------------------------------------------- */

/******************************************************************************
Name         : STNET_Init()

Description  : Initializes STNET. Creates an instance of STNET
                and initializes ethernet hardware

Parameters   : ST_DeviceName_t      DeviceName      STNET device name
               STNET_InitParams_t  *InitParams_p    Pointer to the params structure

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNET_Init(const ST_DeviceName_t DeviceName, const STNET_InitParams_t *InitParams_p)
{
#if defined (STNET_COMPLETE_SUPPORT)
    U32                 Index;
    STNETi_Device_t    *Device_p = NULL;
#endif
    ST_ErrorCode_t      ErrorCode= ST_NO_ERROR;

	if (DeviceName == NULL || InitParams_p == NULL)
		return ST_ERROR_BAD_PARAMETER;
	if (strlen(DeviceName) >= ST_MAX_DEVICE_NAME)
		return ST_ERROR_BAD_PARAMETER;

	switch(InitParams_p->DeviceType)
	{
#ifdef ST_OSLINUX
	case STNET_DEVICE_RECEIVER_SB_INJECTER:
	        ErrorCode = STNETi_ioctl_Init(DeviceName, InitParams_p);
	        break;
#endif

	case STNET_DEVICE_RECEIVER:
	case STNET_DEVICE_TRANSMITTER:
#if defined (STNET_COMPLETE_SUPPORT)
#if defined(OSPLUS_INPUT_STREAM_BYPASS)
    if(InitParams_p->DeviceType == STNET_DEVICE_RECEIVER)
    {
        /* message queue for receiving pkt from OSPLUS task */
        STNETi_TaskMQ_p = message_create_queue_timeout(sizeof(NGbuf **), STNETi_RECEIVER_TASK_MQ_SIZE);
        if(STNETi_TaskMQ_p == NULL )
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STNET Receiver Task message_create_queue failed\n"));
            return ST_ERROR_NO_MEMORY;
        }
        NG_SocketAddr.sin_addr = 0;
        NG_SocketAddr.sin_port = 0;
        NG_Start=FALSE;
    }

#endif
	     /* check if already initialsed */
	     STOS_TaskLock();
	     for(Index = 0; Index < STNETi_MAX_DEVICES; Index++)
	     {
		 if((STNETi_Device[Index].State != DEVICE_NOT_USED) && (STNETi_Device[Index].State != DEVICE_TERMINATED))
		 {
		     /* check if any other device is initialised with the same name */
		     if((strcmp(STNETi_Device[Index].DeviceName, DeviceName) == 0)||(STNETi_Device[Index].DeviceType == InitParams_p->DeviceType))
		     {
			 STOS_TaskUnlock();
			 STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Device %s already initialised\n", DeviceName));
			 return ST_ERROR_ALREADY_INITIALIZED;
		     }
		 }
		 else if(Device_p == NULL)
		 {
		     Device_p = &STNETi_Device[Index];
		 }
	     }
	     if(Device_p == NULL)
	     {
		 STOS_TaskUnlock();
		 STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Maximum possible devices[%d] already initialised\n", STNETi_MAX_DEVICES));
		 return ST_ERROR_NO_MEMORY;
	     }
	     Device_p->State = DEVICE_INITIALISING;

 #ifndef ST_OSLINUX
	     if(!FirstInitDone)
	     if((ErrorCode = STNETi_NexGenInit(InitParams_p)) != ST_NO_ERROR)
	     {
		 Device_p->State = DEVICE_NOT_USED;
		 STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "NexGen init failed\n"));
		 return ErrorCode;
	     }
	     ngInetATON(InitParams_p->IPAddress, &Device_p->IPAddress);
 #endif
	     FirstInitDone = TRUE;

	     STOS_TaskUnlock();

	     strcpy(Device_p->DeviceName, DeviceName);
	     Device_p->DeviceType      = InitParams_p->DeviceType;
	     Device_p->DriverPartition = InitParams_p->DriverPartition;
	     Device_p->NCachePartition = InitParams_p->NCachePartition;
	     Device_p->Num_Open        = 0;
	     Device_p->HandleList_p    = NULL;


	    Device_p->State = DEVICE_INITIALISED;
#else
		FirstInitDone = TRUE;	/* To fix a build warning */
		ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
#endif
		break;

	default:
		ErrorCode = ST_ERROR_BAD_PARAMETER;
		break;

	}	/* switch */

    return ErrorCode;
}

/******************************************************************************
Name         : STNET_Term()

Description  : Terminate STNET

Parameters   : ST_DeviceName_t      DeviceName      STNET device name
               STNET_TermParams_t  *TermParams_p    Pointer to the params structure

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNET_Term(const ST_DeviceName_t DeviceName, const STNET_TermParams_t  *TermParams_p)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    U32                 Index;
    STNETi_Device_t    *Device_p = NULL;
#if defined (STNET_COMPLETE_SUPPORT)
    STNETi_Handle_t    *Handle_p;
#endif

	if (DeviceName == NULL || TermParams_p == NULL)
		return ST_ERROR_BAD_PARAMETER;

    /* retrive the Device from DeviceName */
    STOS_TaskLock();
    for(Index = 0; Index < STNETi_MAX_DEVICES; Index++)
    {
        if((STNETi_Device[Index].State == DEVICE_INITIALISED) &&
           (strcmp(STNETi_Device[Index].DeviceName, DeviceName) == 0))
        {
            Device_p = &STNETi_Device[Index];
        }
    }

    if(Device_p == NULL)
    {
	/* Not a userspace device, must be a kernelspace device */
        STOS_TaskUnlock();
#ifdef ST_OSLINUX
        return(STNETi_ioctl_Term(DeviceName, TermParams_p));
#endif
    }
    Device_p->State = DEVICE_TERMINATING;
    STOS_TaskUnlock();


#if defined (STNET_COMPLETE_SUPPORT)
    if(TermParams_p->ForceTerminate == TRUE)
    {
        if(Device_p->HandleList_p != NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Device has open handles\n", DeviceName));
            return ST_ERROR_OPEN_HANDLE;
        }
    }
    else
    {
        /*Need to close all handles and then terminate*/
        switch(Device_p->DeviceType)
        {

            case STNET_DEVICE_RECEIVER:
                STNETi_Receiver_CloseAll(Device_p);
            break;


            case STNET_DEVICE_TRANSMITTER:
                STNETi_Transmitter_CloseAll(Device_p);
            break;

            default:
            break;

        }

    }

    Device_p->State = DEVICE_TERMINATED;
#else
    ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
#endif

    return ErrorCode;
}

/******************************************************************************
Name         : STNET_GetRevision()

Description  : version

Parameters   :

Return Value : version string
******************************************************************************/

//GETREVISION_LIB_IMPLEMENTATION(STNET, STNET_IOC_GETREVISION,
//				"STNET_IOCTL_DEV_PATH",STNET_REVISION);


/******************************************************************************
Name         : STNET_Open()

Description  : Open a handle on the STNET device
               Open network socket and configure

Parameters   : ST_DeviceName_t      DeviceName      STNET device name
               STNET_OpenParams_t  *OpenParams_p    Pointer to the params structure
               STNET_Handle_t      *Handle          handle returned by open

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNET_Open(const ST_DeviceName_t DeviceName, const STNET_OpenParams_t *OpenParams, STNET_Handle_t *Handle)
{
    U8                  Index;
    STNETi_Device_t    *Device_p = NULL;
    ST_ErrorCode_t	ErrorCode = ST_NO_ERROR;

    if (DeviceName == NULL || OpenParams == NULL || Handle == NULL)
	return ST_ERROR_BAD_PARAMETER;

    /* retrieve the Device from DeviceName */
    STOS_TaskLock();
    for(Index = 0; Index < STNETi_MAX_DEVICES; Index++)
    {
        if((STNETi_Device[Index].State == DEVICE_INITIALISED) &&
           (strcmp(STNETi_Device[Index].DeviceName, DeviceName) == 0))
        {
            Device_p = &STNETi_Device[Index];
        }
    }

    if(Device_p == NULL)
    {

	/* Not a userspace device, must be a kernelspace device */
        STOS_TaskUnlock();
#ifdef ST_OSLINUX
        return(STNETi_ioctl_Open(DeviceName, OpenParams, Handle));
#endif
    }

    STOS_TaskUnlock();


#if defined (STNET_COMPLETE_SUPPORT)

    switch(Device_p->DeviceType)
    {

        case STNET_DEVICE_RECEIVER:
            ErrorCode =   STNETi_Receiver_Open(Device_p,&Index,OpenParams);
        break;


        case STNET_DEVICE_TRANSMITTER:
            ErrorCode =   STNETi_Transmitter_Open(Device_p,&Index,OpenParams);
        break;

        default:
        break;

    }

    if(ErrorCode == ST_NO_ERROR)
    {
        *Handle = (Device_p->DeviceType) << 8;/* MSB contains type of device*/
        *Handle |= Index;/*LSB contains the index of instance*/
    }
#else
    ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
#endif
    return ErrorCode;
}

/******************************************************************************
Name         : STNET_Close()

Description  : Close a handle opened by STNET_Open

Parameters   : STNET_Handle_t       Handle          handle of the STNET object

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNET_Close(STNET_Handle_t Handle)
{
#if defined (STNET_COMPLETE_SUPPORT)
    STNETi_Device_t	*Device_p;
    U8			Index;
#endif
    ST_ErrorCode_t	ErrorCode = ST_NO_ERROR;

#ifdef ST_OSLINUX
	/* Check if it's a kernel-space device */
	if (DEV_TYPE(Handle) == STNET_DEVICE_RECEIVER_SB_INJECTER)
		return(STNETi_ioctl_Close(Handle));
#endif

#if defined (STNET_COMPLETE_SUPPORT)
    /* retrieve the Device from DeviceName */
    STOS_TaskLock();
    for(Index = 0; Index < STNETi_MAX_DEVICES; Index++)
    {
        if((STNETi_Device[Index].State == DEVICE_INITIALISED) &&
           (DEV_TYPE(Handle) == STNETi_Device[Index].DeviceType))
        {
            Device_p = &STNETi_Device[Index];
            break;
        }
    }
    if(Device_p == NULL)
    {
        STOS_TaskUnlock();
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Device %s not initialised\n", Device_p->DeviceType));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    STOS_TaskUnlock();

    Index = INSTANCE_NO(Handle);

    switch(Device_p->DeviceType)
    {

        case STNET_DEVICE_RECEIVER:
            ErrorCode =   STNETi_Receiver_Close(Device_p,Index);
        break;


        case STNET_DEVICE_TRANSMITTER:
            ErrorCode =   STNETi_Transmitter_Close(Device_p,Index);
        break;

        default:
        break;

    }
#else
    ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
#endif

    return ErrorCode;
}

/******************************************************************************
Name         : STNET_Start()

Description  : Start receiver task to receive data from opened socket

Parameters   : STNET_Handle_t               Handle              handle returned by open
               STNET_ReceiverStartParams_t *ReceiverStartParams start parameters

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNET_Start(STNET_Handle_t Handle, const STNET_StartParams_t *StartParams)
{

#if defined (STNET_COMPLETE_SUPPORT)
    STNETi_Device_t    *Device_p = NULL;
    U8 Index;
#endif
    ST_ErrorCode_t	ErrorCode = ST_NO_ERROR;

	/* Check if it's a kernel-space device */
#ifdef ST_OSLINUX
	if (DEV_TYPE(Handle) == STNET_DEVICE_RECEIVER_SB_INJECTER)
		return(STNETi_ioctl_Start(Handle, StartParams));
#endif

#if defined (STNET_COMPLETE_SUPPORT)
    STOS_TaskLock();
    for(Index = 0; Index < STNETi_MAX_DEVICES; Index++)
    {
        if((STNETi_Device[Index].State == DEVICE_INITIALISED) &&
           (DEV_TYPE(Handle) == STNETi_Device[Index].DeviceType))
        {
            Device_p = &STNETi_Device[Index];
            break;
        }
    }
    if(Device_p == NULL)
    {
        STOS_TaskUnlock();
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Device %s not initialised\n", Device_p->DeviceType));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    Index = INSTANCE_NO(Handle);

    switch(Device_p->DeviceType)
    {

        case STNET_DEVICE_RECEIVER:
            STNETi_Receiver_Start(Device_p,Index,&StartParams->u.ReceiverParams);
#ifndef ST_OSLINUX
            NG_Start=TRUE;
#endif
        break;


        case STNET_DEVICE_TRANSMITTER:
            STNETi_Transmitter_Start(Device_p,Index,&StartParams->u.TransmitterParams);
        break;

        default:
        break;

    }
#else
    ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
#endif

    return ErrorCode;
}

/******************************************************************************
Name         : STNET_Stop()

Description  : Stop receiver task

Parameters   : STNET_Handle_t               Handle              handle returned by open

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNET_Stop(STNET_Handle_t Handle)
{

#if defined (STNET_COMPLETE_SUPPORT)
    STNETi_Device_t    *Device_p = NULL;
    U8 Index;
#endif
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	/* Check if it's a kernel-space device */
#ifdef ST_OSLINUX
	if (DEV_TYPE(Handle) == STNET_DEVICE_RECEIVER_SB_INJECTER)
		return(STNETi_ioctl_Stop(Handle));
#endif

#if defined (STNET_COMPLETE_SUPPORT)
    STOS_TaskLock();
    for(Index = 0; Index < STNETi_MAX_DEVICES; Index++)
    {
        if((STNETi_Device[Index].State == DEVICE_INITIALISED) &&
           (DEV_TYPE(Handle) == STNETi_Device[Index].DeviceType))
        {
            Device_p = &STNETi_Device[Index];
            break;
        }
    }
    if(Device_p == NULL)
    {
        STOS_TaskUnlock();
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Device %s not initialised\n", Device_p->DeviceType));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    Index = INSTANCE_NO(Handle);

    switch(Device_p->DeviceType)
    {

        case STNET_DEVICE_RECEIVER:
            STNETi_Receiver_Stop(Device_p,Index);
#ifndef ST_OSLINUX
            NG_Start=FALSE;
#endif
        break;


        case STNET_DEVICE_TRANSMITTER:
            STNETi_Transmitter_Stop(Device_p,Index);
        break;

        default:
        break;

    }
#else
    ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
#endif
    return ErrorCode;

}



/******************************************************************************
Name         : STNET_GetCapability()

Description  : Get device capabilities

Parameters   : ST_DeviceName_t      DeviceName      STNET device name
               STNET_Capability_t  *Capability      Pointer to capability structure

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNET_GetCapability(const ST_DeviceName_t DeviceName, STNET_Capability_t *Capability)
{


    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}

/******************************************************************************
Name         : STNET_ReceiverGetWritePointer()

Description  : Get receiver buffer write pointer

Parameters   : STNET_Handle_t               Handle              handle returned by open
               void                       **Address             Address returned

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNET_ReceiverGetWritePointer(STNET_Handle_t Handle, void **Address)
{
#if defined (STNET_COMPLETE_SUPPORT)
    STNETi_Handle_t    *Handle_p ;
    STNETi_Device_t *Device_p = NULL;
    U8 Index=0;

    STOS_TaskLock();
    for(Index = 0; Index < STNETi_MAX_DEVICES; Index++)
    {
        if((STNETi_Device[Index].State == DEVICE_INITIALISED) &&
           (DEV_TYPE(Handle) == STNETi_Device[Index].DeviceType))
        {
            Device_p = &STNETi_Device[Index];
            break;
        }
    }
    if(Device_p == NULL)
    {
        STOS_TaskUnlock();
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Device %s not initialised\n", Device_p->DeviceType));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    STOS_TaskUnlock();


    if(Device_p->DeviceType != STNET_DEVICE_RECEIVER)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "not a receiver object\n"));
        return STNET_ERROR_INVALID_DEVICE_TYPE;
    }

    Index = INSTANCE_NO(Handle);

    Handle_p = STNETi_GetHandlefromList(Device_p, Index);

    /* check if receiver is started */
    if(Handle_p->Type.Receiver.State == RECEIVER_STOPPED)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "receiver not active\n"));
        return STNET_ERROR_ALREADY_STOPPED;
    }

    *Address = Handle_p->Type.Receiver.BufferWritePtr;
#endif

    return ST_NO_ERROR;
}

/******************************************************************************
Name         : STNET_ReceiverSetReadPointer()

Description  : Set receiver buffer read pointer

Parameters   : STNET_Handle_t               Handle              handle returned by open
               void                        *Address             Address to set

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNET_ReceiverSetReadPointer(STNET_Handle_t Handle, void *Address)
{
#if defined (STNET_COMPLETE_SUPPORT)
    STNETi_Handle_t    *Handle_p ;
    STNETi_Device_t *Device_p = NULL;
    U8 Index=0;

    STOS_TaskLock();
    for(Index = 0; Index < STNETi_MAX_DEVICES; Index++)
    {
        if((STNETi_Device[Index].State == DEVICE_INITIALISED) &&
           (DEV_TYPE(Handle) == STNETi_Device[Index].DeviceType))
        {
            Device_p = &STNETi_Device[Index];
            break;
        }
    }
    if(Device_p == NULL)
    {
        STOS_TaskUnlock();
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Device %s not initialised\n", Device_p->DeviceType));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    STOS_TaskUnlock();


    if(Device_p->DeviceType != STNET_DEVICE_RECEIVER)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "not a receiver object\n"));
        return STNET_ERROR_INVALID_DEVICE_TYPE;
    }

    Index = INSTANCE_NO(Handle);

    Handle_p = STNETi_GetHandlefromList(Device_p, Index);

    /* check if receiver is started */
    if(Handle_p->Type.Receiver.State == RECEIVER_STOPPED)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "receiver not active\n"));
        return STNET_ERROR_ALREADY_STOPPED;
    }

    if((Address < (void *)Handle_p->Type.Receiver.BufferBase) ||
       (Address > (void *)(Handle_p->Type.Receiver.BufferBase + Handle_p->Type.Receiver.BufferSize)))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "buffer Address[%08X] not valid Base[%08X] Size[%08X]\n",
                Address,
                Handle_p->Type.Receiver.BufferBase,
                Handle_p->Type.Receiver.BufferBase + Handle_p->Type.Receiver.BufferSize));
        return ST_ERROR_BAD_PARAMETER;
    }

    Handle_p->Type.Receiver.BufferReadPtr = Address;
#endif

    return ST_NO_ERROR;
}


/******************************************************************************
Name         : STNET_SetReceiverConfig()

Description  : Set receiver configuration parameters

Parameters   : STNET_Handle_t               Handle              handle returned by open
               STNET_ReceiverConfigParams_t *ConfigParams       New parameter values

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNET_SetReceiverConfig(STNET_Handle_t Handle,
			STNET_ReceiverConfigParams_t *ConfigParams)
{
	ST_ErrorCode_t ErrorCode = ST_ERROR_UNKNOWN_DEVICE;

	if (ConfigParams == NULL)
		return ST_ERROR_BAD_PARAMETER;

#ifdef ST_OSLINUX
	/* Check if it's a kernel-space device */
	if (DEV_TYPE(Handle) == STNET_DEVICE_RECEIVER_SB_INJECTER)
	    ErrorCode = STNETi_ioctl_SetReceiverConfig(Handle, ConfigParams);
#endif

	return(ErrorCode);
}


/******************************************************************************
Function Name :STNET_GetRevision
  Description :
   Parameters :
******************************************************************************/
ST_Revision_t  STNET_GetRevision(void)
{
    return( STNET_REVISION );
}


/* EOF --------------------------------------------------------------------- */

