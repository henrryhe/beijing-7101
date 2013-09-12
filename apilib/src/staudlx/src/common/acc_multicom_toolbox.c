// file : acc_multicom_toolbox.c
//
// Author : Gael Lassure
// Creation Date : 05 06 16
// Revision Date : 05 06 16

//#define  STTBX_PRINT
#ifndef ST_OSLINUX
    #include <stdio.h>
    #include <string.h>
    #include "sttbx.h"
#endif

#include "stos.h"
#include "acc_mme.h"


#include "acc_mmedefines.h"
#include "audio_decodertypes.h"
#include "audio_parsertypes.h"
#include "mspp_parser.h"
#include "mp2a_decodertypes.h"
#include "dolbydigital_decodertypes.h"
#include "lpcm_decodertypes.h"
#include "dts_decodertypes.h"
#include "mlp_decodertypes.h"
#include "wma_decodertypes.h"
#include "wmaprolsl_decodertypes.h"
#include "mixer_processortypes.h"
#include "acc_multicom_toolbox.h"
#include "staudlx.h"

typedef MME_ERROR (*tAccSetAudioDecoderConfig)(U32 * decConfig,enum eAccDecoderId id,STAUD_StreamContent_t StreamContent,STAud_DecHandle_t DecoderHandle);

// prototypes
static MME_ERROR acc_setMp2aGlobalParams(U32 * decConfig, enum eAccDecoderId id, STAUD_StreamContent_t StreamContent, STAud_DecHandle_t DecoderHandle);
static MME_ERROR acc_setAc3GlobalParams(U32 * decConfig,  enum eAccDecoderId id, STAUD_StreamContent_t StreamContent, STAud_DecHandle_t DecoderHandle);
static MME_ERROR acc_setAacGlobalParams(U32 * decConfig,  enum eAccDecoderId id, STAUD_StreamContent_t StreamContent, STAud_DecHandle_t DecoderHandle);
static MME_ERROR acc_setLpcmGlobalParams(U32 * decConfig, enum eAccDecoderId id, STAUD_StreamContent_t StreamContent, STAud_DecHandle_t DecoderHandle);
static MME_ERROR acc_setDTSGlobalParams(U32 * decConfig,  enum eAccDecoderId id, STAUD_StreamContent_t StreamContent, STAud_DecHandle_t DecoderHandle);
static MME_ERROR acc_setMLPGlobalParams(U32 * decConfig,  enum eAccDecoderId id, STAUD_StreamContent_t StreamContent, STAud_DecHandle_t DecoderHandle);
static MME_ERROR acc_setWMAGlobalParams(U32 * decConfig,  enum eAccDecoderId id, STAUD_StreamContent_t StreamContent, STAud_DecHandle_t DecoderHandle);
static MME_ERROR acc_setWMAPROLSLGlobalParams(U32 * decConfig,enum eAccDecoderId id, STAUD_StreamContent_t   StreamContent, STAud_DecHandle_t DecoderHandle);
static MME_ERROR acc_setGeneratorGlobalParams(U32 * decConfig,enum eAccDecoderId id, STAUD_StreamContent_t   StreamContent, STAud_DecHandle_t DecoderHandle);
// actual code
tAccSetAudioDecoderConfig acc_setAudioDecoderConfig[] =
{
    NULL, // pcm
    acc_setAc3GlobalParams,
    acc_setMp2aGlobalParams,
    acc_setMp2aGlobalParams,
    acc_setDTSGlobalParams, // dts
    acc_setMLPGlobalParams, // mlp
    acc_setLpcmGlobalParams, // lpcm
    NULL, // sdds
    acc_setWMAGlobalParams, // wma9
    NULL, // oggv
    acc_setAacGlobalParams, // aac
    NULL, // real audio
    NULL, // dhcd
    NULL, //amrwb
    acc_setWMAPROLSLGlobalParams, // wmapro
    acc_setAc3GlobalParams, //ddplus
    acc_setGeneratorGlobalParams, //Beep Tone and Pink Noise
    NULL, // pcm
};


static void acc_setCmcGlobalParams(MME_CMCGlobalParams_t * cmc, U32 NumChannels)
{
    cmc->StructSize       = sizeof(MME_CMCGlobalParams_t);
    cmc->Id               = ACC_CMC_ID;
    cmc->CenterMixCoeff   = ACC_M3DB;
    cmc->SurroundMixCoeff = ACC_M3DB;

    // Set config
    if(NumChannels == 2)
    {
        cmc->Config[CMC_OUTMODE_MAIN]    = (U8) ACC_MODE20;
        cmc->Config[CMC_OUTMODE_AUX]     = (U8) ACC_MODE_ID;
    }
    else
    {
        cmc->Config[CMC_OUTMODE_MAIN]    = (U8) ACC_MODE_ID;
        cmc->Config[CMC_OUTMODE_AUX]     = (U8) ACC_MODE_ID;
    }

    cmc->Config[CMC_DUAL_MODE]       = (U8) (((ACC_DUAL_LR) << 4) | (ACC_DUAL_LR)); // Dual mode for Aux / Main

    cmc->Config[CMC_PCM_DOWN_SCALED] = (U8) ACC_MME_FALSE;
}

static void acc_setPLIIGlobalParams(MME_PLIIGlobalParams_t      *ProLogicII)
{

    ProLogicII->StructSize    = sizeof(MME_PLIIGlobalParams_t);
    ProLogicII->Id            = ACC_PLII_ID;

    ProLogicII->Apply         = ACC_MME_DISABLED;

    ProLogicII->Config[PLII_DEC_MODE]           = (U8) 0xFF;
    ProLogicII->Config[PLII_AUTO_BALANCE]       = ACC_MME_DISABLED;
    ProLogicII->Config[PLII_SPEAKER_DIMENSION]  = (U8) 0;
    ProLogicII->Config[PLII_SURROUND_FILTER]    = ACC_MME_DISABLED;
    ProLogicII->Config[PLII_PANORAMA]           = ACC_MME_DISABLED;
    ProLogicII->Config[PLII_RS_POLARITY]        = ACC_MME_DISABLED;
    ProLogicII->Config[PLII_CENTRE_SPREAD]      = (U8) 0;
    ProLogicII->Config[PLII_OUT_MODE]           = (U8) 0;
    ProLogicII->PcmScale                        = (U32) 0;
}

static void acc_setDeEmphGlobalParams(MME_DeEmphGlobalParams_t * deemph)
{
    deemph->StructSize    = sizeof(MME_DeEmphGlobalParams_t);
    deemph->Id            = ACC_DeEMPH_ID;
    deemph->Apply         = ACC_MME_DISABLED;
    deemph->Mode          = ACC_DEEMPH_50_15_us;
}

static void acc_setDcrmGlobalParams(MME_DCRemoveGlobalParams_t * dcrm)
{
    dcrm->StructSize    = sizeof(MME_DCRemoveGlobalParams_t);
    dcrm->Id            = ACC_DECPCMPRO_DCREMOVE_ID;
    dcrm->Apply         = ACC_MME_DISABLED;
}

static void acc_setDMixGlobalParams(MME_DMixGlobalParams_t * DMix)
{

    DMix->Id = ACC_DMIX_ID;
    DMix->StructSize = sizeof(MME_DMixGlobalParams_t);// - (sizeof(tCoeff15) * 80);

    DMix->Apply =  ACC_MME_AUTO;//ACC_MME_DISABLED;

    DMix->Config[DMIX_USER_DEFINED]     = (U8) 0x0;
    DMix->Config[DMIX_STEREO_UPMIX]     = ACC_MME_FALSE;
    DMix->Config[DMIX_MONO_UPMIX]       = ACC_MME_FALSE;
    DMix->Config[DMIX_MEAN_SURROUND]    = ACC_MME_TRUE;
    DMix->Config[DMIX_SECOND_STEREO]    = ACC_MME_TRUE;
    DMix->Config[DMIX_NORMALIZE]        = ACC_MME_TRUE;
    DMix->Config[DMIX_MIX_LFE]          = ACC_MME_FALSE;
    DMix->Config[DMIX_NORM_IDX]         = 0;
    DMix->Config[DMIX_DIALOG_ENHANCE]   = ACC_MME_FALSE;;
}

static void acc_setTruSurXTGlobalParams( MME_TsxtGlobalParams_t *TruSurXT)
{

    TruSurXT->Id                        = ACC_TSXT_ID;
    TruSurXT->StructSize                = sizeof(MME_TsxtGlobalParams_t);

    TruSurXT->Apply                     =  ACC_MME_DISABLED;

    TruSurXT->Config[TSXT_ENABLE]       = (U8) ACC_MME_FALSE;
    TruSurXT->Config[TSXT_HEADPHONE]    = (U8) ACC_MME_FALSE;
    TruSurXT->Config[TSXT_TBSIZE]       = (U8) 0x0;
    TruSurXT->Config[TSXT_MODE]         = (U8) 12;/* Default Mode - AUTO */

    TruSurXT->FocusElevation            = 0x7FFF;
    TruSurXT->FocusTweeterElevation     =0;
    TruSurXT->TBLevel                   = 0x7FFF;
    TruSurXT->InputGain                 = 0x7FFF;
}

