/*
 * Copyright (C) STMicroelectronics Ltd. 2004.
 *
 * All rights reserved.
 *                              ------->SPDIF PLAYER
 *                             |
 * DECODER -------|
 *                             |
 *                              ------->PCM PLAYER
 */
//#define  STTBX_PRINT
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "stos.h"
#include "aud_pcmplayer.h"
#include "audioreg.h"
#ifndef ST_51XX
extern U32 MAX_SAMPLES_PER_FRAME;
#endif

U32 PlayerUserCount = 0;

#define NUM_PCM_NODES_FOR_STARTUP   4
#define PTS_COUNT_FOR_UNMUTE              8
#define TIME_AFTERSYNC_FOR_UNMUTE    50 // in ms
#define DEFAULT_SAMPLING_FREQUENCY  48000

#define AVSYNC_ADDED    1
#define ENABLE_SYNC_CALC_IN_CALLBACK 1

ST_DeviceName_t PCMPlayer_DeviceName[] = {
#ifdef PCMP0_SUPPORTED
                                                        "PCMPLAYER0",
#endif
#ifdef PCMP1_SUPPORTED
                                                        "PCMPLAYER1",
#endif
#ifdef PCMP2_SUPPORTED
                                                        "PCMPLAYER2",
#endif
#ifdef PCMP3_SUPPORTED
                                                        "PCMPLAYER3",
#endif
#ifdef HDMIPCMP0_SUPPORTED
                                                        "HDMIPCMPLAYER",
#endif
                                                        };

/*Logging Related*/
#define LOG_AVSYNC  0
#if LOG_AVSYNC
    static U32 Aud_Array[2][10000];
    static U32 Aud_Count = 0;
    static U32 Aud_Array1[2][10000];
    static U32 Aud_Count1 = 0;
    static U32 restartcount = 0;
#endif

#define LOG_AVSYNC_LINUX    0
#if LOG_AVSYNC_LINUX
    #define NUM_OF_ELEMENTS 20
    U32 Log_To_Array = 1;
    U32 Aud_Array[NUM_OF_ELEMENTS][3];
    U32 Aud_Count = 0;

    U32 Aud_Pause_Array[NUM_OF_ELEMENTS][3];
    U32 Aud_Pause_Count = 0;
#endif

#define ENABLE_AUTO_TEST_EVTS   0

/*
******************************************************************************
Private Macros
******************************************************************************
*/

#define PTS_LATER_THAN(p1, p2)   (((unsigned int)(p2)-(unsigned int)(p1)) > 0x80000000)
#define PTS_DIFF(p1, p2)         (PTS_LATER_THAN(p1,p2) ? ((unsigned int)(p1)-(unsigned int)(p2)) \
                                                        : ((unsigned int)(p2)-(unsigned int)(p1)))

/* Convert a 90KHz value into ST20 ticks */
#define PTS_TO_TICKS(v)            (((v) * ST_GetClocksPerSecond()) / 90000)
#define TICKS_TO_PTS(v)            (((v) * 90000) / ST_GetClocksPerSecond())

/* Convert a 90KHz value into milliseconds.       */
/* ((v) / 90000 * 1000) = ((v) / 90)              */
#define PTS_TO_MILLISECOND(v)           ((v) / 90)

/* Convert a period in ms to a 90KHz value.       */
/* ((v) * 90000 / 1000) = ((v) * 90)              */
#define MILLISECOND_TO_PTS(v)           ((v) * 90)

#if defined(DVR)
    /* We can consider a discontinuity with a very huge value                                        */
    /* To be sure, we have to take in account the video bit buffer size and the lowest audio bit rate        */
    /* So at least HUGE_TIME_DIFFERENCE is ((video bit buffer size in bits)/(lowest audio bit rate) * 90000) */
    #define HUGE_TIME_DIFFERENCE_PAUSE                     (5 * 90000) /* 5 seconds at 90KHz */
    #define HUGE_TIME_DIFFERENCE_SKIP                        (((188 * 512 * 44 * 8) / 32000) * 90000) /* 17 minutes 38 secondes at 90KHz */
#else
    #define HUGE_TIME_DIFFERENCE_PAUSE                     180000 /* 2 seconds at 90KHz */
    #define HUGE_TIME_DIFFERENCE_SKIP                        180000 /* 2 seconds at 90KHz */
#endif

#define CORRECTIVE_ACTION_AUDIO_AHEAD_THRESHOLD         (10*90)   /* 10ms at 90KHz */
#define CORRECTIVE_ACTION_AUDIO_BEHIND_THRESHOLD        (24*90)   /* 24ms at 90KHz */


/* ---------------------------------------------------------------------------- */
/*                               Private Types                                  */
/* ---------------------------------------------------------------------------- */
PCMPlayerControlBlock_t * pcmPlayerControlBlock = NULL;

static ST_ErrorCode_t PCMReleaseMemory(PCMPlayerControlBlock_t * ControlBlock_p);

