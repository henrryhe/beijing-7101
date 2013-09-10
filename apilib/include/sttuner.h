/*---------------------------------------------------------------
File Name: sttuner.h

Description: API Interfaces for the STTUNER driver.

Copyright (C) 2006-2007 STMicroelectronics

Revision History:

------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_H
#define __STTUNER_H


/* includes --------------------------------------------------------------- */



#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


#include "stddefs.h"    /* Standard definitions */
#include "stpio.h"
#include "stos.h"

/* constants --------------------------------------------------------------- */

/* STAPI Driver ID */
#define STTUNER_DRIVER_ID 6

/* STTUNER specific Error code base value */
#define STTUNER_DRIVER_BASE (STTUNER_DRIVER_ID << 16)

/* Maximum timeout (generic) */
#define STTUNER_MAX_TIMEOUT ((U32)-1)

/* AGC enable/disable */
#define STTUNER_AGC_ENABLE  TRUE
#define STTUNER_AGC_DISABLE FALSE

/* High/Low RF value */
#define STTUNER_HIGH_RF -15
#define STTUNER_LOW_RF  -65

/* number of (STEVT) events STTUNER posts per instance */
#define STTUNER_NUMBER_EVENTS 8

/* maximum number of instances */
#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_NUM_MAX_HANDLES
#define STTUNER_MAX_HANDLES  STTUNER_NUM_MAX_HANDLES
#else
#define STTUNER_MAX_HANDLES 4
#endif
#endif

#ifdef STTUNER_MINIDRIVER
	#define STTUNER_MAX_HANDLES 1
#endif

/* any function index below STTUNER_IOCTL_BASE are
 reserved and should not be used */
#define STTUNER_IOCTL_BASE 0x00010000

/* parameter passed to STTUNER_GetLLARevision() for retrieving LLA versions of all the demods in the current build */
#define STTUNER_ALL_IN_CURRENT_BUILD 0xFF


/* BEGIN ---------- ST ioctl API 1.0.1 ---------- BEGIN */

/* Do raw I/O using the I/O handle of the device-driver (type) selected */
#define STTUNER_IOCTL_RAWIO   (STTUNER_IOCTL_BASE + 0)

/* Perform I/O using the regsister map installed by a specific type of device-driver */
#define STTUNER_IOCTL_REG_OUT   (STTUNER_IOCTL_BASE + 1)      /* write a device register */
#define STTUNER_IOCTL_REG_IN    (STTUNER_IOCTL_BASE + 2)      /* read  a device register */

/* get/set scan task priority (applies to: satellite scan task) */
#define STTUNER_IOCTL_SCANPRI   (STTUNER_IOCTL_BASE + 3)

/* read a chip register (can be used without opening a tuner instance) */
#define STTUNER_IOCTL_PROBE     (STTUNER_IOCTL_BASE + 4)

/* change tuner scan step size (or rely on calculated ) */
#define STTUNER_IOCTL_TUNERSTEP (STTUNER_IOCTL_BASE + 5)

/* Read the IQ mode from a demod */
#define STTUNER_IOCTL_IQMODE (STTUNER_IOCTL_BASE + 6)



/* --- test & debugging --- */

/* demo call, returns ST_NO_ERROR */
#define STTUNER_IOCTL_TEST_01 (STTUNER_IOCTL_BASE + 500)

/* Set psuedorandom noise generation for satellite unitary test */
#define STTUNER_IOCTL_TEST_SN (STTUNER_IOCTL_BASE + 501)

/* Test the search termination in the unitary test  */
#define STTUNER_IOCTL_SEARCH_TEST (STTUNER_IOCTL_BASE + 502)

/* Set LNBH21 ttx, power enable and current protection mode  */

#define STTUNER_IOCTL_SETLNB (STTUNER_IOCTL_BASE + 503)

/* Get LNBH21 ttx, power enable and current protection mode  */
#define STTUNER_IOCTL_GETLNB (STTUNER_IOCTL_BASE + 504)
/*Set TRL Normrate registers*/
#define STTUNER_IOCTL_SETTRL (STTUNER_IOCTL_BASE + 505)
/*Get TRL Normrate registers*/
#define STTUNER_IOCTL_GETTRL (STTUNER_IOCTL_BASE + 506)

/*Set registers(PLLNDIv,TRL Normrate,INC derotator) for 30MHZ crystal*/
#define STTUNER_IOCTL_SET_30MHZ_REG (STTUNER_IOCTL_BASE + 507)
/*Get registers for 30MHZ crystal*/
#define STTUNER_IOCTL_GET_30MHZ_REG (STTUNER_IOCTL_BASE + 508)

/* Set the IF_IQ mode for device(Currently supported in 362 driver only)*/
#define STTUNER_IOCTL_SET_IF_IQMODE (STTUNER_IOCTL_BASE + 509)

/* Get the IF_IQ mode for device(Currently supported in 362 driver only)*/
#define STTUNER_IOCTL_GET_IF_IQMODE (STTUNER_IOCTL_BASE + 510)



/* END ---------- ST ioctl API 1.0.1 ---------- END */

/* -------------------------------------------------
  ID values for enumerating hardware devices,
  every type of hardware WILL have a unique ID number:

    sys:    1 to    9
    com:   10 to  200
    sat:  210 to  400
    cab:  410 to  600
    ter   610 to  800
    ???:  810 to 1000
   ------------------------------------------------- */

/* sys */

/* no driver (*NOT* 'null' driver) */
#define STTUNER_NO_DRIVER     0

/* ID for software function calls via ioctl (not direct to device drivers) */
#define STTUNER_SOFT_ID       1

/* common */
#define STTUNER_DEMOD_BASE   10 /* to 50  */
#define STTUNER_TUNER_BASE   60 /* to 100 */

/* satellite */
#define STTUNER_LNB_BASE    210 /* to 250 */

/* DiSEqC2.0*/
#define MAX_NUM_PULSE_IN_REPLY 90 /* Maximum number of bits in a reply */
#define MAX_NUM_BITS_IN_REPLY MAX_NUM_PULSE_IN_REPLY

#define STTUNER_SCR_BASE    300 /* to 350*/
#define STTUNER_DISEQC_BASE 400 /*to 450*/

/* cable */

/* terrestrial */

/* Linux workaround for task lock with preemted kernel */

  #define STTUNER_task_lock()   STOS_TaskLock() 
  #define STTUNER_task_unlock() STOS_TaskUnlock() 

/* enumerated system types ------------------------------------------------- */

/* error return values (common) */
    typedef enum STTUNER_Error_e
    {
        STTUNER_ERROR_UNSPECIFIED = STTUNER_DRIVER_BASE,
        STTUNER_ERROR_HWFAIL,       /* hardware failure eg. LNB fail for a satellite device */
        STTUNER_ERROR_EVT_REGISTER, /* could not register tuner events with STEVT */
        STTUNER_ERROR_ID,           /* incorrect device/driver ID */
        STTUNER_ERROR_POINTER,      /* pointer was null or did not point to valid memory (RAM) */
        STTUNER_ERROR_INITSTATE,    /* incorrect initalization state encountered */
        STTUNER_ERROR_END_OF_SCAN,  /* End of Scan process */
        STTUNER_ERROR_DISEQC_PARITY,/* DiSEqC reply parity error */
        STTUNER_ERROR_DISEQC_CORRUPTED_REPLY, /* DiSEqC reply command corrupated */
        STTUNER_ERROR_DISEQC_TIMEOUT, /* If no reply received after waiting for the reply */
        STTUNER_ERROR_DISEQC_SYMBOL, /*Diseqc reply symbol error*/
        STTUNER_ERROR_DISEQC_CODE   /*Diseqc reply code error*/
      } STTUNER_Error_t;


/* event codes (common) */
    typedef enum STTUNER_EventType_e
    {
        STTUNER_EV_NO_OPERATION = STTUNER_DRIVER_BASE,  /* No current scan */
        STTUNER_EV_LOCKED,                              /* tuner locked    */
        STTUNER_EV_UNLOCKED,                            /* tuner lost lock */
        STTUNER_EV_SCAN_FAILED,                         /* scan failed     */
        STTUNER_EV_TIMEOUT,                             /* scan timed out  */
        STTUNER_EV_SIGNAL_CHANGE,                       /* Signal threshold has changed */
        STTUNER_EV_SCAN_NEXT,                           /* process next freq for scan (cable) */
        STTUNER_EV_LNB_FAILURE				/* incase of LNB failure due to current
        						 overloading or over heating*/
    } STTUNER_EventType_t;



