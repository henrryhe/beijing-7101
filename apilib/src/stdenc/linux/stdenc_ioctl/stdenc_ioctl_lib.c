/*****************************************************************************
 *
 *  Module      : stdenc_ioctl_lib.c
 *  Date        : 21-10-2005
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

#include "stdenc.h"

#include "stdenc_ioctl.h"
#include "stos.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/
#define EnterCriticalSection()
#define LeaveCriticalSection()

/*** LOCAL VARIABLES *********************************************************/
static int fd=-1; /* not open */
static int UseCount=0; /* Number of initialised devices */

/*** METHODS ****************************************************************/

ST_ErrorCode_t STDENC_Init( const ST_DeviceName_t DeviceName, const STDENC_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (InitParams_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }


    if( !UseCount ){ /* First time? */
        char *devpath;

        devpath = getenv("STDENC_IOCTL_DEV_PATH");  /* get the path for the device */

        if( devpath ){
            fd = open( devpath, O_RDWR );  /* open it */
            if( fd == -1 ){
                perror(strerror(errno));

                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }

        }
        else{
            fprintf(stderr,"The dev path enviroment variable is not defined: STDENC_IOCTL_DEV_PATH\n");
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    UseCount++;

    if( ErrorCode == ST_NO_ERROR ){
        STDENC_Ioctl_Init_t UserParams;

        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
        UserParams.InitParams = *InitParams_p;

        /* IOCTL the command */
        if (ioctl(fd, STDENC_IOC_INIT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            STTBX_Print((" STDENC_IOCTL_Init():Ioctl error\n"));
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }

    return ErrorCode;
}

ST_ErrorCode_t STDENC_Open( const ST_DeviceName_t DeviceName, const STDENC_OpenParams_t * const OpenParams_p, STDENC_Handle_t * const Handle_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STDENC_Ioctl_Open_t UserParams;

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
    if (ioctl(fd, STDENC_IOC_OPEN, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Print((" STDENC_IOCTL_OPEN():Ioctl error\n"));
    }
    else
    {
        /* IOCTL is successfull: */
        /* get the handle. */
        *Handle_p = UserParams.Handle;
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return  ErrorCode;
}

ST_ErrorCode_t STDENC_Close(const STDENC_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STDENC_Ioctl_Close_t UserParams;

    UserParams.Handle = Handle;
    if( !UseCount )
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    /* IOCTL the command */
    if (ioctl(fd, STDENC_IOC_CLOSE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Print((" STDENC_IOC_CLOSE():Ioctl error\n"));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STDENC_Term(const ST_DeviceName_t DeviceName, const STDENC_TermParams_t * const TermParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STDENC_Ioctl_Term_t UserParams;

    /* Exit now if parameters are bad */
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
    if (ioctl(fd, STDENC_IOC_TERM, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Print((" STDENC_IOCTL_Term():Ioctl error\n"));
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

/*
--------------------------------------------------------------------------------
Get revision of DENC API
--------------------------------------------------------------------------------
*/
ST_Revision_t STDENC_GetRevision(void)
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return(DENC_Revision);
} /* End of STDENC_GetRevision() function */


/*
--------------------------------------------------------------------------------
Get capabilities of DENC API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STDENC_GetCapability(const ST_DeviceName_t DeviceName, STDENC_Capability_t * const Capability_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STDENC_Ioctl_GetCapability_t           STDENC_Ioctl_GetCapability;
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
              /* Copy parameters into IOCTL structure */
            strcpy( STDENC_Ioctl_GetCapability.DeviceName, DeviceName);
            STDENC_Ioctl_GetCapability.Capability = *Capability_p;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_GETCAPABILITY, &STDENC_Ioctl_GetCapability) != 0)
            {
                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_GetCapability():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_GetCapability.ErrorCode;
                *Capability_p = STDENC_Ioctl_GetCapability.Capability;
                /* No parameter from IOCTL structure */
            }

    }
    return(ErrorCode);

 } /* End of STDENC_GetCapability() function */

/*
--------------------------------------------------------------------------------
API routine : Set encoding mode
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t  STDENC_SetEncodingMode(
                const STDENC_Handle_t         Handle,
                const STDENC_EncodingMode_t * const EncodingMode_p
                )
{
    STDENC_Ioctl_SetEncodingMode_t        STDENC_Ioctl_SetEncodingMode;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

        if (EncodingMode_p == NULL)
        {
             return (ST_ERROR_BAD_PARAMETER);
        }
            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_SetEncodingMode.Handle = Handle;
            STDENC_Ioctl_SetEncodingMode.EncodingMode = *EncodingMode_p;
            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_SETENCODINGMODE, &STDENC_Ioctl_SetEncodingMode) != 0)
            {
                 perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_SetEncodingMode():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_SetEncodingMode.ErrorCode;
                /* No parameter from IOCTL structure */
            }

    LeaveCriticalSection();

    return(ErrorCode);


}


/*
--------------------------------------------------------------------------------
API routine : Get encoding mode
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t  STDENC_GetEncodingMode(
                const STDENC_Handle_t    Handle,
                STDENC_EncodingMode_t *  const EncodingMode_p
                )
{
    STDENC_Ioctl_GetEncodingMode_t        STDENC_Ioctl_GetEncodingMode;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

        if (EncodingMode_p == NULL)
        {
             return(ST_ERROR_BAD_PARAMETER);
        }
            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_GetEncodingMode.Handle = Handle;
            STDENC_Ioctl_GetEncodingMode.EncodingMode = *EncodingMode_p;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_GETENCODINGMODE, &STDENC_Ioctl_GetEncodingMode) != 0)
            {

                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_GetEncodingMode():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_GetEncodingMode.ErrorCode;

                /* No parameter from IOCTL structure */
                *EncodingMode_p = STDENC_Ioctl_GetEncodingMode.EncodingMode;
            }

    LeaveCriticalSection();

    return(ErrorCode);

} /* end of STDENC_GetEncodingMode */


/*
--------------------------------------------------------------------------------
API routine : Set mode option
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STDENC_SetModeOption (
                    const STDENC_Handle_t          Handle,
                    const STDENC_ModeOption_t *    const ModeOption_p
                    )
{
    STDENC_Ioctl_SetModeOption_t          STDENC_Ioctl_SetModeOption;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

        if (ModeOption_p == NULL)
        {
             return (ST_ERROR_BAD_PARAMETER);
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_SetModeOption.Handle = Handle;
            STDENC_Ioctl_SetModeOption.ModeOption = *ModeOption_p;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_SETMODEOPTION, &STDENC_Ioctl_SetModeOption) != 0)
            {
                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_SetModeOption():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_SetModeOption.ErrorCode;

                /* No parameter from IOCTL structure */

            }

    LeaveCriticalSection();

    return(ErrorCode);
}
/*
--------------------------------------------------------------------------------
API routine : Get mode option
ATTENTION : ModeOption_p->Option is an INPUT although in a "Get" function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STDENC_GetModeOption (
                    const STDENC_Handle_t  Handle,
                    STDENC_ModeOption_t *  const ModeOption_p
                    )
{
    STDENC_Ioctl_GetModeOption_t          STDENC_Ioctl_GetModeOption;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_GetModeOption.Handle = Handle;
            STDENC_Ioctl_GetModeOption.ModeOption = *ModeOption_p;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_GETMODEOPTION, &STDENC_Ioctl_GetModeOption) != 0)
            {

                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_GetModeOption():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_GetModeOption.ErrorCode;

                /* No parameter from IOCTL structure */
                *ModeOption_p = STDENC_Ioctl_GetModeOption.ModeOption;
            }

    LeaveCriticalSection();

    return(ErrorCode);

}

