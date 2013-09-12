/*****************************************************************************
**
**  Name: mt2131.h
**
**  Description:    Microtune MT2131 BX Tuner software interface 
**                  Supports tuners with Part/Rev codes: 0x3E & 0x3F.
**
**  Functions
**  Implemented:    UData_t  MT2131_Open
**                  UData_t  MT2131_Close
**                  UData_t  MT2131_ChangeFreq
**                  UData_t  MT2131_GetGPO
**                  UData_t  MT2131_GetLocked
**                  UData_t  MT2131_GetParam
**                  UData_t  MT2131_GetReg
**                  UData_t  MT2131_GetTemp
**                  UData_t  MT2131_GetUserData
**                  UData_t  MT2131_ReInit
**                  UData_t  MT2131_ResetAGC
**                  UData_t  MT2131_SetExtSRO
**                  UData_t  MT2131_SetGPO
**                  UData_t  MT2131_SetParam
**                  UData_t  MT2131_SetPowerBits
**                  UData_t  MT2131_SetReg
**                  UData_t  MT2131_WaitForAGC
**
**  References:     AN-00075: MT2131 Programming Procedures Application Note
**                  MicroTune, Inc.
**                  AN-00010: MicroTuner Serial Interface Application Note
**                  MicroTune, Inc.
**
**  Enumerations
**  Defined:        MT2131_Ext_SRO
**                  MT2131_LNA_Mode
**                  MT2131_Param
**                  MT2131_Pwr_Bits
**                  MT2131_Rcvr_Mode
**                  MT2131_Temperature
**                  MT2131_VGA_Mode
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
**  CVS ID:         $Id: mt2131.h,v 1.1 2007/02/20 17:53:41 thomas Exp $
**  CVS Source:     $Source: /mnt/pinz/cvsroot/stgui/stv0362/Stb_lla_generic/lla/mt2131.h,v $
**	               
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**   N/A   09-29-2006    DAD    Ver 1.12: Added MT2131_VGA_GAIN_MODE and
**                                        MT2131_LNAGAIN_MODE.
**   N/A   10-06-2006    DAD    Ver 1.13: Removed MT2131_SetUserData.
**   N/A   10-06-2006    DAD    Ver 1.13: Adjusted MT2131_Temperature.
**   N/A   10-10-2006    DAD    Ver 1.14: Added MT2131_DIGITAL_CABLE_2.
**   N/A   11-27-2006    DAD    Ver 1.17: Changed f_outbw parameter to no
**                              longer include the guard band.  Now f_outbw
**                              is equivalent to the actual channel bandwidth
**                              (i.e. 6, 7, or 8 MHz).
**
*****************************************************************************/
#if !defined( __MT2131_H )
#define __MT2131_H