/* enumerated hardware selection ------------------------------------------- */

/* device category (common) */
    typedef enum STTUNER_Device_e
    {
        STTUNER_DEVICE_NONE      = 0,
        STTUNER_DEVICE_SATELLITE = 1,  /* SatBU     */
        STTUNER_DEVICE_CABLE     = 2,  /* iCHD/CMSA */
        STTUNER_DEVICE_TERR      = 4   /* TerBU     */
    } STTUNER_Device_t;


/* demod device-driver (common) */
    typedef enum STTUNER_DemodType_e
    {
        STTUNER_DEMOD_NONE = STTUNER_DEMOD_BASE, /* common: null demodulation driver use with e.g. STV0399 demod driver*/
	        STTUNER_DEMOD_STV0299,                   /* sat: demodulation, fec & digital interface chip, */
	        STTUNER_DEMOD_STX0288,
	        STTUNER_DEMOD_STV0399,                   /* sat: features include built in tuner. */
	        STTUNER_DEMOD_STB0899,
	        STTUNER_DEMOD_STV0360,                   /* terrestrial */
	        STTUNER_DEMOD_STV0361,                   /* terrestrial */
	        STTUNER_DEMOD_STV0362,                   /* terrestrial */
	        STTUNER_DEMOD_STB0370VSB,		/* terrestrial */
	        STTUNER_DEMOD_STV0371 = STTUNER_DEMOD_STB0370VSB, /*371 demod type made as 370VSB*/
	        STTUNER_DEMOD_STV0297,                   /* Cable : demodulation, fec & digital interface chip, J83 A/C */
	        STTUNER_DEMOD_STV0297J,                   /* Cable : demodulation, fec & digital interface chip, J83 A/B/C */
	        STTUNER_DEMOD_STB0370QAM,                  /* Cable: 370 QAM ITU-T J83 FEC B */
	        STTUNER_DEMOD_STV0297E,
	        STTUNER_DEMOD_STV0498,
        
        
        
	        STTUNER_DEMOD_STB0900,		/*Satellite */
        
        	STTUNER_DEMOD_STV0372                  /*terrestrial VSB*/
    } STTUNER_DemodType_t;


/* tuner device-driver (common) */
    typedef enum STTUNER_TunerType_e
    {
        STTUNER_TUNER_NONE = STTUNER_TUNER_BASE, /* null tuner driver e.g. use with 399 which has no external tuner driver */
        STTUNER_TUNER_VG1011,   /* satellite (discrete zif devices) */
        STTUNER_TUNER_TUA6100,  /* satellite */
        STTUNER_TUNER_EVALMAX,  /* satellite */
        STTUNER_TUNER_S68G21,   /* satellite */
        STTUNER_TUNER_VG0011,   /* satellite */
        STTUNER_TUNER_HZ1184,   /* satellite */
        STTUNER_TUNER_MAX2116,  /* satellite */
        STTUNER_TUNER_MAX2118,  /* satellite */
        STTUNER_TUNER_DSF8910,  /* satellite */
        STTUNER_TUNER_IX2476,  /* satellite */
        STTUNER_TUNER_DTT7572,  /* terrestrial : TMM */
        STTUNER_TUNER_DTT7578,  /* terrestrial : TMM */
        STTUNER_TUNER_DTT7592,  /* terrestrial : TMM */
        STTUNER_TUNER_TDLB7,    /* terrestrial : ALPS */
        STTUNER_TUNER_TDQD3,    /* terrestrial : ALPS */
        STTUNER_TUNER_DTT7300X,    /* terrestrial : THOMSON */
        STTUNER_TUNER_ENG47402G1,/* terrestrial: MACO */
        STTUNER_TUNER_EAL2780,  /* terrestrial : SIEL */
        STTUNER_TUNER_TDA6650,  /* terrestrial : PHILIPS */
        STTUNER_TUNER_TDM1316,  /* terrestrial : PHILIPS */
        STTUNER_TUNER_TDEB2,    /* terrestrial : ALPS */
        STTUNER_TUNER_ED5058,   /* terrestrial : SHAPRP */
        STTUNER_TUNER_MIVAR,    /* terrestrial : MIVAR  */
        STTUNER_TUNER_TDED4,    /* terrestrial : ALPS */
        STTUNER_TUNER_DTT7102,  /* terrestrial : TMM */
        STTUNER_TUNER_TECC2849PG, /* terrestrial : SEM */
        STTUNER_TUNER_TDCC2345, /* terrestrial : SEM */	
        STTUNER_TUNER_TDTGD108,   /* Terrestrial LG tuner */
        STTUNER_TUNER_MT2266,     /* Terrestrial and Cable : Tuner MICROTUNE */
        STTUNER_TUNER_TDBE1,      /* Cable : Tuner ALPS J83 A */
        STTUNER_TUNER_TDBE2,      /* Cable : Tuner ALPS J83 A */
        STTUNER_TUNER_TDDE1,      /* Cable : Tuner ALPS J83 A*/
        STTUNER_TUNER_SP5730,     /* Cable : Tuner ALPS J83 A */
        STTUNER_TUNER_MT2030,     /* Cable : Tuner MICROTUNE J83 A */
        STTUNER_TUNER_MT2040,     /* Cable : Tuner MICROTUNE J83 A */
        STTUNER_TUNER_MT2050,     /* Cable : Tuner MICROTUNE J83 A */
        STTUNER_TUNER_MT2060,     /* Cable : Tuner MICROTUNE J83 A */
        STTUNER_TUNER_DCT7040,    /* Cable : Tuner THOMSON J83 A */
        STTUNER_TUNER_DCT7050,    /* Cable : Tuner THOMSON J83 A */
        STTUNER_TUNER_DCT7710,    /* Cable : Tuner THOMSON J83 A */
        STTUNER_TUNER_DCF8710,    /* Cable : Tuner THOMSON J83 A */
        STTUNER_TUNER_DCF8720,    /* Cable : Tuner THOMSON J83 A */
        STTUNER_TUNER_MACOETA50DR,/* Cable : Tuner MATSUSHITA J83 A */
        STTUNER_TUNER_CD1516LI,   /* Cable : Tuner PHILIPS J83 A */
        STTUNER_TUNER_DF1CS1223,  /* Cable : Tuner NOKIA J83 A */
        STTUNER_TUNER_SHARPXX,    /* Cable : Tuner SHARP */
        STTUNER_TUNER_TDBE1X016A, /* Cable : Tuner ALPS J83 A */
        STTUNER_TUNER_TDBE1X601,  /* Cable : Tuner ALPS J83 A */
        STTUNER_TUNER_TDEE4X012A,  /* Cable : Tuner ALPS J83 A */
        STTUNER_TUNER_DCT7045,    /* Cable : Tuner THOMSON */
        STTUNER_TUNER_TDQE3,      /* Cable : Tuner ALPS */
        STTUNER_TUNER_DCF8783,    /* Cable : Tuner THOMSON */
        STTUNER_TUNER_DCT7045EVAL, /* Cable : Tuner THOMSON 7045 on Eval board*/
        STTUNER_TUNER_DCT70700,
        STTUNER_TUNER_TDCHG,
        STTUNER_TUNER_TDCJG,
        STTUNER_TUNER_TDCGG,
        STTUNER_TUNER_MT2011,    /* Cable : Tuner Microtune J83 A/B*/
        STTUNER_TUNER_STB6000,  /* satellite */
         STTUNER_TUNER_STB6110,  /* satellite */
        STTUNER_TUNER_DTT7600,    /* terrestrial VSB */
        STTUNER_TUNER_TDVEH052F,    /* terrestrial VSB and QAM*/
        STTUNER_TUNER_DTVS223, /* Terrestrial VSB Samsung */
        STTUNER_TUNER_TD1336, /* Analog NTSC, Dig ATSC,QAM :Philips*/
        STTUNER_TUNER_FQD1236, /* Analog NTSC, Dig ATSC,QAM :Philips*/
        STTUNER_TUNER_T2000, /* Analog NTSC, Dig ATSC,QAM :Philips*/
        STTUNER_TUNER_STB6100,  /* satellite */
        STTUNER_TUNER_DTT761X,  /* Terrestrial(VSB)+Cable(QAM) */
        STTUNER_TUNER_DTT768XX,  /* Terrestrial(VSB)+Cable(QAM) */
        STTUNER_TUNER_MT2131,     /* Terrestrial and Cable  : Tuner MICROTUNE    : for basic  eco and adv*/
        STTUNER_TUNER_RF4000  /* Terrestrial COFDM STB 4K : for basic  eco and adv*/
    } STTUNER_TunerType_t;

