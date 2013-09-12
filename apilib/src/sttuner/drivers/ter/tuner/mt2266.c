/*****************************************************************************
**
**  Name: mt2266.c
**
**  Description:    Microtune MT2266 Tuner software interface.
**                  Supports tuners with Part/Rev code: 0x85.
**
**  Functions
**  Implemented:    U32  MT2266_Open
**                  U32  MT2266_Close
**                  U32  MT2266_ChangeFreq
**                  U32  MT2266_GetLocked
**                  U32  MT2266_GetParam
**                  U32  MT2266_GetReg
**                  U32  MT2266_GetUserData
**                  U32  MT2266_ReInit
**                  U32  MT2266_SetParam
**                  U32  MT2266_SetPowerModes
**                  U32  MT2266_SetReg
**
**  References:     AN-00010: MicroTuner Serial Interface Application Note
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
**  CVS ID:         $Id: mt2266.c,v 1.2 2006/05/30 21:28:31 dessert Exp $
**  CVS Source:     $Source: /mnt/doc_software/scm/VC++/mtdrivers/mt2266_b/mt2266.c,v $
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
*****************************************************************************/
#include "stcommon.h"
#include "ioreg.h"	
#include "chip.h"		
#include <string.h>


#include "tuntdrv.h"    /* header for this file */	
#include "d0362_map.h"
#ifdef STTUNER_DRV_TER_TUN_MT2266
#include "mt2266.h"
#endif

/*  Version of this module                          */
#define VERSION 10000             /*  Version 01.00 */

/*#define STTUNER_TERR_TUNER_MT2266*/
/*
**  Normally, the "reg" array in the tuner structure is used as a cache
**  containing the current value of the tuner registers.  If the user's
**  application MUST change tuner registers without using the MT2266_SetReg
**  routine provided, he may compile this code with the __NO_CACHE__
**  variable defined.  
**  The PREFETCH macro will insert code code to re-read tuner registers if
**  __NO_CACHE__ is defined.  If it is not defined (normal) then PREFETCH
**  does nothing.
*/

#if defined(__NO_CACHE__)
#define PREFETCH(var, cnt) \
    if (MT_NO_ERROR(status)) \
    status |= MT_ReadSub(pInfo->hUserData, pInfo->address, (var), &pInfo->reg[(var)], (cnt));
#else
#define PREFETCH(var, cnt)
#endif

extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];



/*
** DefaultsEntry points to an array of U8 used to initialize
** various registers (the first byte is the starting subaddress)
** and a count of the bytes (including subaddress) in the array.
**
** DefaultsList is an array of DefaultsEntry elements terminated
** by an entry with a NULL pointer for the data array.
*/
typedef struct MT2266_DefaultsEntryTag
{
    U8 *data;
    U32 cnt;
} MT2266_DefaultsEntry;

typedef MT2266_DefaultsEntry MT2266_DefaultsList[];

#define DEF_LIST_ENTRY(a) {a, sizeof(a)/sizeof(U8) - 1}
#define END_DEF_LIST {0,0}



/*
**  The number of Tuner Registers
*/
/*static const U32 Num_Registers = END_REGS;*/



/*static U32 nMaxTuners = MT2266_CNT;*/
/*static MT2266_Info_t MT2266_Info[MT2266_CNT];
static MT2266_Info_t *Avail[MT2266_CNT];
static U32 nOpenTuners = 0;
static U8 InitDone = 0;*/


/*
**  Constants used to write a minimal set of registers when changing bands.
**  If the user wants a total reset, they should call MT2266_Open() again.
**  Skip 01, 02, 03, 04  (get overwritten anyways)
**  Write 05
**  Skip 06 - 18
**  Write 19   (diff for L-Band)
**  Skip 1A 1B 1C
**  Write 1D - 2B
**  Skip 2C - 3C
*/

static U8 MT2266_VHF_defaults1[] = 
{
    0x05,              /* address 0xC0, reg 0x05 */
    0x04,              /* Reg 0x05 LBANDen = 1 (that's right)*/
};
static U8 MT2266_VHF_defaults2[] = 
{
    0x19,              /* address 0xC0, reg 0x19 */
    0x61,              /* Reg 0x19 CAPto = 3*/
};
static U8 MT2266_VHF_defaults3[] = 
{
    0x1D,              /* address 0xC0, reg 0x1D */
    0xFE,              /* reg 0x1D */
    0x00,              /* reg 0x1E */
    0x00,              /* reg 0x1F */
    0xB4,              /* Reg 0x20 GPO = 1*/
    0x03,              /* Reg 0x21 LBIASen = 1, UBIASen = 1*/
    0xA5,              /* Reg 0x22 */
    0xA5,              /* Reg 0x23 */
    0xA5,              /* Reg 0x24 */
    0xA5,              /* Reg 0x25 */
    0x82,              /* Reg 0x26 CASCM = b0001 (bits reversed)*/
    0xAA,              /* Reg 0x27 */
    0xF1,              /* Reg 0x28 */
    0x17,              /* Reg 0x29 */
    0x80,              /* Reg 0x2A MIXbiasen = 1*/
    0x1F,              /* Reg 0x2B */
};

static MT2266_DefaultsList MT2266_VHF_defaults = {
    DEF_LIST_ENTRY(MT2266_VHF_defaults1),
    DEF_LIST_ENTRY(MT2266_VHF_defaults2),
    DEF_LIST_ENTRY(MT2266_VHF_defaults3),
    END_DEF_LIST
};

static U8 MT2266_UHF_defaults1[] = 
{
    0x05,              /* address 0xC0, reg 0x05 */
    0x52,              /* Reg 0x05 */
};
static U8 MT2266_UHF_defaults2[] = 
{
    0x19,              /* address 0xC0, reg 0x19 */
    0x61,              /* Reg 0x19 CAPto = 3*/
};
static U8 MT2266_UHF_defaults3[] = 
{
    0x1D,              /* address 0xC0, reg 0x1D */
    0xDC,              /* Reg 0x1D */
    0x00,              /* Reg 0x1E */
    0x0A,              /* Reg 0x1F */
    0xD4,              /* Reg 0x20 GPO = 1*/
    0x03,              /* Reg 0x21 LBIASen = 1, UBIASen = 1*/
    0x64,              /* Reg 0x22 */
    0x64,              /* Reg 0x23 */
    0x64,              /* Reg 0x24 */
    0x64,              /* Reg 0x25 */
    0x22,              /* Reg 0x26 CASCM = b0100 (bits reversed)*/
    0xAA,              /* Reg 0x27 */
    0xF2,              /* Reg 0x28 */
    0x1E,              /* Reg 0x29 */
    0x80,              /* Reg 0x2A MIXbiasen = 1*/
    0x14,              /* Reg 0x2B */
};

static MT2266_DefaultsList MT2266_UHF_defaults = {
    DEF_LIST_ENTRY(MT2266_UHF_defaults1),
    DEF_LIST_ENTRY(MT2266_UHF_defaults2),
    DEF_LIST_ENTRY(MT2266_UHF_defaults3),
    END_DEF_LIST
};

/* This table is used when MaxSensitivity is ON */
static U32 MT2266_UHF_XFreq[] = 
{
    443000 / TUNE_STEP_SIZE,     /*        < 443 MHz: 15+1 */
    470000 / TUNE_STEP_SIZE,     /*  443 ..  470 MHz: 15 */
    496000 / TUNE_STEP_SIZE,     /*  470 ..  496 MHz: 14 */
    525000 / TUNE_STEP_SIZE,     /*  496 ..  525 MHz: 13 */
    552000 / TUNE_STEP_SIZE,     /*  525 ..  552 MHz: 12 */
    580000 / TUNE_STEP_SIZE,     /*  552 ..  580 MHz: 11 */
    605000 / TUNE_STEP_SIZE,     /*  580 ..  605 MHz: 10 */
    632000 / TUNE_STEP_SIZE,     /*  605 ..  632 MHz:  9 */
    657000 / TUNE_STEP_SIZE,     /*  632 ..  657 MHz:  8 */
    682000 / TUNE_STEP_SIZE,     /*  657 ..  682 MHz:  7 */
    710000 / TUNE_STEP_SIZE,     /*  682 ..  710 MHz:  6 */
    735000 / TUNE_STEP_SIZE,     /*  710 ..  735 MHz:  5 */
    763000 / TUNE_STEP_SIZE,     /*  735 ..  763 MHz:  4 */
    802000 / TUNE_STEP_SIZE,     /*  763 ..  802 MHz:  3 */
    840000 / TUNE_STEP_SIZE,     /*  802 ..  840 MHz:  2 */
    877000 / TUNE_STEP_SIZE      /*  840 ..  877 MHz:  1 */
                                 /*  877+        MHz:  0 */
};

