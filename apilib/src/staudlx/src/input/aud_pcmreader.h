/*
 * Copyright (C) STMicroelectronics Ltd. 2002.
 *
 * All rights reserved.
 *
 * D program
 *
 */

#ifndef __AUDPCMREADER_H
#define __AUDPCMREADER_H

#include "staud.h"
#include "aud_drv.h"
#include "acc_multicom_toolbox.h"
#include "pcm_spdif_reg.h"
#include "blockmanager.h"
#include "aud_pcmplayer.h"

/* PCM Reader task stack size */
#define PCMREADER_STACK_SIZE     (1024*16)

/* PCM Reader task priority */
#ifndef PCMREADER_TASK_PRIORITY
#define PCMREADER_TASK_PRIORITY     6
#endif

//#define NUM_SAMPLES_PER_NODE 384

enum PCMReaderIdentifier
{
    PCM_READER_0,
    MAX_PCM_READER_INSTANCE
};


typedef U32 STPCMREADER_Handle_t;

/******************************************************************/
/*            PCM READER CONTROL REGISTER BITS                    */
/******************************************************************/
typedef enum PCMReaderControlMode_e
{
    PCMR_OFF,
    PCMR_MUTE,
    PCMR_PCM_MODE_ON,
    PCMR_CD_MODE_ON
}PCMReaderControlMode_t;

typedef struct PCMReaderControlReg_s
{
    PCMReaderControlMode_t  Mode        :2;
    STAUD_PCMReaderMemFmt_t Format      :1;
    STAUD_PCMReaderRnd_t    Rounding    :1;
    U32                     SamplesRead :28;
}PCMReaderControlReg_t;

typedef union PCMReaderControlR_u
{
    U32                   Reg;
    PCMReaderControlReg_t Bits;
}PCMReaderControlR_t;

/**********************************************************************/
/*             PCM READER FORMAT REGISTER BIT                          */
/**********************************************************************/

typedef enum PCMReaderLRPol_s
{
    PCMR_LEFTWORD_LRCLKLOW,
    PCMR_LEFTWORD_LRCLKHIGH
}PCMReaderLRPol_t;


typedef enum PCMReaderSLKEdg_s
{
    PCMR_RISINGEDGE_PCMSCLK,
    PCMR_FALLINGEDGE_PCMSCLK
}PCMReaderSLKEdg_t;

typedef enum PCMReaderPad_s
{
    PCMR_DELAY_DATA_ONEBIT_CLK,
    PCMR_NODELAY_DATA_BIT_CLK
}PCMReaderPad_t;


typedef enum PCMReaderOrder_s
{
    PCMR_DATA_LSBFIRST,
    PCMR_DATA_MSBFIRST
}PCMReaderOrder_t;

typedef struct PCMReaderFormatReg_s
{
    STAUD_DACNumBitsSubframe_t BitsSubframe :1;
    STAUD_DACDataPrecision_t DataLength     :2;
    PCMReaderLRPol_t LRLevel                :1;
    PCMReaderSLKEdg_t SclkEdge              :1;
    PCMReaderPad_t Padding                  :1;
    STAUD_DACDataAlignment_t Align          :1;
    PCMReaderOrder_t Order                  :1;
    U32 Reserved                            :24;
}PCMReaderFormatReg_t;

typedef union PCMReaderFormatR_u
{
    U32 Reg;
    PCMReaderFormatReg_t Bits;
}PCMReaderFormatR_t;

typedef enum PCMReaderState_e
{
    PCMREADER_INIT,
    PCMREADER_START,
    PCMREADER_RESTART,
    PCMREADER_PAUSE,
    PCMREADER_PLAYING,
    PCMREADER_STOPPED,
    PCMREADER_TERMINATE
} PCMReaderState_t;


typedef struct PCMReaderControl_s
{
    ST_DeviceName_t                Name;
    enum PCMReaderIdentifier       pcmReaderIdentifier;    // Identify which reader we are using (should be used for baseaddress identification)
    PCMReaderState_t               CurrentState;
    PCMReaderState_t               NextState;
    STFDMA_TransferId_t            pcmReaderTransferId;
    STFDMA_ChannelId_t             pcmReaderChannelId;
    STFDMA_TransferGenericParams_t TransferParams;
    task_t                         * pcmReaderTask_p;
    semaphore_t                    * pcmReaderTaskSem_p;
    semaphore_t                    * CmdTransitionSem_p;
    ST_Partition_t                 * CPUPartition;
    STAVMEM_PartitionHandle_t      AVMEMPartition;
    STAVMEM_BlockHandle_t          NodeHandle;
    STMEMBLK_Handle_t              BMHandle;
    STAud_Memory_Address_t         MemoryAddress;
    PlayerAudioBlock_t             * AudioBlock_p;
    STOS_Mutex_t                   * LockControlStructure;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_Handle_t               CLKRV_Handle;
    STCLKRV_Handle_t               CLKRV_HandleForSynchro;
#endif
    STAUD_PCMReaderConf_t          pcmReaderDataConfig;
    U32                            SamplingFrequency;
#ifndef ST_5197
    EMBX_TRANSPORT                 tp;
#endif
    U32                            * FDMANodesBaseAddress_p;
    U32                            pcmReaderNumNode;       // Num of Nodes in this PCM player
    U32                            pcmReaderPlayed;        // Num of nodes played by this player
    U32                            pcmReaderToProgram;     // Next node to program for this player
    ST_DeviceName_t                EvtHandlerName;
    STEVT_Handle_t                 EvtHandle;
    ST_DeviceName_t                EvtGeneratorName;       // Module which is generating skip/pause event on which this player will skip/pause
    U32                            CurrentFrameSize;
    STFDMA_Block_t                 FDMABlock;              /* FDMA Silicon used */
    U32                            FDMAAbortSent;      /* Total FDMA Aborts Sent */
    U32                            FDMAAbortCBRecvd;   /* Total FDMA Abort callback received */
    U8                             NumChannels;
    U8                             BytesPerSample;
    BOOL                           pcmPause;
    BOOL                           pcmRestart;             // Flag to restart the player
    BOOL                           pcmMute;                // Mute PCM player by sending data blocks containing NULL
    AudioBaseAddress_t             BaseAddress;
}PCMReaderControl_t;