static void acc_setOmniGlobalParams( MME_OmniGlobalParams_t *OmniMain)
{

    OmniMain->Id                            = ACC_OMNI_ID;
    OmniMain->StructSize                    = sizeof(MME_OmniGlobalParams_t);
    OmniMain->Apply                         = ACC_MME_DISABLED;

    OmniMain->Config[OMNI_ENABLE]           = (U8) ACC_MME_FALSE;
    OmniMain->Config[OMNI_INPUT_MODE]       = (U8) ACC_MME_FALSE;
    OmniMain->Config[OMNI_SURROUND_MODE]    = (U8) ACC_MME_FALSE;
    OmniMain->Config[OMNI_DIALOG_MODE]      = (U8) ACC_MME_FALSE;
    OmniMain->Config[OMNI_LFE_MODE]         = (U8) ACC_MME_FALSE;
    OmniMain->Config[OMNI_DIALOG_LEVEL]     = (U8) ACC_MME_FALSE;
}

static void acc_setAc3ExGlobalParams( MME_AC3ExGlobalParams_t *ac3ex)
{
    ac3ex->Id                               = ACC_AC3Ex_ID;
    ac3ex->StructSize                       = sizeof(MME_AC3ExGlobalParams_t);
    ac3ex->Apply                            = ACC_MME_DISABLED;
}

static void acc_setBassMgtGlobalParams(MME_BassMgtGlobalParams_t * bassmgt)
{
    bassmgt->StructSize                     = sizeof(MME_BassMgtGlobalParams_t);
    bassmgt->Id                             = ACC_DECPCMPRO_BASSMGT_ID;
    bassmgt->Apply                          = ACC_MME_DISABLED;

    // Reset bassmgt control >> Volume control only
    bassmgt->Config[BASSMGT_TYPE]           = (U8) BASSMGT_VOLUME_CTRL;
    bassmgt->Config[BASSMGT_OUT_WS]         = (U8) ACC_WS32;
    bassmgt->Config[BASSMGT_LFE_OUT]        = (U8) ACC_MME_FALSE;
    bassmgt->Config[BASSMGT_BOOST_OUT]      = (U8) ACC_MME_FALSE;
    bassmgt->Config[BASSMGT_PROLOGIC_IN]    = (U8) ACC_MME_FALSE;
    bassmgt->Config[BASSMGT_LIMITER]        = (U8) ACC_MME_FALSE;

    // Reset volume (0dB to all channels
    bassmgt->Volume[ACC_MAIN_LEFT ]         = 0;
    bassmgt->Volume[ACC_MAIN_RGHT ]         = 0;
    bassmgt->Volume[ACC_MAIN_CNTR ]         = 0;
    bassmgt->Volume[ACC_MAIN_LFE  ]         = 0;
    bassmgt->Volume[ACC_MAIN_LSUR ]         = 0;
    bassmgt->Volume[ACC_MAIN_RSUR ]         = 0;
    bassmgt->Volume[ACC_MAIN_CSURL]         = 0;
    bassmgt->Volume[ACC_MAIN_CSURR]         = 0;
    bassmgt->Volume[ACC_AUX_LEFT  ]         = 0;
    bassmgt->Volume[ACC_AUX_RGHT  ]         = 0;

    // Reset delay
    bassmgt->DelayUpdate                    = ACC_MME_TRUE;
    bassmgt->Delay[ACC_MAIN_LEFT ]          = 0;
    bassmgt->Delay[ACC_MAIN_RGHT ]          = 0;
    bassmgt->Delay[ACC_MAIN_CNTR ]          = 0;
    bassmgt->Delay[ACC_MAIN_LFE  ]          = 0;
    bassmgt->Delay[ACC_MAIN_LSUR ]          = 0;
    bassmgt->Delay[ACC_MAIN_RSUR ]          = 0;
    bassmgt->Delay[ACC_MAIN_CSURL]          = 0;
    bassmgt->Delay[ACC_MAIN_CSURR]          = 0;
}

static void acc_setTempoCtrlGlobalParams(MME_TempoGlobalParams_t * tempoctrl)
{
    tempoctrl->StructSize                   = sizeof(MME_TempoGlobalParams_t);
    tempoctrl->Id                           = ACC_TEMPO_ID;
    tempoctrl->Apply                        = ACC_MME_DISABLED;
    tempoctrl->Ratio                        = 100;
}

static void acc_setCSiiGlobalParams(MME_CSIIGlobalParams_t * Csii)
{
    Csii->StructSize                        = sizeof(MME_CSIIGlobalParams_t);
    Csii->Id                                = ACC_CSII_ID;
    Csii->Apply                             = ACC_MME_DISABLED;
    Csii->Config[CSII_ENABLE]               = 0; /**/
    Csii->Config[CSII_TBSIZE]               = 0; /* Default Response40Hz*/
    Csii->Config[CSII_OUTMODE]              = 0; /*Default Stereo*/
    Csii->Config[CSII_MODE]                 = 0; /*Default Cinema*/
    Csii->FocusElevation                    = 0x7FFF;
    Csii->InputGain                         = 0x7FFF;
    Csii->TBLevel                           = 0x7FFF;

}

static void acc_setDTSNeoGlobalParams(MME_NEOGlobalParams_t* DTSNeo)
{
    DTSNeo->Apply                           = ACC_MME_DISABLED;
    DTSNeo->Id                              = ACC_NEO_ID;
    DTSNeo->StructSize                      = sizeof(MME_NEOGlobalParams_t);
    DTSNeo->Config[NEO_IO_STARTIDX]         = 0 ; /*Zero Always from Codec Team*/
    /*IF C gain is zero use default value */
    DTSNeo->Config[NEO_CENTRE_GAIN]         = 0XFF;
    /*If No Mode Specified Use the Cinema as default*/
    DTSNeo->Config[NEO_SETUP_MODE]          = STAUD_DTSNEOMODE_CINEMA;
    DTSNeo->Config[NEO_AUXMODE_FLAG]        = STAUD_DTSNEOMODE_BACK;
    DTSNeo->Config[NEO_OUTPUT_CHAN]         = 6;
}
MME_ERROR acc_setAudioDecoderPcmParams(TARGET_AUDIODECODER_PCMPARAMS_STRUCT * pcm_params, U32 NumChannels)
{
    // Set globalParams Structure info
    pcm_params->StructSize = sizeof(TARGET_AUDIODECODER_PCMPARAMS_STRUCT);

    // Set pcmprocessing specific config
    acc_setCmcGlobalParams      (&pcm_params->CMC, NumChannels);
    acc_setDeEmphGlobalParams   (&pcm_params->DeEmph);
    acc_setBassMgtGlobalParams  (&pcm_params->BassMgt);
    acc_setDcrmGlobalParams     (&pcm_params->DcRemove);
    acc_setDMixGlobalParams     (&pcm_params->DMix);
    acc_setPLIIGlobalParams     (&pcm_params->ProLogicII);
    acc_setTruSurXTGlobalParams (&pcm_params->TruSurXT);
    acc_setOmniGlobalParams     (&pcm_params->OmniMain);
    acc_setAc3ExGlobalParams    (&pcm_params->Ac3Ex);
    acc_setTempoCtrlGlobalParams(&pcm_params->TempoCtrl);
    acc_setCSiiGlobalParams     (&pcm_params->Csii);
    acc_setDTSNeoGlobalParams   (&pcm_params->DTSNeo);

    return (MME_SUCCESS);
}

static MME_ERROR acc_SetDecPcmParams(TARGET_AUDIODECODER_PCMPARAMS_STRUCT * pcm_params,MME_STm7100PcmProcessingGlobalParams_t   *DecPcmProcessing )
{
    // Set globalParams Structure info
    pcm_params->StructSize = sizeof(TARGET_AUDIODECODER_PCMPARAMS_STRUCT);

    pcm_params->CMC         = DecPcmProcessing->CMC;
    pcm_params->DeEmph      = DecPcmProcessing->DeEmph;
    pcm_params->BassMgt     = DecPcmProcessing->BassMgt;
    pcm_params->DcRemove    = DecPcmProcessing->DcRemove;
    pcm_params->DMix        = DecPcmProcessing->DMix;
    pcm_params->ProLogicII  = DecPcmProcessing->ProLogicII;
    pcm_params->TruSurXT    = DecPcmProcessing->TruSurXT;
    pcm_params->OmniMain    = DecPcmProcessing->OmniMain;
    pcm_params->Ac3Ex       = DecPcmProcessing->Ac3Ex;
    pcm_params->TempoCtrl   = DecPcmProcessing->TempoCtrl;
    pcm_params->Csii        = DecPcmProcessing->Csii;
    pcm_params->DTSNeo      = DecPcmProcessing->DTSNeo;

    return (MME_SUCCESS);
}

