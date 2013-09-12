/* ----------------------------------------------------------------------------
File Name: reg0299.c

Description:

Copyright (C) 1999-2001 STMicroelectronics

   date: 29-June-2001
version: 3.1.0
 author: GJP from work by LW/CBB
comment: adapted from reg0299.c

Revision History:
    04/02/00        Code based on original implementation by CBB.

    21/03/00        Received code modifications that eliminate compiler
                    warnings.

    05/03/02        Removed some mfr. specific comments.

---------------------------------------------------------------------------- */

/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   #include <string.h>
#endif
#include "stlite.h"     /* Standard includes */
#include "sttbx.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "ioarch.h"     /* IO access layer */
#include "ioreg.h"      /* register mapping */

#include "d0299.h"      /* top level driver header */
#include "reg0299.h"    /* header for this file */


 

/* functions --------------------------------------------------------------- */


/* ----------------------------------------------------------------------------
Name: Reg0299_Install()

Description:
    install driver register table

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0299_Install(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
#ifdef DVD_DBG_STTUNER_SATDRV_REG0299
    const char *identity = "STTUNER reg0299.c Reg0299_Install()";
#endif
   ST_ErrorCode_t Error = ST_NO_ERROR;

    
    if (Error != ST_NO_ERROR)
    {
#ifdef DVD_DBG_STTUNER_SATDRV_REG0299
        STTBX_Print(("%s fail error=%d\n", identity, Error));
#endif
    }
    {
#ifdef DVD_DBG_STTUNER_SATDRV_REG0299
        STTBX_Print(("%s installed ok\n", identity));
#endif
    }

    return (Error);
}



/* ----------------------------------------------------------------------------
Name: Reg0299_Open()

Description:
    install driver register table

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0299_Open(STTUNER_IOREG_DeviceMap_t *DeviceMap, U32 ExternalClock)
{
#ifdef DVD_DBG_STTUNER_SATDRV_REG0299
    const char *identity = "STTUNER reg0299.c Reg0299_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DeviceMap->RegExtClk = ExternalClock;

#ifdef DVD_DBG_STTUNER_SATDRV_REG0299
    STTBX_Print(("%s External clock=%u\n", identity, ExternalClock));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: Reg0299_SetExtClk()

Description:
    Set the value of the external clock variable
Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0299_SetExtClk(STTUNER_IOREG_DeviceMap_t *DeviceMap, long Value)
{
    DeviceMap->RegExtClk = Value;
}


/* ----------------------------------------------------------------------------
Name: Reg0299_GetExtClk()

Description:
    Get the external clock value
Parameters:

Return Value:
---------------------------------------------------------------------------- */
long Reg0299_GetExtClk(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
    return (DeviceMap->RegExtClk);
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcRefFrequency()

Description:
    Compute reference frequency
Parameters:

Return Value:
---------------------------------------------------------------------------- */
long Reg0299_CalcRefFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int k)
{
    return ( Reg0299_GetExtClk(DeviceMap) / ( (long)k + 1) );   /* cast to eliminate compiler warning --SFS */
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcVCOFrequency()

Description:
    Compute VCO frequency
Parameters:

Return Value:
---------------------------------------------------------------------------- */
long Reg0299_CalcVCOFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int k, int m)
{
    return ( Reg0299_CalcRefFrequency(DeviceMap, k) * 4 * ( (long)m + 1) );  /* cast to eliminate compiler warning --SFS */
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetVCOFreq()

Description:
    Read registers and compute VCO frequency
Parameters:

Return Value:
---------------------------------------------------------------------------- */
long Reg0299_RegGetVCOFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{ U8 RCR;
    /*	Read register	*/
    RCR =  STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_RCR);

    /* compute VCO freq */
    return( Reg0299_CalcVCOFrequency(DeviceMap, STTUNER_IOREG_FieldExtractVal(RCR, F0299_K), STTUNER_IOREG_FieldExtractVal(RCR, F0299_M)) );
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcF22Frequency()

Description:
    Compute F22 frequency
Parameters:

Return Value:
---------------------------------------------------------------------------- */
long Reg0299_CalcF22Frequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int k, int m, int f22)
{
    return (f22 != 0) ? ( Reg0299_CalcVCOFrequency(DeviceMap, k, m)/(128*(long)f22)) : 0;  /* cast to eliminate compiler warning --SFS */
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetF22Freq()

Description:
    Read registers and Compute F22 frequency
Parameters:

Return Value:
---------------------------------------------------------------------------- */
long Reg0299_RegGetF22Freq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{  U8 RCRTemp;
    /*	Read registers	*/
    RCRTemp=STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_RCR);
    

    return Reg0299_CalcF22Frequency(DeviceMap, STTUNER_IOREG_FieldExtractVal(RCRTemp, F0299_K),
                                               STTUNER_IOREG_FieldExtractVal(RCRTemp, F0299_M),
                                               STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_F22FR) );
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcMasterClkFrequency()

