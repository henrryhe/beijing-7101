/* ----------------------------------------------------------------------------
File Name: reg0370qam.c 

Description:

Copyright (C) 2005-2006 STMicroelectronics

   date: 
version: 
 author: 
comment: 

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
/* C libs */
#include <string.h>
#endif
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"

#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "ioarch.h"     /* IO access layer */
#include "ioreg.h"      /* register mapping */

#include "d0370qam.h"     /* top level driver header */
#include "reg0370qam.h"   /* header for this file */


/* private variables ------------------------------------------------------- */


/* functions --------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: Reg0370QAM_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0370QAM_Open(STTUNER_IOREG_DeviceMap_t *DeviceMap, U32 ExternalClock)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DeviceMap->RegExtClk = ExternalClock * 2L;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    STTBX_Print(("%s External clock=%u KHz\n", identity, ExternalClock));
#endif
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: Reg0370QAM_SetQAMSize()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0370QAM_SetQAMSize(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,S32 QAMSize)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_SetQAMSize()";
#endif
ST_ErrorCode_t  Error = ST_NO_ERROR;

    switch(QAMSize)
    {
        case STTUNER_MOD_16QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s QAM16\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_MODE_SEL, _370_QAM_QAM16);
            break;
        case STTUNER_MOD_32QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s QAM32\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_MODE_SEL, _370_QAM_QAM32);
            break;
        case STTUNER_MOD_64QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s QAM64\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_MODE_SEL, _370_QAM_QAM64);
            break;
        case STTUNER_MOD_128QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s QAM128\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_MODE_SEL, _370_QAM_QAM128);
            break;
        case STTUNER_MOD_256QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s QAM256\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_MODE_SEL, _370_QAM_QAM256);
            break;
        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s fail Bad Parameter\n", identity));
#endif
            break;
    }
    return Error;
}

/* ----------------------------------------------------------------------------
Name: Reg0370QAM_GetQAMSize()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
STTUNER_Modulation_t Reg0370QAM_GetQAMSize(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_GetQAMSize()";
#endif
    STTUNER_Modulation_t Value;

    Value = STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_MODE_SEL);

    switch(Value)
    {
        case _370_QAM_QAM16:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s QAM16\n", identity));
#endif
            return STTUNER_MOD_16QAM;
        case _370_QAM_QAM32:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s QAM32\n", identity));
#endif
            return STTUNER_MOD_32QAM;
        case _370_QAM_QAM64:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s QAM64\n", identity));
#endif
            return STTUNER_MOD_64QAM;
        case _370_QAM_QAM128:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s QAM128\n", identity));
#endif
            return STTUNER_MOD_128QAM;
        case _370_QAM_QAM256:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s QAM256\n", identity));
#endif
            return STTUNER_MOD_256QAM;
        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
            STTBX_Print(("%s fail Bad Parameter\n", identity));
#endif
            return STTUNER_MOD_ALL;
            break;
    }
}

/* ----------------------------------------------------------------------------
Name: Reg0370QAM_SetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0370QAM_SetAGC(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,short ref)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_SetAGC()";
#endif
      ST_ErrorCode_t  Error = ST_NO_ERROR;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    STTBX_Print(("%s ==> AGC2 %u\n", identity, ref));
#endif

   
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_AGC2SD_MSB,(unsigned char)(ref>>8));
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_AGC2SD_LSB,(unsigned char)ref);
	
	
	return Error;

}

/* ----------------------------------------------------------------------------
Name: Reg0370QAM_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
U16 Reg0370QAM_GetAGC(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_GetAGC()";
#endif
    short val=0;
    val = ((STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_AGC2SD_MSB))<<8) + STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_AGC2SD_LSB);
    return(val);
}

/*----------------------------------------------------
 FUNCTION      Reg0370QAM_SetWBAGCloop
 ACTION	       Sets the Roll value of the WBAGC loop	
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t Reg0370QAM_SetWBAGCloop(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,unsigned short loop)
{
	ST_ErrorCode_t  Error = ST_NO_ERROR;
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_ROLL_MSB,(unsigned char)(loop>>8));  
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_ROLL_LSB,(unsigned char)loop);
	

	return  Error;
}

/*----------------------------------------------------
 FUNCTION      Reg0370QAM_GetWBAGCloop
 ACTION	       Gets the Roll value of the WBAGC loop	
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
U16 Reg0370QAM_GetWBAGCloop(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
        unsigned short val=0;

	val =  STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_ROLL_MSB)<<8; 
	val += STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_ROLL_LSB); 
	return(val);
}

/*----------------------------------------------------
 FUNCTION      Reg0370QAM_GetSymbolRate
 ACTION	       Gets the symbol rate	
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
unsigned long	Reg0370QAM_GetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,S32 ExtClk)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370QAM.c Reg0370QAM_GetSymbolRate()";
#endif
    unsigned long regsym;
	long ExtClk_Khz;
	unsigned long RegSymbolRate; /* Ecrasie lors de l'appel*/
	ExtClk_Khz=(long)ExtClk/1000;
