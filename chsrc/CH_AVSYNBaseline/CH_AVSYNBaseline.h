/*****************************************************************************
File Name   : inject_swts.h

Description : SWTS header file.

History     :

Copyright (C) 2008 changhong

Reference   :
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __CHAVSYNBASLIN_H
#define __CHAVSYNBASLIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */
//#include "stmergetst.h"
#include "stlite.h"
#include "stdevice.h"
#include "stevt.h"
#include "stmerge.h"
#include "stclkrv.h"
//#include "stmergeerr.h"

#include "sttbx.h"
#include "testtool.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern  STCLKRV_Handle_t gst_CLKRVHandle[];
extern ST_DeviceName_t VIDDeviceName[];


extern void CH_RegistBaseLINESYNEvent(int ri_VidHandleIndex);
extern void  CH_UnRegistBaseLINESYNEvent(int ri_VidHandleIndex);
extern void CH_ResetVID_AYSYNCallback(int ri_vidpid,int i_audpid);


#ifdef __cplusplus
}
#endif

#endif