/* lnb device-driver (sat) */
    typedef enum STTUNER_LnbType_e
    {
        STTUNER_LNB_NONE = STTUNER_LNB_BASE,    /* null LNB driver */
        STTUNER_LNB_STEM,                       /* I2C switch does the LNB work */
        STTUNER_LNB_LNB21,                      /* I2C controlled LNBP21 -> used for tone & DiSEqC*/
        STTUNER_LNB_LNBH21,			/* I2C controlled LNBPH21 -> used for tone & DiSEqC*/
        STTUNER_LNB_LNBH23, 			/* I2C controlled LNBPH23 -> used for tone & DiSEqC*/
        STTUNER_LNB_LNBH24,  			/* I2C controlled LNBPH24 -> used for tone & DiSEqC*/
        STTUNER_LNB_DEMODIO,
        STTUNER_LNB_BACKENDGPIO                 
} STTUNER_LnbType_t;

/* DiSEqC on 5100*/
 typedef enum STTUNER_DiSEqC_Type_e
    {
        STTUNER_DISEQC_NONE = STTUNER_DISEQC_BASE,    /* null DISEQC driver */
        STTUNER_DISEQC_5100,                           /* DiSEqC through backend(5100)*/
        STTUNER_DISEQC_5301 = STTUNER_DISEQC_5100                       /* DiSEqC through backend(5301)*/
} STTUNER_DiSEqCType_t;

/* architecture specifics for I/O mode (common) */
    typedef enum STTUNER_IORoute_e
    {
        STTUNER_IO_DIRECT,      /* open IO address for this instance of device */
        STTUNER_IO_REPEATER,    /* as above but send IO via ioaccess function (STTUNER_IOARCH_RedirFn_t) of other driver (demod) */
        STTUNER_IO_PASSTHRU     /* do not open address just send IO using other drivers open I/O handle (demod) */
    } STTUNER_IORoute_t;


/* select I/O target (common) */
    typedef enum STTUNER_IODriver_e
    {
        STTUNER_IODRV_NULL,     /* null driver */
        STTUNER_IODRV_I2C,      /* use I2C drivers */
        STTUNER_IODRV_DEBUG,    /* screen and keyboard IO*/
        STTUNER_IODRV_MEM       /* ST20 memory driver */
    } STTUNER_IODriver_t;


/* enumerated configuration get/set ---------------------------------------- */

/*  note: not all devices/devices support all configuration options, also with enumerations
   note that the values assigned are not to be taken as a direct mapping of the low level
   hardware registers (individual device drivers may translate these values) */

/* FEC Rates (sat & terr) */
    typedef enum STTUNER_FECRate_e
    {
        STTUNER_FEC_NONE = 0x00,    /* no FEC rate specified */
        STTUNER_FEC_ALL = 0xFFFF,     /* Logical OR of all FECs */
        STTUNER_FEC_1_2 = 1,
        STTUNER_FEC_2_3 = (1 << 1),
        STTUNER_FEC_3_4 = (1 << 2),
        STTUNER_FEC_4_5 = (1 << 3),
        STTUNER_FEC_5_6 = (1 << 4),
        STTUNER_FEC_6_7 = (1 << 5),
        STTUNER_FEC_7_8 = (1 << 6),
        STTUNER_FEC_8_9 = (1 << 7),
        STTUNER_FEC_1_4 = (1 << 8),
        STTUNER_FEC_1_3 = (1 << 9),
        STTUNER_FEC_2_5 = (1 << 10),
        STTUNER_FEC_3_5 = (1 << 11),
        STTUNER_FEC_9_10= (1 << 12)
    } STTUNER_FECRate_t;
    
    typedef enum STTUNER_ModeCode_e
    {
    	STTUNER_MODECODE_DUMMY_PLF,
		STTUNER_MODECODE_QPSK_14,
		STTUNER_MODECODE_QPSK_13,
		STTUNER_MODECODE_QPSK_25,
		STTUNER_MODECODE_QPSK_12,
		STTUNER_MODECODE_QPSK_35,
		STTUNER_MODECODE_QPSK_23,
		STTUNER_MODECODE_QPSK_34,
		STTUNER_MODECODE_QPSK_45,
		STTUNER_MODECODE_QPSK_56,
		STTUNER_MODECODE_QPSK_89,
		STTUNER_MODECODE_QPSK_910,
		STTUNER_MODECODE_8PSK_35,
		STTUNER_MODECODE_8PSK_23,
		STTUNER_MODECODE_8PSK_34,
		STTUNER_MODECODE_8PSK_56,
		STTUNER_MODECODE_8PSK_89,
		STTUNER_MODECODE_8PSK_910,
		STTUNER_MODECODE_16APSK_23,
		STTUNER_MODECODE_16APSK_34,
		STTUNER_MODECODE_16APSK_45,
		STTUNER_MODECODE_16APSK_56,
		STTUNER_MODECODE_16APSK_89,
		STTUNER_MODECODE_16APSK_910,
		STTUNER_MODECODE_32APSK_34,
		STTUNER_MODECODE_32APSK_45,
		STTUNER_MODECODE_32APSK_56,
		STTUNER_MODECODE_32APSK_89,
		STTUNER_MODECODE_32APSK_910
    } 
    STTUNER_ModeCode_t;


/*Sync stripping*/
#if defined(STTUNER_USE_CAB)
typedef enum STTUNER_SyncStrip_e
    {
        STTUNER_SYNC_STRIP_DEFAULT, /* Default is Off */
        STTUNER_SYNC_STRIP_OFF, /* TS valid is high during TS sync pulse */
        STTUNER_SYNC_STRIP_ON   /* TS valid is down during TS sync pulse */
    }
    STTUNER_SyncStrip_t;
#endif


