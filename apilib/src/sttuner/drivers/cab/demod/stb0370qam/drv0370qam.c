/*----------------------------------------------------------------------------
File Name   : drv0370QAM.c 

Description : STB0370QAM front-end driver routines.

Copyright (C) 2005-2006 STMicroelectronics

   date: 
version: 
 author:
comment: 

Revision History:

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
----------------------------------------------------------------------------*/


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
/* C libs */
#include <string.h>

#include <stdlib.h> /* for abs() */

#endif
/* STAPI common includes */
#include "stlite.h"                     /* Standard includes */
#include "sttbx.h"
#include "stddefs.h"

/* STAPI */
#include "sttuner.h"
#include "sti2c.h"
#include "stcommon.h"

#include "stevt.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#include "d0370qam.h"      /* top level header for this driver */
#include "reg0370qam.h"    /* register mappings for the stv0370QAM */


#include "drv0370qam.h"    /* header for this file */


/* defines ----------------------------------------------------------------- */

#define STB0370QAM_FAST_SWEEP_SPEED            750
#define STB0370QAM_SLOW_SWEEP_SPEED            400

#define STB0370QAM_SYMBOLRATE_LEVEL            3000000 /* When SR < STV0297J_SYMBOLRATE_LEVEL, Sweep = Sweep/2 */

extern U32 STB0370_DefVal_256QAM_TD1336[STB0370_QAM_NBREGS];
extern U32 STB0370_DefVal_256QAM_DCT7050[STB0370_QAM_NBREGS];
/* private types ----------------------------------------------------------- */


#define STTUNER_TaskDelayUS(milisec)     STOS_TaskDelayUs(1000*milisec)


/* constants --------------------------------------------------------------- */

/* Current LLA revision */

static ST_Revision_t Revision370QAM  = " STB0370QAM-LLA_REL_01.0 ";

U32 POW_370QAM(U32 Number, U32 Power)
{
	int i;
	long result = 1;
	
	for(i=0;i<Power;i++)
		result *= Number;
		
	return (U32)result;
}

const int FE_370QAM_CN_LookUp[80]= {
											 
						/*64QAM*/	
						31778,30778,29778,28778,27778,26778,25778,24778,23778,22778,
						21778,20778,19778,18778,17778,16778,15778,14778,13778,12778,
						11778,10674,9552,8586,7745,6979,6331,5577,5053,4578,
						4184,3773,3461,3188,2942,2741,2559,2405,2309,2176,
					
					 	/*256QAM*/
						37874,36874,35874,34874,33874,32874,31874,30874,29874,28874,
						27874,26874,25874,24874,23874,22874,21874,20874,19874,18874,
						17874,16874,15874,14874,13874,12874,11874,10874,9866,8950,
						8135,7290,6677,6076,5567,5111,4748,4421,4166,3897
				};
				
				
				

D0370QAM_SignalStatus_t Driv0370QAMSignalStatus; 

/* functions --------------------------------------------------------------- */





/***********************************************************
**FUNCTION	::	Drv0370QAM_GetLLARevision
**ACTION	::	Returns the 370QAM LLA driver revision
**RETURN	::	Revision370QAM
***********************************************************/
ST_Revision_t Drv0370QAM_GetLLARevision(void)
{
	return (Revision370QAM);
}


/*----------------------------------------------------
--FUNCTION      Drv0370QAM_CarrierWidth
--ACTION        Compute the width of the carrier
--PARAMS IN     SymbolRate -> Symbol rate of the carrier (Kbauds or Mbauds)
--              RollOff    -> Rolloff * 100
--PARAMS OUT    NONE
--RETURN        Width of the carrier (KHz or MHz)
------------------------------------------------------*/

long Drv0370QAM_CarrierWidth(long SymbolRate, long RollOff)
{
    return (SymbolRate  + (SymbolRate * RollOff)/100);
}

/*----------------------------------------------------
--FUNCTION      Drv0370QAM_CheckAgc
--ACTION        Check for Agc
--PARAMS IN     Params        => Pointer to SEARCHPARAMS structure
--PARAMS OUT    Params->State => Result of the check
--RETURN        E370QAM_NOAGC carrier not founded, E370QAM_AGCOK otherwise
------------------------------------------------------*/
D0370QAM_SignalType_t Drv0370QAM_CheckAgc(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0370QAM_SearchParams_t *Params)
{

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
   const char *identity = "STTUNER drv0370QAM.c Drv0370QAM_CheckAgc()";
#endif


    if ( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0370QAM_WAGC_ACQ) )
    {
        Params->State = E370QAM_AGCOK;
    }
    else
    {
        Params->State = E370QAM_NOAGC;
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    STTBX_Print(("%s\n", identity));
#endif

    return(Params->State);
}

/*----------------------------------------------------
 FUNCTION      Drv0370QAM_CheckData
 ACTION        Check for data founded
 PARAMS IN     Params        =>    Pointer to SEARCHPARAMS structure
 PARAMS OUT    Params->State    => Result of the check
 RETURN        E370QAM_NODATA data not founded, E370QAM_DATAOK otherwise
------------------------------------------------------*/

D0370QAM_SignalType_t Drv0370QAM_CheckData(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0370QAM_SearchParams_t *Params)
{

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
   const char *identity = "STTUNER drv0370qam.c Drv0370QAM_CheckData()";
#endif


    if ( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0370QAM_MPEG_LOCK) ) /*Check the actual register use (MPEG_SYNC)??? (MPEG_LOCK--->)It is inverted*/
    {
        Params->State = E370QAM_NODATA;
    }
    else
    {
        Params->State = E370QAM_DATAOK;
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    STTBX_Print(("%s\n", identity));
#endif

    return(Params->State);
}

