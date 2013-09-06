

#ifndef _PCM_H_
#define _PCM_H_

#include "stddefs.h"
#include "staudlx.h"
#include "stfdma.h"

#define AUDIO_DEVICE_NUMBER  3
#define STTST_MAX_TOK_LEN          80 /* nb max of bytes for one token      */

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
} AUD_InjectState_t;

typedef enum AUD_DeviceId_e
{
    AUD_INT = 0,
    AUD_EXT = 1,
}AUD_DeviceId_t;

typedef struct AUD_InjectContext_s
{
    U32                     InjectorId;           /* Injector identifier                   */
    STAUD_Handle_t          AUDIO_Handler;        /* Audio handle                          */
    STEVT_Handle_t          EVT_Handle;           /* Event handle                          */
    STAUD_StreamParams_t    AUD_StartParams;      /* Audio start parameters                */
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
    semaphore_t            *SemStart;              /* Start semaphore to injector task     */
    semaphore_t            *SemStop;               /* Stop  semaphore to injector task     */
    semaphore_t            *SemLock;               /* Lock  semaphore for ptr protection   */
    task_t                 *Task;                  /* Task descriptor of the injector      */
    STAVMEM_BlockHandle_t   Block_Handle_p;
}AUD_InjectContext_t;

#endif
