/*****************************************************************************
 *
 *  Module      : stvidtest_ioctl
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description :
 *  Copyright   : STMicroelectronics (c)2006
 *****************************************************************************/


#ifndef STVIDTEST_IOCTL_H
#define STVIDTEST_IOCTL_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */


#include "stvidtest.h"

#include "vidtest_rev.h"
#include "stos.h"

/*** IOCtl defines ***/

#define STVIDTEST_MAGIC_NUMBER      126

/* Use STVIDTEST_MAGIC_NUMBER as magic number */
/* Standard API */

#define STVIDTEST_IOC_INIT                                      _IO(STVIDTEST_MAGIC_NUMBER, 0)
#define STVIDTEST_IOC_OPEN                                      _IO(STVIDTEST_MAGIC_NUMBER, 1)
#define STVIDTEST_IOC_CLOSE                                     _IO(STVIDTEST_MAGIC_NUMBER, 2)
#define STVIDTEST_IOC_TERM                                      _IO(STVIDTEST_MAGIC_NUMBER, 3)

#define STVIDTEST_IOC_GETREVISION                               _IO(STVIDTEST_MAGIC_NUMBER, 4)
#define STVIDTEST_IOC_GETCAPABILITY                             _IO(STVIDTEST_MAGIC_NUMBER, 5)

/* New API */
#define STVIDTEST_IOC_LOADFILEINSTREAMBUFFER                    _IO(STVIDTEST_MAGIC_NUMBER, 6)
#define STVIDTEST_IOC_LOADINTERNALTABLEINSTREAMBUFFER           _IO(STVIDTEST_MAGIC_NUMBER, 7)
#define STVIDTEST_IOC_INJECTFROMSTREAMBUFFER                    _IO(STVIDTEST_MAGIC_NUMBER, 8)
#define STVIDTEST_IOC_KILLINJECTFROMSTREAMBUFFER                _IO(STVIDTEST_MAGIC_NUMBER, 9)
    
#define STVIDTEST_IOC_GETKERNELINJECTIONFUNCTIONPOINTERS        _IO(STVIDTEST_MAGIC_NUMBER, 10)

/* For decode */
#define STVIDTEST_IOC_DECODEFROMSTREAMBUFFER                    _IO(STVIDTEST_MAGIC_NUMBER, 12)

/* ------------------------------------------------------------------------- */
/* Fonction prototypes                                                       */
/* ------------------------------------------------------------------------- */

ST_ErrorCode_t STINJTEST_Ioctl(unsigned int cmd, unsigned long arg);

#ifndef STPTI_UNAVAILABLE 
ST_ErrorCode_t STPTITEST_Ioctl(unsigned int cmd, unsigned long arg);
extern ST_ErrorCode_t GetWritePtrFct(void * const Handle, void ** const Address_p);
extern void SetReadPtrFct(void * const Handle, void * const Address_p);
#endif

#ifndef HDD_UNAVAILABLE 
ST_ErrorCode_t STHDDTEST_Ioctl(unsigned int cmd, unsigned long arg);
#endif

/* =================================================================
   Global types declaration
   ================================================================= */

#ifndef MODULE
typedef struct
{
    /* Device identification */
    ST_DeviceName_t         DeviceName;

    /* Device FILE */
    FILE                  * DevFile;

    /* Device Handle */
    STVID_Handle_t          Handle;

} VIDTESTModDevice_t;
#endif

typedef struct VIDTEST_Ioctl_GetCapability_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STVIDTEST_Capability_t  Capability;
    
} VIDTEST_Ioctl_GetCapability_t;

typedef struct VIDTEST_Ioctl_Init_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STVIDTEST_InitParams_t InitParams;
    
} VIDTEST_Ioctl_Init_t;


typedef struct VIDTEST_Ioctl_Term_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STVIDTEST_TermParams_t TermParams;
    
} VIDTEST_Ioctl_Term_t;

typedef struct VIDTEST_Ioctl_InjectFromStreamBuffer_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t      Handle;
    S32                 FifoNb;
    S32                 BufferNb;
    void               *Buffer_p;
    U32                 NbBytes;
    S32                 NbInjections;
    
} VIDTEST_Ioctl_InjectFromStreamBuffer_t;

typedef struct VIDTEST_Ioctl_KillInjectFromStreamBuffer_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    S32             FifoNb;
    BOOL            WaitEnfOfOnGoingInjection;
        
} VIDTEST_Ioctl_KillInjectFromStreamBuffer_t;

typedef struct VIDTEST_Ioctl_DecodeFromStreamBuffer_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t      Handle;
    S32                 FifoNb;
    S32                 BuffNb;
    S32                 NbLoops;
    void               *Buffer_p;
    U32                 NbBytes;
    
} VIDTEST_Ioctl_DecodeFromStreamBuffer_t;

typedef struct VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    KernelInjectionType_t Type;
    void *GetWriteAdd;
    void *InformReadAdd;
        
} VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t;

#endif  /* STVIDTEST_IOCTL_H */
