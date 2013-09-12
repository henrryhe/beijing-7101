/*****************************************************************************
**
**  Name: mt2060.c
**
**  Description:    Microtune MT2060 Tuner software interface.
**                  Supports tuners with Part/Rev code: 0x63.
**
**  Functions
**  Implemented:    U32  MT2060_Open
**                  U32  MT2060_Close
**                  U32  MT2060_ChangeFreq
**                  U32  MT2060_GetFMF
**                  U32  MT2060_GetGPO
**                  U32  MT2060_GetIF1Center (use MT2060_GetParam instead)
**                  U32  MT2060_GetLocked
**                  U32  MT2060_GetParam
**                  U32  MT2060_GetReg
**                  U32  MT2060_GetTemp
**                  U32  MT2060_GetUserData
**                  U32  MT2060_LocateIF1
**                  U32  MT2060_ReInit
**                  U32  MT2060_SetExtSRO
**                  U32  MT2060_SetGPO
**                  U32  MT2060_SetGainRange
**                  U32  MT2060_SetIF1Center (use MT2060_SetParam instead)
**                  U32  MT2060_SetParam
**                  U32  MT2060_SetReg
**                  U32  MT2060_SetUserData
**
**  References:     AN-00084: MT2060 Programming Procedures Application Note
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
**                  - Delay execution for x milliseconds
**
**  CVS ID:         $Id: mt2060.c,v 1.4 2007/02/20 17:53:41 thomas Exp $
**  CVS Source:     $Source: /mnt/pinz/cvsroot/stgui/stv0362/Stb_lla_generic/lla/mt2060.c,v $
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   03-02-2004    DAD    Adapted from mt2060.c source file.
**   N/A   04-20-2004    JWS    Modified to support handle-based calls
**   N/A   05-04-2004    JWS    Upgrades for A2/A3.
**   N/A   07-22-2004    DAD    Added function MT2060_SetGainRange.
**   N/A   09-03-2004    DAD    Add mean Offset of FMF vs. 1220 MHz over
**                              many devices when choosing the spline center
**                              point.
**   N/A   09-03-2004    DAD    Default VGAG to 1 (was 0).
**   N/A   09-10-2004    DAD    Changed LO1 FracN avoid region from 999,999
**                              to 2,999,999 Hz
**   078   01-26-2005    DAD    Ver 1.11: Force FM1CA bit to 1
**   079   01-20-2005    DAD    Ver 1.11: Fix setting of wrong bits in wrong
**                              register.
**   080   01-21-2005    DAD    Ver 1.11: Removed registers 0x0E through 0x11
**                              from the initialization routine.  All are
**                              reserved and not used.
**   081   01-21-2005    DAD    Ver 1.11: Changed LNA Band names to RF Band
**                              to match description in the data sheet.
**   082   02-01-2005    JWS    Ver 1.11: Support multiple tuners if present.
**   083   02-08-2005    JWS    Ver 1.11: Added separate version number for
**                              expected version of mt_spuravoid.h
**   084   02-08-2005    JWS    Ver 1.11: Reduced LO-related harmonics search
**                              from 11 to 5.
**   085   02-08-2005    DAD    Ver 1.11: Removed support for all but MT2060A3.
**   086   02-08-2005    JWS    Ver 1.11: Added call to MT_RegisterTuner().
**   088   01-26-2005    DAD    Added MT_TUNER_INIT_ERR.
**   089   02-17-2005    DAD    Ver 1.12: Fix bug in MT2060_SetReg() where
**                              the register was not getting set properly.
**   093   04-05-2005    DAD    Ver 1.13: MT2060_SetGainRange() is missing a
**                              break statement before the default handler.
**   095   04-05-2005    JWS    Ver 1.13: Clean up better if registration
**                              of the tuner fails in MT2060_Open.
**   099   06-21-2005    DAD    Ver 1.14: Increment i when waiting for MT2060
**                              to finish initializing.
**   102   08-16-2005    DAD    Ver 1.15: Fixed-point version added.  Location
**                              of 1st IF filter center requires 9-11 samples
**                              instead of 7 for floating-point version.
**   100   09-15-2005    DAD    Ver 1.15: Fixed register where temperature
**                              field is located (was MISC_CTRL_1).
**   101   09-15-2005    DAD    Ver 1.15: Re-wrote max- and zero-finding
**                              routines to take advantage of known cubic
**                              polynomial coefficients.
**
*****************************************************************************/

#include "stcommon.h"
#include "ioreg.h"
#include "chip.h"
#include <string.h>
#include <math.h>
#include <assert.h>  /*  debug purposes  */
#include "tunshdrv.h"    /* header for this file */
#include "d0362_map.h"
#ifdef STTUNER_DRV_SHARED_TUN_MT2060
#include "mt2060.h"
#endif

#include <stdlib.h>                     /* for NULL */

/*  Version of this module                         */


/*
**  By default, the floating-point FIFF location algorithm is used.
**  It provides faster FIFF location using fewer measurements, but
**  requires the use of a floating-point processor.
**
**
**       Interpolation | Processor  | Number of
**          Method     | Capability | Measurements
**      ---------------+------------+--------------
**       Cubic-spline  | floating   |      7
**       Linear        | fixed      |    9-11
**
**
**  See AN-00102 (Rev 1.1 or higher) for a detailed description of the
**  differences between the fixed-point and floating-point algorithm.
**
**  To use the fixed-point version of the FIFF center frequency location,
**  comment out the next source line.
*/
#define __FLOATING_POINT__

/*
**  The expected version of MT_AvoidSpurS32
**  If the version is different, an updated file is needed from Microtune
*/
/* Expecting version 1.20 of the Spur Avoidance API */
#define EXPECTED_MT_AVOID_SPURS_INFO_VERSION  010200

#define END_REGS MT2060_NBREGS

#define ceil(n, d) (((n) < 0) ? (-((-(n))/(d))) : (n)/(d) + ((n)%(d) != 0))
#define uceil(n, d) ((n)/(d) + ((n)%(d) != 0))
#define floor(n, d) (((n) < 0) ? (-((-(n))/(d))) - ((n)%(d) != 0) : (n)/(d))
#define ufloor(n, d) ((n)/(d))
#define    LO1_1      0x000c    /*  0x0C: LO1 Byte 1           */
 #define   LO1_2     0x000d     /*  0x0D: LO1 Byte 2           */
 
 extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];

void MT_AddExclZone(MT_AvoidSpursData_t* pAS_Info,
                    U32 f_min,
                    U32 f_max);


/**  RF Band cross-over frequencies
*/
static U32 MT2060_RF_Bands[] =
{
     95000000,     /*    0 ..  95 MHz: b1011      */
    180000000,     /*   95 .. 180 MHz: b1010      */
    260000000,     /*  180 .. 260 MHz: b1001      */
    335000000,     /*  260 .. 335 MHz: b1000      */
    425000000,     /*  335 .. 425 MHz: b0111      */
    490000000,     /*  425 .. 490 MHz: b0110      */
    570000000,     /*  490 .. 570 MHz: b0101      */
    645000000,     /*  570 .. 645 MHz: b0100      */
    730000000,     /*  645 .. 730 MHz: b0011      */
    810000000      /*  730 .. 810 MHz: b0010      */
                    /*  810 ..     MHz: b0001      */
};



/*
**  The number of Tuner Registers
*/

/*static U32 nMaxTuners = MT2060_CNT;
static MT2060_Info_t MT2060_Info[MT2060_CNT];
static MT2060_Info_t *Avail[MT2060_CNT];
static U32 nOpenTuners = 0;
static U32 InitDone = 0;*/

#if 0

/******************************************************************************
**
**  Name: MT2060_GetFMF
**
**  Description:    Get a new FMF register value.
**                  Uses The single step mode of the MT2060's FMF function.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire-bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire-bus
**                  MT_Sleep    - Delay execution for x milliseconds
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   05-04-2004    JWS    Upgrades for A2/A3.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
******************************************************************************/
U32 MT2060_GetFMF(Handle_t h)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    U32 idx;                             /* loop index variable          */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    /*  Set the FM1 Single-Step bit  */
    Instance->TunerRegVal[LO2C_1] |= 0x40;
    status = MT_WriteSub(pInfo->hUserData, pInfo->address, LO2C_1, &pInfo->reg[LO2C_1], 1);

    if (MT_NO_ERROR(status))
    {
        /*  Set the FM1CA bit to start the FMF location  */
        Instance->TunerRegVal[LO2C_1] |= 0x80;
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO2C_1, &pInfo->reg[LO2C_1], 1);
        Instance->TunerRegVal[LO2C_1] &= 0x7F;    /*  FM1CA bit automatically goes back to 0  */

        /*  Toggle the single-step 8 times  */
        for (idx=0; (idx<8) && (MT_NO_ERROR(status)); ++idx)
        {
            MT_Sleep(pInfo->hUserData, 20);
            /*  Clear the FM1 Single-Step bit  */
            Instance->TunerRegVal[LO2C_1] &= 0xBF;
            status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO2C_1, &pInfo->reg[LO2C_1], 1);

            MT_Sleep(pInfo->hUserData, 20);
            /*  Set the FM1 Single-Step bit  */
            Instance->TunerRegVal[LO2C_1] |= 0x40;
            status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO2C_1, &pInfo->reg[LO2C_1], 1);
        }
    }

    /*  Clear the FM1 Single-Step bit  */
    Instance->TunerRegVal[LO2C_1] &= 0xBF;
    status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO2C_1, &pInfo->reg[LO2C_1], 1);

    if (MT_NO_ERROR(status))
    {
        idx = 25;
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, MISC_STATUS, &pInfo->reg[MISC_STATUS], 1);

        /*  Poll FMCAL until it is set or we time out  */
        while ((MT_NO_ERROR(status)) && (--idx) && (Instance->TunerRegVal[MISC_STATUS] & 0x40) == 0)
        {
            MT_Sleep(pInfo->hUserData, 20);
            status = MT_ReadSub(pInfo->hUserData, pInfo->address, MISC_STATUS, &pInfo->reg[MISC_STATUS], 1);
        }

        if ((MT_NO_ERROR(status)) && ((Instance->TunerRegVal[MISC_STATUS] & 0x40) != 0))
        {
            /*  Read the 1st IF center offset register  */
            status = MT_ReadSub(pInfo->hUserData, pInfo->address, FM_FREQ, &pInfo->reg[FM_FREQ], 1);
            if (MT_NO_ERROR(status))
            {
                Instance->MT2060_Info.AS_Data.f_if1_Center = 1000000L * (1084 + Instance->TunerRegVal[FM_FREQ]);
            }
        }
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_GetGPO
**
**  Description:    Get the current MT2060 GPO setting(s).
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  gpo          - indicates which GPO pin(s) are read
**                                 MT2060_GPO_0 - GPO0 (MT2060 A1 pin 29)
**                                 MT2060_GPO_1 - GPO1 (MT2060 A1 pin 45)
**                                 MT2060_GPO   - GPO1 (msb) & GPO0 (lsb)
**                  *value       - value read from the GPO pin(s)
**
**                                 GPO1      GPO0
**                  gpo           pin 45    pin 29      *value
**                  ---------------------------------------------
**                  MT2060_GPO_0    X         0            0
**                  MT2060_GPO_0    X         1            1
**                  MT2060_GPO_1    0         X            0
**                  MT2060_GPO_1    1         X            1
**                  MT2060_GPO      0         0            0
**                  MT2060_GPO      0         1            1
**                  MT2060_GPO      1         0            2
**                  MT2060_GPO      1         1            3
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire-bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
******************************************************************************/
U32 MT2060_GetGPO(Handle_t h, U32 gpo, U32 *value)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    if (value == NULL)
        return MT_ARG_NULL;

    /*  We'll read the register just in case the write didn't work last time */
    status = MT_ReadSub(pInfo->hUserData, pInfo->address, MISC_CTRL_1, &pInfo->reg[MISC_CTRL_1], 1);

    switch (gpo)
    {
    case MT2060_GPO_0:
    case MT2060_GPO_1:
        *value = Instance->TunerRegVal[MISC_CTRL_1] & gpo ? 1 : 0;
        break;

    case MT2060_GPO:
        *value = Instance->TunerRegVal[MISC_CTRL_1] & gpo >> 1;
        break;

    default:
        status |= MT_ARG_RANGE;
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_GetIF1Center
**
**  Description:    Gets the MT2060 1st IF frequency.
**
**                  This function will be removed in future versions of this
**                  driver.  Instead, use the following function call:
**
**                  status = MT2060_GetParam(hMT2060,
**                                           MT2060_IF1_CENTER,
**                                           &f_if1_Center);
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  *value       - MT2060 1st IF frequency
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**
**  Dependencies:   None
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-15-2004    DAD    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
******************************************************************************/
U32 MT2060_GetIF1Center(Handle_t h, U32* value)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    if (value == NULL)
        return MT_ARG_NULL;

    *value = Instance->MT2060_Info.AS_Data.f_if1_Center;

    return status;
}

