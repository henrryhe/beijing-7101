/*****************************************************************************
 *
 *  Module      : stlayer_ioctl_lib.c
 *  Date        : 13-11-2005
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
#include <sys/mman.h>


#include "stlayer.h"
#include "stlayer_ioctl.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/
#define LeaveCriticalSection()
#define EnterCriticalSection()
/*** LOCAL VARIABLES *********************************************************/
static int fd=-1; /* not open */
static int UseCount=0; /* Number of initialised devices */

/*** METHODS ****************************************************************/
#define SIZE_PAGE 0x1000

#define MAX_ALLOCATIONS  30

typedef struct
{
    U32                     *UserAddress_p;
    U32                     *KernelAddress_p;
    U32                     AllocWidth;
} UserTOKernelAddress_t ;

static UserTOKernelAddress_t      AddressArray[MAX_ALLOCATIONS];

/*#define LAYER_DEBUG
#define LAYER_DEBUG*/
/* Private Functions -------------------------------------------------------- */
/*
--------------------------------------------------------------------------------
Convert address from virtual user to virtual kernel
--------------------------------------------------------------------------------
*/
U32 *STLAYER_UserToKernel( U32 VirtUserAddress_p )
{
    U32 Index = 0;
#ifdef LAYER_DEBUG
    printf( " STLAYER_UserToKernel():Address to convert : 0x%x\n", VirtUserAddress_p );
#endif
    while (  ( (U32)AddressArray[Index].UserAddress_p != ( U32 ) VirtUserAddress_p )
            && ( Index < MAX_ALLOCATIONS ) )
    {
        Index++;
    }

    if ( Index > MAX_ALLOCATIONS )
    {
        printf( "STLAYER_UserToKernel():Error convert user to kernel\n");
    }
    else
    {
#ifdef LAYER_DEBUG
        printf( "STLAYER_UserToKernel():Address(%d) converted from  user 0x%x to kernel 0x%x \n",
                Index,
                AddressArray[Index].UserAddress_p,
                AddressArray[Index].KernelAddress_p );
#endif
        return AddressArray[Index].KernelAddress_p;
    }
    return 0;
}


/*
--------------------------------------------------------------------------------
Convert address from virtual kernel to virtual user
--------------------------------------------------------------------------------
*/
U32 *STLAYER_KernelToUser( U32 VirtKernelAddress_p )
{
    U32 Index = 0 ;

#ifdef LAYER_DEBUG
    printf( " STLAYER_KernelToUser():Address to convert : 0x%x\n", VirtKernelAddress_p );
#endif
    while (  ( AddressArray[Index].KernelAddress_p != ( U32* ) VirtKernelAddress_p )
            && ( Index < MAX_ALLOCATIONS ) )
    {
        Index++;
    }

    if ( Index > MAX_ALLOCATIONS )
    {
        printf( "STLAYER_KernelToUser():Error convert kernel to user\n");
    }
    else
    {
#ifdef LAYER_DEBUG
        printf( "STLAYER_KernelToUser():Address converted to user 0x%x \n",
                AddressArray[Index].UserAddress_p );
#endif
        return AddressArray[Index].UserAddress_p;
    }
    return 0;
}



