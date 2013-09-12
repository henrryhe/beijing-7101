/*******************************************************************************

File name   : blt_be.h

Description : blit_common header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
13 Jun 2000        Created                                           TM
29 May 2006        Modified for WinCE platform						WinCE Team-ST Noida
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_BE_H
#define __BLT_BE_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stevt.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */

typedef enum
{
    SUBMIT,       /* Node insertion */
    SUBMIT_LOCK,  /* Node insertion with lock before first node to insert (Used when Job)*/
    LOCK,         /* Lock request */
    UNLOCK,       /* Unlock request */
    TERMINATE     /* Driver termination */
} stblit_MsgType_t;

typedef struct
{
    stblit_MsgType_t    MessageID;
    void*               Data1_p;
    void*               Data2_p;
} stblit_Msg_t;



/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
void stblit_MasterProcess (void* data_p);
void stblit_SlaveProcess (void* data_p);
void stblit_InterruptProcess (void* data_p);
#ifdef ST_OSLINUX

#ifndef STBLIT_LINUX_FULL_USER_VERSION
irqreturn_t stblit_BlitterInterruptHandler(int irq, void * Param_p, struct pt_regs * regs);
#endif /* STBLIT_LINUX_FULL_USER_VERSION */

#endif  /* ST_OSLINUX */
#if defined(ST_OS21) || defined(ST_OSWINCE)
int stblit_BlitterInterruptHandler (void* Param_p);
#endif  /* ST_OS21 || ST_OSWINCE */
#ifdef ST_OS20
void stblit_BlitterInterruptHandler (void* Param_p);
#endif /* ST_OS20 */


#ifdef ST_OSLINUX

#ifdef STBLIT_LINUX_FULL_USER_VERSION
void stblit_InterruptHandlerTask (const void* SubscriberData_p);
semaphore_t*  BlitCompletionSemaphore;
semaphore_t*  JobCompletionSemaphore;
#else
void stblit_BlitterInterruptEventHandler (STEVT_CallReason_t     Reason,
                                     const ST_DeviceName_t  RegistrantName,
                                     STEVT_EventConstant_t  Event,
                                     const void*            EventData_p,
                                     const void*            SubscriberData_p);
#endif

#endif  /* ST_OSLINUX */

#if defined(ST_OS21) || defined(ST_OSWINCE)
int stblit_BlitterInterruptEventHandler (STEVT_CallReason_t     Reason,
                                     const ST_DeviceName_t  RegistrantName,
                                     STEVT_EventConstant_t  Event,
                                     const void*            EventData_p,
                                     const void*            SubscriberData_p);
#endif  /* ST_OS21 || ST_OSWINCE */
#ifdef ST_OS20
void stblit_BlitterInterruptEventHandler (STEVT_CallReason_t     Reason,
                                     const ST_DeviceName_t  RegistrantName,
                                     STEVT_EventConstant_t  Event,
                                     const void*            EventData_p,
                                     const void*            SubscriberData_p);
#endif /* ST_OS20 */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_BE_H  */

/* End of blt_be.h */