#endif
/**************************************************************************/


U32 IsValidHandle(TUNSHDRV_InstanceData_t *Instance)
{
     return (Instance != NULL) ? 1 : 0;
}

/********************************************/
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
/*****************************************/

 U32 UncheckedSet(TUNSHDRV_InstanceData_t *Instance,
                            U8         reg,
                            U8         val)
{
    U32 status;                  /* Status to be returned */

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
**  Name: MT2060_GetLocked
**
**  Description:    Checks to see if LO1 and LO2 are locked.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_UPC_UNLOCK    - Upconverter PLL unlocked
**                      MT_DNC_UNLOCK    - Downconverter PLL unlocked
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   MT_ReadSub    - Read byte(s) of data from the serial bus
**                  MT_Sleep      - Delay execution for x milliseconds
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
****************************************************************************/
U32 MT2060_GetLocked(TUNSHDRV_InstanceData_t *Instance)
{
    const U32 nMaxWait = 200;            /*  wait a maximum of 200 msec   */
    const U32 nPollRate = 2;             /*  poll status bits every 2 ms */
    const U32 nMaxLoops = nMaxWait / nPollRate;
    U32 status = MT_OK;                  /* Status to be returned        */
    U32 nDelays = 0;

    if (IsValidHandle(Instance) == 0)
        return MT_INV_HANDLE;

    do
    {
        /*status |= MT_ReadSub(pInfo->hUserData, pInfo->address, LO_STATUS, &pInfo->reg[LO_STATUS], 1);*/
        status |= UncheckedGet(Instance, LO_STATUS, &Instance->TunerRegVal[LO_STATUS]);

        if ((MT_IS_ERROR(status)) || ((Instance->TunerRegVal[LO_STATUS] & 0x88) == 0x88))
            return (TRUE);

      /*  MT_Sleep(pInfo->hUserData, nPollRate);*/       /*  Wait between retries  */
    }
    while (++nDelays < nMaxLoops);

    if ((Instance->TunerRegVal[LO_STATUS] & 0x80) == 0x00)
        status |= MT_UPC_UNLOCK;
    if ((Instance->TunerRegVal[LO_STATUS] & 0x08) == 0x00)
        status |= MT_DNC_UNLOCK;

    return (status);
}

#if 0
/****************************************************************************
**
**  Name: MT2060_GetParam
**
**  Description:    Gets a tuning algorithm parameter.
**
**                  This function provides access to the internals of the
**                  tuning algorithm - mostly for testing purposes.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  param       - Tuning algorithm parameter
**                                (see enum MT2060_Param)
**                  pValue      - ptr to returned value
**
**                  param                   Description
**                  ----------------------  --------------------------------
**                  MT2060_IC_ADDR          Serial Bus address of this tuner
**                  MT2060_MAX_OPEN         Max # of MT2060's allowed open
**                  MT2060_NUM_OPEN         # of MT2060's open
**                  MT2060_SRO_FREQ         crystal frequency
**                  MT2060_STEPSIZE         minimum tuning step size
**                  MT2060_INPUT_FREQ       input center frequency
**                  MT2060_LO1_FREQ         LO1 Frequency
**                  MT2060_LO1_STEPSIZE     LO1 minimum step size
**                  MT2060_LO1_FRACN_AVOID  LO1 FracN keep-out region
**                  MT2060_IF1_ACTUAL       Current 1st IF in use
**                  MT2060_IF1_REQUEST      Requested 1st IF
**                  MT2060_IF1_CENTER       Center of 1st IF SAW filter
**                  MT2060_IF1_BW           Bandwidth of 1st IF SAW filter
**                  MT2060_ZIF_BW           zero-IF bandwidth
**                  MT2060_LO2_FREQ         LO2 Frequency
**                  MT2060_LO2_STEPSIZE     LO2 minimum step size
**                  MT2060_LO2_FRACN_AVOID  LO2 FracN keep-out region
**                  MT2060_OUTPUT_FREQ      output center frequency
**                  MT2060_OUTPUT_BW        output bandwidth
**                  MT2060_LO_SEPARATION    min inter-tuner LO separation
**                  MT2060_AS_ALG           ID of avoid-spurs algorithm in use
**                  MT2060_MAX_HARM1        max # of intra-tuner harmonics
**                  MT2060_MAX_HARM2        max # of inter-tuner harmonics
**                  MT2060_EXCL_ZONES       # of 1st IF exclusion zones
**                  MT2060_NUM_SPURS        # of spurs found/avoided
**                  MT2060_SPUR_AVOIDED     >0 spurs avoided
**                  MT2060_SPUR_PRESENT     >0 spurs in output (mathematically)
**
**  Usage:          status |= MT2060_GetParam(hMT2060,
**                                            MT2060_IF1_ACTUAL,
**                                            &f_IF1_Actual);
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Invalid parameter requested
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**  See Also:       MT2060_SetParam, MT2060_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-02-2004    DAD    Original.
**
****************************************************************************/
U32 MT2060_GetParam(Handle_t     h,
                        MT2060_Param param,
                        U32*     pValue)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (pValue == NULL)
        status |= MT_ARG_NULL;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status |= MT_INV_HANDLE;

    if (MT_NO_ERROR(status))
    {
        switch (param)
        {
        /*  Serial Bus address of this tuner      */
        case MT2060_IC_ADDR:
            *pValue = pInfo->address;
            break;

        /*  Max # of MT2060's allowed to be open  */
        case MT2060_MAX_OPEN:
            *pValue = nMaxTuners;
            break;

        /*  # of MT2060's open                    */
        case MT2060_NUM_OPEN:
            *pValue = nOpenTuners;
            break;

        /*  crystal frequency                     */
        case MT2060_SRO_FREQ:
            *pValue = Instance->MT2060_Info.AS_Data.f_ref;
            break;

        /*  minimum tuning step size              */
        case MT2060_STEPSIZE:
            *pValue = Instance->MT2060_Info.AS_Data.f_LO2_Step;
            break;

        /*  input center frequency                */
        case MT2060_INPUT_FREQ:
            *pValue = Instance->MT2060_Info.AS_Data.f_in;
            break;

        /*  LO1 Frequency                         */
        case MT2060_LO1_FREQ:
            *pValue = Instance->MT2060_Info.AS_Data.f_LO1;
            break;

        /*  LO1 minimum step size                 */
        case MT2060_LO1_STEPSIZE:
            *pValue = Instance->MT2060_Info.AS_Data.f_LO1_Step;
            break;

        /*  LO1 FracN keep-out region             */
        case MT2060_LO1_FRACN_AVOID:
            *pValue = Instance->MT2060_Info.AS_Data.f_LO1_FracN_Avoid;
            break;

        /*  Current 1st IF in use                 */
        case MT2060_IF1_ACTUAL:
            *pValue = pInfo->f_IF1_actual;
            break;

        /*  Requested 1st IF                      */
        case MT2060_IF1_REQUEST:
            *pValue = Instance->MT2060_Info.AS_Data.f_if1_Request;
            break;

        /*  Center of 1st IF SAW filter           */
        case MT2060_IF1_CENTER:
            *pValue = Instance->MT2060_Info.AS_Data.f_if1_Center;
            break;

        /*  Bandwidth of 1st IF SAW filter        */
        case MT2060_IF1_BW:
            *pValue = Instance->MT2060_Info.AS_Data.f_if1_bw;
            break;

        /*  zero-IF bandwidth                     */
        case MT2060_ZIF_BW:
            *pValue = Instance->MT2060_Info.AS_Data.f_zif_bw;
            break;

        /*  LO2 Frequency                         */
        case MT2060_LO2_FREQ:
            *pValue = Instance->MT2060_Info.AS_Data.f_LO2;
            break;

        /*  LO2 minimum step size                 */
        case MT2060_LO2_STEPSIZE:
            *pValue = Instance->MT2060_Info.AS_Data.f_LO2_Step;
            break;

        /*  LO2 FracN keep-out region             */
        case MT2060_LO2_FRACN_AVOID:
            *pValue = Instance->MT2060_Info.AS_Data.f_LO2_FracN_Avoid;
            break;

        /*  output center frequency               */
        case MT2060_OUTPUT_FREQ:
            *pValue = Instance->MT2060_Info.AS_Data.f_out;
            break;

        /*  output bandwidth                      */
        case MT2060_OUTPUT_BW:
            *pValue = Instance->MT2060_Info.AS_Data.f_out_bw;
            break;

        /*  min inter-tuner LO separation         */
        case MT2060_LO_SEPARATION:
            *pValue = Instance->MT2060_Info.AS_Data.f_min_LO_Separation;
            break;

        /*  ID of avoid-spurs algorithm in use    */
        case MT2060_AS_ALG:
            *pValue = Instance->MT2060_Info.AS_Data.nAS_Algorithm;
            break;

        /*  max # of intra-tuner harmonics        */
        case MT2060_MAX_HARM1:
            *pValue = Instance->MT2060_Info.AS_Data.maxH1;
            break;

        /*  max # of inter-tuner harmonics        */
        case MT2060_MAX_HARM2:
            *pValue = Instance->MT2060_Info.AS_Data.maxH2;
            break;

        /*  # of 1st IF exclusion zones           */
        case MT2060_EXCL_ZONES:
            *pValue = Instance->MT2060_Info.AS_Data.nZones;
            break;

        /*  # of spurs found/avoided              */
        case MT2060_NUM_SPURS:
            *pValue = Instance->MT2060_Info.AS_Data.nSpursFound;
            break;

        /*  >0 spurs avoided                      */
        case MT2060_SPUR_AVOIDED:
            *pValue = Instance->MT2060_Info.AS_Data.bSpurAvoided;
            break;

        /*  >0 spurs in output (mathematically)   */
        case MT2060_SPUR_PRESENT:
            *pValue = Instance->MT2060_Info.AS_Data.bSpurPresent;
            break;

        case MT2060_EOP:
        default:
            status |= MT_ARG_RANGE;
        }
    }
    return (status);
}


