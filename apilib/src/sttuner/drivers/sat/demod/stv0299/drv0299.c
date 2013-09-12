/*----------------------------------------------------------------------------
File Name   : drv0299.c

Description : STV0299 front-end driver routines.

Copyright (C) 2000 STMicroelectronics

version: 3.1.0

Revision History    :

    04/02/00        Code based on original implementation by CBB.

    21/03/00        Received code modifications that eliminate compiler
                    warnings.

                    Added defensive check to Mclk == 0 during parameter
                    initialization routine to avoid potential divide by
                    zero exceptions.
                    
    04/07/2005     Minidriver Updates
    Reference:
----------------------------------------------------------------------------*/
/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   #include  <stdlib.h>
   #include <string.h>
#endif
#include "stlite.h"     /* Standard includes */
#include "sttbx.h"
#include "stddefs.h"                    

/* STAPI */
#include "sttuner.h"                    
#include "sti2c.h"
#include "stcommon.h"

#include "stevt.h"

/* local to sttuner */	   
#ifdef STTUNER_MINIDRIVER
#include "iodirect.h"
#endif

#ifndef STTUNER_MINIDRIVER
	#include "util.h"       /* generic utility functions for sttuner */
	#include "dbtypes.h"    /* data types for databases */
	#include "drv0299.h"    /* header for this file */
	#include "ioarch.h"     /* I/O for this driver */
	#include "ioreg.h"      /* I/O for this driver */
#endif

#include "reg0299.h"    /* register mappings for the stv0299 */
#include "d0299.h"      /* top level header for this driver */
#include "sysdbase.h"   /* functions to accesss system data */
#include "tunsdrv.h"
/* defines ----------------------------------------------------------------- */

#define DRV0299_INTERPOLATE( LutLow, LutHigh, RealLow, RealHigh, Real) \
    (LutLow + (((Real - RealLow) * (LutHigh - LutLow)) / \
               (RealHigh - RealLow)))

/* Delay routine */
#define SystemWaitFor(x) STV0299_Delay((x*1000))

/* Add carrieroffset to SCPC transponder */
#define CARRIEROFFSET 2000

extern TUNSDRV_InstanceData_t *TUNSDRV_GetInstFromHandle(TUNER_Handle_t Handle);

#ifdef STTUNER_MINIDRIVER
long Reg0299_RegSetSymbolRate(long SymbolRate);
extern ST_ErrorCode_t tuner_tunsdrv_SetFrequency (U32 Frequency, U32 *ReturnFrequency, U32 SymbolRate);
extern BOOL Tuner_GetStatus(void);
#endif


/* private types ----------------------------------------------------------- */

typedef struct
{
    S32 Real;
    S32 Lookup;
} DRV0299_LUT_t;

/* Current LLA revision	*/
#ifndef STTUNER_MINIDRIVER
static ST_Revision_t Revision299  = " STV0299-LLA_REL_3.2(GUI) ";
#endif

static const DRV0299_LUT_t BerLUT[] =
{
    { ((U16)-1), 200000 },
    { 9600, 160000 },
    { 9450, 120000 },                   /* 2 */
    { 9260, 97000 },                    /* 2.5 */
    { 9000, 82000 },                    /* 3 */
    { 8760, 73000 },                    /* 3.5 */
    { 8520, 61500 },                    /* 4 */
    { 8250, 51000 },                    /* 4.5 */
    { 7970, 42000 },                    /* 5 */
    { 7690, 34000 },                    /* 5.5 */
    { 7360, 26000 },                    /* 6 */
    { 7080, 20600 },                    /* 6.5 */
    { 6770, 15000 },                    /* 7 */
    { 6470, 11200 },                    /* 7.5 */
    { 6200, 8000 },                     /* 8 */
    { 5900, 5400 },                     /* 8.5 */
    { 5670, 3600 },                     /* 9 */
    { 5420, 2320 },                     /* 9.5 */
    { 5190, 1450 },                     /* 10 */
    { 4960, 840 },                      /* 10.5 */
    { 4740, 480 },                      /* 11 */
    { 4550, 225 },                      /* 11.5 */
    { 4360, 130 },                      /* 12 */
    { 4170, 60 },                       /* 12.5 */
    { 4010, 30 },                       /* 13 */
    { 3860, 12 },                       /* 13.5 */
    { 3710, 4 },                        /* 14 */
    { 3580, 1 },                        /* 14.5 */
    { 3440, 0 },                        /* 15 */
    { 0, 0 },
    { ((U32)-1), 0 }                    /* End */
};

static const DRV0299_LUT_t SignalNoiseLUT[] =
{
    { ((U16)-1), 0 },                   /* 1 */
    { 9600, 19 },                       /* 1.5 */
    { 9450, 22 },                       /* 2 */
    { 9260, 25 },                       /* 2.5 */
    { 9000, 28 },                       /* 3 */
    { 8760, 31 },                       /* 3.5 */
    { 8520, 34 },                       /* 4 */
    { 8250, 37 },                       /* 4.5 */
    { 7970, 40 },                       /* 5 */
    { 7690, 43 },                       /* 5.5 */
    { 7360, 46 },                       /* 6 */
    { 7080, 49 },                       /* 6.5 */
    { 6770, 52 },                       /* 7 */
    { 6470, 55 },                       /* 7.5 */
    { 6200, 58 },                       /* 8 */
    { 5900, 61 },                       /* 8.5 */
    { 5670, 64 },                       /* 9 */
    { 5420, 67 },                       /* 9.5 */
    { 5190, 70 },                       /* 10 */
    { 4960, 73 },                       /* 10.5 */
    { 4740, 76 },                       /* 11 */
    { 4550, 79 },                       /* 11.5 */
    { 4360, 82 },                       /* 12 */
    { 4170, 85 },                       /* 12.5 */
    { 4010, 88 },                       /* 13 */
    { 3860, 91 },                       /* 13.5 */
    { 3710, 94 },                       /* 14 */
    { 3580, 97 },                       /* 14.5 */
    { 3440, 100 },                      /* 15 */
    { 0, 100 },
    { ((U32)-1), 0 }                          /* End */
};

#ifndef STTUNER_MINIDRIVER
static const DRV0299_LUT_t AgcLUT[] =
{
    { 127, 100 },                       /* 0dBm */
    { 115, 90 },                       /* -20dBm */
    { 75,  70 },                       /* -25dBm */
    { 65,  20 },                        /* -30dBm */
    { 52, -50 },                        /* -35dBm */
    { 44, -100 },                        /* -40dBm */
    { 34, -150 },                        /* -45dBm */
    { 25, -250 },                        /* -50dBm */
    { 14, -300 },                        /* -55dBm */
    { -16, -350 },                        /* -60dBm */
    { -41, -400 },                         /* -65dBm */
    { -69, -450 },                       /* -70dBm */
    { -82, -500 },                       /* -75dBm */
    { -92, -550 },                       /* -80dBm */
    { -100, -600 },                       /* -81dBm */
    { -128, -650 },                       /* -129 actually cannot returned by AGCINTEGRATOR, but it is to stop the while loop above -128, which is the least value to be returned*/
    { ((U32)-1), 0 }                    /* End */
};
#endif

/* variables --------------------------------------------------------------- */

/* Used to turn on the infinite test loop in the search to simulate delay on search */
BOOL TestLoopActive=FALSE;
#ifndef STTUNER_MINIDRIVER
/* functions --------------------------------------------------------------- */

/***********************************************************
**FUNCTION	::	Drv0299_GetLLARevision
**ACTION	::	Returns the 299 LLA driver revision
**RETURN	::	Revision299
***********************************************************/
ST_Revision_t Drv0299_GetLLARevision(void)
{
	return (Revision299);
}


/*----------------------------------------------------
FUNCTION      Drv0299_WaitTuner
ACTION        Wait for tuner locked
PARAMS IN     TimeOut -> Maximum waiting time (in ms)
PARAMS OUT    NONE
RETURN        NONE (Handle == THIS_INSTANCE.Tuner.DrvHandle)
------------------------------------------------------*/
void Drv0299_WaitTuner(STTUNER_tuner_instance_t *TunerInstance, int TimeOut)
{
    int Time = 0;
    BOOL TunerLocked = FALSE;
    ST_ErrorCode_t Error;

    while(!TunerLocked && (Time < TimeOut))
    {
        SystemWaitFor(1);
        Error = (TunerInstance->Driver->tuner_IsTunerLocked)(TunerInstance->DrvHandle, &TunerLocked);
        Time++;
    }
    Time--;
}
#endif
#ifdef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      CheckTiming
 ACTION        Check for timing locked
 PARAMS IN     Params->Ttiming    =>    Time to wait for timing loop locked
 PARAMS OUT    Params->State        =>    result of the check
 RETURN        E299_NOTIMING if timing not locked, E299_TIMINGOK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_CheckTiming(D0299_SearchParams_t *Params)
{
    int  locked,timing;
    U8 nsbuffer[2];
     /**********Delay of 100K Symbols************************/
    STOS_TaskDelayUs(((100000/(U32)(Params->SymbolRate/1000)))*1000);
    
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_TLIR, F0299_TLIR, F0299_TLIR_L, nsbuffer, 1, FALSE); 
    locked = nsbuffer[0];
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_RTF, F0299_RTF, F0299_RTF_L, nsbuffer, 1, FALSE);
    timing = abs((S8)nsbuffer[0]);
    
    if(locked >= 43)
	{
		if((locked > 48) && (timing >= 110))
			 Params->State = E299_ANALOGCARRIER; 
	  	else
			Params->State = E299_TIMINGOK;
	}
	else
		Params->State = E299_NOTIMING; 	

    return(Params->State);
}
#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
FUNCTION      Drv0299_SetAgc1Ref
ACTION        Optimize AGC1 reference value after signal is found at very low AGC1 reference level
PARAMS IN     DeviceMap,IOHandle
RETURN        Error
------------------------------------------------------*/

ST_ErrorCode_t Drv0299_SetAgc1Ref(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle )
{
	char Agc1Ref = 0x1f, agcintvalue;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_SetAgc1Value()";
#endif
	ST_ErrorCode_t Error = ST_NO_ERROR;
    
    Error = STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_AGC1_REF, 0x1f); /* set AGC_Ref as maximum*/
    
    if(Error != ST_NO_ERROR)
    {
	    return Error;
    }
      do
	    {
		    Agc1Ref -= 5;  /* decrease AGC1_Ref by step of 5 */
		    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_AGC1_REF, Agc1Ref);
		    agcintvalue = (char)STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_AGCINTEGRATORVALUE);
		}while((agcintvalue==-128) && (Agc1Ref >= 5));
	    
	return Error;
 
}    

