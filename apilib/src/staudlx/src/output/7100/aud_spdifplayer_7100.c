/*
 * Copyright (C) STMicroelectronics Ltd. 2004.
 *
 * All rights reserved.
 *            ------->SPDIF PLAYER
 *            |
 * DECODER -------|
 *            |
 *            ------->PCM PLAYER
 */
//#define  STTBX_PRINT
#ifndef ST_OSLINUX
#define __OS_20TO21_MAP_H /* prevent os_20to21_map.h from being included*/
#include "sttbx.h"
#endif
#include "stos.h"
#include "aud_spdifplayer.h"
#include "audioreg.h"


/* ---------------------------------------------------------------------------- */
/*                               Private Types                                  */
/* ---------------------------------------------------------------------------- */
extern SPDIFPlayerControlBlock_t *spdifPlayerControlBlock;

ST_ErrorCode_t  SetSPDIFPlayerSamplingFrequency(SPDIFPlayerControl_t * SPDIFPlayerControl_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 SamplingFreqency = SPDIFPlayerControl_p->samplingFrequency;

    switch (SPDIFPlayerControl_p->spdifPlayerIdentifier)
    {
        case SPDIF_PLAYER_0:            
            if(SPDIFPlayerControl_p->StreamParams.StreamContent == STAUD_STREAM_CONTENT_MP3)
            {
                switch(SamplingFreqency)
                {
                    case 48000:
                    case 44100:
                    case 32000:
                    default:
                        SPDIFPlayerControl_p->CurrentBurstPeriod = 1152;
                        break;       
                        
                    case 24000:
                    case 22050:
                    case 16000:
                        SPDIFPlayerControl_p->CurrentBurstPeriod = 576;
                        break;
                        
                    case 12000:
                    case 11025:
                    case 8000:
                        SPDIFPlayerControl_p->CurrentBurstPeriod = 288;
                        break;      
                        
                }
            }
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
            Error = STCLKRV_SetNominalFreq( SPDIFPlayerControl_p->CLKRV_Handle, STCLKRV_CLOCK_SPDIF_0, (SPDIFPlayerControl_p->SPDIFPlayerOutParams.SPDIFPlayerFrequencyMultiplier * SamplingFreqency)); // for HDMI
#else
            Error = SPDIFPlayer_SetFrequencySynthesizer(SPDIFPlayerControl_p, SamplingFreqency);
#endif
            STTBX_Print(("Setting new SPDIF frequency %d\n",SamplingFreqency));
            break;
        
        default:
            //STTBX_Print(("Unidentified SPDIF player identifier\n"));
            Error = ST_ERROR_UNKNOWN_DEVICE;
            break;
    }
    return (Error);
    
}

ST_ErrorCode_t SPDIFPlayer_SetFrequencySynthesizer (SPDIFPlayerControl_t * Control_p, U32 samplingFrequency)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 sdiv = 0, md = 0, pe = 0;
    U32 AudioConfigBaseAddr = Control_p->BaseAddress.AudioConfigBaseAddr;
    
    switch (Control_p->samplingFrequency)
    {
#ifdef  STCLKRV_EXT_CLK_MHZ
        case 48000:
#if STCLKRV_EXT_CLK_MHZ==27
            md = 0x11;
            pe = 0x3600;
            sdiv  = 0x5;
#endif
#if STCLKRV_EXT_CLK_MHZ==30
            md = 0x13;
            pe = 0x3C00;
            sdiv  = 0x5;
#endif
            break;

            case 44100:
#if STCLKRV_EXT_CLK_MHZ==27
            md = 0x13;
            pe = 0x6F05;
            sdiv  = 0x5;
#endif
#if STCLKRV_EXT_CLK_MHZ==30
            md = 0x15;
            pe = 0x5EE9;
            sdiv  = 0x5;
#endif
            break;

        case 32000:            
#if STCLKRV_EXT_CLK_MHZ==27
            md = 0x1A;
            pe = 0x5100;
            sdiv  = 0x5;
#endif
#if STCLKRV_EXT_CLK_MHZ==30
            md = 0x1D;
            pe = 0x5A00;
            sdiv  = 0x5;
#endif
            break;

#endif //#ifdef   STCLKRV_EXT_CLK_MHZ

        default :
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }

    // This sets the audio freq synthesizer should be moved to a place common for all players
    //STSYS_WriteRegDev32LE(AUDIO_FSYN_CFG, 0x7fef);
    //STSYS_WriteRegDev32LE(AUDIO_FSYN_CFG, 0x7fee);

    switch (Control_p->spdifPlayerIdentifier)
    {
        case SPDIF_PLAYER_0:
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x3C , 0x0);//AUDIO_FSYN2_PROG_EN
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x30 , md);//AUDIO_FSYN2_MD
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x34 , pe);//AUDIO_FSYN2_PE
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x38 , sdiv);//AUDIO_FSYN2_SDIV
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x3C, 0x1);//AUDIO_FSYN2_PROG_EN
            
            break;
            
        default:
            //STTBX_Print(("Unidentified SPDIF player identifier\n"));
            Error = ST_ERROR_UNKNOWN_DEVICE;
            break;
    }

    return Error;
}

