/*****************************************************************************
 *
 *  Module      : stvout_ioctl_lib.c
 *  Date        : 23-10-2005
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
#include <sys/mman.h>


#include "stvout.h"

#include "stvout_ioctl.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/
#define SIZE_PAGE 0x1000

/*** LOCAL VARIABLES *********************************************************/
static int fd=-1; /* not open */
static int UseCount=0; /* Number of initialised devices */


/*** METHODS ****************************************************************/

/*******************************************************************************
Name        : STVOUT_GetRevision
Returns     : Error code
*******************************************************************************/
ST_Revision_t STVOUT_GetRevision( void )
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return(VOUT_Revision);
}

/*
--------------------------------------------------------------------------------
Get capabilities of VOUT API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVOUT_GetCapability(const ST_DeviceName_t DeviceName, STVOUT_Capability_t * const Capability_p)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVOUT_Ioctl_GetCapability_t UserParams;
    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if( !UseCount )
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
        UserParams.Capability = *Capability_p;

        /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_GETCAPABILITY, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_GETCAPABILITY():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code */
            ErrorCode     = UserParams.ErrorCode;
            *Capability_p = UserParams.Capability;
        }
    }
    return ErrorCode;
}

/*
--------------------------------------------------------------------------------
Initialization of STVOUT Driver
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVOUT_Init(const ST_DeviceName_t DeviceName, const STVOUT_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

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

        devpath = getenv("STVOUT_IOCTL_DEV_PATH");  /* get the path for the device */

        if( devpath ){
            fd = open( devpath, O_RDWR );  /* open it */
            if( fd == -1 ){
                perror(strerror(errno));

                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }
        else{
            fprintf(stderr,"The dev path enviroment variable is not defined: STVOUT_IOCTL_DEV_PATH\n");
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    UseCount++;

    if( ErrorCode == ST_NO_ERROR ){
        STVOUT_Ioctl_Init_t UserParams;

        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
        UserParams.InitParams = *InitParams_p;

        /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_INIT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOCTL_Init():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }

    return ErrorCode;
}

/*
--------------------------------------------------------------------------------
Function : STVOUT_Open()
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVOUT_Open(const ST_DeviceName_t DeviceName, const STVOUT_OpenParams_t * const OpenParams_p, STVOUT_Handle_t * const Handle_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVOUT_Ioctl_Open_t UserParams;

    /* Exit now if parameters are bad */
    if (   (OpenParams_p == NULL)                           /* There must be parameters ! */
        || (Handle_p == NULL)                               /* Pointer to handle should be valid ! */
        || (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if( !UseCount )
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }


    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.OpenParams = *OpenParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVOUT_IOC_OPEN, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVOUT_IOCTL_OPEN():Ioctl error\n");
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
/*
--------------------------------------------------------------------------------
Function : STVOUT_Close()
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t STVOUT_Close(const STVOUT_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVOUT_Ioctl_Close_t UserParams;

    UserParams.Handle = Handle;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    /* IOCTL the command */
    if (ioctl(fd, STVOUT_IOC_CLOSE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVOUT_IOC_CLOSE():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

/*
--------------------------------------------------------------------------------
Function : STVOUT_Term()
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t STVOUT_Term (const ST_DeviceName_t DeviceName, const STVOUT_TermParams_t* const TermParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVOUT_Ioctl_Term_t UserParams;

    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (TermParams_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if( !UseCount )
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.TermParams = *TermParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVOUT_IOC_TERM, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVOUT_IOCTL_Term():Ioctl error\n");
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
Name        : STVOUT_Disable
Description : Disable VOUT driver
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVOUT_Disable(STVOUT_Handle_t Handle)
{
    STVOUT_Ioctl_Disable_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
                UserParams.Handle = Handle;

        /* IOCTL the command */
                if (ioctl(fd, STVOUT_IOC_DISABLE, &UserParams) != 0)
                {
                    perror(strerror(errno));

                    /* IOCTL failed */
                    ErrorCode = ST_ERROR_BAD_PARAMETER;

                    printf(" STVOUT_IOC_DISABLE():Ioctl error\n");
                }
                else
                {
                    /* IOCTL is successfull: */
                    /* and retrieve Error code and capability parameter */
                    ErrorCode = UserParams.ErrorCode;
                }
     }
     return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_Enable
Description : Enable  VOUT driver
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVOUT_Enable(STVOUT_Handle_t Handle)
{
    STVOUT_Ioctl_Enable_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle = Handle;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_ENABLE, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_ENABLE():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        :   STVOUT_SetOutputParams
Description :
Parameters  :   Handle, Output Parameters (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_SetOutputParams(
                const STVOUT_Handle_t         Handle,
                const STVOUT_OutputParams_t*  const OutputParams_p
                )
{
    STVOUT_Ioctl_SetOutputParams_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (OutputParams_p == NULL)                           /* There must be parameters ! */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle = Handle;
        UserParams.OutputParams = *OutputParams_p;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_SETOUTPUTPARAMS, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_SETOUTPUTPARAMS():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        :   STVOUT_GetOutputParams
Description :
Parameters  :   Handle, Output Parameters (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_GetOutputParams(
                const STVOUT_Handle_t         Handle,
                STVOUT_OutputParams_t*  const OutputParams_p
                )
{
    STVOUT_Ioctl_GetOutputParams_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if (OutputParams_p == NULL)                           /* There must be parameters ! */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle = Handle;
        UserParams.OutputParams = *OutputParams_p;


    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_GETOUTPUTPARAMS, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_GETOUTPUTPARAMS():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode = UserParams.ErrorCode;
            *OutputParams_p = UserParams.OutputParams;

        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_SetOption
Description :
Parameters  : Handle, Option Parameters (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/

ST_ErrorCode_t STVOUT_SetOption(
                const STVOUT_Handle_t         Handle,
                const STVOUT_OptionParams_t*  const OptionParams_p
                )
{
    STVOUT_Ioctl_SetOption_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;
    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {

        UserParams.Handle = Handle;
        UserParams.OptionParams = *OptionParams_p;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_SETOPTION, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_SETOPTION():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_GetOption
Description :
Parameters  : Handle, Option Parameters (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/

ST_ErrorCode_t STVOUT_GetOption(
                const STVOUT_Handle_t         Handle,
                STVOUT_OptionParams_t*  const OptionParams_p
                )
{
    STVOUT_Ioctl_GetOption_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle = Handle;
        UserParams.OptionParams = *OptionParams_p;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_GETOPTION, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_GETOPTION():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode = UserParams.ErrorCode;
            *OptionParams_p = UserParams.OptionParams;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_SetInputSource
Description :
Parameters  : Handle, Source (MAIN/AUX)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/

ST_ErrorCode_t STVOUT_SetInputSource(
                const STVOUT_Handle_t         Handle,
                STVOUT_Source_t               Source
                )
{
    STVOUT_Ioctl_SetInputSource_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle = Handle;
        UserParams.Source = Source;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_SETINPUTSOURCE, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_SETINPUTSOURCE():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode = UserParams.ErrorCode;

        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_GetDacSelect
Description :
Parameters  : Handle, Dacs selected (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/

ST_ErrorCode_t STVOUT_GetDacSelect(
                const STVOUT_Handle_t         Handle,
                U8*   const                    DacSelect_p
                )
{
    STVOUT_Ioctl_GetDacSelect_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle = Handle;
        UserParams.DacSelect = *DacSelect_p;


    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_GETDACSELECT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_GETDACSELECT():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode   = UserParams.ErrorCode;
            *DacSelect_p = UserParams.DacSelect;
        }
    }
    return ErrorCode;
}
/*******************************************************************************
Name        : STVOUT_GetState
Description :
Parameters  : Handle, State (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_GetState(
               const STVOUT_Handle_t         Handle,
               STVOUT_State_t*         const State_p
               )
{
    STVOUT_Ioctl_GetState_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle = Handle;
        UserParams.State  = *State_p;
    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_GETSTATE, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_GETSTATE():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode    = UserParams.ErrorCode;
            *State_p     = UserParams.State;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_GetTargetInformation
Description :
Parameters  : Handle, TargetInformation (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/

ST_ErrorCode_t STVOUT_GetTargetInformation(
               const STVOUT_Handle_t                Handle,
               STVOUT_TargetInformation_t*    const TargetInformation_p
               )
{
    STVOUT_Ioctl_GetTargetInformation_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {

        UserParams.Handle             = Handle;
        UserParams.TargetInformation  = *TargetInformation_p;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_GETTARGETINFORMATION, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_GETTARGETINFORMATION():Ioctl error\n");
        }
        else
        {
               /* offset in mmap must be address divisible by 0x1000 */
                TargetInformation_p->SinkInfo.Buffer_p = (void*) mmap(0,
                                                                     (UserParams.TargetInformation.SinkInfo.Size + SIZE_PAGE - 1),
                                                                     PROT_READ,
                                                                     MAP_SHARED,
                                                                     fd,
                                                                     (U32) (( (U32) UserParams.TargetInformation.SinkInfo.Buffer_p)/SIZE_PAGE) * SIZE_PAGE
                                                                     );
                /*printf(" STVOUT_IOC_GETTARGETINFORMATION():Buffer : %x\n", TargetInformation_p->SinkInfo.Buffer_p );*/

                *TargetInformation_p     = UserParams.TargetInformation;
                TargetInformation_p->SinkInfo.Size = UserParams.TargetInformation.SinkInfo.Size ;
                TargetInformation_p->SinkInfo.Buffer_p = (U8*)((U32)TargetInformation_p->SinkInfo.Buffer_p +
                                                         ((U32)UserParams.TargetInformation.SinkInfo.Buffer_p % SIZE_PAGE))  ;

            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode    = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_SendData
Description :
Parameters  : Handle, Buffer (pointer) , Size
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/

ST_ErrorCode_t STVOUT_SendData(
               const STVOUT_Handle_t                Handle,
               const STVOUT_InfoFrameType_t        InfoFrameType,
               U8* const                            Buffer_p,
               U32                                  Size
               )
{
    STVOUT_Ioctl_SendData_t    UserParams;
    ST_ErrorCode_t             ErrorCode = ST_NO_ERROR;
    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle        = Handle;
        UserParams.InfoFrameType = InfoFrameType;
        UserParams.Buffer        = *Buffer_p;
        UserParams.Size          = Size;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_SENDDATA, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_SENDDATA():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode    = UserParams.ErrorCode;

        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_Start()
Description :
Parameters  : Handle,
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/

ST_ErrorCode_t STVOUT_Start(
               const STVOUT_Handle_t                Handle
               )
{
    STVOUT_Ioctl_Start_t     UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;
    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle        = Handle;
    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_START, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            printf(" STVOUT_IOC_START():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code */
            ErrorCode    = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_GetStatistics
Description :
Parameters  : Handle, Statistics_p (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_GetStatistics(
               const STVOUT_Handle_t                Handle,
               STVOUT_Statistics_t*                 const Statistics_p
               )
{
    STVOUT_Ioctl_GetStatistics_t    UserParams;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle             = Handle;
        UserParams.Statistics         = *Statistics_p;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_GETSTATISTICS, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_GETSTATISTICS():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode           = UserParams.ErrorCode;
           *Statistics_p        = UserParams.Statistics;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_SetHDMIOutputType
Description :
Parameters  : Handle, Statistics_p (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_SetHDMIOutputType(
               const STVOUT_Handle_t                Handle,
               STVOUT_OutputType_t                  OutputType
               )
{
    STVOUT_Ioctl_SetHDMIOutputType_t    UserParams;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle             = Handle;
        UserParams.OutputType         = OutputType;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_SETHDMIOUTPUTTYPE, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_SETHDMIOUTPUTTYPE():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode           = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_SetHDCPParams()
Description : Set Device keys for the authentication protocol.
Parameters  : Handle, HDCP parameters (pointer).
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_SetHDCPParams(
               const STVOUT_Handle_t                Handle,
               const STVOUT_HDCPParams_t*            const HDCPParams_p
               )
{
    STVOUT_Ioctl_SetHDCPParams_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;
    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle        = Handle;
        UserParams.HDCPParams    = *HDCPParams_p;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_SETHDCPPARAMS, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_SETHDCPPARAMS():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code */
            ErrorCode    = UserParams.ErrorCode;

        }
    }
    return ErrorCode;
}


/*******************************************************************************
Name        : STVOUT_GetHDCPSinkParams
Description :
Parameters  : Handle, HDCPSinkParams_p (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/

ST_ErrorCode_t STVOUT_GetHDCPSinkParams(
               const STVOUT_Handle_t                Handle,
               STVOUT_HDCPSinkParams_t*             const HDCPSinkParams_p
               )
{
    STVOUT_Ioctl_GetHDCPSinkParams_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {

        UserParams.Handle             = Handle;
        UserParams.HDCPSinkParams     = *HDCPSinkParams_p;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_GETHDCPSINKPARAMS, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_GETHDCPSINKPARAMS():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode           = UserParams.ErrorCode;
            *HDCPSinkParams_p   = UserParams.HDCPSinkParams;

        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_UpdateForbiddenKSVs
Description :
Parameters  : Handle, KSVList_p (pointer) , KSVNumber
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_UpdateForbiddenKSVs(
               const STVOUT_Handle_t                Handle,
               U8* const                            KSVList_p,
               U32 const                            KSVNumber
               )
{
    STVOUT_Ioctl_UpdateForbiddenKSVs_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle             = Handle;
        UserParams.KSVList            = *KSVList_p;
        UserParams.KSVNumber          = KSVNumber;
    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_UPDATEFORBIDDENKSVS, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_UPDATEFORBIDDENKSVS():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode           = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_EnableDefaultOutput
Description :
Parameters  : Handle, DefaultOutput_p (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_EnableDefaultOutput(
               const STVOUT_Handle_t                Handle,
               const STVOUT_DefaultOutput_t*        const DefaultOutput_p
               )
{
    STVOUT_Ioctl_EnableDefaultOutput_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle             = Handle;
        UserParams.DefaultOutput      = *DefaultOutput_p;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_ENABLEDEFAULTOUTPUT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_ENABLEDEFAULTOUTPUT():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode           = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_DisableDefaultOutput
Description :
Parameters  : Handle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_DisableDefaultOutput(
               const STVOUT_Handle_t                Handle
               )
{
    STVOUT_Ioctl_DisableDefaultOutput_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle             = Handle;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_DISABLEDEFAULTOUTPUT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_DISABLEDEFAULTOUTPUT():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode           = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_AdjustChromaLumaDelay
Description :
Parameters  : Handle, CLDelay(in number of pixels)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_AdjustChromaLumaDelay( const STVOUT_Handle_t     Handle,
                                             const STVOUT_ChromaLumaDelay_t  CLDelay)
{
    STVOUT_Ioctl_AdjustChromaLumaDelay_t    UserParams;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle = Handle;
        UserParams.CLDelay = CLDelay;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_ADJUSTCHROMALUMADELAY, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_ADJUSTCHROMALUMADELAY():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode = UserParams.ErrorCode;

        }
    }
    return ErrorCode;
}





#ifdef STVOUT_CEC_PIO_COMPARE
/*******************************************************************************
Name        : STVOUT_SendCECMessage
Description :
Parameters  : Handle, Message_p (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_SendCECMessage(   const STVOUT_Handle_t   Handle,
                                        const STVOUT_CECMessage_t * const Message_p)
{
    STVOUT_Ioctl_SendCECMessage_t       UserParams;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle             = Handle;
        UserParams.Message            = *Message_p;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_SENDCECMESSAGE, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_SENDCECMESSAGE():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode           = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_CECPhysicalAddressAvailable
Description :
Parameters  : Handle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_CECPhysicalAddressAvailable(
               const STVOUT_Handle_t                Handle
               )
{
    STVOUT_Ioctl_CECPhysicalAddressAvailable_t      UserParams;
    ST_ErrorCode_t                                  ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle             = Handle;

    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_CECPHYSICALADDRAVAILABLE, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_CECPHYSICALADDRESSAVAILABLE():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode           = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVOUT_CECSetAdditionalAddress
Description :
Parameters  : Handle, Role
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_CECSetAdditionalAddress(
               const STVOUT_Handle_t                Handle,
               const STVOUT_CECRole_t               Role
              )
{
    STVOUT_Ioctl_CECSetAdditionalAddress_t      UserParams;
    ST_ErrorCode_t                              ErrorCode = ST_NO_ERROR;

    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        UserParams.Handle             = Handle ;
        UserParams.Role               = Role ;


    /* IOCTL the command */
        if (ioctl(fd, STVOUT_IOC_CECSETADDITIONALADDRESS, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVOUT_IOC_CECSETADDITIONALADDRESS():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: */
            /* and retrieve Error code and capability parameter */
            ErrorCode           = UserParams.ErrorCode;
        }
    }
    return ErrorCode;
}


#endif /*STVOUT_CEC_PIO_COMPARE*/
