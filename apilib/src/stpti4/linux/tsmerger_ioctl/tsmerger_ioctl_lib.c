/*****************************************************************************
 *
 *  Module      : tsmerger_ioctl_lib.c
 *  Date        : 24-07-2005
 *  Author      : STIEGLITZP
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

#include "stpti.h"
#include "tsmerger.h"

#include "tsmerger_ioctl.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/
/* Return a file descriptor to the TSMERGER test driver. */
int OpenTSMERGER()
{
    char *devpath;
    int fd = -1;

    devpath = getenv("TSMERGER_IOCTL_DEV_PATH");  /* get the path for the device */

    if( devpath ){
        fd = open( devpath, O_RDWR );  /* open it */
        if( fd == -1 )
            perror(strerror(errno));
    }
    else{
        fprintf(stderr,"The dev path enviroment variable is not defined: TSMERGER_IOCTL_DEV_PATH\n");
    }

    return fd;
}

/*
ST_ErrorCode_t TSMERGER_SetSWTS0BypassMode(void);                            


typedef struct
{
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SetSWTS0BypassMode_t;

#define TSMERGER_IOC_SETSWTS0BYPASSMODE _IOWR(TSMERGER_DRIVER_ID, 2, TSMERGER_SetTSIn0BypassMode_t *)
*/
ST_ErrorCode_t TSMERGER_SetSWTS0BypassMode(void)
{
    int fd;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TSMERGER_SetSWTS0BypassMode_t UserParams;

    fd = OpenTSMERGER();
    if( fd != -1 ){
        if (ioctl(fd,TSMERGER_IOC_SETSWTS0BYPASSMODE, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" TSMERGER_SetSWTS0BypassMode():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    close(fd);
    
    return ST_NO_ERROR;
}

/*
ST_ErrorCode_t TSMERGER_SetTSIn0BypassMode(void);                            


typedef struct
{
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SetTSIn0BypassMode_t;

#define TSMERGER_IOC_SETTSIN0BYPASSMODE _IOWR(TSMERGER_DRIVER_ID, 2, TSMERGER_SetTSIn0BypassMode_t *)
*/
ST_ErrorCode_t TSMERGER_SetTSIn0BypassMode(void)
{
    int fd;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TSMERGER_SetTSIn0BypassMode_t UserParams;

    fd = OpenTSMERGER();
    if( fd != -1 ){
        if (ioctl(fd,TSMERGER_IOC_SETTSIN0BYPASSMODE, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" TSMERGER_SetTSIn0BypassMode():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    close(fd);
    
    return ST_NO_ERROR;
}


/*
ST_ErrorCode_t TSMERGER_SetInput(TSMERGER_Source_t Source, TSMERGER_Destination_t Destination, U8 SyncLock, U8 SyncDrop, 
                            U16 SyncPeriod, BOOL SerialNotParallel, BOOL SyncNotASync, BOOL Tagging );

typedef struct
{
    TSMERGER_Source_t Source;
    TSMERGER_Destination_t Destination;
    U8 SyncLock;
    U8 SyncDrop;
    U16 SyncPeriod;
    BOOL SerialNotParallel;
    BOOL SyncNotASync;
    BOOL Tagging;
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SetInput_t;

#define TSMERGER_IOC_SETINPUT _IOWR(TSMERGER_DRIVER_ID, 3, TSMERGER_SetInput_t *)
*/
ST_ErrorCode_t TSMERGER_SetInput(TSMERGER_Source_t Source,
                                 TSMERGER_Destination_t Destination,
                                 U8 SyncLock,
                                 U8 SyncDrop, 
                                 U16 SyncPeriod,
                                 BOOL SerialNotParallel,
                                 BOOL SyncNotASync,
                                 BOOL Tagging )
{
    int fd;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TSMERGER_SetInput_t UserParams;

    UserParams.Source = Source;
    UserParams.Destination = Destination;
    UserParams.SyncLock = SyncLock;
    UserParams.SyncDrop = SyncDrop;
    UserParams.SyncPeriod = SyncPeriod;
    UserParams.SerialNotParallel = SerialNotParallel;
    UserParams.SyncNotASync = SyncNotASync;
    UserParams.Tagging = Tagging;
    
    fd = OpenTSMERGER();
    if( fd != -1 ){
        if (ioctl(fd,TSMERGER_IOC_SETINPUT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" TSMERGER_SetInput():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    close(fd);
    
    return ST_NO_ERROR;
}

/*
ST_ErrorCode_t TSMERGER_ClearInput(TSMERGER_Source_t Source, TSMERGER_Destination_t Destination);

typedef struct
{
    TSMERGER_Source_t Source;
    TSMERGER_Destination_t Destination;

    ST_ErrorCode_t  ErrorCode;
} TSMERGER_ClearInput_t;

#define TSMERGER_IOC_CLEARINPUT _IOWR(TSMERGER_DRIVER_ID, 4, TSMERGER_ClearInput_t *)
*/
ST_ErrorCode_t TSMERGER_ClearInput(TSMERGER_Source_t Source,
                                   TSMERGER_Destination_t Destination)
{
    int fd;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TSMERGER_ClearInput_t UserParams;

    UserParams.Source = Source;
    UserParams.Destination = Destination;
    
    fd = OpenTSMERGER();
    if( fd != -1 ){
        if (ioctl(fd,TSMERGER_IOC_CLEARINPUT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" TSMERGER_ClearInput():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    close(fd);
    
    return ST_NO_ERROR;
}

/*
ST_ErrorCode_t TSMERGER_ClearAll(void);

typedef struct
{
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_ClearAll_t;

#define TSMERGER_IOC_CLEARALL _IOWR(TSMERGER_DRIVER_ID, 5, TSMERGER_ClearAll_t *)
*/
ST_ErrorCode_t TSMERGER_ClearAll(void)
{
    int fd;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TSMERGER_ClearAll_t UserParams;

    fd = OpenTSMERGER();
    if( fd != -1 ){
        if (ioctl(fd,TSMERGER_IOC_CLEARALL, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" TSMERGER_ClearAll():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    close(fd);
    
    return ST_NO_ERROR;
}

/*
ST_ErrorCode_t TSMERGER_SetPriority(TSMERGER_Source_t Source, U8 Priority);

typedef struct
{
    TSMERGER_Source_t Source;
    U8 Priority;
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SetPriority_t;

#define TSMERGER_IOC_SETPRIORITY _IOWR(TSMERGER_DRIVER_ID, 6, TSMERGER_SetPriority_t *)
*/
ST_ErrorCode_t TSMERGER_SetPriority(TSMERGER_Source_t Source, U8 Priority)
{
    int fd;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TSMERGER_SetPriority_t UserParams;

    UserParams.Source = Source;
    UserParams.Priority = Priority;
    
    fd = OpenTSMERGER();
    if( fd != -1 ){
        if (ioctl(fd,TSMERGER_IOC_SETPRIORITY, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" TSMERGER_SetPriority():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    close(fd);
    
    return ST_NO_ERROR;
}

/*
ST_ErrorCode_t TSMERGER_SetSync(TSMERGER_Source_t Source, U16 SyncPeriod, U8 SyncLock, U8 SyncDrop);

typedef struct
{
    TSMERGER_Source_t Source;
    U16 SyncPeriod;
    U8 SyncLock;
    U8 SyncDrop;
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SetSync_t;

#define TSMERGER_IOC_SETSYNC _IOWR(TSMERGER_DRIVER_ID, 7, TSMERGER_SetSync_t *)
*/
ST_ErrorCode_t TSMERGER_SetSync(TSMERGER_Source_t Source,
                                U16 SyncPeriod,
                                U8 SyncLock,
                                U8 SyncDrop)
{
    int fd;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TSMERGER_SetSync_t UserParams;

    UserParams.Source = Source;
    UserParams.SyncPeriod = SyncPeriod;
    UserParams.SyncLock = SyncLock;
    UserParams.SyncDrop = SyncDrop;
    
    fd = OpenTSMERGER();
    if( fd != -1 ){
        if (ioctl(fd,TSMERGER_IOC_SETSYNC, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" TSMERGER_SetSync():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    close(fd);
    
    return ST_NO_ERROR;
}

ST_ErrorCode_t TSMERGER_SWTSInjectWordArray(U32 *Data_p, int size, TSMERGER_Source_t ts, int *words_output)
{
    int fd;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TSMERGER_SWTSInjectWordArray_t UserParams;

    UserParams.Data_p = Data_p;
    UserParams.size = size;
    UserParams.ts = ts;
    
    fd = OpenTSMERGER();
    if( fd != -1 ){
        if (ioctl(fd,TSMERGER_IOC_SWTSINJECTWORD, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" TSMERGER_SWTSInjectWordArray():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            *words_output = UserParams.words_output;
            ErrorCode = UserParams.ErrorCode;
        }
    }
    close(fd);
    
    return ST_NO_ERROR;
}

ST_ErrorCode_t TSMERGER_IIFSetSyncPeriod(U32 OutputInjectionSize)
{
    int fd;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    TSMERGER_IIFSetSyncPeriod_t UserParams;

    UserParams.OutputInjectionSize = OutputInjectionSize;
    
    fd = OpenTSMERGER();
    if( fd != -1 ){
        if (ioctl(fd,TSMERGER_IOC_IIFSETSYNCPERIOD, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" TSMERGER_SWTS1InjectWord():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
        }
    }
    close(fd);
    
    return ST_NO_ERROR;
}
