/*****************************************************************************
**
**  Name: mt2131.c
**
**  Description:    Microtune MT2131 BX Tuner software interface
**                  Supports tuners with Part/Rev codes:
**                  MT2131 B3 (0x3E) and MT2131 B4 (0x3F)
**
**  Functions
**  Implemented:    U32  MT2131_Open
**                  U32  MT2131_Close
**                  U32  MT2131_ChangeFreq
**                  U32  MT2131_GetGPO
**                  U32  MT2131_GetLocked
**                  U32  MT2131_GetParam
**                  U32  MT2131_GetReg
**                  U32  MT2131_GetTemp
**                  U32  MT2131_GetUserData
**                  U32  MT2131_ReInit
**                  U32  MT2131_ResetAGC
**                  U32  MT2131_SetExtSRO
**                  U32  MT2131_SetGPO
**                  U32  MT2131_SetParam
**                  U32  MT2131_SetPowerBits
**                  U32  MT2131_SetReg
**                  U32  MT2131_WaitForAGC
**
**  References:     AN-00075: MT2131 Programming Procedures Application Note
**                  MicroTune, Inc.
**                  AN-00010: MicroTuner Serial Interface Application Note
**                  MicroTune, Inc.
**
**  Exports:        None
**
**  Dependencies:   MT_ReadSub(hUserData, IC_Addr, subAddress, *pData, cnt);
**                  - Read byte(s) of data from the two-wire bus.
**
**                  MT_WriteSub(hUserData, IC_Addr, subAddress, *pData, cnt);
**                  - Write byte(s) of data to the two-wire bus.
**
**                  MT_Sleep(hUserData, nMinDelayTime);
**                  - Delay execution for nMinDelayTime milliseconds
**
**  CVS ID:         $Id: mt2131.c,v 1.1 2007/02/20 17:53:41 thomas Exp $
**  CVS Source:     $Source: /mnt/pinz/cvsroot/stgui/stv0362/Stb_lla_generic/lla/mt2131.c,v $
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**   N/A   05-02-2006    DAD    Ver 1.03: Added support for MT2131B3
**   N/A   05-12-2006    DAD    Ver 1.04: Off-Air analog changes:
**                                          PD2lev = 2 (was 1)
**   N/A   05-12-2006    DAD    Ver 1.04: Cable analog changes:
**                                          PD1lev = 6 (was 5)
**                                          AGCbld = 3 (was 4)
**   N/A   05-12-2006    DAD    Ver 1.04: Off-Air digital changes:
**                                          PD2lev = 2 (was 0)
**   N/A   05-12-2006    DAD    Ver 1.04: Cable digital changes:
**                                          PD1lev = 6 (was 5)
**                                          AGCbld = 3 (was 4)
**   N/A   06-12-2006    DAD    Ver 1.05: Cable analog changes:
**                                          AGCbld = 4 (was 3)
**   N/A   06-12-2006    DAD    Ver 1.05: Cable digital changes:
**                                          AGCbld = 4 (was 3)
**   N/A   07-18-2006    DAD    Ver 1.06: Support for MT2131 B4
**                              Remove support for B2 and earlier.
**   N/A   08-21-2006    DAD    Ver 1.07: Moved AGC optimization to a
**                              separate function to be called by the user
**   N/A   08-21-2006    DAD    Ver 1.07: Separate the AGC reset and wait
**                              operations into two functions.
**   N/A   08-29-2006    DAD    Ver 1.08: Turn on quick-tune
**   N/A   08-31-2006    DAD    Ver 1.09: Re-arrange register writes during
**                              frequency change to optimize time spent.
**   N/A   08-31-2006    DAD    Ver 1.09: Set IF1_BW to 18 MHz to match
**                              Launcher.
**   N/A   08-31-2006    DAD    Ver 1.09: Change output channel bandwidth to
**                              the actual channel bandwidth without the
**                              guard of 750 kHz.  The tuner driver will add
**                              the guard band internally.
**   N/A   08-31-2006    DAD    Ver 1.09: Use an average of 100 reads of the
**                              CapTrim register to adjust the ClearTune
**                              filters.
**   N/A   08-31-2006    DAD    Ver 1.09: Add support for SECAM, ISDB-T,
**                              and DMB-T.
**   N/A   08-31-2006    DAD    Ver 1.09: Only check for 1 PLL lock instead
**                              of 3 to declare PLL's locked.
**   N/A   09-25-2006    DAD    Ver 1.10: Updated LO1_FRACN_AVOID and
**                                        LO2_FRACN_AVOID parameters.
**   N/A   09-29-2006    DAD    Ver 1.11  Updated ClearTune filter cross-over
**                                        frequencies.
**   N/A   09-29-2006    DAD    Ver 1.12: Added MT2131_VGA_GAIN_MODE and
**                                        MT2131_LNAGAIN_MODE.
**   N/A   10-06-2006    DAD    Ver 1.13: Removed MT2131_SetUserData.
**   N/A   10-06-2006    DAD    Ver 1.13: Adjusted MT2131_Temperature.
**   N/A   10-10-2006    DAD    Ver 1.14: Added MT2131_SetDigitalCable2().
**   N/A   10-19-2006    DAD    Ver 1.15: Changed default for registers:
**                                          reg[0x0E] = 0x00 (was 0xC0).
**                                          reg[0x18] = 0x80 (was 0xC0).
**                                          reg[0x1B] = 0xA0 (was 0xE0).
**                                        Add routine to decrease LO2 lock
**                                        time.
**   N/A   10-24-2006    DAD    Ver 1.16: Shortcut exit if rcvr_mode already
**                                        set.
**   N/A   11-27-2006    DAD    Ver 1.17: Changed f_outbw parameter to no
**                              longer include the guard band.  Now f_outbw
**                              is equivalent to the actual channel bandwidth
**                              (i.e. 6, 7, or 8 MHz).
**   N/A   11-28-2006    DAD    Ver 1.18: Implement faster AGC settling
**                                        scheme.
**   N/A   12-15-2006    RSK    Ver 1.19: Implement fLO_FractionalTerm calc.
**   N/A   01-25-2007    DAD    Ver 1.20: Changed default for f_if1_Request
**                                        from IF1_CENTER to IF1_FREQ.
**
*****************************************************************************/
#include "stcommon.h"
#include "ioreg.h"
#include "chip.h"
#include <string.h>
#include <assert.h>
#include "tunshdrv.h"    /* header for this file */
#include<stdlib.h>
#ifdef STTUNER_DRV_SHARED_TUN_MT2131
#include "mt2131.h"
#endif



/*  Version of this module                         */
#define VERSION 10200             /*  Version 01.20 */

#define ceil(n, d) (((n) < 0) ? (-((-(n))/(d))) : (n)/(d) + ((n)%(d) != 0))
#define uceil(n, d) ((n)/(d) + ((n)%(d) != 0))
#define floor(n, d) (((n) < 0) ? (-((-(n))/(d))) - ((n)%(d) != 0) : (n)/(d))
#define ufloor(n, d) ((n)/(d))

/*
**  The expected version of MT_AvoidSpursData_t
**  If the version is different, an updated file is needed from Microtune
*/
/* Expecting version 1.20 of the Spur Avoidance API */
#define EXPECTED_MT_AVOID_SPURS_INFO_VERSION  010200



extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];



/*
**  The number of Tuner Registers
*/
#define END_REGS 32  /*no of registers*/
/*static const U32 Num_Registers = END_REGS;*/

#if MT_TUNER_CNT > 1
static MT_AvoidSpursData_t* TunerList[MT_TUNER_CNT];
static U32              TunerCount = 0;
#endif

/*static U32 nMaxTuners = MT2131_CNT;
static MT2131_Info_t MT2131_Info[MT2131_CNT];
static MT2131_Info_t *Avail[MT2131_CNT];
static U32 nOpenTuners = 0;
static U32 MT2131_SetLNAGain(MT2131_Info_t* pInfo, U32 lower_limit, U32upper_limit);
static U32 InitDone = 0;*/
static U32 MT2131_SetLNAGain(TUNSHDRV_InstanceData_t *Instance, U32 lower_limit, U32 upper_limit);
void MT_AddExclZone(MT_AvoidSpursData_t* pAS_Info,
                    U32 f_min,
                    U32 f_max);

/*
**                             |    Analog     |    Digital    |    Analog     |    Digital
**                             |  Ant  | Cable |  Ant  | Cable |  Ant  | Cable |  Ant  | Cable |
**                 ------------+-------+-------+-------+-------+-------+-------+-------+-------+
**                             | NTSC  | NTSC  | 8-VSB |  QAM  | SECAM | SECAM | ISDBT | ISDBT |
**                        Mode |   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |
**                 ------------+-------+-------+-------+-------+-------+-------+-------+-------+
*/
static const U8   VGAEN[] = {  0,      0,      1,      1,      0,      0,      1,      1};
static const U8   FDCEN[] = {  0,      1,      0,      1,      0,      1,      0,      1};
static const U8 LNAGAIN[] = {  1,      2,      0,      2,      1,      2,      0,      2};
static const U8   RLMAX[] = {127,     50,    127,     50,    127,     50,    127,     50};
static const U8  DNCMAX[] = {127,    127,    200,    127,    127,    127,    200,    127};
static const U8  PD1LEV[] = {  1,      6,      0,      6,      1,      6,      0,      6};
static const U8 PD1LEVX[] = {  0,      0,      1,      0,      0,      0,      1,      0};
static const U8  PD2LEV[] = {  2,      2,      2,      2,      2,      2,      2,      2};
static const U8 PD1HYST[] = {  1,      1,      1,      1,      1,      1,      1,      1};
static const U8 PD2HYST[] = {  1,      1,      1,      1,      1,      1,      1,      1};
static const U8 UPCILNA[] = {  1,      1,      1,      1,      1,      1,      1,      1};
static const U8 AGCBLD3[] = {  4,      4,      4,      4,      4,      4,      4,      4};
static const U8 AGCBLD4[] = {  4,      4,      4,      4,      1,      1,      1,      1};
static const U8 AGCCLK3[] = {  2,      2,      2,      2,      0,      0,      0,      0};
static const U8 AGCCLK4[] = {  2,      2,      2,      2,      2,      2,      2,      2};



U32 IsValidHandle(TUNSHDRV_InstanceData_t *Instance)
{
     return (Instance != NULL) ? 1 : 0;
}



/*****************************************/

 U32 UncheckedSet(TUNSHDRV_InstanceData_t *Instance,
                            U8         reg,
                            U8         val)
{
    U32 status = MT_OK; ;                  /* Status to be returned */

#if defined(_DEBUG)
    status = MT_OK;
    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
        status |= MT_INV_HANDLE;

    if (reg >= END_REGS)
        status |= MT_ARG_RANGE;

    if (MT_IS_ERROR(status))
        return (status);
#endif
             #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
              if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	          {
				   /* status = MT_WriteSub(pInfo->hUserData, pInfo->address, reg, &val, 1);*/
                   status = STTUNER_IOREG_SetRegister(&(Instance->DeviceMap), Instance->IOHandle,reg, val);
			   }
			    #endif
			    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
               if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
              {
              	   /* status = MT_WriteSub(pInfo->hUserData, pInfo->address, reg, &val, 1);*/
                status = ChipSetOneRegister(Instance->IOHandle,reg, val);
               }
               #endif


    if (MT_NO_ERROR(status))
    {
        Instance->TunerRegVal[reg] = val;
    }

    return (status);
}


/******************************************/

U32 UncheckedGet(TUNSHDRV_InstanceData_t *Instance,
                            U8   reg,
                            U8*  val)
{
    U32 status;                  /* Status to be returned        */

#if defined(_DEBUG)
    status = MT_OK;
    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
    {
        status |= MT_INV_HANDLE;
    }

    if (val == NULL)
    {
        status |= MT_ARG_NULL;
    }

    if (reg >= END_REGS)
    {
        status |= MT_ARG_RANGE;
    }

    if (MT_IS_ERROR(status))
    {
        return(status);
    }
#endif
 status = MT_OK;
      #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
	     /* status = MT_ReadSub(pInfo->hUserData, pInfo->address, reg, &pInfo->reg[reg], 1);*/
             Instance->TunerRegVal[reg] = STTUNER_IOREG_GetRegister(&(Instance->DeviceMap),Instance->IOHandle, reg);

		   }
		    #endif
		    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
        {
           /* status = MT_ReadSub(pInfo->hUserData, pInfo->address, reg, &pInfo->reg[reg], 1);*/
           Instance->TunerRegVal[reg] = ChipGetOneRegister(Instance->IOHandle, reg);

         }
         #endif


    if (MT_NO_ERROR(status))
    {
        *val = Instance->TunerRegVal[reg];
    }

    return (status);
}




/****************************************************************************
**
**  Name: MT2131_GetLocked
**
**  Description:    Checks to see if LO1 and LO2 are locked.
**
**  Parameters:     h            - Open handle to the tuner (from MT2131_Open).
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_UPC_UNLOCK     - Upconverter PLL unlocked
**                      MT_DNC_UNLOCK     - Downconverter PLL unlocked
**                      MT_COMM_ERR       - Serial bus communications error
**                      MT_INV_HANDLE     - Invalid tuner handle
**
**  Dependencies:   MT_ReadSub    - Read byte(s) of data from the serial bus
**                  MT_Sleep      - Delay execution for x milliseconds
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**
****************************************************************************/
BOOL MT2131_GetLocked(TUNSHDRV_InstanceData_t *Instance)
{
    const U32 nMaxWait = 200;            /*  wait a maximum of 200 msec  */
    const U32 nPollRate = 2;             /*  poll status bits every 2 ms */
    const U32 nMaxLoops = nMaxWait / nPollRate;
    U32 status = MT_OK;                  /* Status to be returned        */
    U32 nDelays = 0;

    if (IsValidHandle(Instance) == 0)
        return MT_INV_HANDLE;

    do
    {
      /*  status |= MT_ReadSub(pInfo->hUserData, pInfo->address, LO_STATUS, &pInfo->reg[LO_STATUS], 1);*/
status |= UncheckedGet(Instance, MT2131_LO_STATUS, &Instance->TunerRegVal[MT2131_LO_STATUS]);

       /* if (MT_IS_ERROR(status))
            return (status);

        if ((Instance->TunerRegVal[LO_STATUS]& 0x88) == 0x88)
            return (status);*/

	 if ((MT_IS_ERROR(status)) || ((Instance->TunerRegVal[MT2131_LO_STATUS]& 0x88) == 0x88))
	 	{
            return (TRUE);
         }

      /*  MT_Sleep(pInfo->hUserData, nPollRate);*/       /*  Wait between retries  */
    }
    while (++nDelays < nMaxLoops);

    if ((Instance->TunerRegVal[MT2131_LO_STATUS] & 0x80) == 0x00)
        status |= MT_UPC_UNLOCK;
    if ((Instance->TunerRegVal[MT2131_LO_STATUS] & 0x08) == 0x00)
        status |= MT_DNC_UNLOCK;

    return (status);
}