/*----------------------------------------------------
 FUNCTION      Drv0370QAM_InitSearch
 ACTION        Set Params fields that are used by the search algorithm
 PARAMS IN
 PARAMS OUT
 RETURN        NONE
------------------------------------------------------*/
void Drv0370QAM_InitSearch(STTUNER_tuner_instance_t *TunerInstance, D0370QAM_StateBlock_t *StateBlock,
                        STTUNER_Modulation_t Modulation, int Frequency, int SymbolRate, STTUNER_Spectrum_t Spectrum,
                        BOOL ScanExact, STTUNER_J83_t J83)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
   const char *identity = "STTUNER drv0370QAM.c Drv0370QAM_InitSearch()";
#endif
    U32 BandWidth;
    ST_ErrorCode_t Error;
    TUNER_Status_t TunerStatus;

    /* Obtain current tuner status */
    Error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);

    /* Select closest bandwidth for tuner */ /* cast to U32 type to match function argument & eliminate compiler warning --SFS */
    Error = (TunerInstance->Driver->tuner_SetBandWidth)( TunerInstance->DrvHandle,
                                                        (U32)( Drv0370QAM_CarrierWidth(SymbolRate, StateBlock->Params.RollOff) /1000 + 3000),
                                                        &BandWidth);

    /*
    --- Set Parameters
    */
    StateBlock->Params.Frequency    = Frequency;
    StateBlock->Params.SymbolRate   = SymbolRate;
    StateBlock->Params.TunerBW      = (long)BandWidth;
    StateBlock->Params.TunerStep    = (long)TunerStatus.TunerStep;
    StateBlock->Params.TunerIF      = (long)TunerStatus.IntermediateFrequency;
    StateBlock->Result.SignalType   = E370QAM_NOAGC;
    StateBlock->Result.Frequency    = 0;
    StateBlock->Result.SymbolRate   = 0;
    StateBlock->SpectrumInversion   = Spectrum;
    StateBlock->Params.Direction    = E370QAM_SCANUP;
    StateBlock->Params.State        = E370QAM_NOAGC;
    StateBlock->J83                 = STTUNER_J83_B;
    StateBlock->Quartz		    = 27000000; /* 54 (27*2Mhz) MHz quartz check this????*/ 


    if (ScanExact)
    {
        StateBlock->ScanMode = DEF0370QAM_CHANNEL;   /* Subranges are scanned in a zig-zag mode */
    }
    else
    {
        StateBlock->ScanMode = DEF0370QAM_SCAN;      /* No Subranges */
    }

    /*
    --- Set QAMSize and SweepRate.
    --- SweepRate = 1000* (SweepRate (in MHz/s) / Symbol Rate (in MBaud/s))
    */
    switch(Modulation)
    {
        case STTUNER_MOD_16QAM :
        case STTUNER_MOD_32QAM :
        case STTUNER_MOD_64QAM :
            StateBlock->QAMSize     = Modulation;           /* Set by user */
            StateBlock->SweepRate   = STB0370QAM_FAST_SWEEP_SPEED;
            break;
	case STTUNER_MOD_128QAM :
        case STTUNER_MOD_256QAM :
            StateBlock->QAMSize     = Modulation;           /* Set by user */
            StateBlock->SweepRate   = STB0370QAM_SLOW_SWEEP_SPEED;
            break;

        case STTUNER_MOD_QAM :
        default:
            StateBlock->QAMSize     = STTUNER_MOD_64QAM;    /* Most Common Modulation for Scan */
            StateBlock->SweepRate   = STB0370QAM_FAST_SWEEP_SPEED;
            break;
    }

    /*
    --- For low SR, we MUST divide Sweep by 2
    */
    if (SymbolRate <= STB0370QAM_SYMBOLRATE_LEVEL)
    {
        StateBlock->SweepRate /= 2;
    }

    /*
    --- CO
    */
    StateBlock->CarrierOffset = DEF0370QAM_CARRIEROFFSET_RATIO * 100;              /* in % */

    /*
    --- Set direction
    */
    if ( StateBlock->Params.Direction == E370QAM_SCANUP )
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
        STTBX_Print(("%s ==> SCANUP\n", identity));
#endif
        StateBlock->CarrierOffset *= -1;
        StateBlock->SweepRate     *= 1;
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
        STTBX_Print(("%s ==> SCANDOWN\n", identity));
#endif
        StateBlock->CarrierOffset *= 1;
        StateBlock->SweepRate     *= -1;
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    STTBX_Print(("%s\n", identity));
#endif
}


/*----------------------------------------------------
 FUNCTION      Driv0370QAMDemodSetting
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Driv0370QAMDemodSetting(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
                          long Offset,int ExtClk,U32	TunerIF)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
   const char *identity = "STTUNER drv0370qam.c Driv0370QAMDemodSetting()";
#endif
        long long_tmp;	
	int int_tmp;
	
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    STTBX_Print(("%s Offset %d\n", identity, Offset));
#endif

	ExtClk/=1000;
	
	long_tmp  = TunerIF + Offset - ExtClk	; /* in kHz */
	long_tmp *= 512 ;
	long_tmp *= 128 ;
	long_tmp /= ExtClk;
	if(long_tmp > 65535) long_tmp = 65535;
	int_tmp = (int)long_tmp;	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_DEM_FREQ_MSB,int_tmp>>8);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_DEM_FREQ_LSB,int_tmp);	
}