ST_ErrorCode_t STLAYER_Init(const ST_DeviceName_t DeviceName, const STLAYER_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if( !UseCount ){ /* First time? */
        char *devpath;

        devpath = getenv("STLAYER_IOCTL_DEV_PATH");  /* get the path for the device */

        if( devpath ){
            fd = open( devpath, O_RDWR );  /* open it */
            if( fd == -1 ){
                perror(strerror(errno));

                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }

        }
        else{
            fprintf(stderr,"The dev path enviroment variable is not defined: STLAYER_IOCTL_DEV_PATH\n");
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    if ( ErrorCode == ST_NO_ERROR )
    {
        UseCount++;
    }

    if( ErrorCode == ST_NO_ERROR ){
        STLAYER_Ioctl_Init_t UserParams;

        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
        UserParams.InitParams = *InitParams_p;

        /* IOCTL the command */
        if (ioctl(fd, STLAYER_IOC_INIT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STLAYER_IOCTL_Init():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }

    return ErrorCode;
}

ST_ErrorCode_t STLAYER_Open(const ST_DeviceName_t DeviceName, const STLAYER_OpenParams_t * const OpenParams_p, STLAYER_Handle_t * Handle_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STLAYER_Ioctl_Open_t UserParams;

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
    if (ioctl(fd, STLAYER_IOC_OPEN, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STLAYER_IOCTL_OPEN():Ioctl error\n");
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

ST_ErrorCode_t STLAYER_Close(STLAYER_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STLAYER_Ioctl_Close_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STLAYER_IOC_CLOSE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STLAYER_IOC_CLOSE():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STLAYER_Term(const ST_DeviceName_t DeviceName, const STLAYER_TermParams_t *const TermParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STLAYER_Ioctl_Term_t UserParams;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (TermParams_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.TermParams = *TermParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STLAYER_IOC_TERM, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STLAYER_IOCTL_Term():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

 if (ErrorCode == ST_NO_ERROR)
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

/*******************************************************************************
Name        : STLAYER_GetCapability
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetCapability(const ST_DeviceName_t DeviceName,
                                     STLAYER_Capability_t * const Capability)

{
    STLAYER_Ioctl_GetCapability_t STLAYER_Ioctl_GetCapability;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (Capability == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

   if (!UseCount)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
              /* Copy parameters into IOCTL structure */
            strcpy(STLAYER_Ioctl_GetCapability.DeviceName, DeviceName);
            STLAYER_Ioctl_GetCapability.Capability = *Capability;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETCAPABILITY, &STLAYER_Ioctl_GetCapability) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                printf(" STLAYER_GetCapability():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_GetCapability.ErrorCode;
                *Capability = STLAYER_Ioctl_GetCapability.Capability;
            }
    }


    return(ErrorCode);


}

/*******************************************************************************
Name        : STLAYER_GetRevision
Returns     : Error code
*******************************************************************************/

ST_Revision_t STLAYER_GetRevision(void)
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return(LAYER_Revision);
} /* End of STLAYER_GetRevision() function */

/*******************************************************************************
Name        : STLAYER_GetInitAllocParams
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetInitAllocParams(STLAYER_Layer_t         LayerType,
                                          U32                     ViewPortsNumber,
                                          STLAYER_AllocParams_t * Params)

{
    STLAYER_Ioctl_GetInitAllocParams_t    STLAYER_Ioctl_GetInitAllocParams;
    ST_ErrorCode_t                      ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetInitAllocParams.LayerType       = LayerType;
            STLAYER_Ioctl_GetInitAllocParams.ViewPortsNumber = ViewPortsNumber;
            STLAYER_Ioctl_GetInitAllocParams.Params          = *Params;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETINITALLOCPARAMS, &STLAYER_Ioctl_GetInitAllocParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                printf(" STLAYER_GetInitAllocParams():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode       = STLAYER_Ioctl_GetInitAllocParams.ErrorCode;
                ViewPortsNumber = STLAYER_Ioctl_GetInitAllocParams.ViewPortsNumber;
                *Params         = STLAYER_Ioctl_GetInitAllocParams.Params;

                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetLayerParams
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetLayerParams(STLAYER_Handle_t  Handle,
                                      STLAYER_LayerParams_t *  LayerParams_p)

{
    STLAYER_Ioctl_GetLayerParams_t    STLAYER_Ioctl_GetLayerParams;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetLayerParams.Handle        = Handle;
            STLAYER_Ioctl_GetLayerParams.LayerParams   = *LayerParams_p;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETLAYERPARAMS, &STLAYER_Ioctl_GetLayerParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                printf(" STLAYER_GetLayerParams():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_GetLayerParams.ErrorCode;
                *LayerParams_p = STLAYER_Ioctl_GetLayerParams.LayerParams;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_SetLayerParams
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetLayerParams(STLAYER_Handle_t  Handle,
                                      STLAYER_LayerParams_t *  LayerParams_p)

{
    STLAYER_Ioctl_SetLayerParams_t    STLAYER_Ioctl_SetLayerParams;
    ST_ErrorCode_t                  ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetLayerParams.Handle      = Handle;
            STLAYER_Ioctl_SetLayerParams.LayerParams = *LayerParams_p;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETLAYERPARAMS, &STLAYER_Ioctl_SetLayerParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                printf(" STLAYER_SetLayerParams():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_SetLayerParams.ErrorCode;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}



/*******************************************************************************
Name        : STLAYER_OpenViewPort
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_OpenViewPort(STLAYER_Handle_t            LayerHandle,
                                    STLAYER_ViewPortParams_t*   Params,
                                    STLAYER_ViewPortHandle_t*   VPHandle)

{
    STLAYER_Ioctl_OpenViewPort_t    STLAYER_Ioctl_OpenViewPort;
    ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;
    U32                           *Data1_p, *Data2_p, *PaletteData_p ;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_OpenViewPort.LayerHandle = LayerHandle;
            STLAYER_Ioctl_OpenViewPort.Params      = *Params;

            if ( STLAYER_Ioctl_OpenViewPort.Params.Source_p )
            {
                    if ( STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p )
                    {
                        if ( Params->Source_p->Data.BitMap_p->Data1_p )
                        {
                            Data1_p = (U32*) Params->Source_p->Data.BitMap_p->Data1_p ;
                            Data2_p = (U32*) Params->Source_p->Data.BitMap_p->Data2_p ;


                            STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p->Data1_p = (void*)(
                                    (U32)STLAYER_UserToKernel( (U32)Params->Source_p->Data.BitMap_p->Data1_p ));
                            if ( Data2_p )
                            {
                                STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p->Data2_p = (void*)
                                    ((U32) Data2_p - (U32) Data1_p + (U32) STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p->Data1_p ) ;

                            }
                            printf( "STLAYER_OpenViewPort():BITMAP Layer:\nkernel Data1 Address: 0x%x\nkernel Data2 Address: 0x%x\n", STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p->Data1_p,
                                        STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p->Data2_p );

                        }
                    }
                    else
                    if ( STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.VideoStream_p )
                    {
                        if ( Params->Source_p->Data.VideoStream_p->BitmapParams.Data1_p )
                        {
                            Data1_p = (U32*) Params->Source_p->Data.VideoStream_p->BitmapParams.Data1_p ;
                            Data2_p = (U32*) Params->Source_p->Data.VideoStream_p->BitmapParams.Data2_p ;

                            STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p->Data1_p = (void*)(
                                    (U32)STLAYER_UserToKernel( (U32) Data1_p ));
                            if ( Data2_p )
                            {
                                STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p->Data2_p = (void*)
                                    ((U32) Data2_p - (U32) Data1_p + (U32) STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p->Data1_p) ;
                            }
                            printf( "STLAYER_OpenViewPort():VIDEO Layer:\nkernel Data1 Address: 0x%x\nkernel Data2 Address: 0x%x\n",  STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p->Data1_p,
                                    STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p->Data2_p );

                        }
                    }

            }


            if ( STLAYER_Ioctl_OpenViewPort.Params.Source_p->Palette_p )
            {
                PaletteData_p = (U32*) Params->Source_p->Palette_p->Data_p ;

                STLAYER_Ioctl_OpenViewPort.Params.Source_p->Palette_p->Data_p = (void*)
                        STLAYER_UserToKernel( (U32)Params->Source_p->Palette_p->Data_p );
            }

            STLAYER_Ioctl_OpenViewPort.VPHandle    = *VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_OPENVIEWPORT, &STLAYER_Ioctl_OpenViewPort) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_OpenViewPort():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_OpenViewPort.ErrorCode;
                *Params   = STLAYER_Ioctl_OpenViewPort.Params;
                *VPHandle = STLAYER_Ioctl_OpenViewPort.VPHandle;
            }


            if ( STLAYER_Ioctl_OpenViewPort.Params.Source_p )
            {
                if ( STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.BitMap_p )
                {
                    Params->Source_p->Data.BitMap_p->Data1_p = (void*) Data1_p ;
                    Params->Source_p->Data.BitMap_p->Data2_p = (void*) Data2_p ;

                }
                else
                if ( STLAYER_Ioctl_OpenViewPort.Params.Source_p->Data.VideoStream_p )
                {
                    Params->Source_p->Data.VideoStream_p->BitmapParams.Data1_p = (void*) Data1_p ;
                    Params->Source_p->Data.VideoStream_p->BitmapParams.Data2_p = (void*) Data2_p ;
                }
            }


            if ( STLAYER_Ioctl_OpenViewPort.Params.Source_p->Palette_p )
            {
                Params->Source_p->Palette_p->Data_p = (void*) PaletteData_p ;
            }

    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_CloseViewPort
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle)

{
    STLAYER_Ioctl_CloseViewPort_t   STLAYER_Ioctl_CloseViewPort;
    ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_CloseViewPort.VPHandle = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_CLOSEVIEWPORT, &STLAYER_Ioctl_CloseViewPort) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_CloseViewPort():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_CloseViewPort.ErrorCode;

                /* No parameter from IOCTL structure */
                printf(" STLAYER_CloseViewPort():Device ViewPort closed\n");
            }

    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_EnableViewPort
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle)

{
    STLAYER_Ioctl_EnableViewPort_t   STLAYER_Ioctl_EnableViewPort;
    ST_ErrorCode_t                 ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_EnableViewPort.VPHandle = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_ENABLEVIEWPORT, &STLAYER_Ioctl_EnableViewPort) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_EnableViewPort():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_EnableViewPort.ErrorCode;

                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_DisableViewPort
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle)

{
    STLAYER_Ioctl_DisableViewPort_t  STLAYER_Ioctl_DisableViewPort;
    ST_ErrorCode_t                 ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_DisableViewPort.VPHandle = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_DISABLEVIEWPORT, &STLAYER_Ioctl_DisableViewPort) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_DisableViewPort():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_DisableViewPort.ErrorCode;

                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_AdjustViewPortParams
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_AdjustViewPortParams( STLAYER_Handle_t LayerHandle,
                                             STLAYER_ViewPortParams_t*   Params_p)

{
    STLAYER_Ioctl_AdjustViewPortParams_t   STLAYER_Ioctl_AdjustViewPortParams;
    ST_ErrorCode_t                       ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_AdjustViewPortParams.LayerHandle = LayerHandle;
            STLAYER_Ioctl_AdjustViewPortParams.Params      = *Params_p;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_ADJUSTVIEWPORTPARAMS, &STLAYER_Ioctl_AdjustViewPortParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_AdjustViewPortParams():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_AdjustViewPortParams.ErrorCode;
                *Params_p   = STLAYER_Ioctl_AdjustViewPortParams.Params;

                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_SetViewPortParams
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortParams( const STLAYER_ViewPortHandle_t  VPHandle,
                                          STLAYER_ViewPortParams_t*       Params_p)

{
    STLAYER_Ioctl_SetViewPortParams_t      STLAYER_Ioctl_SetViewPortParams;
    ST_ErrorCode_t                       ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortParams.VPHandle  = VPHandle;
            STLAYER_Ioctl_SetViewPortParams.Params    = *Params_p;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTPARAMS, &STLAYER_Ioctl_SetViewPortParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_SetViewPortParams():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_SetViewPortParams.ErrorCode;


                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetViewPortParams
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortParams( const STLAYER_ViewPortHandle_t  VPHandle,
                                          STLAYER_ViewPortParams_t*       Params_p)

{
    STLAYER_Ioctl_GetViewPortParams_t      STLAYER_Ioctl_GetViewPortParams;
    ST_ErrorCode_t                       ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortParams.VPHandle = VPHandle;
            STLAYER_Ioctl_GetViewPortParams.Params   = *Params_p;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVIEWPORTPARAMS, &STLAYER_Ioctl_GetViewPortParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_GetViewPortParams():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_GetViewPortParams.ErrorCode;
                *Params_p = STLAYER_Ioctl_GetViewPortParams.Params;

                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_SetViewPortSource
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortSource( STLAYER_ViewPortHandle_t    VPHandle,
                                          STLAYER_ViewPortSource_t*   VPSource)

{
    STLAYER_Ioctl_SetViewPortSource_t      STLAYER_Ioctl_SetViewPortSource;
    ST_ErrorCode_t                       ErrorCode = ST_NO_ERROR;
    U32                                  *BitmapData1_p, *BitmapData2_p, *PaletteData_p ;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortSource.VPHandle = VPHandle;
            STLAYER_Ioctl_SetViewPortSource.VPSource = *VPSource;


            if ( STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p )
            {
                if ( VPSource->Data.BitMap_p->Data1_p )
                {
                    BitmapData1_p = (U32*) VPSource->Data.BitMap_p->Data1_p ;
                    BitmapData2_p = (U32*) VPSource->Data.BitMap_p->Data2_p ;


                STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p->Data1_p = (void*)(
                        (U32)STLAYER_UserToKernel( (U32)VPSource->Data.BitMap_p->Data1_p ));
                if ( BitmapData2_p )
                {
                    STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p->Data2_p = (void*)
                        ((U32) BitmapData2_p - (U32) BitmapData1_p + (U32) STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p->Data1_p ) ;

                }
                printf( "STLAYER_SetViewPortSource():BITMAP Layer:\nkernel Data1 Address: 0x%x\nkernel Data2 Address: 0x%x\n", STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p->Data1_p,
                            STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p->Data2_p );

                }
                else
                if ( STLAYER_Ioctl_SetViewPortSource.VPSource.Data.VideoStream_p )
                {
                    BitmapData1_p = (U32*) VPSource->Data.VideoStream_p->BitmapParams.Data1_p ;
                    BitmapData2_p = (U32*) VPSource->Data.VideoStream_p->BitmapParams.Data2_p ;

                    STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p->Data1_p = (void*)(
                            (U32)STLAYER_UserToKernel( (U32)BitmapData1_p ));
                    if ( BitmapData2_p )
                    {
                        STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p->Data2_p = (void*)
                            ((U32) BitmapData2_p - (U32) BitmapData1_p + (U32) STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p->Data1_p) ;
                    }
                    printf( "STLAYER_SetViewPortSource():VIDEO  Layer:\nkernel Data1 Address: 0x%x\nkernel Data2 Address: 0x%x\n", STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p->Data1_p,
                            STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p->Data2_p );

                }

            }

            if ( STLAYER_Ioctl_SetViewPortSource.VPSource.Palette_p )
            {
                PaletteData_p = (U32*) VPSource->Palette_p->Data_p ;

                STLAYER_Ioctl_SetViewPortSource.VPSource.Palette_p->Data_p = (void*)
                        STLAYER_UserToKernel( (U32) VPSource->Palette_p->Data_p );
            }

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTSOURCE, &STLAYER_Ioctl_SetViewPortSource) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_SetViewPortSource():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_SetViewPortSource.ErrorCode;

                /* No parameter from IOCTL structure */

            }

            if ( STLAYER_Ioctl_SetViewPortSource.VPSource.Data.BitMap_p )
            {
                VPSource->Data.BitMap_p->Data1_p = (void*) BitmapData1_p ;
                VPSource->Data.BitMap_p->Data2_p = (void*) BitmapData2_p ;
            }

            if ( STLAYER_Ioctl_SetViewPortSource.VPSource.Palette_p )
            {
                VPSource->Palette_p->Data_p = (void*) PaletteData_p ;
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetViewPortSource
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortSource( STLAYER_ViewPortHandle_t    VPHandle,
                                          STLAYER_ViewPortSource_t*   VPSource)

{
    STLAYER_Ioctl_GetViewPortSource_t      STLAYER_Ioctl_GetViewPortSource;
    ST_ErrorCode_t                       ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortSource.VPHandle = VPHandle;
            STLAYER_Ioctl_GetViewPortSource.VPSource = *VPSource;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVIEWPORTSOURCE, &STLAYER_Ioctl_GetViewPortSource) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_GetViewPortSource():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_GetViewPortSource.ErrorCode;
                *VPSource = STLAYER_Ioctl_GetViewPortSource.VPSource;

                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_SetViewPortIORectangle
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortIORectangle( STLAYER_ViewPortHandle_t    VPHandle,
                                               STGXOBJ_Rectangle_t*        InputRectangle,
                                               STGXOBJ_Rectangle_t*        OutputRectangle)

{
    STLAYER_Ioctl_SetViewPortIORectangle_t    STLAYER_Ioctl_SetViewPortIORectangle;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortIORectangle.VPHandle        = VPHandle;
            STLAYER_Ioctl_SetViewPortIORectangle.InputRectangle  = *InputRectangle;
            STLAYER_Ioctl_SetViewPortIORectangle.OutputRectangle = *OutputRectangle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTIORECTANGLE, &STLAYER_Ioctl_SetViewPortIORectangle) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_SetViewPortIORectangle():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_SetViewPortIORectangle.ErrorCode;

                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_AdjustIORectangle
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_AdjustIORectangle( STLAYER_Handle_t       Handle,
                                          STGXOBJ_Rectangle_t*   InputRectangle,
                                          STGXOBJ_Rectangle_t*   OutputRectangle)

{
    STLAYER_Ioctl_AdjustIORectangle_t      STLAYER_Ioctl_AdjustIORectangle;
    ST_ErrorCode_t                       ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_AdjustIORectangle.VPHandle        = Handle;
            STLAYER_Ioctl_AdjustIORectangle.InputRectangle  = *InputRectangle;
            STLAYER_Ioctl_AdjustIORectangle.OutputRectangle = *OutputRectangle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_ADJUSTIORECTANGLE, &STLAYER_Ioctl_AdjustIORectangle) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_AdjustIORectangle():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode        = STLAYER_Ioctl_AdjustIORectangle.ErrorCode;
                *InputRectangle  = STLAYER_Ioctl_AdjustIORectangle.InputRectangle;
                *OutputRectangle = STLAYER_Ioctl_AdjustIORectangle.OutputRectangle;
                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetViewPortIORectangle
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortIORectangle( STLAYER_ViewPortHandle_t    VPHandle,
                                               STGXOBJ_Rectangle_t*        InputRectangle,
                                               STGXOBJ_Rectangle_t*        OutputRectangle)

{
    STLAYER_Ioctl_GetViewPortIORectangle_t    STLAYER_Ioctl_GetViewPortIORectangle;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
               /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortIORectangle.VPHandle        = VPHandle;
            STLAYER_Ioctl_GetViewPortIORectangle.InputRectangle  = *InputRectangle;
            STLAYER_Ioctl_GetViewPortIORectangle.OutputRectangle = *OutputRectangle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVIEWPORTIORECTANGLE, &STLAYER_Ioctl_GetViewPortIORectangle) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_GetViewPortIORectangle():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode        = STLAYER_Ioctl_GetViewPortIORectangle.ErrorCode;
                *InputRectangle  = STLAYER_Ioctl_GetViewPortIORectangle.InputRectangle;
                *OutputRectangle = STLAYER_Ioctl_GetViewPortIORectangle.OutputRectangle;
                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_SetViewPortPosition
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortPosition(  STLAYER_ViewPortHandle_t VPHandle,
                                             S32                      XPosition,
                                             S32                      YPosition)

{
    STLAYER_Ioctl_SetViewPortPosition_t       STLAYER_Ioctl_SetViewPortPosition;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortPosition.VPHandle  = VPHandle;
            STLAYER_Ioctl_SetViewPortPosition.XPosition = XPosition;
            STLAYER_Ioctl_SetViewPortPosition.YPosition = YPosition;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTPOSITION, &STLAYER_Ioctl_SetViewPortPosition) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_SetViewPortPosition():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode        = STLAYER_Ioctl_SetViewPortPosition.ErrorCode;
                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetViewPortPosition
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortPosition(  STLAYER_ViewPortHandle_t VPHandle,
                                             S32*                     XPosition,
                                             S32*                     YPosition)
{

    STLAYER_Ioctl_GetViewPortPosition_t       STLAYER_Ioctl_GetViewPortPosition;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortPosition.VPHandle  = VPHandle;
            STLAYER_Ioctl_GetViewPortPosition.XPosition = *XPosition;
            STLAYER_Ioctl_GetViewPortPosition.YPosition = *YPosition;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVIEWPORTPOSITION, &STLAYER_Ioctl_GetViewPortPosition) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_GetViewPortPosition():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_GetViewPortPosition.ErrorCode;
                *XPosition = STLAYER_Ioctl_GetViewPortPosition.XPosition;
                *YPosition = STLAYER_Ioctl_GetViewPortPosition.YPosition;
                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_SetViewPortPSI
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortPSI( STLAYER_ViewPortHandle_t  VPHandle,
                                       STLAYER_PSI_t*            VPPSI_p)

{
    STLAYER_Ioctl_SetViewPortPSI_t       STLAYER_Ioctl_SetViewPortPSI;
    ST_ErrorCode_t                     ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortPSI.VPHandle  = VPHandle;
            STLAYER_Ioctl_SetViewPortPSI.VPPSI     = *VPPSI_p;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTPSI, &STLAYER_Ioctl_SetViewPortPSI) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_SetViewPortPSI():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_SetViewPortPSI.ErrorCode;
                /* No parameter from IOCTL structure */

            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetViewPortPSI
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortPSI( STLAYER_ViewPortHandle_t  VPHandle,
                                       STLAYER_PSI_t*            G_VPPSI_p)

{
    STLAYER_Ioctl_GetViewPortPSI_t       STLAYER_Ioctl_GetViewPortPSI;
    ST_ErrorCode_t                     ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortPSI.VPHandle  = VPHandle;
            STLAYER_Ioctl_GetViewPortPSI.VPPSI     = *G_VPPSI_p;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVIEWPORTPSI, &STLAYER_Ioctl_GetViewPortPSI) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_GetViewPortPSI():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_GetViewPortPSI.ErrorCode;
                *G_VPPSI_p  = STLAYER_Ioctl_GetViewPortPSI.VPPSI;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_SetViewPortSpecialMode
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortSpecialMode ( const STLAYER_ViewPortHandle_t                  VPHandle,
                                                const STLAYER_OutputMode_t                      OuputMode,
                                                const STLAYER_OutputWindowSpecialModeParams_t * const Params_p)

{
    STLAYER_Ioctl_SetViewPortSpecialMode_t    STLAYER_Ioctl_SetViewPortSpecialMode;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortSpecialMode.VPHandle  = VPHandle;
            STLAYER_Ioctl_SetViewPortSpecialMode.OuputMode = OuputMode;
            STLAYER_Ioctl_SetViewPortSpecialMode.Params    = *Params_p;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTSPECIALMODE, &STLAYER_Ioctl_SetViewPortSpecialMode) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_SetViewPortSpecialMode():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_SetViewPortSpecialMode.ErrorCode;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetViewPortSpecialMode
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortSpecialMode ( const STLAYER_ViewPortHandle_t                  VPHandle,
                                                STLAYER_OutputMode_t                    * const OuputMode_p,
                                                STLAYER_OutputWindowSpecialModeParams_t * const Params_p)

{
    STLAYER_Ioctl_GetViewPortSpecialMode_t    STLAYER_Ioctl_GetViewPortSpecialMode;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortSpecialMode.VPHandle  = VPHandle;
            STLAYER_Ioctl_GetViewPortSpecialMode.OuputMode = *OuputMode_p;
            STLAYER_Ioctl_GetViewPortSpecialMode.Params    = *Params_p;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVIEWPORTSPECIALMODE, &STLAYER_Ioctl_GetViewPortSpecialMode) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_GetViewPortSpecialMode():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode    = STLAYER_Ioctl_GetViewPortSpecialMode.ErrorCode;
                *OuputMode_p = STLAYER_Ioctl_GetViewPortSpecialMode.OuputMode;
                *Params_p    = STLAYER_Ioctl_GetViewPortSpecialMode.Params;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_DisableColorKey
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_DisableColorKey(STLAYER_ViewPortHandle_t VPHandle)
{
    STLAYER_Ioctl_DisableColorKey_t           STLAYER_Ioctl_DisableColorKey;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_DisableColorKey.VPHandle  = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_DISABLECOLORKEY, &STLAYER_Ioctl_DisableColorKey)!= 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_DisableColorKey():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode    = STLAYER_Ioctl_DisableColorKey.ErrorCode;
                /* No parameter from IOCTL structure */
            }

    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_EnableColorKey
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_EnableColorKey(STLAYER_ViewPortHandle_t VPHandle)
{
    STLAYER_Ioctl_EnableColorKey_t            STLAYER_Ioctl_EnableColorKey;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_EnableColorKey.VPHandle  = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_ENABLECOLORKEY, &STLAYER_Ioctl_EnableColorKey)!= 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_EnableColorKey():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_EnableColorKey.ErrorCode;
                /* No parameter from IOCTL structure */
            }

    }
    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_SetViewPortColorKey
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortColorKey(STLAYER_ViewPortHandle_t    VPHandle,
                                           STGXOBJ_ColorKey_t*         ColorKey)

{
    STLAYER_Ioctl_SetViewPortColorKey_t       STLAYER_Ioctl_SetViewPortColorKey;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortColorKey.VPHandle = VPHandle;
            STLAYER_Ioctl_SetViewPortColorKey.ColorKey = *ColorKey;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTCOLORKEY, &STLAYER_Ioctl_SetViewPortColorKey) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_SetViewPortColorKey():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_SetViewPortColorKey.ErrorCode;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetViewPortColorKey
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortColorKey(STLAYER_ViewPortHandle_t    VPHandle,
                                           STGXOBJ_ColorKey_t*         ColorKey)

{
    STLAYER_Ioctl_GetViewPortColorKey_t       STLAYER_Ioctl_GetViewPortColorKey;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortColorKey.VPHandle  = VPHandle;
            STLAYER_Ioctl_GetViewPortColorKey.ColorKey =  *ColorKey;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVIEWPORTCOLORKEY, &STLAYER_Ioctl_GetViewPortColorKey) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_GetViewPortColorKey():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_GetViewPortColorKey.ErrorCode;
                *ColorKey   = STLAYER_Ioctl_GetViewPortColorKey.ColorKey;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}



/*******************************************************************************
Name        : STLAYER_DisableViewportColorFill
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_DisableViewportColorFill(STLAYER_ViewPortHandle_t VPHandle)
{
    STLAYER_Ioctl_DisableViewportColorFill_t           STLAYER_Ioctl_DisableViewportColorFill;
    ST_ErrorCode_t                                     ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_DisableViewportColorFill.VPHandle  = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_DISABLEVIEWPORTCOLORFILL, &STLAYER_Ioctl_DisableViewportColorFill)!= 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_DisableViewportColorFill():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode    = STLAYER_Ioctl_DisableViewportColorFill.ErrorCode;
                /* No parameter from IOCTL structure */
            }

    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_EnableViewportColorFill
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_EnableViewportColorFill(STLAYER_ViewPortHandle_t VPHandle)
{
    STLAYER_Ioctl_EnableViewportColorFill_t            STLAYER_Ioctl_ViewportColorFill;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_ViewportColorFill.VPHandle  = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_ENABLEVIEWPORTCOLORFILL, &STLAYER_Ioctl_ViewportColorFill)!= 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_EnableViewportColorFill():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_ViewportColorFill.ErrorCode;
                /* No parameter from IOCTL structure */
            }

    }
    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_SetViewportColorFill
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewportColorFill(STLAYER_ViewPortHandle_t    VPHandle,
                                           STGXOBJ_ColorARGB_t*         ColorFill)

