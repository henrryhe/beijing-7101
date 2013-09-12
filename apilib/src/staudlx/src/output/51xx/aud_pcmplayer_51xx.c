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
#include "sttbx.h"
#include "stos.h"
#include "aud_pcmplayer.h"
#include "audioreg.h"

extern U32 PlayerUserCount; /* Number of player modules which are initialized */

#define SYS_SERVICES_SIZE      		   0x100000
//#define STAUD_REMOVE_CLKRV_SUPPORT 1

#ifdef STAUD_REMOVE_CLKRV_SUPPORT
    #define CKG_REGISTER_LOCK             (SYS_SERVICES_BASE_ADDRESS + 0x300)
    #define CKG_SPDIF_CLK_SETUP0          (SYS_SERVICES_BASE_ADDRESS + 0x030)
    #define CKG_SPDIF_CLK_SETUP1          (SYS_SERVICES_BASE_ADDRESS + 0x034)
    #define CKG_FSA_SETUP                 (SYS_SERVICES_BASE_ADDRESS + 0x010)
    #define CKG_SPARE1_CLK_SETUP0         (SYS_SERVICES_BASE_ADDRESS + 0x014)
    #define CKG_PCM_CLK_SETUP0            (SYS_SERVICES_BASE_ADDRESS + 0x020)
    #define CKG_PCM_CLK_SETUP1            (SYS_SERVICES_BASE_ADDRESS + 0x024)

    typedef struct
    {
        int  SDIV_Val;
        int  PE_Val;
        int  MD_Val;
    }SETCLKDSP;

    static SETCLKDSP CLK_VALUES[]=
    {
    0x5,  0x5100,  0x1A,	/* Freq =  4.096 MHz : 256 * 16000 Hz, sdiv = 64 */
    0x5,  0x6EAC,  0x13,   /* Freq =  5.644 MHz : 256 * 22050 Hz, sdiv = 64 */
    0x5,  0x3600,  0x11,   /* Freq =  6.144 MHz : 256 * 24000 Hz, sdiv = 64 */
    0x4,  0x5100,  0x1A,   /* Freq =  8.192 MHz : 256 * 32000 Hz, sdiv = 32 */
    0x4,  0x6EE3,  0x13,   /* Freq = 11.289 MHz : 256 * 44100 Hz, sdiv = 32 */
    0x4,  0x3600,  0x11   /* Freq = 12.288 MHz : 256 * 48000 Hz, sdiv = 32 */
    };
#endif
extern PCMPlayerControlBlock_t *pcmPlayerControlBlock;

