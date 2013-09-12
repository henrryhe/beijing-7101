#ifndef API_H264_SINGLE_FRAME_PREPROCESSOR_INC
#define API_H264_SINGLE_FRAME_PREPROCESSOR_INC

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stavmem.h"
#include "vid_com.h"

#include "H264_VideoTransformerTypes_Preproc.h"

/* Exported Constants ------------------------------------------------------- */

/* Exported types ----------------------------------------------------------- */

typedef void* H264PreprocHandle_t;

typedef struct H264PreprocCommand_s
{
    U32                  CmdId;
    ST_ErrorCode_t       Error;
    H264_CommandStatus_preproc_t   CommandStatus_preproc;
    H264_TransformParam_preproc_t  TransformerPreprocParam;
} H264PreprocCommand_t;

typedef void (*H264PreprocCallback_t) (H264PreprocCommand_t *CallbackData, void *UserData);

typedef struct H264Preproc_InitParams_s
{
    ST_Partition_t             *CPUPartition_p; /* Partition where to allocate local data */
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;
    void                       *RegistersBaseAddress_p;  /* Delta IP Base address */
    U32                         InterruptNumber;
    void*                       CallbackUserData;
    H264PreprocCallback_t       Callback;
} H264Preproc_InitParams_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t H264Preproc_InitTransformer (H264Preproc_InitParams_t *H264Preproc_InitParams_p, H264PreprocHandle_t *Handle);
#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t H264Preproc_SetupTransformerWA_GNB42332 (H264PreprocHandle_t Handle, const STAVMEM_PartitionHandle_t AVMEMPartition);
#endif /* defined(DVD_SECURED_CHIP) */
ST_ErrorCode_t H264Preproc_TermTransformer(H264PreprocHandle_t Handle);
ST_ErrorCode_t H264Preproc_ProcessCommand(H264PreprocHandle_t Handle, H264PreprocCommand_t *H264PreprocCommand_p);
ST_ErrorCode_t H264Preproc_AbortCommand(H264PreprocHandle_t Handle, H264PreprocCommand_t *H264PreprocCommand_p);

#if !defined PERF1_TASK3
ST_ErrorCode_t H264Preproc_IsHardwareBusy(H264PreprocHandle_t Handle, BOOL *Busy);
#endif /*PERF1_TASK3*/
        /* C++ support */
#ifdef __cplusplus
}
#endif

#endif
