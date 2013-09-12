/*****************************************************************************
**
**  Name: mt2266.h
**
**  Description:    Microtune MT2266 Tuner software interface.
**                  Supports tuners with Part/Rev code: 0x85.
**
**  Functions
**  Implemented:    UData_t  MT2266_Open
**                  UData_t  MT2266_Close
**                  UData_t  MT2266_ChangeFreq
**                  UData_t  MT2266_GetLocked
**                  UData_t  MT2266_GetParam
**                  UData_t  MT2266_GetReg
**                  UData_t  MT2266_GetUserData
**                  UData_t  MT2266_ReInit
**                  UData_t  MT2266_SetParam
**                  UData_t  MT2266_SetPowerModes
**                  UData_t  MT2266_SetReg
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
**  CVS ID:         $Id: mt2266.h,v 1.2 2006/05/30 21:28:32 dessert Exp $
**  CVS Source:     $Source: /mnt/doc_software/scm/VC++/mtdrivers/mt2266_b/mt2266.h,v $
**	               
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-30-2006    DAD    Ver 1.0: Modified version of mt2260.c (Ver 1.01).
**
*****************************************************************************/
#if !defined( __MT2266_H )
#define __MT2266_H



#if defined( __cplusplus )     
extern "C"                     /* Use "C" external linkage                  */
{
#endif

/*
** Constants used by the tuning algorithm
*/
/*Removed UL to remove warning of narrowing cast*/
                                        /* REF_FREQ is now the actual crystal frequency */
#define REF_FREQ          (30000000)/*(30000000UL)*/  /* Reference oscillator Frequency (in Hz) */
#define TUNE_STEP_SIZE          (50)  /* Tune in steps of 50 kHz */
#define MIN_UHF_FREQ     (350000000)  /* Minimum UHF frequency (in Hz) */
#define MAX_UHF_FREQ     (900000000)  /* Maximum UHF frequency (in Hz) */
#define MIN_VHF_FREQ     (174000000)  /* Minimum VHF frequency (in Hz) */
#define MAX_VHF_FREQ     (230000000)  /* Maximum VHF frequency (in Hz) */
#define OUTPUT_BW          (8000000)  /* Output channel bandwidth (in Hz) */
#define UHF_DEFAULT_FREQ (600000000)  /* Default UHF input frequency (in Hz) */

#define MT2266_CNT  (1)
#define MAX_UDATA         0xFFFFFFFF/*(4294967295)*//*for removing warning*/  /*  max value storable in UData_t   */
/*
**  Two-wire serial bus subaddresses of the tuner registers.
**  Also known as the tuner's register addresses.
*/
enum MT2266_Register_Offsets
{
    MT2266_PART_REV = 0,   /*  0x00 */
    MT2266_LO_CTRL_1,      /*  0x01 */
    MT2266_LO_CTRL_2,      /*  0x02 */
    MT2266_LO_CTRL_3,      /*  0x03 */
    MT2266_SMART_ANT,      /*  0x04 */
    MT2266_BAND_CTRL,      /*  0x05 */
    MT2266_CLEARTUNE,      /*  0x06 */
    MT2266_IGAIN,          /*  0x07 */
    MT2266_BBFILT_1,       /*  0x08 */
    MT2266_BBFILT_2,       /*  0x09 */
    MT2266_BBFILT_3,       /*  0x0A */
    MT2266_BBFILT_4,       /*  0x0B */
    MT2266_BBFILT_5,       /*  0x0C */
    MT2266_BBFILT_6,       /*  0x0D */
    MT2266_BBFILT_7,       /*  0x0E */
    MT2266_BBFILT_8,       /*  0x0F */
    MT2266_RCC_CTRL,       /*  0x10 */
    MT2266_RSVD_11,        /*  0x11 */
    MT2266_STATUS_1,       /*  0x12 */
    MT2266_STATUS_2,       /*  0x13 */
    MT2266_STATUS_3,       /*  0x14 */
    MT2266_STATUS_4,       /*  0x15 */
    MT2266_STATUS_5,       /*  0x16 */
    MT2266_SRO_CTRL,       /*  0x17 */
    MT2266_RSVD_18,        /*  0x18 */
    MT2266_RSVD_19,        /*  0x19 */
    MT2266_RSVD_1A,        /*  0x1A */
    MT2266_RSVD_1B,        /*  0x1B */
    MT2266_ENABLES,        /*  0x1C */
    MT2266_RSVD_1D,        /*  0x1D */
    MT2266_RSVD_1E,        /*  0x1E */
    MT2266_RSVD_1F,        /*  0x1F */
    MT2266_GPO,            /*  0x20 */
    MT2266_RSVD_21,        /*  0x21 */
    MT2266_RSVD_22,        /*  0x22 */
    MT2266_RSVD_23,        /*  0x23 */
    MT2266_RSVD_24,        /*  0x24 */
    MT2266_RSVD_25,        /*  0x25 */
    MT2266_RSVD_26,        /*  0x26 */
    MT2266_RSVD_27,        /*  0x27 */
    MT2266_RSVD_28,        /*  0x28 */
    MT2266_RSVD_29,        /*  0x29 */
    MT2266_RSVD_2A,        /*  0x2A */
    MT2266_RSVD_2B,        /*  0x2B */
    MT2266_RSVD_2C,        /*  0x2C */
    MT2266_RSVD_2D,        /*  0x2D */
    MT2266_RSVD_2E,        /*  0x2E */
    MT2266_RSVD_2F,        /*  0x2F */
    MT2266_RSVD_30,        /*  0x30 */
    MT2266_RSVD_31,        /*  0x31 */
    MT2266_RSVD_32,        /*  0x32 */
    MT2266_RSVD_33,        /*  0x33 */
    MT2266_RSVD_34,        /*  0x34 */
    MT2266_RSVD_35,        /*  0x35 */
    MT2266_RSVD_36,        /*  0x36 */
    MT2266_RSVD_37,        /*  0x37 */
    MT2266_RSVD_38,        /*  0x38 */
    MT2266_RSVD_39,        /*  0x39 */
    MT2266_RSVD_3A,        /*  0x3A */
    MT2266_RSVD_3B,        /*  0x3B */
    MT2266_RSVD_3C,        /*  0x3C */
    END_REGS
};




/*
**  Parameter for function MT2266_GetParam & MT2266_SetParam that 
**  specifies the tuning algorithm parameter to be read/written.
*/
typedef enum
{
    /*  tuner address                                  set by MT2266_Open() */
    MT2266_IC_ADDR,

    /*  max number of MT2266 tuners       set by MT2266_CNT in mt_userdef.h */
    MT2266_MAX_OPEN,

    /*  current number of open MT2266 tuners           set by MT2266_Open() */
    MT2266_NUM_OPEN,
    
    /*  Number of tuner registers                                           */
    MT2266_NUM_REGS,

    /*  crystal frequency                            (default: 18000000 Hz) */
    MT2266_SRO_FREQ,

    /*  min tuning step size                            (default: 50000 Hz) */
    MT2266_STEPSIZE,

    /*  input center frequency                   set by MT2266_ChangeFreq() */
    MT2266_INPUT_FREQ,

    /*  LO Frequency                             set by MT2266_ChangeFreq() */
    MT2266_LO_FREQ,

    /*  output channel bandwidth                      (default: 8000000 Hz) */
    MT2266_OUTPUT_BW,

    /*  Base band filter calibration RC code                 (default: N/A) */
    MT2266_RC2_VALUE,

    /*  Base band filter nominal cal RC code                 (default: N/A) */
    MT2266_RC2_NOMINAL,

    /*  RF attenuator A/D readback                              (read-only) */
    MT2266_RF_ADC,

    /*  BB attenuator A/D readback                              (read-only) */
    MT2266_BB_ADC,

    /*  RF attenuator setting                             (default: varies) */
    MT2266_RF_ATTN,

    /*  BB attenuator setting                             (default: varies) */
    MT2266_BB_ATTN,

    /*  RF external / internal atten control              (default: varies) */
    MT2266_RF_EXT,

    /*  BB external / internal atten control                   (default: 1) */
    MT2266_BB_EXT,

    /*  LNA gain setting (0-15)                           (default: varies) */
    MT2266_LNA_GAIN,

    /*  Decrement LNA Gain (where arg=min LNA Gain value)                   */
    MT2266_LNA_GAIN_DECR,

    /*  Increment LNA Gain (where arg=max LNA Gain value)                   */
    MT2266_LNA_GAIN_INCR,

    /*  Set for UHF max sensitivity mode                                    */
    MT2266_UHF_MAXSENS,

    /*  Set for UHF normal mode                                             */
    MT2266_UHF_NORMAL,

    MT2266_EOP                    /*  last entry in enumerated list         */
} MT2266_Param;

/*
**  Constants for Specifying Operating Band of the Tuner
*/
#define MT2266_VHF_BAND (0)
#define MT2266_UHF_BAND (1)
#define MT2266_L_BAND   (2)

/*
**  Constants for specifying power modes these values
**  are bit-mapped and can be added/OR'ed to indicate
**  multiple settings.  Examples:
**     MT2266_SetPowerModes(h, MT2266_NO_ENABLES + MT22266_SROsd);
**     MT2266_SetPowerModes(h, MT2266_ALL_ENABLES | MT22266_SRO_NOT_sd);
**     MT2266_SetPowerModes(h, MT2266_NO_ENABLES + MT22266_SROsd);
**     MT2266_SetPowerModes(h, MT2266_SROen + MT22266_LOen + MT2266_ADCen);
*/
#define MT2266_SROen       (0x01)
#define MT2266_LOen        (0x02)
#define MT2266_ADCen       (0x04)
#define MT2266_PDen        (0x08)
#define MT2266_DCOCen      (0x10)
#define MT2266_BBen        (0x20)
#define MT2266_MIXen       (0x40)
#define MT2266_LNAen       (0x80)
#define MT2266_ALL_ENABLES (0xFF)
#define MT2266_NO_ENABLES  (0x00)
#define MT2266_SROsd       (0x100)
#define MT2266_SRO_NOT_sd  (0x000)

/* ====== Functions which are declared in mt2266.c File ======= */

/******************************************************************************
**
**  Name: MT2266_Open
**
**  Description:    Initialize the tuner's register values.
**
**  Usage:          status = MT2266_Open(0xC0, &hMT2266, NULL);
**                  if (MT_IS_ERROR(status))
**                      //  Check error codes for reason
**
**  Parameters:     MT2266_Addr  - Serial bus address of the tuner.
**                  hMT2266      - Tuner handle passed back.
**                  hUserData    - User-defined data, if needed for the
**                                 MT_ReadSub() & MT_WriteSub functions.
**
**  Returns:        status:
**                      MT_OK             - No errors
**                      MT_TUNER_ID_ERR   - Tuner Part/Rev code mismatch
**                      MT_TUNER_INIT_ERR - Tuner initialization failed
**                      MT_COMM_ERR       - Serial bus communications error
**                      MT_ARG_NULL       - Null pointer argument passed
**                      MT_TUNER_CNT_ERR  - Too many tuners open
**
**  Dependencies:   MT_ReadSub  - Read byte(s) of data from the two-wire bus
**                  MT_WriteSub - Write byte(s) of data to the two-wire bus
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   02-03-2006   DAD/JWS Original.
**
******************************************************************************/






/*
**  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 
**  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
**  M U <- Info Codes --> <# Spurs> < User> <----- Err Codes ----->
**
**  31 = MT_ERROR - Master Error Flag.  If set, check Err Codes for reason.
**  30 = MT_USER_ERROR - User-declared error flag.
**  29 = Unused
**  28 = Unused
**  27 = MT_DNC_RANGE
**  26 = MT_UPC_RANGE
**  25 = MT_FOUT_RANGE
**  24 = MT_FIN_OUT_OF_RANGE
**  23 = MT_SPUR_PRESENT - Unavoidable spur in output
**  22 = Unused
**  21 = Unused
**  20 = Unused
**  19 = MT_SPUR_CNT_MASK (MSB) - Count of avoided spurs
**  18 = MT_SPUR_CNT_MASK
**  17 = MT_SPUR_CNT_MASK
**  16 = MT_SPUR_CNT_MASK
**  15 = MT_SPUR_CNT_MASK (LSB)
**  14 = MT_USER_DEFINED4 - User-definable bit (see MT_Userdef.h)
**  13 = MT_USER_DEFINED3 - User-definable bit (see MT_Userdef.h)
**  12 = MT_USER_DEFINED2 - User-definable bit (see MT_Userdef.h)
**  11 = MT_USER_DEFINED1 - User-definable bit (see MT_Userdef.h)
**  10 = Unused
**   9 = MT_TUNER_INIT_ERR - Tuner initialization error
**   8 = MT_TUNER_ID_ERR - Tuner Part Code / Rev Code mismatches expected value
**   7 = MT_TUNER_CNT_ERR - Attempt to open more than MT_TUNER_CNT tuners
**   6 = MT_ARG_NULL - Null pointer passed as argument
**   5 = MT_ARG_RANGE - Argument value out of range
**   4 = MT_INV_HANDLE - Tuner handle is invalid
**   3 = MT_COMM_ERR - Serial bus communications error
**   2 = MT_DNC_UNLOCK - Downconverter PLL is unlocked
**   1 = MT_UPC_UNLOCK - Upconverter PLL is unlocked
**   0 = MT_UNKNOWN - Unknown error
*/
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


#define LNAGAIN_MIN 0
#define LNAGAIN_MAX 14

#define AGC_STATE_START 0
#define AGC_STATE_LNAGAIN_BELOW_MIN 1
#define AGC_STATE_LNAGAIN_ABOVE_MAX 2
#define AGC_STATE_NORMAL 3
#define AGC_STATE_NO_INTERFERER 4
#define AGC_STATE_GRANDE_INTERFERER 6
#define AGC_STATE_MEDIUM_SIGNAL 7
#define AGC_STATE_GRANDE_SIGNAL 8
#define AGC_STATE_MAS_GRANDE_SIGNAL 9
#define AGC_STATE_MAX_SENSITIVITY 10
#define AGC_STATE_SMALL_SIGNAL 11
 U32 UncheckedSet(TUNTDRV_InstanceData_t *Instance,
                            U8         reg,
                            U8         val);

 U32 UncheckedGet(TUNTDRV_InstanceData_t *Instance,
                            U8   reg,
                            U8*  val);
 U32 MT2266_ChangeFreq(TUNTDRV_InstanceData_t *Instance,
                          U32 f_in) ;
                          BOOL MT2266_GetLocked(TUNTDRV_InstanceData_t *Instance);
 U32 MT2266_GetParam(TUNTDRV_InstanceData_t *Instance,
                        MT2266_Param param,
                        U32*     pValue);
                        
 U32 MT2266_GetReg(TUNTDRV_InstanceData_t *Instance,
                      U8   reg,
                      U8*  val);
 U32 MT2266_GetReg(TUNTDRV_InstanceData_t *Instance,
                      U8   reg,
                      U8*  val); 
 U32 MT2266_SetReg(TUNTDRV_InstanceData_t *Instance,
                      U8   reg,
                      U8   val);                                             
 U32 MT2266_SetPowerModes(TUNTDRV_InstanceData_t *Instance,
                             U32  flags);
 U32 MT2266_SetParam(TUNTDRV_InstanceData_t *Instance,
                        MT2266_Param param,
                        U32      nValue);
 U32 MT2266_ReInit(TUNTDRV_InstanceData_t *Instance);  

 void MT2266_pdcontrol_execute(TUNTDRV_InstanceData_t *Instance, IOARCH_Handle_t IOHandle);
 void demod_get_pd( IOARCH_Handle_t IOHandle, unsigned short* level);
 void demod_get_agc(IOARCH_Handle_t IOHandle, U16* rf_agc, U16* bb_agc);
 void demod_set_agclim(IOARCH_Handle_t IOHandle, U16 dir_up);

#if defined( __cplusplus )
}
#endif
#endif