static MME_ERROR acc_setMp2aGlobalParams(U32 * decConfig,enum eAccDecoderId id,
           STAUD_StreamContent_t    StreamContent,
           STAud_DecHandle_t DecoderHandle)
{
    MME_LxMp2aConfig_t * mp2a_config    = (MME_LxMp2aConfig_t *)decConfig;
    DecoderControlBlockList_t   *Dec_p  = (DecoderControlBlockList_t*)NULL;
    Dec_p = STAud_DecGetControlBlockFromHandle(DecoderHandle);

    if(Dec_p==NULL)                                     // check foe dec_p= null added
    {
        return (MME_INVALID_HANDLE);                    // check foe dec_p= null added
    }

    mp2a_config->StructSize = ACC_MAX_DEC_CONFIG_SIZE*sizeof(U32);
    mp2a_config->DecoderId  = id; // ACC_MP2A_ID or ACC_MP3_ID

    mp2a_config->Config[MP2a_CRC_ENABLE]        = (U8) ACC_MME_TRUE;
    mp2a_config->Config[MP2a_LFE_ENABLE]        = (U8) ACC_MME_FALSE;
    mp2a_config->Config[MP2a_DRC_ENABLE]        = (U8) ACC_MME_FALSE;

    if(Dec_p->DecoderControlBlock.NumChannels == 2)
    {
        mp2a_config->Config[MP2a_MC_ENABLE]     = (U8) ACC_MME_FALSE;
    }
    else
    {
        mp2a_config->Config[MP2a_MC_ENABLE]     = (U8) ACC_MME_TRUE;
    }

    mp2a_config->Config[MP2a_NIBBLE_ENABLE]     = (U8) ACC_MME_FALSE;
    mp2a_config->Config[MP3_FREE_FORMAT]        = (U8) ACC_MME_FALSE;
    mp2a_config->Config[MP3_MUTE_ON_ERROR]      = (U8) ACC_MME_FALSE;
    mp2a_config->Config[MP3_DECODE_LAST_FRAME]  = (U8) ACC_MME_FALSE;

    return (MME_SUCCESS);
}

static MME_ERROR acc_setAc3GlobalParams(U32 * decConfig,enum eAccDecoderId id,
        STAUD_StreamContent_t   StreamContent,
        STAud_DecHandle_t DecoderHandle)
{
    MME_LxAc3Config_t * ac3_config      = (MME_LxAc3Config_t *)decConfig;
    DecoderControlBlockList_t   *Dec_p  = (DecoderControlBlockList_t*)NULL;
    Dec_p = STAud_DecGetControlBlockFromHandle(DecoderHandle);
    if(Dec_p==NULL)                                     // check foe dec_p= null added
    {
        return (MME_INVALID_HANDLE);                    // check foe dec_p= null added
    }

    ac3_config->StructSize                      = ACC_MAX_DEC_CONFIG_SIZE*sizeof(U32);
    ac3_config->DecoderId                       = id;

    ac3_config->Config[DD_CRC_ENABLE]           = (U8) ACC_MME_TRUE;
    ac3_config->Config[DD_LFE_ENABLE]           = (U8) ACC_MME_TRUE;
    ac3_config->Config[DD_COMPRESS_MODE]        = (U8) DD_LINE_OUT;
    ac3_config->Config[DD_HDR]                  = (U8) 0x00;
    ac3_config->Config[DD_LDR]                  = (U8) 0x00;
    ac3_config->Config[DD_HIGH_COST_VCR]        = (U8) ACC_MME_FALSE;
    ac3_config->Config[DD_VCR_REQUESTED]        = (U8) ACC_MME_FALSE;
    /*Mainly for DDPlus */
    ac3_config->Config[DDP_FRAMEBASED_ENABLE]   = (U8) ACC_MME_TRUE;

    #ifndef ST_51XX
        ac3_config->Config[DDP_OUTPUT_SETTING]  = (U8) (0x4 | (Dec_p->DecoderControlBlock.DDPOPSetting & 0x03));// Bit[0..1] :: UPSAMPLE_PCMOUT_ENABLE ; BIT[2] :: LITTLE_ENDIAN_DDout
    #endif

    Dec_p->DecoderControlBlock.TrancodedDataAlignment = LE;
    ac3_config->PcmScale                        = 0x7FFF;

    return (MME_SUCCESS);
}

static MME_ERROR acc_setAacGlobalParams(U32 * decConfig,enum eAccDecoderId id,
                                        STAUD_StreamContent_t   StreamContent,
                                        STAud_DecHandle_t DecoderHandle)
{

    MME_LxMp2aConfig_t * aac_config             = (MME_LxMp2aConfig_t *)decConfig;
    DecoderControlBlock_t * TempDecControlBlock = (DecoderControlBlock_t *)DecoderHandle;
    U32 SamplingFrequency                       = TempDecControlBlock->StreamParams.SamplingFrequency;
    aac_config->StructSize                      = ACC_MAX_DEC_CONFIG_SIZE*sizeof(U32);
    aac_config->DecoderId                       = id;

    aac_config->Config[AAC_CRC_ENABLE]          = (U8) ACC_MME_TRUE;
    aac_config->Config[AAC_DRC_ENABLE]          = (U8) ACC_MME_FALSE;
    #ifndef ST_51XX
        aac_config->Config[AAC_SBR_ENABLE]      = (U8) TempDecControlBlock->SBRFlag;
    #endif

    switch (StreamContent)
    {
        case STAUD_STREAM_CONTENT_HE_AAC:
            aac_config->Config[AAC_FORMAT_TYPE] = (U8) AAC_LOAS_FORMAT;
            break;

        case STAUD_STREAM_CONTENT_MPEG_AAC:
            aac_config->Config[AAC_FORMAT_TYPE] = (U8) AAC_ADTS_FORMAT;
            break;

        case STAUD_STREAM_CONTENT_ADIF:
            aac_config->Config[AAC_FORMAT_TYPE] = (U8) AAC_ADIF_FORMAT;
            break;

        case STAUD_STREAM_CONTENT_MP4_FILE:
            aac_config->Config[AAC_FORMAT_TYPE] = (U8) AAC_MP4_FILE_FORMAT;
            break;

        case STAUD_STREAM_CONTENT_RAW_AAC:
            aac_config->Config[AAC_FORMAT_TYPE] = (U8) AAC_RAW_FORMAT;
            AACMME_SET_TYPE_PARAM(aac_config->Config,(CovertToFsCode(SamplingFrequency)));//need to change.. just for testing.. bharat
            break;

        default:
        // Default consider ADTS format
            aac_config->Config[AAC_FORMAT_TYPE]= (U8) AAC_ADTS_FORMAT;
            break;
    }

    return (MME_SUCCESS);
}

static MME_ERROR acc_setDTSGlobalParams(U32 * decConfig,enum eAccDecoderId id,
                                        STAUD_StreamContent_t   StreamContent,
                                        STAud_DecHandle_t DecoderHandle)
{
    MME_LxDtsConfig_t * dts_config      = (MME_LxDtsConfig_t *)decConfig;
    DecoderControlBlockList_t   *Dec_p  = (DecoderControlBlockList_t*)NULL;
    Dec_p = STAud_DecGetControlBlockFromHandle(DecoderHandle);

    if(Dec_p==NULL)                                     // check foe dec_p= null added
    {
        return (MME_INVALID_HANDLE);                    // check foe dec_p= null added
    }


    dts_config->StructSize                              = ACC_MAX_DEC_CONFIG_SIZE*sizeof(U32);
    dts_config->DecoderId                               = id;

    dts_config->Config[DTSHD_CRC_ENABLE]                = ACC_MME_FALSE;
    dts_config->Config[DTSHD_LFE_ENABLE]                = ACC_MME_TRUE;
    dts_config->Config[DTSHD_DRC_ENABLE]                = ACC_MME_TRUE;
    dts_config->Config[DTSHD_XCH_ENABLE]                = ACC_MME_TRUE; //DTS_ES
    dts_config->Config[DTSHD_96K_ENABLE]                = ACC_MME_TRUE;
    dts_config->Config[DTSHD_NBBLOCKS_PER_TRANSFORM]    = ACC_MME_TRUE;
    dts_config->Config[DTSHD_XBR_ENABLE]                = ACC_MME_TRUE;
    dts_config->Config[DTSHD_XLL_ENABLE]                = ACC_MME_TRUE;
    dts_config->Config[DTSHD_INTERPOLATE_2X_ENABLE]     = ACC_MME_FALSE;
    dts_config->Config[DTSHD_MIX_LFE]                   = ACC_MME_FALSE;

    #ifndef ST_51XX
        dts_config->Config[DTSHD_LBR_ENABLE]            = Dec_p->DecoderControlBlock.DDPOPSetting;//ACC_MME_FALSE;
    #endif

    dts_config->Config[DTSHD_24BITENABLE]               = ACC_MME_FALSE;
    dts_config->Config[DTSHD_OUTSR_CHNGFACTOR]          = ACC_MME_FALSE;

    dts_config->FirstByteEncSamples                     = 0;
    dts_config->Last4ByteEncSamples                     = 0;
    dts_config->DelayLossLess                           = 0;
    dts_config->CoreSize                                = 0;
    return (MME_SUCCESS);
}


