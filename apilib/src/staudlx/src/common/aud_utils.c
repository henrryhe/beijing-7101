/*
 * Copyright (C) STMicroelectronics Ltd. 2004.
 *
 * All rights reserved.
 *                      ------->SPDIF PLAYER
 *                      |
 * DECODER -------|
 *                      |
 *                      ------->PCM PLAYER
 */

//#define  STTBX_PRINT
#ifndef ST_OSLINUX
    #include "sttbx.h"
    #include "string.h"
#endif

#include "stos.h"
#include "stfdma.h"
#include "aud_utils.h"

#if defined(ST_OS21) || defined( ST_OSLINUX )
    #define BYTE_SWAP_CPU   1
#endif

#if !defined(FRAME_PROCESSOR_SUPPORT)
/* ---------------------------------------------------------------------------- */
/*                               Private Types                                  */
/* ---------------------------------------------------------------------------- */
    DataProcesserControlBlock_t * dataProcesserControlBlock = NULL;
    STDataProcesser_Handle_t DataProcesser_Handle[MAX_DATA_PROCESSER_INSTANCE];

    ST_ErrorCode_t      STAud_DataProcesserInit(STDataProcesser_Handle_t *handle,DataProcesserInitParams_t *Param_p)
    {
        ST_ErrorCode_t Error=ST_NO_ERROR;
        DataProcesserControlBlock_t * tempDataProcesserControlBlock = dataProcesserControlBlock;
        DataProcesserControlBlock_t * lastDataProcesserControlBlock = dataProcesserControlBlock;

        DataProcesserControl_t      * DataProcCtrl_p;
        U32 i = 0;
        STAVMEM_AllocBlockParams_t  AllocParams;


        while (tempDataProcesserControlBlock != NULL)
        {
            i++;
            lastDataProcesserControlBlock = tempDataProcesserControlBlock;
            tempDataProcesserControlBlock = tempDataProcesserControlBlock->next;
        }

        if (i >= MAX_DATA_PROCESSER_INSTANCE)
            return(ST_ERROR_NO_FREE_HANDLES);

        tempDataProcesserControlBlock = STOS_MemoryAllocate(Param_p->CPUPartition,sizeof(DataProcesserControlBlock_t));
        if(!tempDataProcesserControlBlock)
        {
            return ST_ERROR_NO_MEMORY;
        }

        memset (tempDataProcesserControlBlock, 0, sizeof(DataProcesserControlBlock_t));
        *handle = (STDataProcesser_Handle_t)tempDataProcesserControlBlock;

        /*only the control block*/
        DataProcCtrl_p = &(tempDataProcesserControlBlock->dataProcesserControl);

        // Get params from init params
        DataProcCtrl_p->dataProcesserIdentifier                 = Param_p->dataProcesserIdentifier;
        DataProcCtrl_p->CPUPartition                            = Param_p->CPUPartition;
        DataProcCtrl_p->inputMemoryBlockManagerHandle0          = Param_p->inputMemoryBlockManagerHandle0;
        DataProcCtrl_p->inputMemoryBlockManagerHandle1          = Param_p->inputMemoryBlockManagerHandle1;
        DataProcCtrl_p->outputMemoryBlockManagerHandle          = Param_p->outputMemoryBlockManagerHandle;
        DataProcCtrl_p->AVMEMPartition                          = Param_p->AVMEMPartition;
        DataProcCtrl_p->EnableMSPPSupport                       = Param_p->EnableMSPPSupport;

        // Set default params for this data processer
        DataProcCtrl_p->dataProcesserCurrentState               = DATAPROCESSER_INIT;
        DataProcCtrl_p->dataProcesserNextState                  = DATAPROCESSER_INIT;
        DataProcCtrl_p->inputMemoryBlock                        = NULL;
        DataProcCtrl_p->outputMemoryBlock                       = NULL;

    /*  32 Bit Support */
        {
            STAUD_BufferParams_t  BufferParams;
            if(Param_p->inputMemoryBlockManagerHandle0)
            {
                Error|=MemPool_GetBufferParams(Param_p->inputMemoryBlockManagerHandle0,&BufferParams);
                if(Error!=ST_NO_ERROR)
                {
                    return Error;
                }
                DataProcCtrl_p->Input0.BaseUnCachedVirtualAddress   = (U32*)BufferParams.BufferBaseAddr_p;
                DataProcCtrl_p->Input0.BasePhyAddress               = (U32*)STOS_VirtToPhys(DataProcCtrl_p->Input0.BaseUnCachedVirtualAddress);
                DataProcCtrl_p->Input0.BaseCachedVirtualAddress     = NULL;
            }

            if(Param_p->inputMemoryBlockManagerHandle1)
            {
                Error   |= MemPool_GetBufferParams(Param_p->inputMemoryBlockManagerHandle1,&BufferParams);
                if(Error!=ST_NO_ERROR)
                {
                    return Error;
                }
                DataProcCtrl_p->Input1.BaseUnCachedVirtualAddress   = (U32*)BufferParams.BufferBaseAddr_p;
                DataProcCtrl_p->Input1.BasePhyAddress               = (U32*)STOS_VirtToPhys(DataProcCtrl_p->Input1.BaseUnCachedVirtualAddress);
                DataProcCtrl_p->Input1.BaseCachedVirtualAddress     = NULL;
            }

            if(Param_p->outputMemoryBlockManagerHandle)
            {

                Error   |= MemPool_GetBufferParams(Param_p->outputMemoryBlockManagerHandle,&BufferParams);
                if(Error != ST_NO_ERROR)
                {
                    return Error;
                }

                DataProcCtrl_p->Output.BaseUnCachedVirtualAddress   = (U32*)BufferParams.BufferBaseAddr_p;
                DataProcCtrl_p->Output.BasePhyAddress               = (U32*)STOS_VirtToPhys(DataProcCtrl_p->Output.BaseUnCachedVirtualAddress);
                DataProcCtrl_p->Output.BaseCachedVirtualAddress     = NULL;
            }

        }


        AllocParams.PartitionHandle             = DataProcCtrl_p->AVMEMPartition;
        AllocParams.Alignment                   = 32;
        AllocParams.AllocMode                   = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AllocParams.ForbiddenRangeArray_p       = NULL;
        AllocParams.ForbiddenBorderArray_p      = NULL;
        AllocParams.NumberOfForbiddenRanges     = 0;
        AllocParams.NumberOfForbiddenBorders    = 0;
        AllocParams.Size                        = sizeof(STFDMA_GenericNode_t);

        Error = STAVMEM_AllocBlock(&AllocParams, &(DataProcCtrl_p->dataProcesserNodeBlockHandle_p));
        if(Error != ST_NO_ERROR)
            return Error;

        Error = STAVMEM_GetBlockAddress(DataProcCtrl_p->dataProcesserNodeBlockHandle_p, (void *)(&(DataProcCtrl_p->dataProcesserDmaNode_p)));
        if(Error != ST_NO_ERROR)
            return Error;

        #if defined(ST_7109)
            DataProcCtrl_p->dataProcesserDmaNode_p->Node.NodeControl.Reserved1 = 0;
            if (DataProcCtrl_p->EnableMSPPSupport == TRUE)
            {
                DataProcCtrl_p->dataProcesserDmaNode_p->Node.NodeControl.Secure = 1;
            }
            else
            {
                DataProcCtrl_p->dataProcesserDmaNode_p->Node.NodeControl.Secure = 0;
            }
        #endif

        DataProcCtrl_p->DataProcessorFDMANode.BaseUnCachedVirtualAddress    = (U32*)DataProcCtrl_p->dataProcesserDmaNode_p;
        DataProcCtrl_p->DataProcessorFDMANode.BasePhyAddress                = (U32*)STOS_VirtToPhys(DataProcCtrl_p->dataProcesserDmaNode_p);
        DataProcCtrl_p->dataProcesserTaskSem_p                              = STOS_SemaphoreCreateFifo(NULL,0);
        DataProcCtrl_p->LockControlStructure                                = STOS_MutexCreateFifo();
        DataProcCtrl_p->dataProcesserCmdTransitionSem_p                     = STOS_SemaphoreCreateFifo(NULL,0);


        STOS_TaskCreate(DataProcesserTask,(void *)tempDataProcesserControlBlock,NULL,
                                            DATAPROCESSER_STACK_SIZE, NULL,NULL,&DataProcCtrl_p->dataProcesserTask_p,
                                            NULL,DATAPROCESSER_TASK_PRIORITY,"DataProcesserTask",0);


        if (DataProcCtrl_p->dataProcesserTask_p == NULL)
        {
            return ST_ERROR_NO_MEMORY;
        }

        tempDataProcesserControlBlock->next = NULL;

        if (dataProcesserControlBlock == NULL)
            dataProcesserControlBlock = tempDataProcesserControlBlock;
        else
            lastDataProcesserControlBlock->next = tempDataProcesserControlBlock;

        return (Error);
    }


    ST_ErrorCode_t      STAud_DataProcesserTerminate(STDataProcesser_Handle_t handle)
    {
        ST_ErrorCode_t Error=ST_NO_ERROR;
        DataProcesserControlBlock_t * tempDataProcesserControlBlock = dataProcesserControlBlock, * lastDataProcesserControlBlock= dataProcesserControlBlock;
        STAVMEM_FreeBlockParams_t FreeBlockParams;
        DataProcesserControl_t      * DataProcCtrl_p;

        while (tempDataProcesserControlBlock != (DataProcesserControlBlock_t *)handle)
        {
            lastDataProcesserControlBlock = tempDataProcesserControlBlock;
            tempDataProcesserControlBlock = tempDataProcesserControlBlock->next;
        }

        if (tempDataProcesserControlBlock == NULL)
            return (ST_ERROR_INVALID_HANDLE);

        /*take the control block*/
        DataProcCtrl_p = &(tempDataProcesserControlBlock->dataProcesserControl);

        Error |= STOS_SemaphoreDelete(NULL,DataProcCtrl_p->dataProcesserTaskSem_p);
        Error |= STOS_SemaphoreDelete(NULL,DataProcCtrl_p->dataProcesserCmdTransitionSem_p);
        Error |= STOS_MutexDelete(DataProcCtrl_p->LockControlStructure);

        FreeBlockParams.PartitionHandle     = DataProcCtrl_p->AVMEMPartition;
        Error |= STAVMEM_FreeBlock(&FreeBlockParams, &(DataProcCtrl_p->dataProcesserNodeBlockHandle_p));

        lastDataProcesserControlBlock->next = tempDataProcesserControlBlock->next;

        // deallocate control block
        if (tempDataProcesserControlBlock == dataProcesserControlBlock)
            dataProcesserControlBlock       = dataProcesserControlBlock->next;//NULL;

        STOS_MemoryDeallocate(DataProcCtrl_p->CPUPartition, tempDataProcesserControlBlock);

        return (Error);
    }


    U32 inputblockCount = 0, outputblockCount = 0;

    void DataProcesserTask(void * tempDataProcesserControlBlock)
    {
        DataProcesserControlBlock_t * tempDataProcesserControlBlock_p = (DataProcesserControlBlock_t *)tempDataProcesserControlBlock;
        DataProcesserControl_t      * dataProcesserControl      = &(tempDataProcesserControlBlock_p->dataProcesserControl);
        BOOL                        waitForSemaphore = TRUE;
        ProducerParams_t            producerParam_s;
        ConsumerParams_t            consumerParam_s;

        STOS_TaskEnter(tempDataProcesserControlBlock);

        while (1)
        {
            if (waitForSemaphore)
                STOS_SemaphoreWait(dataProcesserControl->dataProcesserTaskSem_p);

            waitForSemaphore = FALSE;


            switch (dataProcesserControl->dataProcesserCurrentState)
            {
                case DATAPROCESSER_INIT:
                    if (dataProcesserControl->dataProcesserNextState == DATAPROCESSER_STOPPED)
                    {
                        dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_STOPING;
                    }

                    if (dataProcesserControl->dataProcesserNextState == DATAPROCESSER_GET_INPUT_BLOCK)
                    {
                        dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_STARTING;
                    }

                    if (dataProcesserControl->dataProcesserNextState == DATAPROCESSER_TERMINATE)
                    {
                        dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_TERMINATE;
                    }

                    break;

                case DATAPROCESSER_STARTING:
                    producerParam_s.Handle = (U32)tempDataProcesserControlBlock_p;
                    producerParam_s.sem    = dataProcesserControl->dataProcesserTaskSem_p;
                    // Register to memory block manager as a producer
                    switch(dataProcesserControl->dataProcesserIdentifier)
                    {
                        case FRAME_PROCESSOR:
                        case FRAME_PROCESSOR1:
                            //No output block manager
                            break;

                        default:
                            if (MemPool_RegProducer(dataProcesserControl->outputMemoryBlockManagerHandle, producerParam_s) != ST_NO_ERROR)
                            {
                                STTBX_Print(("Error in regitsering to BM in data processor\n"));
                            }
                            break;
                    }

                    consumerParam_s.Handle = (U32)tempDataProcesserControlBlock;
                    consumerParam_s.sem    = dataProcesserControl->dataProcesserTaskSem_p;
                    switch(dataProcesserControl->dataProcesserIdentifier)
                    {
                        case BYTE_SWAPPER:
                            dataProcesserControl->inputMemoryBlockManagerHandle = dataProcesserControl->inputMemoryBlockManagerHandle0;
                            break;

                        default:
                            dataProcesserControl->inputMemoryBlockManagerHandle = dataProcesserControl->inputMemoryBlockManagerHandle0;
                            break;
                    }

                    // Register to memory block manager as a consumer
                    if (MemPool_RegConsumer(dataProcesserControl->inputMemoryBlockManagerHandle, consumerParam_s) != ST_NO_ERROR)
                    {
                        STTBX_Print(("Error in regitsering to BM in data processor\n"));
                    }

                    dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_GET_INPUT_BLOCK;
                    STOS_SemaphoreSignal(dataProcesserControl->dataProcesserCmdTransitionSem_p);

                    break;

                case DATAPROCESSER_GET_INPUT_BLOCK:
                    if (dataProcesserControl->inputMemoryBlock != NULL)
                    {
                        STTBX_Print(("Error in BYte swapper\n"));
                    }

                    if (MemPool_GetIpBlk(dataProcesserControl->inputMemoryBlockManagerHandle, (U32 *)(& dataProcesserControl->inputMemoryBlock), (U32)tempDataProcesserControlBlock_p) != ST_NO_ERROR)
                    {
                        if (dataProcesserControl->dataProcesserNextState == DATAPROCESSER_STOPPED)
                        {
                            dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_STOPING;
                        }
                        else
                            waitForSemaphore = TRUE;
                        break;
                    }

                    inputblockCount++;

                    if (inputblockCount%499 == 0)
                    {
                        STTBX_Print(("Input Block %d\n",inputblockCount));
                    }

                    if (dataProcesserControl->dataProcesserNextState    == DATAPROCESSER_STOPPED)
                    {
                        dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_STOPING;
                    }
                    else
                    {
                        dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_GET_OUTPUT_BLOCK;
                    }

                    break;

                case DATAPROCESSER_GET_OUTPUT_BLOCK:

                    if (dataProcesserControl->dataProcesserIdentifier == FRAME_PROCESSOR)
                    {

                        if(!(dataProcesserControl->inputMemoryBlock->Flags & PTS_VALID))
                        {
                            dataProcesserControl->inputMemoryBlock->Data[PTS_OFFSET]        = 0;
                            dataProcesserControl->inputMemoryBlock->Data[PTS33_OFFSET]      = 0;
                        }

                        if(dataProcesserControl->FrameDeliverFunc_fp)
                        {
                            BufferMetaData_t bufferData;
                            bufferData.PTS      = dataProcesserControl->inputMemoryBlock->Data[PTS_OFFSET];
                            bufferData.PTS33    = dataProcesserControl->inputMemoryBlock->Data[PTS33_OFFSET];

                            if (dataProcesserControl->inputMemoryBlock->Flags & WMAE_ASF_HEADER_VALID)
                            {
                                bufferData.isWMAEncASFHeaderData    = TRUE;
                            }
                            else
                            {
                                bufferData.isWMAEncASFHeaderData    = FALSE;
                            }

                            if (dataProcesserControl->inputMemoryBlock->Flags & DUMMYBLOCK_VALID)
                            {
                                bufferData.isDummyBuffer            = TRUE;
                            }
                            else
                            {
                                bufferData.isDummyBuffer            = FALSE;
                            }

                            dataProcesserControl->FrameDeliverFunc_fp((U8 *)dataProcesserControl->inputMemoryBlock->MemoryStart,
                                                                    dataProcesserControl->inputMemoryBlock->Size,
                                                                    bufferData,
                                                                    dataProcesserControl->ClientInfo);

                        }
                    }
                    else if (dataProcesserControl->dataProcesserIdentifier == FRAME_PROCESSOR1)
                    {

                        if(!(dataProcesserControl->inputMemoryBlock->Flags & PTS_VALID))
                        {
                            dataProcesserControl->inputMemoryBlock->Data[PTS_OFFSET]        = 0;
                            dataProcesserControl->inputMemoryBlock->Data[PTS33_OFFSET]      = 0;
                        }

                        if(dataProcesserControl->FrameDeliverFunc_fp)
                        {
                            BufferMetaData_t bufferData;
                            bufferData.PTS = dataProcesserControl->inputMemoryBlock->Data[PTS_OFFSET];
                            bufferData.PTS33 = dataProcesserControl->inputMemoryBlock->Data[PTS33_OFFSET];

                            if (dataProcesserControl->inputMemoryBlock->Flags & WMAE_ASF_HEADER_VALID)
                            {
                                bufferData.isWMAEncASFHeaderData    = TRUE;
                            }
                            else
                            {
                                bufferData.isWMAEncASFHeaderData    = FALSE;
                            }

                            if (dataProcesserControl->inputMemoryBlock->Flags & DUMMYBLOCK_VALID)
                            {
                                bufferData.isDummyBuffer            = TRUE;
                            }
                            else
                            {
                                bufferData.isDummyBuffer            = FALSE;
                            }

                            dataProcesserControl->FrameDeliverFunc_fp((U8 *)dataProcesserControl->inputMemoryBlock->MemoryStart,
                                                                    dataProcesserControl->inputMemoryBlock->Size,
                                                                    bufferData,
                                                                    dataProcesserControl->ClientInfo);

                        }
                    }
                    else
                    {
                        if (MemPool_GetOpBlk(dataProcesserControl->outputMemoryBlockManagerHandle, (U32 *)(& dataProcesserControl->outputMemoryBlock)) != ST_NO_ERROR)
                        {
                            if (dataProcesserControl->dataProcesserNextState == DATAPROCESSER_STOPPED)
                            {
                                dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_STOPING;
                            }
                            else
                            {
                                waitForSemaphore = TRUE;
                            }
                            break;
                        }
                        outputblockCount++;
                        if (outputblockCount%499 == 0)
                        {
                            STTBX_Print(("Output Block %d\n",outputblockCount));
                        }

                        if (ProcessData(dataProcesserControl->inputMemoryBlock, dataProcesserControl->outputMemoryBlock, dataProcesserControl->dataProcesserIdentifier, &dataProcesserControl->DataProcessorFDMANode, &dataProcesserControl->Input0,&dataProcesserControl->Output) != ST_NO_ERROR)
                        {
                            STTBX_Print(("Error in Process data \n"));
                        }

                        if (MemPool_PutOpBlk(dataProcesserControl->outputMemoryBlockManagerHandle, (U32 *)(& dataProcesserControl->outputMemoryBlock)) != ST_NO_ERROR)
                        {
                            STTBX_Print(("Error in putting output memory block\n"));
                        }
                    }

                    if (MemPool_FreeIpBlk(dataProcesserControl->inputMemoryBlockManagerHandle, (U32 *)(& dataProcesserControl->inputMemoryBlock), (U32)tempDataProcesserControlBlock_p) != ST_NO_ERROR)
                    {
                        STTBX_Print(("Error in freeing i/p memory block\n"));
                    }

                    dataProcesserControl->inputMemoryBlock      = NULL;
                    dataProcesserControl->outputMemoryBlock     = NULL;


                    if (dataProcesserControl->dataProcesserNextState == DATAPROCESSER_STOPPED)
                    {
                        dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_STOPING;
                    }
                    else
                    {
                        dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_GET_INPUT_BLOCK;
                    }

                    break;

                case DATAPROCESSER_STOPING:
                    if (dataProcesserControl->inputMemoryBlock != NULL)
                    {
                        if (MemPool_FreeIpBlk(dataProcesserControl->inputMemoryBlockManagerHandle, (U32 *)(& dataProcesserControl->inputMemoryBlock), (U32)tempDataProcesserControlBlock_p) != ST_NO_ERROR)
                        {
                            STTBX_Print(("Error in freeing i/p memory block\n"));
                        }
                    }

                    dataProcesserControl->inputMemoryBlock = NULL;
                    if (MemPool_UnRegConsumer(dataProcesserControl->inputMemoryBlockManagerHandle, (U32)tempDataProcesserControlBlock_p) != ST_NO_ERROR)
                    {
                        STTBX_Print(("Error in unregistering BM from spdif player\n"));
                    }

                    switch(dataProcesserControl->dataProcesserIdentifier)
                    {
                        case FRAME_PROCESSOR:
                        case FRAME_PROCESSOR1:
                            //No output block manager
                            dataProcesserControl->outputMemoryBlock = NULL;
                            break;

                        default:
                            if (dataProcesserControl->outputMemoryBlock != NULL)
                            {
                                if (MemPool_PutOpBlk(dataProcesserControl->outputMemoryBlockManagerHandle, (U32 *)(& dataProcesserControl->outputMemoryBlock)) != ST_NO_ERROR)
                                {
                                    STTBX_Print(("Error in putting output memory block\n"));
                                }
                            }



                            if (MemPool_UnRegProducer(dataProcesserControl->outputMemoryBlockManagerHandle, (U32)(tempDataProcesserControlBlock_p)) != ST_NO_ERROR)
                            {
                                STTBX_Print(("Error in unregistering BM in data processor\n"));
                            }
                            break;
                    }

                    dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_STOPPED;

                    #ifdef STAUDLX_ALSA_SUPPORT
                        if(dataProcesserControl->StreamParams.StreamContent == STAUD_STREAM_CONTENT_ALSA)
                            ;
                        else
                        {
                            STOS_SemaphoreSignal(dataProcesserControl->dataProcesserCmdTransitionSem_p);
                        }
                    #else
                        STOS_SemaphoreSignal(dataProcesserControl->dataProcesserCmdTransitionSem_p);
                    #endif

                    break;

                case DATAPROCESSER_STOPPED:
                    if (dataProcesserControl->dataProcesserNextState == DATAPROCESSER_GET_INPUT_BLOCK ||
                        dataProcesserControl->dataProcesserNextState == DATAPROCESSER_TERMINATE)
                    {
                        if (dataProcesserControl->dataProcesserNextState == DATAPROCESSER_GET_INPUT_BLOCK)
                        {
                            dataProcesserControl->dataProcesserCurrentState = DATAPROCESSER_STARTING;
                            outputblockCount = 0;
                        }
                        else if (dataProcesserControl->dataProcesserNextState == DATAPROCESSER_TERMINATE)
                        {
                            dataProcesserControl->dataProcesserCurrentState = dataProcesserControl->dataProcesserNextState;
                            STOS_SemaphoreSignal(dataProcesserControl->dataProcesserCmdTransitionSem_p);
                        }
                    }
                    else
                    {
                        waitForSemaphore = TRUE;
                    }

                    break;

                case DATAPROCESSER_TERMINATE:

                    if (dataProcesserControl->inputMemoryBlock != NULL)
                    {
                        STTBX_Print(("Freeing data processing input block in terminate\n"));
                        if (MemPool_FreeIpBlk(dataProcesserControl->inputMemoryBlockManagerHandle, (U32 *)(& dataProcesserControl->inputMemoryBlock), (U32)tempDataProcesserControlBlock_p) != ST_NO_ERROR)
                        {
                            STTBX_Print(("Error in freeing i/p memory block\n"));
                        }
                    dataProcesserControl->inputMemoryBlock = NULL;
                    }

                    switch(dataProcesserControl->dataProcesserIdentifier)
                    {
                        case FRAME_PROCESSOR:
                        case FRAME_PROCESSOR1:
                            //No output block manager
                            dataProcesserControl->outputMemoryBlock = NULL;
                            break;

                        default:
                            if (dataProcesserControl->outputMemoryBlock != NULL)
                            {
                                STTBX_Print(("Freeing data processing output block in terminate\n"));
                                if (MemPool_PutOpBlk(dataProcesserControl->outputMemoryBlockManagerHandle, (U32 *)(& dataProcesserControl->outputMemoryBlock)) != ST_NO_ERROR)
                                {
                                    STTBX_Print(("Error in putting output memory block\n"));
                                }
                                dataProcesserControl->outputMemoryBlock = NULL;
                            }
                            break;
                    }




                    if (STAud_DataProcesserTerminate((STDataProcesser_Handle_t)tempDataProcesserControlBlock_p) != ST_NO_ERROR)
                    {
                        STTBX_Print(("Error in data processor termination"));
                    }

                    // Exit data processor task
                    STOS_TaskExit(tempDataProcesserControlBlock);
                    #if defined( ST_OSLINUX )
                        return ;
                    #else
                    task_exit(1);
                    #endif

                default:
                    break;
            }

        }



    }
