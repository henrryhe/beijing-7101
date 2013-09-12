/*******************************************************************************

File name   : 

Description :

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
 Nov 2003            Created                                      Michel Bruant

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include "stddefs.h"
#include "stsys.h"
#include "stcommon.h"
#include "stevt.h"
#include "stvtg.h"
#include "clevt.h"

/* Constants ---------------------------------------------------------------- */

#define TASK_STACK              5000

/* Variables ---------------------------------------------------------------- */

static U8                   Stack[TASK_STACK];
static task_t               Task;
static tdesc_t              Desc;

/* Functions ---------------------------------------------------------------- */

int STTBX_WaitMicroseconds()
{
    printf("STTBX_WaitMicroseconds\n");
    return(0);
}

/* -------------------------------------------------------------------------- */

int STCLKRV_Close()
{
    return(0);
}

/* -------------------------------------------------------------------------- */

int STCLKRV_GetExtendedSTC()
{
    printf("STCLKRV_GetExtendedSTC\n");
    return(0);
}

/* -------------------------------------------------------------------------- */

int STCLKRV_Open()
{
    return(0);
}

/* -------------------------------------------------------------------------- */

void trace(const char *format,...)
{
    printf(format);
}

/* -------------------------------------------------------------------------- */

static void SimuVTG(void)
{
    ST_ErrorCode_t ErrorCode;
    STEVT_OpenParams_t STEVT_OpenParams;
    STEVT_Handle_t      EventsHandle;
    STEVT_EventID_t     EVTId;
    U32                 Top,Bot;

    Top = (U32)STVTG_TOP;
    Bot = (U32)STVTG_BOTTOM;

    ErrorCode = STEVT_Open(STEVT_DEVICE_NAME,
                        &STEVT_OpenParams,
                        &EventsHandle);
    
    ErrorCode = STEVT_RegisterDeviceEvent(EventsHandle,
                                "SIMUVTG",
                                STVTG_VSYNC_EVT,
                                &EVTId);
   while(1)
   {
      ErrorCode = STEVT_Notify( EventsHandle,EVTId, STEVT_EVENT_DATA_TYPE_CAST (&Top));
      task_delay(ST_GetClocksPerSecond() * 20 / 1000);
      ErrorCode = STEVT_Notify( EventsHandle,EVTId, STEVT_EVENT_DATA_TYPE_CAST (&Bot));
      task_delay(ST_GetClocksPerSecond() * 20 / 1000);
   }
}

/* -------------------------------------------------------------------------- */
void Init_Stub(void)
{
   ST_ErrorCode_t                          Error;

   Error =task_init((void(*)(void*))SimuVTG,NULL,Stack,
            TASK_STACK, &Task, &Desc,5,
            "SimuVTG",0);
}
/* -------------------------------------------------------------------------- */