/****************************************************************************
**
**  Name: MT2060_GetReg
**
**  Description:    Gets an MT2060 register.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  reg         - MT2060 register/subaddress location
**                  *val        - MT2060 register/subaddress value
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**                  Use this function if you need to read a register from
**                  the MT2060.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-28-2004    DAD    Original.
**
****************************************************************************/
U32 MT2060_GetReg(Handle_t h,
                      U8   reg,
                      U8*  val)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status |= MT_INV_HANDLE;

    if (val == NULL)
        status |= MT_ARG_NULL;

    if (reg >= END_REGS)
        status |= MT_ARG_RANGE;

    if (MT_NO_ERROR(status))
    {
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, reg, &pInfo->reg[reg], 1);
        if (MT_NO_ERROR(status))
            *val = Instance->TunerRegVal[reg];
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_GetTemp
**
**  Description:    Get the MT2060 Temperature register.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  *value       - value read from the register
**
**                                    Binary
**                  Value Returned    Value    Meaning
**                  ---------------------------------------------
**                  MT2060_T_0C       00       Temperature < 0C
**                  MT2060_0C_T_40C   01       0C < Temperature < 40C
**                  MT2060_40C_T_80C  10       40C < Temperature < 80C
**                  MT2060_80C_T      11       80C < Temperature
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-04-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**   100   09-15-2005    DAD    Ver 1.15: Fixed register where temperature
**                              field is located (was MISC_CTRL_1).
**
******************************************************************************/
U32 MT2060_GetTemp(Handle_t h, MT2060_Temperature* value)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    if (value == NULL)
        return MT_ARG_NULL;

    /* The Temp register bits have to be written before they can be read */
    status = MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_STATUS, &pInfo->reg[MISC_STATUS], 1);

    if (MT_NO_ERROR(status))
        status |= MT_ReadSub(pInfo->hUserData, pInfo->address, MISC_STATUS, &pInfo->reg[MISC_STATUS], 1);

    if (MT_NO_ERROR(status))
    {
        switch (Instance->TunerRegVal[MISC_STATUS] & 0x03)
        {
        case 0:
            *value = MT2060_T_0C;
            break;

        case 1:
            *value = MT2060_0C_T_40C;
            break;

        case 2:
            *value = MT2060_40C_T_80C;
            break;

        case 3:
            *value = MT2060_80C_T;
            break;
        }
    }

    return (status);
}


/****************************************************************************
**
**  Name: MT2060_GetUserData
**
**  Description:    Gets the user-defined data item.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**                  The hUserData parameter is a user-specific argument
**                  that is stored internally with the other tuner-
**                  specific information.
**
**                  For example, if additional arguments are needed
**                  for the user to identify the device communicating
**                  with the tuner, this argument can be used to supply
**                  the necessary information.
**
**                  The hUserData parameter is initialized in the tuner's
**                  Open function to NULL.
**
**  See Also:       MT2060_SetUserData, MT2060_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-25-2004    DAD    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
****************************************************************************/
U32 MT2060_GetUserData(Handle_t h,
                           Handle_t* hUserData)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status = MT_INV_HANDLE;

    if (hUserData == NULL)
        status |= MT_ARG_NULL;

    if (MT_NO_ERROR(status))
        *hUserData = pInfo->hUserData;

    return (status);
}

#endif
/******************************************************************************
**
**  Name: MT2060_ReInit
**
**  Description:    Initialize the tuner's register values.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_TUNER_ID_ERR   - Tuner Part/Rev code mismatch
**                      MT_TUNER_INIT_ERR - Tuner initialization failed
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
**   N/A   12-16-2004    DAD    Original, extracted from MT2060_Open.
**   078   01-26-2005    DAD    Ver 1.11: Force FM1CA bit to 1
**   085   02-08-2005    DAD    Ver 1.11: Removed support for all but MT2060A3.
**   081   01-21-2005    DAD    Ver 1.11: Changed LNA Band names to RF Band
**                              to match description in the data sheet.
**   088   01-26-2005    DAD    Added MT_TUNER_INIT_ERR.
**   099   06-21-2005    DAD    Ver 1.14: Increment i when waiting for MT2060
**                              to finish initializing.
**
******************************************************************************/
U32 MT2060_ReInit(TUNSHDRV_InstanceData_t *Instance)
{
    unsigned char MT2060_defaults[] =
    {
        0x3F,            /* reg[0x01] <- 0x3F */
        0x74,            /* reg[0x02] <- 0x74 */
        0x80,            /* reg[0x03] <- 0x80 (FM1CA = 1) */
        0x08,            /* reg[0x04] <- 0x08 */
        0x93,            /* reg[0x05] <- 0x93 */
        0x88,            /* reg[0x06] <- 0x88 (RO) */
        0x80,            /* reg[0x07] <- 0x80 (RO) */
        0x60,            /* reg[0x08] <- 0x60 (RO) */
        0x20,            /* reg[0x09] <- 0x20 */
        0x1E,            /* reg[0x0A] <- 0x1E */
        0x31,            /* reg[0x0B] <- 0x31 (VGAG = 1) */
        0xFF,            /* reg[0x0C] <- 0xFF */
        0x80,            /* reg[0x0D] <- 0x80 */
        0xFF,            /* reg[0x0E] <- 0xFF */
        0x00,            /* reg[0x0F] <- 0x00 */
        0x2C,            /* reg[0x10] <- 0x2C (HW def = 3C) */
        0x42            /* reg[0x11] <- 0x42 */
    };

    U32 status = MT_OK;                  /* Status to be returned        */

    S32 i;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
        status |= MT_INV_HANDLE;

    /*  Read the Part/Rev code from the tuner */
    if (MT_NO_ERROR(status))
       status |= UncheckedGet(Instance, PART_REV, &Instance->TunerRegVal[PART_REV]);

    if (MT_NO_ERROR(status) && (Instance->TunerRegVal[0] != 0x63))  /*  MT2060 A3  */
            status |= MT_TUNER_ID_ERR;      /*  Wrong tuner Part/Rev code   */

    if (MT_NO_ERROR(status))
    {

        for ( i = 0; i < 10; i++ )
        {
           Instance->MT2060_Info.RF_Bands[i] = MT2060_RF_Bands[i];
        }
        
        #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
	      
		    /*  Write the default values to each of the tuner registers.  */
      /*  status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1C_1, defaults, sizeof(defaults));*/
          status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle,LO1C_1,&MT2060_defaults[LO1C_1],sizeof(MT2060_defaults)  );
		   }
		    #endif
		    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
        {
            
		   /*  Write the default values to each of the tuner registers.  */
      /*  status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1C_1, defaults, sizeof(defaults));*/
          status |= ChipSetRegisters (Instance->IOHandle,LO1C_1,&MT2060_defaults[LO1C_1],sizeof(MT2060_defaults)  );
         }
         #endif
     

    }

    /*  Read back all the registers from the tuner */
    if (MT_NO_ERROR(status))
    	        #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
	      
		    /* status |= MT_ReadSub(pInfo->hUserData, pInfo->address, PART_REV, pInfo->reg, END_REGS);*/
        status |= STTUNER_IOREG_GetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle, PART_REV,END_REGS,&Instance->TunerRegVal[PART_REV]);
		   }
		    #endif
		    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
        {
            
		   /* status |= MT_ReadSub(pInfo->hUserData, pInfo->address, PART_REV, pInfo->reg, END_REGS);*/
        status |= ChipGetRegisters (Instance->IOHandle, PART_REV,END_REGS,&Instance->TunerRegVal[PART_REV]);
         }
         #endif
       


    /*  Wait for the MT2060 to finish initializing  */
    i = 0;
    while (MT_NO_ERROR(status) && ((Instance->TunerRegVal[MISC_STATUS] & 0x40) == 0x00))
    {
       /* MT_Sleep(pInfo->hUserData, 20);*/
       /* status |= MT_ReadSub(pInfo->hUserData, pInfo->address, MISC_STATUS, &pInfo->reg[MISC_STATUS], 1);*/
        status |=  UncheckedGet(Instance, MISC_STATUS, &Instance->TunerRegVal[MISC_STATUS]);
        if (i++ > 25)
        {
            /*  Timed out waiting for MT2060 to finish initializing  */
            status |= MT_TUNER_INIT_ERR;
            break;
        }
    }

    return (status);
}