void StopSPDIFPlayerIP(SPDIFPlayerControl_t * SPDIFPlayerControl_p)
{
    U32 SPDIFPlayerBaseAddr = SPDIFPlayerControl_p->BaseAddress.SPDIFPlayerBaseAddr;
    switch(SPDIFPlayerControl_p->spdifPlayerIdentifier)
    {
        case SPDIF_PLAYER_0:
            STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_CTRL,((U32)STSYS_ReadRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_CTRL) & 0xFFFFFFF8));
            break;
        
        default:
            STTBX_Print(("Unidentified SPDIF player identifier\n"));
            break;
    }
}

void StartSPDIFPlayerIP(SPDIFPlayerControl_t * SPDIFPlayerControl_p)
{
    U32 delay, control;
    //U32 Addr1;
    U32 SPDIFPlayerBaseAddr = SPDIFPlayerControl_p->BaseAddress.SPDIFPlayerBaseAddr;

    switch(SPDIFPlayerControl_p->spdifPlayerIdentifier)
    {
        case SPDIF_PLAYER_0:
            //STSYS_WriteRegDev32LE(AUDIO_IOCTL, 0xf);

            STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_SOFTRESET,0x1);
            delay = (ST_GetClocksPerSecond()/30000);
            AUD_TaskDelayMs(delay);
            /*task_delay(200);*/
            
            STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_SOFTRESET,0x0);

            // Set the mode in the control register
            control = SPDIFPlayerControl_p->SPDIFMode;
            control |= (U32)(SPDIFPlayer_ConvertFsMultiplierToClkDivider(SPDIFPlayerControl_p->SPDIFPlayerOutParams.SPDIFPlayerFrequencyMultiplier)<<5);
            if (SPDIFPlayerControl_p->CompressedDataAlignment ==  BE)
            {
                control |= (U32)((1)<<13);
            }
            
            STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_CTRL,control);

            switch (SPDIFPlayerControl_p->SPDIFMode)
            {
                case STAUD_DIGITAL_MODE_NONCOMPRESSED:
                    STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_CL1, CalculateSPDIFChannelStatus0(SPDIFPlayerControl_p));
                    STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_CR1, CalculateSPDIFChannelStatus0(SPDIFPlayerControl_p));

                    // Validity 0
                    STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_CL2_CR2_U_V,(CalculateSPDIFChannelStatus1(SPDIFPlayerControl_p)<<8) | CalculateSPDIFChannelStatus1(SPDIFPlayerControl_p));

                    /*Addr1 = SPDIFPlayerControl_p->BaseAddress.SPDIFFDMAAdditionalDataRegionBaseAddr +  0x28;*/

                    // Write precision mask register to FDMA additional data region
                    //STSYS_WriteRegDev32LE(Addr1, 0xFFFFFF00);
                    STFDMA_SetAddDataRegionParameter(SPDIFPlayerControl_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_DATA_PRECISION_MASK, 0xFFFFFF00);
                    STFDMA_SetAddDataRegionParameter(SPDIFPlayerControl_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAMES_TO_GO, 0);
                    STFDMA_SetAddDataRegionParameter(SPDIFPlayerControl_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAME_COUNT, 0);
                    
                    break;

                case STAUD_DIGITAL_MODE_COMPRESSED:

                    /*Addr1 = SPDIFPlayerControl_p->BaseAddress.SPDIFFDMAAdditionalDataRegionBaseAddr +  0x24;*/

                    // Write compressed mode reset to FDMA additional data region
                    //STSYS_WriteRegDev32LE(Addr1, 0);

                    STFDMA_SetAddDataRegionParameter(SPDIFPlayerControl_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAMES_TO_GO, 0);
                    STFDMA_SetAddDataRegionParameter(SPDIFPlayerControl_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAME_COUNT, 0);

                    STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_CL1, CalculateSPDIFChannelStatus0(SPDIFPlayerControl_p));
                    STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_CR1, CalculateSPDIFChannelStatus0(SPDIFPlayerControl_p));

                    // Validity 1
                    STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_CL2_CR2_U_V, 0x000C0000 | (CalculateSPDIFChannelStatus1(SPDIFPlayerControl_p)<<8) | (CalculateSPDIFChannelStatus1(SPDIFPlayerControl_p)));

                    STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_PA_PB, 0xF8724E1F);
                    STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr  +SPDIF_PC_PD, 0x00013000);

                    // Generate 1 PAUSE burst of 192 SPDIF frames

                    STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_PAUSE_LAT, 0x00010000);
                    STSYS_WriteRegDev32LE(SPDIFPlayerBaseAddr + SPDIF_BURST_LEN, 0x06000600);

                    break;
                    
                default:
                    break;
                    
            }

