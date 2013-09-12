/* ----------------------------------------------------------------------------
File Name: tmanager.h

Description:

    Data structures for managing terrestrial devices.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 10-Sept-2001
version: 3.1.0
 author: GB from work by GJP
comment:

   date: 27-Dec-2004
version: 5.0.0
 author: CC
comment: SymbolWidthMin field from structure STTUNER_TERR_InstanceDbase_t is removed .

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_TER_SMANAGER_H
#define __STTUNER_TER_SMANAGER_H

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
#include "tertask.h"


/* -------------------------------------------------------------------------
    STTUNER_TERR_DriverDbase_t (element of STTUNER_DriverDbase_t)

     This database holds lists (accessable as an array) of terrestrial
    device driver types. There are x types of driver for ter of which
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

    } STTUNER_TERR_DriverDbase_t;


/* -------------------------------------------------------------------------
    STTUNER_TERR_InstanceDbase_t (element of STTUNER_InstanceDbase_t)

     This database holds data blocks of the required driver, there are
    x subcomponents to each complete terrestrial driver, even if not all
    are then used or required to complete a driver. The size of each list depends
    on Init() time user parameters (how many of each type of driver of each
    type were declared)
   ------------------------------------------------------------------------- */

    typedef struct
    {
        /* drivers used */
        STTUNER_demod_instance_t    Demod;
        STTUNER_tuner_instance_t    Tuner;

        /* ter specifics */
        TERTASK_ScanTask_t      ScanTask;
        TERTASK_ScanInfo_t      ScanInfo;

        DEMOD_Capability_t      Demod_Capability;

        U32                     ExternalClock;
        U32                     LastThreshold;
        U32                     LastSignal;
        U32                     Timeout;
        BOOL                    ScanExact;
        U32                     FECRates;

        STTUNER_Scan_t          SingleScan;

        BOOL EnableReLocking;
    } STTUNER_TERR_InstanceDbase_t;


/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_TER_SMANAGER_H */


/* End of tmanager.h */