#if 0
/****************************************************************************
**
**  Name: MT2060_SetExtSRO
**
**  Description:    Sets the external SRO driver.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  Ext_SRO_Setting - external SRO drive setting
**
**       (default)    MT2060_EXT_SRO_OFF  - ext driver off
**                    MT2060_EXT_SRO_BY_1 - ext driver = SRO frequency
**                    MT2060_EXT_SRO_BY_2 - ext driver = SRO/2 frequency
**                    MT2060_EXT_SRO_BY_4 - ext driver = SRO/4 frequency
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**                  The Ext_SRO_Setting settings default to OFF
**                  Use this function if you need to override the default
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-01-2004    DAD    Original.
**   079   01-20-2005    DAD    Ver 1.11: Fix setting of wrong bits in wrong
**                              register.
**
****************************************************************************/
U32 MT2060_SetExtSRO(Handle_t h,
                         MT2060_Ext_SRO Ext_SRO_Setting)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status = MT_INV_HANDLE;
    else
    {
        Instance->TunerRegVal[MISC_CTRL_3] = (U8) ((Instance->TunerRegVal[MISC_CTRL_3] & 0x3F) | (Ext_SRO_Setting << 6));
        status = MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_CTRL_3, &Instance->TunerRegVal[MISC_CTRL_3], 1);   /* 0x0B */
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_SetGainRange
**
**  Description:    Modify the MT2060 VGA Gain range.
**                  The MT2060's VGA has three separate gain ranges that affect
**                  both the minimum and maximum gain provided by the VGA.
**                  This setting moves the VGA's gain range up/down in
**                  approximately 5.5 dB steps.  See figure below.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  value        - Gain range to be set
**
**                  range                       Comment
**                  ---------------------------------------------------------
**                    0    MT2060_VGAG_0DB      VGA gain range offset 0 dB
**       (default)    1    MT2060_VGAG_5DB      VGA gain range offset +5 dB
**                    3    MT2060_VGAG_11DB     VGA gain range offset +11 dB
**
**
**                                     IF AGC Gain Range
**
**                 min     min+5dB   min+11dB        max-11dB  max-6dB      max
**                  v         v         v               v         v          v
**                  |---------+---------+---------------+---------+----------|
**
**                  <--------- MT2060_VGAG_0DB --------->
**
**                            <--------- MT2060_VGAG_5DB --------->
**
**                                      <--------- MT2060_VGAG_11DB --------->
**
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   MT_WRITESUB - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   07-22-2004    DAD    Original.
**   N/A   09-03-2004    DAD    Default VGAG to 1 (was 0).
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**   093   04-05-2005    DAD    Ver 1.13: MT2060_SetGainRange() is missing a
**                              break statement before the default handler.
**
******************************************************************************/
U32 MT2060_SetGainRange(Handle_t h, U32 range)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    switch (range)
    {
    case MT2060_VGAG_0DB:
    case MT2060_VGAG_5DB:
    case MT2060_VGAG_11DB:
        Instance->TunerRegVal[MISC_CTRL_3] = ((Instance->TunerRegVal[MISC_CTRL_3] & 0xFC) | (U8) range);
        status = MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_CTRL_3, &pInfo->reg[MISC_CTRL_3], 1);
        break;
    default:
        status = MT_ARG_RANGE;
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_SetGPO
**
**  Description:    Modify the MT2060 GPO value(s).
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  gpo          - indicates which GPO pin(s) are modified
**                                 MT2060_GPO_0 - GPO0 (MT2060 A1 pin 29)
**                                 MT2060_GPO_1 - GPO1 (MT2060 A1 pin 45)
**                                 MT2060_GPO   - GPO1 (msb) & GPO0 (lsb)
**                  value        - value to be written to the GPO pin(s)
**
**                                 GPO1      GPO0
**                  gpo           pin 45    pin 29       value
**                  ---------------------------------------------
**                  MT2060_GPO_0    X         0            0
**                  MT2060_GPO_0    X         1            1
**                  MT2060_GPO_1    0         X            0
**                  MT2060_GPO_1    1         X            1
**       (default)  MT2060_GPO      0         0            0
**                  MT2060_GPO      0         1            1
**                  MT2060_GPO      1         0            2
**                  MT2060_GPO      1         1            3
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   MT_WriteSub - Write byte(s) of data to the two-wire-bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
******************************************************************************/
U32 MT2060_SetGPO(Handle_t h, U32 gpo, U32 value)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    switch (gpo)
    {
    case MT2060_GPO_0:
    case MT2060_GPO_1:
        Instance->TunerRegVal[MISC_CTRL_1] = (U8) ((Instance->TunerRegVal[MISC_CTRL_1] & (~gpo)) | (value ? gpo : 0));
        status = MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_CTRL_1, &pInfo->reg[MISC_CTRL_1], 1);
        break;

    case MT2060_GPO:
        Instance->TunerRegVal[MISC_CTRL_1] = (U8) ((Instance->TunerRegVal[MISC_CTRL_1] & (~gpo)) | ((value & gpo) << 1));
        status = MT_WriteSub(pInfo->hUserData, pInfo->address, MISC_CTRL_1, &pInfo->reg[MISC_CTRL_1], 1);
        break;

    default:
        status = MT_ARG_RANGE;
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2060_SetIF1Center (obsolete)
**
**  Description:    Sets the MT2060 1st IF frequency.
**
**                  This function will be removed in future versions of this
**                  driver.  Instead, use the following function call:
**
**                  status = MT2060_SetParam(hMT2060,
**                                           MT2060_IF1_CENTER,
**                                           f_if1_Center);
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  value        - MT2060 1st IF frequency
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   None
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-15-2004    DAD    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
******************************************************************************/
U32 MT2060_SetIF1Center(Handle_t h, U32 value)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    Instance->MT2060_Info.AS_Data.f_if1_Center = value;
    Instance->MT2060_Info.AS_Data.f_if1_Request = value;

    return (status);
}


/****************************************************************************
**
**  Name: MT2060_SetParam
**
**  Description:    Sets a tuning algorithm parameter.
**
**                  This function provides access to the internals of the
**                  tuning algorithm.  You can override many of the tuning
**                  algorithm defaults using this function.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  param       - Tuning algorithm parameter
**                                (see enum MT2060_Param)
**                  nValue      - value to be set
**
**                  param                   Description
**                  ----------------------  --------------------------------
**                  MT2060_SRO_FREQ         crystal frequency
**                  MT2060_STEPSIZE         minimum tuning step size
**                  MT2060_INPUT_FREQ       Center of input channel
**                  MT2060_LO1_STEPSIZE     LO1 minimum step size
**                  MT2060_LO1_FRACN_AVOID  LO1 FracN keep-out region
**                  MT2060_IF1_REQUEST      Requested 1st IF
**                  MT2060_IF1_CENTER       Center of 1st IF SAW filter
**                  MT2060_IF1_BW           Bandwidth of 1st IF SAW filter
**                  MT2060_ZIF_BW           zero-IF bandwidth
**                  MT2060_LO2_STEPSIZE     LO2 minimum step size
**                  MT2060_LO2_FRACN_AVOID  LO2 FracN keep-out region
**                  MT2060_OUTPUT_FREQ      output center frequency
**                  MT2060_OUTPUT_BW        output bandwidth
**                  MT2060_LO_SEPARATION    min inter-tuner LO separation
**                  MT2060_MAX_HARM1        max # of intra-tuner harmonics
**                  MT2060_MAX_HARM2        max # of inter-tuner harmonics
**
**  Usage:          status |= MT2060_SetParam(hMT2060,
**                                            MT2060_STEPSIZE,
**                                            50000);
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Invalid parameter requested
**                                         or set value out of range
**                                         or non-writable parameter
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**  See Also:       MT2060_GetParam, MT2060_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-02-2004    DAD    Original.
**
****************************************************************************/
U32 MT2060_SetParam(Handle_t     h,
                        MT2060_Param param,
                        U32      nValue)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status |= MT_INV_HANDLE;

    if (MT_NO_ERROR(status))
    {
        switch (param)
        {
        /*  crystal frequency                     */
        case MT2060_SRO_FREQ:
            Instance->MT2060_Info.AS_Data.f_ref = nValue;
            Instance->MT2060_Info.AS_Data.f_LO1_FracN_Avoid = 3 * nValue / 16 - 1;
            Instance->MT2060_Info.AS_Data.f_LO2_FracN_Avoid = nValue / 160 - 1;
            Instance->MT2060_Info.AS_Data.f_LO1_Step = nValue / 64;
            break;

        /*  minimum tuning step size              */
        case MT2060_STEPSIZE:
            Instance->MT2060_Info.AS_Data.f_LO2_Step = nValue;
            break;

        /*  Center of input channel               */
        case MT2060_INPUT_FREQ:
            Instance->MT2060_Info.AS_Data.f_in = nValue;
            break;

        /*  LO1 minimum step size                 */
        case MT2060_LO1_STEPSIZE:
            Instance->MT2060_Info.AS_Data.f_LO1_Step = nValue;
            break;

        /*  LO1 FracN keep-out region             */
        case MT2060_LO1_FRACN_AVOID:
            Instance->MT2060_Info.AS_Data.f_LO1_FracN_Avoid = nValue;
            break;

        /*  Requested 1st IF                      */
        case MT2060_IF1_REQUEST:
            Instance->MT2060_Info.AS_Data.f_if1_Request = nValue;
            break;

        /*  Center of 1st IF SAW filter           */
        case MT2060_IF1_CENTER:
            Instance->MT2060_Info.AS_Data.f_if1_Center = nValue;
            break;

        /*  Bandwidth of 1st IF SAW filter        */
        case MT2060_IF1_BW:
            Instance->MT2060_Info.AS_Data.f_if1_bw = nValue;
            break;

        /*  zero-IF bandwidth                     */
        case MT2060_ZIF_BW:
            Instance->MT2060_Info.AS_Data.f_zif_bw = nValue;
            break;

        /*  LO2 minimum step size                 */
        case MT2060_LO2_STEPSIZE:
            Instance->MT2060_Info.AS_Data.f_LO2_Step = nValue;
            break;

        /*  LO2 FracN keep-out region             */
        case MT2060_LO2_FRACN_AVOID:
            Instance->MT2060_Info.AS_Data.f_LO2_FracN_Avoid = nValue;
            break;

        /*  output center frequency               */
        case MT2060_OUTPUT_FREQ:
            Instance->MT2060_Info.AS_Data.f_out = nValue;
            break;

        /*  output bandwidth                      */
        case MT2060_OUTPUT_BW:
            Instance->MT2060_Info.AS_Data.f_out_bw = nValue;
            break;

        /*  min inter-tuner LO separation         */
        case MT2060_LO_SEPARATION:
            Instance->MT2060_Info.AS_Data.f_min_LO_Separation = nValue;
            break;

        /*  max # of intra-tuner harmonics        */
        case MT2060_MAX_HARM1:
            Instance->MT2060_Info.AS_Data.maxH1 = nValue;
            break;

        /*  max # of inter-tuner harmonics        */
        case MT2060_MAX_HARM2:
            Instance->MT2060_Info.AS_Data.maxH2 = nValue;
            break;

        /*  These parameters are read-only  */
        case MT2060_IC_ADDR:
        case MT2060_MAX_OPEN:
        case MT2060_NUM_OPEN:
        case MT2060_LO1_FREQ:
        case MT2060_IF1_ACTUAL:
        case MT2060_LO2_FREQ:
        case MT2060_AS_ALG:
        case MT2060_EXCL_ZONES:
        case MT2060_SPUR_AVOIDED:
        case MT2060_NUM_SPURS:
        case MT2060_SPUR_PRESENT:
        case MT2060_EOP:
        default:
            status |= MT_ARG_RANGE;
        }
    }
    return (status);
}


/****************************************************************************
**
**  Name: MT2060_SetReg
**
**  Description:    Sets an MT2060 register.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  reg         - MT2060 register/subaddress location
**                  val         - MT2060 register/subaddress value
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**                  Use this function if you need to override a default
**                  register value
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-28-2004    DAD    Original.
**   089   02-17-2005    DAD    Ver 1.12: Fix bug in MT2060_SetReg() where
**                              the register was not getting set properly.
**
****************************************************************************/
U32 MT2060_SetReg(Handle_t h,
                      U8   reg,
                      U8   val)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status |= MT_INV_HANDLE;

    if (reg >= END_REGS)
        status |= MT_ARG_RANGE;

    if (MT_NO_ERROR(status))
    {
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, reg, &val, 1);
        if (MT_NO_ERROR(status))
            Instance->TunerRegVal[reg] = val;
    }

    return (status);
}


/****************************************************************************
**
**  Name: MT2060_SetUserData
**
**  Description:    Sets the user-defined data item.
**
**  Parameters:     h           - Tuner handle (returned by MT2060_Open)
**                  hUserData   - ptr to user-defined data
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   USERS MUST CALL MT2060_Open() FIRST!
**
**                  The hUserData parameter is a user-specific argument
**                  that is stored internally with the other tuner-
**                  specific information.
**
**                  For example, if additional arguments are needed
**                  for the user to identify the device communicating
**                  with the tuner, this argument can be used to supply
**                  the necessary information.
**
**                  The hUserData parameter is initialized in the tuner's
**                  Open function to NULL.
**
**  See Also:       MT2060_GetUserData, MT2060_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-25-2004    DAD    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
****************************************************************************/
U32 MT2060_SetUserData(Handle_t h,
                           Handle_t hUserData)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
        status = MT_INV_HANDLE;
    else
        pInfo->hUserData = hUserData;

    return (status);
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
    const U32 f_Scale = (f_LO1 / (MAX_UDATA/2 / pAS_Info->maxH1)) + 1;
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



