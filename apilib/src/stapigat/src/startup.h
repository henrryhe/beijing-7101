/*****************************************************************************

File name   : STARTUP.H

Description : common module for inclusion of STARTUP

COPYRIGHT (C) ST-Microelectronics 2002.

Date                Modification                                     Name
----                ------------                                     ----
10 Apr 2002         Created                                          HSdLM
*****************************************************************************/

#ifndef __STARTUP_H
#define __STARTUP_H

/* Includes --------------------------------------------------------------- */
/* Linux porting */
#ifdef ST_OSLINUX
#include "compat.h"
#endif
#include "stddefs.h"

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

extern ST_Partition_t *DriverPartition_p;
extern ST_Partition_t *SystemPartition_p;
extern ST_Partition_t *NCachePartition_p;
extern ST_Partition_t *DataPartition_p;
extern ST_Partition_t *Data2Partition_p;

/* Exported Functions ----------------------------------------------------- */
#ifdef STAPIGAT_USE_NEW_INIT_API
void STAPIGAT_Init(const char * TTDefaultScript);
#else
void STAPIGAT_Init(void);
#endif
void STAPIGAT_Term(void);

#endif /* #ifndef __STARTUP_H */