/*	_ExtClk = (long)(1000*UserGetExternalClock());	 unit kHz*/
	regsym  = (U32)(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_SYMB_RATE_3)<<24)	;
	regsym += (U32)(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_SYMB_RATE_2)<<16)	;
	regsym += (U32)(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_SYMB_RATE_1)<<8)	;
	regsym += STTUNER_IOREG_GetField(DeviceMap,IOHandle,F0370QAM_SYMB_RATE_0);

	if(regsym < 134217728L)				/* 134217728L = 2**27*/
	/* # 1.56 MBd (clock = 50 MHz)*/
	{
		regsym = regsym * 32;			/* 32 = 2**5*/
		regsym = regsym / 65536L;		/* 65536L = 2**16 */
		regsym = ExtClk_Khz * regsym;		/* ExtClk in kHz */
		RegSymbolRate = regsym/256L;	/* 256 = 2**8 */
		RegSymbolRate *= 125 ;			/* 125 = 1000/2**3 */
		RegSymbolRate /= 1024L ;			/* 1024 = 2**10	  */
	}
	else if(regsym < 268435456L)		/* 268435456L = 2**28 */
	/* # 3.13 MBd (clock = 50 MHz) */
	{
		regsym = regsym * 16;			/* 16 = 2**4*/
		regsym = regsym / 65536L;		/* 65536L = 2**16 */
		regsym = ExtClk_Khz * regsym;		/* ExtClk in kHz */
		RegSymbolRate = regsym/256L;	/* 256 = 2**8 */
		RegSymbolRate *= 125 ;			/* 125 = 1000/2**3*/
		RegSymbolRate /= 512L ;			/* 512 = 2**9	  */
	}
	else if(regsym < 536870912L)		/* 536870912L = 2**29 */
	/* # 6.25 MBd (clock = 50 MHz) */
	{
		regsym = regsym * 8;				/* 8 = 2**3*/
		regsym = regsym / 65536L;		/* 65536L = 2**16 */
		regsym = ExtClk_Khz * regsym;		/* ExtClk in kHz */
		RegSymbolRate = regsym/256L;	/* 256 = 2**8 */
		RegSymbolRate *= 125 ;			/* 125 = 1000/2**3*/
		RegSymbolRate /= 256L ;			/* 256 = 2**8	  */
	}
	else
	{
		regsym = regsym * 4;				/* 8 = 2**2*/
		regsym = regsym / 65536L;		/* 65536L = 2**16 */
		regsym = ExtClk_Khz * regsym;		/* ExtClk in kHz */ 
		RegSymbolRate = regsym/256L;	/* 256 = 2**8 */
		RegSymbolRate *= 125 ;			/* 125 = 1000/2**3*/
		RegSymbolRate /= 128L ;			/* 128 = 2**7	  */
	}

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    STTBX_Print(("%s ==> SR %u\n", identity, RegSymbolRate));
#endif
    return(RegSymbolRate);
}
/*----------------------------------------------------
 FUNCTION      Reg0370QAM_SetSymbolRate
 ACTION	       Sets the symbol rate	
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t	Reg0370QAM_SetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,unsigned long _SymbolRate,S32 ExtClk)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_SetSymbolRate()";
#endif
    	unsigned long ulong_tmp;
	ST_ErrorCode_t  Error = ST_NO_ERROR;
	long ExtClk_Khz;
	ExtClk_Khz=(long)ExtClk/1000;
	ulong_tmp = _SymbolRate;
     
	if(ulong_tmp < 2097152L)				/* 8388608 = 2**21*/
	{
		ulong_tmp *= 2048;					/* 2048 = 2**11 */
		ulong_tmp = ulong_tmp /ExtClk_Khz;
		ulong_tmp = ulong_tmp * 32768L;	/* 8192 = 2**15 */
		ulong_tmp /= 125L ;					/* 125 = 1000/2**3*/
		ulong_tmp = ulong_tmp * 8L;		/* 8 = 2**3 */
	}
	else if(ulong_tmp < 4194304L)			/* 4194304 = 2**22*/
	{
		ulong_tmp *= 1024 ;					/* 1024 = 2**10 */
		ulong_tmp = ulong_tmp /ExtClk_Khz;
		ulong_tmp = ulong_tmp * 32768L;	/* 8192 = 2**15 */
		ulong_tmp /= 125L ;					/* 125 = 1000/2**3*/
		ulong_tmp = ulong_tmp * 16L;		/* 16 = 2**4 */
	}
	else if(ulong_tmp < 8388607L)			/* 8388607 = 2**23*/
	{
	
		ulong_tmp *= 512 ;					/* 512 = 2**9 */
		ulong_tmp = (ulong_tmp) /ExtClk_Khz;
		ulong_tmp = ulong_tmp * 32768L;	/* 8192 = 2**15 */
		ulong_tmp /= 125L ;					/* 125 = 1000/2**3*/
		ulong_tmp = ulong_tmp * 32L;		/* 32 = 2**5 */
	
	}
	else
	{
		ulong_tmp *= 256 ;					/* 256 = 2**8*/
		ulong_tmp = ulong_tmp /ExtClk_Khz;
		ulong_tmp = ulong_tmp * 32768L;	/* 8192 = 2**15 */
		ulong_tmp /= 125L ;					/* 125 = 1000/2**3*/
		ulong_tmp = ulong_tmp * 64L;		/* 64 = 2**6 */
	}
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    STTBX_Print(("%s ==> SR %u (%u)\n", identity, _SymbolRate, ulong_tmp));
#endif
  
    
        STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_SYMB_RATE_3,(U32)(ulong_tmp>>24));
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_SYMB_RATE_2,(U32)(ulong_tmp>>16));
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_SYMB_RATE_1,(U32)(ulong_tmp>>8));
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_SYMB_RATE_0,(U32)(ulong_tmp));
	
    return Error;
}
/*----------------------------------------------------
 FUNCTION      Reg0370QAM_SetSpectrumInversion
 ACTION	       Sets the spectrum inversion
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t    Reg0370QAM_SetSpectrumInversion(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FLAG_370QAM _SpectrumInversion)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR; 
	if(_SpectrumInversion == YES)
	{
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_INV_SPEC_MAPPING,1);		
	}
	else
	{
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_INV_SPEC_MAPPING,0); 
		
	}

	return Error;
}
/*----------------------------------------------------
 FUNCTION      Reg0370QAM_SetSweepRate
 ACTION	       Sets the sweep rate 
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t	Reg0370QAM_SetSweepRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,short _FShift,S32 ExtClk)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370QAM.c Reg0370QAM_SetSweepRate()";
#endif
        long long_tmp;
	short FShift;
	unsigned long ulong_tmp;
	ST_ErrorCode_t  Error = ST_NO_ERROR;

	FShift = _FShift;	/* in ms*/
	ulong_tmp = Reg0370QAM_GetSymbolRate(DeviceMap,IOHandle, ExtClk);	/* in Baud/s*/
	if(ulong_tmp == 0) /*No need to check for -ve as ulong_tmp is unsigned long*/
	{
		return Error;/*Return proper error value*/
	}

	long_tmp = FShift * 262144L;	/* 262144 = 2*18*/
	ulong_tmp /= 1024;
	long_tmp /= (long) ulong_tmp;
	long_tmp /= 1000;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    STTBX_Print(("%s ==> _FShift %d long_tmp %d %x\n", identity, _FShift, long_tmp, long_tmp));
