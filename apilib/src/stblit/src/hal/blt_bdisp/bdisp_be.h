/*******************************************************************************

File name   : bdisp_be.h

Description : blit_common header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
01 Jun 2004        Created                                          HE
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_BE_H
#define __BLT_BE_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stevt.h"
#include "bdisp_init.h"
#include "bdisp_com.h"
#ifdef STBLIT_USE_MEMORY_TRACE
#include "memorytrace_utils.h"
#endif /* STBLIT_USE_MEMORY_TRACE */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#ifdef STBLIT_USE_MEMORY_TRACE
#define STBLIT_TRACE MemoryTrace_Write
#else /* !STBLIT_USE_MEMORY_TRACE */
#define STBLIT_TRACE trace
#endif /* STBLIT_USE_MEMORY_TRACE */


#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
#ifndef STBLIT_HW_BLITTER_RESET_CHECK_TIME_OUT
#define STBLIT_HW_BLITTER_RESET_CHECK_TIME_OUT 2
#endif /* STBLIT_HW_BLITTER_RESET_CHECK_TIME_OUT */
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */

#ifdef STBLIT_ENABLE_HW_BLITTER_RESET_SIMULATION
#define STBLIT_HARDWARE_ERROR_SIMULATION_MAX_VALUE 10
#endif /* STBLIT_ENABLE_HW_BLITTER_RESET_SIMULATION */


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
#ifdef STBLIT_LINUX_FULL_USER_VERSION
void stblit_InterruptProcess (stblit_Device_t * Device_p, STBLITMod_Status_t * Status_p);
#else
void stblit_InterruptProcess (void* data_p);
#endif /* STBLIT_LINUX_FULL_USER_VERSION*/ /*ST_ZEUS*/

#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
void stblit_BlitterErrorCheckProcess (void* data_p);
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */


#if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
int stblit_BlitterInterruptHandler(int irq, void * Param_p, struct pt_regs * regs);
#endif  /* ST_OSLINUX */

#ifdef STBLIT_LINUX_FULL_USER_VERSION
void stblit_InterruptHandlerTask (const void* SubscriberData_p);
#endif /*STBLIT_LINUX_FULL_USER_VERSION*/

#ifdef ST_OS21
int stblit_BlitterInterruptHandler (void* Param_p);
#endif /*ST_OS21*/

#ifdef ST_OS20
void stblit_BlitterInterruptHandler (void* Param_p);
#endif /*ST_OS20*/


ST_ErrorCode_t stblit_PostSubmitMessage(stblit_Device_t * const Device_p,
                                        void* FirstNode_p, void* LastNode_p,
                                        BOOL HightPriorityNodes,
                                        BOOL LockBeforeFirstNode,
                                        U32  NumberNodes);


#ifdef STBLIT_DEBUG_GET_STATISTICS
U32 stblit_ConvertFromTicsToUs(STOS_Clock_t Time);
#endif /* STBLIT_DEBUG_GET_STATISTICS */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_BE_H  */

/* End of bdisp_be.h */
