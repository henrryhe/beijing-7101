/*******************************************************************************

File name   : tbx_out.h

Description : Header of Output feature of toolbox API

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                 Name
----               ------------                 ----
10 mar 1999        Created                       HG
30 Jan 2002        New macros                    HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __TBX_OUT_H
#define __TBX_OUT_H

#if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Includes ----------------------------------------------------------------- */

/*#include's here*/


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* Exported Constants ------------------------------------------------------- */

#define STTBX_USE_SEMAPHORES

/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */

#ifdef STTBX_USE_SEMAPHORES
#define SEM_WAIT(semaphore) semaphore_wait(semaphore)
#else
#define SEM_WAIT(semaphore)
#endif /* #ifdef STTBX_USE_SEMAPHORES */

#ifdef STTBX_USE_SEMAPHORES
#define SEM_SIGNAL(semaphore) semaphore_signal(semaphore)
#else
#define SEM_SIGNAL(semaphore)
#endif /* #ifdef STTBX_USE_SEMAPHORES */

/* Exported Functions ------------------------------------------------------- */

void sttbx_Output(STTBX_Device_t Device, char *Format_p, ...);
void sttbx_StringOutput(STTBX_Device_t Device, char *String_p);
void sttbx_NewPrintSlot(U8 * const PrintTableIndex_p);
void sttbx_FreePrintSlot(const U8 PrintTableIndex);

/*extern __inline void sttbx_OutputDCUNoSemaphore(const char *Message_p);
extern __inline void sttbx_OutputUartNoSemaphore(const char *Message_p);
extern __inline void sttbx_OutputUart2NoSemaphore(const char *Message_p);*/
#if 0 /* Not supported yet */
/*extern __inline void OutputI1284NoSemaphore(const char *Message_p);*/
#endif /* Not supported yet */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif  /* #ifndef ST_OSLINUX */
#endif /* #ifndef __TBX_OUT_H */

/* End of tbx_out.h */
