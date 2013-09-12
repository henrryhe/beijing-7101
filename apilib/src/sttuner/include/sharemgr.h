
/* ----------------------------------------------------------------------------
File Name: sharemgr.h

Description: 

    Data structures for managing shared devices.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 24-July-2001
version: 3.1.0
 author: GJP
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SHARE_SHAREMGR_H
#define __STTUNER_SHARE_SHAREMGR_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

#include "mdemod.h"
#include "mtuner.h"

/* -------------------------------------------------------------------------
  ------------------------------------------------------------------------- */

    typedef struct
    {
        /* pointers to the start of a list of device driver i/o functions & 
          data common between instances */

        STTUNER_demod_dbase_t *DemodDbase;     /* DemodDbase[n-1] */
        U32                    DemodDbaseSize; /* n, number of TYPES of driver */

        STTUNER_tuner_dbase_t *TunerDbase;
        U32                    TunerDbaseSize;

    } STTUNER_SHARE_DriverDbase_t;


/* -------------------------------------------------------------------------
   ------------------------------------------------------------------------- */

    typedef struct
    {
        U32 PlaceHolder;
    } STTUNER_SHARE_InstanceDbase_t;


/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SHARE_SHAREMGR_H */


/* End of sharemgr.h */