void PCMPlayer_DACConfigure(PCMPlayerControl_t * PCMPlayerControl_p)
{

	U32 PcmOutputControl;
	U32 AudioConfigBaseAddr     = PCMPlayerControl_p->BaseAddress.NHD2ConfigBaseAddr;
    #if defined(ST_5107)||defined(ST_5105)
    	//soft reset
    	PcmOutputControl            = STSYS_ReadRegDev32LE((AudioConfigBaseAddr+CONFIG_CTRL_D))&0x81E00;
    	STSYS_WriteRegDev32LE((AudioConfigBaseAddr+CONFIG_CTRL_D), PcmOutputControl);
    	task_delay(6250);
    	PcmOutputControl            = PcmOutputControl|0x78000;//audio Unmuted
    	STSYS_WriteRegDev32LE((AudioConfigBaseAddr+CONFIG_CTRL_D), PcmOutputControl);
    #elif defined(ST_5162)
    	PcmOutputControl            = STSYS_ReadRegDev32LE((AudioConfigBaseAddr+CONFIG_CTRL_D))&0x81FFF;
    	PcmOutputControl            = PcmOutputControl|0x00002000;//reset dac
    	STSYS_WriteRegDev32LE((AudioConfigBaseAddr+CONFIG_CTRL_D), PcmOutputControl);
    	task_delay(6250);
    	PcmOutputControl            = PcmOutputControl&0x81FFF;
    	PcmOutputControl            = PcmOutputControl|0x78000;//audio Unmuted
    	STSYS_WriteRegDev32LE((AudioConfigBaseAddr+CONFIG_CTRL_D), PcmOutputControl);
    #endif
//to be changed later
}
/* Dummy Function (Needed for 7200 Chipset) */
ST_ErrorCode_t STAud_OPSetScenario(STAUD_Scenario_t Scenario)
{
	ST_ErrorCode_t Error=ST_ERROR_FEATURE_NOT_SUPPORTED;
    return Error;
}
#ifdef STAUD_REMOVE_CLKRV_SUPPORT
    ST_ErrorCode_t PCMPlayer_SetFrequencySynthesizer (PCMPlayerControl_t * PCMPlayerControl_p, U32 samplingFrequency)
    {
    	U32 Value = 0;
    	U32 i = 0;
    	/* To use a switch for all cases to be used 22.05, 16, 24, 32, 44.1 and 48
    	 * in our case we always use 256*fs */
    	U32 AudioConfigBaseAddr = PCMPlayerControl_p->BaseAddress.SystemServiceBaseAddr;

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
    	Value = ((CLK_VALUES[i].MD_Val) & 0x1F)|((CLK_VALUES[i].SDIV_Val) << 6) | (0 << 5) | (0x3 << 9) |(1<<11);

    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x020, Value);

    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x030, Value);

    	/*---------------------------------------------------------------------
    	 * Bit [15:0]    PE
    	 * Bit [31:16]   Reserved
    	 * ------------------------------------------------------------------------*/
    	Value = ((CLK_VALUES[i].PE_Val) & 0x0000FFFF);
        STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x024, Value);


    	/*Enabling the SET Values to taken into account for Freq Gen*/
        Value = STSYS_ReadRegDev32LE(AudioConfigBaseAddr+0x020);
    	Value |= 0x20;
    	STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x020, Value);
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
void ResetPCMPLayerIP(PCMPlayerControl_t * PCMPlayerControl_p)
{
    U32    Addr2, Addr3, Addr6;

    Addr2 = Addr3 = Addr6 = 0;

	switch(PCMPlayerControl_p->pcmPlayerIdentifier)
	{
		case PCM_PLAYER_0:
			// Nothing to be done for PCM player Ip
			break;
		default:
			//STTBX_Print(("Unidentified PCM player identifier\n"));
			break;
	}



}

void StopPCMPlayerIP(PCMPlayerControl_t * PCMPlayerControl_p)
{
	U32 AudioConfigBaseAddr = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr;

	switch(PCMPlayerControl_p->pcmPlayerIdentifier)
	{
		case PCM_PLAYER_0:
            STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x1C, ((U32)STSYS_ReadRegDev32LE(AudioConfigBaseAddr+0x1C) & 0xFFFFFFFC));
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
    			Control_p->InterruptNumber = PCM_PLAYER_INTERRUPT;
    			break;
    		default:
    			break;
    	}

    }
    void PCMPlayerEnableInterrupt(PCMPlayerControl_t *Control_p)
    {
    	U32 AudioConfigBaseAddr = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr;
    	switch(Control_p->pcmPlayerIdentifier)
    	{
    		case PCM_PLAYER_0:
    			STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x14, 0x1);
    			break;
    		default:
    			//STTBX_Print(("Unidentified PCM player identifier\n"));
    			break;
    	}

    }
    void PCMPlayerDisbleInterrupt(PCMPlayerControl_t *Control_p)
    {
    	U32 AudioConfigBaseAddr = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr;
    	switch(Control_p->pcmPlayerIdentifier)
    	{
    		case PCM_PLAYER_0:
    			STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x18, 0x1);
    			break;
    		default:
    			//STTBX_Print(("Unidentified PCM player identifier\n"));
    			break;
    	}

    }
#endif




