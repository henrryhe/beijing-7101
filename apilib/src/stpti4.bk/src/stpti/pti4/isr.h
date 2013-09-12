/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: isr.h
 Description: pti4 cell interrupt handler definitions.

******************************************************************************/

#ifndef _ISR_H_
 #define _ISR_H_

#define INTERRUPT_STATUS_STATUS_BLOCK_ARRIVED       0x1/*PJW*/
#define INTERRUPT_STATUS_INTERRUPT_FAILED           0x2
#define INTERRUPT_STATUS_PACKET_ERROR_INTERRUPT     0x4
#define INTERRUPT_STATUS_BLOCK_OVERFLOW             0x8 /* TODO */
#define INTERRUPT_STATUS_VALID                      0x7 /* 0x0F */

#if defined( ST_OSLINUX )
#define INTERRUPT_DATA_RING_SIZE 0x10
typedef struct InterruptData_s{
    U32 DeviceHandle;   /* Set by user for this physical device*/
    U32 EventStatus;    /* 0 - Status has interrupt status */
                        /* STPTI_EVENT_INTERRUPT_FAIL_EVT - Buffer overflowed */
                        /* STPTI_EVENT_DMA_COMPLETE_EVT   - Status = DMA destination. */
    U32 Status;         /* The Interrupt status register or DMA destination*/ 
}InterruptData_t;

/* Interrupt Control Structure */
typedef struct InterruptCtl_s{
    U32 WriteIndex;
    U32 ReadIndex;
    InterruptData_t InterruptData[INTERRUPT_DATA_RING_SIZE];
}InterruptCtl_t;

void stptiHAL_InitInterruptCtl(void);
BackendInterruptData_t *stptiHAL_WaitForInterrupt( BackendInterruptData_t *BackendInterruptData_p );
void stptiHAL_StopWaiting(void);
#endif

#endif

/* EOF --------------------------------------------------------- */

