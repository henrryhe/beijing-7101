/*
 * Copyright (C) STMicroelectronics Ltd. 2002.
 *
 * All rights reserved.
 *
 * D program
 *
 */

#ifndef __AUDSPDIFPLAYER_H
#define __AUDSPDIFPLAYER_H

#include "staud.h"
#include "audioreg.h"
#include "aud_drv.h"
#ifndef ST_51XX
#include "acc_multicom_toolbox.h"
#else
#include "aud_utils.h"
#endif
#include "pcm_spdif_reg.h"
#include "blockmanager.h"
#include "aud_pcmplayer.h"
#include "aud_evt.h"

/* SPDIF Player task stack size */
#if defined(ST_51XX)
    #define SPDIFPLAYER_STACK_SIZE     (1024 * 2)
#else
    #define SPDIFPLAYER_STACK_SIZE     (1024 * 16)
#endif

/* SPDIF Player task priority */
#ifndef SPDIFPLAYER_TASK_PRIORITY
    #define SPDIFPLAYER_TASK_PRIORITY   10
#endif

/* Debug Information */
#if defined( ST_OSLINUX )
    /* Install SPDIF interrupt */
    #define LINUX_SPDIF_INTERRUPT   0    
#endif //ST_OSLINUX

#define CATEGORY_CODE_BROADCAST_A_MASK                 0x07
#define CATEGORY_CODE_BROADCAST_A_DATA                 0x04
#define CATEGORY_CODE_BROADCAST_B_MASK                 0x0F
#define CATEGORY_CODE_BROADCAST_B_DATA                 0x0E
#define CATEGORY_CODE_OPTICAL_MASK                          0x07
#define CATEGORY_CODE_OPTICAL_DATA                          0x01
#define CATEGORY_CODE_ORIGINAL_DATA                        0x80

#define CATEGORY_CODE_OPTICAL_DATA_958                  0x01
#define CATEGORY_CODE_OPTICAL_DATA_61937              0x19


enum SPDIFPlayerIdentifier
{
    SPDIF_PLAYER_0,
    HDMI_SPDIF_PLAYER_0,
    MAX_SPDIF_PLAYER_INSTANCE
};


/*#define NUM_NODES_DECODER 6*/

typedef U32 STSPDIFPLAYER_Handle_t;

extern STSPDIFPLAYER_Handle_t SPDIFPLAYER_Handle[];

typedef enum SPDIFPlayerState_e
{
    SPDIFPLAYER_INIT,
    SPDIFPLAYER_START,
    SPDIFPLAYER_RESTART,
    SPDIFPLAYER_PLAYING,
    SPDIFPLAYER_STOPPED,
    SPDIFPLAYER_TERMINATE
} SPDIFPlayerState_t;

