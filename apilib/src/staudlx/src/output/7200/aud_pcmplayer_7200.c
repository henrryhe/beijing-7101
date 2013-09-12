/*
 * Copyright (C) STMicroelectronics Ltd. 2004.
 *
 * All rights reserved.
 *                                       ------->SPDIF PLAYER
 *                                      |
 * DECODER -------         |
 *                                      |
 *                                       ------->PCM PLAYER
 */
 //#define  STTBX_PRINT
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "stos.h"
#include "aud_pcmplayer.h"
#include "audioreg.h"

#define HDMI_PCMFREQMULT      128

extern U32 PlayerUserCount; /* Number of player modules which are initialized */
extern PCMPlayerControlBlock_t * pcmPlayerControlBlock;

ST_ErrorCode_t PCMPlayer_SetFrequencySynthesizer (PCMPlayerControl_t * PCMPlayerControl_p, U32 samplingFrequency)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 sdiv = 0, md = 0, pe = 0;
    U32 AudioConfigBaseAddr = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr;

    switch (PCMPlayerControl_p->samplingFrequency)
    {
        case 48000:
            md = 0x13;
            pe = 0x3c00;
            sdiv = 0x4;
            break;
            
        default :
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }
    
    // This sets the audio freq synthesizers A/B
    STSYS_WriteRegDev32LE(AudioConfigBaseAddr, 0x00017C01);//AUDIO_FSYNA_CFG
    STSYS_WriteRegDev32LE(AudioConfigBaseAddr, 0x00017C00);//AUDIO_FSYNA_CFG
    STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x100, 0x00017C01);//AUDIO_FSYNB_CFG
    STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x100, 0x00017C00);//AUDIO_FSYNB_CFG

    switch (PCMPlayerControl_p->pcmPlayerIdentifier)
    {
        case PCM_PLAYER_0:
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x1C , 0x0);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x10 , md);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x14 , pe);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x18 , sdiv);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x1C , 0x1);
            break;       
            
        case PCM_PLAYER_1:
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x2C , 0x0);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x20 , md);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x24 , pe);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x28 , sdiv);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x2C , 0x1);
            break;
            
        case PCM_PLAYER_2:
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x3C , 0x0);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x30 , md);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x34 , pe);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x38 , sdiv);
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x3C , 0x1);
            break;
            
        case PCM_PLAYER_3:
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x4C , 0x0);//AUDIO_FSYNA3_PROG_EN
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x40 , md);//AUDIO_FSYNA3_MD
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x44 , pe);//AUDIO_FSYNA3_PE
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x48 , sdiv);//AUDIO_FSYNA3_SDIV
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x4C , 0x1);//AUDIO_FSYNA3_PROG_EN
            break;
            
        case HDMI_PCM_PLAYER_0:
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x13C , 0x0);//AUDIO_FSYNB2_PROG_EN
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x130 , md);//AUDIO_FSYNB2_MD
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x134 , pe);//AUDIO_FSYNB2_PE
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x138 , sdiv);//AUDIO_FSYNB2_SDIV
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr + 0x13C , 0x1);//AUDIO_FSYNB2_PROG_EN
            break;
            
        case MAX_PCM_PLAYER_INSTANCE:
        default:
            //STTBX_Print(("Unidentified PCM player identifier\n"));
            Error = ST_ERROR_UNKNOWN_DEVICE;
            break;
    }
    return Error;
}