/*----------------------------------------------------
 FUNCTION        CheckAgc1
 ACTION        Check agc1 value
 PARAMS IN        Params -> Pointer to SEARCHPARAMS structure, TunerID added GNBvd26185
 PARAMS OUT    Params->State is modified
 RETURN        E299_NOAGC1 if AGC1=-128, E299_AGC1SATURATION
                 if AGC1=127, E299_AGC1OK otherwise
                 
------------------------------------------------------*/
D0299_SignalType_t Drv0299_CheckAgc1(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
   const char *identity = "STTUNER drv0299.c Drv0299_CheckAgc1()";
#endif
    int AGC1Value;
    int Agc1Threshold = -128;
    
     if(TunerType!=STTUNER_TUNER_MAX2116)
    {
	AGC1Value = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_AGCINTEGRATORVALUE);
	
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
	STTBX_Print(("%s AGC1Value = %d (", identity, AGC1Value));
	#endif
	
	if (AGC1Value == Agc1Threshold )
	{
		Params->State = E299_NOAGC1;
		#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
		STTBX_Print(("E299_NOAGC1)\n"));
		#endif
	}
	else if(AGC1Value == 127)
	{
		Params->State = E299_AGC1SATURATION;
		#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
		STTBX_Print(("E299_AGC1SATURATION)\n"));
		#endif
	}
	else
	{
		Params->State = E299_AGC1OK;
		#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
		STTBX_Print(("E299_AGC1OK)\n"));
		#endif
	}
   }
    else Params->State = E299_AGC1OK;

    
    return(Params->State);
}



/*----------------------------------------------------
 FUNCTION      CheckTiming
 ACTION        Check for timing locked
 PARAMS IN     Params->Ttiming    =>    Time to wait for timing loop locked
 PARAMS OUT    Params->State        =>    result of the check
 RETURN        E299_NOTIMING if timing not locked, E299_TIMINGOK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_CheckTiming(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params)
{
    U8  locked;
    S8 timing;
    
    SystemWaitFor(Params->Ttiming);

    locked = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_TLIR);
            
    timing = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_RTF);
    if(locked >= 43)
	{
		if((locked > 48) && ((MAC0299_ABS(timing)) >= 110))
			 Params->State = E299_ANALOGCARRIER; 
	  	else
			Params->State = E299_TIMINGOK;
	}
	else
		Params->State = E299_NOTIMING; 	

    return(Params->State);
}
#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      OffsetSymbolRate
 ACTION        Set actual symbol rate by adding offset in the symbol rate
 PARAMS IN     offset          
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/

void FE_299_OffsetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,S32 Offset) 
{
	U32 U32Tmp;
	S32 tst;
	U8 sfrh[3];
 
	STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_SFRH, 3,sfrh);
	
	U32Tmp  = (sfrh[0] & 0xFF)<<12; 	
	U32Tmp += (sfrh[1] & 0xFF)<<4; 
	U32Tmp += (sfrh[2] & 0xf0)>>4;  

	
	tst = ((Offset*(S32)U32Tmp) / 524288); /* 2^19 = 524288*/
	
	U32Tmp += tst;
	

	sfrh[0]=(U8)((U32Tmp>>12))&0xFF;
	sfrh[1]=(U8)((U32Tmp>>4))&0xFF;
	sfrh[2]=(((U8)(U32Tmp&0x0F))<<4)&0xF0;

	STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle, R0299_SFRH, sfrh,3);


}


/*----------------------------------------------------
 FUNCTION      CenterTimingLoop
 ACTION        Timing loop centring
 PARAMS IN     SymbolRate  -> Current symbol rate value
               MasterClock -> Current master clock frequency
 PARAMS OUT    NONE
 RETURN        New symbol rate
------------------------------------------------------*/
long Drv0299_CenterTimingLoop(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, long SymbolRate, long MasterClock)
{
	int i;
	short int tTiming;
	S8 timingOffset;
	
	tTiming = (short int)(Drv0299_CalcTimingTimeConstant(SymbolRate) );
	
	
	for(i=0;i<5;i++)
	{
	
		SystemWaitFor(tTiming);
		
		timingOffset = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_RTF);
				
		if(ABS(timingOffset) >= 20)
		{
			
			FE_299_OffsetSymbolRate(DeviceMap,IOHandle,timingOffset);			
		}
		else
		{
			STOS_TaskDelayUs(1000*tTiming);
			timingOffset = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_RTF);
			
			/* Check timing offset a second time in case of analog transponders */ 
			if(ABS(timingOffset)<20)
			{
				
				SymbolRate = Reg0299_RegGetSymbolRate(DeviceMap, IOHandle);
				
			}

			break;
		}
		
	}

	return(SymbolRate);
	
}



/*----------------------------------------------------
 FUNCTION      CheckAgc2
 ACTION        Check agc2 value
 PARAMS IN     NbSample  -> Number of samples
 PARAMS OUT    Agc2Value -> Mean of Agc2 values
 RETURN        E299_NOAGC2 if AGC2 =0 or AGC2 = 32767, E299_AGC2OK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_CheckAgc2(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, int NbSample,long *Agc2Value)
{
    int  Agc2Threshold, loop;
    long Agc2Limit;
    D0299_SignalType_t Agc2Level;
    U8 agc[2];

    Agc2Threshold = 0;
    Agc2Limit     = 32767 * (long)NbSample;  /* cast added to eliminate compiler warning --SFS */

    for (loop = 0; loop < NbSample; loop++)
    {
        SystemWaitFor(1);   /* 1ms delay: settling time */
        STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_AGC2I1, 2,agc);  /* Read AGC2I1 and AGC2I2 registers */

    /* use int cast below to eliminate compile warning;  why does this work??? (I would think it should be caste to long) --SFS */
        *Agc2Value += (int)((agc[0]<<8) + agc[1]);
    }

    if ( ( *Agc2Value > Agc2Threshold ) && ( *Agc2Value < Agc2Limit ) )
    {
         Agc2Level = E299_AGC2OK;
        *Agc2Value = *Agc2Value / NbSample;
    }
    else
    {
         Agc2Level = E299_NOAGC2;
        *Agc2Value = 0;
    }

    return(Agc2Level);
}

 

/*----------------------------------------------------
--FUNCTION      CheckCarrier
--ACTION        Check for carrier founded
--PARAMS IN     Params        => Pointer to SEARCHPARAMS structure
--PARAMS OUT    Params->State => Result of the check
--RETURN        E299_NOCARRIER carrier not founded, E299_CARRIEROK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_CheckCarrier(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params)
{
    SystemWaitFor(Params->Tderot);  /* wait for derotator ok */

    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_CFD_ALGO, 0);

    if ( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_CF) )
        Params->State = E299_CARRIEROK;
    else
        Params->State = E299_NOCARRIER;

    return(Params->State);
}

#endif
#ifdef STTUNER_MINIDRIVER
 D0299_SignalType_t Drv0299_CheckCarrier(D0299_SearchParams_t *Params)
{
   U8 Data = 0;
    /**********Delay of 100K Symbols************************/
    task_delay((100000/(U32)(Params->SymbolRate/1000))*(ST_GetClocksPerSecond()/1000));

   STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_CFD, F0299_CFD_ALGO, F0299_CFD_ALGO_L, &Data, 1, FALSE);
   STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_VSTATUS, F0299_CF, F0299_CF_L, &Data, 1, FALSE);
    
    if(Data == 1)
        Params->State = E299_CARRIEROK;
    else
        Params->State = E299_NOCARRIER;

    return(Params->State);
}   
#endif    
/*----------------------------------------------------
--FUNCTION      CarrierWidth
--ACTION        Compute the width of the carrier
--PARAMS IN     SymbolRate -> Symbol rate of the carrier (Kbauds or Mbauds)
--              RollOff    -> Rolloff * 100
--PARAMS OUT    NONE
--RETURN        Width of the carrier (KHz or MHz)
------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
long Drv0299_CarrierWidth(long SymbolRate, long RollOff)
{
    return (SymbolRate  + (SymbolRate * RollOff)/100);
}



/*----------------------------------------------------
 FUNCTION      CheckData
 ACTION        Check for data founded
 PARAMS IN     Params        =>    Pointer to SEARCHPARAMS structure
 PARAMS OUT    Params->State    => Result of the check
 RETURN        E299_NODATA data not founded, E299_DATAOK otherwise
------------------------------------------------------*/

D0299_SignalType_t Drv0299_CheckData(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params)
{
	SystemWaitFor(Params->Tdata); /* Wait for data */
    
    if ( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_LK) )
        Params->State = E299_DATAOK;
              
    else
        Params->State = E299_NODATA;

    return(Params->State);
}
#endif
#ifdef STTUNER_MINIDRIVER
D0299_SignalType_t Drv0299_CheckData(D0299_SearchParams_t *Params)
{
    U8 Data;
    int i=0, loopcount = 0, timeout = 100;
    i = timeout/10;
    do{
    	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_VSTATUS, F0299_LK, F0299_LK_L, &Data, 1, FALSE);
        if ( Data == 1)
        {
	        Params->State = E299_DATAOK;
	        return(Params->State);
	}
        else
        Params->State = E299_NODATA;
        
	  task_delay(10*(ST_GetClocksPerSecond()/1000));
    	
    	++loopcount;
	
      }while((loopcount<= i) && (Params->State !=E299_DATAOK));

    return(Params->State);
}	

void Drv0299_IQInvertion(void)
{
    U8 iq;
    
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_IOCFG, F0299_IQ, F0299_IQ_L, &iq, 1, FALSE);
    
    iq = 0x01 & (~iq);
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_IOCFG, F0299_IQ, F0299_IQ_L, &iq, 1, FALSE);
}
#endif
   
 #ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      IQInvertion
 ACTION        Invert I and Q
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/

void Drv0299_IQInvertion(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    U32 iq;
   
    iq = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_IQ);
    
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_IQ, 0x01 & (~iq) );    /* inverting the I/Q configuration */
}

/*----------------------------------------------------
 FUNCTION      IQSet
 ACTION        Set I and Q
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/

void Drv0299_IQSet(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 IQMode)
{
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_IQ, IQMode);    /* Set the I/Q configuration */
}
#endif

#ifdef STTUNER_MINIDRIVER
void Drv0299_IQSet(U32 IQMode)
{
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_IOCFG, F0299_IQ, F0299_IQ_L, (U8 *)IQMode, 1, FALSE);
}
#endif  
#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
--FUNCTION      CalcTimingTimeConstant
--ACTION        Compute the amount of time needed by the timing loop to lock
--PARAMS IN     SymbolRate -> symbol rate value
--PARAMS OUT    NONE
--RETURN        Timing loop time constant (ms)
------------------------------------------------------*/

long Drv0299_CalcTimingTimeConstant(long SymbolRate)
{
    return (200000/(SymbolRate/1000));
}



/*----------------------------------------------------
 FUNCTION      CalcDerotTimeConstant
 ACTION        Compute the amount of time needed by the Derotator to lock
 PARAMS IN     SymbolRate -> symbol rate value
 PARAMS OUT    NONE
 RETURN        Derotator time constant (ms)
------------------------------------------------------*/

long Drv0299_CalcDerotTimeConstant(long SymbolRate)
{
    return (10000/(SymbolRate/1000));
}



/*----------------------------------------------------
 FUNCTION      CalcDataTimeConstant
 ACTION        Compute the amount of time needed to capture data
 PARAMS IN       Er         -> Viterbi rror rate
                 Sn         -> viterbi averaging period
                 To         -> viterbi time out
                 Hy         -> viterbi hysteresis
                 SymbolRate -> symbol rate value
 PARAMS OUT    NONE
 RETURN        Data time constant
------------------------------------------------------*/

