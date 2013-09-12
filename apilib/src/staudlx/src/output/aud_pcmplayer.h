/*
 * Copyright (C) STMicroelectronics Ltd. 2002.
 *
 * All rights reserved.
 *
 *                              ------->SPDIF PLAYER
 *                             |
 * DECODER -------|
 *                             |
 *                              ------->PCM PLAYER
 *
 */

#ifndef __AUDPCMPLAYER_H
#define __AUDPCMPLAYER_H

#include "staud.h"
#include "aud_drv.h"

#ifndef ST_51XX
#include "acc_multicom_toolbox.h"
#else
#include "aud_utils.h"
#endif

#include "pcm_spdif_reg.h"
#include "blockmanager.h"
#include "aud_evt.h"

/* Install PCM interrupt */
#define ENABLE_PCM_INTERRUPT    0

/* PCM Player task stack size */
#if defined(ST_51XX)
#define PCMPLAYER_STACK_SIZE    (1024 * 2)
#else
#define PCMPLAYER_STACK_SIZE    (1024 * 16)
#endif

/* PCM Player task priority */
#ifndef PCMPLAYER_TASK_PRIORITY
#define PCMPLAYER_TASK_PRIORITY     10
#endif

/* FDMA request Line Number */
#if defined (ST_7100)
#define PCM0_REQ_LINE               26
#define PCM1_REQ_LINE               27
#elif defined (ST_7109)
#define PCM0_REQ_LINE               24
#define PCM1_REQ_LINE               25
#elif defined (ST_7200)
#define PCM0_REQ_LINE               23
#define PCM1_REQ_LINE               24
#define PCM2_REQ_LINE               25
#define PCM3_REQ_LINE               26
#define HDMI_PCM0_REQ_LINE     29
#elif defined (ST_51XX)
#define PCM0_REQ_LINE                5
#endif

enum PCMPlayerIdentifier
{
    PCM_PLAYER_0,
    PCM_PLAYER_1,
    PCM_PLAYER_2,
    PCM_PLAYER_3,
    HDMI_PCM_PLAYER_0,
    MAX_PCM_PLAYER_INSTANCE
};

/*#define NUM_NODES_DECODER 6*/

typedef U32 STPCMPLAYER_Handle_t;

typedef enum PCMPlayerState_e
{
    PCMPLAYER_INIT,
    PCMPLAYER_START,
    PCMPLAYER_RESTART,
    PCMPLAYER_PLAYING,
    PCMPLAYER_STOPPED,
    PCMPLAYER_TERMINATE
} PCMPlayerState_t;

typedef struct PlayerAudioBlock_s
{
    STFDMA_GenericNode_t    * PlayerDmaNode_p;
    U32                     *PlayerDMANodePhy_p;
    BOOL                        Valid;
    MemoryBlock_t           * memoryBlock;
#if MEMORY_SPLIT_SUPPORT
    MemoryParams_t          MemoryParams;
#endif
}PlayerAudioBlock_t;

