/*******************************************************************************

File name   : dump_mme.h

Description : Prototype functions of "dump_mme.c"

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
16/12/2003         Creation                                         Didier SIRON

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DUMP_MME_H
#define __DUMP_MME_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "mme.h"

/* Exported Defines --------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

extern ST_ErrorCode_t HDM_Init(void);
extern ST_ErrorCode_t HDM_Term(void);
extern ST_ErrorCode_t HDM_DumpCommand(MME_CommandCode_t CmdCode, MME_Command_t* CmdInfo_p);
extern ST_ErrorCode_t VVID_DEC_TimeDump_Term(void);
extern ST_ErrorCode_t VVID_DEC_TimeDump_Init(char * filename);
extern BOOL VVID_DEC_TimeDump_Command_function(char * filename, U32 DEC_Time_Counter);

#endif /* #ifndef __DUMP_MME_H */

/* End of dump_mme.h */