/* This table is used when MaxSensitivity is OFF */
static U32 MT2266_UHFA_XFreq[] = 
{
    442000 / TUNE_STEP_SIZE,     /*        < 442 MHz: 15+1 */
    472000 / TUNE_STEP_SIZE,     /*  442 ..  472 MHz: 15 */
    505000 / TUNE_STEP_SIZE,     /*  472 ..  505 MHz: 14 */
    535000 / TUNE_STEP_SIZE,     /*  505 ..  535 MHz: 13 */
    560000 / TUNE_STEP_SIZE,     /*  535 ..  560 MHz: 12 */
    593000 / TUNE_STEP_SIZE,     /*  560 ..  593 MHz: 11 */
    620000 / TUNE_STEP_SIZE,     /*  593 ..  620 MHz: 10 */
    647000 / TUNE_STEP_SIZE,     /*  620 ..  647 MHz:  9 */
    673000 / TUNE_STEP_SIZE,     /*  647 ..  673 MHz:  8 */
    700000 / TUNE_STEP_SIZE,     /*  673 ..  700 MHz:  7 */
    727000 / TUNE_STEP_SIZE,     /*  700 ..  727 MHz:  6 */
    752000 / TUNE_STEP_SIZE,     /*  727 ..  752 MHz:  5 */
    783000 / TUNE_STEP_SIZE,     /*  752 ..  783 MHz:  4 */
    825000 / TUNE_STEP_SIZE,     /*  783 ..  825 MHz:  3 */
    865000 / TUNE_STEP_SIZE,     /*  825 ..  865 MHz:  2 */
    905000 / TUNE_STEP_SIZE      /*  865 ..  905 MHz:  1 */
                                 /*  905+        MHz:  0 */
};

static const U32 Num_Registers = END_REGS;
U32 MT2266_pdcontrol_timestamp;
char MT2266_pdcontrol_enable=0;
static U32 nMaxTuners = MT2266_CNT;
U32 current_state = AGC_STATE_START;
U32 lna_gain_old=0;
U16 bbagc_min_start=0xffff;

 U32 IsValidHandle(TUNTDRV_InstanceData_t *Instance)
{
    return (Instance != NULL) ? 1 : 0;
}

/******************************************************************************
**
**  Name: Run_BB_RC_Cal2
**
**  Description:    Run Base Band RC Calibration (Method 2)
**                  MT2266 B0 only, others return MT_OK
**
**  Parameters:     hMT2266      - Handle to the MT2266 tuner
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   mt_errordef.h - definition of error codes
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
******************************************************************************/
 U32 Run_BB_RC_Cal2(TUNTDRV_InstanceData_t *Instance)
{
    U32 status = MT_OK;                  /* Status to be returned */
    U8 tmp_rcc;
    U8 dumy;
    


    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
    {
        status |= MT_INV_HANDLE;
    }

    /*
    ** Set the crystal frequency in the calibration register 
    ** and enable RC calibration #2
    */
    PREFETCH(MT2266_RCC_CTRL, 1);  /* Fetch register(s) if __NO_CACHE__ defined */
    tmp_rcc = Instance->TunerRegVal[MT2266_RCC_CTRL];
    if (Instance->MT2266_Info.f_Ref < (36000000 /*/ TUNE_STEP_SIZE*/))
    {
        tmp_rcc = (tmp_rcc & 0xDF) | 0x10;
    }
    else
    {
        tmp_rcc |= 0x30;
    }
    status |= UncheckedSet(Instance, MT2266_RCC_CTRL, tmp_rcc);

    /*  Read RC Calibration value  */
    status |= UncheckedGet(Instance, MT2266_STATUS_4, &dumy);

    /* Disable RC Cal 2 */
    status |= UncheckedSet(Instance, MT2266_RCC_CTRL, Instance->TunerRegVal[MT2266_RCC_CTRL] & 0xEF);


    /* Store RC Cal 2 value */
    Instance->MT2266_Info.RC2_Value = Instance->TunerRegVal[MT2266_STATUS_4];

    if (Instance->MT2266_Info.f_Ref < (36000000 /* TUNE_STEP_SIZE*/))
    {

        Instance->MT2266_Info.RC2_Nominal = (U8) ((Instance->MT2266_Info.f_Ref + 77570) / 155139);
    }
    else
    {

        Instance->MT2266_Info.RC2_Nominal = (U8) ((Instance->MT2266_Info.f_Ref + 93077) / 186154);
    }

    return (status);
}


/******************************************************************************
**
**  Name: Set_BBFilt
**
**  Description:    Set Base Band Filter bandwidth
**                  Based on SRO frequency & BB RC Calibration
**                  User stores channel bw as 5-8 MHz.  This routine
**                  calculates a 3 dB corner bw based on 1/2 the bandwidth
**                  and a bandwidth related constant.
**
**  Parameters:     hMT2266      - Handle to the MT2266 tuner
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   mt_errordef.h - definition of error codes
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
******************************************************************************/
 U32 Set_BBFilt(TUNTDRV_InstanceData_t *Instance)
{
    U32 f_3dB_bw;
    U8 BBFilt = 0;
    U8 Sel = 0;
    int  TmpFilt;
    int  i;
    U32 status = MT_OK;                  /* Status to be returned */
    



    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
    {
        status |= MT_INV_HANDLE;
    }

    /*
    ** Convert the channel bandwidth into a 3 dB bw by dividing it by 2
    ** and subtracting 300, 250, 200, or 0 kHz based on 8, 7, 6, 5 MHz
    ** channel bandwidth.
    */
    f_3dB_bw = (Instance->MT2266_Info.f_bw / 2);  /* bw -> bw/2 */

    if (Instance->MT2266_Info.f_bw > 7500000)
    {
        /*  >3.75 MHz corner  */
        f_3dB_bw -= 300000;
        Sel = 0x00;
        TmpFilt = ((429916107 / Instance->MT2266_Info.RC2_Value) * Instance->MT2266_Info.RC2_Nominal) / f_3dB_bw - 81;
    }
    else if (Instance->MT2266_Info.f_bw > 6500000)
    {
        /*  >3.25 MHz .. 3.75 MHz corner  */
        f_3dB_bw -= 250000;
        Sel = 0x00;
        TmpFilt = ((429916107 / Instance->MT2266_Info.RC2_Value) * Instance->MT2266_Info.RC2_Nominal) / f_3dB_bw - 81;
    }
    else if (Instance->MT2266_Info.f_bw > 5500000)
    {
        /*  >2.75 MHz .. 3.25 MHz corner  */
        f_3dB_bw -= 200000;
        Sel = 0x80;
        TmpFilt = ((429916107 / Instance->MT2266_Info.RC2_Value) * Instance->MT2266_Info.RC2_Nominal) / f_3dB_bw - 113;
    }
    else
    {
        /*  <= 2.75 MHz corner  */
        Sel = 0xC0;
        TmpFilt = ((429916107 / Instance->MT2266_Info.RC2_Value) * Instance->MT2266_Info.RC2_Nominal) / f_3dB_bw - 129;
    }

    if (TmpFilt > 63)
    {
        TmpFilt = 63;
     }
    else if (TmpFilt < 0)
    {
        TmpFilt = 0;
     }
    BBFilt = ((U8) TmpFilt) | Sel;

    for ( i = MT2266_BBFILT_1; i <= MT2266_BBFILT_8; i++ )
    {
    		
    	  Instance->TunerRegVal[i] = BBFilt;
    }

    if (MT_NO_ERROR(status))
        /*status |= MT_WriteSub(pInfo->hUserData,
                              pInfo->address,
                              MT2266_BBFILT_1,
                              &pInfo->reg[MT2266_BBFILT_1],
                              8);*/
      		 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				   	status |= ChipSetRegisters(Instance->IOHandle,MT2266_BBFILT_1,&Instance->TunerRegVal[MT2266_BBFILT_1],8);
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	   	status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle,MT2266_BBFILT_1,&Instance->TunerRegVal[MT2266_BBFILT_1],8);
			   	  }
			   	 #endif
    
    


    return (status);
}

    
/****************************************************************************
**
**  Name: MT2266_GetLocked
**
**  Description:    Checks to see if the PLL is locked.
**
**  Parameters:     h            - Open handle to the tuner (from MT2266_Open).
**
**  Returns:        status:
**                      MT_OK            - No errors
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
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
****************************************************************************/
BOOL MT2266_GetLocked(TUNTDRV_InstanceData_t *Instance)
{
    const U32 nMaxWait = 200;            /*  wait a maximum of 200 msec   */
    const U32 nPollRate = 2;             /*  poll status bits every 2 ms */
    const U32 nMaxLoops = nMaxWait / nPollRate;
    BOOL  status = FALSE;                  /* Status to be returned */
    U32 nDelays = 0;
    U8 statreg;


    if (IsValidHandle(Instance) == 0)
    {
        return MT_INV_HANDLE;
    }

    do
    {
        status |= UncheckedGet(Instance, MT2266_STATUS_1, &statreg);

        if ((MT_IS_ERROR(status)) || ((statreg & 0x40) == 0x40))
        {
            return (TRUE);
         }

        /*WAIT_N_MS(pInfo->hUserData, nPollRate);*/       /*  Wait between retries  */
        /* Put some wait here **/
        
    }while (++nDelays < nMaxLoops);

    if ((statreg & 0x40) != 0x40)
    {
        status = FALSE;
    }

    return (status);
}