/*----------------------------------------------------
 FUNCTION      Driv0370QAMCNEstimator
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Driv0370QAMCNEstimator(DEMOD_Handle_t Handle,STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
                           int *Mean_p, int *CN_dB100_p, int *CN_Ratio_p)
{

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
   const char *identity = "STTUNER drv0370qam.c Driv0370QAMCNEstimator()";
#endif

      	int int_tmp, i;
	int idB	;
	int iQAM, QAMSize;
	int CurrentMean	;
	int ComputedMean;
	int OldMean;
	int Result;
	FLAG_370QAM EndOfSearch;
        D0370QAM_InstanceData_t        *Instance;            
		
	/* private driver instance data */
       Instance = D0370QAM_GetInstFromHandle(Handle);
       QAMSize = Reg0370QAM_GetQAMSize(DeviceMap,IOHandle);
	
	switch (QAMSize)
	{
		case 64:
			iQAM = 0;
		break;
		case 256:
			iQAM = 1;
		break;
		default:
			iQAM = 0;
		break;
		
	}

	/* the STB0370QAM noise estimation must be filtered. The used filter is :*/
	/* Estimation(n) = 63/64*Estimation(n-1) + 1/64*STV0297JNoiseEstimation */
	for (i = 0; i < 100; i ++)
	{
		/*WAIT_N_MS(1);*/ /* 1 ms */
		int_tmp =  (STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_NOISE_EST_MSB)<<8);  
		int_tmp += STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_NOISE_EST_LSB);
		Instance->Driv0370QAMCNEstimation = (63*Instance->Driv0370QAMCNEstimation)/64 + int_tmp/64;
	}

	ComputedMean	= Instance->Driv0370QAMCNEstimation - Instance->Driv0370QAMCNEstimatorOffset; /* offset correction if any */
	CurrentMean		= FE_370QAM_CN_LookUp[iQAM*40] ;
	idB = 1;
	EndOfSearch = NO;
	 	 
	
	while (EndOfSearch == NO)
	{
		OldMean = CurrentMean;
		CurrentMean = FE_370QAM_CN_LookUp[iQAM*40+idB];
		if ((CurrentMean <= ComputedMean)||(idB >= 39))
			EndOfSearch = YES;
		else
			idB += 1;
	}

	/* An interpolation is performed in order to increase the result accuracy */
	/* Typically, 0.3 dB accuracy is reached. */
	Result = 100*idB;
	if (CurrentMean < OldMean)
	{
		Result -= (100*(CurrentMean - ComputedMean))/(CurrentMean - OldMean);
	}

	*Mean_p	= Instance->Driv0370QAMCNEstimation;
	*CN_Ratio_p = Result;
	*CN_dB100_p = Result;

	


#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    STTBX_Print(("%s Mean %d CNdBx100 %d CNRatio %d\n", identity, *Mean_p, *CN_dB100_p, *CN_Ratio_p));
#endif
     

}


/*----------------------------------------------------
 FUNCTION      Drv0370QAMApplicationSaturationComputation
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Drv0370QAMApplicationSaturationComputation(int * _pSaturation)
{
    int int_tmp ;
    /*
    In order to get a smooth saturation value, this is filtered
    with a low-pass filter : sat(n) = 0.25*sat(n-1) + 0.75*acc(n)
    pSaturation[0] = accumulator value
    pSaturation[1] = low-pass filter memory
    */
    if (_pSaturation[0] != 0)
    {
        int_tmp  = 100*_pSaturation[0];
        int_tmp /= 65536;
        if(_pSaturation[1] > int_tmp)
        {
            int_tmp  = 10*int_tmp/100;
            int_tmp += 90*_pSaturation[1]/100;
        }
        else
        {
            int_tmp  = 75*int_tmp/100;
            int_tmp += 25*_pSaturation[1]/100;
        }

        if(int_tmp > 100) int_tmp = 100;

        _pSaturation[1] = int_tmp ;
    }
    return ;
}

/*----------------------------------------------------
 FUNCTION      Driv0370QAMBertCount
 ACTION
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Driv0370QAMBertCount(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
                       D0370QAM_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p,
                       int *_pBER_Cnt, int *_pBER)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    const char *identity = "STTUNER drv0370qam.c Driv0370QAMBertCount()";
#endif
    int     err_hi1, err_lo1,err_hi0,err_lo0; 
    int     int_tmp;
    U32     BER;
     int	double_tmp ;
   int	NumberBitsPerFrame;
    /*
    --- Set parameters
    */
    _pBER[0] = 0;


    /*
    --- Compute ...
    ---
    ---     BER =   10E6 * (Counter)/(2^(2*NBYTE + 12))
    ---         = 244,14 * (Counter)/(2^(2*NBYTE))                   (Unit 10E-6)
    */
    
   
        /* rate mode */
      
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
            STTBX_Print(("===> Count is finished\n"));
#endif
            /* the previous count is finished */
            /* Get BER from demod */
            err_hi0 = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0370QAM_RS_RATE_LSB);
            err_lo0 = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0370QAM_RS_RATE_MSB);
            err_hi1 = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0370QAM_RS_RATE_LSB);
            err_lo1 = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0370QAM_RS_RATE_MSB);
            if(err_hi0 == err_hi1)
            {
            *_pBER_Cnt = err_lo0 + (err_hi0 <<8);
            }
            else
            {
             *_pBER_Cnt = err_lo1 + (err_hi1 <<8);
            }
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
            STTBX_Print(("===> _pBER_Cnt = %d\n", *_pBER_Cnt));