Description:
    Compute Master clock frequency
Parameters:

Return Value:
---------------------------------------------------------------------------- */
long Reg0299_CalcMasterClkFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int stdby, int dirclk, int k, int m, int p)
{
    long mclk;

    if(stdby)
        mclk = 0;
    else if(dirclk)
        mclk = Reg0299_GetExtClk(DeviceMap);
    else
        mclk = Reg0299_CalcVCOFrequency(DeviceMap, k, m)/((2+((long)p%2)) * STTUNER_Util_PowOf2((p/2)+1));  /* cast to eliminate compiler warning --SFS */

    return(mclk);
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetMasterFreq()

Description:
    Read registers and compute Master clock frequency
Parameters:

Return Value:
---------------------------------------------------------------------------- */
long Reg0299_RegGetMasterFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U8 RCRCopy,MCRCopy;
    /*	Read registers	*/
    RCRCopy=STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_RCR);
    MCRCopy=STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_MCR);

    return Reg0299_CalcMasterClkFrequency( DeviceMap, STTUNER_IOREG_FieldExtractVal(MCRCopy, F0299_STDBY),
                                                      STTUNER_IOREG_FieldExtractVal(RCRCopy, F0299_DIRCLK),
                                                      STTUNER_IOREG_FieldExtractVal(RCRCopy, F0299_K),
                                                      STTUNER_IOREG_FieldExtractVal(RCRCopy, F0299_M),
                                                      STTUNER_IOREG_FieldExtractVal(MCRCopy, F0299_P) ); /* Compute master clock freq */
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcDerotatorFrequency()

Description:
    Compute Derotator frequency
Parameters:

Return Value:
    Derotator frequency (KHz)
---------------------------------------------------------------------------- */
long Reg0299_CalcDerotatorFrequency(int derotmsb, int derotlsb, long fm)
{
    long       dfreq;
    short int  Itmp;

    Itmp   = (derotmsb<<8) + derotlsb;
    dfreq  = ( ((long)Itmp) * (fm/10000L) ) / 65536L;
    dfreq *= 10;

    return(dfreq);
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetDerotatorFreq()

Description:
    Get registeres then Compute Derotator frequency
Parameters:

Return Value:
    Derotator frequency (KHz)
---------------------------------------------------------------------------- */
long Reg0299_RegGetDerotatorFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U8 cfrm[2];
    /*	Read registers	*/
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_CFRM, 2,cfrm);

    return Reg0299_CalcDerotatorFrequency(cfrm[0],cfrm[1],Reg0299_RegGetMasterFreq(DeviceMap, IOHandle));
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcAuxClkFrequency()

Description:
    Compute Auxiliary clock frequency
Parameters:

Return Value:
    Auxiliary Clock frequency