/****************************************************************************
**  LOCAL FUNCTION - DO NOT USE OUTSIDE OF mt2266.c
**
**  Name: UncheckedGet
**
**  Description:    Gets an MT2266 register with minimal checking
**
**                  NOTE: This is a local function that performs the same
**                  steps as the MT2266_GetReg function that is available
**                  in the external API.  It does not do any of the standard
**                  error checking that the API function provides and should
**                  not be called from outside this file.
**
**  Parameters:     *pInfo      - Tuner control structure
**                  reg         - MT2266 register/subaddress location
**                  *val        - MT2266 register/subaddress value
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   USERS MUST CALL MT2266_Open() FIRST!
**
**                  Use this function if you need to read a register from
**                  the MT2266.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
****************************************************************************/
 U32 UncheckedGet(TUNTDRV_InstanceData_t *Instance,
                            U8   reg,
                            U8*  val)
{
    U32 status;                  /* Status to be returned        */

#if defined(_DEBUG)
    status = MT_OK;
    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(pInfo) == 0)
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
   /* status = MT_ReadSub(pInfo->hUserData, pInfo->address, reg, &pInfo->reg[reg], 1);*/
   	 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				   	Instance->TunerRegVal[reg] = ChipGetOneRegister(Instance->IOHandle, reg);
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	   	Instance->TunerRegVal[reg] = STTUNER_IOREG_GetRegister(&(Instance->DeviceMap),Instance->IOHandle, reg);
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
**  Name: MT2266_GetReg
**
**  Description:    Gets an MT2266 register.
**
**  Parameters:     h           - Tuner handle (returned by MT2266_Open)
**                  reg         - MT2266 register/subaddress location
**                  *val        - MT2266 register/subaddress value
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   USERS MUST CALL MT2266_Open() FIRST!
**
**                  Use this function if you need to read a register from
**                  the MT2266.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
****************************************************************************/
U32 MT2266_GetReg(TUNTDRV_InstanceData_t *Instance,
                      U8   reg,
                      U8*  val)
{
    U32 status = MT_OK;                  /* Status to be returned        */
   /* MT2266_Info_t* pInfo = (MT2266_Info_t*) h;*/

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

    if (MT_NO_ERROR(status))
    {
        status |= UncheckedGet(Instance, reg, val);
    }

    return (status);
}


/******************************************************************************
**
**  Name: MT2266_ReInit
**
**  Description:    Initialize the tuner's register values.
**
**  Parameters:     h           - Tuner handle (returned by MT2266_Open)
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
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
******************************************************************************/
U32 MT2266_ReInit(TUNTDRV_InstanceData_t *Instance)
{
    U8 MT2266_Init_Defaults1[] =
    {
    	0x01,0x08,0x88,0x22,0x00,0x52,0xdd,0x3f
    	
    };

    U8 MT2266_Init_Defaults2[] = 
    {
    	0x17,0x6d,0x71,0x61,0xcd,0xbf,0xff,0xdc,0x00,0x0a,0xd4,0x03,0x64,0x64,0x64,0x64,0x22,0xaa,0xf2,0x1e,0x80,0x14,0x01,0x01,0x01,0x01,0x01,0x01,0x7f,0x5e,0x3f,0xff,0xff,0xff,0x00,0x77,0x0f,0x2d
    	
    };

    U32 status = MT_OK;                  /* Status to be returned        */
    /*MT2266_Info_t* pInfo = (MT2266_Info_t*) h;*/
    U8 BBVref;
    U8 tmpreg = 0;
    U8 statusreg = 0;
    U8 tmpreg2 = 0;

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
    {
        status |= MT_INV_HANDLE;
        
    }



    /*  Read the Part/Rev code from the tuner */
    if (MT_NO_ERROR(status))
    {
        status |= UncheckedGet(Instance, MT2266_PART_REV, &tmpreg);
     }

    /*
    **  Read the status register 5
    */
    tmpreg2 = Instance->TunerRegVal[MT2266_RSVD_11] |= 0x03;
    if (MT_NO_ERROR(status))
    {
        status |= UncheckedSet(Instance, MT2266_RSVD_11, tmpreg2);
     }
    tmpreg2 &= ~(0x02);
    if (MT_NO_ERROR(status))
    {
        status |= UncheckedSet(Instance, MT2266_RSVD_11, tmpreg2);
       
     }
    

    /*  Get and store the status 5 register value  */
    if (MT_NO_ERROR(status))
    {
        status |= UncheckedGet(Instance, MT2266_STATUS_5, &statusreg);
    
     }

    /*  MT2266  */
    if (MT_NO_ERROR(status) && ( (tmpreg != 0x85) || ((statusreg & 0x30) != 0x30)))
    {
    
            status |= MT_TUNER_ID_ERR;      /*  Wrong tuner Part/Rev code   */
     }

    if (MT_NO_ERROR(status))
    {
        /*  Initialize the tuner state.  Hold off on f_in and f_LO */
        /*pInfo->version = VERSION;
        pInfo->tuner_id = pInfo->reg[MT2266_PART_REV];
        pInfo->f_Ref = REF_FREQ;
        pInfo->f_Step = TUNE_STEP_SIZE * 1000;*/  /* kHz -> Hz */
       /* pInfo->f_in = UHF_DEFAULT_FREQ;
        pInfo->f_LO = UHF_DEFAULT_FREQ;
        pInfo->f_bw = OUTPUT_BW;
        pInfo->band = MT2266_UHF_BAND;
        pInfo->num_regs = END_REGS;*/

        /*  Write the default values to the tuner registers. Default mode is UHF */
      /*  status |= MT_WriteSub(pInfo->hUserData, 
                              pInfo->address, 
                              MT2266_Init_Defaults1[0], 
                              &MT2266_Init_Defaults1[1], 
                              sizeof(MT2266_Init_Defaults1)/sizeof(U8)-1);*/
          	 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				    	status |= ChipSetRegisters(Instance->IOHandle, MT2266_Init_Defaults1[0],&MT2266_Init_Defaults1[1],sizeof(MT2266_Init_Defaults1)/sizeof(U8)-1);
				     	status |= ChipSetRegisters(Instance->IOHandle, MT2266_Init_Defaults2[0],&MT2266_Init_Defaults2[1],sizeof(MT2266_Init_Defaults2)/sizeof(U8)-1);
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	   	status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle, MT2266_Init_Defaults1[0],&MT2266_Init_Defaults1[1],sizeof(MT2266_Init_Defaults1)/sizeof(U8)-1);
			   	   status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle, MT2266_Init_Defaults2[0],&MT2266_Init_Defaults2[1],sizeof(MT2266_Init_Defaults2)/sizeof(U8)-1);
			   	  }
			   	 #endif
                        
                            
                              
         
          /*status |= MT_WriteSub(pInfo->hUserData, 
                              pInfo->address, 
                              MT2266_Init_Defaults2[0], 
                              &MT2266_Init_Defaults2[1], 
                              sizeof(MT2266_Init_Defaults2)/sizeof(U8)-1);*/
                              
                           
         
    }

    /*  Read back all the registers from the tuner */
    if (MT_NO_ERROR(status))
    {
    	
    	   	 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				    	status |= ChipGetRegisters(Instance->IOHandle,0/*REGISTER NAME */,END_REGS,&Instance->TunerRegVal[0]);
			
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	   	status |= STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,0/*REGISTER NAME */,END_REGS,&Instance->TunerRegVal[0]);
			   	  }
			   	 #endif
       /* status |= MT_ReadSub(pInfo->hUserData, pInfo->address, 0, &pInfo->reg[0], END_REGS);*/
        
    }

    /*
    **  Set reg[0x33] based on statusreg
    */
    if (MT_NO_ERROR(status))
    {
        BBVref = (((statusreg >> 6) + 2) & 0x03);
        tmpreg = (Instance->TunerRegVal[MT2266_RSVD_33] & ~(0x60)) | (BBVref << 5);
        status |= UncheckedSet(Instance, MT2266_RSVD_33, tmpreg);
    }

    /*  Run the baseband filter calibration  */
    if (MT_NO_ERROR(status))
    {
        status |= Run_BB_RC_Cal2(Instance);
     }

    /*  Set the baseband filter bandwidth to the default  */
    if (MT_NO_ERROR(status))
    {
        status |= Set_BBFilt(Instance);
    }

    return (status);
}