ST_ErrorCode_t STAud_PCMPlayerInit(ST_DeviceName_t  Name, STPCMPLAYER_Handle_t *handle,PCMPlayerInitParams_t *Param_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock, * lastPCMPlayerControlBlock_p = pcmPlayerControlBlock;
    PCMPlayerControl_t * Control_p;
    U32 i = 0;
    STAVMEM_AllocBlockParams_t  AllocParams;

    if ((strlen(Name) <= ST_MAX_DEVICE_NAME))
    {
        while (tempPCMplayerControlBlock_p != NULL)
        {
            if (strcmp(tempPCMplayerControlBlock_p->pcmPlayerControl.Name, Name) != 0)
            {
                lastPCMPlayerControlBlock_p = tempPCMplayerControlBlock_p;
                tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
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
    tempPCMplayerControlBlock_p = memory_allocate(Param_p->CPUPartition,sizeof(PCMPlayerControlBlock_t));
    if(!tempPCMplayerControlBlock_p)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(tempPCMplayerControlBlock_p,0,sizeof(PCMPlayerControlBlock_t));

    Control_p = &(tempPCMplayerControlBlock_p->pcmPlayerControl);
    strncpy(Control_p->Name, Name, ST_MAX_DEVICE_NAME);
    *handle = (STPCMPLAYER_Handle_t)tempPCMplayerControlBlock_p;

    // Get params from init params
    Control_p->pcmPlayerDataConfig      = Param_p->pcmPlayerDataConfig;

#if defined (ST_7109)
    //check for InvertBitClock value from cut for 7109
    {
        U32 Rev_7109 = ST_GetCutRevision();
        STTBX_Print(("7109 Revision %x\n",Rev_7109));
        Rev_7109 &= 0xF0;
        switch(Rev_7109)
        {
            case 0xC0:
                Control_p->pcmPlayerDataConfig.InvertBitClock = TRUE;
                break;
            default:
                break;

        }
    }
#endif
#if defined (ST_7200)
    Control_p->pcmPlayerDataConfig.InvertBitClock = TRUE;
#endif
    Control_p->CPUPartition = Param_p->CPUPartition;
    Control_p->AVMEMPartition = Param_p->AVMEMPartition;
    Control_p->DummyBufferPartition = Param_p->DummyBufferPartition;
    Control_p->pcmPlayerNumNode = Param_p->pcmPlayerNumNode;
    Control_p->pcmPlayerIdentifier = Param_p->pcmPlayerIdentifier;
    Control_p->memoryBlockManagerHandle = Param_p->memoryBlockManagerHandle;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    Control_p->CLKRV_Handle         = Param_p->CLKRV_Handle;
    Control_p->CLKRV_HandleForSynchro  = Param_p->CLKRV_HandleForSynchro;
#endif
    Control_p->NumChannels = Param_p->NumChannels;
    Control_p->EnableMSPPSupport = Param_p->EnableMSPPSupport;
    //strcpy(Control_p->EvtHandlerName, Param_p->EvtHandlerName);
    strncpy(Control_p->EvtHandlerName, Param_p->EvtHandlerName, ST_MAX_DEVICE_NAME);
    // Set default params for this PCM player
    Control_p->pcmPlayerCurrentState = PCMPLAYER_INIT;
    Control_p->pcmPlayerNextState = PCMPLAYER_INIT;
    Control_p->samplingFrequency = DEFAULT_SAMPLING_FREQUENCY;
    Control_p->pcmPlayerPlayed = 0;
    Control_p->pcmPlayerToProgram = 0;
    Control_p->pcmPlayerTransferId = 0;                   // Indicates that no transfer is running
    Control_p->pcmMute = FALSE;

    Control_p->playBackFromLive = FALSE;

    /* Set the FDMA block used */
    Control_p->FDMABlock = STFDMA_1;
    /*Debug Info */
    Control_p->FDMAAbortSent = 0;
    Control_p->FDMAAbortCBRecvd = 0;
    Control_p->AbortSent = FALSE;
    Control_p->EOFBlock_Received = FALSE;
    Control_p->AnalogMode = PCM_ON;
    Control_p->Pause_Performed = 0;
    Control_p->Skip_Performed = 0;
    Control_p->Huge_Diff = 0;

    if (Control_p->NumChannels == 2)
    {
        /* Set the FDMA request line count */
        switch(Control_p->pcmPlayerIdentifier)
        {
#ifdef PCMP0_SUPPORTED
#ifndef ST_51XX
            case PCM_PLAYER_0:
                if (Control_p->NumChannels == 2)
                {
                    Error |= STFDMA_SetTransferCount(PCM0_REQ_LINE, TW0CH_TRANSFER_COUNT, Control_p->FDMABlock);
                }
                else
                {
                    /*10 Channel case*/
                    Error |= STFDMA_SetTransferCount(PCM0_REQ_LINE, TENCH_TRANSFER_COUNT, Control_p->FDMABlock);
                }

                if(Error != ST_NO_ERROR)
                {
                    STTBX_Print((" PCM Player :: STFDMA_SetTransferCount Failed %x \n",Error));
                }
                break;
#endif
#endif
#ifdef PCMP1_SUPPORTED
            case PCM_PLAYER_1:
                if (Control_p->NumChannels == 2)
                {
                    Error |= STFDMA_SetTransferCount(PCM1_REQ_LINE, TW0CH_TRANSFER_COUNT, Control_p->FDMABlock);
                }
                else
                {
                    /*10 Channel case*/
                    Error |= STFDMA_SetTransferCount(PCM1_REQ_LINE, TENCH_TRANSFER_COUNT, Control_p->FDMABlock);
                }

                if(Error != ST_NO_ERROR)
                {
                    STTBX_Print((" PCM Player :: STFDMA_SetTransferCount Failed %x \n",Error));
                }

                break;
#endif
#ifdef PCMP2_SUPPORTED
            case PCM_PLAYER_2:
                if (Control_p->NumChannels == 2)
                {
                    Error |= STFDMA_SetTransferCount(PCM2_REQ_LINE, TW0CH_TRANSFER_COUNT, Control_p->FDMABlock);
                }
                else
                {
                    /*10 Channel case*/
                    Error |= STFDMA_SetTransferCount(PCM2_REQ_LINE, TENCH_TRANSFER_COUNT, Control_p->FDMABlock);
                }

                if(Error != ST_NO_ERROR)
                {
                    STTBX_Print((" PCM Player :: STFDMA_SetTransferCount Failed %x \n",Error));
                }
                break;
#endif
#ifdef PCMP3_SUPPORTED
            case PCM_PLAYER_3:
                if (Control_p->NumChannels == 2)
                {
                    Error |= STFDMA_SetTransferCount(PCM3_REQ_LINE, TW0CH_TRANSFER_COUNT, Control_p->FDMABlock);
                }
                else
                {
                    /*10 Channel case*/
                    Error |= STFDMA_SetTransferCount(PCM3_REQ_LINE, TENCH_TRANSFER_COUNT, Control_p->FDMABlock);
                }

                if(Error != ST_NO_ERROR)
                {
                    STTBX_Print((" PCM Player :: STFDMA_SetTransferCount Failed %x \n",Error));
                }
                break;
#endif
#ifdef HDMIPCMP0_SUPPORTED
            case HDMI_PCM_PLAYER_0:
          Control_p->HDMI_PcmPlayerEnable = FALSE;
                if (Control_p->NumChannels == 2)
                {
                    Error |= STFDMA_SetTransferCount(HDMI_PCM0_REQ_LINE, TW0CH_TRANSFER_COUNT, Control_p->FDMABlock);
                }
                else
                {
                    Error |= STFDMA_SetTransferCount(HDMI_PCM0_REQ_LINE, TENCH_TRANSFER_COUNT, Control_p->FDMABlock);
                }

                if(Error != ST_NO_ERROR)
                {
                    STTBX_Print((" PCM Player :: STFDMA_SetTransferCount Failed %x \n",Error));
                }
                break;
#endif
            case MAX_PCM_PLAYER_INSTANCE:
            default:
                break;
        }
    }

    Error = STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL, &(Control_p->pcmPlayerChannelId),Control_p->FDMABlock);
    if (Error != ST_NO_ERROR)
    {
        Error |= PCMReleaseMemory(tempPCMplayerControlBlock_p);
        return(Error);
    }

    Control_p->PlayerAudioBlock_p = memory_allocate(Param_p->CPUPartition,(sizeof(PlayerAudioBlock_t) * Control_p->pcmPlayerNumNode));
    if(!Control_p->PlayerAudioBlock_p)
    {
        Error = PCMReleaseMemory(tempPCMplayerControlBlock_p);
        return ST_ERROR_NO_MEMORY;
    }
    memset(Control_p->PlayerAudioBlock_p, 0, (sizeof(PlayerAudioBlock_t) * Control_p->pcmPlayerNumNode));

    AllocParams.PartitionHandle = Control_p->AVMEMPartition;
    AllocParams.Alignment = 32;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocParams.ForbiddenRangeArray_p = NULL;
    AllocParams.ForbiddenBorderArray_p = NULL;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.NumberOfForbiddenBorders = 0;
#ifdef MSPP_PARSER
    AllocParams.Size = sizeof(STFDMA_GenericNode_t) * (Control_p->pcmPlayerNumNode + 1) ;//for resetting the dummy buffer
#else
    AllocParams.Size = sizeof(STFDMA_GenericNode_t) * Control_p->pcmPlayerNumNode ;
#endif
    Error = STAVMEM_AllocBlock(&AllocParams, &(Control_p->PCMPlayerNodeHandle));
    if(Error != ST_NO_ERROR)
    {
        Error |= PCMReleaseMemory(tempPCMplayerControlBlock_p);
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(Control_p->PCMPlayerNodeHandle, (void *)(&(Control_p->pcmPlayerFDMANodesBaseAddress)));
    if(Error != ST_NO_ERROR)
    {
        Error |= PCMReleaseMemory(tempPCMplayerControlBlock_p);
        return Error;
    }

    if (Param_p->dummyBufferBaseAddress != NULL)
    {
        Control_p->dummyBufferSize = Param_p->dummyBufferSize;
        Control_p->dummyBufferBaseAddress = Param_p->dummyBufferBaseAddress;
    }
    else
    {
        Control_p->dummyBufferSize = (((MAX_SAMPLES_PER_FRAME/192)*192) * Control_p->NumChannels * BYTES_PER_CHANNEL_IN_PLAYER) * 2; // We need double the size of max pcm buffer

        if(Control_p->DummyBufferPartition)
        {
            AllocParams.PartitionHandle = Control_p->DummyBufferPartition;
            AllocParams.Alignment = 64;
            AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AllocParams.ForbiddenRangeArray_p = NULL;
            AllocParams.ForbiddenBorderArray_p = NULL;
            AllocParams.NumberOfForbiddenRanges = 0;
            AllocParams.NumberOfForbiddenBorders = 0;
            AllocParams.Size = Control_p->dummyBufferSize;

            Error = STAVMEM_AllocBlock(&AllocParams, &Control_p->DummyBufferHandle);
            if(Error != ST_NO_ERROR)
            {
                Error |= PCMReleaseMemory(tempPCMplayerControlBlock_p);
                return Error;
            }

            Error = STAVMEM_GetBlockAddress(Control_p->DummyBufferHandle, (void *)(&(Control_p->dummyBufferBaseAddress)));
            if(Error != ST_NO_ERROR)
            {
                Error |= PCMReleaseMemory(tempPCMplayerControlBlock_p);
                return Error;
            }

        }
#ifndef STAUD_REMOVE_EMBX
        else
        {
            Error = EMBX_OpenTransport(EMBX_TRANSPORT_NAME, &Control_p->tp);
            if(Error != EMBX_SUCCESS)
            {
                Error |= PCMReleaseMemory(tempPCMplayerControlBlock_p);
                return Error;
            }

            Error = EMBX_Alloc(Control_p->tp, Control_p->dummyBufferSize, (EMBX_VOID **)(&Control_p->dummyBufferBaseAddress));
            if(Error != EMBX_SUCCESS)
            {
                Error |= PCMReleaseMemory(tempPCMplayerControlBlock_p);
                return Error;
            }
        }
#endif
    }

#ifndef MSPP_PARSER
    memset((char *)Control_p->dummyBufferBaseAddress, 0, Control_p->dummyBufferSize);
#endif

    Control_p->DummyBuffer.BasePhyAddress = (U32*)STOS_VirtToPhys(Control_p->dummyBufferBaseAddress);
    Control_p->DummyBuffer.BaseUnCachedVirtualAddress = (U32*)Control_p->dummyBufferBaseAddress;
    Control_p->DummyBuffer.BaseCachedVirtualAddress=NULL;
    for (i=0; i < Control_p->pcmPlayerNumNode; i++)
    {
        STFDMA_GenericNode_t    * PlayerDmaNode_p = 0;

        PlayerDmaNode_p = (((STFDMA_GenericNode_t   *)Control_p->pcmPlayerFDMANodesBaseAddress) + i);
        Control_p->PlayerAudioBlock_p[i].PlayerDmaNode_p = PlayerDmaNode_p;
        Control_p->PlayerAudioBlock_p[i].PlayerDMANodePhy_p = (U32*)STOS_VirtToPhys(PlayerDmaNode_p);

        if(i < (Control_p->pcmPlayerNumNode - 1))
        {
            PlayerDmaNode_p->Node.Next_p = (struct  STFDMA_Node_s *)STOS_VirtToPhys((U32)(PlayerDmaNode_p + 1));
        }
        else
        {
            PlayerDmaNode_p->Node.Next_p = (struct  STFDMA_Node_s *)STOS_VirtToPhys((U32)Control_p->pcmPlayerFDMANodesBaseAddress );
        }

        // Fill fdma node default parameters
        PlayerDmaNode_p->Node.NumberBytes           = 0;
        PlayerDmaNode_p->Node.SourceAddress_p       = 0;

#ifdef PCMP0_SUPPORTED
        if(Control_p->pcmPlayerIdentifier == PCM_PLAYER_0)
        {
            PlayerDmaNode_p->Node.DestinationAddress_p  = (void *)(PCMPLAYER0_BASE + PCMP_DATA); /* Address of the PCM Player */
            PlayerDmaNode_p->Node.NodeControl.PaceSignal    = STFDMA_REQUEST_SIGNAL_PCM0;
        } else
#endif
#ifdef PCMP1_SUPPORTED
        if (Control_p->pcmPlayerIdentifier == PCM_PLAYER_1)
        {
            PlayerDmaNode_p->Node.DestinationAddress_p  = (void *)(PCMPLAYER1_BASE + PCMP_DATA); /* Address of the PCM Player */
            PlayerDmaNode_p->Node.NodeControl.PaceSignal    = STFDMA_REQUEST_SIGNAL_PCM1;
        } else
#endif
#ifdef PCMP2_SUPPORTED
        if (Control_p->pcmPlayerIdentifier == PCM_PLAYER_2)
        {
            PlayerDmaNode_p->Node.DestinationAddress_p  = (void *)(PCMPLAYER2_BASE + PCMP_DATA); /* Address of the PCM Player */
            PlayerDmaNode_p->Node.NodeControl.PaceSignal    = STFDMA_REQUEST_SIGNAL_PCM2;
        } else
#endif
#ifdef PCMP3_SUPPORTED
        if (Control_p->pcmPlayerIdentifier == PCM_PLAYER_3)
        {
            PlayerDmaNode_p->Node.DestinationAddress_p  = (void *)(PCMPLAYER3_BASE + PCMP_DATA); /* Address of the PCM Player */
            PlayerDmaNode_p->Node.NodeControl.PaceSignal    = STFDMA_REQUEST_SIGNAL_PCM3;
        } else
#endif
#ifdef HDMIPCMP0_SUPPORTED
        if (Control_p->pcmPlayerIdentifier == HDMI_PCM_PLAYER_0)
        {
            PlayerDmaNode_p->Node.DestinationAddress_p  = (void *)(HDMI_PCMPLAYER_BASE + PCMP_DATA); /* Address of the hdmi PCM Player */
            PlayerDmaNode_p->Node.NodeControl.PaceSignal    = STFDMA_REQUEST_SIGNAL_HDMI_PCM_PLYR;
        }
#endif
        {
            /* Dummy {} to handle last else */
        }

#if !defined(ST_7200)
        PlayerDmaNode_p->Node.DestinationAddress_p  = (void *)STOS_VirtToPhys((U32)PlayerDmaNode_p->Node.DestinationAddress_p );
#endif
        PlayerDmaNode_p->Node.Length                        = 0;
        PlayerDmaNode_p->Node.SourceStride              = 0;
        PlayerDmaNode_p->Node.DestinationStride     = 0;
        PlayerDmaNode_p->Node.NodeControl.SourceDirection           = STFDMA_DIRECTION_INCREMENTING;
        PlayerDmaNode_p->Node.NodeControl.DestinationDirection      = STFDMA_DIRECTION_STATIC;
        PlayerDmaNode_p->Node.NodeControl.NodeCompleteNotify        = TRUE;
        PlayerDmaNode_p->Node.NodeControl.NodeCompletePause     = FALSE;
        PlayerDmaNode_p->Node.NodeControl.Reserved                  = 0;

#if defined(ST_7109) || defined(ST_7200)
    PlayerDmaNode_p->Node.NodeControl.Reserved1 = 0;
        if (Control_p->EnableMSPPSupport == TRUE)
        {
            PlayerDmaNode_p->Node.NodeControl.Secure = 1;
        }
        else
        {
            PlayerDmaNode_p->Node.NodeControl.Secure = 0;
        }
#endif
        Control_p->PlayerAudioBlock_p[i].Valid              = FALSE;
        Control_p->PlayerAudioBlock_p[i].memoryBlock        = NULL;
    }

#ifdef MSPP_PARSER
    Control_p->PCMPlayerDmaNode_p = (((STFDMA_GenericNode_t *)Control_p->pcmPlayerFDMANodesBaseAddress) + Control_p->pcmPlayerNumNode);
    Control_p->MemcpyNodeAddress.BaseUnCachedVirtualAddress =(U32*)Control_p->PCMPlayerDmaNode_p;
    Control_p->MemcpyNodeAddress.BasePhyAddress = (U32*)STOS_VirtToPhys(Control_p->PCMPlayerDmaNode_p);
    Control_p->PCMPlayerDmaNode_p->Node.NumberBytes                     = 0; /*will change depending upon frame Size*/
    Control_p->PCMPlayerDmaNode_p->Node.SourceAddress_p                 = NULL;/* will change depending upon Offset  */
    Control_p->PCMPlayerDmaNode_p->Node.DestinationAddress_p                =NULL; /* will change depening upon ES Buffer*/
    Control_p->PCMPlayerDmaNode_p->Node.Length                              = 0;  /* will chnage depending upon frame size*/
    Control_p->PCMPlayerDmaNode_p->Node.SourceStride                        = 0;
    Control_p->PCMPlayerDmaNode_p->Node.DestinationStride                   = 0;
    Control_p->PCMPlayerDmaNode_p->Node.NodeControl.PaceSignal              = STFDMA_REQUEST_SIGNAL_NONE;
    Control_p->PCMPlayerDmaNode_p->Node.NodeControl.SourceDirection     = STFDMA_DIRECTION_INCREMENTING;
    Control_p->PCMPlayerDmaNode_p->Node.NodeControl.DestinationDirection    = STFDMA_DIRECTION_INCREMENTING;
    Control_p->PCMPlayerDmaNode_p->Node.NodeControl.NodeCompleteNotify      = TRUE;
    Control_p->PCMPlayerDmaNode_p->Node.NodeControl.NodeCompletePause       = TRUE;
    Control_p->PCMPlayerDmaNode_p->Node.NodeControl.Reserved                    = 0;
    Control_p->PCMPlayerDmaNode_p->Node.Next_p=NULL;
#endif

    // Initialize Tranfer player params for FDMA
    Control_p->TransferPlayerParams_p.ChannelId     = Control_p->pcmPlayerChannelId;
    Control_p->TransferPlayerParams_p.Pool              = STFDMA_DEFAULT_POOL;
    Control_p->TransferPlayerParams_p.NodeAddress_p = Control_p->PlayerAudioBlock_p[0].PlayerDmaNode_p;
    Control_p->TransferPlayerParams_p.BlockingTimeout   = 0;
    Control_p->TransferPlayerParams_p.CallbackFunction  = (STFDMA_CallbackFunction_t)PCMPlayerFDMACallbackFunction;
    Control_p->TransferPlayerParams_p.ApplicationData_p= (void *)(tempPCMplayerControlBlock_p);
    Control_p->TransferPlayerParams_p.FDMABlock = Control_p->FDMABlock;


    Control_p->pcmPlayerTaskSem_p = STOS_SemaphoreCreateFifo(NULL,0);
    Control_p->pcmPlayerCmdTransitionSem_p = STOS_SemaphoreCreateFifo(NULL,0);
    Control_p->LockControlStructure = STOS_MutexCreateFifo();

       /*Get the Physical Address of Block Manager output */
    {
        STAUD_BufferParams_t  BufferParams;
        Error |= MemPool_GetBufferParams(Param_p->memoryBlockManagerHandle,&BufferParams);
        if(Error!=ST_NO_ERROR)
        {
            Error |= PCMReleaseMemory(tempPCMplayerControlBlock_p);
            return Error;
        }
        Control_p->PlayerInputMemoryAddress.BaseUnCachedVirtualAddress=(U32*)BufferParams.BufferBaseAddr_p;
        Control_p->PlayerInputMemoryAddress.BasePhyAddress=(U32*)STOS_VirtToPhys(BufferParams.BufferBaseAddr_p);
        Control_p->PlayerInputMemoryAddress.BaseCachedVirtualAddress=NULL;

    }

    /*Set AVSync Params*/
    Control_p->AVSyncEnable                     = FALSE;
    ResetPCMPlayerSyncParams(&(tempPCMplayerControlBlock_p->pcmPlayerControl));
    Control_p->audioSTCSync.UsrLatency          = 0; /*User Set latency can be +ve*/
    Control_p->audioSTCSync.Offset              = 0;

    {
        STEVT_OpenParams_t EVT_OpenParams;
        /* Open the ST Event */
        Error |= STEVT_Open(Control_p->EvtHandlerName,&EVT_OpenParams,&Control_p->EvtHandle);
        if (Error == ST_NO_ERROR)
        {
            strncpy(Control_p->EvtGeneratorName, Param_p->EvtGeneratorName, ST_MAX_DEVICE_NAME);
            Control_p->ObjectID = Param_p->ObjectID;
            Control_p->skipEventID = Param_p->skipEventID;
            Control_p->pauseEventID = Param_p->pauseEventID;
            PCMPlayer_RegisterEvents(Control_p);
            PCMPlayer_SubscribeEvents(Control_p);
        }
    }

    Control_p->pcmRestart = FALSE;
    tempPCMplayerControlBlock_p->next = NULL;

    Error = STAud_PlayerMemRemap(&Control_p->BaseAddress);
    if(Error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_PlayerMemRemap Failed %x \n",Error));
    }

#if defined (ST_OS20)
    STOS_TaskCreate(PCMPlayerTask,(void *)tempPCMplayerControlBlock_p,Control_p->CPUPartition,PCMPLAYER_STACK_SIZE,(void **)&Control_p->pcmPlayerTask_pstack,Control_p->CPUPartition,
                                        &Control_p->pcmPlayerTask_p,&Control_p->pcmPlayerTaskDesc,PCMPLAYER_TASK_PRIORITY, Name, 0);

#else
    STOS_TaskCreate(PCMPlayerTask,(void *)tempPCMplayerControlBlock_p, NULL, PCMPLAYER_STACK_SIZE, NULL, NULL,
                            &Control_p->pcmPlayerTask_p, NULL, PCMPLAYER_TASK_PRIORITY, Name, 0);
#endif


    if (Control_p->pcmPlayerTask_p == NULL)
    {
        Error = PCMReleaseMemory(tempPCMplayerControlBlock_p);
        return ST_ERROR_NO_MEMORY;
    }

    if (pcmPlayerControlBlock == NULL)
    {
        pcmPlayerControlBlock = tempPCMplayerControlBlock_p;
    }
    else
    {
        lastPCMPlayerControlBlock_p->next = tempPCMplayerControlBlock_p;
    }

#if ENABLE_PCM_INTERRUPT
    PCMPlayerSetInterruptNumber(Control_p);
    if (STOS_SUCCESS == STOS_InterruptInstall(Control_p->InterruptNumber,0,PcmPInterruptHandler,"PCMP",(void *)tempPCMplayerControlBlock_p ))
    {
        if (STOS_SUCCESS != STOS_InterruptEnable(Control_p->InterruptNumber,0))
            {
                STOS_InterruptUninstall(Control_p->InterruptNumber,0,tempPCMplayerControlBlock_p);
                STTBX_Print(("ERROR: Could not enable interrupt\n"));
            }
    }
    else
    {
        STTBX_Print(("ERROR: Interrupt handler load failed\n"));
    }
#endif
/*
    {
        U32 * HoldoffReg ;
        HoldoffReg=(U32*)0xB9220000 + 0x45F0;
        HoldoffReg = ioremap((U32)HoldoffReg,4);
        //HoldoffReg+=0x45F0;
        *HoldoffReg= 3 * 266;
    }
*/
    return (Error);
}
ST_ErrorCode_t      STAud_PCMPlayerTerminate(STPCMPLAYER_Handle_t handle)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock, * lastPCMPlayerControlBlock_p = pcmPlayerControlBlock;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    PCMPlayerControl_t *Control_p;

    while (tempPCMplayerControlBlock_p != (PCMPlayerControlBlock_t *)handle)
    {
        lastPCMPlayerControlBlock_p = tempPCMplayerControlBlock_p;
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if (tempPCMplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p = &(tempPCMplayerControlBlock_p->pcmPlayerControl);

#if ENABLE_PCM_INTERRUPT
    PCMPlayerDisbleInterrupt(Control_p);


    if (STOS_SUCCESS == STOS_InterruptDisable(Control_p->InterruptNumber,0))
    {
        if (STOS_SUCCESS != STOS_InterruptUninstall(Control_p->InterruptNumber,0,tempPCMplayerControlBlock_p))
        {
            STTBX_Print(("ERROR: Could not uninstall interrupt\n"));
        }
    }
    else
    {
        STTBX_Print(("ERROR: Could not diable interrupt\n"));
    }

#endif

    Error |= PCMPlayer_UnRegisterEvents(Control_p);
    Error |= PCMPlayer_UnSubscribeEvents(Control_p);
    // Close evt driver
    Error |= STEVT_Close(Control_p->EvtHandle);

    Error |=STFDMA_UnlockChannel(Control_p->pcmPlayerChannelId, Control_p->FDMABlock);

    FreeBlockParams.PartitionHandle = Control_p->AVMEMPartition;
    Error |= STAVMEM_FreeBlock(&FreeBlockParams, &(Control_p->PCMPlayerNodeHandle));

    // Deallocate audio block
    memory_deallocate(Control_p->CPUPartition, Control_p->PlayerAudioBlock_p);

    //deallocate dummybuffer
    if(Control_p->dummyBufferBaseAddress)
    {
        if(Control_p->DummyBufferHandle && Control_p->DummyBufferPartition)
        {
            FreeBlockParams.PartitionHandle = Control_p->DummyBufferPartition;
            Error |= STAVMEM_FreeBlock(&FreeBlockParams, &(Control_p->DummyBufferHandle));
            Control_p->DummyBufferHandle = 0;
        }
#ifndef STAUD_REMOVE_EMBX
        else if (Control_p->tp)
        {
            Error |= EMBX_Free((EMBX_VOID *)Control_p->dummyBufferBaseAddress);
            Error |= EMBX_CloseTransport(Control_p->tp);
        }
#endif
        Control_p->dummyBufferBaseAddress = NULL;
        Control_p->dummyBufferSize = 0;
    }

    Error |= STOS_SemaphoreDelete(NULL,Control_p->pcmPlayerTaskSem_p);
    Error |= STOS_MutexDelete(Control_p->LockControlStructure);
    Error |= STOS_SemaphoreDelete(NULL,Control_p->pcmPlayerCmdTransitionSem_p);
#if 0
#if LINUX_PCM_INTERRUPT
    /* Enable the Under flow interrupt */
    *((U32*)(Control_p->BaseAddress.PCMPlayer0BaseAddr + 0x018)) = 0x00000001;

    STOS_InterruptUninstall(Interrupt_number,0, tempPCMplayerControlBlock_p);

#endif //LINUX_PCM_INTERRUPT
#endif
    STAud_PlayerMemUnmap(&Control_p->BaseAddress);

    lastPCMPlayerControlBlock_p->next = tempPCMplayerControlBlock_p->next;
    // deallocate control block
    if (tempPCMplayerControlBlock_p == pcmPlayerControlBlock)
        pcmPlayerControlBlock = pcmPlayerControlBlock->next;//NULL;
    memory_deallocate(Control_p->CPUPartition, tempPCMplayerControlBlock_p);

    return (Error);
}

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t STAud_PCMPlayerSetClkSynchro(STPCMPLAYER_Handle_t handle,STCLKRV_Handle_t ClkSource)
{
    PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock, * lastPCMPlayerControlBlock_p = pcmPlayerControlBlock;

    while (tempPCMplayerControlBlock_p != (PCMPlayerControlBlock_t *)handle)
    {
        lastPCMPlayerControlBlock_p = tempPCMplayerControlBlock_p;
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if (tempPCMplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    tempPCMplayerControlBlock_p->pcmPlayerControl.CLKRV_HandleForSynchro=ClkSource;

    return(ST_NO_ERROR);
}
#endif

void    PCMPlayerTask(void * tempPCMplayerControlBlock_p)
{
    PCMPlayerControlBlock_t * tempPCMPlayerControlBlock_p = (PCMPlayerControlBlock_t *)tempPCMplayerControlBlock_p;
    PCMPlayerControl_t      * Control_p     = &(tempPCMPlayerControlBlock_p->pcmPlayerControl);
    PlayerAudioBlock_t  * PlayerAudioBlock_p    = Control_p->PlayerAudioBlock_p;
    AudAVSync_t         * PlayerAVsync_p = &(Control_p->audioAVsync);
    ST_ErrorCode_t          err = ST_NO_ERROR;
    U32                     i = 0;
    STOS_TaskEnter(tempPCMPlayerControlBlock_p);

    while (1)
    {
        STOS_SemaphoreWait(Control_p->pcmPlayerTaskSem_p);

        switch (Control_p->pcmPlayerCurrentState)
        {
            case PCMPLAYER_INIT:
                STOS_MutexLock(Control_p->LockControlStructure);
                if (Control_p->pcmPlayerNextState != Control_p->pcmPlayerCurrentState)
                {
                    Control_p->pcmPlayerCurrentState = Control_p->pcmPlayerNextState;
                    //STTBX_Print(("PCM:from PCMPLAYER_INIT\n"));
                    STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);
                    // Signal the command completion transtition
                    STOS_SemaphoreSignal(Control_p->pcmPlayerCmdTransitionSem_p);
                }
                STOS_MutexRelease(Control_p->LockControlStructure);
                if(Control_p->pcmPlayerCurrentState == PCMPLAYER_START)
                {
                    ResetPCMPlayerSyncParams(Control_p);
                    //SetPCMPlayerSamplingFrequency(Control_p); // Moved to just before StartPCMPlayerIP
                }
#ifdef ST_51XX
                PCMPlayer_DACConfigure(Control_p);
#endif

                break;
            case PCMPLAYER_START:

                Control_p->Pause_Performed=0;
                Control_p->Skip_Performed=0;
                Control_p->Huge_Diff=0;
                if(Control_p->AnalogMode != PCM_OFF)
                {
#if AVSYNC_ADDED
                    if (Control_p->playBackFromLive && !PlayerAVsync_p->GenerateMute)
                        {
                            //STTBX_Print(("Setting Mute for Live decode\n"));
                            PlayerAVsync_p->GenerateMute = TRUE;
                            PlayerAVsync_p->ValidPTSCount = 0;
                        }
#endif
                    // Get number of buffers required for startup
                    for (i=Control_p->pcmPlayerPlayed; i < (Control_p->pcmPlayerPlayed + NUM_PCM_NODES_FOR_STARTUP); i++)
                    {
                        U8 Index = (i % Control_p->pcmPlayerNumNode);
                        if (PlayerAudioBlock_p[Index].Valid == FALSE)
                        {
#if MEMORY_SPLIT_SUPPORT
                            if (MemPool_GetSplitIpBlk(Control_p->memoryBlockManagerHandle,
                                          (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock),
                                          (U32)tempPCMPlayerControlBlock_p,
                                          &PlayerAudioBlock_p[Index].MemoryParams) != ST_NO_ERROR)
#else
                            if (MemPool_GetIpBlk(Control_p->memoryBlockManagerHandle,
                                          (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock),
                                          (U32)tempPCMPlayerControlBlock_p) != ST_NO_ERROR)
#endif
                            {
                                break;
                            }
                            else
                            {
                                if(PCMPlayer_StartupSync(&PlayerAudioBlock_p[Index],tempPCMPlayerControlBlock_p) == FALSE)
                                {
                                    break;
                                }
                                STOS_MutexLock(Control_p->LockControlStructure);
                                FillPCMPlayerFDMANode(&PlayerAudioBlock_p[Index], Control_p);
                                STOS_MutexRelease(Control_p->LockControlStructure);

                                {
#if MEMORY_SPLIT_SUPPORT
                                    U32 Flags = PlayerAudioBlock_p[Index].MemoryParams.Flags;
#else
                                    U32 Flags = PlayerAudioBlock_p[Index].memoryBlock->Flags;
#endif
                                    if(Flags & EOF_VALID)
                                    {
                                        Control_p->EOFBlock_Received =TRUE;
                                    }
                                }

                                PlayerAudioBlock_p[Index].Valid = TRUE;
                            }
                        }
                    }

                    // If all the required nodes are valid start the transfer
                    if (PlayerAudioBlock_p[(Control_p->pcmPlayerPlayed + NUM_PCM_NODES_FOR_STARTUP - 1) % Control_p->pcmPlayerNumNode].Valid ||
                        Control_p->EOFBlock_Received)
                    {

#ifdef MSPP_PARSER
                        {
                            U32 length = 0,sourceStride,destinationStride,buffersize ;
                            void * destinationAddress,* sourceAddress;
                            U32 localoffset = (Control_p->pcmPlayerPlayed + NUM_PCM_NODES_FOR_STARTUP - 1) % Control_p->pcmPlayerNumNode;
                            if(PlayerAudioBlock_p[localoffset].Valid)
                            {
                                buffersize = Control_p->dummyBufferSize;
                                sourceAddress = (void *)PlayerAudioBlock_p[localoffset].memoryBlock->MemoryStart;
                                destinationAddress = Control_p->DummyBuffer.BasePhyAddress;
                                sourceStride = 1;
                                destinationStride = 1;

                                sourceAddress = (void *)STOS_AddressTranslate(Control_p->PlayerInputMemoryAddress.BasePhyAddress,Control_p->PlayerInputMemoryAddress.BaseUnCachedVirtualAddress,sourceAddress);

                                while(buffersize)
                                {
                                    destinationAddress = (void *)((U32)destinationAddress + length);
                                    if(buffersize >= PlayerAudioBlock_p[localoffset].memoryBlock->Size)
                                    {
                                        length = PlayerAudioBlock_p[localoffset].memoryBlock->Size;
                                        buffersize -= length;
                                    }
                                    else
                                    {
                                        length = buffersize;
                                        buffersize = 0;
                                    }
                                    FDMAMemcpy(destinationAddress,sourceAddress,length,length,sourceStride,destinationStride,&Control_p->MemcpyNodeAddress);
                                }
                            }
                        }
#endif
#if ENABLE_AUTO_TEST_EVTS
                        Player_NotifyEvt(STAUD_TEST_PCM_START_EVT,0,Control_p);
#endif
                        {
                            U32 BlockIndex = Control_p->pcmPlayerPlayed % Control_p->pcmPlayerNumNode;
                            MemoryBlock_t * MemoryBlock_p = PlayerAudioBlock_p[BlockIndex].memoryBlock;
#if MEMORY_SPLIT_SUPPORT
                            U32 Flags = PlayerAudioBlock_p[BlockIndex].MemoryParams.Flags;
#else
                            U32 Flags = MemoryBlock_p->Flags;
#endif

                            if ((Flags & FREQ_VALID) && (Control_p->samplingFrequency != MemoryBlock_p->Data[FREQ_OFFSET]))
                            {
                                Control_p->samplingFrequency = MemoryBlock_p->Data[FREQ_OFFSET];
                                SetPCMPlayerSamplingFrequency(Control_p);
                            }
                        }

                        Control_p->pcmPlayerCurrentState = PCMPLAYER_PLAYING;
                        SetPCMPlayerSamplingFrequency(Control_p);
                        StartPCMPlayerIP(Control_p);
                        /*while (1)
                        {
                            *((volatile U32*)(PCM_PLAYER_BASE_ADDRESS + PCMP_DATA)) = 0xaaaaaaaa;
                        }*/

                        Control_p->pcmPlayerToProgram = Control_p->pcmPlayerPlayed;

                        for(i = Control_p->pcmPlayerToProgram; i < (Control_p->pcmPlayerToProgram + NUM_PCM_NODES_FOR_STARTUP); i++)
                        {
                            if (PlayerAudioBlock_p[i % Control_p->pcmPlayerNumNode].Valid)
                            {
                                PlayerAudioBlock_p[i % Control_p->pcmPlayerNumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = FALSE;
                            }
                            else
                            {
                                break;
                            }
                        }
                        /* Set PAUSE for last block in queue*/
                        PlayerAudioBlock_p[(i -1) % Control_p->pcmPlayerNumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = TRUE;

                        /* Set ToProgram to the last node in queue*/
                        Control_p->pcmPlayerToProgram = (i-1);
                        Control_p->TransferPlayerParams_p.NodeAddress_p = (STFDMA_GenericNode_t *)(U32)PlayerAudioBlock_p[Control_p->pcmPlayerPlayed % Control_p->pcmPlayerNumNode].PlayerDMANodePhy_p ;

                        err = STFDMA_StartGenericTransfer(&(Control_p->TransferPlayerParams_p), &(Control_p->pcmPlayerTransferId));
                        if(err != ST_NO_ERROR)
                        {
                            Control_p->pcmPlayerCurrentState = PCMPLAYER_START;
                            STTBX_Print(("PCM:Start failure\n"));
                            STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);

                        }
                        else
                        {
                            ResetPCMPLayerIP(Control_p);
#if ENABLE_PCM_INTERRUPT
                            PCMPlayerEnableInterrupt(Control_p);
#endif
                            Control_p->EOFBlock_Received = FALSE;
                            STTBX_Print(("PCM:Started\n"));
                        }
                    }


                    STOS_MutexLock(Control_p->LockControlStructure);
                    if (Control_p->pcmPlayerNextState == PCMPLAYER_STOPPED)
                    {
                        //STTBX_Print(("PCM:from PCMPLAYER_START\n"));
                        Control_p->pcmPlayerCurrentState = Control_p->pcmPlayerNextState;
                        if (Control_p->pcmPlayerTransferId != 0)
                        {
                            // FDMA transfer is already started
                            STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);
                        }
                        else
                        {
                            // FDMA transfer was not started
                            for (i=Control_p->pcmPlayerPlayed; i < (Control_p->pcmPlayerPlayed + Control_p->pcmPlayerNumNode); i++)
                            {
                                U8 Index = i % Control_p->pcmPlayerNumNode;
                                if (PlayerAudioBlock_p[Index].Valid == TRUE)
                                {
                                    STTBX_Print(("Freeing MBlock in PCMP start \n"));
                                    if (PCMPFreeIpBlk(tempPCMPlayerControlBlock_p, &PlayerAudioBlock_p[Index]) != ST_NO_ERROR)
                                    {
                                        STTBX_Print(("Error in releasing Ip Pcm player mem block\n"));
                                    }
                                    PlayerAudioBlock_p[Index].Valid = FALSE;

                                }
                            }
                            // Signal the command completion transtition
                            STTBX_Print(("PCMP:Going to Stop from start\n"));
                            STOS_SemaphoreSignal(Control_p->pcmPlayerCmdTransitionSem_p);
                        }
                    }
                    STOS_MutexRelease(Control_p->LockControlStructure);
                }

                break;
            case PCMPLAYER_RESTART:

#if LOG_AVSYNC
                //STTBX_Print(("PCMPR  =%d\n",restartcount++));

#endif
#if AVSYNC_ADDED
                // Get number of buffers required for startup
                if (Control_p->playBackFromLive && !PlayerAVsync_p->GenerateMute)
                {
                    //STTBX_Print(("Setting Mute for Live decode\n"));
                    PlayerAVsync_p->GenerateMute = TRUE;
                    PlayerAVsync_p->ValidPTSCount = 0;
                }
#endif
                for (i=Control_p->pcmPlayerPlayed; i < (Control_p->pcmPlayerPlayed + NUM_PCM_NODES_FOR_STARTUP); i++)
                {
                    U8 Index = (i % Control_p->pcmPlayerNumNode);
                    if (PlayerAudioBlock_p[Index].Valid == FALSE)
                    {
#if MEMORY_SPLIT_SUPPORT
                        if (MemPool_GetSplitIpBlk(Control_p->memoryBlockManagerHandle,
                                      (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock),
                                      (U32)tempPCMPlayerControlBlock_p,
                                      &PlayerAudioBlock_p[Index].MemoryParams) != ST_NO_ERROR)
#else
                        if (MemPool_GetIpBlk(Control_p->memoryBlockManagerHandle,
                                      (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock),
                                      (U32)tempPCMPlayerControlBlock_p) != ST_NO_ERROR)
#endif
                        {
                            break;
                        }
                        else
                        {
                            if(PCMPlayer_StartupSync(&PlayerAudioBlock_p[Index],tempPCMPlayerControlBlock_p) == FALSE)
                            {
                                break;
                            }
                            STOS_MutexLock(Control_p->LockControlStructure);
                            FillPCMPlayerFDMANode(&PlayerAudioBlock_p[Index], Control_p);
                            STOS_MutexRelease(Control_p->LockControlStructure);

                            {
#if MEMORY_SPLIT_SUPPORT
                                U32 Flags = PlayerAudioBlock_p[Index].MemoryParams.Flags;
#else
                                U32 Flags = PlayerAudioBlock_p[Index].memoryBlock->Flags;
#endif
                                if(Flags & EOF_VALID)
                                {
                                    Control_p->EOFBlock_Received =TRUE;
                                }
                            }
                            PlayerAudioBlock_p[Index].Valid = TRUE;
                        }
                    }
                }

                // If all the required nodes are valid start the transfer
                if (PlayerAudioBlock_p[(Control_p->pcmPlayerPlayed + NUM_PCM_NODES_FOR_STARTUP - 1) % Control_p->pcmPlayerNumNode].Valid ||
                    Control_p->EOFBlock_Received)
                {
                    {
                        U32 BlockIndex = Control_p->pcmPlayerPlayed % Control_p->pcmPlayerNumNode;
                        MemoryBlock_t * MemoryBlock_p = PlayerAudioBlock_p[BlockIndex].memoryBlock;
#if MEMORY_SPLIT_SUPPORT
                        U32 Flags = PlayerAudioBlock_p[BlockIndex].MemoryParams.Flags;
#else
                        U32 Flags = MemoryBlock_p->Flags;
#endif
                        if ((Flags & FREQ_VALID) && (Control_p->samplingFrequency != MemoryBlock_p->Data[FREQ_OFFSET]))
                        {
                            Control_p->samplingFrequency = MemoryBlock_p->Data[FREQ_OFFSET];
                            SetPCMPlayerSamplingFrequency(Control_p);
                        }
                    }

                    Control_p->pcmPlayerCurrentState = PCMPLAYER_PLAYING;

                    Control_p->pcmPlayerToProgram = Control_p->pcmPlayerPlayed;
#if ENABLE_AUTO_TEST_EVTS
                    Player_NotifyEvt(STAUD_TEST_PCM_RESTART_EVT,0,Control_p);
#endif
                    for(i = Control_p->pcmPlayerToProgram; i < (Control_p->pcmPlayerToProgram + NUM_PCM_NODES_FOR_STARTUP); i++)
                    {
                        if (PlayerAudioBlock_p[i % Control_p->pcmPlayerNumNode].Valid)
                        {
                            PlayerAudioBlock_p[i % Control_p->pcmPlayerNumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = FALSE;
                        }
                        else
                        {
                            break;
                        }
                    }
                    PlayerAudioBlock_p[(i - 1) % Control_p->pcmPlayerNumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = TRUE;
                    Control_p->pcmPlayerToProgram = (i - 1);

                    err = STFDMA_ResumeTransfer(Control_p->pcmPlayerTransferId);
                    if(err != ST_NO_ERROR)
                    {
                        Control_p->pcmPlayerCurrentState = PCMPLAYER_RESTART;
                        STTBX_Print(("PCM:ReStart failure\n"));
                        STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);

                    }
                    else
                    {
#if ENABLE_PCM_INTERRUPT
                        PCMPlayerEnableInterrupt(Control_p);
#endif
                        Control_p->EOFBlock_Received = FALSE;
                        STTBX_Print(("PCM:ReStarted\n"));
                    }
                }

                STOS_MutexLock(Control_p->LockControlStructure);
                if ((Control_p->pcmPlayerNextState == PCMPLAYER_STOPPED) )
                {
                    Control_p->pcmPlayerCurrentState = Control_p->pcmPlayerNextState;
                    //STTBX_Print(("PCM:from PCMPLAYER_RESTART\n"));
                    STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);
                    // Signal the command completion transtition
                    /*STOS_SemaphoreSignal(Control_p->pcmPlayerCmdTransitionSem_p);*/
                }
                STOS_MutexRelease(Control_p->LockControlStructure);
                break;

            case PCMPLAYER_PLAYING:
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
#if AVSYNC_ADDED
                if (PlayerAVsync_p->PTSValid)
                {

                    U32 tempPTS = 0;
                    AudSTCSync_t    * AudioSTCSync_p = &(Control_p->audioSTCSync);

                    tempPTS = time_minus(time_now(), PlayerAVsync_p->CurrentSystemTime);
                    tempPTS = TICKS_TO_PTS(tempPTS);
                    ADDToEPTS(&(PlayerAVsync_p->CurrentPTS), tempPTS);
                    PTSPCROffsetInms(Control_p, PlayerAVsync_p->CurrentPTS);

                    Player_NotifyEvt(STAUD_PTS_EVT, 0, Control_p);

                    if (AudioSTCSync_p->SkipPause != 0)
                    {
                        UpdatePCMSynced(Control_p);
                        Player_NotifyEvt(STAUD_AVSYNC_SKIP_EVT,AudioSTCSync_p->SkipPause,Control_p);
                        Player_NotifyEvt(STAUD_OUTOF_SYNC,0,Control_p);
                        if (Control_p->playBackFromLive)
                        {
                            PlayerAVsync_p->GenerateMute = TRUE;
                            PlayerAVsync_p->ValidPTSCount = 0;
                        }
                        PlayerAVsync_p->SyncCompleted = FALSE;
                        PlayerAVsync_p->InSyncCount = 0;
                    }
                    else
                    {
                        PlayerAVsync_p->ValidPTSCount++;
                    }

                    if (AudioSTCSync_p->PauseSkip != 0)
                    {
                        UpdatePCMSynced(Control_p);
                        Player_NotifyEvt(STAUD_AVSYNC_PAUSE_EVT,AudioSTCSync_p->PauseSkip,Control_p);
                        Player_NotifyEvt(STAUD_OUTOF_SYNC,0,Control_p);
                        if (Control_p->playBackFromLive)
                        {
                            PlayerAVsync_p->GenerateMute = TRUE;
                            PlayerAVsync_p->ValidPTSCount = 0;
                        }
                        PlayerAVsync_p->SyncCompleted = FALSE;
                        PlayerAVsync_p->InSyncCount = 0;
                    }
                    else
                    {
                        PlayerAVsync_p->ValidPTSCount++;
                    }

                    if ((AudioSTCSync_p->SkipPause == 0) &&
                         (AudioSTCSync_p->PauseSkip == 0) &&
                         (PlayerAVsync_p->SyncCompleted == FALSE))

                    {
                        PlayerAVsync_p->InSyncCount++;
                        //STTBX_Print(("TH:%d\n",PlayerAVsync_p->InSyncCount));
                        if(PlayerAVsync_p->InSyncCount == IN_SYNC_THRESHHOLD)
                        {
                            PlayerAVsync_p->InSyncCount = 0;
                            PlayerAVsync_p->SyncCompleted = TRUE;
                            PlayerAVsync_p->SyncCompleteTime = time_now();
                            Player_NotifyEvt(STAUD_IN_SYNC,0,Control_p);
                            //STTBX_Print(("Sync completed for Live decode\n"));
                        }
                    }

                    //if (PlayerAVsync_p->ValidPTSCount >= PTS_COUNT_FOR_UNMUTE)
                    {
                        if ((PlayerAVsync_p->GenerateMute == TRUE) &&
                             (PlayerAVsync_p->SyncCompleted == TRUE) &&
#if defined( ST_OSLINUX )
                             (((time_minus(time_now(), PlayerAVsync_p->SyncCompleteTime) * 1000)/HZ) > TIME_AFTERSYNC_FOR_UNMUTE))
#else
                             (((time_minus(time_now(), PlayerAVsync_p->SyncCompleteTime) * 1000)/time_ticks_per_sec()) > TIME_AFTERSYNC_FOR_UNMUTE))
#endif
                        {
                            //STTBX_Print(("Setting Unmute for Live decode\n"));
                            PlayerAVsync_p->GenerateMute = FALSE;
                        }
                        PlayerAVsync_p->ValidPTSCount = 0;

                    }
                    STOS_MutexLock(Control_p->LockControlStructure);
                    PlayerAVsync_p->PTSValid = FALSE;
                    STOS_MutexRelease(Control_p->LockControlStructure);
                }

#endif
#endif //STAUD_REMOVE_CLKRV_SUPPORT
                STOS_MutexLock(Control_p->LockControlStructure);
                if (Control_p->pcmPlayerNextState == PCMPLAYER_STOPPED)
                {
                    Control_p->pcmPlayerCurrentState = Control_p->pcmPlayerNextState;
                    //STTBX_Print(("PCM:from PCMPLAYER_PLAYING\n"));
                    STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);
                    // Signal the command completion transtition
                    /*STOS_SemaphoreSignal(Control_p->pcmPlayerCmdTransitionSem_p);*/
                }
                else
                {
                    if(Control_p->pcmRestart)
                    {
                        Control_p->pcmRestart = FALSE;
                        ResetPCMPlayerSyncParams(Control_p);
                        Control_p->pcmPlayerCurrentState = PCMPLAYER_RESTART;
                        STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);
#if ENABLE_AUTO_TEST_EVTS
                        Player_NotifyEvt(STAUD_TEST_PCM_PAUSE_EVT, 0, Control_p);
#endif
                        //STTBX_Print(("PCM:from PLAYING to RESTART\n"));
                    }
                }
                STOS_MutexRelease(Control_p->LockControlStructure);
                break;

            case PCMPLAYER_STOPPED:
                if ((Control_p->pcmPlayerTransferId != 0)&& (!Control_p->AbortSent))
                {
                    // Abort currrently running transfer
                    err =  STFDMA_AbortTransfer(Control_p->pcmPlayerTransferId);
                    if(err)
                    {
                        STTBX_Print(("STFDMA_AbortTransfer PCMPlayer Failure=%x\n",err));
                    }
                    else
                    {
                        Control_p->AbortSent = TRUE;
                        STTBX_Print(("Sending PCM player abort \n"));
                        Control_p->FDMAAbortSent++;
                        //STTBX_Print(("AbPCMPS\n"));
                    }

                }
                else
                {
                STOS_MutexLock(Control_p->LockControlStructure);
                if (Control_p->pcmPlayerNextState == PCMPLAYER_START ||
                    Control_p->pcmPlayerNextState == PCMPLAYER_TERMINATE)
                {
                    ResetPCMPlayerSyncParams(Control_p);
                    Control_p->pcmPlayerPlayed = 0;
                    Control_p->AbortSent = FALSE;
                    Control_p->pcmPlayerCurrentState = Control_p->pcmPlayerNextState;
                    STTBX_Print(("PCM:from PCMPLAYER_STOPPED\n"));
                    STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);
                    // Signal the command completion transtition
                    STOS_SemaphoreSignal(Control_p->pcmPlayerCmdTransitionSem_p);
                }
                STOS_MutexRelease(Control_p->LockControlStructure);
                }
                break;

            case PCMPLAYER_TERMINATE:
                // clean up all the memory allocations
                if (Control_p->pcmPlayerTransferId != 0)
                {
                    STTBX_Print((" PCM player task terminated with FDMA runing \n"));
                }
                for (i=Control_p->pcmPlayerPlayed; i < (Control_p->pcmPlayerPlayed + Control_p->pcmPlayerNumNode); i++)
                {
                    U8 Index = i % Control_p->pcmPlayerNumNode;
                    if (PlayerAudioBlock_p[Index].Valid == TRUE)
                    {
                        //STTBX_Print(("Freeing MBlock in PCMP terminate \n"));
                        if (PCMPFreeIpBlk(tempPCMPlayerControlBlock_p, &PlayerAudioBlock_p[Index]) != ST_NO_ERROR)
                        {
                            STTBX_Print(("Error in releasing Ip Pcm player mem block\n"));
                        }

                        PlayerAudioBlock_p[Index].Valid = FALSE;
                    }
                }
                if (STAud_PCMPlayerTerminate((STPCMPLAYER_Handle_t)tempPCMPlayerControlBlock_p) != ST_NO_ERROR)
                {
                    STTBX_Print(("Error in pcm player terminate\n"));
                }

                // Exit player task
                STOS_TaskExit(tempPCMPlayerControlBlock_p);
#if defined( ST_OSLINUX )
                return;
#else
                task_exit(1);
#endif
                break;

            default:
                break;
        }
    }
}


void PCMPlayerFDMACallbackFunction(U32 TransferId,U32 CallbackReason,U32 *CurrentNode_p,U32 NodeBytesRemaining,BOOL Error,void *ApplicationData_p, clock_t  InterruptTime)
{

    PCMPlayerControlBlock_t * tempPcmPlayerControlBlock_p = (PCMPlayerControlBlock_t *)ApplicationData_p;
    PCMPlayerControl_t * Control_p = &(tempPcmPlayerControlBlock_p->pcmPlayerControl);
    PlayerAudioBlock_t  * PlayerAudioBlock_p    = Control_p->PlayerAudioBlock_p;
    AudAVSync_t         * PlayerAVsync_p = &(Control_p->audioAVsync);
    U32 Played=0,ToProgram=0, Count = 0, i=0;
    ST_ErrorCode_t err = ST_NO_ERROR;
    U8 Index = 0;

    if(Error)
    {
        STTBX_Print(("PCMP:Err:%d:TrfId:%u,ActualId:%u\n",Error,TransferId,Control_p->pcmPlayerTransferId));
        STTBX_Print(("PCMP:Call State:%d,Pause:%d\n",CallbackReason,PlayerAudioBlock_p[Control_p->pcmPlayerPlayed % Control_p->pcmPlayerNumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause));
    }

    if(TransferId == Control_p->pcmPlayerTransferId && TransferId)
    {
        // Lock the Block
        STOS_MutexLock(Control_p->LockControlStructure);

        Played = Control_p->pcmPlayerPlayed;
        ToProgram = Control_p->pcmPlayerToProgram;

        if(Played%500 == 499)
        {
            STTBX_Print(("PCM:%d\n",Played));
        }

        switch(CallbackReason)
        {
            case STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED:
                /*Error Recovery*/
#if ENABLE_PCM_INTERRUPT
                PCMPlayerDisbleInterrupt(Control_p);
#endif
                while(!((U32)(PlayerAudioBlock_p[Count % Control_p->pcmPlayerNumNode].PlayerDMANodePhy_p) == ((U32)CurrentNode_p )) && (Count < Control_p->pcmPlayerNumNode))
                {
                    Count++;
                }

                Index = Played % Control_p->pcmPlayerNumNode;
                while(Index != Count)
                {
                    // Release the Block
                    if (PCMPFreeIpBlk(tempPcmPlayerControlBlock_p, &PlayerAudioBlock_p[Index]) != ST_NO_ERROR)
                    {
                        STTBX_Print(("Error in releasing Pcm player mem blcok\n"));
                    }


                    // Block is consumed and ready to be filled
                    PlayerAudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/

                    Played ++;
                    Index = Played % Control_p->pcmPlayerNumNode;
                }


                // This is to free the block on which the pause bit is set
                // Release the Block
                if (PCMPFreeIpBlk(tempPcmPlayerControlBlock_p, &PlayerAudioBlock_p[Index]) != ST_NO_ERROR)
                {
                    STTBX_Print(("Error in releasing Ip Pcm player mem block\n"));
                }


                // Block is consumed and ready to be filled
                PlayerAudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/

                Count ++;

                STTBX_Print(("PCM:Setting restart player: Played:%d\n", Played));
                // Put counters at last stop status
                Played = Count;
                ToProgram = Count;

                if(Control_p->EOFBlock_Received)
                {
                    Control_p->EOFBlock_Received = FALSE;
                    Player_NotifyEvt(STAUD_EOF_EVT, 0, Control_p);
                }

                Control_p->pcmRestart = TRUE;

                STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);

                break;
            case STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING:

                if(Error)
                {
                    while(!((U32)(PlayerAudioBlock_p[Count % Control_p->pcmPlayerNumNode].PlayerDMANodePhy_p) == ((U32)(U32)CurrentNode_p) ) && (Count < Control_p->pcmPlayerNumNode))

                    {
                        Count++;
                    }

                    Index = Played % Control_p->pcmPlayerNumNode;
                    while(Index != Count)
                    {

                        // Release the Block
                        err = PCMPFreeIpBlk(tempPcmPlayerControlBlock_p, &PlayerAudioBlock_p[Index]);
                        if (err != ST_NO_ERROR)
                        {
                            STTBX_Print(("error in freeing pcm player mem block %d\n",err));
                        }

                        // Block is consumed and ready to be filled
                        PlayerAudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/
                        Played ++;
                        Index = Played % Control_p->pcmPlayerNumNode;
                    }

                    //don't release the current block as it is playing
                }
                else
                {
                    Index = Played % Control_p->pcmPlayerNumNode;
                    // Release the Block
                    //STTBX_Print(("FreeIpBlk 0x%x\n",PlayerAudioBlock_p[Played % Control_p->pcmPlayerNumNode].memoryBlock));
                    err = PCMPFreeIpBlk(tempPcmPlayerControlBlock_p, &PlayerAudioBlock_p[Index]);
                    if (err != ST_NO_ERROR)
                    {
                        STTBX_Print(("1:Error in pcm player releasing memory block %d\n",err));
                    }

                    // Block is consumed and ready to be filled
                    PlayerAudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/
                    Played ++;
                }
#if 0
#if defined( ST_OSLINUX )
#if LINUX_PCM_INTERRUPT
                    {
                        /* Gather some info for Linux debug and show show it in /proc file system */
                        U32 Addr;
                        Addr = Control_p->BaseAddress.PCMPlayer0BaseAddr+ 0x008;
                        InterruptValue = *(U32*)Addr;
                        Addr = Control_p->BaseAddress.PCMPlayer0BaseAddr+ 0x010;
                        InterruptEnable=*(U32*)Addr;
                    }
#endif
#endif
#endif
#if AVSYNC_ADDED
                Index = Played % Control_p->pcmPlayerNumNode;

                if(PlayerAudioBlock_p[Index].memoryBlock)
                {
                    MemoryBlock_t           * MemoryBlock_p = PlayerAudioBlock_p[Index].memoryBlock;
#if MEMORY_SPLIT_SUPPORT
                    U32 Flags = PlayerAudioBlock_p[Index].MemoryParams.Flags;
#else
                    U32 Flags = MemoryBlock_p->Flags;
#endif
                    //check for frequency change
                    if ((Flags & FREQ_VALID) && (Control_p->samplingFrequency != MemoryBlock_p->Data[FREQ_OFFSET]))
                    {
                        Control_p->samplingFrequency = MemoryBlock_p->Data[FREQ_OFFSET];
                        SetPCMPlayerSamplingFrequency(Control_p);
                    }
                    if((Flags & PTS_VALID) && !Control_p->audioSTCSync.Synced &&
                        (PlayerAVsync_p->PTSValid == FALSE))
                    {
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                        PlayerAVsync_p->CurrentPTS.BaseValue        = MemoryBlock_p->Data[PTS_OFFSET];
                        PlayerAVsync_p->CurrentPTS.BaseBit32        = MemoryBlock_p->Data[PTS33_OFFSET];
#endif

#if defined( ST_OSLINUX )
                        PlayerAVsync_p->CurrentSystemTime = InterruptTime;
#else
                        PlayerAVsync_p->CurrentSystemTime = time_now();
#endif //ST_OSLINUX
#if ENABLE_SYNC_CALC_IN_CALLBACK
                        CalculatePCMPSync(Control_p);
#else
                        PlayerAVsync_p->PTSValid                        = TRUE;
                        STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);
#endif
                    }
                    else
                    {
                        if(Control_p->audioSTCSync.Synced)
                        {
                            Control_p->audioSTCSync.Synced--;
                        }
                    }
#endif
                }

                Index = (Played + 1)%Control_p->pcmPlayerNumNode;
                if((PlayerAudioBlock_p[Index].Valid == TRUE) && ((U32)PlayerAudioBlock_p[Index].PlayerDMANodePhy_p != ((U32)CurrentNode_p )))
                {
                    UpdatePCMPlayerNodeSize(&PlayerAudioBlock_p[Index], Control_p);
                }

                if((Played < ToProgram) && !Control_p->EOFBlock_Received)
                {
                    for(i = ToProgram; i < (Played + (NUM_PCM_NODES_FOR_STARTUP-1)); i++)
                    {
                        Index = (i + 1) % Control_p->pcmPlayerNumNode;
                        if(PlayerAudioBlock_p[Index].Valid == FALSE)
                        {
                            // Try to get next block
#if MEMORY_SPLIT_SUPPORT
                            if (MemPool_GetSplitIpBlk(Control_p->memoryBlockManagerHandle,
                                          (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock),
                                          (U32)tempPcmPlayerControlBlock_p,
                                          &PlayerAudioBlock_p[Index].MemoryParams) != ST_NO_ERROR)
#else
                            if (MemPool_GetIpBlk(Control_p->memoryBlockManagerHandle,
                                          (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock),
                                          (U32)tempPcmPlayerControlBlock_p) != ST_NO_ERROR)
#endif
                            {
                                // No block available
                                break;
                            }
                            else
                            {
#if MEMORY_SPLIT_SUPPORT
                                U32 Flags = PlayerAudioBlock_p[Index].MemoryParams.Flags;
#else
                                U32 Flags = PlayerAudioBlock_p[Index].memoryBlock->Flags;
#endif
                                // Got next block

                                if(Flags & DUMMYBLOCK_VALID)
                                {
                                    if(Flags & EOF_VALID)
                                    {
                                        Control_p->EOFBlock_Received = TRUE;
                                    }
                                    //Free this block
                                    if(PCMPFreeIpBlk(tempPcmPlayerControlBlock_p, &PlayerAudioBlock_p[Index])!=ST_NO_ERROR)
                                    {
                                        STTBX_Print(("1:Err in pcm player releasing memory block\n"));
                                    }
                                }
                                else
                                {
                                    FillPCMPlayerFDMANode(&PlayerAudioBlock_p[Index], Control_p);

                                    PlayerAudioBlock_p[Index].Valid = TRUE;
                                }
                            }
                        }

                        if(PlayerAudioBlock_p[Index].Valid)
                        {
                            PlayerAudioBlock_p[Index].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = TRUE;
                            PlayerAudioBlock_p[i % Control_p->pcmPlayerNumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = FALSE;

                            ToProgram++; /*Increase the no of buffer to program*/
                        }
                        else
                        {
                            break;
                            /*STTBX_Print(("No Valid buffer\n"));*/
                        }
                    }
                }
                break;

            case STFDMA_NOTIFY_TRANSFER_COMPLETE:
                STTBX_Print(("PCMP:Call State:STFDMA_NOTIFY_TRANSFER_COMPLETE\n"));
                break;

            case STFDMA_NOTIFY_TRANSFER_ABORTED:
                    STTBX_Print(("PCM Aborted\n"));
                    Control_p->FDMAAbortCBRecvd++;

                    while(Played <= ToProgram)
                    {
                        Index = Played % Control_p->pcmPlayerNumNode;
                        if (PlayerAudioBlock_p[Index].Valid)
                        {
                            // This condition should always be true in this case
                            // Release the Block
                            if(PCMPFreeIpBlk(tempPcmPlayerControlBlock_p, &PlayerAudioBlock_p[Index]) != ST_NO_ERROR)
                            {
                                STTBX_Print(("Abrt: Error in pcm player releasing memory block \n"));
                            }

                            PlayerAudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/
                        }
                        Played ++;
                    }

                    {
                        U32 j=0;
                        for (j=0;j<Control_p->pcmPlayerNumNode;j++)
                        {
                            PlayerAudioBlock_p[j].Valid = FALSE;
                        }
                    }

                    // reset the trasnsfer id to 0
                    Control_p->pcmPlayerTransferId = 0;
                    Played = 0;
                    ToProgram = 0;

                    /* Put PCM player Ip in Stop mode*/
                    /*StopPCMPlayerIP(Control_p); *//* Causing noise at PCM player while applying stop function */

                    /* This would complete the transition to STOP State*/
                    STOS_SemaphoreSignal(Control_p->pcmPlayerCmdTransitionSem_p);
                    /* To move state from stop to start/terminate in case command is given before abort is completed*/
                    STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);
                break;

            default:
                break;
        }

        Control_p->pcmPlayerPlayed = Played;
        Control_p->pcmPlayerToProgram = ToProgram;

        // Release the Lock
        STOS_MutexRelease(Control_p->LockControlStructure);
    }
    else
    {
        STTBX_Print(("PCMP:mismatch:TrfId:%u,ActualId:%u\n",TransferId,Control_p->pcmPlayerTransferId));
        STTBX_Print(("PCMP:Call State:%d\n",CallbackReason));
    }
}