/*
--------------------------------------------------------------------------------
API routine : Set colorbar pattern
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t  STDENC_SetColorBarPattern(
                const STDENC_Handle_t       Handle,
                const BOOL                  ColorBarPattern
                )
 {
    STDENC_Ioctl_SetColorBarPattern_t STDENC_Ioctl_SetColorBarPattern;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_SetColorBarPattern.Handle  = Handle;
            STDENC_Ioctl_SetColorBarPattern.ColorBarPattern = ColorBarPattern;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_SETCOLORBARPATTERN, &STDENC_Ioctl_SetColorBarPattern) != 0)
            {

                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_SetColorBarPattern():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_SetColorBarPattern.ErrorCode;


            }

    LeaveCriticalSection();

    return(ErrorCode);

 } /* end of STDENC_SetColorBarPattern */



/*******************************************************************************
Name        : STDENC_ReadReg8
Description : register access functions
Parameters  : Handle  : IN  : Handle on DENC driver connection
 *            Addr    : IN  : Register offset
 *            Value_p : OUT : value read
Assumptions : Handle is valid.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t STDENC_ReadReg8(STDENC_Handle_t Handle, const U32 Addr, U8 * const Value_p)
{
    STDENC_Ioctl_ReadReg8_t               STDENC_Ioctl_ReadReg8;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_ReadReg8.Handle  = Handle;
            STDENC_Ioctl_ReadReg8.Addr    = Addr;
            /*STDENC_Ioctl_ReadReg8.Value = *Value_p;*/

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_READREG8, &STDENC_Ioctl_ReadReg8) != 0)
            {
                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_ReadReg8():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_ReadReg8.ErrorCode;

                /* No parameter from IOCTL structure */
                *Value_p = (STDENC_Ioctl_ReadReg8.Value);
            }

    LeaveCriticalSection();

    return(ErrorCode);

} /* End of STDENC_ReadReg8() function */