long Drv0299_CalcDataTimeConstant(int Er, int Sn, int To, int Hy, long SymbolRate, U32 R0299_FECMVal)
{
    long Tviterbi = 0;
    long TimeOut =0;
    long THysteresis = 0;
    long PhaseNumberDVB[5] = {2,6,4,6,8};
    long PhaseNumberDSS[5] = {2,6,4,6,14};
    long averaging[4] = {1024L, 4096L, 16384L, 65536L};
    long InnerCode = 1000;
    long HigherRate = 1000;

    int loop, DSS_Mode;


    /* Data capture time (in ms)
      -------------------------
      This time is due to the Viterbi synchronisation.
     
         For each authorized inner code, the Viterbi search time is calculated,
         and the results are cumulated in ViterbiSearch.
    */   
	DSS_Mode = (STTUNER_IOREG_FieldExtractVal(R0299_FECMVal, F0299_FECMODE) == 0x04);

    for(loop = 0; loop < 5; loop++)
    {
        if ( ((Er >> loop)& 0x01) == 0x01 )
        {
            switch(loop)
            {
                case 0:                 /*    inner code 1/2    */
                    InnerCode = 2000;   /* 2.0 */
                    break;

                case 1:                 /*    inner code 2/3    */
                    InnerCode = 1500;   /* 1.5 */
                    break;

                case 2:                 /*    inner code 3/4  */
                    InnerCode = 1333;   /* 1.333 */
                    break;

                case 3:                 /*    inner code 5/6    */
                    InnerCode = 1200;   /* 1.2 */
                    break;

                case 4:                 /*    inner code 7/8    */
                    InnerCode = 1143;   /* 1.143 */
                    break;
            }

			if(!DSS_Mode)
				Tviterbi +=(int)((PhaseNumberDVB[loop]*averaging[Sn]*InnerCode)/SymbolRate);
			else
				Tviterbi +=(int)((PhaseNumberDSS[loop]*averaging[Sn]*InnerCode)/SymbolRate);   

            if(HigherRate < InnerCode) HigherRate = InnerCode;
        }

    }   /* for(loop) */


    /* time out calculation (TimeOut) This value indicates the maximum duration of the synchro word research. */
    TimeOut = (int)((HigherRate * 16384L * ((long)To + 1))/(2*SymbolRate));  /* cast To to long (eliminate compiler warning --SFS */


    /* Hysteresis duration (Hysteresis) */
    THysteresis = (int)((HigherRate * 26112L * ((long)Hy +1))/(2*SymbolRate)); /* 26112= 16*204*8 bits */ /* cast Hy to long (eliminate compiler warning --SFS */


    /* a guard time of 1 mS is added */
    return (1 + Tviterbi + TimeOut + THysteresis);
}
#endif
#ifndef STTUNER_MINIDRIVER

/*----------------------------------------------------
 FUNCTION      InitParams
 ACTION        Set Params fields that are never changed during search algorithm
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Drv0299_InitParams(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_StateBlock_t *StateBlock)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
   const char *identity = "STTUNER drv0299.c Drv0299_InitParams()";
#endif
    int stdby, dirclk, k, m, p, m1, betaagc1, agc2coef, MasterClock;
    U8 rcr[2],agcr[2],agc1c;

    /* Read registers (in burst mode) */
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_RCR,   2,rcr);   /* Read RCR and MCR registers     */
    agc1c =     STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_AGC1C);      /* Read AGC1C register            */
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_AGC1R, 2,agcr);   /* Read AGC1R and AGC2O registers */

    /*    Get fields values    */
    stdby    = STTUNER_IOREG_FieldExtractVal(rcr[1], F0299_STDBY);
    dirclk   = STTUNER_IOREG_FieldExtractVal(rcr[0], F0299_DIRCLK);
    k        = STTUNER_IOREG_FieldExtractVal(rcr[0], F0299_K);
    m        = STTUNER_IOREG_FieldExtractVal(rcr[0], F0299_M);
    p        = STTUNER_IOREG_FieldExtractVal(rcr[1], F0299_P);
    m1       = STTUNER_IOREG_FieldExtractVal(agcr[0], F0299_AGC1_REF);
    betaagc1 = STTUNER_IOREG_FieldExtractVal(agc1c, F0299_BETA_AGC1);
    agc2coef = STTUNER_IOREG_FieldExtractVal(agcr[1], F0299_AGC2COEF);

    /*    Initial calculations    */    /* cast to match parameter type --SFS */
    MasterClock = (int)Reg0299_CalcMasterClkFrequency(DeviceMap, stdby, dirclk, k, m, p);  
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
        STTBX_Print(("%s MasterClock = %d,  Mclk = %d\n", identity, MasterClock, MasterClock/65536L));
#endif

    StateBlock->Params.Tagc1       = (int)( Reg0299_CalcAGC1TimeConstant(m1, MasterClock, betaagc1)/ 20L);       /* cast to match parameter type --SFS */
    StateBlock->Params.Tagc2       = (int)( Reg0299_CalcAGC2TimeConstant(agc2coef, m1, MasterClock) / 1000000L); /* cast to match parameter type --SFS */
    StateBlock->Params.MasterClock = MasterClock;
    StateBlock->Params.Mclk        = MasterClock/65536L;
    StateBlock->Params.RollOff     = Reg0299_GetRollOff(DeviceMap, IOHandle);

    /* Added defensive check against Mclk == 0 to avoid potential
       divide by zero exceptions throughout the code.  In practice
       Mclk should never equal zero - if it does, something
       seriously bad has happened!  In the event that Mclk is
       zero at this point we reset it to 1 to ensure
       driver computations involving Mclk do not produce
       exceptions.
     */

    if (StateBlock->Params.Mclk == 0) StateBlock->Params.Mclk = 1; /* Serious error - reset to avoid exceptions */
}
#endif
#ifndef STTUNER_MINIDRIVER

/*----------------------------------------------------
 FUNCTION      InitSearch
 ACTION        Set Params fields that are used by the search algorithm
 PARAMS IN     Frequency   => Frequency used to start zig zag
               SymbolRate  => searched symbol rate
               SearchRange => Range of the search
               DerotStep   => Size of derotator's steps used in the carrier search (in per thousand of symbol frequency)
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void Drv0299_InitSearch(STTUNER_tuner_instance_t *TunerInstance, D0299_StateBlock_t *StateBlock, int Frequency, int SymbolRate, int SearchRange, int DerotStep)
{
    U32 BandWidth;
    ST_ErrorCode_t Error;
    TUNER_Status_t TunerStatus;

    /* Obtain current tuner status */
    Error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);

    /* Select closest bandwidth for tuner */ /* cast to U32 type to match function argument & eliminate compiler warning --SFS */
    Error = (TunerInstance->Driver->tuner_SetBandWidth)( TunerInstance->DrvHandle, 
                                                        (U32)( Drv0299_CarrierWidth(SymbolRate, StateBlock->Params.RollOff) /1000 + 12000),/* set bandwidth some more to optimize scanning->GNBvd26185*/
                                                        &BandWidth);

    StateBlock->Params.Frequency    = Frequency;
    StateBlock->Params.SymbolRate   = SymbolRate;
    if(SearchRange < 10000000)
    {
    	StateBlock->Params.SearchRange  = 10000000;
    }
    else
    {
    	StateBlock->Params.SearchRange  = SearchRange;
    }
        
    StateBlock->Params.DerotPercent = DerotStep;
    StateBlock->Params.TunerBW      = (long)BandWidth * 1000;  /* cast from U32 to long to eliminate compiler warning --SFS */
    StateBlock->Params.TunerStep    = (long)TunerStatus.TunerStep;
    StateBlock->Params.TunerIF      = (long)TunerStatus.IntermediateFrequency;
    StateBlock->Params.TunerIQSense = (long)TunerStatus.IQSense;

    StateBlock->Result.SignalType = E299_NOAGC1;
    StateBlock->Result.Frequency  = 0;
    StateBlock->Result.SymbolRate = 0;
}
#endif

#ifdef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      SearchTiming
 ACTION        Perform an Fs/2 zig zag to found timing
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        E299_NOTIMING if no valid timing had been found, E299_TIMINGOK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_SearchTiming(D0299_SearchParams_t *Params)
{
    short int DerotStep;
    short int DerotMin,DerotMax;
    short int DerotFreq     = 0;
    short int LastDerotFreq = 0;
    short int NextLoop      = 2;
    int index = 0;
    
    U8 nsbuffer[5];
   
    Params->State = E299_NOTIMING;
    Params->Direction = 1;

    /* timing loop computation & symbol rate optimisation */
    
    DerotMin  = (short int)  (((-(long)(Params->SubRange))/ 2L + Params->FreqOffset) / Params->Mclk); /* introduce 14 MHz derotator offset */
    DerotMax  = (short int)  ((Params->SubRange/2L + Params->FreqOffset)/Params->Mclk); 
    DerotStep = (short int)( (Params->SymbolRate/2L) / Params->Mclk ); /* cast to eliminate compiler warning --SFS */

    do
    {

        if (Drv0299_CheckTiming(Params) != E299_TIMINGOK)
        {
            index++;
            LastDerotFreq = DerotFreq;
            DerotFreq = DerotFreq + ( (int)Params->TunerIQSense * index * Params->Direction * DerotStep);    /* Compute the next derotator position for the zig zag */

            if((DerotFreq < DerotMin) || (DerotFreq > DerotMax))
		NextLoop--;

            if(NextLoop)
            {
                nsbuffer[1] = MAC0299_LSB(DerotFreq);
    	        nsbuffer[0] = MAC0299_MSB(DerotFreq);
    	        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_CFRM, 0, 0, nsbuffer,2, FALSE);
    	    }
    	
        }
        Params->Direction = -Params->Direction; /* Change the zigzag direction */
    }
    while((Params->State != E299_TIMINGOK) && NextLoop);  /* do..while */


    if(Params->State == E299_TIMINGOK)
    {
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_CFRM, 0, 0, nsbuffer, 2, FALSE);
        /* Multiply DerotFreq with IQ wiring sense of tuner & Demod */
        Params->DerotFreq = (short int)Params->TunerIQSense * (short int)MAC0299_MAKEWORD( nsbuffer[0],nsbuffer[1] );
        
       
    }
    else
    {
        Params->DerotFreq = LastDerotFreq;
    }

    return(Params->State);
}

