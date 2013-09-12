/*******************************************************************************

File name   : tbx_init.h

Description : Header of standard API features of toolbox API

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
10 Mar 99           Created                                          HG
11 Aug 99           Change Semaphores structure (now one
                    semaphore for each I/O device)                   HG
                    Implement UART input/output                      HG
20 Aug 99           Added SupportedDevices                           HG
19 Oct 99           Change Semaphores structure (now two sem
                    for each I/O device: one In and one Out)         HG
06 Apr 00           Declare buffers for the output                   FQ
28 Jan 2002         DDTS GNBvd10702 "Use ST_GetClocksPerSecond       HS
 *                  instead of Hardcoded value"
 *                  Add STTBX_NO_UART compilation flag.
10 Dec 2002         DDTS GNBvd14716, GNBvd14284: allow user choice   HS
 *                  of some buffer sizes
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __TBX_INIT_H
#define __TBX_INIT_H

 #if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#ifndef STTBX_NO_UART
#include "stuart.h"
#endif /* #ifndef STTBX_NO_UART  */


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* Exported Constants ------------------------------------------------------- */

#ifndef STTBX_DCU_OUTPUT_BUFFER_SIZE
#define STTBX_DCU_OUTPUT_BUFFER_SIZE            2048  /* nb of bytes of buffer for DCU   */
#endif /* STTBX_DCU_OUTPUT_BUFFER_SIZE */

#ifndef STTBX_UART_OUTPUT_BUFFER_SIZE
#define STTBX_UART_OUTPUT_BUFFER_SIZE           2048  /* nb of bytes of buffer for UART  */
#endif /* STTBX_UART_OUTPUT_BUFFER_SIZE */

#ifndef STTBX_UART2_OUTPUT_BUFFER_SIZE
#define STTBX_UART2_OUTPUT_BUFFER_SIZE          2048  /* nb of bytes of buffer for UART2 */
#endif /* STTBX_UART2_OUTPUT_BUFFER_SIZE */

#ifndef STTBX_MAX_LINE_SIZE
#define STTBX_MAX_LINE_SIZE                     512   /* nb of bytes of print line */
#endif /* STTBX_MAX_LINE_SIZE */

#ifndef STTBX_PRINT_TABLE_INIT_SIZE
#define STTBX_PRINT_TABLE_INIT_SIZE             10    /* nb of simultaneous prints allocated at init */
#endif /* STTBX_PRINT_TABLE_INIT_SIZE */

enum
{
    DCU_OUTPUT,
#ifndef STTBX_NO_UART
    UART_OUTPUT,
    UART2_OUTPUT,
#endif /* #ifndef STTBX_NO_UART  */
    NUMBER_OF_OUTPUTS
};


/* Exported Types ----------------------------------------------------------- */

typedef struct sttbx_CallingDevice_s
{
    ST_DeviceName_t DeviceName;
    ST_Partition_t *CPUPartition_p;     /* partition for internal memory allocation */
    STTBX_Device_t SupportedDevices;    /* OR of supported devices for this init */
#ifndef STTBX_NO_UART
    STUART_Handle_t UartHandle;
    STUART_Handle_t Uart2Handle;
#endif /* #ifndef STTBX_NO_UART  */
#if 0 /* Not supported yet */
    STI1284_Handle_t I1284Handle;
#endif /* Not supported yet */

    STTBX_Device_t CurrentOutputDevice;
    STTBX_Device_t CurrentInputDevice;
} sttbx_CallingDevice_t;

typedef struct sttbx_PrintTable_s
{
    char *String;
    BOOL IsSlotFree;
} sttbx_PrintTable_t;

/* Exported Variables ------------------------------------------------------- */

/* Input/Output devices locking semaphores */
extern semaphore_t * sttbx_LockDCUIn_p;
extern semaphore_t * sttbx_LockDCUOut_p;
#ifndef STTBX_NO_UART
extern semaphore_t * sttbx_LockUartIn_p;
extern semaphore_t * sttbx_LockUartOut_p;
extern semaphore_t * sttbx_LockUart2In_p;
extern semaphore_t * sttbx_LockUart2Out_p;
#endif /* #ifndef STTBX_NO_UART  */

extern sttbx_PrintTable_t **sttbx_PrintTable_p;
extern U8 sttbx_PrintTableSize;
extern semaphore_t * sttbx_LockPrintTable_p;

/* Flushing buffer locking semaphore */
extern semaphore_t * sttbx_WakeUpFlushingBuffer_p;

#if 0 /* Not supported yet */
extern semaphore_t * sttbx_LockI1284_p;
#endif /* Not supported yet */


/* Handles on opened drivers for I/O devices which require some */
#ifndef STTBX_NO_UART
extern STUART_Handle_t sttbx_GlobalUartHandle;
extern STUART_Handle_t sttbx_GlobalUart2Handle;
#endif /* #ifndef STTBX_NO_UART  */
#if 0 /* Not supported yet */
extern STI1284_Handle_t GlobalI1284Handle;
#endif /* Not supported yet */


/* Current input/output devices, seen in all the toolbox at any time */
extern STTBX_Device_t sttbx_GlobalCurrentOutputDevice;
extern STTBX_Device_t sttbx_GlobalCurrentInputDevice;

extern void *sttbx_GlobalCPUPartition_p;

#if defined(ST_OS21) || defined(ST_OSWINCE)
extern osclock_t sttbx_ClocksPerSecond;
#else
extern U32 sttbx_ClocksPerSecond;
#endif

#ifdef ST_OSWINCE
/* managing of the input thread (non-blocking input) */
extern HANDLE sttbx_ghInputThread; /* input thread handle */
extern HANDLE sttbx_ghInputQueue;  /* message queue handle */
extern DWORD  sttbx_fInputEntryPoint(LPVOID lpParameter); /* input thread entry point */
#endif /* ST_OSWINCE */

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif  /* #ifndef ST_OSLINUX */
#endif /* #ifndef __TBX_INIT_H */

/* End of tbx_init.h */

