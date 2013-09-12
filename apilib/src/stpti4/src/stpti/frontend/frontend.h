/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2007.  All rights reserved.

   File Name: frontend.h
 Description: Internal defines and prototypes for access to fronend functions
              that control the STFE & TSGDMA blocks

******************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __FRONTEND_H
#define __FRONTEND_H

/* Includes ------------------------------------------------------------ */

#include "stcommon.h"
#include "stdevice.h"
#include "stpti.h"

/* Exported Types ------------------------------------------------------ */
/* Exported Constants -------------------------------------------------- */
/* Exported Variables -------------------------------------------------- */
/* Exported Macros ----------------------------------------------------- */
/* Exported Functions -------------------------------------------------- */

ST_ErrorCode_t stptiHAL_FrontendGetStatus  ( FullHandle_t FullFrontendHandle, STPTI_FrontendStatus_t * FrontendStatus_p, STPTI_StreamID_t StreamID );
ST_ErrorCode_t stptiHAL_FrontendCloneInput ( FullHandle_t FullFrontendHandle, FullHandle_t             FullPrimaryFrontendHandle                   );
ST_ErrorCode_t stptiHAL_TSINLinkToPTI      ( FullHandle_t FullFrontendHandle, FullHandle_t             FullSessionHandle                           );
void           stptiHAL_SWTSLinkToPTI      ( void );
ST_ErrorCode_t stptiHAL_FrontendReset      ( FullHandle_t FullFrontendHandle, STPTI_StreamID_t         StreamID                                    );
void           stptiHAL_FrontendSetParams  ( FullHandle_t FullFrontendHandle, FullHandle_t             FullSessionHandle                           );
ST_ErrorCode_t stptiHAL_FrontendUnlink     ( FullHandle_t FullFrontendHandle, STPTI_StreamID_t         StreamID                                    );
ST_ErrorCode_t stptiHAL_FrontendClearPid   ( FullHandle_t FullSlotHandle,     STPTI_Pid_t              Pid                                         );
ST_ErrorCode_t stptiHAL_FrontendSetPid     ( FullHandle_t FullSlotHandle,     STPTI_Pid_t              Pid                                         );
void           stptiHAL_SyncFrontendPids   ( FullHandle_t FullFrontendHandle, FullHandle_t             FullSessionHandle                           );
ST_ErrorCode_t stptiHAL_FrontendInjectData ( FullHandle_t FullFrontendHandle, STPTI_FrontendSWTSNode_t *FrontendSWTSNode_p, U32 NumberOfSWTSNodes, STPTI_StreamID_t StreamID );
ST_ErrorCode_t stptiHAL_FrontendSWTSSetPace( FullHandle_t FullSessionHandle,  U32 PaceBps );
#endif /* #ifndef __FRONTEND_H */
/* End of frontend.h */