/****************************************************************************
**
**  Name: MT2266_SetParam
**
**  Description:    Sets a tuning algorithm parameter.
**
**                  This function provides access to the internals of the
**                  tuning algorithm.  You can override many of the tuning
**                  algorithm defaults using this function.
**
**  Parameters:     h           - Tuner handle (returned by MT2266_Open)
**                  param       - Tuning algorithm parameter 
**                                (see enum MT2266_Param)
**                  nValue      - value to be set
**
**                  param                   Description
**                  ----------------------  --------------------------------
**                  MT2266_SRO_FREQ         crystal frequency                   
**                  MT2266_STEPSIZE         minimum tuning step size            
**                  MT2266_INPUT_FREQ       Center of input channel             
**                  MT2266_OUTPUT_BW        Output channel bandwidth
**                  MT2266_RF_ATTN          RF attenuation (0-255)
**                  MT2266_RF_EXT           External control of RF atten
**                  MT2266_LNA_GAIN         LNA gain setting (0-15)
**                  MT2266_LNA_GAIN_DECR    Decrement LNA Gain (arg=min)
**                  MT2266_LNA_GAIN_INCR    Increment LNA Gain (arg=max)
**                  MT2266_BB_ATTN          Baseband attenuation (0-255)
**                  MT2266_BB_EXT           External control of BB atten
**                  MT2266_UHF_MAXSENS      Set for UHF max sensitivity mode
**                  MT2266_UHF_NORMAL       Set for UHF normal mode
**
**  Usage:          status |= MT2266_SetParam(hMT2266, 
**                                            MT2266_STEPSIZE,
**                                            50000);
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_RANGE     - Invalid parameter requested
**                                         or set value out of range
**                                         or non-writable parameter
**
**  Dependencies:   USERS MUST CALL MT2266_Open() FIRST!
**
**  See Also:       MT2266_GetParam, MT2266_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
****************************************************************************/
U32 MT2266_SetParam(TUNTDRV_InstanceData_t *Instance,
                        MT2266_Param param,
                        U32      nValue)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    U8 tmpreg;
   

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
    {
        status |= MT_INV_HANDLE;
    }

    if (MT_NO_ERROR(status))
    {
        switch (param)
        {
        /*  crystal frequency                     */
        case MT2266_SRO_FREQ:
            Instance->MT2266_Info.f_Ref = nValue;
            if (Instance->MT2266_Info.f_Ref < 22000000)
            {
                /*  Turn off f_SRO divide by 2  */
                status |= UncheckedSet(Instance, 
                                       MT2266_SRO_CTRL, 
                                      Instance->TunerRegVal[MT2266_SRO_CTRL] &= 0xFE);/*Typecasing by U8 removed
                                      								 for eliminating compilation error in OS20*/
            }
            else
            {
                /*  Turn on f_SRO divide by 2  */
                status |= UncheckedSet(Instance, 
                                       MT2266_SRO_CTRL, 
                                        Instance->TunerRegVal[MT2266_SRO_CTRL] |= 0x01);/*Typecasing by U8 removed
                                      								 for eliminating compilation error in OS20*/
            }
          
            status |= Run_BB_RC_Cal2(Instance);
            if (MT_NO_ERROR(status))
            {
                status |= Set_BBFilt(Instance);
            }
            break;

        /*  minimum tuning step size              */
        case MT2266_STEPSIZE:
            Instance->MT2266_Info.f_Step = nValue;
            break;

        /*  Width of output channel               */
        case MT2266_OUTPUT_BW:
            Instance->MT2266_Info.f_bw = nValue;
            status |= Set_BBFilt(Instance);
            break;

        /*  BB attenuation (0-255)                */
        case MT2266_BB_ATTN:
            if (nValue > 255)
            {
                status |= MT_ARG_RANGE;
            }
            else
            {
                U32 BBA_Stage1;
                U32 BBA_Stage2;
                U32 BBA_Stage3;

                BBA_Stage3 = (nValue > 102) ? 103 : nValue + 1;
                BBA_Stage2 = (nValue > 175) ? 75 : nValue + 2 - BBA_Stage3;
                BBA_Stage1 = (nValue > 176) ? nValue - 175 : 1;
                Instance->TunerRegVal[MT2266_RSVD_2C] = (U8) BBA_Stage1;
                Instance->TunerRegVal[MT2266_RSVD_2D] = (U8) BBA_Stage2;
                Instance->TunerRegVal[MT2266_RSVD_2E] = (U8) BBA_Stage3;
                Instance->TunerRegVal[MT2266_RSVD_2F] = (U8) BBA_Stage1;
                Instance->TunerRegVal[MT2266_RSVD_30] = (U8) BBA_Stage2;
                Instance->TunerRegVal[MT2266_RSVD_31] = (U8) BBA_Stage3;
               /* status |= MT_WriteSub(pInfo->hUserData, 
                                      pInfo->address, 
                                      MT2266_RSVD_2C, 
                                      &pInfo->reg[MT2266_RSVD_2C], 
                                      6);*/
                 	   	 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				    status |= ChipSetRegisters(Instance->IOHandle, MT2266_RSVD_2C,&Instance->TunerRegVal[MT2266_RSVD_2C],6);     
			
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	   status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle, MT2266_RSVD_2C,&Instance->TunerRegVal[MT2266_RSVD_2C],6);                                     
			   	  }
			   	 #endif                        
	         
            }
            break;

        /*  RF attenuation (0-255)                */
        case MT2266_RF_ATTN:
            if (nValue > 255)
            {
                status |= MT_ARG_RANGE;
             }
            else
            {
                status |= UncheckedSet(Instance, MT2266_RSVD_1F, (U8) nValue);
            }
            break;

        /*  RF external / internal atten control  */
        case MT2266_RF_EXT:
            if (nValue == 0)
            {
                tmpreg = Instance->TunerRegVal[MT2266_GPO] &= ~0x40;
            }
            else
            {
                tmpreg = Instance->TunerRegVal[MT2266_GPO] |= 0x40;
            }
            status |= UncheckedSet(Instance, MT2266_GPO, tmpreg);
            break;

        /*  LNA gain setting (0-15)               */
        case MT2266_LNA_GAIN:
            if (nValue > 15)
            {
                status |= MT_ARG_RANGE;
            }
            else
            {
                tmpreg = (Instance->TunerRegVal[MT2266_IGAIN] & 0xC3) | ((U8)nValue << 2);
                status |= UncheckedSet(Instance, MT2266_IGAIN, tmpreg);
            }
            break;

        /*  Decrement LNA Gain setting, argument is min LNA Gain setting  */
        case MT2266_LNA_GAIN_DECR:
            if (nValue > 15)
            {
                status |= MT_ARG_RANGE;
            }
            else
            {
                PREFETCH(MT2266_IGAIN, 1);
                if (MT_NO_ERROR(status) && ((U8) ((Instance->TunerRegVal[MT2266_IGAIN] & 0x3C) >> 2) > (U8) nValue))
                {
                    status |= UncheckedSet(Instance, MT2266_IGAIN, Instance->TunerRegVal[MT2266_IGAIN] - 0x04);
                }
            }
            break;

        /*  Increment LNA Gain setting, argument is max LNA Gain setting  */
        case MT2266_LNA_GAIN_INCR:
            if (nValue > 15)
            {
                status |= MT_ARG_RANGE;
            }
            else
            {
                PREFETCH(MT2266_IGAIN, 1);
                if (MT_NO_ERROR(status) && ((U8) ((Instance->TunerRegVal[MT2266_IGAIN] & 0x3C) >> 2) < (U8) nValue))
                {
                    status |= UncheckedSet(Instance, MT2266_IGAIN, Instance->TunerRegVal[MT2266_IGAIN] + 0x04);
                }
            }
            break;

        /*  BB external / internal atten control  */
        case MT2266_BB_EXT:
            if (nValue == 0)
            {
                tmpreg = Instance->TunerRegVal[MT2266_RSVD_33] &= ~0x08;
            }
            else
            {
                tmpreg = Instance->TunerRegVal[MT2266_RSVD_33] |= 0x08;
            }
            status |= UncheckedSet(Instance, MT2266_RSVD_33, tmpreg);
            break;

        /*  Set for UHF max sensitivity mode  */
        case MT2266_UHF_MAXSENS:
            PREFETCH(MT2266_BAND_CTRL, 1);
            if (MT_NO_ERROR(status) && ((Instance->TunerRegVal[MT2266_BAND_CTRL] & 0x30) == 0x10))
            {
                status |= UncheckedSet(Instance, MT2266_BAND_CTRL, Instance->TunerRegVal[MT2266_BAND_CTRL] ^ 0x30);
            }
            break;

        /*  Set for UHF normal mode  */
        case MT2266_UHF_NORMAL:
            if (MT_NO_ERROR(status) && ((Instance->TunerRegVal[MT2266_BAND_CTRL] & 0x30) == 0x20))
            {
                status |= UncheckedSet(Instance, MT2266_BAND_CTRL, Instance->TunerRegVal[MT2266_BAND_CTRL] ^ 0x30);
            }
            break;

        /*  These parameters are read-only  */
        case MT2266_IC_ADDR:
        case MT2266_MAX_OPEN:
        case MT2266_NUM_OPEN:
        case MT2266_NUM_REGS:
        case MT2266_INPUT_FREQ:
        case MT2266_LO_FREQ:
        case MT2266_RC2_VALUE:
        case MT2266_RF_ADC:
        case MT2266_BB_ADC:
        case MT2266_EOP:
        default:
            status |= MT_ARG_RANGE;
        }
    }
    return (status);
}





/***************************************************************************

*****************************************************************************/


