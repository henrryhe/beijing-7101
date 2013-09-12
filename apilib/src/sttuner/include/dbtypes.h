/* ----------------------------------------------------------------------------
File Name: dbtypes.h

Description: 

    Instance and driver database top level structure.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 6-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_DBTYPES_H
#define __STTUNER_DBTYPES_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */
#include "stevt.h"
#include "sttuner.h"
    
#include "ioarch.h"
#include "dastatus.h" 

/* shared devices */
#ifdef STTUNER_USE_SHARED
 #include "sharemgr.h"
#endif

/* sat devices */
#ifdef STTUNER_USE_SAT
 #include "smanager.h"
#endif

/* cab devices */
#ifdef STTUNER_USE_CAB
 #include "cmanager.h"
#endif

/* ter devices */
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
 #include "tmanager.h"
#endif

/* -------------------------------------------------------------------------
    STTUNER_IoctlState_t (element of STTUNER_InstanceDbase_t)
    
    This structure holds all the common ioctl state.
    ------------------------------------------------------------------------- */
    typedef struct
    {
        U32 PlaceHolder;    /* nothing yet */
    } STTUNER_IoctlState_t;


/* -------------------------------------------------------------------------
    STTUNER_DeviceDbase_t
    
     This holds data and function pointers to every driver in the system
    catagorized by transmission medium (sat, cable, terr etc.)
   ------------------------------------------------------------------------- */
    typedef struct
    {
        U32 PlaceHolder;

        /* (shared drivers) */
#ifdef STTUNER_USE_SHARED
        STTUNER_SHARE_DriverDbase_t Share;      /* common drivers database */
#endif

        /* (sat) */
#ifdef STTUNER_USE_SAT
        STTUNER_SAT_DriverDbase_t   Sat;        /* satellite specific driver database   */
#endif

        /* (cable) */
#ifdef STTUNER_USE_CAB
        STTUNER_CABLE_DriverDbase_t Cable;      /* cable specific driver database       */
#endif

        /* (terrestrial) */
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
        STTUNER_TERR_DriverDbase_t  Terr;       /* terrestrial specific driver database */
#endif

    } STTUNER_DriverDbase_t;


/* -------------------------------------------------------------------------
    STTUNER_InstanceDbase_t
    
     Database of instances of driver, there should be STTUNER_MAX_HANDLES
    number of elements of this (one for every STTUNER_Handle_t handle)
    Each XYZ__InstanceDbase_t points to an array of instances of
    STTUNER_MAX_HANDLES size (to be done at the Init() of each media type)
   ------------------------------------------------------------------------- */
    typedef struct
    {

        /* (common) */
        STTUNER_Device_t        Device;     /* either sat, cable, terr, etc. */
        STTUNER_Da_Status_t     Status;     /* manager level handle status */
        char                    Name[ST_MAX_DEVICE_NAME]; /* of instance */

        /* lists */
        U32                     Max_BandList;
        U32                     Max_ThresholdList;
        U32                     Max_ScanList;
        
       STTUNER_BandList_t      BandList;
        STTUNER_ThresholdList_t ThresholdList;
        STTUNER_ScanList_t      ScanList;
        U32                    *ThresholdHits; 

        /* partition for dynamic memory allocation */
        ST_Partition_t         *DriverPartition; 
        
        /* _External_ init()'d STEVT instance to use when STEVT_Open() for an instance */
        ST_DeviceName_t         EVT_DeviceName;

        /* STEVT_Open() parameters and returns */
        STEVT_Handle_t          EVTHandle;
        char                    EVT_RegistrantName[ST_MAX_DEVICE_NAME]; /* event open() param */
        STEVT_EventID_t         EvtId[STTUNER_NUMBER_EVENTS];
        
        /* event callback function */
        void (*NotifyFunction)(STTUNER_Handle_t Handle, STTUNER_EventType_t EventType, ST_ErrorCode_t Error);

        /* API Data */
        STTUNER_Capability_t    Capability;     /* record into this at every Open() */
        STTUNER_TunerInfo_t     TunerInfo;
        STTUNER_TunerInfo_t     CurrentTunerInfo;
        volatile BOOL  criticaldataflag;
        STTUNER_QualityFormat_t QualityFormat;

#ifdef STTUNER_USE_SHARED
        /* n/a -  no manager for shared device-drivers. */
#endif

        /* (sat) */
#ifdef STTUNER_USE_SAT
        STTUNER_SAT_InstanceDbase_t   Sat;      /* satellite specific instance data (drivers used in this instance) */
#endif
        
        /* (cable) */
#ifdef STTUNER_USE_CAB
        STTUNER_CABLE_InstanceDbase_t Cable;    /* cable specific instance data */
#endif
        
        /* (terrestrial) */
#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
        STTUNER_TERR_InstanceDbase_t  Terr;     /* terrestrial specific instance data */
#endif

        /* instance ioctl state (common to all managers) */
        STTUNER_IoctlState_t Ioctl;
        BOOL                 ForceSearchTerminate; /* To allow us to stop the search in the middle of a scan */
        ST_ErrorCode_t TunerInfoError;
       
        /**** Standby Mode specific variables*****/
        U32 Task_Run ;/**** Control suspeneded task doesn't suspend
                            again . Similarly for task resume***/
                     
        BOOL 	TRL_IOCTL_Set_Flag ;	/*this flag is used for TRL register settings from application*/
        BOOL    IOCTL_Set_30MZ_REG_Flag ;    /*This flag is used for register setting from application for 30MHZ crystal */         
        STTUNER_StandByMode_t PowerMode,CurrentPowerMode;
        STTUNER_Status_t   PreStatus,CurrentPreStatus;     
    } STTUNER_InstanceDbase_t;
                      
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_DBTYPES_H */


/* End of dbtypes.h */
