/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embxlb_factory.c                                          */
/*                                                                 */
/* Description:                                                    */
/*    EMBX Inprocess loopback transport factory                    */
/*                                                                 */
/*******************************************************************/

#include "embx_osheaders.h"
#include "embx_osinterface.h"
#include "embxP.h"

#include "embxlb.h"
#include "embxlbP.h"

EMBX_Transport_t *EMBXLB_loopback_factory(void *param)
{
EMBXLB_Config_t *config = (EMBXLB_Config_t *)param;
EMBXLB_Transport_t *tp;

    if(config->name == 0)
        return 0;

    if(strlen(config->name) > EMBX_MAX_TRANSPORT_NAME)
        return 0;


    /* The transport structure _must_ be allocated from OS
     * memory. It is managed from now on by the framework.
     */
    tp = (EMBXLB_Transport_t *)EMBX_OS_MemAlloc(sizeof(EMBXLB_Transport_t));
    if(tp == 0)
        return 0;


    /* Decide which type of initialization to do. 
     * PLEASE NOTE: This is for the purposes of testing
     * the behaviour of the framework ONLY, a more realistic
     * transport implementation would either block, or not,
     * depending on the nature of its initialization
     * requirements. 
     */
    if(config->blockInit)
    {
        memcpy(tp,&_embxlb_blocktransport_template,sizeof(EMBXLB_Transport_t));
    }
    else
    {
        memcpy(tp,&_embxlb_transport_template,sizeof(EMBXLB_Transport_t));
    }


    /* Limiting the number of alocations is for testing the framework
     * error handling only. A more realistic transport would not do this.
     */
    tp->maxAllocations = config->maxAllocations; /* 0 = no limit */

    /* A more realistic transport would likely support only one of the
     * connection semantics. This configuration is for the purposes of
     * testing the framework.
     */
    tp->transport.transportInfo.allowsMultipleConnections = config->multiConnect;

    tp->transport.transportInfo.maxPorts = config->maxPorts; /* 0 = no limit */

    strcpy(tp->transport.transportInfo.name,config->name);


    if(config->maxObjects > 0)
    {
        if(!EMBX_HandleManager_SetSize(&tp->objectHandleManager, config->maxObjects))
        {
            EMBX_OS_MemFree(tp);
            return 0;
        }
    }
    else
    {
        EMBX_HandleManager_SetSize(&tp->objectHandleManager, EMBX_HANDLE_DEFAULT_TABLE_SIZE);
    }

    return &tp->transport;
}