static MME_ERROR acc_setMLPGlobalParams(U32 * decConfig,enum eAccDecoderId id,
                                        STAUD_StreamContent_t   StreamContent,
                                        STAud_DecHandle_t DecoderHandle)
{
    MME_LxMlpConfig_t * mlp_config                  = (MME_LxMlpConfig_t *)decConfig;

    mlp_config->StructSize                          = ACC_MAX_DEC_CONFIG_SIZE*sizeof(U32);
    mlp_config->DecoderId                           = id;

    mlp_config->Config[MLP_CRC_ENABLE]              = ACC_MME_TRUE;
    mlp_config->Config[MLP_DRC_ENABLE]              = ACC_MME_FALSE;
    mlp_config->Config[MLP_CHANNEL_ASSIGNMENT]      = 0xFF;
    mlp_config->Config[MLP_SERIAL_ACCESS_UNITS]     = MLP_ACCESS_UNITS_PER_FRAME;
    mlp_config->Config[MLP_DOWNMIX_2_0]             = ACC_MME_TRUE;
    mlp_config->Config[MLP_AUX_ENABLE]              = ACC_MME_FALSE;

    return (MME_SUCCESS);
}

static MME_ERROR acc_setCddaGlobalParams(U32 * decConfig)
{

    MME_LxLpcmConfig_t * lpcm_config                = (MME_LxLpcmConfig_t *)decConfig;

    U32 fscode                                      = CovertFsLPCMCodeToSampleRate(ACC_LPCM_FS44); // needed for CDDA and PCm to be able to playback any frequency

    lpcm_config->Config[LPCM_MODE]                  = ACC_RAW_PCM;
    lpcm_config->Config[LPCM_DRC_CODE]              = 0x80;
    lpcm_config->Config[LPCM_DRC_ENABLE]            = ACC_MME_FALSE;
    lpcm_config->Config[LPCM_MUTE_FLAG]             = ACC_MME_FALSE;
    lpcm_config->Config[LPCM_EMPHASIS_FLAG]         = ACC_MME_FALSE;
    lpcm_config->Config[LPCM_NB_CHANNELS]           = 2;
    lpcm_config->Config[LPCM_WS_CH_GR1]             = ACC_LPCM_WS16;
    lpcm_config->Config[LPCM_WS_CH_GR2]             = ACC_LPCM_WS_NO_GR2;
    lpcm_config->Config[LPCM_FS_CH_GR1]             = CovertToFsCode(fscode);
    lpcm_config->Config[LPCM_FS_CH_GR2]             = ACC_LPCM_FS_NO_GR2;
    lpcm_config->Config[LPCM_BIT_SHIFT_CH_GR2]      = 0;
    lpcm_config->Config[LPCM_CHANNEL_ASSIGNMENT]    = 1; // Changed as per the recommendation of Satej Pankey
    lpcm_config->Config[LPCM_MIXING_PHASE]          = 0x3000;//LPCM_MIX_DEFAULT_PHASE;
    lpcm_config->Config[LPCM_NB_ACCESS_UNITS]       = 0;
    lpcm_config->Config[LPCM_OUT_RESAMPLING]        = ACC_LPCM_NO_RSPL;//chandan
    lpcm_config->Config[LPCM_NB_SAMPLES]            = CDDA_NB_SAMPLES;

    return (MME_SUCCESS);
}

static MME_ERROR acc_setDvdLpcmGlobalParams(U32 * decConfig,STAUD_StreamContent_t   StreamContent)
{

    MME_LxLpcmConfig_t * lpcm_config = (MME_LxLpcmConfig_t *)decConfig;

    switch(StreamContent)
    {
        case STAUD_STREAM_CONTENT_LPCM :
            lpcm_config->Config[LPCM_MODE] = ACC_LPCM_VIDEO;
            break;

        case STAUD_STREAM_CONTENT_LPCM_DVDA:
            lpcm_config->Config[LPCM_MODE] = ACC_LPCM_AUDIO;
            break;

        default:
            STTBX_Print(("acc_setDvdVLpcmGlobalParams: Error-Invalid Stream Content Type \n" ));
    }
    lpcm_config->Config[LPCM_DRC_CODE]          = 0x80;
    lpcm_config->Config[LPCM_DRC_ENABLE]        = ACC_MME_FALSE;
    lpcm_config->Config[LPCM_MUTE_FLAG]         = ACC_MME_FALSE;
    lpcm_config->Config[LPCM_EMPHASIS_FLAG]     = ACC_MME_FALSE;
    lpcm_config->Config[LPCM_NB_CHANNELS]       = 2;/* Chandan -Default- Should be changed before first Transform cmd */
    lpcm_config->Config[LPCM_WS_CH_GR1]         = ACC_LPCM_WS16;
    lpcm_config->Config[LPCM_WS_CH_GR2]         = ACC_LPCM_WS_NO_GR2;
    lpcm_config->Config[LPCM_FS_CH_GR1]         = ACC_LPCM_FS48;
    lpcm_config->Config[LPCM_FS_CH_GR2]         = ACC_LPCM_FS_NO_GR2;
    lpcm_config->Config[LPCM_BIT_SHIFT_CH_GR2]  = 0;
    lpcm_config->Config[LPCM_CHANNEL_ASSIGNMENT]= 0xFF; /* Chandan -Default- Should be changed before first Transform cmd */
    lpcm_config->Config[LPCM_MIXING_PHASE]      = 0x3000;//LPCM_MIX_DEFAULT_PHASE;
    lpcm_config->Config[LPCM_NB_ACCESS_UNITS]   = 0;
    lpcm_config->Config[LPCM_OUT_RESAMPLING]    = ACC_LPCM_NO_RSPL;//chandan
    lpcm_config->Config[LPCM_NB_SAMPLES]        = LPCMV_SAMPLES_PER_FRAME;/*Default */

    return (MME_SUCCESS);
}

static MME_ERROR acc_setLpcmGlobalParams(U32 * decConfig,enum eAccDecoderId id,
        STAUD_StreamContent_t   StreamContent,
        STAud_DecHandle_t DecoderHandle)
{
    MME_LxLpcmConfig_t * lpcm_config = (MME_LxLpcmConfig_t *)decConfig;

    lpcm_config->StructSize = ACC_MAX_DEC_CONFIG_SIZE*sizeof(U32);
    lpcm_config->DecoderId  = id;

    switch(StreamContent)
    {
        case STAUD_STREAM_CONTENT_CDDA:
        case STAUD_STREAM_CONTENT_PCM:
            acc_setCddaGlobalParams(decConfig);
            break;

        case STAUD_STREAM_CONTENT_LPCM :
            acc_setDvdLpcmGlobalParams(decConfig,StreamContent);
            break;

        default :
            break;

    }

    return (MME_SUCCESS);
}

static MME_ERROR acc_setWMAGlobalParams(U32 * decConfig,enum eAccDecoderId id,
                                        STAUD_StreamContent_t   StreamContent,
                                        STAud_DecHandle_t DecoderHandle)
{

    MME_LxWmaConfig_t* wma_config = (MME_LxWmaConfig_t *)decConfig;
    wma_config->DecoderId = id;
    wma_config->StructSize = ACC_MAX_DEC_CONFIG_SIZE*sizeof(U32);

    wma_config->AudioStreamInfo.nVersion            = 0;
    wma_config->AudioStreamInfo.wFormatTag          = 0;
    wma_config->AudioStreamInfo.nSamplesPerSec      = 0;
    wma_config->AudioStreamInfo.nAvgBytesPerSec     = 0;
    wma_config->AudioStreamInfo.nBlockAlign         = 0;
    wma_config->AudioStreamInfo.nChannels           = 0;
    wma_config->AudioStreamInfo.nEncodeOpt          = 0;
    wma_config->AudioStreamInfo.nSamplesPerBlock    = 0;
    wma_config->AudioStreamInfo.dwChannelMask       = 0;
    wma_config->AudioStreamInfo.nBitsPerSample      = 0;
    wma_config->AudioStreamInfo.wValidBitsPerSample = 0;
    wma_config->AudioStreamInfo.wStreamId           = 0;

    wma_config->MaxNbPages                          = 1;
    wma_config->MaxPageSize                         = 4*1024;
    wma_config->MaxBitRate                          = 320*1024;
    wma_config->NbSamplesOut                        = 2048;

    wma_config->NewAudioStreamInfo                  = TRUE;

    return (MME_SUCCESS);
}