{
    STLAYER_Ioctl_SetViewportColorFill_t       STLAYER_Ioctl_SetViewPortColorFill;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortColorFill.VPHandle = VPHandle;
            STLAYER_Ioctl_SetViewPortColorFill.ColorFill = *ColorFill;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTCOLORFILL, &STLAYER_Ioctl_SetViewPortColorFill) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_SetViewportColorFill():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_SetViewPortColorFill.ErrorCode;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetViewPortColorFill
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewportColorFill(STLAYER_ViewPortHandle_t    VPHandle,
                                           STGXOBJ_ColorARGB_t*         ColorFill)

{
    STLAYER_Ioctl_GetViewportColorFill_t       STLAYER_Ioctl_GetViewPortColorFill;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortColorFill.VPHandle  = VPHandle;
            STLAYER_Ioctl_GetViewPortColorFill.ColorFill =  *ColorFill;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVIEWPORTCOLORFILL, &STLAYER_Ioctl_GetViewPortColorFill) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_GetViewportColorFill():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_GetViewPortColorFill.ErrorCode;
                *ColorFill   = STLAYER_Ioctl_GetViewPortColorFill.ColorFill;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_DisableBorderAlpha
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_DisableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle)
{
    STLAYER_Ioctl_DisableBorderAlpha_t        STLAYER_Ioctl_DisableBorderAlpha;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_DisableBorderAlpha.VPHandle  = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_DISABLEBORDERALPHA, &STLAYER_Ioctl_DisableBorderAlpha)!= 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_DisableBorderAlpha():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_DisableBorderAlpha.ErrorCode;
                /* No parameter from IOCTL structure */
            }
        }
    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_EnableBorderAlpha
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_EnableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle)
{
    STLAYER_Ioctl_EnableBorderAlpha_t         STLAYER_Ioctl_EnableBorderAlpha;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_EnableBorderAlpha.VPHandle  = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_ENABLEBORDERALPHA, &STLAYER_Ioctl_EnableBorderAlpha)!= 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_EnableBorderAlpha():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_EnableBorderAlpha.ErrorCode;
                /* No parameter from IOCTL structure */
            }
    }
    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_GetViewPortAlpha
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortAlpha(STLAYER_ViewPortHandle_t  VPHandle,
                                        STLAYER_GlobalAlpha_t*    Alpha)

