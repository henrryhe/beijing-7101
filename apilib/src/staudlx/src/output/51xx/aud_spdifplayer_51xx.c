/*
 * Copyright (C) STMicroelectronics Ltd. 2004.
 *
 * All rights reserved.
 *						------->SPDIF PLAYER
 *						|
 * DECODER -------|
 *						|
 *						------->PCM PLAYER
 */
//#define  STTBX_PRINT
#ifndef ST_OSLINUX
    #define __OS_20TO21_MAP_H /* prevent os_20to21_map.h from being included*/
    #include "sttbx.h"
#endif
#include "stos.h"
#include "aud_spdifplayer.h"
#include "audioreg.h"
//#define STAUD_REMOVE_CLKRV_SUPPORT 1
#ifdef STAUD_REMOVE_CLKRV_SUPPORT
    #define SYS_SERVICES_BASE_ADDRESS      0x20F00000
    #define CKG_REGISTER_LOCK             (SYS_SERVICES_BASE_ADDRESS + 0x300)
    #define CKG_SPDIF_CLK_SETUP0          (SYS_SERVICES_BASE_ADDRESS + 0x030)
    #define CKG_SPDIF_CLK_SETUP1          (SYS_SERVICES_BASE_ADDRESS + 0x034)
    #define CKG_FSA_SETUP                 (SYS_SERVICES_BASE_ADDRESS + 0x010)
    #define CKG_SPARE1_CLK_SETUP0         (SYS_SERVICES_BASE_ADDRESS + 0x014)
    typedef struct
    {
        int  SDIV_Val;
        int  PE_Val;
        int  MD_Val;
    }SETCLKDSP;

    static SETCLKDSP CLK_VALUES[]=
    {
        0x5, 0x5100, 0x1A, 	/* Freq =  4.096 MHz : 256 * 16000 Hz, sdiv = 64 */
        0x5, 0x6EAC, 0x13,   /* Freq =  5.644 MHz : 256 * 22050 Hz, sdiv = 64 */
        0x5, 0x3600, 0x11,   /* Freq =  6.144 MHz : 256 * 24000 Hz, sdiv = 64 */
        0x4, 0x5100, 0x1A,   /* Freq =  8.192 MHz : 256 * 32000 Hz, sdiv = 32 */
        0x4, 0x6EE3, 0x13,   /* Freq = 11.289 MHz : 256 * 44100 Hz, sdiv = 32 */
        0x4, 0x3600, 0x11   /* Freq = 12.288 MHz : 256 * 48000 Hz, sdiv = 32 */
    };
