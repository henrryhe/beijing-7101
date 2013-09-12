/*******************************************************************************

File name   : h264preproc.h

Description :

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
10 Oct 2003       Created
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __H264PREPROC_H
#define __H264PREPROC_H

/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <assert.h>
#include <stlite.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "stavmem.h"
#include "vid_com.h"
#include "preprocessor.h"

/* Exported Constants ------------------------------------------------------- */

#define GNBvd42332

/* Always force usage of a single preproc HW */
#define STVID_USE_ONE_PREPROC

#if (defined(ST_7100) || (defined(ST_7109) && defined(ST_CUT_1_X)) || defined(ST_DELTAPHI_HE) || (defined(ST_DELTAMU_HE) && !defined(ST_DELTAMU_GREEN_HE))) && !defined(STVID_USE_ONE_PREPROC)
    #define H264PP_NB_PREPROCESSORS 2
#else
    #define H264PP_NB_PREPROCESSORS 1
#endif

/* Exported Types ----------------------------------------------------------- */

typedef struct H264PreprocHardware_Data_s H264PreprocHardware_Data_t;

typedef struct H264Preproc_Context_s
{
    U32                         PreprocNumber;
    H264PreprocHardware_Data_t *H264PreprocHardware_Data_p; /* Pointer to get Base address, */
                                                            /* Job completion semaphore ...*/
    void*                       CallbackUserData;
    H264PreprocCallback_t       Callback;
    H264PreprocCommand_t        *H264PreprocCommand_p;      /* Current Command */
    BOOL                        IsPreprocRunning;
    BOOL                        IsAbortPending;
    BOOL                        IRQ_Occured;
    U32                         pp_isbg;    /* Intermediate Slice Error Status Buffer Begin */
    U32                         pp_ipbg;    /* Intermediate Picture Buffer Begin */
    U32                         pp_wdl;
    U32                         pp_its;
    semaphore_t                *SRS_Wait_semaphore_p;
#ifdef GNBvd42332
    semaphore_t                *GNBvd42332_Wait_semaphore;
    BOOL                        GNBvd42332_PictureInProcess;
    U32                         GNBvd42332_Previous_MBAFF_flag;
    BOOL                        GNBvd42332_PreviousPreprocHasError;
    BOOL                        GNBvd42332_CurrentPreprocHasError;
#endif /* GNBvd42332 */
} H264Preproc_Context_t;

struct H264PreprocHardware_Data_s
{
    ST_Partition_t             *CPUPartition_p;         /* Where the module can allocate memory for its internal usage */
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;
    void                       *RegistersBaseAddress_p; /* Delta IP base address */
    U32                         InterruptNumber[H264PP_NB_PREPROCESSORS];
#ifdef GNBvd42332
    U32                         GNBvd42332_PicStart;
    U32                         GNBvd42332_PicStop;
    void                       *GNBvd42332_Data_Address;
    STAVMEM_BlockHandle_t       GNBvd42332_Data_Handle;
#endif /* GNBvd42332 */

#if !defined PERF1_TASK3
    VIDCOM_Task_t               PreprocessorTask;
    semaphore_t                *PreprocJobCompletion;   /* Semaphore for signaling a job completion */

    semaphore_t                *PreprocStatusSemaphore;
#endif /* PERF1_TASK3*/

    H264Preproc_Context_t       Preproc_Context[H264PP_NB_PREPROCESSORS];

    U32                         NbLinkedTransformer;    /* Number of transformer instance currently using this hardware */
    H264PreprocHardware_Data_t *NextHardware_p;         /* Link for Hardware IPs list */
};

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */
/* Exported Functions ------------------------------------------------------- */
/* from h264preproc.c module */
ST_ErrorCode_t H264PreprocHardware_Init(H264Preproc_InitParams_t *H264Preproc_InitParams_p, void **Handle);
#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t H264PreprocHardware_SetupReservedPartitionForH264PreprocWA_GNB42332(void *Handle, const STAVMEM_PartitionHandle_t AVMEMPartition);
#endif /* defined(DVD_SECURED_CHIP) */
ST_ErrorCode_t H264PreprocHardware_ProcessPicture(void *Handle,
                                                  H264PreprocCommand_t *H264PreprocCommand_p,
                                                  H264PreprocCallback_t Callback,
                                                  void* CallbackUserData);
ST_ErrorCode_t H264PreprocHardware_Term(void *Handle);
#if !defined PERF1_TASK3
ST_ErrorCode_t H264PreprocHardware_IsHardwareBusy(void *Handle, BOOL *Busy);
#endif /* PERF1_TASK3 */
ST_ErrorCode_t H264PreprocHardware_AbortCommand(void *Handle, H264PreprocCommand_t *H264PreprocCommand_p);

#endif /* #ifndef __H264PREPROC_H */

/* End of h264preproc.h */
