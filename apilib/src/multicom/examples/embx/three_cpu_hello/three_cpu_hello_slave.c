/*******************************************************************/ 
/* Copyright 2003 STMicroelectronics R&D Ltd. All rights reserved. */ 
/*                                                                 */ 
/* File: three_cpu_hello_slave.c                                   */
/*                                                                 */ 
/* Description:                                                    */ 
/*    ST220 transport slave for three cpu hello example            */
/*                                                                 */
/*******************************************************************/ 

/*
 * Standard OS21, ST220 and STm8000 board support headers
 */
#include <os21.h>
#include <os21/st200.h>
#include <os21/st200/stm8000_audioenc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Include the EMBX API, shared memory transport and
 * hardware mailbox headers.
 */
#include "embx.h"
#include "embxshm.h"
#include "embxmailbox.h"

/*
 * The ST220 core being used can be determined at runtime by
 * examining the peripheral base core register. 
 */
#define VIDEOENC_PBASE 0x14100000
#define AUDIOENC_PBASE 0x14200000
#define AUDIODEC_PBASE 0x18000000

static volatile unsigned long *pbasereg = (volatile unsigned long *)0xffffffb0;


EMBXSHM_MailboxConfig_t config = {
   "shm",             /* Local transport name                                */
   0,                 /* CPU id, determined at runtime                       */
   { 1, 1, 1, 0 },    /* The participants map must be the same on all CPUs   */
   0x00000000         /* No pointer warp as ST220 addresses are bus addresses*/
                      /* in this setup.                                      */
};


char *cpu_names[] = { "audioenc","audiodec" };

int init_mailboxes();
int init_comms(EMBX_FACTORY *pFact, EMBX_TRANSPORT *pTrans, EMBX_PORT *pPort);


int main(int argc, char **argv)
{
EMBX_FACTORY   hFact;
EMBX_TRANSPORT hTrans;
EMBX_PORT      myPort;
int status = 0;

    /*
     * Start OS21
     */
    kernel_initialize(NULL);
    kernel_start();
    kernel_timeslice(OS21_TRUE);

    printf("three_cpu_hello started\n");

    /*
     * Determine which ST220 we are running on
     */
    switch(*pbasereg)
    {
        case VIDEOENC_PBASE:
            printf("This example does not run on the video encoder\n");
            return 1;
        case AUDIOENC_PBASE:
            config.cpuID = 1;
            break;
        case AUDIODEC_PBASE:
            config.cpuID = 2;
            break;
    }

    if(init_mailboxes() < 0)
    {
        status = 1;
        goto exit;
    }

    if(init_comms(&hFact, &hTrans, &myPort) < 0)
    {
        status = 1;
        goto exit;
    }

    if(do_work(hTrans, myPort) < 0)
        status = 1;


exit:
    /*
     * Add brief delay here to give other CPUs a chance to
     * do stuff before calling EMBX_Deinit, which will 
     * invalidate the transport for everyone.
     */
    task_delay(time_ticks_per_sec());

    /*
     * Close everything down. In this case it doesn't matter
     * if it fails as there is nothing we can do anyway.
     */
    EMBX_ClosePort(myPort);
    EMBX_CloseTransport(hTrans);
    EMBX_Deinit();
    EMBX_UnregisterTransport(hFact);

    return status;
}



int init_mailboxes(void)
{
EMBX_ERROR err;

    if((err = EMBX_Mailbox_Init()) != EMBX_SUCCESS)
    {
        printf("Failed to intialise mailbox library err = %d\n", err);
        return -1;
    }

    /*
     * Register the first hardware mailbox on the STm8000. This will be
     * used to signal the ST40
     */
    if((err = EMBX_Mailbox_Register((void *)0x10200000, -1, -1, 0)) != EMBX_SUCCESS)
    {
        printf("Failed to register mbox 0 err = %d\n",err);
        return -1;
    }


    /*
     * Register the second hardware mailbox on the STm8000. The audio encoder
     * will own the first half of the register set and the audio decoder will
     * own the second half of the register set. The interrupt level parameter
     * is ignored when using OS21.
     */ 
    err = EMBX_Mailbox_Register((void *)0x10210000,
            (config.cpuID==1)?OS21_INTERRUPT_MBOX2_0:OS21_INTERRUPT_MBOX2_1,
            0,
            (config.cpuID==1)?EMBX_MAILBOX_FLAGS_SET1:EMBX_MAILBOX_FLAGS_SET2);

    if(err != EMBX_SUCCESS)
    {
        printf("Failed to register mbox 1 err = %d\n",err);
        return -1;
    }

    printf("Initialized mailbox library\n");

    return 0;
}



