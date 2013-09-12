/*******************************************************************************

File name   : vid_tran.h

Description : Video Transcoder private header file

COPYRIGHT (C) STMicroelectronics 2006.

Date               Modification                                     Name
----               ------------                                     ----
17 Aug 2006        Created from producer module                       OD
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VID_TRAN_H
#define __VID_TRAN_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#include "vid_ctcm.h"
#include "buffers.h"
#include "transcoder.h"
#include "transcodec.h"
#include "stevt.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Temporary */
#define LINEAR_PES_BUFFER                               TRUE

#define TRANSCODER_OUTPUT_BUFFER_TOTAL_SIZE             (1024*1024)
#define TRANSCODER_COMMANDS_BUFFER_SIZE                 20          /* More than the nb of picture in transcoder queue */

#define TRANSCODED_OUTPUT_BUFFER_MAX_SIZE               (1024*1024) /* More than TRANSCODER_OUTPUT_BUFFER_TOTAL_SIZE for inactive (???) */
#define VIDTRAN_MAX_NUMBER_OF_REFERENCES_FRAMES         VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES

#define TRANSCODED_OUTPUT_BUFFER_THRESHOLD_SIZE         TRANSCODER_OUTPUT_BUFFER_TOTAL_SIZE
#define TRANSCODED_OUTPUT_BUFFER_MIN_PICTURE_REMINING   5     /* This number is the minimum space remaining in the transcoded
                                                               output buffer from which we need to flush into the application
                                                               or Hdd */

enum
{
    STVID_TRANSCODER_XCODER_JOB_COMPLETED_EVT_ID,
    STVID_TRANSCODER_STOPPED_EVT_ID,                /* Event that notifies that the transcoder controller has stopped */
    STVID_TRANSCODER_NEW_PICTURE_TRANSCODED_EVT_ID, /* Event that notifies that a new picture has just been transcoded */
    STVID_TRANSCODER_FLUSH_TRANSCODED_BUFFER_EVT_ID,/* Event that notifies that the transcoder output buffer needs flush */
    STVID_TRANSCODER_ERROR_EVT_ID,                  /* Event that reports an internal error */
    VIDTRAN_NB_REGISTERED_EVENTS_IDS
};

/* No more than 128 commands: it needs to be casted to U8 */
typedef enum TranscoderCommandID_e {
    TRANSCODER_NEW_SOURCE_PICTURE_AVAILABLE,        /* A new picture is available for process */
    TRANSCODER_FLUSH_CHECK_QUEUE_AND_STOP,          /* Verify queue is empty and finilize transcode stop */
    NB_CONTROLLER_COMMANDS                          /* Keep this one as the last one */
} TranscoderCommandID_t;

typedef enum VIDTRAN_TranscoderState_e
{
    VIDTRAN_TRANSCODER_STATE_STARTING,              /* Intermediate state prior running */
    VIDTRAN_TRANSCODER_STATE_RUNNING,               /* Pictures in queue are transcoded */
    VIDTRAN_TRANSCODER_STATE_STALLED,               /* Running state but transcoder temporary unable to perform transcodes */
    VIDTRAN_TRANSCODER_STATE_STOP_WHEN_QUEUE_EMPTY, /* Process pictures in queue then stop */
    VIDTRAN_TRANSCODER_STATE_WAITING,               /* Queue is flushing but transcoder temporary unable to perform transcodes */
    VIDTRAN_TRANSCODER_STATE_RESUMED,               /* Resuming from a waiting state */
    VIDTRAN_TRANSCODER_STATE_STOP_AND_FLUSH,        /* Flush and release pictures in queue one by one w/o processing, then stop */
    VIDTRAN_TRANSCODER_STATE_STOPPED                /* Stop the transcode and keep pictures in queue */
} VIDTRAN_TranscoderState_t;

typedef enum VIDTRAN_AvailableReferenceStatus_e
{
    VIDTRAN_AVAILABLE_REFERENCE_NONE,
    VIDTRAN_AVAILABLE_REFERENCE_I,
    VIDTRAN_AVAILABLE_REFERENCE_IP
} VIDTRAN_AvailableReferenceStatus_t;

