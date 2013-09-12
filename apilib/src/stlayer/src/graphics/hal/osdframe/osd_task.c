/*******************************************************************************

File name   : osd_task.c

Description :

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----
2001-01-11          Creation                                    Michel Bruant

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */


#include "stddefs.h"
#include "stvtg.h"
#include "osd_task.h"
#include "osd_cm.h"
#include "osd_hal.h"
#include "stevt.h"
#include <string.h>


/* Functions --------------------------------------------------------------- */


/*******************************************************************************
Name        : ReceiveEvtVSync
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/

static void ReceiveEvtVSync (STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                const void *EventData,
                                const void *SubscriberData_p )
{
    STVTG_VSYNC_t EventType;

    EventType = *((STVTG_VSYNC_t*)EventData);
    switch (EventType)
    {
        case STVTG_TOP:
            stlayer_osd_context.stosd_ActiveTopNotBottom = TRUE;
            break;

        case STVTG_BOTTOM:
            stlayer_osd_context.stosd_ActiveTopNotBottom = FALSE;
            break;

        default :
            break;
    }
    if ((stlayer_osd_context.UpdateTop)
            || (stlayer_osd_context.UpdateBottom))
    {
        semaphore_signal(stlayer_osd_context.stosd_VSync_p);
    }
}

/*******************************************************************************
Name        : ProcessEvtVSync
Description : OSD Task
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
void stlayer_ProcessEvtVSync (void)
{

    while (!stlayer_osd_context.DriverTermination)
    {
        /* Wait a synchro */
        semaphore_wait(stlayer_osd_context.stosd_VSync_p);

        if(stlayer_osd_context.stosd_ActiveTopNotBottom)
        {
            if (stlayer_osd_context.UpdateBottom)
            {
                /* VSync Top -> Update Bottom */
                semaphore_wait(stlayer_osd_context.VPDecriptorAccess_p);
                stlayer_HardUpdateViewportList(TRUE);
                stlayer_osd_context.UpdateBottom = FALSE;
                semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
            }
        }
        else
        {
            if (stlayer_osd_context.UpdateTop)
            {
                /* VSync Bottom -> Update Top */
                semaphore_wait(stlayer_osd_context.VPDecriptorAccess_p);
                stlayer_HardUpdateViewportList(FALSE);
                stlayer_osd_context.UpdateTop = FALSE;
                semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
            }
        }
    } /* end while */
    /* termination : update the viewports extinctions */
    semaphore_wait(stlayer_osd_context.VPDecriptorAccess_p);
    stlayer_HardUpdateViewportList(TRUE);
    stlayer_HardUpdateViewportList(FALSE);
    semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
}
/*******************************************************************************
Name        : SubscribeEvents
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t stlayer_SubscribeEvents(ST_DeviceName_t NewVTG)
{

    STEVT_DeviceSubscribeParams_t SubscribeParams;
    ST_DeviceName_t Empty = "\0\0";

    SubscribeParams.NotifyCallback     = ReceiveEvtVSync;

    if(strcmp(stlayer_osd_context.VTGName,Empty) != 0 )
    {
        /* A synchro was already in effect */
        /* -> No more synchro */
        STEVT_UnsubscribeDeviceEvent(stlayer_osd_context.EvtHandle,
                                     stlayer_osd_context.VTGName,
                                     STVTG_VSYNC_EVT);
    }
    if(NewVTG != NULL)
    {
        /* subscription to the event handler */
        SubscribeParams.NotifyCallback     = ReceiveEvtVSync;
        SubscribeParams.SubscriberData_p   = NULL;
        STEVT_SubscribeDeviceEvent(stlayer_osd_context.EvtHandle,
                                   NewVTG,
                                   STVTG_VSYNC_EVT,
                                   &SubscribeParams);
        strcpy(stlayer_osd_context.VTGName,NewVTG);
    }
    else
    {
        /* No new VTG */
        strcpy(stlayer_osd_context.VTGName,Empty);
    }

    return(ST_NO_ERROR);
}
/* end of osd_task.c */
