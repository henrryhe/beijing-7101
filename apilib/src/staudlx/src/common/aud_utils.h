/*
 * Copyright (C) STMicroelectronics Ltd. 2002.
 *
 * All rights reserved.
 *
 * D program
 *
 */

#ifndef __AUDUTILS_H
    #define __AUDUTILS_H

    #ifndef ST_OSLINUX
        #include <stdlib.h>
        #include <stdio.h>
    #endif

    #include "stddefs.h"
    #include "stcommon.h"
    #include "stlite.h" /*include the proper os*/
    #include "stos.h"
    #include "stfdma.h"
    #include "blockmanager.h"
    #include "staud.h"


    #ifdef ST_OSWINCE
        #define AUD_TaskDelayMs(v)              Sleep(v)
        #define AUD_TaskDelay(v)                Sleep(v/1000)
        #define SNPRINTF                        _snprintf
    #else
        #ifndef ST_51XX
            #define AUD_TaskDelayMs(v)          STOS_TaskDelayUs((signed long long)v*1000)
            #define AUD_TaskDelay(v)            STOS_TaskDelay((signed long long)v)
            #define SNPRINTF                    snprintf
        #else
            #define AUD_TaskDelayMs(v)          STOS_TaskDelayUs((U32)v*1000)
            #define AUD_TaskDelay(v)            STOS_TaskDelay((U32)v)
            #define SNPRINTF                    snprintf
        #endif
    #endif

    /* 7200 LX selection code */
    #if defined(ST_7200)
        #define TOTAL_LX_PROCESSORS             2 /* Total number available of ST231 on 7200 */
        #define LX_TRANSFORMER_NAME_SUFFIX0     1 /* Audio lx 0 -> cpu id 1 for MME communication */
        #define LX_TRANSFORMER_NAME_SUFFIX1     3 /* Audio lx 1 -> cpu id 3 for MME communication */

        #define GetLxName(Index,LXString,OutString,OutStringSize)                                   \
        if(Index==0)                                                            \
            SNPRINTF(OutString, OutStringSize,"%s%d",(char*)LXString,LX_TRANSFORMER_NAME_SUFFIX0);      \
        else                                                                    \
            SNPRINTF(OutString, OutStringSize,"%s%d",(char*)LXString,LX_TRANSFORMER_NAME_SUFFIX1)

    #else
        #define TOTAL_LX_PROCESSORS             1 /* Total number available of ST231 on 71xx */
        #define GetLxName(Index,LXString,OutString,OutStringSize)                                   \
        SNPRINTF(OutString, OutStringSize, "%s", (char*)LXString)
    #endif // 7200

    /* Utils task stack size */
    #define DATAPROCESSER_STACK_SIZE            (1024*16)

    /* Utils task priority */
    #ifndef DATAPROCESSER_TASK_PRIORITY
        #define DATAPROCESSER_TASK_PRIORITY     6
    #endif

    enum DataProcesserIdentifier
    {
        BYTE_SWAPPER,
        BIT_CONVERTER,
        FRAME_PROCESSOR,
        FRAME_PROCESSOR1,
        MAX_DATA_PROCESSER_INSTANCE             = 4
    };



