/*****************************************************************************
File Name   : app_data.h

Description : Global Application header

Copyright (C) 1999 STMicroelectronics

Reference   :

*****************************************************************************/

#ifndef __APP_DATA_H
#define __APP_DATA_H

#include "stddefs.h"  /* standard definitions                    */
#if !defined(ST_OSLINUX)
#include "stboot.h"
#ifndef STEVT_NO_TBX
#include "sttbx.h"
#endif
#endif

/*=========================================================================
    Global Initialisation
=========================================================================== */

#ifdef GLOBAL_DATA
#define DATA

    /* List of device names */
    ST_DeviceName_t         TBXDeviceName     = "TBX0";
    ST_DeviceName_t         BOOTDeviceName    = "BOOT0";

#else
#define DATA    extern

#endif

/*=========================================================================
    Global definitions
=========================================================================== */

/*=========================================================================
    Global Variables
=========================================================================== */

/*=========================================================================
    Global function declarations
=========================================================================== */
/* initterm.c */
extern ST_ErrorCode_t   GlobalInit( void );
extern ST_ErrorCode_t   GlobalOpen( void );
extern ST_ErrorCode_t   GlobalTerm( void );
extern void  GlobalGetRevision( void );

#endif