static MME_ERROR acc_setWMAPROLSLGlobalParams(U32 * decConfig,enum eAccDecoderId id,
        STAUD_StreamContent_t   StreamContent,
        STAud_DecHandle_t DecoderHandle)
{
    MME_LxWmaProLslConfig_t* wmaprolsl_config = (MME_LxWmaProLslConfig_t *)decConfig;
    wmaprolsl_config->DecoderId = id;
    wmaprolsl_config->StructSize = ACC_MAX_DEC_CONFIG_SIZE*sizeof(U32);

    wmaprolsl_config->AudioStreamInfo.nVersion              = 0;
    wmaprolsl_config->AudioStreamInfo.wFormatTag            = 0;
    wmaprolsl_config->AudioStreamInfo.nSamplesPerSec        = 0;
    wmaprolsl_config->AudioStreamInfo.nAvgBytesPerSec       = 0;
    wmaprolsl_config->AudioStreamInfo.nBlockAlign           = 0;
    wmaprolsl_config->AudioStreamInfo.nChannels             = 0;
    wmaprolsl_config->AudioStreamInfo.nEncodeOpt            = 0;
    wmaprolsl_config->AudioStreamInfo.nSamplesPerBlock      = 0;
    wmaprolsl_config->AudioStreamInfo.dwChannelMask         = 0;
    wmaprolsl_config->AudioStreamInfo.nBitsPerSample        = 0;
    wmaprolsl_config->AudioStreamInfo.wValidBitsPerSample   = 0;
    wmaprolsl_config->AudioStreamInfo.wStreamId             = 0;
    wmaprolsl_config->MaxNbPages                            = 1;
    wmaprolsl_config->MaxPageSize                           = 4*1024;
    wmaprolsl_config->_DUMMY                                = 0;
    wmaprolsl_config->NbSamplesOut                          = 2048;
    wmaprolsl_config->NewAudioStreamInfo                    = TRUE;
    wmaprolsl_config->nDecoderFlags                         = 0;
    wmaprolsl_config->nDRCSetting                           = 0;
    wmaprolsl_config->nInterpResampRate                     = 0;

    return (MME_SUCCESS);
}

static MME_ERROR acc_setGeneratorGlobalParams(  U32 * decConfig,enum eAccDecoderId id,
                                                STAUD_StreamContent_t   StreamContent,
                                                STAud_DecHandle_t DecoderHandle)
{
    MME_MixerInputConfig_t *InputParams;
    MME_LxDecConfig_t * dec_config  = (MME_LxDecConfig_t *)decConfig;

    dec_config->StructSize          = ACC_MAX_DEC_CONFIG_SIZE*sizeof(U32);
    dec_config->DecoderId           = id;

    InputParams = (MME_MixerInputConfig_t*)&dec_config->Config;

    if(StreamContent == STAUD_STREAM_CONTENT_BEEP_TONE)
    {
        InputParams->InputId            = 1;//Beep tone
        InputParams->NbChannels         = 6;
        InputParams->Alpha              = 1024;//Number of samples per transform
        InputParams->Mono2Stereo        = 0;
        InputParams->WordSize           = 0;//0 db
        InputParams->AudioMode          = 0x3; //Assigne all tones to all channels
        InputParams->SamplingFreq       = STAud_DecGetSamplFreq(DecoderHandle);
        InputParams->FirstOutputChan    = ACC_MAIN_LEFT;
        InputParams->AutoFade           = ACC_MME_FALSE;
        #ifndef ST_51XX
        {
            U32 Temp;
            InputParams->Config = STAud_DecGetBeepToneFreq(DecoderHandle,&Temp);//10 * 11 * 4 = 440HZ
        }
        #endif
    }
    else if (StreamContent == STAUD_STREAM_CONTENT_PINK_NOISE)
    {
        InputParams->InputId            = 2;//Pink Noise
        InputParams->NbChannels         = 6;
        InputParams->Alpha              = 1024;//Number of samples per transform
        InputParams->Mono2Stereo        = 0;
        InputParams->WordSize           = 0;
        InputParams->AudioMode          = ACC_MODE20t; //Assigne all tones to all channels
        InputParams->SamplingFreq       = STAud_DecGetSamplFreq(DecoderHandle);
        InputParams->FirstOutputChan    = ACC_MAIN_LEFT;
        InputParams->AutoFade           = ACC_MME_FALSE;
        InputParams->Config             = 0;
    }

    return (MME_SUCCESS);
}

static MME_ERROR acc_setAudioDecoderGlobalParams(   TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT * global_params,
                                                    enum eAccDecoderId id, STAUD_StreamContent_t    StreamContent,
                                                    MME_STm7100PcmProcessingGlobalParams_t  *DecPcmProcessing,
                                                    STAud_DecHandle_t DecoderHandle)
{
    // Set globalParams Structure info
    global_params->StructSize = sizeof(TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT);

    // Set decoder specific config
    if (acc_setAudioDecoderConfig[id & 0xFF]!= NULL)
    {
        acc_setAudioDecoderConfig[id & 0xFF](global_params->DecConfig,id,StreamContent,DecoderHandle);
    }
    else
    {
        return (MME_INVALID_ARGUMENT);
    }

    /* Set decoder PCM config based on "Last" settting */
    acc_SetDecPcmParams(&global_params->PcmParams, DecPcmProcessing);

    return (MME_SUCCESS);
}


MME_ERROR acc_setAudioDecoderInitParams(MME_GenericParams_t params_p, MME_UINT size, enum eAccDecoderId id,
                                        STAUD_StreamContent_t   StreamContent,
                                        MME_STm7100PcmProcessingGlobalParams_t *DecPcmProcessing,
                                        STAud_DecHandle_t DecoderHandle)
{
    MME_ERROR status =  MME_SUCCESS;
    TARGET_AUDIODECODER_INITPARAMS_STRUCT * init_params = (TARGET_AUDIODECODER_INITPARAMS_STRUCT  *) params_p;
    DecoderControlBlockList_t   *Dec_p = (DecoderControlBlockList_t*)NULL;

    #ifdef MSPP_PARSER
        DecoderControlBlock_t * TempDecControlBlock = (DecoderControlBlock_t *)DecoderHandle;
        STAUD_StreamType_t StreamType = TempDecControlBlock->StreamParams.StreamType;
    #endif

    Dec_p = STAud_DecGetControlBlockFromHandle(DecoderHandle);
    if(Dec_p==NULL)                                     // check for dec_p= null added
        return (MME_INVALID_HANDLE);                    // check for dec_p= null added

    if (size < sizeof(TARGET_AUDIODECODER_INITPARAMS_STRUCT))
    {
        return (MME_NOMEM);
    }

    // Set initParams Structure info
    init_params->StructSize = sizeof(TARGET_AUDIODECODER_INITPARAMS_STRUCT);


    // Set processing flow info
    init_params->BlockWise  = (ACC_MME_FALSE); // | (ACC_MME_FALSE << 16) / EnableFsRange

    switch (StreamContent)
    {
        #ifdef MSPP_PARSER
            case STAUD_STREAM_CONTENT_HE_AAC:
            case STAUD_STREAM_CONTENT_MPEG_AAC:
                // Set stream base for these audio types
                AUDIODEC_SET_STREAMBASE(init_params, ACC_MME_ENABLED);
                break;
        #endif

        case STAUD_STREAM_CONTENT_ADIF:
        case STAUD_STREAM_CONTENT_MP4_FILE:
        case STAUD_STREAM_CONTENT_RAW_AAC:
        // Set stream base for these audio types
            AUDIODEC_SET_STREAMBASE(init_params, ACC_MME_ENABLED);
            break;
        #ifdef MSPP_PARSER
            case STAUD_STREAM_CONTENT_WMA:
            case STAUD_STREAM_CONTENT_WMAPROLSL:
                // Set stream base for these audio types
                AUDIODEC_SET_STREAMBASE(init_params, ACC_MME_ENABLED);
                break;
            #endif

        default:
            break;

    }


    #ifdef MSPP_PARSER
        switch(StreamType)
        {
            case STAUD_STREAM_TYPE_PES:
                AUDIODEC_SET_PACKET_TYPE(init_params, ACC_PES_MP2);
                break;

            case STAUD_STREAM_TYPE_PES_ST:
                AUDIODEC_SET_PACKET_TYPE(init_params, ACC_WMA_ST_FILE);
                break;

            default:
                AUDIODEC_SET_PACKET_TYPE(init_params, ACC_ES);
                break;
        }
    #endif

    init_params->CacheFlush = ACC_MME_TRUE;
    init_params->SfreqRange = ACC_FSRANGE_48k;

    // Provide 10 interlaced channels output buffer with 8 main channels and 2 auxilliary channels

    if (Dec_p->DecoderControlBlock.NumChannels == 2)
    {
        init_params->NChans [ACC_MIX_MAIN] = 2;
        init_params->NChans [ACC_MIX_AUX]  = 0;
    }
    else
    {
        init_params->NChans [ACC_MIX_MAIN] = 8;
        init_params->NChans [ACC_MIX_AUX]  = 2;
    }

    init_params->ChanPos[ACC_MIX_MAIN] = ACC_MAIN_LEFT;

    init_params->ChanPos[ACC_MIX_AUX]  = ACC_AUX_LEFT;

    // Set Global Params
    acc_setAudioDecoderGlobalParams(&init_params->GlobalParams, id, StreamContent, DecPcmProcessing,DecoderHandle);

    return (status);

}


