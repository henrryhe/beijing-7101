/*
 * Copyright (C) STMicroelectronics Ltd. 2004.
 *
 * All rights reserved.
 *                      ------->SPDIF PLAYER
 *                      |
 * DECODER -------|
 *                      |
 *                      ------->PCM PLAYER
 */

//#define  STTBX_PRINT
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "stos.h"
#include "aud_pcmplayer.h"
#include "audioreg.h"

#ifdef STAUD_REMOVE_CLKRV_SUPPORT

#define AUDIO_CONF_FSYN_CFG                  0x00

#define AUDIO_CONF_FSYN0_MD                  0x10      // For PCM player 0
#define AUDIO_CONF_FSYN0_PE                   0x14
#define AUDIO_CONF_FSYN0_SDIV               0x18
#define AUDIO_CONF_FSYN0_PROG_EN        0x1C

#define AUDIO_CONF_FSYN1_MD                   0x20      // For PCM player 1
#define AUDIO_CONF_FSYN1_PE                    0x24
#define AUDIO_CONF_FSYN1_SDIV                0x28
#define AUDIO_CONF_FSYN1_PROG_EN         0x2C

#define AUDIO_CONF_FSYN2_MD                    0x30      // For SPDIF player 0
#define AUDIO_CONF_FSYN2_PE                     0x34
#define AUDIO_CONF_FSYN2_SDIV                 0x38
#define AUDIO_CONF_FSYN2_PROG_EN          0x3C

#endif

extern U32 PlayerUserCount; /* Number of player modules which are initialized */
extern PCMPlayerControlBlock_t * pcmPlayerControlBlock;

ST_ErrorCode_t PCMPlayer_SetFrequencySynthesizer (PCMPlayerControl_t * Control_p, U32 samplingFrequency)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 sdiv = 0, md = 0, pe = 0;
    U32 AudioConfigBaseAddr = Control_p->BaseAddress.AudioConfigBaseAddr;

    switch (Control_p->samplingFrequency)
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
    
    // This sets the audio freq synthesizer
    /*STSYS_WriteRegDev32LE(AUDIO_FSYN_CFG, 0x7fef);
     STSYS_WriteRegDev32LE(AUDIO_FSYN_CFG, 0x7fee);*/

    switch (Control_p->pcmPlayerIdentifier)
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
            
        default:
            //STTBX_Print(("Unidentified PCM player identifier\n"));
            Error = ST_ERROR_UNKNOWN_DEVICE;
            break;
            
    }
    return Error;
}

void ResetPCMPLayerIP(PCMPlayerControl_t * Control_p)
{
    U32 Addr2, Addr3, Addr6;
    
    Addr2 = Addr3 = Addr6 = 0;

    switch(Control_p->pcmPlayerIdentifier)
    {
        case PCM_PLAYER_0:
            // Nothing to be done for PCM player Ip
            break;

        case PCM_PLAYER_1:
            // PCM player1 and internal DAC needs an addition reset to be in sync

            Addr2 = Control_p->BaseAddress.AudioConfigBaseAddr + 0x100;
            Addr3 = Control_p->BaseAddress.PCMPlayer1BaseAddr + 0x1C;
            Addr6 = Control_p->BaseAddress.PCMPlayer1BaseAddr + 0x00;

            // Reset PCMP1 and internal DAC once more
            STSYS_WriteRegDev32LE(Addr6, 0x1);
            STSYS_WriteRegDev32LE(Addr2, 0x68);
            STSYS_WriteRegDev32LE(Addr6, 0x0);
            STSYS_WriteRegDev32LE(Addr2, 0x69);

            // Set PCM mode to ON
            STSYS_WriteRegDev32LE(Addr3, (STSYS_ReadRegDev32LE(Addr3) | ((U32)0x2)));
            break;
            
        default:
            //STTBX_Print(("Unidentified PCM player identifier\n"));
            break;
            
    }
}

void StopPCMPlayerIP(PCMPlayerControl_t * Control_p)
{
    U32 Addr2 = 0;

    switch(Control_p->pcmPlayerIdentifier)
    {
        case PCM_PLAYER_0:
            Addr2 = Control_p->BaseAddress.PCMPlayer0BaseAddr + 0x1C;//PCMPLAYER0_CONTROL_REG
            STSYS_WriteRegDev32LE(Addr2, ((U32)STSYS_ReadRegDev32LE(Addr2) & 0xFFFFFFFC));
            break;

        case PCM_PLAYER_1:
            Addr2 = Control_p->BaseAddress.AudioConfigBaseAddr + 0x100;//AUDIO_DAC_PCMP1
            STSYS_WriteRegDev32LE(Addr2, ((U32)STSYS_ReadRegDev32LE(Addr2) & 0xFFFFFFFC));
            break;

        default:
            //STTBX_Print(("Unidentified PCM player identifier\n"));
            break;
            
    }
}


