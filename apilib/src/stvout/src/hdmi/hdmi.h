/*******************************************************************************
File name   : hdmi.h

Description : VOUT driver header file for hdmi functions

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
3 Apr 2004         Created                                          AC

*******************************************************************************/

#ifndef __HDMI_H
#define __HDMI_H

/* Includes ----------------------------------------------------------------- */
#include "vout_drv.h"
#include "stddefs.h"
#include "stcommon.h"


#include "sttbx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ---------------------------------------------------------- */

/* Exported Types--------------------------------------------------------- */

typedef struct {
    U8 * Data_p;
    U32  TotalSize;
    U32  UsedSize;
    U32  MaxUsedSize;       /* To monitor max size used in commands buffer */
    U8 * BeginPointer_p;    /* This is a circular buffer, with BeginPointer_p and UsedSize we know what is in */
} CommandsBuffer_t;

typedef struct {
    U32 * Data_p;
    U32  TotalSize;
    U32  UsedSize;
    U32  MaxUsedSize;       /* To monitor max size used in commands buffer */
    U32 * BeginPointer_p;    /* This is a circular buffer, with BeginPointer_p and UsedSize we know what is in */
} CommandsBuffer32_t;

/* Private Constants ------------------------------------------------------ */
#define  ONE_SECOND              ST_GetClocksPerSecond()
/* Private variables (static) --------------------------------------------- */

/* Global Variables ------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */
#define PushInterruptCommand(Handle, U32Var) Hdmicom_PushCommand32(&(((HDMI_Data_t *) Handle)->InterruptsBuffer), ((U32) (U32Var)))
#define PopInterruptCommand(Handle, U32Var_p) Hdmicom_PopCommand32(&(((HDMI_Data_t *) Handle)->InterruptsBuffer), ((U32 *) (U32Var_p)))

#define PushControllerCommand(Handle, U8Var) Hdmicom_PushCommand(&(((HDMI_Data_t *) (Handle))->CommandsBuffer), ((U8) (U8Var)))
#define PopControllerCommand(Handle, U8Var_p) Hdmicom_PopCommand(&(((HDMI_Data_t *) (Handle))->CommandsBuffer), ((U8 *) (U8Var_p)))

/* Private Macros --------------------------------------------------------- */
#define HDMI_HANDLE_LOCK(Handle){ \
      STOS_SemaphoreWait(((HDMI_Data_t*)Handle)->DataAccessSem_p); \
}

#define HDMI_HANDLE_UNLOCK(Handle) { \
     STOS_SemaphoreSignal(((HDMI_Data_t *)Handle)->DataAccessSem_p); \
}

/* Exported Functions ----------------------------------------------------- */
ST_ErrorCode_t  Hdmicom_PopCommand32                (CommandsBuffer32_t * const Buffer_p, U32 * const Data_p);
void            Hdmicom_PushCommand32               (CommandsBuffer32_t * const Buffer_p, const U32 Data);
ST_ErrorCode_t  Hdmicom_PopCommand                  (CommandsBuffer_t * const Buffer_p, U8 * const Data_p);
void            Hdmicom_PushCommand                 (CommandsBuffer_t * const Buffer_p, const U8 Data);
void            HDMI_SoftReset                      (const HDMI_Handle_t Handle);
ST_ErrorCode_t  StartInternalTask                   (const HDMI_Handle_t Handle);
ST_ErrorCode_t  StopInternalTask                    (const HDMI_Handle_t Handle);
/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HDMI_H */
