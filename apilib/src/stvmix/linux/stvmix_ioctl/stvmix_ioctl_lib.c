/*****************************************************************************
 *
 *  Module      : stvmix_ioctl_lib.c
 *  Date        : 30-10-2005
 *  Author      : AYARITAR
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

#include "stos.h"

#include "stvmix.h"

#include "stvmix_ioctl.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/
#define EnterCriticalSection()
#define LeaveCriticalSection()
/*** LOCAL VARIABLES *********************************************************/
static int fd=-1; /* not open */
static int UseCount=0; /* Number of initialised devices */


/*** METHODS ****************************************************************/
ST_ErrorCode_t STVMIX_Init( const ST_DeviceName_t DeviceName, const STVMIX_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (InitParams_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if( !UseCount ){ /* First time? */
        char *devpath;

        devpath = getenv("STVMIX_IOCTL_DEV_PATH");  /* get the path for the device */

        if( devpath ){
            fd = open( devpath, O_RDWR );  /* open it */
            if( fd == -1 ){
                perror(strerror(errno));

                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }

        }
        else{
            fprintf(stderr,"The dev path enviroment variable is not defined: STVMIX_IOCTL_DEV_PATH\n");
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
   if ( ErrorCode == ST_NO_ERROR )
   {
        UseCount++;
   }

    if( ErrorCode == ST_NO_ERROR ){
        STVMIX_Ioctl_Init_t UserParams;
        int NbVout = -1;
        ST_DeviceName_t*            OutputArrayTemp_p;

        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
        UserParams.InitParams = *InitParams_p;
        OutputArrayTemp_p = InitParams_p->OutputArray_p;
        UserParams.NbVoutParams = 0;


    if ( OutputArrayTemp_p != NULL)
    {
        do
        {
            NbVout++ ;
            strcpy( UserParams.VoutName[NbVout],(*(OutputArrayTemp_p)));
            OutputArrayTemp_p++ ;
        }while ( (UserParams.VoutName[NbVout][0] != '\0') && (NbVout < MAX_VOUT) );
        UserParams.NbVoutParams = NbVout;
   }

        /* IOCTL the command */
        if (ioctl(fd, STVMIX_IOC_INIT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            STTBX_Print((" STVMIX_IOCTL_Init():Ioctl error\n"));
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }

    return ErrorCode;
}

ST_ErrorCode_t STVMIX_Open( const ST_DeviceName_t DeviceName, const STVMIX_OpenParams_t * OpenParams_p, STVMIX_Handle_t * const Handle_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVMIX_Ioctl_Open_t UserParams;

    /* Exit now if parameters are bad */
    if (   (OpenParams_p == NULL)                           /* There must be parameters ! */
        || (Handle_p == NULL)                               /* Pointer to handle should be valid ! */
        || (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.OpenParams = *OpenParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVMIX_IOC_OPEN, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Print((" STVMIX_IOCTL_OPEN():Ioctl error\n"));
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

ST_ErrorCode_t STVMIX_Close(const STVMIX_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVMIX_Ioctl_Close_t UserParams;

    if (!UseCount)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVMIX_IOC_CLOSE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Print((" STVMIX_IOC_CLOSE():Ioctl error\n"));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STVMIX_Term(const ST_DeviceName_t DeviceName, const STVMIX_TermParams_t * const TermParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVMIX_Ioctl_Term_t UserParams;

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
    if (ioctl(fd, STVMIX_IOC_TERM, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Print((" STVMIX_IOCTL_Term():Ioctl error\n"));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

   if ( ErrorCode == ST_NO_ERROR )
    {
        if( UseCount ){
            UseCount--;
            if( !UseCount ){ /* Last Termination */
                close(fd);
            }
        }
    }
    return ErrorCode;
}

/*--------------------------------------------------------------------------------
Get revision of VMIX API
--------------------------------------------------------------------------------
*/
ST_Revision_t STVMIX_GetRevision(void)
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return(VMIX_Revision);
} /* End of STVMIX_GetRevision() function */


/*
--------------------------------------------------------------------------------
Get capabilities of VMIX API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVMIX_GetCapability(const ST_DeviceName_t DeviceName, STVMIX_Capability_t * const Capability_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVMIX_Ioctl_GetCapability_t           STVMIX_Ioctl_GetCapability;
    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
              /* Copy parameters into IOCTL structure */
            strcpy( STVMIX_Ioctl_GetCapability.DeviceName, DeviceName);
            STVMIX_Ioctl_GetCapability.Capability = *Capability_p;

            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_GETCAPABILITY, &STVMIX_Ioctl_GetCapability) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_GetCapability():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_GetCapability.ErrorCode;
                *Capability_p = STVMIX_Ioctl_GetCapability.Capability;
                /* No parameter from IOCTL structure */
            }



    }

    return(ErrorCode);

 } /* End of STVMIX_GetCapability() function */



/*******************************************************************************
Name        : STVMIX_Disable
Description : Disable VMIX driver
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVMIX_Disable(STVMIX_Handle_t Handle)
{
    STVMIX_Ioctl_Disable_t    STVMIX_Ioctl_Disable;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_Disable.Handle = Handle;

            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_DISABLE, &STVMIX_Ioctl_Disable) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_Disable():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_Disable.ErrorCode;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STVMIX_Enable
Description : Enable  VMIX driver
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVMIX_Enable(STVMIX_Handle_t Handle)
{
    STVMIX_Ioctl_Enable_t     STVMIX_Ioctl_Enable;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_Enable.Handle = Handle;

            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_ENABLE, &STVMIX_Ioctl_Enable) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_Enable():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_Enable.ErrorCode;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STVMIX_DisconnectLayers
Description : Enable  VMIX driver
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVMIX_DisconnectLayers(STVMIX_Handle_t Handle)
{
    STVMIX_Ioctl_DisconnectLayers_t     STVMIX_Ioctl_DisconnectLayers;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_DisconnectLayers.Handle = Handle;

            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_DISCONNECTLAYERS, &STVMIX_Ioctl_DisconnectLayers) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_DisconnectLayers():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_DisconnectLayers.ErrorCode;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        :   STVMIX_SetBackgroundColor
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetBackgroundColor(
        const STVMIX_Handle_t    Handle,
        const STGXOBJ_ColorRGB_t RGB888,
        const BOOL               Enable )
{
    STVMIX_Ioctl_SetBackgroundColor_t        STVMIX_Ioctl_SetBackgroundColor;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;


    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_SetBackgroundColor.Handle = Handle;
            STVMIX_Ioctl_SetBackgroundColor.RGB888 = RGB888;
            STVMIX_Ioctl_SetBackgroundColor.Enable = Enable;
            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_SETBACKGROUNDCOLOR, &STVMIX_Ioctl_SetBackgroundColor) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_SetBackgroundColor():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_SetBackgroundColor.ErrorCode;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);


}

/*******************************************************************************
Name        :   STVMIX_SetScreenOffset
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetScreenOffset(
        const STVMIX_Handle_t   Handle,
        const S8                Horizontal,
        const S8                Vertical  )
{
    STVMIX_Ioctl_SetScreenOffset_t        STVMIX_Ioctl_SetScreenOffset;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;


    EnterCriticalSection();



    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_SetScreenOffset.Handle = Handle;
            STVMIX_Ioctl_SetScreenOffset.Horizontal = Horizontal;
            STVMIX_Ioctl_SetScreenOffset.Vertical = Vertical;
            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_SETSCREENOFFSET, &STVMIX_Ioctl_SetScreenOffset) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_SetBackgroundColor():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_SetScreenOffset.ErrorCode;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);


}

/*******************************************************************************
Name        :   STVMIX_SetScreenParams
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetScreenParams(
        const STVMIX_Handle_t   Handle,
        const STVMIX_ScreenParams_t* const ScreenParams_p )
{
    STVMIX_Ioctl_SetScreenParams_t        STVMIX_Ioctl_SetScreenParams;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;


    EnterCriticalSection();

    if (ScreenParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }


    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_SetScreenParams.Handle = Handle;
            STVMIX_Ioctl_SetScreenParams.ScreenParams = *ScreenParams_p;
            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_SETSCREENPARAMS, &STVMIX_Ioctl_SetScreenParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_SetScreenParams():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_SetScreenParams.ErrorCode;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);


}

/*******************************************************************************
Name        :   STVMIX_SetTimeBase
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetTimeBase( const STVMIX_Handle_t  Handle,
                      const ST_DeviceName_t  VTGDriver )
{
    STVMIX_Ioctl_SetTimeBase_t        STVMIX_Ioctl_SetTimeBase;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;


    EnterCriticalSection();

    if( VTGDriver == NULL )
        return (ST_ERROR_BAD_PARAMETER);


    if (!UseCount)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_SetTimeBase.Handle = Handle;
             strcpy (STVMIX_Ioctl_SetTimeBase.VTGDriver , VTGDriver);
            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_SETTIMEBASE, &STVMIX_Ioctl_SetTimeBase) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_SetTimeBase():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_SetTimeBase.ErrorCode;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);


}

/*******************************************************************************
Name        :   STVMIX_GetScreenParams
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetScreenParams(
        const STVMIX_Handle_t   Handle,
        STVMIX_ScreenParams_t*  const ScreenParams_p )
{
    STVMIX_Ioctl_GetScreenParams_t            STVMIX_Ioctl_GetScreenParams;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;


    EnterCriticalSection();

    /* Check Params */
    if(ScreenParams_p == NULL)
        return (ST_ERROR_BAD_PARAMETER);


    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_GetScreenParams.Handle = Handle;
            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_GETSCREENPARAMS, &STVMIX_Ioctl_GetScreenParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_GetScreenParams():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_GetScreenParams.ErrorCode;
                *ScreenParams_p = STVMIX_Ioctl_GetScreenParams.ScreenParams;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);


}

/*******************************************************************************
Name        :   STVMIX_GetScreenOffset
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetScreenOffset(
        const STVMIX_Handle_t     Handle,
        S8* const           Horizontal_p,
        S8* const           Vertical_p )
{
    STVMIX_Ioctl_GetScreenOffset_t            STVMIX_Ioctl_GetScreenOffset;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;


    EnterCriticalSection();

    /* Check Params */
    if((Horizontal_p == NULL) || (Vertical_p == NULL))
        return (ST_ERROR_BAD_PARAMETER);


    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_GetScreenOffset.Handle = Handle;
            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_GETSCREENOFFSET, &STVMIX_Ioctl_GetScreenOffset) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_GetScreenOffset():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_GetScreenOffset.ErrorCode;
                *Horizontal_p = STVMIX_Ioctl_GetScreenOffset.Horizontal;
                *Vertical_p = STVMIX_Ioctl_GetScreenOffset.Vertical;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);


}

/*******************************************************************************
Name        :   STVMIX_GetConnectedLayers
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetConnectedLayers(const STVMIX_Handle_t Handle,
                                         const U16 LayerPosition,
                                         STVMIX_LayerDisplayParams_t* const LayerParams_p)
{
    STVMIX_Ioctl_GetConnectedLayers_t            STVMIX_Ioctl_GetConnectedLayers;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;


    EnterCriticalSection();

    /* Check Params */
    if ( LayerParams_p == NULL )
    {
        return ST_ERROR_BAD_PARAMETER ;
    }

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_GetConnectedLayers.Handle = Handle;
            STVMIX_Ioctl_GetConnectedLayers.LayerPosition = LayerPosition;
            STVMIX_Ioctl_GetConnectedLayers.LayerParams = *LayerParams_p;
            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_GETCONNECTEDLAYERS, &STVMIX_Ioctl_GetConnectedLayers) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_GetConnectedLayers():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_GetConnectedLayers.ErrorCode;
                if ( LayerParams_p )
                {
                    *LayerParams_p = STVMIX_Ioctl_GetConnectedLayers.LayerParams;
                }
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);


}