typedef struct PCMPlayerControl_s
{
    ST_DeviceName_t                     Name;
    enum PCMPlayerIdentifier         pcmPlayerIdentifier;    // Identify which player we are using (should be used for baseaddress identification)
    PCMPlayerState_t                      pcmPlayerCurrentState;
    PCMPlayerState_t                      pcmPlayerNextState;

    STFDMA_TransferId_t                pcmPlayerTransferId;
    STFDMA_ChannelId_t                pcmPlayerChannelId;
    STFDMA_TransferGenericParams_t  TransferPlayerParams_p;

    task_t                          * pcmPlayerTask_p;
#if defined (ST_OS20)
    U8                              * pcmPlayerTask_pstack;
    tdesc_t                         pcmPlayerTaskDesc;
#endif

    semaphore_t                     * pcmPlayerTaskSem_p;
    semaphore_t                     * pcmPlayerCmdTransitionSem_p;
    ST_Partition_t                    * CPUPartition;
    STAVMEM_PartitionHandle_t   AVMEMPartition;
    STAVMEM_PartitionHandle_t   DummyBufferPartition;
    STAVMEM_BlockHandle_t       DummyBufferHandle;
    STAVMEM_BlockHandle_t       PCMPlayerNodeHandle;

    STMEMBLK_Handle_t             memoryBlockManagerHandle;
    PlayerAudioBlock_t            * PlayerAudioBlock_p;
    STOS_Mutex_t                  * LockControlStructure;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_Handle_t                CLKRV_Handle;
    STCLKRV_Handle_t                CLKRV_HandleForSynchro;
#endif
    STAUD_PCMDataConf_t        pcmPlayerDataConfig;
    U32                                     samplingFrequency;
#ifndef STAUD_REMOVE_EMBX
    EMBX_TRANSPORT                  tp;
#endif
    U32                              * dummyBufferBaseAddress;
    U32                                 dummyBufferSize;
    U32                              * pcmPlayerFDMANodesBaseAddress;
    U32                                 pcmPlayerNumNode;       // Num of Nodes in this PCM player
    U32                                 pcmPlayerPlayed;           // Num of nodes played by this player
    U32                                 pcmPlayerToProgram;    // Next node to program for this player

    ST_DeviceName_t            EvtHandlerName;
    STEVT_Handle_t               EvtHandle;

    ST_DeviceName_t            EvtGeneratorName;       // Module which is generating skip/pause event on which this player will skip/pause
    STAUD_Object_t               ObjectID;
    STEVT_EventID_t             EventIDEOF;

    U32                                 skipEventID;            // Events on which skip and pause actions have to be taken
    U32                                 pauseEventID;
    U32                                 skipDuration;           // Used to skip in PCM player
    U32                                 pauseDuration;        // Used to pause in PCM player
    STAUD_PCMMode_t          AnalogMode;

    /*AVSYNC*/
    AudAVSync_t                   audioAVsync;            // Required to generate PAUSE/SKIP events
    AudSTCSync_t                 audioSTCSync;         // Required to generate PAUSE/SKIP events

    BOOL                              AVSyncEnable;
    BOOL                              pcmRestart;            // Flag to restart the player
    BOOL                              pcmMute;                // Mute PCM player by sending data blocks containing NULL
    BOOL                              HDMI_PcmPlayerEnable;  //Mark whether HDMI pcm player is enabled or disabled

    BOOL                              playBackFromLive;
    U32                                CurrentFrameSize;
    STFDMA_Block_t              FDMABlock;                 /* FDMA Silicon used */
    U32                                FDMAAbortSent;          /* Total FDMA Aborts Sent */
    U32                                FDMAAbortCBRecvd;   /* Total FDMA Abort callback received */
    BOOL                              AbortSent;
    U8                                  NumChannels;
    BOOL                              EOFBlock_Received;
    BOOL                              EnableMSPPSupport;
    U32                                 Pause_Performed;
    U32                                 Skip_Performed;
    U32                                 Huge_Diff;
    U32                                 Underflow_Count;    
    /*For 32 Bit Support*/
    STAud_Memory_Address_t          PlayerInputMemoryAddress;
    STAud_Memory_Address_t          DummyBuffer;
    
#ifdef MSPP_PARSER
    STFDMA_GenericNode_t            * PCMPlayerDmaNode_p;
    STAud_Memory_Address_t          MemcpyNodeAddress;
#endif
    U32                                           InterruptNumber;
    AudioBaseAddress_t                   BaseAddress;
}PCMPlayerControl_t;

typedef struct PCMPlayerControlBlock_s
{
    STPCMPLAYER_Handle_t                handle;
    PCMPlayerControl_t                      pcmPlayerControl;
    struct PCMPlayerControlBlock_s  * next;
}PCMPlayerControlBlock_t;