typedef struct SPDIFPlayerControl_s
{
    ST_DeviceName_t                            Name;

    enum SPDIFPlayerIdentifier              spdifPlayerIdentifier;  // Identify which player we are using (should be used for baseaddress identification)
    SPDIFPlayerState_t                          spdifPlayerCurrentState;
    SPDIFPlayerState_t                          spdifPlayerNextState;

    STFDMA_TransferId_t                       spdifPlayerTransferId;
    STFDMA_ChannelId_t                       spdifPlayerChannelId;
    STFDMA_TransferGenericParams_t    TransferPlayerParams_p;

    task_t                                             * spdifPlayerTask_p;
#if defined (ST_OS20)
    U8                                                  * spdifPlayerTask_pstack;
    tdesc_t                                            spdifPlayerTaskDesc;
#endif

    semaphore_t                                   * spdifPlayerTaskSem_p;
    semaphore_t                                   * spdifPlayerCmdTransitionSem_p;
    ST_Partition_t                                  * CPUPartition;
    STAVMEM_PartitionHandle_t              AVMEMPartition;
    STAVMEM_PartitionHandle_t              DummyBufferPartition;
    STAVMEM_BlockHandle_t                  DummyBufferHandle;
    STAVMEM_BlockHandle_t                  SPDIFPlayerNodeHandle;

    STMEMBLK_Handle_t           memoryBlockManagerHandle;
    STMEMBLK_Handle_t           pcmMemoryBlockManagerHandle;// pcm block manager handle
    STMEMBLK_Handle_t           compressedMemoryBlockManagerHandle0;// compressed block manager handle for data from parser/enc
    STMEMBLK_Handle_t           compressedMemoryBlockManagerHandle1;// compressed block manager handle for transcoded data (from decoder)
    STMEMBLK_Handle_t           compressedMemoryBlockManagerHandle2;// compressed block manager handle for encoded data (from encoder)

    STAud_Memory_Address_t      PlayerInputMemoryAddressPCM;
    STAud_Memory_Address_t      PlayerInputMemoryAddressCom0;
    STAud_Memory_Address_t      PlayerInputMemoryAddressCom1;
    STAud_Memory_Address_t      PlayerInputMemoryAddressCom2;
    STAud_Memory_Address_t      DummyMemoryAddress;

    STAud_Memory_Address_t      PlayerCurrentMemoryAddress;
    PlayerAudioBlock_t                 * PlayerAudioBlock_p;
    STOS_Mutex_t                       * LockControlStructure;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_Handle_t                CLKRV_Handle;
    STCLKRV_Handle_t                CLKRV_HandleForSynchro;
#endif
    U32                                      samplingFrequency;
#ifndef STAUD_REMOVE_EMBX
    EMBX_TRANSPORT                tp;
#endif
    U32                                      * dummyBufferBaseAddress;
    U32                                      dummyBufferSize;
    U32                                      * spdifPlayerFDMANodesBaseAddress;
    U32                                      spdifPlayerNumNode;         // Num of Nodes in this SPDIF player
    U32                                      spdifPlayerPlayed;             // Num of nodes played by this player
    U32                                      spdifPlayerToProgram;      // Next node to program for this player

    STAUD_DigitalMode_t             SPDIFMode;
    STAUD_SPDIFOutParams_t     SPDIFPlayerOutParams;
    BOOL                                    CopyPermitted;                  // Copyright based on SPDIF player output params
    U8                                        CategoryCode;                   // Category code based on SPDIF player output paramss
    U32                                      CurrentBurstPeriod;           // Current Burst period based on audio type

    STAUD_StreamParams_t        StreamParams;
    MPEGAudioLayer_t                 mpegLayer;                 // Mpeg layer information
    ST_DeviceName_t                 EvtHandlerName;
    STEVT_Handle_t                    EvtHandle;

    ST_DeviceName_t                 EvtGeneratorName;    // Module which is generating skip/pause event on which this player will skip/pause
    STAUD_Object_t                   ObjectID;
    U32                                     skipEventID;               // Events on which skip and pause actions have to be taken
    U32                                     pauseEventID;
    U32                                     skipDuration;              // Used to skip in PCM player
    U32                                     pauseDuration;           // Used to pause in PCM player

    /*AVSYNC*/
    AudAVSync_t                       audioAVsync;             // Required to generate PAUSE/SKIP events
    AudSTCSync_t                     audioSTCSync;           // Required to generate PAUSE/SKIP events

    BOOL                                  AVSyncEnable;

    BOOL                                  spdifRestart;                         // Flag to restart the player
    BOOL                                  spdifMute;
    BOOL                                  spdifForceRestart;
    BOOL                                  HDMI_SPDIFPlayerEnable;  //Mark whether HDMI spdif player is enabled or disabled
    
    U32                                    underFlowCount;
#if defined (ST_OS21)
    interrupt_t*                         hInterrupt;
#endif
    STFDMA_Block_t                 FDMABlock;                   /* FDMA Silicon used */
    U32                                   FDMAAbortSent;            /* Total FDMA Aborts Sent */
    U32                                    FDMAAbortCBRecvd;     /* Total FDMA Abort callback received */
    BOOL                                 AbortSent;
    U8                                     NumChannels;
    BOOL                                 EOFBlock_Received;
    DataAlignment_t                 CompressedDataAlignment;
    BOOL                                 EnableMSPPSupport;
    BOOL                                 EnableEncOutput;
    BOOL                                 BypassMode;
    /* Debug info variables */
    U32                                   Spdif_Pause_Performed;
    U32                                   Spdif_Skip_Performed;
    U32                                   Spdif_Huge_Diff;

    /* Parameters to be updated based on mode and data source*/
    U32                                   bytesPerChannel;        //BYTES_PER_CHANNEL_IN_PLAYER;
    U32                                   shiftPerChannel;
    U32                                   shiftForTwoChannel;   //SHIFTS_FOR_TWO_CHANNEL;

#ifdef MSPP_PARSER
    STFDMA_GenericNode_t        * SPDIFPlayerDmaNode_p;
    STAud_Memory_Address_t    MemcpyNodeAddress;
#endif
    AudioBaseAddress_t              BaseAddress;

}SPDIFPlayerControl_t;



typedef struct SPDIFPlayerControlBlock_s
{
    STSPDIFPLAYER_Handle_t              handle;
    SPDIFPlayerControl_t                     spdifPlayerControl;
    struct SPDIFPlayerControlBlock_s  * next;
}SPDIFPlayerControlBlock_t;