---------------------------------------------------------------------------- */
long Reg0299_CalcAuxClkFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int k, int m, int acr)
{
    long aclk, factor;
    int  function, value;

    function = (acr >> 5);
    value = acr & 0x1F;

    switch(function)
    {
        case 0:
            aclk = 0;
            break;

        case 1:
            if(value)
                aclk = Reg0299_CalcVCOFrequency(DeviceMap, k, m) / 8 / value;
            else
                aclk = Reg0299_CalcVCOFrequency(DeviceMap, k, m) / 8;
            break;

        default:
            factor = 8192L * STTUNER_Util_PowOf2(function - 2);
            aclk = Reg0299_CalcVCOFrequency(DeviceMap, k, m)/(long)factor/(32+(long)value); /* cast to eliminate compiler warning --SFS */
            break;
    }

    return(aclk);
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetAuxFreq()

Description:
    Read registers and compute Auxiliary clock frequency
Parameters:

Return Value:
    Auxiliary Clock frequency
---------------------------------------------------------------------------- */
long Reg0299_RegGetAuxFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{ U8 RCRTemp;
    /*	Read registers	*/
    RCRTemp=STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_RCR);
   

    return Reg0299_CalcAuxClkFrequency( DeviceMap, STTUNER_IOREG_FieldExtractVal(RCRTemp, F0299_K),
                                                   STTUNER_IOREG_FieldExtractVal(RCRTemp, F0299_M),
                                                   STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_ACR));    /* Compute auxiliary clock freq */
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetErrorCount()

Description:

Parameters:

Return Value:

---------------------------------------------------------------------------- */
int Reg0299_RegGetErrorCount(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U8 errcnt[2];
    /* Obtain high and low bytes for error count register */
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_ERRCNT_HIGH, 2,errcnt);

    return ( MAC0299_MAKEWORD(errcnt[0],errcnt[1]) );
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegSetErrorCount()

Description:

Parameters:

Return Value:

---------------------------------------------------------------------------- */
void Reg0299_RegSetErrorCount(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, int Value)
{
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_ERRCNTMSB, MAC0299_MSB(Value));
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_ERRCNTLSB, MAC0299_LSB(Value));
}


/* ----------------------------------------------------------------------------
Name: Reg0299_GetRollOff()

Description:
    Simple function whichs read the register 00h  bit bit1 and which sets
    corresponding Nyquist filter roll-off value.
Parameters:

Return Value:

---------------------------------------------------------------------------- */
int Reg0299_GetRollOff(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    if ( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_NYQUISTFILTER) == 1 )
        return 20;
    else
        return 35;
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegSetRollOff()

Description:

Parameters:

Return Value:

---------------------------------------------------------------------------- */
void Reg0299_RegSetRollOff(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, int Value)
{
    if(Value == 35)
        STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_NYQUISTFILTER, 0);
    else
        STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_NYQUISTFILTER, 1);
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcCarrierNaturalFrequency()

Description:
    Compute natural clock frequency for carrier loop

Parameters:
    m2		->	AGC2 reference level
    betacar ->	Beta carrier
    fm		->	Master clock frequency (MHz)
    fs		->	Symbol frequency (MHz)

Return Value:
    Carrier natural frequency (KHz)
---------------------------------------------------------------------------- */
long Reg0299_CalcCarrierNaturalFrequency(int m2 , int betacar , long fm , long fs)
{
    long NatFreq=0;
    long c, d, e;

    if (fm > 0L)
    {
        c= ((long)betacar & 0x02)>>1;   /* cast to eliminate compiler warning --SFS */
        d=  (long)betacar & 0x01;       /* cast to eliminate compiler warning --SFS */
        e= ((long)betacar & 0x3C)>>2;   /* cast to eliminate compiler warning --SFS */
        betacar= (int)(4+2*c+d)*(int)STTUNER_Util_PowOf2((int)e); /* cast to eliminate compiler warning --SFS */

        NatFreq  = (long)((long)m2*betacar);  /* cast to eliminate compiler warning --SFS */
        NatFreq *= fs/(fm/1000);
        NatFreq /= 1000;
        NatFreq  = STTUNER_Util_LongSqrt(NatFreq)*(fm/1000)*7L;
        NatFreq /= 1000L;
    }

    return NatFreq;
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetCarrierNaturalFrequency()

Description:
    Read registers and compute natural clock frequency for carrier loop
Parameters:

Return Value:
    Carrier natural frequency
---------------------------------------------------------------------------- */
long Reg0299_RegGetCarrierNaturalFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{   U8 RCRTemp,MCRTemp,sfrh[3];
    long fm, fs;

RCRTemp =  STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_RCR);
MCRTemp =  STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_MCR);
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_SFRH, 3,sfrh);    /* SFRH, SFRM, SFRL */

    fm = Reg0299_CalcMasterClkFrequency(DeviceMap, STTUNER_IOREG_FieldExtractVal(MCRTemp, F0299_STDBY),
                                                   STTUNER_IOREG_FieldExtractVal(RCRTemp, F0299_DIRCLK),
                                                   STTUNER_IOREG_FieldExtractVal(RCRTemp, F0299_K),
                                                   STTUNER_IOREG_FieldExtractVal(RCRTemp, F0299_M),
                                                   STTUNER_IOREG_FieldExtractVal(MCRTemp, F0299_P));

    fs = Reg0299_CalcSymbolRate(DeviceMap, IOHandle, sfrh[0],
                                                     sfrh[1],
                                                     STTUNER_IOREG_FieldExtractVal(sfrh[2], F0299_SYMB_FREQL));

    return Reg0299_CalcCarrierNaturalFrequency( STTUNER_IOREG_GetField(DeviceMap,IOHandle, F0299_AGC2_REF),
                                                STTUNER_IOREG_GetField(DeviceMap,IOHandle, F0299_BETA_CAR),
                                                fm,
                                                fs );
}


  