#endif
            BER = *_pBER_Cnt;

            double_tmp = (int)*_pBER_Cnt;
	    int_tmp =  STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_RS_RATE_ADJ);
	    NumberBitsPerFrame = 78888; 
	    if (STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_F_64_256) == 1)
	    {	
	    	NumberBitsPerFrame = 53802;
	    }
	    double_tmp /= (4*NumberBitsPerFrame);
	    double_tmp *= 7;
	    double_tmp /= (int)(POW_370QAM(4,int_tmp));	   
           
            _pBER[0] = *_pBER_Cnt;
          
          Drv0370QAMApplicationSaturationComputation(_pBER); 
    /*
    --- Get Blk parameters
    */
    StateBlock_p->BlkCounter = Reg0370QAM_GetBlkCounter(DeviceMap, IOHandle);
    StateBlock_p->CorrBlk    = Reg0370QAM_GetCorrBlk(DeviceMap, IOHandle);
    StateBlock_p->UncorrBlk  = Reg0370QAM_GetUncorrBlk(DeviceMap, IOHandle);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    STTBX_Print(("%s BER_Cnt %9d BER Cnt %9d Sat %3d Rate %9d (10E-6)\n", identity,
                *_pBER_Cnt,
                _pBER[0],
                _pBER[1],
                _pBER[2]
                ));
    STTBX_Print(("%s BlkCounter %9d CorrBlk %9d UncorrBlk %9d\n", identity,
                StateBlock_p->BlkCounter,
                StateBlock_p->CorrBlk,
                StateBlock_p->UncorrBlk
                ));
#endif

}


/*----------------------------------------------------
 FUNCTION      Driv0370QAMFECLockCheck
 ACTION	       
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/

FLAG_370QAM Driv0370QAMFECLockCheck (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_370QAM_InternalParams_t *pParams, FLAG_370QAM EndOfSearch, int TimeOut, int DataSearchTime)
{
	int QAMSize, MPEGLockCount;
	int noCodeWordsInAframe = 60, Log2QAMSize=0;
	int FrameSyncTimeOut, ViterbiTimeOut, MPEGTimeOut;
	long SymbolRate;
	FLAG_370QAM DataFound = NO;
		
	QAMSize     = pParams->Modulation	;
	SymbolRate  = pParams->SymbolRate	; /* in Baud/s */
	switch (QAMSize)
	{
		case 16:
			Log2QAMSize = 4;
			break;
		case 32 :
			Log2QAMSize = 5;
			break ;
		case 64 :
			Log2QAMSize = 6;
			noCodeWordsInAframe = 60;
			break;
		case 128 :
			Log2QAMSize = 7;
			break ;
		case 256 :
			Log2QAMSize = 8;
			noCodeWordsInAframe = 88;
			break;
	}

		ViterbiTimeOut = 20;
		MPEGTimeOut = 200;
		/* frameSyncTimeOut (in mSec) : it is the time necessary for an MPEG frame lock in the worst case. */
		FrameSyncTimeOut =((int) (noCodeWordsInAframe*128*7)/(int)(Log2QAMSize*(SymbolRate/1000))); /* in ms */
		/* length of a code word is 128 words; each word is of 7 bits 
		 Waiting for MPEG sync Lock or a time-out; A few  successful 
		tracking indicators are required in succession to decide MPEG Sync */
		EndOfSearch = NO;
		MPEGTimeOut = MPEGTimeOut + ViterbiTimeOut + FrameSyncTimeOut;
		TimeOut = DataSearchTime + MPEGTimeOut;
		MPEGLockCount = 0;
		while (EndOfSearch == NO)
			{
				STTUNER_TaskDelayUS(5);
				DataSearchTime += 5;
				if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_MPEG_SYNC) == 1)	MPEGLockCount++;
				else	MPEGLockCount = 0;
				if((MPEGLockCount == 1)||(DataSearchTime > TimeOut))
					EndOfSearch = YES;
			}
			if(MPEGLockCount == 1)
				DataFound = YES;
			
			else
			{
				
				STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_INV_SPEC_MAPPING,!STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_INV_SPEC_MAPPING)); /*Check this syntax???? */
				STTUNER_TaskDelayUS(20);
				DataSearchTime += 20 ;
				if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_MPEG_SYNC) == 1)
					DataFound = YES;
			}		

			if(STTUNER_IOREG_GetRegister(DeviceMap, IOHandle,R0370QAM_SYNC_STAT) != 0x1e)
			{
				
				DataFound = NO;			
			
					if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_COMB_STATE) != 2)
					{
						DataFound = NO;
					   
							if( (STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_VIT_I_SYNC) == 0) || (STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_VIT_Q_SYNC) == 0))
							{								
								DataFound = NO;
					
							}
					}
				return (DataFound);
			}
		DataFound = YES;

	return DataFound;
}



/*----------------------------------------------------
 FUNCTION      Driv0370QAMFecInit
 ACTION	       
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t Driv0370QAMFecInit (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle) /*FECType is the value of FEC_AC_OR_B*/
{
	ST_ErrorCode_t  Error = ST_NO_ERROR;

		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_FEC_NRESET,0)		;	/* FEC B reset */
		
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_FEC_NRESET,1)		;	/* FEC B reset cleared */
		
		
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_NRST,0)		;	/* FEC B viterbi reset */
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_RS_EN,0)				;	/* FEC B RS correction disabled */
	
	
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_NRST,1)		;	/* FEC B viterbi reset cleared */
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_RS_EN,1)				;	/* FEC B RS correction enabled */
	
	return Error;
}


