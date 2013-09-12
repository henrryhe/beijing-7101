/* ----------------------------------------------------------------------------
File Name: reg0297J.c (was reg0297.c)

Description:

Copyright (C) 1999-2001 STMicroelectronics

   date: 15-May-2002
version: 3.5.0
 author: from STV0297 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */
#ifndef ST_OSLINUX
  

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

#include "d0297j.h"     /* top level driver header */
#include "reg0297j.h"   /* header for this file */





/* functions --------------------------------------------------------------- */


/* ----------------------------------------------------------------------------
Name: Reg0297J_Install()

Description:
    install driver register table

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0297J_Install(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_Install()";
#endif
   ST_ErrorCode_t Error = ST_NO_ERROR;


    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
        STTBX_Print(("%s fail error=%d\n", identity, Error));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
        STTBX_Print(("%s installed ok\n", identity));
#endif
    }

    return (Error);
}



/* ----------------------------------------------------------------------------
Name: Reg0297J_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0297J_Open(STTUNER_IOREG_DeviceMap_t *DeviceMap, U32 ExternalClock)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DeviceMap->RegExtClk = ExternalClock * 2L;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s External clock=%u KHz\n", identity, ExternalClock));
#endif
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: Reg0297J_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0297J_Close(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s\n", identity));
#endif
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: Reg0297J_UnInstall()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0297J_UnInstall(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s\n", identity));
#endif
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: Reg0297J_SetQAMSize()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297J_SetQAMSize(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, STTUNER_Modulation_t Value)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_SetQAMSize()";
#endif

    switch(Value)
    {
        case STTUNER_MOD_16QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s QAM16\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_MODE_SELECT, QAM16);
            break;
        case STTUNER_MOD_32QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s QAM32\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_MODE_SELECT, QAM32);
            break;
        case STTUNER_MOD_64QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s QAM64\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_MODE_SELECT, QAM64);
            break;
        case STTUNER_MOD_128QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s QAM128\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_MODE_SELECT, QAM128);
            break;
        case STTUNER_MOD_256QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s QAM256\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_MODE_SELECT, QAM256);
            break;
        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s fail Bad Parameter\n", identity));
#endif
            break;
    }
}

/* ----------------------------------------------------------------------------
Name: Reg0297J_GetQAMSize()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
STTUNER_Modulation_t Reg0297J_GetQAMSize(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_GetQAMSize()";
#endif
    STTUNER_Modulation_t Value;

    Value = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297J_MODE_SELECT);

    switch(Value)
    {
        case QAM16:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s QAM16\n", identity));
#endif
            return STTUNER_MOD_16QAM;
        case QAM32:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s QAM32\n", identity));
#endif
            return STTUNER_MOD_32QAM;
        case QAM64:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s QAM64\n", identity));
#endif
            return STTUNER_MOD_64QAM;
        case QAM128:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s QAM128\n", identity));
#endif
            return STTUNER_MOD_128QAM;
        case QAM256:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s QAM256\n", identity));
#endif
            return STTUNER_MOD_256QAM;
        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
            STTBX_Print(("%s fail Bad Parameter\n", identity));
#endif
            return STTUNER_MOD_ALL;
            break;
    }
}

/* ----------------------------------------------------------------------------
Name: Reg0297J_SetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297J_SetAGC(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U16 Value)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297j.c Reg0297J_SetAGC()";
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s ==> AGC2 %u\n", identity, Value));
#endif
  
    
     STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_AGC2SD_LO, (int)(MAC0297J_B0(Value)));
     STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_AGC2SD_HI, (int)(MAC0297J_B1(Value)));


}

/* ----------------------------------------------------------------------------
Name: Reg0297J_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
U16 Reg0297J_GetAGC(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297.c Reg0297J_GetAGC()";
#endif
    U16 Value;
    
    Value = MAC0297J_MAKEWORD(STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0297J_AGC2SD_HI),
                              STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0297J_AGC2SD_LO));
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s ==> AGC2 %u\n", identity, Value));
#endif

    return (Value);
}

/****** SetWBAGCLoop *********************************************************/
/* Set the Roll value of the WBAGC loop                                      */
/*****************************************************************************/
void Reg0297J_SetWBAGCloop(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, unsigned short loop)
{
   
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_ROLL_LO, (int)(MAC0297J_B0(loop)));
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_ROLL_HI, (int)(MAC0297J_B1(loop)));
  
    return;
}

