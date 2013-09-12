/******************************************************************************

File name   : net_ioctl.c

Description : API implementation for the Linux ioctl interface into the
	      kernel. Initially used by the stack bypass device.

COPYRIGHT (C) 2007 STMicroelectronics

******************************************************************************/

#ifdef ST_OSLINUX

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "../linux/stnet_ioctl/stnet_ioctl_ioctl.h"

/*** LOCAL VARIABLES *********************************************************/
static int fd = -1;		/* not open */
static char UseCount = 0;	/* Number of initialised devices */

#endif

/*** METHODS ****************************************************************/

ST_ErrorCode_t STNETi_ioctl_Init(const ST_DeviceName_t DeviceName,
		const STNET_InitParams_t *InitParams)
{
#ifdef ST_OSLINUX
	STNET_Ioctl_Init_t UserParams;
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	if (!UseCount) {
		char *devpath;

		/* get the path for the device */
		devpath = getenv("STNET_IOCTL_DEV_PATH");
		if (!devpath) {
			STTBX_Print(("The dev path enviroment variable is not "
				"defined: STNET_IOCTL_DEV_PATH\n"));
			return ST_ERROR_BAD_PARAMETER;
		}

		fd = open( devpath, O_RDWR );  /* open it */
		if( fd == -1 ) {
			perror("STNETi_Init() Open error");
			return ST_ERROR_BAD_PARAMETER;
		}
	}
	UseCount++;

	memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
	UserParams.InitParams = *InitParams;

	/* IOCTL the command */
	if (ioctl(fd, STNET_IOC_INIT, &UserParams) < 0)
	{
	    /* IOCTL failed */
	    if (errno == EBADF)
		ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
	    else
		ErrorCode = ST_ERROR_BAD_PARAMETER;

	    perror("STNETi_Init() Ioctl error");
	}
	else
	{
	    /* IOCTL is successfull: retrieve Error code */
	    ErrorCode = UserParams.ErrorCode;
	}

	if( ErrorCode != ST_NO_ERROR ){
		UseCount--;
		if( !UseCount ){ /* First initialisation failed */
			close(fd);
			fd = -1;
		}
	}

	return ErrorCode;
	
#else
	UNUSED_PARAMETER(DeviceName);
	UNUSED_PARAMETER(InitParams);

	return ST_ERROR_BAD_PARAMETER;
#endif
}

ST_ErrorCode_t STNETi_ioctl_Term(const ST_DeviceName_t DeviceName,
		const STNET_TermParams_t *TermParams)
{
#ifdef ST_OSLINUX
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STNET_Ioctl_Term_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.TermParams = *TermParams;

    /* IOCTL the command */
    if (ioctl(fd, STNET_IOC_TERM, &UserParams) != 0)
    {
        /* IOCTL failed */
        if (errno == EBADF)
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        else
            ErrorCode = ST_ERROR_BAD_PARAMETER;

        perror("STNET_IOCTL_Term() Ioctl error");
    }
    else
    {
        /* IOCTL is successful: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }


    if( UseCount && (ErrorCode == ST_NO_ERROR) ){
        UseCount--;
        if( !UseCount ){ /* Last Termination */
            close(fd);
            fd = -1;
        }
    }
    return ErrorCode;
#else
	UNUSED_PARAMETER(DeviceName);
	UNUSED_PARAMETER(TermParams);
	return ST_ERROR_BAD_PARAMETER;
#endif
}