#if defined( __cplusplus )     
extern "C"                     /* Use "C" external linkage                  */
{
#endif

/*
**  Values returned by the MT2131's on-chip temperature sensor
**  to be read/written.
*/




/*
**  Two-wire serial bus subaddresses of the tuner registers.
**  Also known as the tuner's register addresses.
*/



/*
** Constants used by the tuning algorithm
*/
#define REF_FREQ          (16000000)  /* Reference oscillator Frequency (in Hz) */
#define IF1_FREQ        (1217500000)  /* Default IF1 Frequency (in Hz) */
#define IF1_CENTER      (1218000000)  /* Center of the IF1 filter (in Hz) */
#define IF1_BW            (18000000)  /* The IF1 filter bandwidth (in Hz) */
#define TUNE_STEP_SIZE       (50000)  /* Tune in steps of 50 kHz */
#define SPUR_STEP_HZ        (250000)  /* Step size (in Hz) to move IF1 when avoiding spurs */
#define LO1_STEP_SIZE       (250000)  /* Step size (in Hz) of PLL1 */
#define ZIF_BW             (2000000)  /* Zero-IF spur-free bandwidth (in Hz) */
#define MAX_HARMONICS_1         (15)  /* Highest intra-tuner LO Spur Harmonic to be avoided */
#define MAX_HARMONICS_2          (7)  /* Highest inter-tuner LO Spur Harmonic to be avoided */
#define MIN_LO_SEP         (1000000)  /* Minimum inter-tuner LO frequency separation */
#define LO1_FRACN_AVOID     (749999)  /* LO1 FracN numerator avoid region (in Hz) */
#define LO2_FRACN_AVOID     (274999)  /* LO2 FracN numerator avoid region (in Hz) */
#define MIN_FIN_FREQ      (48000000)  /* Minimum input frequency (in Hz) */
#define MAX_FIN_FREQ    (1000000000)  /* Maximum input frequency (in Hz) */
#define MIN_FOUT_FREQ     (30000000)  /* Minimum output frequency (in Hz) */
#define MAX_FOUT_FREQ     (57000000)  /* Maximum output frequency (in Hz) */
#define MIN_DNC_FREQ    (1140000000)  /* Minimum LO2 frequency (in Hz) */
#define MAX_DNC_FREQ    (1220000000)  /* Maximum LO2 frequency (in Hz) */
#define MIN_UPC_FREQ    (1225000000)  /* Minimum LO1 frequency (in Hz) */
#define MAX_UPC_FREQ    (227000000/*2270000000*/)  /* Maximum LO1 frequency (in Hz) *//*for removing warning in C90*/

/*
**  Define the supported Part/Rev codes for the MT2131
*/
#define MT2131_B3       (0x3E)
#define MT2131_B4       (0x3F)

#define MT_ERROR (1 << 31)
#define MT_USER_ERROR (1 << 30)

/*  Macro to be used to check for errors  */
#define MT_IS_ERROR(s) (((s) >> 30) != 0)
#define MT_NO_ERROR(s) (((s) >> 30) == 0)

#define MT_OK                           (0x00000000)
/*  Unknown error  */
#define MT_UNKNOWN                      (0x80000001)

/*  Error:  Upconverter PLL is not locked  */
#define MT_UPC_UNLOCK                   (0x80000002)

/*  Error:  Downconverter PLL is not locked  */
#define MT_DNC_UNLOCK                   (0x80000004)

/*  Error:  Two-wire serial bus communications error  */
#define MT_COMM_ERR                     (0x80000008)

/*  Error:  Tuner handle passed to function was invalid  */
#define MT_INV_HANDLE                   (0x80000010)

/*  Error:  Function argument is invalid (out of range)  */
#define MT_ARG_RANGE                    (0x80000020)

/*  Error:  Function argument (ptr to return value) was NULL  */
#define MT_ARG_NULL                     (0x80000040)

/*  Error: Attempt to open more than MT_TUNER_CNT tuners  */
#define MT_TUNER_CNT_ERR                (0x80000080)

/*  Error: Tuner Part Code / Rev Code mismatches expected value  */
#define MT_TUNER_ID_ERR                 (0x80000100)

/*  Error: Tuner Initialization failure  */
#define MT_TUNER_INIT_ERR               (0x80000200)
#define MT_TUNER_TIMEOUT                (0x00400000)
/*  User-definable fields (see MT_Userdef.h)  */
#define MT_USER_DEFINED1                (0x00001000)
#define MT_USER_DEFINED2                (0x00002000)
#define MT_USER_DEFINED3                (0x00004000)
#define MT_USER_DEFINED4                (0x00008000)
#define MT_USER_MASK                    (0x4000f000)
#define MT_USER_SHIFT                   (12)

/*  Info: Mask of bits used for # of LO-related spurs that were avoided during tuning  */
#define MT_SPUR_CNT_MASK                (0x001f0000)
#define MT_SPUR_SHIFT                   (16)

/*  Info: Unavoidable LO-related spur may be present in the output  */
#define MT_SPUR_PRESENT                 (0x00800000)

/*  Info: Tuner input frequency is out of range */
#define MT_FIN_RANGE                    (0x01000000)

/*  Info: Tuner output frequency is out of range */
#define MT_FOUT_RANGE                   (0x02000000)

/*  Info: Upconverter frequency is out of range (may be reason for MT_UPC_UNLOCK) */
#define MT_UPC_RANGE                    (0x04000000)

/*  Info: Downconverter frequency is out of range (may be reason for MT_DPC_UNLOCK) */
#define MT_DNC_RANGE                    (0x08000000)


#define MT2131_CNT  (1)    

#define MT_TUNER_CNT               (1)  /*  total num of MicroTuner tuners  */
#define MAX_UDATA         	0xFFFFFFFF/*(4294967295)*//*for removing warning*/  /*  max value storable in UData_t   */



/*
**  Parameter for function MT2131_SetExtSRO that specifies the external
**  SRO drive frequency.
**
**  MT2131_EXT_SRO_OFF is the power-up default value.
*/
typedef enum
{
    MT2131_EXT_SRO_OFF,                 /*  External SRO drive off          */
    MT2131_EXT_SRO_BY_4,                /*  External SRO drive divide by 4  */
    MT2131_EXT_SRO_BY_2,                /*  External SRO drive divide by 2  */
    MT2131_EXT_SRO_BY_1                 /*  External SRO drive divide by 1  */
} MT2131_Ext_SRO;


/*
**  Parameter for function MT2131_SetParam(h, MT2131_RCVR_MODE, mode) that
**  specifies the type of signal the MT2131 is receiving.  The MT2131's
**  internal gain distribution is customized based on the expected RF 
**  spectrum as specified by this parameter.
**
**  MT2131_DIGITAL_CABLE is the initialized default value.
*/
typedef enum
{
    MT2131_ANALOG_OFFAIR,               /*  Antenna (NTSC, PAL, etc.)       */
    MT2131_ANALOG_CABLE,                /*  Cable (NTSC, PAL, etc.)         */
    MT2131_DIGITAL_OFFAIR,              /*  Antenna (8-VSB)                 */
    MT2131_DIGITAL_CABLE,               /*  Cable (64-QAM, 256-QAM, etc.)   */
    MT2131_ANALOG_OFFAIR_2,             /*  Antenna 2 (SECAM etc.)          */
    MT2131_ANALOG_CABLE_2,              /*  Cable 2 (SECAM etc.)            */
    MT2131_DIGITAL_OFFAIR_2,            /*  Antenna 2 (ISDB-T)              */
    MT2131_DIGITAL_CABLE_2             /*  Cable (ISDB-T)                  */
} MT2131_Rcvr_Mode;


/*
**  Parameter for function MT2131_SetPowerBits that specifies the power down
**  of various sections of the MT2131.
**
**  0xF2 is the power-up default value.
*/
typedef enum
{
    MT2131_PWR_AGCen = 0x80,      /*  Enable automatic RF AGC         */
    MT2131_PWR_UPCen = 0x40,      /*  Enable upconverter              */
    MT2131_PWR_DNCen = 0x20,      /*  Enable downconverter            */
    MT2131_PWR_VGAen = 0x10,      /*  Enable VGA                      */
    MT2131_PWR_FDCen = 0x08,      /*  Enable FDC amplifier            */
    MT2131_PWR_AFCen = 0x04,      /*  Enable AFC A/D                  */
    MT2131_PWR_SROen = 0x02,      /*  Enable SRO                      */
    MT2131_PWR_ALL   = 0xFE,      /*  Enable all                      */
    MT2131_PWR_NONE  = 0x00       /*  Disable all                     */
} MT2131_Pwr_Bits;


/*
**  Parameter for function MT2131_SetParam (MT2131_VGA_GAIN_STATE) that
**  specifies the VGA gain mode.
**
**  MT2131_VGA_AUTO is the default value and is set by the level of the
**  IF AGC voltage after tuning.
*/
typedef enum
{
    MT2131_VGA_FIXED_LOW,         /*  Force VGA into low gain range   */
    MT2131_VGA_FIXED_HIGH,        /*  Force VGA into high gain range  */
    MT2131_VGA_AUTO             /*  Use automatic VGA gain range    */
} MT2131_VGA_Mode;

/*
**  Parameter for function MT2131_SetParam (MT2131_LNAGAIN_MODE) that
**  specifies the LNA gain mode.
**
**  MT2131_LNA_AUTO is the default value.
*/
typedef enum
{
    MT2131_LNA_FIXED_0p0,         /*  Force LNAGain to -0.0 dB gain   */
    MT2131_LNA_FIXED_1p6,         /*  Force LNAGain to -1.6 dB gain   */
    MT2131_LNA_FIXED_2p7,         /*  Force LNAGain to -2.7 dB gain   */
    MT2131_LNA_FIXED_3p7,         /*  Force LNAGain to -3.7 dB gain   */
    MT2131_LNA_AUTO             /*  Use automatic LNA gain range    */
} MT2131_LNA_Mode;

/*
**  Parameter for function MT2131_GetParam & MT2131_SetParam that 
**  specifies the tuning algorithm parameter to be read/written.
*/
typedef enum
{
    /*  tuner address                                  set by MT2131_Open() */
    MT2131_IC_ADDR,

    /*  max number of MT2131 tuners     set by MT_TUNER_CNT in mt_userdef.h */
    MT2131_MAX_OPEN,

    /*  current number of open MT2131 tuners           set by MT2131_Open() */
    MT2131_NUM_OPEN,
    
    /*  crystal frequency                            (default: 16000000 Hz) */
    MT2131_SRO_FREQ,

    /*  Receiver Mode                       (default: MT2131_DIGITAL_CABLE) */
    MT2131_RCVR_MODE,

    /*  R-Load Maximum Value                                (default: 127) */
    MT2131_RLMAX,

    /*  Down Converter Maximum Value                           (default: 0) */
    MT2131_DNCMAX,

    /*  AGC Bleed Current                                      (default: 4) */
    MT2131_AGCBLD,

    /*  Power Detector #1 Level                                (default: 4) */
    MT2131_PD1LEV,

    /*  Power Detector #2 Level                                (default: 4) */
    MT2131_PD2LEV,

    /*  LNA Gain Level                                         (default: 0) */
    MT2131_LNAGAIN,

    /*  min tuning step size                            (default: 50000 Hz) */
    MT2131_STEPSIZE,

    /*  input center frequency             parameter of MT2131_ChangeFreq() */
    MT2131_INPUT_FREQ,

    /*  LO1 Frequency                            set by MT2131_ChangeFreq() */
    MT2131_LO1_FREQ,
              
    /*  LO1 minimum step size                          (default: 250000 Hz) */
    MT2131_LO1_STEPSIZE,
    
    /*  LO1 FracN keep-out region                      (default: 999999 Hz) */
    MT2131_LO1_FRACN_AVOID,

    /*  Current 1st IF in use                    set by MT2131_ChangeFreq() */
    MT2131_IF1_ACTUAL,
          
    /*  Requested 1st IF                         set by MT2131_ChangeFreq() */
    MT2131_IF1_REQUEST,
                
    /*  Center of 1st IF SAW filter                (default: 1218000000 Hz) */
    MT2131_IF1_CENTER,
                    
    /*  Bandwidth of 1st IF SAW filter               (default: 20000000 Hz) */
    MT2131_IF1_BW,
                    
    /*  zero-IF bandwidth                             (default: 2000000 Hz) */
    MT2131_ZIF_BW,

    /*  LO2 Frequency                            set by MT2131_ChangeFreq() */
    MT2131_LO2_FREQ,

    /*  LO2 minimum step size                           (default: 50000 Hz) */
    MT2131_LO2_STEPSIZE,

    /*  LO2 FracN keep-out region                      (default: 374999 Hz) */
    MT2131_LO2_FRACN_AVOID,

    /*  output center frequency                      (default: 44000000 Hz) */
    MT2131_OUTPUT_FREQ,

    /*  output bandwidth                   parameter of MT2131_ChangeFreq() */
    MT2131_OUTPUT_BW,

    /*  min inter-tuner LO separation                 (default: 1000000 Hz) */
    MT2131_LO_SEPARATION,

    /*  ID of avoid-spurs algorithm in use            compile-time constant */
    MT2131_AS_ALG,

    /*  max # of intra-tuner harmonics                        (default: 15) */
    MT2131_MAX_HARM1,

    /*  max # of inter-tuner harmonics                         (default: 7) */
    MT2131_MAX_HARM2,

    /*  # of 1st IF exclusion zones used         set by MT2131_ChangeFreq() */
    MT2131_EXCL_ZONES,
                 
    /*  # of spurs found/avoided                 set by MT2131_ChangeFreq() */
    MT2131_NUM_SPURS,
          
    /*  >0 spurs avoided                         set by MT2131_ChangeFreq() */
    MT2131_SPUR_AVOIDED,
              
    /*  >0 spurs in output (mathematically)      set by MT2131_ChangeFreq() */
    MT2131_SPUR_PRESENT,

    /*  set mode of VGA amplifier                (default: MT2131_VGA_AUTO) */
    MT2131_VGA_GAIN_MODE,

    /*  set mode of LNA amplifier                (default: MT2131_LNA_AUTO) */
    MT2131_LNAGAIN_MODE,

    MT2131_EOP                    /*  last entry in enumerated list         */
} MT2131_Param;

/* ====== Functions which are declared in MT2131.c File ======= */


/****************************************************************************
**
**  Name: MT2131_ChangeFreq
**
**  Description:    Change the tuner's tuned frequency to f_in.
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
**  Dependencies:   MUST CALL MT2131_Open BEFORE MT2131_ChangeFreq!
**
**                  MT_ReadSub   - Read data from the two-wire serial bus
**                  MT_WriteSub  - Write data to the two-wire serial bus
**                  MT_Sleep     - Delay execution for x milliseconds
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**   N/A   11-27-2006    DAD    Ver 1.17: Changed f_outbw parameter to no
**                              longer include the guard band.  Now f_outbw
**                              is equivalent to the actual channel bandwidth
**                              (i.e. 6, 7, or 8 MHz).
**
****************************************************************************/
U32 MT2131_ChangeFreq(TUNSHDRV_InstanceData_t *Instance,
                          U32 f_in,     /* RF input center frequency   */
                          U32 outbw);   /* IF output channel bandwidth */





/****************************************************************************
**
**  Name: MT2131_GetLocked
**
**  Description:    Checks to see if LO1 and LO2 are locked.
**
**  Parameters:     h            - Open handle to the tuner (from MT2131_Open).
**
**  Usage:          status = MT2131_GetLocked(hMT2131);
**                  if (status & (MT_UPC_UNLOCK | MT_DNC_UNLOCK))
**                      //  error!, one or more PLL's unlocked
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
**   N/A   04-19-2006    DAD    Original Release.
**
****************************************************************************/
BOOL MT2131_GetLocked(TUNSHDRV_InstanceData_t *Instance);


/****************************************************************************
**
**  Name: MT2131_GetParam
**
**  Description:    Gets a tuning algorithm parameter.
**
**                  This function provides access to the internals of the
**                  tuning algorithm - mostly for testing purposes.
**
**  Parameters:     h           - Tuner handle (returned by MT2131_Open)
**                  param       - Tuning algorithm parameter 
**                                (see enum MT2131_Param)
**                  pValue      - ptr to returned value
**
**                  param                   Description
**                  ----------------------  --------------------------------
**                  MT2131_IC_ADDR          Serial Bus address of this tuner
**                  MT2131_MAX_OPEN         Max # of MT2131's allowed open
**                  MT2131_NUM_OPEN         # of MT2131's open                  
**                  MT2131_SRO_FREQ         crystal frequency                   
**                  MT2131_RLMAX            R-Load Maximum Value
**                  MT2131_DNCMAX           Down Converter Maximum Value
**                  MT2131_AGCBLD           AGC Bleed Current
**                  MT2131_PD1LEV           Power Detector #1 target level
**                  MT2131_PD2LEV           Power Detector #2 target level
**                  MT2131_LNAGAIN          LNA Gain Level
**                  MT2131_STEPSIZE         minimum tuning step size            
**                  MT2131_INPUT_FREQ       input center frequency              
**                  MT2131_LO1_FREQ         LO1 Frequency                       
**                  MT2131_LO1_STEPSIZE     LO1 minimum step size               
**                  MT2131_LO1_FRACN_AVOID  LO1 FracN keep-out region           
**                  MT2131_IF1_ACTUAL       Current 1st IF in use               
**                  MT2131_IF1_REQUEST      Requested 1st IF                    
**                  MT2131_IF1_CENTER       Center of 1st IF SAW filter         
**                  MT2131_IF1_BW           Bandwidth of 1st IF SAW filter      
**                  MT2131_ZIF_BW           zero-IF bandwidth                   
**                  MT2131_LO2_FREQ         LO2 Frequency                       
**                  MT2131_LO2_STEPSIZE     LO2 minimum step size               
**                  MT2131_LO2_FRACN_AVOID  LO2 FracN keep-out region           
**                  MT2131_OUTPUT_FREQ      output center frequency             
**                  MT2131_OUTPUT_BW        output bandwidth                    
**                  MT2131_LO_SEPARATION    min inter-tuner LO separation       
**                  MT2131_AS_ALG           ID of avoid-spurs algorithm in use  
**                  MT2131_MAX_HARM1        max # of intra-tuner harmonics      
**                  MT2131_MAX_HARM2        max # of inter-tuner harmonics      
**                  MT2131_EXCL_ZONES       # of 1st IF exclusion zones         
**                  MT2131_NUM_SPURS        # of spurs found/avoided            
**                  MT2131_SPUR_AVOIDED     >0 spurs avoided                    
**                  MT2131_SPUR_PRESENT     >0 spurs in output (mathematically) 
**                  MT2131_RCVR_MODE        Receiver Mode
**                  MT2131_VGA_GAIN_MODE    VGA Gain mode
**                  MT2131_LNAGAIN_MODE     LNA Gain mode
**
**  Usage:          status |= MT2131_GetParam(hMT2131, 
**                                            MT2131_IF1_ACTUAL,
**                                            &f_IF1_Actual);
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_ARG_NULL      - Null pointer argument passed
**                      MT_ARG_RANGE     - Invalid parameter requested
**
**  Dependencies:   USERS MUST CALL MT2131_Open() FIRST!
**
**  See Also:       MT2131_SetParam, MT2131_Open
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**   N/A   09-29-2006    DAD    Ver 1.12: Added MT2131_VGA_GAIN_MODE and
**                                        MT2131_LNAGAIN_MODE.
**
****************************************************************************/
U32 MT2131_GetParam(TUNSHDRV_InstanceData_t *Instance,
                        MT2131_Param param,
                        U32*     pValue);





/******************************************************************************
**
**  Name: MT2131_OptimizeAGC
**
**  Description:    Optimize the MT2131 AGC settings based on signal conditions.
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
**
******************************************************************************/
U32 MT2131_OptimizeAGC(TUNSHDRV_InstanceData_t *Instance);


/******************************************************************************
**
**  Name: MT2131_ReInit
**
**  Description:    Initialize the tuner's register values.
**
**  Parameters:     h           - Tuner handle (returned by MT2131_Open)
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_TUNER_ID_ERR   - Tuner Part/Rev code mismatch
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_COMM_ERR      - Serial bus communications error
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**
******************************************************************************/
U32 MT2131_ReInit(TUNSHDRV_InstanceData_t *Instance);


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
**   N/A   08-21-2006    DAD    1.07: Separate the reset and wait operations
**                                    into two functions.
**
******************************************************************************/
U32 MT2131_ResetAGC(TUNSHDRV_InstanceData_t *Instance);


/****************************************************************************
**
**  Name: MT2131_SetExtSRO
**
**  Description:    Sets the external SRO driver.
**
**  Parameters:     h           - Tuner handle (returned by MT2131_Open)
**                  Ext_SRO_Setting - external SRO drive setting
**
**       (default)    MT2131_EXT_SRO_OFF  - ext driver off
**                    MT2131_EXT_SRO_BY_1 - ext driver = SRO frequency
**                    MT2131_EXT_SRO_BY_2 - ext driver = SRO/2 frequency
**                    MT2131_EXT_SRO_BY_4 - ext driver = SRO/4 frequency
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_COMM_ERR      - Serial bus communications error
**                      MT_INV_HANDLE    - Invalid tuner handle
**
**  Dependencies:   USERS MUST CALL MT2131_Open() FIRST!
**
**                  The Ext_SRO_Setting settings default to OFF
**                  Use this function if you need to override the default
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**
****************************************************************************/
U32 MT2131_SetExtSRO(TUNSHDRV_InstanceData_t *Instance,
                         MT2131_Ext_SRO Ext_SRO_Setting);




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
**                  MT2131_OUTPUT_BW        output bandwidth                    
**                  MT2131_LO_SEPARATION    min inter-tuner LO separation       
**                  MT2131_MAX_HARM1        max # of intra-tuner harmonics      
**                  MT2131_MAX_HARM2        max # of inter-tuner harmonics      
**                  MT2131_VGA_GAIN_MODE    VGA Gain mode
**                  MT2131_LNAGAIN_MODE     LNA Gain mode
**
**  Usage:          status |= MT2131_SetParam(hMT2131, 
**                                            MT2131_STEPSIZE,
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
**
****************************************************************************/
U32 MT2131_SetParam(TUNSHDRV_InstanceData_t *Instance,
                        MT2131_Param param,
                        U32      nValue);


/****************************************************************************
**
**  Name: MT2131_SetPowerBits
**
**  Description:    Sets the power-down of various sections of the MT2131
**
**  Parameters:     h           - Tuner handle (returned by MT2131_Open)
**                  Pwr_Enable_Setting - power enable settings for the
**                                       MT2131.
**
**                    MT2131_PWR_AGCen    Enable automatic RF AGC
**                    MT2131_PWR_UPCen    Enable upconverter
**                    MT2131_PWR_DNCen    Enable downconverter
**                    MT2131_PWR_VGAen    Enable VGA
**                    MT2131_PWR_FDCen    Enable FDC amplifier
**                    MT2131_PWR_AFCen    Enable AFC A/D
**                    MT2131_PWR_ALL      Enable all
**                    MT2131_PWR_NONE     Disable all
**
**  Usage:
**                  //  Enable all sections
**                  status |= MT2131_SetPowerBits(hMT2131, 
**                                                MT2131_PWR_ALL);
**
**                  //  Disable only the AFC A/D
**                  status |= MT2131_SetPowerBits(hMT2131, 
**                                                MT2131_PWR_ALL & ~MT2131_PWR_AFCen);
**
**                  //  Disable all sections
**                  status |= MT2131_SetPowerBits(hMT2131, MT2131_PWR_NONE);
**
**  Returns:        status:
**                      MT_OK            - No errors
**                      MT_INV_HANDLE    - Invalid tuner handle
**                      MT_COMM_ERR      - Serial bus communications error
**
**  Dependencies:   USERS MUST CALL MT2131_Open() FIRST!
**
**                  The power enable settings default to ON
**                  Use this function if you need to override the default
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-19-2006    DAD    Original Release.
**
****************************************************************************/
U32 MT2131_SetPowerBits(TUNSHDRV_InstanceData_t *Instance,
                            MT2131_Pwr_Bits Pwr_Enable_Setting);




/******************************************************************************
**
**  Name: MT2131_WaitForAGC
**
**  Description:    Wait for the MT2131 internal RF AGC to settle.
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
**  Dependencies:   USERS MUST CALL MT2131_Open() FIRST!
**
**                  MT_ReadSub  - Read byte(s) of data from the two-wire bus
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
**
******************************************************************************/
U32 MT2131_WaitForAGC(TUNSHDRV_InstanceData_t *Instance);

/*static U32 MT2131_SetLNAGain(TUNTDRV_InstanceData_t *Instance, U32 lower_limit, U32 upper_limit)*/


#if defined( __cplusplus )
}
#endif

#endif