MME_ERROR acc_setAudioDecoderGlobalCmd( MME_Command_t * cmd, MME_GenericParams_t params_p, MME_UINT size,
                                        enum eAccDecPcmProcId pcmprocessing_id, partition_t     *Partition)
{
    U32   gp_size = 0;
    U8  * gp      = NULL;

    MME_ERROR                                 status      =  MME_SUCCESS;
    TARGET_AUDIODECODER_INITPARAMS_STRUCT   * init_params = (TARGET_AUDIODECODER_INITPARAMS_STRUCT  *) params_p;

    TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT * gbl_params;
    U32                                     * pcm_params;

    gbl_params  = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

    // Set command characteristics
    cmd->StructSize                     = sizeof(MME_Command_t);
    cmd->CmdCode                        = MME_SET_GLOBAL_TRANSFORM_PARAMS;
    cmd->CmdEnd                         = MME_COMMAND_END_RETURN_NOTIFY;//MME_COMMAND_END_RETURN_NO_INFO;
    cmd->DueTime                        = 0; // immediate


    // Set buffer Info
    cmd->NumberInputBuffers             = 0;
    cmd->NumberOutputBuffers            = 0;
    cmd->DataBuffers_p                  = NULL;

    // Set status Info
    cmd->CmdStatus.AdditionalInfo_p     = NULL; // do not expect status;
    cmd->CmdStatus.AdditionalInfoSize   = 0;
    cmd->CmdStatus.Error                = MME_SUCCESS;
    cmd->CmdStatus.ProcessedTime        = 0;
    cmd->CmdStatus.State                = MME_COMMAND_IDLE;

    // Set transformer specific Info
    if (pcmprocessing_id == ACC_LAST_DECPCMPROCESS_ID)
    {
        gp      = (U8 *) gbl_params;
        gp_size = gbl_params->StructSize;
    }
    else
    {
        int pcmpro_structsize, default_structsize;

        default_structsize = gbl_params->DecConfig[ACC_GP_SIZE] + 2 * sizeof(U32);
        pcmpro_structsize  = gbl_params->StructSize   - default_structsize ;
        pcm_params         = (U32 *) ((U8*)gbl_params + default_structsize);

        while((pcmpro_structsize > 0) & (pcm_params[ACC_GP_ID] != pcmprocessing_id))
        {
            pcm_params         = (U32*)((U8*)pcm_params + pcm_params[ACC_GP_SIZE]);
            pcmpro_structsize -= pcm_params[ACC_GP_SIZE];
        }

        if (pcmpro_structsize == 0)
        {
            status = MME_INVALID_ARGUMENT;
        }
        else
        {
            int size_of_processing,Index;
            int *structsize;
            gp_size = default_structsize + pcm_params[ACC_GP_SIZE];
            gp = (U8 *)STOS_MemoryAllocate(Partition, gp_size);
            if(!gp)
            {
                return MME_NOMEM; //ST_ERROR_NO_MEMORY;
            }

            // Copy DecConfig and structsize
            memcpy(gp, (U8 *)gbl_params, (unsigned int)default_structsize);

            // Copy specified PcmProcessing config
            memcpy(&gp[default_structsize], pcm_params, pcm_params[ACC_GP_SIZE]);

            // Back annotate structsizes
            * ((U32 *)gp) = gp_size;
            size_of_processing =  pcm_params[ACC_GP_SIZE];
            size_of_processing += sizeof(U32);
            Index = default_structsize - sizeof(U32);
            #ifndef ST_51XX
                structsize = (int*)&gp[Index];
            #else
                structsize = (int*)&gp[Index];
            #endif
            *structsize =  size_of_processing;
        }
    }

    // Send the command
    if (status == MME_SUCCESS)
    {
        cmd->Param_p    = gp;
        cmd->ParamSize  = gp_size;
    }

    return (status);
}

MME_ERROR acc_setAudioDecoderTransformCmd(  MME_Command_t    * cmd,
                                            MME_GenericParams_t params_p, MME_UINT params_size,
                                            MME_GenericParams_t status_p, MME_UINT status_size,
                                            MME_DataBuffer_t **buffer_array,
                                            enum eAccCmdCode      playback_mode, U32 pause_duration, enum eAccBoolean restart,
                                            U32 numInputBuffers, U32 numOutputBuffers)
{
    MME_ERROR                         status       =  MME_SUCCESS;
    MME_LxAudioDecoderFrameParams_t * frame_params = (MME_LxAudioDecoderFrameParams_t *) params_p;

    U32 playback = playback_mode & 0x7FFF;

    if (playback == ACC_CMD_MUTE)
    {
        playback = ACC_CMD_MUTE;
    }

    // Set command characteristics
    cmd->StructSize                     = sizeof(MME_Command_t);
    cmd->CmdCode                        = MME_TRANSFORM;
    cmd->CmdEnd                         = MME_COMMAND_END_RETURN_NOTIFY;
    cmd->DueTime                        = 0; // immediate

    cmd->NumberInputBuffers             = numInputBuffers;//1;
    cmd->NumberOutputBuffers            = numOutputBuffers;//1;
    cmd->DataBuffers_p                  = buffer_array;

    // Set status Info
    cmd->CmdStatus.AdditionalInfo_p     = status_p;
    cmd->CmdStatus.AdditionalInfoSize   = status_size;
    cmd->CmdStatus.Error                = MME_SUCCESS;
    cmd->CmdStatus.ProcessedTime        = 0;
    cmd->CmdStatus.State                = MME_COMMAND_IDLE;

    // Set transform specific info
    cmd->Param_p                        = params_p;
    cmd->ParamSize                      = params_size;
    frame_params->Restart               = restart;
    frame_params->Cmd                   = (U16) playback  ;
    frame_params->PauseDuration         = (U16) pause_duration;

    return (status);
}



MME_ERROR acc_setAudioDecoderBufferCmd( MME_Command_t           * cmd,
                                        MME_GenericParams_t     params_p, MME_UINT params_size,
                                        MME_GenericParams_t     status_p, MME_UINT status_size,
                                        MME_DataBuffer_t        **buffer_array,
                                        enum eAccCmdCode        playback_mode, enum eAccBoolean restart,
                                        enum eAccBoolean        last_data_block,
                                        STAUD_StreamContent_t   stream_content)
{
    MME_ERROR                         status            =  MME_SUCCESS;
    MME_LxAudioDecoderBufferParams_t * buffer_params    = (MME_LxAudioDecoderBufferParams_t *) params_p;

    U32 playback = playback_mode & 0x7FFF;

    if (playback == ACC_CMD_MUTE)
    {
        playback = ACC_CMD_MUTE;
    }

    // Set command characteristics
    cmd->StructSize = sizeof(MME_Command_t);
    cmd->CmdCode    = MME_SEND_BUFFERS;
    cmd->CmdEnd     = MME_COMMAND_END_RETURN_NOTIFY;
    cmd->DueTime    = 0; // immediate

    cmd->NumberOutputBuffers = 0;
    if (playback == ACC_CMD_PAUSE)
    {
        // Set buffer Info
        cmd->NumberInputBuffers  = 0;
    }
    else
    {
        cmd->NumberInputBuffers  = 1;
    }
    cmd->DataBuffers_p = buffer_array;

    // Set status Info
    cmd->CmdStatus.AdditionalInfo_p     = status_p;
    cmd->CmdStatus.AdditionalInfoSize   = status_size;
    cmd->CmdStatus.Error                = MME_SUCCESS;
    cmd->CmdStatus.ProcessedTime        = 0;
    cmd->CmdStatus.State                = MME_COMMAND_IDLE;

    // Set transform specific info
    cmd->Param_p                        = params_p;
    cmd->ParamSize                      = params_size;


    buffer_params->StructSize = sizeof(MME_LxAudioDecoderBufferParams_t);

    if (!last_data_block)
    {
        buffer_params->BufferParams[STREAMING_BUFFER_TYPE] |= STREAMING_SET_BUFFER_TYPE(buffer_params->BufferParams,STREAMING_DEC_RUNNING);
    }
    else
    {
        buffer_params->BufferParams[STREAMING_BUFFER_TYPE] |= STREAMING_SET_BUFFER_TYPE(buffer_params->BufferParams,STREAMING_DEC_EOF);
    }

    return (status);
}


static MME_ERROR acc_setAudioParserGlobalParams(TARGET_AUDIOPARSER_GLOBALPARAMS_STRUCT *global_params,
                    enum eAccDecoderId id,  enum eAccPacketCode packcode,MspParser_Handle_t  ParserHandle)
{
    // Set globalParams Structure info
    MME_DecConfig_t *DecConfig  = (MME_DecConfig_t *) (global_params->DecConfig);
    global_params->StructSize   = sizeof(TARGET_AUDIOPARSER_GLOBALPARAMS_STRUCT);

    DecConfig->DecoderId        = (U32)id;
    DecConfig->StructSize       = (U32)sizeof(MME_DecConfig_t);
    DecConfig->Config           = (U32)packcode;/*Is it required*/
    return (MME_SUCCESS);
}