ST_ErrorCode_t  PCMPlayerCheckStateTransition(PCMPlayerControl_t *Control_p,PCMPlayerState_t State)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    PCMPlayerState_t PCMPlayerCurrentState = Control_p->pcmPlayerCurrentState;

    switch (State)
    {
        case PCMPLAYER_START:
            if ((PCMPlayerCurrentState != PCMPLAYER_INIT) &&
                (PCMPlayerCurrentState != PCMPLAYER_STOPPED))
            {
                Error = STAUD_ERROR_INVALID_STATE;
            }
            break;

        case PCMPLAYER_STOPPED:
            if ((PCMPlayerCurrentState != PCMPLAYER_START)&&
                (PCMPlayerCurrentState != PCMPLAYER_RESTART)&&
                (PCMPlayerCurrentState != PCMPLAYER_PLAYING))
            {
                Error = STAUD_ERROR_INVALID_STATE;
            }
            break;

        case PCMPLAYER_TERMINATE:
            if ((PCMPlayerCurrentState != PCMPLAYER_INIT) &&
                (PCMPlayerCurrentState != PCMPLAYER_STOPPED))
            {
                Error = STAUD_ERROR_INVALID_STATE;
            }
            break;

        case PCMPLAYER_INIT:
        case PCMPLAYER_RESTART:
        case PCMPLAYER_PLAYING:
        default:
            break;
    }
    return(Error);

}