/*----------------------------------------------------
 FUNCTION      LoadRegisters
 ACTION
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/

ST_ErrorCode_t LoadRegisters(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int QAMSize,int TunerType)
{	
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    const char *identity = "STTUNER drv0370qam.c LoadRegisters()";
#endif	
	ST_ErrorCode_t  Error = ST_NO_ERROR;	
	
	switch (QAMSize)
	{
		case  64:
		switch(TunerType)
		{
			case STTUNER_TUNER_TD1336:
			 /*QAMDefaultValue = STB0370_DefVal_64QAM_TD1336 ;*/
			 /*Set the corresponding registers which are required to reprogram for 
			   lock for 64 QAM from the default value of 256 QAM*/
			 STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0370QAM_MODE_SEL,0x04);
			 STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,R0370QAM_FREQ_6,0x80); 
			 STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_F_64_256,1);  
			   
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    STTBX_Print(("%s Default Value of Tuner TD1336, QAM-64 used\n", identity));
#endif
			break;
			case STTUNER_TUNER_DCT7050:
			case STTUNER_TUNER_DTT7600:
			/* QAMDefaultValue =STB0370_DefVal_64QAM_DCT7050;*/
			  /*Set the corresponding registers which are required to reprogram for 
			   lock for 64 QAM from the default value of 256 QAM*/
			 STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0370QAM_MODE_SEL,0x04);
			 STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,R0370QAM_FREQ_6,0x80); 
			 STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_F_64_256,1); 
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    STTBX_Print(("%s Default Value of Tuner DCT7050, QAM-64 used\n", identity));
#endif			 
			break;
			default:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s WARNING - Any Supported  Tuner is not  specified\n", identity));
#endif	
			break;
			 
		}
		break;
		case 256:
		  switch(TunerType)
		{
			case STTUNER_TUNER_TD1336:
			 /*QAMDefaultValue = STB0370_DefVal_256QAM_TD1336 ;*/
			 /*Set the corresponding registers which are required to reprogram to 
			   lock for 256 QAM from the default value of 256 QAM and after 
			   applying modifications for 64 QAM*/
			 STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0370QAM_MODE_SEL,0x03);
			 STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,R0370QAM_FREQ_6,0x81); 
			 STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_F_64_256,0);  
			break;
			case STTUNER_TUNER_DCT7050:
			case STTUNER_TUNER_DTT7600:
			 /*QAMDefaultValue =STB0370_DefVal_256QAM_DCT7050;*/
			 /*Set the corresponding registers which are required to reprogram to 
			   lock for 256 QAM from the default value of 256 QAM and after 
			   applying modifications for 64 QAM*/
			 STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0370QAM_MODE_SEL,0x03);
			 STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,R0370QAM_FREQ_6,0x81); 
			 STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_F_64_256,0);  
			break;
			default:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s WARNING - Any Supported  Tuner is not  specified\n", identity));
#endif	
			break;
			 
		}
		break;
		default:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s WARNING - Any Supported  QAM(64/256) is not  specified\n", identity));
#endif			
		break;
	}
   
	return Error;
}