/* type of modulation (common) */
    typedef enum STTUNER_Modulation_e
    {
        STTUNER_MOD_NONE   = 0x00,  /* Modulation unknown */
        STTUNER_MOD_ALL    = 0x3FF, /* Logical OR of all MODs */
        STTUNER_MOD_QPSK   = 1,
        STTUNER_MOD_8PSK   = (1 << 1),
        STTUNER_MOD_QAM    = (1 << 2),
        STTUNER_MOD_4QAM   = (1 << 3),
        STTUNER_MOD_16QAM  = (1 << 4),
        STTUNER_MOD_32QAM  = (1 << 5),
        STTUNER_MOD_64QAM  = (1 << 6),
        STTUNER_MOD_128QAM = (1 << 7),
        STTUNER_MOD_256QAM = (1 << 8),
        STTUNER_MOD_BPSK   = (1 << 9),
        STTUNER_MOD_16APSK,
        STTUNER_MOD_32APSK,
        STTUNER_MOD_8VSB  =  (1 << 10)
    } STTUNER_Modulation_t;

 /* mode of OFDM signal (ter) */
    typedef enum STTUNER_Mode_e
    {
        STTUNER_MODE_2K,
        STTUNER_MODE_8K,
        STTUNER_MODE_4K
    } STTUNER_Mode_t;


 /* guard of OFDM signal (ter) */
    typedef enum STTUNER_Guard_e
    {
        STTUNER_GUARD_1_32,               /* Guard interval = 1/32 */
        STTUNER_GUARD_1_16,               /* Guard interval = 1/16 */
        STTUNER_GUARD_1_8,                /* Guard interval = 1/8  */
        STTUNER_GUARD_1_4                 /* Guard interval = 1/4  */
    } STTUNER_Guard_t;


 /* hierarchy (ter) */
    typedef enum STTUNER_Hierarchy_e
    {
        STTUNER_HIER_NONE,              
        STTUNER_HIER_LOW_PRIO,          
        STTUNER_HIER_HIGH_PRIO,         
        STTUNER_HIER_PRIO_ANY           
    } STTUNER_Hierarchy_t;

  /* Alpha value  during Hierarchical Modulation */
    typedef enum STTUNER_Hierarchy_Alpha_e
    {
       STTUNER_HIER_ALPHA_NONE,
       STTUNER_HIER_ALPHA_1,
       STTUNER_HIER_ALPHA_2,
       STTUNER_HIER_ALPHA_4
    } STTUNER_Hierarchy_Alpha_t;

  /* Alpha value  during Hierarchical Modulation */
 /* (ter & cable) */
    typedef enum STTUNER_Spectrum_e
	{
        STTUNER_INVERSION_NONE = 0,
        STTUNER_INVERSION      = 1,
        STTUNER_INVERSION_AUTO = 2,
        STTUNER_INVERSION_UNK  = 4
    } STTUNER_Spectrum_t;
    
    typedef enum STTUNER_J83_e
    {
        STTUNER_J83_NONE   = 0x00,
        STTUNER_J83_ALL    = 0xFF,
        STTUNER_J83_A      = 1,
        STTUNER_J83_B      = (1 << 1),
        STTUNER_J83_C      = (1 << 2)
    } STTUNER_J83_t;

    /* (sat) */
    typedef enum STTUNER_IQMode_e
	{
        STTUNER_IQ_MODE_NORMAL       = 0,
        STTUNER_IQ_MODE_INVERTED     = 1,
        STTUNER_IQ_MODE_AUTO         = 2
    } STTUNER_IQMode_t;


 /* (ter) */
    typedef enum STTUNER_FreqOff_e
	{
        STTUNER_OFFSET_NONE = 0,
        STTUNER_OFFSET      = 1,
        STTUNER_OFFSET_POSITIVE = 2,
        STTUNER_OFFSET_NEGATIVE = 3
    } STTUNER_FreqOff_t;

 /* (ter) */
    typedef enum STTUNER_Force_e
	{
        STTUNER_FORCE_NONE  = 0,
        STTUNER_FORCE_M_G   = 1
    } STTUNER_Force_t;

/* (ter) */
    typedef enum STTUNER_ChannelBW_e
    {
    	STTUNER_CHAN_BW_NONE =0x00,
        STTUNER_CHAN_BW_6M  = 6,
        STTUNER_CHAN_BW_7M  = 7,
        STTUNER_CHAN_BW_8M  = 8
    } STTUNER_ChannelBW_t;


/* type of polarization (sat) */
    typedef enum STTUNER_Polarization_e
    {
        STTUNER_PLR_ALL        = 0x07, /* Logical OR of all PLRs */
        STTUNER_PLR_HORIZONTAL = 1,
        STTUNER_PLR_VERTICAL   = (1 << 1),
        STTUNER_LNB_OFF        = (1 << 2)  /* Add for LNB off-> in case tuner is fed from modulator */
    } STTUNER_Polarization_t;


/* FEC Modes (sat & terr) */
    typedef enum STTUNER_FECMode_e
    {
        STTUNER_FEC_MODE_DEFAULT,   /* Use default FEC mode */
        STTUNER_FEC_MODE_DIRECTV,    /* DirecTV Legacy mode */
        STTUNER_FEC_MODE_ATSC,       /*For ATSC VSB transmission*/
        STTUNER_FEC_MODE_DVB       /* DVB mode */        
    } STTUNER_FECMode_t;

/* FEC  Type detail (sat - 899) */
    typedef enum STTUNER_FECType_e
    {
    	
        STTUNER_FEC_MODE_DVBS2 ,         /* used for DVBS2 based Devices driver only, for 899 and ADV1*/        
		STTUNER_FEC_MODE_DVBS1          /* Needed only in 899 driver*/
    } STTUNER_FECType_t;
    

    /* used in 900 only */
    typedef enum
	{	
		STTUNER_BLIND_SEARCH,					/* offset freq and SR are Unknown */
		STTUNER_COLD_START,						/* only the SR is known */
		STTUNER_WARM_START						/* offset freq and SR are known */
	
	} STTUNER_SearchAlgo_t;

/* StandByMode Events */
   typedef enum STTUNER_StandByMode_e
   {
      STTUNER_NORMAL_POWER_MODE ,
      STTUNER_STANDBY_POWER_MODE   	
   } STTUNER_StandByMode_t;
 
/* LNB tone state (sat) */
    typedef enum STTUNER_LNBToneState_e
    {
        STTUNER_LNB_TONE_DEFAULT,   /* Default (current) LNB state */
        STTUNER_LNB_TONE_OFF,       /* LNB disabled */
        STTUNER_LNB_TONE_22KHZ      /* LNB set to 22kHz */
    } STTUNER_LNBToneState_t;

    typedef enum STTUNER_22KHzToneControl_e
    {
	    STTUNER_22KHz_TONE_DemodOP0Pin,
        STTUNER_22KHz_TONE_DemodDiseqcPin
    } STTUNER_22KHzToneControl_t;

    typedef enum STTUNER_PowerControl_e
    {
	      STTUNER_LNB_POWER_DemodDACPin,
        STTUNER_LNB_POWER_LNBPDefault
    } STTUNER_PowerControl_t;

    typedef enum STTUNER_LNBCurrentThresholdSelection_e
    {
  	  STTUNER_LNB_CURRENT_THRESHOLD_HIGH = 0,
  	  STTUNER_LNB_CURRENT_THRESHOLD_LOW
    } STTUNER_LNBCurrentThresholdSelection_t;

    typedef enum STTUNER_LNBShortCircuitProtection_e
    {
    	STTUNER_LNB_PROTECTION_DYNAMIC=0,/* LNB output will toggle at short circuit*/
    	STTUNER_LNB_PROTECTION_STATIC
    } STTUNER_LNBShortCircuitProtectionMode_t;

    typedef BOOL STTUNER_CoaxCableLossCompensation_t;  /*TRUE corresponds to 13/18 + 1 V */

 /* (cab) */
    typedef enum STTUNER_Sti5518_e
	{
        STTUNER_STI5518_NONE = 0,
        STTUNER_STI5518      = 1
    } STTUNER_Sti5518_t;

/* tuner status (common) */
    typedef enum STTUNER_Status_e
    {
        STTUNER_STATUS_UNLOCKED,
        STTUNER_STATUS_SCANNING,
        STTUNER_STATUS_LOCKED,
        STTUNER_STATUS_NOT_FOUND,
        STTUNER_STATUS_STANDBY,
        STTUNER_STATUS_IDLE        
    } STTUNER_Status_t;


/* quality format (common) */
    typedef enum STTUNER_QualityFormat_e
    {
        STTUNER_QUALITY_BER,    /* Bit error rate */
        STTUNER_QUALITY_CN      /* C/N ratio based 0-100 scale */
    } STTUNER_QualityFormat_t;


/* serial clock source (common) */
    typedef enum STTUNER_SerialClockSource_e
    {
        STTUNER_SCLK_DEFAULT,   /* Use default SCLK */
        STTUNER_SCLK_MASTER,    /* Derived from MCLK */
        STTUNER_SCLK_VCODIV6    /* SCLK = FVCO/6 */
    }
    STTUNER_SerialClockSource_t;