/*******************************************************************************
Name        :   STVMIX_GetBackgroundColor
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetBackgroundColor(
        const STVMIX_Handle_t     Handle,
        STGXOBJ_ColorRGB_t* const RGB888_p,
        BOOL* const               Enable_p)
{
    STVMIX_Ioctl_GetBackgroundColor_t            STVMIX_Ioctl_GetBackgroundColor;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    STGXOBJ_ColorRGB_t   x;
    EnterCriticalSection();

    /* Check Params */
    if ( ( RGB888_p == NULL )|| (Enable_p == NULL ))
    {
        return ST_ERROR_BAD_PARAMETER ;
    }

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE ;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_GetBackgroundColor.Handle = Handle;
            STVMIX_Ioctl_GetBackgroundColor.RGB888 = *RGB888_p;
            STVMIX_Ioctl_GetBackgroundColor.Enable = *Enable_p;
            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_GETBACKGROUNDCOLOR, &STVMIX_Ioctl_GetBackgroundColor) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_GetBackgroundColor():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_GetBackgroundColor.ErrorCode;
                if ( RGB888_p != NULL )
                {
                    *RGB888_p = STVMIX_Ioctl_GetBackgroundColor.RGB888;
                }
                if ( Enable_p != NULL  )
                {
                    *Enable_p = STVMIX_Ioctl_GetBackgroundColor.Enable;
                }

                /* No parameter from IOCTL structure */
            }
    }
    LeaveCriticalSection();


    return(ErrorCode);


}