typedef struct SPDIFPlayerInitParams_s
{
    enum SPDIFPlayerIdentifier      spdifPlayerIdentifier;
    U32                                         spdifPlayerNumNode;                     // Num of Nodes in this SPDIF player
    ST_Partition_t                          * CPUPartition;
    STMEMBLK_Handle_t                 pcmMemoryBlockManagerHandle;  // pcm block manager handle
    STMEMBLK_Handle_t                 compressedMemoryBlockManagerHandle0;// compressed block manager handle for data from parser/enc
    STMEMBLK_Handle_t                 compressedMemoryBlockManagerHandle1;// compressed block manager handle for transcoded data (from decoder)
    STMEMBLK_Handle_t                 compressedMemoryBlockManagerHandle2;// compressed block manager handle for encoded data (from encoder)
    STAVMEM_PartitionHandle_t       AVMEMPartition;
    STAVMEM_PartitionHandle_t       DummyBufferPartition;
    STAUD_DigitalMode_t                SPDIFMode;
    STAUD_SPDIFOutParams_t        SPDIFPlayerOutParams;
    U32                                         dummyBufferSize;
    U32                                         * dummyBufferBaseAddress;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_Handle_t                  CLKRV_Handle;
    STCLKRV_Handle_t                  CLKRV_HandleForSynchro;
#endif
    ST_DeviceName_t                   EvtHandlerName;
    ST_DeviceName_t                   EvtGeneratorName;      // Module which is generating skip/pause event on which this player will skip/pause
    STAUD_Object_t                      ObjectID;
    U32                                        skipEventID;                 // Events on which skip and pause actions have to be taken
    U32                                        pauseEventID;
    U8                                          NumChannels;
    BOOL                                      EnableMSPPSupport;
}SPDIFPlayerInitParams_t;

ST_ErrorCode_t      STAud_SPDIFPlayerInit(ST_DeviceName_t Name, STSPDIFPLAYER_Handle_t *handle,SPDIFPlayerInitParams_t *Param_p);
ST_ErrorCode_t      STAud_SPDIFPlayerStart(STSPDIFPLAYER_Handle_t handle, STAUD_StreamParams_t * StreamParams_p);
ST_ErrorCode_t      STAud_SPDIFPlayerStop(STSPDIFPLAYER_Handle_t handle);
ST_ErrorCode_t      STAud_SPDIFPlayerTerminate(STSPDIFPLAYER_Handle_t handle);
ST_ErrorCode_t      STAud_SPDIFPlayerSetCmd(STSPDIFPLAYER_Handle_t handle, SPDIFPlayerState_t State);
ST_ErrorCode_t      STAud_SPDIFPlayerSetStreamParams(STSPDIFPLAYER_Handle_t handle, STAUD_StreamParams_t * StreamParams_p);
ST_ErrorCode_t      STAud_SPDIFPlayerSetSPDIFMode(STSPDIFPLAYER_Handle_t handle, STAUD_DigitalMode_t    SPDIFMode);
ST_ErrorCode_t      STAud_SPDIFPlayerGetSPDIFMode(STSPDIFPLAYER_Handle_t handle, STAUD_DigitalMode_t    * SPDIFMode);
ST_ErrorCode_t      STAud_SPDIFPlayerSetEncOutput(STSPDIFPLAYER_Handle_t handle, BOOL   EnableEncOutput);
ST_ErrorCode_t      STAud_SPDIFPlayerGetEncOutput(STSPDIFPLAYER_Handle_t handle, BOOL   * EnableEncOutput);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t      STAud_SPDIFPlayerSetClkSynchro(STSPDIFPLAYER_Handle_t handle, STCLKRV_Handle_t Clksource);
#endif
ST_ErrorCode_t      STAud_SPDIFPlayerMute(STSPDIFPLAYER_Handle_t handle, BOOL   SPDIFMute);
ST_ErrorCode_t      STAud_SPDIFPlayerGetCapability(STSPDIFPLAYER_Handle_t handle, STAUD_OPCapability_t * Capability_p);
ST_ErrorCode_t      STAud_SPDIFPlayerSetOutputParams(STSPDIFPLAYER_Handle_t handle, STAUD_SPDIFOutParams_t SPDIFOutputParams);
ST_ErrorCode_t      STAud_SPDIFPlayerGetOutputParams(STSPDIFPLAYER_Handle_t handle, STAUD_SPDIFOutParams_t * SPDIFOutputParams_p);
ST_ErrorCode_t      STAud_SPDIFPlayerSetOffset(STSPDIFPLAYER_Handle_t PlayerHandle, S32 Offset);
ST_ErrorCode_t      STAud_SPDIFPlayerGetOffset(STSPDIFPLAYER_Handle_t PlayerHandle, S32 *Offset_p);
ST_ErrorCode_t      STAud_SPDIFPlayerSetLatency(STPCMPLAYER_Handle_t PlayerHandle, U32 Latency);
ST_ErrorCode_t      STAud_SPDIFPlayerAVSyncCmd(STSPDIFPLAYER_Handle_t PlayerHandle, BOOL Command);
ST_ErrorCode_t      STAud_SPDIFEnableHDMIOutput(STSPDIFPLAYER_Handle_t PlayerHandle, BOOL Command);
ST_ErrorCode_t      SetSPDIFPlayerSamplingFrequency(SPDIFPlayerControl_t * Control_p);
ST_ErrorCode_t      SPDIFPlayer_SetFrequencySynthesizer (SPDIFPlayerControl_t * Control_p, U32 samplingFrequency);