/* ----------------------------------------------------------------------------
Name: Reg0299_CalcCarrierDampingFactor()

Description:
    Compute damping factor for carrier loop
Parameters:
    m2		->	AGC2 reference level
    alphacar->	Alpha carrier
    betacar ->	Beta carrier
    fm		->	Master clock frequency
    fs		->	Symbol frequency

Return Value:
    Carrier damping factor
---------------------------------------------------------------------------- */
long Reg0299_CalcCarrierDampingFactor(int m2, int alphacar, int betacar, long fm, long fs)
{
    /* the damping factor can be below 1. So, in order to have
     ** an accurate integer value, the result will be 1000*damping factor	*/

    long DampFact = 0;
    long a, b, c, d, e;

    if(fm > 0)
    {
        a=  (long)alphacar & 0x01;      /* cast to eliminate compiler warning --SFS */
        b= ((long)alphacar & 0x0E)>>1;  /* cast to eliminate compiler warning --SFS */
        c= ((long)betacar  & 0x02)>>1;  /* cast to eliminate compiler warning --SFS */
        d=  (long)betacar  & 0x01;      /* cast to eliminate compiler warning --SFS */
        e= ((long)betacar  & 0x3C)>>2;  /* cast to eliminate compiler warning --SFS */

        alphacar = (int)((2+a)*STTUNER_Util_PowOf2((int)b+14));     /* cast to eliminate compiler warning --SFS */
        betacar  = (int)((4+2*c+d)*STTUNER_Util_PowOf2((int)e));    /* cast to eliminate compiler warning --SFS */

        /*	Erreur d'arrondi lorsque Fs est faible	*/
        DampFact  = ((long)m2*fs/betacar*100/fm);
        DampFact  = STTUNER_Util_LongSqrt(DampFact)*alphacar/10;
        DampFact /= 1000L; /*	=1000000 / 1000	*/
        DampFact *= 22L;
    }

    return DampFact;
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetCarrierDampingFactor()

Description:
    Read registers and compute damping factor for carrier loop
Parameters:

Return Value:
    Carrier damping factor
---------------------------------------------------------------------------- */
long Reg0299_RegGetCarrierDampingFactor(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    long fm, fs;
    U8 rcr[2],aclc[2],sfrh[3];

    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_RCR,  2,rcr);    /* RCR, MCR */
    STTUNER_IOREG_GetRegister(DeviceMap,           IOHandle, R0299_AGC2O);
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_ACLC, 2,aclc);    /* ACLC, BCLC	*/
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_SFRH, 3,sfrh);    /* SFRH, SFRM, SFRL */

    fm = Reg0299_CalcMasterClkFrequency(DeviceMap, STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_STDBY),
                                                   STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_DIRCLK),
                                                   STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_K),
                                                   STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_M),
                                                   STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_P) );

    fs = Reg0299_CalcSymbolRate(DeviceMap, IOHandle, STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_SYMB_FREQH),
                                                     STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_SYMB_FREQM),
                                                     STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_SYMB_FREQL));

    return Reg0299_CalcCarrierDampingFactor( STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_AGC2_REF),
                                             STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_ALPHA_CAR),
                                             STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_BETA_CAR),
                                             fm,
                                             fs );
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcTimingNaturalFrequency()

