/*****************************************************************************
File Name   : stflashtst.h

Description : STFLASH test harness header file.

Copyright (C) 2006 STMicroelectronics

Revision History :

    10/04/00    Added device map information.

Reference   :

ST API Definition "SMART Driver API" DVD-API-06
*****************************************************************************/

#ifndef __STFLASH_TEST_H
#define __STFLASH_TEST_H

/* Includes --------------------------------------------------------------- */
#ifndef DISABLE_TOOLBOX
#include "sttbx.h"
#endif

/* Exported Macros -------------------------------------------------------- */

/* Debug output */
#ifndef DISABLE_TOOLBOX
#define STFLASH_Print(x)          STTBX_Print(x);
#else
#define STFLASH_Print(x)          printf x;
#endif

/* Test harness revision number */
const U8 HarnessRev[] = "3.1.0";
#endif /* __STFLASH_TEST_H */
/* End of header */