void ResetPCMPLayerIP(PCMPlayerControl_t * PCMPlayerControl_p)
{
    U32    Addr1, Addr2, Addr3, Addr6;
    Addr1 = Addr2 = Addr3 = Addr6 = 0;

    switch(PCMPlayerControl_p->pcmPlayerIdentifier)
    {
        case PCM_PLAYER_0:
            Addr1 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x400;//AUDIO_DAC_PCMP0
            Addr2 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x500;//AUDIO_DAC_PCMP1
            Addr3 = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr + 0x1C;//PCMPLAYER0_CONTROL_REG
            Addr6 = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr + 0x00;//PCMPLAYER0_SOFT_RESET_REG

            // Reset PCMP0 and internal DAC 0/1
            STSYS_WriteRegDev32LE(Addr6, 0x1);
            STSYS_WriteRegDev32LE(Addr1, 0x68);
            STSYS_WriteRegDev32LE(Addr2, 0x68);

            STSYS_WriteRegDev32LE(Addr6, 0x0);
            STSYS_WriteRegDev32LE(Addr1, 0x69);
            STSYS_WriteRegDev32LE(Addr2, 0x69);

            // Set PCM mode to ON
            STSYS_WriteRegDev32LE(Addr3, (STSYS_ReadRegDev32LE(Addr3) | ((U32)0x2)));
            break;

    case PCM_PLAYER_1:
        Addr1 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x400;//AUDIO_DAC_PCMP0
        Addr2 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x500;//AUDIO_DAC_PCMP1
        Addr3 = PCMPlayerControl_p->BaseAddress.PCMPlayer1BaseAddr + 0x1C;//PCMPLAYER1_CONTROL_REG
        Addr6 = PCMPlayerControl_p->BaseAddress.PCMPlayer1BaseAddr + 0x00;//PCMPLAYER1_SOFT_RESET_REG

        // Reset PCMP1 and internal DAC 0/1
        STSYS_WriteRegDev32LE(Addr6, 0x1);
        STSYS_WriteRegDev32LE(Addr2, 0x68);
        STSYS_WriteRegDev32LE(Addr1, 0x68);

        STSYS_WriteRegDev32LE(Addr6, 0x0);
        STSYS_WriteRegDev32LE(Addr2, 0x69);
        STSYS_WriteRegDev32LE(Addr1, 0x69);

        // Set PCM mode to ON
        STSYS_WriteRegDev32LE(Addr3, (STSYS_ReadRegDev32LE(Addr3) | ((U32)0x2)));
        break;

    case PCM_PLAYER_2:
        // Nothing to be done for PCM player Ip
        break;

    case PCM_PLAYER_3:  // PCM player3 reset
        // Nothing to be done for PCM player Ip
        break;

    case HDMI_PCM_PLAYER_0: // HDMI player reset
        // Nothing to be done for HDMI_PCM_PLAYER_0 Ip
        break;        
        
    case MAX_PCM_PLAYER_INSTANCE:
    default:
        STTBX_Print(("Unidentified PCM player identifier\n"));
        break;
    }
}

void StopPCMPlayerIP(PCMPlayerControl_t * PCMPlayerControl_p)
{
    U32 Addr2 = 0;
    U32 Addr3 = 0;

    switch(PCMPlayerControl_p->pcmPlayerIdentifier)
    {
        case PCM_PLAYER_0:
            Addr2 = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr + 0x1C;//PCMPLAYER0_CONTROL_REG
            Addr3 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x400;//AUDIO_DAC_PCMP0

            STSYS_WriteRegDev32LE(Addr2, ((U32)STSYS_ReadRegDev32LE(Addr2) & 0xFFFFFFFC));
            //STSYS_WriteRegDev32LE(Addr3, ((U32)STSYS_ReadRegDev32LE(Addr3) & 0xFFFFFFFC));
            break;

        case PCM_PLAYER_1:
            Addr2 = PCMPlayerControl_p->BaseAddress.PCMPlayer1BaseAddr + 0x1C;//PCMPLAYER1_CONTROL_REG
            Addr3 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x500;//AUDIO_DAC_PCMP1

            STSYS_WriteRegDev32LE(Addr2, ((U32)STSYS_ReadRegDev32LE(Addr2) & 0xFFFFFFFC));
            //STSYS_WriteRegDev32LE(Addr3, ((U32)STSYS_ReadRegDev32LE(Addr3) & 0xFFFFFFFC));
            break;

        case PCM_PLAYER_2:
            Addr2 = PCMPlayerControl_p->BaseAddress.PCMPlayer2BaseAddr + 0x1C;//PCMPLAYER2_CONTROL_REG
            STSYS_WriteRegDev32LE(Addr2, ((U32)STSYS_ReadRegDev32LE(Addr2) & 0xFFFFFFFC));
            break;

        case PCM_PLAYER_3:
            Addr2 = PCMPlayerControl_p->BaseAddress.PCMPlayer3BaseAddr + 0x1C;//PCMPLAYER3_CONTROL_REG
            STSYS_WriteRegDev32LE(Addr2, ((U32)STSYS_ReadRegDev32LE(Addr2) & 0xFFFFFFFC));
            break;

        case HDMI_PCM_PLAYER_0:
            Addr2 = PCMPlayerControl_p->BaseAddress.HDMIPCMPlayerBaseAddr + 0x1C;//HDMI_PCMPLAYER_CONTROL_REG
            STSYS_WriteRegDev32LE(Addr2, ((U32)STSYS_ReadRegDev32LE(Addr2) & 0xFFFFFFFC));
            break;

        case MAX_PCM_PLAYER_INSTANCE:
        default:
            STTBX_Print(("Unidentified PCM player identifier\n"));
            break;
    }
}