/*******************************************************************************
Name        : STDENC_WriteReg8
Description : register access function
Parameters  : Handle  : IN  : Handle on DENC driver connection
 *            Addr    : IN  : Register offset
 *            Value   : IN  : value to write
Assumptions : Handle is valid.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t STDENC_WriteReg8(STDENC_Handle_t Handle, const U32 Addr, const U8 Value)
{
    STDENC_Ioctl_WriteReg8_t              STDENC_Ioctl_WriteReg8;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_WriteReg8.Handle  = Handle;
            STDENC_Ioctl_WriteReg8.Addr    = Addr;
            STDENC_Ioctl_WriteReg8.Value   = Value;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_WRITEREG8, &STDENC_Ioctl_WriteReg8) != 0)
            {
                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_GetModeOption():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_WriteReg8.ErrorCode;

                /* No parameter from IOCTL structure */
            }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STDENC_WriteReg8() function */

/*******************************************************************************
Name        : STDENC_CheckHandle
Description : check STDENC connection Handle
Parameters  : Handle  : IN  : Handle on DENC driver connection
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STDENC_CheckHandle(STDENC_Handle_t Handle)
{
    STDENC_Ioctl_CheckHandle_t            STDENC_Ioctl_CheckHandle;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_CheckHandle.Handle  = Handle;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_CHECKHANDLE, &STDENC_Ioctl_CheckHandle) != 0)
            {
                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_CheckHandle():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_CheckHandle.ErrorCode;

                /* No parameter from IOCTL structure */
            }

    LeaveCriticalSection();

    return(ErrorCode);

} /* End of STDENC_CheckHandle() function */


/*******************************************************************************
Name        : STDENC_RegAccessLock
Description : lock register access for current device
Parameters  : Handle  : IN  : Handle on DENC driver connection
Assumptions : Handle is valid.
Limitations :
Returns     : none
*******************************************************************************/
void STDENC_RegAccessLock(STDENC_Handle_t Handle)
{
    STDENC_Ioctl_RegAccessLock_t          STDENC_Ioctl_RegAccessLock;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            ErrorCode = ST_ERROR_INVALID_HANDLE;
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_RegAccessLock.Handle  = Handle;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_REGACCESSLOCK, &STDENC_Ioctl_RegAccessLock) != 0)
            {
                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_RegAccessLock():Ioctl error\n"));
            }

    LeaveCriticalSection();

    if ( ErrorCode != ST_NO_ERROR ) {
       STTBX_Print(("STDENC_RegAccessLock():error\n"));
    }

    return;

} /* End of STDENC_RegAccessLock() function */