typedef struct PCMPlayerInitParams_s
{
    enum PCMPlayerIdentifier        pcmPlayerIdentifier;
    U32                                         pcmPlayerNumNode; // Num of Nodes in this PCM player
    ST_Partition_t                        * CPUPartition;
    STAUD_PCMDataConf_t             pcmPlayerDataConfig;
    STMEMBLK_Handle_t                 memoryBlockManagerHandle;
    STAVMEM_PartitionHandle_t       AVMEMPartition;
    STAVMEM_PartitionHandle_t       DummyBufferPartition;
    U32                                         dummyBufferSize;
    U32                                      * dummyBufferBaseAddress;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_Handle_t                  CLKRV_Handle;
    STCLKRV_Handle_t                  CLKRV_HandleForSynchro;
#endif
    ST_DeviceName_t                    EvtHandlerName;
    ST_DeviceName_t                    EvtGeneratorName; // Module which is generating skip/pause event on which this player will skip/pause
    STAUD_Object_t                      ObjectID;
    U32                                        skipEventID; // Events on which skip and pause actions have to be taken
    U32                                        pauseEventID;
    U8                                          NumChannels;
    BOOL                                      EnableMSPPSupport;
}PCMPlayerInitParams_t;

ST_ErrorCode_t      STAud_PCMPlayerInit(ST_DeviceName_t  Name, STPCMPLAYER_Handle_t *handle, PCMPlayerInitParams_t *Param_p);
ST_ErrorCode_t      STAud_PCMPlayerStart(STPCMPLAYER_Handle_t handle);
ST_ErrorCode_t      STAud_PCMPlayerStop(STPCMPLAYER_Handle_t handle);
ST_ErrorCode_t      STAud_PCMPlayerTerminate(STPCMPLAYER_Handle_t handle);
ST_ErrorCode_t      STAud_PCMPlayerSetCmd(STPCMPLAYER_Handle_t handle, PCMPlayerState_t State);
ST_ErrorCode_t      STAud_PCMPlayerPlaybackFromLive(STPCMPLAYER_Handle_t handle, BOOL Playbackfromlive);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t      STAud_PCMPlayerSetClkSynchro(STPCMPLAYER_Handle_t handle, STCLKRV_Handle_t Clksource);
#endif
ST_ErrorCode_t      STAud_PCMPlayerMute(STPCMPLAYER_Handle_t handle, BOOL   PCMMute);
ST_ErrorCode_t      STAud_PCMPlayerGetCapability(STPCMPLAYER_Handle_t handle, STAUD_OPCapability_t * Capability_p);
ST_ErrorCode_t      STAud_PCMPlayerSetOutputParams(STPCMPLAYER_Handle_t handle, STAUD_PCMDataConf_t PCMOutParams);
ST_ErrorCode_t      STAud_PCMPlayerGetOutputParams(STPCMPLAYER_Handle_t handle, STAUD_PCMDataConf_t * PCMOutParams);
ST_ErrorCode_t      STAud_PCMEnableHDMIOutput(STPCMPLAYER_Handle_t handle, BOOL Command);
ST_ErrorCode_t      STAud_PCMPlayerSetAnalogMode(STPCMPLAYER_Handle_t handle, STAUD_PCMMode_t AnalogMode);
ST_ErrorCode_t      STAud_PCMPlayerGetAnalogMode(STPCMPLAYER_Handle_t handle, STAUD_PCMMode_t   * AnalogMode);
ST_ErrorCode_t      SetPCMPlayerSamplingFrequency(PCMPlayerControl_t * Control_p);
ST_ErrorCode_t      PCMPlayer_SetFrequencySynthesizer (PCMPlayerControl_t * Control_p, U32 samplingFrequency);
ST_ErrorCode_t      STAud_PCMPlayerSetOffset(STPCMPLAYER_Handle_t PlayerHandle, S32 Offset);
ST_ErrorCode_t      STAud_PCMPlayerGetOffset(STPCMPLAYER_Handle_t PlayerHandle, S32 *Offset_p);
ST_ErrorCode_t      STAud_PCMPlayerSetLatency(STPCMPLAYER_Handle_t PlayerHandle, U32 Latency);

