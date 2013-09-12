/*****************************************************************************
 *
 *  Module      : tsmerger_ioctl
 *  Date        : 24-07-2005
 *  Author      : STIEGLITZP
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005 
 *****************************************************************************/


#ifndef TSMERGER_IOCTL_H
#define TSMERGER_IOCTL_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */

#include "stpti.h"
#include "tsmerger.h"


/*** IOCtl defines ***/

#define TSMERGER_IOCTL_TYPE   0XFF     /* Unique module id - See 'ioctl-number.txt' */
#define TSMERGER_DRIVER_ID TSMERGER_IOCTL_TYPE

            
/*
ST_ErrorCode_t TSMERGER_SetSWTS0BypassMode(void);                        
*/

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SetSWTS0BypassMode_t;

#define TSMERGER_IOC_SETSWTS0BYPASSMODE _IOWR(TSMERGER_DRIVER_ID, 1, TSMERGER_SetSWTS0BypassMode_t *)

/*
ST_ErrorCode_t TSMERGER_SetTSIn0BypassMode(void);                            
*/

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SetTSIn0BypassMode_t;

#define TSMERGER_IOC_SETTSIN0BYPASSMODE _IOWR(TSMERGER_DRIVER_ID, 2, TSMERGER_SetTSIn0BypassMode_t *)

/*
ST_ErrorCode_t TSMERGER_SetInput(TSMERGER_Source_t Source, TSMERGER_Destination_t Destination, U8 SyncLock, U8 SyncDrop, 
                            U16 SyncPeriod, BOOL SerialNotParallel, BOOL SyncNotASync, BOOL Tagging );
*/
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
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SetInput_t;

#define TSMERGER_IOC_SETINPUT _IOWR(TSMERGER_DRIVER_ID, 3, TSMERGER_SetInput_t *)


/*
ST_ErrorCode_t TSMERGER_ClearInput(TSMERGER_Source_t Source, TSMERGER_Destination_t Destination);
*/
typedef struct
{
    TSMERGER_Source_t Source;
    TSMERGER_Destination_t Destination;

    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_ClearInput_t;

#define TSMERGER_IOC_CLEARINPUT _IOWR(TSMERGER_DRIVER_ID, 4, TSMERGER_ClearInput_t *)

/*
ST_ErrorCode_t TSMERGER_ClearAll(void);
*/
typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_ClearAll_t;

#define TSMERGER_IOC_CLEARALL _IOWR(TSMERGER_DRIVER_ID, 5, TSMERGER_ClearAll_t *)

/*
ST_ErrorCode_t TSMERGER_SetPriority(TSMERGER_Source_t Source, U8 Priority);
*/
typedef struct
{
    TSMERGER_Source_t Source;
    U8 Priority;
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SetPriority_t;

#define TSMERGER_IOC_SETPRIORITY _IOWR(TSMERGER_DRIVER_ID, 6, TSMERGER_SetPriority_t *)

/*
ST_ErrorCode_t TSMERGER_SetSync(TSMERGER_Source_t Source, U16 SyncPeriod, U8 SyncLock, U8 SyncDrop);
*/
typedef struct
{
    TSMERGER_Source_t Source;
    U16 SyncPeriod;
    U8 SyncLock;
    U8 SyncDrop;
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SetSync_t;

#define TSMERGER_IOC_SETSYNC _IOWR(TSMERGER_DRIVER_ID, 7, TSMERGER_SetSync_t *)

/*
ST_ErrorCode_t TSMERGER_SWTSInjectWordArray(U32 *Data_p, int size, TSMERGER_Source_t ts, int *words_output)
*/
typedef struct
{
    U32 *Data_p;
    int size;
    TSMERGER_Source_t ts;
    int words_output;
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_SWTSInjectWordArray_t;

#define TSMERGER_IOC_SWTSINJECTWORD _IOWR(TSMERGER_DRIVER_ID, 8, TSMERGER_SWTSInjectWordArray_t *)

/*
ST_ErrorCode_t TSMERGER_IIFSetSyncPeriod(U32 OutputInjectionSize);
*/

#define TSMERGER_IOC_IIFSETSYNCPERIOD _IOR(TSMERGER_DRIVER_ID, 9, U32)
        
typedef struct
{
    U32 OutputInjectionSize;
    ST_ErrorCode_t  ErrorCode;
} TSMERGER_IIFSetSyncPeriod_t;
        
#endif