{
    STLAYER_Ioctl_GetViewPortAlpha_t          STLAYER_Ioctl_GetViewPortAlpha;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortAlpha.VPHandle  = VPHandle;
            STLAYER_Ioctl_GetViewPortAlpha.Alpha     = *Alpha;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVIEWPORTALPHA, &STLAYER_Ioctl_GetViewPortAlpha) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_GetViewPortAlpha():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_GetViewPortAlpha.ErrorCode;
                *Alpha      = STLAYER_Ioctl_GetViewPortAlpha.Alpha;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_SetViewPortAlpha
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortAlpha(STLAYER_ViewPortHandle_t  VPHandle,
                                        STLAYER_GlobalAlpha_t*    Alpha)

{
    STLAYER_Ioctl_SetViewPortAlpha_t          STLAYER_Ioctl_SetViewPortAlpha;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortAlpha.VPHandle  = VPHandle;
            STLAYER_Ioctl_SetViewPortAlpha.Alpha     = *Alpha;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTALPHA, &STLAYER_Ioctl_SetViewPortAlpha) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_SetViewPortAlpha():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_SetViewPortAlpha.ErrorCode;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_SetViewPortGain
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortGain(STLAYER_ViewPortHandle_t  VPHandle,
                                       STLAYER_GainParams_t*     Params)

