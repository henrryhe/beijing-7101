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
#define __OS_20TO21_MAP_H /* prevent os_20to21_map.h from being included*/
#include "sttbx.h"
#endif
#include "stos.h"
#include "aud_spdifplayer.h"
#include "audioreg.h"

//#define ENABLE_SPDIF_INTERRUPT 1

#ifndef ST_51XX
    extern U32 MAX_SAMPLES_PER_FRAME;
#endif

#if defined( ST_OSLINUX )
    #if defined (ENABLE_SPDIF_INTERRUPT)
        U32 Spdif_Interrupt_number = 147;//ST7100_SPDIF_PLAYER_INTERRUPT
        irqreturn_t SpdifPInterruptHandler( int irq, void *Data, struct pt_regs *regs);
        U32 Spdif_underflow_count = 0;
        U32 Spdif_InterruptValue = 0;
        U32 Spdif_InterruptEnable = 0;
    #endif //ENABLE_SPDIF_INTERRUPT
#endif //ST_OSLINUX

#if defined (ENABLE_SPDIF_INTERRUPT) && (defined(ST_OS21) || defined(ST_51XX))
    #if defined(ST_OS21)
        int SpdifPInterruptHandler(void* pParams);
    #elif defined(ST_51XX)
        void SpdifPInterruptHandler(void* pParams);
    #endif
    U32 Spdif_Interrupt_number;
#endif

#define NUM_SPDIF_NODES_FOR_STARTUP           4
#define SPDIF_DEFAULT_SAMPLING_FREQUENCY  48000
#define ENABLE_SYNC_CALC_IN_CALLBACK           1

#define AVSYNC_ADDED    1

#define LOG_AVSYNC_SPDIF_LINUX  0
#if LOG_AVSYNC_SPDIF_LINUX
    #define NUM_OF_ELEMENTS 20
    U32 Log_To_Spdif_Array = 1;
    U32 Aud_Spdif_skip_Array[NUM_OF_ELEMENTS][3];
    U32 Aud_spdif_skip_Count = 0;

    U32 Aud_spdif_Pause_Array[NUM_OF_ELEMENTS][3];
    U32 Aud_spdif_Pause_Count = 0;
#endif

STSPDIFPLAYER_Handle_t SPDIFPLAYER_Handle[MAX_SPDIF_PLAYER_INSTANCE];
ST_DeviceName_t SPDIFPlayer_DeviceName[] = {
#ifdef SPDIFP0_SUPPORTED
                                            "SPDIFPLAYER",
#endif
#ifdef HDMISPDIFP0_SUPPORTED
                                            "HDMI_SPDIFPLAY",
#endif
                                            };

#define LOG_AVSYNC 0
#if LOG_AVSYNC
    static U32 Aud_Array[2][10000];
    static U32 Aud_Count = 0;
    //static U32 Aud_Array1[2][10000];
    //static U32 Aud_Count1=0;
    static U32 restartcount = 0;
#endif

#ifdef ST_OSWINCE
    #if defined (ENABLE_SPDIF_INTERRUPT)
        #define ST7109_SPDIF_PLAYER_INTERRUPT         IRQ_SPDIFPLYR
        #undef interrupt_install
        #undef interrupt_uninstall
        #undef interrupt_handle
        #undef interrupt_handle
        #undef interrupt_enable

        #define interrupt_install(a,b,c)     interrupt_install_WinCE((U32)interrupt_handle(a),(interrupt_handler_t )b,c)
        #define interrupt_uninstall(a)       interrupt_uninstall_WinCE((U32)interrupt_handle(a))
        #define interrupt_enable(a)          interrupt_enable_WinCE((U32)interrupt_handle(a))
        #define interrupt_handle(a)          ((U32)(a))
    #endif
#endif

#if (BYTES_PER_CHANNEL_IN_PLAYER == 4)
    #define SHIFTS_PER_CHANNEL  2
#else
    #if (BYTES_PER_CHANNEL_IN_PLAYER == 2)
        #define     SHIFTS_PER_CHANNEL  1
    #endif
#endif

#define SHIFTS_FOR_TWO_CHANNEL  (SHIFTS_PER_CHANNEL+1)
#define DTS_PREAMBLE_D 2031

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
    #define HUGE_TIME_DIFFERENCE_PAUSE                (5 * 90000) /* 5 seconds at 90KHz */
    #define HUGE_TIME_DIFFERENCE_SKIP                   (((188 * 512 * 44 * 8) / 32000) * 90000) /* STFAE - 17 minutes 38 secondes at 90KHz */
#else
    #define HUGE_TIME_DIFFERENCE_PAUSE                180000 /* 2 seconds at 90KHz */
    #define HUGE_TIME_DIFFERENCE_SKIP                   180000 /* 2 seconds at 90KHz */
#endif

#define CORRECTIVE_ACTION_AUDIO_AHEAD_THRESHOLD         (10 * 90)   /* 10ms at 90KHz */
#define CORRECTIVE_ACTION_AUDIO_BEHIND_THRESHOLD        (24 * 90)   /* 24ms at 90KHz */


/* ---------------------------------------------------------------------------- */
/*                               Private Types                                  */
/* ---------------------------------------------------------------------------- */
SPDIFPlayerControlBlock_t * spdifPlayerControlBlock = NULL;

static ST_ErrorCode_t SPDIFReleaseMemory(SPDIFPlayerControlBlock_t * ControlBlock_p);

