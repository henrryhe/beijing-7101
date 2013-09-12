/*
******************************************************************************
staud.h - (C) Copyright STMicroelectronics (C) 2005
******************************************************************************
*/
#ifndef __STAUD_H__
#define __STAUD_H__
/*
******************************************************************************
Includes
******************************************************************************
*/

#include "stos.h"
#ifndef ST_OSLINUX
#include <stdlib.h>
#include <stdio.h>
#include "string.h"
#include "stsys.h"
#endif
#include "staudlx.h"
#include "aud_drv.h"
#include "stavmem.h"
#include "stddefs.h"
#include "stlite.h"
#include "stfdma.h"
#include "stcommon.h"
#include "stevt.h"
#ifndef ST_51XX
#include "acc_mme.h"
#endif


#ifndef STAUD_REMOVE_CLKRV_SUPPORT
#include "stclkrv.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
******************************************************************************
Exported Types
******************************************************************************
*/
#define		USER_LATENCY_ENABLE	0 //set 1 to enable USER LATENCY
#define		ENABLE_STARTUP_SYNC 1 //set 1 to enable startupsync in players
#define		STFAE_HARDCODE 1 //set to 1 for HEAAC to DTS transcode.
#define		PES_SIZE_ON_AUDIOTYPE 0 //set to 1 if PES SIZE is to be varied as per audio type

#if defined (STAUD_INSTALL_WMA) || defined (STAUD_INSTALL_WMAPROLSL) || defined (STAUD_INSTALL_DTS)
#define MAX_SAMPLES_PER_FRAME1			(4 * 1024)
#else
#ifdef STAUD_INSTALL_LPCMA
#define MAX_SAMPLES_PER_FRAME1			(2560)
#else
#if defined(ST_51XX)
#define MAX_SAMPLES_PER_FRAME			1152
#else
#define MAX_SAMPLES_PER_FRAME1			(2 * 1024)
#endif
#endif
#endif

#if defined(ST_7109)||defined(ST_7200)||defined(ST_51XX)

// Need more nodes for 7109 if we want to remove software byte swapper
#define	NUM_NODES_PARSER				(4+2)
#define	NUM_NODES_PARSER_LATENCY 	(12+2)

#elif defined(ST_7100)

#define	NUM_NODES_PARSER				4
#define	NUM_NODES_PARSER_LATENCY 	12

#endif

#define		NUM_NODES_PCMINPUT	6
#define		NUM_NODES_PCMREADER	8
#define		NUM_NODES_MIXER		6
#define		NUM_NODES_PP			6
#define		NUM_NODES_DATAPROS	6
#define		NUM_NODES_DECODER		6
#define		NUM_NODES_ENCODER		(6 + 2)


#if USER_LATENCY_ENABLE

#define NUM_NODES_BEFORE_PLAYER 18

#else

#define NUM_NODES_BEFORE_PLAYER 6

#endif

#define BIT_BUFFER_THRESHOLD	64 /*Specifies the number of divisions of the Bit buffer*/

#ifndef MSPP_PARSER

	#if defined (STAUD_INSTALL_WMA)  || defined (STAUD_INSTALL_WMAPROLSL) || defined (STAUD_INSTALL_LPCMA) || defined (STAUD_INSTALL_LPCMV)
		#define MAX_INPUT_COMPRESSED_FRAME_SIZE1		(36*1024)
	#else
		#if defined (STAUD_INSTALL_AC3) || defined (STAUD_INSTALL_DTS) || defined (STAUD_INSTALL_MLP)
		#if defined(ST_51XX)
			#if defined(MIXER_SUPPORT) || defined (PCM_SUPPORT)
				#define MAX_INPUT_COMPRESSED_FRAME_SIZE_MPEG	(5*1024)
				#define MAX_INPUT_COMPRESSED_FRAME_SIZE_AC3		(5*1024)
			#else
				#define MAX_INPUT_COMPRESSED_FRAME_SIZE_MPEG	(2*1024)
				#define MAX_INPUT_COMPRESSED_FRAME_SIZE_AC3		(4*1024)
			#endif
		#else
			#define MAX_INPUT_COMPRESSED_FRAME_SIZE1		(16*1024)
		#endif
		#else
			#define MAX_INPUT_COMPRESSED_FRAME_SIZE1		(4*1024)
		#endif
	#endif

#else //MSPP_PARSER

	#if defined (STAUD_INSTALL_WMA)  || defined (STAUD_INSTALL_WMAPROLSL)
		#define MAX_INPUT_COMPRESSED_FRAME_SIZE1		(12*1024)
	#else
		#define MAX_INPUT_COMPRESSED_FRAME_SIZE1		(4*1024)
	#endif