/*-------------------------------------------------------------------------
 * Function : STAud_GetBufferParameter
 *            This function returns the buffer parameters (size and start
 *            address) for any buffer given the handle for the related
 *            object (eg.Frame Processor)
 *
 * Input    : Handle, BufferParam (pointer to STAUD_GetBufferParam_t)
 * Output   : Length (buffersize) and StartBase (start address)
 * Return   : error code(ST_ErrorCode_t) if error, ST_NO_ERROR if success
 * -------------------------------------------------------------------------
*/

    ST_ErrorCode_t STAud_GetBufferParameter(STAUD_Handle_t Handle, STAUD_GetBufferParam_t *BufferParam)
    {
        ST_ErrorCode_t Error = ST_NO_ERROR;
        STAUD_BufferParams_t AUD_BufferParam;
        DataProcesserControlBlock_t * tempDataProcesserControlBlock = dataProcesserControlBlock;
        DataProcesserControl_t          *   DataProcCtrl_p;

        if(Handle==(U32)NULL)
        {
            return ST_ERROR_INVALID_HANDLE;
        }

        STTBX_Print(("STAud_GetBufferParameter() Called \n"));

        while (tempDataProcesserControlBlock != (DataProcesserControlBlock_t *)Handle)
        {
            //lastDataProcesserControlBlock = tempDataProcesserControlBlock;
            tempDataProcesserControlBlock = tempDataProcesserControlBlock->next;
        }

        if (tempDataProcesserControlBlock == NULL)
            return ST_ERROR_INVALID_HANDLE;

        DataProcCtrl_p  = &(tempDataProcesserControlBlock->dataProcesserControl);

        Error           = MemPool_GetBufferParams(DataProcCtrl_p->inputMemoryBlockManagerHandle0,&(AUD_BufferParam));

        if(Error != ST_NO_ERROR)
        {
            STTBX_Print(("Either Invalid Handle or No memory available! \n "));
            return Error;
        }
        /* Return Buffer Param */


#ifdef ST_OSLINUX
        BufferParam->StartBase  = DataProcCtrl_p->Input0.BasePhyAddress;
#else
        BufferParam->StartBase  = AUD_BufferParam.BufferBaseAddr_p;
#endif
        BufferParam->Length     = AUD_BufferParam.BufferSize;

        STTBX_Print(("The value of AUD_BufferParam.BufferSize is %x \n",AUD_BufferParam.BufferSize));
        STTBX_Print(("The value of AUD_BufferParam.BufferBaseAddr_p is %x \n",(U32 *)AUD_BufferParam.BufferBaseAddr_p));

        return Error;
    }

    ST_ErrorCode_t      STAud_DataProcesserStart(STDataProcesser_Handle_t handle)
    {
        ST_ErrorCode_t Error=ST_NO_ERROR;
        DataProcesserControlBlock_t * tempDataProcesserControlBlock = dataProcesserControlBlock;
        DataProcesserControl_t          *   DataProcCtrl_p;

        while (tempDataProcesserControlBlock != (DataProcesserControlBlock_t *)handle)
        {
            tempDataProcesserControlBlock   = tempDataProcesserControlBlock->next;
        }

        if (tempDataProcesserControlBlock == NULL)
            return (ST_ERROR_INVALID_HANDLE);

        DataProcCtrl_p = &(tempDataProcesserControlBlock->dataProcesserControl);

        if (DataProcCtrl_p->dataProcesserCurrentState == DATAPROCESSER_INIT ||
            DataProcCtrl_p->dataProcesserCurrentState == DATAPROCESSER_STOPPED)
        {
            DataProcCtrl_p->dataProcesserNextState = DATAPROCESSER_GET_INPUT_BLOCK;
            STOS_SemaphoreSignal(DataProcCtrl_p->dataProcesserTaskSem_p);
        }
        else
            return(ST_ERROR_BAD_PARAMETER);



        return (Error);
    }

    ST_ErrorCode_t      STAud_DataProcesserStop(STDataProcesser_Handle_t handle)
    {
        ST_ErrorCode_t Error    = ST_NO_ERROR;
        DataProcesserControlBlock_t * tempDataProcesserControlBlock = dataProcesserControlBlock;
        DataProcesserControl_t          *   DataProcCtrl_p;

        while (tempDataProcesserControlBlock != (DataProcesserControlBlock_t *)handle)
        {
            tempDataProcesserControlBlock   = tempDataProcesserControlBlock->next;
        }

        if (tempDataProcesserControlBlock == NULL)
            return (ST_ERROR_INVALID_HANDLE);

        DataProcCtrl_p                          = &(tempDataProcesserControlBlock->dataProcesserControl);
        DataProcCtrl_p->dataProcesserNextState  = DATAPROCESSER_STOPPED;
        STOS_SemaphoreSignal(DataProcCtrl_p->dataProcesserTaskSem_p);

        return (Error);
    }


    ST_ErrorCode_t  STAud_DataProcesserSetCmd(STDataProcesser_Handle_t handle, DataProcesserState_t State)
    {
        ST_ErrorCode_t Error    = ST_NO_ERROR;
        DataProcesserControlBlock_t * tempDataProcesserControlBlock = dataProcesserControlBlock;
        task_t  * dataProcesserTask_p                               = NULL;
        DataProcesserControl_t          *   DataProcCtrl_p;

        while (tempDataProcesserControlBlock != (DataProcesserControlBlock_t *)handle)
        {
            tempDataProcesserControlBlock   = tempDataProcesserControlBlock->next;
        }

        if (tempDataProcesserControlBlock == NULL)
            return (ST_ERROR_INVALID_HANDLE);

        DataProcCtrl_p  = &(tempDataProcesserControlBlock->dataProcesserControl);

        Error           = DataProcessorCheckStateTransition(DataProcCtrl_p, State);
        if ( Error != ST_NO_ERROR)
        {
            return (Error);
        }

        if (State == DATAPROCESSER_TERMINATE)
        {
            dataProcesserTask_p = DataProcCtrl_p->dataProcesserTask_p;
        }


        STOS_MutexLock(DataProcCtrl_p->LockControlStructure);
        DataProcCtrl_p->dataProcesserNextState  = State;
        STOS_MutexRelease(DataProcCtrl_p->LockControlStructure);

        STOS_SemaphoreSignal(DataProcCtrl_p->dataProcesserTaskSem_p);

        if (State == DATAPROCESSER_TERMINATE)
        {

            STOS_TaskWait(&dataProcesserTask_p,TIMEOUT_INFINITY);
            if (STOS_TaskDelete(dataProcesserTask_p,NULL,NULL,NULL) != ST_NO_ERROR)
            {
                STTBX_Print(("Failed to delete data processor task\n"));
            }

        }
        else
        {
            #ifdef STAUDLX_ALSA_SUPPORT
                if(DataProcCtrl_p->StreamParams.StreamContent
                    == STAUD_STREAM_CONTENT_ALSA)
                {
                    /*DO NOT WAIT FOR SEMAPHORE IN CASE OF ALSA STOP*/
                    if(State == DATAPROCESSER_STOPPED);

                    else
                        STOS_SemaphoreWait(DataProcCtrl_p->dataProcesserCmdTransitionSem_p);

                }
                else
                {
                    STOS_SemaphoreWait(DataProcCtrl_p->dataProcesserCmdTransitionSem_p);
                }

            #else
                STOS_SemaphoreWait(DataProcCtrl_p->dataProcesserCmdTransitionSem_p);
            #endif

        }

        return (Error);

    }

    ST_ErrorCode_t  STAud_DataProcessorSetStreamParams(STDataProcesser_Handle_t handle, STAUD_StreamParams_t * StreamParams_p)
    {
        ST_ErrorCode_t Error    = ST_NO_ERROR;
        DataProcesserControlBlock_t * tempDataProcesserControlBlock = dataProcesserControlBlock/*, * lastDataProcesserControlBlock = dataProcesserControlBlock*/;

        DataProcesserControl_t          *   DataProcCtrl_p;

        while (tempDataProcesserControlBlock != (DataProcesserControlBlock_t *)handle)
        {
            tempDataProcesserControlBlock = tempDataProcesserControlBlock->next;
        }

        if (tempDataProcesserControlBlock == NULL)
            return (ST_ERROR_INVALID_HANDLE);

        DataProcCtrl_p                  = &(tempDataProcesserControlBlock->dataProcesserControl);

        STOS_MutexLock(DataProcCtrl_p->LockControlStructure);
        DataProcCtrl_p->StreamParams    = *(StreamParams_p);
        STOS_MutexRelease(DataProcCtrl_p->LockControlStructure);

        return (Error);

    }

    ST_ErrorCode_t      STAud_DataProcesserUpdateCallback(STDataProcesser_Handle_t handle,FrameDeliverFunc_t FrameDeliverFunc_fp, void * clientInfo)
    {
        ST_ErrorCode_t Error    = ST_NO_ERROR;
        DataProcesserControlBlock_t * tempDataProcesserControlBlock = dataProcesserControlBlock/*, * lastDataProcesserControlBlock = dataProcesserControlBlock*/;
        DataProcesserControl_t          *   DataProcCtrl_p;

        while (tempDataProcesserControlBlock != (DataProcesserControlBlock_t *)handle)
        {
            tempDataProcesserControlBlock = tempDataProcesserControlBlock->next;
        }

        if (tempDataProcesserControlBlock == NULL)
            return (ST_ERROR_INVALID_HANDLE);

        DataProcCtrl_p = &(tempDataProcesserControlBlock->dataProcesserControl);

        if (DataProcCtrl_p->dataProcesserCurrentState == DATAPROCESSER_INIT ||
            DataProcCtrl_p->dataProcesserCurrentState == DATAPROCESSER_STOPPED)
        {
            DataProcCtrl_p->FrameDeliverFunc_fp     = FrameDeliverFunc_fp;
            DataProcCtrl_p->ClientInfo              = clientInfo;
        }
        else
            return(ST_ERROR_BAD_PARAMETER);

        return (Error);
    }


    ST_ErrorCode_t      ProcessData (MemoryBlock_t * sourceBlock, MemoryBlock_t * destinationBlock, enum DataProcesserIdentifier dataProcessingOperation, STAud_Memory_Address_t * Address_p,STAud_Memory_Address_t *Input, STAud_Memory_Address_t *Output)
    {
        U32 SourcePhyAddress;
        U32 DestinationPhyAddress;

        SourcePhyAddress        = (U32)STOS_AddressTranslate(Input->BasePhyAddress,Input->BaseUnCachedVirtualAddress,sourceBlock->MemoryStart);
        DestinationPhyAddress   = (U32)STOS_AddressTranslate(Output->BasePhyAddress,Output->BaseUnCachedVirtualAddress,destinationBlock->MemoryStart);
        switch (dataProcessingOperation)
        {
            case BYTE_SWAPPER:
                #if defined(ST_7100)||defined (ST_7109)||defined(ST_5525)
                    #if BYTE_SWAP_CPU
                        {
                            #if defined (ST_OSWINCE)
                                U8 *src,*dst;
                                U32 i,size;

                                size = sourceBlock->Size;
                                src = (U8 *)sourceBlock->MemoryStart;
                                dst = (U8 *)destinationBlock->MemoryStart;

                                for(i=0;i<size;i+=2)
                                {
                                    *dst = *(src + 1);
                                    *(dst + 1) = *src;

                                    src+=2;
                                    dst+=2;
                                }

                            #else
                                U16 *src,*dst;
                                U32 i,size;
                                src = (U16 *)sourceBlock->MemoryStart;
                                dst = (U16 *)destinationBlock->MemoryStart;
                                size = (sourceBlock->Size >> 1);

                                for ( i = 0 ; i < size; i++ )
                                {
                                    __asm__("swap.b %1, %0" : "=r" (dst[i]): "r" (src[i]));
                                }
                            #endif
                            }
                        #else
                            FDMAMemcpy((void *)((U32)DestinationPhyAddress+ 1),(void *)SourcePhyAddress, 1, ((sourceBlock->Size)/2), 2, 2, Address_p);
                            FDMAMemcpy( (void *)DestinationPhyAddress, (void *)((U32)SourcePhyAddress + 1), 1, ((sourceBlock->Size)/2), 2, 2, Address_p);
                        #endif
                    destinationBlock->Size = sourceBlock->Size;
                #endif

                break;

            case BIT_CONVERTER:

                #if defined(ST_7100)||defined (ST_7109)
                    FDMAMemcpy((void *)DestinationPhyAddress,(void *)((U32)SourcePhyAddress + 2), 2, ((sourceBlock->Size)/2), 4, 2, Address_p);
                    destinationBlock->Size = (sourceBlock->Size)/2;

                #elif defined(ST_5525)

                {
                        U8 *src,*dst;
                        U32 i,size;

                        size = sourceBlock->Size;
                        src = (U8 *)sourceBlock->MemoryStart;
                        dst = (U8 *)destinationBlock->MemoryStart;

                        for(i=0;i<=size;i+=4)
                        {
                            *dst = *(src + 2);
                            *(dst + 1) = *(src+3);

                            src+=4;
                            dst+=2;
                        }
                }
                destinationBlock->Size = (sourceBlock->Size)/2;

                #elif defined(ST_8010)
                    memcpy((void *)destinationBlock->MemoryStart,(void *)((U32)sourceBlock->MemoryStart ),(U32)(sourceBlock->Size));
                    destinationBlock->Size = (sourceBlock->Size);
                #endif

                break;

            case FRAME_PROCESSOR:
            case FRAME_PROCESSOR1:

                break;

            default:
                break;
        }

        if(sourceBlock->Flags)
        {
            U32 i;
            destinationBlock->Flags     = sourceBlock->Flags;
            destinationBlock->AppData_p = sourceBlock->AppData_p;

            /*copy all data forward*/
            for(i=0;i<32;i++)
            {
                destinationBlock->Data[i] = sourceBlock->Data[i];
            }

            if ((destinationBlock->Flags & DATAFORMAT_VALID) && (dataProcessingOperation == BYTE_SWAPPER))
            {
                // Data alignment is reversed by byte swapper so we need to reverse the flag
                if (destinationBlock->Data[DATAFORMAT_OFFSET] == BE)
                {
                    destinationBlock->Data[DATAFORMAT_OFFSET] = LE;
                }
                else
                {
                    destinationBlock->Data[DATAFORMAT_OFFSET] = BE;
                }

            }

            if (destinationBlock->Flags & PTS_VALID)
            {
                STTBX_Print(("Byte swapper Sending Frame %d, PTS 0x%x, PTS33bit 0x%x\n",
                                                                outputblockCount,destinationBlock->Data[PTS_OFFSET],destinationBlock->Data[PTS33_OFFSET]));
            }
        }

        return (ST_NO_ERROR);
    }
