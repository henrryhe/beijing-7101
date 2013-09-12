/****************************************************************************/
/*                                                                          */
/* File name   : aud_pcmreader.c                                           */
/*                                                                          */
/* Description : PCM Reader file                                            */
/*                                                                          */
/* COPYRIGHT (C) ST-Microelectronics 2006                                   */
/* History     :                                                            */
/* Date           Modification        Name                                 */
/* ----           ------------         ----                                */
/* 08/09/05       Created           kausik maiti                               */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

//#define  STTBX_PRINT
#if defined( ST_OSLINUX )
#include "compat.h"
#include "linux/kthread.h"
#else
#include "sttbx.h"
#endif
#include "aud_pcmreader.h"
#include "audioreg.h"

#define NUM_PCM_NODES_FOR_STARTUP 4

ST_DeviceName_t PCMReader_DeviceName[] = {"PCMREADER0"};

/*
******************************************************************************
Private Macros
******************************************************************************
*/
#define DEFAULT_SAMPLING_FREQUENCY  48000

/* ---------------------------------------------------------------------------- */
/*               Private Types                  */
/* ---------------------------------------------------------------------------- */
PCMReaderControlBlock_t *PCMReaderHead_p = NULL;

ST_ErrorCode_t PCMRReleaseMemory(PCMReaderControlBlock_t * ControlBlock_p);

/******************************************************************************
 *  Function name   : STAud_PCMReaderInit
 *  Arguments       :
 *       IN         : Name, Name of the PCM reader
 *                  : handle, pointer to the Handle of the PCM Reader
 *                  : Param_p, PCMReader Input Params
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will create the PCM Reader task, initialize
 *                  : and will update main structure with current values
 ***************************************************************************** */