ST_ErrorCode_t      STAud_PCMPlayerSetCmd(STPCMPLAYER_Handle_t handle,PCMPlayerState_t State)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock, * lastPCMPlayerControlBlock_p = pcmPlayerControlBlock;
    task_t  * pcmPlayerTask_p = NULL;
    PCMPlayerControl_t *Control_p;
    ConsumerParams_t consumerParam;

    while (tempPCMplayerControlBlock_p != (PCMPlayerControlBlock_t *)handle)
    {
        lastPCMPlayerControlBlock_p = tempPCMplayerControlBlock_p;
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if (tempPCMplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p = &(tempPCMplayerControlBlock_p->pcmPlayerControl);

    Error = PCMPlayerCheckStateTransition(Control_p, State);
    if ( Error != ST_NO_ERROR)
    {
        return (Error);
    }

    if (State == PCMPLAYER_TERMINATE)
    {
        pcmPlayerTask_p = Control_p->pcmPlayerTask_p;
    }

    if(State == PCMPLAYER_START)
    {
        consumerParam.Handle = (U32)tempPCMplayerControlBlock_p;
        consumerParam.sem = Control_p->pcmPlayerTaskSem_p;
    switch(Control_p->pcmPlayerIdentifier)
    {
#ifdef HDMIPCMP0_SUPPORTED
    case HDMI_PCM_PLAYER_0:
        if((Control_p->AnalogMode == PCM_ON) && (Control_p->HDMI_PcmPlayerEnable == TRUE))
        {
            if(Control_p->memoryBlockManagerHandle)
            {
                Error = MemPool_RegConsumer(Control_p->memoryBlockManagerHandle, consumerParam);
            }
        }
        break;
#endif
    default:
        if(Control_p->AnalogMode == PCM_ON)
        {
            if(Control_p->memoryBlockManagerHandle)
            {
                Error = MemPool_RegConsumer(Control_p->memoryBlockManagerHandle, consumerParam);
            }
        }
        break;
    }
    }
    /*change the next state*/
    STOS_MutexLock(Control_p->LockControlStructure);

#ifdef ST_OS21
#ifdef STTBX_PRINT
    {
        U32 Count __attribute__ ((unused)) = semaphore_value(Control_p->pcmPlayerCmdTransitionSem_p);

        STTBX_Print(("PCMMP:CMD:Semcount %d\n",Count));
    }
#endif
#endif

    Control_p->pcmPlayerNextState = State;
    STOS_MutexRelease(Control_p->LockControlStructure);

    /*signal the task to change its state*/
    STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);

    if (State == PCMPLAYER_TERMINATE)
    {
        STOS_TaskWait(&pcmPlayerTask_p, TIMEOUT_INFINITY);
#if defined (ST_OS20)
        STOS_TaskDelete(pcmPlayerTask_p,Control_p->CPUPartition,(void *)Control_p->pcmPlayerTask_pstack,Control_p->CPUPartition);
#else
        STOS_TaskDelete(pcmPlayerTask_p,NULL,NULL,NULL);
#endif
    }
    else
    {
        // Wait for command completion transtition
        STOS_SemaphoreWait(Control_p->pcmPlayerCmdTransitionSem_p);
    }

    if(State == PCMPLAYER_STOPPED)
    {
        /*Unregister from the BM*/
        if(Control_p->memoryBlockManagerHandle)
        {
            Error |= MemPool_UnRegConsumer(Control_p->memoryBlockManagerHandle, (U32)tempPCMplayerControlBlock_p);
        }
#if ENABLE_AUTO_TEST_EVTS
        Player_NotifyEvt(STAUD_TEST_PCM_STOP_EVT, 0, Control_p);
#endif
    }

    return Error;
}