#if ENABLE_PCM_INTERRUPT

void PCMPlayerSetInterruptNumber(PCMPlayerControl_t *Control_p)
{
    switch(Control_p->pcmPlayerIdentifier)
    {
        case PCM_PLAYER_0:
#if defined(ST_7100)
            Control_p->InterruptNumber = ST7100_PCM_PLAYER_0_INTERRUPT;
#elif defined(ST_7109)
            Control_p->InterruptNumber = ST7109_PCM_PLAYER_0_INTERRUPT;
#endif
            break;

        case PCM_PLAYER_1:
#if defined(ST_7100)
            Control_p->InterruptNumber = ST7100_PCM_PLAYER_1_INTERRUPT;
#elif defined(ST_7109)
            Control_p->InterruptNumber = ST7109_PCM_PLAYER_1_INTERRUPT;
#endif
            break;

        default:
            //STTBX_Print(("Unidentified PCM player identifier\n"));
            break;
    }
}

void PCMPlayerEnableInterrupt(PCMPlayerControl_t *Control_p)
{
    U32 Addr1;
    
    switch(Control_p->pcmPlayerIdentifier)
    {
        case PCM_PLAYER_0:
            Addr1 = Control_p->BaseAddress.PCMPlayer0BaseAddr + 0x14;//PCMPLAYER0_ITRP_ENABLE_SET_REG
            STSYS_WriteRegDev32LE(Addr1,0x1);
            break;

        case PCM_PLAYER_1:
            Addr1 = Control_p->BaseAddress.PCMPlayer1BaseAddr + 0x14;//PCMPLAYER1_ITRP_ENABLE_SET_REG
            STSYS_WriteRegDev32LE(Addr1,0x1);
            break;

        default:
            //STTBX_Print(("Unidentified PCM player identifier\n"));
            break;
    }
}

void PCMPlayerDisbleInterrupt(PCMPlayerControl_t *Control_p)
{
    U32 Addr1;
    switch(Control_p->pcmPlayerIdentifier)
    {
        case PCM_PLAYER_0:
            Addr1 = Control_p->BaseAddress.PCMPlayer0BaseAddr + 0x18;//PCMPLAYER0_ITRP_ENABLE_CLEAR_REG
            STSYS_WriteRegDev32LE(Addr1,0x1);
            break;

        case PCM_PLAYER_1:
            Addr1 = Control_p->BaseAddress.PCMPlayer1BaseAddr + 0x18;//PCMPLAYER1_ITRP_ENABLE_CLEAR_REG
            STSYS_WriteRegDev32LE(Addr1,0x1);
            break;

        default:
            //STTBX_Print(("Unidentified PCM player identifier\n"));
            break;
    }
}

#endif //ENABLE_PCM_INTERRUPT