#endif

#if defined(ST_7100)||defined (ST_7109)||defined(ST_5525)|| defined(ST_7200)|| defined(ST_51XX)

    ST_ErrorCode_t FDMAMemcpy(void *destinationAddress, void *sourceAddress, U32 length, U32 NumberOfBytes, U32 sourceStride, U32 destinationStride, STAud_Memory_Address_t  * FDMAAdress)
    {
        STFDMA_TransferGenericParams_t  memcpyTransferParam;
        STFDMA_TransferId_t             TransferId;
        ST_ErrorCode_t                  Error;
        STFDMA_GenericNode_t*           node_p;
        STFDMA_GenericNode_t*           nodephy_p;
        node_p = (STFDMA_GenericNode_t*)FDMAAdress->BaseUnCachedVirtualAddress;
        nodephy_p = (STFDMA_GenericNode_t*)FDMAAdress->BasePhyAddress;

        memcpyTransferParam.Pool                                    = STFDMA_DEFAULT_POOL;
        memcpyTransferParam.ChannelId                               = STFDMA_USE_ANY_CHANNEL;
        memcpyTransferParam.BlockingTimeout                         = 0;
        memcpyTransferParam.CallbackFunction                        = NULL;
        memcpyTransferParam.ApplicationData_p                       = NULL;

        #if defined(ST_5525)
        memcpyTransferParam.FDMABlock                           =STFDMA_1;
        memcpyTransferParam.NodeAddress_p                           = (STFDMA_GenericNode_t *)((U32)nodephy_p);
        node_p->Node.SourceAddress_p                                = (void *)((U32) sourceAddress);
        node_p->Node.DestinationAddress_p                           = (void *)((U32) destinationAddress);

        #else
        memcpyTransferParam.FDMABlock                           =STFDMA_1;
        memcpyTransferParam.NodeAddress_p                           = (STFDMA_GenericNode_t *)((U32)nodephy_p);
        node_p->Node.SourceAddress_p                                = (void *)((U32) sourceAddress );
        node_p->Node.DestinationAddress_p                           = (void *)((U32) destinationAddress );
        #endif

        node_p->Node.NumberBytes                                    = NumberOfBytes;
        node_p->Node.Length                                         = length;
        node_p->Node.SourceStride                                   = sourceStride;
        node_p->Node.DestinationStride                              = destinationStride;
        node_p->Node.NodeControl.PaceSignal                         = STFDMA_REQUEST_SIGNAL_NONE;
        node_p->Node.NodeControl.SourceDirection                    = STFDMA_DIRECTION_INCREMENTING;
        node_p->Node.NodeControl.DestinationDirection               = STFDMA_DIRECTION_INCREMENTING;
        node_p->Node.NodeControl.NodeCompleteNotify                 = FALSE;
        node_p->Node.NodeControl.NodeCompletePause                  = FALSE;
        node_p->Node.NodeControl.Reserved                           = 0;
        node_p->Node.Next_p                                         = NULL;

        Error = STFDMA_StartGenericTransfer(&memcpyTransferParam,&TransferId); /*Start FDMA tranfer from the bit buffer*/
        return(Error);
    }