/******************************************************************************
**
**  Name: MT2131_OptimizeAGC
**
**  Description:    Optimize the MT2131 AGC settings based on signal conditions.
**
**                  Does not assume that the internal AGC routine has settled
**                  before entry.
**
**                  Internal AGC is not guaranteed to have settled upon exit.
**
**  Parameters:     h            - Open handle to the tuner (from MT2131_Open).
**
**  Usage:          status = MT2131_OptimizeAGC(hMT2131);
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_INV_HANDLE     - Invalid tuner handle
**                      MT_COMM_ERR       - Serial bus communications error
**
**  Dependencies:   MT_ReadSub - Read byte(s) of data from the two-wire bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   08-17-2006    DAD    Ver 1.07: Original, extracted from ChangeFreq.
**   N/A   09-29-2006    DAD    Ver 1.12: Added MT2131_VGA_GAIN_MODE and
**                                        MT2131_LNAGAIN_MODE.
**
******************************************************************************/
U32 MT2131_OptimizeAGC(TUNSHDRV_InstanceData_t *Instance)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    U8 o_UPC1;
    U8 o_UPC2;
    U8 o_PDET1;

    int bRegChanged = 0;

    if (IsValidHandle(Instance) == 0)
        return MT_INV_HANDLE;

    /*  Keep track of which registers are changed to minimize writing  */
    o_UPC1 = Instance->TunerRegVal[MT2131_UPC_1];
    o_UPC2 = Instance->TunerRegVal[MT2131_UPC_2];
    o_PDET1 = Instance->TunerRegVal[MT2131_PDET_1];

    /*  Set the UPC current to the default value (UPCILNA = 1) */
    Instance->TunerRegVal[MT2131_UPC_2] = (Instance->TunerRegVal[MT2131_UPC_2] & ~0x0C) | (1 << 2);

    switch (Instance->MT2131_Info.rcvr_mode)
    {
    case MT2131_ANALOG_OFFAIR:
    case MT2131_ANALOG_OFFAIR_2:
        /*
        **  Analog Off-Air only
        **
        **  RFBand   Nominal value ->                                       | Offset by 1 ->
        **  UPCILNA  1 ->                                              <- 1 | 0 ->
        **  LNAGain  0 |XXX| 1 ->
        **          |--+---+------------------------------------------------+----------------->
        **  gcAttn  0       100       200       300       400       500       600       700
        **
        **          <--  smaller signals                                    larger signals  -->
        **
        */
    case MT2131_DIGITAL_OFFAIR:
    case MT2131_DIGITAL_OFFAIR_2:
        /*
        **  Digital Off-Air only
        **
        **  RFBand    Nominal value ->                                      | Offset by 1 ->
        **  UPCILNA   1 ->                                             <- 1 | 0 ->
        **  LNAGain   0 ->                                    <- 0 |XXX| 1 ->
        **          |---+--+------------------------------------------------+----------------->
        **  gcAttn  0       100       200       300       400       500       600       700
        **
        **          <--  smaller signals                                    larger signals  -->
        */

        /*
        **  Make sure that the internal RF AGC circuit has finished.  We are
        **  reading the attenuator values and they need to be settled first.
        */
        status |= MT2131_WaitForAGC(Instance);
        o_PDET1 = Instance->TunerRegVal[MT2131_PDET_1];       /*  PDET_1 changed by WaitForAGC()  */

        /*
        **  Get the current gcATTN value
        **  In the following code, when checking gcATTN we are only using the
        **  8 MSB's of the register field and ignoring the two LSB's that are
        **  located in UPC_2.  This will minimize the I2C-bus traffic without
        **  affecting performance.
        **  Note that the comparisons to the AGC_1 register are 1/4 the value
        **  compared for the entire gcATTN register field.
        */
        if  ((Instance->MT2131_Info.rcvr_mode == MT2131_ANALOG_OFFAIR)
          || (Instance->MT2131_Info.rcvr_mode == MT2131_ANALOG_OFFAIR_2))
            /*  NOTE: MT2131_SetLNAGain does not write the changed LNAGain  */
            status |= MT2131_SetLNAGain(Instance, 24, 72);
        else
            /*  NOTE: MT2131_SetLNAGain does not write the changed LNAGain  */
            status |= MT2131_SetLNAGain(Instance, 448, 524);

        /*  Check for very large signal  */
        if (Instance->TunerRegVal[MT2131_AGC_1] >= 147)
        {
            /*  Increase the LNA current (UPCILNA = 0) */
            Instance->TunerRegVal[MT2131_UPC_2] &= 0xF3;
            /*
            **  Very large signal, mis-tune the ClearTune filter by one
            **  to attenuate the power a little bit
            */
            if ((Instance->TunerRegVal[MT2131_UPC_1] & 0x1F) > 0)
                --Instance->TunerRegVal[MT2131_UPC_1];
            else
                ++Instance->TunerRegVal[MT2131_UPC_1];
        }

        break;

    case MT2131_ANALOG_CABLE:
    case MT2131_DIGITAL_CABLE:
    case MT2131_ANALOG_CABLE_2:
    case MT2131_DIGITAL_CABLE_2:
        /*
        **  For low-density channel spectrum (51 to 85 MHz), we're
        **  increasing the PD1lev setting.
        */
        if ((Instance->MT2131_Info.AS_Data.f_in >= 51000000)
            && (Instance->MT2131_Info.AS_Data.f_in <= 85000000))
            /*  PD1Lev = 7  */
           Instance->TunerRegVal[MT2131_PDET_1] &= 0xE0;
        else
            /*  PD1Lev = 6  */
            Instance->TunerRegVal[MT2131_PDET_1] = (Instance->TunerRegVal[MT2131_PDET_1] & ~0xE0) | (6 << 5);

        break;

    default:
        break;
    }

    /*  Write UPC_1 & UPC_2 out if changed  */
    if ((Instance->TunerRegVal[MT2131_UPC_1] != o_UPC1) || (Instance->TunerRegVal[MT2131_UPC_2] != o_UPC2))
    {
    	#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
	      /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, UPC_1, &pInfo->reg[UPC_1], 2);*/
	     status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle, MT2131_UPC_1,&Instance->TunerRegVal[MT2131_UPC_1],2);

		   }
		    #endif
		    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
        {
            /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, UPC_1, &pInfo->reg[UPC_1], 2);*/
	     status |= ChipSetRegisters (Instance->IOHandle, MT2131_UPC_1,&Instance->TunerRegVal[MT2131_UPC_1],2);

         }
         #endif
      

        bRegChanged = 1;
    }

    /*  Write PDET_1 out if changed  */
    if (Instance->TunerRegVal[MT2131_PDET_1] != o_PDET1)
    {
        /*status |= MT_WriteSub(pInfo->hUserData, pInfo->address, PDET_1, &pInfo->reg[PDET_1], 1);*/
		status |= UncheckedSet(Instance, MT2131_PDET_1, Instance->TunerRegVal[MT2131_PDET_1] );

        bRegChanged = 1;
    }

    if (MT_NO_ERROR(status)
        && (bRegChanged != 0)
        && (Instance->MT2131_Info.VGA_gain_mode == MT2131_VGA_AUTO))
        /*  Wait for the AGC to settle  */
        status |= MT2131_WaitForAGC(Instance);

    if (MT_NO_ERROR(status) && (Instance->MT2131_Info.VGA_gain_mode == MT2131_VGA_AUTO))
    {
        /*  Check VGAGOK bit  */
        /*status |= MT_ReadSub(pInfo->hUserData, pInfo->address, AFC, &pInfo->reg[AFC], 1);*/
		 status |= UncheckedGet(Instance, MT2131_AFC, &Instance->TunerRegVal[ MT2131_AFC]);
        if (MT_NO_ERROR(status) && ((Instance->TunerRegVal[MT2131_AFC] & 0x40) == 0x00))
        {
            /*  if VGAGOK bit is OFF, toggle state of VGAHiGain  */
            Instance->TunerRegVal[MT2131_MISC_1] ^= 0x40;
            /*status |= MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_1, &pInfo->reg[MISC_1], 1);*/
			status |= UncheckedSet(Instance, MT2131_MISC_1, Instance->TunerRegVal[MT2131_MISC_1] );

        }
    }

    return (status);
}