#endif
#ifdef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      SearchCarrier
 ACTION        Search a QPSK carrier with the derotator
 PARAMS IN 
 PARAMS OUT    NONE
 RETURN        E299_NOCARRIER if no carrier had been found, E299_CARRIEROK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_SearchCarrier(D0299_SearchParams_t *Params)
{
    short int DerotMin,DerotMax;
    short int DerotFreq     = 0;
    short int LastDerotFreq = 0;
    short int NextLoop      = 2;    
    int index = 0;
    U8 nsbuffer[5], Data;
    
    Params->State = E299_NOCARRIER;
    Params->Direction = 1;

    
    DerotMin =  (short int)  (((-(long)(Params->SubRange))/ 2L + Params->FreqOffset) / Params->Mclk); /* introduce 14 MHz derotator offset */
    DerotMax =  (short int)  ((Params->SubRange/2L + Params->FreqOffset)/Params->Mclk); 
    DerotFreq  = Params->DerotFreq;
    
    Data = 1;
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_CFD, F0299_CFD_ALGO, F0299_CFD_ALGO_L, &Data, 1, FALSE);
            

    do
    {
    
        if(Drv0299_CheckCarrier(Params) == E299_NOCARRIER)
        {
	        index++;
            LastDerotFreq = DerotFreq;
            DerotFreq     = DerotFreq + ( (int)Params->TunerIQSense * index * Params->Direction * Params->DerotStep);  /* Compute the next derotator position for the zig zag */

            if((DerotFreq < DerotMin) || (DerotFreq > DerotMax))
	      NextLoop--;

            if(NextLoop)
            {
                Data = 1;
                STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_CFD, F0299_CFD_ALGO, F0299_CFD_ALGO_L, &Data, 1, FALSE);

                nsbuffer[1] = MAC0299_LSB(DerotFreq);
    	        nsbuffer[0] = MAC0299_MSB(DerotFreq);
    	        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_CFRM, 0, 0, nsbuffer, 2, FALSE);
            }
        }
        
        Params->Direction = -Params->Direction; /* Change the zig-zag direction */
    }
    while((Params->State != E299_CARRIEROK) && NextLoop);


    if(Params->State == E299_CARRIEROK)
    {
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_CFRM, 0, 0, nsbuffer, 2, FALSE);
        /* Multiply DerotFreq with IQ wiring sense of tuner & Demod */
        Params->DerotFreq = (short int)Params->TunerIQSense * (short int)MAC0299_MAKEWORD( nsbuffer[0],nsbuffer[1] );
    }
    else
    {
        Params->DerotFreq = LastDerotFreq;
    }


    return(Params->State);
}

#endif

#ifndef STTUNER_MINIDRIVER

/*----------------------------------------------------
 FUNCTION      SearchTiming
 ACTION        Perform an Fs/2 zig zag to found timing
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        E299_NOTIMING if no valid timing had been found, E299_TIMINGOK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_SearchTiming(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result, STTUNER_Handle_t TopLevelHandle)
{
    short int DerotStep;
    short int DerotMin,DerotMax;
    short int DerotFreq     = 0;
    short int LastDerotFreq = 0;
    short int NextLoop      = 2;
    int index = 0;
    U8 sfrm[2];
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;    



    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;

    Params->State = E299_NOTIMING;
    Params->Direction = 1;

    /* timing loop computation & symbol rate optimisation */
    
    DerotMin =  (short int)  (((-(long)(Params->SubRange))/ 2L + Params->FreqOffset) / Params->Mclk); /* introduce 14 MHz derotator offset */
    DerotMax =  (short int)  ((Params->SubRange/2L + Params->FreqOffset)/Params->Mclk); 
    DerotStep  = (short int)( (Params->SymbolRate/2L) / Params->Mclk ); /* cast to eliminate compiler warning --SFS */

    do
    {

        /* Try the terminate check here */
        if(Inst[TopLevelHandle].ForceSearchTerminate==TRUE)
              break;
        
        if (Drv0299_CheckTiming(DeviceMap, IOHandle, Params) != E299_TIMINGOK)
        {
            index++;
            LastDerotFreq = DerotFreq;
            DerotFreq = DerotFreq + ( (int)Params->TunerIQSense * index * Params->Direction * DerotStep);    /* Compute the next derotator position for the zig zag */

            if((DerotFreq < DerotMin) || (DerotFreq > DerotMax))
		NextLoop--;

            if(NextLoop)
            {
                sfrm[0]=MAC0299_MSB(DerotFreq);
                sfrm[1]= MAC0299_LSB(DerotFreq);
                STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle, R0299_CFRM, sfrm,2);   /* Set the derotator frequency */
            }
        }
        else
        {
            Result->SymbolRate = Params->SymbolRate;
        }

        Params->Direction = -Params->Direction; /* Change the zigzag direction */
    }
    while((Params->State != E299_TIMINGOK) && NextLoop);  /* do..while */


    if(Params->State == E299_TIMINGOK)
    {
        STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_CFRM,2,sfrm); /* Get the derotator frequency */
        /* Multiply DerotFreq with IQ wiring sense of tuner & Demod */
        Params->DerotFreq = (short int)Params->TunerIQSense * (short int)MAC0299_MAKEWORD( sfrm[0],sfrm[1] );
        
       
    }
    else
    {
        Params->DerotFreq = LastDerotFreq;
    }

    return(Params->State);
}



/*----------------------------------------------------
 FUNCTION      SearchCarrier
 ACTION        Search a QPSK carrier with the derotator
 PARAMS IN 
 PARAMS OUT    NONE
 RETURN        E299_NOCARRIER if no carrier had been found, E299_CARRIEROK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_SearchCarrier(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result, STTUNER_Handle_t TopLevelHandle)
{
    short int DerotMin,DerotMax;
    short int DerotFreq     = 0;
    short int LastDerotFreq = 0;
    short int NextLoop      = 2;    
    int index = 0;
    U8 cfrm[2];
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;
    


    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;

    Params->State = E299_NOCARRIER;
    Params->Direction = 1;

    
    DerotMin =  (short int)  (((-(long)(Params->SubRange))/ 2L + Params->FreqOffset) / Params->Mclk); /* introduce 14 MHz derotator offset */
    DerotMax =  (short int)  ((Params->SubRange/2L + Params->FreqOffset)/Params->Mclk); 
    DerotFreq  = Params->DerotFreq;
    
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_CFD_ALGO, 1);

    do
    {

        /* Try the terminate check here */
        if(Inst[TopLevelHandle].ForceSearchTerminate==TRUE)
              break;
        
        if(Drv0299_CheckCarrier(DeviceMap, IOHandle, Params) == E299_NOCARRIER)
        {
	        index++;
            LastDerotFreq = DerotFreq;
            DerotFreq     = DerotFreq + ( (int)Params->TunerIQSense * index * Params->Direction * Params->DerotStep);  /* Compute the next derotator position for the zig zag */

            if((DerotFreq < DerotMin) || (DerotFreq > DerotMax))
	      NextLoop--;

            if(NextLoop)
            {
                STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_CFD_ALGO, 1);

                cfrm[0]=MAC0299_MSB(DerotFreq);
                cfrm[1]=MAC0299_LSB(DerotFreq);
                STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle, R0299_CFRM, cfrm,2);   /* Set the derotator frequency */
            }
        }
        else
        {
            Result->SymbolRate = Params->SymbolRate;
        }

        Params->Direction = -Params->Direction; /* Change the zig-zag direction */
    }
    while((Params->State != E299_CARRIEROK) && NextLoop);


    if(Params->State == E299_CARRIEROK)
    {
        STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle,  R0299_CFRM, 2,cfrm);   /* Get the derotator frequency */
        /* Multiply DerotFreq with IQ wiring sense of tuner & Demod */
        Params->DerotFreq = (short int)Params->TunerIQSense * (short int)MAC0299_MAKEWORD( cfrm[0],cfrm[1] );
    }
    else
    {
        Params->DerotFreq = LastDerotFreq;
    }


    return(Params->State);
}



/*----------------------------------------------------
 FUNCTION      SearchFalseLock
 ACTION        Search a QPSK carrier with the derotator, if there is a false lock
 PARAMS IN    
 PARAMS OUT    NONE
 RETURN        E299_NOCARRIER if no carrier had been found, E299_CARRIEROK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_SearchFalseLock(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result, STTUNER_Handle_t TopLevelHandle)
{
    short int DerotFreq;
    short int DerotStep;
    short int DerotLimit;
    short int DerotOffset;
    short int NextLoop = 2;
    int  index = 1;
    U8 cfrm[2];
    long CurrentFreq=0, NewFrequency;
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;
    

    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;

    Params->State = E299_NOCARRIER;

    DerotStep  = (short int)( (Params->SymbolRate/4L) / Params->Mclk); /* cast to eliminate compiler warning --SFS */
    DerotLimit = (short int)( (Params->SubRange/2L)   / Params->Mclk); /* cast to eliminate compiler warning --SFS */
    DerotFreq  = Params->DerotFreq;

    do
    {

        /* Try the terminate check here */
        if(Inst[TopLevelHandle].ForceSearchTerminate==TRUE)
              break;
        
        DerotFreq   = DerotFreq + ( (int)Params->TunerIQSense * index * Params->Direction * DerotStep);   /* Compute the next derotator position for the zig zag */
        CurrentFreq = Params->Frequency + ( DerotFreq * Params->Mclk)/1000;

        if(MAC0299_ABS(DerotFreq) > DerotLimit) NextLoop--;

        if(NextLoop)
        {
            /* if the False Lock is outside the derotator capture range */ /*(U32)CurrentFreq cast to eliminate compiler warning --SFS */
            Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, (U32)CurrentFreq, (U32 *)&NewFrequency);
            
            DerotOffset = (short int)(( (NewFrequency - CurrentFreq) * 1000 ) / Params->Mclk);    /* Move the tuner */ /* cast to eliminate compiler warning --SFS */

            Drv0299_WaitTuner(TunerInstance, 100); /* Is tuner Locked? */

            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_CFD_ALGO, 1);

            cfrm[0]=MAC0299_MSB(DerotOffset);
            cfrm[1]=MAC0299_LSB(DerotOffset);
            STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle, R0299_CFRM,cfrm, 2);    /* Reset the derotator */

            Drv0299_CheckCarrier(DeviceMap, IOHandle, Params);

            if(Params->State == E299_CARRIEROK) Drv0299_CheckData(DeviceMap, IOHandle, Params);

            index++;
        }

        Params->Direction = -Params->Direction; /* Change the zigzag direction */
    }
    while((Params->State != E299_DATAOK) && NextLoop);


    if(Params->State == E299_DATAOK)
    {
        STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle,  R0299_CFRM, 2,cfrm);   /* Get the derotator frequency */
        /* Multiply DerotFreq with IQ wiring sense of tuner & Demod */
        Params->DerotFreq = (short int)Params->TunerIQSense * (short int)MAC0299_MAKEWORD( cfrm[0],cfrm[1]);
        Params->Frequency = CurrentFreq;
    }


    return(Params->State);
}

 

/*----------------------------------------------------
 FUNCTION      Searchlock
 ACTION        Search for data
 PARAMS IN     
 PARAMS OUT    
 RETURN        E299_NODATA if data not founded, E299_DATAOK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_Searchlock(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result, STTUNER_Handle_t TopLevelHandle)
{

    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;
    

    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;
   
    switch(Params->DemodIQMode)
    {
        case STTUNER_IQ_MODE_NORMAL:

            Drv0299_IQSet(DeviceMap, IOHandle,0);       /* Set to Normal */
            
            Drv0299_CheckData(DeviceMap, IOHandle, Params);
        break;
        
        case STTUNER_IQ_MODE_INVERTED:

            Drv0299_IQSet(DeviceMap, IOHandle,1);     /* Set to Inverted */

            Drv0299_CheckData(DeviceMap, IOHandle, Params);
            
        break;
        case STTUNER_IQ_MODE_AUTO:
            
            if (Drv0299_CheckData(DeviceMap, IOHandle, Params) == E299_NODATA) /* Check for data */
            {
                Drv0299_IQInvertion(DeviceMap, IOHandle);       /* Invert I and Q */
                
                if(Drv0299_CheckData(DeviceMap, IOHandle, Params) == E299_NODATA)    /* Check for data */
                {
                    Drv0299_IQInvertion(DeviceMap, IOHandle);   /* Invert I and Q */
                }
            }
            
        break;
        default:
            #ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
                STTBX_Print(("%s Drv0299 SearchData inavlid IQ Mode\n"));
            #endif
        break;

    }

    return(Params->State);
    
}

