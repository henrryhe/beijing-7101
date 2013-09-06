/*******************************************************************************

File name   : aud_cmd.h

Description : Interface for audio related APIs

COPYRIGHT (C) STMicroelectronics 2006

References  :

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __AUD_CMD_H
#define __AUD_CMD_H

/* Includes ----------------------------------------------------------------- */

#include "staudlx.h"
#include "stevt.h"
#include "stfdma.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef  READ_FROM_USB
   #include <osplus/fat.h>
   #include <osplus/vfs.h>
   #include <osplus.h>
#endif

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */
/* DeviceId AUD_xxxx (quick access to hardware) */
typedef enum AUD_DeviceId_e
{
    AUD_INT = 0,
    AUD_EXT = 1,
}AUD_DeviceId_t;

typedef enum Path_e
{
    MAIN_PATH,
    AUX_PATH
}PATH_t  ;

typedef enum Decoder_e
{
    DECODER_MPEG2,
    DECODER_H264,
    DECODER_VC1,
    DECODER_MPEG4P2,
    DIGITAL_INPUT
}DECODER_t;

typedef enum
{
    AC3,
    MPEG2,
    MPEG1_LAYER1,
    MPEG1_LAYER2,
    MP3,
    MPEG_AAC,
    WMA,
    WMAPRO,
    HE_AAC,
    DDPLUS,
    BEEPTONE,
    PCM
}StreamContent_t;

typedef enum
{
    AUD_INJECT_SLEEP,
    AUD_INJECT_START,
    AUD_INJECT_STOP
}AUD_InjectState_t;

typedef struct AUD_InjectContext_s
{
    U32                     InjectorId;           /* Injector identifier                   */
    STAUD_Handle_t          AUDIO_Handler;        /* Audio handle                          */
    STEVT_Handle_t          EVT_Handle;           /* Event handle                          */
    STAUD_StreamParams_t    AUD_StartParams;      /* Audio start parameters                */
    #ifdef READ_FROM_USB
    vfs_file_t*             FileHandle;
    #else
    void                   *FileHandle;            /* File handle descriptor               */
    #endif
    U32                     FileSize;              /* File size to play                    */
    U32                     FilePosition;          /* Current file position                */
    U32                     BufferPtr;             /* Buffer to read the file              */
    U32                     BufferBase;            /* Buffer base to read the file         */
    U32                     BufferProducer;        /* Producer pointer in the file buffer  */
    U32                     BufferConsumer;        /* Consumer pointer in the file buffer  */
    U32                     BufferSize;            /* Buffer size to read the file         */
    U32                     BufferAudio;           /* Audio pes buffer                     */
    U32                     BufferAudioProducer;   /* Producer pointer in the audio buffer */
    U32                     BufferAudioConsumer;   /* Consumer pointer in the audio buffer */
    U32                     BufferAudioSize;       /* Audio pes buffer size                */
    U32                     BitBufferAudioSize;    /* Audio bit buffer empty size          */
    AUD_InjectState_t       State;                 /* State of the injector                */
    U32                     NbLoops;               /* Nb loops to do (0=Infinity)          */
    U32                     NbLoopsAlreadyDone;    /* Nb loops already done                */
    semaphore_t            *SemStart;              /* Start semaphore to injector task     */
    semaphore_t            *SemStop;               /* Stop  semaphore to injector task     */
    semaphore_t            *SemLock;               /* Lock  semaphore for ptr protection   */
    task_t                 *Task;                  /* Task descriptor of the injector      */
    STAVMEM_BlockHandle_t   Block_Handle_p;
}AUD_InjectContext_t;

/* Exported Constants ------------------------------------------------------- */
#define AUDIO_DEVICE_NUMBER  3

/* Exported Variables ------------------------------------------------------- */
extern   AUD_InjectContext_t*   AUDi_InjectContext[AUDIO_DEVICE_NUMBER];
extern   ST_DeviceName_t        AUDIO_Names[AUDIO_DEVICE_NUMBER];
extern   STAUD_Handle_t         AUDIO_Handler[AUDIO_DEVICE_NUMBER];

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t AUD_PcmConfig(AUD_DeviceId_t DeviceId, STAUD_PCMInputParams_t PCMInputParams, STAUD_Object_t InputObject);
ST_ErrorCode_t AUD_PcmMixing(AUD_DeviceId_t DeviceId);
ST_ErrorCode_t AUD_Init(AUD_DeviceId_t DeviceId);
ST_ErrorCode_t AUD_Setup(AUD_DeviceId_t DeviceId);
ST_ErrorCode_t AUD_Start(AUD_DeviceId_t  DeviceId, STAUD_StreamContent_t StreamContent , U32 freq, STAUD_StreamType_t StreamType);
ST_ErrorCode_t AUD_Close (AUD_DeviceId_t DeviceId);
ST_ErrorCode_t AUD_Term(AUD_DeviceId_t DeviceId, BOOL ForceTerminate);
ST_ErrorCode_t AUD_Stop(AUD_DeviceId_t DeviceId);
void ToggleAudioMuteState(void);
ST_ErrorCode_t AUD_InjectStart(U32 DeviceId, U32 ContextId, U8 *FileName, STAUD_Object_t InputObject, U32 NbLoops, BOOL FileIsByteReversed);
ST_ErrorCode_t AUD_InjectStop(U32 DeviceId, U32 ContextId);
ST_ErrorCode_t AUD_SetDigitalOutput(AUD_DeviceId_t DeviceId, U8 SpdifMode);
ST_ErrorCode_t AUD_DRSetSyncOffset(AUD_DeviceId_t DeviceId, U32 Offset);
ST_ErrorCode_t AUD_Configure_Event(AUD_DeviceId_t DeviceId);
ST_ErrorCode_t AUD_Unsubscribe_Events(PATH_t path, DECODER_t decoder_t);

#ifdef TESTTOOL_SUPPORT
	void AUDIO_RegisterCommands();
	BOOL TT_AUD_Close(parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_DRSetSyncOffset(parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_Start (parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_Mute(parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_OPEnableSynchronization (parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_PcmConfig(parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_PcmMixing(parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_SetDigitalOutput (parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_Stop (parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_UnMute(parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_InjectStart(parse_t *pars_p, char *result_sym_p);
	BOOL TT_AUD_InjectStop(parse_t *pars_p, char *result_sym_p);
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __AUD_CMD_H */