/*******************************************************************************
Name        :   STVMIX_ConnectLayers
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVMIX_ConnectLayers(const STVMIX_Handle_t                      Handle,
                                    const STVMIX_LayerDisplayParams_t*  const  LayerDisplayParams[],
                                    const U16                                  NbLayerParams )
{
    STVMIX_Ioctl_ConnectLayers_t          STVMIX_Ioctl_ConnectLayers;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;
    S32                                 i ;


    EnterCriticalSection();
    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;

    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_ConnectLayers.Handle = Handle;

            for ( i=0 ; i < NbLayerParams ; i++ )
            {
                strcpy( STVMIX_Ioctl_ConnectLayers.LayerDisplayParams[i].DeviceName, LayerDisplayParams[i]->DeviceName );
                STTBX_Print((" STVMIX_ConnectLayers(): device layer name = %s\n", LayerDisplayParams[i]->DeviceName));
                STVMIX_Ioctl_ConnectLayers.LayerDisplayParams[i].ActiveSignal = LayerDisplayParams[i]->ActiveSignal ;
            }


            STVMIX_Ioctl_ConnectLayers.NbLayerParams = NbLayerParams;

            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_CONNECTLAYERS, &STVMIX_Ioctl_ConnectLayers) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_ConnectLayers():Ioctl error\n"));

            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_ConnectLayers.ErrorCode;
/*                *LayerDisplayParams = STVMIX_Ioctl_ConnectLayers.LayerDisplayParams;*/
                /* No parameter from IOCTL structure */
                STTBX_Print((" STVMIX_ConnectLayers():SUCCESS\n"));
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);


}

/*******************************************************************************
Name        :   STVMIX_GetTimeBase
Parameters  :   Handle
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetTimeBase( const STVMIX_Handle_t  Handle,
                      ST_DeviceName_t* const  VTGDriver_p)
{
    STVMIX_Ioctl_GetTimeBase_t        STVMIX_Ioctl_GetTimeBase;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;


    EnterCriticalSection();

    /* Check Params */
    if ( VTGDriver_p == NULL )
        return (ST_ERROR_BAD_PARAMETER);


    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STVMIX_Ioctl_GetTimeBase.Handle = Handle;
            strcpy( STVMIX_Ioctl_GetTimeBase.VTGDriver,*VTGDriver_p);

            /* IOCTL the command */
            if (ioctl(fd, STVMIX_IOC_GETTIMEBASE, &STVMIX_Ioctl_GetTimeBase) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STVMIX_SetTimeBase():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STVMIX_Ioctl_GetTimeBase.ErrorCode;
                strcpy (*VTGDriver_p,STVMIX_Ioctl_GetTimeBase.VTGDriver);
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);


}