#endif
ST_ErrorCode_t	SetSPDIFPlayerSamplingFrequency(SPDIFPlayerControl_t * SPDIFPlayerControl_p)
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
    		STTBX_Print(("Setting new SPDIF frequency %d\n", SamplingFreqency));
			break;
		default:
			//STTBX_Print(("Unidentified SPDIF player identifier\n"));
			Error = ST_ERROR_UNKNOWN_DEVICE;
			break;
	}

	return (Error);
}
#ifdef STAUD_REMOVE_CLKRV_SUPPORT

    ST_ErrorCode_t SPDIFPlayer_SetFrequencySynthesizer (SPDIFPlayerControl_t *SPDIFPlayerControl_p, U32 samplingFrequency)
    {
    	U32 Value = 0;
    	U32 i = 0;
    	/* To use a switch for all cases to be used 22.05, 16, 24, 32, 44.1 and 48
    	 * in our case we always use 256*fs */
    	U32 AudioConfigBaseAddr = SPDIFPlayerControl_p->BaseAddress.SystemServiceBaseAddr;

    	switch(samplingFrequency)
    	{
    		case 16000 :
    			i = 0;
    			break;
    		case 22050 :
    			i = 1;
    			break;
    		case 24000 :
    			i = 2;
    			break;
    		case 32000 :
    			i = 3;
    			break;
    		case 44100 :
    			i = 4;
    			break;
    		case 48000 :
    			i = 5;
    			break;
    		default:
    			break;
    	}


      	 /*------Unlocking the REGISTER_LOCK_CFG, to access the Sythesizer registers------*/
    	 STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x300, 0xF0);
    	 STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x300, 0x0F);
    	 STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x170, 0x0);

    	  /*------Programing the Sythesizer registers to generate Clock------*/
    	  /*---------------------------------------------------------------------
    	 * Bit [4:0]   MD
    	 * Bit 5       EN_PRG  = 1 => Enable Prog
    	 * Bit [8:6]   SDIV
    	 * Bit 9       SEL_OUT = 1 => Enable Clock Output
    	 * Bit 10      RESET_N = 1 => Not reset
    	 * Bit 11      Enable clock = 1
    	 * Bit [31:11] Reserved
    	 * ------------------------------------------------------------------------*/
    	Value = 0x0;
    	Value = ((CLK_VALUES[i].MD_Val) & 0x1F)|((CLK_VALUES[i].SDIV_Val) << 6) | (0 << 5) | (0x3 << 9) |(1<<11);

    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x030, Value);

    	/*---------------------------------------------------------------------
    	 * Bit [15:0]    PE
    	 * Bit [31:16]   Reserved
    	 * ------------------------------------------------------------------------*/
    	Value = 0;
    	Value = ((CLK_VALUES[i].PE_Val) & 0x0000FFFF);

    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x034, Value);


    	/*Enabling the SET Values to taken into account for Freq Gen*/
    	Value = 0;
    	Value = STSYS_ReadRegDev32LE(AudioConfigBaseAddr+0x030);
    	Value |= 0x20;
    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x030, Value);
    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x170, 0x1);

    	/*------Programing the Config register of the Sythesizer------*/
    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x010, 0xF08);

    	 /*Initiating the Reset Sequency */
    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x014, 0x000);
    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x014, 0x400);


    	/*------Programing the Config register of the Sythesizer------*/
    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x010, 0xF00);

    	/*------Locking the REGISTER_LOCK_CFG-----------------------*/
    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x300, 0x100);

    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x170, 1);
    }
#endif
void StopSPDIFPlayerIP(SPDIFPlayerControl_t * SPDIFPlayerControl_p)
{
	U32 AudioConfigBaseAddr = SPDIFPlayerControl_p->BaseAddress.SPDIFPlayerBaseAddr;

	switch(SPDIFPlayerControl_p->spdifPlayerIdentifier)
	{
		case SPDIF_PLAYER_0:
			STSYS_WriteRegDev32LE(AudioConfigBaseAddr + SPDIF_CTRL, ((U32)STSYS_ReadRegDev32LE(AudioConfigBaseAddr + SPDIF_CTRL) & 0xFFFFFFF8));
			break;
		default:
			STTBX_Print(("Unidentified SPDIF player identifier\n"));
			break;

	}
}