/******************************************************************************
**
**  Name: MT2131_ReInit
**
**  Description:    Initialize the tuner's register values.
**
**  Parameters:     h           - Tuner handle (returned by MT2131_Open)
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_TUNER_ID_ERR   - Tuner Part/Rev code mismatch
**                      MT_INV_HANDLE     - Invalid tuner handle
**                      MT_COMM_ERR       - Serial bus communications error
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**   N/A   05-12-2006    DAD    Ver 1.04: Cable digital changes:
**                                          PD1lev = 6 (was 5)
**                                          AGCbld = 3 (was 4)
**   N/A   06-12-2006    DAD    Ver 1.05: Cable digital changes:
**                                          AGCbld = 4 (was 3)
**   N/A   08-29-2006    DAD    Ver 1.08: Turn on quick-tune
**   N/A   09-29-2006    DAD    Ver 1.11  Updated ClearTune filter cross-over
**                                        frequencies.
**   N/A   09-29-2006    DAD    Ver 1.12: Added MT2131_VGA_GAIN_MODE and
**                                        MT2131_LNAGAIN_MODE.
**   N/A   10-19-2006    DAD    Ver 1.15: Changed default for registers:
**                                          reg[0x0E] = 0x00 (was 0xC0).
**                                          reg[0x18] = 0x80 (was 0xC0).
**                                          reg[0x1B] = 0xA0 (was 0xE0).
**                                        Add routine to decrease LO2 lock
**                                        time.
**   N/A   01-25-2007    DAD    Ver 1.20: Changed default for f_if1_Request
**                                        from IF1_CENTER to IF1_FREQ.
**
******************************************************************************/
U32 MT2131_ReInit(TUNSHDRV_InstanceData_t *Instance)
{
    /*
    **  Nominal RF Band cross-over frequencies
    */
    U32 MT2131_RF_Default_Bands[] =
    {
         77000000,     /*    0.0 ..   77.0 MHz: b00000   */
        133600000,     /*   77.0 ..  133.6 MHz: b00001   */
        185400000,     /*  133.6 ..  185.4 MHz: b00010   */
        237100000,     /*  185.4 ..  237.1 MHz: b00011   */
        290100000,     /*  237.1 ..  290.1 MHz: b00100   */
        344700000,     /*  290.1 ..  344.7 MHz: b00101   */
        400600000,     /*  344.7 ..  400.6 MHz: b00110   */
        456900000,     /*  400.6 ..  456.9 MHz: b00111   */
        512100000,     /*  456.9 ..  512.1 MHz: b01000   */
        572900000,     /*  512.1 ..  572.9 MHz: b01001   */
        637900000,     /*  572.9 ..  637.9 MHz: b01010   */
        693300000,     /*  637.9 ..  693.3 MHz: b01011   */
        749100000,     /*  693.3 ..  749.1 MHz: b01100   */
        803100000,     /*  749.1 ..  803.1 MHz: b01101   */
        856200000,     /*  803.1 ..  856.2 MHz: b01110   */
        908700000,     /*  856.2 ..  908.7 MHz: b01111   */
        962000000,     /*  908.7 ..  962.0 MHz: b10000   */
       1014100000,     /*  962.0 .. 1014.1 MHz: b10001   */
       1065500000,     /* 1014.1 .. 1065.5 MHz: b10010   */
                        /* 1065.5 ..        MHz: b10011   */
    };

    U8 MT2131_Init_Defaults[] =
    {

        0x50,            /* reg[0x01] <- 0x50 */
        0x00,            /* reg[0x02] <- 0x00 */
        0x50,            /* reg[0x03] <- 0x50 */
        0x80,            /* reg[0x04] <- 0x80 */
        0x00,            /* reg[0x05] <- 0x00 */
        0x49,            /* reg[0x06] <- 0x49 */
        0xFA,            /* reg[0x07] <- 0xFA */
        0x88,            /* reg[0x08] <- 0x88 */
        0x08,            /* reg[0x09] <- 0x08 */
        0x77,            /* reg[0x0A] <- 0x77 */
        0x41,            /* reg[0x0B] <- 0x41 */
        0x04,            /* reg[0x0C] <- 0x04 */
        0x00,            /* reg[0x0D] <- 0x00 */
        0x00,            /* reg[0x0E] <- 0x00 */
        0x00,            /* reg[0x0F] <- 0x00 */
        0x32,            /* reg[0x10] <- 0x32 */
        0x7F,            /* reg[0x11] <- 0x7F */
        0xDA,            /* reg[0x12] <- 0xDA */
        0x4C,            /* reg[0x13] <- 0x4C */
        0x00,            /* reg[0x14] <- 0x00 */
        0x10,            /* reg[0x15] <- 0x10 */
        0xAA,            /* reg[0x16] <- 0xAA */
        0x78,            /* reg[0x17] <- 0x78 */
        0x80,            /* reg[0x18] <- 0x80 */
        0xFF,            /* reg[0x19] <- 0xFF */
        0x68,            /* reg[0x1A] <- 0x68 */
        0xA0,            /* reg[0x1B] <- 0xA0 */
        0xFF,            /* reg[0x1C] <- 0xFF */
        0xDD,            /* reg[0x1D] <- 0xDD */
        0x00,            /* reg[0x1E] <- 0x00 */
        0x00            /* reg[0x1F] <- 0x00 */

        /*MT2131*/
        /*0xa7,0x1f,0x75,0xc7,0x1f,0x49,0xf2,0x88,0xc2,0x05,0x0b,0x05,0x0a,0x7f,0xc8,0x7f,0xc8,0x1a,0x4c,0x00,0x04,0xaa,0x78,0xab,0xd5,0x68,0xf2,0xff,0xdd,0x00,0x7f*/

    };

    U32 i;
    U8 reg;
	U8 tmpreg = 0;
    U32 status = MT_OK;                  /* Status to be returned        */


    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
        status |= MT_INV_HANDLE;

    /*  Read the Part/Rev code from the tuner */
    if (MT_NO_ERROR(status))
        status |=  UncheckedGet(Instance, MT2131_PART_REV, &Instance->TunerRegVal[MT2131_PART_REV]);

    if (MT_NO_ERROR(status)
        && ((Instance->TunerRegVal[MT2131_PART_REV]  != MT2131_B3)
            && (Instance->TunerRegVal[MT2131_PART_REV] != MT2131_B4)))
            status |= MT_TUNER_ID_ERR;      /*  Wrong tuner Part/Rev code   */

    if (MT_NO_ERROR(status))
    {
    	#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
	      /*  Write the default values to each of the tuner registers.  */
       /* status |= MT_WriteSub(pInfo->hUserData,
                              pInfo->address,
                              LO1C_1,
                              MT2131_Init_Defaults,
                              sizeof(MT2131_Init_Defaults));*/
      status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle,MT2131_LO1C_1,MT2131_Init_Defaults,sizeof(MT2131_Init_Defaults));
		   }
		    #endif
		    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
        {
            /*  Write the default values to each of the tuner registers.  */
       /* status |= MT_WriteSub(pInfo->hUserData,
                              pInfo->address,
                              LO1C_1,
                              MT2131_Init_Defaults,
                              sizeof(MT2131_Init_Defaults));*/
      status |= ChipSetRegisters (Instance->IOHandle,MT2131_LO1C_1,MT2131_Init_Defaults,sizeof(MT2131_Init_Defaults));
         }
         #endif

        
    }

    /*  Read back all the registers from the tuner                    */
    if (MT_NO_ERROR(status))
    	    	#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
	        /*  status |= MT_ReadSub(pInfo->hUserData,
                             pInfo->address,
                             PART_REV,
                             pInfo->reg,
                             END_REGS);*/
	   status |= STTUNER_IOREG_GetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle, MT2131_PART_REV,END_REGS,Instance->TunerRegVal);

		   }
		    #endif
		    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
        {
              /*  status |= MT_ReadSub(pInfo->hUserData,
                             pInfo->address,
                             PART_REV,
                             pInfo->reg,
                             END_REGS);*/
	   status |= ChipGetRegisters (Instance->IOHandle, MT2131_PART_REV,END_REGS,Instance->TunerRegVal);

         }
         #endif
    

    /*  Read the CapTrim register 100X and record the result  */
    if (MT_NO_ERROR(status))
    {
        /*  Turn on CapTrim reset bit  */
        /*pInfo->reg[MISC_2]*/Instance->TunerRegVal[MT2131_MISC_2] |= 0x10;

        for (i=0; i<100; ++i)
        {
            /*  Reset the CapTrim circuit, make new measurement  */
           /* status |= MT_WriteSub(pInfo->hUserData,
                                  pInfo->address,
                                  MISC_2,
                                  &pInfo->reg[MISC_2],
                                  1);*/
                status |=  UncheckedSet(Instance, MT2131_MISC_2, Instance->TunerRegVal[MT2131_MISC_2]);
            /*  Read result  */
           /* status |= MT_ReadSub(pInfo->hUserData,
                                 pInfo->address,
                                 CAPTRIM,
                                 &pInfo->reg[CAPTRIM],
                                 1);*/
                status |=  UncheckedGet(Instance, MT2131_CAPTRIM, &Instance->TunerRegVal[MT2131_CAPTRIM]);
            if (MT_IS_ERROR(status))
                break;

            Instance->MT2131_Info.CTrim_sum+= tmpreg/*pInfo->reg[CAPTRIM]*/;
        }

        /*  Turn off CapTrim reset bit in the cache  */
        /*pInfo->reg[MISC_2]*/Instance->TunerRegVal[MT2131_MISC_2]&= (~0x10);
    }

    /*
    **  Adjust the default ClearTune cross-over frequencies based on the
    **  average CapTrim readback.
    */
    if (MT_NO_ERROR(status))
    {
        if ((Instance->MT2131_Info.CTrim_sum  < 10000) || (Instance->MT2131_Info.CTrim_sum  > 16000))
        {
            for ( i = 0; i < 19; i++ )
                Instance->MT2131_Info.RF_Bands[i] = MT2131_RF_Default_Bands[i];
        }
        else
        {
            for ( i = 0; i < 19; i++ )
               Instance->MT2131_Info.RF_Bands[i] = MT2131_RF_Default_Bands[i] + (S32) ((S32) Instance->MT2131_Info.CTrim_sum - (0x80 * 100)) * (MT2131_RF_Default_Bands[i] / (0x80 * 100));
        }
    }
    else
        Instance->MT2131_Info.CTrim_sum = 0;

    /*
    **  Improve LO2 lock speed
    */
    if (MT_NO_ERROR(status))
    {

        if ((/*pInfo->reg[LO_STATUS]*/Instance->TunerRegVal[MT2131_LO_STATUS] & 0x88) != 0x88)
        {
            /*  Re-read register 0x1B again  */
           /* status |= MT_ReadSub(pInfo->hUserData,
                                 pInfo->address,
                                 RSVD_1B,
                                 &pInfo->reg[RSVD_1B],
                                 1);*/
             status |=  UncheckedGet(Instance, MT2131_RSVD_1B, &Instance->TunerRegVal[MT2131_RSVD_1B]);
        }

        if (MT_NO_ERROR(status))
        {
            reg =Instance->TunerRegVal[MT2131_RSVD_1B] & ~0xE0;

            reg += ((Instance->TunerRegVal[MT2131_TEMP_STATUS] & 0x0F) < 3) ? 2 : 1;
            if (reg > 31)
                reg = 31;
            status |=  UncheckedSet(Instance, MT2131_RSVD_1B, 0xE0 | reg);
        }
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2131_ResetAGC
**
**  Description:    Reset the MT2131 AGC firmware.
**
**  Parameters:     h            - Open handle to the tuner (from MT2131_Open).
**
**  Usage:          status = MT2131_ResetAGC(hMT2131);
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_INV_HANDLE     - Invalid tuner handle
**                      MT_COMM_ERR       - Serial bus communications error
**
**  Dependencies:   MT_ReadSub - Read byte(s) of data from the two-wire bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   08-02-2006    DAD    nTries loop variable was not incremented.
**   N/A   08-16-2006    DAD    Don't wait for AGCdone if AGC is disabled.
**   N/A   08-21-2006    DAD    Ver 1.07: Separate the AGC reset and wait
**                              operations into two functions.
**
******************************************************************************/
U32 MT2131_ResetAGC(TUNSHDRV_InstanceData_t *Instance)
{
    U32 status = MT_OK;                  /* Status to be returned        */

    if (IsValidHandle(Instance) == 0)
        return MT_INV_HANDLE;

    /*  Reset the AGC  */
    Instance->TunerRegVal[MT2131_MISC_2] |= 0x40;
    /*status |= MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_2, &pInfo->reg[MISC_2], 1);*/
	status |=  UncheckedSet(Instance, MT2131_MISC_2,Instance->TunerRegVal[MT2131_MISC_2] );

    /*  Clear the reset bit in the cache  */
     Instance->TunerRegVal[MT2131_MISC_2] &= (~0x40);

    return (status);
}

/******************************************************************************
**
**  Name: MT2131_SetLNAGain
**
**  Description:    Set the MT2131 LNAGain setting based on gcATTN values.
**
**                  The LNAGain is unchanged unless the LNAGain mode is
**                  set to MT2131_LNA_AUTO.
**
**                  The lower_limit and upper_limit arguments define three
**                  ranges of gcATTN as shown in the figure below.
**
**                  Below the lower_limit, the LNAGain is set to 0.
**                  Above the upper_limit, the LNAGain is set to 1.
**                  In between, the LNAGain is unchanged.
**
**                      | Set LNAGain to 0 | Don't Change | Set LNAGain to 1 |
**              LNAGain |<- 0 ------------>|XXXXXXXXXXXXXX|<- 1 ------------>|
**                      |------------------+--------------+----------------->|
**              gcATTN  0                lower_         upper_
**                                       limit          limit
**
**                       <--  smaller signals              larger signals  -->
**
**                  NOTE:  DOES NOT WRITE OUT THE NEW UPC_1 REGISTER VALUE!
**
**  Parameters:     pInfo        - ptr to MT2131_Info_t structure
**                  lower_limit  - lower limit of gcATTN
**                  upper_limit  - upper limit of gcATTN
**
**  Usage:          status = MT2131_SetGPO(hMT2131, 1);
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_COMM_ERR       - Serial bus communications error
**
**  Dependencies:   MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   09-29-2006    DAD    Ver 1.12: Added MT2131_VGA_GAIN_MODE and
**                                        MT2131_LNAGAIN_MODE.
**
******************************************************************************/
static U32 MT2131_SetLNAGain(TUNSHDRV_InstanceData_t *Instance, U32 lower_limit, U32 upper_limit)
{
    U32 status = MT_OK;                  /* Status to be returned        */

    /*  If the LNAGain mode is set for automatic  */
    if (Instance->MT2131_Info.LNAgain_mode == MT2131_LNA_AUTO)
    {
        /*
        **  Get the current gcATTN value
        **  In the following code, when checking gcATTN we are only using the
        **  8 MSB's of the register field and ignoring the two LSB's that are
        **  located in UPC_2.  This will minimize the I2C-bus traffic without
        **  affecting performance.
        **  Note that the comparisons to the AGC_1 register are 1/4 the value
        **  compared for the entire gcATTN register field.
        */
       /* status |= MT_ReadSub(pInfo->hUserData, pInfo->address, AGC_1, &pInfo->reg[AGC_1], 1);*/
		 status |= UncheckedGet(Instance, MT2131_AGC_1,&Instance->TunerRegVal[MT2131_AGC_1]);

        /*  Check for channel power below the lower gcATTN limit  */
        if (Instance->TunerRegVal[MT2131_AGC_1] < (lower_limit / 4))
        {
            /*  Set LNAGain to back to the default for Off-Air channels  */
            /*  then set the LNAGain register   */
            Instance->TunerRegVal[MT2131_UPC_1] = (Instance->TunerRegVal[MT2131_UPC_1] & ~0x60)
                              | (LNAGAIN[Instance->MT2131_Info.rcvr_mode] << 5);
        }
        /*  Check for medium or larger signal  */
        else if (Instance->TunerRegVal[MT2131_AGC_1] > (upper_limit / 4))
        {
            /*  Set LNAGain to 1  */
            Instance->TunerRegVal[MT2131_UPC_1] = (Instance->TunerRegVal[MT2131_UPC_1] & ~0x60) | (1 << 5);
        }
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2131_SetRcvrMode
**
**  Description:    Set the MT2131 reception mode
**
**                |    Analog     |    Digital    |    Analog     |    Digital
**                |  Ant  | Cable |  Ant  | Cable |  Ant  | Cable |  Ant  | Cable |
**    ------------+-------+-------+-------+-------+-------+-------+-------+-------+
**                | NTSC  | NTSC  | 8-VSB |  QAM  | SECAM | SECAM | ISDBT | ISDBT |
**    Mode        |   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |
**    ------------+-------+-------+-------+-------+-------+-------+-------+-------+
**    VGAen       |  OFF  |  OFF  |   ON  |   ON  |  OFF  |  OFF  |   ON  |   ON  |
**    FDCen       |  OFF  |   ON  |  OFF  |   ON  |  OFF  |   ON  |  OFF  |   ON  |
**    LNAGain     | -1.6  | -2.7  | -0.0  | -2.7  | -1.6  | -2.7  | -0.0  | -2.7  |
**    RLmax       |  127  |   50  |  127  |   50  |  127  |   50  |  127  |   50  |
**    DNCmax      |  127  |  127  |  200  |  127  |  127  |  127  |  200  |  127  |
**    PD1lev      |    1  |    6  |    0  |    6  |    1  |    6  |    0  |    6  |
**    PD1levEx    |    0  |    0  |    1  |    0  |    0  |    0  |    1  |    0  |
**    PD2lev      |    2  |    2  |    2  |    2  |    2  |    2  |    2  |    2  |
**    PD1hyst     |    1  |    1  |    1  |    1  |    1  |    1  |    1  |    1  |
**    PD2hyst     |    1  |    1  |    1  |    1  |    1  |    1  |    1  |    1  |
**    UPCILNA     |    1  |    1  |    1  |    1  |    1  |    1  |    1  |    1  |
**    AGCbld (B3) |    4  |    4  |    4  |    4  |    4  |    4  |    4  |    4  |
**    AGCbld (B4) |    4  |    4  |    4  |    4  |    1  |    1  |    1  |    1  |
**    AGCclk (B3) |    2  |    2  |    2  |    2  |    0  |    0  |    0  |    0  |
**    AGCclk (B4) |    2  |    2  |    2  |    2  |    2  |    2  |    2  |    2  |
**                 ^^^^^^^
**
**  Parameters:     pInfo        - ptr to MT2131_Info_t structure
**                  rcvr_mode    - receiver mode to be set
**
**  Usage:          status = MT2131_SetRcvrMode(hMT2131, MT2131_ANALOG_CABLE);
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_COMM_ERR       - Serial bus communications error
**
**  Dependencies:   MT_WriteSub - Write byte(s) of data to the two-wire bus
**                  MT2131_ResetAGC - Reset MT2131 AGC
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**   N/A   05-12-2006    DAD    Ver 1.04: Off-Air analog changes:
**                                          PD2lev = 2 (was 1)
**   N/A   09-29-2006    DAD    Ver 1.12: Added MT2131_VGA_GAIN_MODE and
**                                        MT2131_LNAGAIN_MODE.
**   N/A   10-24-2006    DAD    Ver 1.16: Shortcut exit if rcvr_mode already
**                                        set.
**   N/A   11-29-2006    DAD    Ver 1.18: Consolidated all separate receiver
**                                        mode set functions.
**
******************************************************************************/
static U32 MT2131_SetRcvrMode(TUNSHDRV_InstanceData_t *Instance, MT2131_Rcvr_Mode rcvr_mode)
{

    U32 status = MT_OK;                  /* Status to be returned        */
    const U8 o_PWR = Instance->TunerRegVal[MT2131_PWR]/*pInfo->reg[PWR]*/;
    const U8 o_UPC1 = Instance->TunerRegVal[MT2131_UPC_1];
    const U8 o_UPC2 = Instance->TunerRegVal[MT2131_UPC_2];
    const U8 o_MISC2 = Instance->TunerRegVal[MT2131_MISC_2];
    const U8 o_AGCRL = Instance->TunerRegVal[MT2131_AGC_RL];
    const U8 o_AGCDNC = Instance->TunerRegVal[MT2131_AGC_DNC];
    const U8 o_PDET1 = Instance->TunerRegVal[MT2131_PDET_1];
    const U8 o_PDET2 = Instance->TunerRegVal[MT2131_PDET_2];
    U32 bRegChanged = 0;

    /*  VGAen, FDCen  */
    Instance->TunerRegVal[MT2131_PWR] = (Instance->TunerRegVal[MT2131_PWR] & ~0x18)
                    | (VGAEN[rcvr_mode] << 4)
                    | (FDCEN[rcvr_mode] << 3);

    /*  If the LNAGain mode is set for automatic  */
    if (Instance->MT2131_Info.LNAgain_mode == MT2131_LNA_AUTO)
        /*  then set the LNAGain register   */
       Instance->TunerRegVal[MT2131_UPC_1] = (Instance->TunerRegVal[MT2131_UPC_1] & ~0x60)
                          | (LNAGAIN[rcvr_mode] << 5);

    /*  UPCILNA  */
   Instance->TunerRegVal[MT2131_UPC_2] = (Instance->TunerRegVal[MT2131_UPC_2] & ~0x0C)
                      | (UPCILNA[rcvr_mode] << 2);
    /*  PD1levEx  */
    Instance->TunerRegVal[MT2131_MISC_2] = (Instance->TunerRegVal[MT2131_MISC_2] & ~0x04)
                      | (PD1LEVX[rcvr_mode] << 2);
    /*  RLmax  */
    Instance->TunerRegVal[MT2131_AGC_RL] = RLMAX[rcvr_mode];
    /*  DNCmax  */
    Instance->TunerRegVal[MT2131_AGC_DNC] = DNCMAX[rcvr_mode];
    /*  PD1lev, AGCCrst, PD1hyst, AGCclk  */
    Instance->TunerRegVal[MT2131_PDET_1] = 0x10
                       | (PD1LEV[rcvr_mode] << 5)
                       | (PD1HYST[rcvr_mode] << 3)
                       | (Instance->TunerRegVal[MT2131_PART_REV] == MT2131_B3 ? AGCCLK3[rcvr_mode] : AGCCLK4[rcvr_mode]);
    /*  PD2lev, AGCtgt, PD2hyst, AGCbld  */
    Instance->TunerRegVal[MT2131_PDET_2] = 0x00
                       | (PD2LEV[rcvr_mode] << 5)
                       | (PD2HYST[rcvr_mode] << 3)
                       | (Instance->TunerRegVal[MT2131_PART_REV] == MT2131_B3 ? AGCBLD3[rcvr_mode] : AGCBLD4[rcvr_mode]);

    /*  Write out new AGC settings, if changed  */
    /*
    **  Changing this register does not affect internal AGC,
    **  so do not set bRegChanged
    */
    if (MT_NO_ERROR(status) & (Instance->TunerRegVal[MT2131_PWR] != o_PWR))
      /*  status |= MT_WriteSub(pInfo->hUserData, pInfo->address, PWR, &pInfo->reg[PWR], 1);*/
	status |= UncheckedSet(Instance, MT2131_PWR, Instance->TunerRegVal[MT2131_PWR]);

    if (MT_NO_ERROR(status) & ((Instance->TunerRegVal[MT2131_UPC_1] != o_UPC1)
                            || (Instance->TunerRegVal[MT2131_UPC_2] != o_UPC2)))
    {
    	#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
			 /*  status |= MT_WriteSub(pInfo->hUserData, pInfo->address, UPC_1, &pInfo->reg[UPC_1], 2);*/
				    status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle, MT2131_UPC_1,&Instance->TunerRegVal[MT2131_UPC_1],2);

		   }
		    #endif
		   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
        {
		 /*  status |= MT_WriteSub(pInfo->hUserData, pInfo->address, UPC_1, &pInfo->reg[UPC_1], 2);*/
			    status |= ChipSetRegisters (Instance->IOHandle, MT2131_UPC_1,&Instance->TunerRegVal[MT2131_UPC_1],2);
         }
         #endif
     
        bRegChanged = 1;
    }
    if (MT_NO_ERROR(status) & ((Instance->TunerRegVal[MT2131_AGC_RL] != o_AGCRL)
                            || (Instance->TunerRegVal[MT2131_AGC_DNC] != o_AGCDNC)
                            || (Instance->TunerRegVal[MT2131_PDET_1] != o_PDET1)
                            || (Instance->TunerRegVal[MT2131_PDET_2] != o_PDET2)))
    {
    	#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
			 /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, AGC_RL, &pInfo->reg[AGC_RL], 4);*/
	     status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle, MT2131_AGC_RL,&Instance->TunerRegVal[MT2131_AGC_RL],4);

		   }
		    #endif
		   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
        {
		 /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, AGC_RL, &pInfo->reg[AGC_RL], 4);*/
	     status |= ChipSetRegisters (Instance->IOHandle, MT2131_AGC_RL,&Instance->TunerRegVal[MT2131_AGC_RL],4);
         }
         #endif
       

        bRegChanged = 1;
    }
    if (MT_NO_ERROR(status) & (Instance->TunerRegVal[MT2131_MISC_2] != o_MISC2))
    {
        /*status |= MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_2, &pInfo->reg[MISC_2], 1);*/
		status |= UncheckedSet(Instance, MT2131_MISC_2, Instance->TunerRegVal[MT2131_MISC_2]);
        bRegChanged = 1;
    }

    if (MT_NO_ERROR(status))
       Instance->MT2131_Info.rcvr_mode = rcvr_mode;

    /*  Reset the AGC  */
    if (MT_NO_ERROR(status) && (bRegChanged != 0))
    {
/*        status |= MT2131_FastResetAGC(pInfo);  */
        status |= MT2131_ResetAGC(Instance);

        if (MT_NO_ERROR(status))
            status |= MT2131_OptimizeAGC(Instance);

        /*  We want to call this after FastResetAGC() regardless of any errors  */
        status |= MT2131_WaitForAGC(Instance);
    }

    return (status);
}