int init_comms(EMBX_FACTORY *pFact, EMBX_TRANSPORT *pTrans, EMBX_PORT *pPort)
{
EMBX_ERROR err;
EMBX_TPINFO tpinfo;

    /*
     * Register the transport factory for the shared memory
     * transport configuration we require.
     */
    err = EMBX_RegisterTransport(EMBXSHM_mailbox_factory,
                                 &config, sizeof(config), pFact);

    if(err != EMBX_SUCCESS)
    {
        printf("Factory registration failed err = %d\n",err);
        return -1;
    }

    /*
     * Initialise the EMBX library, this will attempt to create
     * a transport based on the factory we just registered.
     */
    if((err = EMBX_Init()) != EMBX_SUCCESS)
    {
        printf("EMBX initialisation failed err = %d\n",err);
        return -1;
    }

    /*
     * Try and get the information structure for the transport
     * we hope has just been created. If this returns an error
     * no transports were created during EMBX_Init, which means
     * something went wrong in the transport factory we registered.
     */
    if((err = EMBX_GetFirstTransport(&tpinfo)) != EMBX_SUCCESS)
    {
        printf("Transport Factory failed err = %d\n",err);
        return -1;
    }

    /*
     * Open a transport handle for the transport we just queried.
     * As we only registered a single transport we know this is
     * the one we want to use.
     */
    if((err = EMBX_OpenTransport(tpinfo.name, pTrans)) != EMBX_SUCCESS)
    {
        printf("Failed to open transport '%s' err = %d\n",tpinfo.name,err);
        return -1;
    }

    printf("Successfully registered and opened transport '%s'\n",tpinfo.name);

    if((err = EMBX_CreatePort(*pTrans, cpu_names[config.cpuID-1], pPort)) != EMBX_SUCCESS)
    {
        printf("Failed to create port, err = %d\n",err);
        return -1;
    }

    printf("Successfully opened transport and created port\n");

    return 0;
}


int do_work(EMBX_TRANSPORT hTrans, EMBX_PORT myPort)
{
EMBX_PORT  masterPort;
EMBX_ERROR err;
EMBX_VOID *message;
EMBX_UINT  buffer_size;
EMBX_RECEIVE_EVENT ev;


    buffer_size = strlen(cpu_names[config.cpuID-1])+1;

    /*
     * Try and connect to the master port, this will wait until
     * the port is available if it does not already exist.
     */
    if((err = EMBX_ConnectBlock(hTrans, "master", &masterPort)) != EMBX_SUCCESS)
    {
        printf("Failed to connect to master port err = %d\n",err);
        return -1;
    }

    /*
     * Create a new message and send it to the master
     */
    if((err = EMBX_Alloc(hTrans, buffer_size, &message)) != EMBX_SUCCESS)
    {
        EMBX_ClosePort(masterPort);
        printf("Failed to allocate message err = %d\n",err);
        return -1;
    }

    strcpy(message, cpu_names[config.cpuID-1]);

    if((err = EMBX_SendMessage(masterPort, message, buffer_size)) != EMBX_SUCCESS)
    {
        EMBX_ClosePort(masterPort);
        EMBX_Free(message);
        printf("Failed to send message to master err = %d\n",err);
        return -1;
    }

    /*
     * At this point we no longer own "message" so must not use it again
     * or free it.
     */
    if((err = EMBX_ReceiveBlock(myPort, &ev)) != EMBX_SUCCESS)
    {
        EMBX_ClosePort(masterPort);
        printf("Failed to receive reply err = %d\n",err);
        return -1;
    }

    printf("Master replied\n");

    EMBX_ClosePort(masterPort);

    /*
     * We own the received message buffer so it is our responsibility to
     * free it.
     */
    EMBX_Free(ev.data);

    return 0;
}