U32 MT2266_GetParam(TUNTDRV_InstanceData_t *Instance,MT2266_Param param, U32      *pValue)
{
    U32 status = MT_OK;                  /* Status to be returned        */
    U8 tmp;


    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
    {
        status |= MT_INV_HANDLE;
    }
if (MT_NO_ERROR(status))
    {
        switch (param)
        {
         /*  Serial Bus address of this tuner      */
         case MT2266_IC_ADDR:
         *pValue = Instance->MT2266_Info.address;
         break;
         /*  Max # of MT2266's allowed to be open  */
         case MT2266_MAX_OPEN:
         *pValue = nMaxTuners;
         break;
         /*  Number of tuner registers             */
         case MT2266_NUM_REGS:
         *pValue = Num_Registers;
         break;
         /*  crystal frequency                     */
         case MT2266_SRO_FREQ:
         *pValue = Instance->MT2266_Info.f_Ref;
         break;
         /*  minimum tuning step size              */
         case MT2266_STEPSIZE:
         *pValue = Instance->MT2266_Info.f_Step;
         break;        /*  input center frequency                */
         case MT2266_INPUT_FREQ:
         *pValue = Instance->MT2266_Info.f_in;
         break;        /*  LO Frequency                          */
         case MT2266_LO_FREQ:
         *pValue = Instance->MT2266_Info.f_LO;
         break;
         /*  Output Channel Bandwidth              */
         case MT2266_OUTPUT_BW:
         *pValue = Instance->MT2266_Info.f_bw;
         break;
         /*  Base band filter cal RC code          */
         case MT2266_RC2_VALUE:
         *pValue = (U32) Instance->MT2266_Info.RC2_Value;
         break;
         /*  Base band filter nominal cal RC code          */

         case MT2266_RC2_NOMINAL:
         *pValue = (U32) Instance->MT2266_Info.RC2_Nominal;
         break;        /*  RF attenuator A/D readback            */
         case MT2266_RF_ADC:
         status |= UncheckedGet(Instance, MT2266_STATUS_2, &tmp);
         if (MT_NO_ERROR(status))
         *pValue = (U32) tmp;
         break;
         /*  BB attenuator A/D readback            */
         case MT2266_BB_ADC:
         status |= UncheckedGet(Instance, MT2266_STATUS_3, &tmp);
         if (MT_NO_ERROR(status))
         *pValue = (U32) tmp;
         break;
         /*  RF attenuator setting                 */
         case MT2266_RF_ATTN:
         PREFETCH(MT2266_RSVD_1F, 1);
         /* Fetch register(s) if __NO_CACHE__ defined */
         if (MT_NO_ERROR(status))
         *pValue = Instance->TunerRegVal[MT2266_RSVD_1F];
         break;
         /*  BB attenuator setting                 */
         case MT2266_BB_ATTN:
         PREFETCH(MT2266_RSVD_2C, 3);
         /* Fetch register(s) if __NO_CACHE__ defined */
         *pValue = Instance->TunerRegVal[MT2266_RSVD_2C] + Instance->TunerRegVal[MT2266_RSVD_2D]    + Instance->TunerRegVal[MT2266_RSVD_2E] - 3;
         break;
         /*  RF external / internal atten control  */
         case MT2266_RF_EXT:
         PREFETCH(MT2266_GPO, 1);
         /* Fetch register(s) if __NO_CACHE__ defined */
         *pValue = ((Instance->TunerRegVal[MT2266_GPO] & 0x40) != 0x00);
         break;
         /*  BB external / internal atten control  */
         case MT2266_BB_EXT:
         PREFETCH(MT2266_RSVD_33, 1);
         /* Fetch register(s) if __NO_CACHE__ defined */
         *pValue = ((Instance->TunerRegVal[MT2266_RSVD_33] & 0x10) != 0x00);
         break;
         /*  LNA gain setting (0-15)               */
         case MT2266_LNA_GAIN:            PREFETCH(MT2266_IGAIN, 1);
         /* Fetch register(s) if __NO_CACHE__ defined */
         *pValue = ((Instance->TunerRegVal[MT2266_IGAIN] & 0x3C) >> 2);

         break;
         case MT2266_EOP:

         default:
         status |= MT_ARG_RANGE;
         }
         }






  return (status);
}



/****************************************************************************
**
**  Name: MT2266_SetPowerModes
**
**  Description:    Sets the bits in the MT2266_ENABLES register and the
**                  SROsd bit in the MT2266_SROADC_CTRL register.
**
**  Parameters:     h           - Tuner handle (returned by MT2266_Open)
**                  flags       - Bit mask of flags to indicate enabled
**                                bits.
**
**  Usage:          status = MT2266_SetPowerModes(hMT2266, flags);
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   USERS MUST CALL MT2266_Open() FIRST!
**
**                  The bits in the MT2266_ENABLES register and the
**                  SROsd bit are set according to the supplied flags.
**
**                  The pre-defined flags are as follows:
**                      MT2266_SROen
**                      MT2266_LOen
**                      MT2266_ADCen
**                      MT2266_PDen
**                      MT2266_DCOCen
**                      MT2266_BBen
**                      MT2266_MIXen
**                      MT2266_LNAen
**                      MT2266_ALL_ENABLES
**                      MT2266_NO_ENABLES
**                      MT2266_SROsd
**                      MT2266_SRO_NOT_sd
**
**                  ONLY the enable bits (or SROsd bit) specified in the
**                  flags parameter will be set.  Any flag which is not
**                  included, will cause that bit to be disabled.
**
**                  The ALL_ENABLES, NO_ENABLES, and SRO_NOT_sd constants
**                  are for convenience.  The NO_ENABLES and SRO_NOT_sd
**                  do not actually have to be included, but are provided
**                  for clarity.
**
**  See Also:       MT2266_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
****************************************************************************/
U32 MT2266_SetPowerModes(TUNTDRV_InstanceData_t *Instance,
                             U32  flags)
{
    U32 status = MT_OK;                  /* Status to be returned */
/*    MT2266_Info_t* pInfo = (MT2266_Info_t*) h;*/
    U8 tmpreg;

    /*  Verify that the handle passed points to a valid tuner */
    if (IsValidHandle(Instance) == 0)
    {
        status |= MT_INV_HANDLE;
    }

    PREFETCH(MT2266_SRO_CTRL, 1);  /* Fetch register(s) if __NO_CACHE__ defined */
    if (MT_NO_ERROR(status))
    {
        if (flags & MT2266_SROsd)
        {
            tmpreg = Instance->TunerRegVal[MT2266_SRO_CTRL] |= 0x10;  /* set the SROsd bit */
        }
        else
        {
            tmpreg = Instance->TunerRegVal[MT2266_SRO_CTRL] &= 0xEF;  /* clear the SROsd bit */
        }
        status |= UncheckedSet(Instance, MT2266_SRO_CTRL, tmpreg);
    }

    PREFETCH(MT2266_ENABLES, 1);  /* Fetch register(s) if __NO_CACHE__ defined */

    if (MT_NO_ERROR(status))
    {
        status |= UncheckedSet(Instance, MT2266_ENABLES, (U8)(flags & 0xff));
    }

    return status;
}


/****************************************************************************
**  LOCAL FUNCTION - DO NOT USE OUTSIDE OF mt2266.c
**
**  Name: UncheckedSet
**
**  Description:    Sets an MT2266 register.
**
**                  NOTE: This is a local function that performs the same
**                  steps as the MT2266_SetReg function that is available
**                  in the external API.  It does not do any of the standard
**                  error checking that the API function provides and should
**                  not be called from outside this file.
**
**  Parameters:     *pInfo      - Tuner control structure
**                  reg         - MT2266 register/subaddress location
**                  val         - MT2266 register/subaddress value
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   USERS MUST CALL MT2266_Open() FIRST!
**
**                  Sets a register value without any preliminary checking for
**                  valid handles or register numbers.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
****************************************************************************/
 U32 UncheckedSet(TUNTDRV_InstanceData_t *Instance,
                            U8         reg,
                            U8         val)
{
    U32 status=ST_NO_ERROR;                  /* Status to be returned */

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

               	   	 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				   
			            status = ChipSetOneRegister(Instance->IOHandle,reg, val);
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	   	 status = STTUNER_IOREG_SetRegister(&(Instance->DeviceMap), Instance->IOHandle,reg, val);
			   	  }
			   	 #endif  

   /* status = MT_WriteSub(pInfo->hUserData, pInfo->address, reg, &val, 1);*/


    if (MT_NO_ERROR(status))
    {
        Instance->TunerRegVal[reg] = val;
    }

    return (status);
}


/****************************************************************************
**
**  Name: MT2266_SetReg
**
**  Description:    Sets an MT2266 register.
**
**  Parameters:     h           - Tuner handle (returned by MT2266_Open)
**                  reg         - MT2266 register/subaddress location
**                  val         - MT2266 register/subaddress value
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_RANGE     - Argument out of range
**
**  Dependencies:   USERS MUST CALL MT2266_Open() FIRST!
**
**                  Use this function if you need to override a default
**                  register value
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
****************************************************************************/
U32 MT2266_SetReg(TUNTDRV_InstanceData_t *Instance,
                      U8   reg,
                      U8   val)
{
    U32 status = MT_OK;                  /* Status to be returned */
    /*MT2266_Info_t* pInfo = (MT2266_Info_t*) h;*/

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
    {
        status |= MT_INV_HANDLE;
    }

    if (reg >= END_REGS)
    {
        status |= MT_ARG_RANGE;
    }

    if (MT_NO_ERROR(status))
    {
        status |= UncheckedSet(Instance, reg, val);
    }

    return (status);
}


