/*******************************************************************************

File name   : MemoryTrace_utils.h

Description : Memory Trace managment

COPYRIGHT (C) STMicroelectronics 2006.

Date                    Modification                                     Name
----                    ------------                                     ----
26 Ferbruary 2006       Created                                          ellafiha
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __MEM_TRACE_H
#define __MEM_TRACE_H
#endif

/* Includes ----------------------------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

/* Global Variables */

/* ---- Functions ------------------------------------------------------- */
void MemoryTrace_Init( ST_Partition_t* AllocationPartition_p );
void MemoryTrace_Write (const char *format,...);
void MemoryTrace_Write_Config (const char *format,...);
ST_ErrorCode_t MemoryTrace_DumpMemory(void);

