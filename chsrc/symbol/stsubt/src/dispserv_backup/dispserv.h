/******************************************************************************\
 *
 * File Name   : dispserv.h
 *  
 * Description : Predefined Display Services for common Devices
 *  
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author      : Marino Congiu - June 1999
 *  
\******************************************************************************/

#ifndef __DISPLCB_H
#define __DISPLCB_H

#include <stddefs.h>
#include <compbuf.h>

typedef struct STSUBT_DisplayServiceParams_s {
          ST_Partition_t    *MemoryPartition;
          void              *Handle_p;
          U32               RDEBufferSize;
          U8*               RDEBuffer_p;
} STSUBT_DisplayServiceParams_t;



/* --- Prototypes of display callback functions --- */

typedef void * (*STSUBT_InitCallback_t)(STSUBT_DisplayServiceParams_t *DisplayServiceParams);

typedef ST_ErrorCode_t (*STSUBT_TermCallback_t)(void *ServiceHandle);
 
typedef ST_ErrorCode_t (*STSUBT_PrepareCallback_t)(PCS_t *PageComposition,
                                                void  *ServiceHandle,
                                                void **DisplayDescriptor);
 
typedef ST_ErrorCode_t (*STSUBT_ShowCallback_t)(void *DisplayDescriptor,
                                                void *ServiceHandle);
typedef ST_ErrorCode_t (*STSUBT_HideCallback_t)(void *DisplayDescriptor,
                                                void *ServiceHandle);

/* --- Structure defining a Display Service --- */
 
typedef struct STSUBT_DisplayService_t {
  STSUBT_InitCallback_t    STSUBT_InitializeService;
  STSUBT_PrepareCallback_t STSUBT_PrepareDisplay;
  STSUBT_ShowCallback_t    STSUBT_ShowDisplay;
  STSUBT_HideCallback_t    STSUBT_HideDisplay;
  STSUBT_TermCallback_t    STSUBT_TerminateService;
  void                    *STSUBT_ServiceHandle;
} STSUBT_DisplayService_t;



/* --- Predefined Display Service Structures --- */
 
/* Predefined implementation of null display callback functions;
 * subtitles will be not displayed at all.
 */
extern const STSUBT_DisplayService_t STSUBT_NULL_DisplayServiceStructure;

#ifndef DISABLE_OSD_DISPLAY_SERVICE

/* Predefined implementation of the display callback functions
 * based on the OSD graphics system
 */
extern STSUBT_DisplayService_t STSUBT_OSD_DisplayServiceStructure;

#endif

#ifdef RDE_DISPLAY_SERVICE

/* Definition of the RDE  display service. Implementation of
 * callback function is externally provided.
 */
extern const STSUBT_DisplayService_t STSUBT_RDE_DisplayServiceStructure;

#endif

#ifdef USER_DISPLAY_SERVICE

/* Definition of the user defined display service. Implementation of
 * callback function is externally provided.
 */

extern const STSUBT_DisplayService_t STSUBT_UserDefined_DisplayServiceStructure;

#endif
 
#ifdef TEST_DISPLAY_SERVICE

/* Predefined implementation of the display callback functions
 * to be used for testing; they reports on achieved displays.
 */
extern const STSUBT_DisplayService_t STSUBT_TEST_DisplayServiceStructure;
 
#endif

#endif  /* #ifndef __DISPLCB_H */