{
    STLAYER_Ioctl_SetViewPortGain_t           STLAYER_Ioctl_SetViewPortGain;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortGain.VPHandle  = VPHandle;
            STLAYER_Ioctl_SetViewPortGain.Params    = *Params;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTGAIN, &STLAYER_Ioctl_SetViewPortGain) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_SetViewPortGain():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_SetViewPortGain.ErrorCode;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetViewPortGain
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortGain(STLAYER_ViewPortHandle_t  VPHandle,
                                       STLAYER_GainParams_t*     Params)

{
    STLAYER_Ioctl_GetViewPortGain_t           STLAYER_Ioctl_GetViewPortGain;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortGain.VPHandle  = VPHandle;
            STLAYER_Ioctl_GetViewPortGain.Params    = *Params;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTGAIN, &STLAYER_Ioctl_GetViewPortGain) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_GetViewPortGain():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_GetViewPortGain.ErrorCode;
                *Params    = STLAYER_Ioctl_GetViewPortGain.Params;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_SetViewPortRecordable
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetViewPortRecordable(STLAYER_ViewPortHandle_t  VPHandle,
                                             BOOL       Recordable)

{
    STLAYER_Ioctl_SetViewPortRecordable_t     STLAYER_Ioctl_SetViewPortRecordable;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetViewPortRecordable.VPHandle   = VPHandle;
            STLAYER_Ioctl_SetViewPortRecordable.Recordable = Recordable;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETVIEWPORTRECORDABLE, &STLAYER_Ioctl_SetViewPortRecordable) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_SetViewPortRecordable():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_SetViewPortRecordable.ErrorCode;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}



