/*******************************************************************************

File name   : osd_task.c

Description : OSD Top Bottom task

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----
2001/03/09          Creation                                Michel Bruant

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */


#include "stddefs.h"
#include "stvtg.h"
#include "osd_task.h"
#include "osd_hal.h"
#include <stdlib.h>
#include <string.h>


/* Functions ---------------------------------------------------------------- */

static void stosd_UpdateBottomHeaders(void);
static void stosd_UpdateTopHeaders(void);

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
    semaphore_signal(stlayer_osd_context.stosd_VSync_p);
}
/*******************************************************************************
Name        : stlayer_SubscribeEvents
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
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

/*******************************************************************************
Name        : stlayer_UpdateHeadersTask
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void stlayer_UpdateHeadersTask(void)
{

    while (!stlayer_osd_context.DriverTermination)
    {
        semaphore_wait(stlayer_osd_context.stosd_VSync_p);
        if (stlayer_osd_context.stosd_ActiveTopNotBottom)
        {
            /* VSync Top -> Update Bottom */
            semaphore_wait(stlayer_osd_context.ContextAcess_p);
            stosd_UpdateBottomHeaders();
            semaphore_signal(stlayer_osd_context.ContextAcess_p);
        }
        else
        {
            /* VSync Bottom -> Update Top */
            semaphore_wait(stlayer_osd_context.ContextAcess_p);
            stosd_UpdateTopHeaders();
            semaphore_signal(stlayer_osd_context.ContextAcess_p);
        }
    }
}
/*******************************************************************************
Name        : stosd_UpdateBottomHeaders
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void stosd_UpdateBottomHeaders(void)
{
    stosd_ViewportDesc *    ViewPort_p;

    ViewPort_p = stlayer_osd_context.FirstViewPortEnabled;
    while(ViewPort_p != NULL) /* Scan the list of enabled VP */
    {
        if (ViewPort_p->BotHeaderUpdate)
        {
            ViewPort_p->BotHeaderUpdate = FALSE;
            stlayer_HardUpdateViewportList(ViewPort_p,/*VsyncIsTop=*/TRUE);
            stlayer_HardEnable(ViewPort_p,/*VsyncIsTop=*/TRUE);
        }
        ViewPort_p = ViewPort_p->Next_p;
    }
}
/*******************************************************************************
Name        : stosd_UpdateTopHeaders
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void stosd_UpdateTopHeaders(void)
{
    stosd_ViewportDesc *    ViewPort_p;

    ViewPort_p = stlayer_osd_context.FirstViewPortEnabled;
    while(ViewPort_p != NULL) /* Scan the list of enabled VP */
    {
        if (ViewPort_p->TopHeaderUpdate)
        {
            ViewPort_p->TopHeaderUpdate = FALSE;
            stlayer_HardUpdateViewportList(ViewPort_p,/*VsyncIsTop=*/FALSE);
            stlayer_HardEnable(ViewPort_p,/*VsyncIsTop=*/FALSE);
        }
        ViewPort_p = ViewPort_p->Next_p;
    }
}

/* end of osd_task.c */