void StartSPDIFPlayerIP(SPDIFPlayerControl_t * SPDIFPlayerControl_p)
{
	U32 delay, control, Addr1;
	U32 AudioConfigBaseAddr = SPDIFPlayerControl_p->BaseAddress.SPDIFPlayerBaseAddr;

	switch(SPDIFPlayerControl_p->spdifPlayerIdentifier)
	{
		case SPDIF_PLAYER_0:

			STSYS_WriteRegDev32LE(AudioConfigBaseAddr+SPDIF_SOFTRESET, 1);
			delay = 10;
			AUD_TaskDelayMs(delay);
			STSYS_WriteRegDev32LE(AudioConfigBaseAddr+SPDIF_SOFTRESET, 0);
			// Set the mode in the control register
			control = SPDIFPlayerControl_p->SPDIFMode;
			control |= (U32)(SPDIFPlayer_ConvertFsMultiplierToClkDivider(SPDIFPlayerControl_p->SPDIFPlayerOutParams.SPDIFPlayerFrequencyMultiplier)<<5);
            if (SPDIFPlayerControl_p->CompressedDataAlignment ==  BE)
            {
			    control |= (U32)((1)<<13);
            }
			STSYS_WriteSpdifPlayerReg32(SPDIF_CTRL, control);
			switch (SPDIFPlayerControl_p->SPDIFMode)
			{
				case STAUD_DIGITAL_MODE_NONCOMPRESSED:
					STSYS_WriteRegDev32LE(AudioConfigBaseAddr+SPDIF_CL1, CalculateSPDIFChannelStatus0(SPDIFPlayerControl_p));
					STSYS_WriteRegDev32LE(AudioConfigBaseAddr+SPDIF_CR1, CalculateSPDIFChannelStatus0(SPDIFPlayerControl_p));
					// Validity 0
					STSYS_WriteRegDev32LE(AudioConfigBaseAddr+SPDIF_CL2_CR2_U_V, (CalculateSPDIFChannelStatus1(SPDIFPlayerControl_p)<<8) | CalculateSPDIFChannelStatus1(SPDIFPlayerControl_p));
					Addr1 = SPDIFPlayerControl_p->BaseAddress.SPDIFFDMAAdditionalDataRegionBaseAddr +  0x28;
					// Write precision mask register to FDMA additional data region
					//STSYS_WriteRegDev32LE(Addr1, 0xFFFFFF00);
					STFDMA_SetAddDataRegionParameter(SPDIFPlayerControl_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_DATA_PRECISION_MASK, 0xFFFFFF00);
					STFDMA_SetAddDataRegionParameter(SPDIFPlayerControl_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAMES_TO_GO, 0);
					STFDMA_SetAddDataRegionParameter(SPDIFPlayerControl_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAME_COUNT, 0);
					break;
				case STAUD_DIGITAL_MODE_COMPRESSED:
					Addr1 = SPDIFPlayerControl_p->BaseAddress.SPDIFFDMAAdditionalDataRegionBaseAddr +  0x24;

					// Write compressed mode reset to FDMA additional data region
					//STSYS_WriteRegDev32LE(Addr1, 0);
					STFDMA_SetAddDataRegionParameter(SPDIFPlayerControl_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAMES_TO_GO, 0);
					STFDMA_SetAddDataRegionParameter(SPDIFPlayerControl_p->FDMABlock, SPDIF_ADDITIONAL_DATA_REGION_3, SPDIF_FRAME_COUNT, 0);


					STSYS_WriteRegDev32LE(AudioConfigBaseAddr + SPDIF_CL1, CalculateSPDIFChannelStatus0(SPDIFPlayerControl_p));
					STSYS_WriteRegDev32LE(AudioConfigBaseAddr + SPDIF_CR1, CalculateSPDIFChannelStatus0(SPDIFPlayerControl_p));
					// Validity 1
					STSYS_WriteRegDev32LE(AudioConfigBaseAddr + SPDIF_CL2_CR2_U_V, 0x000C0000 | (CalculateSPDIFChannelStatus1(SPDIFPlayerControl_p)<<8) | (CalculateSPDIFChannelStatus1(SPDIFPlayerControl_p)));

					STSYS_WriteRegDev32LE(AudioConfigBaseAddr + SPDIF_PA_PB, 0xF8724E1F);
					STSYS_WriteRegDev32LE(AudioConfigBaseAddr + SPDIF_PC_PD, 0x00013000);

					// Generate 1 PAUSE burst of 192 SPDIF frames
					STSYS_WriteRegDev32LE(AudioConfigBaseAddr + SPDIF_PAUSE_LAT, 0x00010000);
					STSYS_WriteRegDev32LE(AudioConfigBaseAddr + SPDIF_BURST_LEN, 0x06000600);
					break;
				default:
					break;
			}


//to be done
			STSYS_WriteRegDev32LE(0xB9210200, 0xF);
			break;
		default:
			STTBX_Print(("Unidentified SPDIF player identifier\n"));
			break;

	}
}