#endif


ST_ErrorCode_t  DataProcessorCheckStateTransition(DataProcesserControl_t    * DataProcCtrl_p, DataProcesserState_t State)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    switch (State)
    {
        case DATAPROCESSER_GET_INPUT_BLOCK:
            if ((DataProcCtrl_p->dataProcesserCurrentState != DATAPROCESSER_INIT) &&
                (DataProcCtrl_p->dataProcesserCurrentState != DATAPROCESSER_STOPPED))
            {
                Error = STAUD_ERROR_INVALID_STATE;
            }
            break;

        case DATAPROCESSER_STOPPED:
            if ((DataProcCtrl_p->dataProcesserCurrentState != DATAPROCESSER_INIT) &&
                (DataProcCtrl_p->dataProcesserCurrentState != DATAPROCESSER_GET_INPUT_BLOCK) &&
                (DataProcCtrl_p->dataProcesserCurrentState != DATAPROCESSER_GET_OUTPUT_BLOCK))
            {
                Error = STAUD_ERROR_INVALID_STATE;
            }
            break;

        case DATAPROCESSER_TERMINATE:
            if ((DataProcCtrl_p->dataProcesserCurrentState != DATAPROCESSER_INIT) &&
                (DataProcCtrl_p->dataProcesserCurrentState != DATAPROCESSER_STOPPED))
            {
                Error = STAUD_ERROR_INVALID_STATE;
            }
            break;

        default:
            break;
    }
    return(Error);
}

