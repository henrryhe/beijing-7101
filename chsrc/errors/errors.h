/* ----------------------------------------------------------------------------

  File Name: errors.h
Description: get error as string representation from error code.

Copyright (C) 2002 STMicroelectronics

---------------------------------------------------------------------------- */

#ifndef __ERRORS_H
 #define __ERRORS_H


/* Includes ---------------------------------------------------------------- */

#include "stddefs.h"    /* STAPI includes */


/* Function prototypes ----------------------------------------------------- */

char *GetErrorText(ST_ErrorCode_t ST_ErrorCode);


#endif /* __ERRORS_H */

/* EOF --------------------------------------------------------------------- */