/****************************************************************************
**
**  Name: MT2131_SetParam
**
**  Description:    Sets a tuning algorithm parameter.
**
**                  This function provides access to the internals of the
**                  tuning algorithm.  You can override many of the tuning
**                  algorithm defaults using this function.
**
**  Parameters:     h           - Tuner handle (returned by MT2131_Open)
**                  param       - Tuning algorithm parameter
**                                (see enum MT2131_Param)
**                  nValue      - value to be set
**
**                  param                   Description
**                  ----------------------  --------------------------------
**                  MT2131_RCVR_MODE        Receiver Mode
**                  MT2131_RLMAX            R-Load Maximum Value
**                  MT2131_DNCMAX           Down Converter Maximum Value
**                  MT2131_AGCBLD           AGC Bleed Current
**                  MT2131_PD1LEV           Power Detector #1 target level
**                  MT2131_PD2LEV           Power Detector #2 target level
**                  MT2131_LNAGAIN          LNA Gain level
**                  MT2131_SRO_FREQ         crystal frequency
**                  MT2131_STEPSIZE         minimum tuning step size
**                  MT2131_LO1_STEPSIZE     LO1 minimum step size
**                  MT2131_LO1_FRACN_AVOID  LO1 FracN keep-out region
**                  MT2131_IF1_REQUEST      Requested 1st IF
**                  MT2131_IF1_CENTER       Center of 1st IF SAW filter
**                  MT2131_IF1_BW           Bandwidth of 1st IF SAW filter
**                  MT2131_ZIF_BW           zero-IF bandwidth
**                  MT2131_LO2_STEPSIZE     LO2 minimum step size
**                  MT2131_LO2_FRACN_AVOID  LO2 FracN keep-out region
**                  MT2131_OUTPUT_FREQ      output center frequency
**                  MT2131_OUTPUT_BW        output channel bandwidth
**                  MT2131_LO_SEPARATION    min inter-tuner LO separation
**                  MT2131_MAX_HARM1        max # of intra-tuner harmonics
**                  MT2131_MAX_HARM2        max # of inter-tuner harmonics
**
**  Usage:          status |= MT2131_SetParam(hMT2131,
**                                            MT2131_STEPSIZE,
**                                            50000);
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_INV_HANDLE     - Invalid tuner handle
**                      MT_ARG_NULL       - Null pointer argument passed
**                      MT_ARG_RANGE      - Invalid parameter requested
**                                          or set value out of range
**                                          or non-writable parameter
**
**  Dependencies:   USERS MUST CALL MT2131_Open() FIRST!
**
**  See Also:       MT2131_GetParam, MT2131_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**   N/A   09-29-2006    DAD    Ver 1.12: Added MT2131_VGA_GAIN_MODE and
**                                        MT2131_LNAGAIN_MODE.
**   N/A   11-29-2006    DAD    Ver 1.18: Consolidated all separate receiver
**                                        mode set functions.
**
****************************************************************************/
U32 MT2131_SetParam(TUNSHDRV_InstanceData_t *Instance,
                        MT2131_Param param,
                        U32      nValue)
{
    U32 status = MT_OK;                  /* Status to be returned        */

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
        status |= MT_INV_HANDLE;

    if (MT_NO_ERROR(status))
    {
        switch (param)
        {
        /*  Receiver Mode                         */
        case MT2131_RCVR_MODE:
            switch (nValue)
            {
            case MT2131_ANALOG_OFFAIR:
            case MT2131_ANALOG_CABLE:
            case MT2131_DIGITAL_OFFAIR:
            case MT2131_DIGITAL_CABLE:
            case MT2131_ANALOG_OFFAIR_2:
            case MT2131_ANALOG_CABLE_2:
            case MT2131_DIGITAL_OFFAIR_2:
            case MT2131_DIGITAL_CABLE_2:
                status |= MT2131_SetRcvrMode(Instance	, (MT2131_Rcvr_Mode) nValue);
                break;

            default:
                status |= MT_ARG_RANGE;
                break;
            }
            break;

        /*  R-Load maximum value                  */
        case MT2131_RLMAX:
            Instance->TunerRegVal[MT2131_AGC_RL] = ( Instance->TunerRegVal[MT2131_AGC_RL] & ~0x7F) | (U8) (nValue & 0x7F);
           /* status |= MT2131_SetReg(h, AGC_RL, pInfo->reg[AGC_RL]);*/
		   status |= UncheckedSet(Instance, MT2131_AGC_RL, Instance->TunerRegVal[MT2131_AGC_RL]);
            break;

        /*  Down Converter Maximum Value          */
        case MT2131_DNCMAX:
             Instance->TunerRegVal[MT2131_AGC_DNC] = (U8) nValue;
            /*status |= MT2131_SetReg(h, AGC_DNC, pInfo->reg[AGC_DNC]);*/
			 status |= UncheckedSet(Instance, MT2131_AGC_DNC, Instance->TunerRegVal[MT2131_AGC_DNC]);
            break;

        /*  AGC Bleed Current                     */
        case MT2131_AGCBLD:
             Instance->TunerRegVal[MT2131_PDET_2] = ( Instance->TunerRegVal[MT2131_PDET_2] & ~0x07) | (U8) (nValue & 0x07);
            /*status |= MT2131_SetReg(h, PDET_2, pInfo->reg[PDET_2]);*/
		 status |= UncheckedSet(Instance, MT2131_PDET_2, Instance->TunerRegVal[MT2131_PDET_2]);
            break;

        /*  Power Detector #1 Level               */
        case MT2131_PD1LEV:
             Instance->TunerRegVal[MT2131_PDET_1] = ( Instance->TunerRegVal[MT2131_PDET_1] & ~0xE0) | (U8) ((nValue & 0x07) << 5);
           /* status |= MT2131_SetReg(h, PDET_1, pInfo->reg[PDET_1]);*/
		    status |= UncheckedSet(Instance, MT2131_PDET_1, Instance->TunerRegVal[MT2131_PDET_1]);
            break;

        /*  Power Detector #2 Level               */
        case MT2131_PD2LEV:
             Instance->TunerRegVal[MT2131_PDET_2] = ( Instance->TunerRegVal[MT2131_PDET_2] & ~0xE0) | (U8) ((nValue & 0x07) << 5);
            /*status |= MT2131_SetReg(h, PDET_2, pInfo->reg[PDET_2]);*/
			  status |= UncheckedSet(Instance, MT2131_PDET_2, Instance->TunerRegVal[MT2131_PDET_2]);
            break;

        /*  LNA Gain Level                        */
        case MT2131_LNAGAIN:
             Instance->TunerRegVal[MT2131_UPC_1] = ( Instance->TunerRegVal[MT2131_UPC_1] & ~0x60) | (U8) ((nValue & 0x03) << 5);
            /*status |= MT2131_SetReg(h, UPC_1, pInfo->reg[UPC_1]);*/
			  status |= UncheckedSet(Instance, MT2131_UPC_1, Instance->TunerRegVal[MT2131_UPC_1]);
            break;

        /*  crystal frequency                     */
        case MT2131_SRO_FREQ:
            Instance->MT2131_Info.AS_Data.f_ref = nValue;
            Instance->MT2131_Info.AS_Data.f_LO1_FracN_Avoid = nValue / 32 - 1;
            Instance->MT2131_Info.AS_Data.f_LO2_FracN_Avoid = nValue / 80 - 1;
            Instance->MT2131_Info.AS_Data.f_LO1_Step = nValue / 64;
            break;

        /*  minimum tuning step size              */
        case MT2131_STEPSIZE:
            Instance->MT2131_Info.AS_Data.f_LO2_Step = nValue;
            break;

        /*  LO1 minimum step size                 */
        case MT2131_LO1_STEPSIZE:
            Instance->MT2131_Info.AS_Data.f_LO1_Step = nValue;
            break;

        /*  LO1 FracN keep-out region             */
        case MT2131_LO1_FRACN_AVOID:
            Instance->MT2131_Info.AS_Data.f_LO1_FracN_Avoid = nValue;
            break;

        /*  Requested 1st IF                      */
        case MT2131_IF1_REQUEST:
            Instance->MT2131_Info.AS_Data.f_if1_Request = nValue;
            break;

        /*  Center of 1st IF SAW filter           */
        case MT2131_IF1_CENTER:
            Instance->MT2131_Info.AS_Data.f_if1_Center = nValue;
            break;

        /*  Bandwidth of 1st IF SAW filter        */
        case MT2131_IF1_BW:
            Instance->MT2131_Info.AS_Data.f_if1_bw = nValue;
            break;

        /*  zero-IF bandwidth                     */
        case MT2131_ZIF_BW:
            Instance->MT2131_Info.AS_Data.f_zif_bw = nValue;
            break;

        /*  LO2 minimum step size                 */
        case MT2131_LO2_STEPSIZE:
            Instance->MT2131_Info.AS_Data.f_LO2_Step = nValue;
            break;

        /*  LO2 FracN keep-out region             */
        case MT2131_LO2_FRACN_AVOID:
            Instance->MT2131_Info.AS_Data.f_LO2_FracN_Avoid = nValue;
            break;

        /*  output center frequency               */
        case MT2131_OUTPUT_FREQ:
            Instance->MT2131_Info.AS_Data.f_out = nValue;

            if ((nValue < MIN_FOUT_FREQ) || (nValue > MAX_FOUT_FREQ))
                status |= MT_FOUT_RANGE;
            break;

        /*  output channel bandwidth              */
        case MT2131_OUTPUT_BW:
            Instance->MT2131_Info.AS_Data.f_out_bw = nValue + 750000;
            break;

        /*  min inter-tuner LO separation         */
        case MT2131_LO_SEPARATION:
            Instance->MT2131_Info.AS_Data.f_min_LO_Separation = nValue;
            break;

        /*  max # of intra-tuner harmonics        */
        case MT2131_MAX_HARM1:
            Instance->MT2131_Info.AS_Data.maxH1 = nValue;
            break;

        /*  max # of inter-tuner harmonics        */
        case MT2131_MAX_HARM2:
            Instance->MT2131_Info.AS_Data.maxH2 = nValue;
            break;

        /*  VGA Gain mode                         */
        case MT2131_VGA_GAIN_MODE:
            if (nValue > MT2131_VGA_AUTO)
                status |= MT_ARG_RANGE;
            else
            {
                Instance->MT2131_Info.VGA_gain_mode = nValue;
                if (Instance->MT2131_Info.VGA_gain_mode < MT2131_VGA_AUTO)
                {
                     Instance->TunerRegVal[MT2131_MISC_1] = ( Instance->TunerRegVal[MT2131_MISC_1] & ~0x40) | ((U8) nValue << 6);
                   /* status |= MT_WriteSub(pInfo->hUserData,
                                          pInfo->address,
                                          MISC_1,
                                          &pInfo->reg[MISC_1],
                                          1);*/
                    status |= UncheckedSet(Instance, MT2131_MISC_1, Instance->TunerRegVal[MT2131_MISC_1]);
                }
            }
            break;

        /*  LNAGain Mode                         */
        case MT2131_LNAGAIN_MODE:
            if (nValue > MT2131_LNA_AUTO)
                status |= MT_ARG_RANGE;
            else
            {
               Instance->MT2131_Info.LNAgain_mode = nValue;
                if (Instance->MT2131_Info.LNAgain_mode < MT2131_LNA_AUTO)
                    status |= MT2131_SetParam(Instance, MT2131_LNAGAIN, nValue);
            }
            break;

        /*  These parameters are read-only  */
        case MT2131_IC_ADDR:
        case MT2131_MAX_OPEN:
        case MT2131_NUM_OPEN:
        case MT2131_INPUT_FREQ:
        case MT2131_LO1_FREQ:
        case MT2131_IF1_ACTUAL:
        case MT2131_LO2_FREQ:
        case MT2131_AS_ALG:
        case MT2131_EXCL_ZONES:
        case MT2131_SPUR_AVOIDED:
        case MT2131_NUM_SPURS:
        case MT2131_SPUR_PRESENT:
        case MT2131_EOP:
        default:
            status |= MT_ARG_RANGE;
        }
    }
    return (status);
}