ST_ErrorCode_t      STAud_PCMPlayerAVSyncCmd(STPCMPLAYER_Handle_t PlayerHandle,BOOL Command);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t      PTSPCROffsetInms(PCMPlayerControl_t *ControlBlock_p,STCLKRV_ExtendedSTC_t CurrentInputPTS);
#endif
ST_ErrorCode_t      PCMPFreeIpBlk(PCMPlayerControlBlock_t * tempPcmPlayerControlBlock_p, PlayerAudioBlock_t * PlayerAudioBlock_p);
ST_ErrorCode_t      STAud_OPSetScenario(STAUD_Scenario_t Scenario);
ST_ErrorCode_t      STAud_PlayerMemRemap(AudioBaseAddress_t *BaseAddress);
ST_ErrorCode_t      STAud_PlayerMemUnmap(AudioBaseAddress_t *BaseAddress);


// PCM player event handler and registration/subscription/notification functions
void                       PCMPlayer_EventHandler(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p);
ST_ErrorCode_t      PCMPlayer_RegisterEvents(PCMPlayerControl_t * Control_p);
ST_ErrorCode_t      PCMPlayer_UnRegisterEvents(PCMPlayerControl_t * Control_p);
ST_ErrorCode_t      PCMPlayer_SubscribeEvents(PCMPlayerControl_t * Control_p);
ST_ErrorCode_t      PCMPlayer_UnSubscribeEvents(PCMPlayerControl_t * Control_p);
ST_ErrorCode_t      Player_NotifyEvt(U32 EvtType, U32 Value, PCMPlayerControl_t * Control_p );
ST_ErrorCode_t      PCMPlayerCheckStateTransition(PCMPlayerControl_t *Control_p, PCMPlayerState_t State);

void        PCMPlayerTask(void * tempPCMplayerControlBlock_p);
void        PCMPlayerFDMACallbackFunction(U32 TransferId, U32 CallbackReason, U32 *CurrentNode_p, U32 NodeBytesRemaining,BOOL Error,void *ApplicationData_p,clock_t  InterruptTime);

void        StartPCMPlayerIP(PCMPlayerControl_t * Control_p);
void        StopPCMPlayerIP(PCMPlayerControl_t * Control_p);

void        UpdatePCMPlayerNodeSize(PlayerAudioBlock_t * PlayerAudioBlock_p, PCMPlayerControl_t * Control_p);
void        FillPCMPlayerFDMANode(PlayerAudioBlock_t * PlayerAudioBlock_p, PCMPlayerControl_t * Control_p);

void        ResetPCMPlayerSyncParams(PCMPlayerControl_t *Control_p);
void        ResetPCMPLayerIP(PCMPlayerControl_t * Control_p);

void        UpdatePCMSynced(PCMPlayerControl_t *Control_p);
U32         PCMPlayer_ConvertFsMultiplierToClkDivider(U32 FrequencyMultiplier);
BOOL      PCMPlayer_StartupSync(PlayerAudioBlock_t * PlayerAudioBlock_p, PCMPlayerControlBlock_t * PCMPlayerControlBlock_p);
void        CalculatePCMPSync(PCMPlayerControl_t * Control_p);
void        PCMPlayer_DACConfigure(PCMPlayerControl_t * Control_p);

#if ENABLE_PCM_INTERRUPT
#if defined( ST_OSLINUX )
irqreturn_t PcmPInterruptHandler( int irq, void *Data, struct pt_regs *regs);
#else
#ifdef ST_51XX
void         PcmPInterruptHandler(void* pParams);
#elif defined (ST_OS21)
int           PcmPInterruptHandler(void* pParams);
#endif
#endif
void        PCMPlayerSetInterruptNumber(PCMPlayerControl_t * Control_p);
void        PCMPlayerEnableInterrupt(PCMPlayerControl_t * Control_p);
void        PCMPlayerDisbleInterrupt(PCMPlayerControl_t * Control_p);
#endif
#endif /*#define __AUDPCMPLAYER_H*/