/******************************************************/

static U32 Round_fLO(U32 f_LO, U32 f_LO_Step, U32 f_ref)
{
    return f_ref * (f_LO / f_ref)
        + f_LO_Step * (((f_LO % f_ref) + (f_LO_Step / 2)) / f_LO_Step);
}


/***********************************************************/

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
        tmpMin = floor( (pNode->min_ - f_Center),  f_Step);

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
U32 MT_AvoidSpurs(TUNSHDRV_InstanceData_t *Instance,
                      MT_AvoidSpursData_t* pAS_Info)
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
              (pAS_Info->bSpurPresent = IsSpurInBand(pAS_Info, &fm, &fp)));

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




/************************************************************/



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
**  Name: CalcLO1Mult
**
**  Description:    Calculates Integer divider value and the numerator
**                  value for LO1's FracN PLL.
**
**                  This function assumes that the f_LO and f_Ref are
**                  evenly divisible by f_LO_Step.
**
**  Parameters:     Div       - OUTPUT: Whole number portion of the multiplier
**                  FracN     - OUTPUT: Fractional portion of the multiplier
**                  f_LO      - desired LO frequency.
**                  f_LO_Step - Minimum step size for the LO (in Hz).
**                  f_Ref     - SRO frequency.
**
**  Returns:        Recalculated LO frequency.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-05-2004    JWS    Original
**
****************************************************************************/
static U32 CalcLO1Mult(U32 *Div,
                           U32 *FracN,
                           U32  f_LO,
                           U32  f_LO_Step,
                           U32  f_Ref)
{
    /*  Calculate the whole number portion of the divider */
    *Div = f_LO / f_Ref;

    /*  Calculate the numerator value (round to nearest f_LO_Step) */
    *FracN = (64 * (((f_LO % f_Ref) + (f_LO_Step / 2)) / f_LO_Step)+ (f_Ref / f_LO_Step / 2)) / (f_Ref / f_LO_Step);

    return (f_Ref * (*Div)) + (f_LO_Step * (*FracN));
}


/****************************************************************************
**
**  Name: CalcLO2Mult
**
**  Description:    Calculates Integer divider value and the numerator
**                  value for LO2's FracN PLL.
**
**                  This function assumes that the f_LO and f_Ref are
**                  evenly divisible by f_LO_Step.
**
**  Parameters:     Div       - OUTPUT: Whole number portion of the multiplier
**                  FracN     - OUTPUT: Fractional portion of the multiplier
**                  f_LO      - desired LO frequency.
**                  f_LO_Step - Minimum step size for the LO (in Hz).
**                  f_Ref     - SRO frequency.
**
**  Returns:        Recalculated LO frequency.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   11-04-2003    JWS    Original, derived from Mtuner version
**
****************************************************************************/
static U32 CalcLO2Mult(U32 *Div,
                           U32 *FracN,
                           U32  f_LO,
                           U32  f_LO_Step,
                           U32  f_Ref)
{
    const U32 FracN_Scale = (f_Ref / (MAX_UDATA >> 13)) + 1;

    /*  Calculate the whole number portion of the divider */
    *Div = f_LO / f_Ref;

    /*  Calculate the numerator value (round to nearest f_LO_Step) */
    *FracN = (8191 * (((f_LO % f_Ref) + (f_LO_Step / 2)) / f_LO_Step) + (f_Ref / f_LO_Step / 2)) / (f_Ref / f_LO_Step);

    return (f_Ref * (*Div))
         + FracN_Scale * (((f_Ref / FracN_Scale) * (*FracN)) / 8191);

}


/****************************************************************************
**
**  Name: LO1LockWorkAround
**
**  Description:    Correct for problem where LO1 does not lock at the
**                  transition point between VCO1 and VCO2.
**
**  Parameters:     pInfo       - Pointer to MT2060_Info Structure
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_UPC_UNLOCK    - Upconverter PLL unlocked
**                      MT_DNC_UNLOCK    - Downconverter PLL unlocked
**                      MT_COMM_ERR      - Serial bus communications error
**
**  Dependencies:   None
**
**                  MT_ReadSub      - Read byte(s) of data from the two-wire-bus
**                  MT_WriteSub     - Write byte(s) of data to the two-wire-bus
**                  MT_Sleep        - Delay execution for x milliseconds
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**
****************************************************************************/
static U32 LO1LockWorkAround(TUNSHDRV_InstanceData_t *Instance)
{
    U8 cs;
    U8 tad;
    U32 status = MT_OK;                  /* Status to be returned        */
    S32 ChkCnt = 0;

    cs = 30;  /* Pick starting CapSel Value */

    Instance->TunerRegVal[LO1_1] &= 0x7F;  /* Auto CapSelect off */
    Instance->TunerRegVal[LO1_2] = (Instance->TunerRegVal[LO1_2] & 0xC0) | cs;
   /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_1, &pInfo->reg[LO1_1], 2);*/  /* 0x0C - 0x0D */
status |= UncheckedSet(Instance, LO1_1, Instance->TunerRegVal[LO1_1]);
    while ((ChkCnt < 64) && (MT_NO_ERROR(status)))
    {
        /*MT_Sleep(pInfo->hUserData, 2);*/
        /*status |= MT_ReadSub(pInfo->hUserData, pInfo->address, LO_STATUS, &pInfo->reg[LO_STATUS], 1);*/
        status |= UncheckedGet(Instance, LO_STATUS, &Instance->TunerRegVal[LO_STATUS]);
        tad = (Instance->TunerRegVal[LO_STATUS] & 0x70) >> 4;  /* LO1AD */
        if (MT_NO_ERROR(status))
        {
            if (tad == 0)   /* Found good spot -- quit looking */
                break;
            else if (tad & 0x04 )  /* either 4 or 5; decrement */
            {
                Instance->TunerRegVal[LO1_2] = (Instance->TunerRegVal[LO1_2] & 0xC0) | --cs;
                /*status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_2, &pInfo->reg[LO1_2], 1);  */ /* 0x0D */
                status |= UncheckedSet(Instance, LO1_2, Instance->TunerRegVal[LO1_2]);
            }
            else if (tad & 0x02 )  /* either 2 or 3; increment */
            {
                Instance->TunerRegVal[LO1_2] = (Instance->TunerRegVal[LO1_2] & 0xC0) | ++cs;
               /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_2, &pInfo->reg[LO1_2], 1);  */ /* 0x0D */
                status |= UncheckedSet(Instance, LO1_2, Instance->TunerRegVal[LO1_2]);
            }
            ChkCnt ++;  /* Count this attempt */
        }
    }
    if (MT_NO_ERROR(status))
    {
       /* status |= MT_ReadSub(pInfo->hUserData, pInfo->address, LO_STATUS, &pInfo->reg[LO_STATUS], 1);*/
       status |= UncheckedGet(Instance, LO_STATUS, &Instance->TunerRegVal[LO_STATUS]);
    }

    if (MT_NO_ERROR(status))
    {
        if ((Instance->TunerRegVal[LO_STATUS] & 0x80) == 0x00)
            status |= MT_UPC_UNLOCK;

        if ((Instance->TunerRegVal[LO_STATUS] & 0x08) == 0x00)
            status |= MT_DNC_UNLOCK;
    }

    return (status);
}