MME_ERROR acc_setAudioParserInitParams(MME_GenericParams_t params_p, MME_UINT size, enum eAccDecoderId id,
         enum eAccPacketCode packcode,MspParser_Handle_t  ParserHandle)
{
    MME_ERROR status =  MME_SUCCESS;
    TARGET_AUDIOPARSER_INITPARAMS_STRUCT * init_params = (TARGET_AUDIOPARSER_INITPARAMS_STRUCT  *) params_p;

    if (size < sizeof(TARGET_AUDIOPARSER_INITPARAMS_STRUCT))
    {
        return (MME_NOMEM);
    }
    // Set initParams Structure info
    init_params->StructSize = sizeof(TARGET_AUDIOPARSER_INITPARAMS_STRUCT);

    // Set stream type
    init_params->BlockWise  = (ACC_MME_FALSE) | (packcode << 24); // | (ACC_MME_FALSE << 16) / EnableFsRange
    init_params->CacheFlush = ACC_MME_TRUE;

    // Set Global Params
    acc_setAudioParserGlobalParams(&init_params->GlobalParams,id,packcode,ParserHandle);

    return (status);

}


MME_ERROR acc_setAudioParserGlobalCmd( MME_Command_t * cmd, MME_GenericParams_t params_p, MME_UINT size,
            partition_t     *Partition , enum eAccDecoderId id)
{

    MME_ERROR                                 status        =  MME_SUCCESS;
    TARGET_AUDIOPARSER_INITPARAMS_STRUCT   * init_params    = (TARGET_AUDIOPARSER_INITPARAMS_STRUCT  *) params_p;
    TARGET_AUDIOPARSER_GLOBALPARAMS_STRUCT * gbl_params     =  (TARGET_AUDIOPARSER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;
    MME_DecConfig_t *DecConfig                              = (MME_DecConfig_t *) (gbl_params->DecConfig);

    // Set command characteristics
    cmd->StructSize = sizeof(MME_Command_t);
    cmd->CmdCode    = MME_SET_GLOBAL_TRANSFORM_PARAMS;
    cmd->CmdEnd     = MME_COMMAND_END_RETURN_NOTIFY;//MME_COMMAND_END_RETURN_NO_INFO;
    cmd->DueTime    = 0; // immediate


    // Set buffer Info
    cmd->NumberInputBuffers             = 0;
    cmd->NumberOutputBuffers            = 0;
    cmd->DataBuffers_p                  = NULL;

    cmd->Param_p                        = (U8 *) gbl_params;;
    cmd->ParamSize                      = gbl_params->StructSize;

    // Set status Info
    cmd->CmdStatus.AdditionalInfo_p     = NULL; // do not expect status;
    cmd->CmdStatus.AdditionalInfoSize   = 0;
    cmd->CmdStatus.Error                = MME_SUCCESS;
    cmd->CmdStatus.ProcessedTime        = 0;
    cmd->CmdStatus.State                = MME_COMMAND_IDLE;

    DecConfig->DecoderId                = (U32)id;
    DecConfig->StructSize               = (U32)sizeof(MME_DecConfig_t);

    return (status);
}

MME_ERROR acc_setAudioParserTransformCmd(   MME_Command_t    * cmd,
                                            MME_GenericParams_t params_p, MME_UINT params_size,
                                            MME_GenericParams_t status_p, MME_UINT status_size,
                                            MME_DataBuffer_t **buffer_array,
                                            enum eAccCmdCode      playback_mode, U32 CmdDuration, enum eAccBoolean restart,
                                            U32 numInputBuffers, U32 numOutputBuffers)
{
    MME_ERROR                         status    =  MME_SUCCESS;
    MME_AudioParserFrameParams_t * frame_params = (MME_AudioParserFrameParams_t *) params_p;

    U32 playback = playback_mode & 0x7FFF;

    if (playback == ACC_CMD_MUTE)
    {
        playback    = ACC_CMD_MUTE;
    }

    // Set command characteristics
    cmd->StructSize                     = sizeof(MME_Command_t);
    cmd->CmdCode                        = MME_TRANSFORM;
    cmd->CmdEnd                         = MME_COMMAND_END_RETURN_NOTIFY;
    cmd->DueTime                        = 0; // immediate

    cmd->NumberInputBuffers             = numInputBuffers;//1;
    cmd->NumberOutputBuffers            = numOutputBuffers;//1;
    cmd->DataBuffers_p                  = buffer_array;

    // Set status Info
    cmd->CmdStatus.AdditionalInfo_p     = status_p;
    cmd->CmdStatus.AdditionalInfoSize   = status_size;

    cmd->CmdStatus.Error                = MME_SUCCESS;
    cmd->CmdStatus.ProcessedTime        = 0;
    cmd->CmdStatus.State                = MME_COMMAND_IDLE;

    // Set transform specific info
    cmd->Param_p   = params_p;
    cmd->ParamSize = params_size;

    frame_params->Restart               = restart;
    frame_params->Cmd                   = (U16) playback  ;
    frame_params->CmdDuration           = (U16) CmdDuration;

    return (status);
}



MME_ERROR acc_setAudioParserBufferCmd(  MME_Command_t       * cmd,
                                        MME_GenericParams_t params_p, MME_UINT params_size,
                                        MME_GenericParams_t status_p, MME_UINT status_size,
                                        MME_DataBuffer_t    **buffer_array,
                                        enum eAccCmdCode    playback_mode, enum eAccBoolean restart,
                                        enum eAccBoolean    last_data_block,
                                        STAUD_StreamContent_t stream_content,enum eAccBoolean eof)
{
    MME_ERROR                         status       =  MME_SUCCESS;
    MME_AudioParserBufferParams_t * buffer_params = (MME_AudioParserBufferParams_t *) params_p;

    U32 playback = playback_mode & 0x7FFF;
    if (playback == ACC_CMD_MUTE)
    {
        playback = ACC_CMD_MUTE;
    }

        // Set command characteristics
    cmd->StructSize             = sizeof(MME_Command_t);
    cmd->CmdCode                = MME_SEND_BUFFERS;
    cmd->CmdEnd                 = MME_COMMAND_END_RETURN_NOTIFY;
    cmd->DueTime                = 0; // immediate
    cmd->NumberOutputBuffers    = 0;

    if (playback == ACC_CMD_PAUSE)
    {
        // Set buffer Info
        cmd->NumberInputBuffers  = 0;
    }
    else
    {
        cmd->NumberInputBuffers  = 1;
    }
    cmd->DataBuffers_p = buffer_array;

    // Set status Info
    cmd->CmdStatus.AdditionalInfo_p     = status_p;
    cmd->CmdStatus.AdditionalInfoSize   = status_size;
    cmd->CmdStatus.Error                = MME_SUCCESS;
    cmd->CmdStatus.ProcessedTime        = 0;
    cmd->CmdStatus.State                = MME_COMMAND_IDLE;

    // Set transform specific info
    cmd->Param_p                        = params_p;
    cmd->ParamSize                      = params_size;
    buffer_params->StructSize           = sizeof(MME_AudioParserBufferParams_t);

    if(eof)
        buffer_params->BufferParams[AUDIOPARSER_BUFFER_TYPE]=(U32)AUDIOPARSER_EOF;
    else
        buffer_params->BufferParams[AUDIOPARSER_BUFFER_TYPE]=(U32)AUDIOPARSER_STREAM;

    return (status);
}

MME_ERROR acc_setAudioMixerTransformCmd(    MME_Command_t    * cmd,
                                            MME_GenericParams_t params_p, MME_UINT params_size,
                                            MME_GenericParams_t status_p, MME_UINT status_size,
                                            MME_DataBuffer_t **buffer_array,
                                            enum eAccCmdCode      playback_mode, U32 InBufNum)
{

    MME_ERROR   status  =  MME_SUCCESS;
    U32         i;

    MME_LxMixerTransformerFrameDecodeParams_t * mix_params = (MME_LxMixerTransformerFrameDecodeParams_t *) params_p;

    U32         playback = playback_mode & 0x7FFF;


    if (playback == ACC_CMD_MUTE)
        playback = ACC_CMD_MUTE;
    // Set command characteristics
    cmd->StructSize                     = sizeof(MME_Command_t);
    cmd->CmdCode                        = MME_TRANSFORM;
    cmd->CmdEnd                         = MME_COMMAND_END_RETURN_NOTIFY;
    cmd->DueTime                        = 0; // immediate

    cmd->NumberInputBuffers             = InBufNum;/*1;*/
    cmd->NumberOutputBuffers            = 1;
    cmd->DataBuffers_p = buffer_array;

    // Set status Info
    cmd->CmdStatus.AdditionalInfo_p     = status_p;
    cmd->CmdStatus.AdditionalInfoSize   = status_size;
    cmd->CmdStatus.Error                = MME_SUCCESS;
    cmd->CmdStatus.ProcessedTime        = 0;
    cmd->CmdStatus.State                = MME_COMMAND_IDLE;

    // Set transform specific info
    cmd->Param_p                        = params_p;
    cmd->ParamSize                      = params_size;

    for(i = 0;i<InBufNum;i++)
    {
        mix_params->InputParam[i].Command       =   (U16) playback  ;
        mix_params->InputParam[i].StartOffset   =   0;
    }

    return (status);
}

MME_ERROR acc_setAudioPPTransformCmd(   MME_Command_t       * cmd,
                                        MME_GenericParams_t params_p, MME_UINT params_size,
                                        MME_GenericParams_t status_p, MME_UINT status_size,
                                        MME_DataBuffer_t    **buffer_array,
                                        enum eAccCmdCode    playback_mode)
{

    MME_ERROR                         status       =  MME_SUCCESS;
    MME_LxMixerTransformerFrameDecodeParams_t * pp_params = (MME_LxMixerTransformerFrameDecodeParams_t *) params_p;

    U32 playback = playback_mode & 0x7FFF;


    if (playback == ACC_CMD_MUTE)
    {
        playback = ACC_CMD_MUTE;
    }

    // Set command characteristics
    cmd->StructSize                     = sizeof(MME_Command_t);
    cmd->CmdCode                        = MME_TRANSFORM;
    cmd->CmdEnd                         = MME_COMMAND_END_RETURN_NOTIFY;
    cmd->DueTime                        = 0; // immediate
    cmd->NumberInputBuffers             = 1;
    cmd->NumberOutputBuffers            = 1;
    cmd->DataBuffers_p                  = buffer_array;

    // Set status Info
    cmd->CmdStatus.AdditionalInfo_p     = status_p;
    cmd->CmdStatus.AdditionalInfoSize   = status_size;
    cmd->CmdStatus.Error                = MME_SUCCESS;
    cmd->CmdStatus.ProcessedTime        = 0;
    cmd->CmdStatus.State                = MME_COMMAND_IDLE;

    // Set transform specific info
    cmd->Param_p   = params_p;
    cmd->ParamSize = params_size;

    pp_params->InputParam[0].Command    = (U16) playback  ;
    pp_params->InputParam[0].StartOffset=0;

    return (status);
}


enum eAccLpcmMode CovertToLPCMMode (U8 lpcmmode)
{
    enum eAccLpcmMode code;

    switch(lpcmmode)
    {
        case 17:
            code =ACC_LPCM_IMA_ADPCM;
            break;

        case 2:
            code =ACC_LPCM_MS_ADPCM;
            break;

        default:
            code = ACC_RAW_PCM;
            break;
    }

    return code;

}

enum eAccFsCode CovertToFsCode(U32 SampleRate)
{

    enum eAccFsCode code;

    switch(SampleRate)
    {
        case 32000:
            code = ACC_FS32k;
            break;

        case 44100:
            code = ACC_FS44k;
            break;

        case 48000:
            code = ACC_FS48k;
            break;

        case 96000:
            code = ACC_FS96k;
            break;

        case 88200:
            code = ACC_FS88k;
            break;

        case 64000:
            code = ACC_FS64k;
            break;

        case 192000:
            code = ACC_FS192k;
            break;

        case 176400:
            code = ACC_FS176k;
            break;

        case 128000:
            code = ACC_FS128k;
            break;

        case 384000:
            code = ACC_FS384k;
            break;

        case 352000:
            code = ACC_FS352k;
            break;

        case 256000:
            code = ACC_FS256k;
            break;

        case 12000:
            code = ACC_FS12k;
            break;

        case 11025:
            code = ACC_FS11k;
            break;

        case 8000:
            code = ACC_FS8k;
            break;

        case 24000:
            code = ACC_FS24k;
            break;

        case 22050:
            code = ACC_FS22k;
            break;

        case 16000:
            code = ACC_FS16k;
            break;

        default:
            code = ACC_FS48k;
            break;
    }
    return code;
}

U32 CovertFsCodeToSampleRate(enum eAccFsCode FsCode)
{
    U32 SampleRate = 0;

    switch(FsCode)
    {

        case ACC_FS48k:
            SampleRate = 48000;
            break;

        case ACC_FS44k:
            SampleRate = 44100;
            break;

        case ACC_FS32k:
            SampleRate = 32000;
            break;

        case ACC_FS96k:
            SampleRate = 96000;
            break;

        case ACC_FS88k:
            SampleRate = 88200;
            break;

        case ACC_FS64k:
            SampleRate = 64000;
            break;

        case ACC_FS192k:
            SampleRate = 192000;
            break;

        case ACC_FS176k:
            SampleRate = 176400;
            break;

        case ACC_FS128k:
            SampleRate = 128000;
            break;

        case ACC_FS384k:
            SampleRate = 384000;
            break;

        case ACC_FS352k:
            SampleRate = 352000;
            break;

        case ACC_FS256k:
            SampleRate = 256000;
            break;

        case ACC_FS12k:
            SampleRate = 12000;
            break;

        case ACC_FS11k:
            SampleRate = 11025;
            break;

        case ACC_FS8k:
            SampleRate = 8000;
            break;

        case ACC_FS24k:
            SampleRate = 24000;
            break;

        case ACC_FS22k:
            SampleRate = 22050;
            break;

        case ACC_FS16k:
            SampleRate = 16000;
            break;

        default:
            SampleRate = 48000;
            break;
        }
    return SampleRate;
}

enum eAccLpcmFs CovertToFsLPCMCode(U32 SampleRate)
{

    enum eAccLpcmFs code;

    switch(SampleRate)
    {
        case 44100:
            code = ACC_LPCM_FS44;
            break;

        case 48000:
            code = ACC_LPCM_FS48;
            break;

        case 88200:
            code = ACC_LPCM_FS88;
            break;

        case 96000:
            code = ACC_LPCM_FS96;
            break;

        case 192000:
            code = ACC_LPCM_FS192;
            break;

        case 176400:
            code = ACC_LPCM_FS176;
            break;

        default:
            code = ACC_LPCM_FS48;
            break;
        }
    return code;
}

U32 CovertFsLPCMCodeToSampleRate(enum eAccLpcmFs FsCode)
{
    U32 SampleRate = 0;

    switch(FsCode)
    {
        case ACC_LPCM_FS48:
            SampleRate = 48000;
            break;

        case ACC_LPCM_FS96:
            SampleRate = 96000;
            break;

        case ACC_LPCM_FS192:
            SampleRate = 192000;
            break;

        case ACC_LPCM_FS44:
            SampleRate = 44100;
            break;

        case ACC_LPCM_FS88:
            SampleRate = 88200;
            break;

        case ACC_LPCM_FS176:
            SampleRate = 176400;
            break;

        default:
            SampleRate = 48000;
            break;
        }
    return SampleRate;
}

enum eAccLpcmWs CovertToLpcmWSCode(U32 SampleSize)
{
    enum eAccLpcmWs code;

    switch(SampleSize)
    {
        case 8:
            code = ACC_LPCM_WS8s;
            break;

        case 16:
            code = ACC_LPCM_WS16;
            break;

        case 20:
            code = ACC_LPCM_WS20;
            break;

        case 24:
            code = ACC_LPCM_WS24;
            break;

        case 65536:
            code = ACC_LPCM_WS16le; // 16bit little endian
            break;

        case 131072:
            code = ACC_LPCM_WS16u;  // 16bit unsigned
            break;

        case 196608:
            code = ACC_LPCM_WS16ule;
            break;

        case 17:
            code = ACC_LPCM_WS8u;
            break;

        default:
            code = ACC_LPCM_WS16;
            break;
        }
    return code;
}

U8 CovertFromLpcmWSCode(enum eAccLpcmWs WsCode)
{
    U8 WordSize = 0;

    switch(WsCode)
    {
        case ACC_LPCM_WS8u:
        case ACC_LPCM_WS8s:
            WordSize = 1;
            break;

        case ACC_LPCM_WS16:
        case ACC_LPCM_WS16le:
        case ACC_LPCM_WS16u:
        case ACC_LPCM_WS16ule:
            WordSize = 2 ;
            break;

        case ACC_LPCM_WS20:
            WordSize = 5 ;
            break;

        case ACC_LPCM_WS24:
            WordSize = 3 ;
            break;

        default:
            WordSize = 2;
            break;
        }
    return WordSize;
}

enum eAccWordSizeCode ConvertToAccWSCode(U32 SampleSize)
{
    enum eAccWordSizeCode code;

    switch(SampleSize)
    {
        case 32:
            code = ACC_WS32;
            break;

        case 16:
            code = ACC_WS16;
            break;

        case 8:
            code = ACC_WS8;

        default:
            code = ACC_WS32;
            break;
    }
    return code;
}

U32 ConvertFromAccWSCode(enum eAccWordSizeCode code)
{
    U32 SampleSize;

    switch(code)
    {
        case ACC_WS32:
            SampleSize = 32;
            break;

        case ACC_WS16:
            SampleSize = 16;
            break;

        case ACC_WS8:
            SampleSize = 8;
            break;

        default:
            SampleSize = 32;
            break;
    }
    return SampleSize;
}