/*----------------------------------------------------
 FUNCTION      SearchData
 ACTION        Search for data
 PARAMS IN     Params->Tdata => Time to wait for data
 PARAMS OUT    Params->State => Result of the search
 RETURN        E299_NODATA if data not founded, E299_DATAOK otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_SearchData(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result, STTUNER_Handle_t TopLevelHandle)
{
    short int DerotFreq;
    short int DerotStep;
    short int DerotLimit;
    short int NextLoop = 2;
    int  index = 1;
    U8 cfrm[2];
    long CurrentFreq;
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;
    
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;

    DerotStep  = (short int)( (Params->SymbolRate/4L) / Params->Mclk); /* cast to eliminate compiler warning --SFS */
    DerotLimit = (short int)( (Params->SubRange/2L)   / Params->Mclk); /* cast to eliminate compiler warning --SFS */
    DerotFreq  = Params->DerotFreq;
    CurrentFreq = Params->Frequency + ( ((long)(DerotFreq)) * Params->Mclk)/1000;

    do
    {
        /* Try the terminate check here */
        if(Inst[TopLevelHandle].ForceSearchTerminate==TRUE)
              break;
        
        if((Params->State != E299_CARRIEROK)||(Drv0299_Searchlock(DeviceMap, IOHandle, Params, Result, TopLevelHandle)!=E299_DATAOK))
        {
	        DerotFreq   = DerotFreq + ((int)Params->TunerIQSense * index * Params->Direction * DerotStep);   /* Compute the next derotator position for the zig zag */
	        
	
	        if(MAC0299_ABS(DerotFreq) > DerotLimit) NextLoop--;
	
	        if(NextLoop)
	        {
	            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_CFD_ALGO, 1);
	
	            cfrm[0]=MAC0299_MSB(DerotFreq);
	            cfrm[1]=MAC0299_LSB(DerotFreq);
	            STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle, R0299_CFRM, cfrm,2);    /* Reset the derotator */
			
		    SystemWaitFor(Params->Ttiming);
	            Drv0299_CheckCarrier(DeviceMap, IOHandle, Params);
	
	            index++;
	        }
	}
        Params->Direction = -Params->Direction; /* Change the zigzag direction */
    }
    while((Params->State != E299_DATAOK) && NextLoop);


    if(Params->State == E299_DATAOK)
    {
        STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle,  R0299_CFRM, 2,cfrm);   /* Get the derotator frequency */
        /* Multiply DerotFreq with IQ wiring sense of tuner & Demod */
        Params->DerotFreq = (short int)Params->TunerIQSense * (short int)MAC0299_MAKEWORD( cfrm[0],cfrm[1] );
        
    }


    return(Params->State);
}

#endif
#ifdef STTUNER_MINIDRIVER
D0299_SignalType_t Drv0299_Searchlock(D0299_SearchParams_t *Params)
{
	switch(Params->DemodIQMode)
        {
        case STTUNER_IQ_MODE_NORMAL:

            Drv0299_IQSet(0);       /* Set to Normal */
            
            Drv0299_CheckData(Params);
        break;
        
        case STTUNER_IQ_MODE_INVERTED:

            Drv0299_IQSet(1);     /* Set to Inverted */

            Drv0299_CheckData(Params);
            
        break;
        case STTUNER_IQ_MODE_AUTO:
            
            if (Drv0299_CheckData(Params) == E299_NODATA) /* Check for data */
            {
                Drv0299_IQInvertion();       /* Invert I and Q */
                
                if(Drv0299_CheckData(Params) == E299_NODATA)    /* Check for data */
                {
                    Drv0299_IQInvertion();   /* Invert I and Q */
                }
            }
            
        break;
        default:
            #ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
                STTBX_Print(("%s Drv0299 SearchData inavlid IQ Mode\n"));
            #endif
        break;

    }
   
    return(Params->State);
}

D0299_SignalType_t Drv0299_SearchData(D0299_SearchParams_t *Params)
{
    short int DerotFreq;
    short int DerotStep;
    short int DerotLimit;
    short int NextLoop = 2;
    int  index = 0;
    U8 nsbuffer[2];
    long CurrentFreq;
    U8 Data;
    DerotStep  = (short int)( (Params->SymbolRate/4L) / Params->Mclk); /* cast to eliminate compiler warning --SFS */
    DerotLimit = (short int)( (Params->SubRange/2L)   / Params->Mclk); /* cast to eliminate compiler warning --SFS */
    DerotFreq  = Params->DerotFreq;
    CurrentFreq = Params->Frequency + ( ((long)(DerotFreq)) * Params->Mclk)/1000;
    do
    {
        if((Params->State != E299_CARRIEROK)||(Drv0299_Searchlock(Params)!=E299_DATAOK))
        {
        	index++;
        	DerotFreq   = DerotFreq + ((int)Params->TunerIQSense * index * Params->Direction * DerotStep);   /* Compute the next derotator position for the zig zag */
	        
            if(MAC0299_ABS(DerotFreq) > DerotLimit) NextLoop--;
            
            if(NextLoop)
            {
                Data = 1;
                STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_CFD, F0299_CFD_ALGO, F0299_CFD_ALGO_L, &Data, 1, FALSE);
                nsbuffer[1] = MAC0299_LSB(DerotFreq);
    	        nsbuffer[0] = MAC0299_MSB(DerotFreq);
    	        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_CFRM, 0, 0, nsbuffer, 2, FALSE);
    	        SystemWaitFor(5);
    	        Drv0299_CheckCarrier(Params);
            }
	}
        Params->Direction = -Params->Direction; /* Change the zigzag direction */
    }
    while((Params->State != E299_DATAOK) && NextLoop);


    if(Params->State == E299_DATAOK)
    {
       STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_CFRM, 0, 0, nsbuffer, 2, FALSE);
        /* Multiply DerotFreq with IQ wiring sense of tuner & Demod */
        Params->DerotFreq = (short int)Params->TunerIQSense * (short int)MAC0299_MAKEWORD( nsbuffer[0],nsbuffer[1] );
    }
    
    return(Params->State);
    
}


D0299_SignalType_t Drv0299_CheckRange(D0299_SearchParams_t *Params)
{
	int	RangeOffset;
	int	TransponderFrequency;

	
    RangeOffset =          (int)(Params->SearchRange / 2000);  /* cast to eliminate compiler warning --SFS */
    TransponderFrequency = (int)(Params->Frequency + (Params->DerotFreq * Params->Mclk)/1000); /* cast to eliminate compiler warning --SFS */

	if( (TransponderFrequency >= Params->Frequency - RangeOffset) &&
        (TransponderFrequency <= Params->Frequency + RangeOffset) )
        Params->State = E299_RANGEOK;
    else
        Params->State = E299_OUTOFRANGE;

	return(Params->State);
}

#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION        CheckRange
 ACTION        Check if the founded frequency is in the correct range
 PARAMS IN        Params->BaseFreq =>
 PARAMS OUT    Params->State    =>    Result of the check
 RETURN        E299_RANGEOK if check success, E299_OUTOFRANGE otherwise
------------------------------------------------------*/
D0299_SignalType_t Drv0299_CheckRange(D0299_SearchParams_t *Params, D0299_Searchresult_t *Result)
{
	int	RangeOffset;
	int	TransponderFrequency;

	
    RangeOffset =          (int)(Params->SearchRange / 2000);  /* cast to eliminate compiler warning --SFS */
    TransponderFrequency = (int)(Params->Frequency + (((long)(Params->DerotFreq)) * Params->Mclk)/1000); /* cast to eliminate compiler warning --SFS */

	if( (TransponderFrequency >= Params->BaseFreq - RangeOffset) &&
        (TransponderFrequency <= Params->BaseFreq + RangeOffset) )
        Params->State = E299_RANGEOK;
    else
        Params->State = E299_OUTOFRANGE;

	return(Params->State);
}

/*----------------------------------------------------
 FUNCTION      CarrierNotCentered
 ACTION        Check if the carrier is correctly centered
 PARAMS IN 
 PARAMS OUT
 RETURN        1 if not centered, 0 otherwise
------------------------------------------------------*/
int Drv0299_CarrierNotCentered(STTUNER_IOREG_DeviceMap_t *DeviceMap, D0299_SearchParams_t *Params, int AllowedOffset)
{
    int Fs;
    int DerotFreq;
    int NotCentered = 0;

    Fs = (int)(Drv0299_CarrierWidth(Params->SymbolRate, Params->RollOff));  /* cast to eliminate compiler warning --SFS */

    DerotFreq = MAC0299_ABS( (int)Params->DerotFreq * (int)Params->Mclk );  /* cast with macro to eliminate compiler warning --SFS */

    if( (Params->TunerIF == 0) && (Fs < 4000000) )
        NotCentered = (int)( (Params->TunerBW - Fs) / 4);   /* cast to eliminate compiler warning --SFS */

    else
        NotCentered = (( (int)Params->TunerBW/2 - DerotFreq - Fs/2) < AllowedOffset) &&
                         (DerotFreq > Params->TunerStep);  /* cast to eliminate compiler warning --SFS */

    return(NotCentered);
}



/*----------------------------------------------------
 FUNCTION      TunerCentering
 ACTION        Optimisation of the tuner position to allow frequency variations
 PARAMS IN     Params
 PARAMS OUT    Result
 RETURN        E299_DATAOK if success
------------------------------------------------------*/
D0299_SignalType_t Drv0299_TunerCentering(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result, int AllowedOffset,STTUNER_Handle_t TopLevelHandle )
{
    int	DerotFreq, MinOffset/*, MaxOffset*/;
    int	CentringFrequency;
    int	TransponderFrequency;
    int	Fs, sign;
    U8 Derottemp[2];
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;
    

    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;    
    
    Fs = (int)Drv0299_CarrierWidth(Params->SymbolRate, Params->RollOff);    /* cast to eliminate compiler warning --SFS */
    DerotFreq = MAC0299_ABS( (int)Params->DerotFreq * (int)Params->Mclk );  /* cast to eliminate compiler warning --SFS */

    if((Params->TunerIF == 0) && (Params->SymbolRate <= 6000000))
		MinOffset = CARRIEROFFSET;
	else
		MinOffset = 0;
	
	/*MaxOffset = MAC0299_MAX( (int)(Params->TunerBW)/2 - Fs/2- AllowedOffset , MinOffset + (int)(Params->TunerStep) );*/   /* casts to eliminate compiler warning --SFS */

	/* Do tuner retuning if derotator frequency offset is more than 1MHz */
	if( ((DerotFreq >= 1000000) && (Params->SymbolRate > 6000000))  /*|| (Drv0299_CheckData(DeviceMap, IOHandle, Params) != E299_DATAOK)*/ )
	{
		sign = (Params->DerotFreq > 0) ? 1: -1; 
		TransponderFrequency = (int)(Params->Frequency + (Params->DerotFreq * Params->Mclk)/1000);  /* cast to eliminate compiler warning --SFS */
		CentringFrequency    = TransponderFrequency + MinOffset;
		
        Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, CentringFrequency, (U32 *)&CentringFrequency);
 
        Drv0299_WaitTuner(TunerInstance, 100); /*	Is tuner Locked	? (wait 100 ms maxi) */

		/* Reset Derotator	*/
		Params->DerotFreq = (short int)Params->TunerIQSense * (short int)(((TransponderFrequency - CentringFrequency)*1000)/(int)Params->Mclk); /* casts to eliminate compiler warning --SFS */
		Params->Frequency = CentringFrequency;
		Params->Direction = 1;

		Derottemp[0]= MAC0299_MSB(Params->DerotFreq);
	        Derottemp[1]= MAC0299_LSB(Params->DerotFreq);
	        STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle, R0299_CFRM,Derottemp, 2); 
			
        /* Wait for agc1,agc2 and timing loop */ 
        SystemWaitFor(Params->Tagc1 + Params->Tagc2 + Params->Ttiming);

        /* Search for carrier */  
	if ( Drv0299_SearchCarrier(DeviceMap, IOHandle, Params, Result, TopLevelHandle) == E299_CARRIEROK )
        {
            Drv0299_SearchData(DeviceMap, IOHandle, Params, Result, TopLevelHandle); /* Search for data */ 
        }
	}

	return(Params->State);
}