/* Exported Constants ------------------------------------------------------- */

extern const VIDTRAN_FunctionsTable_t   VIDTRAN_Functions;

/* Exported Types ----------------------------------------------------------- */
typedef struct VIDTRAN_MonitorOutputBufferParams_s
{
  	void * BaseAddress_p;
	U32    Size;
	U32    OutputBufferLevelInBytes;
	void * Write_p;
} VIDTRAN_MonitorOutputBufferParams_t;

typedef struct VIDTRAN_Picture_s
{
    VIDTRQUEUE_QueuePictureState_t  State;
    struct VIDTRAN_Picture_s        * Next_p;                  /* Points to next picture in queue if any */
    struct VIDTRAN_Picture_s        * Previous_p;              /* Points to previous picture in queue if any */
    VIDBUFF_PictureBuffer_t         * MainPictureBuffer_p;
    VIDBUFF_PictureBuffer_t         * DecimatedPictureBuffer_p;
}VIDTRAN_Picture_t;

typedef struct VIDTRAN_Decoder_s {
    VideoDeviceData_t*  DeviceData_p;
    ST_DeviceName_t     DecoderDeviceName;
    VIDTRAN_Handle_t    DecoderHandle;
    XCODEC_Handle_t     TranscodecHandle;
} VIDTRAN_Decoder_t;

#pragma pack(1)
typedef struct VIDTRAN_PES_Header_s
{
/*
    U32 packet_start_code_prefix: 24;
    U32 stream_id               :  8;
*/
    U32 packet_start_code_prefix_AND_stream_id;
/*
    U32 PES_packet_length       : 16;
    U32 Header_10               :  2;
    U32 PES_scrambling_control  :  2;
    U32 PES_priority            :  1;
    U32 data_alignment_indicator:  1;
    U32 copyright               :  1;
    U32 original_or_copy        :  1;
    U32 PTS_DTS_flags           :  2;
    U32 ESCR_flag               :  1;
    U32 ES_rate_flag            :  1;
    U32 DSM_trick_mode_flag     :  1;
    U32 additional_copy_info_flag: 1;
    U32 PES_CRC_flag            :  1;
    U32 PES_extension_flag      :  1;
*/
    U32 PES_Flag;
    U8  PES_header_data_length; /* 8bits length */
} VIDTRAN_PES_Header_t;

typedef struct VIDTRAN_PES_Header_PTS_s
{
/*
    U8  Header_0010             :  4;
    U8  PTS_32_30               :  3;
    U8  marker_bit1             :  1;
*/
    U8  H_PTS_32_30_MarkerBit1;
/*
    U32 PTS_29_15               : 15;
    U32 marker_bit2             :  1;
*/
    U16 PTS_29_15_marker_bit2;
/*
    U32 PTS_14_0                : 15;
    U32 marker_bit3             :  1;
*/
    U16 PTS_14_0_marker_bit3;
} VIDTRAN_PES_Header_PTS_t;
#pragma pack()