ST_ErrorCode_t STAud_SPDIFPlayerInit(ST_DeviceName_t Name,STSPDIFPLAYER_Handle_t *handle,SPDIFPlayerInitParams_t *Param_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock, * lastSPDIFPlayerControlBlock_p = spdifPlayerControlBlock;
    U32 i = 0;
    SPDIFPlayerControl_t *Control_p;
    STAVMEM_AllocBlockParams_t  AllocParams;

    if ((strlen(Name) <= ST_MAX_DEVICE_NAME))
    {
        while (tempSPDIFplayerControlBlock_p != NULL)
        {
            if (strcmp(tempSPDIFplayerControlBlock_p->spdifPlayerControl.Name, Name) != 0)
            {
                lastSPDIFPlayerControlBlock_p = tempSPDIFplayerControlBlock_p;
                tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
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

    tempSPDIFplayerControlBlock_p = memory_allocate(Param_p->CPUPartition,sizeof(SPDIFPlayerControlBlock_t));
    if(!tempSPDIFplayerControlBlock_p)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(tempSPDIFplayerControlBlock_p,0,sizeof(SPDIFPlayerControlBlock_t));

    Control_p = &(tempSPDIFplayerControlBlock_p->spdifPlayerControl);

    strncpy(Control_p->Name, Name, ST_MAX_DEVICE_NAME);
    *handle = (STSPDIFPLAYER_Handle_t)tempSPDIFplayerControlBlock_p;

    // Get params from init params
    Control_p->CPUPartition                             = Param_p->CPUPartition;
    Control_p->AVMEMPartition                         = Param_p->AVMEMPartition;
    Control_p->DummyBufferPartition               = Param_p->DummyBufferPartition;
    Control_p->spdifPlayerNumNode                 = Param_p->spdifPlayerNumNode;
    Control_p->spdifPlayerIdentifier                  = Param_p->spdifPlayerIdentifier;
    Control_p->pcmMemoryBlockManagerHandle               = Param_p->pcmMemoryBlockManagerHandle;
    Control_p->compressedMemoryBlockManagerHandle0  = Param_p->compressedMemoryBlockManagerHandle0;
    Control_p->compressedMemoryBlockManagerHandle1  = Param_p->compressedMemoryBlockManagerHandle1;
    Control_p->compressedMemoryBlockManagerHandle2  = Param_p->compressedMemoryBlockManagerHandle2;

    Control_p->SPDIFPlayerOutParams             = Param_p->SPDIFPlayerOutParams;
    Control_p->SPDIFMode                              = Param_p->SPDIFMode;
    Control_p->NumChannels                           = Param_p->NumChannels;
    Control_p->EnableMSPPSupport                  = Param_p->EnableMSPPSupport;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    Control_p->CLKRV_Handle                         = Param_p->CLKRV_Handle;
    Control_p->CLKRV_HandleForSynchro               = Param_p->CLKRV_HandleForSynchro;
#endif
    //strcpy(Control_p->EvtHandlerName, Param_p->EvtHandlerName);
    strncpy(Control_p->EvtHandlerName, Param_p->EvtHandlerName, ST_MAX_DEVICE_NAME);

    // Set default params for this SPDIF player
    Control_p->spdifPlayerCurrentState          = SPDIFPLAYER_INIT;
    Control_p->spdifPlayerNextState              = SPDIFPLAYER_INIT;
    Control_p->samplingFrequency                = SPDIF_DEFAULT_SAMPLING_FREQUENCY;

    Control_p->spdifPlayerPlayed = 0;
    Control_p->spdifPlayerToProgram = 0;
    Control_p->spdifPlayerTransferId = 0; // Indicates that no transfer is running
    Control_p->spdifMute = FALSE;
    Control_p->mpegLayer = LAYER_2;
    Control_p->CompressedDataAlignment = LE;
    Control_p->EnableEncOutput = FALSE;
    Control_p->Spdif_Pause_Performed = 0;
    Control_p->Spdif_Skip_Performed = 0;
    Control_p->Spdif_Huge_Diff = 0;

    /* Set the FDMA block used */
    Control_p->FDMABlock = STFDMA_1;

    /* Debug info */
    Control_p->FDMAAbortSent = 0;
    Control_p->FDMAAbortCBRecvd = 0;
    Control_p->AbortSent = FALSE;
    Control_p->EOFBlock_Received = FALSE;

    Error = STFDMA_LockChannelInPool(STFDMA_SPDIF_POOL, &(Control_p->spdifPlayerChannelId),Control_p->FDMABlock);
    if (Error != ST_NO_ERROR)
    {
        Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
        return(Error);
    }

    Control_p->PlayerAudioBlock_p = memory_allocate(Param_p->CPUPartition,(sizeof(PlayerAudioBlock_t) * Control_p->spdifPlayerNumNode));
    if(Control_p->PlayerAudioBlock_p == NULL)
    {
        Error = SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
        return ST_ERROR_NO_MEMORY;
    }
    memset(Control_p->PlayerAudioBlock_p, 0, (sizeof(PlayerAudioBlock_t) * Control_p->spdifPlayerNumNode));

    AllocParams.PartitionHandle = Control_p->AVMEMPartition;
    AllocParams.Alignment = 32;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocParams.ForbiddenRangeArray_p = NULL;
    AllocParams.ForbiddenBorderArray_p = NULL;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.NumberOfForbiddenBorders = 0;
#ifdef MSPP_PARSER
    AllocParams.Size = sizeof(STFDMA_GenericNode_t) * (Control_p->spdifPlayerNumNode + 1) ;
#else
    AllocParams.Size = sizeof(STFDMA_GenericNode_t) * Control_p->spdifPlayerNumNode ;
#endif
    Error = STAVMEM_AllocBlock(&AllocParams, &(Control_p->SPDIFPlayerNodeHandle));
    if(Error != ST_NO_ERROR)
    {
        Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(Control_p->SPDIFPlayerNodeHandle, (void *)(&(Control_p->spdifPlayerFDMANodesBaseAddress)));
    if(Error != ST_NO_ERROR)
    {
        Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
        return Error;
    }

    Control_p->dummyBufferSize = (((MAX_SAMPLES_PER_FRAME/192)*192) * Control_p->NumChannels * BYTES_PER_CHANNEL_IN_PLAYER) * 2; // We need double the size of max pcm buffer
    if (Param_p->dummyBufferBaseAddress != NULL)
    {
        Control_p->dummyBufferSize = Param_p->dummyBufferSize;
        Control_p->dummyBufferBaseAddress = Param_p->dummyBufferBaseAddress;
    }
    else
    {
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
                Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
                return Error;
            }

            Error = STAVMEM_GetBlockAddress(Control_p->DummyBufferHandle, (void *)(&(Control_p->dummyBufferBaseAddress)));
            if(Error != ST_NO_ERROR)
            {
                Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
                return Error;
            }
        }
#ifndef STAUD_REMOVE_EMBX
        else
        {
            Error = EMBX_OpenTransport(EMBX_TRANSPORT_NAME, &Control_p->tp);
            if(Error != EMBX_SUCCESS)
            {
                Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
                return Error;
            }

            Error = EMBX_Alloc(Control_p->tp, Control_p->dummyBufferSize, (EMBX_VOID **)(&Control_p->dummyBufferBaseAddress));
            if(Error != EMBX_SUCCESS)
            {
                Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
                return Error;
            }
        }
#endif
    }
#ifndef MSPP_PARSER
    memset((char *)Control_p->dummyBufferBaseAddress, 0, Control_p->dummyBufferSize);
#endif

    for (i=0; i < Control_p->spdifPlayerNumNode; i++)
    {
        STFDMA_GenericNode_t    * PlayerDmaNode_p = 0;

        PlayerDmaNode_p = (((STFDMA_GenericNode_t   *)Control_p->spdifPlayerFDMANodesBaseAddress) + i);
        Control_p->PlayerAudioBlock_p[i].PlayerDmaNode_p = PlayerDmaNode_p;
        Control_p->PlayerAudioBlock_p[i].PlayerDMANodePhy_p = (U32*)STOS_VirtToPhys(PlayerDmaNode_p);

        if(i < (Control_p->spdifPlayerNumNode - 1))
        {
            PlayerDmaNode_p->SPDIFNode.Next_p = (struct STFDMA_SPDIFNode_s *)STOS_VirtToPhys((U32)(PlayerDmaNode_p + 1) );
        }
        else
        {
            PlayerDmaNode_p->SPDIFNode.Next_p = (struct STFDMA_SPDIFNode_s *)STOS_VirtToPhys((U32)Control_p->spdifPlayerFDMANodesBaseAddress );
        }

        // Fill fdma node default parameters
        PlayerDmaNode_p->SPDIFNode.NumberBytes = 0;
        PlayerDmaNode_p->SPDIFNode.SourceAddress_p = 0;

        PlayerDmaNode_p->SPDIFNode.NodeCompleteNotify = TRUE;
        PlayerDmaNode_p->SPDIFNode.NodeCompletePause = FALSE;

        PlayerDmaNode_p->SPDIFNode.Extended = STFDMA_EXTENDED_NODE;
        PlayerDmaNode_p->SPDIFNode.Type = STFDMA_EXT_NODE_SPDIF;

        /*Get the Addresses*/
        {
            STAUD_BufferParams_t     Buffer;
            if(Param_p->pcmMemoryBlockManagerHandle)
            {
                Error|=MemPool_GetBufferParams(Param_p->pcmMemoryBlockManagerHandle,&Buffer);
                if(Error!=ST_NO_ERROR)
                {
                    Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
                    return Error;
                }
                Control_p->PlayerInputMemoryAddressPCM.BaseUnCachedVirtualAddress = (U32 *)Buffer.BufferBaseAddr_p;
                Control_p->PlayerInputMemoryAddressPCM.BasePhyAddress = (U32 *)STOS_VirtToPhys(Buffer.BufferBaseAddr_p);
                Control_p->PlayerInputMemoryAddressPCM.BaseCachedVirtualAddress = NULL;
            }

            if(Param_p->compressedMemoryBlockManagerHandle0)
            {
                Error |= MemPool_GetBufferParams(Param_p->compressedMemoryBlockManagerHandle0,&Buffer);
                if(Error!=ST_NO_ERROR)
                {
                    Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
                    return Error;
                }
                Control_p->PlayerInputMemoryAddressCom0.BaseUnCachedVirtualAddress = (U32 *)Buffer.BufferBaseAddr_p;
                Control_p->PlayerInputMemoryAddressCom0.BasePhyAddress = (U32 *)STOS_VirtToPhys(Buffer.BufferBaseAddr_p);
                Control_p->PlayerInputMemoryAddressCom0.BaseCachedVirtualAddress = NULL;
            }

            if(Param_p->compressedMemoryBlockManagerHandle1)
            {
                Error|=MemPool_GetBufferParams(Param_p->compressedMemoryBlockManagerHandle1,&Buffer);
                if(Error!=ST_NO_ERROR)
                {
                    Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
                    return Error;
                }
                Control_p->PlayerInputMemoryAddressCom1.BaseUnCachedVirtualAddress = (U32 *)Buffer.BufferBaseAddr_p;
                Control_p->PlayerInputMemoryAddressCom1.BasePhyAddress = (U32 *)STOS_VirtToPhys(Buffer.BufferBaseAddr_p);
                Control_p->PlayerInputMemoryAddressCom1.BaseCachedVirtualAddress = NULL;
            }


            if(Param_p->compressedMemoryBlockManagerHandle2)
            {

                Error|=MemPool_GetBufferParams(Param_p->compressedMemoryBlockManagerHandle2,&Buffer);
                if(Error!=ST_NO_ERROR)
                {
                    Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
                    return Error;
                }
                Control_p->PlayerInputMemoryAddressCom2.BaseUnCachedVirtualAddress = (U32 *)Buffer.BufferBaseAddr_p;
                Control_p->PlayerInputMemoryAddressCom2.BasePhyAddress = (U32 *)STOS_VirtToPhys(Buffer.BufferBaseAddr_p);
                Control_p->PlayerInputMemoryAddressCom2.BaseCachedVirtualAddress = NULL;

            }

            Control_p->DummyMemoryAddress.BaseUnCachedVirtualAddress= (U32*)Control_p->dummyBufferBaseAddress;
            Control_p->DummyMemoryAddress.BasePhyAddress= (U32 *)STOS_VirtToPhys(Control_p->dummyBufferBaseAddress);
            Control_p->DummyMemoryAddress.BaseCachedVirtualAddress = NULL;
        }

#ifdef  SPDIFP0_SUPPORTED
        if(Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
        {
            PlayerDmaNode_p->SPDIFNode.DestinationAddress_p = (void *)(SPDIF_BASE_ADDRESS + SPDIF_FIFO_DATA); /* Address of the SPDIF Player */
            PlayerDmaNode_p->SPDIFNode.DReq                        = STFDMA_REQUEST_SIGNAL_SPDIF_AUDIO;
        }else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
        if(Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
        {
            Control_p->HDMI_SPDIFPlayerEnable = FALSE;
            PlayerDmaNode_p->SPDIFNode.DestinationAddress_p = (void *)(HDMI_SPDIFPLAYER_BASE + SPDIF_FIFO_DATA); /* Address of the SPDIF Player */
            PlayerDmaNode_p->SPDIFNode.DReq                        = STFDMA_REQUEST_SIGNAL_HDMI_SPDIF_PLYR;
        }else
#endif
        {
            /* Dummy {} to handle last else */
        }

#if !defined(ST_7200)
        PlayerDmaNode_p->SPDIFNode.DestinationAddress_p = (void *)STOS_VirtToPhys((U32)PlayerDmaNode_p->SPDIFNode.DestinationAddress_p );
#endif

        PlayerDmaNode_p->SPDIFNode.Pad = 0x0;
#if defined(ST_7109) || defined(ST_7200)
        if (Control_p->EnableMSPPSupport == TRUE)
        {
            PlayerDmaNode_p->SPDIFNode.Secure = 0x1;
        }
        else
        {
            PlayerDmaNode_p->SPDIFNode.Secure = 0x0;
        }

        PlayerDmaNode_p->SPDIFNode.Pad2 = 0x0;
#endif
        PlayerDmaNode_p->SPDIFNode.Valid = TRUE;

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
        PlayerDmaNode_p->SPDIFNode.ModeData = 0x0;  // compressed mode

        PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.BurstPeriod = MAX_SAMPLES_PER_FRAME;
        PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleA = 0xF872;
        PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleB = 0x4E1F;
        PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleC = 0x0001;
        PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleD = MAX_SAMPLES_PER_FRAME * 8;
#else
        PlayerDmaNode_p->SPDIFNode.BurstPeriod = MAX_SAMPLES_PER_FRAME;
        PlayerDmaNode_p->SPDIFNode.PreambleA = 0xF872;
        PlayerDmaNode_p->SPDIFNode.PreambleB = 0x4E1F;
        PlayerDmaNode_p->SPDIFNode.PreambleC = 0x0001;
        PlayerDmaNode_p->SPDIFNode.PreambleD = MAX_SAMPLES_PER_FRAME * 8;
#endif
        Control_p->PlayerAudioBlock_p[i].Valid = FALSE;
        Control_p->PlayerAudioBlock_p[i].memoryBlock = NULL;
    }

#ifdef MSPP_PARSER
    Control_p->SPDIFPlayerDmaNode_p = (((STFDMA_GenericNode_t   *)Control_p->spdifPlayerFDMANodesBaseAddress) + Control_p->spdifPlayerNumNode);
    Control_p->MemcpyNodeAddress.BaseUnCachedVirtualAddress =(U32*)Control_p->SPDIFPlayerDmaNode_p;
    Control_p->MemcpyNodeAddress.BasePhyAddress = (U32*)STOS_VirtToPhys(Control_p->SPDIFPlayerDmaNode_p);
    Control_p->SPDIFPlayerDmaNode_p->Node.NumberBytes                 = 0;      /*will change depending upon frame Size*/
    Control_p->SPDIFPlayerDmaNode_p->Node.SourceAddress_p           = NULL; /* will change depending upon Offset  */
    Control_p->SPDIFPlayerDmaNode_p->Node.DestinationAddress_p     = NULL; /* will change depening upon ES Buffer*/
    Control_p->SPDIFPlayerDmaNode_p->Node.Length                           = 0;      /* will chnage depending upon frame size*/
    Control_p->SPDIFPlayerDmaNode_p->Node.SourceStride                  = 0;
    Control_p->SPDIFPlayerDmaNode_p->Node.DestinationStride            = 0;
    Control_p->SPDIFPlayerDmaNode_p->Node.NodeControl.PaceSignal                = STFDMA_REQUEST_SIGNAL_NONE;
    Control_p->SPDIFPlayerDmaNode_p->Node.NodeControl.SourceDirection         = STFDMA_DIRECTION_INCREMENTING;
    Control_p->SPDIFPlayerDmaNode_p->Node.NodeControl.DestinationDirection   = STFDMA_DIRECTION_INCREMENTING;
    Control_p->SPDIFPlayerDmaNode_p->Node.NodeControl.NodeCompleteNotify   = TRUE;
    Control_p->SPDIFPlayerDmaNode_p->Node.NodeControl.NodeCompletePause   = TRUE;
    Control_p->SPDIFPlayerDmaNode_p->Node.NodeControl.Reserved = 0;
    Control_p->SPDIFPlayerDmaNode_p->Node.Next_p = NULL;
#endif

    // Initialize Tranfer player params for FDMA
    Control_p->TransferPlayerParams_p.ChannelId     = Control_p->spdifPlayerChannelId;
    Control_p->TransferPlayerParams_p.Pool              = STFDMA_SPDIF_POOL;
    Control_p->TransferPlayerParams_p.NodeAddress_p = Control_p->PlayerAudioBlock_p[0].PlayerDmaNode_p;
    Control_p->TransferPlayerParams_p.BlockingTimeout   = 0;
    Control_p->TransferPlayerParams_p.CallbackFunction  = (STFDMA_CallbackFunction_t)SPDIFPlayerFDMACallbackFunction;
    Control_p->TransferPlayerParams_p.ApplicationData_p= (void *)(tempSPDIFplayerControlBlock_p);
    Control_p->TransferPlayerParams_p.FDMABlock = Control_p->FDMABlock;


    Control_p->spdifPlayerTaskSem_p = STOS_SemaphoreCreateFifo(NULL,0);
    Control_p->LockControlStructure = STOS_MutexCreateFifo();
    Control_p->spdifPlayerCmdTransitionSem_p = STOS_SemaphoreCreateFifo(NULL,0);

    /*Set AVSync Params*/
    Control_p->AVSyncEnable                     = FALSE;

    ResetSPDIFPlayerSyncParams(&(tempSPDIFplayerControlBlock_p->spdifPlayerControl));
    Control_p->audioSTCSync.UsrLatency = 0; /*User Set latency can be +ve*/
    Control_p->audioSTCSync.Offset = 0;

    Control_p->spdifRestart = FALSE;
    Control_p->spdifForceRestart = FALSE;
    Control_p->underFlowCount = 0;

    {
        STEVT_OpenParams_t EVT_OpenParams;
        /* Open the ST Event */
        Error |= STEVT_Open(Control_p->EvtHandlerName,&EVT_OpenParams,&Control_p->EvtHandle);
        if (Error == ST_NO_ERROR)
        {
            strncpy(Control_p->EvtGeneratorName, Param_p->EvtGeneratorName, ST_MAX_DEVICE_NAME);
            Control_p->ObjectID = Param_p->ObjectID;
            Control_p->skipEventID   = Param_p->skipEventID;
            Control_p->pauseEventID = Param_p->pauseEventID;
            SPDIFPlayer_RegisterEvents(Control_p);
            SPDIFPlayer_SubscribeEvents(Control_p);
        }
        else
        {
            Error |= SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
            return Error;
        }
    }

    Error |= STAud_PlayerMemRemap(&Control_p->BaseAddress);
    if(Error != ST_NO_ERROR)
    {
        STTBX_Print(("SPDIF :: STAud_PlayerMemRemap Failed %x \n",Error));
    }

#if defined (ST_OS20)
    STOS_TaskCreate (SPDIFPlayerTask,(void *)tempSPDIFplayerControlBlock_p, Control_p->CPUPartition,SPDIFPLAYER_STACK_SIZE, (void **)&Control_p->spdifPlayerTask_pstack,Control_p->CPUPartition,
                        &Control_p->spdifPlayerTask_p,&Control_p->spdifPlayerTaskDesc,SPDIFPLAYER_TASK_PRIORITY,Name, 0);

#else
    STOS_TaskCreate (SPDIFPlayerTask,(void *)tempSPDIFplayerControlBlock_p, NULL, SPDIFPLAYER_STACK_SIZE,
                        NULL, NULL, &Control_p->spdifPlayerTask_p, NULL, SPDIFPLAYER_TASK_PRIORITY,
                        Name, 0);
#endif


    if (Control_p->spdifPlayerTask_p == NULL)
    {
        Error = SPDIFReleaseMemory(tempSPDIFplayerControlBlock_p);
        return ST_ERROR_NO_MEMORY;
    }

    tempSPDIFplayerControlBlock_p->next = NULL;

    if (spdifPlayerControlBlock == NULL)
    {
        spdifPlayerControlBlock = tempSPDIFplayerControlBlock_p;
    }
    else
    {
        lastSPDIFPlayerControlBlock_p->next = tempSPDIFplayerControlBlock_p;
    }

    {
#if defined (ENABLE_SPDIF_INTERRUPT)
#if defined(ST_OS21) && defined(ST_7100)
        Spdif_Interrupt_number = ST7100_SPDIF_PLAYER_INTERRUPT;
#elif defined(ST_OS21) && defined(ST_7109)
        Spdif_Interrupt_number = ST7109_SPDIF_PLAYER_INTERRUPT;
#elif defined(ST_OS21) && defined(ST_7200)
        Spdif_Interrupt_number = ST7200_SPDIF_PLAYER_INTERRUPT;
#elif defined(ST_51XX)
        Spdif_Interrupt_number = SPDIF_INTERRUPT;
#endif

        if (STOS_SUCCESS == STOS_InterruptInstall(Spdif_Interrupt_number,0,SpdifPInterruptHandler,"SPDIFP",(void *)tempSPDIFplayerControlBlock_p))
        {
            if (STOS_SUCCESS != STOS_InterruptEnable(Spdif_Interrupt_number,0))
            {
                STOS_InterruptUninstall(Spdif_Interrupt_number,0,tempSPDIFplayerControlBlock_p);
                STTBX_Print(("ERROR: Could not enable interrupt\n"));
            }
        }
        else
        {
                STTBX_Print(("ERROR: Interrupt handler load failed\n"));
        }
#endif

    }


#if defined(ST_OSLINUX)
#if defined (ENABLE_SPDIF_INTERRUPT)
    /* Enable the Under flow interrupt */
    {
        U32 Addr;
        Addr = Control_p->BaseAddress.SPDIFPlayerBaseAddr + 0x014;
        STSYS_WriteRegDev32LE(Addr,0x00000001);
    }
#endif //ENABLE_SPDIF_INTERRUPT
#endif//ST_OSLINUX

    return (Error);
}
#if defined( ST_OSLINUX )
#if defined (ENABLE_SPDIF_INTERRUPT)
irqreturn_t SpdifPInterruptHandler( int irq, void *Data, struct pt_regs *regs)
{

    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p;
    U32 Addr1,Addr2,SPDIFPlayerBaseAddr;

    tempSPDIFplayerControlBlock_p = (SPDIFPlayerControlBlock_t*)Data;
    SPDIFPlayerBaseAddr = tempSPDIFplayerControlBlock_p->spdifPlayerControl.BaseAddress.SPDIFPlayerBaseAddr;
    Addr1 = SPDIFPlayerBaseAddr + 0x08;
    if((*(U32*)Addr1) & 1)
    {
        Spdif_underflow_count++;
    }
    Addr2 = SPDIFPlayerBaseAddr + 0x0C;
    STSYS_WriteRegDev32LE(Addr2,0x00000003);
    return IRQ_HANDLED;
}
#endif //ENABLE_SPDIF_INTERRUPT
#endif //ST_OSLINUX

#if defined (ENABLE_SPDIF_INTERRUPT) && (defined(ST_OS21) || defined(ST_51XX))
#if defined(ST_OS21)
int SpdifPInterruptHandler(void* pParams)
#elif defined(ST_51XX)
void SpdifPInterruptHandler(void* pParams)
#endif
{
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = (SPDIFPlayerControlBlock_t*)pParams;
    SPDIFPlayerControl_t * Control_p = &(tempSPDIFplayerControlBlock_p->spdifPlayerControl);
    U32 status,SPDIFPlayerBaseAddr,HDMISPDIFPlayerBaseAddr;

    SPDIFPlayerBaseAddr = Control_p->BaseAddress.SPDIFPlayerBaseAddr;
    HDMISPDIFPlayerBaseAddr = Control_p->BaseAddress.HDMISPDIFPlayerBaseAddr;

#ifdef SPDIFP0_SUPPORTED
    if(Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
    {
        status = STSYS_ReadRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_INT_STATUS);
    }else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
    if(Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
    {
        status = STSYS_ReadRegDev32LE(HDMISPDIFPlayerBaseAddr + SPDIF_INT_STATUS);
    }else
#endif
    {
        /* Dummy {} to handle last else */
    }

    if(status & 1)
    {
        if (Control_p->spdifPlayerCurrentState == SPDIFPLAYER_PLAYING)
                Control_p->underFlowCount++;

        Control_p->spdifForceRestart = TRUE;

#ifdef SPDIFP0_SUPPORTED
        if(Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
        {
            STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_INT_EN_CLR, 0x1); // Disable underflow interrupt otherwise it will kill the system with underflows
            STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_INT_STATUS_CLR, 0x3f); // Clear all the interrupts
        }else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
        if(Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
        {
            STSYS_WriteRegDev32LE(HDMISPDIFPlayerBaseAddr + SPDIF_INT_EN_CLR, 0x1); // Disable underflow interrupt otherwise it will kill the system with underflows
            STSYS_WriteRegDev32LE(HDMISPDIFPlayerBaseAddr + SPDIF_INT_STATUS_CLR, 0x3f); // Clear all the interrupts
        }else
 #endif
        {
            /* Dummy {} to handle last else */
        }
    }
#if defined(ST_OS21)
    return (OS21_SUCCESS);
#endif

}
#endif

ST_ErrorCode_t STAud_SPDIFPlayerTerminate(STSPDIFPLAYER_Handle_t handle)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock, * lastSPDIFPlayerControlBlock_p = spdifPlayerControlBlock;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    SPDIFPlayerControl_t * Control_p;

//    U32 i;
    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        lastSPDIFPlayerControlBlock_p = tempSPDIFplayerControlBlock_p;
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p = &(tempSPDIFplayerControlBlock_p->spdifPlayerControl);

    if(Control_p->EvtHandle)
    {
        Error |= SPDIFPlayer_UnRegisterEvents(Control_p);
        Error |= SPDIFPlayer_UnSubscribeEvents(Control_p);
        // Close evt driver
        Error |= STEVT_Close(Control_p->EvtHandle);
        Control_p->EvtHandle = 0;
    }

    // Unlock FDMA Channel

    Error |=STFDMA_UnlockChannel(Control_p->spdifPlayerChannelId,Control_p->FDMABlock);

    if(Control_p->SPDIFPlayerNodeHandle)
    {
        FreeBlockParams.PartitionHandle = Control_p->AVMEMPartition;
        Error |= STAVMEM_FreeBlock(&FreeBlockParams, &(Control_p->SPDIFPlayerNodeHandle));
        Control_p->SPDIFPlayerNodeHandle = 0;
    }

    // Deallocate audio block
    if(Control_p->PlayerAudioBlock_p)
    {
        memory_deallocate(Control_p->CPUPartition, Control_p->PlayerAudioBlock_p);
    }

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
        else if(Control_p->tp)
        {
            Error |= EMBX_Free((EMBX_VOID *)Control_p->dummyBufferBaseAddress);
            Error |= EMBX_CloseTransport(Control_p->tp);
        }
#endif
        Control_p->dummyBufferBaseAddress = NULL;
        Control_p->dummyBufferSize = 0;
    }
#if defined (ENABLE_SPDIF_INTERRUPT)
    STOS_InterruptUninstall(Spdif_Interrupt_number,0,(void *) tempSPDIFplayerControlBlock_p);
#endif

    STAud_PlayerMemUnmap(&Control_p->BaseAddress);


    // Delete the SPDIF player semaphore and mutex
    Error |= STOS_SemaphoreDelete(NULL,Control_p->spdifPlayerTaskSem_p);
    Error |= STOS_MutexDelete(Control_p->LockControlStructure);
    Error |= STOS_SemaphoreDelete(NULL,Control_p->spdifPlayerCmdTransitionSem_p);

    lastSPDIFPlayerControlBlock_p->next = tempSPDIFplayerControlBlock_p->next;
    // deallocate control block
    if (tempSPDIFplayerControlBlock_p == spdifPlayerControlBlock)
        spdifPlayerControlBlock = spdifPlayerControlBlock->next;//NULL;
    memory_deallocate(Control_p->CPUPartition, tempSPDIFplayerControlBlock_p);

    return (Error);
}

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t STAud_SPDIFPlayerSetClkSynchro(STSPDIFPLAYER_Handle_t handle,STCLKRV_Handle_t ClkSource)
{
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock, * lastSPDIFPlayerControlBlock_p = spdifPlayerControlBlock;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        lastSPDIFPlayerControlBlock_p = tempSPDIFplayerControlBlock_p;
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    tempSPDIFplayerControlBlock_p->spdifPlayerControl.CLKRV_HandleForSynchro=ClkSource;

    return(ST_NO_ERROR);
}
#endif

void    SPDIFPlayerTask(void * tempSPDIFplayerControlBlock_p)
{
    SPDIFPlayerControlBlock_t * tempSPDIFPlayerControlBlock_p = (SPDIFPlayerControlBlock_t *)tempSPDIFplayerControlBlock_p;
    SPDIFPlayerControl_t        * Control_p     = &(tempSPDIFPlayerControlBlock_p->spdifPlayerControl);
    PlayerAudioBlock_t  * PlayerAudioBlock_p    = Control_p->PlayerAudioBlock_p;
    AudAVSync_t     * PlayerAVsync_p = &(Control_p->audioAVsync);
    U32             SPDIFPlayerBaseAddr,HDMISPDIFPlayerBaseAddr;
    ST_ErrorCode_t              err = ST_NO_ERROR;
    U32                         i = 0;
    SPDIFPlayerBaseAddr = Control_p->BaseAddress.SPDIFPlayerBaseAddr;
    HDMISPDIFPlayerBaseAddr = Control_p->BaseAddress.HDMISPDIFPlayerBaseAddr;


    STOS_TaskEnter(tempSPDIFPlayerControlBlock_p);

    while (1)
    {
        STOS_SemaphoreWait(Control_p->spdifPlayerTaskSem_p);

        switch (Control_p->spdifPlayerCurrentState)
        {
            case SPDIFPLAYER_INIT:

                STOS_MutexLock(Control_p->LockControlStructure);
                if (Control_p->spdifPlayerNextState != Control_p->spdifPlayerCurrentState)
                {
                    Control_p->spdifPlayerCurrentState = Control_p->spdifPlayerNextState;
                    //STTBX_Print(("SPDIF:from SPDIFPLAYER_INIT\n"));
                    STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);
                    // Signal the command completion transtition
                    STOS_SemaphoreSignal(Control_p->spdifPlayerCmdTransitionSem_p);
                }
                STOS_MutexRelease(Control_p->LockControlStructure);
                if(Control_p->spdifPlayerCurrentState == SPDIFPLAYER_START)
                {
                    ResetSPDIFPlayerSyncParams(Control_p);
                    //SetSPDIFPlayerSamplingFrequency(Control_p); Moved to just before StartSPDIFPlayerIP()
                }
                break;

            case SPDIFPLAYER_START:

                Control_p->Spdif_Pause_Performed=0;
                Control_p->Spdif_Skip_Performed=0;
                Control_p->Spdif_Huge_Diff =0;

                if ((Control_p->SPDIFMode != STAUD_DIGITAL_MODE_OFF)&&(Control_p->BypassMode!=TRUE))
                {

                    // Get number of buffers required for startup
                    for (i=Control_p->spdifPlayerPlayed; i < (Control_p->spdifPlayerPlayed + NUM_SPDIF_NODES_FOR_STARTUP); i++)
                    {
                        U32 Index = i % Control_p->spdifPlayerNumNode;
                        PlayerAudioBlock_t *AudioBlock_p = &PlayerAudioBlock_p[Index];
                        if (AudioBlock_p->Valid == FALSE)
                        {
#if MEMORY_SPLIT_SUPPORT
                            if (MemPool_GetSplitIpBlk(Control_p->memoryBlockManagerHandle,
                                     (U32 *)(& AudioBlock_p->memoryBlock),
                                     (U32)tempSPDIFPlayerControlBlock_p,
                                     & AudioBlock_p->MemoryParams) != ST_NO_ERROR)
#else
                            if (MemPool_GetIpBlk(Control_p->memoryBlockManagerHandle,
                                     (U32 *)(& AudioBlock_p->memoryBlock),
                                     (U32)tempSPDIFPlayerControlBlock_p) != ST_NO_ERROR)
#endif
                            {
                                break;
                            }
                            else
                            {

#ifdef HDMISPDIFP0_SUPPORTED
                                if (HDMISPDIF_TestSampFreq(AudioBlock_p , tempSPDIFPlayerControlBlock_p) == TRUE)
                                {
#endif
                                    if (SPDIFPlayer_StartupSync(AudioBlock_p,tempSPDIFPlayerControlBlock_p)== FALSE)
                                    {
                                        break;
                                    }
                                    STOS_MutexLock(Control_p->LockControlStructure);
                                    FillSPDIFPlayerFDMANode(AudioBlock_p, Control_p);
                                    STOS_MutexRelease(Control_p->LockControlStructure);
                                    {
#if MEMORY_SPLIT_SUPPORT
                                        U32 Flags = AudioBlock_p->MemoryParams.Flags;
#else
                                        U32 Flags = AudioBlock_p->memoryBlock->Flags;
#endif
                                        if(Flags & EOF_VALID)
                                        {
                                            Control_p->EOFBlock_Received =TRUE;
                                        }
                                    }
                                    AudioBlock_p->Valid = TRUE;
#ifdef HDMISPDIFP0_SUPPORTED
                                }
                                else
                                {
                                    i--;
                                }
#endif
                            }
                        }
                    }

                    // If all the required nodes are valid start the transfer
                    if (PlayerAudioBlock_p[(Control_p->spdifPlayerPlayed + NUM_SPDIF_NODES_FOR_STARTUP - 1) % Control_p->spdifPlayerNumNode].Valid ||
                            Control_p->EOFBlock_Received)
                    {
#ifdef MSPP_PARSER
                        {
                            U32 length = 0,sourceStride,destinationStride,buffersize ;
                            void * destinationAddress,* sourceAddress;
                            U32 localoffset = (Control_p->spdifPlayerPlayed + NUM_SPDIF_NODES_FOR_STARTUP - 1) % Control_p->spdifPlayerNumNode;
                            if(PlayerAudioBlock_p[localoffset].Valid)
                            {
                                buffersize         = Control_p->dummyBufferSize;
                                sourceAddress      = (void *)PlayerAudioBlock_p[localoffset].memoryBlock->MemoryStart;
                                destinationAddress = Control_p->DummyMemoryAddress.BasePhyAddress;
                                sourceStride = 1;
                                destinationStride = 1;

                                sourceAddress = (void *)STOS_AddressTranslate(Control_p->PlayerInputMemoryAddressPCM.BasePhyAddress,Control_p->PlayerInputMemoryAddressPCM.BaseUnCachedVirtualAddress,sourceAddress);

                                while(buffersize)
                                {
                                    destinationAddress = (void *)((U32)destinationAddress + length);
                                    if(buffersize >= PlayerAudioBlock_p[localoffset].memoryBlock->Size)
                                    {
                                        length      = PlayerAudioBlock_p[localoffset].memoryBlock->Size;
                                        buffersize -= length;
                                    }
                                    else
                                    {
                                        length     = buffersize;
                                        buffersize = 0;
                                    }

                                    FDMAMemcpy(destinationAddress,sourceAddress,length,length,sourceStride,destinationStride,&Control_p->MemcpyNodeAddress);
                                }
                            }
                        }
#endif

                        {
                            U32 BlockIndex = Control_p->spdifPlayerPlayed % Control_p->spdifPlayerNumNode;
                            MemoryBlock_t * MemoryBlock_p = PlayerAudioBlock_p[BlockIndex].memoryBlock;
#if MEMORY_SPLIT_SUPPORT
                            U32 Flags = PlayerAudioBlock_p[BlockIndex].MemoryParams.Flags;
#else
                            U32 Flags = MemoryBlock_p->Flags;
#endif
                            if ((Flags & FREQ_VALID) && (MemoryBlock_p->Data[FREQ_OFFSET]))
                            {
                                // Sampling frequency changed this should ideally occur only at the begining of playback
                                Control_p->samplingFrequency = MemoryBlock_p->Data[FREQ_OFFSET];
                                SetSPDIFPlayerSamplingFrequency(Control_p);
                                SetSPDIFChannelStatus(Control_p);
                            }

                        }
                        Control_p->spdifPlayerCurrentState = SPDIFPLAYER_PLAYING;
                        SetSPDIFPlayerSamplingFrequency(Control_p);
                        StartSPDIFPlayerIP(Control_p);

                        Control_p->spdifPlayerToProgram = Control_p->spdifPlayerPlayed;

                        for(i = Control_p->spdifPlayerToProgram; i < (Control_p->spdifPlayerToProgram + NUM_SPDIF_NODES_FOR_STARTUP); i++)
                        {
                            U32 Index = i % Control_p->spdifPlayerNumNode;
                            if (PlayerAudioBlock_p[Index].Valid)
                            {
                                PlayerAudioBlock_p[Index].PlayerDmaNode_p->SPDIFNode.NodeCompletePause = FALSE;
                            }
                            else
                            {
                                break;
                            }
                        }

                        /* Set PAUSE for last block in queue*/
                        PlayerAudioBlock_p[(i -1) % Control_p->spdifPlayerNumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = TRUE;

                        /* Set ToProgram to the last node in queue*/
                        Control_p->spdifPlayerToProgram = i - 1;

                        Control_p->TransferPlayerParams_p.NodeAddress_p = (STFDMA_GenericNode_t *)(U32)PlayerAudioBlock_p[Control_p->spdifPlayerPlayed % Control_p->spdifPlayerNumNode].PlayerDMANodePhy_p ;

#ifdef SPDIFP0_SUPPORTED
                        if(Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
                        {
                            STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_SOFTRESET, 0x1);
                        }else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
                        if(Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
                        {
                            STSYS_WriteRegDev32LE(HDMISPDIFPlayerBaseAddr + SPDIF_SOFTRESET, 0x1);
                        }else
#endif
                        {
                            /* Dummy {} to handle last else */
                        }

                        err = STFDMA_StartGenericTransfer(&(Control_p->TransferPlayerParams_p), &(Control_p->spdifPlayerTransferId));
                        if(err != ST_NO_ERROR)
                        {
                            Control_p->spdifPlayerCurrentState = SPDIFPLAYER_START;
                            STTBX_Print(("SPDIF:Start failure\n"));
                            STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);

                        }
                        else
                        {
                            Control_p->EOFBlock_Received = FALSE;
                            STTBX_Print(("SPDIF:Started\n"));

#if defined(ST_OS21)|| defined(ST_51XX)
#if defined (ENABLE_SPDIF_INTERRUPT)
#ifdef SPDIFP0_SUPPORTED
                            if (Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
                            {
                                STSYS_WriteRegDev32LE (SPDIFPlayerBaseAddr + SPDIF_INT_EN_SET, 0x1);// Enable underflow interrupt
                            }else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
                            if (Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
                            {
                                STSYS_WriteRegDev32LE (HDMISPDIFPlayerBaseAddr + SPDIF_INT_EN_SET, 0x1);// Enable underflow interrupt
                            }else
#endif
                            {
                                /* Dummy {} to handle last else */
                            }
                            Control_p->spdifForceRestart = FALSE;
                            if (Control_p->underFlowCount != 0)
                                STTBX_Print(("SPDIF:Received an underflow in playing condition\n"));
                            Control_p->underFlowCount = 0;
#endif
#endif

#ifdef SPDIFP0_SUPPORTED
                            if(Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
                            {
                                STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_SOFTRESET, 0x0);
                            } else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
                            if(Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
                            {
                                STSYS_WriteRegDev32LE(HDMISPDIFPlayerBaseAddr + SPDIF_SOFTRESET, 0x0);
                            }else
#endif
                            {
                                /* Dummy {} to handle last else */
                            }
                        }
                    }
                }

                STOS_MutexLock(Control_p->LockControlStructure);
                if (Control_p->spdifPlayerNextState == SPDIFPLAYER_STOPPED)
                {
                    Control_p->spdifPlayerCurrentState = Control_p->spdifPlayerNextState;
                    //STTBX_Print(("SPDIF:from SPDIFPLAYER_START\n"));

                    if (Control_p->spdifPlayerTransferId != 0)
                    {
                        // FDMA transfer is already started
                        STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);
                    }
                    else
                    {
                        // FDMA transfer was not started
                        SPDIFFreeBlocks(Control_p->spdifPlayerPlayed, Control_p->spdifPlayerToProgram, tempSPDIFPlayerControlBlock_p);

                        // Signal the command completion transtition
                        STTBX_Print(("SPDIF:Going to Stop from start\n"));
                        STOS_SemaphoreSignal(Control_p->spdifPlayerCmdTransitionSem_p);
                    }
                }
                STOS_MutexRelease(Control_p->LockControlStructure);
                break;

            case SPDIFPLAYER_RESTART:
                // Get number of buffers required for startup
                //STTBX_Print(("SPDIF Restart Played : %d\n",Control_p->spdifPlayerPlayed));
                for (i=Control_p->spdifPlayerPlayed; i < (Control_p->spdifPlayerPlayed + NUM_SPDIF_NODES_FOR_STARTUP); i++)
                {
                    U32 Index = i % Control_p->spdifPlayerNumNode;
                    if (PlayerAudioBlock_p[Index].Valid == FALSE)
                    {
#if MEMORY_SPLIT_SUPPORT
                        if (MemPool_GetSplitIpBlk(Control_p->memoryBlockManagerHandle,
                                    (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock),
                                    (U32)tempSPDIFPlayerControlBlock_p,
                                    & PlayerAudioBlock_p[Index].MemoryParams) != ST_NO_ERROR)
#else
                        if (MemPool_GetIpBlk(Control_p->memoryBlockManagerHandle,
                                    (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock),
                                    (U32)tempSPDIFPlayerControlBlock_p) != ST_NO_ERROR)
#endif
                        {
                            break;
                        }
                        else
                        {
#ifdef HDMISPDIFP0_SUPPORTED
                            if (HDMISPDIF_TestSampFreq(&PlayerAudioBlock_p[Index], tempSPDIFPlayerControlBlock_p) == TRUE)
                            {
#endif
                                if (SPDIFPlayer_StartupSync(&PlayerAudioBlock_p[Index],tempSPDIFPlayerControlBlock_p)== FALSE)
                                {
                                    break;
                                }
                                STOS_MutexLock(Control_p->LockControlStructure);
                                FillSPDIFPlayerFDMANode(&PlayerAudioBlock_p[Index], Control_p);
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
#ifdef HDMISPDIFP0_SUPPORTED
                            }
                            else
                            {
                                i--;
                            }
#endif
                        }
                    }
                    else
                    {
                        //STTBX_Print(("SPDIF Restart valid is set for %d\n",i));
                    }
                }

                // If all the required nodes are valid start the transfer
                if (PlayerAudioBlock_p[(Control_p->spdifPlayerPlayed + NUM_SPDIF_NODES_FOR_STARTUP - 1) % Control_p->spdifPlayerNumNode].Valid ||
                    Control_p->EOFBlock_Received)
                {
                    {
                        U32 BlockIndex = Control_p->spdifPlayerPlayed % Control_p->spdifPlayerNumNode;
                        MemoryBlock_t * MemoryBlock_p = PlayerAudioBlock_p[BlockIndex].memoryBlock;
#if MEMORY_SPLIT_SUPPORT
                        U32 Flags = PlayerAudioBlock_p[BlockIndex].MemoryParams.Flags;
#else
                        U32 Flags = MemoryBlock_p->Flags;
#endif
                        if ((Flags & FREQ_VALID) && (MemoryBlock_p->Data[FREQ_OFFSET]))
                        {
                            // Sampling frequency changed this should ideally occur only at the begining of playback
                            Control_p->samplingFrequency = MemoryBlock_p->Data[FREQ_OFFSET];
                            SetSPDIFPlayerSamplingFrequency(Control_p);
                            SetSPDIFChannelStatus(Control_p);
                        }
                    }

                    Control_p->spdifPlayerCurrentState = SPDIFPLAYER_PLAYING;

                    Control_p->spdifPlayerToProgram = Control_p->spdifPlayerPlayed;
                    for(i = Control_p->spdifPlayerToProgram; i < (Control_p->spdifPlayerToProgram + NUM_SPDIF_NODES_FOR_STARTUP); i++)
                    {
                        U32 Index = i % Control_p->spdifPlayerNumNode;
                        if (PlayerAudioBlock_p[Index].Valid)
                        {
                            PlayerAudioBlock_p[Index].PlayerDmaNode_p->SPDIFNode.NodeCompletePause = FALSE;
                        }
                        else
                        {
                            break;
                        }
                    }

                    /* Set PAUSE for last block in queue*/
                    PlayerAudioBlock_p[(i -1) % Control_p->spdifPlayerNumNode].PlayerDmaNode_p->Node.NodeControl.NodeCompletePause = TRUE;

                    /* Set ToProgram to the last node in queue*/
                    Control_p->spdifPlayerToProgram = i - 1;

                    /* Reset FDMA additional data region - Fixing SPDIF bit toggle(specially AAC type) */
#ifdef SPDIFP0_SUPPORTED
                    if(Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
                    {
                        STFDMA_SetAddDataRegionParameter(Control_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAMES_TO_GO, 0);
                        STFDMA_SetAddDataRegionParameter(Control_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAME_COUNT, 0);
                    }else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
                    /*Should be different region for hdmi spdif player*/
                    if(Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
                    {
                        STFDMA_SetAddDataRegionParameter(Control_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAMES_TO_GO, 0);
                        STFDMA_SetAddDataRegionParameter(Control_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAME_COUNT, 0);
                    }else
#endif
                    {
                        /* Dummy {} to handle last else */
                    }

                    //STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr+SPDIF_SOFTRESET, 0x1);
                    err = STFDMA_ResumeTransfer(Control_p->spdifPlayerTransferId);

                    if(err != ST_NO_ERROR)
                    {
                        Control_p->spdifPlayerCurrentState = SPDIFPLAYER_RESTART;
                        STTBX_Print(("SPDIF:ReStart failure\n"));
                        STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);

                    }
                    else
                    {
                        Control_p->EOFBlock_Received = FALSE;
                        STTBX_Print(("SPDIF:ReStarted\n"));
#if defined(ST_OS21) || defined(ST_51XX)
#if defined (ENABLE_SPDIF_INTERRUPT)
#ifdef SPDIFP0_SUPPORTED
                        if(Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
                        {
                            STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr+SPDIF_INT_EN_SET, 0x1);// Enable underflow interrupt
                        }else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
                        if(Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
                        {
                            STSYS_WriteRegDev32LE(HDMISPDIFPlayerBaseAddr+SPDIF_INT_EN_SET, 0x1);// Enable underflow interrupt
                        }else
#endif
                        {
                            /* Dummy {} to handle last else */
                        }

                        if (Control_p->underFlowCount != 0)
                        {
                            STTBX_Print(("SPDIF:Received an underflow in playing condition\n"));
                        }
                        Control_p->underFlowCount = 0;
#endif
                        Control_p->spdifForceRestart = FALSE;
#endif

                    }
                }

                STOS_MutexLock(Control_p->LockControlStructure);
                if (Control_p->spdifPlayerNextState == SPDIFPLAYER_STOPPED)
                {
                    Control_p->spdifPlayerCurrentState = Control_p->spdifPlayerNextState;
                    STTBX_Print(("SPDIF:from SPDIFPLAYER_RESTART\n"));
                    STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);
                    // Signal the command completion transtition
                    // Command completion will only be done through Abort Callback, if abort callback doesn't come we will be stuck forever
                    /*STOS_SemaphoreSignal(Control_p->spdifPlayerCmdTransitionSem_p);*/
                }
                STOS_MutexRelease(Control_p->LockControlStructure);

                break;

            case SPDIFPLAYER_PLAYING:
#if AVSYNC_ADDED
                if (PlayerAVsync_p->PTSValid)
                {

                    U32 tempPTS = 0;
                    AudSTCSync_t    * AudioSTCSync_p = &(Control_p->audioSTCSync);

                    tempPTS = time_minus(time_now(), PlayerAVsync_p->CurrentSystemTime);
                    tempPTS = TICKS_TO_PTS(tempPTS);

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                    ADDToEPTS(&(PlayerAVsync_p->CurrentPTS), tempPTS);
                    SPDIFPTSPCROffsetInms(Control_p, PlayerAVsync_p->CurrentPTS);
#endif

                    if (AudioSTCSync_p->SkipPause != 0)
                    {
                        UpdateSPDIFSynced(Control_p);
                        SPDIFPlayer_NotifyEvt(STAUD_AVSYNC_SKIP_EVT, AudioSTCSync_p->SkipPause, Control_p);
                        SPDIFPlayer_NotifyEvt(STAUD_OUTOF_SYNC, 0, Control_p);
                        PlayerAVsync_p->SyncCompleted = FALSE;
                        PlayerAVsync_p->InSyncCount = 0;

                    }

                    if (AudioSTCSync_p->PauseSkip != 0)
                    {
                        UpdateSPDIFSynced(Control_p);
                        SPDIFPlayer_NotifyEvt(STAUD_AVSYNC_PAUSE_EVT, AudioSTCSync_p->PauseSkip, Control_p);
                        SPDIFPlayer_NotifyEvt(STAUD_OUTOF_SYNC, 0, Control_p);
                        PlayerAVsync_p->SyncCompleted = FALSE;
                        PlayerAVsync_p->InSyncCount = 0;
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
                            SPDIFPlayer_NotifyEvt(STAUD_IN_SYNC,0,Control_p);
                            //STTBX_Print(("SPDIF Sync completed for Live decode\n"));
                        }

                    }

                    STOS_MutexLock(Control_p->LockControlStructure);
                    PlayerAVsync_p->PTSValid = FALSE;
                    STOS_MutexRelease(Control_p->LockControlStructure);
                }

#endif
                STOS_MutexLock(Control_p->LockControlStructure);
                if (Control_p->spdifPlayerNextState == SPDIFPLAYER_STOPPED)
                {
                    Control_p->spdifPlayerCurrentState = Control_p->spdifPlayerNextState;
                    //STTBX_Print(("SPDIF:from SPDIFPLAYER_PLAYING\n"));
                    STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);
                    // Signal the command completion transtition
                    // command completion transition is moved to abort completed callback
                    //STOS_SemaphoreSignal(Control_p->spdifPlayerCmdTransitionSem_p);
                }
                else
                {
                    if(Control_p->spdifRestart)
                    {
                        Control_p->spdifRestart = FALSE;
                        ResetSPDIFPlayerSyncParams(Control_p);
                        Control_p->spdifPlayerCurrentState = SPDIFPLAYER_RESTART;
                        STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);
                        //STTBX_Print(("SPDIF:from PLAYING to RESTART\n"));
                    }
                }
                STOS_MutexRelease(Control_p->LockControlStructure);

                break;
            case SPDIFPLAYER_STOPPED:
                if ((Control_p->spdifPlayerTransferId != 0) && (!Control_p->AbortSent))
                {
                    // Abort currrently running transfer

                    err =  STFDMA_AbortTransfer(Control_p->spdifPlayerTransferId);
                    if(err)
                    {
                        STTBX_Print(("STFDMA_AbortTransfer SPDIFPlayer Failure=%x\n",err));
                    }
                    else
                    {
                        Control_p->AbortSent = TRUE;
                        STTBX_Print(("Sending spdif player abort \n"));
                        Control_p->FDMAAbortSent++;
                    }
                }
                else
                {
                    STOS_MutexLock(Control_p->LockControlStructure);
                    if (Control_p->spdifPlayerNextState == SPDIFPLAYER_START ||
                        Control_p->spdifPlayerNextState == SPDIFPLAYER_TERMINATE)
                    {
                        ResetSPDIFPlayerSyncParams(Control_p);
                        Control_p->spdifPlayerPlayed = 0;
                        Control_p->CompressedDataAlignment = LE; /*reset alignment to LE*/
                        SetSPDIFStreamParams(Control_p);
                        Control_p->AbortSent = FALSE;
                        Control_p->spdifPlayerCurrentState = Control_p->spdifPlayerNextState;
                        STTBX_Print(("SPDIF:from SPDIFPLAYER_STOPPED\n"));
                        STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);
                        // Signal the command completion transtition
                        STOS_SemaphoreSignal(Control_p->spdifPlayerCmdTransitionSem_p);
                    }
                    STOS_MutexRelease(Control_p->LockControlStructure);
                }

                break;
            case SPDIFPLAYER_TERMINATE:
                // clean up all the memory allocations
                if (Control_p->spdifPlayerTransferId != 0)
                {
                    STTBX_Print((" SPDIF player task terminated with FDMA runing \n"));
                }
                for (i=Control_p->spdifPlayerPlayed; i < (Control_p->spdifPlayerPlayed + Control_p->spdifPlayerNumNode); i++)
                {
                    U32 Index = i % Control_p->spdifPlayerNumNode;
                    if (PlayerAudioBlock_p[Index].Valid == TRUE)
                    {
                        //STTBX_Print(("Freeing MBlock in SPDIF terminate \n"));
                        if (MemPool_FreeIpBlk(Control_p->memoryBlockManagerHandle, (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock), (U32)tempSPDIFPlayerControlBlock_p) != ST_NO_ERROR)
                        {
                            STTBX_Print(("Error in releasing Ip spdif player mem block\n"));
                        }

                        PlayerAudioBlock_p[Index].Valid = FALSE;

                    }
                }
                if (STAud_SPDIFPlayerTerminate((STSPDIFPLAYER_Handle_t)tempSPDIFPlayerControlBlock_p) != ST_NO_ERROR)
                {
                    STTBX_Print(("Error in spdif player terminate\n"));
                }

                // Exit player task
                    STOS_TaskExit(tempSPDIFPlayerControlBlock_p);
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


void SPDIFPlayerFDMACallbackFunction(U32 TransferId,U32 CallbackReason,U32 *CurrentNode_p,U32 NodeBytesRemaining,BOOL Error,void *ApplicationData_p, clock_t  InterruptTime )
{

    SPDIFPlayerControlBlock_t * tempSPDIFPlayerControlBlock_p = (SPDIFPlayerControlBlock_t *)ApplicationData_p;
    SPDIFPlayerControl_t * Control_p = &(tempSPDIFPlayerControlBlock_p->spdifPlayerControl);
    PlayerAudioBlock_t  * PlayerAudioBlock_p    = Control_p->PlayerAudioBlock_p;
    AudAVSync_t         * PlayerAVsync_p = &(Control_p->audioAVsync);
    U32 Played=0,ToProgram=0, Count = 0, i=0;
    U8 Index = 0;
// ST_ErrorCode_t err = ST_NO_ERROR;
    U32 SPDIFPlayerBaseAddr,HDMISPDIFPlayerBaseAddr;

    SPDIFPlayerBaseAddr = Control_p->BaseAddress.SPDIFPlayerBaseAddr;
    HDMISPDIFPlayerBaseAddr = Control_p->BaseAddress.HDMISPDIFPlayerBaseAddr;

    if(Error)
    {
        STTBX_Print(("SPDIFP:Err:%d:TrfId:%u,ActualId:%u\n",Error,TransferId,Control_p->spdifPlayerTransferId));
        STTBX_Print(("SPDIFP:Call State:%d,Pause:%d\n",CallbackReason,PlayerAudioBlock_p[Control_p->spdifPlayerPlayed % Control_p->spdifPlayerNumNode].PlayerDmaNode_p->SPDIFNode.NodeCompletePause));
    }

    if(TransferId == Control_p->spdifPlayerTransferId && TransferId)
    {

        // Lock the Block
        STOS_MutexLock(Control_p->LockControlStructure);

        Played = Control_p->spdifPlayerPlayed;
        ToProgram = Control_p->spdifPlayerToProgram;

        if(Played%500 == 499)
        {
            STTBX_Print(("SPDIF:%d\n",Played));
        }

        switch(CallbackReason)
        {
            case STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED:
                /*Error Recovery*/

                while(!((U32)(PlayerAudioBlock_p[Count % Control_p->spdifPlayerNumNode].PlayerDMANodePhy_p) == ((U32)CurrentNode_p )) && (Count < Control_p->spdifPlayerNumNode))
                {
                    Count++;
                }

                Index = Played % Control_p->spdifPlayerNumNode;
                while(Index != Count)
                {
                    // Release the Block
                    if (MemPool_FreeIpBlk(Control_p->memoryBlockManagerHandle, (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock), (U32)tempSPDIFPlayerControlBlock_p) != ST_NO_ERROR)
                    {
                        STTBX_Print(("Error in releasing SPDIF player mem blcok\n"));
                    }
                    else
                    {
                        STTBX_Print(("Freed SPDIF block in callbacl \n"));
                    }


                    // Block is consumed and ready to be filled
                    PlayerAudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/

                    Played ++;
                    Index = Played % Control_p->spdifPlayerNumNode;
                }


                // This is to free the block on which the pause bit is set
                // Release the Block
                if (MemPool_FreeIpBlk(Control_p->memoryBlockManagerHandle, (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock), (U32)tempSPDIFPlayerControlBlock_p) != ST_NO_ERROR)
                {
                    STTBX_Print(("Error in releasing SPDIF player mem blcok\n"));
                }

                // Block is consumed and ready to be filled
                PlayerAudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/

                Count ++;

                STTBX_Print(("SPDIF:Setting restart player: Played:%d\n", Played));
                // Put counters at last stop status
                Played = Count;
                ToProgram = Count;

                if(Control_p->EOFBlock_Received)
                {
                    Control_p->EOFBlock_Received = FALSE;
                }


                Control_p->spdifRestart = TRUE;
                //STTBX_Print(("Setting restart player: Played:%d\n", Played));

                STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);

                break;

            case STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING:
                if(Error)
                {

                    while(!((U32)(PlayerAudioBlock_p[Count % Control_p->spdifPlayerNumNode].PlayerDMANodePhy_p) == ((U32)CurrentNode_p )) && (Count < Control_p->spdifPlayerNumNode))
                    {
                        Count++;
                    }

                    Index =  Played % Control_p->spdifPlayerNumNode;
                    while(Index != Count)
                    {

                        // Release the Block
                        if (MemPool_FreeIpBlk(Control_p->memoryBlockManagerHandle, (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock), (U32)tempSPDIFPlayerControlBlock_p) != ST_NO_ERROR)
                        {
                            STTBX_Print(("error in freeing spdif player mem block %d\n", Played));
                        }

                        // Block is consumed and ready to be filled
                        PlayerAudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/
                        Played ++;
                        Index =  Played % Control_p->spdifPlayerNumNode;
                    }

                    //don't release the current block as it is playing

                }
                else
                {
                    Index =  Played % Control_p->spdifPlayerNumNode;

                    // Release the Block
                    if (MemPool_FreeIpBlk(Control_p->memoryBlockManagerHandle, (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock), (U32)tempSPDIFPlayerControlBlock_p) != ST_NO_ERROR)
                    {
                        STTBX_Print(("Error in spdif player relasing memory blcok\n"));
                    }

                    // Block is consumed and ready to be filled
                    PlayerAudioBlock_p[Index].Valid = FALSE; /*Block is consumed and ready to be filled*/

                    Played ++;
                }
#if defined( ST_OSLINUX )
#if defined (ENABLE_SPDIF_INTERRUPT)
                {
                    U32 Addr;
                    Addr = SPDIFPlayerBaseAddr + 0x008;
                    Spdif_InterruptValue = *(U32*)Addr;
                    Addr = SPDIFPlayerBaseAddr+ 0x010;
                    Spdif_InterruptEnable=*(U32*)Addr;
                }
#endif
#endif //ST_OSLINUX
#if AVSYNC_ADDED
                Index = Played % Control_p->spdifPlayerNumNode;

                if(PlayerAudioBlock_p[Index].memoryBlock)
                {
                    MemoryBlock_t * MemoryBlock_p = PlayerAudioBlock_p[Index].memoryBlock;
#if MEMORY_SPLIT_SUPPORT
                    U32 Flags = PlayerAudioBlock_p[Index].MemoryParams.Flags;
#else
                    U32 Flags = MemoryBlock_p->Flags;
#endif
                    if ((Flags & FREQ_VALID) && (Control_p->samplingFrequency != MemoryBlock_p->Data[FREQ_OFFSET]))
                    {
                        // Sampling frequency changed this should ideally occur only at the begining of playback
                        Control_p->samplingFrequency = MemoryBlock_p->Data[FREQ_OFFSET];
                        SetSPDIFPlayerSamplingFrequency(Control_p);
                        SetSPDIFChannelStatus(Control_p);
                    }

                    /*Don't do sync unless you have done all the required skip pause*/
                    if((Flags & PTS_VALID) && (Control_p->audioSTCSync.Synced == 0)
                        && (PlayerAVsync_p->PTSValid == FALSE))
                    {
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                        PlayerAVsync_p->CurrentPTS.BaseValue = MemoryBlock_p->Data[PTS_OFFSET];
                        PlayerAVsync_p->CurrentPTS.BaseBit32  = MemoryBlock_p->Data[PTS33_OFFSET];
#endif

#if defined( ST_OSLINUX )
                        PlayerAVsync_p->CurrentSystemTime = InterruptTime;
#else
                        PlayerAVsync_p->CurrentSystemTime = time_now();
#endif
#if ENABLE_SYNC_CALC_IN_CALLBACK
                        CalculateSPDIFSync(Control_p);
#else
                        PlayerAVsync_p->PTSValid            = TRUE;
                        STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);
#endif
                    }
                    else
                    {
                        if(Control_p->audioSTCSync.Synced)
                        {
                            Control_p->audioSTCSync.Synced--;
                        }
                        //STTBX_Print(("SPDIF:Synced %d\n",Control_p->audioSTCSync.Synced));
                    }
                }
#endif

                Index = (Played + 1)%Control_p->spdifPlayerNumNode;
                if((PlayerAudioBlock_p[Index].Valid == TRUE) && ((U32)PlayerAudioBlock_p[Index].PlayerDMANodePhy_p!= ((U32)CurrentNode_p )))
                {
                    UpdateSPDIFPlayerNodeSize(&PlayerAudioBlock_p[Index], Control_p);
                    //STTBX_Print(("SPDIF: update %d\n",Index));
                }

                if (Control_p->spdifForceRestart == FALSE)
                {
                    if((Played < ToProgram) && !Control_p->EOFBlock_Received)
                    {
                        for(i = ToProgram; i < (Played + (NUM_SPDIF_NODES_FOR_STARTUP - 1)); i++)
                        {
                            Index = (i + 1) % Control_p->spdifPlayerNumNode;
                            if(PlayerAudioBlock_p[Index].Valid == FALSE)
                            {
                                // Try to get next block
#if MEMORY_SPLIT_SUPPORT
                                if (MemPool_GetSplitIpBlk(Control_p->memoryBlockManagerHandle,
                                        (U32 *)(&PlayerAudioBlock_p[Index].memoryBlock),
                                        (U32)tempSPDIFPlayerControlBlock_p,
                                        &PlayerAudioBlock_p[Index].MemoryParams) != ST_NO_ERROR)
#else
                                if (MemPool_GetIpBlk(Control_p->memoryBlockManagerHandle,
                                        (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock),
                                        (U32)tempSPDIFPlayerControlBlock_p) != ST_NO_ERROR)
#endif
                                {
                                    break;
                                }
                                else
                                {
                                    // Got next block..

#ifdef HDMISPDIFP0_SUPPORTED
                                    if(HDMISPDIF_TestSampFreq(&PlayerAudioBlock_p[Index] , tempSPDIFPlayerControlBlock_p) == FALSE)
                                    {
                                        break;/*The block has been freed if run time freq change for ddplus*/
                                    }

#endif
#if MEMORY_SPLIT_SUPPORT
                                    U32 Flags = PlayerAudioBlock_p[Index].MemoryParams.Flags;
#else
                                    U32 Flags = PlayerAudioBlock_p[Index].memoryBlock->Flags;
#endif
                                    if(Flags & DUMMYBLOCK_VALID)
                                    {
                                        if(Flags & EOF_VALID)
                                        {
                                            Control_p->EOFBlock_Received = TRUE;
                                        }
                                        //Free this block
                                        if(MemPool_FreeIpBlk(Control_p->memoryBlockManagerHandle, (U32 *)(& PlayerAudioBlock_p[Index].memoryBlock), (U32)tempSPDIFPlayerControlBlock_p)!=ST_NO_ERROR)
                                        {
                                            STTBX_Print(("1:Err in spdif player releasing memory block\n"));
                                        }
                                    }
                                    else
                                    {
                                        FillSPDIFPlayerFDMANode(&PlayerAudioBlock_p[Index], Control_p);

                                        PlayerAudioBlock_p[Index].Valid = TRUE;
                                    }
                                }
                            }

                            if(PlayerAudioBlock_p[Index].Valid)
                            {
                                PlayerAudioBlock_p[Index].PlayerDmaNode_p->SPDIFNode.NodeCompletePause = TRUE;
                                PlayerAudioBlock_p[i % Control_p->spdifPlayerNumNode].PlayerDmaNode_p->SPDIFNode.NodeCompletePause = FALSE;

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
                STTBX_Print(("SPDIF Aborted\n"));

                Control_p->FDMAAbortCBRecvd++;
                SPDIFFreeBlocks(Played, ToProgram, tempSPDIFPlayerControlBlock_p);

                Played = 0;
                ToProgram = 0;

                /* Stoping SPDIF player IP*/
                StopSPDIFPlayerIP(Control_p);

                // Signal Stop command completion for SPDIF player
                STOS_SemaphoreSignal(Control_p->spdifPlayerCmdTransitionSem_p);
                break;

            default:
                break;
        }

        Control_p->spdifPlayerPlayed = Played;
        Control_p->spdifPlayerToProgram = ToProgram;

        // Release the Block
        STOS_MutexRelease(Control_p->LockControlStructure);

    }
    else
    {
        STTBX_Print(("SPDIFP:mismatch:TrfId:%u,ActualId:%u\n",TransferId,Control_p->spdifPlayerTransferId));
        STTBX_Print(("SPDIFP:Call State:%d\n",CallbackReason));
    }
}

void SPDIFFreeBlocks(U32 Played, U32 ToProgram, SPDIFPlayerControlBlock_t * tempSPDIFPlayerControlBlock_p)
{
    SPDIFPlayerControl_t * Control_p = &(tempSPDIFPlayerControlBlock_p->spdifPlayerControl);
    PlayerAudioBlock_t  * PlayerAudioBlock_p    = Control_p->PlayerAudioBlock_p;
    ST_ErrorCode_t err = ST_NO_ERROR;
    U32 i = 0;

    //STTBX_Print(("SPDIF player aborted \n"));
    while(Played <= ToProgram)
    {
        if (PlayerAudioBlock_p[Played % Control_p->spdifPlayerNumNode].Valid)
        {
            // This condition should always be true in this case
            // Release the Block
            MemPool_FreeIpBlk(Control_p->memoryBlockManagerHandle, (U32 *)(& PlayerAudioBlock_p[Played % Control_p->spdifPlayerNumNode].memoryBlock), (U32)tempSPDIFPlayerControlBlock_p);
            // Block is consumed and ready to be filled
            PlayerAudioBlock_p[Played % Control_p->spdifPlayerNumNode].Valid = FALSE; /*Block is consumed and ready to be filled*/
        }

        Played ++;
    }
    for (i=0; i<Control_p->spdifPlayerNumNode ;i++)
        PlayerAudioBlock_p[i].Valid = FALSE; /*Block is consumed and ready to be filled*/


    // reset the trasnsfer id to 0
    Control_p->spdifPlayerTransferId = 0;

    // To move state from stop to start/terminate in case command is given before abort is completed
    //STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);

    // Unregister consumer after stop is completed
    err = MemPool_UnRegConsumer(Control_p->memoryBlockManagerHandle, (U32)(tempSPDIFPlayerControlBlock_p));
    if (err != ST_NO_ERROR)
    {
        STTBX_Print(("Error in unregistering BM from spdif player\n"));
    }
    else
    {
        //STTBX_Print(("Unregistered BM from spdif player\n"));
    }
}

ST_ErrorCode_t      STAud_SPDIFPlayerSetStreamParams(STSPDIFPLAYER_Handle_t handle, STAUD_StreamParams_t * StreamParams_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock, * lastSPDIFPlayerControlBlock_p = spdifPlayerControlBlock;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        lastSPDIFPlayerControlBlock_p = tempSPDIFplayerControlBlock_p;
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);
    /*change the next state*/
    STOS_MutexLock(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);
    tempSPDIFplayerControlBlock_p->spdifPlayerControl.StreamParams = *(StreamParams_p);
    SetSPDIFStreamParams(&(tempSPDIFplayerControlBlock_p->spdifPlayerControl));
    STOS_MutexRelease(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);

    return (Error);

}

ST_ErrorCode_t      STAud_SPDIFPlayerGetEncOutput(STSPDIFPLAYER_Handle_t handle, BOOL   * EnableEncOutput)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock;
    SPDIFPlayerControl_t        * Control_p;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p       = &(tempSPDIFplayerControlBlock_p->spdifPlayerControl);

    STOS_MutexLock(Control_p->LockControlStructure);
    //STTBX_Print(("Geting enc ouput on spdif to %d\n",(U32)Control_p->EnableEncOutput));
    *EnableEncOutput = Control_p->EnableEncOutput;
    STOS_MutexRelease(Control_p->LockControlStructure);

    return (Error);
}

ST_ErrorCode_t      STAud_SPDIFPlayerSetEncOutput(STSPDIFPLAYER_Handle_t handle, BOOL   EnableEncOutput)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock;
    SPDIFPlayerControl_t        * Control_p;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p       = &(tempSPDIFplayerControlBlock_p->spdifPlayerControl);

    STOS_MutexLock(Control_p->LockControlStructure);
    if (Control_p->spdifPlayerCurrentState == SPDIFPLAYER_INIT ||
        Control_p->spdifPlayerCurrentState == SPDIFPLAYER_STOPPED)
    {
        //STTBX_Print(("Setting enc ouput on spdif to %d\n",(U32)EnableEncOutput));
        Control_p->EnableEncOutput = EnableEncOutput;
    }
    else
    {
        //STTBX_Print(("Invalid state for changing SPDIF enc output\n"));
        Error = STAUD_ERROR_INVALID_STATE;
    }
    STOS_MutexRelease(Control_p->LockControlStructure);

    return (Error);
}

ST_ErrorCode_t      STAud_SPDIFPlayerSetSPDIFMode(STSPDIFPLAYER_Handle_t handle, STAUD_DigitalMode_t    SPDIFMode)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock;
    SPDIFPlayerControl_t        * Control_p;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p       = &(tempSPDIFplayerControlBlock_p->spdifPlayerControl);

    STOS_MutexLock(Control_p->LockControlStructure);
    if (Control_p->spdifPlayerCurrentState == SPDIFPLAYER_INIT ||
        Control_p->spdifPlayerCurrentState == SPDIFPLAYER_STOPPED)
    {
        //STTBX_Print(("Setting spdif to %d\n",(U32)SPDIFMode));
        Control_p->SPDIFMode = SPDIFMode;
    }
    else
    {
        //STTBX_Print(("Invalid state for changing SPDIF mode\n"));
        Error = STAUD_ERROR_INVALID_STATE;
    }
    STOS_MutexRelease(Control_p->LockControlStructure);

    return (Error);
}

ST_ErrorCode_t      STAud_SPDIFPlayerGetSPDIFMode(STSPDIFPLAYER_Handle_t handle, STAUD_DigitalMode_t    * SPDIFMode)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    STOS_MutexLock(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);
    *SPDIFMode = tempSPDIFplayerControlBlock_p->spdifPlayerControl.SPDIFMode;
    STOS_MutexRelease(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);

    return (Error);
}
ST_ErrorCode_t  SPDIFPlayerCheckStateTransition(SPDIFPlayerControl_t * Control_p,SPDIFPlayerState_t State)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    SPDIFPlayerState_t SPDIFPlayerCurrentState = Control_p->spdifPlayerCurrentState;

    switch (State)
    {
        case SPDIFPLAYER_START:
            if ((SPDIFPlayerCurrentState != SPDIFPLAYER_INIT) &&
                (SPDIFPlayerCurrentState != SPDIFPLAYER_STOPPED))
            {
                Error = STAUD_ERROR_INVALID_STATE;
            }
            break;

        case SPDIFPLAYER_STOPPED:
            if ((SPDIFPlayerCurrentState != SPDIFPLAYER_START)&&
                (SPDIFPlayerCurrentState != SPDIFPLAYER_RESTART)&&
                (SPDIFPlayerCurrentState != SPDIFPLAYER_PLAYING))
            {
                Error = STAUD_ERROR_INVALID_STATE;
            }
            break;

        case SPDIFPLAYER_TERMINATE:
            if ((SPDIFPlayerCurrentState != SPDIFPLAYER_INIT) &&
                (SPDIFPlayerCurrentState != SPDIFPLAYER_STOPPED))
            {
                Error = STAUD_ERROR_INVALID_STATE;
            }
            break;
        default:
            break;
    }
    return(Error);

}
ST_ErrorCode_t      STAud_SPDIFPlayerSetCmd(STSPDIFPLAYER_Handle_t handle,SPDIFPlayerState_t State)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock, * lastSPDIFPlayerControlBlock_p = spdifPlayerControlBlock;
    SPDIFPlayerControl_t        * Control_p;
    task_t  * spdifPlayerTask_p = NULL;
    ConsumerParams_t consumerParam_s;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        lastSPDIFPlayerControlBlock_p = tempSPDIFplayerControlBlock_p;
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Control_p = &(tempSPDIFplayerControlBlock_p->spdifPlayerControl);

    Error = SPDIFPlayerCheckStateTransition(Control_p, State);
    if ( Error != ST_NO_ERROR)
    {
        return (Error);
    }

    if (State == SPDIFPLAYER_TERMINATE)
    {
        spdifPlayerTask_p = Control_p->spdifPlayerTask_p;
    }

    if (State == SPDIFPLAYER_START)
    {
        consumerParam_s.Handle = (U32)tempSPDIFplayerControlBlock_p;
        consumerParam_s.sem    = Control_p->spdifPlayerTaskSem_p;

        // Default value should work for most of the cases
        Control_p->bytesPerChannel    = BYTES_PER_CHANNEL_IN_PLAYER;
        Control_p->shiftPerChannel    = SHIFTS_PER_CHANNEL;
        Control_p->shiftForTwoChannel = SHIFTS_FOR_TWO_CHANNEL;

        switch (Control_p->SPDIFMode)
        {
        case STAUD_DIGITAL_MODE_NONCOMPRESSED:
            Control_p->memoryBlockManagerHandle = Control_p->pcmMemoryBlockManagerHandle;
            Control_p->PlayerCurrentMemoryAddress = Control_p->PlayerInputMemoryAddressPCM;

            switch(Control_p->spdifPlayerIdentifier)
            {
#ifdef HDMISPDIFP0_SUPPORTED
            case HDMI_SPDIF_PLAYER_0:
            if(Control_p->HDMI_SPDIFPlayerEnable== TRUE)
            {
                // Register to memory block manager as a consumer
                Error |= MemPool_RegConsumer(Control_p->memoryBlockManagerHandle, consumerParam_s);
            }
            break;
#endif
            default:
                // Register to memory block manager as a consumer
                Error |= MemPool_RegConsumer(Control_p->memoryBlockManagerHandle, consumerParam_s);
            break;
            }

            break;
        case STAUD_DIGITAL_MODE_COMPRESSED:
            if(!Control_p->BypassMode)
                {
                    if (Control_p->EnableEncOutput == FALSE)
                    {

#ifdef SPDIFP0_SUPPORTED
                        if (Control_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DDPLUS
                                   && Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
                        {
                            Control_p->memoryBlockManagerHandle = Control_p->compressedMemoryBlockManagerHandle1;
                            Control_p->PlayerCurrentMemoryAddress = Control_p->PlayerInputMemoryAddressCom1;
                        }else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
                        /*HDMI spdif player output compressed ddplus spdif: no transcoding to dolby digital*/
                        if (Control_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DDPLUS
                                   && Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
                        {
                            Control_p->memoryBlockManagerHandle = Control_p->compressedMemoryBlockManagerHandle0;
                            Control_p->PlayerCurrentMemoryAddress = Control_p->PlayerInputMemoryAddressCom0;
                        }else
#endif
                        {
                            /*Case when stream type is not STAUD_STREAM_CONTENT_DDPLUS*/
                            Control_p->memoryBlockManagerHandle = Control_p->compressedMemoryBlockManagerHandle0;
                            Control_p->PlayerCurrentMemoryAddress = Control_p->PlayerInputMemoryAddressCom0;
                        }

                    }
                    else
                    {
#ifdef ENABLE_TRANSCODED_DTS_IN_PCM_MODE
                        if (Control_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DTS)
                        {
                        Control_p->bytesPerChannel    = 2;
                        Control_p->shiftPerChannel    = 1;
                        Control_p->shiftForTwoChannel = 2;
                        }
#endif
                        Control_p->memoryBlockManagerHandle = Control_p->compressedMemoryBlockManagerHandle2;
                        Control_p->PlayerCurrentMemoryAddress = Control_p->PlayerInputMemoryAddressCom2;
                    }
                    switch(Control_p->spdifPlayerIdentifier)
                    {
#ifdef HDMISPDIFP0_SUPPORTED
                    case HDMI_SPDIF_PLAYER_0:
                    if(Control_p->HDMI_SPDIFPlayerEnable== TRUE)
                    {
                        // Register to memory block manager as a consumer
                        Error |= MemPool_RegConsumer(Control_p->memoryBlockManagerHandle, consumerParam_s);
                    }
                    break;
#endif
                    default:
                        // Register to memory block manager as a consumer
                        Error |= MemPool_RegConsumer(Control_p->memoryBlockManagerHandle, consumerParam_s);
                    break;
                    }
                }

            break;
        default:
            break;
        }
    }

    /*change the next state*/
    STOS_MutexLock(Control_p->LockControlStructure);

#ifdef ST_OS21
#ifdef STTBX_PRINT
    {
        U32 Count __attribute__ ((unused))= semaphore_value(tempSPDIFplayerControlBlock_p->spdifPlayerControl.spdifPlayerCmdTransitionSem_p);

        STTBX_Print(("SPDIFP:CMD:Semcount %d\n",Count));
    }
#endif
#endif

    Control_p->spdifPlayerNextState = State;
    STOS_MutexRelease(Control_p->LockControlStructure);

    /*signal the task to change its state*/
    STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);

    if (State == SPDIFPLAYER_TERMINATE)
    {
        STOS_TaskWait(&spdifPlayerTask_p, TIMEOUT_INFINITY);
#if defined (ST_OS20)
        STOS_TaskDelete(spdifPlayerTask_p,Control_p->CPUPartition,(void *)Control_p->spdifPlayerTask_pstack,Control_p->CPUPartition);
#else
        STOS_TaskDelete(spdifPlayerTask_p,NULL,NULL,NULL);
#endif
    }
    else
    {
        // Wait for command completion transtition
        STOS_SemaphoreWait(Control_p->spdifPlayerCmdTransitionSem_p);
        //STTBX_Print(("Wait for command completion done\n"));
        if (State == SPDIFPLAYER_STOPPED)
        {
            STTBX_Print(("SPDIFP:Calling UNReg\n"));
            MemPool_UnRegConsumer(Control_p->memoryBlockManagerHandle, (U32)tempSPDIFplayerControlBlock_p);
            //STTBX_Print(("MemPool_UnRegConsumer Done\n"));
        }
    }

    return Error;
}

void    UpdateSPDIFPlayerNodeSize(PlayerAudioBlock_t * PlayerAudioBlock_p, SPDIFPlayerControl_t * Control_p)
{
#if AVSYNC_ADDED
    STFDMA_GenericNode_t    * PlayerDmaNode_p = PlayerAudioBlock_p->PlayerDmaNode_p;

    //STTBX_Print(("SPDIF:1Update Size %d\n",PlayerDmaNode_p->SPDIFNode.NumberBytes));

    if(Control_p->SPDIFMode == STAUD_DIGITAL_MODE_NONCOMPRESSED
#ifdef ENABLE_TRANSCODED_DTS_IN_PCM_MODE
        || ((Control_p->EnableEncOutput == TRUE) && Control_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DTS)
#endif
    )
    {
        //Control_p->CurrentBurstPeriod = (PlayerDmaNode_p->SPDIFNode.NumberBytes / (BYTES_PER_CHANNEL_IN_PLAYER*2));

        //Data is always two channels for SPDIF node
        Control_p->CurrentBurstPeriod = (PlayerDmaNode_p->SPDIFNode.NumberBytes >> Control_p->shiftForTwoChannel);
    }

    if (Control_p->skipDuration && (Control_p->pauseDuration == 0))
    {
        U32 SamplesToSkip           = ((Control_p->skipDuration * Control_p->samplingFrequency)/1000);
        //U32 SampleSize                = (U32)(1 << SHIFTS_FOR_TWO_CHANNEL); //Only two channel
        U32 SamplesAvailableToSkip  = 0;
        U32 SamplesToGo             = 0;

        if(Control_p->CurrentBurstPeriod > 192)
        {
            SamplesAvailableToSkip  = Control_p->CurrentBurstPeriod - 192;
        }

        STTBX_Print(("S:%d,%d\n",SamplesToSkip,SamplesAvailableToSkip));
        if(SamplesToSkip > SamplesAvailableToSkip)
        {
            Control_p->skipDuration -= ((SamplesAvailableToSkip * 1000) / Control_p->samplingFrequency);
            SamplesToGo = 192;
        }
        else
        {
            Control_p->skipDuration = 0;
            SamplesToSkip = ((SamplesToSkip / 192) * 192);
            SamplesToGo = Control_p->CurrentBurstPeriod - SamplesToSkip;
        }

        switch (Control_p->SPDIFMode)
        {
            case STAUD_DIGITAL_MODE_NONCOMPRESSED:

                //PlayerDmaNode_p->SPDIFNode.NumberBytes = SamplesToGo * SampleSize;
                PlayerDmaNode_p->SPDIFNode.NumberBytes = SamplesToGo << SHIFTS_FOR_TWO_CHANNEL;
                break;
            case STAUD_DIGITAL_MODE_COMPRESSED:
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
                PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.BurstPeriod = SamplesToGo;
                if (PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.BurstPeriod <= (PlayerDmaNode_p->SPDIFNode.NumberBytes + 8))
                {
                    PlayerDmaNode_p->SPDIFNode.NumberBytes = PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.BurstPeriod - 8;
                }
#else

#ifdef ENABLE_TRANSCODED_DTS_IN_PCM_MODE
                if (Control_p->EnableEncOutput == TRUE && Control_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DTS)
                {
                    // Contact RB or KM to understand Why ???
                    PlayerDmaNode_p->SPDIFNode.NumberBytes = SamplesToGo << Control_p->shiftForTwoChannel;
                }
                else
#endif
                {
                    PlayerDmaNode_p->SPDIFNode.BurstPeriod = SamplesToGo;
                    if (PlayerDmaNode_p->SPDIFNode.BurstPeriod <= (PlayerDmaNode_p->SPDIFNode.NumberBytes + 8))
                    {
                        PlayerDmaNode_p->SPDIFNode.NumberBytes = PlayerDmaNode_p->SPDIFNode.BurstPeriod - 8;
                    }
                }
#endif
                break;
            default:
                break;
        }
    }

    if (Control_p->pauseDuration && (Control_p->skipDuration == 0))
    {
        U32 SamplesToPause          = ((Control_p->pauseDuration * Control_p->samplingFrequency)/1000);
        //U32 SampleSize                    = (U32)(1 << SHIFTS_FOR_TWO_CHANNEL); //Only two channel
        //U32 DummyBufferSampleSize = (Control_p->dummyBufferSize / SampleSize);
        U32 DummyBufferSampleSize   = (Control_p->dummyBufferSize >> Control_p->shiftForTwoChannel);
        U32 SamplesForPause         = 0;
        U32 SamplesToGo             = 0;

        if(DummyBufferSampleSize > Control_p->CurrentBurstPeriod)
        {
            SamplesForPause = DummyBufferSampleSize - Control_p->CurrentBurstPeriod;
        }

        STTBX_Print(("P:%d,%d,%d\n",SamplesToPause,SamplesForPause,DummyBufferSampleSize));
        if(SamplesToPause > SamplesForPause)
        {
            Control_p->pauseDuration -= ((SamplesForPause*1000) / Control_p->samplingFrequency);
            SamplesToGo = DummyBufferSampleSize;
        }
        else
        {
            Control_p->pauseDuration = 0;
            SamplesToPause = ((SamplesToPause / 192) * 192);
            SamplesToGo = Control_p->CurrentBurstPeriod + SamplesToPause;
        }

        switch (Control_p->SPDIFMode)
        {
            case STAUD_DIGITAL_MODE_NONCOMPRESSED:

                PlayerDmaNode_p->SPDIFNode.SourceAddress_p      = (void *)((U32)Control_p->DummyMemoryAddress.BasePhyAddress);
                //PlayerDmaNode_p->SPDIFNode.NumberBytes    = SamplesToGo * (2 * 4);
                PlayerDmaNode_p->SPDIFNode.NumberBytes  = (SamplesToGo << SHIFTS_FOR_TWO_CHANNEL);
                break;
            case STAUD_DIGITAL_MODE_COMPRESSED:

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
                PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.BurstPeriod = SamplesToGo;
#else

#ifdef ENABLE_TRANSCODED_DTS_IN_PCM_MODE
                if (Control_p->EnableEncOutput == TRUE && Control_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DTS)
                {
                    // Contact RB or KM to understand Why ???
                    PlayerDmaNode_p->SPDIFNode.SourceAddress_p      = (void *)((U32)Control_p->DummyMemoryAddress.BasePhyAddress);
                    PlayerDmaNode_p->SPDIFNode.NumberBytes = SamplesToGo << Control_p->shiftForTwoChannel;
                }
                else
#endif
                {
                    PlayerDmaNode_p->SPDIFNode.BurstPeriod = SamplesToGo;
                }
#endif
                break;
            default:
                break;
        }
    }

#endif
    //STTBX_Print(("SPDIF:Update Size %d\n",PlayerDmaNode_p->SPDIFNode.NumberBytes));

}


void    FillSPDIFPlayerFDMANode(PlayerAudioBlock_t * PlayerAudioBlock_p, SPDIFPlayerControl_t * Control_p)
{
    STFDMA_GenericNode_t    * PlayerDmaNode_p = PlayerAudioBlock_p->PlayerDmaNode_p;
    MemoryBlock_t           * MemoryBlock_p = PlayerAudioBlock_p->memoryBlock;
#if MEMORY_SPLIT_SUPPORT
    U32 MemoryStart = PlayerAudioBlock_p->MemoryParams.MemoryStart;
    U32 MemorySize = PlayerAudioBlock_p->MemoryParams.Size;
    U32 Flags = PlayerAudioBlock_p->MemoryParams.Flags;
#else
    U32 MemoryStart = MemoryBlock_p->MemoryStart;
    U32 MemorySize = MemoryBlock_p->Size;
    U32 Flags = MemoryBlock_p->Flags;
#endif
    if ((Flags & DATAFORMAT_VALID) && (Control_p->CompressedDataAlignment != MemoryBlock_p->Data[DATAFORMAT_OFFSET]))
    {
        Control_p->CompressedDataAlignment = MemoryBlock_p->Data[DATAFORMAT_OFFSET];
        // This should be based on LE/BE stream info
        STTBX_Print(("Setting new data alignment %d\n",Control_p->CompressedDataAlignment));
        SPDIFPlayer_SwapFDMANodeParams(Control_p);

    }

    if ((Flags & LAYER_VALID) && (Control_p->mpegLayer != (enum MPEG)((U32)MemoryBlock_p->AppData_p)))
    {
        Control_p->mpegLayer = (enum MPEG)((U32)MemoryBlock_p->AppData_p);
        STTBX_Print(("Setting new mpeg layer %d\n",Control_p->mpegLayer));
        UpdateBurstPeriod(Control_p);/*Update only the burst period*/
    }

    PlayerDmaNode_p->SPDIFNode.SourceAddress_p      = (void *)STOS_AddressTranslate(Control_p->PlayerCurrentMemoryAddress.BasePhyAddress, Control_p->PlayerCurrentMemoryAddress.BaseUnCachedVirtualAddress, MemoryStart);

    switch (Control_p->SPDIFMode)
    {
        case STAUD_DIGITAL_MODE_NONCOMPRESSED:
            // Number of bytes is always two channel as SPDIF player can handle only two channels
            PlayerDmaNode_p->SPDIFNode.NumberBytes          = ((MemorySize / (Control_p->NumChannels << SHIFTS_PER_CHANNEL)) << SHIFTS_FOR_TWO_CHANNEL) ;
            break;
        case STAUD_DIGITAL_MODE_COMPRESSED:
            PlayerDmaNode_p->SPDIFNode.NumberBytes                      = MemorySize;
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
            PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.BurstPeriod  = Control_p->CurrentBurstPeriod;
			if((Control_p->EnableEncOutput == TRUE) && Control_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DTS)
				PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleD    = (DTS_PREAMBLE_D << 3);//size * 8
			else
				PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleD    = (MemorySize << 3);//size * 8
#else
            PlayerDmaNode_p->SPDIFNode.BurstPeriod  = Control_p->CurrentBurstPeriod;
            PlayerDmaNode_p->SPDIFNode.PreambleD    = (MemorySize << 3);//size * 8
#endif

            if (Control_p->CompressedDataAlignment == BE)
            {
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
                PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleD =
                    (((PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleD & 0xFF) << 8) |
                     ((PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleD & 0xFF00) >> 8));
#else

                PlayerDmaNode_p->SPDIFNode.PreambleD =
                    (((PlayerDmaNode_p->SPDIFNode.PreambleD & 0xFF) << 8) |
                     ((PlayerDmaNode_p->SPDIFNode.PreambleD & 0xFF00) >> 8));

#endif

            }
            break;
        default:
            break;
    }

    if (Control_p->spdifMute || (Flags & DUMMYBLOCK_VALID))
    {
        PlayerDmaNode_p->SPDIFNode.SourceAddress_p      = (void *)((U32)Control_p->DummyMemoryAddress.BasePhyAddress);
    }

}

void SetSPDIFChannelStatus(SPDIFPlayerControl_t * Control_p)
{
    U32 i;
    U32 channelStatus0, channelStatus1;
    STFDMA_SPDIFNode_t  * SPDIFNode_p;
    channelStatus0 = CalculateSPDIFChannelStatus0(Control_p);
    channelStatus1 = CalculateSPDIFChannelStatus1(Control_p);

    switch (Control_p->SPDIFMode)
    {
        case STAUD_DIGITAL_MODE_NONCOMPRESSED:
            for (i=0; i < Control_p->spdifPlayerNumNode; i++)
            {
                SPDIFNode_p = &(Control_p->PlayerAudioBlock_p[i].PlayerDmaNode_p->SPDIFNode);

                SPDIFNode_p->Channel0.Status_0 = channelStatus0;
                SPDIFNode_p->Channel1.Status_0 = channelStatus0;

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
                SPDIFNode_p->Channel0.Status.PCMMode.Status_1       = channelStatus1;
                SPDIFNode_p->Channel1.Status.PCMMode.Status_1       = channelStatus1;
#else
                SPDIFNode_p->Channel0.Status_1 = channelStatus1;
                SPDIFNode_p->Channel1.Status_1 = channelStatus1;
#endif

            }
            break;
        case STAUD_DIGITAL_MODE_COMPRESSED:
            for (i=0; i < Control_p->spdifPlayerNumNode; i++)
            {
                SPDIFNode_p = &(Control_p->PlayerAudioBlock_p[i].PlayerDmaNode_p->SPDIFNode);
                SPDIFNode_p->Channel0.Status_0  = channelStatus0;
                SPDIFNode_p->Channel1.Status_0  = channelStatus0;
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
                SPDIFNode_p->Channel0.Status.CompressedMode.Status_1        = channelStatus1;
                SPDIFNode_p->Channel1.Status.CompressedMode.Status_1        = channelStatus1;
#else
                SPDIFNode_p->Channel0.Status_1 = channelStatus1;
                SPDIFNode_p->Channel1.Status_1 = channelStatus1;
#endif
            }
            break;
        default:
            break;
    }
}

void UpdateBurstPeriod(SPDIFPlayerControl_t * Control_p)
{
    Control_p->BypassMode = FALSE;

    switch (Control_p->StreamParams.StreamContent)
    {
        case STAUD_STREAM_CONTENT_AC3:
            Control_p->CurrentBurstPeriod = 1536;
            break;

        case STAUD_STREAM_CONTENT_DDPLUS:
#ifdef SPDIFP0_SUPPORTED
            if(Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
            {
                Control_p->CurrentBurstPeriod = 1536;
            }else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
            if(Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
            {
                Control_p->CurrentBurstPeriod = 6144;
            }else
#endif
            {
                /* Dummy {} to handle last else */
            }
            break;

        case STAUD_STREAM_CONTENT_MPEG1:
            switch (Control_p->mpegLayer)
            {
                case LAYER_1:
                    Control_p->CurrentBurstPeriod = 384;
                    break;
                case LAYER_2:
                case LAYER_3:
                    Control_p->CurrentBurstPeriod = 1152;
                    break;
                default:
                    break;
            }
            break;

        case STAUD_STREAM_CONTENT_MPEG2:
            Control_p->CurrentBurstPeriod = 1152;
            break;

        case STAUD_STREAM_CONTENT_MP3:
            Control_p->CurrentBurstPeriod = 1152;
            Control_p->mpegLayer = LAYER_3;
            break;

        case STAUD_STREAM_CONTENT_MPEG_AAC:
            Control_p->CurrentBurstPeriod = 1024;
            break;

        case STAUD_STREAM_CONTENT_DTS:
            Control_p->CurrentBurstPeriod = 512;
            break;

        default:
            STTBX_Print(("Audio type not supported in SPDIF\n"));
            Control_p->BypassMode = TRUE;
            break;
    }

    if(Control_p->SPDIFMode == STAUD_DIGITAL_MODE_COMPRESSED)
    {
        U32 i;
        for(i=0; i < Control_p->spdifPlayerNumNode; i++)
        {
            STFDMA_SPDIFNode_t * SPDIFNode_p = &(Control_p->PlayerAudioBlock_p[i].PlayerDmaNode_p->SPDIFNode);

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
            SPDIFNode_p->Data.CompressedMode.BurstPeriod = Control_p->CurrentBurstPeriod;
            SPDIFNode_p->Data.CompressedMode.PreambleD = Control_p->CurrentBurstPeriod << 3;
#else
            SPDIFNode_p->BurstPeriod = Control_p->CurrentBurstPeriod;
            SPDIFNode_p->PreambleD = Control_p->CurrentBurstPeriod << 3;
#endif
        }
    }
}

void SetSPDIFStreamParams(SPDIFPlayerControl_t * Control_p)
{
    U32 i;
    U32 channelStatus0, channelStatus1;
    STFDMA_SPDIFNode_t  * SPDIFNode_p;

    UpdateBurstPeriod(Control_p);
    SetSPDIFPlayerCategoryCode(Control_p);

    channelStatus0 = CalculateSPDIFChannelStatus0(Control_p);
    channelStatus1 = CalculateSPDIFChannelStatus1(Control_p);

    switch (Control_p->SPDIFMode)
    {
        U16 PreambleA, PreambleB;

        case STAUD_DIGITAL_MODE_NONCOMPRESSED:
            for (i=0; i < Control_p->spdifPlayerNumNode; i++)
            {
                SPDIFNode_p = &(Control_p->PlayerAudioBlock_p[i].PlayerDmaNode_p->SPDIFNode);

                SPDIFNode_p->BurstEnd                                    = FALSE;
                SPDIFNode_p->Channel0.Status_0                      = channelStatus0;   //0;//0x02000100; // should be set to channel status 0
                SPDIFNode_p->Channel1.Status_0                      = channelStatus0;   //0;//0x02000100; // should be set to channel status 0

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)

                SPDIFNode_p->Channel0.Status.PCMMode.Status_1     = channelStatus1;    //0x2;
                SPDIFNode_p->Channel0.Status.PCMMode.UserStatus  = 0;
                SPDIFNode_p->Channel0.Status.PCMMode.Valid          = 0;
                SPDIFNode_p->Channel0.Status.PCMMode.Pad            = 0;

                SPDIFNode_p->Channel1.Status.PCMMode.Status_1     = channelStatus1;   //0x2;
                SPDIFNode_p->Channel1.Status.PCMMode.UserStatus  = 0;
                SPDIFNode_p->Channel1.Status.PCMMode.Valid          = 0;
                SPDIFNode_p->Channel1.Status.PCMMode.Pad            = 0;

                SPDIFNode_p->BurstEnd = FALSE;
                SPDIFNode_p->ModeData = 0x1;

                SPDIFNode_p->Data.PCMMode.SStride                      = Control_p->NumChannels * BYTES_PER_CHANNEL_IN_PLAYER;
                SPDIFNode_p->Data.PCMMode.Reserved1                 = 0;
                SPDIFNode_p->Data.PCMMode.Reserved2                 = 0;
#else
                SPDIFNode_p->Channel0.Status_1                    = channelStatus1;//0x2;
                SPDIFNode_p->Channel0.UserStatus                 = 0;
                SPDIFNode_p->Channel0.Valid                         = 0;
                SPDIFNode_p->Channel0.Pad                           = 0;

                SPDIFNode_p->Channel1.Status_1                    = channelStatus1;//0x2;
                SPDIFNode_p->Channel1.UserStatus                 = 0;
                SPDIFNode_p->Channel1.Valid                         = 0;
                SPDIFNode_p->Channel1.Pad                           = 0;

                SPDIFNode_p->PreambleB                              = 0;
                SPDIFNode_p->PreambleA                              = 0;
                SPDIFNode_p->PreambleD                              = 0;
                SPDIFNode_p->PreambleC                              = 0;

#endif
            }
            Control_p->BypassMode = FALSE;
            break;

        case STAUD_DIGITAL_MODE_COMPRESSED:
            //Control_p->CompressedDataAlignment = LE; // By default nodes are set for LE and then converted to BE if required
            // The above line was removed as there was danger of it being overwritten.
            for (i=0; i < Control_p->spdifPlayerNumNode; i++)
            {
                SPDIFNode_p = &(Control_p->PlayerAudioBlock_p[i].PlayerDmaNode_p->SPDIFNode);

#ifdef ENABLE_TRANSCODED_DTS_IN_PCM_MODE
                        if (Control_p->EnableEncOutput == TRUE && Control_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DTS)
                        {
                            // DTS Encoder output will be played in PCM mode
                            SPDIFNode_p->BurstEnd                                               = FALSE;
                        }
                        else
#endif
                        {
                            SPDIFNode_p->BurstEnd                                               = TRUE;
                        }

                SPDIFNode_p->Channel0.Status_0                              = channelStatus0;//0x02001906;
                SPDIFNode_p->Channel1.Status_0                              = channelStatus0;//0x02001906;

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
                SPDIFNode_p->Channel0.Status.CompressedMode.Status_1        = channelStatus1;
                SPDIFNode_p->Channel0.Status.CompressedMode.UserStatus  = 0;
                SPDIFNode_p->Channel0.Status.CompressedMode.Valid           = 1;
                SPDIFNode_p->Channel0.Status.CompressedMode.Pad             = 0;


                SPDIFNode_p->Channel1.Status.CompressedMode.Status_1        = channelStatus1;
                SPDIFNode_p->Channel1.Status.CompressedMode.UserStatus  = 0;
                SPDIFNode_p->Channel1.Status.CompressedMode.Valid           = 1;
                SPDIFNode_p->Channel1.Status.CompressedMode.Pad             = 0;
#else
                SPDIFNode_p->Channel0.Status_1                              = channelStatus1;
                SPDIFNode_p->Channel0.UserStatus                            = 0;
                SPDIFNode_p->Channel0.Valid                                 = 1;
                SPDIFNode_p->Channel0.Pad                                   = 0;


                SPDIFNode_p->Channel1.Status_1                              = channelStatus1;
                SPDIFNode_p->Channel1.UserStatus                            = 0;
                SPDIFNode_p->Channel1.Valid                                 = 1;
                SPDIFNode_p->Channel1.Pad                                   = 0;
#endif

                if(Control_p->CompressedDataAlignment == LE)
                {
                    PreambleA = 0xF872;
                    PreambleB = 0x4E1F;

                }
                else
                {
                    PreambleA = 0x72F8;
                    PreambleB = 0x1F4E;
                }
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
                SPDIFNode_p->ModeData                                                     = 0x0;

                SPDIFNode_p->Data.CompressedMode.PreambleA                  = PreambleA;
                SPDIFNode_p->Data.CompressedMode.PreambleB                  = PreambleB;

                /*SPDIFNode_p->Data.CompressedMode.BurstPeriod              = Control_p->CurrentBurstPeriod;
                SPDIFNode_p->Data.CompressedMode.PreambleD                  = Control_p->CurrentBurstPeriod * 8;*/
#else
                SPDIFNode_p->PreambleA                                                    = PreambleA;
                SPDIFNode_p->PreambleB                                                    = PreambleB;
    /*          SPDIFNode_p->BurstPeriod                                    = Control_p->CurrentBurstPeriod;
                SPDIFNode_p->PreambleD                                      = Control_p->CurrentBurstPeriod * 8;*/
#endif
            }
            break;
        default:
            break;
    }

    // Set preamble C based on audio stream type and audio contents
    SetSPDIFPreambleC(Control_p);
}

void SPDIFPlayer_EventHandler(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
    SPDIFPlayerControl_t * Control_p = (SPDIFPlayerControl_t *)SubscriberData_p;

    if (Reason == CALL_REASON_NOTIFY_CALL)
    {
        STOS_MutexLock(Control_p->LockControlStructure);

        switch(Event)
        {
            case STAUD_AVSYNC_SKIP_EVT:
                //STTBX_Print(("SPDIF : SKIP_EVT:Received:Skip = %d\n",*((U32 *)EventData)));
                if ((Control_p->skipDuration == 0) || (*((U32 *)EventData) == 0))
                {
                    Control_p->skipDuration = *((U32 *)EventData);
                }
                break;

            case STAUD_AVSYNC_PAUSE_EVT:
                //STTBX_Print(("SPDIF : PAUSE_EVT:Received:Pause = %d\n",*((U32 *)EventData)));
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


ST_ErrorCode_t SPDIFPlayer_SubscribeEvents(SPDIFPlayerControl_t *Control_p)
{
    STEVT_Handle_t  EvtHandle = Control_p->EvtHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STEVT_DeviceSubscribeParams_t       EVT_SubcribeParams;
    EVT_SubcribeParams.NotifyCallback   = SPDIFPlayer_EventHandler;
    EVT_SubcribeParams.SubscriberData_p = (void *)Control_p;

    if (Control_p->skipEventID != 0)
        ErrorCode |= STEVT_SubscribeDeviceEvent(EvtHandle, Control_p->Name, Control_p->skipEventID, &EVT_SubcribeParams);

    if (Control_p->pauseEventID != 0)
        ErrorCode |= STEVT_SubscribeDeviceEvent(EvtHandle, Control_p->Name, Control_p->pauseEventID, &EVT_SubcribeParams);

    //ErrorCode|=STEVT_SubscribeDeviceEvent(EvtHandle, (char*)"PLAYER0", STAUD_AVSYNC_PAUSE_EVT, &EVT_SubcribeParams);

    return (ErrorCode);

}

ST_ErrorCode_t SPDIFPlayer_UnSubscribeEvents(SPDIFPlayerControl_t *Control_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (Control_p->skipEventID != 0)
        ErrorCode |= STEVT_UnsubscribeDeviceEvent(Control_p->EvtHandle, Control_p->Name, Control_p->skipEventID);

    if (Control_p->pauseEventID != 0)
        ErrorCode |= STEVT_UnsubscribeDeviceEvent(Control_p->EvtHandle, Control_p->Name, Control_p->pauseEventID);

    return (ErrorCode);
}

/* Register player control block as producer for sending AVSync events to all releated Modules  */
ST_ErrorCode_t SPDIFPlayer_RegisterEvents(SPDIFPlayerControl_t *Control_p)
{
    AudAVSync_t * PlayerAVsync_p = &(Control_p->audioAVsync);
    STEVT_Handle_t      EvtHandle = Control_p->EvtHandle;
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    /* Register to the events */
    Error |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_AVSYNC_SKIP_EVT, &PlayerAVsync_p->EventIDSkip);
    Error |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_AVSYNC_PAUSE_EVT, &PlayerAVsync_p->EventIDPause);

    /* External Events generated by SPDIF player*/
    Error |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_IN_SYNC, &PlayerAVsync_p->EventIDInSync);
    Error |= STEVT_RegisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_OUTOF_SYNC, &PlayerAVsync_p->EventIDOutOfSync);

    return Error;
}

/* Un register a/v sync events */
ST_ErrorCode_t SPDIFPlayer_UnRegisterEvents(SPDIFPlayerControl_t *Control_p)
{
    STEVT_Handle_t  EvtHandle = Control_p->EvtHandle;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* UnRegister to the events */
    Error =STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_AVSYNC_SKIP_EVT);
    Error|=STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->Name, STAUD_AVSYNC_PAUSE_EVT);

    Error|=STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_IN_SYNC);
    Error|=STEVT_UnregisterDeviceEvent(EvtHandle, (char*)Control_p->EvtGeneratorName, STAUD_OUTOF_SYNC);

    return Error;
}