/****************************************************************************
**
**  Name: MT2060_ChangeFreq
**
**  Description:    Change the tuner's tuned frequency to f_in.
**
**  Parameters:     h           - Open handle to the tuner (from MT2060_Open).
**                  f_in        - RF input center frequency (in Hz).
**                  f_IF1       - Desired IF1 center frequency (in Hz).
**                  f_out       - Output IF center frequency (in Hz)
**                  f_IFBW      - Output IF Bandwidth (in Hz)
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_UPC_UNLOCK    - Upconverter PLL unlocked
**                      MT_DNC_UNLOCK    - Downconverter PLL unlocked
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_SPUR_CNT_MASK - Count of avoided LO spurs
**                      MT_SPUR_PRESENT  - LO spur possible in output
**                      MT_FIN_RANGE     - Input freq out of range
**                      MT_FOUT_RANGE    - Output freq out of range
**                      MT_UPC_RANGE     - Upconverter freq out of range
**                      MT_DNC_RANGE     - Downconverter freq out of range
**
**  Dependencies:   MUST CALL MT2060_Open BEFORE MT2060_ChangeFreq!
**
**                  MT_ReadSub       - Read byte(s) of data from the two-wire-bus
**                  MT_WriteSub      - Write byte(s) of data to the two-wire-bus
**                  MT_Sleep         - Delay execution for x milliseconds
**                  MT2060_GetLocked - Checks to see if LO1 and LO2 are locked
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**   081   01-21-2005    DAD    Ver 1.11: Changed LNA Band names to RF Band
**                              to match description in the data sheet.
**
****************************************************************************/
U32 MT2060_ChangeFreq(TUNSHDRV_InstanceData_t *Instance, U32 f_in)   /* IF output bandwidth + guard */
{


    U32 status = MT_OK;       /*  status of operation             */
    U32 t_status;             /*  temporary status of operation   */
    U32 LO1;                  /*  1st LO register value           */
    U32 Num1;                 /*  Numerator for LO1 reg. value    */
    U32 LO2;                  /*  2nd LO register value           */
    U32 Num2;                 /*  Numerator for LO2 reg. value    */
    U32 ofLO1, ofLO2;         /*  last time's LO frequencies      */
    U32 ofin, ofout;          /*  last time's I/O frequencies     */
    U32 center;
    U32 RFBand;               /*  RF Band setting                 */
    U32 idx;                  /*  index loop                      */

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
        return MT_INV_HANDLE;

    /*  Check the input and output frequency ranges                   */
    if ((f_in < MIN_FIN_FREQ) || (f_in > MAX_FIN_FREQ))
        status |= MT_FIN_RANGE;

    if ((Instance->Status.IntermediateFrequency < MIN_FOUT_FREQ) || (Instance->Status.IntermediateFrequency > MAX_FOUT_FREQ))
        status |= MT_FOUT_RANGE;

    /*
    **  Save original LO1 and LO2 register values
    */
    ofLO1 = Instance->MT2060_Info.AS_Data.f_LO1;
    ofLO2 = Instance->MT2060_Info.AS_Data.f_LO2;
    ofin = Instance->MT2060_Info.AS_Data.f_in;
    ofout = Instance->MT2060_Info.AS_Data.f_out;

    /*
    **  Find RF Band setting
    */
    RFBand = 1;                        /*  def when f_in > all    */
    for (idx=0; idx<10; ++idx)
    {
        if (Instance->MT2060_Info.RF_Bands[idx] >= f_in)
        {
            RFBand = 11 - idx;
            break;
        }
    }
/*   VERBOSE_PRINT1("Using RF Band #%d.\n", RFBand);*/
    /*
    **  Assign in the requested values
    */
     Instance->MT2060_Info.AS_Data.f_in = f_in;
    Instance->MT2060_Info.AS_Data.f_out = Instance->Status.IntermediateFrequency;
    Instance->MT2060_Info.AS_Data.f_out_bw = Instance->ChannelBW;
    /*  Request a 1st IF such that LO1 is on a step size*/
    Instance->MT2060_Info.AS_Data.f_if1_Request = Round_fLO(Instance->FirstIF + f_in, Instance->MT2060_Info.AS_Data.f_LO1_Step, Instance->MT2060_Info.AS_Data.f_ref) - f_in;

    /*
    **  Calculate frequency settings.  f_IF1_FREQ + f_in is the
    **  desired LO1 frequency
    */
    MT_ResetExclZones(&Instance->MT2060_Info.AS_Data);

    if (((f_in % Instance->MT2060_Info.AS_Data.f_ref) <= Instance->ChannelBW/2)
        || ((Instance->MT2060_Info.AS_Data.f_ref - (f_in % Instance->MT2060_Info.AS_Data.f_ref)) <= Instance->ChannelBW/2))
    {
        /*
        **  Exclude LO frequencies that allow SRO harmonics to pass through
        */
        center = Instance->MT2060_Info.AS_Data.f_ref * ((Instance->MT2060_Info.AS_Data.f_if1_Center - Instance->MT2060_Info.AS_Data.f_if1_bw/2 + Instance->MT2060_Info.AS_Data.f_in) / Instance->MT2060_Info.AS_Data.f_ref) - Instance->MT2060_Info.AS_Data.f_in;
        while (center < Instance->MT2060_Info.AS_Data.f_if1_Center + Instance->MT2060_Info.AS_Data.f_if1_bw/2 + Instance->MT2060_Info.AS_Data.f_LO1_FracN_Avoid)
        {
            /*  Exclude LO1 FracN*/
            MT_AddExclZone(&Instance->MT2060_Info.AS_Data, center-Instance->MT2060_Info.AS_Data.f_LO1_FracN_Avoid, center+Instance->MT2060_Info.AS_Data.f_LO1_FracN_Avoid);
            center += Instance->MT2060_Info.AS_Data.f_ref;
        }

        center = Instance->MT2060_Info.AS_Data.f_ref * ((Instance->MT2060_Info.AS_Data.f_if1_Center - Instance->MT2060_Info.AS_Data.f_if1_bw/2 - Instance->MT2060_Info.AS_Data.f_out) / Instance->MT2060_Info.AS_Data.f_ref) + Instance->MT2060_Info.AS_Data.f_out;
        while (center < Instance->MT2060_Info.AS_Data.f_if1_Center + Instance->MT2060_Info.AS_Data.f_if1_bw/2 + Instance->MT2060_Info.AS_Data.f_LO2_FracN_Avoid)
        {
            /*  Exclude LO2 FracN*/
            MT_AddExclZone(&Instance->MT2060_Info.AS_Data, center-Instance->MT2060_Info.AS_Data.f_LO2_FracN_Avoid, center+Instance->MT2060_Info.AS_Data.f_LO2_FracN_Avoid);
            center += Instance->MT2060_Info.AS_Data.f_ref;
        }
    }

    Instance->FirstIF = MT_ChooseFirstIF(&Instance->MT2060_Info.AS_Data);
    Instance->MT2060_Info.AS_Data.f_LO1 = Round_fLO(Instance->FirstIF + f_in, Instance->MT2060_Info.AS_Data.f_LO1_Step, Instance->MT2060_Info.AS_Data.f_ref);
    Instance->MT2060_Info.AS_Data.f_LO2 = Round_fLO(Instance->MT2060_Info.AS_Data.f_LO1 - Instance->Status.IntermediateFrequency - f_in, Instance->MT2060_Info.AS_Data.f_LO2_Step, Instance->MT2060_Info.AS_Data.f_ref);

    /*
    ** Check for any LO spurs in the output bandwidth and adjust
    ** the LO settings to avoid them if needed
    */
    status |= MT_AvoidSpurs(Instance,&Instance->MT2060_Info.AS_Data);

    /*
    ** MT_AvoidSpurs spurs may have changed the LO1 & LO2 values.
    ** Recalculate the LO frequencies and the values to be placed
    ** in the tuning registers.
    */
    Instance->MT2060_Info.AS_Data.f_LO1 = CalcLO1Mult(&LO1, &Num1, Instance->MT2060_Info.AS_Data.f_LO1, Instance->MT2060_Info.AS_Data.f_LO1_Step, Instance->MT2060_Info.AS_Data.f_ref);
    Instance->MT2060_Info.AS_Data.f_LO2 = Round_fLO(Instance->MT2060_Info.AS_Data.f_LO1 - Instance->Status.IntermediateFrequency - f_in, Instance->MT2060_Info.AS_Data.f_LO2_Step, Instance->MT2060_Info.AS_Data.f_ref);
    Instance->MT2060_Info.AS_Data.f_LO2 = CalcLO2Mult(&LO2, &Num2, Instance->MT2060_Info.AS_Data.f_LO2, Instance->MT2060_Info.AS_Data.f_LO2_Step, Instance->MT2060_Info.AS_Data.f_ref);

    if ((ofLO1 == Instance->MT2060_Info.AS_Data.f_LO1) && (ofLO2 == Instance->MT2060_Info.AS_Data.f_LO2) && ((Instance->TunerRegVal[LO_STATUS] & 0x88) == 0x88))
    {
        Instance->MT2060_Info.f_IF1_actual = Instance->MT2060_Info.AS_Data.f_LO1 - f_in;
        return (status);
    }

    /*
    **  Check the upconverter and downconverter frequency ranges
    */
    if ((Instance->MT2060_Info.AS_Data.f_LO1 < MIN_UPC_FREQ) || (Instance->MT2060_Info.AS_Data.f_LO1 > MAX_UPC_FREQ))
        status |= MT_UPC_RANGE;

    if ((Instance->MT2060_Info.AS_Data.f_LO2 < MIN_DNC_FREQ) || (Instance->MT2060_Info.AS_Data.f_LO2 > MAX_DNC_FREQ))
        status |= MT_DNC_RANGE;

    /*
    **  Make sure that the Auto CapSelect is turned on since it
    **  may have been turned off last time we tuned.
    */
    if (MT_NO_ERROR(status) && ((Instance->TunerRegVal[LO1_1] & 0x80) == 0x00))
    {
        Instance->TunerRegVal[LO1_1] |= 0x80;
      /*  status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_1, &pInfo->reg[LO1_1], 1); */  /* 0x0C */
    status |=  UncheckedSet(Instance, LO1_1,Instance->TunerRegVal[LO1_1]);
    }

    /*
    **  Place all of the calculated values into the local tuner
    **  register fields.
    */
    if (MT_NO_ERROR(status))
    {
        Instance->TunerRegVal[LO1C_1] = (U8)((RFBand << 4) | (Num1 >> 2));
        Instance->TunerRegVal[LO1C_2] = (U8)(LO1 & 0xFF);

        Instance->TunerRegVal[LO2C_1] = (U8)((Num2 & 0x000F) | ((Num1 & 0x03) << 4));
        Instance->TunerRegVal[LO2C_2] = (U8)((Num2 & 0x0FF0) >> 4);
        Instance->TunerRegVal[LO2C_3] = (U8)(((LO2 << 1) & 0xFE) | ((Num2 & 0x1000) >> 12));
                #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
        {
	      
		     /*
        ** Now write out the computed register values
        ** IMPORTANT: There is a required order for writing some of the
        **            registers.  R2 must follow R1 and  R5 must
        **            follow R3 & R4. Simple numerical order works, so...
        */
       /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1C_1, &pInfo->reg[LO1C_1], 5);*/   /* 0x01 - 0x05 */
         status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle,LO1C_1,&Instance->TunerRegVal[LO1C_1],5 );
		   }
		    #endif
		    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
		         if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
        {
            
		    /*
        ** Now write out the computed register values
        ** IMPORTANT: There is a required order for writing some of the
        **            registers.  R2 must follow R1 and  R5 must
        **            follow R3 & R4. Simple numerical order works, so...
        */
       /* status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1C_1, &pInfo->reg[LO1C_1], 5);*/   /* 0x01 - 0x05 */
         status |= ChipSetRegisters (Instance->IOHandle,LO1C_1,&Instance->TunerRegVal[LO1C_1],5 );
         }
         #endif

       
    }

    /*
    **  Check for LO's locking
    */
    if (MT_NO_ERROR(status))
    {
        t_status = MT2060_GetLocked(Instance);
        if ((t_status & (MT_UPC_UNLOCK | MT_DNC_UNLOCK)) == MT_UPC_UNLOCK)
            /*  Special Case: LO1 not locked, LO2 locked  */
            status |= (t_status & ~MT_UPC_UNLOCK) | LO1LockWorkAround(Instance);
        else
            /*  OR in the status from the call to MT2060_GetLocked()  */
            status |= t_status;
    }

    /*
    **  If we locked OK, assign calculated data to MT2060_Info_t structure
    */
    if (MT_NO_ERROR(status))
    {
        Instance->MT2060_Info.f_IF1_actual = Instance->MT2060_Info.AS_Data.f_LO1 - f_in;
    }

    return (status);
}


#if defined (__FLOATING_POINT__)
#if 0
/****************************************************************************
**
**  Name: spline
**
**  Description:    Compute the 2nd derivatives of a set of x, y data for use
**                  in a cubic-spline interpolation.
**
**  Parameters:     x[9]        - array of independent variable (freqs)
**                  y[9]        - array of dependent variable (power)
**
**  Returns:        y2[9]       - 2nd derivative values at each x[]
**
**  Dependencies:   Requires floating point arithmetic.
**
**                  Assumes that the 1st derivative at each end point
**                  is zero (no slope).
**
**                  Assumes that there are exactly 9 data points.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-15-2004    DAD    From "Numerical Recipes in C", 2nd Ed.
**                              Modified for this particular application.
**   N/A   08-11-2004    DAD    Re-derived from cubic-spline equations
**                              using a specialized tri-diagonal matrix.
**
****************************************************************************/
static void spline(FData_t x[], FData_t y[], FData_t y2[])
{
    int i;
    FData_t dx[8],dy[8],w[8],d[8];

    dx[0] = x[1]-x[0];
    dy[0] = y[1]-y[0];
    d[0] = dx[0] * dx[0];       //  Assumes 2(x[2]-x[0]) is much greater than 1
    y2[0] = y2[8] = 0.0;
    w[0] = 0.0;

    /*  Notes for further optimization:
    **
    **  d[0] = 6889000000000000.0
    **  d[1] = 195999999.00000000
    **  d[2] = 58852040.810469598
    **  d[3] = 56176853.055536300
    **  d[4] = 55994791.666639537
    **  d[5] = 55981769.137752682
    **  d[6] = 55980834.413318574
    **  d[7] = 191980767.30441749
    **  dx[0]=dx[7] = 83000000
    **  dx[1..6] = 15000000  (15 MHz spacing from x[1] through x[7])
    **
    */
    for (i=1;i<8;i++)
    {
        dx[i] = (x[i+1] - x[i]);
        dy[i] = (y[i+1] - y[i]);
        w[i] = 6*((dy[i] / dx[i]) - (dy[i-1] / dx[i-1])) - (w[i-1] * dx[i-1] / d[i-1]);
        d[i] = 2 * (x[i+1] - x[i-1]) - dx[i-1] * dx[i-1] / d[i-1];
    }

    for (i=7;i>0;i--)
        y2[i] = (w[i] - dx[i]*y2[i+1]) / d[i];
}