#endif //MSPP_PARSER


#if defined(DVR)
	#define		PES_BUFFER_SIZE		(1024 * 1024)
#else
	#if PES_SIZE_ON_AUDIOTYPE
		#define PES_BUFFER_SIZE (MAX_INPUT_COMPRESSED_FRAME_SIZE * 3)
	#else
		#ifdef ST_OSLINUX
			#ifdef STAUDLX_ALSA_SUPPORT
				#define		PES_BUFFER_SIZE		(32 * 1024)
			#else
				#define		PES_BUFFER_SIZE		(25 * 1024)
			#endif
		#elif defined ST_OSWINCE
			#if defined MSPP_PARSER
				#define		PES_BUFFER_SIZE		(1024 * 1024)
			#else
				#define		PES_BUFFER_SIZE		(256 * 1024)
			#endif
		#else //!ST_OSLINUX & !ST_OSWINCE
			#if defined MSPP_PARSER
				#define		PES_BUFFER_SIZE		(1024 * 1024)
			#else
				#define		PES_BUFFER_SIZE		(8 * 1024)
			#endif
		#endif
	#endif
#endif //#if defined(DVR)

#define      MAX_BIT_BUFFER		(PES_BUFFER_SIZE / BIT_BUFFER_THRESHOLD) /*PES_BUFFER_SIZE should be multiple of THRESHOLD*/


#define MAX_OBJECTS_PER_CLASS 7 /*Update as the Objects increase max 3 PCM players + 1 SPDIF player + 1 HDMI PCM player + 1 HDMI spdif player*/

#define MAX_OUTPUT_BLOCKS_PER_MODULE	4

#if defined (STAUD_INSTALL_WMA)  || defined (STAUD_INSTALL_WMAPROLSL)
#define MAX_ENCODED_FRAMESIZE				(16*1024)
#else
#define MAX_ENCODED_FRAMESIZE				(2*1024)
#endif

#ifdef STAUDLX_ALSA_SUPPORT
#define NUM_SAMPLES_PER_PCM_NODE   192
#else
#define NUM_SAMPLES_PER_PCM_NODE   384
#endif

#if defined(ST_7200)
#define EMBX_TRANSPORT_NAME	"TransportAudio1"
#else
#define EMBX_TRANSPORT_NAME	"ups"
#endif

typedef struct HALAUD_PTIInterface_s
{
	GetWriteAddress_t WriteAddress_p;
	InformReadAddress_t ReadAddress_p;
	void *  Handle;

} HALAUD_PTIInterface_t;

#ifdef ST_51XX
    #define IN_SYNC_THRESHHOLD 2
#else
    #define IN_SYNC_THRESHHOLD 5
#endif

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

#define PARSER_DROP_THRESHOLD				(100*90)	/*100 ms*/

#define MAX_PTS_STC_DRIFT               700         // Max Skip/Pause to be done in ms
#define CEIL_PTSSTC_DRIFT(v)            ((v > MAX_PTS_STC_DRIFT) ? MAX_PTS_STC_DRIFT:v)
typedef enum MPEG
{
	LAYER_AAC,
	LAYER_3,
	LAYER_2,
	LAYER_1
}MPEGAudioLayer_t;

typedef enum
{
    SYNCHRO_STATE_IDLE,
    SYNCHRO_STATE_FIRST_PTS,
    SYNCHRO_STATE_NORMAL
} SyncState_t;

typedef struct AudAVSync_s
{
#ifndef ST_51XX
	osclock_t				CurrentSystemTime;
#else
	clock_t				    CurrentSystemTime;
#endif
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
	STCLKRV_ExtendedSTC_t	CurrentPTS;
#endif
	BOOL							PTSValid;
	STEVT_EventID_t			EventIDSkip;
	STEVT_EventID_t			EventIDPause;

	STEVT_EventID_t			EventIDInSync;
	STEVT_EventID_t			EventIDOutOfSync;
	STEVT_EventID_t			EventIDPTS;
	/*------events only for audio test harness ------------*/

	STEVT_EventID_t			EventIDTestNode;
	STEVT_EventID_t			EventIDTestPCMStart;
	STEVT_EventID_t			EventIDTestPCMReStart;
	STEVT_EventID_t			EventIDTestPCMStop;
	STEVT_EventID_t			EventIDTestPCMPause;

	/*------------------------------------------------------*/

	U32							ValidPTSCount;		// These will be used to generate mute/unmute signal when we will be doing Skip\Pause
	BOOL							GenerateMute;
	BOOL							SyncCompleted;
	U32							InSyncCount;
#ifndef ST_51XX
	osclock_t					SyncCompleteTime;
#else
	clock_t						SyncCompleteTime;// System time at which last audio sync is completed
#endif
	U32 							StartupSyncCount;
	BOOL 							StartSync;
	U32 							Freq;
}AudAVSync_t;

