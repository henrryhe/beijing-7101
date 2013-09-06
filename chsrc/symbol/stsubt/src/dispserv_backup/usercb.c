/******************************************************************************\
 * File Name : usercb.c
 * Description:
 *     Define User Defined display service 
 * 
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author      : Marino Congiu - Oct 1999
 *
\******************************************************************************/
 
#include <stdlib.h>
#include <stddefs.h>
#include "dispserv.h"
 
#ifdef USER_DISPLAY_SERVICE

/* To enable the User Defined Display Service, the macro USER_DISPLAY_SERVICE 
 * has to be defined at compile time (-D USER_DISPLAY_SERVICE).
 * Definition of display callback functions is externally provided by the user.
 * Application must be linked to object file containing their implementation.
 */

extern void *STSUBT_UserDefined_InitializeService(
                       STSUBT_DisplayServiceParams_t *DisplayServiceParams);

extern ST_ErrorCode_t STSUBT_UserDefined_TerminateService(
                                         void *ServiceHandle);

extern ST_ErrorCode_t STSUBT_UserDefined_PrepareDisplay(
                                         PCS_t *PageComposition_p,
                                         void  *ServiceHandle,
                                         void **display_descriptor);

extern ST_ErrorCode_t STSUBT_UserDefined_ShowDisplay(
                                         void *ServiceHandle,
                                         void *display_descriptor);

extern ST_ErrorCode_t STSUBT_UserDefined_HideDisplay(
                                         void *ServiceHandle,
                                         void *display_descriptor);

/* ---------------------------------------------- */
/* --- User Defined Display Service Structure --- */
/* ---------------------------------------------- */
 
const STSUBT_DisplayService_t STSUBT_UserDefined_DisplayServiceStructure = {
  STSUBT_UserDefined_InitializeService,
  STSUBT_UserDefined_PrepareDisplay,
  STSUBT_UserDefined_ShowDisplay,
  STSUBT_UserDefined_HideDisplay,
  STSUBT_UserDefined_TerminateService,
  NULL
} ;

#endif
