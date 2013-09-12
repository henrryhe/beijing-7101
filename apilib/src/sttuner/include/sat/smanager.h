
/* ----------------------------------------------------------------------------
File Name: smanager.h

Description: 

    Data structures for managing satellite devices.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 6-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: 

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_SMANAGER_H
#define __STTUNER_SAT_SMANAGER_H

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
#include "mlnb.h"

#include "mscr.h"

#include "mdiseqc.h"

/* top level scantask */
#include "sattask.h"

/* ------------------------------------------------------------------------
    STTUNER_SAT_ioctl_t (element of STTUNER_SAT_InstanceDbase_t)
    
     This structure holds all the satellite manager layer ioctl state.
  ------------------------------------------------------------------------- */

    typedef struct
    {
        BOOL SignalNoise;    /* unitary test, turn on random BER noise */
        U32  TunerStepSize;  /* fixed tuner step size (TunerStepSize > 0) or (default) calculated (TunerStepSize = 0) */
    }
    STTUNER_SAT_IoctlState_t;


/* -------------------------------------------------------------------------
    STTUNER_SAT_DriverDbase_t (element of STTUNER_DriverDbase_t)
    
     This database holds lists (accessable as an array) of satellite
    device driver types. There are three types of driver for sat of which
    two are common to all devices (services) and one (LNB) exclusive to it.
  ------------------------------------------------------------------------- */

    typedef struct
    {
        /* pointers to the start of a list of device driver i/o functions & 
          data common between instances */

        STTUNER_demod_dbase_t *DemodDbase;     /* DemodDbase[n-1] */
        U32                    DemodDbaseSize; /* n, number of TYPES of driver */

        STTUNER_tuner_dbase_t *TunerDbase;
        U32                    TunerDbaseSize;

        STTUNER_lnb_dbase_t    *LnbDbase;
        U32                     LnbDbaseSize;
        
        STTUNER_scr_dbase_t    *ScrDbase;
        U32                     ScrDbaseSize;
		
        STTUNER_diseqc_dbase_t *DiseqcDbase;
        U32 			DiseqcDbaseSize;

    }
    STTUNER_SAT_DriverDbase_t;


/* -------------------------------------------------------------------------
    STTUNER_SAT_InstanceDbase_t (element of STTUNER_InstanceDbase_t)
    
     This database holds data blocks of the required driver, there are
    three subcomponents to each complete satellite driver, even if not all
    are then used or required to complete the required functionality
    required e.g. only external ZIF tuner is required.
   ------------------------------------------------------------------------- */

    typedef struct
    {
        /* drivers used */
        STTUNER_demod_instance_t    Demod;
        STTUNER_tuner_instance_t    Tuner;
        STTUNER_lnb_instance_t      Lnb;
        STTUNER_scr_instance_t      Scr;    
        STTUNER_diseqc_instance_t   Diseqc;
        

        /* sat specifics */
        SATTASK_ScanTask_t      ScanTask;
        SATTASK_ScanInfo_t      ScanInfo;

        LNB_Capability_t        Lnb_Capability;
        DEMOD_Capability_t      Demod_Capability;
        TUNER_Capability_t      Tuner_Capability;
        SCR_Capability_t        Scr_Capability;
        DISEQC_Capability_t	Diseqc_Capability;

        U32                     ExternalClock;
        U32                     LastThreshold;
        U32                     LastSignal;
        U32                     Timeout;
        BOOL                    ScanExact;
        BOOL                    BlindScan;
        U32                     SymbolWidthMin;
        U32                     PolarizationMask;
        U32                     FECRates;
		STTUNER_BlindScan_t     BlindScanInfo;
        STTUNER_Scan_t          SingleScan;
        
        BOOL EnableReLocking;
        
        /* Added to allow us to test the search exiting */
        BOOL EnableSearchTest;
        
        /* ioctl state & switches */
        STTUNER_SAT_IoctlState_t Ioctl;
		/* DiSEqC2.0 */
#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO
       STTUNER_DiSEqC2_Via_PIO_t       DiSEqC;  /* Structure that contains DiSEqC2 related fields*/
#endif 
	SATTASK_ScanTask_t      TimeoutTask; /* New task added for Timeout implementation in SetFrequency */

    }
    STTUNER_SAT_InstanceDbase_t;


/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_SMANAGER_H */


/* End of smanager.h */