void StartPCMPlayerIP(PCMPlayerControl_t * Control_p)
{
    U32 Addr1, Addr2, Addr3, Addr4 /*,Addr5*/, Addr6;
    U32 value = 0;
    STAUD_PCMDataConf_t * pcmPlayerDataConfig_p = &(Control_p->pcmPlayerDataConfig);

    Addr1 = Addr2 = Addr3 = Addr4 = 0;    
    //Addr5=0;
    
    if(Control_p->NumChannels==2)
    {
#if defined(ST_7100)
        value = 0x00004100;
#elif defined(ST_7109)
        value = 0x00002100;
#endif
    }
    else
    {
#if defined(ST_7100)
        value = 0x0000a500;
#elif defined(ST_7109)
        value = 0x0000a500;
#endif
    }

    switch(Control_p->pcmPlayerIdentifier)
    {
        case PCM_PLAYER_0:

            Addr1 = Control_p->BaseAddress.AudioConfigBaseAddr +  0x200;
            Addr2 = Control_p->BaseAddress.PCMPlayer0BaseAddr + 0x1C;
            Addr3 = Control_p->BaseAddress.PCMPlayer0BaseAddr + 0x24;
            //Addr4 = Control_p->BaseAddress.PCMPlayer0BaseAddr + 0x14;
            //Addr5 = Control_p->BaseAddress.PCMPlayer0BaseAddr + 0x00;

            STSYS_WriteRegDev32LE(Addr1, 0xf);
            STSYS_WriteRegDev32LE(Addr2, ((U32)PCMPlayer_ConvertFsMultiplierToClkDivider(pcmPlayerDataConfig_p->PcmPlayerFrequencyMultiplier)<<4) |
                                                                ((U32)pcmPlayerDataConfig_p->MemoryStorageFormat16 << 2) |
                                                                ((U32)0x2));

            STSYS_WriteRegDev32LE(Addr3, ((U32)pcmPlayerDataConfig_p->MSBFirst << 7) |
                                                                ((U32)pcmPlayerDataConfig_p->Alignment << 6) |
                                                                ((U32)pcmPlayerDataConfig_p->Format << 5) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertBitClock << 4) |
                                                                ((U32)pcmPlayerDataConfig_p->InvertWordClock << 3) |
                                                                ((U32)pcmPlayerDataConfig_p->Precision << 1) |
                                                                (value));
            
            //STSYS_WriteRegDev32LE(Addr4,0x3);
            //STSYS_WriteRegDev32LE(Addr5,0x1);
            //STSYS_WriteRegDev32LE(Addr5,0x0);
            
            break;

        case PCM_PLAYER_1:

            Addr1 = Control_p->BaseAddress.AudioConfigBaseAddr +  0x200;
            Addr2 = Control_p->BaseAddress.AudioConfigBaseAddr + 0x100;
            Addr3 = Control_p->BaseAddress.PCMPlayer1BaseAddr + 0x1C;
            Addr4 = Control_p->BaseAddress.PCMPlayer1BaseAddr + 0x24;
            //Addr5 = Control_p->BaseAddress.PCMPlayer1BaseAddr + 0x14;
            Addr6 = Control_p->BaseAddress.PCMPlayer1BaseAddr + 0x00;

            STSYS_WriteRegDev32LE(Addr1, 0xf);
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

            //STSYS_WriteRegDev32LE(Addr5,0x3);
            STSYS_WriteRegDev32LE(Addr6, 0x1);
            STSYS_WriteRegDev32LE(Addr2, 0x68);
            STSYS_WriteRegDev32LE(Addr6, 0x0);
            STSYS_WriteRegDev32LE(Addr2, 0x69);

            break;

        default:
            //STTBX_Print(("Unidentified PCM player identifier\n"));
            break;
            
    }
}

#if ENABLE_PCM_INTERRUPT
#if defined( ST_OSLINUX )

irqreturn_t PcmPInterruptHandler( int irq, void *Data, struct pt_regs *regs)
{
    PCMPlayerControlBlock_t * ControlBlock_p = (PCMPlayerControlBlock_t*) Data;
    
    if(ControlBlock_p)
    {
        PCMPlayerControl_t * Control_p= &(ControlBlock_p->pcmPlayerControl);
        U32 Addr1 , Addr2;
        U32 AudInterruptStatus = 0;

        switch(Control_p->pcmPlayerIdentifier)
        {
            case PCM_PLAYER_0:
                Addr1 = ControlBlock_p->BaseAddress.PCMPlayer0BaseAddr + 0x08;
                Addr2 = ControlBlock_p->BaseAddress.PCMPlayer0BaseAddr + 0x0C;
                
                AudInterruptStatus = (STSYS_ReadRegDev32LE(Addr1) & 0x00000001);
                if(AudInterruptStatus)
                {
                    Control_p->Underflow_Count++;
                }
                STSYS_WriteRegDev32LE(Addr2,0x1);
                break;

            case PCM_PLAYER_1:
                Addr1 = ControlBlock_p->BaseAddress.PCMPlayer1BaseAddr + 0x08;
                Addr2 = ControlBlock_p->BaseAddress.PCMPlayer1BaseAddr + 0x0C;
                
                AudInterruptStatus = (STSYS_ReadRegDev32LE(Addr1) & 0x00000001);
                if(AudInterruptStatus)
                {
                    Control_p->Underflow_Count++;
                }
                STSYS_WriteRegDev32LE(Addr2, 0x1);
                break;
                
            default:
            break;
        }
    }
    return IRQ_HANDLED;
}
#endif