void StartPCMPlayerIP(PCMPlayerControl_t * PCMPlayerControl_p)
{
    U32 Addr1, Addr2, Addr3, Addr4, Addr5, Addr6;
    U32 value = 0, FrequencyMultiplier = 0;
    STAUD_PCMDataConf_t * pcmPlayerDataConfig_p = &(PCMPlayerControl_p->pcmPlayerDataConfig);

    Addr1 = Addr2 = Addr3 = Addr4 = Addr5 = 0;

    if(PCMPlayerControl_p->NumChannels==2)
    {
        value = 0x00002100;
    }
    else
    {
        value = 0x0000a500;/*bit 11 to 15 signify number of available free cell*/
    }

    switch(PCMPlayerControl_p->pcmPlayerIdentifier)
    {
        case PCM_PLAYER_0:
            Addr1 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x200;//AUDIO_IOCTL
            Addr2 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x400;//AUDIO_DAC_PCMP0;//internal DAC 0
            Addr3 = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr + 0x1C;//PCMPLAYER0_CONTROL_REG
            Addr4 = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr + 0x24;//PCMPLAYER0_FORMAT_REG
            Addr5 = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr + 0x14;//PCMPLAYER0_ITRP_ENABLE_SET_REG
            Addr6 = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr + 0x00;//PCMPLAYER0_SOFT_RESET_REG

            STSYS_WriteRegDev32LE(Addr1, 0x1f);
            STSYS_WriteRegDev32LE(Addr3, ((U32)PCMPlayer_ConvertFsMultiplierToClkDivider(pcmPlayerDataConfig_p->PcmPlayerFrequencyMultiplier)<<4)|
                                                                ((U32)pcmPlayerDataConfig_p->MemoryStorageFormat16 << 2) |
                                                                ((U32)0x0));
            STSYS_WriteRegDev32LE(Addr4, ((U32)pcmPlayerDataConfig_p->MSBFirst << 7) |
                                                                ((U32)pcmPlayerDataConfig_p->Alignment << 6) |
                                                                ((U32)pcmPlayerDataConfig_p->Format << 5) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertBitClock << 4) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertWordClock << 3) |
                                                                ((U32)pcmPlayerDataConfig_p->Precision << 1) |
                                                                (value));

            //STSYS_WriteRegDev32LE(Addr5, 0x3);
            break;

        case PCM_PLAYER_1:
            Addr1 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x200;//AUDIO_IOCTL
            Addr2 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x400;//AUDIO_DAC_PCMP1;//internal DAC 1 
            Addr3 = PCMPlayerControl_p->BaseAddress.PCMPlayer1BaseAddr + 0x1C;//PCMPLAYER1_CONTROL_REG
            Addr4 = PCMPlayerControl_p->BaseAddress.PCMPlayer1BaseAddr + 0x24;//PCMPLAYER1_FORMAT_REG
            Addr5 = PCMPlayerControl_p->BaseAddress.PCMPlayer1BaseAddr + 0x14;//PCMPLAYER1_ITRP_ENABLE_SET_REG
            Addr6 = PCMPlayerControl_p->BaseAddress.PCMPlayer1BaseAddr + 0x00;//PCMPLAYER1_SOFT_RESET_REG

            STSYS_WriteRegDev32LE(Addr1, 0x1f);
            STSYS_WriteRegDev32LE(Addr3, ((U32)PCMPlayer_ConvertFsMultiplierToClkDivider(pcmPlayerDataConfig_p->PcmPlayerFrequencyMultiplier)<<4)|
                                                                ((U32)pcmPlayerDataConfig_p->MemoryStorageFormat16 << 2) |
                                                                ((U32)0x0));
            STSYS_WriteRegDev32LE(Addr4, ((U32)pcmPlayerDataConfig_p->MSBFirst << 7) |
                                                                ((U32)pcmPlayerDataConfig_p->Alignment << 6) |
                                                                ((U32)pcmPlayerDataConfig_p->Format << 5) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertBitClock << 4) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertWordClock << 3) |
                                                                ((U32)pcmPlayerDataConfig_p->Precision << 1) |
                                                                (value));

            //STSYS_WriteRegDev32LE(Addr5, 0x3);
            break;

        case PCM_PLAYER_2:
            Addr1 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x200;//AUDIO_IOCTL     
            Addr3 = PCMPlayerControl_p->BaseAddress.PCMPlayer2BaseAddr + 0x1C;//PCMPLAYER2_CONTROL_REG
            Addr4 = PCMPlayerControl_p->BaseAddress.PCMPlayer2BaseAddr + 0x24;//PCMPLAYER2_FORMAT_REG
            Addr5 = PCMPlayerControl_p->BaseAddress.PCMPlayer2BaseAddr + 0x14;//PCMPLAYER2_ITRP_ENABLE_SET_REG
            Addr6 = PCMPlayerControl_p->BaseAddress.PCMPlayer2BaseAddr + 0x00;//PCMPLAYER2_SOFT_RESET_REG         

            STSYS_WriteRegDev32LE(Addr1, 0x1f);
            STSYS_WriteRegDev32LE(Addr3, ((U32)PCMPlayer_ConvertFsMultiplierToClkDivider(pcmPlayerDataConfig_p->PcmPlayerFrequencyMultiplier)<<4)|
                                                                ((U32)pcmPlayerDataConfig_p->MemoryStorageFormat16 << 2) |
                                                                ((U32)0x0));
            STSYS_WriteRegDev32LE(Addr4, ((U32)pcmPlayerDataConfig_p->MSBFirst << 7) |
                                                                ((U32)pcmPlayerDataConfig_p->Alignment << 6) |
                                                                ((U32)pcmPlayerDataConfig_p->Format << 5) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertBitClock << 4) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertWordClock << 3) |
                                                                ((U32)pcmPlayerDataConfig_p->Precision << 1) |
                                                                (value));
            //STSYS_WriteRegDev32LE(Addr5, 0x3);
            STSYS_WriteRegDev32LE(Addr6, 0x1);
            STSYS_WriteRegDev32LE(Addr6, 0x0);

            /*Start the player Set PCM mode to ON*/
            STSYS_WriteRegDev32LE(Addr3, (STSYS_ReadRegDev32LE(Addr3) | ((U32)0x2)));
            break;

        case PCM_PLAYER_3:
            Addr1 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x200;//AUDIO_IOCTL     
            Addr3 = PCMPlayerControl_p->BaseAddress.PCMPlayer3BaseAddr + 0x1C;//PCMPLAYER3_CONTROL_REG
            Addr4 = PCMPlayerControl_p->BaseAddress.PCMPlayer3BaseAddr + 0x24;//PCMPLAYER3_FORMAT_REG
            Addr5 = PCMPlayerControl_p->BaseAddress.PCMPlayer3BaseAddr + 0x14;//PCMPLAYER3_ITRP_ENABLE_SET_REG
            Addr6 = PCMPlayerControl_p->BaseAddress.PCMPlayer3BaseAddr + 0x00;//PCMPLAYER3_SOFT_RESET_REG


            STSYS_WriteRegDev32LE(Addr1, 0x1f);   
            FrequencyMultiplier = pcmPlayerDataConfig_p->PcmPlayerFrequencyMultiplier;
            FrequencyMultiplier = 1;//No clock division (Hack to handle ch3/4 bug(7200 cut1.0)): PCMCLK equal to SCLK (Required for channel 3/4 modulator)
            STSYS_WriteRegDev32LE(Addr3, ((U32)PCMPlayer_ConvertFsMultiplierToClkDivider(FrequencyMultiplier)<<4)|
                                                                ((U32)pcmPlayerDataConfig_p->MemoryStorageFormat16 << 2) |
                                                                ((U32)0x0));

            STSYS_WriteRegDev32LE(Addr4, ((U32)pcmPlayerDataConfig_p->MSBFirst << 7) |
                                                                ((U32)pcmPlayerDataConfig_p->Alignment << 6) |
                                                                ((U32)pcmPlayerDataConfig_p->Format << 5) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertBitClock << 4) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertWordClock << 3) |
                                                                ((U32)pcmPlayerDataConfig_p->Precision << 1) |
                                                                (value));
            //STSYS_WriteRegDev32LE(Addr5, 0x3);
            STSYS_WriteRegDev32LE(Addr6, 0x1);
            STSYS_WriteRegDev32LE(Addr6, 0x0);

            /*Start the player Set PCM mode to ON*/
            STSYS_WriteRegDev32LE(Addr3, (STSYS_ReadRegDev32LE(Addr3) | ((U32)0x2)));
            break;
            
        case HDMI_PCM_PLAYER_0:

            Addr1 = PCMPlayerControl_p->BaseAddress.AudioConfigBaseAddr + 0x200;//AUDIO_IOCTL     
            Addr3 = PCMPlayerControl_p->BaseAddress.HDMIPCMPlayerBaseAddr + 0x1C;//HDMI_PCMPLAYER_CONTROL_REG
            Addr4 = PCMPlayerControl_p->BaseAddress.HDMIPCMPlayerBaseAddr + 0x24;//HDMI_PCMPLAYER_FORMAT_REG
            Addr5 = PCMPlayerControl_p->BaseAddress.HDMIPCMPlayerBaseAddr + 0x14;//HDMI_PCMPLAYER_ITRP_ENABLE_SET_REG
            Addr6 = PCMPlayerControl_p->BaseAddress.HDMIPCMPlayerBaseAddr + 0x00;//HDMI_PCMPLAYER_SOFT_RESET_REG

            STSYS_WriteRegDev32LE(Addr1, 0x1f);
            STSYS_WriteRegDev32LE(Addr3, ((U32)PCMPlayer_ConvertFsMultiplierToClkDivider(pcmPlayerDataConfig_p->PcmPlayerFrequencyMultiplier)<<4)|
                                                                ((U32)pcmPlayerDataConfig_p->MemoryStorageFormat16 << 2) |
                                                                ((U32)0x0));
            STSYS_WriteRegDev32LE(Addr4, ((U32)pcmPlayerDataConfig_p->MSBFirst << 7) |
                                                                ((U32)pcmPlayerDataConfig_p->Alignment << 6) |
                                                                ((U32)pcmPlayerDataConfig_p->Format << 5) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertBitClock << 4) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertWordClock << 3) |
                                                                ((U32)pcmPlayerDataConfig_p->Precision << 1) |
                                                                (value));

            //STSYS_WriteRegDev32LE(Addr5, 0x3);
            STSYS_WriteRegDev32LE(Addr6, 0x1);
            STSYS_WriteRegDev32LE(Addr6, 0x0);

            /*Start the player Set PCM mode to ON*/
            STSYS_WriteRegDev32LE(Addr3, (STSYS_ReadRegDev32LE(Addr3) | ((U32)0x2)));
            break;

        case MAX_PCM_PLAYER_INSTANCE:
        default:
            STTBX_Print(("Unidentified PCM player identifier\n"));
            break;
    }
}