/* Data Clock Freq, in case of FIFO enabled */
    typedef enum STTUNER_DataFIFOMode_e
    {
    	STTUNER_DATAFIFO_ENABLED,
    	STTUNER_DATAFIFO_DISABLED
    }
    STTUNER_DataFIFOMode_t;

    /* Data Clock Freq, in case of FIFO enabled */
    typedef enum STTUNER_TSDataOutputControl_e
    {
    	STTUNER_TSDATA_OUTPUT_CONTROL_AUTO,
    	STTUNER_TSDATA_OUTPUT_CONTROL_MANUAL
    }
    STTUNER_TSDataOutputControl_t;

    typedef enum STTUNER_OutputRateCompensationMode_e
    {
    	STTUNER_COMPENSATE_DATACLOCK,
    	STTUNER_COMPENSATE_DATAVALID
    }
    STTUNER_OutputRateCompensationMode_t;

    typedef struct STTUNER_OutputFIFOConfig_s
    {
        int CLOCKPERIOD;/* when Parallel 4<=CLOCKPERIOD<=60, when serial 1<=CLOCKPERIOD<=4, clk = CLOCKPERIOD*mclk */
        STTUNER_OutputRateCompensationMode_t OutputRateCompensationMode;/* either data clock can be punctured 0r data valid in case of invalid data*/
    }
    STTUNER_OutputFIFOConfig_t;

    typedef enum STTUNER_BlockSyncMode_e
    {
        STTUNER_SYNC_DEFAULT,   /* Use default SCLK */
        STTUNER_SYNC_NORMAL,    /*First byte 0x47, complemented to 0xB8 with every 8th packet*/
        STTUNER_SYNC_FORCED     /* Always forced to 0x47*/
    }
    STTUNER_BlockSyncMode_t;


/* serial clock modes (common) */
    typedef enum STTUNER_SerialDataMode_e
    {
        STTUNER_SDAT_DEFAULT       = -1,  /* Use default SDAT mode */
        STTUNER_SDAT_VALID_RISING  = 1,             /* Data valid on clock rising edge */
        STTUNER_SDAT_PARITY_ENABLE = (1 << 1)       /* Data includes parity */
    }
    STTUNER_SerialDataMode_t;

/* clock & parity status during parity bytes*/
    typedef enum STTUNER_DataClockAtParityBytes_e
    {
        STTUNER_DATACLOCK_DEFAULT       = -1,   /* data clock will be continous & parity bytes are transmitted */
        STTUNER_DATACLOCK_CONTINOUS  = 1,             /* data clock will be continous & parity bytes are transmitted */
        STTUNER_DATACLOCK_NULL_AT_PARITYBYTES = (1 << 1)       /* data[7:0],data clock & error are null during parity bytes
                                                                  but in serial mode data clock is always running */
    }
    STTUNER_DataClockAtParityBytes_t;


/* transport stream output mode (common) */
    typedef enum STTUNER_TSOutputMode_e
    {
        STTUNER_TS_MODE_DEFAULT,    /* TS output not changeable */
        STTUNER_TS_MODE_SERIAL,     /* TS output serial */
        STTUNER_TS_MODE_PARALLEL,   /* TS output parallel */
        STTUNER_TS_MODE_DVBCI       /* TS output DVB-CI */
    }
    STTUNER_TSOutputMode_t;


/* downlink frequency bands (sat) */
    typedef enum STTUNER_DownLink_e
    {
        	STTUNER_DOWNLINK_Ku = 1,   /* e.g. Europe */
        	STTUNER_DOWNLINK_C         /*      Asia   */
    }
    STTUNER_DownLink_t;
 /*	Commnad for  DISEqC API IMPLEMENTATION (Satellite )*/

typedef enum STTUNER_DiSEqCCommand_e
    {
        	STTUNER_DiSEqC_TONE_BURST_OFF,					/* Generic */
		STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH,			/* f22 pin high after tone off */
		STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW,  		/* f22 pin low after tone off*/
		STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON,  		/* unmodulated */
		STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED,   /* send of 0 for 12.5 ms ;continuous tone*/
		STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED,     /* 0-2/3 duty cycle tone*/
		STTUNER_DiSEqC_TONE_BURST_SEND_1, 				/* 1-1/3 duty cycle tone*/
		STTUNER_DiSEqC_COMMAND							/* DiSEqC (1.2/2)command   */

    }
    STTUNER_DiSEqCCommand_t;

/* data structures --------------------------------------------------------- */

    /* band start, end and Local Oscillator frequency (all values in KHz) (common) */
    typedef struct STTUNER_Band_s
    {
        U32 BandStart;
        U32 BandEnd;
        #ifdef STTUNER_USE_SAT
        U32 LO;
        STTUNER_DownLink_t DownLink;    /* (sat) select how IF for demod is calculated from the F and LO values */
        #endif
    }
    STTUNER_Band_t;

    /* List of band frequencies (common) */
    typedef struct STTUNER_BandList_s
    {
        U32             NumElements;
        STTUNER_Band_t *BandList;
    }
    STTUNER_BandList_t;


    /* signal threshold (common) */
    typedef struct STTUNER_SignalThreshold_s
    {
        U32 SignalLow;
        U32 SignalHigh;
    }
    STTUNER_SignalThreshold_t;

    /* Signal threshold list (common) */
    typedef struct STTUNER_ThresholdList_s
    {
        U32                        NumElements;
        STTUNER_SignalThreshold_t *ThresholdList;
    }
    STTUNER_ThresholdList_t;

/************************/

/************************/


/* ---------------------------------SCR DATABASE------------------------------------*/
typedef enum STTUNER_SCR_Mode_e
{
  	STTUNER_SCR_MANUAL, /* user will set all lnb parameters (LO, LPF)*/
  	STTUNER_SCR_AUTO,   /* a search tone will detect free SCR and set LNB parameters*/
  	STTUNER_UBCHANGE
}
STTUNER_SCR_Mode_t;

typedef  enum STTUNER_SCRLNB_Index_e
{
	STTUNER_LNB0, /* LNB index 0 for RF Input*/
	STTUNER_LNB1, /* LNB index 1 for RF Input*/
	STTUNER_LNB2, /* LNB index 2 for RF Input*/
	STTUNER_LNB3  /* LNB index 3 for RF Input*/
}
STTUNER_SCRLNB_Index_t;

typedef enum STTUNER_SCRBPF_e
{
	STTUNER_SCRBPF0,  /* output BPF index 0*/
	STTUNER_SCRBPF1,  /* output BPF index 1*/
	STTUNER_SCRBPF2,  /* output BPF index 2*/
	STTUNER_SCRBPF3   /* output BPF index 3*/
}
STTUNER_SCRBPF_t ;

typedef enum STTUNER_SCR_FConnector_Type_e
{
	RTA_Stb ,            /* SCR only supports RTA STB */
	Legacy_Stb ,         /* Only for legacy STB  */
	RTA_Legacy           /* SCR supports both RTA and  Legacy box */
}
STTUNER_SCR_FConnector_Type_t;

typedef enum STTUNER_LNB_SignalRouting_e
{
	STTUNER_VIA_CONVENTIONAL_LNB,
	STTUNER_VIA_SCRENABLED_LNB
}	
STTUNER_LNB_SignalRouting_t;

typedef  enum STTUNER_ScrType_e
{
 /* Put all the 7 applications supported by ST7LNB1*/
 	STTUNER_SCR_NULL,
 	STTUNER_SCR_APPLICATION,
 	STTUNER_SCR_APPLICATION_1,
 	STTUNER_SCR_APPLICATION_2,
 	STTUNER_SCR_APPLICATION_3,
 	STTUNER_SCR_APPLICATION_4,
 	STTUNER_SCR_APPLICATION_5,
 	STTUNER_SCR_APPLICATION_6,
 	STTUNER_SCR_APPLICATION_7,
 	STTUNER_SCR_APPLICATION_NOT_SUPPORTED
}
STTUNER_ScrType_t ;

typedef struct STTUNER_SCR_s
{
	STTUNER_SCR_Mode_t     SCR_Mode ;   /* Set SCR params manually or autodetection*/
	STTUNER_SCRLNB_Index_t              LNBIndex ;   /* From which LNB to get RF signal*/
	STTUNER_SCRBPF_t                   SCRBPF ;    /* RF should route on this SCR*/
	U32                                                  BPFFrequency ;  /* Band Pass Filter center frequency*/
	U32                                                  SCRVCOFrequency; /*Frequency of  VCO of SCR to programmed*/
	U8            no_of_F_Connector; /* number of f connector at scr output*/
	U8            no_of_LO_IstConversion;  /* number of LO for first IF conversion */
	U8            no_of_SCR;                      /* number of SCR available for output*/
	STTUNER_SCR_FConnector_Type_t    FConnectorType ; /* FConnector support for STB*/
	STTUNER_ScrType_t              SCRApplication;
	U8 NbLnbs;
	U16   SCRLNB_LO_Frequencies[8]; /* In MHz*/
	U8 NbScr;
	U16   SCRBPFFrequencies[8];  /* In MHz*/
}
STTUNER_SCR_t ;