typedef struct PCMReaderControlBlock_s
{
    STPCMREADER_Handle_t                handle;
    PCMReaderControl_t                  pcmReaderControl;
    struct PCMReaderControlBlock_s      * next_p;
}PCMReaderControlBlock_t;

typedef struct PCMReaderInitParams_s
{
    enum PCMReaderIdentifier    pcmReaderIdentifier;
    U32                         pcmReaderNumNode;       // Num of Nodes in this PCM player
    ST_Partition_t              * CPUPartition;
    STAUD_PCMReaderConf_t       pcmReaderDataConfig;
    STMEMBLK_Handle_t           memoryBlockManagerHandle;
    STAVMEM_PartitionHandle_t   AVMEMPartition;
    ST_DeviceName_t             EvtHandlerName;
    ST_DeviceName_t             EvtGeneraterName;       // Module which is generating skip/pause event on which this player will skip/pause
    U32                         NumChannels;
    U32                         Frequency;
}PCMReaderInitParams_t;

ST_ErrorCode_t      STAud_PCMReaderInit(ST_DeviceName_t  Name, STPCMREADER_Handle_t *handle,
                                        PCMReaderInitParams_t *Param_p);
ST_ErrorCode_t      STAud_PCMReaderStart(STPCMREADER_Handle_t handle);
ST_ErrorCode_t      STAud_PCMReaderStop(STPCMREADER_Handle_t handle);
ST_ErrorCode_t      STAud_PCMReaderTerminate(STPCMREADER_Handle_t handle);
ST_ErrorCode_t      STAud_PCMReaderSetCmd(STPCMREADER_Handle_t handle,PCMReaderState_t State);
ST_ErrorCode_t      STAud_PCMReaderPlaybackFromLive(STPCMREADER_Handle_t handle,BOOL Playbackfromlive);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t      STAud_PCMReaderSetClkSynchro(STPCMREADER_Handle_t handle,STCLKRV_Handle_t Clksource);
#endif
ST_ErrorCode_t      STAud_PCMReaderMute(STPCMREADER_Handle_t handle, BOOL   PCMMute);
ST_ErrorCode_t      STAud_PCMReaderGetCapability(STPCMREADER_Handle_t handle,
                                                 STAUD_ReaderCapability_t * Capability_p);
ST_ErrorCode_t      STAud_PCMReaderSetInputParams(STPCMREADER_Handle_t ReaderHandle,
                                                  STAUD_PCMReaderConf_t *PCMInParams_p);
ST_ErrorCode_t      STAud_PCMReaderGetInputParams(STPCMREADER_Handle_t ReaderHandle,
                                                  STAUD_PCMReaderConf_t * PCMInParams_p);
void                PCMReaderTask(void * tempPCMReaderControlBlock_p);
void                PCMReaderFDMACallbackFunction(U32 TransferId,U32 CallbackReason,
                                                  U32 *CurrentNode_p,
                                                  U32 NodeBytesRemaining,
                                                  BOOL Error,void *ApplicationData_p,
                                                  clock_t  InterruptTime);
void                StartPCMReaderIP(PCMReaderControl_t *   Control_p);
void                ConfigurePCMReaderNode(PCMReaderControl_t * Control_p);
void                FillPCMReaderFDMANode(PlayerAudioBlock_t * ControlBlock_p, PCMReaderControl_t * Control_p);

// PCM player event handler and registration/subscription/notification functions
void                PCMReader_EventHandler(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                                           STEVT_EventConstant_t Event,const void *EventData,
                                           const void *SubscriberData_p);
ST_ErrorCode_t      PCMReader_RegisterEvents(PCMReaderControl_t * Control_p);
ST_ErrorCode_t      PCMReader_UnRegisterEvents(PCMReaderControl_t * Control_p);
ST_ErrorCode_t      PCMReader_SubscribeEvents(PCMReaderControl_t * Control_p);
ST_ErrorCode_t      PCMReader_UnSubscribeEvents(PCMReaderControl_t * Control_p);
ST_ErrorCode_t      Reader_NotifyEvt(U32 EvtType,U32 Value, PCMReaderControl_t *Control_p );
ST_ErrorCode_t      PCMReaderCheckStateTransition(PCMReaderControl_t * Control_p,
                                                  PCMReaderState_t State);
ST_ErrorCode_t      STAud_PCMReaderGetBufferParams(STPCMREADER_Handle_t ReaderHandle,
                                                   STAUD_BufferParams_t* DataParams_p);
#endif /*#define __AUDPCMREADER_H*/