/* Set Scenario for 7200 Chipset */
ST_ErrorCode_t STAud_OPSetScenario(STAUD_Scenario_t Scenario)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    U32 SystemConfigBaseAddr = 0;
    U32 SysCon20 = 0, SysCon41 = 0;

    /* System Config memory */
    SystemConfigBaseAddr = (U32)STOS_MapRegisters(ST7200_CFG_BASE_ADDRESS, 4*1024, "SYSCONFIG");
    if(!SystemConfigBaseAddr)
    {
        STTBX_Print(("SystemConfigBaseAddr STOS_MapRegisters failed.\n"));
        Error = STAUD_ERROR_IO_REMAP_FAILED;
    }
    SysCon20 = SystemConfigBaseAddr + 0x150;
    SysCon41 = SystemConfigBaseAddr + 0x1a4;    
    switch(Scenario)
    {
        case STAUD_SET_SENARIO_0:
            STSYS_WriteRegDev32LE(SysCon20, STSYS_ReadRegDev32LE(SysCon20) & 0xfffffffe);
            STSYS_WriteRegDev32LE(SysCon41, STSYS_ReadRegDev32LE(SysCon41) | 0x00220000);
            STSYS_WriteRegDev32LE(SysCon41, STSYS_ReadRegDev32LE(SysCon41) & 0xffeeffff);
            break;

        case STAUD_SET_SENARIO_1:
            STSYS_WriteRegDev32LE(SysCon20, STSYS_ReadRegDev32LE(SysCon20) | 0x1);
            STSYS_WriteRegDev32LE(SysCon41, STSYS_ReadRegDev32LE(SysCon41) | 0x00110000);
            STSYS_WriteRegDev32LE(SysCon41, STSYS_ReadRegDev32LE(SysCon41) & 0xffddffff);
            STSYS_WriteRegDev32LE(SysCon41, STSYS_ReadRegDev32LE(SysCon41) & 0xfffeffff);
            break;
            
        case STAUD_SET_SENARIO_2:
            break;
            
        default:
            break;
    }
    STOS_UnmapRegisters((void*)SystemConfigBaseAddr, (4 * 1024));
    return Error;
}