/*******************************************************************************
Name        : STDENC_RegAccessUnlock
Description : unlock register access for current device
Parameters  : Handle  : IN  : Handle on DENC driver connection
Assumptions : Handle is valid.
Limitations :
Returns     : none
*******************************************************************************/
void STDENC_RegAccessUnlock(STDENC_Handle_t Handle)
{
    STDENC_Ioctl_RegAccessUnlock_t        STDENC_Ioctl_RegAccessUnlock;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            ErrorCode = ST_ERROR_INVALID_HANDLE;
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_RegAccessUnlock.Handle  = Handle;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_REGACCESSUNLOCK, &STDENC_Ioctl_RegAccessUnlock) != 0)
            {
                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print(("STDENC_RegAccessUnlock():Ioctl error\n"));
            }

    if ( ErrorCode != ST_NO_ERROR ) {
       STTBX_Print(("STDENC_RegAccessUnLock():error\n"));
    }

    LeaveCriticalSection();

    return;

} /* End of STDENC_RegAccessUnlock() function */


/*******************************************************************************
Name        : STDENC_AndReg8
Description : register access functions
Parameters  : Handle  : IN  : Handle on DENC driver connection
 *            Addr    : IN  : Register offset
 *            AndMask : IN  : mask to apply to read value before write
Assumptions : Handle is valid.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t STDENC_AndReg8(STDENC_Handle_t Handle, const U32 Addr, const U8 AndMask)
{
    STDENC_Ioctl_AndReg8_t                STDENC_Ioctl_AndReg8;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_AndReg8.Handle  = Handle;
            STDENC_Ioctl_AndReg8.Addr    = Addr;
            STDENC_Ioctl_AndReg8.AndMask = AndMask;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_ANDREG8, &STDENC_Ioctl_AndReg8) != 0)
            {

                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_AndReg8():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_AndReg8.ErrorCode;

                /* No parameter from IOCTL structure */
            }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STDENC_AndReg8() function */

/*******************************************************************************
Name        : STDENC_OrReg8
Description : register access functions
Parameters  : Handle  : IN  : Handle on DENC driver connection
 *            Addr    : IN  : Register offset
 *            OrMask : IN  : mask to apply to read value before write
Assumptions : Handle is valid.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t STDENC_OrReg8(STDENC_Handle_t Handle, const U32 Addr, const U8 OrMask)
{
    STDENC_Ioctl_OrReg8_t                 STDENC_Ioctl_OrReg8;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_OrReg8.Handle = Handle;
            STDENC_Ioctl_OrReg8.Addr = Addr;
            STDENC_Ioctl_OrReg8.OrMask = OrMask;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_ORREG8, &STDENC_Ioctl_OrReg8) != 0)
            {
                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_OrReg8():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_OrReg8.ErrorCode;

                /* No parameter from IOCTL structure */
            }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STDENC_OrReg8() function */

/*******************************************************************************
Name        : STDENC_MaskReg8
Description : register access functions
Parameters  : Handle  : IN  : Handle on DENC driver connection
 *            Addr    : IN  : Register offset
 *            AndMask : IN  : mask to apply to read value before write
 *            OrMask : IN  : mask to apply to read value before write
Assumptions : Handle is valid.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t STDENC_MaskReg8(STDENC_Handle_t Handle, const U32 Addr, const U8 AndMask, const U8 OrMask)
{
    STDENC_Ioctl_MaskReg8_t               STDENC_Ioctl_MaskReg8;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();
        if( !UseCount )
        {
            return (ST_ERROR_INVALID_HANDLE);
        }

            /* Copy parameters into IOCTL structure */
            STDENC_Ioctl_MaskReg8.Handle = Handle;
            STDENC_Ioctl_MaskReg8.Addr = Addr;
            STDENC_Ioctl_MaskReg8.AndMask = AndMask;
            STDENC_Ioctl_MaskReg8.OrMask = OrMask;

            /* IOCTL the command */
            if (ioctl(fd, STDENC_IOC_MASKREG8, &STDENC_Ioctl_MaskReg8) != 0)
            {

                perror(strerror(errno));
                /* IOCTL failed */
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Print((" STDENC_MaskReg8():Ioctl error\n"));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = STDENC_Ioctl_MaskReg8.ErrorCode;

                /* No parameter from IOCTL structure */
            }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STDENC_MaskReg8() function */


