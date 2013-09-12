/*****************************************************************************

File Name  : handle.h

Description: Callback functions for STPRM header

COPYRIGHT (C) 2005 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __HANDLE_H
#define __HANDLE_H

/* Includes --------------------------------------------------------------- */
#include "dvmint.h"

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */
extern STDVMi_Handle_t     *STDVMi_Handles_p;
extern U32                  STDVMi_NbOfHandles;

/* Exported Macros -------------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t      STDVMi_AllocateHandles(U32 MaxHandles);
ST_ErrorCode_t      STDVMi_DeallocateHandles(void);
STDVMi_Handle_t    *STDVMi_GetFreeHandle(void);
ST_ErrorCode_t      STDVMi_ReleaseHandle(STDVMi_Handle_t *Handle_p);
STDVMi_Handle_t    *STDVMi_GetHandleFromPRMHandle(STPRM_Handle_t    PRM_Handle);
#ifdef ENABLE_MP3_PLAYBACK
STDVMi_Handle_t    *STDVMi_GetHandleFromMP3PBHandle(MP3PB_Handle_t  MP3_Handle);
#endif
ST_ErrorCode_t      STDVMi_ValidateOpen(STDVMi_Handle_t *Handle_p);
STDVMi_Handle_t    *STDVMi_GetAlreadyOpenHandle(STDVMi_Handle_t *Handle_p);
STDVMi_Handle_t    *STDVMi_GetHandleFromBuffer(void *Buffer);
BOOL                STDVMi_IsHandleValid(STDVMi_Handle_t *Handle_p);
BOOL                STDVMi_IsAnyHandleOpen(void);

#endif /*__HANDLE_H*/

/* EOF --------------------------------------------------------------------- */