/*----------------------------------------------------
 FUNCTION      FirstSubRange
 ACTION        Compute the first SubRange of the search
 PARAMS IN     Params->SearchRange
 PARAMS OUT    Params->SubRange
 RETURN        NONE
------------------------------------------------------*/
D0299_SignalType_t Drv0299_FirstSubRange(D0299_SearchParams_t *Params)
{
        Params->SubRange    = MAC0299_MIN(Params->SearchRange,9000000);/* fix to 9MHz to ensure signal not to get out from tuner BW*/
	Params->Frequency   = Params->BaseFreq;
	Params->TunerOffset = 0L;
	Params->SubDir      = 1; 

	if( Params->TunerBW > Drv0299_CarrierWidth(Params->SymbolRate, Params->RollOff)/2 )/* to give support for 30Msym/sec symbol rate and solve bug GNBvd17144*/
	Params->State = E299_BWOK;
        else
        Params->State = E299_BWTOONARROW;

        return(Params->State);
}


 #endif
/*----------------------------------------------------
 FUNCTION      NextSubRange
 ACTION        Compute the next SubRange of the search
 PARAMS IN     Frequency    ->    Start frequency
                 Params->SearchRange
 PARAMS OUT    Params->SubRange
 RETURN        NONE
------------------------------------------------------*/
void Drv0299_NextSubRange(D0299_SearchParams_t *Params)
{
    long OldSubRange;

    if(Params->SubDir > 0)
    {
        OldSubRange = Params->SubRange;
        Params->SubRange = MAC0299_MIN( (Params->SearchRange/2) - (Params->TunerOffset + Params->SubRange/2)  ,  Params->SubRange);
        Params->TunerOffset = Params->TunerOffset + ( (OldSubRange + Params->SubRange)/2 );
     }

    Params->Frequency =  Params->BaseFreq + (Params->SubDir * Params->TunerOffset)/1000;
    Params->SubDir    = -Params->SubDir;
}
#ifndef STTUNER_MINIDRIVER
/* This define is to remain commented out as it is used as an internal test */
/* #define STTUNER_TEST_SEARCH_MODULE_SATDRV_DRV0299 1 */

/*----------------------------------------------------
 FUNCTION      AutoSearchAlgo
 ACTION        Search for Signal, Timing, Carrier and then data at a given Frequency,
                 in a given range
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        Type of the founded signal (if any)
 
 Note: Drv0299_CheckAgc( ) removed, no need because different tuners have different sensitivity
------------------------------------------------------*/
D0299_SignalType_t Drv0299_AutoSearchAlgo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_StateBlock_t *StateBlock, STTUNER_Handle_t TopLevelHandle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
   const char *identity = "STTUNER drv0299.c Drv0299_AutoSearchAlgo()";
#endif
    int errorrate, sn, to, hy;
    D0299_SearchParams_t *Params;
    D0299_Searchresult_t *Result;
    ST_ErrorCode_t Error;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
    U32 Fold;
#endif
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance; 
    TUNSDRV_InstanceData_t *Instance;  
    long TransponderFreq;
    U8 Derottemp[2];
    U8  VsearchCopy;
    
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;
    Instance = TUNSDRV_GetInstFromHandle(TunerInstance->DrvHandle);
    
    Params = &StateBlock->Params;
    Result = &StateBlock->Result;
    Instance->SymbolRate = (U32)Params->SymbolRate;
    Params->BaseFreq = Params->Frequency;
   
   
     /*The following setting is dependent on Symbol Rate*/
   
    /* Values are freezed after tests done in SKY Italia Lab */
     if((Params->SymbolRate>=5000000) && (Params->SymbolRate<=45000000))
     {
	    STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R0299_ACLC, 0x99);
	    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_BETA_CAR, 0x17); 
	 }
     else if((Params->SymbolRate>=1000000) && (Params->SymbolRate<=5000000))
     {
	    STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R0299_ACLC, 0x9B);
	    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_BETA_CAR, 0x13);
	 }
	 
	 
     if(Params->SymbolRate < 2000000)
     STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0299_AGC1_REF, 0x10);
     else
     STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0299_AGC1_REF, 0x14);
	            
    /*PRCopy = STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_PR);*/
    VsearchCopy = STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_VSEARCH);

    /*    Get fields value    */
    sn        = STTUNER_IOREG_FieldExtractVal(VsearchCopy, F0299_SN);
    to        = STTUNER_IOREG_FieldExtractVal(VsearchCopy, F0299_TO);
    hy        = STTUNER_IOREG_FieldExtractVal(VsearchCopy, F0299_H);
    errorrate = STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_PR);


    Params->DerotStep = (short int)(Params->DerotPercent * (Params->SymbolRate/1000L) / Params->Mclk);                  /* saut de DerotStep/1000 * Fsymbol */   /* cast to eliminate compiler warning --SFS */
    Params->Ttiming   = (short int)(Drv0299_CalcTimingTimeConstant(Params->SymbolRate) );                               /* cast to eliminate compiler warning --SFS */
    Params->Tderot    = (short int)(2 + 20 * Drv0299_CalcDerotTimeConstant(Params->SymbolRate) );                       /* cast to eliminate compiler warning --SFS */
    Params->Tdata     = (short int)(2 *      Drv0299_CalcDataTimeConstant(errorrate, sn, to, hy, Params->SymbolRate, 
                                        STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_FECM)) ); /* cast to eliminate compiler warning --SFS */

    /* Reg0299_RegTriggerOff(DeviceMap, IOHandle);     <<<<<< TRIGGER >>>>>> OFF */


    if (Drv0299_FirstSubRange(Params) == E299_BWOK)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
        STTBX_Print(("%s Drv0299_FirstSubRange(Params) == E299_BWOK\n", identity));
#endif
        do
        {
            /* Try the terminate check here */
            if(Inst[TopLevelHandle].ForceSearchTerminate==TRUE)
              break;

		    /* Set the symbol rate again because symbol rate register were updated during timingcenteringloop */
		    Reg0299_RegSetSymbolRate(DeviceMap, IOHandle, Params->SymbolRate);

            /* Code inside define below is purely for internal testing */
            /* Do not set this flag as it causes an infinite loop */
            #ifdef STTUNER_TEST_SEARCH_MODULE_SATDRV_DRV0299
                STTBX_Print(("Algorithm Looping\n"));
                /* Simulate an infinite loop */
                if (Inst[TopLevelHandle].Sat.EnableSearchTest==TRUE)
                {
                    Params->State=E299_OUTOFRANGE;
                    continue;
                }
            #endif /* STTUNER_TEST_SEARCH_MODULE_SATDRV_DRV0299 */
                
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_RTF,      0);
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_CFD_ALGO, 1);

            Params->DerotFreq = 0;
            Params->State     = E299_NOAGC1;
            
            /* added to improve scanning -> GNBvd26185:Set maximun subrange 9MHz,*/
            /* i.e. tuner will retuned with every 9Mhz will increase reliability of scanning to found a channel*/
            if (!Inst[TopLevelHandle].Sat.ScanExact)
	    Params->SubRange    = MAC0299_MIN(Params->SubRange,9000000);
	    else
	    Params->SubRange    = MAC0299_MIN(Params->SubRange,6000000);
	    
          /*    Reg0299_RegTriggerOn(DeviceMap, IOHandle);     <<<<<< TRIGGER >>>>>> ON */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
            Fold = (U32)Params->Frequency;
#endif

		TransponderFreq = Params->Frequency ;
		
           /* Add 2 MHz of offset in tuner setting at low symbol rate due to tuner LPF limitation*/
		   if((Params->TunerIF == 0) && (Params->SymbolRate <= 6000000))
		   {
			   Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, (U32)Params->Frequency + CARRIEROFFSET, (U32 *)&Params->Frequency);
		   }
		   else
		   {
			   Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, (U32)Params->Frequency, (U32 *)&Params->Frequency);
		   }
           
          Params->FreqOffset = (TransponderFreq - Params->Frequency)*1000;
          Params->DerotFreq  = (short int)((Params->TunerIQSense)*(Params->FreqOffset/Params->Mclk));
           
		  /* Activate the carrier offset detection algo & reset the derotator freq registers*/
    	  STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_CFD_ALGO, 1);
          Derottemp[0]= MAC0299_MSB(Params->DerotFreq);
		  Derottemp[1]= MAC0299_LSB(Params->DerotFreq);
		  STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle, R0299_CFRM,Derottemp, 2);    /* Reset of the derotator frequency */


#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
        STTBX_Print(("%s Fold: %u,  Fnew: %u\n", identity, Fold, (U32)Params->Frequency));
#endif



                                 
	            /* Update symbol rate register if there is any timing offset in symbol rate->GNBvd28396*/
	            Result->SymbolRate = Drv0299_CenterTimingLoop(DeviceMap, IOHandle, Params->SymbolRate, Params->MasterClock);	       
	    
	            /* There is signal in the band */
	            if( Params->SymbolRate <= Params->TunerBW/2L )  /* if symbolrate<=TunerBW/2 i.e. SCPC Mode and it should go for searchtiming     */       
	            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
                    STTBX_Print(("%s Drv0299_SearchTiming()\n", identity));
#endif
                    Drv0299_SearchTiming(DeviceMap, IOHandle, Params, Result, TopLevelHandle);  /* For low rates (SCPC) */
                }
                else
                {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
                    STTBX_Print(("%s Drv0299_CheckTiming()\n", identity));
#endif
                    Drv0299_CheckTiming(DeviceMap, IOHandle, Params);           /* For high rates (MCPC) */
                    if(Params->State != E299_TIMINGOK)
                    Drv0299_SearchTiming(DeviceMap, IOHandle, Params, Result, TopLevelHandle);   /* recommended For low symbol rates  */
                } 
                if(Params->State == E299_TIMINGOK)
                {   
                	
                    if(Drv0299_SearchCarrier(DeviceMap, IOHandle, Params, Result, TopLevelHandle) == E299_CARRIEROK)     /* Search for carrier */
                    {
                    	  /*Drv0299_TunerCentering(DeviceMap, IOHandle, Params, Result, 200000, TopLevelHandle);*/
                    	  if(Drv0299_SearchData(DeviceMap, IOHandle, Params, Result, TopLevelHandle) == E299_DATAOK)  /* Check for data */
                            {
                            	
	                            /* Tuner centering should be done after first sync found, it will improve performance */
	                            /*Drv0299_TunerCentering(DeviceMap, IOHandle, Params, Result, 200000, TopLevelHandle);*/
	                            if(Drv0299_CheckRange(Params, Result) == E299_RANGEOK)
                                {
                                	
                                	Result->Frequency    = Params->Frequency + (((long)(Params->DerotFreq))*(Params->Mclk)/1000);
                                	Result->PunctureRate = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_CPR);
                                }
                            else
                            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
                                STTBX_Print(("%s fail Drv0299_CheckRange() != E299_RANGEOK\n", identity));
#endif
                            }
                         }
                        else
                        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
                            STTBX_Print(("%s fail Drv0299_SearchData() != DEF0299_SCAN\n", identity));