/*******************************************************************************
Name        : STLAYER_GetViewPortRecordable
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortRecordable(STLAYER_ViewPortHandle_t  VPHandle,
                                             BOOL *  Recordable)

{
    STLAYER_Ioctl_GetViewPortRecordable_t     STLAYER_Ioctl_GetViewPortRecordable;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetViewPortRecordable.VPHandle   = VPHandle;
            STLAYER_Ioctl_GetViewPortRecordable.Recordable = *Recordable;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVIEWPORTRECORDABLE, &STLAYER_Ioctl_GetViewPortRecordable) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_GetViewPortRecordable():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_GetViewPortRecordable.ErrorCode;
                *Recordable = STLAYER_Ioctl_GetViewPortRecordable.Recordable;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_GetBitmapAllocParams
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetBitmapAllocParams(STLAYER_Handle_t             LayerHandle,
                                            STGXOBJ_Bitmap_t*            Bitmap_p,
                                            STGXOBJ_BitmapAllocParams_t* Params1_p,
                                            STGXOBJ_BitmapAllocParams_t* Params2_p)

{
    STLAYER_Ioctl_GetBitmapAllocParams_t      STLAYER_Ioctl_GetBitmapAllocParams;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetBitmapAllocParams.LayerHandle = LayerHandle;
            STLAYER_Ioctl_GetBitmapAllocParams.Bitmap      = *Bitmap_p;
            STLAYER_Ioctl_GetBitmapAllocParams.Params1     = *Params1_p;
            STLAYER_Ioctl_GetBitmapAllocParams.Params2     = *Params2_p;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETBITMAPALLOCPARAMS, &STLAYER_Ioctl_GetBitmapAllocParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_GetBitmapAllocParams():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_GetBitmapAllocParams.ErrorCode;
                *Bitmap_p   = STLAYER_Ioctl_GetBitmapAllocParams.Bitmap;
                *Params1_p  = STLAYER_Ioctl_GetBitmapAllocParams.Params1;
                *Params2_p  = STLAYER_Ioctl_GetBitmapAllocParams.Params2;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_GetBitmapHeaderSize
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetBitmapHeaderSize(STLAYER_Handle_t             LayerHandle,
                                           STGXOBJ_Bitmap_t*            Bitmap_p,
                                           U32 *                        HeaderSize_p)

{
    STLAYER_Ioctl_GetBitmapHeaderSize_t       STLAYER_Ioctl_GetBitmapHeaderSize;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetBitmapHeaderSize.LayerHandle = LayerHandle;
            STLAYER_Ioctl_GetBitmapHeaderSize.Bitmap      = *Bitmap_p;
            STLAYER_Ioctl_GetBitmapHeaderSize.HeaderSize  = *HeaderSize_p;


            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETBITMAPHEADERSIZE, &STLAYER_Ioctl_GetBitmapHeaderSize) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_GetBitmapHeaderSize():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode     = STLAYER_Ioctl_GetBitmapHeaderSize.ErrorCode;
                *Bitmap_p     = STLAYER_Ioctl_GetBitmapHeaderSize.Bitmap;
                *HeaderSize_p = STLAYER_Ioctl_GetBitmapHeaderSize.HeaderSize;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_GetPaletteAllocParams
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetPaletteAllocParams(STLAYER_Handle_t              LayerHandle,
                                             STGXOBJ_Palette_t*            Palette_p,
                                             STGXOBJ_PaletteAllocParams_t* Params_p)

{
    STLAYER_Ioctl_GetPaletteAllocParams_t     STLAYER_Ioctl_GetPaletteAllocParams;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetPaletteAllocParams.LayerHandle = LayerHandle;
            STLAYER_Ioctl_GetPaletteAllocParams.Palette     = *Palette_p;
            STLAYER_Ioctl_GetPaletteAllocParams.Params      = *Params_p;


            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETPALETTEALLOCPARAMS, &STLAYER_Ioctl_GetPaletteAllocParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_GetPaletteAllocParams():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_GetPaletteAllocParams.ErrorCode;
                *Palette_p  = STLAYER_Ioctl_GetPaletteAllocParams.Palette;
                *Params_p   = STLAYER_Ioctl_GetPaletteAllocParams.Params;

                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_GetVTGName
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetVTGName( STLAYER_Handle_t  LayerHandle,
                                   ST_DeviceName_t * const VTGName_p)

{
    STLAYER_Ioctl_GetVTGName_t             STLAYER_Ioctl_GetVTGName;
    ST_ErrorCode_t                       ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetVTGName.LayerHandle = LayerHandle;
            /*STLAYER_Ioctl_GetVTGName.VTGName     = *VTGName_p;*/
            strcpy(STLAYER_Ioctl_GetVTGName.VTGName, *VTGName_p);



            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVTGNAME, &STLAYER_Ioctl_GetVTGName) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_GetVTGName():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_GetVTGName.ErrorCode;
               /* *VTGName_p  = STLAYER_Ioctl_GetVTGName.VTGName;*/
                strcpy(*VTGName_p, STLAYER_Ioctl_GetVTGName.VTGName);


                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_GetVTGParams
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetVTGParams( STLAYER_Handle_t               LayerHandle,
                                     STLAYER_VTGParams_t * const    VTGParams_p)

{
    STLAYER_Ioctl_GetVTGParams_t           STLAYER_Ioctl_GetVTGParams;
    ST_ErrorCode_t                       ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetVTGParams.LayerHandle = LayerHandle;
            STLAYER_Ioctl_GetVTGParams.VTGParams   = *VTGParams_p;



            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETVTGPARAMS, &STLAYER_Ioctl_GetVTGParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_GetVTGParams():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode    = STLAYER_Ioctl_GetVTGParams.ErrorCode;
                *VTGParams_p = STLAYER_Ioctl_GetVTGParams.VTGParams;


                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_UpdateFromMixer
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_UpdateFromMixer( STLAYER_Handle_t              LayerHandle,
                                        STLAYER_OutputParams_t *      OutputParams_p)

{
    STLAYER_Ioctl_UpdateFromMixer_t        STLAYER_Ioctl_UpdateFromMixer;
    ST_ErrorCode_t                       ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_UpdateFromMixer.LayerHandle  = LayerHandle;
            STLAYER_Ioctl_UpdateFromMixer.OutputParams = *OutputParams_p;



            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_UPDATEFROMMIXER, &STLAYER_Ioctl_UpdateFromMixer) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_UpdateFromMixer():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode       = STLAYER_Ioctl_UpdateFromMixer.ErrorCode;
                *OutputParams_p = STLAYER_Ioctl_UpdateFromMixer.OutputParams;


                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_DisableViewPortFilter
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_DisableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle)
{
    STLAYER_Ioctl_DisableViewPortFilter_t     STLAYER_Ioctl_DisableViewPortFilter;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_DisableViewPortFilter.VPHandle  = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_DISABLEVIEWPORTFILTER, &STLAYER_Ioctl_DisableViewPortFilter)!= 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_DisableViewPortFilter():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_DisableViewPortFilter.ErrorCode;
                /* No parameter from IOCTL structure */
            }
    }
    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_EnableViewPortFilter
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_EnableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle,
                                            STLAYER_Handle_t FilterHandle)
{
    STLAYER_Ioctl_EnableViewPortFilter_t      STLAYER_Ioctl_EnableViewPortFilter;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_EnableViewPortFilter.VPHandle     = VPHandle;
            STLAYER_Ioctl_EnableViewPortFilter.FilterHandle = FilterHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_ENABLEVIEWPORTFILTER, &STLAYER_Ioctl_EnableViewPortFilter)!= 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_EnableViewPortFilter():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_EnableViewPortFilter.ErrorCode;
                /* No parameter from IOCTL structure */
            }
    }
    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_AttachAlphaViewPort
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_AttachAlphaViewPort(STLAYER_ViewPortHandle_t VPHandle,
                                           STLAYER_Handle_t MaskedLayer)
{
    STLAYER_Ioctl_AttachAlphaViewPort_t       STLAYER_Ioctl_AttachAlphaViewPort;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_AttachAlphaViewPort.VPHandle     = VPHandle;
            STLAYER_Ioctl_AttachAlphaViewPort.MaskedLayer = MaskedLayer;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_ATTACHALPHAVIEWPORT, &STLAYER_Ioctl_AttachAlphaViewPort)!= 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_AttachAlphaViewPort():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_AttachAlphaViewPort.ErrorCode;
                /* No parameter from IOCTL structure */
            }
    }
    LeaveCriticalSection();

    return(ErrorCode);

}