/****************************************************************************
**
**  Name: findMax
**
**  Description:    Find the maximum x,y of a cubic-spline interpolated
**                  function.
**
**  Parameters:     x[9]        - array of independent variable (freqs)
**                  y[9]        - array of dependent variable (power)
**                  y2[9]       - 2nd derivative values at each x[]
**
**  Returns:        xmax        - frequency of maximum power
**                  return val  - interpolated power value at xmax
**
**  Dependencies:   Requires floating point arithmetic.
**
**                  Assumes that spline() has been called to calculate the
**                  values of y2[].
**
**                  Assumes that there is a peak between x[0] and x[8].
**
**                  Assumes that there are exactly 9 data points.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   101   09-15-2005    DAD    Ver 1.15: Re-wrote max- and zero-finding
**                              routines to take advantage of known cubic
**                              polynomial coefficients.
**
****************************************************************************/
static FData_t findMax(FData_t x[], FData_t y[], FData_t y2[], FData_t *xmax)
{
    FData_t ymax = y[0];
    U32 i;

    /*  Calculate the maximum of each cubic spline segment  */
    for (i=0; i<8; i++)
    {
        /*
        **  Compute the cubic polynomial coefficients:
        **
        **  y = ax^3 + bx^2 + cx + d
        **
        */
        FData_t dx = x[i+1] - x[i];
        FData_t dy = y[i+1] - y[i];
        FData_t a = (y2[i+1] - y2[i]) / (6 * dx);
        FData_t b = (x[i+1] * y2[i] - x[i] * y2[i+1]) / (2 * dx);
        FData_t c = (dy + a*(x[i]*x[i]*x[i]-x[i+1]*x[i+1]*x[i+1]) + b*(x[i]*x[i]-x[i+1]*x[i+1])) / dx;
        FData_t d = y[i] - a*x[i]*x[i]*x[i] - b*x[i]*x[i] - c*x[i];
        FData_t t = b*b - 3*a*c;

        if (ymax < y[i+1])
        {
            *xmax = x[i+1];
            ymax = y[i+1];
        }

        /*
        **  Solve the cubic polynomial's 1st derivative for zero
        **  to find the maxima or minima
        **
        **  y' = 3ax^2 + 2bx + c = 0
        **
        **       -b +/- sqrt(b^2 - 3ac)
        **  x = ---------------------------
        **                3a
        */

        /*  Skip this segment if x will be imaginary  */
        if (t>=0.0)
        {
            FData_t x_t;

            t = sqrt(t);
            x_t = (-b + t) / (3*a);
            /*  Only check answers that are between x[i] & x[i+1]  */
            if ((x[i]<=x_t) && (x_t<=x[i+1]))
            {
                FData_t y_t = a*x_t*x_t*x_t + b*x_t*x_t + c*x_t + d;
                if (ymax < y_t)
                {
                    *xmax = x_t;
                    ymax = y_t;
                }
            }

            x_t = (-b - t) / (3*a);
            /*  Only check answers that are between x[i] & x[i+1]  */
            if ((x[i]<=x_t) && (x_t<=x[i+1]))
            {
                FData_t y_t = a*x_t*x_t*x_t + b*x_t*x_t + c*x_t + d;
                if (ymax < y_t)
                {
                    *xmax = x_t;
                    ymax = y_t;
                }
            }
        }
    }
    return (ymax);
}


/****************************************************************************
**
**  Name: findRoot
**
**  Description:    Find the root x,y of a cubic-spline interpolated
**                  function, given the x, y, 2nd derivatives and a
**                  set of x values that bound the particular root to
**                  be found.
**
**                  If more than one root is bounded, the root closest to
**                  x_at_ymax will be found.
**
**
**                               -*  x_at_ymax
**                              /  \
**                             X    X
**                            /      \
**                  0 --------------------------
**                          /          \
**                         /            \
**                        X              X
**                       /                \
**                   X---                  ---X
**                   ^                        ^
**                   |                        |
**                   x_at_ymin    -- OR --    x_at_ymin
**
**
**  Parameters:     x[9]        - array of independent variable (freqs)
**                  y[9]        - array of dependent variable (power)
**                  y2[9]       - 2nd derivative values at each x[]
**                  x_at_ymax   - frequency where power > 0
**                  x_at_ymin   - frequency where power < 0
**
**  Returns:        xroot       - x-axis value of the root
**
**  Dependencies:   Requires floating point arithmetic.
**
**                  Assumes that spline() has been called to calculate the
**                  values of y2[].
**
**                  Assumes:
**                      y(x_at_ymax) > 0
**                      y(x_at_ymin) < 0
**
**                  Assumes that there are exactly 9 data points.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   101   09-15-2005    DAD    Ver 1.15: Re-wrote max- and zero-finding
**                              routines to take advantage of known cubic
**                              polynomial coefficients.
**
****************************************************************************/
static FData_t findRoot(FData_t x[],
                        FData_t y[],
                        FData_t y2[],
                        FData_t x_at_ymax,
                        FData_t x_at_ymin)
{
    FData_t x_upper, x_lower;           /*  limits for zero-crossing       */
    U32 i;
    FData_t dx, dy;
    FData_t a, b, c, d;                 /*  cubic coefficients             */
    FData_t xg;                         /*  x-axis guesses                 */
    FData_t y_err;                      /*  y-axis error of x-axis guess   */

    /*  Find the index into x[] that is just below x_ymax  */
    i = 8;
    while (x[i] > x_at_ymax)
        --i;

    if (x_at_ymax > x_at_ymin)
    {
        /*  Decrement i until y[i] < 0
        **  We want to end up with this:
        **
        **             X  {x[i+1], y[i+1]}
        **            /
        **  0 ----------------------
        **          /
        **         /
        **        X  {x[i], y[i]}
        **
        */
        while (y[i] > 0)
            --i;
        x_upper = (x_at_ymax < x[i+1]) ? x_at_ymax : x[i+1];
        x_lower = (x[i] < x_at_ymin) ? x_at_ymin : x[i];
    }
    else
    {
        /*  Increment i until y[i] < 0
        **  We want to end up with this:
        **
        **        X  {x[i], y[i]}
        **         \
        **  0 ----------------------
        **           \
        **            \
        **             X  {x[i+1], y[i+1]}
        **
        */
        while (y[++i] > 0);
        --i;
        x_upper = (x_at_ymin < x[i+1]) ? x_at_ymin : x[i+1];
        x_lower = (x[i] < x_at_ymax) ? x_at_ymax : x[i];
    }

    /*
    **  Compute the cubic polynomial coefficients:
    **
    **  y = ax^3 + bx^2 + cx + d
    **
    */
    dx = x[i+1] - x[i];
    dy = y[i+1] - y[i];
    a = (y2[i+1] - y2[i]) / (6 * dx);
    b = (x[i+1] * y2[i] - x[i] * y2[i+1]) / (2 * dx);
    c = (dy + a*(x[i]*x[i]*x[i]-x[i+1]*x[i+1]*x[i+1]) + b*(x[i]*x[i]-x[i+1]*x[i+1])) / dx;
    d = y[i] - a*x[i]*x[i]*x[i] - b*x[i]*x[i] - c*x[i];

    /*
    **  Make an initial guess at the root (bisection)
    */
    xg = (x_lower + x_upper) / 2.0;

    while (1)
    {
        FData_t u = 3*a*xg*xg+2*b*xg+c;
        FData_t v = 6*a*xg+2*b;
        FData_t t;

        /*
        **  Compute the y-axis value of this guess (error)
        */
        y_err = a*xg*xg*xg + b*xg*xg + c*xg + d;

        /*  Halt search if we're within 0.01 dB  */
        if (fabs(y_err) < 0.01)
            break;

        /*  Update upper and/or lower bound  */
        if ((x_at_ymax > x_at_ymin) ^ (y_err < 0))
            x_upper = xg;
        else
            x_lower = xg;

        /*  Check to see if we can use the quadratic form of the root solution  */
        t = u*u - 2*y_err*v;
        if (t>=0)
        {
            U32 bInBound2, bInBound3;
            FData_t xg2, xg3;

            /*  Compute new guesses  */
            t = sqrt(t);
            xg2 = xg - (u - t) / v;
            bInBound2 = (xg2 >= x_lower) && (xg2 <= x_upper);
            xg3 = xg - (u + t) / v;
            bInBound3 = (xg3 >= x_lower) && (xg3 <= x_upper);

            if (bInBound2 && bInBound3)
                /*  If both xg2 & xg3 are valid, choose the one closest to x_at_ymax  */
                xg = (fabs(xg2 - x_at_ymax) < fabs(xg3 - x_at_ymax)) ? xg2 : xg3;

            else if (bInBound2 != 0)
                xg = xg2;               /*  Keep this new guess  */

            else if (bInBound3 != 0)
                xg = xg3;               /*  Keep this new guess  */

            else
                /*  Quadratic form failed, use bisection  */
                xg = (x_lower + x_upper) / 2.0;
        }
        else
        {
            /*  Quadratic form failed, use bisection  */
            xg = (x_lower + x_upper) / 2.0;
        }
    }

    return (xg);
}
#endif
#endif

#if 0
/******************************************************************************
**
**  Name: MT2060_Program_LOs
**
**  Description:    Computes and programs MT2060 LO frequencies
**
**  Parameters:     pinfo       - Pointer to MT2060_Info Structure
**                  f_in        - Input frequency to the MT2060
**                                If 0 Hz, use injected LO frequency
**                  f_out       - Output frequency from MT2060
**                  f_IF1       - Desired 1st IF filter center frequency
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**
**  Dependencies:   None
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   102   08-16-2005    DAD    Ver 1.15: Fixed-point version added.  Location
**                              of 1st IF filter center requires 9-11 samples
**                              instead of 7 for floating-point version.
**
******************************************************************************/
static U32 MT2060_Program_LOs(MT2060_Info_t* pInfo,
                                  U32 f_in,
                                  U32 f_IF1,
                                  U32 f_out)
{
    U32 status = MT_OK;
    U32 LO1, Num1, LO2, Num2;
    U32 idx;
    long RFBand = 1;                  /*  RF Band setting  */

    if (f_in == 0)
    {
        /*  Force LO1 to feed through mode  */
        Instance->TunerRegVal[LO1_2] |= 0x40;
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_2, &pInfo->reg[LO1_2], 1);   /* 0x0D */
    }
    else
    {
        /*
        **  Find RF Band setting
        */
        for (idx=0; idx<10; ++idx)
        {
            if (pInfo->RF_Bands[idx] >= f_in)
            {
                RFBand = 11 - idx;
                break;
            }
        }
    }

    /*
    **  Assign in the requested values
    */
    Instance->MT2060_Info.AS_Data.f_in = f_in;
    Instance->MT2060_Info.AS_Data.f_out = f_out;

    /*  Program LO1 and LO2 for a 1st IF frequency of x[i]  */
    Instance->MT2060_Info.AS_Data.f_LO1 = CalcLO1Mult(&LO1, &Num1, (U32) f_IF1 + f_in, Instance->MT2060_Info.AS_Data.f_LO1_Step, Instance->MT2060_Info.AS_Data.f_ref);
    Instance->MT2060_Info.AS_Data.f_LO2 = Round_fLO(Instance->MT2060_Info.AS_Data.f_LO1 - f_out - f_in, Instance->MT2060_Info.AS_Data.f_LO2_Step, Instance->MT2060_Info.AS_Data.f_ref);
    Instance->MT2060_Info.AS_Data.f_LO2 = CalcLO2Mult(&LO2, &Num2, Instance->MT2060_Info.AS_Data.f_LO2, Instance->MT2060_Info.AS_Data.f_LO2_Step, Instance->MT2060_Info.AS_Data.f_ref);

    Instance->TunerRegVal[LO1C_1] = (U8)((RFBand << 4) | (Num1 >> 2));
    Instance->TunerRegVal[LO1C_2] = (U8)(LO1 & 0xFF);

    Instance->TunerRegVal[LO2C_1] = (U8)((Num2 & 0x000F) | ((Num1 & 0x03) << 4));
    Instance->TunerRegVal[LO2C_2] = (U8)((Num2 & 0x0FF0) >> 4);
    Instance->TunerRegVal[LO2C_3] = (U8)(((LO2 << 1) & 0xFE) | ((Num2 & 0x1000) >> 12));

    /*  Now write out the computed register values  */
    status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1C_1, &pInfo->reg[LO1C_1], 5);   /* 0x01 - 0x05 */

    if (MT_NO_ERROR(status))
        status |= MT2060_GetLocked((Handle_t) pInfo);

    return (status);
}
#endif
#if 0

