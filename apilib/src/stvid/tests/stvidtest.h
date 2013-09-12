/*******************************************************************************
File Name   : stvidtest.h

Description : Video injection driver header file

Copyright (C) 2005 STMicroelectronics

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVIDTEST_H
#define __STVIDTEST_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stos.h"

#include "stdevice.h"

#include "stevt.h"
#include "stavmem.h"

#include "stgxobj.h"
#include "stlayer.h"

#include "stvid.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Exported Constants ------------------------------------------------------- */

#define STVIDTEST_DRIVER_ID                 50
#define STVIDTEST_DRIVER_BASE               (STVIDTEST_DRIVER_ID << 16)


enum
{
    STVIDTEST_ERROR_INJECTION_FAILED = STVIDTEST_DRIVER_BASE,
    STVIDTEST_ERROR_NO_FILE_IN_MEMORY
};

enum
{
    /* This event passes a (STVID_DisplayAspectRatio_t) as parameter */
    STVIDTEST_SAMPLE1_EVT = STVIDTEST_DRIVER_BASE,
    /* This event passes no parameter */
    STVIDTEST_SAMPLE2_EVT
};

typedef enum KernelInjectionType_e
{
    STVIDTEST_INJECT_NONE          = 0,
    STVIDTEST_INJECT_MEMORY,
    STVIDTEST_INJECT_PTI,
    STVIDTEST_INJECT_HDD                 
} KernelInjectionType_t;

/* Exported Types ----------------------------------------------------------- */

typedef U32 STVIDTEST_Handle_t;

typedef struct STVIDTEST_Capability_s
{
    BOOL                                 SupportedInjectionFromMemory;
} STVIDTEST_Capability_t;

typedef struct STVIDTEST_InitParams_s
{
    BOOL        InitFlag; /* Dummy: to be removed */
} STVIDTEST_InitParams_t;

typedef struct STVIDIN_OpenParams_s
{
} STVIDTEST_OpenParams_t;

typedef struct STVIDTEST_CloseParams_s
{
} STVIDTEST_CloseParams_t;

typedef struct STVIDTEST_TermParams_s
{
    BOOL ForceTerminate;
} STVIDTEST_TermParams_t;


/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

ST_Revision_t  STVIDTEST_GetRevision(void);
ST_ErrorCode_t STVIDTEST_GetCapability(const ST_DeviceName_t DeviceName, STVIDTEST_Capability_t  * const Capability_p);
ST_ErrorCode_t STVIDTEST_Init(const ST_DeviceName_t DeviceName, const STVIDTEST_InitParams_t * const InitParams_p);
ST_ErrorCode_t STVIDTEST_Term(const ST_DeviceName_t DeviceName, const STVIDTEST_TermParams_t * const TermParams_p);

ST_ErrorCode_t STVIDTEST_LoadFileInStreamBuffer(const STVIDTEST_Handle_t Handle, const STVID_SetupObject_t SetupObject, const char * const FileName);
ST_ErrorCode_t STVIDTEST_LoadInternalTableInStreamBuffer(const STVIDTEST_Handle_t Handle, const STVID_SetupObject_t SetupObject);

#if 0
ST_ErrorCode_t STVIDTEST_InjectFromStreamBuffer(const STVIDTEST_Handle_t Handle, const STVID_SetupObject_t SetupObject, const S32 NbInjections);
ST_ErrorCode_t STVIDTEST_KillInjectFromStreamBuffer(const STVIDTEST_Handle_t Handle, const STVID_SetupObject_t SetupObject, const BOOL WaitEnfOfOnGoingInjection);
#else
ST_ErrorCode_t STVIDTEST_InjectFromStreamBuffer(STVID_Handle_t  CurrentHandle,
                                                S32             InjectLoop,
                                                S32             FifoNb,
                                                S32             BufferNb,
                                                void           *Buffer_p,
                                                U32             NbBytes);
ST_ErrorCode_t STVIDTEST_KillInjectFromStreamBuffer(S32 FifoNb, BOOL WaitEnd);
#endif

ST_ErrorCode_t STVIDTEST_GetKernelInjectionFunctionPointers(KernelInjectionType_t Type, void **GetWriteAdd, void **InformReadAdd);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __STVIDTEST_H */

/* End of stvidtest.h */