typedef struct AudSTCSync_s
{
	SyncState_t SyncState;
	U32 			SyncDelay;
	S32 			Offset;
	U32 			UsrLatency;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
	STCLKRV_ExtendedSTC_t LastPTS;
	STCLKRV_ExtendedSTC_t LastPCR;
	STCLKRV_ExtendedSTC_t NewPCR;
#endif
	U32 BadClockCount;
	U32 HugeCount;
	U32 SkipPause;
	U32 PauseSkip;
	U32 Synced;
}AudSTCSync_t;

typedef struct
{
	U32 *BaseUnCachedVirtualAddress;
	U32 *BasePhyAddress;
	U32 *BaseCachedVirtualAddress;
} STAud_Memory_Address_t;



#ifndef ST_51XX
typedef struct AudDriver_s
{
	STAUD_DrvChainConstituent_t	*Handle;
	ST_DeviceName_t				Name;
	ST_Partition_t				*CPUPartition_p;
	STAUD_Configuration_t	Configuration;
	STAUD_DigitalMode_t		SPDIFMode;
	BOOL						      Valid;
	S32							      Speed;
#if STFAE_HARDCODE
	BOOL						      StartEncoder;
#endif
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
	STCLKRV_Handle_t 			CLKRV_Handle;
	STCLKRV_Handle_t 			CLKRV_HandleForSynchro;
#endif
	/*For Events*/
	ST_DeviceName_t				EvtHandlerName;
	STEVT_Handle_t				EvtHandle;
	STEVT_EventID_t				EventIDStopped;
	STEVT_EventID_t				EventIDPaused;
	STEVT_EventID_t				EventIDResumed;
	struct AudDriver_s		*Next_p;
}AudDriver_t;
#else
typedef struct AudDriver_s
{
	STAUD_DrvChainConstituent_t	*Handle;
	ST_DeviceName_t					Name;
	ST_Partition_t					*CPUPartition_p;
	STAUD_Configuration_t			Configuration;
	STAUD_DigitalMode_t				SPDIFMode;
	BOOL							Valid;
	S32								Speed;
	semaphore_t                     *AmphionAccess_p;
	STFDMA_Block_t					FDMABlock;              /* FDMA Silicon used */
	STFDMA_ChannelId_t              AmphionInChannel;
	STFDMA_ChannelId_t              AmphionOutChannel;
	STAVMEM_PartitionHandle_t 		BufferPartition;
	U32								dummyBufferSize;
	STAVMEM_BlockHandle_t			DummyBufferHandle;
	U32								* dummyBufferBaseAddress;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
	STCLKRV_Handle_t CLKRV_Handle;
	STCLKRV_Handle_t CLKRV_HandleForSynchro;
#endif
	/*For Events*/
	ST_DeviceName_t					EvtHandlerName;
	STEVT_Handle_t					EvtHandle;
	STEVT_EventID_t					EventIDStopped;
	STEVT_EventID_t					EventIDPaused;
	STEVT_EventID_t					EventIDResumed;
	struct AudDriver_s				*Next_p;
}AudDriver_t;
ST_ErrorCode_t FreeDummyBuffer(AudDriver_t *audDriver_p);
#endif
ST_ErrorCode_t 	STAud_CreateDriver(STAUD_InitParams_t *InitParams_p, AudDriver_t *audDriver_p);
BOOL AudDri_IsInit(const ST_DeviceName_t Name);
void AudDri_QueueAdd(AudDriver_t *Item_p);
void AudDri_QueueRemove(AudDriver_t *Item_p);
ST_ErrorCode_t STAud_RegisterEvents(AudDriver_t *audDriver_p);
ST_ErrorCode_t STAud_UnSubscribeEvents(AudDriver_t *audDriver_p);
AudDriver_t *AudDri_GetBlockFromName(const ST_DeviceName_t Name);
AudDriver_t *AudDri_GetBlockFromHandle(const STAUD_Handle_t Handle);
ST_ErrorCode_t RemoveFromDriver(AudDriver_t	*AudDriver_p,STAUD_Handle_t	Handle);
STAUD_Handle_t	GetHandle(AudDriver_t *drv_p,STAUD_Object_t	Object);




#ifdef __cplusplus
}
#endif



/* ------------------------------- End of file ---------------------------- */
#endif /* #ifndef __STAUD_H__ */