/******************************************************************************
**
**  Name: MT2060_LocateIF1
**
**  Description:    Locates the MT2060 1st IF frequency with the help
**                  of a demodulator.
**
**                  The caller should examine the returned value P_max
**                  and verify that it is within the acceptable limits
**                  for the demodulator input levels.  If P_max is too
**                  large or small, the IF AGC voltage should be adjusted and
**                  this routine should be called again.  The gain slope
**                  of the IF AGC is approximately 20 dB/V.
**
**                  If P_max is acceptable, the user should then save
**                  f_IF1 in permanent storage (such as EEPROM).  After
**                  powering up and after calling MT2060_Open(), the
**                  user should call MT2060_SetIF1Center() with the saved
**                  f_IF1 to set the MT2060's 1st IF center frequency.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**                  f_in         - Input frequency to the MT2060
**                                 If 0 Hz, use injected LO frequency
**                  f_out        - Output frequency from MT2060
**                  *f_IF1       - 1st IF filter center frequency (output)
**                  *P_max       - maximum measured output power level (output)
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**
**  Dependencies:   None
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-15-2004    DAD    Original.
**   N/A   06-28-2004    DAD    Changed to use MT2060_GetFMF().
**   N/A   06-28-2004    DAD    Fix RF Band selection when f_in != 0.
**   N/A   07-01-2004    DAD    Place limits on FMF so that LO1 and LO2
**                              frequency ranges aren't breached.
**   N/A   09-03-2004    DAD    Add mean Offset of FMF vs. 1220 MHz over
**                              many devices when choosing the spline center
**                              point.
**   N/A   09-13-2004    DAD    Changed return value to provide status.
**   081   01-21-2005    DAD    Ver 1.11: Changed LNA Band names to RF Band
**                              to match description in the data sheet.
**   102   08-16-2005    DAD    Ver 1.15: Fixed-point version added.  Location
**                              of 1st IF filter center requires 9-11 samples
**                              instead of 7 for floating-point version.
**   101   09-15-2005    DAD    Ver 1.15: Re-wrote max- and zero-finding
**                              routines to take advantage of known cubic
**                              polynomial coefficients.
**
******************************************************************************/
U32 MT2060_LocateIF1(Handle_t h,
                         U32 f_in,
                         U32 f_out,
                         U32* f_IF1,
                         S32* P_max)
{
    U32 status = MT_OK;
    U32 i;
    U32 f_FMF;
    S32 Pmeas;
    U32 i_max = 1;

#if defined (__FLOATING_POINT__)
    FData_t x[9], y[9], y2[9];
    FData_t x_max, y_max;
    FData_t xm, xp;
#else
    U32 x[9];
    U32 x_lo, x_hi;
    S32 y[9];
    S32 y_lo, y_hi, y_meas;
    U32 dx, xm, xp;
    U32 n;
#endif
    MT2060_Info_t* pInfo = (MT2060_Info_t*) h;

    if (IsValidHandle(pInfo) == 0)
        return MT_INV_HANDLE;

    if ((f_IF1 == NULL) || (P_max == NULL))
        return MT_ARG_NULL;

    /*  Get a new FMF value from the MT2060  */
    status |= MT2060_GetFMF(h);

    if (MT_IS_ERROR(status))
        return (status);

    f_FMF = Instance->TunerRegVal[FM_FREQ];
    /*  Enforce limits on FMF so that the LO1 & LO2 limits are not breached  */
    /*  1088 <= f_LO1 <= 2214  */
    /*  1041 <= f_LO2 <= 1310  */
    if (f_FMF < 49)
        f_FMF = 49;
    if (f_FMF < ((f_out / 1000000) + 2))
        f_FMF = ((f_out / 1000000) + 2);
    if (f_FMF > ((f_out / 1000000) + 181))
        f_FMF = ((f_out / 1000000) + 181);

    /*  Set the frequencies to be used by the cubic spline  */
    x[0] = 1000000 * (Instance->MT2060_Info.AS_Data.f_ref / 16000000) * (f_FMF + 956);
    x[8] = 1000000 * (Instance->MT2060_Info.AS_Data.f_ref / 16000000) * (f_FMF + 1212);
    for (i=1; i<8; i++)
        x[i] = 1000000 * (Instance->MT2060_Info.AS_Data.f_ref / 16000000) * (f_FMF + 1024 + (15 * i));

    /*  Measure the power levels for the cubic spline at the given frequencies */
    for (i=1; ((i<8) && (MT_NO_ERROR(status))); i++)
    {
        /*  Program LO1 and LO2 for a 1st IF frequency of x[i]  */
        status |= MT2060_Program_LOs(pInfo, f_in, (U32) x[i], f_out);

        if (MT_NO_ERROR(status))
        {
            /*  Ask the demodulator to measure the gain of the MT2060  */
            Pmeas = (S32) x[i];
            status |= MT_TunerGain(pInfo->hUserData, &Pmeas);
#if defined (__FLOATING_POINT__)
            y[i] = Pmeas / 100.0;
#else
            y[i] = Pmeas;
#endif

            /*  Keep track of the maximum power level measured  */
            if ((i == 1) || (Pmeas > *P_max))
            {
                i_max = i;
                *P_max = Pmeas;
            }
        }
    }
    /*  Force end-points to be -50 dBc  */
#if defined (__FLOATING_POINT__)
    y[0] = y[8] = (FData_t) *P_max / 100.0 - 50.0;
#else
    y[0] = y[8] = *P_max - 5000;
#endif

    if (f_in == 0)                      /*  want to try, even if there were errors  */
    {
        /*  Turn off LO1 feed through mode  */
        Instance->TunerRegVal[LO1_2] &= 0xBF;
        status |= MT_WriteSub(pInfo->hUserData, pInfo->address, LO1_2, &pInfo->reg[LO1_2], 1);   /* 0x0D */
    }

    if (MT_NO_ERROR(status))
    {
#if defined (__FLOATING_POINT__)
        /*  Calculate the 2nd derivatives for each data point  */
        spline(x, y, y2);

        /*  Find the maximum of the cubic-spline curve */
        y_max = findMax(x, y, y2, &x_max);

        /*  Move the entire curve such that the peak is at +1.5 dB  */
        for (i=0;i<9;i++)
            y[i]=y[i]-y_max+1.5;

        /*  Find the -1.5 dBc points to the left and right of the peak  */
        xm = findRoot(x, y, y2, x_max, x[0]);
        xp = findRoot(x, y, y2, x_max, x[8]);
#else
        /*  The peak of the filter is x[i_max], y[i_max]  */

        /*  To Do:  Change y[] measurements type U32  */

        /*  Move the entire curve such that the peak is at +1.5 dB  */
        for (i=0;i<9;i++)
            y[i] -= (*P_max-150);

        /*
        **  Look for the 1.5 dBc (low frequency side)
        **  We are guaranteed that x[0] & x[8] are 50 dB
        **  below the peak measurement.
        */
        i = i_max - 1;
        while (y[i] > 0)
            i--;

        /*  The 1.5 dBc point is now bounded by x[i] .. x[i+1]  */
        y_lo = y[i];
        x_lo = x[i];
        y_hi = y[i+1];
        x_hi = x[i+1];

        /*
        **
        **               (x_hi, y_hi)
        **  Peak               o--------------o------
        **                    /
        **                  /
        **                /
        **  1.5 dBc ..../............................
        **            /
        **          /
        **         o
        **   (x_lo, y_lo)
        **
        */
        n = 0;                          /*  # times through the loop         */
        dx = (-y_lo < y_hi) ? 10000000 : 5000000;
        while (((x_hi - x_lo) > 5000000) && (++n<3))
        {
            status |= MT2060_Program_LOs(pInfo, f_in, x_hi - dx, f_out);
            if (MT_NO_ERROR(status))
            {
                y_meas = (S32) (x_hi - dx);
                status |= MT_TunerGain(pInfo->hUserData, &y_meas);
                if (MT_NO_ERROR(status))
                {
                    y_meas -= (*P_max-150);
                    if (y_meas > 0)
                    {
                        y_hi = y_meas;
                        x_hi = x_hi - dx;
                    }
                    else
                    {
                        y_lo = y_meas;
                        x_lo = x_hi - dx;
                    }
                }
            }
            dx = 5000000;
        }
        /*  Use linear interpolation  */
        xm = x_lo + (-y_lo * (x_hi - x_lo)) / (y_hi - y_lo);

        /*
        **  Look for the 1.5 dBc (high frequency side)
        **  We are guaranteed that x[0] & x[8] are 50 dB
        **  below the peak measurement.
        */
        i = i_max + 1;
        while (y[i] > 0)
            i++;

        /*  The 1.5 dBc point is now bounded by x[i-1] .. x[i]  */
        y_lo = y[i-1];
        x_lo = x[i-1];
        y_hi = y[i];
        x_hi = x[i];

        n = 0;                          /*  # times through the loop         */
        dx = (-y_hi > y_lo) ? 5000000 : 10000000;
        while (((x_hi - x_lo) > 5000000) && (++n<3))
        {
            status |= MT2060_Program_LOs(pInfo, f_in, x_lo + dx, f_out);
            if (MT_NO_ERROR(status))
            {
                y_meas = (S32) (x_lo + dx);
                status |= MT_TunerGain(pInfo->hUserData, &y_meas);
                if (MT_NO_ERROR(status))
                {
                    y_meas -= (*P_max-150);
                    if (y_meas > 0)
                    {
                        y_lo = y_meas;
                        x_lo += dx;
                    }
                    else
                    {
                        y_hi = y_meas;
                        x_hi = x_lo + dx;
                    }
                }
            }
            dx = 5000000;
        }
        /*  Use linear interpolation  */
        xp = x_lo + (y_lo * (x_hi - x_lo)) / (y_lo - y_hi);
#endif
        *f_IF1 = (U32) ((xm + xp) / 2);
    }

    return (status);
}
#endif