/******************************************************************************
**
**  Name: MT2131_WaitForAGC
**
**  Description:    Wait for the MT2131 AGC firmware to settle.
**                  After settling, set the AGC back to normal speed.
**
**  Parameters:     h            - Open handle to the tuner (from MT2131_Open).
**
**  Usage:          status = MT2131_WaitForAGC(hMT2131);
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_INV_HANDLE     - Invalid tuner handle
**                      MT_COMM_ERR       - Serial bus communications error
**                      MT_TUNER_TIMEOUT  - Timed out waiting for AGC complete
**
**  Dependencies:   MT_ReadSub - Read byte(s) of data from the two-wire bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   08-02-2006    DAD    nTries loop variable was not incremented.
**   N/A   08-16-2006    DAD    Don't wait for AGCdone if AGC is disabled.
**   N/A   08-21-2006    DAD    1.07: Separate the reset and wait operations
**                                    into two functions.
**   N/A   11-29-2006    DAD    Ver 1.18: Added code to set AGC speed back
**                                        to normal (if needed).
**
******************************************************************************/
U32 MT2131_WaitForAGC(TUNSHDRV_InstanceData_t *Instance)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    U32 nTries = 0;

    if (IsValidHandle(Instance) == 0)
        return MT_INV_HANDLE;

    if (((Instance->TunerRegVal[MT2131_PWR] & 0x80) != 0x00)            /*  RF AGC enabled     */
        && ((Instance->TunerRegVal[MT2131_AGC_RL] & 0x80) == 0x00))     /*  RF AGC not frozen  */
    {
        const U8 o_PDET1 = Instance->TunerRegVal[MT2131_PDET_1];
        const U8 o_PDET2 = Instance->TunerRegVal[MT2131_PDET_2];

        /*  PD1lev, AGCCrst, PD1hyst, AGCclk  */
        Instance->TunerRegVal[MT2131_PDET_1] = 0x10
                           | PD1LEV[Instance->MT2131_Info.rcvr_mode] << 5
                           | PD1HYST[Instance->MT2131_Info.rcvr_mode] << 3
                           | (Instance->TunerRegVal[MT2131_PART_REV] == MT2131_B3 ? AGCCLK3[Instance->MT2131_Info.rcvr_mode] : AGCCLK4[Instance->MT2131_Info.rcvr_mode]);
        /*  PD2lev, AGCtgt, PD2hyst, AGCbld  */
        Instance->TunerRegVal[MT2131_PDET_2] = 0x00
                           | PD2LEV[Instance->MT2131_Info.rcvr_mode] << 5
                           | PD2HYST[Instance->MT2131_Info.rcvr_mode] << 3
                           | (Instance->TunerRegVal[MT2131_PART_REV] == MT2131_B3 ? AGCBLD3[Instance->MT2131_Info.rcvr_mode] : AGCBLD4[Instance->MT2131_Info.rcvr_mode]);

        /*  Wait for the AGCdone bit  */
        Instance->TunerRegVal[MT2131_AFC] &= ~0x80;            /*  Reset the bit in the cache   */
        while (((Instance->TunerRegVal[MT2131_AFC] & 0x80) == 0x00) && MT_NO_ERROR(status) && (nTries++ < 100))
        {
           /* MT_Sleep(pInfo->hUserData, 10);*//*check this????????*/
           /* status |= MT_ReadSub(pInfo->hUserData, pInfo->address, AFC, &pInfo->reg[AFC], 1);*/
		   status |=UncheckedGet(Instance, MT2131_AFC, &Instance->TunerRegVal[MT2131_AFC]);
        }

        if (MT_NO_ERROR(status) && ((Instance->TunerRegVal[MT2131_AFC] & 0x80) == 0x00))
            status |= MT_TUNER_TIMEOUT;

        if (MT_NO_ERROR(status) && ((Instance->TunerRegVal[MT2131_PDET_1] != o_PDET1)
                                 || (Instance->TunerRegVal[MT2131_PDET_2] != o_PDET2)))
         /*   status |= MT_WriteSub(pInfo->hUserData, pInfo->address, PDET_1, &pInfo->reg[PDET_1], 2);*/
         
         
         #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
			 /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, AGC_RL, &pInfo->reg[AGC_RL], 4);*/
	    status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle, MT2131_PDET_1,&Instance->TunerRegVal[MT2131_PDET_1],2);

		   }
		    #endif
		   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
        {
		 /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, AGC_RL, &pInfo->reg[AGC_RL], 4);*/
	     status |= ChipSetRegisters (Instance->IOHandle, MT2131_PDET_1,&Instance->TunerRegVal[MT2131_PDET_1],2);
         }
         #endif
		  

    }

    return (status);
}


static U32 Round_fLO(U32 f_LO, U32 f_LO_Step, U32 f_ref)
{
    return f_ref * (f_LO / f_ref)
        + f_LO_Step * (((f_LO % f_ref) + (f_LO_Step / 2)) / f_LO_Step);
}


/****************************************************************************
**
**  Name: fLO_FractionalTerm
**
**  Description:    Calculates the portion contributed by FracN / denom.
**
**                  This function preserves maximum precision without
**                  risk of overflow.  It accurately calculates
**                  f_ref * num / denom to within 1 HZ with fixed math.
**
**  Parameters:     num       - Fractional portion of the multiplier
**                  denom     - denominator portion of the ratio
**                              This routine successfully handles denom values
**                              up to and including 2^18.
**                  f_Ref     - SRO frequency.  This calculation handles
**                              f_ref as two separate 14-bit fields.
**                              Therefore, a maximum value of 2^28-1
**                              may safely be used for f_ref.  This is
**                              the genesis of the magic number "14" and the
**                              magic mask value of 0x03FFF.
**
**  Returns:        f_ref * num / denom
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-15-2006    RSK    Ver 1.19: New fLO_FractionalTerm function.
**
****************************************************************************/
static U32 fLO_FractionalTerm( U32 f_ref,
                                   U32 num,
                                   U32 denom )
{
    U32 t1     = (f_ref >> 14) * num;
    U32 term1  = t1 / denom;
    U32 loss   = t1 % denom;
    U32 term2  = ( ((f_ref & 0x00003FFF) * num + (loss<<14)) + (denom/2) )  / denom;
    return ((term1 << 14) + term2);
}


U32 MT_ChooseFirstIF(MT_AvoidSpursData_t* pAS_Info)
{
    /* Update "f_Desired" to be the nearest "combinational-multiple" of "f_LO1_Step".
	 The resulting number, F_LO1 must be a multiple of f_LO1_Step.  And F_LO1 is the arithmetic sum
	 of f_in + f_Center.  Neither f_in, nor f_Center must be a multiple of f_LO1_Step.
	 However, the sum must be.*/
    const U32 f_Desired = pAS_Info->f_LO1_Step * ((pAS_Info->f_if1_Request + pAS_Info->f_in + pAS_Info->f_LO1_Step/2) / pAS_Info->f_LO1_Step) - pAS_Info->f_in;
    const U32 f_Step = (pAS_Info->f_LO1_Step > pAS_Info->f_LO2_Step) ? pAS_Info->f_LO1_Step : pAS_Info->f_LO2_Step;
    U32 f_Center;

    S32 i;
    S32 j = 0;
    U32 bDesiredExcluded = 0;
    U32 bZeroExcluded = 0;
    S32 tmpMin, tmpMax;
    S32 bestDiff;
    struct MT_ExclZone_t* pNode = pAS_Info->usedZones;
    struct MT_FIFZone_t zones[MAX_ZONES];

    if (pAS_Info->nZones == 0)
        return f_Desired;

    /*  f_Center needs to be an integer multiple of f_Step away from f_Desired*/
    if ((pAS_Info->f_if1_Center + f_Step/2) > f_Desired)
        f_Center = f_Desired + f_Step * (((pAS_Info->f_if1_Center + f_Step/2) - f_Desired) / f_Step);
    else
        f_Center = f_Desired - f_Step * ((f_Desired - (pAS_Info->f_if1_Center - f_Step/2)) / f_Step);

    assert(abs((S32) f_Center - (S32) pAS_Info->f_if1_Center) <= (S32) (f_Step/2));

    /*  Take MT_ExclZones, center around f_Center and change the resolution to f_Step*/
    while (pNode != NULL)
    {
        /*  floor function*/
        tmpMin = floor( ((S32)pNode->min_ - (S32)f_Center), (S32) f_Step);

        /*  ceil function*/
        tmpMax = ceil((S32) (pNode->max_ - f_Center), (S32) f_Step);

        if ((pNode->min_ < f_Desired) && (pNode->max_ > f_Desired))
            bDesiredExcluded = 1;

        if ((tmpMin < 0) && (tmpMax > 0))
            bZeroExcluded = 1;

        /*  See if this zone overlaps the previous*/
        if ((j>0) && (tmpMin < zones[j-1].max_))
            zones[j-1].max_ = tmpMax;
        else
        {
            /*  Add new zone*/
            assert(j<MAX_ZONES);
            zones[j].min_ = tmpMin;
            zones[j].max_ = tmpMax;
            j++;
        }
        pNode = pNode->next_;
    }

    /*
    **  If the desired is okay, return with it
    */
    if (bDesiredExcluded == 0)
        return f_Desired;

    /*
    **  If the desired is excluded and the center is okay, return with it
    */
    if (bZeroExcluded == 0)
        return f_Center;

    /*  Find the value closest to 0 (f_Center)*/
    bestDiff = zones[0].min_;
    for (i=0; i<j; i++)
    {
        if (abs(zones[i].min_) < abs(bestDiff)) bestDiff = zones[i].min_;
        if (abs(zones[i].max_) < abs(bestDiff)) bestDiff = zones[i].max_;
    }


    if (bestDiff < 0)
        return f_Center - ((U32) (-bestDiff) * f_Step);

    return f_Center + (bestDiff * f_Step);
}