/* Type for internal structures for the transcoder */
typedef struct VIDTRAN_Data_s {
    /* Transcoder general  ------------------------------------------------------------ */
    ST_Partition_t *    CPUPartition_p;
    STAVMEM_PartitionHandle_t AVMEMPartition; /* For PES Buffer */
    ST_DeviceName_t     EvtHandlerName; /* Global STEVT Name (for Linking) */
    STEVT_Handle_t      EventsHandle;
    STEVT_EventID_t     RegisteredEventsID[VIDTRAN_NB_REGISTERED_EVENTS_IDS];
    VIDBUFF_Handle_t    BufferManagerHandle;

    U32                 ValidityCheck;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping; /* Needed to translate data1 and data2 in NEW_PICTURE_DECODED_EVT */


    VIDTRAN_Decoder_t   Source;                /* Source static params */
    VIDTRAN_Decoder_t   Target;                /* Target static params */

    XCODE_Handle_t      XcodeHandle;

    STVID_DeviceType_t  DeviceType;
    U8                  DecoderNumber;         /* As defined by DECODER API. */

    /* Internal parameters ----------------------------------------------------------- */

    STVID_TranscoderSetMode_t       TranscoderMode; /* general mode for transcoder */
    STVID_EncoderOuputProfile_t     OutputStreamProfile; /* Internal source display or decode order */

    /* Output ------------------------------------------------------------------------ */
    struct
    {
      BOOL                  IsAllocatedByUs;
      STAVMEM_BlockHandle_t Handle;
      U32                   Size;
      void *                Base_p;
      void *                Top_p;
      void *                Read_p;
      void *                Write_p;
#ifdef LINEAR_PES_BUFFER
      void *                Loop_p;
#endif
    } TranscoderOutputBuffer;

    /* Transcoder main task ----------------------------------------------------------- */
    VIDTRAN_TranscoderState_t   TranscoderState;

    VIDCOM_Task_t     TranscoderTask;
    semaphore_t *     TranscoderOrder_p;      /* To synchronise orders in the task */
    semaphore_t *     TranscoderStop_p;
    semaphore_t *     FreeSpaceInOutputBufferSem_p;

    struct
    {
        U32 Width;
        U32 Height;
    } CurrentPictureToTranscode;

    struct
    {
        osclock_t                               MaxWaitOrderTime;
    } ForTask;

#if 1 /* Firmware Term workaround */
    BOOL                            IsTranscodeOccur;
#endif

    /* Transcode count ---------------------------------------------------------------- */
    BOOL                              IsLastFlush;

    /* ErrorRecovery ------------------------------------------------------------------ */
    STVID_EncoderErrorRecoveryMode_t  ErrorRecoveryMode;

    /* Transcoder Queue --------------------------------------------------------------- */
    VIDTRAN_Picture_t * Queue_p; /* pointer to 1st TrPicture element */
    VIDTRAN_Picture_t   QueueElements[NB_TRQUEUE_ELEM];/* pool containing both Main & decimated pictures entries */

    /* Reference list ----------------------------------------------------------------- */
    VIDBUFF_PictureBuffer_t             * ReferencesPictureBufferPointer[VIDTRAN_MAX_NUMBER_OF_REFERENCES_FRAMES];
    VIDTRAN_AvailableReferenceStatus_t    ReferenceStatus;

    /* Controller ----------------------------------------------------------- */
    U8                  CommandsData[TRANSCODER_COMMANDS_BUFFER_SIZE];
    CommandsBuffer_t    CommandsBuffer;                 /* Data concerning the decode controller commands queue */

    /* Transcode events notification configuration ----------------------------- */
    struct
    {
        STVID_ConfigureEventParams_t ConfigureEventParam;    /* Contains informations : Event enable/disable notification
                                                            and Number of event notification to skip before notifying the event. */
        U32                          NotificationSkipped;    /* Number of notification skipped since the last one. */
    } EventNotificationConfiguration [VIDTRAN_NB_REGISTERED_EVENTS_IDS];

    /* Statistics ----------------------------------------------------------- */
#ifdef STVID_DEBUG_GET_STATISTICS
    STVID_TranscoderStatistics_t      Statistics;
#endif /* STVID_DEBUG_GET_STATISTICS */

    /* Transcode sub-structures --------------------------------------------- */
    XCODE_TranscodePictureParams_t  XCODE_TranscodePictureParams;
    STVID_PictureTranscodedParam_t  PictureTranscodedParam;

} VIDTRAN_Data_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */

#define PushTranscoderCommand(TranscodeHandle, U8Var) vidcom_PushCommand(&(((VIDTRAN_Data_t *) (TranscodeHandle))->CommandsBuffer), ((U8) (U8Var)))
#define PopTranscoderCommand(TranscodeHandle, U8Var_p) vidcom_PopCommand(&(((VIDTRAN_Data_t *) (TranscodeHandle))->CommandsBuffer), ((U8 *) (U8Var_p)))

/* Exported Functions ------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VID_TRAN_H */

/* End of vid_tran.h */