#ifndef ST_OSLINUX
            STSYS_WriteRegDev32LE(0xB9210200, 0xF);
#else
            //STSYS_WriteAudioReg32(0x200, 0xf);//AUDIO_IOCTL
            STSYS_WriteRegDev32LE(SPDIFPlayerControl_p->BaseAddress.AudioConfigBaseAddr + AUDIO_IOCTL_OFFSET, 0xf);
#endif
            /*Code commented for debugging*/

            //STSYS_WriteSpdifPlayerReg32(SPDIF_SOFTRESET,1);
            //STSYS_WriteSpdifPlayerReg32(SPDIF_SOFTRESET,0);
            //for (i=0 ; i < 16; i++)
            //  STSYS_WriteSpdifPlayerReg32(SPDIF_FIFO_DATA, 0xffffffff);
            break;

        default:
            STTBX_Print(("Unidentified SPDIF player identifier\n"));
            break;
    }
}

ST_ErrorCode_t STAud_SPDIFEnableHDMIOutput(STSPDIFPLAYER_Handle_t PlayerHandle, BOOL Command)
{
    U32 Addr1, Addr2, Addr3;
    SPDIFPlayerControlBlock_t * tempspdifplayerControlBlock = spdifPlayerControlBlock;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempspdifplayerControlBlock != NULL)
    {
        if (tempspdifplayerControlBlock == (SPDIFPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempspdifplayerControlBlock = tempspdifplayerControlBlock->next;
    }
    if(tempspdifplayerControlBlock== NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
    
    /* Get hold of HDMI registers */
    Addr1 = tempspdifplayerControlBlock->spdifPlayerControl.BaseAddress.AudioConfigBaseAddr + 0x204;//AUDIO_HDMI
    Addr2 = tempspdifplayerControlBlock->spdifPlayerControl.BaseAddress.I2S2SpdifConverterBaseAddr + I2S_AUD_SPDIF_PR_CFG;
    Addr3 = tempspdifplayerControlBlock->spdifPlayerControl.BaseAddress.I2S2SpdifConverterBaseAddr + I2S_AUD_SPDIF_PR_SPDIF_CTRL;

    if(Command)
    {
        /* Enable HDMI */

        /* HDMI is supported on at 128FS */
        if(tempspdifplayerControlBlock->spdifPlayerControl.SPDIFPlayerOutParams.SPDIFPlayerFrequencyMultiplier == 128)
        {
            STSYS_WriteRegDev32LE(Addr1, 0x1);
        }
        else
        {
            Error = STAUD_ERROR_INVALID_STATE;
        }
    }
    else
    {
        /* Disable HDMI - Go back to Reset State */
        STSYS_WriteRegDev32LE(Addr1, 0x0);
        STSYS_WriteRegDev32LE(Addr2, 0x0);
        STSYS_WriteRegDev32LE(Addr3, 0x0);
    }
    return Error;
}