Description:
    Compute natural frequency for timing loop
Parameters:
    m2		->	AGC2 reference level
    betatmg ->	Beta timing
    fs		->	Symbol frequency (MHz)
Return Value:
    Timing natural frequency (MHz)
---------------------------------------------------------------------------- */
long Reg0299_CalcTimingNaturalFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, int m2, int betatmg, long fs)
{
    long NatFreq=0;

    if(betatmg >= 0)
    {
        NatFreq  = STTUNER_Util_LongSqrt( (long)m2 * STTUNER_Util_PowOf2(betatmg) );
        NatFreq *= (fs / 1000000L);
        NatFreq *= 52;
        NatFreq /= 10;
    }

    return NatFreq;
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetTimingNaturalFrequency()

Description:
    Read registers and compute natural clock frequency for timing loop
Parameters:

Return Value:
    Timing natural frequency (MHz)
---------------------------------------------------------------------------- */
long Reg0299_RegGetTimingNaturalFrequency(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    long fs;

  /*  STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_AGC2O);
    STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_RTC);
    STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_SFRH);
    STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_SFRM);
    STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_SFRL);*/

    fs = Reg0299_CalcSymbolRate( DeviceMap, IOHandle, STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_SYMB_FREQH),
                                                      STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_SYMB_FREQM),
                                                      STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_SYMB_FREQL) );

    return Reg0299_CalcTimingNaturalFrequency( DeviceMap, STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_AGC2_REF),
                                                          STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0299_BETA_TMG),
                                                          fs );
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcTimingDampingFactor()

Description:
    Compute damping factor for timing loop
Parameters:
    m2		->	AGC2 reference level
    alphatmg->	Alpha timing
    betatmg ->	Beta timing
Return Value:
    Timing damping factor (*1000)
---------------------------------------------------------------------------- */
long Reg0299_CalcTimingDampingFactor(int m2, int alphatmg, int betatmg)
{
    long DampFact=0;

    if( (alphatmg >= 0) && (betatmg >= 0) )
    {
        DampFact  = 134 * STTUNER_Util_LongSqrt((long)m2*16) * STTUNER_Util_PowOf2(alphatmg);   /* cast to eliminate compiler warning --SFS */
        DampFact /= STTUNER_Util_LongSqrt(STTUNER_Util_PowOf2(betatmg)*16);
    }

    return DampFact;
}




/* ----------------------------------------------------------------------------
Name: Reg0299_CalcSymbolRate()

Description:
    Compute symbol frequency
Parameters:
    Hbyte	->	High order byte
    Mbyte	->	Mid byte
    Lbyte	->	Low order byte
Return Value:
    Symbol frequency
---------------------------------------------------------------------------- */
long Reg0299_CalcSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, int Hbyte, int Mbyte, int Lbyte)
{
    long Ltmp, Ltmp2;
    long Mclk;

    Mclk  =  Reg0299_RegGetMasterFreq(DeviceMap, IOHandle) / 4096L;	/* Fm_clk*10/2^20 */
    Ltmp  = ( ((long)Hbyte<<12) + ((long)Mbyte<<4) ) / 16;
    Ltmp *= Mclk;
    Ltmp /=16;
    Ltmp2 =((long)Lbyte*Mclk) / 256;
    Ltmp +=Ltmp2;

    return Ltmp;
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegSetSymbolRate()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
long Reg0299_RegSetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, long SymbolRate)
{
    unsigned long Result;
    long MasterClock;
    U8 freq[3];

    /*
     ** in order to have the maximum precision, the symbol rate entered into
     ** the chip is computed as the closest value of the "true value".
     ** In this purpose, the symbol rate value is rounded (1 is added on the bit
     ** below the LSB )
     */

    MasterClock = Reg0299_RegGetMasterFreq(DeviceMap, IOHandle);
    Result = STTUNER_Util_BinaryFloatDiv(SymbolRate, MasterClock, 20);
    freq[0]=(U8)(Result>>12) & 0xFF;
    freq[1]=(U8)(Result>>4)  & 0xFF;
    freq[2]= (U8)(Result & 0x0F)<<4;
    STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle, R0299_SFRH, freq,3);

    return(SymbolRate);
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetSymbolRate()