/*----------------------------------------------------
 FUNCTION      Driv0370QAMDataSearch
 ACTION
                This routine performs a carrier lock trial with a given offset and sweep rate.
                The returned value is a flag (TRUE, FALSE) which indicates if the trial is
                successful or not. In case of lock, _pSignal->Frequency and _pSignal->SymbolRate
                are modifyed accordingly with the found carrier parameters.
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
BOOL Driv0370QAMDataSearch(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
                        FE_370QAM_InternalParams_t *pParams, STTUNER_tuner_instance_t *TunerInstance_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
    const char *identity = "STTUNER drv0370QAM.c Driv0370QAMDataSearch()";
    clock_t                 time_spend_register;
    clock_t                 time_start_lock, time_end_lock;
#endif
 
   int	TimeOut, LMS2TimeOut, AcqLoopsTime, DataSearchTime;
   int	NbSymbols, Log2QAMSize, LMS2TrackingLock;
   int	QAMSize, SweepRate, CarrierOffset;
   long	SymbolRate;
   long	Frequency;
   FLAG_370QAM	SpectrumInversion;
   int	AGC2SD, int_tmp, WBAGCLock, AGCLoop, ACQThreshold;
   int	Freq19, Freq23, Freq24;
   FLAG_370QAM	EndOfSearch, DataFound;
  
   int multiply_factor_times_quartz;
   int Clk50_QAM;	


	/**/
	/******************************/
	/*	Variables Initialization	*/
	/******************************/
	DataFound			= NO;
	EndOfSearch			= NO;
	DataSearchTime		= 0;
	Frequency			= pParams->Frequency;
	QAMSize				= pParams->Modulation;
	SymbolRate			= pParams->SymbolRate; /* in Baud/s */
	SweepRate			= pParams->SweepRate; /* in 1/ms */
	CarrierOffset		= pParams->CarrierOffset;
	SpectrumInversion	= pParams->SpecInv;
	/**/
	/******************************/
	/*	Timouts Computation			*/
	/******************************/
	/*	AcqLoopsTime (In ms): It Is The Time Required For The Lock
		Of The Carrier And Timing Loops */
	switch (QAMSize)
	{
		case 16:
			Log2QAMSize = 4;
			break;
		case 32:
			Log2QAMSize = 5;
			break;
		case 64:
			Log2QAMSize = 6;
			break;
		case 128:
			Log2QAMSize = 7;
			break;
		case 256:
			Log2QAMSize = 8;
			break;
	}
	NbSymbols = 100000;	/* 100000 Symbols Needed At Least To Lock The STL */
	AcqLoopsTime = (int) (NbSymbols/(SymbolRate/1000));	/* In ms */

	/*	SweepRate = 1000*(Sweep Rate/Symbol Rate)
		CarrierOffset = 10000 * (Offset/Symbol Rate)
		Then:
		LMS2TimeOut = 100 * (CarrierOffset/SweepRate)
		In Order To Scan [-Offset, +Offset], We Must Double LMS2TimeOut
		And To Be Safer, 25% Margin Is Added */
	if(SweepRate != 0)	/* TimeOut When Sweep Function Is Used */
	{
		LMS2TimeOut  = 250*CarrierOffset;
		LMS2TimeOut /= SweepRate;	/* In ms */
		if (LMS2TimeOut < 0)	LMS2TimeOut = - LMS2TimeOut;
		/*	Even In Low Carrier Offset Cases, LMS2TimeOut Must Not Be Lower Than A Minimum
			Value Which Is Linked To The QAM Size. */
		switch (QAMSize)
		{
			case 16:
				if( LMS2TimeOut < 50)
					LMS2TimeOut = 100;
				break;
			case 32:
				if( LMS2TimeOut < 50)
					LMS2TimeOut = 150;
				break;
			case 64:
				if( LMS2TimeOut < 50)
					LMS2TimeOut = 100;
				break;
			case 128:
				if( LMS2TimeOut < 100)
					LMS2TimeOut = 200;
				break;
			case 256:
				if( LMS2TimeOut < 100)
					LMS2TimeOut = 150;
				break;
		}
	}
	else						/* TimeOut When Estimator Is Used */
		LMS2TimeOut = 200;	/* Actual TimeOuts Will Be Used In Following Releases */

	/*	The Equalizer Timeout Must Be Greater Than The Carrier And Timing Lock Time */
	LMS2TimeOut += AcqLoopsTime;
	
	/******************************/
	/*	Chip Initialization			*/
	/******************************/
	
	LoadRegisters(DeviceMap,IOHandle,QAMSize,TunerInstance_p->Driver->ID);

	
	/******************************/
	/*	Derotator Initialization	*/
	/******************************/
	
	Clk50_QAM = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_CLK50_QAM);
	multiply_factor_times_quartz =((-2*Clk50_QAM* pParams->Quartz)/3)+((8*pParams->Quartz)/3);
	
	/* The Initial Demodulator Frequency Is Set If enabled */
	if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_IN_DEMOD_EN) ==1)
		Driv0370QAMDemodSetting(DeviceMap,IOHandle,pParams->InitDemodOffset,multiply_factor_times_quartz,pParams->IF);
	/**/
	/******************************/
	/*	Delayed AGC Initialization	*/
	/******************************/
	/* Wide Band AGC Loop Freeze, Wide Band AGC Clear And
	 Wide Band AGC Status Forced To Unlock Position */
	
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,R0370QAM_WBAGC_3,0x40);
	/* Wide Band AGC AGC2SD Initialisation To 25% Range */
	AGC2SD = 256;
	Reg0370QAM_SetAGC(DeviceMap, IOHandle,AGC2SD);
	/* AGC RollOff Set To 384 And Acquisition Threshold To 512
	 In Order To Speed-up The WBAGC Lock. Old values are saved*/
	AGCLoop = Reg0370QAM_GetWBAGCloop(DeviceMap,IOHandle);
	ACQThreshold = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_ACQ_TH);
	Reg0370QAM_SetWBAGCloop(DeviceMap,IOHandle,384);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_ACQ_TH,9);
	/**/
	

	/******************************/
	/*	STL Initialization			*/
	/******************************/
	/* Symbol Timing Loop Reset And Release
	 Both Integral And Direct Paths Are Cleared */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_STL_RESET,1);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_STL_RESET,0);
	/* Integral Path Enabled Only After The WBAGC Lock */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_ERR_EN,0);
	/* Direct Path Enabled Immediatly */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_PHASE_EN,1);
	/******************************/
	/*	CRL Initialization			*/
	/******************************/
	/* Carrier Recovery Loop Reset And Release
	 Frequency And Phase Offset Cleared As Well As All Internal Signals */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_CRL_RESET,1);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_CRL_RESET,0);
	/* Integral Path Enabled Only After The PMFAGC Lock */
	/* Direct Path Enabled Only After The PMFAGC Lock */
	/* Sweep OFF */
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,R0370QAM_CRL_2,0x02);
	/* Corner Detector OFF */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_LOCK_DETECT_ENA,0);
	/* PN Loop Bypass */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_BYP_SEC_LOOP,1);
	/* Fading Detector OFF */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_EN_FADDET,0);
	/* Frequency Estimator Reset */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_FREQ_EST_RESET,0); /* was 1 before*/
	
	/**/
	/******************************/
	/*	PMF AGC Initialization		*/
	/******************************/
	/* PMF AGC Status Forced To Unlock Position. Accumulator Is Also Reset */
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,R0370QAM_PMFAGC_2,0x08);
	/**/
	/******************************/
	/*	Equalizer Initialization	*/
	/******************************/
	/* Equalizer Values Capture */
	Freq19	=	STTUNER_IOREG_GetRegister(DeviceMap, IOHandle,R0370QAM_FREQ_19)	;
	Freq23	=	STTUNER_IOREG_GetRegister(DeviceMap, IOHandle,R0370QAM_FREQ_23)	;
	Freq24	=	STTUNER_IOREG_GetRegister(DeviceMap, IOHandle,R0370QAM_FREQ_24)	;
	/* Equalizer Reset And Release */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_EQ_RESET,1);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_EQ_RESET,0);
	/* Equalizer Values Reload */
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R0370QAM_FREQ_19,Freq19);
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R0370QAM_FREQ_23,Freq23);
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R0370QAM_FREQ_24,Freq24);
	/**/
	/******************************/
	/*	Signal Parameters Setting	*/
	/******************************/
	Reg0370QAM_SetQAMSize(DeviceMap,IOHandle,QAMSize);
	/* STL Freeze (Avoiding STV0297J Symbol Rate Update) */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_PHASE_CLR,1);
	Reg0370QAM_SetSymbolRate(DeviceMap,IOHandle,SymbolRate,multiply_factor_times_quartz);
	Reg0370QAM_SetSpectrumInversion(DeviceMap,IOHandle,SpectrumInversion);
	/**/
	
	/******************************/
	/*	FEC Initialization			*/
	/******************************/
	Driv0370QAMFecInit(DeviceMap,IOHandle);
	/**/
	
	
	/**/
	/******************************/
	/*	AGC Lock Part					*/
	/******************************/
	/* WBAGC Loop Enable */
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R0370QAM_WBAGC_3,0x10);
	/* Wide Band AGC Lock Check */
	EndOfSearch = NO;
	TimeOut = DataSearchTime + 30;	/* 30 ms Timeout */
	while(EndOfSearch == NO)
	{
		STB0370QAM_DelayInMilliSec(5);/*SystemWaitFor(5); to be checked whether this 
						replacement is correct or not ????*/
		DataSearchTime += 5;
		WBAGCLock = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_WAGC_ACQ);
		if((WBAGCLock == 1)||(DataSearchTime > TimeOut))
			EndOfSearch = YES;			
		
	}
	if(WBAGCLock == 0)	/* The WBAGC Has Not Converged */
	{
		return (DataFound);
	}
	/* The Code Commented Below Can Be Used To Avoid Locking On Very Low Signals */
	
	/* AGC Roll And Acquisition Threshold Set To the formerly chosen values
	 In Order To Switch To Tracking Mode */
	Reg0370QAM_SetWBAGCloop(DeviceMap,IOHandle,AGCLoop);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_ACQ_TH,ACQThreshold);
	
	/******************************/
	/*	Post WBAGC Blocks Lock		*/
	/******************************/
	/* STL Release */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_PHASE_CLR,0x00);
	/* Different Settings According To Whether The Estimator Is Used Or Not */
	if ((STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_FREQ_EST_CORR_BLIND) || STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_FREQ_EST_CORR_LMS1) || STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_FREQ_EST_CORR_LMS1)) != 1)
	{
		Reg0370QAM_SetFrequencyOffset(DeviceMap,IOHandle,CarrierOffset);
		Reg0370QAM_SetSweepRate(DeviceMap,IOHandle,SweepRate,multiply_factor_times_quartz)				;
		STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R0370QAM_CRL_2,0x03)			;	/* Enable Sweep, Estimator Reset Above */
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_LOCK_DETECT_ENA,0x01)		;	/* Corner Detector On */
		
	}
	else	/* Estimator Used */
	{
		/* Corner Detector Is Not Used Because There Is A Similar Detector In The Frequency Estimator */
		Reg0370QAM_SetFrequencyOffset(DeviceMap,IOHandle,0)		;	/* Offset Zeroed */
		Reg0370QAM_SetSweepRate(DeviceMap,IOHandle,0,multiply_factor_times_quartz)				;	/* Sweep Rate Zeroed, Sweep Is Disabled Above */
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_FREQ_EST_RESET,1)	;	/* Release Of Estimator Reset */	
		
	}	 
	/* PMF AGC Unlock Forcing Is Disabled, The CRL Can Act Now */	
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R0370QAM_PMFAGC_2,0x02);
	
	/* Waiting For A LMS2 Lock Indication Or A Timeout; Several Successive
	 Lock Indications Are Requested To Decide Equalizer Convergence */
	EndOfSearch    = NO;
	TimeOut = DataSearchTime + LMS2TimeOut;
	LMS2TrackingLock = 0;
	while (EndOfSearch == NO)
	{
		STB0370QAM_DelayInMilliSec(5);
		DataSearchTime += 5;
		int_tmp =  STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_LMS_STEP2);
		if(int_tmp == 1)
		{
			LMS2TrackingLock += 1;
		}
		else
		{
			LMS2TrackingLock = 0;
		}
		if((LMS2TrackingLock >= 5)||(DataSearchTime > TimeOut))
		{		
			EndOfSearch = YES;
		}
		
			
			
	}
	if(int_tmp == 0)	/* The Equalizer Has Not Converged */
	{		
			return (DataFound);
	}
	if ((STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_FREQ_EST_CORR_BLIND) || STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_FREQ_EST_CORR_LMS1) || STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_FREQ_EST_CORR_LMS2)) != 1)
	{
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_SW_EN,0);		/* LMS2 Is Locked: Sweep Is Disabled */
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_LOCK_DETECT_ENA,0);	/* Corner Detector Disabled */
		
	}
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_EN_FADDET,1);		/* Fading Detector Enabled */
	if (SymbolRate <= 2000000)	/* Phase-Noise Loop Activated For Low Symbol Rate Signals */
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_BYP_SEC_LOOP,0);
	/**/
	/******************************/
	/*	FEC Lock Check					*/
	/******************************/
	EndOfSearch = NO;
	
	 
	DataFound = Driv0370QAMFECLockCheck(DeviceMap,IOHandle,pParams,EndOfSearch, TimeOut, DataSearchTime);
	/**/
	/******************************/
	/*	Signal Parameters Update	*/
	/******************************/
	pParams->SymbolRate			= (S32)Reg0370QAM_GetSymbolRate(DeviceMap,IOHandle,multiply_factor_times_quartz)		;	/* Symbol Rate Update In Baud/s .... 
																		check this typecasting */
	pParams->Frequency			-=(S32) pParams->InitDemodOffset;  /* Carrier Offset Update ... check this typecasting */
	/* Signal Structure Update */
	pParams->Results.Modulation= pParams->Modulation		;
	pParams->Results.SymbolRate	= pParams->SymbolRate	;
	pParams->Results.Frequency	= pParams->Frequency	;
	
	
	return (DataFound);
}