typedef enum STTUNER_LnbTTxMode_e
{
	STTUNER_LNB_RX = 0,
	STTUNER_LNB_TX 	
}STTUNER_LnbTTxMode_t;


typedef enum STTUNER_DISEQC_Mode_e
{
  	STTUNER_DISEQC_MANUAL = 1, /* user will set all register configuration)*/
  	STTUNER_DISEQC_DEFAULT    /*registers will be configured by default values*/	
}STTUNER_DISEQC_Mode_t;

typedef enum STTUNER_DISEQC_Version_e
{
  	STTUNER_DISEQC_VER_1_2 = 1, /* version 1.2*/
  	STTUNER_DISEQC_VER_2_0 /* version 2.0*/	
}STTUNER_DISEQC_Version_t;

typedef enum
{
	STTUNER_DEMOD_1,
	STTUNER_DEMOD_2
}STTUNER_DEMOD_t;
	
	
/*-------------------------------------------------------------------------*/
typedef struct STTUNER_BlindScan_s
{
	U32 Band;
	STTUNER_Polarization_t 		      Polarization; /*** Polarization mode for satellite Horizontal or Vertical**/       
    	STTUNER_LNBToneState_t                LNBToneState;/*** Set different Tone State**/
    #ifdef  STTUNER_DRV_SAT_SCR
 	STTUNER_SCR_t                         ScrParams; 
 	STTUNER_LNB_SignalRouting_t           LNB_SignalRouting;
	#endif
}
STTUNER_BlindScan_t;



    /* scan list (common) */
    typedef struct STTUNER_Scan_s
    {
    	/******Common Scan Parameter Settings********/
    	STTUNER_Modulation_t   Modulation; /*** Configurable parameter for modulation scheme**/
    	U32                    Band; /****** Define the which band to be searched***/
        S16                    AGC;  /***** Store AGC value returned from low level driver***/
        S32                    ResidualOffset;/***** Store delta frequency value for fine tunning ***/
        
        /*******Satellite related parameters******************/      
        #ifdef STTUNER_USE_SAT
	        STTUNER_Polarization_t 		      Polarization; /*** Polarization mode for satellite Horizontal or Vertical**/       
	        STTUNER_LNBToneState_t                LNBToneState;/*** Set different Tone State**/
	        
	        STTUNER_DEMOD_t			      Path;
	        STTUNER_ModeCode_t                    ModeCode;/* Only for DVBS2*/
	        STTUNER_SearchAlgo_t       	      SearchAlgo;
	        
	        
	               
	        STTUNER_FECType_t      	      	      FECType;
	        
				BOOL                                  Pilots;
				#ifdef  STTUNER_DRV_SAT_SCR
				 	STTUNER_SCR_t                         ScrParams;
				 	STTUNER_LNB_SignalRouting_t           LNB_SignalRouting;  
				#endif
	        BOOL                                  DishPositioning_ToneOnOff;  /* Flag to enable the dish position routine */
        #endif
        
        /*******Terrestrial related parameters******************/
        #ifdef STTUNER_USE_TER 	    
	        STTUNER_Mode_t                                Mode; /**** Configurable parameter for TER mode****/            
	        STTUNER_Guard_t                               Guard;/**** Configurable parameter for TER guard****/          
	        STTUNER_Force_t                               Force;/**** Configurable parameter to understand whether the
	                                                                  demod will only work with the search parameter
	                                                                  given by user or it will do automatic search with all
	                                                                  possible parameter****/          
	        STTUNER_Hierarchy_t                           Hierarchy;/**Configurable parameter for hierarchical transmission**/      
	             
	        S32                                           EchoPos;/*** Gives back the echo position value************/               
        #endif
        
        #if defined(STTUNER_USE_TER) ||defined(STTUNER_USE_ATSC_VSB)
        STTUNER_FreqOff_t                             FreqOff;
        STTUNER_ChannelBW_t                           ChannelBW;/*For VSB this parameter is not used now. 
        							Used  BW is 6Mhz*/
        #endif
        
        /*******Cable related parameters******************/
        #ifdef STTUNER_USE_CAB
        STTUNER_J83_t          J83; 
        STTUNER_Spectrum_t     SpectrumIs;    
        #endif
        
        /*******Terrestrial and Cable related parameters******************/
        #if defined (STTUNER_USE_TER) || defined (STTUNER_USE_CAB) || defined(STTUNER_USE_ATSC_VSB)
        STTUNER_Spectrum_t     Spectrum; /**** Take care  whether demod will search for both inverted and non inverted spectrum
                                               or any one of them***/
        #endif
        
        /*******Terrestrial and Satellite related parameters******************/
        #if defined (STTUNER_USE_TER) || defined (STTUNER_USE_SAT) || defined(STTUNER_USE_ATSC_VSB)
        STTUNER_FECRate_t       FECRates;/****Configurable parameter for different code rate***/ 
        #endif
       
        #if  defined(STTUNER_USE_ATSC_VSB) || defined (STTUNER_USE_SAT)
        STTUNER_IQMode_t                      IQMode;/*Put here for 8VSB*/
        #endif
        
        /*******Satellite and Cable related parameters******************/
        #if defined (STTUNER_USE_SAT) || defined (STTUNER_USE_CAB)|| defined(STTUNER_USE_ATSC_VSB) /*Done for 8VSB*/
        U32 		       SymbolRate;/**** Stores the symbol rate of particular SAT or CAB transmission**/
        #endif
               
        #ifdef STTUNER_MINIDRIVER
        U32 Frequency;
        U32                  FrequencyLO;
        STTUNER_DownLink_t   DownLink;
	#endif
    }
    STTUNER_Scan_t;

    /* Scan list (common) */
    typedef struct STTUNER_ScanList_s
    {
        U32             NumElements;
        STTUNER_Scan_t *ScanList;
    } STTUNER_ScanList_t;


    /* driver instance current configuration & capability (common) */
    typedef struct STTUNER_Capability_s
    {
        STTUNER_Device_t     Device;
        STTUNER_Modulation_t Modulation;
        STTUNER_FECRate_t    FECRates;  /*  (sat & terr) */
        BOOL AGCControl;
        U32  SymbolMin;
        U32  SymbolMax;
        U32  BerMax;
        U32  SignalQualityMax;
        U32  AgcMax;

        STTUNER_DemodType_t     DemodType;
        STTUNER_TunerType_t     TunerType;

        /*  (sat) */
        STTUNER_LnbType_t       LnbType;
        STTUNER_Polarization_t  Polarization;
        
        /*  (cable) */
        STTUNER_J83_t           J83;            /* (cab) */
	    /*  (terrestrial) */
        STTUNER_Mode_t          Mode;
        STTUNER_Guard_t         Guard;
        STTUNER_Hierarchy_t     Hierarchy;
        STTUNER_Spectrum_t      Spectrum;        /* (ter & cable) */
        
        STTUNER_FreqOff_t       FreqOff;         /* (ter) */
        S32                     ResidualOffset;  /* (ter) */
        /*scr*/
        BOOL                    SCREnable;   /*fsu addition*/
        STTUNER_ScrType_t       Scr_App_Type;
        U8 NbLnb;
		U16   SCRLNB_LO_Frequencies[8]; /* In MHz*/
		U8 NbScr;
		U16   SCRBPFFrequencies[8]; /* In MHz*/
		STTUNER_SCRBPF_t          SCRBPF   ;        /* SCR indexing***/
	   	STTUNER_SCRLNB_Index_t    LNBIndex ;       /* LNBIndex***/ 
        U32 FreqMin;           /* w.r.t DDTS GNBvd09094 */
        U32 FreqMax;	       /* w.r.t DDTS GNBvd09094 */

    }
    STTUNER_Capability_t;


