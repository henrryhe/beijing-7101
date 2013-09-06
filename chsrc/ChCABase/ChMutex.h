#ifndef __CHMUTEX_H__
#define __CHMUTEX_H__

/*****************************************************************************
  include
 *****************************************************************************/
#include       "ChSys.h"




/*****************************************************************************
 *BASIC DEFINITIONS
 *****************************************************************************/


typedef struct
{
         TCHCAMutexHandle                MutexHandle;
	  semaphore_t*                        psemMutex;
	  CHCA_BOOL                          InUsed;  
	  CHCA_BOOL                          Locked;
} TCHCAMutex;


#endif                 /* __CHMUTEX_H__ */