/****************************************************************************
** LOCAL FUNCTION
**
**  Name: RoundToStep
**
**  Description:    Rounds the given frequency to the closes f_Step value
**                  given the tuner ref frequency..
**
**
**  Parameters:     freq      - Frequency to be rounded (in Hz).
**                  f_Step    - Step size for the frequency (in Hz).
**                  f_Ref     - SRO frequency (in Hz).
**
**  Returns:        Rounded frequency.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
****************************************************************************/
 U32 RoundToStep(U32 freq, U32 f_Step, U32 f_ref)
{
    return f_ref * (freq / f_ref)
        + f_Step * (((freq % f_ref) + (f_Step / 2)) / f_Step);
}


/****************************************************************************
** LOCAL FUNCTION
**
**  Name: CalcLOMult
**
**  Description:    Calculates Integer divider value and the numerator
**                  value for LO's FracN PLL.
**
**                  This function assumes that the f_LO and f_Ref are
**                  evenly divisible by f_LO_Step.
**
**  Parameters:     Div       - OUTPUT: Whole number portion of the multiplier
**                  FracN     - OUTPUT: Fractional portion of the multiplier
**                  f_LO      - desired LO frequency.
**                  denom     - LO FracN denominator value
**                  f_Ref     - SRO frequency.
**
**  Returns:        Recalculated LO frequency.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
****************************************************************************/
 U32 CalcLOMult(U32 *Div,
                          U32 *FracN,
                          U32  f_LO,
                          U32  denom,
                          U32  f_Ref)
{
    U32 a, b, i;
	const int TwoNShift = 13;   /* bits to shift to obtain 2^n qty*/
    const int RoundShift = 18;  /* bits to shift before rounding*/
    const U32 FracN_Scale = (U32)((f_Ref / (MAX_UDATA >> TwoNShift)) + 1);

    /*  Calculate the whole number portion of the divider */
    *Div = f_LO / f_Ref;

    /*
    **  Calculate the FracN numerator 1 bit at a time.  This keeps the
    **  integer values from overflowing when large values are multiplied.
    **  This loop calculates the fractional portion of F/20MHz accurate
    **  to 32 bits.  The 2^n factor is represented by the placement of
    **  the value in the 32-bit word.  Since we want as much accuracy
    **  as possible, we'll leave it at the top of the word.
    */
    *FracN = 0;
    a = f_LO;
    for (i=32; i>0; --i)
    {
        b = 2*(a % f_Ref);
        *FracN = (*FracN * 2) + (b >= f_Ref);
        a = b;
    }

    /*
    **  If the denominator is a 2^n - 1 value (the usual case) then the
    **  value we really need is (F/20) * 2^n - (F/20).  Shifting the
    **  calculated (F/20) value to the right and subtracting produces
    **  the desired result -- still accurate to 32 bits.
    */
    if ((denom & 0x01) != 0)
    {
        *FracN -= (*FracN >> TwoNShift);
     }

    /*
    ** Now shift the result so that it is 1 bit bigger than we need,
    ** use the low-order bit to round the remaining bits, and shift
    ** to make the answer the desired size.
    */
    *FracN >>= RoundShift;
    *FracN = (*FracN & 0x01) + (*FracN >> 1);

    /*  Check for rollover (cannot happen with 50 kHz step size) */
    if (*FracN == (denom | 1))
    {
        *FracN = 0;
        ++Div;
    }


    return (f_Ref * (*Div))
         + FracN_Scale * (((f_Ref / FracN_Scale) * (*FracN)) / denom);
}


/****************************************************************************
**
**  Name: MT2266_ChangeFreq
**
**  Description:    Change the tuner's tuned frequency to f_in.
**
**  Parameters:     h           - Open handle to the tuner (from MT2266_Open).
**                  f_in        - RF input center frequency (in Hz).
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_DNC_UNLOCK    - Downconverter PLL unlocked
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_FIN_RANGE     - Input freq out of range
**                      MT_DNC_RANGE     - Downconverter freq out of range
**
**  Dependencies:   MUST CALL MT2266_Open BEFORE MT2266_ChangeFreq!
**
**                  MT_ReadSub       - Read byte(s) of data from the two-wire-bus
**                  MT_WriteSub      - Write byte(s) of data to the two-wire-bus
**                  MT_Sleep         - Delay execution for x milliseconds
**                  MT2266_GetLocked - Checks to see if the PLL is locked
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
****************************************************************************/
U32 MT2266_ChangeFreq(TUNTDRV_InstanceData_t *Instance,
                          U32 f_in)     /* RF input center frequency   */
{
   /* MT2266_Info_t* pInfo = (MT2266_Info_t*) h;*/

    U32 status = MT_OK;       /*  status of operation             */
    U32 LO;                   /*  LO register value               */
    U32 Num;                  /*  Numerator for LO reg. value     */
    U32 ofLO;                 /*  last time's LO frequency        */
    U32 ofin;                 /*  last time's input frequency     */
    U8  LO_Band;              /*  LO Mode bits                    */
    U32 s_fRef;               /*  Ref Freq scaled for LO Band     */
    U32 this_band;            /*  Band for the requested freq     */
    U32 SROx2;                /*  SRO times 2                     */

    /*  Verify that the handle passed points to a valid tuner         */
    if (IsValidHandle(Instance) == 0)
    {
        return MT_INV_HANDLE;
    }

    /*
    **  Save original input and LO value
    */
    ofLO = Instance->MT2266_Info.f_LO;
    ofin = Instance->MT2266_Info.f_in;

    /*
    **  Assign in the requested input value
    */
    Instance->MT2266_Info.f_in = f_in;

    /*
    **  Get the SRO multiplier value
    */
    SROx2 = (2 - (Instance->TunerRegVal[MT2266_SRO_CTRL] & 0x01));

    /*  Request an LO that is on a step size boundary  */
    Instance->MT2266_Info.f_LO = RoundToStep(f_in, Instance->MT2266_Info.f_Step, Instance->MT2266_Info.f_Ref);
    
    if (Instance->MT2266_Info.f_LO < MIN_VHF_FREQ)
    {
        status |= MT_FIN_RANGE | MT_ARG_RANGE | MT_DNC_RANGE;
        return status;  /* Does not support frequencies below MIN_VHF_FREQ  */
    }
    else if (Instance->MT2266_Info.f_LO <= MAX_VHF_FREQ)
    {
        /*  VHF Band  */
        s_fRef = Instance->MT2266_Info.f_Ref * SROx2 / 4;
        LO_Band = 0;
        this_band = MT2266_VHF_BAND;
    }
    else if (Instance->MT2266_Info.f_LO < MIN_UHF_FREQ)
    {
        status |= MT_FIN_RANGE | MT_ARG_RANGE | MT_DNC_RANGE;
        return status;  /* Does not support frequencies between MAX_VHF_FREQ & MIN_UHF_FREQ */
    }
    else if (Instance->MT2266_Info.f_LO <= MAX_UHF_FREQ)
    {
        /*  UHF Band  */
        s_fRef = Instance->MT2266_Info.f_Ref * SROx2 / 2;
        LO_Band = 1;
        this_band = MT2266_UHF_BAND;
    }
    else
    {
        status |= MT_FIN_RANGE | MT_ARG_RANGE | MT_DNC_RANGE;
        return status;  /* Does not support frequencies above MAX_UHF_FREQ */
    }

    /*
    ** Calculate the LO frequencies and the values to be placed
    ** in the tuning registers.
    */
    Instance->MT2266_Info.f_LO = CalcLOMult(&LO, &Num, Instance->MT2266_Info.f_LO, 8191, s_fRef);

    /*
    **  If we have the same LO frequencies and we're already locked,
    **  then just return without writing any registers.
    */
    if ((ofLO == Instance->MT2266_Info.f_LO) 
        && ((Instance->TunerRegVal[MT2266_STATUS_1] & 0x40) == 0x40))
    {
        return (status);
    }

    /*
    ** Reset defaults here if we're tuning into a new band
    */
    if (MT_NO_ERROR(status))
    {
        if (this_band != Instance->MT2266_Info.band)
        {
            MT2266_DefaultsEntry *defaults = NULL;
            switch (this_band)
            {
                case MT2266_VHF_BAND:
                    defaults = &MT2266_VHF_defaults[0];
                    break;
                case MT2266_UHF_BAND:
                    defaults = &MT2266_UHF_defaults[0];
                    break;
                default:
                    status |= MT_ARG_RANGE;
            }
            if ( MT_NO_ERROR(status))
            {
                while (defaults->data && MT_NO_ERROR(status))
                {
                  /*  status |= MT_WriteSub(pInfo->hUserData, pInfo->address, defaults->data[0], &defaults->data[1], defaults->cnt);*/
                                	   	 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				    status |= ChipSetRegisters(Instance->IOHandle,defaults->data[0], &defaults->data[1], defaults->cnt);   
			
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	   status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle,defaults->data[0], &defaults->data[1], defaults->cnt);
			   	  }
			   	 #endif                        
	         
                    
                    defaults++;
                }
                /* re-read the new registers into the cached values */
                /*status |= MT_ReadSub(pInfo->hUserData, pInfo->address, 0, &pInfo->reg[0], END_REGS);*/
                	 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
                if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				    status |= ChipGetRegisters(Instance->IOHandle, 0, END_REGS,&Instance->TunerRegVal[0]); 
			
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	   status |= STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle, 0, END_REGS,&Instance->TunerRegVal[0]);
			   	  }
			   	 #endif    
                
                
                Instance->MT2266_Info.band = this_band;
            }
        }
    }

    /*
    **  Place all of the calculated values into the local tuner
    **  register fields.
    */
    if (MT_NO_ERROR(status))
    {
    	
        Instance->TunerRegVal[MT2266_LO_CTRL_1] = (U8)(Num >> 8);
        Instance->TunerRegVal[MT2266_LO_CTRL_2] = (U8)(Num & 0xFF);
        Instance->TunerRegVal[MT2266_LO_CTRL_3] = (U8)(LO & 0xFF);

        /*
        ** Now write out the computed register values
        */
        /*status |= MT_WriteSub(pInfo->hUserData, pInfo->address, MT2266_LO_CTRL_1, &pInfo->reg[MT2266_LO_CTRL_1], 3);*/
        	 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
        if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				    status |= ChipSetRegisters(Instance->IOHandle,MT2266_LO_CTRL_1, &Instance->TunerRegVal[MT2266_LO_CTRL_1], 3);
			
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	  status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle,MT2266_LO_CTRL_1, &Instance->TunerRegVal[MT2266_LO_CTRL_1], 3);
			   	  }
			   	 #endif    
                
        
         

        if (Instance->MT2266_Info.band == MT2266_UHF_BAND)
        {
            U8 CapSel = 0;                        /*  def when f_in > all    */
            U8 idx;
            U32* XFreq;
            int ClearTune_Fuse;
            int f_offset;
            U32 f_in_;

            PREFETCH(MT2266_BAND_CTRL, 2);  /* Fetch register(s) if __NO_CACHE__ defined */
            PREFETCH(MT2266_STATUS_5, 1);  /* Fetch register(s) if __NO_CACHE__ defined */

            XFreq = (Instance->TunerRegVal[MT2266_BAND_CTRL] & 0x10) ? MT2266_UHFA_XFreq : MT2266_UHF_XFreq;
            ClearTune_Fuse = Instance->TunerRegVal[MT2266_STATUS_5] & 0x07;
            f_offset = (10000000) * ((ClearTune_Fuse > 3) ? (ClearTune_Fuse - 8) : ClearTune_Fuse);
            f_in_ = (f_in - f_offset) / 1000 / TUNE_STEP_SIZE;

            for (idx=0; idx<16; ++idx)
            {
                if (XFreq[idx] >= f_in_)
                {
                    CapSel = 16 - idx;
                    break;
                }
            }
            /*  If CapSel == 16, set UBANDen and set CapSel = 15  */
            if (CapSel == 16)
            {
                Instance->TunerRegVal[MT2266_BAND_CTRL] |= 0x01;
                CapSel = 15;
            }
            else
            {
                Instance->TunerRegVal[MT2266_BAND_CTRL] &= ~(0x01);
            }

            Instance->TunerRegVal[MT2266_BAND_CTRL] =
                    (Instance->TunerRegVal[MT2266_BAND_CTRL] & 0x3F) | (LO_Band << 6);
            Instance->TunerRegVal[MT2266_CLEARTUNE] = (CapSel << 4) | CapSel;
            /*  Write UBANDsel  [05] & ClearTune [06]  */
            /*status |= MT_WriteSub(pInfo->hUserData, pInfo->address, MT2266_BAND_CTRL, &pInfo->reg[MT2266_BAND_CTRL], 2);*/
            	 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
            if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				    status |= ChipSetRegisters(Instance->IOHandle,MT2266_BAND_CTRL,&Instance->TunerRegVal[MT2266_BAND_CTRL], 2);
			
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	  status |= STTUNER_IOREG_SetContigousRegisters (&(Instance->DeviceMap),Instance->IOHandle,MT2266_BAND_CTRL,&Instance->TunerRegVal[MT2266_BAND_CTRL], 2);
			   	  }
			   	 #endif    
                
        
            
        }
    }

    /*
    **  Check for LO lock
    */
    if (MT_NO_ERROR(status))
    {
        status |= MT2266_GetLocked(Instance);
    }

    return (status);
}



