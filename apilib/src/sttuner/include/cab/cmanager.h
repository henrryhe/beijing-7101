
/* ----------------------------------------------------------------------------
File Name: cmanager.h

Description: 

    Data structures for managing cable devices.

Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:
    
Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CAB_CMANAGER_H
#define __STTUNER_CAB_CMANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */
#include "sttuner.h"
#include "ioarch.h"
    
#include "dastatus.h"

/* driver data structures*/
#include "mdemod.h"
#include "mtuner.h"

/* top level scantask */
#include "cabtask.h"


/* -------------------------------------------------------------------------
    STTUNER_CAB_DriverDbase_t (element of STTUNER_DriverDbase_t)
    
     This database holds lists (accessable as an array) of cable
    device driver types. There are four types of driver for cable of which
    two are common to all devices (services) so there are the other two lists
     whose size is found at run time by scanning included drivers.
     
  ------------------------------------------------------------------------- */

    typedef struct
    {
        /* pointers to the start of a list of device driver i/o functions & 
          data common between instances */

        STTUNER_demod_dbase_t *DemodDbase;     /* DemodDbase[n-1] */
        U32                    DemodDbaseSize; /* n, number of TYPES of driver */

        STTUNER_tuner_dbase_t *TunerDbase;
        U32                    TunerDbaseSize;

    }
    STTUNER_CABLE_DriverDbase_t;


/* -------------------------------------------------------------------------
    STTUNER_CAB_InstanceDbase_t (element of STTUNER_InstanceDbase_t)
    
     This database holds data blocks of the required driver, there are
    four subcomponents to each complete cable driver, even if not all
    are then used or required to complete a driver. The size of each list depends
    on Init() time user parameters (how many of each type of driver of each
    type were declared)
   ------------------------------------------------------------------------- */

    typedef struct
    {
        /* drivers used */
        STTUNER_demod_instance_t    Demod;
        STTUNER_tuner_instance_t    Tuner;

        CABTASK_ScanTask_t      ScanTask;
        CABTASK_ScanInfo_t      ScanInfo;

        DEMOD_Capability_t      Demod_Capability;
        STTUNER_Modulation_t    Modulation;

        U32                     ExternalClock;
        U32                     LastThreshold;
        U32                     LastSignal;
        U32                     Timeout;
        BOOL                    ScanExact;
        U32                     SymbolWidthMin;

        STTUNER_Scan_t          SingleScan;
        
        BOOL                    EnableReLocking;
    }
    STTUNER_CABLE_InstanceDbase_t;


/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_CMANAGER_H */


/* End of cmanager.h */