ST_ErrorCode_t      STAud_PCMPlayerPlaybackFromLive(STPCMPLAYER_Handle_t handle,BOOL Playbackfromlive)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock, * lastPCMPlayerControlBlock_p = pcmPlayerControlBlock;
    // task_t  * pcmPlayerTask_p;

    while (tempPCMplayerControlBlock_p != (PCMPlayerControlBlock_t *)handle)
    {
        lastPCMPlayerControlBlock_p = tempPCMplayerControlBlock_p;
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if (tempPCMplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    STOS_MutexLock(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);
    tempPCMplayerControlBlock_p->pcmPlayerControl.playBackFromLive = Playbackfromlive;
    STOS_MutexRelease(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);
    //STTBX_Print(("Setting Live decode\n"));

    return Error;

}

void    UpdatePCMPlayerNodeSize(PlayerAudioBlock_t * PlayerAudioBlock_p, PCMPlayerControl_t * Control_p)
{
#if AVSYNC_ADDED
    STFDMA_GenericNode_t    * PlayerDmaNode_p = PlayerAudioBlock_p->PlayerDmaNode_p;
#if MEMORY_SPLIT_SUPPORT
        U32 MemorySize  = PlayerAudioBlock_p->MemoryParams.Size;
#else
        U32 MemorySize  = PlayerAudioBlock_p->memoryBlock->Size;
#endif

    if (Control_p->skipDuration && (Control_p->pauseDuration == 0))
    {
        U32 SamplesToSkip           = ((Control_p->skipDuration * Control_p->samplingFrequency)/1000);
        U32 SampleSize                 = (Control_p->NumChannels * BYTES_PER_CHANNEL_IN_PLAYER);

        U32 SamplesAvailableToSkip  = (MemorySize / SampleSize)- 192;
        U32 SamplesToGo = 0;

        if(SamplesToSkip > SamplesAvailableToSkip)
        {
            Control_p->skipDuration -= ((SamplesAvailableToSkip * 1000) / Control_p->samplingFrequency);
            SamplesToGo = 192;
        }
        else
        {
            Control_p->skipDuration = 0;
            SamplesToSkip                   = ((SamplesToSkip / 192) * 192);
            SamplesToGo                     = SamplesAvailableToSkip - SamplesToSkip + 192; /*As we had already reduced 192 samples*/
        }

        PlayerDmaNode_p->Node.SourceAddress_p   = (void *)((U32)Control_p->DummyBuffer.BasePhyAddress);
        PlayerDmaNode_p->Node.NumberBytes       = SamplesToGo * SampleSize;
        PlayerDmaNode_p->Node.Length                = PlayerDmaNode_p->Node.NumberBytes;
    }

    if (Control_p->pauseDuration && (Control_p->skipDuration == 0))
    {
        U32 SamplesToPause             = ((Control_p->pauseDuration * Control_p->samplingFrequency)/1000);
        U32 SampleSize                      = (Control_p->NumChannels * BYTES_PER_CHANNEL_IN_PLAYER);

        U32 FrameSizeInSamples = (MemorySize / SampleSize);
        U32 DummyBufferSampleSize = (Control_p->dummyBufferSize / SampleSize);
        U32 SamplesForPause = DummyBufferSampleSize - FrameSizeInSamples;
        U32 SamplesToGo = 0;

        if(SamplesToPause  > SamplesForPause)
        {
            Control_p->pauseDuration -= ((SamplesForPause * 1000) / Control_p->samplingFrequency);
            SamplesToGo =  DummyBufferSampleSize;
        }
        else
        {
            Control_p->pauseDuration = 0;
            SamplesToPause                  = ((SamplesToPause / 192) * 192);
            SamplesToGo                     = FrameSizeInSamples + SamplesToPause;
        }
        PlayerDmaNode_p->Node.SourceAddress_p       = (void *)(U32)Control_p->DummyBuffer.BasePhyAddress;
        // Size of original data + paused samples
        PlayerDmaNode_p->Node.NumberBytes       = SamplesToGo * SampleSize;
        PlayerDmaNode_p->Node.Length                = PlayerDmaNode_p->Node.NumberBytes;
    }
#endif
}

void    FillPCMPlayerFDMANode(PlayerAudioBlock_t * PlayerAudioBlock_p, PCMPlayerControl_t * Control_p)
{
    STFDMA_GenericNode_t    * PlayerDmaNode_p = PlayerAudioBlock_p->PlayerDmaNode_p;
#if MEMORY_SPLIT_SUPPORT
    U32 MemoryStart = PlayerAudioBlock_p->MemoryParams.MemoryStart;
    U32 MemorySize = PlayerAudioBlock_p->MemoryParams.Size;
    U32 Flags = PlayerAudioBlock_p->MemoryParams.Flags;
#else
    U32 MemoryStart = PlayerAudioBlock_p->memoryBlock->MemoryStart;
    U32 MemorySize = PlayerAudioBlock_p->memoryBlock->Size;
    U32 Flags = PlayerAudioBlock_p->memoryBlock->Flags;
#endif

    PlayerDmaNode_p->Node.SourceAddress_p  = (void *)STOS_AddressTranslate(Control_p->PlayerInputMemoryAddress.BasePhyAddress,Control_p->PlayerInputMemoryAddress.BaseUnCachedVirtualAddress,MemoryStart);
    //update the frame size
    Control_p->CurrentFrameSize = MemorySize;

    PlayerDmaNode_p->Node.Length           = MemorySize;
    PlayerDmaNode_p->Node.NumberBytes  = MemorySize;
    PlayerDmaNode_p->Node.SourceStride = 0;

    // Generate Mute for all the buffers in case we are out of sync
    if (Control_p->pcmMute || Control_p->audioAVsync.GenerateMute || (Flags & DUMMYBLOCK_VALID))
    {
        PlayerDmaNode_p->Node.SourceAddress_p = (void *)((U32)Control_p->DummyBuffer.BasePhyAddress);
    }

}

ST_ErrorCode_t  SetPCMPlayerSamplingFrequency(PCMPlayerControl_t * Control_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 SamplingFrequency = Control_p->samplingFrequency;

    switch (Control_p->pcmPlayerIdentifier)
    {

#ifdef PCMP0_SUPPORTED
        case PCM_PLAYER_0:
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
            Error=STCLKRV_SetNominalFreq( Control_p->CLKRV_Handle, STCLKRV_CLOCK_PCM_0, (Control_p->pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier * SamplingFrequency));
            STTBX_Print(("PCM_PLAYER_0 samplingFrequency %d\n",SamplingFrequency));
#else
            Error = PCMPlayer_SetFrequencySynthesizer(Control_p , SamplingFrequency);
#endif
            break;
#endif

#ifdef PCMP1_SUPPORTED
        case PCM_PLAYER_1:
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
            Error=STCLKRV_SetNominalFreq( Control_p->CLKRV_Handle, STCLKRV_CLOCK_PCM_1, (Control_p->pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier * SamplingFrequency));
            STTBX_Print(("PCM_PLAYER_1 samplingFrequency %d\n",SamplingFrequency));
#else
            Error = PCMPlayer_SetFrequencySynthesizer(Control_p , SamplingFrequency);
#endif
            break;
#endif

#ifdef PCMP2_SUPPORTED
        case PCM_PLAYER_2:
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
            Error=STCLKRV_SetNominalFreq( Control_p->CLKRV_Handle, STCLKRV_CLOCK_PCM_2, (Control_p->pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier * SamplingFrequency));
            STTBX_Print(("PCM_PLAYER_2 samplingFrequency %d\n",SamplingFrequency));
#else
            Error = PCMPlayer_SetFrequencySynthesizer(Control_p , SamplingFrequency);
#endif
            break;
#endif

#ifdef PCMP3_SUPPORTED
        case PCM_PLAYER_3:
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
            {
            U32 FrequencyMultiplier = 0;//Required for channel 3/4 modulator FS prgrammation.(7200 cut 1.0)
            /*Required for channel 3/4 modulator FS prgrammation.(7200 cut 1.0) for channel 3/4 modulator clock*/
            FrequencyMultiplier = Control_p->pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier;
            if(Control_p->pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier == 256)
            {
                FrequencyMultiplier = (Control_p->pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier) /4;
            }
            else if(Control_p->pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier == 128)
            {
                FrequencyMultiplier = (Control_p->pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier) /2;
            }
            Error=STCLKRV_SetNominalFreq( Control_p->CLKRV_Handle, STCLKRV_CLOCK_PCM_3, (FrequencyMultiplier * SamplingFrequency));
            STTBX_Print(("PCM_PLAYER_3 samplingFrequency %d\n",SamplingFrequency));
            }
#else
            Error = PCMPlayer_SetFrequencySynthesizer(Control_p , SamplingFrequency);
#endif
            break;
#endif

#ifdef HDMIPCMP0_SUPPORTED
        case HDMI_PCM_PLAYER_0:
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
            Error=STCLKRV_SetNominalFreq( Control_p->CLKRV_Handle, STCLKRV_CLOCK_SPDIF_HDMI_0, (Control_p->pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier * SamplingFrequency));
            STTBX_Print(("HDMI_PCM_PLAYER_0 samplingFrequency %d\n",SamplingFrequency));
#else
            Error = PCMPlayer_SetFrequencySynthesizer(Control_p , SamplingFrequency);
#endif
            break;
#endif

        case MAX_PCM_PLAYER_INSTANCE:
        default:
            //STTBX_Print(("Unidentified PCM player identifier\n"));
            Error = ST_ERROR_UNKNOWN_DEVICE;
            break;
    }

    return (Error);
}

void PCMPlayer_EventHandler(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
    PCMPlayerControl_t * Control_p = (PCMPlayerControl_t *)SubscriberData_p;

    if (Reason == CALL_REASON_NOTIFY_CALL)
    {
        STOS_MutexLock(Control_p->LockControlStructure);

        switch(Event)
        {
            case STAUD_AVSYNC_SKIP_EVT:
                //STTBX_Print(("SKIP_EVT:Received:Skip = %d\n",*((U32 *)EventData)));
                if ((Control_p->skipDuration == 0) || (*((U32 *)EventData) == 0))
                {
                    Control_p->skipDuration = *((U32 *)EventData);
                }

                break;
            case STAUD_AVSYNC_PAUSE_EVT:
                //STTBX_Print(("PAUSE_EVT:Received:Pause = %d\n",*((U32 *)EventData)));
                if ((Control_p->pauseDuration == 0) || (*((U32 *)EventData) == 0))
                {
                    Control_p->pauseDuration = *((U32 *)EventData);
                }
                break;
            default :
                break;
        }

        STOS_MutexRelease(Control_p->LockControlStructure);
    }

}


ST_ErrorCode_t PCMPlayer_SubscribeEvents(PCMPlayerControl_t *Control_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STEVT_DeviceSubscribeParams_t       EVT_SubcribeParams;
    EVT_SubcribeParams.NotifyCallback   = PCMPlayer_EventHandler;
    EVT_SubcribeParams.SubscriberData_p = (void *)Control_p;

    if (Control_p->skipEventID != 0)
        ErrorCode |= STEVT_SubscribeDeviceEvent(Control_p->EvtHandle, Control_p->Name, Control_p->skipEventID, &EVT_SubcribeParams);

    if (Control_p->pauseEventID != 0)
        ErrorCode |= STEVT_SubscribeDeviceEvent(Control_p->EvtHandle, Control_p->Name, Control_p->pauseEventID, &EVT_SubcribeParams);

    /*ErrorCode |= STEVT_SubscribeDeviceEvent(Control_p->EvtHandle, (char*)"PLAYER0", STAUD_AVSYNC_PAUSE_EVT, &EVT_SubcribeParams);*/

    return (ErrorCode);

}

ST_ErrorCode_t PCMPlayer_UnSubscribeEvents(PCMPlayerControl_t *Control_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (Control_p->skipEventID != 0)
        ErrorCode |= STEVT_UnsubscribeDeviceEvent(Control_p->EvtHandle, Control_p->Name, Control_p->skipEventID);

    if (Control_p->pauseEventID != 0)
        ErrorCode |= STEVT_UnsubscribeDeviceEvent(Control_p->EvtHandle, Control_p->Name, Control_p->pauseEventID);

    return (ErrorCode);
}

/* Register player control block as producer for sending AVSync events to all releated Modules  */
ST_ErrorCode_t PCMPlayer_RegisterEvents(PCMPlayerControl_t *Control_p)
{
    AudAVSync_t * PlayerAVsync_p = &(Control_p->audioAVsync);
    STEVT_Handle_t      EvtHandle = Control_p->EvtHandle;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    /* Register to the events */
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_AVSYNC_SKIP_EVT, &PlayerAVsync_p->EventIDSkip);
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_AVSYNC_PAUSE_EVT, &PlayerAVsync_p->EventIDPause);
    /*Above are Internal Events*/
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_IN_SYNC, &PlayerAVsync_p->EventIDInSync);
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_OUTOF_SYNC, &PlayerAVsync_p->EventIDOutOfSync);
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_PTS_EVT, &PlayerAVsync_p->EventIDPTS);
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_EOF_EVT, &Control_p->EventIDEOF);