/****** GetWBAGCloop *********************************************************/
/* Get the Roll value of the WBAGC loop                                      */
/*****************************************************************************/
U16 Reg0297J_GetWBAGCloop(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    U16 Value;
    U8 temp[1];
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297J_WBAGC_6, 2, temp);

    Value = MAC0297J_MAKEWORD(temp[1],temp[0]);

    return (Value);
}

/***************************** STV0297Jreset ***************************************/
/*  STV0297J Software reset                                                       */
/*  Set bit 0 @reg0x80 to 1 then 0												  */
/**********************************************************************************/
void Reg0297J_STV0297Jreset(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_STV0297Jreset()";
#endif
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_SOFT_RESET,1);
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_SOFT_RESET,0);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s ==> SOFT RESET\n", identity));
#endif
}
/**/
/******* GetSymbolRate ***********/
/*********************************/
unsigned long	Reg0297J_GetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_GetSymbolRate()";
#endif
    unsigned long   regsym;
    long            _ExtClk;
    unsigned long   RegSymbolRate;
   
    /**/
    _ExtClk = DeviceMap->RegExtClk;
    
    regsym  =  STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0297J_SYMB_RATE_0);
    regsym += (STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0297J_SYMB_RATE_1)*256L);
    regsym += (STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0297J_SYMB_RATE_2)*65536L);
    regsym += (STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0297J_SYMB_RATE_3)*16777216L);

    if(regsym < 134217728L)         /* 134217728L = 2**27 */
                                    /* # 1.56 MBd (clock = 50 MHz) */
	{
        regsym = regsym * 32;       /* 32 = 2**5 */
        regsym = regsym / 65536L;   /* 65536L = 2**16 */
        regsym = _ExtClk * regsym;  /* _ExtClk in kHz */
        RegSymbolRate = regsym/256L;/* 256 = 2**8 */
        RegSymbolRate *= 125 ;      /* 125 = 1000/2**3 */
        RegSymbolRate /= 1024 ;     /* 1024 = 2**10 */
	}
    else if(regsym < 268435456L)    /* 268435456L = 2**28 */
                                    /* # 3.13 MBd (clock = 50 MHz) */
	{
        regsym = regsym * 16;       /* 16 = 2**4 */
        regsym = regsym / 65536L;   /* 65536L = 2**16 */
        regsym = _ExtClk * regsym;  /* _ExtClk in kHz */
        RegSymbolRate = regsym/256L;/* 256 = 2**8 */
        RegSymbolRate *= 125 ;      /* 125 = 1000/2**3 */
        RegSymbolRate /= 512 ;      /* 512 = 2**9 */
	}
    else if(regsym < 536870912L)    /* 536870912L = 2**29 */
                                    /* # 6.25 MBd (clock = 50 MHz) */
	{
        regsym = regsym * 8;        /* 8 = 2**3 */
        regsym = regsym / 65536L;   /* 65536L = 2**16 */
        regsym = _ExtClk * regsym;  /* _ExtClk in kHz */
        RegSymbolRate = regsym/256L;/* 256 = 2**8 */
        RegSymbolRate *= 125 ;      /* 125 = 1000/2**3 */
        RegSymbolRate /= 256 ;      /* 256 = 2**8 */
	}
	else
	  {
        regsym = regsym * 4;        /* 8 = 2**2 */
        regsym = regsym / 65536L;   /* 65536L = 2**16 */
        regsym = _ExtClk * regsym;  /* _ExtClk in kHz */
        RegSymbolRate = regsym/256L;/* 256 = 2**8 */
        RegSymbolRate *= 125 ;      /* 125 = 1000/2**3 */
        RegSymbolRate /= 128 ;      /* 128 = 2**7 */
	  }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s ==> SR %u\n", identity, RegSymbolRate));