ST_ErrorCode_t STNETi_ioctl_Open(const ST_DeviceName_t DeviceName,
		const STNET_OpenParams_t *OpenParams, STNET_Handle_t *Handle)
{
#ifdef ST_OSLINUX
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STNET_Ioctl_Open_t UserParams;

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.OpenParams = *OpenParams;

    /* IOCTL the command */
    if (ioctl(fd, STNET_IOC_OPEN, &UserParams) != 0)
    {
        /* IOCTL failed */
        if (errno == EBADF)
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        else
            ErrorCode = ST_ERROR_BAD_PARAMETER;

        perror("STNET_IOCTL_OPEN() Ioctl error");
    }
    else
    {
        /* IOCTL is successful: */
        /* get the handle. */
        *Handle = UserParams.Handle;
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
#else
	UNUSED_PARAMETER(DeviceName);
	UNUSED_PARAMETER(OpenParams);
	UNUSED_PARAMETER(Handle);
	return ST_ERROR_BAD_PARAMETER;
#endif
}

ST_ErrorCode_t STNETi_ioctl_Close(STNET_Handle_t Handle)
{
#ifdef ST_OSLINUX
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STNET_Ioctl_Close_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STNET_IOC_CLOSE, &UserParams) != 0)
    {
        /* IOCTL failed */
        if (errno == EBADF)
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        else
            ErrorCode = ST_ERROR_BAD_PARAMETER;

        perror("STNET_IOC_CLOSE() Ioctl error");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
#else
	UNUSED_PARAMETER(Handle);
	return ST_ERROR_BAD_PARAMETER;
#endif
}

ST_ErrorCode_t STNETi_ioctl_Start(STNET_Handle_t Handle,
		const STNET_StartParams_t *StartParams)
{
#ifdef ST_OSLINUX
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STNET_Ioctl_Start_t UserParams;

    UserParams.Handle = Handle;
    /* StartParams is not used for any kernel device, so could be NULL */
    if (StartParams)
	UserParams.StartParams = *StartParams;
    else
	memset(&UserParams.StartParams, 0, sizeof(UserParams.StartParams));

    /* issue the command via the IOCTL system call */
    if (ioctl(fd, STNET_IOC_START, &UserParams) != 0)
    {
        /* IOCTL failed */
        if (errno == EBADF)
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        else
            ErrorCode = ST_ERROR_BAD_PARAMETER;

        perror("STNET_IOC_START() Ioctl error");
    }
    else
    {
        /* IOCTL was successful: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
#else
	UNUSED_PARAMETER(Handle);
	UNUSED_PARAMETER(StartParams);
	return ST_ERROR_BAD_PARAMETER;
#endif
}

ST_ErrorCode_t STNETi_ioctl_Stop(STNET_Handle_t Handle)
{
#ifdef ST_OSLINUX
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STNET_Ioctl_Stop_t UserParams;

    UserParams.Handle = Handle;

    /* issue the command via the IOCTL system call */
    if (ioctl(fd, STNET_IOC_STOP, &UserParams) != 0)
    {
        /* IOCTL failed */
        if (errno == EBADF)
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        else
            ErrorCode = ST_ERROR_BAD_PARAMETER;

        perror("STNET_IOC_STOP() Ioctl error");
    }
    else
    {
        /* IOCTL was successful: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
#else
	UNUSED_PARAMETER(Handle);
	return ST_ERROR_BAD_PARAMETER;
#endif
}


ST_ErrorCode_t STNETi_ioctl_SetReceiverConfig(STNET_Handle_t Handle,
		STNET_ReceiverConfigParams_t *ConfigParams)
{
#ifdef ST_OSLINUX
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STNET_Ioctl_ReceiverConfigParams_t UserParams;

    UserParams.Handle = Handle;
    UserParams.ConfigParams = *ConfigParams;

    /* issue the command via the IOCTL system call */
    if (ioctl(fd, STNET_IOC_CONFIG, &UserParams) != 0)
    {
        /* IOCTL failed */
        if (errno == EBADF)
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        else
            ErrorCode = ST_ERROR_BAD_PARAMETER;

        perror("STNET_IOC_CONFIG() Ioctl error");
    }
    else
    {
        /* IOCTL was successful: retrieve Error code and updated config */
        ErrorCode = UserParams.ErrorCode;
	*ConfigParams = UserParams.ConfigParams;
    }

    return ErrorCode;
#else
	UNUSED_PARAMETER(Handle);
	UNUSED_PARAMETER(ConfigParams);
	return ST_ERROR_BAD_PARAMETER;
#endif
}