static U32 umax(U32 a, U32 b)
{
    return (a >= b) ? a : b;
}


/****************************************************************************
**
**  Name: gcd
**
**  Description:    Uses Euclid's algorithm
**
**  Parameters:     u, v     - unsigned values whose GCD is desired.
**
**  Global:         None
**
**  Returns:        greatest common divisor of u and v, if either value
**                  is 0, the other value is returned as the result.
**
**  Dependencies:   None.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-01-2004    JWS    Original
**   N/A   08-03-2004    DAD    Changed to Euclid's since it can handle
**                              unsigned numbers.
**
****************************************************************************/
static U32 gcd(U32 u, U32 v)
{
    U32 r;

    while (v != 0)
    {
        r = u % v;
        u = v;
        v = r;
    }

    return u;
}


#if MT_TUNER_CNT > 1
static S32 RoundAwayFromZero(S32 n, S32 d)
{
    return (n<0) ? floor(n, d) : ceil(n, d);
}


/****************************************************************************
**
**  Name: IsSpurInAdjTunerBand
**
**  Description:    Checks to see if a spur will be present within the IF's
**                  bandwidth or near the zero IF.
**                  (fIFOut +/- fIFBW/2, -fIFOut +/- fIFBW/2)
**                                  and
**                  (0 +/- fZIFBW/2)
**
**                    ma   mb               me   mf               mc   md
**                  <--+-+-+-----------------+-+-+-----------------+-+-+-->
**                     |   ^                   0                   ^   |
**                     ^   b=-fIFOut+fIFBW/2      -b=+fIFOut-fIFBW/2   ^
**                     a=-fIFOut-fIFBW/2              -a=+fIFOut+fIFBW/2
**
**                  Note that some equations are doubled to prevent round-off
**                  problems when calculating fIFBW/2
**
**                  The spur frequencies are computed as:
**
**                     fSpur = n * f1 - m * f2 - fOffset
**
**  Parameters:     f1      - The 1st local oscillator (LO) frequency
**                            of the tuner whose output we are examining
**                  f2      - The 1st local oscillator (LO) frequency
**                            of the adjacent tuner
**                  fOffset - The 2nd local oscillator of the tuner whose
**                            output we are examining
**                  fIFOut  - Output IF center frequency
**                  fIFBW   - Output IF Bandwidth
**                  nMaxH   - max # of LO harmonics to search
**                  n       - If spur, # of harmonics of f1 (returned)
**                  m       - If spur, # of harmonics of f2 (returned)
**
**  Returns:        1 if an LO spur would be present, otherwise 0.
**
**  Dependencies:   None.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   01-21-2005    JWS    Original, adapted from MT_DoubleConversion.
**
****************************************************************************/
static U32 IsSpurInAdjTunerBand(U32 bIsMyOutput,
                                    U32 f1,
                                    U32 f2,
                                    U32 fOffset,
                                    U32 fIFOut,
                                    U32 fIFBW,
                                    U32 fZIFBW,
                                    U32 nMaxH,
                                    U32 *fp,
                                    U32 *fm)
{
    U32 bSpurFound = 0;

    const U32 fHalf_IFBW = fIFBW / 2;
    const U32 fHalf_ZIFBW = fZIFBW / 2;

    /* Calculate a scale factor for all frequencies, so that our
       calculations all stay within 31 bits */
    const S32 f_Scale = ((f1 + (fOffset + fIFOut + fHalf_IFBW) / nMaxH) / ((MAX_UDATA/2) / nMaxH)) + 1;

    const S32 _f1 = (S32) f1 / f_Scale;
    const S32 _f2 = (S32) f2 / f_Scale;
    const S32 _f3 = (S32) fOffset / f_Scale;

    const S32 c = (S32) (fIFOut - fHalf_IFBW) / f_Scale;
    const S32 d = (S32) (fIFOut + fHalf_IFBW) / f_Scale;
    const S32 f = (S32) fHalf_ZIFBW / f_Scale;

    S32  ma, mb, mc, md, me, mf;

    S32  fp_ = 0;
    S32  fm_ = 0;
    S32  n;


    /*
    **  If the other tuner does not have an LO frequency defined,
    **  assume that we cannot interfere with it
    */
    if (f2 == 0)
        return 0;


    /* Check out all multiples of f1 from -nMaxH to +nMaxH */
    for (n = -(S32)nMaxH; n <= (S32)nMaxH; ++n)
    {
        md = (_f3 + d - n*_f1) / _f2;

        /* If # f2 harmonics > nMaxH, then no spurs present */
        if (md <= -(S32) nMaxH )
            break;

        ma = (_f3 - d - n*_f1) / _f2;
        if ((ma == md) || (ma >= (S32) (nMaxH)))
            continue;

        mc = (_f3 + c - n*_f1) / _f2;
        if (mc != md)
        {
            const S32 m = (n<0) ? md : mc;
            const S32 fspur = (n*_f1 + m*_f2 - _f3);
            bSpurFound = 1;
            fp_ = RoundAwayFromZero((d - fspur)* f_Scale, (bIsMyOutput ? n - 1 : n));
            fm_ = RoundAwayFromZero((fspur - c)* f_Scale, (bIsMyOutput ? n - 1 : n));
            break;
        }

        /* Location of Zero-IF-spur to be checked */
        mf = (_f3 + f - n*_f1) / _f2;
        me = (_f3 - f - n*_f1) / _f2;
        if (me != mf)
        {
            const S32 m = (n<0) ? mf : me;
            const S32 fspur = (n*_f1 + m*_f2 - _f3);
            bSpurFound = 1;
            fp_ = (S32) RoundAwayFromZero((f - fspur)* f_Scale, (bIsMyOutput ? n - 1 : n));
            fm_ = (S32) RoundAwayFromZero((fspur + f)* f_Scale, (bIsMyOutput ? n - 1 : n));
            break;
        }

        mb = (_f3 - c - n*_f1) / _f2;
        if (ma != mb)
        {
            const S32 m = (n<0) ? mb : ma;
            const S32 fspur = (n*_f1 + m*_f2 - _f3);
            bSpurFound = 1;
            fp_ = (S32) RoundAwayFromZero((-c - fspur)* f_Scale, (bIsMyOutput ? n - 1 : n));
            fm_ = (S32) RoundAwayFromZero((fspur +d)* f_Scale, (bIsMyOutput ? n - 1 : n));
            break;
        }
    }

    //  Verify that fm & fp are both positive
    //  Add one to ensure next 1st IF choice is not right on the edge
    if (fp_ < 0)
    {
        *fp = -fm_ + 1;
        *fm = -fp_ + 1;
    }
    else if (fp_ > 0)
    {
        *fp = fp_ + 1;
        *fm = fm_ + 1;
    }
    else
    {
        *fp = 1;
        *fm = abs(fm_) + 1;
    }

    return bSpurFound;
}
#endif



/****************************************************************************
**
**  Name: IsSpurInBand
**
**  Description:    Checks to see if a spur will be present within the IF's
**                  bandwidth. (fIFOut +/- fIFBW, -fIFOut +/- fIFBW)
**
**                    ma   mb                                     mc   md
**                  <--+-+-+-------------------+-------------------+-+-+-->
**                     |   ^                   0                   ^   |
**                     ^   b=-fIFOut+fIFBW/2      -b=+fIFOut-fIFBW/2   ^
**                     a=-fIFOut-fIFBW/2              -a=+fIFOut+fIFBW/2
**
**                  Note that some equations are doubled to prevent round-off
**                  problems when calculating fIFBW/2
**
**  Parameters:     pAS_Info - Avoid Spurs information block
**                  fm       - If spur, amount f_IF1 has to move negative
**                  fp       - If spur, amount f_IF1 has to move positive
**
**  Global:         None
**
**  Returns:        1 if an LO spur would be present, otherwise 0.
**
**  Dependencies:   None.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   11-28-2002    DAD    Implemented algorithm from applied patent
**
****************************************************************************/
static U32 IsSpurInBand(MT_AvoidSpursData_t* pAS_Info,
                            U32* fm,
                            U32* fp)
{
    /*
    **  Calculate LO frequency settings.
    */
    U32 n, n0;
    const U32 f_LO1 = pAS_Info->f_LO1;
    const U32 f_LO2 = pAS_Info->f_LO2;
    const U32 d = pAS_Info->f_out + pAS_Info->f_out_bw/2;
    const U32 c = d - pAS_Info->f_out_bw;
    const U32 f = pAS_Info->f_zif_bw/2;
    
    const U32 f_Scale = (f_LO1 / ((U32)((MAX_UDATA/2)) / pAS_Info->maxH1)) + 1;
    S32 f_nsLO1, f_nsLO2;
    S32 f_Spur;
    U32 ma, mb, mc, md, me, mf;
    U32 lo_gcd, gd_Scale, gc_Scale, gf_Scale, hgds, hgfs, hgcs;
#if MT_TUNER_CNT > 1
    U32 index;

    MT_AvoidSpursData_t *adj;
#endif
    *fm = 0;
     
    /*
    ** For each edge (d, c & f), calculate a scale, based on the gcd
    ** of f_LO1, f_LO2 and the edge value.  Use the larger of this
    ** gcd-based scale factor or f_Scale.
    */
    lo_gcd = gcd(f_LO1, f_LO2);
    gd_Scale = umax((U32) gcd(lo_gcd, d), f_Scale);
	hgds = gd_Scale/2;
    gc_Scale = umax((U32) gcd(lo_gcd, c), f_Scale);
	hgcs = gc_Scale/2;
    gf_Scale = umax((U32) gcd(lo_gcd, f), f_Scale);
	hgfs = gf_Scale/2;

    n0 = uceil(f_LO2 - d, f_LO1 - f_LO2);

    /*  Check out all multiples of LO1 from n0 to m_maxLOSpurHarmonic*/
    for (n=n0; n<=pAS_Info->maxH1; ++n)
    {
        md = (n*((f_LO1+hgds)/gd_Scale) - ((d+hgds)/gd_Scale)) / ((f_LO2+hgds)/gd_Scale);

        /*  If # fLO2 harmonics > m_maxLOSpurHarmonic, then no spurs present*/
        if (md >= pAS_Info->maxH1)
            break;

        ma = (n*((f_LO1+hgds)/gd_Scale) + ((d+hgds)/gd_Scale)) / ((f_LO2+hgds)/gd_Scale);

        /*  If no spurs between +/- (f_out + f_IFBW/2), then try next harmonic*/
        if (md == ma)
            continue;

        mc = (n*((f_LO1+hgcs)/gc_Scale) - ((c+hgcs)/gc_Scale)) / ((f_LO2+hgcs)/gc_Scale);
        if (mc != md)
        {
            f_nsLO1 = (S32) (n*(f_LO1/gc_Scale));
            f_nsLO2 = (S32) (mc*(f_LO2/gc_Scale));
            f_Spur = (gc_Scale * (f_nsLO1 - f_nsLO2)) + n*(f_LO1 % gc_Scale) - mc*(f_LO2 % gc_Scale);

            *fp = ((f_Spur - (S32) c) / (mc - n)) + 1;
            *fm = (((S32) d - f_Spur) / (mc - n)) + 1;
            return 1;
        }

        /*  Location of Zero-IF-spur to be checked*/
        me = (n*((f_LO1+hgfs)/gf_Scale) + ((f+hgfs)/gf_Scale)) / ((f_LO2+hgfs)/gf_Scale);
        mf = (n*((f_LO1+hgfs)/gf_Scale) - ((f+hgfs)/gf_Scale)) / ((f_LO2+hgfs)/gf_Scale);
        if (me != mf)
        {
            f_nsLO1 = n*(f_LO1/gf_Scale);
            f_nsLO2 = me*(f_LO2/gf_Scale);
            f_Spur = (gf_Scale * (f_nsLO1 - f_nsLO2)) + n*(f_LO1 % gf_Scale) - me*(f_LO2 % gf_Scale);

            *fp = ((f_Spur + (S32) f) / (me - n)) + 1;
            *fm = (((S32) f - f_Spur) / (me - n)) + 1;
            return 1;
        }

        mb = (n*((f_LO1+hgcs)/gc_Scale) + ((c+hgcs)/gc_Scale)) / ((f_LO2+hgcs)/gc_Scale);
        if (ma != mb)
        {
            f_nsLO1 = n*(f_LO1/gc_Scale);
            f_nsLO2 = ma*(f_LO2/gc_Scale);
            f_Spur = (gc_Scale * (f_nsLO1 - f_nsLO2)) + n*(f_LO1 % gc_Scale) - ma*(f_LO2 % gc_Scale);

            *fp = (((S32) d + f_Spur) / (ma - n)) + 1;
            *fm = (-(f_Spur + (S32) c) / (ma - n)) + 1;
            return 1;
        }
    }

#if MT_TUNER_CNT > 1
    /*  If no spur found, see if there are more tuners on the same board*/
    for (index = 0; index < TunerCount; ++index)
    {
        adj = TunerList[index];
        if (pAS_Info == adj)    /* skip over our own data, don't process it */
            continue;

        /*  Look for LO-related spurs from the adjacent tuner generated into my IF output*/
        if (IsSpurInAdjTunerBand(1,                   /*  check my IF output*/
                                 pAS_Info->f_LO1,     /*  my fLO1*/
                                 adj->f_LO1,          /*  the other tuner's fLO1*/
                                 pAS_Info->f_LO2,     /*  my fLO2*/
                                 pAS_Info->f_out,     /*  my fOut*/
                                 pAS_Info->f_out_bw,  /*  my output IF bandiwdth*/
                                 pAS_Info->f_zif_bw,  /*  my Zero-IF bandwidth*/
                                 pAS_Info->maxH2,
                                 fp,                  /*  minimum amount to move LO's positive*/
                                 fm))                 /*  miminum amount to move LO's negative*/
            return 1;
        /*  Look for LO-related spurs from my tuner generated into the adjacent tuner's IF output*/
        if (IsSpurInAdjTunerBand(0,             /*  check his IF output*/
                                 pAS_Info->f_LO1,     /*  my fLO1*/
                                 adj->f_LO1,          /*  the other tuner's fLO1*/
                                 adj->f_LO2,          /*  the other tuner's fLO2*/
                                 adj->f_out,          /*  the other tuner's fOut*/
                                 adj->f_out_bw,       /*  the other tuner's output IF bandiwdth*/
                                 pAS_Info->f_zif_bw,  /*  the other tuner's Zero-IF bandwidth*/
                                 adj->maxH2,
                                 fp,                  /*  minimum amount to move LO's positive*/
                                 fm))                 /*  miminum amount to move LO's negative*/
            return 1;
    }
#endif
    /* No spurs found*/
    return 0;
}



