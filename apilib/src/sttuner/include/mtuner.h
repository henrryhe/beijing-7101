
/* ----------------------------------------------------------------------------
File Name: mtuner.h

Description: 

    tuenr driver,instance and API structure.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 6-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: 
    
   date: 17-August-2001
version: 3.1.1
 author: GJP
comment: update SubAddr to U16

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_MTUNER_H
#define __STTUNER_MTUNER_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */
#include "sttuner.h"
#include "ioarch.h"


/* -------------------------------------------------------------------------
    STTUNER_tuner_dbase_t
   ------------------------------------------------------------------------- */
/* This structure added to return the tuner frequency range capability,DDTS GNBvd09094 */
	typedef struct
	{
		U32 FreqMin;
		U32 FreqMax;
	}
	TUNER_Capability_t;

    /* support type: TUNER status */
    typedef struct
    {
        U32 Frequency;
        U32 TunerStep;
        U32 IntermediateFrequency;
        U32 Bandwidth;
        S32 IQSense;
        U32 Reference;
    } TUNER_Status_t;
    
    
    
    /* support type: TUNER initialization parameters */
    typedef struct
    {
        IOARCH_Handle_t     IOHandle;          /* already open handle for STTUNER_IOARCH_ReadWrite I/O */
        ST_Partition_t     *MemoryPartition;   /* for first init of driver collection */
        STTUNER_TunerType_t TunerType;
         U32           ExternalClock;            /* External VCO */
    } TUNER_InitParams_t;
    
    
    /* support type: TUNER termination parameters */
    typedef struct
    {
        BOOL ForceTerminate;        /* force driver termination (don't wait) */
    } TUNER_TermParams_t;
    
    
    /* support type: TUNER open parameters */
    typedef struct
    {
        STTUNER_Handle_t    TopLevelHandle;
    } TUNER_OpenParams_t;
    
    
    /* support type: TUNER close parameters */
    typedef struct
    {
        STTUNER_Da_Status_t Status;
    } TUNER_CloseParams_t;


    typedef U32 TUNER_Handle_t; /* handle returned by tuner driver open */


    /* we have one of these for every TYPE of driver in the system */
    typedef struct
    {

        STTUNER_TunerType_t ID;  /* enumerated hardware ID */
        
        /* ---------- API to tuner ---------- */
        ST_ErrorCode_t (*tuner_Init)(ST_DeviceName_t *DeviceName,TUNER_InitParams_t *InitParams);
        ST_ErrorCode_t (*tuner_Term)(ST_DeviceName_t *DeviceName,TUNER_TermParams_t *TermParams);

        ST_ErrorCode_t (*tuner_Open) (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t  *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
        ST_ErrorCode_t (*tuner_Close)(TUNER_Handle_t  Handle, TUNER_CloseParams_t *CloseParams);
        
        /* repeater/passthrough port for other drivers to use, type: STTUNER_IOARCH_RedirFn_t */
        ST_ErrorCode_t (*tuner_ioaccess)(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

        /* access device specific low-level functions */
        ST_ErrorCode_t (*tuner_ioctl)(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);

        ST_ErrorCode_t (*tuner_SetFrequency) (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency);
        ST_ErrorCode_t (*tuner_GetStatus)    (TUNER_Handle_t Handle, TUNER_Status_t *Status);
        ST_ErrorCode_t (*tuner_IsTunerLocked)(TUNER_Handle_t Handle, BOOL *Locked);
        ST_ErrorCode_t (*tuner_SetBandWidth) (TUNER_Handle_t Handle, U32 BandWidth, U32 *NewBandWidth);

        /* new for 3.2.0 */
        ST_ErrorCode_t (*tuner_GetFrequency) (TUNER_Handle_t Handle, U32 *CurrentFrequency);
        void           (*tuner_GetProperties)(TUNER_Handle_t Handle, void *PropertiesBlock);
        void           (*tuner_Select )      (TUNER_Handle_t Handle, STTUNER_TunerType_t type, unsigned char address);
    } STTUNER_tuner_dbase_t;


    /* we have one of these for every INSTANCE of driver in the system */
    typedef struct
    {
        ST_ErrorCode_t          Error;      /* last reported driver error  */
        STTUNER_tuner_dbase_t  *Driver;     /* which driver                */    
        IOARCH_Handle_t         IOHandle;   /* IO handle                   */
        TUNER_Handle_t          DrvHandle;  /* handle to instance of driver*/
        U32 realfrequency;
    } STTUNER_tuner_instance_t;
 

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_MTUNER_H */


/* End of mtuner.h */