#if defined(ST_OS21)
int PcmPInterruptHandler(void* pParams)
{
    PCMPlayerControlBlock_t * ControlBlock_p = (PCMPlayerControlBlock_t*)pParams;
    
    if(ControlBlock_p)
    {
        PCMPlayerControl_t * Control_p= &(ControlBlock_p->pcmPlayerControl);
        U32 Addr1 , Addr2;
        U32 AudInterruptStatus = 0;

        switch(Control_p->pcmPlayerIdentifier)
        {
            case PCM_PLAYER_0:
                Addr1 = ControlBlock_p->BaseAddress.PCMPlayer0BaseAddr + 0x08;
                Addr2 = ControlBlock_p->BaseAddress.PCMPlayer0BaseAddr + 0x0C;
                
                AudInterruptStatus = (STSYS_ReadRegDev32LE(Addr1) & 0x00000001);                
                if(AudInterruptStatus)
                {
                    Control_p->Underflow_Count++;
                }
                STSYS_WriteRegDev32LE(Addr2, 0x1);
                break;

            case PCM_PLAYER_1:
                Addr1 = ControlBlock_p->BaseAddress.PCMPlayer1BaseAddr + 0x08;
                Addr2 = ControlBlock_p->BaseAddress.PCMPlayer1BaseAddr + 0x0C;

                AudInterruptStatus = (STSYS_ReadRegDev32LE(Addr1) & 0x00000001);
                if(AudInterruptStatus)
                {
                    Control_p->Underflow_Count++;
                }
                STSYS_WriteRegDev32LE(Addr2, 0x1);
                break;

            default:
                break;
        }
    }    
    return (OS21_SUCCESS);
}
#endif
#endif

