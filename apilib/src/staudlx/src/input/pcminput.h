/*******************************************************

 File Name: pcminput.h

 Description: Header File for PCM input
 Copyright: STMicroelectronics (c) 2005

*******************************************************/

#ifndef _PCM_INPUT_H
#define _PCM_INPUT_H
#include "staudlx.h"
#include "staud.h"
#include "blockmanager.h"

#ifndef ST_51XX
#include "aud_mmeaudiostreamdecoder.h"
#include "acc_multicom_toolbox.h"
#else
#include "aud_defines.h"
#endif

#include "stos.h"
typedef U32 STPCMInput_Handle_t;
typedef enum {

    PCM_INPUT_STATE_INIT=0,
    PCM_INPUT_STATE_START,
    PCM_INPUT_STATE_STOP,
    PCM_INPUT_STATE_TERMINATE

}PCMInputStates_t;

typedef struct
{
    ST_Partition_t            * CPUPartition_p;
    STAVMEM_PartitionHandle_t   AVMEMPartition;
    STMEMBLK_Handle_t           PCMBlockHandle;
    U8                          NumChannels;
}STAudPCMInputInit_t;

typedef struct
{
    STPCMInput_Handle_t Handle;
    STAudPCMInputInit_t InitParams;
    ProducerParams_t    ProducerParam_s;
    U32                 Frequency;
    U32                 SampleSize;
    U8                  Nch;
    BOOL                UpdateParams;
    semaphore_t              * PCMBufferSem_p;
    STAud_LpcmStreamParams_t * AppData_p[NUM_NODES_PCMINPUT];
    U32                     OPBlkSent;
    STMEMBLK_Handle_t       PCMBlockHandle;
    U32                     PCM_Buff_Size;/* Size of PCM input buffers */
    BOOL                    EOFFlag;
    PCMInputStates_t        PCMCurrentState;
    STAud_Memory_Address_t  MemoryAddress;
}STAudPCMInputControl_t;



ST_ErrorCode_t STAud_InitPCMInput(STAudPCMInputInit_t *, STPCMInput_Handle_t *);
ST_ErrorCode_t STAud_QueuePCMBuffer(STPCMInput_Handle_t Handle, STAUD_PCMBuffer_t*, U32, U32*);
ST_ErrorCode_t STAud_GetPCMBuffer(STPCMInput_Handle_t Handle, STAUD_PCMBuffer_t *PCMBuffer_p);
ST_ErrorCode_t STAud_TermPCMInput(STPCMInput_Handle_t);
ST_ErrorCode_t STAud_SetPCMParams(STPCMInput_Handle_t Handle, U32 Frequency, U32 SampleSize, U32 NCh);
ST_ErrorCode_t STAud_GetPCMBufferSize(STPCMInput_Handle_t Handle, U32 *BufferSize);
ST_ErrorCode_t STAud_StartPCMInput(STPCMInput_Handle_t);
ST_ErrorCode_t STAud_StopPCMInput(STPCMInput_Handle_t);

ST_ErrorCode_t      STAud_PCMInputBufferParams(STPCMInput_Handle_t Handle, STAUD_BufferParams_t* DataParams_p);
ST_ErrorCode_t      STAud_PCMInputHandleEOF(STPCMInput_Handle_t Handle);
#endif