#endif
    return(RegSymbolRate);
}
/******* SetSymbolRate ***********/
/*********************************/
void	Reg0297J_SetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, unsigned long _SymbolRate)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_SetSymbolRate()";
#endif
    unsigned long   ulong_tmp;
    long            _ExtClk;
    
    /**/
    _ExtClk = DeviceMap->RegExtClk;
    ulong_tmp = _SymbolRate ;  /* _SymbolRate is in Bd */

    if(ulong_tmp < 2097152L)        /* 8388608 = 2**21 */
	{
        ulong_tmp *= 2048 ;             /* 2048 = 2**11 */
		ulong_tmp = ulong_tmp /_ExtClk ;
        ulong_tmp = ulong_tmp * 32768L; /* 8192 = 2**15 */
        ulong_tmp /= 125 ;              /* 125 = 1000/2**3 */
        ulong_tmp = ulong_tmp * 8L;     /* 8 = 2**3 */
	}
    else if(ulong_tmp < 4194304L)   /* 4194304 = 2**22 */
	{
        ulong_tmp *= 1024 ;             /* 1024 = 2**10 */
		ulong_tmp = ulong_tmp /_ExtClk ;
        ulong_tmp = ulong_tmp * 32768L; /* 8192 = 2**15 */
        ulong_tmp /= 125 ;              /* 125 = 1000/2**3 */
        ulong_tmp = ulong_tmp * 16L;    /* 16 = 2**4 */
	}
    else if(ulong_tmp < 8388607L)   /* 8388607 = 2**23 */
	{
        ulong_tmp *= 512 ;              /* 512 = 2**9 */
		ulong_tmp = ulong_tmp /_ExtClk ;
        ulong_tmp = ulong_tmp * 32768L; /* 8192 = 2**15 */
        ulong_tmp /= 125 ;              /* 125 = 1000/2**3 */
        ulong_tmp = ulong_tmp * 32L;    /* 32 = 2**5 */
	}
	else
	{
        ulong_tmp *= 256 ;              /* 256 = 2**8 */
		ulong_tmp = ulong_tmp /_ExtClk ;
        ulong_tmp = ulong_tmp * 32768L; /* 8192 = 2**15 */
        ulong_tmp /= 125 ;              /* 125 = 1000/2**3 */
        ulong_tmp = ulong_tmp * 64L;    /* 64 = 2**6 */
	}
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s ==> SR %u (%u)\n", identity, _SymbolRate, ulong_tmp));
#endif
       
   
    
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_SYMB_RATE_0,(int)(MAC0297J_B0(ulong_tmp)));
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_SYMB_RATE_1,(int)(MAC0297J_B1(ulong_tmp)));
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_SYMB_RATE_2,(int)(MAC0297J_B2(ulong_tmp)));
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_SYMB_RATE_3,(int)(MAC0297J_B3(ulong_tmp)));

    return;
}
/****** RegSetSpectrumInversion *********************************************/
/* Sets the spectrum inversion                                              */
/****************************************************************************/
void    Reg0297J_SetSpectrumInversion(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, STTUNER_Spectrum_t SpectrumInversion)
{
    if(SpectrumInversion == TRUE)
        STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_SPEC_INV, 1);
    else
        STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_SPEC_INV, 0);

	return;
}
/******* SetSweepRate ************/
/*                               */
/*********************************/
void	Reg0297J_SetSweepRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, short _FShift)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_SetSweepRate()";
#endif
    long            long_tmp;
    short           FShift ;
    unsigned long   ulong_tmp ;
  
    /**/
    FShift = _FShift ;  /* in ms */
    ulong_tmp = Reg0297J_GetSymbolRate(DeviceMap, IOHandle) ;  /* in Baud/s */
    /**/
    long_tmp = FShift * 262144L ;  /* 262144 = 2*18 */
    ulong_tmp /= 1024 ;
    long_tmp /= (long) ulong_tmp ;
    long_tmp /= 1000 ;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s ==> _FShift %d long_tmp %d %x\n", identity, _FShift, long_tmp, long_tmp));
#endif
 
   
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_SWEEP_LO,(int)(MAC0297J_B0(long_tmp)));
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_SWEEP_HI,(int)(MAC0297J_B1N0(long_tmp)));
    return;
}