/*******************************************************************************
Name        : STLAYER_MapAddress
Function    : return a mapped address from a kernel address
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_MapAddress( void *KernelAddress_p, void **UserAddress_p )
{
    ST_ErrorCode_t                           ErrorCode = ST_NO_ERROR;
    U32                                      Index ;

    EnterCriticalSection();

    if( !UseCount ){ /* First time? */
        char *devpath;

        devpath = getenv("STLAYER_IOCTL_DEV_PATH");  /* get the path for the device */

        if( devpath ){
            fd = open( devpath, O_RDWR );  /* open it */
            if( fd == -1 ){
                perror(strerror(errno));

                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }

        }
        else{
            fprintf(stderr,"The dev path enviroment variable is not defined: STLAYER_IOCTL_DEV_PATH\n");
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

 if ( ErrorCode == ST_NO_ERROR )
  {
    /* offset in mmap must be address divisible by 0x1000 */
    *UserAddress_p = (void*) mmap(0,
                        1,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED,
                        fd,
                        (U32) (( (U32) KernelAddress_p)/SIZE_PAGE) * SIZE_PAGE );
    *UserAddress_p = (void*)((U32)*UserAddress_p + ((U32)KernelAddress_p % SIZE_PAGE))  ;
    Index = 0 ;
    while (    (AddressArray[Index].UserAddress_p != NULL )
        && ( Index < MAX_ALLOCATIONS ) )
    {
        Index++;
    }

    if ( Index < MAX_ALLOCATIONS )
    {
        AddressArray[Index].UserAddress_p = (U32*) *UserAddress_p ;
        AddressArray[Index].KernelAddress_p = (U32*) KernelAddress_p ;
        AddressArray[Index].AllocWidth = (U32) 1;
#ifdef LAYER_DEBUG
        printf("STLAYER_MapAddress(): Address inserted : 0x%x \n", (U32) AddressArray[Index].KernelAddress_p);
#endif
    }
    else
    {
        printf("STLAYER_MapAddress(): Error\n" );
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

 }

 if (ErrorCode == ST_NO_ERROR)
  {
    if( UseCount ){
        UseCount--;
        if( !UseCount ){ /* Last Termination */
            close(fd);
        }
    }
  }


    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STLAYER_MapAddress() */


ST_ErrorCode_t STLAYER_AllocData( STLAYER_Handle_t  LayerHandle, STLAYER_AllocDataParams_t *Params_p, void **Address_p )
{
    STLAYER_Ioctl_AllocData_t                  STLAYER_Ioctl_AllocData;
    ST_ErrorCode_t                           ErrorCode = ST_NO_ERROR;
    U32                                      Index ;

    EnterCriticalSection();

    if ( Address_p == NULL || Params_p == NULL ) {
        return ST_ERROR_BAD_PARAMETER;
    }

    if ( Params_p->Size == 0 )
    {
        printf("STLAYER_AllocData():Size error\n");
        return ST_ERROR_BAD_PARAMETER;
    }

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
            STLAYER_Ioctl_AllocData.LayerHandle = LayerHandle ;
            STLAYER_Ioctl_AllocData.AllocDataParams.Size = Params_p->Size ;
            STLAYER_Ioctl_AllocData.AllocDataParams.Alignment = Params_p->Alignment ;
#ifdef LAYER_DEBUG
            printf("STLAYER_AllocData(): Alignment: 0x%x , Size: 0x%x, Allocated size: 0x%x \n", Params_p->Alignment, Params_p->Size,
                      Params_p->Size + SIZE_PAGE - 1 );
#endif
            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_ALLOCDATA, &STLAYER_Ioctl_AllocData) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_AllocData():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_AllocData.ErrorCode;


                /* offset in mmap must be address divisible by 0x1000 */
                *Address_p = (void*) mmap(0,
                                    (STLAYER_Ioctl_AllocData.AllocDataParams.Size + SIZE_PAGE - 1),
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    fd,
                                    (U32) (( (U32) STLAYER_Ioctl_AllocData.Address_p)/SIZE_PAGE) * SIZE_PAGE );

                *Address_p = (void*)((U32)*Address_p + ((U32)STLAYER_Ioctl_AllocData.Address_p % SIZE_PAGE))  ;

                Index = 0 ;
                while (    (AddressArray[Index].UserAddress_p != NULL )
                    && ( Index < MAX_ALLOCATIONS ) )
                {
                    Index++;
                }

                if ( Index < MAX_ALLOCATIONS )
                {
                    AddressArray[Index].UserAddress_p = (U32*) *Address_p ;
                    AddressArray[Index].KernelAddress_p = (U32*) STLAYER_Ioctl_AllocData.Address_p ;
                    AddressArray[Index].AllocWidth = (U32) (STLAYER_Ioctl_AllocData.AllocDataParams.Size + SIZE_PAGE - 1);
#ifdef LAYER_DEBUG
                    printf("STLAYER_AllocData(): Address inserted : 0x%x \n", (U32) AddressArray[Index].KernelAddress_p);
#endif
                }
                else
                {
                    printf("STLAYER_AllocData(): No memory\n" );
                }



                printf("STLAYER_AllocData(): Address Kernel :0x%x Address User : 0x%x\n", STLAYER_Ioctl_AllocData.Address_p,
                            *Address_p );
            }


    }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STLAYER_AllocData() */



/*******************************************************************************
Name        : STLAYER_AllocDataSecure
Returns     : Error code
*******************************************************************************/

ST_ErrorCode_t STLAYER_AllocDataSecure( STLAYER_Handle_t  LayerHandle, STLAYER_AllocDataParams_t *Params_p, void **Address_p )
{
    STLAYER_Ioctl_AllocData_t                  STLAYER_Ioctl_AllocData;
    ST_ErrorCode_t                           ErrorCode = ST_NO_ERROR;
    U32                                      Index ;

    EnterCriticalSection();

    if ( Address_p == NULL || Params_p == NULL ) {
        return ST_ERROR_BAD_PARAMETER;
    }

    if ( Params_p->Size == 0 )
    {
        printf("STLAYER_AllocDataSecure():Size error\n");
        return ST_ERROR_BAD_PARAMETER;
    }

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
            STLAYER_Ioctl_AllocData.LayerHandle = LayerHandle ;
            STLAYER_Ioctl_AllocData.AllocDataParams.Size = Params_p->Size ;
            STLAYER_Ioctl_AllocData.AllocDataParams.Alignment = Params_p->Alignment ;
#ifdef LAYER_DEBUG
            printf("STLAYER_AllocDataSecure(): Alignment: 0x%x , Size: 0x%x, Allocated size: 0x%x \n", Params_p->Alignment, Params_p->Size,
                      Params_p->Size + SIZE_PAGE - 1 );
#endif
            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_ALLOCDATASECURE, &STLAYER_Ioctl_AllocData) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_AllocDataSecure():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_AllocData.ErrorCode;


                /* offset in mmap must be address divisible by 0x1000 */
                *Address_p = (void*) mmap(0,
                                    (STLAYER_Ioctl_AllocData.AllocDataParams.Size + SIZE_PAGE - 1),
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    fd,
                                    (U32) (( (U32) STLAYER_Ioctl_AllocData.Address_p)/SIZE_PAGE) * SIZE_PAGE );

                *Address_p = (void*)((U32)*Address_p + ((U32)STLAYER_Ioctl_AllocData.Address_p % SIZE_PAGE))  ;

                Index = 0 ;
                while (    (AddressArray[Index].UserAddress_p != NULL )
                    && ( Index < MAX_ALLOCATIONS ) )
                {
                    Index++;
                }

                if ( Index < MAX_ALLOCATIONS )
                {
                    AddressArray[Index].UserAddress_p = (U32*) *Address_p ;
                    AddressArray[Index].KernelAddress_p = (U32*) STLAYER_Ioctl_AllocData.Address_p ;
                    AddressArray[Index].AllocWidth = (U32) (STLAYER_Ioctl_AllocData.AllocDataParams.Size + SIZE_PAGE - 1);
#ifdef LAYER_DEBUG
                    printf("STLAYER_AllocDataSecure(): Address inserted : 0x%x \n", (U32) AddressArray[Index].KernelAddress_p);
#endif
                }
                else
                {
                    printf("STLAYER_AllocDataSecure(): No memory\n" );
                }



                printf("STLAYER_AllocDataSecure(): Address Kernel :0x%x Address User : 0x%x\n", STLAYER_Ioctl_AllocData.Address_p,
                            *Address_p );
            }


    }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STLAYER_AllocSecuredData() */