/* tuner instance information (common) */
    typedef struct STTUNER_TunerInfo_s
    {
    	
        STTUNER_Status_t   Status;      
        STTUNER_Scan_t     ScanInfo;
	U32 Frequency;
        U32 NumberChannels;
        U32 SignalQuality;
        U32 BitErrorRate;
        U32 FrequencyFound;
        S32 RFLevel; /* Terr RF level */
        STTUNER_Hierarchy_Alpha_t Hierarchy_Alpha; /** Hierarchy Alpha level used for reporting**/   
        U32 StandBy_Check;           
    } STTUNER_TunerInfo_t;

/* top level handle */
    typedef U32 STTUNER_Handle_t; /* index into driver device allocation database (common) */

/* Event parameters - for Event Handler only (common) */
    typedef struct STTUNER_EvtParams_s
    {
        STTUNER_Handle_t  Handle;
        ST_ErrorCode_t    Error;
    } STTUNER_EvtParams_t;


/* device I/O selection (common) */
    typedef struct STTUNER_IOParam_s
    {
        STTUNER_IORoute_t   Route;          /* where IO will go */
        STTUNER_IODriver_t  Driver;         /* which IO driver to use IO will go */

        /* specific STAPI driver parameters (e.g. STI2C) */
        ST_DeviceName_t     DriverName;     /* driver name (for Init/Open) if applicable */
        U32                 Address;        /* and final destination        */
    } STTUNER_IOParam_t;

/* DiSEqC Structure that contains fileds used in DiSEqC2.0 s/w implementation*/
typedef struct STTUNER_DiSEqC2_Via_PIO_s
   {
#ifndef ST_OSLINUX
   	STPIO_Compare_t setCmpReg; /* PIO Compare registers used in PIO interrupt */
   	STPIO_Handle_t HandlePIO; /* PIO Handle to be used for PIO interrupt*/
#endif
   	U32 PIOPort; /* PIO port number */
   	U32 PIOPin; /* PIO Pin number */   	
        U32 IgnoreFirstIntr; /* Workaround flag for ignoring first PIO Interrupt */
        U32 FallingTime[MAX_NUM_PULSE_IN_REPLY];/* Array to store the time at falling edge */
        U32 PulseWidth[MAX_NUM_PULSE_IN_REPLY]; /* Array to store the tone pulse width */
        U8 PulseCounter; /* Counter for counting number of pulses in a reply */
        U8 FirstBitRcvdFlag; /* Flag that will indicates arrival of the first pulse*/
        U8 ReplyContinueFlag; /* Flag to indicate that reply is still coming */
   }STTUNER_DiSEqC2_Via_PIO_t;

typedef struct STTUNER_LNB_Via_PIO_s
{
	ST_DeviceName_t PIODeviceName;
	U32 VSEL_PIOPort; /* PIO port number */
   	U32 VSEL_PIOPin; /* PIO Pin number */   
   	U32 VSEL_PIOBit; /* PIO Pin number */   
   	U32 VEN_PIOPort; /* PIO port number */
   	U32 VEN_PIOPin; /* PIO Pin number */  
   	U32 VEN_PIOBit; /* PIO Pin number */  
   	U32 TEN_PIOPort; /* PIO port number */
   	U32 TEN_PIOPin; /* PIO Pin number */
   	U32 TEN_PIOBit; /* PIO Pin number */
}
STTUNER_LNB_Via_PIO_t;

/* STAPI init,open,close & term -------------------------------------------- */

/*DiSEqC init params*/

typedef struct STTUNER_Init_Diseqc_s
{
    U32                  *BaseAddress; /*Base address for accessing the diseqc registers*/
#ifdef ST_OS21
    interrupt_name_t     InterruptNumber; /*Interrupt number for diseqc*/
#else 
    U32                  InterruptNumber;/*Interrupt number for diseqc*/
#endif
    U32                  InterruptLevel; /*interrupt level for diseqc*/
    U32                  PIOPort; /* PIO port number */
    U32                  PIOPinTx; /* PIO Pin number */   
    U32                  PIOPinRx; /* PIO Pin number */   	
}STTUNER_Init_Diseqc_t;

/*Configuration of clock polarity in case of terrestrial*/
typedef enum STTUNER_DataClockPolarity_e{
         STTUNER_DATA_CLOCK_POLARITY_DEFAULT = 0x00,  /*Clock polarity default*/
	STTUNER_DATA_CLOCK_POLARITY_FALLING  ,       /*Clock polarity in rising edge*/
	STTUNER_DATA_CLOCK_POLARITY_RISING  ,     /*Clock polarity in falling edge*/ 
	STTUNER_DATA_CLOCK_NONINVERTED  ,       /*Non inverted*/
	STTUNER_DATA_CLOCK_INVERTED       /*inverted*/   		          
}STTUNER_DataClockPolarity_t;



/* initialization parameters, expand to incorporate cable & terrestrial parameters as needed (common) */
    typedef struct STTUNER_InitParams_s
    {
        /* (common) */
        STTUNER_Device_t Device;    /* kind of device to initalize */

        U32   Max_BandList;         /* max number of elememts in band list   */
        U32   Max_ThresholdList;    /* max size of signal threshold list     */
        U32   Max_ScanList;         /* max size of scan parameter block list */
		ST_DeviceName_t EVT_DeviceName;       /* name required to Init() STEVT for STTUNER events */
        ST_DeviceName_t EVT_RegistrantName;   /* name given to opened instance of EVT_DeviceName by STTUNER */

        ST_Partition_t *DriverPartition;    /* dynamic memory allocation */

        STTUNER_DemodType_t         DemodType;      /* ID/type of device-driver to use */
        STTUNER_IOParam_t           DemodIO;        /* configure driver IO */

        STTUNER_TunerType_t         TunerType;
        STTUNER_IOParam_t           TunerIO;
	STTUNER_DEMOD_t		    Path;

        /* configure demod. Note: not all parameters valid for all devices */
        U32                         ExternalClock;
        STTUNER_TSOutputMode_t      TSOutputMode;
        STTUNER_SerialClockSource_t SerialClockSource;
        STTUNER_SerialDataMode_t    SerialDataMode;
        #if defined (STTUNER_USE_TER) || defined (STTUNER_USE_SAT) || defined(STTUNER_USE_ATSC_VSB)
        STTUNER_FECMode_t           FECMode;        /* (sat & terr) */
        #endif
        
        #if defined(STTUNER_USE_SAT) || defined(STTUNER_USE_CAB)
        STTUNER_BlockSyncMode_t          BlockSyncMode;/*Not used in ADV1 now as its care is taken in the Driver itself */ /*control sync byte*/ /*stripping sync byte*/
        STTUNER_DataClockAtParityBytes_t DataClockAtParityBytes; /*parity byte to be discarded or not*/
        #endif
        
        /*******Satellite related parameters******************/
        #ifdef STTUNER_USE_SAT
        /*  (sat) */
        STTUNER_LnbType_t           LnbType;        /* device-drivers to use */
        STTUNER_IOParam_t           LnbIO;          /* configure driver IO */
        STTUNER_LNB_Via_PIO_t       LnbIOPort;
        
        /* added against bug GNBvd27452, for controlling block sync mode of DVB packets*/
        STTUNER_DataFIFOMode_t     	 DataFIFOMode; /*controls TS output datarate and use of FIFO for dataflow*/
        STTUNER_OutputFIFOConfig_t	 OutputFIFOConfig; /*for programming data clock*/
        STTUNER_TSDataOutputControl_t    TSDataOutputControl;/* if auto it will automatically control o/P clock & DVALID or DCLC puncturing */ 
          #ifdef STTUNER_ADV	
        #if defined(STTUNER_USE_FSK) 
        STTUNER_FSK_ModeParams_t FSKModeParams;/*FSK related parameters*/
        #endif
        #endif
        #ifdef STTUNER_DRV_SAT_SCR
        STTUNER_SCRLNB_Index_t      LNBIndex ;    /*fsu addition*/   
        U8                          SCREnable;    /*fsu addition*/
        STTUNER_ScrType_t           Scr_App_Type;    /*Scr type*/
        STTUNER_IOParam_t           ScrIO; 	/* configure scr driver IO */
        STTUNER_SCRBPF_t            SCRBPF ;    /* SCR indexing***/   
        STTUNER_SCR_Mode_t          SCR_Mode;  /* Scr Installation -> Manual or Auto Mode */
        U8 NbLnb;
		U16   SCRLNB_LO_Frequencies[8]; /* In MHz*/
		U8 NbScr;
		U16   SCRBPFFrequencies[8];  /* In MHz*/
        #endif
        
        #ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO
        STTUNER_DiSEqC2_Via_PIO_t       DiSEqC;  /* Structure that contains the DiSEqC2.0 related fields*/      
        #endif

        
        #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND 
       /* Added for 5100 diseqc */   
        STTUNER_DiSEqCType_t       DiseqcType; 
        STTUNER_Init_Diseqc_t      Init_Diseqc; /* DiseqC2 init param for 5100 */
        STTUNER_IOParam_t           DiseqcIO; 	/* configure diseqc driver IO */
        #endif 
       
        #endif
       #if defined(STTUNER_USE_SAT) || defined (STTUNER_USE_TER)  || defined(STTUNER_USE_ATSC_VSB) || defined (STTUNER_USE_CAB)
       STTUNER_DataClockPolarity_t      ClockPolarity; /*Configuration of clock rising/falling edge*/
       #endif
       
       /*******Cable related parameters******************/
       #ifdef STTUNER_USE_CAB
       STTUNER_SyncStrip_t         SyncStripping; /*Sync Stripping on/off*/
       STTUNER_Sti5518_t           Sti5518; /* Add 5518 specific TS output mode */
       #endif
	   
    }
    STTUNER_InitParams_t;