void StartPCMPlayerIP(PCMPlayerControl_t * PCMPlayerControl_p)
{
    U32 PcmOutputControl;
    U32	value = 0;
	STAUD_PCMDataConf_t	* pcmPlayerDataConfig_p = &(PCMPlayerControl_p->pcmPlayerDataConfig);
	U32 AudioConfigBaseAddr = PCMPlayerControl_p->BaseAddress.PCMPlayer0BaseAddr;
	U32 AudioConfigBaseAddr1 = PCMPlayerControl_p->BaseAddress.NHD2ConfigBaseAddr;

	switch(PCMPlayerControl_p->pcmPlayerIdentifier)
	{
		case PCM_PLAYER_0:

            #if defined(ST_5162)
          		value = 0x00001100;
            #else
         		value = 0;
            #endif

    		STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x1C, ((U32)PCMPlayer_ConvertFsMultiplierToClkDivider(pcmPlayerDataConfig_p->PcmPlayerFrequencyMultiplier)<<4) |
    											((U32)pcmPlayerDataConfig_p->MemoryStorageFormat16 << 2) |
    											((U32)0x2));
    		STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x24, ((U32)pcmPlayerDataConfig_p->MSBFirst << 7) |
    											((U32)pcmPlayerDataConfig_p->Alignment << 6) |
    											((U32)pcmPlayerDataConfig_p->Format << 5) |
    											((U32)pcmPlayerDataConfig_p->InvertBitClock << 4) |
    											((U32)pcmPlayerDataConfig_p->InvertWordClock << 3) |
    											((U32)pcmPlayerDataConfig_p->Precision << 1) |
    											(value));

            //Alternate output at pio2
            PcmOutputControl = STSYS_ReadRegDev32LE(AudioConfigBaseAddr1+CONFIG_CTRL_F);
            PcmOutputControl = (PcmOutputControl& 0xFF00FFFF)|0x00E00000;
    		STSYS_WriteRegDev32LE(AudioConfigBaseAddr1+CONFIG_CTRL_F, PcmOutputControl);
    		break;
		default:
			//STTBX_Print(("Unidentified PCM player identifier\n"));
			break;

	}
}
#if ENABLE_PCM_INTERRUPT
    void PcmPInterruptHandler(void* pParams)
    {
    	PCMPlayerControlBlock_t * ControlBlock_p = (PCMPlayerControlBlock_t*)pParams;
    	if(ControlBlock_p)
    	{
    		PCMPlayerControl_t * Control_p = &(ControlBlock_p->pcmPlayerControl);
    		U32 AudInterruptStatus = 0;
    		U32 AudioConfigBaseAddr = ControlBlock_p->BaseAddress.PCMPlayer0BaseAddr;

    		switch(Control_p->pcmPlayerIdentifier)
    		{
    			case PCM_PLAYER_0:
    				AudInterruptStatus = (STSYS_ReadRegDev32LE(AudioConfigBaseAddr+0x08) & 0x00000001);
    				if(AudInterruptStatus)
    				{
    					Control_p->Underflow_Count++;
    				}
    				STSYS_WriteRegDev32LE(AudioConfigBaseAddr+0x0C, 0x1);
    				break;
    			default:
    				break;
    		}
    	}
    }
#endif