/* Notify to the Subscribed Modules */
ST_ErrorCode_t SPDIFPlayer_NotifyEvt(U32 EvtType,U32 Value,SPDIFPlayerControl_t *Control_p )
{
    AudAVSync_t * PlayerAVsync_p = &(Control_p->audioAVsync);
    STEVT_Handle_t      EvtHandle = Control_p->EvtHandle;
    STAUD_Object_t      ObjectID = Control_p->ObjectID;
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    switch(EvtType)
    {
    case STAUD_AVSYNC_SKIP_EVT:
        Error=STEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDSkip, &Value);
        STTBX_Print(("SPDIF SKIP_EVT:Notify:=%d\n",Value));
        break;
    case STAUD_AVSYNC_PAUSE_EVT:
        Error=STEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDPause, &Value);
        STTBX_Print(("SPDIF PAUSE_EVT:Notify:=%d\n",Value));
        break;
    case STAUD_IN_SYNC:
        Error=STAudEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDInSync, &Value, sizeof(Value), ObjectID);
        //STTBX_Print(("INSYNC_EVT:\n"));
        break;
    case STAUD_OUTOF_SYNC:
        Error=STAudEVT_Notify(EvtHandle, PlayerAVsync_p->EventIDOutOfSync, &Value, sizeof(Value), ObjectID);
        //STTBX_Print(("OUTOFSYNC_EVT:\n"));
        break;

    default :
        break;
    }
    return Error;
}