/*****************************************************************************
**
**  Name: MT_AvoidSpurs
**
**  Description:    Main entry point to avoid spurs.
**                  Checks for existing spurs in present LO1, LO2 freqs
**                  and if present, chooses spur-free LO1, LO2 combination
**                  that tunes the same input/output frequencies.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   096   04-06-2005    DAD    Ver 1.11: Fix divide by 0 error if maxH==0.
**
*****************************************************************************/
U32 MT_AvoidSpurs(MT_AvoidSpursData_t* pAS_Info)
{
    U32 status = MT_OK;
    U32 fm, fp;                     /*  restricted range on LO's        */
    pAS_Info->bSpurAvoided = 0;
    pAS_Info->nSpursFound = 0;                     /*  warning expected: code has no effect    */

    if (pAS_Info->maxH1 == 0)
        return MT_OK;

    /*
    **  Avoid LO Generated Spurs
    **
    **  Make sure that have no LO-related spurs within the IF output
    **  bandwidth.
    **
    **  If there is an LO spur in this band, start at the current IF1 frequency
    **  and work out until we find a spur-free frequency or run up against the
    **  1st IF SAW band edge.  Use temporary copies of fLO1 and fLO2 so that they
    **  will be unchanged if a spur-free setting is not found.
    */
    pAS_Info->bSpurPresent = IsSpurInBand(pAS_Info, &fm, &fp);
    if (pAS_Info->bSpurPresent)
    {
        U32 zfIF1 = pAS_Info->f_LO1 - pAS_Info->f_in; /*  current attempt at a 1st IF*/
        U32  zfLO1 = pAS_Info->f_LO1;         /*  current attempt at an LO1 freq*/
        U32  zfLO2 = pAS_Info->f_LO2;         /*  current attempt at an LO2 freq*/
        U32  delta_IF1;
        U32  new_IF1;


        do

        {
            pAS_Info->nSpursFound++;

            /*  Raise f_IF1_upper, if needed*/
            MT_AddExclZone(pAS_Info, zfIF1 - fm, zfIF1 + fp);

            /*  Choose next IF1 that is closest to f_IF1_CENTER              */
            new_IF1 = MT_ChooseFirstIF(pAS_Info);

            if (new_IF1 > zfIF1)
            {
                pAS_Info->f_LO1 += (new_IF1 - zfIF1);
                pAS_Info->f_LO2 += (new_IF1 - zfIF1);
            }
            else
            {
                pAS_Info->f_LO1 -= (zfIF1 - new_IF1);
                pAS_Info->f_LO2 -= (zfIF1 - new_IF1);
            }
            zfIF1 = new_IF1;

            if (zfIF1 > pAS_Info->f_if1_Center)
                delta_IF1 = zfIF1 - pAS_Info->f_if1_Center;
            else
                delta_IF1 = pAS_Info->f_if1_Center - zfIF1;
        }
        /*  Continue while the new 1st IF is still within the 1st IF bandwidth
          and there is a spur in the band (again)*/
        while ((2*delta_IF1 + pAS_Info->f_out_bw <= pAS_Info->f_if1_bw) &&
              (pAS_Info->bSpurPresent == IsSpurInBand(pAS_Info, &fm, &fp)));

        /*
        ** Use the LO-spur free values found.  If the search went all the way to
        ** the 1st IF band edge and always found spurs, just leave the original
        ** choice.  It's as "good" as any other.
        */
        if (pAS_Info->bSpurPresent == 1)
        {
            status |= MT_SPUR_PRESENT;
            pAS_Info->f_LO1 = zfLO1;
            pAS_Info->f_LO2 = zfLO2;
        }
        else
            pAS_Info->bSpurAvoided = 1;
    }

    status |= ((pAS_Info->nSpursFound << MT_SPUR_SHIFT) & MT_SPUR_CNT_MASK);

    return (status);
}



void MT_ResetExclZones(MT_AvoidSpursData_t* pAS_Info)
{
    U32 center;
#if MT_TUNER_CNT > 1
    U32 index;
    MT_AvoidSpursData_t* adj;
#endif

    pAS_Info->nZones = 0;                           /*  this clears the used list*/
    pAS_Info->usedZones = NULL;                     /*  reset ptr*/
    pAS_Info->freeZones = NULL;                     /*  reset ptr*/

    center = pAS_Info->f_ref * ((pAS_Info->f_if1_Center - pAS_Info->f_if1_bw/2 + pAS_Info->f_in) / pAS_Info->f_ref) - pAS_Info->f_in;
    while (center < pAS_Info->f_if1_Center + pAS_Info->f_if1_bw/2 + pAS_Info->f_LO1_FracN_Avoid)
    {
        /*  Exclude LO1 FracN*/
        MT_AddExclZone(pAS_Info, center-pAS_Info->f_LO1_FracN_Avoid, center-1);
        MT_AddExclZone(pAS_Info, center+1, center+pAS_Info->f_LO1_FracN_Avoid);
        center += pAS_Info->f_ref;
    }

    center = pAS_Info->f_ref * ((pAS_Info->f_if1_Center - pAS_Info->f_if1_bw/2 - pAS_Info->f_out) / pAS_Info->f_ref) + pAS_Info->f_out;
    while (center < pAS_Info->f_if1_Center + pAS_Info->f_if1_bw/2 + pAS_Info->f_LO2_FracN_Avoid)
    {
        /*  Exclude LO2 FracN*/
        MT_AddExclZone(pAS_Info, center-pAS_Info->f_LO2_FracN_Avoid, center-1);
        MT_AddExclZone(pAS_Info, center+1, center+pAS_Info->f_LO2_FracN_Avoid);
        center += pAS_Info->f_ref;
    }

#if MT_TUNER_CNT > 1
	/*
    ** Iterate through all adjacent tuners and exclude frequencies related to them
    */
    for (index = 0; index < TunerCount; ++index)
    {
        adj = TunerList[index];
        if (pAS_Info == adj)    /* skip over our own data, don't process it */
            continue;

        /*
        **  Add 1st IF exclusion zone covering adjacent tuner's LO2
        **  at "adjfLO2 + f_out" +/- m_MinLOSpacing
        */
        if (adj->f_LO2 != 0)
            MT_AddExclZone(pAS_Info,
                           adj->f_LO2 + pAS_Info->f_out - pAS_Info->f_min_LO_Separation,
                           adj->f_LO2 + pAS_Info->f_out + pAS_Info->f_min_LO_Separation );

        /*
        **  Add 1st IF exclusion zone covering adjacent tuner's LO1
        **  at "adjfLO1 - f_in" +/- m_MinLOSpacing
        */
        if (adj->f_LO1 != 0)
            MT_AddExclZone(pAS_Info,
                           adj->f_LO1 - pAS_Info->f_in - pAS_Info->f_min_LO_Separation,
                           adj->f_LO1 - pAS_Info->f_in + pAS_Info->f_min_LO_Separation );
    }
#endif
}



static struct MT_ExclZone_t* InsertNode(MT_AvoidSpursData_t* pAS_Info,
                                  struct MT_ExclZone_t* pPrevNode)
{
    struct MT_ExclZone_t* pNode;
    /*  Check for a node in the free list  */
    if (pAS_Info->freeZones != NULL)
    {
        /*  Use one from the free list  */
        pNode = pAS_Info->freeZones;
        pAS_Info->freeZones = pNode->next_;
    }
    else
    {
        /*  Grab a node from the array  */
        pNode = &pAS_Info->MT_ExclZones[pAS_Info->nZones];
    }

    if (pPrevNode != NULL)
    {
        pNode->next_ = pPrevNode->next_;
        pPrevNode->next_ = pNode;
    }
    else    /*  insert at the beginning of the list  */
    {
        pNode->next_ = pAS_Info->usedZones;
        pAS_Info->usedZones = pNode;
    }

    pAS_Info->nZones++;
    return pNode;
}


static struct MT_ExclZone_t* RemoveNode(MT_AvoidSpursData_t* pAS_Info,
                                  struct MT_ExclZone_t* pPrevNode,
                                  struct MT_ExclZone_t* pNodeToRemove)
{
    struct MT_ExclZone_t* pNext = pNodeToRemove->next_;

    /*  Make previous node point to the subsequent node  */
    if (pPrevNode != NULL)
        pPrevNode->next_ = pNext;

    /*  Add pNodeToRemove to the beginning of the freeZones  */
    pNodeToRemove->next_ = pAS_Info->freeZones;
    pAS_Info->freeZones = pNodeToRemove;

    /*  Decrement node count  */
    pAS_Info->nZones--;

    return pNext;
}



/*****************************************************************************
**
**  Name: MT_AddExclZone
**
**  Description:    Add (and merge) an exclusion zone into the list.
**                  If the range (f_min, f_max) is totally outside the
**                  1st IF BW, ignore the entry.
**                  If the range (f_min, f_max) is negative, ignore the entry.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   103   01-31-2005    DAD    Ver 1.14: In MT_AddExclZone(), if the range
**                              (f_min, f_max) < 0, ignore the entry.
**
*****************************************************************************/
void MT_AddExclZone(MT_AvoidSpursData_t* pAS_Info,
                    U32 f_min,
                    U32 f_max)
{
    /*  Check to see if this overlaps the 1st IF filter  */
    if ((f_max > (pAS_Info->f_if1_Center - (pAS_Info->f_if1_bw / 2)))
        && (f_min < (pAS_Info->f_if1_Center + (pAS_Info->f_if1_bw / 2)))
        && (f_min < f_max))
    {
        /*
        **                1           2          3        4         5          6
        **
        **   New entry:  |---|    |--|        |--|       |-|      |---|         |--|
        **                     or          or        or       or        or
        **   Existing:  |--|          |--|      |--|    |---|      |-|      |--|
        */
        struct MT_ExclZone_t* pNode = pAS_Info->usedZones;
        struct MT_ExclZone_t* pPrev = NULL;
        struct MT_ExclZone_t* pNext = NULL;

        /*  Check for our place in the list  */
        while ((pNode != NULL) && (pNode->max_ < f_min))
        {
            pPrev = pNode;
            pNode = pNode->next_;
        }

        if ((pNode != NULL) && (pNode->min_ < f_max))
        {
            /*  Combine me with pNode  */
            if (f_min < pNode->min_)
                pNode->min_ = f_min;
            if (f_max > pNode->max_)
                pNode->max_ = f_max;
        }
        else
        {
            pNode = InsertNode(pAS_Info, pPrev);
            pNode->min_ = f_min;
            pNode->max_ = f_max;
        }

        /*  Look for merging possibilities  */
        pNext = pNode->next_;
        while ((pNext != NULL) && (pNext->min_ < pNode->max_))
        {
            if (pNext->max_ > pNode->max_)
                pNode->max_ = pNext->max_;
            pNext = RemoveNode(pAS_Info, pNode, pNext);  /*  Remove pNext, return ptr to pNext->next  */
        }
    }
}




/****************************************************************************
**
**  Name: CalcLOMult
**
**  Description:    Calculates Integer divider value and the numerator
**                  value for a FracN PLL.
**
**                  This function assumes that the f_LO and f_Ref are
**                  evenly divisible by f_LO_Step.
**
**  Parameters:     Div       - OUTPUT: Whole number portion of the multiplier
**                  FracN     - OUTPUT: Fractional portion of the multiplier
**                  f_LO      - desired LO frequency.
**                  f_LO_Step - Minimum step size for the LO (in Hz).
**                  f_Ref     - SRO frequency.
**                  f_Avoid   - Range of PLL frequencies to avoid near
**                              integer multiples of f_Ref (in Hz).
**
**  Returns:        Recalculated LO frequency.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**   N/A   12-15-2006    RSK    Ver 1.19: Using fLO_FractionalTerm calc.
**
****************************************************************************/
static U32 CalcLOMult(U32 *Div,
                          U32 *FracN,
                          U32  f_LO,
                          U32  f_LO_Step,
                          U32  f_Ref)
{
    /*  Calculate the whole number portion of the divider */
    *Div = f_LO / f_Ref;

    /*  Calculate the numerator value (round to nearest f_LO_Step) */
    *FracN = (8191 * (((f_LO % f_Ref) + (f_LO_Step / 2)) / f_LO_Step) + (f_Ref / f_LO_Step / 2)) / (f_Ref / f_LO_Step);

    return ((f_Ref * (*Div)) + fLO_FractionalTerm( f_Ref, *FracN, 8191 ));
}


