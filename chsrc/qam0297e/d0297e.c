/* ----------------------------------------------------------------------------
File Name: d0297e.c

Description:

    stv0297e demod driver.

Copyright (C) 1999-2006 STMicroelectronics

---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
/* C libs */
#include <string.h>
#endif
#include "sttbx.h"
#include "stcommon.h"
#include "chip.h"
#include "math.h"
/* STAPI */

/* local to sttuner */
#include "drv0297e.h"   /* misc driver functions */
#include "d0297e.h"     /* header for this file */
#include "stos.h"
#include "ch_tuner_mid.h"

/* Private types/constants ------------------------------------------------ */

/* local functions --------------------------------------------------------- */

U8 Def297eVal[STB0297E_NBREGS]=	/* Default values for STB0297E registers	*/ 
{
	/* register at @ 0x06 is put on top of the list as it defines the pll output frequency */
	/* 0x0006    */ 0x0f,/* 0x0000 RO */ 0x10,/* 0x0002    */ 0x40,/* 0x0003    */ 0x00,/* 0x0004    */ 0x01,
	/* 0x0005    */ 0x10,/* 0x0007    */ 0x40,/* 0x0008 RO */ 0x2b,/* 0x0009 RO */ 0x08,/* 0x000a    */ 0x00,
	/* 0x000b    */ 0x00,/* 0x000c RO */ 0x04,/* 0x000e    */ 0x00,/* 0x000f    */ 0x00,/* 0x0010    */ 0x77,
	/* 0x0011    */ 0x50,/* 0x0012    */ 0x00,/* 0x0013    */ 0x00,/* 0x0014    */ 0x5a,/* 0x0015    */ 0x00,
	/* 0x0016    */ 0xff,/* 0x0017    */ 0x0b,/* 0x0018    */ 0x99,/* 0x0019    */ 0x09,/* 0x001a    */ 0x68,
	/* 0x001b    */ 0x0e,/* 0x001c RO */ 0x69,/* 0x001d RO */ 0xe9,/* 0x001e RO */ 0x03,/* 0x0020    */ 0x67,
	/* 0x0021    */ 0x0e,/* 0x0022    */ 0xff,/* 0x0023    */ 0x03,/* 0x0024    */ 0x00,/* 0x0025    */ 0x89,
	/* 0x0026    */ 0x50,/* 0x0027    */ 0x55,/* 0x0028    */ 0xf6,/* 0x0029    */ 0xf0,/* 0x002a    */ 0x97,
	/* 0x002b    */ 0x20,/* 0x002c    */ 0x0d,/* 0x002d    */ 0x01,/* 0x0030    */ 0xfe,/* 0x0031    */ 0xff,
	/* 0x0032    */ 0x0f,/* 0x0033    */ 0x00,/* 0x0034    */ 0x0f,/* 0x0035    */ 0x32,/* 0x0036    */ 0x46,
	/* 0x0037    */ 0x7a,/* 0x0038    */ 0x89,/* 0x0039    */ 0x5e,/* 0x003a    */ 0x7f,/* 0x003b    */ 0x02,
	/* 0x003c    */ 0x0d,/* 0x003d    */ 0x82,/* 0x0040    */ 0x90,/* 0x0041    */ 0x66,/* 0x0042    */ 0x80,
	/* 0x0043    */ 0xba,/* 0x0044    */ 0x2f,/* 0x0045    */ 0xbf,/* 0x0046    */ 0x2d,/* 0x0047    */ 0x0d,
	/* 0x0048    */ 0x20,/* 0x0049    */ 0x16,/* 0x004a    */ 0xd2,/* 0x004b    */ 0x46,/* 0x004c    */ 0x54,
	/* 0x004d    */ 0xd8,/* 0x004e    */ 0x91,/* 0x004f    */ 0x4c,/* 0x0050    */ 0xc4,/* 0x0051    */ 0xe4,
	/* 0x0052    */ 0x48,/* 0x0053    */ 0x55,/* 0x0054    */ 0x3b,/* 0x0055    */ 0x47,/* 0x0056    */ 0x00,
	/* 0x0057    */ 0x85,/* 0x0058    */ 0x28,/* 0x0064    */ 0x20,/* 0x0070    */ 0x28,/* 0x0071    */ 0x44,
	/* 0x0072    */ 0x22,/* 0x0073    */ 0x03,/* 0x0074    */ 0x04,/* 0x0075    */ 0x11,/* 0x0076    */ 0x20,
	/* 0x0080    */ 0xb0,/* 0x0081    */ 0x08,/* 0x0082 RO */ 0x0c,/* 0x0083    */ 0x00,/* 0x0084    */ 0x00,
	/* 0x0085    */ 0x22,/* 0x0086    */ 0x00,/* 0x0087    */ 0x00,/* 0x0088    */ 0x00,/* 0x0089    */ 0x32,
	/* 0x0094 RO */ 0xa7,/* 0x0095 RO */ 0x00,/* 0x0096 RO */ 0x00,/* 0x0097    */ 0x00,/* 0x0098 RO */ 0x27,
	/* 0x0099 RO */ 0x00,/* 0x009a RO */ 0x00,/* 0x009b    */ 0x06,/* 0x009c RO */ 0x8e,/* 0x009d RO */ 0x01,
	/* 0x009e    */ 0x00,/* 0x009f    */ 0x00,/* 0x00a0 RO */ 0x9a,/* 0x00a1 RO */ 0x00,/* 0x00a2 RO */ 0xaa,
	/* 0x00a3 RO */ 0x00,/* 0x00a4    */ 0xc3,/* 0x00a5    */ 0x80,/* 0x00a6 RO */ 0xc9,/* 0x00a7 RO */ 0x02,
	/* 0x00a8    */ 0x00,/* 0x00a9    */ 0x00,/* 0x00aa    */ 0x36,/* 0x00ab    */ 0xaa,/* 0x00ac    */ 0x00,
	/* 0x00ae    */ 0x63,/* 0x00af    */ 0xdf,/* 0x00b0    */ 0x88,/* 0x00b1    */ 0x41,/* 0x00b2    */ 0xc1,
	/* 0x00b3    */ 0xa7,/* 0x00b4    */ 0x02,/* 0x00b5    */ 0x95,/* 0x00b6    */ 0xe2,/* 0x00b7    */ 0x20,
	/* 0x00b8    */ 0x00,/* 0x00b9    */ 0x00,/* 0x00ba    */ 0x00,/* 0x00bb    */ 0x00,/* 0x00bc    */ 0x40,
	/* 0x00bd    */ 0x90,/* 0x00be    */ 0x99,/* 0x00c0    */ 0x82,/* 0x00c1    */ 0x82,/* 0x00c2    */ 0x82,
	/* 0x00c3    */ 0x12,/* 0x00c4    */ 0x0d,/* 0x00c5    */ 0x82,/* 0x00c6    */ 0x82,/* 0x00c7    */ 0x11,
	/* 0x00c8    */ 0x98,/* 0x00c9    */ 0x96,/* 0x00d8    */ 0x16,/* 0x00d9    */ 0x0b,/* 0x00da    */ 0x88,
	/* 0x00db    */ 0x02,/* 0x00dc RO */ 0x12,/* 0x00de RO */ 0x0a,/* 0x00df RO */ 0xc0,/* 0x00e0 RO */ 0x00,
	/* 0x00e1 RO */ 0x00,/* 0x00e2 RO */ 0x00,/* 0x00e3 RO */ 0x00,/* 0x00e4    */ 0x01,/* 0x00e5    */ 0x06,
	/* 0x00e6 RO */ 0x00,/* 0x00e7 RO */ 0x00,/* 0x00e8    */ 0x01,/* 0x00e9    */ 0x22,/* 0x00ea    */ 0x08,
	/* 0x00ec    */ 0x01,/* 0x00ed    */ 0xc6,/* 0x00ef    */ 0x43,/* 0x00f0    */ 0x00,/* 0x00f1    */ 0x00,
	/* 0x00f2    */ 0x00,/* 0x00f3    */ 0x00,/* 0x00f4 RO */ 0x00,/* 0x00f5 RO */ 0x00,/* 0x00f6 RO */ 0x00,
	/* 0x00f7 RO */ 0x00,/* 0x00f8 RO */ 0x00,/* 0x00f9 RO */ 0x00,/* 0x00fa RO */ 0xa0,/* 0x00fb    */ 0x00,
	/* 0x00fc RO */ 0x00,/* 0x00fd RO */ 0x00,/* 0x00fe RO */ 0x00,/* 0x00ff RO */ 0x00,/* 0x006d    */ 0x20
	
};