#ifndef STAUD_REMOVE_CLKRV_SUPPORT

    BOOL EPTS_LATER_THAN(STCLKRV_ExtendedSTC_t p1, STCLKRV_ExtendedSTC_t p2)
    {
        p1.BaseBit32 = p1.BaseBit32 & 0x1;
        p2.BaseBit32 = p2.BaseBit32 & 0x1;

        if (p1.BaseBit32 == p2.BaseBit32)
        {
            return (p1.BaseValue > p2.BaseValue);
        }
        else
        {
            return (p1.BaseBit32 > p2.BaseBit32);
        }
    }

    U32  EPTS_DIFF(STCLKRV_ExtendedSTC_t p1, STCLKRV_ExtendedSTC_t p2)
    {
        p1.BaseBit32 = p1.BaseBit32 & 0x1;
        p2.BaseBit32 = p2.BaseBit32 & 0x1;

        if (p1.BaseBit32 == p2.BaseBit32)
        {
            // Both 32 bit or both 33 bit
            if (p2.BaseValue > p1.BaseValue)
                return (p2.BaseValue - p1.BaseValue);
            else
                return (p1.BaseValue - p2.BaseValue);
        }
        else if ((p1.BaseBit32 == 0) && (p2.BaseBit32 == 1))
        {
            //p2 is 33 bit and p1 32 bit
            if (p2.BaseValue >= p1.BaseValue)
            {
                // P2 is atleast 2exp32 bigger so return max diff
                return (0xFFFFFFFF);
            }
            else
            {
                return ((((p2.BaseValue >> 1) | 0x80000000) - (p1.BaseValue >> 1)) << 1);
            }

        }
        else
        {
            //p1 is 33 bit and p2 32 bit
            if (p1.BaseValue >= p2.BaseValue)
            {
                // P1 is atleast 2exp32 bigger so return max diff
                return (0xFFFFFFFF);
            }
            else
            {
                return ((((p1.BaseValue >> 1) | 0x80000000) - (p2.BaseValue >> 1)) << 1);
            }
        }
    }

    void ADDToEPTS(STCLKRV_ExtendedSTC_t * p1_p, S32 value)
    {
        if(value > 0)
        {
            //value is +ve
            if ((p1_p->BaseValue + value) < p1_p->BaseValue)
            {
                p1_p->BaseBit32++;
            }
            p1_p->BaseValue += value;
        }
        else
        {
            //value is -ve or zero
            SubtractFromEPTS(p1_p, ( - value));
        }

    }

    void SubtractFromEPTS(STCLKRV_ExtendedSTC_t * p1_p, U32 value)
    {
        if ((p1_p->BaseValue - value) > p1_p->BaseValue)
        {
            if (p1_p->BaseBit32)
            {
                p1_p->BaseBit32--;
                p1_p->BaseValue += (0xffffffff - value);

            }
            else
            {
                // This should not happen except in rare wraparound case
                p1_p->BaseBit32 = 0;
                p1_p->BaseValue = 0;
            }
        }
        else
        {
            p1_p->BaseValue -= value;
        }
    }

#endif