ST_ErrorCode_t      STAud_PCMReaderInit(ST_DeviceName_t  Name, STPCMREADER_Handle_t *handle,PCMReaderInitParams_t *Param_p)
{
    ST_ErrorCode_t            Error         = ST_NO_ERROR;
    PCMReaderControlBlock_t * tempCtrlBlk_p = PCMReaderHead_p, * lastCtrlBlk_p = PCMReaderHead_p;
    PCMReaderControl_t      * Control_p     = NULL;
    STAVMEM_AllocBlockParams_t  AllocParams;
    STEVT_OpenParams_t          EVT_OpenParams;
    U32 i = 0;

    if ((strlen(Name) <= ST_MAX_DEVICE_NAME))
    {
        while (tempCtrlBlk_p != NULL)
        {
            if (strcmp(tempCtrlBlk_p->pcmReaderControl.Name, Name) != 0)
            {
                lastCtrlBlk_p = tempCtrlBlk_p;
                tempCtrlBlk_p = tempCtrlBlk_p->next_p;
            }
            else
            {
                return (ST_ERROR_ALREADY_INITIALIZED);
            }
        }
    }
    else
    {
        return (ST_ERROR_BAD_PARAMETER);
    }


    tempCtrlBlk_p = STOS_MemoryAllocate(Param_p->CPUPartition,sizeof(PCMReaderControlBlock_t));
    if(!tempCtrlBlk_p)
    {
        return ST_ERROR_NO_MEMORY;
    }

    memset(tempCtrlBlk_p,0,sizeof(PCMReaderControlBlock_t));

    Control_p = &(tempCtrlBlk_p->pcmReaderControl);

    strncpy(Control_p->Name, Name, ST_MAX_DEVICE_NAME_TOCOPY);
    *handle = (STPCMREADER_Handle_t)tempCtrlBlk_p;

    // Get params from init params
    Control_p->pcmReaderDataConfig = Param_p->pcmReaderDataConfig;
    Control_p->CPUPartition        = Param_p->CPUPartition;
    Control_p->AVMEMPartition      = Param_p->AVMEMPartition;
    Control_p->pcmReaderNumNode    = Param_p->pcmReaderNumNode;
    Control_p->pcmReaderIdentifier = Param_p->pcmReaderIdentifier;
    Control_p->BMHandle            = Param_p->memoryBlockManagerHandle;
    Control_p->NumChannels         = Param_p->NumChannels;

    /*Get Address*/
    {
        STAUD_BufferParams_t    Buffer;
        Error|=MemPool_GetBufferParams(Control_p->BMHandle, &Buffer);
        if(Error!=ST_NO_ERROR)
        {
            Error |= PCMRReleaseMemory(tempCtrlBlk_p);
            return Error;
        }

        Control_p->MemoryAddress.BaseUnCachedVirtualAddress=(U32*)Buffer.BufferBaseAddr_p;
        Control_p->MemoryAddress.BasePhyAddress=(U32*)STOS_VirtToPhys(Buffer.BufferBaseAddr_p);
        Control_p->MemoryAddress.BaseCachedVirtualAddress= NULL;
    }

    strncpy(Control_p->EvtHandlerName, Param_p->EvtHandlerName,ST_MAX_DEVICE_NAME_TOCOPY);
    // Set default params for this PCM Reader
    Control_p->CurrentState          = PCMREADER_INIT;
    Control_p->NextState             = PCMREADER_INIT;
    Control_p->SamplingFrequency     = (Param_p->Frequency)?Param_p->Frequency:DEFAULT_SAMPLING_FREQUENCY;
    Control_p->pcmReaderPlayed       = 0;
    Control_p->pcmReaderToProgram    = 0;
    Control_p->pcmReaderTransferId   = 0;     // Indicates that no transfer is running

    /* Set the FDMA block used */
    Control_p->FDMABlock           = STFDMA_1;
    /*Debug Info */
    Control_p->FDMAAbortSent       = 0;
    Control_p->FDMAAbortCBRecvd    = 0;

    Error = STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL, &(Control_p->pcmReaderChannelId),Control_p->FDMABlock);

    if(Error != ST_NO_ERROR)
    {
        Error |= PCMRReleaseMemory(tempCtrlBlk_p);
        return Error;
    }

    Control_p->AudioBlock_p = STOS_MemoryAllocate(Param_p->CPUPartition,
                                            (sizeof(PlayerAudioBlock_t) * Control_p->pcmReaderNumNode));

    if(Control_p->AudioBlock_p == NULL)
    {
        Error |= PCMRReleaseMemory(tempCtrlBlk_p);
        return ST_ERROR_NO_MEMORY;
    }

    memset(Control_p->AudioBlock_p, 0, (sizeof(PlayerAudioBlock_t) * Control_p->pcmReaderNumNode));

    AllocParams.PartitionHandle             = Control_p->AVMEMPartition;
    AllocParams.Alignment                   = 32;
    AllocParams.AllocMode                   = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocParams.ForbiddenRangeArray_p       = NULL;
    AllocParams.ForbiddenBorderArray_p      = NULL;
    AllocParams.NumberOfForbiddenRanges     = 0;
    AllocParams.NumberOfForbiddenBorders    = 0;
    AllocParams.Size                        = sizeof(STFDMA_GenericNode_t) * Control_p->pcmReaderNumNode ;

    Error = STAVMEM_AllocBlock(&AllocParams, &(Control_p->NodeHandle));

    if(Error != ST_NO_ERROR)
    {
        Error |= PCMRReleaseMemory(tempCtrlBlk_p);
        return Error;
    }


    Error = STAVMEM_GetBlockAddress(Control_p->NodeHandle, (void *)(&(Control_p->FDMANodesBaseAddress_p)));
    if(Error != ST_NO_ERROR)
    {
        Error |= PCMRReleaseMemory(tempCtrlBlk_p);
        return Error;
    }

    for (i=0; i < Control_p->pcmReaderNumNode; i++)
    {
        STFDMA_GenericNode_t *PCMReaderNode_p = 0 ;

        PCMReaderNode_p = (((STFDMA_GenericNode_t   *)Control_p->FDMANodesBaseAddress_p) + i);
        Control_p->AudioBlock_p[i].PlayerDmaNode_p = PCMReaderNode_p;
        Control_p->AudioBlock_p[i].PlayerDMANodePhy_p = (U32*) STOS_VirtToPhys(PCMReaderNode_p);

        if(i< (Control_p->pcmReaderNumNode - 1))
        {
            PCMReaderNode_p->Node.Next_p = (struct  STFDMA_Node_s *)STOS_VirtToPhys((U32)(PCMReaderNode_p + 1) );
        }
        else
        {
            PCMReaderNode_p->Node.Next_p = (struct  STFDMA_Node_s *)STOS_VirtToPhys((U32)Control_p->FDMANodesBaseAddress_p );
        }

        Control_p->AudioBlock_p[i].Valid              = FALSE;
        Control_p->AudioBlock_p[i].memoryBlock        = NULL;

        // Fill fdma node default parameters
        PCMReaderNode_p->Node.NumberBytes             = (NUM_SAMPLES_PER_PCM_NODE << 3); // (L+R), 32 bits per channel
        PCMReaderNode_p->Node.SourceAddress_p         = (void *)(PCMREADER_FIFO_DATA_REG); /* Address of the PCM Reader */;
#if !defined(ST_7200)
        PCMReaderNode_p->Node.SourceAddress_p         = (void *)STOS_VirtToPhys((U32)PCMReaderNode_p->Node.SourceAddress_p );
#endif
        PCMReaderNode_p->Node.DestinationAddress_p    = (U32 *)0;//to be set at runtime
        PCMReaderNode_p->Node.NodeControl.PaceSignal  = STFDMA_REQUEST_SIGNAL_PCMREADER;
        PCMReaderNode_p->Node.SourceStride            = 0;

        if(Control_p->NumChannels == 2)
        {
            PCMReaderNode_p->Node.Length            = (NUM_SAMPLES_PER_PCM_NODE << 3); // (L+R), 32 bits per channel;
            PCMReaderNode_p->Node.DestinationStride = 0;
        }
        else
        {
            //10 channels
            PCMReaderNode_p->Node.Length            = 8; // (L+R),32 bits per channel;
            PCMReaderNode_p->Node.DestinationStride = 40; // 10 channels, 32 bits per channel;
        }

        PCMReaderNode_p->Node.NodeControl.SourceDirection       = STFDMA_DIRECTION_STATIC;
        PCMReaderNode_p->Node.NodeControl.DestinationDirection  = STFDMA_DIRECTION_INCREMENTING;
        PCMReaderNode_p->Node.NodeControl.NodeCompleteNotify    = TRUE;
        PCMReaderNode_p->Node.NodeControl.NodeCompletePause     = FALSE;
        PCMReaderNode_p->Node.NodeControl.Reserved              = 0;
#if defined(ST_7200)
        PCMReaderNode_p->Node.NodeControl.Reserved1             = 0;
#endif
    }

    // Initialize Tranfer Reader params for FDMA
    Control_p->TransferParams.ChannelId        = Control_p->pcmReaderChannelId;
    Control_p->TransferParams.Pool             = STFDMA_DEFAULT_POOL;
    Control_p->TransferParams.NodeAddress_p    = Control_p->AudioBlock_p[0].PlayerDmaNode_p;
    Control_p->TransferParams.BlockingTimeout  = 0;
    Control_p->TransferParams.CallbackFunction = (STFDMA_CallbackFunction_t)PCMReaderFDMACallbackFunction;
    Control_p->TransferParams.ApplicationData_p= (void *)(tempCtrlBlk_p);
    Control_p->TransferParams.FDMABlock        = Control_p->FDMABlock;

    Control_p->pcmReaderTaskSem_p          = STOS_SemaphoreCreateFifo(NULL,0);
    Control_p->CmdTransitionSem_p          = STOS_SemaphoreCreateFifo(NULL,0);
    Control_p->LockControlStructure        = STOS_MutexCreateFifo();


    Error = STAud_PlayerMemRemap(&Control_p->BaseAddress);
    if(Error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_ReaderMemRemap Failed %x \n",Error));
    }

    /* Open the ST Event */
    Error |= STEVT_Open(Control_p->EvtHandlerName,&EVT_OpenParams,&Control_p->EvtHandle);
    if (Error != ST_NO_ERROR)
    {
        Error |= PCMRReleaseMemory(tempCtrlBlk_p);
        return Error;
    }

    strncpy(Control_p->EvtGeneratorName, Param_p->EvtGeneraterName, ST_MAX_DEVICE_NAME_TOCOPY);

    Error  = PCMReader_RegisterEvents(Control_p);
    Error |= PCMReader_SubscribeEvents(Control_p);
    if (Error != ST_NO_ERROR)
    {
        Error |= PCMRReleaseMemory(tempCtrlBlk_p);
        return Error;
    }

    Control_p->pcmRestart = FALSE;
    Control_p->pcmPause   = FALSE;

    STOS_TaskCreate(PCMReaderTask,(void *)tempCtrlBlk_p,NULL,PCMREADER_STACK_SIZE,NULL,NULL,
                    &Control_p->pcmReaderTask_p,NULL,PCMREADER_TASK_PRIORITY, Control_p->Name, 0);

    if (Control_p->pcmReaderTask_p == NULL)
    {
        Error |= PCMRReleaseMemory(tempCtrlBlk_p);
        return ST_ERROR_NO_MEMORY;
    }

    if (PCMReaderHead_p == NULL)
    {
        PCMReaderHead_p = tempCtrlBlk_p;
    }
    else
    {
        lastCtrlBlk_p->next_p = tempCtrlBlk_p;
    }

    tempCtrlBlk_p->next_p = NULL;

    return (Error);
}

/******************************************************************************
 *  Function name   : STAud_PCMReaderTerminate
 *  Arguments       :
 *       IN         : handle, pointer to the Handle of the PCM Reader
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will terminate the PCM Reader task,
 *                  : deallocate all structures allocated
 ***************************************************************************** */