#endif
                        }
                    }
                    else
                    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
                        STTBX_Print(("%s fail Drv0299_SearchCarrier() != E299_CARRIEROK\n", identity));
#endif
                    }
                
                }
                else
                {
	                /* This is to find the offset (if timing not found) from actual symbol rate and then set symbol frequency register with new symbol rate(symbol rate+offset)*/
	         
	                Result->SymbolRate = Drv0299_CenterTimingLoop(DeviceMap, IOHandle, Params->SymbolRate, Params->MasterClock);
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
                    STTBX_Print(("%s fail Params->State != E299_TIMINGOK\n", identity));
#endif
                }
           
            if(Params->State != E299_RANGEOK) Drv0299_NextSubRange(Params);

         /*     Reg0299_RegTriggerOff(DeviceMap, IOHandle);    <<<<<< TRIGGER >>>>>> OFF */
                
        }
        while(Params->SubRange && Params->State != E299_RANGEOK );

    }   /* if (Drv0299_FirstSubRange(Params) == E299_BWOK) */
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
        STTBX_Print(("%s fail Drv0299_FirstSubRange(Params) != E299_BWOK\n", identity));
#endif
    }


    Result->SignalType = Params->State;

    return(Params->State);
}



/*----------------------------------------------------
 FUNCTION      CarrierTracking
 ACTION        Track the carrier
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        New frequency of the carrier
------------------------------------------------------*/
int Drv0299_CarrierTracking(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_StateBlock_t *StateBlock, STTUNER_Handle_t TopLevelHandle)
{
    
    if(StateBlock->Result.SignalType == E299_RANGEOK)
    {
        Drv0299_TunerCentering(DeviceMap, IOHandle, &StateBlock->Params, &StateBlock->Result, 2000000, TopLevelHandle);
    }

    return( (int)StateBlock->Params.Frequency );
}
#endif


/*----------------------------------------------------
 FUNCTION      GetNoiseEstimator
 ACTION
 PARAMS IN
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
void Drv0299_GetNoiseEstimator(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 *pNoise, U32 *pBer)
{
    U32 LutHigh, LutLow, Real, RealLow, RealHigh, CN, Ber;
    DRV0299_LUT_t *Lut;
    U8 NoiseIndicator[2];
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_NIRH, 2,NoiseIndicator);
    Real = MAC0299_MAKEWORD(  NoiseIndicator[0],   NoiseIndicator[1] );
                           /*MSB,LSB*/
    /* S/N */
    Lut = (DRV0299_LUT_t *)SignalNoiseLUT;

    while (Lut->Real >= Real && Lut->Real != ((U32)-1))
    {
        Lut++;
    }

    if (Lut->Real != ((U32)-1))
    {
        RealHigh = (U32)(Lut-1)->Real;
        LutLow   = (U32)(Lut-1)->Lookup;
        RealLow  = (U32)Lut->Real;
        LutHigh  = (U32)Lut->Lookup;
        CN       = DRV0299_INTERPOLATE(-LutLow, -LutHigh, -RealLow, -RealHigh, -Real);
    }
    else
    {
        CN = 0;
    }


    /* BER */
    Lut = (DRV0299_LUT_t *)BerLUT;
    
    while (Lut->Real >= Real && Lut->Real != ((U32)-1))
    {
        Lut++;
    }
    
    if (Lut->Real != ((U32)-1))
    {
        RealHigh = (U32)(Lut-1)->Real;
        RealLow  = (U32) Lut->Real;
        LutHigh  = (U32)(Lut-1)->Lookup;
        LutLow   = (U32) Lut->Lookup;
        Ber      = DRV0299_INTERPOLATE(LutLow, LutHigh, RealLow, RealHigh, Real);
    }
    else
    {
        Ber = 200000;
    }

    *pBer   = Ber;
    *pNoise = -CN;
}
#endif

#ifdef STTUNER_MINIDRIVER

void Drv0299_GetNoiseEstimator(U32 *pNoise, U32 *pBer)
{
    U32 LutHigh, LutLow, Real, RealLow, RealHigh, CN, Ber;
    DRV0299_LUT_t *Lut;

    U8 nsbuffer[2];
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_NIRH, 0, 0, nsbuffer, 2, FALSE);
    
    Real = MAC0299_MAKEWORD( nsbuffer[0], nsbuffer[1] );

    /* S/N */
    Lut = (DRV0299_LUT_t *)SignalNoiseLUT;

    while (Lut->Real >= Real && Lut->Real != ((U32)-1))
    {
        Lut++;
    }

    if (Lut->Real != ((U32)-1))
    {
        RealHigh = (U32)(Lut-1)->Real;
        LutLow   = (U32)(Lut-1)->Lookup;
        RealLow  = (U32)Lut->Real;
        LutHigh  = (U32)Lut->Lookup;
        CN       = DRV0299_INTERPOLATE(-LutLow, -LutHigh, -RealLow, -RealHigh, -Real);
    }
    else
    {
        CN = 0;
    }


    /* BER */
    Lut = (DRV0299_LUT_t *)BerLUT;
    
    while (Lut->Real >= Real && Lut->Real != ((U32)-1))
    {
        Lut++;
    }
    
    if (Lut->Real != ((U32)-1))
    {
        RealHigh = (U32)(Lut-1)->Real;
        RealLow  = (U32) Lut->Real;
        LutHigh  = (U32)(Lut-1)->Lookup;
        LutLow   = (U32) Lut->Lookup;
        Ber      = DRV0299_INTERPOLATE(LutLow, LutHigh, RealLow, RealHigh, Real);
    }
    else
    {
        Ber = 200000;
    }

    *pBer   = Ber;
    *pNoise = -CN;
}
#endif
#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DRV_SAT_SCR

U32 Drv0299_SpectrumAnalysis(STTUNER_tuner_instance_t *TunerInstance, STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 StartFreq,U32 StopFreq,U32 StepSize,U32 *ToneList,U8 mode,int* power_detection_level)
{ 
    U32 freq, realTunerFreq, freqstep = 2000000,
	tempfreq,lastfreq,freqlowerthreshold, freqhigherthreshold,epsilon;
    int  i=0, j = 1,pt_max;
    int  agcVal[500],index=0,points;
    int direction = 1, agc_threshold, *spectrum_agc ,agc_threshold_adjust,agc_high,agc_low,agc_threshold_detect;
    U32 *spectrum;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    TUNSDRV_InstanceData_t *Instance;

	points = (U16)((StopFreq - StartFreq)/StepSize + 1);
	spectrum = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, points, sizeof(U16));
	spectrum_agc = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, points, sizeof(int));
	
	Instance = TUNSDRV_GetInstFromHandle(TunerInstance->DrvHandle);
	if(Instance->TunerType == STTUNER_TUNER_STB6000)
	STTUNER_IOREG_SetField(&(Instance->DeviceMap),Instance->IOHandle, FSTB6000_G,0);		/* Gain = 0 */
	else if(Instance->TunerType == STTUNER_TUNER_MAX2116) 
	STTUNER_IOREG_SetField(&(Instance->DeviceMap),Instance->IOHandle, FMAX2116_G,0x1F);
	
	/*Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, 0, (Instance->DeviceMap).Registers);*/
	    		
	Error |= STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_AGC1_REF, 16); /* set AGC_Ref as 26*/
	lastfreq = StartFreq - 100000000;
	
	/*added by fsu*/
	/*auto detection of max AGC*/
        if(mode==1) 
        {
	 agc_high=-500;
	 agc_low=500;
	 for(freq=StartFreq;freq<StopFreq;freq+=3000000)
	 {
	  Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, freq/1000, (U32 *)&realTunerFreq);
	  STOS_TaskDelayUs(2000);
	  agc_threshold = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_AGCINTEGRATORVALUE);
	  agcVal[index]=agc_threshold;
	  index++;
	  if(agc_threshold>agc_high) {
	     agc_high=agc_threshold;
	  }
	  if(agc_threshold<agc_low) {
	     agc_low=agc_threshold;
	  }
         };
        
	 /*adjust agc_threshold*/
	 points=0;
	 agc_threshold_adjust=(agc_high+agc_low)/2;
	 pt_max=3*index/4;
	 epsilon=agc_high-agc_low;
	 
	 while(epsilon>5) {
	  points=0;
	  /*get numbers of points for which treshold is lower than agc_threshold_adjust*/		
	  for(i=0;i<index;i++) {
	   if(agcVal[i]<agc_threshold_adjust) {
	   points++;
	   }
	  }
	  if(points>pt_max) { 
	  	/*most of points are bellow agc_threshold_adjust level*/
	  	agc_high=agc_threshold_adjust;
	  } else {
	  	agc_low=agc_threshold_adjust;
	  }
	  agc_threshold_adjust=(agc_high+agc_low)/2;
	  epsilon=agc_high-agc_low;
	 }
	  agc_threshold_detect=agc_threshold_adjust;
	  *power_detection_level=agc_threshold_adjust;
	} else {
	  agc_threshold_detect=*power_detection_level; /*quick setup*/
	}
	
	i=0;
	points=0;
	
	for(freq=StartFreq;freq<StopFreq;freq+=StepSize)
	{
		direction = 1;j = 1;
		Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, freq/1000, (U32 *)&realTunerFreq);
		STOS_TaskDelayUs(2000);
		agc_threshold = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_AGCINTEGRATORVALUE);
		if(agc_threshold > agc_threshold_detect)/* - 70 */
		{
				if(((agc_threshold >= agc_threshold_detect) && (MAC0299_ABS((S32)(freq - lastfreq)))>40000000))
				{	
					while(agc_threshold > agc_threshold_detect)/* -128*/
					{
						tempfreq = freq+(freqstep)*direction * j/* *2 */;
						Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, tempfreq/1000, (U32 *)&realTunerFreq);
						STOS_TaskDelayUs(2000);
						agc_threshold = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_AGCINTEGRATORVALUE);
						++j;
				        }
				        freqhigherthreshold = tempfreq;
				        direction *= -1;
				        agc_threshold = 0; j = 1;
				        do
					{
						tempfreq = freq+(freqstep)*direction * j/* *2 */;
						Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, tempfreq/1000, (U32 *)&realTunerFreq);
						STOS_TaskDelayUs(2000);
						agc_threshold = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_AGCINTEGRATORVALUE);
						++j;
				        }while( agc_threshold> agc_threshold_detect);
				        freqlowerthreshold = tempfreq;

				        spectrum[i] = freqlowerthreshold + (freqhigherthreshold - freqlowerthreshold)/2;
				        if(spectrum[i] >= StartFreq && spectrum[i] <= StopFreq)
				        {
				        	lastfreq = spectrum[i];
				        	Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, spectrum[i]/1000, (U32 *)&realTunerFreq);
						STOS_TaskDelayUs(2000);
						spectrum_agc[i] = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_AGCINTEGRATORVALUE);
				        
				        	++i;
				        	
				        }
				        
					
				}	
		}
				
	}
	
	if(mode==1) {
	 agc_threshold=agc_threshold_detect;
	 for(j = 0; j<i; ++j) {
			ToneList[points] = spectrum[j];
			points++;
			if(points>=8)
			break;
	 }
	} else {

	 for(j = 0; j<i; ++j) {
			ToneList[points] = spectrum[j];
			points++;
			if(points>=8)
			break;
	 }		
		
	  
	}
        
	
	STOS_MemoryDeallocate(DeviceMap->MemoryPartition, spectrum);
	STOS_MemoryDeallocate(DeviceMap->MemoryPartition, spectrum_agc);
	/*memory_deallocate(DeviceMap->MemoryPartition, agcVal);*/

	return points;		
}
#endif
#endif
#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      GetPowerEstimator
 ACTION
 PARAMS IN
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
S16 Drv0299_GetPowerEstimator(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    S32 LutHigh, LutLow, Real, RealLow, RealHigh;
    DRV0299_LUT_t *Lut;
    S16 Power;
    
    Real = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_AGCINTEGRATORVALUE);

    /* AGC */
    Lut = (DRV0299_LUT_t *)AgcLUT;

    while (Lut->Real >= Real && Lut->Real != ((U32)-1))
    {
        Lut++;
    }
    
    if (Lut->Real != ((U32)-1))
    {
        RealHigh = (Lut-1)->Real;
        LutLow   = Lut->Lookup;
        RealLow  = Lut->Real;
        LutHigh  = (Lut-1)->Lookup;
        Power    = (S16)DRV0299_INTERPOLATE(LutLow, LutHigh, RealLow, RealHigh, Real);
    }
    else
    {
        Power = 0;
    }
    
    return(Power-100); /* take -10 dBm pf cable loss in account*/
}

