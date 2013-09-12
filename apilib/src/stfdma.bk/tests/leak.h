/*****************************************************************************
File Name   : leak.h

Description : 

History     : 

Copyright (C) 2002 STMicroelectronics

Reference   :
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LEAK_H
#define __LEAK_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"         /* Standard definitions */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
void leak_RunLeakTest(void);                  
                  

#ifdef __cplusplus
}
#endif

#endif /* __LEAK_H */

/* End of leak.h */