ST_ErrorCode_t STAud_PCMEnableHDMIOutput(STPCMPLAYER_Handle_t PlayerHandle, BOOL Command)
{
    U32 Addr1, Addr2, Addr3, Addr4;
    PCMPlayerControlBlock_t * tempControlBlock_p = pcmPlayerControlBlock;
    PCMPlayerControl_t * Control_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* Obtain the control block from the passed in handle */
    while (tempControlBlock_p != NULL)
    {
        if (tempControlBlock_p == (PCMPlayerControlBlock_t *)PlayerHandle)
        {
            /* Got the control block for current instance so break */
            break;
        }
        tempControlBlock_p = tempControlBlock_p->next;
    }
    
    if(tempControlBlock_p== NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    Control_p = &(tempControlBlock_p->pcmPlayerControl);

    /* Get hold of HDMI registers */
    Addr1 = Control_p->BaseAddress.AudioConfigBaseAddr +  0x204;
    Addr2 = Control_p->BaseAddress.I2S2SpdifConverterBaseAddr + I2S_AUD_SPDIF_PR_CFG;
    Addr3 = Control_p->BaseAddress.I2S2SpdifConverterBaseAddr + I2S_AUD_SPDIF_PR_SPDIF_CTRL;
    Addr4 = Control_p->BaseAddress.HDMIBaseAddress + 0x200;
    
    if(Command)
    {
        /* Enable HDMI */
        switch(Control_p->pcmPlayerDataConfig.PcmPlayerFrequencyMultiplier)
        {
            case 128:
                STSYS_WriteRegDev32LE(Addr1, 0x0);
                STSYS_WriteRegDev32LE(Addr2, 0x25);
                STSYS_WriteRegDev32LE(Addr3, 0x4033);
                STSYS_WriteRegDev32LE(Addr4, 0);
                break;
                
#if defined (ST_7109)
            case 256:
            {
                U32 Rev_7109 = ST_GetCutRevision();
                STTBX_Print(("7109 Revision %x\n",Rev_7109));
                Rev_7109 &= 0xF0;
                switch(Rev_7109)
                {
                    case 0xC0:
                        STSYS_WriteRegDev32LE(Addr1, 0x0);
                        STSYS_WriteRegDev32LE(Addr2, 0x25);
                        STSYS_WriteRegDev32LE(Addr3, 0x4053);
                        STSYS_WriteRegDev32LE(Addr4, 2);
                        break;
                        
                    default:
                        Error = STAUD_ERROR_INVALID_STATE;
                        break;
                }
            }
                break;
#endif

            default:
                Error = STAUD_ERROR_INVALID_STATE;
                break;
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


ST_ErrorCode_t  STAud_PlayerMemRemap(AudioBaseAddress_t *BaseAddress)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 AudioPeriBaseAdd = 0 , AudioPeriTotalSize = 0;

    /* Update the number of users using remapped buffers */
    PlayerUserCount++;

    if(BaseAddress->IsInit == FALSE)
    {
        /* Map IO Space to kernel */
        AudioPeriBaseAdd = PCMPLAYER0_BASE;//for 7200 it will be PCMREADER_BASE and AudioPeriTotalSize = 4 * ( 4 * 1024);
        AudioPeriTotalSize = 4 * ( 4 * 1024);/* 4 Peripherals , 4KB mem for each */

        memset(BaseAddress,0,sizeof(AudioBaseAddress_t));

        /* PCM0 Player */
        BaseAddress->PCMPlayer0BaseAddr = (U32)STOS_MapRegisters((void*)AudioPeriBaseAdd, AudioPeriTotalSize,"AUD_PLAYERS");
        if(!BaseAddress->PCMPlayer0BaseAddr)
        {
            STTBX_Print(("Audio STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;;
        }
        
        BaseAddress->PCMPlayer1BaseAddr = BaseAddress->PCMPlayer0BaseAddr + (PCMPLAYER1_BASE-PCMPLAYER0_BASE);
        BaseAddress->PCMReaderBaseAddr = BaseAddress->PCMPlayer1BaseAddr + (PCMREADER_BASE-PCMPLAYER1_BASE);
        BaseAddress->SPDIFPlayerBaseAddr = BaseAddress->PCMReaderBaseAddr + (SPDIFPLAYER_BASE-PCMREADER_BASE);

        /* Audio config */
        BaseAddress->AudioConfigBaseAddr = (U32)STOS_MapRegisters((void*)AUDIO_CONFIG_BASE, AUDIO_CONFIG_REGION_SIZE, "AUD_CONFIG");
        if(!BaseAddress->AudioConfigBaseAddr)
        {
            STTBX_Print(("BaseAddress.AudioConfigBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }

        /* I2S to SPDIF coverter, used for HDMI output */
        BaseAddress->I2S2SpdifConverterBaseAddr = (U32)STOS_MapRegisters((void*)I2S_SPDIF_CONVERTER_BASE_ADDRESS, I2S_SPDIF_CONVERTER_REGION_SIZE, "AUD_I2S_REG1");
        if((!BaseAddress->I2S2SpdifConverterBaseAddr))
        {
            STTBX_Print(("BaseAddress.I2S2SpdifConverterBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }

        /* SPDIF additional data region used for SPDIF PCM mode*/
        BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr = 0;
        BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr = (U32)STOS_MapRegisters((void*)FDMA_BASE_ADDRESS + SPDIF_ADDITIONAL_DATA_REGION, 64, "AUD_FDMA_ADR");
        if(!BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr)
        {
            STTBX_Print(("BaseAddress.SPDIFFDMAAdditionalDataRegionBaseAddr STOS_MapRegisters failed.\n"));
            Error = STAUD_ERROR_IO_REMAP_FAILED;
        }
        
        /* HDMI Base address for writing the frequency divider value to the HDMI reg */
        BaseAddress->HDMIBaseAddress=0;
        BaseAddress->HDMIBaseAddress= (U32)STOS_MapRegisters((void*)HDMI_BASE_ADDRESS, 1024, "HDMI_BASE_ADR");
        if(!BaseAddress->HDMIBaseAddress)
        {
            STTBX_Print(("BaseAddress.HDMIBaseAddress STOS_MapRegisters failed.\n"));
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
            STOS_UnmapRegisters((void*)BaseAddress->PCMPlayer0BaseAddr, (4 * (4 * 1024)));
            STOS_UnmapRegisters((void*)BaseAddress->AudioConfigBaseAddr, AUDIO_CONFIG_REGION_SIZE);
            STOS_UnmapRegisters((void*)BaseAddress->I2S2SpdifConverterBaseAddr, I2S_SPDIF_CONVERTER_REGION_SIZE);
            STOS_UnmapRegisters((void*)BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr, 64);
            STOS_UnmapRegisters((void*)BaseAddress->HDMIBaseAddress, 1024);
            BaseAddress->IsInit = FALSE;
        }
    }
    return Error;    
}

/* Dummy Function (Needed for 7200 Chipset) */
ST_ErrorCode_t STAud_OPSetScenario(STAUD_Scenario_t Scenario)
{
    ST_ErrorCode_t Error=ST_ERROR_FEATURE_NOT_SUPPORTED;
    return Error;
}


