/*****************************************************************************
 *
 *  Module      : stvidtest_ioctl_lib.c
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description : Implementation for calling STAPI ioctl functions
 *  Copyright   : STMicroelectronics (c)2006
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


#include "stvidtest.h"
#include "stvidtest_ioctl.h"


/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/
static int      fd = -1; /* not open */
static int      UseCount = 0; /* Number of initialised devices */

/*** METHODS ****************************************************************/



/*******************************************************************************
Name        : STVIDTEST_Init
Description : Init VIDTEST driver
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVIDTEST_Init(const ST_DeviceName_t DeviceName, const STVIDTEST_InitParams_t * const InitParams_p)
{
    VIDTEST_Ioctl_Init_t        VIDTEST_Ioctl_Init;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (InitParams_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)      /* First time: initialise devices and units as empty */
    {
        char *devpath;

        devpath = getenv("STVIDTEST_IOCTL_DEV_PATH");       /* get the path for the device */
        if (devpath)
        {
            printf("Opening %s\n", devpath);
            if ((fd = open(devpath, O_RDWR)) == -1)   /* Open device */
            {
                perror(strerror(errno));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                /* Device is opened, use counter is incremented */
                UseCount++;
            }
        }
        else
        {
            fprintf(stderr,"The dev path enviroment variable is not defined: STVIDTEST_IOCTL_DEV_PATH \n");
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        /* Device already opened, increments use counter */
        UseCount++;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Copy parameters into IOCTL structure */
        strcpy(VIDTEST_Ioctl_Init.DeviceName, DeviceName);
        VIDTEST_Ioctl_Init.InitParams = *InitParams_p;

        /* IOCTL the command */
        if (ioctl(fd, STVIDTEST_IOC_INIT, &VIDTEST_Ioctl_Init) != 0)
        {
            /* IOCTL failed */
            perror(strerror(errno));
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STVIDTEST_Init():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = VIDTEST_Ioctl_Init.ErrorCode;

            /* Copy parameters from IOCTL structure */
            strcpy((char *)DeviceName, VIDTEST_Ioctl_Init.DeviceName);

            printf( "STVIDTEST_Init():Device '%s' initialised\n", VIDTEST_Ioctl_Init.DeviceName);
        }

        if (ErrorCode != ST_NO_ERROR)
        {
            UseCount--;
            if (UseCount == 0)
            {
                close(fd);
                printf( "%s(): Closing module VIDTEST !!!\n", __FUNCTION__);
            }
        }
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIDTEST_Term
Description : Terminate VIDTEST driver
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVIDTEST_Term(const ST_DeviceName_t DeviceName, const STVIDTEST_TermParams_t *const TermParams_p)
{
    VIDTEST_Ioctl_Term_t        VIDTEST_Ioctl_Term;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

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
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    strcpy(VIDTEST_Ioctl_Term.DeviceName, DeviceName);
    VIDTEST_Ioctl_Term.TermParams = *TermParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVIDTEST_IOC_TERM, &VIDTEST_Ioctl_Term) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVIDTEST_Term():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VIDTEST_Ioctl_Term.ErrorCode;

        /* Copy parameters from IOCTL structure */
        strcpy((char *)DeviceName, VIDTEST_Ioctl_Term.DeviceName);
    }

    if (UseCount)
    {
        UseCount--;
        if (UseCount == 0)
        {
            close(fd);
            printf( "%s(): Device VIDTEST closed !!!\n", __FUNCTION__);
        }
        printf("%s():Device '%s' terminated\n", __FUNCTION__, VIDTEST_Ioctl_Term.DeviceName);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIDTEST_GetRevision
Description : Revision
Parameters  : void
Assumptions : void
Limitations : void
Returns     : ST_Revision_t
*******************************************************************************/
ST_Revision_t  STVIDTEST_GetRevision(void)
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */
    return(VIDTEST_Revision);

} /* End of STVIDTEST_GetRevision() function */

/*******************************************************************************
Name        : STVIDTEST_GetCapability
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t  STVIDTEST_GetCapability( const ST_DeviceName_t DeviceName,
                                    STVIDTEST_Capability_t *  const Capability_p)
{
    VIDTEST_Ioctl_GetCapability_t   VIDTEST_Ioctl_GetCapability;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

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
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    strcpy( VIDTEST_Ioctl_GetCapability.DeviceName, DeviceName);
    VIDTEST_Ioctl_GetCapability.Capability = *Capability_p;

    /* IOCTL the command */
    if (ioctl(fd, STVIDTEST_IOC_GETCAPABILITY, &VIDTEST_Ioctl_GetCapability) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVIDTEST_GetCapability():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VIDTEST_Ioctl_GetCapability.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIDTEST_GetKernelInjectionFunctionPointers
Description : Get kernel pointers
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVIDTEST_GetKernelInjectionFunctionPointers(KernelInjectionType_t Type, void **GetWriteAdd, void **InformReadAdd)
{
    VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t        VIDTEST_Ioctl_GetKernelInjectionFunctionPointers;
    ST_ErrorCode_t                                            ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (   (Type == STVIDTEST_INJECT_NONE)
        || (GetWriteAdd == NULL)
        || (InformReadAdd == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    VIDTEST_Ioctl_GetKernelInjectionFunctionPointers.Type = Type;

    /* IOCTL the command */
    if (ioctl(fd, STVIDTEST_IOC_GETKERNELINJECTIONFUNCTIONPOINTERS, &VIDTEST_Ioctl_GetKernelInjectionFunctionPointers) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STVIDTEST_GetKernelInjectionFunctionPointers():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VIDTEST_Ioctl_GetKernelInjectionFunctionPointers.ErrorCode;

        /* Retrieve the kernel pointers */
        *GetWriteAdd = VIDTEST_Ioctl_GetKernelInjectionFunctionPointers.GetWriteAdd;
        *InformReadAdd = VIDTEST_Ioctl_GetKernelInjectionFunctionPointers.InformReadAdd;

        printf("GetWriteAdd returned to user: 0x%.8x\n",(U32)*GetWriteAdd);
        printf("InformReadAdd returned to user: 0x%.8x\n",(U32)*InformReadAdd);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIDTEST_InjectFromStreamBuffer
Description : Inject buffer in kernel
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVIDTEST_InjectFromStreamBuffer(STVID_Handle_t  CurrentHandle,
                                                S32             InjectLoop,
                                                S32             FifoNb,
                                                S32             BufferNb,
                                                void           *Buffer_p,
                                                U32             NbBytes)
{
    VIDTEST_Ioctl_InjectFromStreamBuffer_t  InjectFromStreamBuffer;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (Buffer_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    InjectFromStreamBuffer.Handle = CurrentHandle;
    InjectFromStreamBuffer.NbInjections = InjectLoop;
    InjectFromStreamBuffer.FifoNb = FifoNb;
    InjectFromStreamBuffer.BufferNb = BufferNb;
    InjectFromStreamBuffer.Buffer_p = Buffer_p;     /* Must be kernel address */
    InjectFromStreamBuffer.NbBytes = NbBytes;

    /* IOCTL the command */
    if (ioctl(fd, STVIDTEST_IOC_INJECTFROMSTREAMBUFFER, &InjectFromStreamBuffer) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" %s():Ioctl error\n", __FUNCTION__);
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = InjectFromStreamBuffer.ErrorCode;

        /* No data to retrieve */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIDTEST_KillInjectFromStreamBuffer
Description : Inject buffer in kernel
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVIDTEST_KillInjectFromStreamBuffer(S32 FifoNb, BOOL WaitEnd)
{
    VIDTEST_Ioctl_KillInjectFromStreamBuffer_t  KillInjectFromStreamBuffer;
    ST_ErrorCode_t                              ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    KillInjectFromStreamBuffer.FifoNb = FifoNb;
    KillInjectFromStreamBuffer.WaitEnfOfOnGoingInjection = WaitEnd;

    /* IOCTL the command */
    if (ioctl(fd, STVIDTEST_IOC_KILLINJECTFROMSTREAMBUFFER, &KillInjectFromStreamBuffer) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" %s():Ioctl error\n", __FUNCTION__);
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = KillInjectFromStreamBuffer.ErrorCode;

        /* No data to retrieve */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIDTEST_DecodeFromStreamBuffer
Description : Decode buffer in kernel
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVIDTEST_DecodeFromStreamBuffer(STVID_Handle_t  CurrentHandle,
                                                S32             FifoNb,
                                                S32             BuffNb,
                                                S32             NbLoops,
                                                void           *Buffer_p,
                                                U32             NbBytes)
{
    VIDTEST_Ioctl_DecodeFromStreamBuffer_t  DecodeFromStreamBuffer;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    if (Buffer_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    DecodeFromStreamBuffer.Handle = CurrentHandle;
    DecodeFromStreamBuffer.FifoNb = FifoNb;
    DecodeFromStreamBuffer.BuffNb = BuffNb;
    DecodeFromStreamBuffer.NbLoops = NbLoops;
    DecodeFromStreamBuffer.Buffer_p = Buffer_p;     /* Must be kernel address */
    DecodeFromStreamBuffer.NbBytes = NbBytes;

    /* IOCTL the command */
    if (ioctl(fd, STVIDTEST_IOC_DECODEFROMSTREAMBUFFER, &DecodeFromStreamBuffer) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" %s():Ioctl error\n", __FUNCTION__);
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = DecodeFromStreamBuffer.ErrorCode;

        /* No data to retrieve */
    }

    return(ErrorCode);
}
/* --------------------------------------------------------------------------- */
/* End of stvidtest_ioctl_lib.c */