#if ENABLE_AUTO_TEST_EVTS
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_TEST_NODE_EVT, &PlayerAVsync_p->EventIDTestNode);
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_TEST_PCM_START_EVT, &PlayerAVsync_p->EventIDTestPCMStart);
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_TEST_PCM_RESTART_EVT, &PlayerAVsync_p->EventIDTestPCMReStart);
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_TEST_PCM_STOP_EVT, &PlayerAVsync_p->EventIDTestPCMStop);
    ErrorCode |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_TEST_PCM_PAUSE_EVT, &PlayerAVsync_p->EventIDTestPCMPause);
#endif

    return ErrorCode;
}


/* Un register a/v sync events */
ST_ErrorCode_t PCMPlayer_UnRegisterEvents(PCMPlayerControl_t *Control_p)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t  EvtHandle = Control_p->EvtHandle;

    /* UnRegister to the events */
    Error = STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_AVSYNC_SKIP_EVT);
    Error |= STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_AVSYNC_PAUSE_EVT);
    Error |= STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_IN_SYNC);
    Error |= STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_OUTOF_SYNC);
    Error |= STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_PTS_EVT);
    Error |= STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_EOF_EVT);

#if ENABLE_AUTO_TEST_EVTS
    Error |= STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_TEST_NODE_EVT);
    Error |= STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_TEST_PCM_START_EVT);
    Error |= STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_TEST_PCM_RESTART_EVT);
    Error |= STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_TEST_PCM_STOP_EVT);
    Error |= STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_TEST_PCM_PAUSE_EVT);
#endif

    return Error;
}


/* Notify to the Subscribed Modules */
ST_ErrorCode_t Player_NotifyEvt(U32 EvtType,U32 Value,PCMPlayerControl_t *Control_p )
{
    AudAVSync_t * PlayerAVsync_p = &(Control_p->audioAVsync);
    STEVT_Handle_t EvtHandle = Control_p->EvtHandle;
    STAUD_Object_t ObjectID = Control_p->ObjectID;
    ST_ErrorCode_t  Error = ST_NO_ERROR;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STAUD_PTS_t         PTS;
#endif

    switch(EvtType)
    {
        case STAUD_AVSYNC_SKIP_EVT:
            Error=STEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDSkip, &Value);
            //STTBX_Print(("SKIP_EVT:Notify:=%d\n",Value));
            break;

        case STAUD_AVSYNC_PAUSE_EVT:
            Error=STEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDPause, &Value);
            //STTBX_Print(("PAUSE_EVT:Notify:=%d\n",Value));
            break;

        case STAUD_IN_SYNC:
            Error=STAudEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDInSync, &Value, sizeof(Value), ObjectID);
            //STTBX_Print(("INSYNC_EVT:\n"));
            break;

        case STAUD_OUTOF_SYNC:
            Error=STAudEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDOutOfSync, &Value, sizeof(Value), ObjectID);
            //STTBX_Print(("OUTOFSYNC_EVT:\n"));
            break;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
        case STAUD_PTS_EVT:
            PTS.Low = PlayerAVsync_p->CurrentPTS.BaseValue;
            PTS.High = PlayerAVsync_p->CurrentPTS.BaseBit32;
            PTS.Interpolated = TRUE;
            Error=STAudEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDPTS, &PTS, sizeof(PTS), ObjectID);
            //STTBX_Print(("PTS_EVT:%d\n",Value));
            break;
#endif

        case STAUD_EOF_EVT:
            Error=STAudEVT_Notify(EvtHandle, Control_p->EventIDEOF, &Value, sizeof(Value), ObjectID);
            break;

#if ENABLE_AUTO_TEST_EVTS
        case STAUD_TEST_NODE_EVT:
            Error=STEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDTestNode, &Value);
            //STTBX_Print(("TEST_NODE_EVT: \n"));
            break;

        case STAUD_TEST_PCM_START_EVT:
            Error=STEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDTestPCMStart, &Value);
            //STTBX_Print(("TEST_PCM_START: \n"));
            break;

        case STAUD_TEST_PCM_RESTART_EVT:
            Error=STEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDTestPCMReStart, &Value);
            //STTBX_Print(("TEST_PCM_RESTART: \n"));
            break;

        case STAUD_TEST_PCM_STOP_EVT:
            Error=STEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDTestPCMStop, &Value);
            //STTBX_Print(("TEST_PCM_START: \n"));
            break;

        case STAUD_TEST_PCM_PAUSE_EVT:
            Error=STEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDTestPCMPause, &Value);
            //STTBX_Print(("TEST_PCM_START: \n"));
            break;
#endif

        default :
            break;
    }

    return Error;
}