ST_ErrorCode_t      STAud_PCMReaderTerminate(STPCMREADER_Handle_t handle)
{
    ST_ErrorCode_t            Error         = ST_NO_ERROR;
    PCMReaderControlBlock_t * tempCtrlBlk_p = PCMReaderHead_p, * lastCtrlBlk_p = PCMReaderHead_p;
    PCMReaderControl_t      * Control_p     = NULL;

    while (tempCtrlBlk_p != (PCMReaderControlBlock_t *)handle)
    {
        lastCtrlBlk_p = tempCtrlBlk_p;
        tempCtrlBlk_p = tempCtrlBlk_p->next_p;
    }

    if (tempCtrlBlk_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p = &(tempCtrlBlk_p->pcmReaderControl);

    Error |= STAud_PlayerMemUnmap(&Control_p->BaseAddress);

    lastCtrlBlk_p->next_p = tempCtrlBlk_p->next_p;
    // deallocate control block
    if (tempCtrlBlk_p == PCMReaderHead_p)
    {
        PCMReaderHead_p = PCMReaderHead_p->next_p;//NULL;
    }

    Error |= PCMRReleaseMemory(tempCtrlBlk_p);


    return Error;
}

/******************************************************************************
*                       PCM READER TASK                                        *
******************************************************************************/
/******************************************************************************
*  Function name    : PCMReaderTask
*  Arguments        :
*       IN          : tempPCMReaderControlBlock, PCM reader control block
*       OUT         : void
*       INOUT       :
*  Return           : void
*  Description      : This function is the entry point for PCM Reader task. Ihe reader task will be in
*                  different states depending of the command received from upper/lower layers
***************************************************************************** */
void    PCMReaderTask(void * tempCtrlBlk_p)
{
    PCMReaderControlBlock_t * ControlBlock_p = (PCMReaderControlBlock_t *)tempCtrlBlk_p;
    PCMReaderControl_t      * Control_p      = &(ControlBlock_p->pcmReaderControl);
    PlayerAudioBlock_t      * AudioBlock_p   = Control_p->AudioBlock_p;
    ST_ErrorCode_t err    = ST_NO_ERROR;
    U32 i                 = 0;
    U32 PCMReaderBaseAddr = Control_p->BaseAddress.PCMReaderBaseAddr;
    U32 NumNode           = Control_p->pcmReaderNumNode;

    STOS_TaskEnter(ControlBlock_p);

    while (1)
    {
        STOS_SemaphoreWait(Control_p->pcmReaderTaskSem_p);
        switch (Control_p->CurrentState)
        {
        case PCMREADER_INIT:
            STOS_MutexLock(Control_p->LockControlStructure);
            if (Control_p->NextState != Control_p->CurrentState)
            {
                Control_p->CurrentState = Control_p->NextState;
                //STTBX_Print(("PCMR:from PCMREADER_INIT\n"));
                STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
                // Signal the command completion transtition
                STOS_SemaphoreSignal(Control_p->CmdTransitionSem_p);
            }
            STOS_MutexRelease(Control_p->LockControlStructure);
            if(Control_p->CurrentState == PCMREADER_START)
            {


            }
            break;
        case PCMREADER_START:
            // Get number of buffers required for startup
            for (i=Control_p->pcmReaderPlayed; i < (Control_p->pcmReaderPlayed + NUM_PCM_NODES_FOR_STARTUP); i++)
            {
                U32 Index = i % NumNode;
                if (AudioBlock_p[Index].Valid == FALSE)
                {
                    if (MemPool_GetOpBlk(Control_p->BMHandle, (U32 *)(& AudioBlock_p[Index].memoryBlock)) != ST_NO_ERROR)
                        break;
                    else
                    {
                        //STTBX_Print(("Start:GetIpBlk 0x%x\n",(U32)AudioBlock_p[Index].memoryBlock));
                        STOS_MutexLock(Control_p->LockControlStructure);
                        FillPCMReaderFDMANode(&AudioBlock_p[Index], Control_p);
                        STOS_MutexRelease(Control_p->LockControlStructure);
                        AudioBlock_p[Index].Valid = TRUE;
                    }
                }
            }

                // If all the required nodes are valid start the transfer
            if (AudioBlock_p[(Control_p->pcmReaderPlayed + NUM_PCM_NODES_FOR_STARTUP - 1) % NumNode].Valid)
            {
                U32 Index = Control_p->pcmReaderPlayed;

                Control_p->CurrentState = PCMREADER_PLAYING;

                StartPCMReaderIP(Control_p);

                Control_p->pcmReaderToProgram = Index;

                for(i = Control_p->pcmReaderToProgram; i < (Control_p->pcmReaderToProgram + NUM_PCM_NODES_FOR_STARTUP - 1); i++)
                {
                    AudioBlock_p[i % NumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = FALSE;
                }

                /* Set PAUSE for last block in queue*/
                AudioBlock_p[i% NumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = TRUE;

                Control_p->pcmReaderToProgram = i;


                Control_p->TransferParams.NodeAddress_p = (STFDMA_GenericNode_t *)((U32)AudioBlock_p[Index % NumNode].PlayerDMANodePhy_p );

                /*Put frequency in the flag*/
                AudioBlock_p[Index % NumNode].memoryBlock->Data[FREQ_OFFSET]= Control_p->SamplingFrequency;
                AudioBlock_p[Index % NumNode].memoryBlock->Data[NCH_OFFSET] = Control_p->NumChannels;
                {
                    U32 WordSize = 0;
                    U32 Temp_BitsSubFrame_16 = 16;
                    U32 Temp_BitsSubFrame_32 = 32;
                    switch(Control_p->pcmReaderDataConfig.BitsPerSubFrame)
                    {
                        case STAUD_DAC_NBITS_SUBFRAME_32:
                            WordSize = ConvertToAccWSCode((U32)Temp_BitsSubFrame_32);
                            break;
                        case STAUD_DAC_NBITS_SUBFRAME_16:
                            WordSize = ConvertToAccWSCode((U32)Temp_BitsSubFrame_16);
                            break;
                        default:
                            WordSize = ConvertToAccWSCode((U32)Temp_BitsSubFrame_32);
                            break;
                    }
                    AudioBlock_p[Index % NumNode].memoryBlock->Data[SAMPLESIZE_OFFSET] = WordSize;
                }

                AudioBlock_p[Index % NumNode].memoryBlock->Flags |= FREQ_VALID | NCH_VALID | SAMPLESIZE_VALID;


                err = STFDMA_StartGenericTransfer(&(Control_p->TransferParams), &(Control_p->pcmReaderTransferId));
                if(err != ST_NO_ERROR)
                {
                    Control_p->CurrentState = PCMREADER_START;
                    STTBX_Print(("PCMR:Start    failure\n"));
                    STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);

                }
                else
                {
                    PCMReaderControlR_t ControlReg;

                    ControlReg.Reg = STSYS_ReadRegDev32LE(PCMReaderBaseAddr+0x1C);//PCMREADER_CONTROL_REG
                    ControlReg.Bits.Mode        = PCMR_PCM_MODE_ON;

                    STSYS_WriteRegDev32LE(PCMReaderBaseAddr,((U32)0x1));//PCMREADER_SOFT_RESET_REG
                    STSYS_WriteRegDev32LE(PCMReaderBaseAddr,((U32)0x0));//PCMREADER_SOFT_RESET_REG

                    STSYS_WriteRegDev32LE(PCMReaderBaseAddr+0x1C ,  ((U32)ControlReg.Reg));//PCMREADER_CONTROL_REG
                    STTBX_Print(("PCMR:Started\n"));
                }
            }


            STOS_MutexLock(Control_p->LockControlStructure);
            if (Control_p->NextState == PCMREADER_STOPPED)
            {
                //STTBX_Print(("PCMR:from PCMREADER_START\n"));
                Control_p->CurrentState = Control_p->NextState;
                if (Control_p->pcmReaderTransferId != 0)
                {
                    // FDMA transfer is already started
                }
                else
                {
                    // FDMA transfer was not started
                    for (i=Control_p->pcmReaderPlayed; i < (Control_p->pcmReaderPlayed + NumNode); i++)
                    {
                        U8 Index = i % NumNode;
                        if (AudioBlock_p[Index].Valid == TRUE)
                        {
                            STTBX_Print(("Freeing MBlock in PCMP start \n"));
                            if (MemPool_PutOpBlk(Control_p->BMHandle, (U32 *)(& AudioBlock_p[Index].memoryBlock)) != ST_NO_ERROR)
                            {
                                STTBX_Print(("Error in releasing Ip Pcm Reader mem block\n"));
                            }
                            AudioBlock_p[Index].Valid = FALSE;

                        }
                    }
                }

                STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
            }
            else if(Control_p->NextState == PCMREADER_PAUSE)
            {
                Control_p->CurrentState = Control_p->NextState;
                if(Control_p->pcmReaderTransferId)
                {
                    // FDMA transfer is already started
                    Control_p->pcmPause = TRUE;
                    // command completion will be from callback
                }
                else
                {
                    //FDMA has not started. So no need to set pcmPause = TRUE;
                    STOS_SemaphoreSignal(Control_p->CmdTransitionSem_p);
                }

                STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
            }
            STOS_MutexRelease(Control_p->LockControlStructure);

            break;
        case PCMREADER_RESTART:

                //STTBX_Print(("PCMPR  =%d\n",restartcount++));
            for (i=Control_p->pcmReaderPlayed; i < (Control_p->pcmReaderPlayed + NUM_PCM_NODES_FOR_STARTUP); i++)
            {
                U8 Index = i % NumNode;
                if (AudioBlock_p[Index].Valid == FALSE)
                {
                    if (MemPool_GetOpBlk(Control_p->BMHandle, (U32 *)(& AudioBlock_p[Index].memoryBlock)) != ST_NO_ERROR)
                    {
                        break;
                    }
                    else
                    {
                        //STTBX_Print(("PCMR:ReStart got    %d\n",Index));
                        STOS_MutexLock(Control_p->LockControlStructure);
                        FillPCMReaderFDMANode(&AudioBlock_p[Index], Control_p);
                        STOS_MutexRelease(Control_p->LockControlStructure);
                        AudioBlock_p[Index].Valid = TRUE;
                    }
                }
            }

                // If all the required nodes are valid start the transfer
            if (AudioBlock_p[(Control_p->pcmReaderPlayed + NUM_PCM_NODES_FOR_STARTUP - 1) % NumNode].Valid)
            {
                Control_p->CurrentState = PCMREADER_PLAYING;

                Control_p->pcmReaderToProgram = Control_p->pcmReaderPlayed;

                for(i = Control_p->pcmReaderToProgram; i < (Control_p->pcmReaderToProgram + NUM_PCM_NODES_FOR_STARTUP - 1); i++)
                {
                    AudioBlock_p[i % NumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = FALSE;
                }

                /* Set PAUSE for last block in queue*/
                AudioBlock_p[i% NumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = TRUE;

                Control_p->pcmReaderToProgram = i;

                //Soft reset the IP
                STSYS_WriteRegDev32LE(PCMReaderBaseAddr,((U32)0x1));//PCMREADER_SOFT_RESET_REG
                err = STFDMA_ResumeTransfer(Control_p->pcmReaderTransferId);

                if(err != ST_NO_ERROR)
                {
                    Control_p->CurrentState = PCMREADER_RESTART;
                    STTBX_Print(("PCMR:ReStart failure\n"));
                    STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);

                }
                else
                {
                    //Bring out of reset
                    STSYS_WriteRegDev32LE(PCMReaderBaseAddr,((U32)0x0));//PCMREADER_SOFT_RESET_REG
                    STTBX_Print(("PCMR:ReStarted\n"));
                }
            }

            STOS_MutexLock(Control_p->LockControlStructure);
            if (Control_p->NextState == PCMREADER_STOPPED)
            {
                Control_p->CurrentState = Control_p->NextState;
                //STTBX_Print(("PCMR:from PCMREADER_RESTART\n"));
                STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
                // Signal the command completion transtition from callback
            }
            STOS_MutexRelease(Control_p->LockControlStructure);

            break;
        case PCMREADER_PLAYING:

            STOS_MutexLock(Control_p->LockControlStructure);
            if(Control_p->NextState != Control_p->CurrentState)
            {
                if (Control_p->NextState == PCMREADER_STOPPED)
                {
                    Control_p->CurrentState = Control_p->NextState;
                    //STTBX_Print(("PCMR:from PCMREADER_PLAYING\n"));
                    STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
                    // Signal the command completion transtition

                }
                else if (Control_p->NextState == PCMREADER_PAUSE)
                    {
                        Control_p->pcmPause = TRUE;
                        STTBX_Print(("PCMR:PLAYING to PAUSE\n"));
                        Control_p->CurrentState = Control_p->NextState;
                        STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
                        // Command completion will be from callback
                    }
                    else
                    {
                        if(Control_p->pcmRestart)
                        {
                            Control_p->pcmRestart = FALSE;

                            Control_p->CurrentState = PCMREADER_RESTART;
                            STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);

                            STTBX_Print(("PCMR:PLAYING to RESTART\n"));
                        }
                    }

                //STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
            }
            STOS_MutexRelease(Control_p->LockControlStructure);


            break;
        case PCMREADER_PAUSE:
            STOS_MutexLock(Control_p->LockControlStructure);
            if(Control_p->NextState == PCMREADER_START)
            {
                //We have to resume the transfer again. either go to start or to resume
                if(Control_p->pcmReaderTransferId)
                {
                    STTBX_Print(("PCMR:from PASUE to RESTART\n"));
                    Control_p->CurrentState = PCMREADER_RESTART;
                }
                else
                {
                    STTBX_Print(("PCMR:from PASUE to START\n"));
                    Control_p->CurrentState = PCMREADER_START;
                }
                STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
                // Signal the command completion transtition
                STOS_SemaphoreSignal(Control_p->CmdTransitionSem_p);
            }
            else if(Control_p->NextState == PCMREADER_STOPPED)
                {
                    // We have to go to stop
                    Control_p->CurrentState = Control_p->NextState;
                    STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
                }
            STOS_MutexRelease(Control_p->LockControlStructure);
            break;
        case PCMREADER_STOPPED:
            if (Control_p->pcmReaderTransferId != 0)
            {
                // Abort currrently running transfer
                err =  STFDMA_AbortTransfer(Control_p->pcmReaderTransferId);
                if(err)
                {
                    STTBX_Print(("STFDMA_AbortTransfer PCMReader Failure=%x\n",err));
                }
                else
                {
                    Control_p->FDMAAbortSent++;
                    STTBX_Print(("AbPCMPS\n"));
                }

            }
            else
            {
                STOS_MutexLock(Control_p->LockControlStructure);
                if (Control_p->NextState == PCMREADER_START ||
                    Control_p->NextState == PCMREADER_TERMINATE)
                {

                    Control_p->CurrentState = Control_p->NextState;
                    STTBX_Print(("PCMR:from PCMREADER_STOPPED\n"));
                    STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
                    // Signal the command completion transtition
                    STOS_SemaphoreSignal(Control_p->CmdTransitionSem_p);
                }
                STOS_MutexRelease(Control_p->LockControlStructure);
            }
            break;
        case PCMREADER_TERMINATE:
            // clean up all the memory allocations
            if (Control_p->pcmReaderTransferId != 0)
            {
                STTBX_Print((" PCM Reader task terminated with FDMA runing \n"));
            }

            for (i=Control_p->pcmReaderPlayed; i < (Control_p->pcmReaderPlayed + NumNode); i++)
            {
                U8 Index = i % NumNode;
                if (AudioBlock_p[Index].Valid == TRUE)
                {
                    //STTBX_Print(("Freeing MBlock in PCMP terminate \n"));
                    if (MemPool_PutOpBlk(Control_p->BMHandle, (U32 *)(& AudioBlock_p[Index].memoryBlock)) != ST_NO_ERROR)
                    {
                        STTBX_Print(("Error in releasing Ip Pcm Reader mem block\n"));
                    }

                    AudioBlock_p[Index].Valid = FALSE;
                }
            }

            if (STAud_PCMReaderTerminate((STPCMREADER_Handle_t)ControlBlock_p) != ST_NO_ERROR)
            {
                STTBX_Print(("Error in pcm Reader terminate\n"));
            }

            // Exit Reader task
            STOS_TaskExit(ControlBlock_p);
#if defined( ST_OSLINUX )
            return ;
#else
            task_exit(1);
#endif
            break;

        default:
            break;
        }

    }
}

/******************************************************************************
 *  Function name   : PCMReaderFDMACallbackFunction
 *  Arguments       :
 *  IN              : TransferId,
 *                  : CallbackReason,
 *                  : CurrentNode_p,
 *                  : NodeBytesRemaining
 *                  : ApplicationData_p
 *                  : InterruptTime
 *  Return          : void
 *  Description     : This is the callback function from the FDMA
 *                  :
 ***************************************************************************** */
void PCMReaderFDMACallbackFunction(U32 TransferId,U32 CallbackReason,U32 *CurrentNode_p,U32 NodeBytesRemaining,BOOL Error,void *ApplicationData_p, clock_t  InterruptTime)
{

    PCMReaderControlBlock_t * tempCtrlBlk_p = (PCMReaderControlBlock_t *)ApplicationData_p;
    PCMReaderControl_t      * Control_p          =   &(tempCtrlBlk_p->pcmReaderControl);
    PlayerAudioBlock_t   * AudioBlock_p       = Control_p->AudioBlock_p;
    U32 Played = 0,ToProgram = 0, Count = 0, i = 0;
    ST_ErrorCode_t err = ST_NO_ERROR;
    U8 Index           = 0;
    U32 NumNode        = Control_p->pcmReaderNumNode;


    if(TransferId == Control_p->pcmReaderTransferId)
    {
        // Lock the Block
        STOS_MutexLock(Control_p->LockControlStructure);

        Played = Control_p->pcmReaderPlayed;
        ToProgram = Control_p->pcmReaderToProgram;


        /*if(Played%500 == 499)
        {
            STTBX_Print(("PCMR:%d\n",Played));
        }*/

        switch(CallbackReason)
        {
        case STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED:
            STTBX_Print(("PCMR: PAUSED:%d\n",(Played)));
            /*Error Recovery*/

            while(!((U32)(AudioBlock_p[Count % NumNode].PlayerDMANodePhy_p) == ((U32)CurrentNode_p) ) && (Count < NumNode))
            {
                Count++;
            }

            Index = Played % NumNode;
            while(Index != Count)
            {
                // Put the valid Block
                if (MemPool_PutOpBlk(Control_p->BMHandle, (U32 *)(& AudioBlock_p[Index].memoryBlock)) != ST_NO_ERROR)
                {
                    STTBX_Print(("Error in releasing Pcm Reader mem blcok\n"));
                }


                // Block is consumed and ready to be filled
                AudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/

                Played ++;
                Index = Played % NumNode;
            }


            // This is to free the block on which the pause bit is set
            // Put the valid Block
            if (MemPool_PutOpBlk(Control_p->BMHandle, (U32 *)(& AudioBlock_p[Index].memoryBlock)) != ST_NO_ERROR)
            {
                STTBX_Print(("Error in releasing Ip Pcm Reader mem block\n"));
            }

            // Block is consumed and ready to be filled
            AudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/

            Count ++;

            // Put counters at last stop status
            Played = Count;
            ToProgram = Count;
            //STTBX_Print(("PCMCallback: count=%d\n",Played));

            STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);
            if(Control_p->pcmPause)
            {

                Control_p->pcmPause = FALSE;
                /* This would complete the transition to PAUSE State*/
                STOS_SemaphoreSignal(Control_p->CmdTransitionSem_p);
            }
            else
            {
                Control_p->pcmRestart = TRUE;
            }

            break;
        case STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING:
            //STTBX_Print(("Reader CONT:%d\n",(Played % NumNode)));
            if(Error)
            {

                while(!((U32)(AudioBlock_p[Count % NumNode].PlayerDMANodePhy_p) == ((U32)CurrentNode_p)   ) && (Count < NumNode))

                {
                    Count++;
                }

                Index = Played % NumNode;
                while(Index != Count)
                {

                    // Put the valid Block
                    err = MemPool_PutOpBlk(Control_p->BMHandle, (U32   *)(& AudioBlock_p[Index].memoryBlock));
                    if (err != ST_NO_ERROR)
                    {
                        STTBX_Print(("error in freeing pcm Reader mem block %d\n",err));
                    }

                    // Block is filled and resy to be used
                    AudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/
                    Played ++;
                    Index = Played % NumNode;
                }
            }
            else
            {
                Index = Played % NumNode;
                // Put the valid Block
                //STTBX_Print(("putOpBlk 0x%x\n",AudioBlock_p[Played % NumNode].memoryBlock));
                err = MemPool_PutOpBlk(Control_p->BMHandle, (U32   *)(&AudioBlock_p[Index].memoryBlock));
                if (err != ST_NO_ERROR)
                {
                    STTBX_Print(("1:Error in pcm Reader releasing memory block %d\n",err));
                }
                // Block is filled and ready to be used
                AudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/
                Played ++;
            }

            if(Control_p->pcmPause == FALSE)
            {
                if(Played < ToProgram)
                {
                    for(i = ToProgram; i < (Played + 3); i++)
                    {
                        Index = (i + 1) % NumNode;
                        if(AudioBlock_p[Index].Valid == FALSE)
                        {
                            // Try to get next empty block
                            if (MemPool_GetOpBlk(Control_p->BMHandle, (U32 *)(& AudioBlock_p[Index].memoryBlock)) != ST_NO_ERROR)
                            {
                                //STTBX_Print(("PCMR:No block:%d\n",Index));
                                break;
                            }
                            else
                            {
                                // Got next block..
                                //STTBX_Print(("PCMR:Got block:%d\n",Index));
                                FillPCMReaderFDMANode(&AudioBlock_p[Index], Control_p);
                                AudioBlock_p[Index].Valid = TRUE; /**/
                            }
                        }

                        if(AudioBlock_p[Index].Valid)
                        {
                            AudioBlock_p[Index].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = TRUE;
                            AudioBlock_p[i % NumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = FALSE;

                            ToProgram++; /*Increase the no of buffer to program*/

                        }
                        else
                        {
                            break;
                            /*STTBX_Print(("No Valid buffer\n"));*/
                        }
                    }
                }
            }

            break;
        case STFDMA_NOTIFY_TRANSFER_COMPLETE:
            break;
        case STFDMA_NOTIFY_TRANSFER_ABORTED:

            Control_p->FDMAAbortCBRecvd++;
            // Lock the Block
            while(Played <= ToProgram)
            {
                Index = Played % NumNode;
                if (AudioBlock_p[Index].Valid)
                {
                    // This condition should always be true in this case
                    // Put the Block back
                    if(MemPool_PutOpBlk(Control_p->BMHandle, (U32 *)(& AudioBlock_p[Index].memoryBlock)) != ST_NO_ERROR)
                    {
                        STTBX_Print(("Error in pcm Reader putting memory block\n"));
                    }
                    // Block is consumed and ready to be filled

                    AudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/
                }

                Played ++;
            }

            for(i=0;i < NumNode; i++)
            {
                AudioBlock_p[i].Valid = FALSE; /*Block is consumed and ready to be filled*/
            }


            // reset the trasnsfer id to 0
            Control_p->pcmReaderTransferId =   0;

            Played = 0;
            ToProgram = 0;

            /* This would complete the transition to STOP State*/
            STOS_SemaphoreSignal(Control_p->CmdTransitionSem_p);
            // To move state from stop to start/terminate in case command is given before abort is completed
            STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);

            break;
        default:
            break;
        }

        Control_p->pcmReaderPlayed = Played;
        Control_p->pcmReaderToProgram = ToProgram;

        // Release the Lock
        STOS_MutexRelease(Control_p->LockControlStructure);
    }
}

/******************************************************************************
 *  Function name   : PCMReaderCheckStateTransition
 *  Arguments       :
 *  IN              : Control_p, Reader Control Block
 *                  : State, Next state
 *  Return          : ST_ErrorCode_t
 *  Description     : Checks the validity of the next state
 *                  :
 ***************************************************************************** */
ST_ErrorCode_t  PCMReaderCheckStateTransition(PCMReaderControl_t * Control_p,PCMReaderState_t State)
{
    ST_ErrorCode_t Error          = ST_NO_ERROR;
    PCMReaderState_t CurrentState = Control_p->CurrentState;
    switch (State)
    {
    case PCMREADER_START:
        STTBX_Print(("PCMR:START cmd\n"));
        if ((CurrentState != PCMREADER_INIT) &&
            (CurrentState != PCMREADER_STOPPED) &&
            (CurrentState != PCMREADER_PAUSE))
        {
            Error = STAUD_ERROR_INVALID_STATE;
            STTBX_Print(("PCMR:Err cmd\n"));
        }
        break;

    case PCMREADER_PAUSE:
        STTBX_Print(("PCMR:PAUSE cmd\n"));
    case PCMREADER_STOPPED:
        if ((CurrentState != PCMREADER_START) &&
            (CurrentState != PCMREADER_RESTART) &&
            (CurrentState != PCMREADER_PLAYING) &&
            (CurrentState != PCMREADER_PAUSE))
        {
            Error = STAUD_ERROR_INVALID_STATE;
            STTBX_Print(("PCMR:Err cmd\n"));
        }
        break;

    case PCMREADER_TERMINATE:
        if ((CurrentState != PCMREADER_INIT) &&
            (CurrentState != PCMREADER_STOPPED))
        {
            Error = STAUD_ERROR_INVALID_STATE;
            STTBX_Print(("PCMR:Err cmd\n"));
        }
        break;
    default:
        STTBX_Print(("PCMR:Err cmd\n"));
        break;
    }
    return(Error);

}

/******************************************************************************
 *  Function name   : STAud_PCMReaderSetCmd
 *  Arguments       :
 *       IN         : handle, Reader handle
 *                  : State, Next state
 *  Return          : ST_ErrorCode_t
 *  Description     : command to to handle the next state
 *                          :
 ***************************************************************************** */
ST_ErrorCode_t      STAud_PCMReaderSetCmd(STPCMREADER_Handle_t handle,PCMReaderState_t State)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    PCMReaderControlBlock_t * tempCtrlBlk_p = PCMReaderHead_p, * lastCtrlBlk_p = PCMReaderHead_p;
    task_t                  * PCMReaderTask_p    = NULL;
    PCMReaderControl_t      * Control_p          = NULL;

    if(lastCtrlBlk_p == NULL)
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    while (tempCtrlBlk_p!= (PCMReaderControlBlock_t *)handle)
    {
        lastCtrlBlk_p= tempCtrlBlk_p;
        tempCtrlBlk_p= tempCtrlBlk_p->next_p;
    }

    if (tempCtrlBlk_p== NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p = &(tempCtrlBlk_p->pcmReaderControl);

    STOS_MutexLock(Control_p->LockControlStructure);
    Error = PCMReaderCheckStateTransition(Control_p, State);
    if ( Error != ST_NO_ERROR)
    {
        STOS_MutexRelease(Control_p->LockControlStructure);
        return (Error);
    }

    if(State == PCMREADER_START)
    {
        ProducerParams_t        ProducerParams;
        ProducerParams.Handle   = (U32)tempCtrlBlk_p;
        ProducerParams.sem      = Control_p->pcmReaderTaskSem_p;
        // Register to memory block manager as a Producer
        Error = MemPool_RegProducer(Control_p->BMHandle, ProducerParams);
        ConfigurePCMReaderNode(Control_p);
    }

    if (State == PCMREADER_TERMINATE)
    {
        PCMReaderTask_p = Control_p->pcmReaderTask_p;
    }
    /*change the next state*/

    Control_p->NextState = State;
    STOS_MutexRelease(Control_p->LockControlStructure);

    /*signal the task to change its state*/
    STOS_SemaphoreSignal(Control_p->pcmReaderTaskSem_p);

    if (State == PCMREADER_TERMINATE)
    {

        STOS_TaskWait(&PCMReaderTask_p, TIMEOUT_INFINITY);
        STOS_TaskDelete(PCMReaderTask_p,NULL,NULL,NULL);
    }
    else
    {
        // Wait for command completion transtition
        STTBX_Print(("PCMR:Waiting for cnd completion\n"));
        STOS_SemaphoreWait(Control_p->CmdTransitionSem_p);
        STTBX_Print(("PCMR:cnd completed\n"));
#if defined(ST_7100) || defined(ST_7109)
        if (State == PCMREADER_STOPPED)
        {
            STTBX_Print(("PCMR:Calling UNReg\n"));
            MemPool_UnRegProducer(Control_p->BMHandle, (U32)tempCtrlBlk_p);
            //STTBX_Print(("MemPool_UnRegConsumer Done\n"));
        }
#endif
    }


    return Error;
}

/******************************************************************************
 *  Function name   : FillPCMReaderFDMANode
 *  Arguments       :
 *  IN              : pcmReaderAudioBlock, Reader Audio control block
 *                  : Control_p, Reader Control Block
 *  Return          : void
 *  Description     : command to fill the empty audio block to the FDMA node
 *                          :
 ***************************************************************************** */
void    FillPCMReaderFDMANode(PlayerAudioBlock_t * AudioBlock_p, PCMReaderControl_t *   Control_p)
{
    STFDMA_GenericNode_t  * PlayerDmaNode_p = AudioBlock_p->PlayerDmaNode_p;
    U32                     BytesPerSample  = Control_p->BytesPerSample;
    U32                     NumSamples      = (AudioBlock_p->memoryBlock->MaxSize / BytesPerSample);

    //update the frame size
#ifndef STAUDLX_ALSA_SUPPORT
    if(NUM_SAMPLES_PER_PCM_NODE < NumSamples)
    {
        NumSamples = NUM_SAMPLES_PER_PCM_NODE;
    }
#endif
    Control_p->CurrentFrameSize                = (NumSamples * BytesPerSample);

    if(Control_p->NumChannels == 2)
    {
        PlayerDmaNode_p->Node.Length           = Control_p->CurrentFrameSize;
    }

    PlayerDmaNode_p->Node.NumberBytes          = Control_p->CurrentFrameSize;

    PlayerDmaNode_p->Node.DestinationAddress_p = (void *)STOS_AddressTranslate(
                                                  Control_p->MemoryAddress.BasePhyAddress ,
                                                  Control_p->MemoryAddress.BaseUnCachedVirtualAddress,
                                                  AudioBlock_p->memoryBlock->MemoryStart);

    if(Control_p->NumChannels == 2)
    {
        AudioBlock_p->memoryBlock->Size        = Control_p->CurrentFrameSize;
    }
    else
    {
        AudioBlock_p->memoryBlock->Size        =  5*Control_p->CurrentFrameSize;
    }
}

/******************************************************************************
 *  Function name   : ConfigurePCMReaderNode
 *  Arguments       :
 *  IN              :
 *                  : Control_p, Reader Control Block
 *  Return          : void
 *  Description     : configure FDMA node as per data size
 *                          :
 ***************************************************************************** */
void    ConfigurePCMReaderNode(PCMReaderControl_t * Control_p)
{
    U32 i, BytesPerSampleExp = 3;

    for(i = 0; i < Control_p->pcmReaderNumNode; i++)
    {
        STFDMA_GenericNode_t    * PlayerDmaNode_p = Control_p->AudioBlock_p[i].PlayerDmaNode_p;

        if(Control_p->pcmReaderDataConfig.MemFormat == STAUD_PCMR_BITS_16_16)
        {
            /*implies that we read 16 bit per channel*/
            BytesPerSampleExp = 2; /* 2*2 bytes == 2^2 */
        }
        else
        {
            /*implies "STAUD_PCMR_BITS_16_0" that we read 32 bit per channel*/
            /*which is the default*/ /* 2*4 bytes == 2^3 */
        }

        Control_p->BytesPerSample         =   (1 << BytesPerSampleExp);

        PlayerDmaNode_p->Node.NumberBytes = (NUM_SAMPLES_PER_PCM_NODE << BytesPerSampleExp); // (L+R)

        if(Control_p->NumChannels == 2)
        {
            PlayerDmaNode_p->Node.Length             = PlayerDmaNode_p->Node.NumberBytes; // (L+R)
            PlayerDmaNode_p->Node.DestinationStride  = 0;
        }
        else
        {
            //10 channels
            PlayerDmaNode_p->Node.Length             = Control_p->BytesPerSample; // (L+R)
            PlayerDmaNode_p->Node.DestinationStride  = (PlayerDmaNode_p->Node.Length * 5); // 10 channels
        }
    }

}

/******************************************************************************
 *  Function name   : StartPCMReaderIP
 *  Arguments       :
 *  IN              : pcmReaderIdentifier, Reader Identifier
 *                  : pcmReaderDataConfig, Reader data configuration
 *  Return          : void
 *  Description     : command to initialize the PCM Reader registers
 *                          :
 ***************************************************************************** */
void StartPCMReaderIP(PCMReaderControl_t *  Control_p)
{
    U32                      Addr1, Addr2, Addr3;
    PCMReaderControlR_t      ControlReg;
    PCMReaderFormatR_t       FormatReg;
    STAUD_PCMReaderConf_t  * DataConfig_p = &(Control_p->pcmReaderDataConfig);

    Addr1=Addr2=Addr3=0;

    switch(Control_p->pcmReaderIdentifier)
    {
    case PCM_READER_0:

        Addr1 = Control_p->BaseAddress.PCMReaderBaseAddr;
        Addr2 = Control_p->BaseAddress.PCMReaderBaseAddr +0x1C;
        Addr3 = Control_p->BaseAddress.PCMReaderBaseAddr+0x24;

        // Reset the IP
        STSYS_WriteRegDev32LE(Addr1,((U32)0x01));

        ControlReg.Bits.Mode        = PCMR_OFF;
        ControlReg.Bits.Format      = DataConfig_p->MemFormat;
        ControlReg.Bits.Rounding    = DataConfig_p->Rounding;
        ControlReg.Bits.SamplesRead = 0;

        STSYS_WriteRegDev32LE(Addr2,((U32)ControlReg.Reg));

        FormatReg.Bits.Order        = (DataConfig_p->MSBFirst)?PCMR_DATA_MSBFIRST:PCMR_DATA_LSBFIRST;
        FormatReg.Bits.Align        = (DataConfig_p->Alignment)?STAUD_DAC_DATA_ALIGNMENT_LEFT:STAUD_DAC_DATA_ALIGNMENT_RIGHT;// Inverted in the driver as the data sheet is wrong
        FormatReg.Bits.Padding      = (DataConfig_p->Padding)?PCMR_DELAY_DATA_ONEBIT_CLK:PCMR_NODELAY_DATA_BIT_CLK;//PCMR_DELAY_DATA_ONEBIT_CLK;
        FormatReg.Bits.SclkEdge     = (DataConfig_p->FallingSCLK)?PCMR_FALLINGEDGE_PCMSCLK:PCMR_RISINGEDGE_PCMSCLK;
        FormatReg.Bits.LRLevel      = (DataConfig_p->LeftLRHigh)?PCMR_LEFTWORD_LRCLKHIGH:PCMR_LEFTWORD_LRCLKLOW;
        FormatReg.Bits.DataLength   = DataConfig_p->Precision;
        FormatReg.Bits.BitsSubframe = DataConfig_p->BitsPerSubFrame;
        FormatReg.Bits.Reserved     = 0;

        STSYS_WriteRegDev32LE(Addr3, ((U32)FormatReg.Reg));
        STTBX_Print(("PCMR: Format Reg 0x%x\n",FormatReg.Reg));

        break;

    default:
        //STTBX_Print(("Unidentified PCM Reader identifier\n"));
        break;

    }
}

void PCMReader_EventHandler(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
    //PCMReaderControl_t * Control_p = (PCMReaderControl_t *)SubscriberData_p;

    if (Reason == CALL_REASON_NOTIFY_CALL)
    {
        switch(Event)
        {

            default :
                break;
        }
    }

}


ST_ErrorCode_t PCMReader_SubscribeEvents(PCMReaderControl_t * Control_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    //STEVT_DeviceSubscribeParams_t     EVT_SubcribeParams;
    //EVT_SubcribeParams.NotifyCallback = PCMReader_EventHandler;
    //EVT_SubcribeParams.SubscriberData_p = (void *)Control_p;


    return Error;

}

ST_ErrorCode_t PCMReader_UnSubscribeEvents(PCMReaderControl_t * Control_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

/* Register Reader control block as producer for sending AVSync events to all releated Modules  */
ST_ErrorCode_t PCMReader_RegisterEvents(PCMReaderControl_t * Control_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* Register to the events */
    return Error;
}

ST_ErrorCode_t PCMReader_UnRegisterEvents(PCMReaderControl_t * Control_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* UnRegister to the events */
    return Error;
}

/* Notify to the Subscribed Modules */

ST_ErrorCode_t Reader_NotifyEvt(U32 EvtType,U32 Value,PCMReaderControl_t *Control_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    /*STAUD_PTS_t   PTS;

    switch(EvtType)
    {

        default :
            break;
    }*/
    return Error;
}

ST_ErrorCode_t  STAud_PCMReaderGetCapability(STPCMREADER_Handle_t ReaderHandle, STAUD_ReaderCapability_t * Capability_p)
{
    ST_ErrorCode_t            Error         = ST_NO_ERROR;
    PCMReaderControlBlock_t * tempCtrlBlk_p = PCMReaderHead_p;


    /* Obtain the control block from the passed in handle */
    while (tempCtrlBlk_p != NULL)
    {
        if (tempCtrlBlk_p == (PCMReaderControlBlock_t *)ReaderHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempCtrlBlk_p = tempCtrlBlk_p->next_p;
    }

    if(tempCtrlBlk_p== NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Capability_p->NumChannels       = tempCtrlBlk_p->pcmReaderControl.NumChannels;
    Capability_p->I2SInputCapable   = TRUE;

    return (Error);
}

ST_ErrorCode_t      STAud_PCMReaderSetInputParams(STPCMREADER_Handle_t ReaderHandle, STAUD_PCMReaderConf_t *PCMInParams_p)
{
   PCMReaderControlBlock_t * tempCtrlBlk_p = PCMReaderHead_p;
   PCMReaderControl_t      * Control_p     = NULL;
   ST_ErrorCode_t            Error         = ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempCtrlBlk_p != NULL)
    {
        if (tempCtrlBlk_p == (PCMReaderControlBlock_t *)ReaderHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempCtrlBlk_p = tempCtrlBlk_p->next_p;
    }

    if(tempCtrlBlk_p== NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p = &(tempCtrlBlk_p->pcmReaderControl);

    STOS_MutexLock(Control_p->LockControlStructure);
    Control_p->pcmReaderDataConfig = *PCMInParams_p;
    Control_p->SamplingFrequency   = PCMInParams_p->Frequency;
    STOS_MutexRelease(Control_p->LockControlStructure);

    return (Error);
}


ST_ErrorCode_t      STAud_PCMReaderGetInputParams(STPCMREADER_Handle_t ReaderHandle, STAUD_PCMReaderConf_t * PCMInParams_p)
{
   PCMReaderControlBlock_t * tempCtrlBlk_p = PCMReaderHead_p;
   PCMReaderControl_t      * Control_p     = NULL;
   ST_ErrorCode_t            Error         = ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempCtrlBlk_p != NULL)
    {
        if (tempCtrlBlk_p == (PCMReaderControlBlock_t *)ReaderHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempCtrlBlk_p = tempCtrlBlk_p->next_p;
    }

    if(tempCtrlBlk_p== NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p = &(tempCtrlBlk_p->pcmReaderControl);

    STOS_MutexLock(Control_p->LockControlStructure);
    *PCMInParams_p = Control_p->pcmReaderDataConfig;
    STOS_MutexRelease(Control_p->LockControlStructure);

    return (Error);
}

ST_ErrorCode_t STAud_PCMReaderGetBufferParams(STPCMREADER_Handle_t ReaderHandle, STAUD_BufferParams_t* DataParams_p)
{
    PCMReaderControlBlock_t * tempCtrlBlk_p = PCMReaderHead_p;
    PCMReaderControl_t      * Control_p     = NULL;
    ST_ErrorCode_t            Error         = ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempCtrlBlk_p != NULL)
    {
        if (tempCtrlBlk_p == (PCMReaderControlBlock_t *)ReaderHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempCtrlBlk_p = tempCtrlBlk_p->next_p;
    }

    if(tempCtrlBlk_p== NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p = &(tempCtrlBlk_p->pcmReaderControl);

    STOS_MutexLock(Control_p->LockControlStructure);
    if(Control_p->BMHandle)
    {
        Error = MemPool_GetBufferParams(Control_p->BMHandle, DataParams_p);
    }
    else
    {
        Error = ST_ERROR_NO_MEMORY;
    }
    STOS_MutexRelease(Control_p->LockControlStructure);

    return Error;
}

/******************************************************************************
 *  Function name   : PCMRReleaseMemory
 *  Arguments       :
 *       IN         : tempCtrlBlk_p, PCM reader control block
 *                  :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will deallocate all allocated resources
 *                  :
 ***************************************************************************** */
ST_ErrorCode_t PCMRReleaseMemory(PCMReaderControlBlock_t * ControlBlock_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(ControlBlock_p)
    {
        PCMReaderControl_t * Control_p    = &(ControlBlock_p->pcmReaderControl);
        ST_Partition_t     * CPUPartition = Control_p->CPUPartition;
        STAVMEM_FreeBlockParams_t FreeBlockParams;

        if(Control_p->EvtHandle)
        {
            Error |= PCMReader_UnRegisterEvents(Control_p);
            Error |= PCMReader_UnSubscribeEvents(Control_p);
            // Close evt driver
            Error |= STEVT_Close(Control_p->EvtHandle);
            Control_p->EvtHandle = 0;
        }

        if(Control_p->pcmReaderChannelId)
        {
            Error |= STFDMA_UnlockChannel(Control_p->pcmReaderChannelId, Control_p->FDMABlock);

            Control_p->pcmReaderChannelId = 0;
        }

        if(Control_p->NodeHandle)
        {
            FreeBlockParams.PartitionHandle = Control_p->AVMEMPartition;
            Error |= STAVMEM_FreeBlock(&FreeBlockParams, &(Control_p->NodeHandle));
            Control_p->NodeHandle = 0;
        }

        if(Control_p->AudioBlock_p)
        {
            STOS_MemoryDeallocate(Control_p->CPUPartition, Control_p->AudioBlock_p);
            Control_p->AudioBlock_p = NULL;
        }

        if(Control_p->pcmReaderTaskSem_p)
        {
            Error |= STOS_SemaphoreDelete(NULL,Control_p->pcmReaderTaskSem_p);
            Control_p->pcmReaderTaskSem_p = NULL;
        }

        if(Control_p->LockControlStructure)
        {
            Error |= STOS_MutexDelete(Control_p->LockControlStructure);
            Control_p->LockControlStructure = NULL;
        }

        if(Control_p->CmdTransitionSem_p)
        {
            Error |= STOS_SemaphoreDelete(NULL,Control_p->CmdTransitionSem_p);
            Control_p->CmdTransitionSem_p = NULL;
        }

        STOS_MemoryDeallocate(CPUPartition, ControlBlock_p);
    }

    return Error;
}