Description:
    Return the symbol rate
Parameters:

Return Value:
    Symbol rate
---------------------------------------------------------------------------- */
long Reg0299_RegGetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U8 sfrh[2];
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_SFRH, 3,sfrh);

    return Reg0299_CalcSymbolRate( DeviceMap, IOHandle, sfrh[0],sfrh[1],(sfrh[2]>>4));
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegIncrSymbolRate()

Description:
    add a correction to the symbol rate
Parameters:

Return Value:
    New symbol rate
---------------------------------------------------------------------------- */
long Reg0299_RegIncrSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, long correction)
{
    long Ltmp	  ;
    U8 sfrh[3],freq[3];

    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0299_SFRH, 3,sfrh);

    Ltmp  = (long)sfrh[0]<< 12;
    Ltmp +=(long)sfrh[1] <<  4;
    Ltmp +=(long)sfrh[2] >>  4;


    Ltmp += correction;

    freq[0]= (U8)((Ltmp>>12) & 0xFF);   /* cast to eliminate compiler warning --SFS */
    freq[1]=(U8)((Ltmp>>4)  & 0xFF);    /* cast to eliminate compiler warning --SFS */
    freq[2]=(U8)((Ltmp&0x0F)<<4);
    /*STTUNER_IOREG_SetFieldVal(DeviceMap, F0299_SYMB_FREQL,(int)( Ltmp      & 0x0F), (freq+2)); */        /* cast to eliminate compiler warning --SFS */
    STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle,  R0299_SFRH, freq,3);

    return	Reg0299_RegGetSymbolRate(DeviceMap, IOHandle);
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcAGC1TimeConstant()

Description:
    compute AGC1 time constant
Parameters:
    m1			->	AGC1 reference value
    fm			->	Master clock frequency (MHz)
    betaagc1	->	BeatAgc1
Return Value:
    AGC1 time constant (us)