ST_ErrorCode_t STAud_PCMEnableHDMIOutput(STPCMPLAYER_Handle_t PlayerHandle,BOOL Command)
{
    U32 Addr1, Addr2, Addr3, Addr4, Addr5, Addr6, Addr7, Addr8, Addr9, Addr10, Addr11;
    PCMPlayerControlBlock_t * tempPCMplayerControlBlock = pcmPlayerControlBlock;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempPCMplayerControlBlock != NULL)
    {
        if (tempPCMplayerControlBlock == (PCMPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempPCMplayerControlBlock = tempPCMplayerControlBlock->next;
    }
    
    if(tempPCMplayerControlBlock== NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
    if(tempPCMplayerControlBlock->pcmPlayerControl.pcmPlayerIdentifier != HDMI_PCM_PLAYER_0)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Get hold of HDMI registers */
    Addr1 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.AudioConfigBaseAddr +  0x204;
    Addr2 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.I2S2SpdifConverterBaseAddr   + I2S_AUD_SPDIF_PR_CFG;
    Addr3 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.I2S2SpdifConverterBaseAddr   + I2S_AUD_SPDIF_PR_SPDIF_CTRL;
    Addr4 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.I2S2SpdifConverterBaseAddr1 + I2S_AUD_SPDIF_PR_CFG ;
    Addr5 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.I2S2SpdifConverterBaseAddr1 + I2S_AUD_SPDIF_PR_SPDIF_CTRL ;
    Addr6 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.I2S2SpdifConverterBaseAddr2 + I2S_AUD_SPDIF_PR_CFG ;
    Addr7 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.I2S2SpdifConverterBaseAddr2 + I2S_AUD_SPDIF_PR_SPDIF_CTRL ;
    Addr8 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.I2S2SpdifConverterBaseAddr3 + I2S_AUD_SPDIF_PR_CFG ;
    Addr9 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.I2S2SpdifConverterBaseAddr3 + I2S_AUD_SPDIF_PR_SPDIF_CTRL ;
    Addr10 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.HDTVOutAuxGlueBaseAddr;
    Addr11 = tempPCMplayerControlBlock->pcmPlayerControl.BaseAddress.HDMISyncConfigBaseAddr;

    /*For 7200 cut 1.0 it is essential to set bit31 of HD_TVOUT_AUX_GLUE_BASE_ADDRESS in order to access i2stospdif register*/

    STSYS_WriteRegDev32LE(Addr10 , STSYS_ReadRegDev32LE(Addr10) | (0x1 << 31));

    if(Command)
    {
        /* HDMI is supported on at 128FS */
        if(tempPCMplayerControlBlock->pcmPlayerControl.pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier == HDMI_PCMFREQMULT)
        {
        STSYS_WriteRegDev32LE(Addr1, 0x0);

        /*Enable I2S_spdif_0*/
        STSYS_WriteRegDev32LE(Addr2, 0x25);
        STSYS_WriteRegDev32LE(Addr3, 0x4033);

        /*Enable I2S_spdif_1*/
        STSYS_WriteRegDev32LE(Addr4, 0x25);
        STSYS_WriteRegDev32LE(Addr5, 0x4033);

        /*Enable I2S_spdif_2*/
        STSYS_WriteRegDev32LE(Addr6, 0x25);
        STSYS_WriteRegDev32LE(Addr7, 0x4033);

        /*Enable I2S_spdif_3*/
        STSYS_WriteRegDev32LE(Addr8, 0x25);
        STSYS_WriteRegDev32LE(Addr9, 0x4033);

        /*Mark HDMI player state as enabled*/
        tempPCMplayerControlBlock->pcmPlayerControl.HDMI_PcmPlayerEnable = TRUE;
        }
        else
        {
        /*Mark HDMI player state as disabled*/
        tempPCMplayerControlBlock->pcmPlayerControl.HDMI_PcmPlayerEnable = FALSE;
        Error = STAUD_ERROR_INVALID_STATE;
        }
    }
    else
    {
        /* Disable HDMI - Go back to Reset State */
        STSYS_WriteRegDev32LE(Addr1, 0x0);
        STSYS_WriteRegDev32LE(Addr2, 0x0);
        STSYS_WriteRegDev32LE(Addr3, 0x0);
        STSYS_WriteRegDev32LE(Addr4, 0x0);
        STSYS_WriteRegDev32LE(Addr5, 0x0);
        STSYS_WriteRegDev32LE(Addr6, 0x0);
        STSYS_WriteRegDev32LE(Addr7, 0x0);
        STSYS_WriteRegDev32LE(Addr8, 0x0);
        STSYS_WriteRegDev32LE(Addr9, 0x0);

        /*Mark HDMI player state as disabled*/
        tempPCMplayerControlBlock->pcmPlayerControl.HDMI_PcmPlayerEnable = FALSE;
    }

    /*Now reset back bit31 of HD_TVOUT_AUX_GLUE_BASE_ADDRESS register.*/
    STSYS_WriteRegDev32LE(Addr10 , STSYS_ReadRegDev32LE(Addr10) & (0xFFFFFFFF>>1));
    /*Set Bit 0 and Bit 2 of HDMI gpout register*/
    STSYS_WriteRegDev32LE(Addr11,  STSYS_ReadRegDev32LE(Addr11) | (0x5));
    
    return Error;
}

ST_ErrorCode_t  STAud_PlayerMemRemap(AudioBaseAddress_t *BaseAddress)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 AudioPeriBaseAdd=0, AudioPeriTotalSize=0;

    /* Update the number of users using remapped buffers */
    PlayerUserCount++;

    if(BaseAddress->IsInit == FALSE)
    {
        /* Map IO Space to kernel */
        AudioPeriBaseAdd = PCMREADER_BASE;
        AudioPeriTotalSize = 6 * ( 4 * 1024);/* 6 Peripherals , 4KB mem for each */

        memset(BaseAddress,0,sizeof(AudioBaseAddress_t));

        /* PCM Reader */
        BaseAddress->PCMReaderBaseAddr = (U32)STOS_MapRegisters((void*)AudioPeriBaseAdd, AudioPeriTotalSize,"AUD_PLAYERS");
        if(!BaseAddress->PCMReaderBaseAddr)
        {
            STTBX_Print(("Audio STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }
        
        BaseAddress->PCMPlayer0BaseAddr = BaseAddress->PCMReaderBaseAddr + PCMREADER_REGION_SIZE;
        BaseAddress->PCMPlayer1BaseAddr = BaseAddress->PCMPlayer0BaseAddr + PCMPLAYER0_REGION_SIZE;
        BaseAddress->PCMPlayer2BaseAddr = BaseAddress->PCMPlayer1BaseAddr + PCMPLAYER1_REGION_SIZE;
        BaseAddress->PCMPlayer3BaseAddr = BaseAddress->PCMPlayer2BaseAddr + PCMPLAYER2_REGION_SIZE;
        BaseAddress->SPDIFPlayerBaseAddr = BaseAddress->PCMPlayer3BaseAddr + PCMPLAYER3_REGION_SIZE; 

        /* PCM HDMI Player */
        BaseAddress->HDMIPCMPlayerBaseAddr = (U32)STOS_MapRegisters((void*)HDMI_PCMPLAYER_BASE, HDMI_PCMPLAYER_REGION_SIZE,"AUD_HDMIPCM");
        if(!BaseAddress->HDMIPCMPlayerBaseAddr)
        {
            STTBX_Print(("BaseAddress.HDMIPCMPlayerBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }

        /* Audio config */
        BaseAddress->AudioConfigBaseAddr = (U32)STOS_MapRegisters((void*)AUDIO_CONFIG_BASE, AUDIO_CONFIG_REGION_SIZE,"AUD_CONFIG");
        if(!BaseAddress->AudioConfigBaseAddr)
        {
            STTBX_Print(("BaseAddress.AudioConfigBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }

        /* SPDIF HDMI Player */
        BaseAddress->HDMISPDIFPlayerBaseAddr = (U32)STOS_MapRegisters((void*)HDMI_SPDIFPLAYER_BASE, HDMI_SPDIF_REGION_SIZE,"AUD_HDMISPDIF");
        if(!BaseAddress->HDMISPDIFPlayerBaseAddr)
        {
            STTBX_Print(("BaseAddress.HDMISPDIFPlayerBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }

        /* I2S to SPDIF coverter, used for HDMI output */
        BaseAddress->I2S2SpdifConverterBaseAddr = (U32)STOS_MapRegisters((void*)I2S_SPDIF_CONVERTER_BASE_ADDRESS_0, I2S_SPDIF_CONVERTER_REGION_SIZE,"AUD_I2S_REG1");
        BaseAddress->I2S2SpdifConverterBaseAddr1 = (U32)STOS_MapRegisters((void*)I2S_SPDIF_CONVERTER_BASE_ADDRESS_1, I2S_SPDIF_CONVERTER_REGION_SIZE,"AUD_I2S_REG2");
        BaseAddress->I2S2SpdifConverterBaseAddr2 = (U32)STOS_MapRegisters((void*)I2S_SPDIF_CONVERTER_BASE_ADDRESS_2, I2S_SPDIF_CONVERTER_REGION_SIZE,"AUD_I2S_REG3");
        BaseAddress->I2S2SpdifConverterBaseAddr3 = (U32)STOS_MapRegisters((void*)I2S_SPDIF_CONVERTER_BASE_ADDRESS_3, I2S_SPDIF_CONVERTER_REGION_SIZE,"AUD_I2S_REG4");
        if((!BaseAddress->I2S2SpdifConverterBaseAddr) || (!BaseAddress->I2S2SpdifConverterBaseAddr1) ||
        (!BaseAddress->I2S2SpdifConverterBaseAddr2) || (!BaseAddress->I2S2SpdifConverterBaseAddr3))
        {
            STTBX_Print(("BaseAddress.I2S2SpdifConverterBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }

        /* SPDIF additional data region used for SPDIF PCM mode*/
        BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr = 0;
        BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr = (U32)STOS_MapRegisters((void*)FDMA_BASE_ADDRESS + SPDIF_ADDITIONAL_DATA_REGION, 64,"AUD_FDMA_ADR");
        if(!BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr)
        {
            STTBX_Print(("BaseAddress.SPDIFFDMAAdditionalDataRegionBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }

        /* HD_TVOUT_AUX_GLUE_BASE_ADDRESS */
        BaseAddress->HDTVOutAuxGlueBaseAddr = 0;
        BaseAddress->HDTVOutAuxGlueBaseAddr = (U32)STOS_MapRegisters(HD_TVOUT_AUX_GLUE_BASE_ADDRESS,4,"HD_TVOUT_ADR");
        if(!BaseAddress->HDTVOutAuxGlueBaseAddr)
        {
            STTBX_Print(("BaseAddress.HDTVOutAuxGlueBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }

        /* HDMI_SYNC_CONFIG_BASE_ADDRESS*/
        BaseAddress->HDMISyncConfigBaseAddr = 0;
        BaseAddress->HDMISyncConfigBaseAddr = (U32)STOS_MapRegisters(HDMI_SYNC_CONFIG_BASE_ADDRESS,4,"HDMISYNC_CONFIG_ADR");
        if(!BaseAddress->HDMISyncConfigBaseAddr)
        {
            STTBX_Print(("BaseAddress.HDMISyncConfigBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }

        /* System Config memory */
        BaseAddress->SystemConfigBaseAddr = 0;
        BaseAddress->SystemConfigBaseAddr = (U32)STOS_MapRegisters(ST7200_CFG_BASE_ADDRESS,4*1024,"SYSCONFIG_ADR");
        if(!BaseAddress->SystemConfigBaseAddr)
        {
            STTBX_Print(("BaseAddress.SystemConfigBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }

        /* Set the Init bool */
        BaseAddress->IsInit = TRUE;

        if(Error == STAUD_ERROR_IO_REMAP_FAILED)
        {
            /* Free the resourses */
            STAud_PlayerMemUnmap(BaseAddress);

            /* If error , reset init */
            BaseAddress->IsInit = FALSE;
        }
        else
        {
            STTBX_Print(("STAud_PlayerMemRemap() Success. \n"));
        }
    }
    return Error;
}

ST_ErrorCode_t  STAud_PlayerMemUnmap(AudioBaseAddress_t *BaseAddress)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* Reduce the number of users using remapped buffers */
    PlayerUserCount--;

    if(!PlayerUserCount)
    {
        /* When all the players are terminated, release the buffers */
        if(BaseAddress->IsInit == TRUE)
        {
            STOS_UnmapRegisters((void*)BaseAddress->PCMReaderBaseAddr, (6 * (4*1024)));
            STOS_UnmapRegisters((void*)BaseAddress->HDMIPCMPlayerBaseAddr, HDMI_PCMPLAYER_REGION_SIZE);
            STOS_UnmapRegisters((void*)BaseAddress->AudioConfigBaseAddr, AUDIO_CONFIG_REGION_SIZE);
            STOS_UnmapRegisters((void*)BaseAddress->HDMISPDIFPlayerBaseAddr, HDMI_SPDIF_REGION_SIZE);

            STOS_UnmapRegisters((void*)BaseAddress->I2S2SpdifConverterBaseAddr,   I2S_SPDIF_CONVERTER_REGION_SIZE);
            STOS_UnmapRegisters((void*)BaseAddress->I2S2SpdifConverterBaseAddr1, I2S_SPDIF_CONVERTER_REGION_SIZE);
            STOS_UnmapRegisters((void*)BaseAddress->I2S2SpdifConverterBaseAddr2, I2S_SPDIF_CONVERTER_REGION_SIZE);
            STOS_UnmapRegisters((void*)BaseAddress->I2S2SpdifConverterBaseAddr3, I2S_SPDIF_CONVERTER_REGION_SIZE);

            STOS_UnmapRegisters((void*)BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr, 64);
            STOS_UnmapRegisters((void*)BaseAddress->HDTVOutAuxGlueBaseAddr, 4);
            STOS_UnmapRegisters((void*)BaseAddress->HDMISyncConfigBaseAddr , 4);
            STOS_UnmapRegisters((void*)BaseAddress->SystemConfigBaseAddr, (4 * 1024));

            BaseAddress->IsInit = FALSE;
        }
    }
    return Error;
}



