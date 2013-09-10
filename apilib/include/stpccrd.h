/**********************************************************************

File Name   : stpccrd.h

Description : External function prototypes for stpccrd API
              Declares the function calls and their formal
              parameters.  Requires prior inclusion of
              stddefs.h file to define parameter sizes.

History     :

Copyright (C) 2006 STMicroelectronics

Reference  : ST API Definition "STPCCRD Driver API" STB-API-332
***********************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STPCCRD_H
#define __STPCCRD_H

/* Includes --------------------------------------------------------- */
#include "stddefs.h"

#ifdef ST_OSLINUX
#include "compat.h"
#endif

#ifdef __cplusplus
    extern "C" {
#endif
/* Exported Constants ----------------------------------------------- */

/* Driver ID */
#define STPCCRD_DRIVER_ID           24

/* Driver Base */
#define STPCCRD_DRIVER_BASE         ( STPCCRD_DRIVER_ID << 16 )

/* Error Codes */
#define STPCCRD_ERROR_BADREAD       ( STPCCRD_DRIVER_BASE )
#define STPCCRD_ERROR_BADWRITE      ( STPCCRD_DRIVER_BASE + 1 )
#define STPCCRD_ERROR_CIS_READ      ( STPCCRD_DRIVER_BASE + 2 )
#define STPCCRD_ERROR_NO_MODULE     ( STPCCRD_DRIVER_BASE + 3 )
#define STPCCRD_ERROR_EVT           ( STPCCRD_DRIVER_BASE + 4 )
#define STPCCRD_ERROR_TASK_FAILURE  ( STPCCRD_DRIVER_BASE + 5 )
#define STPCCRD_ERROR_CI_RESET      ( STPCCRD_DRIVER_BASE + 6 )
#define STPCCRD_ERROR_CANT_POWEROFF ( STPCCRD_DRIVER_BASE + 7 )
#define STPCCRD_ERROR_I2C           ( STPCCRD_DRIVER_BASE + 8 )
#define STPCCRD_ERROR_I2C_READ      ( STPCCRD_DRIVER_BASE + 9 )
#define STPCCRD_ERROR_I2C_WRITE     ( STPCCRD_DRIVER_BASE + 10 )
#define STPCCRD_ERROR_MOD_RESET     ( STPCCRD_DRIVER_BASE + 11 )

#define STPCCRD_MIN_OPEN_DEVICE     1   /* minimum value for MaxOpen */

#if defined(ST_5516) || defined(ST_5517) || defined(ST_7100) || defined(ST_7710) || defined(ST_5100) \
|| defined(ST_7109)
#define STPCCRD_MAX_OPEN_DEVICE     2   /* maximum value for MaxOpen */
#elif defined(ST_5105) || defined(ST_5107)
#define STPCCRD_MAX_OPEN_DEVICE     1
#elif defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
#ifdef STV0700
#define STPCCRD_MAX_OPEN_DEVICE     2   /* maximum value for MaxOpen */
#endif
/* In case of STV0701 only one open is allowed */
#ifdef STV0701
#define STPCCRD_MAX_OPEN_DEVICE     1   /* maximum value for MaxOpen */
#endif
#endif /*ST_5514 || ST_5518*/

/* Exported Events ----------------------------------------------- */

typedef enum STPCCRD_EventType_e
{
    STPCCRD_EVT_MOD_A_INSERTED = STPCCRD_DRIVER_BASE,
    STPCCRD_EVT_MOD_B_INSERTED,
    STPCCRD_EVT_MOD_A_REMOVED,
    STPCCRD_EVT_MOD_B_REMOVED

}STPCCRD_EventType_t;

/* Exported Types --------------------------------------------------- */

/*Enums****************************************************************/
/* Constants to specify the Name of the Module */
typedef enum
{
    STPCCRD_MOD_A,
    STPCCRD_MOD_B

}STPCCRD_ModName_t;

/* Constants to specify Transport Stream Processing */
typedef enum
{
    STPCCRD_ENABLE_TS_PROCESSING,
    STPCCRD_DISABLE_TS_PROCESSING

}STPCCRD_TSBypassMode_t;

/* Constants to specify Status Bit of Command/Status Register */
typedef enum
{
    STPCCRD_CI_DATA_AVAILABLE,
    STPCCRD_CI_FREE,
    STPCCRD_CI_READ_ERROR,
    STPCCRD_CI_WRITE_ERROR

}STPCCRD_Status_t;

/* Constants to specify Command Bit of Command/Status Register */
typedef enum
{
    STPCCRD_CI_RESET,
    STPCCRD_CI_SIZE_READ,
    STPCCRD_CI_SIZE_WRITE,
    STPCCRD_CI_HOST_CONTROL

} STPCCRD_Command_t;

/* Constants to specify Power Value */
typedef enum
{
    STPCCRD_POWER_5000,
    STPCCRD_POWER_3300

}STPCCRD_PowerMode_t;


/* Data Structure **************************************************************/

/* Identifier to PCCRD device */
typedef U32 STPCCRD_Handle_t;

/* Init Parameters */
typedef struct
{

#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5105) && !defined(ST_5301) \
&& !defined(ST_5107)
    ST_DeviceName_t         I2CDeviceName;      /* I2C bus device name */
#endif
    U32                     DeviceCode;         /* I2C bus address/EPLD address */
    U32                     MaxHandle ;         /* max modules on this controller */
    ST_Partition_t          *DriverPartition_p; /* memory partition */
    /*U32                   *BaseAddress_p;*/
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    ST_DeviceName_t         EvtHandlerName;     /* EVT Handler name */
#ifdef ST_OS21
    interrupt_name_t        InterruptNumber;
#else
    U32                     InterruptNumber;
#endif
    U32                     InterruptLevel;     /* Interrupt Level  */
#endif /*STPCCRD_HOT_PLUGIN_ENABLED*/

}STPCCRD_InitParams_t;

/* Open Parameters */
typedef struct{

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    void (* NotifyFunction)(STPCCRD_Handle_t Handle,
                            STPCCRD_EventType_t Event,
                            ST_ErrorCode_t ErrorCode);
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    STPCCRD_ModName_t ModName;

}STPCCRD_OpenParams_t;

/* Term Parameters */
typedef struct
{

    BOOL                    ForceTerminate;

} STPCCRD_TermParams_t;


/* Exported Functions ----------------------------------------------- */
ST_ErrorCode_t STPCCRD_Init( const ST_DeviceName_t          DeviceName,     /* Name for PCCRD device */
                             const STPCCRD_InitParams_t     *InitParams_p );/* pointer to Init parameters */

ST_ErrorCode_t STPCCRD_Term( const ST_DeviceName_t          DeviceName,     /* Name for PCCRD device */
                             const STPCCRD_TermParams_t     *TermParams_p );/* ForceTerminate */

ST_ErrorCode_t STPCCRD_Open( const ST_DeviceName_t          DeviceName,     /* Name for PCCRD device */
                             const STPCCRD_OpenParams_t     *OpenParams_p,  /* pointer to Open Parameters */
                             STPCCRD_Handle_t               *Handle_p );    /* returned handle */

ST_ErrorCode_t STPCCRD_Close( STPCCRD_Handle_t    Handle );                 /* device identifier to be closed */

ST_ErrorCode_t STPCCRD_Read( STPCCRD_Handle_t     Handle,                   /* device identifier */
                             U8                   *Buffer_p,                /* Destination buffer head */
                             U16                  *NumberReadOK_p );        /* no. successfully read */

ST_ErrorCode_t STPCCRD_Write( STPCCRD_Handle_t    Handle,                   /* device identifier */
                              U8                  *Buffer_p,                /* Source buffer head */
                              U16                 NumberToWrite,            /* number of bytes to write */
                              U16                 *NumberWrittenOK_p );     /* no. successfully written */

ST_ErrorCode_t STPCCRD_GetStatus ( STPCCRD_Handle_t       Handle,    /* Device identifier */
                                   STPCCRD_Status_t       Status,    /* Name of the Status bit */
                                   U8                     *Value_p); /* retruned status 0 or 1*/

ST_ErrorCode_t STPCCRD_SetStatus ( STPCCRD_Handle_t       Handle,    /* Device identifier */
                                   STPCCRD_Command_t      command,   /* Name of the Command bit */
                                   U8                     Value);    /* retruned status 0 or 1*/

ST_ErrorCode_t STPCCRD_CIReset( STPCCRD_Handle_t Handle);            /* Device identifier */

ST_Revision_t STPCCRD_GetRevision( void );                           /* version string */

ST_ErrorCode_t STPCCRD_ModReset(STPCCRD_Handle_t Handle);            /* Device identifier */

ST_ErrorCode_t STPCCRD_ModCheck(STPCCRD_Handle_t Handle);            /* Device identifier */

ST_ErrorCode_t STPCCRD_ModPowerOn( STPCCRD_Handle_t       Handle,    /* Device identifier */
                                   STPCCRD_PowerMode_t    Voltage);  /* Voltage value */

ST_ErrorCode_t STPCCRD_ModPowerOff(STPCCRD_Handle_t       Handle);   /* Device identifier */

ST_ErrorCode_t STPCCRD_ControlTS( STPCCRD_Handle_t        Handle,    /* Device identifier */
                                  STPCCRD_TSBypassMode_t  Control);  /* TS Mode to be set */

ST_ErrorCode_t STPCCRD_WriteCOR( STPCCRD_Handle_t Handle);          /* Device identifier */

ST_ErrorCode_t STPCCRD_CheckCIS( STPCCRD_Handle_t Handle);           /* Device identifier */

#ifdef __cplusplus
}
#endif

#endif   /* #ifndef __STPCCRD_H */