/* instance open parameters (common) */
    typedef struct STTUNER_OpenParams_s
    {
        void (*NotifyFunction)(STTUNER_Handle_t Handle, STTUNER_EventType_t EventType, ST_ErrorCode_t Error);
    } STTUNER_OpenParams_t;


/* termination parameters (common) */
    typedef struct STTUNER_TermParams_s
    {
        BOOL ForceTerminate;    /* force close() of open handles if TRUE */
    } STTUNER_TermParams_t;

/* Send and Receive strcutures for  DISEqC API */

 /*sat*/

typedef struct
{
STTUNER_DiSEqCCommand_t	DiSEqCCommandType;			/* Command type */
unsigned char  uc_TotalNoOfBytes ;			/* No of Data bytes to be sent including framing and address bytes*/
unsigned char  *pFrmAddCmdData ;			/* Pointer to data to be sent; Data has to be sequentially placed*/
unsigned char  uc_msecBeforeNextCommand;	/* Time gap required in milliseconds (ms) */
} STTUNER_DiSEqCSendPacket_t;

 /*sat*/
typedef struct
{
unsigned char  uc_TotalBytesReceived ;			/* No of Data bytes received including framing  byte*/
unsigned char  *ReceivedFrmAddData ;			/* Pointer to data received; Data has to be sequentially placed*/
unsigned char  uc_ExpectedNoOfBytes;			/* Higher limit of expected bytes from peripheral*/
        } STTUNER_DiSEqCResponsePacket_t;
  
  /*If DTV quiet mode is not defined then define it here*/   
#ifndef STTUNER_ERROR_DTV_I2C_QUIET_MODE
     #define STTUNER_ERROR_DTV_I2C_QUIET_MODE 0x8000F /*Use the same value as defined in  i2c driver(DTV)-STI2C_ERROR_QUIET_MODE*/
#endif   
  typedef enum
	{
		STTUNER_NORMAL_IF_TUNER, 
		STTUNER_NORMAL_LONGPATH_IF_TUNER,
		STTUNER_LONGPATH_IQ_TUNER
		
	}STTUNER_IF_IQ_Mode;
/* functions --------------------------------------------------------------- */

    /* (common) */
    ST_ErrorCode_t STTUNER_Init(ST_DeviceName_t DeviceName,       STTUNER_InitParams_t *InitParams_p);
    ST_ErrorCode_t STTUNER_Term(ST_DeviceName_t DeviceName, const STTUNER_TermParams_t *TermParams_p);

    ST_ErrorCode_t STTUNER_Open(const ST_DeviceName_t DeviceName, const STTUNER_OpenParams_t *OpenParams_p, STTUNER_Handle_t *Handle_p);
    ST_ErrorCode_t STTUNER_Close(STTUNER_Handle_t Handle);

    ST_ErrorCode_t STTUNER_GetBandList      (STTUNER_Handle_t Handle, STTUNER_BandList_t      *BandList_p);
    ST_ErrorCode_t STTUNER_GetScanList      (STTUNER_Handle_t Handle, STTUNER_ScanList_t      *ScanList_p);
    ST_ErrorCode_t STTUNER_GetStatus        (STTUNER_Handle_t Handle, STTUNER_Status_t        *Status_p);
    ST_ErrorCode_t STTUNER_GetThresholdList (STTUNER_Handle_t Handle, STTUNER_ThresholdList_t *ThresholdList_p, STTUNER_QualityFormat_t *QualityFormat_p);
    ST_ErrorCode_t STTUNER_GetTunerInfo     (STTUNER_Handle_t Handle, STTUNER_TunerInfo_t     *TunerInfo_p);

    ST_ErrorCode_t STTUNER_GetCapability(const ST_DeviceName_t DeviceName, STTUNER_Capability_t *Capability_p);
    ST_Revision_t  STTUNER_GetRevision(void);
    ST_Revision_t  STTUNER_GetLLARevision(int demodtype);

    ST_ErrorCode_t STTUNER_SetBandList     (STTUNER_Handle_t Handle, const STTUNER_BandList_t      *BandList_p);
    ST_ErrorCode_t STTUNER_SetScanList     (STTUNER_Handle_t Handle, const STTUNER_ScanList_t      *ScanList_p);
    ST_ErrorCode_t STTUNER_SetThresholdList(STTUNER_Handle_t Handle, const STTUNER_ThresholdList_t *ThresholdList_p, STTUNER_QualityFormat_t QualityFormat);
    ST_ErrorCode_t STTUNER_SetFrequency    (STTUNER_Handle_t Handle, U32 Frequency, STTUNER_Scan_t *ScanParams_p, U32 Timeout);
   
    /**********Standby Mode *******************/
    ST_ErrorCode_t STTUNER_StandByMode     (STTUNER_Handle_t Handle,STTUNER_StandByMode_t PowerMode);
    /*******************************************/
  
    ST_ErrorCode_t STTUNER_DishPositioning(STTUNER_Handle_t Handle, U32 Frequency, STTUNER_Scan_t *ScanParams_p, BOOL DishPosition_ToneOnOff, U32 Timeout);
    
    ST_ErrorCode_t STTUNER_Scan        (STTUNER_Handle_t Handle, U32 FreqFrom, U32 FreqTo, U32 FreqStep, U32 Timeout);
    
    ST_ErrorCode_t STTUNER_BlindScan   (STTUNER_Handle_t Handle, U32 FreqFrom, U32 FreqTo, U32 FreqStep, STTUNER_BlindScan_t *BlindScanParams, U32 Timeout);
    
    ST_ErrorCode_t STTUNER_ScanContinue(STTUNER_Handle_t Handle, U32 Timeout);
    ST_ErrorCode_t STTUNER_Unlock      (STTUNER_Handle_t Handle);

    ST_ErrorCode_t STTUNER_Ioctl(STTUNER_Handle_t Handle, U32 DeviceID, U32 FunctionIndex, void *InParams, void *OutParams);

   /*  (sat) */
   ST_ErrorCode_t STTUNER_SetLNBConfig(STTUNER_Handle_t Handle, STTUNER_LNBToneState_t LNBToneState, STTUNER_Polarization_t LNBVoltage);
   ST_ErrorCode_t STTUNER_DiSEqCSendReceive(STTUNER_Handle_t Handle,
                                          STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
									      STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket);
    /* Sat-SCR*/
   ST_ErrorCode_t STTUNER_ScrTone_Enable(STTUNER_Handle_t Handle); 
  
   ST_ErrorCode_t STTUNER_SCRParams_Update(const ST_DeviceName_t DeviceName, STTUNER_SCRBPF_t SCR,U16* SCRBPFFrequencies);
   /* ter */									     
   ST_ErrorCode_t STTUNER_GetTPSCellId(STTUNER_Handle_t Handle, U16 *TPSCellID);	
   

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_H */

/* End of sttuner.h */

