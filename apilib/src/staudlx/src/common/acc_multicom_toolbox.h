// file : acc_multicom_toolbox.h
//
// Author : Audio Competence Center
// Creation Date : 05 06 16
// Revision Date : 05 06 16



#ifndef _ACC_MULTICOM_TOOLBOX_H_
    #define _ACC_MULTICOM_TOOLBOX_H_

    #include "acc_mme.h"

    #ifndef ST_51XX
        #include "aud_mmeaudiostreamdecoder.h"
    #else
        #include "aud_amphiondecoder.h"
        #include "lpcm_decodertypes.h"
    #endif

    #include "audio_parsertypes.h"
    #include "mspp_parser.h"
    #include "acc_mmedefines.h"
    #include "staudlx.h"
    #include "aud_decoderparams.h"

    /* Default number of samples */
    #define LPCMV_SAMPLES_PER_FRAME     640
    #define LPCMA_SAMPLES_PER_FRAME     2560
    #define MLP_ACCESS_UNITS_PER_FRAME  24

    #ifndef ST_51XX
        #define CDDA_NB_SAMPLES         588
    #else
        #define CDDA_NB_SAMPLES         1152
    #endif

    #define STM7100_AUDIODECODER_INITPARAMS_STRUCT  MME_STm7100AudioDecoderInitParams_t

    extern  MME_ERROR acc_setAudioDecoderInitParams(MME_GenericParams_t params_p, MME_UINT size, enum eAccDecoderId id,
                                                    STAUD_StreamContent_t   StreamContent,
                                                    MME_STm7100PcmProcessingGlobalParams_t *DecPcmProcessing,   STAud_DecHandle_t DecHandle);

    extern MME_ERROR acc_setAudioDecoderGlobalCmd(  MME_Command_t * cmd, MME_GenericParams_t params_p, MME_UINT size,
                                                    enum eAccDecPcmProcId pcmprocessing_id, partition_t     *Partition);


    extern MME_ERROR acc_setAudioDecoderTransformCmd(   MME_Command_t       * cmd,
                                                        MME_GenericParams_t params_p, MME_UINT params_size,
                                                        MME_GenericParams_t status_p, MME_UINT status_size,
                                                        MME_DataBuffer_t ** buffer_array,
                                                        enum eAccCmdCode    playback_mode, U32 pause_duration, enum eAccBoolean restart,
                                                        U32 numInputBuffers, U32 numOutputBuffers);

    extern MME_ERROR acc_setAudioDecoderBufferCmd(  MME_Command_t    * cmd,
                                                    MME_GenericParams_t params_p, MME_UINT params_size,
                                                    MME_GenericParams_t status_p, MME_UINT status_size,
                                                    MME_DataBuffer_t ** buffer_array,
                                                    enum eAccCmdCode      playback_mode, enum eAccBoolean restart,
                                                    enum eAccBoolean last_data_block,
                                                    STAUD_StreamContent_t stream_content);


    extern MME_ERROR acc_setAudioParserInitParams(  MME_GenericParams_t params_p, MME_UINT size, enum eAccDecoderId id,
                                                    enum eAccPacketCode packcode,MspParser_Handle_t  ParserHandle);




    extern MME_ERROR acc_setAudioParserGlobalCmd(   MME_Command_t * cmd, MME_GenericParams_t params_p, MME_UINT size,
                                                    partition_t     *Partition , enum eAccDecoderId id);


    extern MME_ERROR acc_setAudioParserTransformCmd(MME_Command_t    * cmd,
                                                    MME_GenericParams_t params_p, MME_UINT params_size,
                                                    MME_GenericParams_t status_p, MME_UINT status_size,
                                                    MME_DataBuffer_t **buffer_array,
                                                    enum eAccCmdCode      playback_mode, U32 CmdDuration, enum eAccBoolean restart,
                                                    U32 numInputBuffers, U32 numOutputBuffers);

    extern MME_ERROR acc_setAudioParserBufferCmd(   MME_Command_t    * cmd,
                                                    MME_GenericParams_t params_p, MME_UINT params_size,
                                                    MME_GenericParams_t status_p, MME_UINT status_size,
                                                    MME_DataBuffer_t **buffer_array,
                                                    enum eAccCmdCode      playback_mode, enum eAccBoolean restart,
                                                    enum eAccBoolean last_data_block,
                                                    STAUD_StreamContent_t stream_content,enum eAccBoolean eof);

    extern MME_ERROR acc_setAudioMixerTransformCmd( MME_Command_t    * cmd,
                                                    MME_GenericParams_t params_p, MME_UINT params_size,
                                                    MME_GenericParams_t status_p, MME_UINT status_size,
                                                    MME_DataBuffer_t **buffer_array,
                                                    enum eAccCmdCode      playback_mode, U32 InBufNum);

    extern MME_ERROR acc_setAudioPPTransformCmd(    MME_Command_t    * cmd,
                                                    MME_GenericParams_t params_p, MME_UINT params_size,
                                                    MME_GenericParams_t status_p, MME_UINT status_size,
                                                    MME_DataBuffer_t **buffer_array,
                                                    enum eAccCmdCode      playback_mode);


    MME_ERROR acc_setAudioDecoderPcmParams(TARGET_AUDIODECODER_PCMPARAMS_STRUCT * pcm_params, U32 NumChannels);

    enum eAccLpcmMode CovertToLPCMMode (U8 LpcmMode);

    enum eAccFsCode CovertToFsCode(U32 SampleRate);

    U32 CovertFsCodeToSampleRate(enum eAccFsCode FsCode);

    enum eAccLpcmWs CovertToLpcmWSCode(U32 SampleSize);

    U32 CovertFsLPCMCodeToSampleRate(enum eAccLpcmFs FsCode);

    enum eAccLpcmFs CovertToFsLPCMCode(U32 SampleRate);

    U8  CovertFromLpcmWSCode(enum eAccLpcmWs WsCode);

    enum eAccWordSizeCode ConvertToAccWSCode(U32 SampleSize);

    U32 ConvertFromAccWSCode(enum eAccWordSizeCode code);

#endif // _ACC_MULTICOM_TOOLBOX_H_


