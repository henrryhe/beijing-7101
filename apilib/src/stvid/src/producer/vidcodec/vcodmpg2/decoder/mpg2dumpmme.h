/*******************************************************************************

File name   : vc1dumpmme.h

Description : Prototype functions of "vc1dumpmme.c"

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
02/09/2005         Creation                                         Laurent Delaporte

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DUMP_MME_H
#define __DUMP_MME_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "mme.h"
#include <stdio.h>  /* FILE, va_start */
#include <stdarg.h> /* va_start */

/* Exported Defines --------------------------------------------------------- */
#ifdef STVID_VALID_DEC_TIME
#define DEC_TIME_DUMP_FILE_NAME  "../../dump_dec_time.txt"
#endif
/* Exported Types ----------------------------------------------------------- */

typedef struct {
    FILE* File_p;
    unsigned int* Counter_p;
    unsigned int CounterBegin;
    unsigned int CounterEnd;
    unsigned int FlushFlag;
} DumpFile_s;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
#ifdef STVID_VALID_DEC_TIME
extern BOOL VVID_MPEG2DEC_TimeDump_Command_function(char * filename, U32 DEC_Time_Counter);
#endif  /*STVID_VALID_DEC_TIME*/

extern void DumpFile_fopen(DumpFile_s* DumpFile_p, char* FileName_p, char* Mode_p, char* Pattern_p, unsigned int* Counter_p);
extern void DumpFile_fclose(DumpFile_s* DumpFile_p);
extern int DumpFile_fprintf(DumpFile_s* DumpFile_p, const char *format, ...);
extern int DumpFile_vfprintf(DumpFile_s* DumpFile_p, const char *format, va_list argptr);
extern long DumpFile_fwrite(const void *buffer, long size, long count, DumpFile_s *DumpFile_p);

extern int DumpFile_check(DumpFile_s* DumpFile_p);

/*extern ST_ErrorCode_t VVID_MPEG2DEC_TimeDump_Term(void);
extern ST_ErrorCode_t VVID_MPEG2DEC_TimeDump_Init(char * filename);
*/
#endif /* #ifndef __DUMP_MME_H */

/* End of dump_mme.h */