/****************************************************************************
**
**  Name: ChangeFreq
**
**  Description:    Change the tuner's tuned frequency to RFin.
**
**  Parameters:     h           - Tuner handle (returned by MT2131_Open)
**                  f_in        - RF input center frequency (in Hz).
**                  f_outbw     - Output channel bandwidth (in Hz)
**
**  Usage:          status = MT2131_ChangeFreq(hMT2131,
**                                             f_in,
**                                             6000000UL);
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_INV_HANDLE     - Invalid tuner handle
**                      MT_UPC_UNLOCK     - Upconverter PLL unlocked
**                      MT_DNC_UNLOCK     - Downconverter PLL unlocked
**                      MT_COMM_ERR       - Serial bus communications error
**                      MT_SPUR_CNT_MASK  - Count of avoided LO spurs
**                      MT_SPUR_PRESENT   - LO spur possible in output
**                      MT_FIN_RANGE      - Input freq out of range
**                      MT_FOUT_RANGE     - Output freq out of range
**                      MT_UPC_RANGE      - Upconverter freq out of range
**                      MT_DNC_RANGE      - Downconverter freq out of range
**
**  Dependencies:   MUST CALL MT2131_Open BEFORE MT2131_ChangeFreq!
**
**                  MT_ReadSub   - Read data from the two-wire serial bus
**                  MT_WriteSub  - Write data to the two-wire serial bus
**                  MT_Sleep         - Delay execution for x milliseconds
**                  MT2131_GetLocked - Checks to see if LO1 and LO2 are locked
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**   N/A   05-12-2006    DAD    Ver 1.04: For Cable analog & digital, set
**                              PD2lev to 3 when f_in is 79 or 85 MHz.
**                              Set PD2lev to 2 for all others.
**   N/A   08-21-2006    DAD    Ver 1.07: Moved AGC optimization to a
**                              separate function to be called by the user
**   N/A   11-27-2006    DAD    Ver 1.17: Changed f_outbw parameter to no
**                              longer include the guard band.  Now f_outbw
**                              is equivalent to the actual channel bandwidth
**                              (i.e. 6, 7, or 8 MHz).
**   N/A   11-29-2006    DAD    Ver 1.18: Use fast AGC reset.
**
****************************************************************************/
U32 MT2131_ChangeFreq(TUNSHDRV_InstanceData_t *Instance,
                          U32 f_in,     /* RF input center frequency   */
                          U32 f_outbw)  /* output channel bandwidth    */
{


    U32 status = MT_OK;       /*  status of operation             */
    U32 LO1;                  /*  1st LO register value           */
    U32 Num1;                 /*  Numerator for LO1 reg. value    */
    U32 f_IF1;                /*  1st IF requested                */
    U32 LO2;                  /*  2nd LO register value           */
    U32 Num2;                 /*  Numerator for LO2 reg. value    */
    U32 ofLO1, ofLO2;         /*  last time's LO frequencies      */
    U32 ofin, ofout;          /*  last time's I/O frequencies     */
    U32 RFBand;               /*  RF Band setting                 */
    U32 idx;                  /*  index loop                      */
    U32 center;               /*  calculated center frequency     */
    U8  o_UPC1;               /*  original UPC_1 register value   */

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
        return MT_INV_HANDLE;

    /*
    **  Save original LO1 and LO2 frequency and register values
    */
    ofLO1 = Instance->MT2131_Info.AS_Data.f_LO1;
    ofLO2 = Instance->MT2131_Info.AS_Data.f_LO2;
    ofin = Instance->MT2131_Info.AS_Data.f_in;
    ofout = Instance->MT2131_Info.AS_Data.f_out;
    o_UPC1 = Instance->TunerRegVal[MT2131_UPC_1];

    /*  Check the input and output frequency ranges                   */
    if ((f_in < MIN_FIN_FREQ) || (f_in > MAX_FIN_FREQ))
        status |= MT_FIN_RANGE;

    /*
    **  Find RF Band setting
    */
    RFBand = 19;                        /*  def when f_in > all    */
    for (idx=0; idx<19; ++idx)
    {
        if (Instance->MT2131_Info.RF_Bands[idx] >= f_in)
        {
            RFBand = idx;
            break;
        }
    }

    /*
    **  Assign in the requested values
    */
    Instance->MT2131_Info.AS_Data.f_in = f_in;
    Instance->MT2131_Info.AS_Data.f_out_bw = f_outbw + 750000;

    /*  Request a 1st IF such that LO1 is on a step size  */
    f_IF1 = Round_fLO(Instance->MT2131_Info.AS_Data.f_if1_Request + f_in, Instance->MT2131_Info.AS_Data.f_LO1_Step, Instance->MT2131_Info.AS_Data.f_ref) - f_in;

    /*
    **  Calculate frequency settings.  f_IF1_FREQ + f_in is the
    **  desired LO1 frequency
    */
    MT_ResetExclZones(&Instance->MT2131_Info.AS_Data);

    /*
    **  When f_in <= 25th harmonic of SRO, and a harmonic of the SRO (f_ref)
    **  is in the input channel, we need to make sure that neither the LO1 or
    **  LO2 FracN numerator is 0.
    */
    if ((2*f_in <= (51 * Instance->MT2131_Info.AS_Data.f_ref))
        && (((f_in % Instance->MT2131_Info.AS_Data.f_ref) <= Instance->MT2131_Info.AS_Data.f_out_bw/2)
           || ((Instance->MT2131_Info.AS_Data.f_ref - (f_in % Instance->MT2131_Info.AS_Data.f_ref)) <= Instance->MT2131_Info.AS_Data.f_out_bw/2)))
    {
        /*
        **  Exclude LO frequencies that allow SRO harmonics to pass through
        */
        center = Instance->MT2131_Info.AS_Data.f_ref * ((Instance->MT2131_Info.AS_Data.f_if1_Center - Instance->MT2131_Info.AS_Data.f_if1_bw/2 + Instance->MT2131_Info.AS_Data.f_in) / Instance->MT2131_Info.AS_Data.f_ref) - Instance->MT2131_Info.AS_Data.f_in;
        while (center < Instance->MT2131_Info.AS_Data.f_if1_Center + Instance->MT2131_Info.AS_Data.f_if1_bw/2 + Instance->MT2131_Info.AS_Data.f_LO1_FracN_Avoid)
        {
            /*  Exclude LO1 FracN  */
            MT_AddExclZone(&Instance->MT2131_Info.AS_Data, center-Instance->MT2131_Info.AS_Data.f_LO1_FracN_Avoid, center+Instance->MT2131_Info.AS_Data.f_LO1_FracN_Avoid);
            center += Instance->MT2131_Info.AS_Data.f_ref;
        }

        center = Instance->MT2131_Info.AS_Data.f_ref * ((Instance->MT2131_Info.AS_Data.f_if1_Center - Instance->MT2131_Info.AS_Data.f_if1_bw/2 - Instance->MT2131_Info.AS_Data.f_out) / Instance->MT2131_Info.AS_Data.f_ref) + Instance->MT2131_Info.AS_Data.f_out;
        while (center < Instance->MT2131_Info.AS_Data.f_if1_Center + Instance->MT2131_Info.AS_Data.f_if1_bw/2 + Instance->MT2131_Info.AS_Data.f_LO2_FracN_Avoid)
        {
            /*  Exclude LO2 FracN  */
            MT_AddExclZone(&Instance->MT2131_Info.AS_Data, center-Instance->MT2131_Info.AS_Data.f_LO2_FracN_Avoid, center+Instance->MT2131_Info.AS_Data.f_LO2_FracN_Avoid);
            center += Instance->MT2131_Info.AS_Data.f_ref;
        }
    }

    f_IF1 = MT_ChooseFirstIF(&Instance->MT2131_Info.AS_Data);
    Instance->MT2131_Info.AS_Data.f_LO1 = Round_fLO(f_IF1 + f_in, Instance->MT2131_Info.AS_Data.f_LO1_Step, Instance->MT2131_Info.AS_Data.f_ref);
    Instance->MT2131_Info.AS_Data.f_LO2 = Round_fLO(Instance->MT2131_Info.AS_Data.f_LO1 - Instance->MT2131_Info.AS_Data.f_out - f_in, Instance->MT2131_Info.AS_Data.f_LO2_Step, Instance->MT2131_Info.AS_Data.f_ref);

    /*
    ** Check for any LO spurs in the output bandwidth and adjust
    ** the LO settings to avoid them if needed
    */
    status |= MT_AvoidSpurs(&Instance->MT2131_Info.AS_Data);/* shobhit #if 0  shuold be???*/

    /*
    ** MT_AvoidSpurs spurs may have changed the LO1 & LO2 values.
    ** Recalculate the LO frequencies and the values to be placed
    ** in the tuning registers.
    */
    Instance->MT2131_Info.AS_Data.f_LO1 = CalcLOMult(&LO1, &Num1, Instance->MT2131_Info.AS_Data.f_LO1, Instance->MT2131_Info.AS_Data.f_LO1_Step, Instance->MT2131_Info.AS_Data.f_ref);
    Instance->MT2131_Info.AS_Data.f_LO2 = Round_fLO(Instance->MT2131_Info.AS_Data.f_LO1 - Instance->MT2131_Info.AS_Data.f_out - f_in, Instance->MT2131_Info.AS_Data.f_LO2_Step, Instance->MT2131_Info.AS_Data.f_ref);
   Instance->MT2131_Info.AS_Data.f_LO2 = CalcLOMult(&LO2, &Num2, Instance->MT2131_Info.AS_Data.f_LO2, Instance->MT2131_Info.AS_Data.f_LO2_Step,Instance->MT2131_Info.AS_Data.f_ref);

    /*
    **  If we have the same LO frequencies and we're already locked,
    **  then skip re-programming the LO registers.
    */
    if ((ofLO1 != Instance->MT2131_Info.AS_Data.f_LO1)
        || (ofLO2 != Instance->MT2131_Info.AS_Data.f_LO2)
        || ((Instance->TunerRegVal[MT2131_LO_STATUS] & 0x88) != 0x88))
    {
        /*
        **  Check the upconverter and downconverter frequency ranges
        */
        if ((Instance->MT2131_Info.AS_Data.f_LO1 < MIN_UPC_FREQ) || ((Instance->MT2131_Info.AS_Data.f_LO1/10) > MAX_UPC_FREQ))
            status |= MT_UPC_RANGE;

        if ((Instance->MT2131_Info.AS_Data.f_LO2 < MIN_DNC_FREQ) || (Instance->MT2131_Info.AS_Data.f_LO2 > MAX_DNC_FREQ))
            status |= MT_DNC_RANGE;

        /*
        **  Place all of the calculated values into the local tuner
        **  register fields.
        */
        if (MT_NO_ERROR(status))
        {
            Instance->TunerRegVal[MT2131_UPC_1]  = (U8)((Instance->TunerRegVal[MT2131_UPC_1] & ~0x1F)
                                          | RFBand);       /* LNAGain, Band */
            Instance->TunerRegVal[MT2131_LO1C_1] = (U8)(Num1 >> 5);      /* LO1NUM (hi) */
            Instance->TunerRegVal[MT2131_LO1C_2] = (U8)(Num1 & 0x1F);    /* LO1NUM (lo) */
            Instance->TunerRegVal[MT2131_LO1C_3] = (U8)(LO1 & 0xFF);     /* LO1DIV */
            Instance->TunerRegVal[MT2131_LO2C_1] = (U8)(Num2 >> 5);      /* LO2NUM (hi) */
            Instance->TunerRegVal[MT2131_LO2C_2] = (U8)(Num2 & 0x1F);    /* LO2NUM (lo) */
            Instance->TunerRegVal[MT2131_LO2C_3] = (U8)(LO2 & 0xFF);     /* LO2DIV */
         #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
			 /*
            ** Now write out the computed register values
            ** IMPORTANT: There is a required order for writing some of the
            **            registers.  R3 must follow R1 & R2 and  R6 must
            **            follow R4 & R5. Simple numerical order works, so...
            */
           /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1C_1, &pInfo->reg[LO1C_1], 6);*/   /* 0x01 - 0x06 */
			status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle, MT2131_LO1C_1,&Instance->TunerRegVal[MT2131_LO1C_1],6);

		   }
		    #endif
		   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
	        {
	            /*
	            ** Now write out the computed register values
	            ** IMPORTANT: There is a required order for writing some of the
	            **            registers.  R3 must follow R1 & R2 and  R6 must
	            **            follow R4 & R5. Simple numerical order works, so...
	            */
	           /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1C_1, &pInfo->reg[LO1C_1], 6);*/   /* 0x01 - 0x06 */
				status |= ChipSetRegisters (Instance->IOHandle, MT2131_LO1C_1,&Instance->TunerRegVal[MT2131_LO1C_1],6);
	        }
         #endif
           
            if (o_UPC1 != Instance->TunerRegVal[MT2131_UPC_1])
                /*status |= MT_WriteSub(pInfo->hUserData, pInfo->address, UPC_1, &pInfo->reg[UPC_1], 1);*/ /* 0x0B        */
			status |= UncheckedSet(Instance, MT2131_UPC_1, Instance->TunerRegVal[MT2131_UPC_1]);
        }
    }

    /*  Reset the AGC  */
    if (MT_NO_ERROR(status))
    {
/*        status |= MT2131_FastResetAGC(pInfo);  */
        status |= MT2131_ResetAGC(Instance);

        /*
        **  Check for LO's locking
        */
        if (MT_NO_ERROR(status)
            && ((ofLO1 != Instance->MT2131_Info.AS_Data.f_LO1)
             || (ofLO2 != Instance->MT2131_Info.AS_Data.f_LO2)
             || ((Instance->TunerRegVal[MT2131_LO_STATUS] & 0x88) != 0x88)))
            status |= MT2131_GetLocked(Instance);

        /*
        **  If we locked OK, assign calculated data to MT2131_Info_t structure
        */
        if (MT_NO_ERROR(status))
        {
            Instance->MT2131_Info.f_IF1_actual = Instance->MT2131_Info.AS_Data.f_LO1 - f_in;
            status |= MT2131_OptimizeAGC(Instance);
        }

        /*  We want to call this after FastResetAGC() regardless of any errors  */
        status |= MT2131_WaitForAGC(Instance);
    }

    return (status);
}