ST_ErrorCode_t STAud_SPDIFPlayerSetOffset(STSPDIFPLAYER_Handle_t PlayerHandle,S32 Offset)
{
   SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempSPDIFplayerControlBlock_p != NULL)
    {
        if (tempSPDIFplayerControlBlock_p == (SPDIFPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if(tempSPDIFplayerControlBlock_p== NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if(tempSPDIFplayerControlBlock_p->spdifPlayerControl.audioSTCSync.Offset != Offset)
    {
        STOS_MutexLock(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);
        tempSPDIFplayerControlBlock_p->spdifPlayerControl.audioSTCSync.Offset = Offset;
        STOS_MutexRelease(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);
    }

    return Error;
}

ST_ErrorCode_t STAud_SPDIFPlayerGetOffset(STSPDIFPLAYER_Handle_t PlayerHandle,S32 *Offset_p)
{
   SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempSPDIFplayerControlBlock_p != NULL)
    {
        if (tempSPDIFplayerControlBlock_p == (SPDIFPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if(tempSPDIFplayerControlBlock_p== NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    STOS_MutexLock(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);
    *Offset_p = tempSPDIFplayerControlBlock_p->spdifPlayerControl.audioSTCSync.Offset;
    STOS_MutexRelease(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);

    return Error;

}


ST_ErrorCode_t STAud_SPDIFPlayerSetLatency(STSPDIFPLAYER_Handle_t PlayerHandle,U32 Latency)
{
   SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempSPDIFplayerControlBlock_p != NULL)
    {
        if (tempSPDIFplayerControlBlock_p == (SPDIFPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if(tempSPDIFplayerControlBlock_p== NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if(tempSPDIFplayerControlBlock_p->spdifPlayerControl.audioSTCSync.UsrLatency != Latency)
    {
        STOS_MutexLock(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);
        tempSPDIFplayerControlBlock_p->spdifPlayerControl.audioSTCSync.UsrLatency = Latency;
        STOS_MutexRelease(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);
    }

    return Error;
}

ST_ErrorCode_t STAud_SPDIFPlayerAVSyncCmd(STSPDIFPLAYER_Handle_t PlayerHandle,BOOL Command)
{
   SPDIFPlayerControlBlock_t * tempspdifplayerControlBlock = spdifPlayerControlBlock;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempspdifplayerControlBlock != NULL)
    {
        if (tempspdifplayerControlBlock == (SPDIFPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempspdifplayerControlBlock = tempspdifplayerControlBlock->next;
    }
    if(tempspdifplayerControlBlock== NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    tempspdifplayerControlBlock->spdifPlayerControl.AVSyncEnable = Command;

    return Error;
}


#if defined( ST_OSLINUX )
void log_spdif_skip_values(SPDIFPlayerControl_t *ControlBlock_p,STCLKRV_ExtendedSTC_t CurrentPTS)
{
#if LOG_AVSYNC_SPDIF_LINUX
        if(Log_To_Spdif_Array)
        {
            Aud_Spdif_skip_Array[Aud_spdif_skip_Count][0]=ControlBlock_p->audioSTCSync.NewPCR;
            Aud_Spdif_skip_Array[Aud_spdif_skip_Count][1]=ControlBlock_p->audioAVsync.CurrentPTS;
            Aud_Spdif_skip_Array[Aud_spdif_skip_Count][2]=CurrentPTS.BaseValue;

            Aud_spdif_skip_Count++;
            if(Aud_spdif_skip_Count >= NUM_OF_ELEMENTS)
            {
                //Log_To_Array = 0;
                STTBX_Print(("Skip SPDIF Logged \n"));
                {
                    int i,j;
                    for(i=0;i<Aud_spdif_skip_Count;i++)
                    {
                        STTBX_Print(("%u %8u %8u \n", Aud_Spdif_skip_Array[i][0],Aud_Spdif_skip_Array[i][1],Aud_Spdif_skip_Array[i][2]));
                    }
                    Aud_spdif_skip_Count = 0;
                }
            }
        }
#endif
}

void log_spdif_pause_values(SPDIFPlayerControl_t *ControlBlock_p, STCLKRV_ExtendedSTC_t CurrentPTS)
{
#if LOG_AVSYNC_SPDIF_LINUX
        if(Log_To_Spdif_Array)
        {
            Aud_spdif_Pause_Array[Aud_spdif_Pause_Count][0]=ControlBlock_p->audioSTCSync.NewPCR;
            Aud_spdif_Pause_Array[Aud_spdif_Pause_Count][1]=ControlBlock_p->audioAVsync.CurrentPTS;
            Aud_spdif_Pause_Array[Aud_spdif_Pause_Count][2]=CurrentPTS.BaseValue;

            Aud_spdif_Pause_Count++;
            if(Aud_spdif_Pause_Count >= NUM_OF_ELEMENTS)
            {
                //Log_To_Array = 0;
                STTBX_Print((" pause spdif Logged \n"));
                {
                    int i,j;
                    for(i=0;i<Aud_spdif_Pause_Count;i++)
                    {
                        STTBX_Print(("%u %8u %8u \n", Aud_spdif_Pause_Array[i][0],Aud_spdif_Pause_Array[i][1],Aud_spdif_Pause_Array[i][2]));
                    }
                    Aud_spdif_Pause_Count = 0;
                }
            }
        }
#endif
}
#endif //ST_OSLINUX
/* ------------------------------------------------------------------------------
 * function :       PTSPCROffsetInms
 * parameters:      U32 PTS
 *
 * returned value:  ST_ErrorCode_t
 *
   -----------------------------------------------------------------------------*/

#ifndef STAUD_REMOVE_CLKRV_SUPPORT

ST_ErrorCode_t SPDIFPTSPCROffsetInms(SPDIFPlayerControl_t *ControlBlock_p,STCLKRV_ExtendedSTC_t CurrentInputPTS)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    BOOL DoSync=FALSE;
#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
    U32 PauseSkip=0,SkipPause=0;
    STCLKRV_ExtendedSTC_t CurrentPTS = CurrentInputPTS;
#endif

    if(ControlBlock_p->AVSyncEnable==TRUE)
    {
        U32 PTSSTCDiff = 0;
        AudSTCSync_t        * AudioSTCSync_p = &(ControlBlock_p->audioSTCSync);

        AudioSTCSync_p->SkipPause = 0;
        AudioSTCSync_p->PauseSkip = 0;

#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
        if(AudioSTCSync_p->SyncState == SYNCHRO_STATE_FIRST_PTS)
        {
            AudioSTCSync_p->LastPTS = CurrentPTS;

            Error = STCLKRV_GetExtendedSTC(ControlBlock_p->CLKRV_HandleForSynchro, &AudioSTCSync_p->LastPCR);
            if ( Error != ST_NO_ERROR)
            {
                return Error;
            }
            //STTBX_Print(("SPDIF SYNCHRO_STATE_NORMAL: PCR=%d\n",AudioSTCSync_p->LastPCR.BaseValue));
            AudioSTCSync_p->SyncState = SYNCHRO_STATE_NORMAL;
#if 0//LOG_AVSYNC
            Aud_Array1[0][Aud_Count1]=AudioSTCSync_p->LastPCR.BaseValue;
            Aud_Array1[1][Aud_Count1]=CurrentPTS.BaseValue;
            Aud_Count1++;
            if(Aud_Count1>400)
            {
                FILE *fp;
                U32 i;
                fp= fopen("d:\\stc_drift_spdif_fr.csv","w+");
                if(fp!=NULL)
                {
                    for (i=0;i<Aud_Count1;i++)
                            fprintf(fp,"%u,%u \n",Aud_Array1[0][i],Aud_Array1[1][i]);
                    fclose(fp);
                }
            }
#endif
            return Error; // Comment this to to sync on first valid PTS
        }

        Error = STCLKRV_GetExtendedSTC(ControlBlock_p->CLKRV_HandleForSynchro, &AudioSTCSync_p->NewPCR);
        if ( Error != ST_NO_ERROR)
        {
            AudioSTCSync_p->BadClockCount++;
            return Error;
        }

        ADDToEPTS(&CurrentPTS, MILLISECOND_TO_PTS((AudioSTCSync_p->Offset + AudioSTCSync_p->UsrLatency)));
        AudioSTCSync_p->BadClockCount = 0;
#if LOG_AVSYNC
        Aud_Array[0][Aud_Count]=AudioSTCSync_p->NewPCR.BaseValue;
        Aud_Array[1][Aud_Count]=CurrentPTS.BaseValue;
        Aud_Count++;
        if(Aud_Count>1000)
        {
            FILE *fp;
            U32 i;
            fp= fopen("d:\\stc_drift_spdif.csv","w+");
            if(fp!=NULL)
            {
                for (i=0;i<Aud_Count;i++)
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

        STTBX_Print(("SPDIFPLAYER(%s):STC,%u,%u, PTS,%u,%u \n", ControlBlock_p->Name,AudioSTCSync_p->NewPCR.BaseBit32,AudioSTCSync_p->NewPCR.BaseValue,CurrentPTS.BaseBit32,CurrentPTS.BaseValue));
        /* -------------------------------- */
        /* Compare PTS against decoder time */
        /* -------------------------------- */

        /* If time difference between pts and pcr is too big, we don't apply correction
        * This handles the typical case of a packet injector that will wrap around
        * the time references at the end of his stream. */
        //STTBX_Print(("CPTSD SPDIF \n"));

        DoSync = FALSE;

        if (EPTS_LATER_THAN(CurrentPTS, AudioSTCSync_p->NewPCR))
        {
            PTSSTCDiff = EPTS_DIFF(CurrentPTS, ControlBlock_p->audioSTCSync.NewPCR);
            if (PTSSTCDiff <= HUGE_TIME_DIFFERENCE_PAUSE)
            {
                DoSync = TRUE;
            }
        }
        else
        {
            PTSSTCDiff = EPTS_DIFF(CurrentPTS, AudioSTCSync_p->NewPCR);
            if (PTSSTCDiff <= HUGE_TIME_DIFFERENCE_SKIP)
            {
                DoSync = TRUE;
            }
        }

        if (DoSync==TRUE/*PTSSTCDiff <= HUGE_TIME_DIFFERENCE*/)
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
                    PauseSkip  = CEIL_PTSSTC_DRIFT(PauseSkip);
                    AudioSTCSync_p->PauseSkip = PauseSkip;
                    AudioSTCSync_p->SkipPause = 0;

                    STTBX_Print(("SPDIFPLAYER(%s) => Pause (ms)=%d\n",ControlBlock_p->Name,PauseSkip));

#if defined( ST_OSLINUX )
                    log_spdif_pause_values(ControlBlock_p,CurrentPTS);
                    ControlBlock_p->Spdif_Pause_Performed++;
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

                    STTBX_Print(("SPDIFPLAYER(%s) => Skip(ms)=%d\n",ControlBlock_p->Name,SkipPause));

#if defined( ST_OSLINUX )
                    log_spdif_skip_values(ControlBlock_p,CurrentPTS);
                    ControlBlock_p->Spdif_Skip_Performed++;
#endif
                }
            }
        }
        else
        {
            /* The difference between the values is too big, let's go back to*/
            AudioSTCSync_p->SyncState = SYNCHRO_STATE_FIRST_PTS;
            AudioSTCSync_p->HugeCount++;

            STTBX_Print(("SPDIFPLAYER(%s) => Reset synchro\n",ControlBlock_p->Name));

#if defined( ST_OSLINUX )
            ControlBlock_p->Spdif_Huge_Diff++;
#endif
        }
#endif
    }
    //Hack- to test PTS values without clkrv
    //ControlBlock_p->audioSTCSync.SkipPause = 0;
    //ControlBlock_p->audioSTCSync.PauseSkip = 0;
    return Error;
}

#endif

void ResetSPDIFPlayerSyncParams(SPDIFPlayerControl_t * ControlBlock_p)
{
    AudSTCSync_t        * AudioSTCSync_p = &(ControlBlock_p->audioSTCSync);
    AudAVSync_t * PlayerAVsync_p = &(ControlBlock_p->audioAVsync);

    AudioSTCSync_p->SyncState = SYNCHRO_STATE_FIRST_PTS;  /*AVSync state of Parser*/
    AudioSTCSync_p->SyncDelay = 0;

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

    PlayerAVsync_p->Freq = ControlBlock_p->samplingFrequency;//reset to default frequency

    /* Reset everything related to avsync*/
    ControlBlock_p->skipDuration = 0;
    ControlBlock_p->pauseDuration = 0;

}

ST_ErrorCode_t      STAud_SPDIFPlayerMute(STSPDIFPLAYER_Handle_t handle, BOOL   SPDIFMute)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock, * lastSPDIFPlayerControlBlock_p = spdifPlayerControlBlock;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        lastSPDIFPlayerControlBlock_p = tempSPDIFplayerControlBlock_p;
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);
    /*change the next state*/
    STOS_MutexLock(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);
    tempSPDIFplayerControlBlock_p->spdifPlayerControl.spdifMute             = SPDIFMute;
    STOS_MutexRelease(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);

    // Set SPDIF preamble C based in mute flag and audio contents
    SetSPDIFPreambleC(&(tempSPDIFplayerControlBlock_p->spdifPlayerControl));

    return (Error);
}

void SetSPDIFPreambleC(SPDIFPlayerControl_t * Control_p)
{
    U32 i,PreambleC = 0x0003;
    STFDMA_GenericNode_t    * PlayerDmaNode_p;

    if (Control_p->spdifMute)
    {
        switch (Control_p->SPDIFMode)
        {
            case STAUD_DIGITAL_MODE_NONCOMPRESSED:

                break;
            case STAUD_DIGITAL_MODE_COMPRESSED:
                if(Control_p->CompressedDataAlignment == BE)
                {
                    PreambleC = (((PreambleC & 0x00FF) << 8)|((PreambleC & 0xFF00) >> 8));
                }

                for (i=0; i < Control_p->spdifPlayerNumNode; i++)
                {

                    PlayerDmaNode_p = Control_p->PlayerAudioBlock_p[i].PlayerDmaNode_p;
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
                    PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleC = PreambleC;    // Pause Burst to be generated for all audio types in compressed mode
#else
                    PlayerDmaNode_p->SPDIFNode.PreambleC = PreambleC;    // Pause Burst to be generated for all audio types in compressed mode
#endif
                }
                break;
            default:
                break;
        }
    }
    else
    {
        switch (Control_p->SPDIFMode)
        {
            case STAUD_DIGITAL_MODE_NONCOMPRESSED:

                break;
            case STAUD_DIGITAL_MODE_COMPRESSED:
                switch (Control_p->StreamParams.StreamContent)
                {
                    case STAUD_STREAM_CONTENT_AC3:
                        PreambleC = 0x0001;
                        break;

                    case STAUD_STREAM_CONTENT_DDPLUS:
#ifdef SPDIFP0_SUPPORTED
                        if(Control_p->spdifPlayerIdentifier == SPDIF_PLAYER_0)
                        {
                            PreambleC = 0x0001;
                        }else
#endif
#ifdef HDMISPDIFP0_SUPPORTED
                        if(Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
                        {
                            PreambleC = 0x0015;
                        }else
#endif
                        {
                            /* Dummy {} to handle last else */
                        }
                        break;

                    case STAUD_STREAM_CONTENT_MPEG1:
                        switch (Control_p->mpegLayer)
                        {
                            case LAYER_1:

                                PreambleC = 0x0004;
                                break;
                            case LAYER_2:
                            case LAYER_3:

                                PreambleC = 0x0005;
                                break;
                            case LAYER_AAC:
                            default:
                                break;
                        }
                        break;

                    case STAUD_STREAM_CONTENT_MPEG2:

                        PreambleC = 0x0005;
                        break;

                    case STAUD_STREAM_CONTENT_MP3:

                        PreambleC = 0x0005;
                        break;

                    case STAUD_STREAM_CONTENT_MPEG_AAC:

                        PreambleC = 0x0007;
                        break;

                    case STAUD_STREAM_CONTENT_DTS:

                        PreambleC = 0x000B; // DTS type 1 512
                        break;

                    default:
                        STTBX_Print(("2.Audio type not supported in SPDIF\n"));
                        break;
                }

                if(Control_p->CompressedDataAlignment == BE)
                {
                    PreambleC = (((PreambleC & 0x00FF) << 8)|((PreambleC & 0xFF00) >> 8));
                }

                for (i=0; i < Control_p->spdifPlayerNumNode; i++)
                {
                    PlayerDmaNode_p = Control_p->PlayerAudioBlock_p[i].PlayerDmaNode_p;
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
                    PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleC = PreambleC;
#else
                    PlayerDmaNode_p->SPDIFNode.PreambleC = PreambleC;
#endif
                }
                break;

            default:
                break;
        }
    }
}


ST_ErrorCode_t      STAud_SPDIFPlayerGetCapability(STSPDIFPLAYER_Handle_t handle, STAUD_OPCapability_t * Capability_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock, * lastSPDIFPlayerControlBlock_p = spdifPlayerControlBlock;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        lastSPDIFPlayerControlBlock_p = tempSPDIFplayerControlBlock_p;
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    Capability_p->ChannelMuteCapable = FALSE;
    Capability_p->MaxSyncOffset      = 0;
    Capability_p->MinSyncOffset      = 0;

    return (Error);
}




ST_ErrorCode_t      STAud_SPDIFPlayerSetOutputParams(STSPDIFPLAYER_Handle_t handle, STAUD_SPDIFOutParams_t SPDIFOutputParams)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock, * lastSPDIFPlayerControlBlock_p = spdifPlayerControlBlock;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        lastSPDIFPlayerControlBlock_p = tempSPDIFplayerControlBlock_p;
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    STOS_MutexLock(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);
    tempSPDIFplayerControlBlock_p->spdifPlayerControl.SPDIFPlayerOutParams = SPDIFOutputParams;
    STOS_MutexRelease(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);


    return (Error);
}

ST_ErrorCode_t      STAud_SPDIFPlayerGetOutputParams(STSPDIFPLAYER_Handle_t handle, STAUD_SPDIFOutParams_t * SPDIFOutputParams_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    SPDIFPlayerControlBlock_t * tempSPDIFplayerControlBlock_p = spdifPlayerControlBlock, * lastSPDIFPlayerControlBlock_p = spdifPlayerControlBlock;

    while (tempSPDIFplayerControlBlock_p != (SPDIFPlayerControlBlock_t *)handle)
    {
        lastSPDIFPlayerControlBlock_p = tempSPDIFplayerControlBlock_p;
        tempSPDIFplayerControlBlock_p = tempSPDIFplayerControlBlock_p->next;
    }

    if (tempSPDIFplayerControlBlock_p == NULL)
        return (ST_ERROR_INVALID_HANDLE);

    STOS_MutexLock(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);
    *SPDIFOutputParams_p = tempSPDIFplayerControlBlock_p->spdifPlayerControl.SPDIFPlayerOutParams;
    STOS_MutexRelease(tempSPDIFplayerControlBlock_p->spdifPlayerControl.LockControlStructure);

    return (Error);
}


void SetSPDIFPlayerCategoryCode(SPDIFPlayerControl_t * Control_p)
{
    BOOL OriginalMaskIsNegated;
    U8   CategoryCode;

    if( Control_p->SPDIFPlayerOutParams.AutoCategoryCode == TRUE )
    {
    /* Category code is automatically set according to stream being decoded */
        CategoryCode = CATEGORY_CODE_OPTICAL_DATA_61937;
    }
    else
    {
        /* CategoryCode has been manually set by application */
        CategoryCode = Control_p->SPDIFPlayerOutParams.CategoryCode;
    }

    /* Depending upon the category code in use the bit indicating
       an orignal data stream may be inverted */
    if( ( ( CategoryCode & CATEGORY_CODE_BROADCAST_A_MASK ) == CATEGORY_CODE_BROADCAST_A_DATA ) ||
        ( ( CategoryCode & CATEGORY_CODE_BROADCAST_B_MASK ) == CATEGORY_CODE_BROADCAST_B_DATA ) ||
        ( ( CategoryCode & CATEGORY_CODE_OPTICAL_MASK ) == CATEGORY_CODE_OPTICAL_DATA     ) )
    {
        /* Negated, 0 = Original, 1 = No indication of generation */
        OriginalMaskIsNegated = TRUE;
   }
    else
    {
        /* Not negated, 1 = Original, 0 = No indication of generation */
        OriginalMaskIsNegated = FALSE;
    }

    switch(Control_p->SPDIFPlayerOutParams.CopyPermitted)
    {
        case  STAUD_COPYRIGHT_MODE_NO_COPY:
            Control_p->CopyPermitted = FALSE;
            Control_p->CategoryCode = CategoryCode;
            break;

        case  STAUD_COPYRIGHT_MODE_NO_RESTRICTION:
            Control_p->CopyPermitted = TRUE;

            /* Data is marked as no indication of generation */
            if( OriginalMaskIsNegated )
            {
                /* Original Bit = 1 = no indication of generation */
                 Control_p->CategoryCode = CategoryCode | CATEGORY_CODE_ORIGINAL_DATA;
            }
            else
            {
                /* Original Bit = 0 = no indication of generation */
                Control_p->CategoryCode = CategoryCode & ~(CATEGORY_CODE_ORIGINAL_DATA);
            }
            break;

        case  STAUD_COPYRIGHT_MODE_ONE_COPY:
            Control_p->CopyPermitted = TRUE;

            /* Data is marked as 1st generation original */
           if( OriginalMaskIsNegated )
            {
                /* Original Bit = 0 = 1st generation */
                Control_p->CategoryCode = CategoryCode & ~(CATEGORY_CODE_ORIGINAL_DATA);
            }
            else
            {
                /* Original Bit = 1 = 1st generation */
                Control_p->CategoryCode = CategoryCode | CATEGORY_CODE_ORIGINAL_DATA;
            }
            break;

        default:
            break;
    }

}

U32 CalculateSPDIFChannelStatus0(SPDIFPlayerControl_t * Control_p)
{
    U32 samplingFrequency = Control_p->samplingFrequency;
    U32 channelStatus0 = (Control_p->CategoryCode << 8) | (Control_p->CopyPermitted << 2);

    switch (Control_p->SPDIFMode)
    {
        case STAUD_DIGITAL_MODE_NONCOMPRESSED:
            if(Control_p->SPDIFPlayerOutParams.Emphasis == STAUD_SPDIF_EMPHASIS_CD_TYPE)
            {
                channelStatus0 |= 0x8;
            }
            break;

        case STAUD_DIGITAL_MODE_COMPRESSED:
#ifdef ENABLE_TRANSCODED_DTS_IN_PCM_MODE
                if (Control_p->EnableEncOutput == TRUE && Control_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DTS)
                {
                    // In encoder output in compressed mode will be played back in PCM mode
                }
                else
#endif
                {
                    channelStatus0 |= 0x2;
                }
            break;

        default:
            break;
    }

    switch (samplingFrequency)
    {
        case 22050:
            channelStatus0 = channelStatus0 | (0x4 << 24);
            break;

        case 44100:
            channelStatus0 = channelStatus0 | (0x0 << 24);
            break;

        case 88200:
            channelStatus0 = channelStatus0 | (0x8 << 24);
            break;

        case 176400:
            channelStatus0 = channelStatus0 | (0xC << 24);
            break;

        case 24000:
            channelStatus0 = channelStatus0 | (0x6 << 24);
            break;

        case 48000:
            channelStatus0 = channelStatus0 | (0x2 << 24);
            break;

        case 96000:
            channelStatus0 = channelStatus0 | (0xA << 24);
            break;

        case 192000:
            channelStatus0 = channelStatus0 | (0xE << 24);
            break;

        case 32000:
            channelStatus0 = channelStatus0 | (0x3 << 24);
            break;

        default:
            break;
    }

    return (channelStatus0);

}

U32 CalculateSPDIFChannelStatus1(SPDIFPlayerControl_t * Control_p)
{
    U32 channelStatus1 = 0;

    if (Control_p->SPDIFMode == STAUD_DIGITAL_MODE_COMPRESSED)
    {
        channelStatus1 = 0x2;
    }
    else
    {
        switch (Control_p->SPDIFPlayerOutParams.SPDIFDataPrecisionPCMMode)
        {
            case STAUD_SPDIF_DATA_PRECISION_24BITS:
                channelStatus1 = 0xB;
            case STAUD_SPDIF_DATA_PRECISION_23BITS:
                channelStatus1 = 0x9;
            case STAUD_SPDIF_DATA_PRECISION_22BITS:
                channelStatus1 = 0x5;
            case STAUD_SPDIF_DATA_PRECISION_21BITS:
                channelStatus1 = 0xD;
            case STAUD_SPDIF_DATA_PRECISION_20BITS:
                channelStatus1 = 0x3;
            case STAUD_SPDIF_DATA_PRECISION_19BITS:
                channelStatus1 = 0x8;
            case STAUD_SPDIF_DATA_PRECISION_18BITS:
                channelStatus1 = 0x4;
            case STAUD_SPDIF_DATA_PRECISION_17BITS:
                channelStatus1 = 0xC;
            case STAUD_SPDIF_DATA_PRECISION_16BITS:
                channelStatus1 = 0x2;
            default:
                channelStatus1 = 0x0;

        }
    }

    return (channelStatus1);

}

void UpdateSPDIFSynced(SPDIFPlayerControl_t * Control_p)
{
    AudSTCSync_t    * AudioSTCSync = &(Control_p->audioSTCSync);
    U32 BurstTime = 0;
    //U32 SampleSize    = (Control_p->NumChannels * BYTES_PER_CHANNEL_IN_PLAYER);
    U32 FrameSize   = Control_p->CurrentBurstPeriod;
    //U32 PauseSize = (Control_p->dummyBufferSize / SampleSize) - FrameSize;
    //We only play two channels on SPDIF Player
    U32 PauseSize   = (Control_p->dummyBufferSize >> Control_p->shiftForTwoChannel) - FrameSize;
    U32 Duration = 0;

    if(AudioSTCSync->SkipPause)
    {
        BurstTime = (((FrameSize - 192)* 1000)/Control_p->samplingFrequency);
        Duration = AudioSTCSync->SkipPause;
    }

    if(AudioSTCSync->PauseSkip)
    {
        BurstTime = ((PauseSize * 1000)/Control_p->samplingFrequency);
        Duration = AudioSTCSync->PauseSkip;
    }

    AudioSTCSync->Synced = ((Duration + BurstTime)/BurstTime);

    if(AudioSTCSync->Synced)
    {
        AudioSTCSync->Synced += 2;
    }
    STTBX_Print(("SPDIF Time=%d, Sync=%d, \n",Duration, AudioSTCSync->Synced));

}

U32 SPDIFPlayer_ConvertFsMultiplierToClkDivider(U32 FrequencyMultiplier)
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

BOOL SPDIFPlayer_StartupSync(PlayerAudioBlock_t * PlayerAudioBlock_p, SPDIFPlayerControlBlock_t * SPDIFPlayerControlBlock_p)
{
#if ENABLE_STARTUP_SYNC
#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
    SPDIFPlayerControl_t * Control_p = &(SPDIFPlayerControlBlock_p->spdifPlayerControl);
    AudAVSync_t * PlayerAVsync_p = &(Control_p->audioAVsync);
    MemoryBlock_t * MemoryBlock_p = PlayerAudioBlock_p->memoryBlock;

    if(Control_p->AVSyncEnable == TRUE)
    {
        STCLKRV_ExtendedSTC_t tempPCR;
        ST_ErrorCode_t Error = ST_NO_ERROR;
#if MEMORY_SPLIT_SUPPORT
        U32 MemorySize = PlayerAudioBlock_p->MemoryParams.Size;
        U32 Flags = PlayerAudioBlock_p->MemoryParams.Flags;
#else
       U32 MemorySize = MemoryBlock_p->Size;
       U32 Flags = MemoryBlock_p->Flags;
#endif
        memset(&tempPCR, 0x00, sizeof(STCLKRV_ExtendedSTC_t));
        //check for PTS
        if(PlayerAVsync_p->StartupSyncCount && (!PlayerAVsync_p->StartSync))
        {
            PlayerAVsync_p->StartupSyncCount--;
            STTBX_Print(("SPDIF:SYNC %d \n",PlayerAVsync_p->StartupSyncCount));
            if(Flags & FREQ_VALID)
            {
                PlayerAVsync_p->Freq = MemoryBlock_p->Data[FREQ_OFFSET];
            }
            else
            {
                PlayerAVsync_p->Freq = Control_p->samplingFrequency;
            }

            if(Flags & PTS_VALID)
            {

                PlayerAVsync_p->CurrentPTS.BaseValue = MemoryBlock_p->Data[PTS_OFFSET];
                PlayerAVsync_p->CurrentPTS.BaseBit32 = MemoryBlock_p->Data[PTS33_OFFSET];
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
                    switch (Control_p->SPDIFMode)
                    {
                        case STAUD_DIGITAL_MODE_COMPRESSED:
                            ADDToEPTS(&(PlayerAVsync_p->CurrentPTS), MILLISECOND_TO_PTS((Control_p->CurrentBurstPeriod * 1000)/PlayerAVsync_p->Freq));
                            break;

                        case STAUD_DIGITAL_MODE_NONCOMPRESSED:
                            ADDToEPTS(&(PlayerAVsync_p->CurrentPTS), MILLISECOND_TO_PTS(((MemorySize/ (Control_p->NumChannels * BYTES_PER_CHANNEL_IN_PLAYER)) * 1000)/PlayerAVsync_p->Freq));
                            break;

                        default:
                            break;
                    }
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

                    ADDToEPTS(&CurrentPTS, MILLISECOND_TO_PTS((Control_p->audioSTCSync.Offset + Control_p->audioSTCSync.UsrLatency)));
                    PTSSTCDiff = EPTS_DIFF(CurrentPTS,tempPCR);

                    DoSync = FALSE;
                    if (EPTS_LATER_THAN(CurrentPTS, tempPCR))
                    {
                        if (PTSSTCDiff <= HUGE_TIME_DIFFERENCE_PAUSE) DoSync = TRUE;
                    }
                    else
                    {
                        if (PTSSTCDiff <= HUGE_TIME_DIFFERENCE_SKIP) DoSync = TRUE;
                    }

                    if(DoSync == TRUE/*PTSSTCDiff <= HUGE_TIME_DIFFERENCE*/)
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

                            STTBX_Print(("SPDIFP:Delay %d,%d\n",PTS_TO_MILLISECOND(PTSSTCDiff),PlayerAVsync_p->StartupSyncCount));
                            PlayerAVsync_p->StartSync = TRUE;
                        }
                        else if(EPTS_LATER_THAN(tempPCR,CurrentPTS))
                            {
                                /*We are going to drop the frame so update any characteristics that have to updated*/
                                if ((Flags & FREQ_VALID) && (Control_p->samplingFrequency != MemoryBlock_p->Data[FREQ_OFFSET]))
                                {
                                    // Sampling frequency changed this should ideally occur only at the begining of playback
                                    Control_p->samplingFrequency = MemoryBlock_p->Data[FREQ_OFFSET];
                                    STTBX_Print(("Setting new SPDIF frequency %d\n",Control_p->samplingFrequency));
                                    SetSPDIFPlayerSamplingFrequency(Control_p);
                                    SetSPDIFChannelStatus(Control_p);
                                }

                                if ((Flags & LAYER_VALID) && (Control_p->mpegLayer != (enum MPEG)((U32)MemoryBlock_p->AppData_p)))
                                {
                                    Control_p->mpegLayer = (enum MPEG)((U32)MemoryBlock_p->AppData_p);
                                    STTBX_Print(("Setting new mpeg layer %d\n",Control_p->mpegLayer));
                                    /*SetSPDIFStreamParams(Control_p);*/
                                    UpdateBurstPeriod(Control_p);
                                }

                                if((MemPool_FreeIpBlk(Control_p->memoryBlockManagerHandle, (U32 *)(& MemoryBlock_p), (U32)SPDIFPlayerControlBlock_p))!= ST_NO_ERROR)
                                {
                                    STTBX_Print(("SPDIF:err in freeing startsync \n"));
                                    PlayerAVsync_p->StartSync = TRUE;
                                    return (TRUE);
                                }
                                else
                                {
                                    STTBX_Print(("SPDIF:drop %d,%d\n",PTS_TO_MILLISECOND(PTSSTCDiff),PlayerAVsync_p->StartupSyncCount));
                                }
                                STOS_SemaphoreSignal(Control_p->spdifPlayerTaskSem_p);
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

#endif // #if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
#endif // #if ENABLE_STARTUP_SYNC
    return (TRUE);
}

void SPDIFPlayer_SwapFDMANodeParams(SPDIFPlayerControl_t * Control_p)
{
    STFDMA_SPDIFNode_t  * SPDIFNode_p;

    if ((Control_p->SPDIFMode == STAUD_DIGITAL_MODE_COMPRESSED) && (Control_p->CompressedDataAlignment == BE))
    {
        U32 PreambleA = 0x72F8, i;
        U32 PreambleB = 0x1F4E;
        /*U32 PreambleC ;*/

        /*SPDIFNode_p = &(Control_p->PlayerAudioBlock_p[0].PlayerDmaNode_p->SPDIFNode);*/

        /*PreambleC = (((PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleC & 0x00FF) << 8) |
                        ((PlayerDmaNode_p->SPDIFNode.Data.CompressedMode.PreambleC & 0xFF00) >> 8)) ;*/

        for (i=0; i < Control_p->spdifPlayerNumNode; i++)
        {
            SPDIFNode_p = &(Control_p->PlayerAudioBlock_p[i].PlayerDmaNode_p->SPDIFNode);
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5162)
            SPDIFNode_p->Data.CompressedMode.PreambleA = PreambleA;
            SPDIFNode_p->Data.CompressedMode.PreambleB = PreambleB;
#else
            SPDIFNode_p->PreambleA = PreambleA;
            SPDIFNode_p->PreambleB = PreambleB;
#endif
            //SPDIFNode_p->Data.CompressedMode.PreambleC = PreambleC;

        }
        SetSPDIFPreambleC(Control_p);
    }
}

#if ENABLE_SYNC_CALC_IN_CALLBACK
void CalculateSPDIFSync(SPDIFPlayerControl_t * Control_p)
{
    AudSTCSync_t * AudioSTCSync_p = &(Control_p->audioSTCSync);
    AudAVSync_t * PlayerAVsync_p = &(Control_p->audioAVsync);

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    SPDIFPTSPCROffsetInms(Control_p, PlayerAVsync_p->CurrentPTS);
#endif

    if (AudioSTCSync_p->SkipPause != 0)
    {
        UpdateSPDIFSynced(Control_p);

        if(Control_p->skipDuration == 0)
        {
            Control_p->skipDuration = AudioSTCSync_p->SkipPause;
        }
        SPDIFPlayer_NotifyEvt(STAUD_OUTOF_SYNC,0,Control_p);
        PlayerAVsync_p->SyncCompleted = FALSE;
        PlayerAVsync_p->InSyncCount = 0;
    }

    if (AudioSTCSync_p->PauseSkip != 0)
    {
        UpdateSPDIFSynced(Control_p);

        if (Control_p->pauseDuration == 0)
        {
            Control_p->pauseDuration = AudioSTCSync_p->PauseSkip;
        }
        SPDIFPlayer_NotifyEvt(STAUD_OUTOF_SYNC,0,Control_p);
        PlayerAVsync_p->SyncCompleted = FALSE;
        PlayerAVsync_p->InSyncCount = 0;
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
            SPDIFPlayer_NotifyEvt(STAUD_IN_SYNC,0,Control_p);
            //STTBX_Print(("Sync completed for Live decode\n"));
        }

    }
}
#endif

ST_ErrorCode_t SPDIFReleaseMemory(SPDIFPlayerControlBlock_t * ControlBlock_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(ControlBlock_p)
    {
        SPDIFPlayerControl_t * Control_p = &(ControlBlock_p->spdifPlayerControl);
        ST_Partition_t * CPUPartition = Control_p->CPUPartition;
        STAVMEM_FreeBlockParams_t FreeBlockParams;

        if(Control_p->PlayerAudioBlock_p)
        {
            memory_deallocate(CPUPartition,Control_p->PlayerAudioBlock_p);
            Control_p->PlayerAudioBlock_p = NULL;
        }

        if(Control_p->SPDIFPlayerNodeHandle)
        {
            FreeBlockParams.PartitionHandle = Control_p->AVMEMPartition;
            Error |= STAVMEM_FreeBlock(&FreeBlockParams, &(Control_p->SPDIFPlayerNodeHandle));
            Control_p->SPDIFPlayerNodeHandle = 0;
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
            else if(Control_p->tp)
            {
                Error |= EMBX_Free((EMBX_VOID *)Control_p->dummyBufferBaseAddress);
                Error |= EMBX_CloseTransport(Control_p->tp);
            }
#endif
            Control_p->dummyBufferBaseAddress = NULL;
            Control_p->dummyBufferSize = 0;
        }

        /* Unlock FDMA Channel*/

        Error |= STFDMA_UnlockChannel(Control_p->spdifPlayerChannelId , Control_p->FDMABlock);

        if(Control_p->spdifPlayerTaskSem_p)
        {
            STOS_SemaphoreDelete(NULL,Control_p->spdifPlayerTaskSem_p);
            Control_p->spdifPlayerTaskSem_p = NULL;
        }

        if(Control_p->LockControlStructure)
        {
            STOS_MutexDelete(Control_p->LockControlStructure);
            Control_p->LockControlStructure = NULL;
        }

        if(Control_p->spdifPlayerCmdTransitionSem_p)
        {
            STOS_SemaphoreDelete(NULL,Control_p->spdifPlayerCmdTransitionSem_p);
            Control_p->spdifPlayerCmdTransitionSem_p = NULL;
        }

        if(Control_p->EvtHandle)
        {
            Error |= SPDIFPlayer_UnRegisterEvents(Control_p);
            Error |= SPDIFPlayer_UnSubscribeEvents(Control_p);
            // Close evt driver
            Error |= STEVT_Close(Control_p->EvtHandle);
            Control_p->EvtHandle = 0;
        }

        /*release control block*/
        memory_deallocate(CPUPartition,ControlBlock_p);
    }

    return Error;
}


#ifdef HDMISPDIFP0_SUPPORTED
BOOL HDMISPDIF_TestSampFreq (PlayerAudioBlock_t *AudioBlock_p , SPDIFPlayerControlBlock_t * ControlBlock_p)
{
    BOOL Error = TRUE;
    MemoryBlock_t * MemoryBlock_p = AudioBlock_p->memoryBlock;
    SPDIFPlayerControl_t * Control_p = &ControlBlock_p->spdifPlayerControl;
    U32 SamplingFrequency;

#if MEMORY_SPLIT_SUPPORT
    U32 Flags = AudioBlock_p->MemoryParams.Flags;
#else
    U32 Flags = AudioBlock_p->memoryBlock->Flags;
#endif

    if (Control_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DDPLUS
            && Control_p->SPDIFMode == STAUD_DIGITAL_MODE_COMPRESSED
            && Control_p->spdifPlayerIdentifier == HDMI_SPDIF_PLAYER_0)
    {
        if (Flags & FREQ_VALID)
        {
            SamplingFrequency = MemoryBlock_p->Data[FREQ_OFFSET];
            Control_p->samplingFrequency = SamplingFrequency; //Mark current valid recieved frequency
        }

        if(Control_p->samplingFrequency != 176400 && Control_p->samplingFrequency != 192000)
        {
            if ((MemPool_FreeIpBlk(Control_p->memoryBlockManagerHandle, (U32 *)(& MemoryBlock_p), (U32)ControlBlock_p))!= ST_NO_ERROR)
            {
                STTBX_Print(("SPDIF:err in freeing hdmi spdif ddplus sampling frequency not 17600 and 19200 \n"));
            }
            AudioBlock_p->Valid = FALSE;
            Error = FALSE;
        }
    }
    return Error;

}
#endif