/****************************



****************************/

void MT2266_pdcontrol_loop(TUNER_Handle_t *Handle, IOARCH_Handle_t IOHandle)
{

TUNTDRV_InstanceData_t *Instance;
 Instance = TUNTDRV_GetInstFromHandle(*Handle);
/*timer=STOS_time_now();
timer -= MT2266_pdcontrol_timestamp;*/


/*STOS Now timer =1548979
time timer =1548979
MT2266_pdcontrol_timestamp Now timer =0
MinCycleDelay  Now timer =0
MinCycleDelay && MT2266_pdcontrol_enable  Now timer =0i*/



MT2266_pdcontrol_execute(Instance,IOHandle);
			/*MT2266_pdcontrol_execute(DeviceMap,IOHandle);*/

}

/****************************



****************************/
void MT2266_pdcontrol_execute(TUNTDRV_InstanceData_t *Instance, IOARCH_Handle_t IOHandle)
{

U16 pd_value;
	U16 rf_level, bb_level;
	U32 lna_gain;
	
	U8 band=1;  /* band=0: vhf, band=1: uhf low, band=2: uhf high */
	U32 freq;

	U16 sens_on[] = {0x0000, 0x2100, 0x2500};
	U16 sens_off[] = {0x0001, 0x5900, 0x6200};
	U16 lin_off[] = {0x800, 0x800, 0x0800};
	U16 lin_on[] = {0x0f00, 0x0f00, 0x0eff};
	U16 pd_upper[] = {0x0172, 0x0172, 0x0172};
	U16 pd_lower[] = {0x00140, 0x0140, 0x0140};

	U32 next_state;


/*TUNTDRV_InstanceData_t *Instance;*/



    MT2266_GetParam(Instance,MT2266_INPUT_FREQ, &freq);
	MT2266_GetParam(Instance, MT2266_LNA_GAIN, &lna_gain);

    if (freq <= 250e6) band=0;
	else if (freq < 660e6) band=1;
	else band=2;

	demod_get_pd(IOHandle, &pd_value);
	demod_get_agc(IOHandle, &rf_level, &bb_level);

    rf_level=0xffff-rf_level;
	bb_level=0xffff-bb_level;

	next_state = current_state;

switch (current_state)
{
	case AGC_STATE_START :
	#ifdef STTUNER_TERR_TUNER_MT2266
    printf("\nAGC_STATE_START \n");
    #endif
		demod_set_agclim( IOHandle,0);
		if (lna_gain < LNAGAIN_MIN)
			next_state=AGC_STATE_LNAGAIN_BELOW_MIN;
		else if (lna_gain > LNAGAIN_MAX)
			next_state=AGC_STATE_LNAGAIN_ABOVE_MAX;
		else
			next_state=AGC_STATE_NORMAL;
		break;
    case AGC_STATE_LNAGAIN_BELOW_MIN :
    #ifdef STTUNER_TERR_TUNER_MT2266
    printf("\nAGC_STATE_LNAGAIN_BELOW_MIN \n");
    #endif
		if (lna_gain < LNAGAIN_MIN )
			next_state=AGC_STATE_LNAGAIN_BELOW_MIN;
		else next_state=AGC_STATE_NORMAL;

		break;

    case AGC_STATE_LNAGAIN_ABOVE_MAX :
    #ifdef STTUNER_TERR_TUNER_MT2266
    printf("\nAGC_STATE_LNAGAIN_ABOVE_MAX \n");
    #endif
		if (lna_gain > LNAGAIN_MAX )
			next_state=AGC_STATE_LNAGAIN_ABOVE_MAX;
		else next_state=AGC_STATE_NORMAL;
		break;

    case AGC_STATE_NORMAL :
    #ifdef STTUNER_TERR_TUNER_MT2266
    printf("\nAGC_STATE_NORMAL \n");
    #endif

		if (rf_level > lin_on[band] ) {
			lna_gain_old = lna_gain;
			next_state = AGC_STATE_MAS_GRANDE_SIGNAL;
			}
		else if (pd_value > pd_upper[band]) {
			next_state = AGC_STATE_GRANDE_INTERFERER;
			}
		else if ( (pd_value < pd_lower[band]) && (lna_gain < LNAGAIN_MAX) ) {
			next_state = AGC_STATE_NO_INTERFERER;
			}
		else if ( bb_level < sens_on[band]) {
			next_state = AGC_STATE_SMALL_SIGNAL;
			}
		break;
		case AGC_STATE_NO_INTERFERER :
		#ifdef STTUNER_TERR_TUNER_MT2266
		printf("\nAGC_STATE_NO_INTERFERER \n");
		#endif
		if (pd_value > pd_upper[band] )
			next_state = AGC_STATE_GRANDE_INTERFERER;
		else if (pd_value > pd_lower[band] )
			next_state = AGC_STATE_NORMAL;
		else if ( lna_gain == LNAGAIN_MAX )
			next_state = AGC_STATE_NORMAL;
		break;

    case AGC_STATE_GRANDE_INTERFERER :
    #ifdef STTUNER_TERR_TUNER_MT2266
    printf("\nAGC_STATE_GRANDE_INTERFERER \n");
    #endif
		if (pd_value < pd_lower[band] )
			next_state = AGC_STATE_NO_INTERFERER;

		break;
	case AGC_STATE_MAS_GRANDE_SIGNAL :
	#ifdef STTUNER_TERR_TUNER_MT2266
	printf("\nAGC_STATE_MAS_GRANDE_SIGNAL \n");
	#endif
		if (rf_level < lin_on[band])
			next_state = AGC_STATE_GRANDE_SIGNAL;
		else if (pd_value > pd_upper[band])
			next_state = AGC_STATE_GRANDE_INTERFERER;
		break;

    case AGC_STATE_MEDIUM_SIGNAL :
    #ifdef STTUNER_TERR_TUNER_MT2266
    printf("\nAGC_STATE_MEDIUM_SIGNAL \n");
    #endif
		if (rf_level > lin_off[band])
			next_state = AGC_STATE_GRANDE_SIGNAL;
		else if (lna_gain >= lna_gain_old)
			next_state = AGC_STATE_NORMAL;
		else if (pd_value > pd_upper[band])
			next_state = AGC_STATE_GRANDE_INTERFERER;
		break;

    case AGC_STATE_GRANDE_SIGNAL :
    #ifdef STTUNER_TERR_TUNER_MT2266
    printf("\nAGC_STATE_GRANDE_SIGNAL \n");
    #endif
		if (rf_level > lin_on[band])
			next_state = AGC_STATE_MAS_GRANDE_SIGNAL;
		else if (rf_level < lin_off[band])
			next_state = AGC_STATE_MEDIUM_SIGNAL;
		else if (pd_value > pd_upper[band])
			next_state = AGC_STATE_GRANDE_INTERFERER;

		break;
	case AGC_STATE_SMALL_SIGNAL :
	#ifdef STTUNER_TERR_TUNER_MT2266
	printf("\nAGC_STATE_SMALL_SIGNAL \n");
	#endif
		if (pd_value > pd_upper[band])
			next_state = AGC_STATE_GRANDE_INTERFERER;
		else if (bb_level > sens_off[band])
			next_state = AGC_STATE_NORMAL;
		else if ( (bb_level < sens_on[band]) && (lna_gain == LNAGAIN_MAX) )
			next_state = AGC_STATE_MAX_SENSITIVITY;
		break;

    case AGC_STATE_MAX_SENSITIVITY :
    #ifdef STTUNER_TERR_TUNER_MT2266
    printf("\n AGC_STATE_MAX_SENSITIVITY \n");
    #endif
		if (bb_level > sens_off[band])
			next_state = AGC_STATE_SMALL_SIGNAL;
		break;
}


current_state = next_state;


switch (current_state) {

		case AGC_STATE_LNAGAIN_BELOW_MIN :
		#ifdef STTUNER_TERR_TUNER_MT2266
		 printf("\n AGC_STATE_LNAGAIN_BELOW_MIN \n");
		 #endif
			MT2266_SetParam(/*this is tuner instance*/Instance,MT2266_LNA_GAIN_INCR, LNAGAIN_MAX);
			break;


		case AGC_STATE_LNAGAIN_ABOVE_MAX :
		#ifdef STTUNER_TERR_TUNER_MT2266
		 printf("\n AGC_STATE_LNAGAIN_ABOVE_MAX \n");
		 #endif
			MT2266_SetParam(Instance,MT2266_LNA_GAIN_DECR, LNAGAIN_MIN);
			break;


		case AGC_STATE_NORMAL :
		#ifdef STTUNER_TERR_TUNER_MT2266
		 printf("\n AGC_STATE_NORMAL \n");
		 #endif
			demod_set_agclim(IOHandle,0);
			break;


		case AGC_STATE_NO_INTERFERER :
		#ifdef STTUNER_TERR_TUNER_MT2266
		 printf("\n AGC_STATE_NO_INTERFERER \n");
		 #endif
			MT2266_SetParam(Instance,MT2266_LNA_GAIN_INCR, LNAGAIN_MAX);
			demod_set_agclim(IOHandle,0);
			break;


		case AGC_STATE_GRANDE_INTERFERER :
		#ifdef STTUNER_TERR_TUNER_MT2266
		 printf("\n AGC_STATE_GRANDE_INTERFERER \n");
		 #endif
			MT2266_SetParam(Instance,MT2266_LNA_GAIN_DECR, LNAGAIN_MIN);
			demod_set_agclim(IOHandle,1);
			break;


		case AGC_STATE_MEDIUM_SIGNAL :
		#ifdef STTUNER_TERR_TUNER_MT2266
		 printf("\n AGC_STATE_MEDIUM_SIGNAL \n");
		 #endif
			MT2266_SetParam(Instance,MT2266_LNA_GAIN_INCR, LNAGAIN_MAX);
			demod_set_agclim(IOHandle,0);
			break;


		case AGC_STATE_GRANDE_SIGNAL :
		#ifdef STTUNER_TERR_TUNER_MT2266
		 printf("\n AGC_STATE_GRANDE_SIGNAL \n");
		 #endif
			break;


		case AGC_STATE_MAS_GRANDE_SIGNAL :
		#ifdef STTUNER_TERR_TUNER_MT2266
		 printf("\n AGC_STATE_MAS_GRANDE_SIGNAL \n");
		 #endif
			MT2266_SetParam(Instance,MT2266_LNA_GAIN_DECR, LNAGAIN_MIN);
			demod_set_agclim(IOHandle,0);
			break;


		case AGC_STATE_SMALL_SIGNAL :
		#ifdef STTUNER_TERR_TUNER_MT2266
		 printf("\n AGC_STATE_SMALL_SIGNAL \n");
		 #endif
			MT2266_SetParam(Instance,MT2266_LNA_GAIN_INCR, LNAGAIN_MAX);
			MT2266_SetParam(Instance,MT2266_UHF_NORMAL,1);
			demod_set_agclim(IOHandle,0);
			Instance->MT2266_Info.UHFSens=0;
			break;


		case AGC_STATE_MAX_SENSITIVITY :
		#ifdef STTUNER_TERR_TUNER_MT2266
		 printf("\n AGC_STATE_MAX_SENSITIVITY \n");
		 #endif
			MT2266_SetParam(Instance,MT2266_UHF_MAXSENS,1);
			demod_set_agclim(IOHandle,0);
			Instance->MT2266_Info.UHFSens=1;
			break;

	}

	MT2266_GetParam(Instance, MT2266_LNA_GAIN,/*&Instance->MT2266_Info.LNAConfig*/&lna_gain_old);

/*printf("lna old gain  =%d \n",lna_gain_old);*/


}


