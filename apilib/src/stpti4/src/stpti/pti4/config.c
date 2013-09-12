/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: basic.c
 Description: Provides minimum required interface to PTI transport controller

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#if !defined( ST_OSLINUX )
#include <assert.h>
#include <string.h>
#include <stdio.h>
#endif /* #endif !defined( ST_OSLINUX ) */

#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "stpti.h"

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"

#include "memget.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"


/* Private Function Prototypes --------------------------------------------- */


/* Functions --------------------------------------------------------------- */


/* -----------------------------------------------------------------------------
    Function Name: stptiHAL_LoaderGetConfig
      Description: wrapper for access to TC specific values
       Parameters:
----------------------------------------------------------------------------- */
U32 stptiHAL_LoaderGetConfig(FullHandle_t FullHandle, stptiHAL_LoaderConfig_t LoaderConfig)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32 Value = 0;


    switch( LoaderConfig )
    {
        case SIGNALEVERYTRANSPORTPACKET: 
            Value = TC_Params_p->TC_SignalEveryTransportPacket;
            break;

        default:
            break;
    }

    return( Value );
}

/* EOF */

