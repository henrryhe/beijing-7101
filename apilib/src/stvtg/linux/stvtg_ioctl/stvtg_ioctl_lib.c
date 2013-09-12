/*****************************************************************************
 *
 *  Module      : stvtg_ioctl_lib.c
 *  Date        : 15-10-2005
 *  Author      : ATROUSWA
 *  Description : Implementation for calling STAPI ioctl functions
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>


#include "stvtg.h"

#include "stvtg_ioctl.h"


/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

#if defined(ST_7100) || defined(ST_7109)|| defined(ST_7200)|| defined(ST_7111)|| defined(ST_7105)
#define STVTG_MAX_OPEN    3   /* Max number of Open() allowed per Init() */
#elif defined(ST_5528)
#define STVTG_MAX_OPEN    2   /* Max number of Open() allowed per Init() */
#endif


/*** LOCAL VARIABLES *********************************************************/
static int fd=-1; /* not open */
static int UseCount=0; /* Number of initialised devices */

/*static BOOL                     FirstInitDone = FALSE;*/
/*static BOOL                     IsPollRegistered = FALSE;*/
/*static U32                      NbInstance = 0 ;*/
/*static FILE                     *DeviceFile;*/ /* Device FILE */

/*** METHODS ****************************************************************/

ST_ErrorCode_t STVTG_Init( const ST_DeviceName_t DeviceName, const STVTG_InitParams_t * const InitParams_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||                        /* There must be parameters */
        (InitParams_p->MaxOpen > STVTG_MAX_OPEN) ||      /* No more than MAX_OPEN open handles supported */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }


    if( !UseCount ){ /* First time? */
        char *devpath;

        devpath = getenv("STVTG_IOCTL_DEV_PATH");  /* get the path for the device */

        if( devpath ){
            fd = open( devpath, O_RDWR );  /* open it */
            if( fd == -1 ){
                perror(strerror(errno));

                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }

        }
        else{
            fprintf(stderr,"The dev path enviroment variable is not defined: STVTG_IOCTL_DEV_PATH \n");
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    UseCount++;

    if( ErrorCode == ST_NO_ERROR ){
        STVTG_Ioctl_Init_t UserParams;

        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
        UserParams.InitParams = *InitParams_p;

        /* IOCTL the command */
        if (ioctl(fd, STVTG_IOC_INIT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;



        }
    }

    return ErrorCode;
}

ST_ErrorCode_t STVTG_Open(const ST_DeviceName_t DeviceName, const STVTG_OpenParams_t * const OpenParams_p, STVTG_Handle_t * const Handle_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_Open_t UserParams;

    /* Exit now if parameters are bad */
    if (   (OpenParams_p == NULL)                           /* There must be parameters ! */
        || (Handle_p == NULL)                               /* Pointer to handle should be valid ! */
        || (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.OpenParams = *OpenParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_OPEN, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOCTL_OPEN():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* get the handle. */
        *Handle_p = UserParams.Handle;
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STVTG_Close(STVTG_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_Close_t UserParams;

    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_CLOSE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_CLOSE():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STVTG_Term(const ST_DeviceName_t DeviceName, const STVTG_TermParams_t *const TermParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_Term_t UserParams;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (TermParams_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.TermParams = *TermParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_TERM, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOCTL_Term():Ioctl error\n");
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
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVTG_GetRevision
Returns     : Error code
*******************************************************************************/
ST_Revision_t STVTG_GetRevision( void )
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return(VTG_Revision);
}


/*******************************************************************************
Name        : STVTG_GetCapability
Returns     : Error code, Capability_p
*******************************************************************************/
ST_ErrorCode_t  STVTG_GetCapability( const ST_DeviceName_t DeviceName,
                                    STVTG_Capability_t *  const Capability_p)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_GetCapability_t UserParams;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (Capability_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

   if (!UseCount)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.Capability = *Capability_p;


 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_GETCAPABILITY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_GETCAPABILITY():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code and capability parameter */
        ErrorCode = UserParams.ErrorCode;
        *Capability_p = UserParams.Capability;
    }

    return ErrorCode;
}


/*******************************************************************************
Name        : STVTG_GetVtgId
Returns     : Error code,  VtgId_p
*******************************************************************************/
ST_ErrorCode_t  STVTG_GetVtgId( const ST_DeviceName_t DeviceName,
                                    STVTG_VtgId_t *  const VtgId_p)

{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_GetVtgId_t      UserParams;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (VtgId_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

   if (!UseCount)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.VtgId = *VtgId_p;

 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_GETVTGID, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_GETVTGID():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *VtgId_p  = UserParams.VtgId;
    }
    return ErrorCode;
}
/******************************************************************************
Name        : STVTG_GetModeSyncParams
Returns     : Error code,
*******************************************************************************/
ST_ErrorCode_t  STVTG_GetModeSyncParams( const STVTG_Handle_t Handle,
                               			 STVTG_SyncParams_t * const SyncParams)

{
    ST_ErrorCode_t                     ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_GetModeSyncParams_t    UserParams;

    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (SyncParams == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    UserParams.Handle = Handle;
    UserParams.SyncParams = *SyncParams;

 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_GETMODESYNCPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_GETMODESYNCPARAMS():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
       *SyncParams = UserParams.SyncParams;
	}
    return ErrorCode;
}
/*******************************************************************************
/*******************************************************************************
Name        : STVTG_SetMode
Returns     : Error code,
*******************************************************************************/
ST_ErrorCode_t  STVTG_SetMode( const STVTG_Handle_t Handle,
                               const STVTG_TimingMode_t  Mode)

{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_SetMode_t       UserParams;

    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    UserParams.Handle = Handle;
    UserParams.Mode   = Mode;

 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_SETMODE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_SETMODE():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVTG_SetOptionalConfiguration
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t  STVTG_SetOptionalConfiguration( const STVTG_Handle_t Handle,
                                     const STVTG_OptionalConfiguration_t * const OptionalConfiguration_p)
{
    STVTG_Ioctl_SetOptionalConfiguration_t       UserParams;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;


    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (OptionalConfiguration_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    UserParams.Handle = Handle;
    UserParams.OptionalConfiguration = *OptionalConfiguration_p;

 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_SETOPTIONALCONFIGURATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_SETOPTIONALCONFIGURATION():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVTG_SetSlaveModeParams
Returns     : Error code,
*******************************************************************************/
ST_ErrorCode_t  STVTG_SetSlaveModeParams( const STVTG_Handle_t Handle,
                                          const STVTG_SlaveModeParams_t * const SlaveModeParams_p)
{
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_SetSlaveModeParams_t        UserParams;

    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (SlaveModeParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    UserParams.Handle          = Handle;
    UserParams.SlaveModeParams = *SlaveModeParams_p;

 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_SETSLAVEMODEPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_SETSLAVEMODEPARAMS():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVTG_GetMode
Returns     : Error code,
*******************************************************************************/
ST_ErrorCode_t  STVTG_GetMode( const STVTG_Handle_t Handle,
                               STVTG_TimingMode_t * const TimingMode_p,
                               STVTG_ModeParams_t * const ModeParams_p)

{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_GetMode_t       UserParams;

    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if ((ModeParams_p == NULL) || (TimingMode_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    UserParams.Handle = Handle;
    UserParams.TimingMode = *TimingMode_p;
    UserParams.ModeParams = *ModeParams_p;

 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_GETMODE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_GETMODE():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
       *TimingMode_p = UserParams.TimingMode;
       *ModeParams_p = UserParams.ModeParams;
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVTG_GetOptionalConfiguration
Returns     : Error code,
*******************************************************************************/
ST_ErrorCode_t  STVTG_GetOptionalConfiguration( const STVTG_Handle_t Handle,
                                                STVTG_OptionalConfiguration_t * const OptionalConfiguration_p)

{
    ST_ErrorCode_t                               ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_GetOptionalConfiguration_t       UserParams;

    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (OptionalConfiguration_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    UserParams.Handle = Handle;
    UserParams.OptionalConfiguration = *OptionalConfiguration_p;


 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_GETOPTIONALCONFIGURATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_GETOPTIONALCONFIGURATION():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *OptionalConfiguration_p = UserParams.OptionalConfiguration;

     }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVTG_GetSlaveModeParams
Returns     : Error code,
*******************************************************************************/
ST_ErrorCode_t  STVTG_GetSlaveModeParams( const STVTG_Handle_t Handle,
                                          STVTG_SlaveModeParams_t * const SlaveModeParams_p)

{
    ST_ErrorCode_t                               ErrorCode = ST_NO_ERROR;
    STVTG_Ioctl_GetSlaveModeParams_t       UserParams;

    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (SlaveModeParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    UserParams.Handle = Handle;
    UserParams.SlaveModeParams = *SlaveModeParams_p;


 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_GETSLAVEMODEPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_GETSLAVEMODEPARAMS():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *SlaveModeParams_p = UserParams.SlaveModeParams;

     }
    return ErrorCode;
}
/*******************************************************************************
Name        : STVTG_CalibrateSyncLevel
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVTG_CalibrateSyncLevel( const STVTG_Handle_t    Handle , STVTG_SyncLevel_t   *SyncLevel_p)
{
    STVTG_Ioctl_CalibrateSyncLevel_t       UserParams;
    ST_ErrorCode_t                         ErrorCode = ST_NO_ERROR;


    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (SyncLevel_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    UserParams.Handle = Handle;
    UserParams.SyncLevel = *SyncLevel_p;

 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_CALIBRATESYNCLEVEL, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_CALIBRATESYNCLEVEL():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}
/*******************************************************************************
Name        : STVTG_CalibrateSyncLevel
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVTG_GetLevelSettings( const STVTG_Handle_t   Handle , STVTG_SyncLevel_t   *SyncLevel_p)
{
    STVTG_Ioctl_GetLevelSettings_t       UserParams;
    ST_ErrorCode_t                       ErrorCode = ST_NO_ERROR;


    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (SyncLevel_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    UserParams.Handle = Handle;
    UserParams.SyncLevel = *SyncLevel_p;

 /* IOCTL the command */
    if (ioctl(fd, STVTG_IOC_GETLEVELSETTINGS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVTG_IOC_GETLEVELSETTINGS():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *SyncLevel_p = UserParams.SyncLevel;
    }
    return ErrorCode;
}