ST_ErrorCode_t STAud_PCMPlayerSetOffset(STPCMPLAYER_Handle_t PlayerHandle,S32 Offset)
{
   PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempPCMplayerControlBlock_p != NULL)
    {
        if (tempPCMplayerControlBlock_p == (PCMPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if(tempPCMplayerControlBlock_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if(tempPCMplayerControlBlock_p->pcmPlayerControl.audioSTCSync.Offset != Offset)
    {
        //STTBX_Print(("Setting Offset:%d\n", Offset));
        STOS_MutexLock(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);
        tempPCMplayerControlBlock_p->pcmPlayerControl.audioSTCSync.Offset = Offset;
        STOS_MutexRelease(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);
    }
    return Error;
}

ST_ErrorCode_t STAud_PCMPlayerGetOffset(STPCMPLAYER_Handle_t PlayerHandle,S32 *Offset_p)
{
   PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempPCMplayerControlBlock_p != NULL)
    {
        if (tempPCMplayerControlBlock_p == (PCMPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }
    if(tempPCMplayerControlBlock_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    STOS_MutexLock(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);
    *Offset_p = tempPCMplayerControlBlock_p->pcmPlayerControl.audioSTCSync.Offset;
    STOS_MutexRelease(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);

    return Error;
}

ST_ErrorCode_t STAud_PCMPlayerSetLatency(STPCMPLAYER_Handle_t PlayerHandle,U32 Latency)
{
   PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempPCMplayerControlBlock_p != NULL)
    {
        if (tempPCMplayerControlBlock_p == (PCMPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if(tempPCMplayerControlBlock_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if(tempPCMplayerControlBlock_p->pcmPlayerControl.audioSTCSync.UsrLatency != Latency)
    {
        //STTBX_Print(("Setting Offset:%d\n", Offset));
        STOS_MutexLock(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);
        tempPCMplayerControlBlock_p->pcmPlayerControl.audioSTCSync.UsrLatency = Latency;
        STOS_MutexRelease(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);
    }
    return Error;
}

ST_ErrorCode_t STAud_PCMPlayerAVSyncCmd(STPCMPLAYER_Handle_t PlayerHandle,BOOL Command)
{
   PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempPCMplayerControlBlock_p != NULL)
    {
        if (tempPCMplayerControlBlock_p == (PCMPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if(tempPCMplayerControlBlock_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    tempPCMplayerControlBlock_p->pcmPlayerControl.AVSyncEnable = Command;

    return Error;
}

ST_ErrorCode_t      STAud_PCMPlayerSetAnalogMode(STPCMPLAYER_Handle_t handle, STAUD_PCMMode_t AnalogMode)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    PCMPlayerControlBlock_t * temppcmplayerControlBlock = pcmPlayerControlBlock;
    PCMPlayerControl_t      * pcmPlayerControl_p;

    while (temppcmplayerControlBlock != (PCMPlayerControlBlock_t *)handle)
    {
        temppcmplayerControlBlock = temppcmplayerControlBlock->next;
    }

    if (temppcmplayerControlBlock == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    pcmPlayerControl_p      = &(temppcmplayerControlBlock->pcmPlayerControl);

    STOS_MutexLock(pcmPlayerControl_p->LockControlStructure);
    if (pcmPlayerControl_p->pcmPlayerCurrentState == PCMPLAYER_INIT ||
        pcmPlayerControl_p->pcmPlayerCurrentState == PCMPLAYER_STOPPED)
    {
        //STTBX_Print(("Setting pcm to %d\n",(U32)AnalogMode));
        pcmPlayerControl_p->AnalogMode = AnalogMode;
    }
    else
    {
        //STTBX_Print(("Invalid state for changing pcm mode\n"));
        Error = STAUD_ERROR_INVALID_STATE;
    }
    STOS_MutexRelease(pcmPlayerControl_p->LockControlStructure);

    return (Error);
}

ST_ErrorCode_t      STAud_PCMPlayerGetAnalogMode(STPCMPLAYER_Handle_t handle, STAUD_PCMMode_t   * AnalogMode_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    PCMPlayerControlBlock_t * temppcmplayerControlBlock = pcmPlayerControlBlock;

    while (temppcmplayerControlBlock != (PCMPlayerControlBlock_t *)handle)
    {
        temppcmplayerControlBlock = temppcmplayerControlBlock->next;
    }

    if (temppcmplayerControlBlock == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    STOS_MutexLock(temppcmplayerControlBlock->pcmPlayerControl.LockControlStructure);
    *AnalogMode_p = temppcmplayerControlBlock->pcmPlayerControl.AnalogMode;
    STOS_MutexRelease(temppcmplayerControlBlock->pcmPlayerControl.LockControlStructure);

    return (Error);
}

#if defined( ST_OSLINUX )
void log_skip_values(PCMPlayerControl_t *Control_p,STCLKRV_ExtendedSTC_t CurrentPTS)
{
#if LOG_AVSYNC_LINUX
        if(Log_To_Array)
        {
            Aud_Array[Aud_Count][0]=Control_p->audioSTCSync.NewPCR;
            Aud_Array[Aud_Count][1]=Control_p->audioAVsync.CurrentPTS;
            Aud_Array[Aud_Count][2]=CurrentPTS.BaseValue;

            Aud_Count++;
            if(Aud_Count >= NUM_OF_ELEMENTS)
            {
                //Log_To_Array = 0;
                STTBX_Print(("PCM Skip Logged \n"));
                {
                    int i,j;
                    for(i=0;i<Aud_Count;i++)
                    {
                        STTBX_Print(("%u %8u %8u \n", Aud_Array[i][0],Aud_Array[i][1],Aud_Array[i][2]));
                    }
                    Aud_Count = 0;
                }
            }
        }
#endif
}
#endif

#if defined( ST_OSLINUX )
void log_pause_values(PCMPlayerControl_t *Control_p,STCLKRV_ExtendedSTC_t CurrentPTS)
{
#if LOG_AVSYNC_LINUX
        if(Log_To_Array)
        {
            Aud_Pause_Array[Aud_Pause_Count][0]=Control_p->audioSTCSync.NewPCR;
            Aud_Pause_Array[Aud_Pause_Count][1]=Control_p->audioAVsync.CurrentPTS;
            Aud_Pause_Array[Aud_Pause_Count][2]=CurrentPTS.BaseValue;

            Aud_Pause_Count++;
            if(Aud_Pause_Count >= NUM_OF_ELEMENTS)
            {
                //Log_To_Array = 0;
                STTBX_Print(("PCM Pause Logged\n"));
                {
                    int i,j;
                    for(i=0;i<Aud_Pause_Count;i++)
                    {
                        STTBX_Print(("%u %8u %8u \n", Aud_Pause_Array[i][0],Aud_Pause_Array[i][1],Aud_Pause_Array[i][2]));
                    }
                    Aud_Pause_Count = 0;
                }
            }
        }
#endif
}
#endif
/* ------------------------------------------------------------------------------
 * function :       PTSPCROffsetInms
 * parameters:      U32 PTS
 *
 * returned value:  ST_ErrorCode_t
 *
   -----------------------------------------------------------------------------*/

#ifndef STAUD_REMOVE_CLKRV_SUPPORT

ST_ErrorCode_t PTSPCROffsetInms(PCMPlayerControl_t *Control_p,STCLKRV_ExtendedSTC_t CurrentInputPTS)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    BOOL DoSync = FALSE;


#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
    U32 PauseSkip = 0, SkipPause = 0;
    STCLKRV_ExtendedSTC_t CurrentPTS = CurrentInputPTS;
#endif

    if(Control_p->AVSyncEnable == TRUE)
    {
        U32 PTSSTCDiff = 0;
        AudSTCSync_t * AudioSTCSync_p = &(Control_p->audioSTCSync);

        AudioSTCSync_p->SkipPause = 0;
        AudioSTCSync_p->PauseSkip = 0;

#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
        if(AudioSTCSync_p->SyncState == SYNCHRO_STATE_FIRST_PTS)
        {
            AudioSTCSync_p->LastPTS = CurrentPTS;

            Error = STCLKRV_GetExtendedSTC(Control_p->CLKRV_HandleForSynchro, &AudioSTCSync_p->LastPCR);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("PCMPLAYER(%s):No STC %d\n", Control_p->Name,(U32)CurrentPTS.BaseValue));
                return Error;
            }
            //STTBX_Print(("SYNCHRO_STATE_NORMAL: PCR=%u,%u\n",AudioSTCSync_p->LastPCR.BaseBit32,AudioSTCSync_p->LastPCR.BaseValue));
            AudioSTCSync_p->SyncState = SYNCHRO_STATE_NORMAL;

#if 0//LOG_AVSYNC
        Aud_Array1[0][Aud_Count1]=AudioSTCSync_p->LastPCR.BaseValue;
        Aud_Array1[1][Aud_Count1]=CurrentPTS.BaseValue;
        Aud_Count1++;
        if(Aud_Count1>400)
        {
            FILE *fp;
            U32 i;
            fp= fopen("d:\\stc_drift1.csv","w+");
            if(fp!=NULL)
            {
                for (i=0;i<Aud_Count1;i++)
                        fprintf(fp,"%u,%u \n",Aud_Array1[0][i],Aud_Array1[1][i]);
                fclose(fp);
            }
        }
#endif
        //STTBX_Print(("1 - Error = %d\n", (U32)Error));
        return Error;
    }

/*
        Error = STCLKRV_GetClocksFrequency(Control_p->CLKRV_HandleForSynchro,
                                                                   &STCFreq,
                                                                   &PCMFreq,
                                                                   &HDFreq);
        if ( Error != ST_NO_ERROR)
        {
            STTBX_Print(("No valid clock freq.%d\n", (U32)Error));
            AudioSTCSync_p->BadClockCount++;
            return Error;
        }
*/
        Error = STCLKRV_GetExtendedSTC(Control_p->CLKRV_HandleForSynchro, &AudioSTCSync_p->NewPCR);
        if ( Error != ST_NO_ERROR)
        {
            STTBX_Print(("No STC In Normal state%x\n", (U32)Error));
            AudioSTCSync_p->BadClockCount++;
            return Error;
        }

        ADDToEPTS(&CurrentPTS, MILLISECOND_TO_PTS(AudioSTCSync_p->Offset + AudioSTCSync_p->UsrLatency));
        AudioSTCSync_p->BadClockCount = 0;

#if 1
/*      if((dump_log%10) == 0)
        {
            STTBX_Print(("%u %8u %8u \n", AudioSTCSync_p->NewPCR.BaseValue,Control_p->audioAVsync.CurrentPTS.BaseValue, Control_p->audioAVsync.CurrentSystemTime));
            STTBX_Print(("%8u \n", Control_p->audioAVsync.CurrentSystemTime));/FDMA callback time
            STTBX_Print(("%u,  %u, %u  %u \n",STCFreq, PCMFreq, HDFreq,(U32)time_now()));
        }
        dump_log++;*/
#endif

#if LOG_AVSYNC
        Aud_Array[0][Aud_Count] = AudioSTCSync_p->NewPCR.BaseValue;
        Aud_Array[1][Aud_Count] = CurrentPTS.BaseValue;
        Aud_Count++;
        if(Aud_Count>100)
        {
            FILE *fp;
            U32 i;
            fp= fopen("d:\\stc_drift.csv","w+");
            if(fp!=NULL)
            {
                for (i = 0; i < Aud_Count; i++)
                        fprintf(fp,"%u,%u \n",Aud_Array [0][i],Aud_Array [1][i]);
                fclose(fp);
            }
        }
#endif

        if (EPTS_LATER_THAN(AudioSTCSync_p->LastPCR, AudioSTCSync_p->NewPCR) || EPTS_LATER_THAN(AudioSTCSync_p->LastPTS, CurrentPTS))
        {
            /* just skip the audioSTCSync when the PCR or PTS values wrap around 0 */
            AudioSTCSync_p->LastPCR = AudioSTCSync_p->NewPCR;
            AudioSTCSync_p->LastPTS = CurrentPTS;
            return Error;
        }

        AudioSTCSync_p->LastPCR = AudioSTCSync_p->NewPCR;
        AudioSTCSync_p->LastPTS = CurrentPTS;
        STTBX_Print(("PCMPLAYER(%s):STC,%u,%u, PTS,%u,%u \n", Control_p->Name,AudioSTCSync_p->NewPCR.BaseBit32,AudioSTCSync_p->NewPCR.BaseValue,CurrentPTS.BaseBit32,CurrentPTS.BaseValue));
        /* -------------------------------- */
        /* Compare PTS against decoder time */
        /* -------------------------------- */

        /* If time difference between pts and pcr is too big, we don't apply correction
        * This handles the typical case of a packet injector that will wrap around
        * the time references at the end of his stream. */

        DoSync=FALSE;
        if (EPTS_LATER_THAN(CurrentPTS, Control_p->audioSTCSync.NewPCR))
        {
            PTSSTCDiff = EPTS_DIFF(CurrentPTS, AudioSTCSync_p->NewPCR);
            if (PTSSTCDiff<= HUGE_TIME_DIFFERENCE_PAUSE) DoSync=TRUE;
        }
        else
        {
            PTSSTCDiff = EPTS_DIFF(CurrentPTS, AudioSTCSync_p->NewPCR);
            if (PTSSTCDiff<= HUGE_TIME_DIFFERENCE_SKIP) DoSync=TRUE;
        }

        if ( DoSync == TRUE/*PTSSTCDiff <= HUGE_TIME_DIFFERENCE*/)
        {
            AudioSTCSync_p->HugeCount = 0;
            SkipPause = 0;
            PauseSkip = 0;
            /* ----------------------- */
            /* Audio decoding is ahead */
            /* ----------------------- */

            if (EPTS_LATER_THAN(CurrentPTS, AudioSTCSync_p->NewPCR))
            {
                if (PTSSTCDiff > CORRECTIVE_ACTION_AUDIO_AHEAD_THRESHOLD)
                {
                    /* Calculate how much data has to be added (in bytes) */
                    PauseSkip  = PTSSTCDiff;

                    /* Calculate how long the decoder should be stopped */
                    PauseSkip  = PTS_TO_MILLISECOND(PauseSkip);
                    PauseSkip   = CEIL_PTSSTC_DRIFT(PauseSkip);
                    AudioSTCSync_p->PauseSkip = PauseSkip;
                    AudioSTCSync_p->SkipPause = 0;

                    STTBX_Print(("PCMPLAYER(%s) => Pause (ms)=%d\n",Control_p->Name,PauseSkip));

#if defined( ST_OSLINUX )
                    Control_p->Pause_Performed++;
                    log_pause_values(Control_p,CurrentPTS);
#endif
                }
            }
            /* ----------------------- */
            /* Audio decoding is late  */
            /* ----------------------- */
            else
            {
                if (PTSSTCDiff > CORRECTIVE_ACTION_AUDIO_BEHIND_THRESHOLD)
                {
                    /* Calculate how much data has to be dropped */
                    SkipPause  = PTSSTCDiff;
                    SkipPause  = PTS_TO_MILLISECOND(SkipPause);
                    SkipPause  = CEIL_PTSSTC_DRIFT(SkipPause);
                    AudioSTCSync_p->SkipPause = SkipPause;
                    AudioSTCSync_p->PauseSkip = 0;
                    //STTBX_Print(("%u  %10u %10u %10u \n", AudioSTCSync_p->NewPCR.BaseValue , Control_p->audioAVsync.CurrentPTS.BaseValue,CurrentPTS.BaseValue, SkipPause));

                    STTBX_Print(("PCMPLAYER(%s) => Skip (ms)=%d\n",Control_p->Name,SkipPause));

#if defined( ST_OSLINUX )
                    log_skip_values(Control_p,CurrentPTS);
                    Control_p->Skip_Performed++;
#endif
                }
            }
        }
        else
        {
            /* The difference between the values is too big, let's go back to*/
            //STTBX_Print(("Chnage to SYNCHRO_STATE_FIRST_PTS\n"));

            STTBX_Print(("PCMPLAYER(%s) => Reset synchro\n",Control_p->Name));

            AudioSTCSync_p->SyncState = SYNCHRO_STATE_FIRST_PTS;
            AudioSTCSync_p->HugeCount++;
#if defined( ST_OSLINUX )
            Control_p->Huge_Diff++;
#endif
        }
#endif
    }
    //Hack- to test PTS values without clkrv
    //Control_p->audioSTCSync.SkipPause = 0;
    //Control_p->audioSTCSync.PauseSkip = 0;
    return Error;
}

#endif

void    ResetPCMPlayerSyncParams(PCMPlayerControl_t * Control_p)
{
    AudSTCSync_t * AudioSTCSync_p = &(Control_p->audioSTCSync);
    AudAVSync_t  * PlayerAVsync_p = &(Control_p->audioAVsync);

    AudioSTCSync_p->SyncState            = SYNCHRO_STATE_FIRST_PTS;  /*AVSync state of Parser*/
    AudioSTCSync_p->SyncDelay           = 0;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    EPTS_INIT(AudioSTCSync_p->LastPTS);
    EPTS_INIT(AudioSTCSync_p->LastPCR);
    EPTS_INIT(AudioSTCSync_p->NewPCR);
#endif
    AudioSTCSync_p->BadClockCount = 0;
    AudioSTCSync_p->HugeCount = 0;
    AudioSTCSync_p->SkipPause = 0;
    AudioSTCSync_p->PauseSkip = 0;
    AudioSTCSync_p->Synced = 0;

    PlayerAVsync_p->PTSValid = FALSE;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    EPTS_INIT(PlayerAVsync_p->CurrentPTS);
#endif
    PlayerAVsync_p->CurrentSystemTime = 0;
    PlayerAVsync_p->SyncCompleted = FALSE;
    PlayerAVsync_p->InSyncCount = 0;

    PlayerAVsync_p->StartupSyncCount = 1000;
    PlayerAVsync_p->StartSync = FALSE;
    PlayerAVsync_p->Freq = Control_p->samplingFrequency;// Use previous frequency

    /* Reset everything related to avsync*/
    Control_p->skipDuration = 0;
    Control_p->pauseDuration = 0;
}

ST_ErrorCode_t      STAud_PCMPlayerMute(STPCMPLAYER_Handle_t PlayerHandle, BOOL PCMMute)
{
   PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempPCMplayerControlBlock_p != NULL)
    {
        if (tempPCMplayerControlBlock_p == (PCMPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if(tempPCMplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);
    /*change the next state*/
    STOS_MutexLock(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);
    tempPCMplayerControlBlock_p->pcmPlayerControl.pcmMute = PCMMute;
    STOS_MutexRelease(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);

    return (Error);
}



ST_ErrorCode_t      STAud_PCMPlayerGetCapability(STPCMPLAYER_Handle_t PlayerHandle, STAUD_OPCapability_t * Capability_p)
{
   PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempPCMplayerControlBlock_p != NULL)
    {
        if (tempPCMplayerControlBlock_p == (PCMPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if(tempPCMplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Capability_p->ChannelMuteCapable = FALSE;
    Capability_p->MaxSyncOffset      = 0;
    Capability_p->MinSyncOffset      = 0;

    return (Error);
}

ST_ErrorCode_t      STAud_PCMPlayerSetOutputParams(STPCMPLAYER_Handle_t PlayerHandle, STAUD_PCMDataConf_t PCMOutParams)
{
   PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempPCMplayerControlBlock_p != NULL)
    {
        if (tempPCMplayerControlBlock_p == (PCMPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if(tempPCMplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    STOS_MutexLock(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);
    tempPCMplayerControlBlock_p->pcmPlayerControl.pcmPlayerDataConfig = PCMOutParams;
    STOS_MutexRelease(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);

    return (Error);
}


ST_ErrorCode_t      STAud_PCMPlayerGetOutputParams(STPCMPLAYER_Handle_t PlayerHandle, STAUD_PCMDataConf_t * PCMOutParams)
{
   PCMPlayerControlBlock_t * tempPCMplayerControlBlock_p = pcmPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempPCMplayerControlBlock_p != NULL)
    {
        if (tempPCMplayerControlBlock_p == (PCMPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempPCMplayerControlBlock_p = tempPCMplayerControlBlock_p->next;
    }

    if(tempPCMplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    STOS_MutexLock(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);
    *PCMOutParams = tempPCMplayerControlBlock_p->pcmPlayerControl.pcmPlayerDataConfig;
    STOS_MutexRelease(tempPCMplayerControlBlock_p->pcmPlayerControl.LockControlStructure);

    return (Error);
}

void UpdatePCMSynced(PCMPlayerControl_t *Control_p)
{
    U32 FrameTime   = 0;
    U32 SampleSize  = (Control_p->NumChannels * BYTES_PER_CHANNEL_IN_PLAYER);
    U32 FrameSize   = (Control_p->CurrentFrameSize / SampleSize);
    U32 PauseSize   = (Control_p->dummyBufferSize / SampleSize) - FrameSize;
    U32 Duration    = 0;

    if(Control_p->audioSTCSync.SkipPause)
    {
        FrameTime = (((FrameSize - 192) * 1000)/Control_p->samplingFrequency);

        Duration = Control_p->audioSTCSync.SkipPause;
    }

    if(Control_p->audioSTCSync.PauseSkip)
    {
        FrameTime = ((PauseSize * 1000)/Control_p->samplingFrequency);

        Duration = Control_p->audioSTCSync.PauseSkip;
    }

    Control_p->audioSTCSync.Synced = ((Duration + FrameTime)/FrameTime);
    if(Control_p->audioSTCSync.Synced)
    {
        Control_p->audioSTCSync.Synced += 2;
    }
    STTBX_Print(("PCM Time=%d, Sync=%d, \n",Duration, Control_p->audioSTCSync.Synced));
}

U32 PCMPlayer_ConvertFsMultiplierToClkDivider(U32 FrequencyMultiplier)
{
    U32 clk_div = 0;
    switch (FrequencyMultiplier)
    {
        case 1:
            clk_div = 0;
            break;
        case 128:
            clk_div = 1;
            break;
        case 192:
        case 768:
            clk_div = 6;
            break;
        case 256:
            clk_div = 2;
            break;
        case 384:
            clk_div = 3;
            break;
        case 512:
            clk_div = 4;
            break;

        default:
            // For any unknown value go to 256 Fs
            clk_div = 2;
            break;
    }
    return(clk_div);
}


BOOL PCMPlayer_StartupSync(PlayerAudioBlock_t * PlayerAudioBlock_p, PCMPlayerControlBlock_t * PCMPlayerControlBlock_p)
{

#if ENABLE_STARTUP_SYNC

#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
    PCMPlayerControl_t  * Control_p = &(PCMPlayerControlBlock_p->pcmPlayerControl);
    AudAVSync_t * PlayerAVsync_p = &(Control_p->audioAVsync);

    if(Control_p->AVSyncEnable == TRUE)
    {
        STCLKRV_ExtendedSTC_t tempPCR;
        ST_ErrorCode_t Error = ST_NO_ERROR;

#if MEMORY_SPLIT_SUPPORT
        U32 MemorySize = PlayerAudioBlock_p->MemoryParams.Size;
        U32 Flags = PlayerAudioBlock_p->MemoryParams.Flags;
#else
        U32 MemorySize = PlayerAudioBlock_p->memoryBlock->Size;
        U32 Flags = PlayerAudioBlock_p->memoryBlock->Flags;
#endif

        memset(&tempPCR,0,sizeof(STCLKRV_ExtendedSTC_t));

        //check for PTS
        if(PlayerAVsync_p->StartupSyncCount && (!PlayerAVsync_p->StartSync))
        {
            PlayerAVsync_p->StartupSyncCount--;
            STTBX_Print(("PCM:SYNC %d \n",PlayerAVsync_p->StartupSyncCount));
            if(Flags & FREQ_VALID)
            {
                PlayerAVsync_p->Freq = PlayerAudioBlock_p->memoryBlock->Data[FREQ_OFFSET];
            }

            if(Flags & PTS_VALID)
            {
                PlayerAVsync_p->CurrentPTS.BaseValue = PlayerAudioBlock_p->memoryBlock->Data[PTS_OFFSET];
                PlayerAVsync_p->CurrentPTS.BaseBit32 = PlayerAudioBlock_p->memoryBlock->Data[PTS33_OFFSET];
                Control_p->audioSTCSync.LastPTS      = PlayerAVsync_p->CurrentPTS;
                Control_p->audioSTCSync.SyncState    = SYNCHRO_STATE_NORMAL;

            }
            else
            {
                if(PlayerAVsync_p->StartupSyncCount == 999)
                {
                    PlayerAVsync_p->StartSync = TRUE;
                }
                else
                {
                    ADDToEPTS(&(PlayerAVsync_p->CurrentPTS), MILLISECOND_TO_PTS((MemorySize/(Control_p->NumChannels *
                                                                 BYTES_PER_CHANNEL_IN_PLAYER) * 1000)/PlayerAVsync_p->Freq));
                }
            }

            if(!PlayerAVsync_p->StartSync)
            {
                Error = STCLKRV_GetExtendedSTC(Control_p->CLKRV_HandleForSynchro, &tempPCR);

                if(Error == ST_NO_ERROR)
                {
                    U32 PTSSTCDiff = 0;
                    BOOL DoSync=FALSE;
                    STCLKRV_ExtendedSTC_t CurrentPTS = PlayerAVsync_p->CurrentPTS;

                    ADDToEPTS(&CurrentPTS, MILLISECOND_TO_PTS(Control_p->audioSTCSync.Offset + Control_p->audioSTCSync.UsrLatency));
                    PTSSTCDiff = EPTS_DIFF(CurrentPTS,tempPCR);
                    DoSync=FALSE;
                    if (EPTS_LATER_THAN(CurrentPTS, tempPCR))
                    {
                        if (PTSSTCDiff<= HUGE_TIME_DIFFERENCE_PAUSE) DoSync=TRUE;
                    }
                    else
                    {
                        if (PTSSTCDiff<= HUGE_TIME_DIFFERENCE_SKIP) DoSync=TRUE;
                    }

                    if( DoSync == TRUE/*PTSSTCDiff <= HUGE_TIME_DIFFERENCE*/)
                    {
                        if(EPTS_LATER_THAN(CurrentPTS,tempPCR))
                        {
                            U32 Diffms = PTS_TO_MILLISECOND(PTSSTCDiff);

                            if(Diffms > 500)
                            {
                                Diffms = 500;
                            }
                            Diffms = (Diffms *8)/10; // Reduce to 80%
                            AUD_TaskDelayMs(Diffms);
                            STTBX_Print(("PCMP:Delay %d,%d\n",PTS_TO_MILLISECOND(PTSSTCDiff),PlayerAVsync_p->StartupSyncCount));
                            PlayerAVsync_p->StartSync = TRUE;
                        }
                        else if(EPTS_LATER_THAN(tempPCR,CurrentPTS))
                            {
                                /*We are going to drop the frame so update any characteristics that have to updated*/
                                if ((Flags & FREQ_VALID) && (Control_p->samplingFrequency != PlayerAudioBlock_p->memoryBlock->Data[FREQ_OFFSET]))
                                {
                                    // Sampling frequency changed this should ideally occur only at the begining of playback
                                    Control_p->samplingFrequency = PlayerAudioBlock_p->memoryBlock->Data[FREQ_OFFSET];
                                    SetPCMPlayerSamplingFrequency(Control_p);
                                }

                                if (PCMPFreeIpBlk(PCMPlayerControlBlock_p, PlayerAudioBlock_p) != ST_NO_ERROR)
                                {
                                    STTBX_Print(("PCM:err freeing startsync\n"));
                                    PlayerAVsync_p->StartSync = TRUE;
                                    return (TRUE);
                                }
                                else
                                {
                                    STTBX_Print(("PCM:drop %d,%d\n",PTS_TO_MILLISECOND(PTSSTCDiff),PlayerAVsync_p->StartupSyncCount));
                                }
                                STOS_SemaphoreSignal(Control_p->pcmPlayerTaskSem_p);
                                return (FALSE);
                            }
                        }
                        else
                        {
                            PlayerAVsync_p->StartSync = TRUE;
                        }
                }
                else
                {
                    PlayerAVsync_p->StartSync = TRUE;
                }
            }

        }
    }
    else
    {
        PlayerAVsync_p->StartSync = TRUE;
    }
#endif //#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
#endif // #if ENABLE_STARTUP_SYNC
    return (TRUE);
}


ST_ErrorCode_t PCMPFreeIpBlk(PCMPlayerControlBlock_t * tempPcmPlayerControlBlock_p,PlayerAudioBlock_t * PlayerAudioBlock_p)
{
    PCMPlayerControl_t      * Control_p     = &(tempPcmPlayerControlBlock_p->pcmPlayerControl);
    MemoryBlock_t * memoryBlock_p = PlayerAudioBlock_p->memoryBlock;
#if MEMORY_SPLIT_SUPPORT
    U32 Flags = PlayerAudioBlock_p->MemoryParams.Flags;
#else
    U32 Flags = PlayerAudioBlock_p->memoryBlock->Flags;
#endif
    if (memoryBlock_p != NULL)
    {
        if (Flags & EOF_VALID)
        {
            // Last block of file so generate EOF event
            Player_NotifyEvt(STAUD_EOF_EVT, 0, Control_p);
            STTBX_Print(("PCMP:Sent EOF\n"));
        }
        return (MemPool_FreeIpBlk(Control_p->memoryBlockManagerHandle, (U32 *)(& memoryBlock_p), (U32)tempPcmPlayerControlBlock_p));
    }
    else
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
}

#if ENABLE_SYNC_CALC_IN_CALLBACK
void CalculatePCMPSync(PCMPlayerControl_t * Control_p)
{
    AudSTCSync_t  * AudioSTCSync_p = &(Control_p->audioSTCSync);
    AudAVSync_t * PlayerAVsync_p = &(Control_p->audioAVsync);

    Player_NotifyEvt(STAUD_PTS_EVT, 0, Control_p);

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    PTSPCROffsetInms(Control_p, PlayerAVsync_p->CurrentPTS);
#endif

    if (AudioSTCSync_p->SkipPause != 0)
    {
        UpdatePCMSynced(Control_p);

        if(Control_p->skipDuration == 0)
        {
            Control_p->skipDuration = AudioSTCSync_p->SkipPause;
        }
        Player_NotifyEvt(STAUD_OUTOF_SYNC,0,Control_p);
        PlayerAVsync_p->SyncCompleted = FALSE;
        PlayerAVsync_p->InSyncCount = 0;
    }
    else
    {
        PlayerAVsync_p->ValidPTSCount++;
    }

    if (AudioSTCSync_p->PauseSkip != 0)
    {
        UpdatePCMSynced(Control_p);

        if (Control_p->pauseDuration == 0)
        {
            Control_p->pauseDuration = AudioSTCSync_p->PauseSkip;
        }
        Player_NotifyEvt(STAUD_OUTOF_SYNC,0,Control_p);
        PlayerAVsync_p->SyncCompleted = FALSE;
        PlayerAVsync_p->InSyncCount = 0;
    }
    else
    {
        PlayerAVsync_p->ValidPTSCount++;
    }

    if ((AudioSTCSync_p->SkipPause == 0) &&
         (AudioSTCSync_p->PauseSkip == 0) &&
         (PlayerAVsync_p->SyncCompleted == FALSE))

    {
        PlayerAVsync_p->InSyncCount++;
        //STTBX_Print(("TH:%d\n",PlayerAVsync_p->InSyncCount));
        if(PlayerAVsync_p->InSyncCount == IN_SYNC_THRESHHOLD)
        {
            PlayerAVsync_p->InSyncCount = 0;
            PlayerAVsync_p->SyncCompleted = TRUE;
            Player_NotifyEvt(STAUD_IN_SYNC,0,Control_p);
            //STTBX_Print(("Sync completed for Live decode\n"));
        }

    }
}
#endif

ST_ErrorCode_t PCMReleaseMemory(PCMPlayerControlBlock_t * ControlBlock_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(ControlBlock_p)
    {
        PCMPlayerControl_t * Control_p = &(ControlBlock_p->pcmPlayerControl);
        ST_Partition_t * CPUPartition = Control_p->CPUPartition;
        STAVMEM_FreeBlockParams_t FreeBlockParams;

        if(Control_p->PlayerAudioBlock_p)
        {
            memory_deallocate(CPUPartition,Control_p->PlayerAudioBlock_p);
            Control_p->PlayerAudioBlock_p = NULL;
        }

        if(Control_p->PCMPlayerNodeHandle)
        {
            FreeBlockParams.PartitionHandle = Control_p->AVMEMPartition;
            Error |= STAVMEM_FreeBlock(&FreeBlockParams, &(Control_p->PCMPlayerNodeHandle));
            Control_p->PCMPlayerNodeHandle = 0;
        }

        /*deallocate dummybuffer*/
        if(Control_p->dummyBufferBaseAddress)
        {
            if(Control_p->DummyBufferHandle && Control_p->DummyBufferPartition)
            {
                FreeBlockParams.PartitionHandle = Control_p->DummyBufferPartition;
                Error |= STAVMEM_FreeBlock(&FreeBlockParams, &(Control_p->DummyBufferHandle));
                Control_p->DummyBufferHandle = 0;
            }
#ifndef STAUD_REMOVE_EMBX
            else if (Control_p->tp)
            {
                Error |= EMBX_Free((EMBX_VOID *)Control_p->dummyBufferBaseAddress);
                Error |= EMBX_CloseTransport(Control_p->tp);
            }
#endif
            Control_p->dummyBufferBaseAddress = NULL;
            Control_p->dummyBufferSize = 0;
        }

        /* Unlock FDMA Channel*/
        if(Control_p->pcmPlayerChannelId)
        {
            Error |= STFDMA_UnlockChannel(Control_p->pcmPlayerChannelId,
                                    Control_p->FDMABlock);
            Control_p->pcmPlayerChannelId = 0;
        }

        if(Control_p->pcmPlayerTaskSem_p)
        {
            STOS_SemaphoreDelete(NULL,Control_p->pcmPlayerTaskSem_p);
            Control_p->pcmPlayerTaskSem_p = NULL;
        }

        if(Control_p->LockControlStructure)
        {
            STOS_MutexDelete(Control_p->LockControlStructure);
            Control_p->LockControlStructure = NULL;
        }

        if(Control_p->pcmPlayerCmdTransitionSem_p)
        {
            STOS_SemaphoreDelete(NULL,Control_p->pcmPlayerCmdTransitionSem_p);
            Control_p->pcmPlayerCmdTransitionSem_p = NULL;
        }

        if(Control_p->EvtHandle)
        {
            Error |= PCMPlayer_UnRegisterEvents(Control_p);
            Error |= PCMPlayer_UnSubscribeEvents(Control_p);
            // Close evt driver
            Error |= STEVT_Close(Control_p->EvtHandle);
            Control_p->EvtHandle = 0;
        }

        /*release control block*/
        memory_deallocate(CPUPartition,ControlBlock_p);
    }

    return Error;
}