/*#define NUM_NODES_DECODER 6*/

    typedef U32 STDataProcesser_Handle_t;

    extern STDataProcesser_Handle_t DataProcesser_Handle[];

    typedef enum DataProcesserState_e
    {
        DATAPROCESSER_INIT,
        DATAPROCESSER_STARTING,
        DATAPROCESSER_GET_INPUT_BLOCK,  //DATAPROCESSER_STARTED
        DATAPROCESSER_GET_OUTPUT_BLOCK,
        DATAPROCESSER_STOPING,
        DATAPROCESSER_STOPPED,
        DATAPROCESSER_TERMINATE
    } DataProcesserState_t;


    typedef struct DataProcesserControl_s
    {
        enum DataProcesserIdentifier    dataProcesserIdentifier;
        DataProcesserState_t            dataProcesserCurrentState;
        DataProcesserState_t            dataProcesserNextState;

        task_t                          * dataProcesserTask_p;
        semaphore_t                     * dataProcesserTaskSem_p;
        semaphore_t                     * dataProcesserCmdTransitionSem_p;
        STOS_Mutex_t                    * LockControlStructure;

        ST_Partition_t                  * CPUPartition;
        STAVMEM_PartitionHandle_t       AVMEMPartition;

        STAVMEM_BlockHandle_t           dataProcesserNodeBlockHandle_p;
        STFDMA_GenericNode_t            * dataProcesserDmaNode_p;


        STMEMBLK_Handle_t               inputMemoryBlockManagerHandle0;
        STMEMBLK_Handle_t               inputMemoryBlockManagerHandle1;
        STMEMBLK_Handle_t               outputMemoryBlockManagerHandle;

        // Input Block Manager Handle to be used
        STMEMBLK_Handle_t               inputMemoryBlockManagerHandle;

        STAUD_StreamParams_t            StreamParams;
        MemoryBlock_t                   * inputMemoryBlock;
        MemoryBlock_t                   * outputMemoryBlock;
        FrameDeliverFunc_t              FrameDeliverFunc_fp;
        void                            *ClientInfo;
        BOOL                            EnableMSPPSupport;

        /*32 Bit Support*/
        STAud_Memory_Address_t          Input0;
        STAud_Memory_Address_t          Input1;
        STAud_Memory_Address_t          Output;
        STAud_Memory_Address_t          DataProcessorFDMANode;

    }DataProcesserControl_t;



    typedef struct DataProcesserControlBlock_s
    {
        STDataProcesser_Handle_t            handle;
        DataProcesserControl_t              dataProcesserControl;
        struct DataProcesserControlBlock_s  * next;
    }DataProcesserControlBlock_t;

    typedef struct DataProcesserInitParams_s
    {
        enum DataProcesserIdentifier    dataProcesserIdentifier;
        ST_Partition_t                  * CPUPartition;
        STAVMEM_PartitionHandle_t       AVMEMPartition;
        STMEMBLK_Handle_t               inputMemoryBlockManagerHandle0;
        STMEMBLK_Handle_t               inputMemoryBlockManagerHandle1;
        STMEMBLK_Handle_t               outputMemoryBlockManagerHandle;
        BOOL                            EnableMSPPSupport;
    }DataProcesserInitParams_t;

    ST_ErrorCode_t      STAud_DataProcesserInit (STDataProcesser_Handle_t *handle, DataProcesserInitParams_t *Param_p);
    ST_ErrorCode_t      STAud_DataProcesserStart (STDataProcesser_Handle_t handle);
    ST_ErrorCode_t      STAud_DataProcesserStop (STDataProcesser_Handle_t handle);
    ST_ErrorCode_t      STAud_DataProcesserTerminate (STDataProcesser_Handle_t handle);
    ST_ErrorCode_t      STAud_DataProcesserSetCmd (STDataProcesser_Handle_t handle, DataProcesserState_t State);
    ST_ErrorCode_t      STAud_DataProcesserUpdateCallback (STDataProcesser_Handle_t handle, FrameDeliverFunc_t FrameDeliverFunc_fp, void * clientInfo);
    ST_ErrorCode_t      STAud_DataProcessorSetStreamParams (STDataProcesser_Handle_t handle, STAUD_StreamParams_t * StreamParams_p);
    ST_ErrorCode_t      STAud_GetBufferParameter (STAUD_Handle_t Handle, STAUD_GetBufferParam_t *BufferParam);

    void                DataProcesserTask(void * tempDataProcesserControlBlock);

    ST_ErrorCode_t      ProcessData (MemoryBlock_t * sourceBlock, MemoryBlock_t * destinationBlock, enum DataProcesserIdentifier dataProcessingOperation, STAud_Memory_Address_t * Address_p, STAud_Memory_Address_t *Input, STAud_Memory_Address_t *Output);
    ST_ErrorCode_t      FDMAMemcpy (void *destinationAddress, void *sourceAddress, U32 length, U32 NumberOfBytes, U32 sourceStride, U32 destinationStride, STAud_Memory_Address_t  * FDMAAdress);
    ST_ErrorCode_t      DataProcessorCheckStateTransition (DataProcesserControl_t   * DataProcCtrl_p, DataProcesserState_t State);



    #define EPTS_INIT(p1)       {p1.BaseBit32 = 0; p1.BaseValue = 0; p1.Extension = 0;}

    #ifndef STAUD_REMOVE_CLKRV_SUPPORT
        BOOL EPTS_LATER_THAN    (STCLKRV_ExtendedSTC_t p1,      STCLKRV_ExtendedSTC_t p2);
        U32  EPTS_DIFF          (STCLKRV_ExtendedSTC_t p1,      STCLKRV_ExtendedSTC_t p2);
        void SubtractFromEPTS   (STCLKRV_ExtendedSTC_t * p1_p,  U32 value);
        void ADDToEPTS          (STCLKRV_ExtendedSTC_t * p1_p,  S32 value);
    #endif

#endif /*#define __AUDUTILS_H*/