---------------------------------------------------------------------------- */
long Reg0299_CalcAGC1TimeConstant(int m1, long fm, int betaagc1)
{
    long  Tagc1;

    if((m1==0)||(fm==0))
        return(0);

    Tagc1 = 67108864L;			/*	2^26	*/
    Tagc1 /= STTUNER_Util_PowOf2(betaagc1);	/*	2^betaagc1	*/
    Tagc1 /= m1;
    Tagc1 /= (fm/10000);		/*	Result is in mS*10	*/

    return Tagc1;
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetAGC1TimeConstant()

Description:
    compute AGC1 time constant
Parameters:
    m1			->	AGC1 reference value
    fm			->	Master clock frequency (MHz)
    betaagc1	->	BeatAgc1
Return Value:
    AGC1 time constant
---------------------------------------------------------------------------- */
long Reg0299_RegGetAGC1TimeConstant(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    long fm;
    U8 rcr[2];

    /*STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_AGC1C);
    STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_AGC1R);*/
    STTUNER_IOREG_GetContigousRegisters(DeviceMap,  IOHandle, R0299_RCR, 2,rcr);	/*	RCR, MCR */

    fm = Reg0299_CalcMasterClkFrequency( DeviceMap, STTUNER_IOREG_FieldExtractVal(rcr[1], F0299_STDBY),
                                                    STTUNER_IOREG_FieldExtractVal(rcr[0], F0299_DIRCLK),
                                                    STTUNER_IOREG_FieldExtractVal(rcr[0], F0299_K),
                                                    STTUNER_IOREG_FieldExtractVal(rcr[0], F0299_M),
                                                    STTUNER_IOREG_FieldExtractVal(rcr[1], F0299_P) );

    return Reg0299_CalcAGC1TimeConstant( STTUNER_IOREG_GetField(DeviceMap,IOHandle, F0299_AGC1_REF),
                                         fm,
                                         STTUNER_IOREG_GetField(DeviceMap,IOHandle, F0299_BETA_AGC1) );
}


/* ----------------------------------------------------------------------------
Name: Reg0299_CalcAGC2TimeConstant()

Description:
    compute AGC2 time constant
Parameters:
    agc2coef	->	AGC2 coeficient
    m1			->	AGC1 reference value
    fm			->	Master clock frequency (MHz)
Return Value:
    AGC2 time constant (ns)
---------------------------------------------------------------------------- */
long Reg0299_CalcAGC2TimeConstant(long agc2coef, long m1, long fm)
{
    long	BetaAgc2,
    Tagc2=0;

    if( (m1 != 0) && (fm != 0) )
    {
        BetaAgc2 = STTUNER_Util_PowOf2((int)(agc2coef-1)*2);  /* cast to eliminate compiler warning --SFS */

        Tagc2=60000*10000;
        Tagc2/=(fm/1000);
        Tagc2/=m1*BetaAgc2;
        Tagc2*=100;
    }

    return Tagc2;
}


/* ----------------------------------------------------------------------------
Name: Reg0299_RegGetAGC2TimeConstant()

Description:
    read registers then compute AGC2 time constant
Parameters:
Return Value:
    AGC2 time constant
---------------------------------------------------------------------------- */
long Reg0299_RegGetAGC2TimeConstant(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    long fm;
    U8  RCRTemp,MCRTemp,AGC2OTemp,AGC1Temp;
    AGC1Temp = STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_AGC1R);
    AGC2OTemp = STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_AGC2O);
    RCRTemp = STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_RCR);
    MCRTemp = STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, R0299_MCR);

    fm = Reg0299_CalcMasterClkFrequency(DeviceMap, STTUNER_IOREG_FieldExtractVal(MCRTemp, F0299_STDBY),
                                                   STTUNER_IOREG_FieldExtractVal(RCRTemp, F0299_DIRCLK),
                                                   STTUNER_IOREG_FieldExtractVal(RCRTemp, F0299_K),
                                                   STTUNER_IOREG_FieldExtractVal(RCRTemp, F0299_M),
                                                   STTUNER_IOREG_FieldExtractVal(MCRTemp, F0299_P));

    return Reg0299_CalcAGC2TimeConstant( STTUNER_IOREG_FieldExtractVal(AGC2OTemp, F0299_AGC2COEF),
                                         STTUNER_IOREG_FieldExtractVal(AGC1Temp, F0299_AGC1_REF),
                                         fm );
}


/*=====================================================
 **=====================================================
 **||||                                             ||||
 **||||           F22 GENERATION FACILITIES         ||||
 **||||                                             ||||
 **=====================================================
 **===================================================*/

void Reg0299_RegF22On(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
    /*RegSetField(F22OUTPUT, 1); */
}

void Reg0299_RegF22Off(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
    /*RegSetField(F22OUTPUT, 0);	*/
}


/*=====================================================
 **=====================================================
 **||||                                             ||||
 **||||           TRIGGER FACILITIES                ||||
 **||||                                             ||||
 **=====================================================
 **===================================================*/
/*
void Reg0299_RegTriggerOn(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    if(DeviceMap->RegTrigger == IOREG_YES)*/
        /* I/O = 1 */
/*        STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_DACMODE, 1);
}*/

/*
void Reg0299_RegTriggerOff(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    if(DeviceMap->RegTrigger == IOREG_YES)*/
        /* I/O = 0 */
      /*  STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0299_DACMODE, 0);
}
*/

/*STTUNER_IOREG_Flag_t Reg0299_RegGetTrigger(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
    return(DeviceMap->RegTrigger);
}


void Reg0299_RegSetTrigger(STTUNER_IOREG_DeviceMap_t *DeviceMap, STTUNER_IOREG_Flag_t Trigger)
{
    DeviceMap->RegTrigger = Trigger;
}
*/

/* End of reg0299.c */