/*----------------------------------------------------
 FUNCTION Drv0299_TurnOnInfinteLoop
 ACTION
 PARAMS 
 PARAMS     
 RETURN        
------------------------------------------------------*/
void Drv0299_TurnOnInfiniteLoop(STTUNER_Handle_t Handle, BOOL *OnOrOff)
{

    STTUNER_InstanceDbase_t  *Inst;             /* pointer to instance database */
    

    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();
    

    /* Set the test search variable using the top level handle */
    Inst[Handle].Sat.EnableSearchTest=*OnOrOff;

    
    return;
    
}
#endif

#ifdef STTUNER_MINIDRIVER
long Reg0299_RegSetSymbolRate(long SymbolRate)
{
    unsigned long Result;
    long MasterClock, n1, n2;
    int i = 0;
    U8 nsbuffer[3];

    MasterClock = 88000000;
    n1 = SymbolRate;
    n2 = MasterClock;
    while(i <= 20)   /* n1 > 0	*/
    {
        if(n1<n2)
        {
            Result *= 2;      
            n1 *= 2;
        }
        else
        {
            Result = Result * 2 + 1;
            n1 = (n1-n2) * 2;
        }
        i++;
    }

    /*Result = STTUNER_Util_BinaryFloatDiv(SymbolRate, MasterClock, 20);*/
    nsbuffer[2] = (U8)(Result & 0x0F)<<4;
    nsbuffer[1] = (U8)(Result>>4) & 0xFF;
    nsbuffer[0] = (U8)(Result>>12) & 0xFF;
    
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_SFRH, 0, 0, nsbuffer, 3, FALSE);
	
    return(SymbolRate);
}

/*----------------------------------------------------
 FUNCTION      AutoSearchAlgo
 ACTION        Search for Signal, Timing, Carrier and then data at a given Frequency,
                 in a given range
 PARAMS IN     NONE
 PARAMS OUT    NONE
 RETURN        Type of the founded signal (if any)
 
 Note: Drv0299_CheckAgc( ) removed, no need because different tuners have different sensitivity
------------------------------------------------------*/
D0299_SignalType_t Drv0299_AutoSearchAlgo(U32 Frequency, U32 SymbolRate,  D0299_SearchParams_t *Params)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
   const char *identity = "STTUNER drv0299.c Drv0299_AutoSearchAlgo()";
#endif
   
    ST_ErrorCode_t Error;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DRV0299
    U32 Fold;
#endif
   
    U32 TransponderFreq;
    U32 ReturnFrequency;
    U8 nsbuffer[5], Data;
       
    Params->Frequency = Frequency;
    Params->BaseFreq = Frequency;
    Params->SymbolRate = SymbolRate;
    Params->SubDir      = 1; 
    Params->TunerOffset = 0L;
    Params->Mclk = 1342;   /*88000000/65536L*/
    Params->MasterClock = 88000000;
    Params->SubRange = 8000000;
   
     /* Values are freezed after tests done in SKY Italia Lab */
     if((SymbolRate>=5000000) && (SymbolRate<=45000000))
     {	    Data = 0x99;
	    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_ACLC, 0, 0, &Data, 1, FALSE);
	    Data = 0x17;
	    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_BCLC, F0299_BETA_CAR, F0299_BETA_CAR_L, &Data,1, FALSE);
     }	
     else if((SymbolRate>=1000000) && (SymbolRate<=5000000))
     {
	    Data = 0x9B;
	    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_ACLC, 0, 0, &Data, 1, FALSE);
	    Data = 0x13;
	    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_BCLC, F0299_BETA_CAR, F0299_BETA_CAR_L, &Data,1, FALSE);
     } 
	 
     if(SymbolRate < 6000000)
     {
     	Data =0XA;
     	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_AGC1R,F0299_AGC1_REF, F0299_AGC1_REF_L, &Data, 1, FALSE);
     }
     else
     {
     	Data = 0x14;
     	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_AGC1R, F0299_AGC1_REF, F0299_AGC1_REF_L, &Data, 1, FALSE);
     }

       do
        {
		/* Set the symbol rate again because symbol rate register were updated during timingcenteringloop */
		Reg0299_RegSetSymbolRate(SymbolRate);
		Data = 0;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_RTF, F0299_RTF, F0299_RTF_L, &Data, 1, FALSE);
		Params->DerotFreq = 0;
		Params->State     = E299_NOAGC1;
		Params->TunerIQSense = 1;
		Params->SearchRange = 10000000;
		TransponderFreq = Frequency;
		/* Add 2 MHz of offset in tuner setting at low symbol rate due to tuner LPF limitation*/
		if(SymbolRate <= 2000000)
		{
		   Error = tuner_tunsdrv_SetFrequency((Frequency+CARRIEROFFSET), &ReturnFrequency, (U32)Params->SymbolRate);
		}
		else
		{
		   Error = tuner_tunsdrv_SetFrequency(Frequency, &ReturnFrequency, (U32)Params->SymbolRate);
		}
		
		if(Tuner_GetStatus() == FALSE)
		{
			Params->State = E299_NOAGC1;
			return Params->State;
		}
		
		Params->FreqOffset = ((long)((U32)(TransponderFreq - ReturnFrequency))*((U32)1000));
		Params->DerotFreq  = (short int)((Params->TunerIQSense)*(Params->FreqOffset/Params->Mclk));
           
		/* Activate the carrier offset detection algo & reset the derotator freq registers*/
		Data = 1;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_CFD, F0299_CFD_ALGO, F0299_CFD_ALGO_L, &Data, 1, FALSE);
		nsbuffer[1] = MAC0299_LSB(Params->DerotFreq);
		nsbuffer[0] = MAC0299_MSB(Params->DerotFreq);
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_CFRM, 0, 0, nsbuffer, 2, FALSE);
    	        /**********Delay of 200K Symbols************************/
		STOS_TaskDelayUs(((200000/(U32)(Params->SymbolRate/1000)))*1000);
		
		Drv0299_CheckTiming(Params);           /* For high rates (MCPC) */
		if(Params->State != E299_TIMINGOK)
		Drv0299_SearchTiming(Params);   /* recommended For low symbol rates  */
                if(Params->State == E299_TIMINGOK)
                {   
                    if(Drv0299_SearchCarrier(Params) == E299_CARRIEROK)     /* Search for carrier */
                    { 
                    	if(Drv0299_SearchData(Params) == E299_DATAOK)  /* Check for data */
                        {
                            	if(Drv0299_CheckRange(Params) == E299_RANGEOK)
                                {
                                	Frequency = Frequency + (U32)((Params->DerotFreq*(Params->Mclk)/1000));
                                }
                        }
                     }
                    
                }

            if(Params->State != E299_RANGEOK) Drv0299_NextSubRange(Params);
                
        }
        while(Params->SubRange && Params->State != E299_RANGEOK );


    return(Params->State);
}

#endif
#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DRV_SAT_SCR
U32 Drv0299_ToneDetection(STTUNER_IOREG_DeviceMap_t *DeviceMap,STTUNER_Handle_t TopLevelHandle,U32 StartFreq,U32 StopFreq,U32 *ToneList,U8 mode,int* power_detection_level)
{
	U32     step;		/* Frequency step */ 
		
	U32     BandWidth;
	U8	nbTones=0;
	STTUNER_InstanceDbase_t *Inst;
	ST_ErrorCode_t Error = ST_NO_ERROR;

	STTUNER_tuner_instance_t *TunerInstance;
        Inst = STTUNER_GetDrvInst();
       /* get the tuner instance for this driver from the top level handle */
       TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;
	
	/* wide band acquisition */
	step = 36000000;						/* use 36MHz step */
	if(TunerInstance->Driver->ID == STTUNER_TUNER_STB6000)
	{
		step = 9000000;
		Error = (TunerInstance->Driver->tuner_SetBandWidth)(TunerInstance->DrvHandle, 12000, &BandWidth); /* LPF cut off freq = 4+5+5MHz*/
	}
	else if(TunerInstance->Driver->ID == STTUNER_TUNER_MAX2116)
	Error = (TunerInstance->Driver->tuner_SetBandWidth)(TunerInstance->DrvHandle, 36000, &BandWidth); /* LPF cut off freq = 18MHz*/
	else if(TunerInstance->Driver->ID == STTUNER_TUNER_HZ1184)
	Error = (TunerInstance->Driver->tuner_SetBandWidth)(TunerInstance->DrvHandle, 36000, &BandWidth); /* LPF cut off freq = 18MHz*/
	else 		
	Error = (TunerInstance->Driver->tuner_SetBandWidth)(TunerInstance->DrvHandle, 36000, &BandWidth); /* LPF cut off freq = 18MHz*/
	
	nbTones = Drv0299_SpectrumAnalysis(TunerInstance, DeviceMap, Inst[TopLevelHandle].Sat.Demod.IOHandle, StartFreq, StopFreq, step, ToneList,mode,power_detection_level);	/* spectrum acquisition */
		
	return nbTones;
}
#endif
#endif
/* End of drv0299.c */