void demod_get_pd( IOARCH_Handle_t IOHandle, unsigned short* level)
{

	*level = ChipGetField(IOHandle, F0362_RF_AGC1_LEVEL_HI);
	*level = *level << 2;
	*level |= (ChipGetField(IOHandle, F0362_RF_AGC1_LEVEL_LO)&0x03);

}



void demod_get_agc(IOARCH_Handle_t IOHandle, U16* rf_agc, U16* bb_agc)
{
	U16 rf_low, rf_high, bb_low, bb_high;

	rf_low = ChipGetOneRegister(IOHandle, R0362_AGC1VAL1);
	rf_high = ChipGetOneRegister(IOHandle, R0362_AGC1VAL2);

	rf_high <<= 8;
	rf_low &= 0x00ff;
	*rf_agc = (rf_high+rf_low)<<4;

	bb_low = ChipGetOneRegister(IOHandle, R0362_AGC2VAL1);
	bb_high = ChipGetOneRegister(IOHandle, R0362_AGC2VAL2);

	bb_high <<= 8;
	bb_low &= 0x00ff;
	*bb_agc = (bb_high+bb_low)<<4;




}

void demod_set_agclim(IOARCH_Handle_t IOHandle, U16 dir_up)
{
	U8 agc_min=0;

	agc_min = ChipGetOneRegister(IOHandle, R0362_AGC2MIN);

	if (bbagc_min_start==0xffff)
		bbagc_min_start=agc_min;


	if (dir_up) {
		if ((agc_min >= bbagc_min_start) && (agc_min<=(0xa4-0x04)) )
			agc_min += 0x04;

	    }
	else {
		if ((agc_min >= (bbagc_min_start+0x04)) && (agc_min<=0xa4) )
			agc_min -= 0x04;
	    }


	ChipSetOneRegister(IOHandle,R0362_AGC2MIN,agc_min);
}