/******* SetFrequencyOffset *********/
/*                                  */
/************************************/
void	Reg0297J_SetFrequencyOffset(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, long _CarrierOffset)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_SetFrequencyOffset()";
#endif
    long long_tmp;
    long CarrierOffset ;
   
    /**/
    CarrierOffset = _CarrierOffset     ; /* in 0/000 of Fs */
    /**/
    long_tmp = CarrierOffset * 26844L ; /* (2**28)/10000 */
    if(long_tmp < 0) long_tmp += 0x10000000 ;
    long_tmp &= 0x0FFFFFFF ;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s ==> _CarrierOffset %d long_tmp %d %x\n", identity, _CarrierOffset, long_tmp, long_tmp));
#endif
  
  
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_IPHASE_0,(int)(MAC0297J_B0(long_tmp)));
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_IPHASE_1,(int)(MAC0297J_B1(long_tmp)));
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_IPHASE_2,(int)(MAC0297J_B2(long_tmp)));
    STTUNER_IOREG_SetField(DeviceMap, IOHandle,F0297J_IPHASE_3,(int)(MAC0297J_B3(long_tmp)));
    return;
}



unsigned short  Reg0297J_GetBlkCounter(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_GetBlkCounter()";
#endif
    unsigned short Value;
    U8 temp[1];
    if (STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297J_FEC_AC_OR_B) == 1)
    {
        STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297J_RS_DESC_0, 2, temp);
        Value =  temp[0];
        Value += temp[1]<<8;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
        STTBX_Print(("%s ==> BlkCounter (FEC_AC_OR_B==1) %u\n", identity, Value));
#endif
    }
    else
    {
        STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297J_RS_UNERR_CNT_0, 2, temp);
        Value =  temp[0];
        Value += temp[1]<<8;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
        STTBX_Print(("%s ==> BlkCounter (FEC_AC_OR_B==0) %u\n", identity, Value));
#endif
    }
    return(Value);
}



unsigned short  Reg0297J_GetCorrBlk(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_GetCorrBlk()";
#endif
    unsigned short Value;
    U8 temp[1];
    if (STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297J_FEC_AC_OR_B) == 1)
    {
        STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297J_RS_DESC_2, 2, temp);
        Value =  temp[0];
        Value += temp[1]<<8;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
        STTBX_Print(("%s ==> CorrBlk (FEC_AC_OR_B==1) %u\n", identity, Value));
#endif
    }
    else
    {
        STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297J_RS_CORR_CNT_0, 2, temp);
        Value =  temp[0];
        Value += temp[1]<<8;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
        STTBX_Print(("%s ==> CorrBlk (FEC_AC_OR_B==0) %u\n", identity, Value));
#endif
    }
    return(Value);
}



unsigned short  Reg0297J_GetUncorrBlk(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_GetUncorrBlk()";
#endif
    unsigned short Value;
    U8 temp[1];
    if (STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297J_FEC_AC_OR_B) == 1)
    {
        STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297J_RS_DESC_4, 2, temp);
        Value =  temp[0];
        Value += temp[1]<<8;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
        STTBX_Print(("%s ==> UncorrBlk (FEC_AC_OR_B==1) %u\n", identity, Value));
#endif
    }
    else
    {
        
        Value =  STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0297J_RS_UNC_CNT_LO);
        Value += STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0297J_RS_UNC_CNT_HI)<<8;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
        STTBX_Print(("%s ==> UncorrBlk (FEC_AC_OR_B==0) %u\n", identity, Value));
#endif
    }
    return(Value);
}


/* ----------------------------------------------------------------------------
Name: Reg0297J_GetSTV0297JId()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
U8 Reg0297J_GetSTV0297JId(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    const char *identity = "STTUNER reg0297J.c Reg0297J_GetSTV0297JId()";
#endif
    U8    Value;

    Value =   0x11;     /* See top-view package */

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297J
    STTBX_Print(("%s ==> Version %x\n", identity, Value));
#endif
    return(Value);
}

/* ----------------------------------------------------------------------------
Name: Reg0297J_StartBlkCounter()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297J_StartBlkCounter(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_CT_HOLD, 0);
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_CT_CLEAR, 0);
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_CT_CLEAR, 1);
}

/* ----------------------------------------------------------------------------
Name: Reg0297J_StopBlkCounter()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297J_StopBlkCounter(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297J_CT_HOLD, 1);
}

/* End of reg0297J.c */