/*----------------------------------------------------
 FUNCTION      FE_370QAM_Search
 ACTION
                This routine is the ENTRY POINT of the STB0370QAM driver.
                ======================================================

                This routine performs the carrier search following the user indications.
                This routine is the entry point of the STB0370QAM driver.

                The returned value is a flag (TRUE, FALSE) which indicates if the trial is
                successful or not. In case of lock, _pSignal->Frequency and _pSignal->SymbolRate
                are modifyed accordingly with the found carrier parameters.
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
BOOL FE_370QAM_Search(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,D0370QAM_StateBlock_t *pSearch, STTUNER_Handle_t  TopLevelHandle)
{
	FLAG_370QAM	DataFound = NO	;
	int	CarrierOffset	;
	int	RangeOffset;
	int	SweepRate;
	int	int_tmp	;
	int	InitDemodOffset	;
	ST_ErrorCode_t              Error = ST_NO_ERROR;
	STTUNER_tuner_instance_t *TunerInstance; 
	STTUNER_InstanceDbase_t     *Inst;
	FE_370QAM_InternalParams_t pParams; 
	FLAG_370QAM Driv0370QAMInitDemodOk;
	/* top level public instance data */ 
        Inst = STTUNER_GetDrvInst();
        
        /* get the tuner instance for this driver from the top level handle */
        TunerInstance = &Inst[TopLevelHandle].Cable.Tuner;
	
			
		
			Driv0370QAMSignalStatus.Status	=	_370QAM_NO_LOCKED				;	/* disable the signal monitoring */ 
			Driv0370QAMSignalStatus.NbLockLoss=	0;	
			/* results init */
			pSearch->Result.Lock.Status	    = _370QAM_NO_LOCKED				;	/* disable the signal monitoring */
			pSearch->Result.Lock.NbLockLoss	= 0						;	
			pParams.Quartz = pSearch->Quartz;
			pParams.Modulation			= pSearch->QAMSize;	/* QAM Size =  64, 256 */
			pParams.SymbolRate		    = (S32)pSearch->Params.SymbolRate;	/* in Baud/s ...check this typecasting*/
			pParams.Frequency		    = (S32)pSearch->Params.Frequency	;	/* in kHz */
			pParams.SpecInv	        = pSearch->SpectrumInversion;	/* YES or NO */
			pParams.SweepRate			= pSearch->SweepRate;	/* = 1000* (SweepRate (in MHz/s) / Symbol Rate (in MBaud/s)) */
			pParams.CarrierOffset		= pSearch->CarrierOffset;	/* = 10000 * (Carrier Offset (in KHz) / Symbol Rate (in KBaud)) */
                        pParams.IF                     = (U32) pSearch->Params.TunerIF;
			if (((pParams.SweepRate > 0)&&(pParams.CarrierOffset > 0)) ||
				((pParams.SweepRate < 0)&&(pParams.CarrierOffset < 0)))
				return(DataFound);
			if (pParams.SweepRate < 0)
			{
				SweepRate     = -pParams.SweepRate;
				CarrierOffset = -pParams.CarrierOffset;
			}
			else
			{
				SweepRate     = pParams.SweepRate;
				CarrierOffset = pParams.CarrierOffset;
			}


			Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle,(U32)pParams.Frequency,(U32 *)&pParams.Frequency);	/*	Move the tuner	*/   			


			DataFound = NO;
			RangeOffset = CarrierOffset;

			/* Initial Derotator Enabled ? */
			int_tmp = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_IN_DEMOD_EN);
			if ((int_tmp == 1)&&(RangeOffset < -400))
			{
				Driv0370QAMInitDemodOk = YES;
				RangeOffset = -400;				
			}
			else 
			{
				Driv0370QAMInitDemodOk = NO;
				
			}

			/* [- RangeOffset, + RangeOffset] range is scanned */
			pParams.InitDemodOffset = 0;
			pParams.CarrierOffset= (110*RangeOffset)/100;
			pParams.SweepRate= SweepRate;
			DataFound = Driv0370QAMDataSearch(DeviceMap,IOHandle,&pParams,TunerInstance);

			if (Driv0370QAMInitDemodOk == YES)
			{
				/* Initial Demodulation Is Required */
				InitDemodOffset = 0 ;
				while ((DataFound == NO)&&((InitDemodOffset + RangeOffset) >= CarrierOffset))
				{
					/* The subranges are scanned in a zig-zag mode around the initial subrange */
					SweepRate = -SweepRate;
					RangeOffset = -RangeOffset;
					if (RangeOffset > 0)
						InitDemodOffset = -InitDemodOffset + RangeOffset;
					else
						InitDemodOffset = -InitDemodOffset;

					/* [InitDemod - RangeOffset, InitDemod + RangeOffset] range is scanned */
					pParams.InitDemodOffset =  (int)((InitDemodOffset*(pParams.SymbolRate/1000))/10000);
					pParams.CarrierOffset= (110*RangeOffset)/100;
					pParams.SweepRate= SweepRate;
					
					DataFound = Driv0370QAMDataSearch(DeviceMap,IOHandle,&pParams,TunerInstance);
				}
			}

			if (DataFound)	
			{
				
				pSearch->Result.Lock.Status = _370QAM_LOCKED;
				Driv0370QAMSignalStatus.Status = _370QAM_LOCKED;
				
				/* Signal Structure Update */
	                    pSearch->Result.QAMSize= pParams.Modulation		;
	                    pSearch->Result.SymbolRate	= pParams.SymbolRate	;
	                    pSearch->Result.Frequency	= pParams.Frequency	;
			}

	return (DataFound);
}


/* End of drv370qam.c */