U16 Addressarray[STB0297E_NBREGS]=
{
0x0006, 0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0008, 0x0009, 0x000A,
0x000B, 0x000C, 0x000E, 0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015,
0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x0020,
0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A,
0x002B, 0x002C, 0x002D, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036,
0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x0040, 0x0041, 0x0042,
0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C,
0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056,
0x0057, 0x0058, 0x0064, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076,
0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089,
0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D,
0x009E, 0x009F, 0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AE, 0x00AF, 0x00B0, 0x00B1, 0x00B2,
0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC,
0x00BD, 0x00BE, 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
0x00C8, 0x00C9, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DE, 0x00DF, 0x00E0,
0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA,
0x00EC, 0x00ED, 0x00EF, 0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6,
0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF, 0x006D
};


/* ----------------------------------------------------------------------------
Name: demod_d0297E_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_Init(void)
{
	ST_ErrorCode_t          Error = ST_NO_ERROR;
	
	U8 ChipID, i=0;
	U8 index;

	CH_TunerType_t  ch_tunerType;

	ch_tunerType = ch_Tuner_GetTunerType();

	/*  soft reset of the whole chip, 297e starts in standby mode by default */
	Error = ChipSetOneRegister(R297e_CTRL_0, 0x42);
	if (Error != ST_NO_ERROR)
	{
		return(Error);
	}
	
	Error = ChipSetOneRegister(R297e_CTRL_0, 0x40);
	if (Error != ST_NO_ERROR)
	{
		return(Error);
	}

	/*
	--- Get chip ID
	*/
	ChipID = ChipGetOneRegister(R297e_MIS_ID);
	if ( (ChipID & 0xF0) !=  0x10)
	{
		return(ST_ERROR_UNKNOWN_DEVICE);
	}

	for (index=0;index < STB0297E_NBREGS;index++)
	{
		ChipSetOneRegister(Addressarray[index], Def297eVal[index]);
	}
    
    /* TS output mode PARALLEL*/
     #ifndef TS_MODE_SERIAL

	ChipSetOneRegister( R297e_OUTFORMAT_0, 0x08);
	ChipSetOneRegister( R297e_FEC_AC_CTR_0, 0x1E);            
	ChipSetOneRegister( R297e_OUTFORMAT_1, 0x24);
	ChipSetOneRegister( F297e_SMOOTH_BYPASS, 0x1);
	 /*set Sync Stripping (On/Off)*/
	ChipSetField(F297e_SYNC_STRIP, 1);
	#else
	/*TS output mode Serial*/
       ChipSetOneRegister( R297e_OUTFORMAT_0, 0x0D);
	ChipSetOneRegister( R297e_FEC_AC_CTR_0, 0x1E);            
	ChipSetOneRegister( R297e_OUTFORMAT_1, 0x24);
	    /*set Sync Stripping (On/Off)*/
	ChipSetField(F297e_SYNC_STRIP, 0);
	#endif

    /*set data clock polarity mode (rising/falling)*/
	ChipSetField(F297e_CLK_POLARITY, 1);

    /*set Sync Stripping (On/Off)*/
	ChipSetField(F297e_REFRESH47, 1);

	switch(ch_tunerType)
	{
		case TUNER_PLL_THOMSON_DCT7042A:
			/* Setting to add on the eval board and the NIM */
			ChipSetField( F297e_CTR_INC_GAIN,0);
			/* These settings apply to ST NIM and provide performances which meet the Chinese specification */
			/* Crystal is 27MHz and system clock 57.375MHz */
			/* Note that in serial mode, RS parity bytes must be discarded */
			ChipSetField( F297e_PLL_BYPASS,1);
			ChipSetField( F297e_PLL_POFF,1);
			ChipSetField( F297e_PLL_NDIV,0x10);
			ChipSetField( F297e_PLL_POFF,0);
			while((ChipGetField( F297e_PLL_LOCK) == 1)&&(i<10))
			{
				STOS_TaskDelay(ST_GetClocksPerSecond()/1000 ); /*1 msec delay*/
				i ++;
			}
			/*if (ChipGetField( F297e_PLL_LOCK) == 1)*/
			ChipSetField( F297e_PLL_BYPASS,0);
			
			/* AGC settings */
			/* These settings apply to all QAM sizes but more changes on AGC thresholds
			will be done in the Algo according to the QAM and RF frequency */
			ChipSetOneRegister(R297e_GPIO_7_CFG,0x11);
			ChipSetOneRegister(R297e_AD_CFG,0x40);
			ChipSetOneRegister(R297e_AGC_PWM_CFG,0x03);
			/* PNT loop is disabled */
			ChipSetField(F297e_PNT_EN,0);
			/* MSM threshold changed to lock on lower C/N levels */
			ChipSetOneRegister( R297e_FSM_SNR2_HTH,0x23);
			/* Anti-adjacent and all-pass filters must be enabled */
			ChipSetOneRegister( R297e_IQDEM_ADJ_EN,0x0d);
			break;

		case TUNER_PLL_DCT70700:
			/* These special settings are ok for ST reference boards with DCT70700 and QAMi5107 or STB0297E */
				/* These settings provide performances which meet the Chinese specification */
				ChipSetField(F297e_CTR_INC_GAIN,1);
				/* Crystal is 27MHz and system clock 57.375MHz */
				/* Note that in serial mode, RS parity bytes must be discarded */
				ChipSetField(F297e_PLL_BYPASS,1);
				ChipSetField(F297e_PLL_POFF,1);
				ChipSetField(F297e_PLL_NDIV,0x10);
				ChipSetField(F297e_PLL_POFF,0);
				while((ChipGetField(F297e_PLL_LOCK) == 1)&&(i<10))
				{
					STOS_TaskDelay(ST_GetClocksPerSecond()/1000 ); /*1 msec delay*/
					i ++;
				}
				/*if (ChipGetField(F297e_PLL_LOCK) == 0)*/
					ChipSetField(F297e_PLL_BYPASS,0);
				/* AGC settings */
				/* The same settings apply to all QAM sizes so no more changes will be done elsewhere */
				ChipSetOneRegister(R297e_GPIO_7_CFG,0x82);
				ChipSetField(F297e_AGC_RF_TH_LO,0xff);
				ChipSetField(F297e_AGC_RF_TH_HI,0x0b);
				ChipSetField(F297e_AGC_IF_THLO_LO,0x99);
				ChipSetField(F297e_AGC_IF_THLO_HI,0x09);
				ChipSetField(F297e_AGC_IF_THHI_LO,0xff);
				ChipSetField(F297e_AGC_IF_THHI_HI,0x07);
				ChipSetOneRegister(R297e_AD_CFG,0x40);
				ChipSetOneRegister(R297e_AGC_PWM_CFG,0x00);
				/* PNT loop is disabled */
				ChipSetField(F297e_PNT_EN,0);
				/* MSM threshold changed to lock on lower C/N levels */
				ChipSetOneRegister( R297e_FSM_SNR2_HTH,0x23);
				/* Anti-adjacent and all-pass filters are disabled */
				ChipSetOneRegister( R297e_IQDEM_ADJ_EN,0x0d);
			break;
/*20080221 add for PHILIPS TUNER*/			
		case TUNER_PLL_CD11XX:
		case TUNER_PLL_CD1616LF:
            /* These settings provide performances which meet the Chinese specification */
		ChipSetField(F297e_CTR_INC_GAIN,1);
		/* Crystal is 27MHz and system clock 57.375MHz */
		/* Note that in serial mode, RS parity bytes must be discarded */
		ChipSetField(F297e_PLL_BYPASS,1);
		ChipSetField(F297e_PLL_POFF,1);
		ChipSetField(F297e_PLL_NDIV,0x10);
		ChipSetField(F297e_PLL_POFF,0);
		while((ChipGetField(F297e_PLL_LOCK) == 1)&&(i<10))
		{
			STOS_TaskDelay(ST_GetClocksPerSecond()/1000 ); /*1 msec delay*/
			i ++;
		}
		/*if (ChipGetField(Instance->IOHandle,F297e_PLL_LOCK) == 0)*/
			ChipSetField(F297e_PLL_BYPASS,0);
		/* AGC settings */
		/* These settings apply to all QAM sizes but more changes on AGC thresholds
		will be done in the Algo according to the QAM and RF frequency */
		ChipSetOneRegister(R297e_GPIO_7_CFG,0x90);
		ChipSetOneRegister(R297e_AD_CFG,0x40);
		ChipSetOneRegister(R297e_AGC_PWM_CFG,0x00);
		/* PNT loop is disabled */
		ChipSetField(F297e_PNT_EN,0);
		/* MSM threshold changed to lock on lower C/N levels */
		ChipSetOneRegister( R297e_FSM_SNR2_HTH,0x23);
		/* Anti-adjacent and all-pass filters must be enabled */
		ChipSetOneRegister( R297e_IQDEM_ADJ_EN,0x0d);
	      break;
/*****************************/			
		default:
			break;
	}
	
	
    
  	
    /* Set default error count mode, bit error rate */
	ChipSetField( F297e_CT_HOLD, 1); /* holds the counters from being updated */

	return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0297E_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_GetSignalQuality(U32  *SignalQuality_p, U32 *rpui_Strength, U32 *Ber_p, U32 *rpui_SNRdb)
{
	ST_ErrorCode_t              Error = ST_NO_ERROR;
	FE_297e_SignalInfo_t        pInfo;
	BOOL                        IsLocked;
	*SignalQuality_p = 0;
	*Ber_p = 0;

	pInfo.BER = 0;
	pInfo.CN_dBx10 = 0;

	semaphore_wait(pg_semSignalAccess);

	/*
	--- Get Carrier And Data (TS) status
	*/
	Error = demod_d0297E_IsLocked(&IsLocked);
	if ( Error == ST_NO_ERROR && IsLocked )
	{
		/*
		--- Read Blk Counter
		*/
		pInfo.SymbolRate = FE_297e_GetSymbolRate(FE_297e_GetMclkFreq(27000000) );
		ChipSetField(F297e_CT_HOLD, 1); /* holds the counters from being updated */
		Error = FE_297e_GetSignalInfo(&pInfo);
		
		
		*Ber_p = pInfo.BER;
		*rpui_SNRdb = pInfo.SNR;

		if(pInfo.CN_dBx10 < 0)
			{
                        pInfo.CN_dBx10 = (-1)*pInfo.CN_dBx10;
			}
		if(pInfo.Power_dBmx10 < 0)
			{
                        pInfo.Power_dBmx10 = (-1)*pInfo.Power_dBmx10;
			}
		*SignalQuality_p =pInfo.CN_dBx10;
		*rpui_Strength = pInfo.Power_dBmx10;
		ChipSetField(F297e_CT_HOLD, 0);
		ChipSetField(F297e_CT_CLEAR, 0);
		ChipSetField(F297e_CT_CLEAR, 1);
	}

	semaphore_signal(pg_semSignalAccess);

	return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_SetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_SetModulation(FE_297e_Modulation_t Modulation)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
		

	Error = ChipSetField(F297e_QAM_MODE, Modulation);

	return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_GetAGC(S16  *Agc)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;

	*Agc = FE_297e_GetRFLevel();

	return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_IsLocked(BOOL *IsLocked)
{
	ST_ErrorCode_t          Error = ST_NO_ERROR;

	*IsLocked = ChipGetField( F297e_QAMFEC_LOCK) ? TRUE : FALSE;

	return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_ScanFrequency(U32 rui_FrequencyKHz, U32 rui_SymbolRateBds, U32 rui_QamMode, U32 rui_Spectrum)
                                                             
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
	FE_297e_SearchParams_t Params;
	FE_297e_SearchResult_t Result;


	Params.ExternalClock = 27000000;
	Params.Frequency_Khz   = rui_FrequencyKHz;              /* demodulator (output from LNB) frequency (in KHz) */
	Params.SymbolRate_Bds  = rui_SymbolRateBds;                    /* transponder symbol rate  (in bds) */
	Params.SearchRange_Hz = 280000;  /*nominal 280Khz*/
	Params.SweepRate_Hz = 0;
	/*20080327 add for A1 version*/
	 ChipSetOneRegister(R297e_CTRL_2,0x00);
	/*************************/
	/*
	--- Modulation type is set to
	*/
	switch(rui_QamMode)
	{
		case 0:
			Params.Modulation = FE_297e_MOD_QAM4;
			break;
		
		case 1:
		case 16:
			Params.Modulation = FE_297e_MOD_QAM16;
			break;
		
		case 2:
		case 32:
			Params.Modulation = FE_297e_MOD_QAM32;
			break;	
		
		case 3:
		case 64:
			Params.Modulation = FE_297e_MOD_QAM64;
			break;	
		
		case 4:
		case 128:
			Params.Modulation = FE_297e_MOD_QAM128;
			break;	
		
		case 5:
		case 256:
			Params.Modulation = FE_297e_MOD_QAM256;
			break;
		
		default:
			Params.Modulation = FE_297e_MOD_QAM64;
			break;
	}
	Error = demod_d0297E_SetModulation(Params.Modulation); /*fills in Instance->FE_297e_Modulation*/

	/*
	--- Spectrum inversion is set to
	*/
	switch(rui_Spectrum)
	{			
		default:
			break;
	}

	Error |= FE_297e_Search(&Params, &Result);

	return(Error);
}


/* End of d0297E.c */