void                SPDIFPlayerTask(void * tempSPDIFplayerControlBlock_p);
void                SPDIFPlayerFDMACallbackFunction(U32 TransferId, U32 CallbackReason, U32 *CurrentNode_p, U32 NodeBytesRemaining, BOOL Error, void *ApplicationData_p, clock_t  InterruptTime);

void                StartSPDIFPlayerIP(SPDIFPlayerControl_t * Control_p);
void                StopSPDIFPlayerIP(SPDIFPlayerControl_t * Control_p);

void                UpdateSPDIFPlayerNodeSize(PlayerAudioBlock_t * PlayerAudioBlock_p, SPDIFPlayerControl_t * Control_p);
void                FillSPDIFPlayerFDMANode(PlayerAudioBlock_t * PlayerAudioBlock_p, SPDIFPlayerControl_t * Control_p);


void                SetSPDIFStreamParams(SPDIFPlayerControl_t * Control_p);
void                SetSPDIFPreambleC(SPDIFPlayerControl_t * Control_p);
void                SetSPDIFPlayerCategoryCode(SPDIFPlayerControl_t * Control_p);

// SPDIF player event handler and registration/subscription/notification functions

void                       SPDIFPlayer_EventHandler(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData, const void *SubscriberData_p);
ST_ErrorCode_t      SPDIFPlayer_RegisterEvents(SPDIFPlayerControl_t * Control_p);
ST_ErrorCode_t      SPDIFPlayer_UnRegisterEvents(SPDIFPlayerControl_t * Control_p);
ST_ErrorCode_t      SPDIFPlayer_SubscribeEvents(SPDIFPlayerControl_t * Control_p);
ST_ErrorCode_t      SPDIFPlayer_UnSubscribeEvents(SPDIFPlayerControl_t * Control_p);
ST_ErrorCode_t      SPDIFPlayer_NotifyEvt(U32 EvtType,U32 Value,SPDIFPlayerControl_t *Control_p);
ST_ErrorCode_t      SPDIFPlayerCheckStateTransition(SPDIFPlayerControl_t * Control_p, SPDIFPlayerState_t State);

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t      SPDIFPTSPCROffsetInms(SPDIFPlayerControl_t *Control_p, STCLKRV_ExtendedSTC_t CurrentInputPTS);
#endif

void                ResetSPDIFPlayerSyncParams(SPDIFPlayerControl_t *Control_p);
void                SPDIFFreeBlocks(U32 Played, U32 ToProgram, SPDIFPlayerControlBlock_t * SPDIFPlayerControlBlock_p);
void                UpdateSPDIFSynced(SPDIFPlayerControl_t * Control_p);

U32                CalculateSPDIFChannelStatus0(SPDIFPlayerControl_t * Control_p);
U32                CalculateSPDIFChannelStatus1(SPDIFPlayerControl_t * Control_p);
U32                SPDIFPlayer_ConvertFsMultiplierToClkDivider(U32 FrequencyMultiplier);
BOOL             SPDIFPlayer_StartupSync(PlayerAudioBlock_t * PlayerAudioBlock_p , SPDIFPlayerControlBlock_t * SPDIFPlayerControlBlock_p);
void               SPDIFPlayer_SwapFDMANodeParams(SPDIFPlayerControl_t * Control_p);
void               SetSPDIFChannelStatus(SPDIFPlayerControl_t * Control_p);
void               UpdateBurstPeriod(SPDIFPlayerControl_t * Control_p);
void               CalculateSPDIFSync(SPDIFPlayerControl_t * Control_p);
BOOL             HDMISPDIF_TestSampFreq(PlayerAudioBlock_t *PlayerAudioBlock_p , SPDIFPlayerControlBlock_t * SPDIFPlayerControlBlock_p);

#endif /*#define __AUDSPDIFPLAYER_H*/