ST_ErrorCode_t	STAud_PlayerMemRemap(AudioBaseAddress_t	*BaseAddress)
{

	ST_ErrorCode_t Error = ST_NO_ERROR;
	U32	AudioPeriBaseAdd = 0, AudioPeriTotalSize = 0;
	U32	AudioPeriBaseAdd1 = 0, AudioPeriTotalSize1 = 0;

	/* Update the number of users using remapped buffers */
	PlayerUserCount++;

	if(BaseAddress->IsInit == FALSE)
	{
		/* Map IO Space to kernel */
		AudioPeriBaseAdd = PCMPLAYER0_BASE;//for 7200 it will be PCMREADER_BASE and AudioPeriTotalSize = 4 * ( 4 * 1024);
		AudioPeriTotalSize = 1024* 1024;/* 4 Peripherals , 4KB mem for each */

		AudioPeriBaseAdd1 = SPDIFPLAYER_BASE;
		AudioPeriTotalSize1 = 1024* 1024;

		memset(BaseAddress, 0, sizeof(AudioBaseAddress_t));

		/* PCM0 Player */
		BaseAddress->PCMPlayer0BaseAddr = (U32)STOS_MapRegisters((void*)AudioPeriBaseAdd, AudioPeriTotalSize, "AUD_PLAYERS");
		if(!BaseAddress->PCMPlayer0BaseAddr)
		{
			STTBX_Print(("Audio STOS_MapRegisters failed.\n"));
			Error = STAUD_ERROR_IO_REMAP_FAILED;;
		}

		/* Audio config */
		/* SPDIF additional data region used for SPDIF PCM mode*/
		BaseAddress->SPDIFPlayerBaseAddr = (U32)STOS_MapRegisters((void*)AudioPeriBaseAdd1, AudioPeriTotalSize1, "AUD_PLAYERS");
		if(!BaseAddress->SPDIFPlayerBaseAddr)
		{
			STTBX_Print(("Audio STOS_MapRegisters failed.\n"));
			Error = STAUD_ERROR_IO_REMAP_FAILED;;
		}

		BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr = 0;
		BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr = (U32)STOS_MapRegisters(FDMA_BASE_ADDRESS + SPDIF_ADDITIONAL_DATA_REGION,  64, "AUD_FDMA_ADR");
		if(!BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr)
		{
			STTBX_Print(("BaseAddress.SPDIFFDMAAdditionalDataRegionBaseAddr STOS_MapRegisters failed.\n"));
			Error = STAUD_ERROR_IO_REMAP_FAILED;
		}
		BaseAddress->SystemServiceBaseAddr                 = 0;
		BaseAddress->SystemServiceBaseAddr                 = (U32)STOS_MapRegisters(SYS_SERVICES_BASE_ADDRESS, SYS_SERVICES_SIZE, "AUD_SYSSERVICE_ADR");
		if(!BaseAddress->SystemServiceBaseAddr)
		{
			STTBX_Print(("BaseAddress.SystemServiceBaseAddr STOS_MapRegisters failed.\n"));
			Error = STAUD_ERROR_IO_REMAP_FAILED;
		}
		BaseAddress->NHD2ConfigBaseAddr                    = 0;
		BaseAddress->NHD2ConfigBaseAddr                    = (U32)STOS_MapRegisters(ICMONITOR_BASE_ADDRESS , NHD2_CONFIG_SIZE, "AUD_NHD2CONFIG_ADR");
		if(!BaseAddress->NHD2ConfigBaseAddr)
		{
			STTBX_Print(("BaseAddress.NHD2ConfigBaseAddr STOS_MapRegisters failed.\n"));
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

ST_ErrorCode_t	STAud_PlayerMemUnmap(AudioBaseAddress_t	*BaseAddress)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
	PlayerUserCount--;

	if(!PlayerUserCount)
	{

	/* Reduce the number of users using remapped buffers */
		/* When all the players are terminated, release the buffers */
		if(BaseAddress->IsInit == TRUE)
		{
			STOS_UnmapRegisters((void*)BaseAddress->PCMPlayer0BaseAddr, (1024*1024));
			STOS_UnmapRegisters((void*)BaseAddress->SPDIFPlayerBaseAddr, (1024*1024));
			STOS_UnmapRegisters((void*)BaseAddress->SPDIFFDMAAdditionalDataRegionBaseAddr, 64);
			STOS_UnmapRegisters((void*)BaseAddress->SystemServiceBaseAddr, SYS_SERVICES_SIZE);
			STOS_UnmapRegisters((void*)BaseAddress->NHD2ConfigBaseAddr, NHD2_CONFIG_SIZE);
			BaseAddress->IsInit = FALSE;
		}
	}
	return Error;
}