#endif
    STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_SWEEP_MSB,(U32)((long_tmp>>8) & 0x0F)); 
    STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_SWEEP_LSB,(U32)(long_tmp & 0xFF));
	
	
    return Error;
}
/*----------------------------------------------------
 FUNCTION      Reg0370QAM_SetFrequencyOffset
 ACTION	       Sets the frequency offset 
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t	Reg0370QAM_SetFrequencyOffset(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,long _CarrierOffset)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_SetFrequencyOffset()";
#endif
    long long_tmp;
	long CarrierOffset;
	ST_ErrorCode_t  Error = ST_NO_ERROR;
	
	CarrierOffset = _CarrierOffset	;	/* in 1/000 of Fs*/
	long_tmp = CarrierOffset * 26844L;	/* (2**28)/10000*/
	if(long_tmp < 0)	long_tmp += 0x10000000	;
	long_tmp &= 0x0FFFFFFF;
	
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    STTBX_Print(("%s ==> _CarrierOffset %d long_tmp %d %x\n", identity, _CarrierOffset, long_tmp, long_tmp));
#endif
        STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_IPHASE_3,(U32)(long_tmp>>24));
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_IPHASE_2,(U32)(long_tmp>>16));
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_IPHASE_1,(U32)(long_tmp>>8));
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370QAM_IPHASE_0,(U32)long_tmp);
 
        return Error;
}