ST_ErrorCode_t STLAYER_FreeData( STLAYER_Handle_t  LayerHandle, void *Address_p )
{
    STLAYER_Ioctl_FreeData_t                      STLAYER_Ioctl_FreeData;
    ST_ErrorCode_t                              ErrorCode = ST_NO_ERROR;
    U32                                         Index ;

    EnterCriticalSection();
    if ( Address_p == NULL )
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
            Index = 0 ;
            /* Get Kernel address and Mapped size */
            while (   (((U32)Address_p < (U32)AddressArray[Index].UserAddress_p)
                    || ((U32)Address_p > (U32)AddressArray[Index].UserAddress_p + AddressArray[Index].AllocWidth))
                && ( Index < MAX_ALLOCATIONS ) )
            {
                Index++;
            }

            /* All debug messages */
            if (Index >= MAX_ALLOCATIONS)
            {
                printf( "%s(): Error to convert user address (0x.8x) to kernel\n", __FUNCTION__, (U32)Address_p);
                return ST_ERROR_BAD_PARAMETER;
            }


            STLAYER_Ioctl_FreeData.Address_p = (void*) STLAYER_UserToKernel( (U32) Address_p );

#ifdef LAYER_DEBUG
            printf("%s(): Address to free 0x%x\n", __FUNCTION__, (U32)STLAYER_Ioctl_FreeData.Address_p );
#endif

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_FREEDATA, &STLAYER_Ioctl_FreeData) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf("STLAYER_FreeData():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_FreeData.ErrorCode;

                if ( Index < MAX_ALLOCATIONS )
                {
                    if (munmap( (void*)((U32)AddressArray[Index].UserAddress_p - ((U32)AddressArray[Index].KernelAddress_p % SIZE_PAGE)),
                                AddressArray[Index].AllocWidth ) < 0)
                    {
                        printf ( "STLAYER_FreeData():error in unmapping address: 0x%x\n", (U32)AddressArray[Index].UserAddress_p );
                    }
                    else
                    {
#ifdef LAYER_DEBUG
                        printf("STLAYER_FreeData(): Unmapping : Address 0x%x  Size : 0x%x \n", (U32)AddressArray[Index].UserAddress_p - ((U32)AddressArray[Index].KernelAddress_p % SIZE_PAGE),
                                    AddressArray[Index].AllocWidth );
#endif
                        /* Remove mapping informations from array */
                        for ( ; Index<MAX_ALLOCATIONS-1 ; Index++)
                        {
                            AddressArray[Index] = AddressArray[Index+1];
                        }
                        /* resets last element */
                        AddressArray[MAX_ALLOCATIONS-1].UserAddress_p = NULL ;
                        AddressArray[MAX_ALLOCATIONS-1].KernelAddress_p = NULL ;
                        AddressArray[MAX_ALLOCATIONS-1].AllocWidth = 0 ;

                    }
                }
                else
                {
                    printf("STLAYER_FreeData(): No memory\n" );
                }
            }


    }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STLAYER_FreeData() */

/*******************************************************************************
Name        : STLAYER_SetViewPortFlickerFilterMode
Returns     : Error code
*******************************************************************************/

ST_ErrorCode_t STLAYER_SetViewPortFlickerFilterMode( STLAYER_ViewPortHandle_t    VPHandle,
                                                     STLAYER_FlickerFilterMode_t   FlickerFilterMode)

{
    STLAYER_Ioctl_SetFlickerFilterMode_t      STLAYER_Ioctl_SetFlickerFilterMode;
    ST_ErrorCode_t                       ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_SetFlickerFilterMode.VPHandle = VPHandle;
            STLAYER_Ioctl_SetFlickerFilterMode.FFMode = FlickerFilterMode;


            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_SETFLICKERFILTERMODE, &STLAYER_Ioctl_SetFlickerFilterMode) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_SetViewPortFlickerFilterMode():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_SetFlickerFilterMode.ErrorCode;
                /* No parameter from IOCTL structure */

            }

    }

    LeaveCriticalSection();

    return(ErrorCode);

}    /* STLAYER_SetViewPortFlickerFilterMode  */


/*******************************************************************************
Name        : STLAYER_GetViewPortFlickerFilterMode
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetViewPortFlickerFilterMode( STLAYER_ViewPortHandle_t    VPHandle,
                                                     STLAYER_FlickerFilterMode_t*      FlickerFilterMode_p)


{
    STLAYER_Ioctl_GetFlickerFilterMode_t      STLAYER_Ioctl_GetFlickerFilterMode;
    ST_ErrorCode_t                       ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_GetFlickerFilterMode.VPHandle = VPHandle;
            STLAYER_Ioctl_GetFlickerFilterMode.FFMode = *FlickerFilterMode_p;


            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_GETFLICKERFILTERMODE, &STLAYER_Ioctl_GetFlickerFilterMode) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_GetViewPortFlickerFilterMode():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STLAYER_Ioctl_GetFlickerFilterMode.ErrorCode;

                *FlickerFilterMode_p = STLAYER_Ioctl_GetFlickerFilterMode.FFMode;

            }

    }

    LeaveCriticalSection();

    return(ErrorCode);

}    /* STLAYER_GetViewPortFlickerFilterMode  */


/*******************************************************************************
Name        : STLAYER_InformPictureToBeDecoded
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_InformPictureToBeDecoded(const STLAYER_ViewPortHandle_t    VPHandle,
                                                STGXOBJ_PictureInfos_t*           PictureInfos)

{
    STLAYER_Ioctl_InformPictureToBeDecoded_t    STLAYER_Ioctl_InformPictureToBeDecoded;
    ST_ErrorCode_t                              ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_InformPictureToBeDecoded.VPHandle  = VPHandle;
            STLAYER_Ioctl_InformPictureToBeDecoded.PictureInfos =  *PictureInfos;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_INFORMPICTURETOBEDECODED, &STLAYER_Ioctl_InformPictureToBeDecoded) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_InformPictureToBeDecoded():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_InformPictureToBeDecoded.ErrorCode;
                *PictureInfos   = STLAYER_Ioctl_InformPictureToBeDecoded.PictureInfos;
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}


/*******************************************************************************
Name        : STLAYER_CommitViewPortParams
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_CommitViewPortParams( STLAYER_ViewPortHandle_t    VPHandle )
{
    STLAYER_Ioctl_CommitViewPortParams_t    STLAYER_Ioctl_CommitViewPortParams;
    ST_ErrorCode_t                              ErrorCode   = ST_NO_ERROR;

    EnterCriticalSection();

    if (!UseCount)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
            /* Copy parameters into IOCTL structure */
            STLAYER_Ioctl_CommitViewPortParams.VPHandle  = VPHandle;

            /* IOCTL the command */
            if (ioctl(fd, STLAYER_IOC_COMMITVIEWPORTPARAMS, &STLAYER_Ioctl_CommitViewPortParams) != 0)
            {
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                printf(" STLAYER_CommitViewPortParams():Ioctl error\n");
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode   = STLAYER_Ioctl_CommitViewPortParams.ErrorCode;
                /* No parameter from IOCTL structure */
            }
    }

    LeaveCriticalSection();

    return(ErrorCode);

}

