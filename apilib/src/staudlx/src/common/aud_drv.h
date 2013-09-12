/*
******************************************************************************
AUD_DRV.H - (C)  Copyright STMicroelectronics
******************************************************************************
*/

#ifndef __AUDDRV_H
    #define __AUDDRV_H

/*
******************************************************************************
Includes
******************************************************************************
*/
    #include "staudlx.h"
    //#include "aud_pcmplayer.h"
    #ifndef ST_51XX
        #include "acc_mme.h"

        #ifndef ST_5197
            #include "embx.h"
            #include "embxmailbox.h"
            #include "embxshm.h"
        #endif

        #include "audio_decodertypes.h"
        #include "pcm_postprocessingtypes.h"
        #include "aud_decoderparams.h"

    #else
        /*Do Nothing*/
    #endif




    #ifndef STAUD_REMOVE_CLKRV_SUPPORT
        #include "stclkrv.h"
    #endif

    #if defined(ST_8010)
        /* AUDIO ------------------------------------------------------------------- */
        #ifndef SPDIF_BASE_ADDRESS
            #define SPDIF_BASE_ADDRESS          0x57006000
        #endif
    #endif

    /*
    ******************************************************************************
    Exported Constants
    ******************************************************************************
    */
    #define AUDIO_INVALID_HANDLE            ((STAUD_Handle_t)0xFFFFFFFF)
    #define AUDIO_VALID_DECODER             0xFEDBCA98

    /* Linux Base Addresses (Common Structure for 71xx and 72xx) */
    typedef struct AudioBaseAddress
    {
        U32 PCMPlayer0BaseAddr;
        U32 PCMPlayer1BaseAddr;
        U32 PCMPlayer2BaseAddr;
        U32 PCMPlayer3BaseAddr;
        U32 HDMIPCMPlayerBaseAddr;
        U32 SPDIFPlayerBaseAddr;
        U32 HDMISPDIFPlayerBaseAddr;
        U32 PCMReaderBaseAddr;
        U32 AudioConfigBaseAddr;
        U32 I2S2SpdifConverterBaseAddr;
        U32 I2S2SpdifConverterBaseAddr1;
        U32 I2S2SpdifConverterBaseAddr2;
        U32 I2S2SpdifConverterBaseAddr3;
        U32 SPDIFFDMAAdditionalDataRegionBaseAddr;
        U32 HDTVOutAuxGlueBaseAddr;
        U32 HDMISyncConfigBaseAddr;
        U32 SystemConfigBaseAddr;
        U32 SystemServiceBaseAddr;
        U32 NHD2ConfigBaseAddr;
	 U32 HDMIBaseAddress;
        BOOL IsInit;
    } AudioBaseAddress_t;

    #if defined(ST_OSLINUX)
        extern AudioBaseAddress_t BaseAddress;
    #endif

    /*
    ******************************************************************************
    Exported Variables (Globals, not static)
    ******************************************************************************
    */

    /*
    ******************************************************************************
    Exported Macros
    ******************************************************************************
    */
    #if defined (ST_7200)

    /*There is documentation problem wrt addresses of I2S_SPDIF_CONVERTER_BASE_ADDRESS for 7200
        These settings are recieved from HDMI player validation*/

        #define I2S_SPDIF_CONVERTER_BASE_ADDRESS_0      ((U32)ST7200_SPDIF_PLAYER_BASE_ADDRESS + 0x1000)
        #define I2S_SPDIF_CONVERTER_BASE_ADDRESS_1      ((U32)ST7200_SPDIF_PLAYER_BASE_ADDRESS + 0x1400)
        #define I2S_SPDIF_CONVERTER_BASE_ADDRESS_2      ((U32)ST7200_SPDIF_PLAYER_BASE_ADDRESS + 0x1800)
        #define I2S_SPDIF_CONVERTER_BASE_ADDRESS_3      ((U32)ST7200_SPDIF_PLAYER_BASE_ADDRESS + 0x1c00)
        #define HD_TVOUT_AUX_GLUE_BASE_ADDRESS          ST7200_HD_TVOUT_AUX_GLUE_BASE_ADDRESS
        #define HDMI_SYNC_CONFIG_BASE_ADDRESS           ((U32)ST7200_HDMI_BASE_ADDRESS + 0x20)
        #define I2S_SPDIF_CONVERTER_REGION_SIZE         (0x224)
    #else
        #define I2S_SPDIF_CONVERTER_BASE_ADDRESS        ((U32)SPDIF_BASE_ADDRESS + 0x800)
        #define I2S_SPDIF_CONVERTER_REGION_SIZE         (0x224)
    #endif


    /* offset from start */
    #define I2S_AUD_SPDIF_PR_CFG                        0x0
    #define I2S_AUD_SPDIF_PR_SPDIF_CTRL                 0x200


    #define SET_AUDIO_DECODER_STATUS(d, s)              (d->Status = s)
    #define GET_AUDIO_DECODER_STATUS(d)                 (d->Status)

    #define IS_VALID_AUDIO_DECODER(d)                   (d ? (d->Flag == AUDIO_VALID_DECODER) : 0)

    /*-------------------------- */
    #ifndef ST_OSLINUX
        #define STSYS_WriteAudioReg32(a, v)                 ( *((volatile U32*)(((U32)AUDIO_BASE_ADDRESS) + (a))) = ((U32)v) )
        #define STSYS_ReadAudioReg32(a)                     ( *(volatile U32*)(((U32)AUDIO_BASE_ADDRESS) + (a)) )

        #define STSYS_WritePcmPlayerReg32(a, v)             ( *((volatile U32*)(((U32)PCM_PLAYER_BASE_ADDRESS) + (a))) = ((U32)v) )
        #define STSYS_ReadPcmPlayerReg32(a)                 ( *(volatile U32*)(((U32)PCM_PLAYER_BASE_ADDRESS) + (a)) )

        #define STSYS_WriteSpdifPlayerReg32(a, v)           ( *((volatile U32*)(((U32)SPDIF_BASE_ADDRESS) + (a))) = ((U32)v) )
        #define STSYS_ReadSpdifPlayReg32(a)                 ( *(volatile U32*)(((U32)SPDIF_BASE_ADDRESS) + (a)) )

        #define STSYS_WriteI2S2SpdifConverterReg32(a, v)    ( *((volatile U32*)(((U32)I2S_SPDIF_CONVERTER_BASE_ADDRESS) + (a))) = ((U32)v) )
        #define STSYS_ReadI2S2SpdifConverterReg32(a)        ( *(volatile U32*)(((U32)I2S_SPDIF_CONVERTER_BASE_ADDRESS) + (a)) )

        #define STSYS_WriteIcMonitorReg32(a, v)             ( *((volatile U32*)(((U32)ICMONITOR_BASE_ADDRESS) + (a))) = ((U32)v) )
        #define STSYS_ReadIcMonitorReg32(a)                 ( *(volatile U32*)(((U32)ICMONITOR_BASE_ADDRESS) + (a)) )

        #define STSYS_WriteIcControlReg32(a, v)             ( *((volatile U32*)(((U32)ICCONTROL_BASE_ADDRESS) + (a))) = ((U32)v) )
        #define STSYS_ReadIcControlReg32(a)                 ( *(volatile U32*)(((U32)ICCONTROL_BASE_ADDRESS) + (a)) )

        #define STSYS_WriteAudioFreqSynthesizerReg32(a, v)  ( *((volatile U32*)(((U32)SYS_SERVICES_BASE_ADDRESS) + (a))) = ((U32)v) )
        #define STSYS_ReadAudioFreqSynthesizerReg32(a)      ( *(volatile U32*)(((U32)SYS_SERVICES_BASE_ADDRESS) + (a)) )

        #define STSYS_ReadPtiReg32(a)                       ( *(volatile U32*)(((U32)PTI_BASE_ADDRESS) + (a)) )

        #define STSYS_WriteSpdifHDMIPlayerReg32(a, v)       ( *((volatile U32*)(((U32)ST7200_HDMI_BASE_ADDRESS+0x0C00) + (a))) = ((U32)v) )
        #define STSYS_ReadSpdifHDMIPlayerReg32(a)           ( *(volatile U32*)(((U32)ST7200_HDMI_BASE_ADDRESS+0x0C00) + (a)) )

    #else

        #define STSYS_WriteAudioReg32(a,v)                  STSYS_WriteRegDev32LE((void*)BaseAddress.AudioConfigBaseAddr + (a), v)
        #define STSYS_ReadAudioReg32(a)                     STSYS_ReadRegDev32LE((void*)BaseAddress.AudioConfigBaseAddr + (a))

        #define STSYS_WritePcmPlayerReg32(a,v)              STSYS_WriteRegDev32LE((void*)BaseAddress.PCMPlayer0BaseAddr + (a), v)
        #define STSYS_ReadPcmPlayerReg32(a)                 STSYS_ReadRegDev32LE((void*)BaseAddress.PCMPlayer0BaseAddr + (a))

        #define STSYS_WriteSpdifPlayerReg32(a,v)            STSYS_WriteRegDev32LE((void*)BaseAddress.SPDIFPlayerBaseAddr + (a), v)
        #define STSYS_ReadSpdifPlayReg32(a)                 STSYS_ReadRegDev32LE((void*)BaseAddress.SPDIFPlayerBaseAddr + (a))

        #define STSYS_WriteI2S2SpdifConverterReg32(a,v)     STSYS_WriteRegDev32LE((void*)BaseAddress.I2C2SpdifConverter + (a), v)
        #define STSYS_ReadI2S2SpdifConverterReg32(a)        STSYS_ReadRegDev32LE((void*)BaseAddress.I2C2SpdifConverter + (a))

        #define STSYS_WriteSpdifHDMIPlayerReg32(a, v)       STSYS_WriteRegDev32LE((void*)BaseAddress.HDMISPDIFPlayerBaseAddr + (a), v)
        #define STSYS_ReadSpdifHDMIPlayerReg32(a)           STSYS_ReadRegDev32LE((void*)BaseAddress.HDMISPDIFPlayerBaseAddr + (a))

    #endif

#endif /* #ifndef __AUD_DRV_H */

/* ------------------------------- End of file ---------------------------- */