/*----------------------------------------------------
 FUNCTION      Reg0370QAM_GetBlkCounter
 ACTION	       Gets the number of blocks 
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/

unsigned short  Reg0370QAM_GetBlkCounter(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_GetBlkCounter()";
#endif
    unsigned short Value;
    Value =  STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_RS_UNERR_CNT_LSB);
    Value += STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_RS_UNERR_CNT_MSB)<<8;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
        STTBX_Print(("%s ==> Blk (FEC_B) %u\n", identity, Value));
#endif
    return(Value);
    
}

/*----------------------------------------------------
 FUNCTION      Reg0370QAM_GetCorrBlk
 ACTION	       Gets the number of corrected blocks
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/

unsigned short  Reg0370QAM_GetCorrBlk(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_GetCorrBlk()";
#endif
     unsigned short Value;
     Value =  STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_RS_CORR_CNT_LSB);
     Value += STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_RS_CORR_CNT_MSB)<<8;	
	
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
        STTBX_Print(("%s ==> CorrBlk (FEC_B) %u\n", identity, Value));
#endif
   
    return(Value);
}

/*----------------------------------------------------
 FUNCTION      Reg0370QAM_GetUncorrBlk
 ACTION	       Gets the number of uncorrected blocks
           
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/

unsigned short  Reg0370QAM_GetUncorrBlk(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_GetUncorrBlk()";
#endif
    unsigned short Value;
    Value =  STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_RS_UNC_CNT_LSB);
    Value += STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370QAM_RS_UNC_CNT_MSB)<<8;
    
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
        STTBX_Print(("%s ==> UncorrBlk (FEC_B==1) %u\n", identity, Value));
#endif
      
    return(Value);
}


/* ----------------------------------------------------------------------------
Name: Reg0370QAM_GetSTB0370QAMId()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
U8 Reg0370QAM_GetSTB0370QAMId(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    const char *identity = "STTUNER reg0370qam.c Reg0370QAM_GetSTB0370QAMId()";
#endif
    U8    Value;

    Value= STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0370QAM_ID);/*change register name to specify QAM reg*/

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
    STTBX_Print(("%s ==> Version %x\n", identity, Value));
#endif

    return(Value);
}


/* End of reg0370qam.c */








