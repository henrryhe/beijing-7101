/*******************************************************************/
/* Copyright 2005 STMicroelectronics R&D Ltd. All rights reserved. */ 
/*                                                                 */
/* File: ping_pong_slave.c                                         */
/*                                                                 */
/* Description:                                                    */ 
/*    ST220 slave for ping pong example                            */
/*                                                                 */
/*******************************************************************/

/*
 * Standard OS21, ST200 and STm8000 board support headers
 */
#include <os21.h>
#include <os21/st200.h>
#include <os21/st200/stm8000_audioenc.h>

#include <stdio.h>
#include <stdlib.h>

#include "embx.h"
#include "embxmailbox.h"

EMBX_Mailbox_t *mbox_local, *mbox_remote;

/*
 * Mailbox callback
 */
static void mbox_callback (void *arg)
{
  semaphore_t *signal = (semaphore_t *) arg;

  /* Clear an interrupt for bit 0 */
  EMBX_Mailbox_InterruptClear (mbox_local, 0);

  semaphore_signal (signal);
}


/*
 * Mailbox test
 */
int main (void)
{
  EMBX_ERROR err;
  semaphore_t *mbox_signal;
  int i;

  /* Initialise and start OS21. Also enable timeslicing */
  kernel_initialize (NULL);
  kernel_start ();
  kernel_timeslice (OS21_TRUE);

  printf ("ping_pong started\n");

  /* Create the mailbox signal */
  if ((mbox_signal = semaphore_create_fifo (0)) == NULL)
  {
    printf ("Failed to create mailbox signal\n");
    return 1;
  }

  /* Initialise the mailbox library on the slave CPU */
  if (EMBX_Mailbox_Init () != EMBX_SUCCESS)
  {
    printf ("Failed to initialise mailbox library\n");
    return 1;
  }

  printf ("Mailbox library initialised\n");

  /*
   * Register a mailbox where the second register set will generate interrupts
   * on the slave CPU. The mailbox is also registered by the master CPU such
   * that the first register set will generate interrupts for the master CPU.
   */
  err = EMBX_Mailbox_Register ((void *) 0x10200000,
			       OS21_INTERRUPT_MBOX1_1,
			       0,
			       EMBX_MAILBOX_FLAGS_SET2);

  if (err != EMBX_SUCCESS)
  {
    printf ("Failed to register mbox err = %d\n",err);
    return 1;
  }

  printf ("Mailbox successfully registered\n");

  /* Allocate a matched status/enable pair for the master CPU */
  err = EMBX_Mailbox_Alloc (mbox_callback, (void *) mbox_signal, &mbox_local);
  if (err != EMBX_SUCCESS)
  {
    printf ("Failed to allocate mbox err = %d\n", err);
    return 1;
  }

  printf ("Mailbox successfully allocated\n");

  /* Synchronise with remote mailbox */
  err = EMBX_Mailbox_Synchronize (mbox_local, 0xb0dd1e, &mbox_remote);
  if (err != EMBX_SUCCESS)
  {
    printf ("Failed to synchronize mbox err = %d\n",err);
    return 1;
  }

  printf ("Mailbox successfully synchronised\n");

  /* Enable interrupt generation for bit 0 */
  EMBX_Mailbox_InterruptEnable (mbox_local, 0);

  for (i = 0; i < 5; ++i)
  {
    printf ("Raising mailbox interrupt %d for master CPU\n", i);

    /* Raise an interrupt for bit 0 on master CPU */
    EMBX_Mailbox_InterruptRaise (mbox_remote, 0);

    printf ("Waiting on mailbox interrupt %d from master CPU\n", i);

    /* Wait for an interrupt from the master CPU */
    semaphore_wait (mbox_signal);

    printf ("Received interrupt %d from master CPU\n", i);
  }

  /* Disable the interrupt generation for bit 0 */
  EMBX_Mailbox_InterruptDisable (mbox_local, 0);

  /* Free the matched status/enable pair for the slave CPU */
  EMBX_Mailbox_Free (mbox_local);

  /* Deregister the slave CPU mailbox */
  EMBX_Mailbox_Deregister ((void *) 0x10200000);

  /* Delete the mailbox signal */
  semaphore_delete (mbox_signal);

  printf ("ping_pong finished\n");

  return 0;
}
