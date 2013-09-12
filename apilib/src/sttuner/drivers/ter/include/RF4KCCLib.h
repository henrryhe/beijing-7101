/* RF44KCCLib.h 
 Version 1.0
 Note: this version number always follows that from RF44KCCLib.c.
 See RF44KCCLib.c for version information.
*/


/****************************************************************************************************************
*****************************************************************************************************************
**                                                                                                             **
** (C) Copyright 2005 RF Magic Inc., All rights reserved                                                       **
**                                                                                                             **
** The information presented here is subject to the RF Magic License agreement.  It is "RF Magic Confidential" **
** and therefore subject to corporate Non Disclosure Agreements.  The software is believed to be accurate and  **
** reliable and may be changed at any time without notice. RF Magic does not warrant the accuracy or           **
** completeness of this software nor documents describing this software. Furthermore, information provided     **
** herein covers RF Magic products only. RF Magic is not responsible for any circuitry used with RF Magic      **
** products. RF Magic assumes no liability whatsoever for this information except as may be provided in        **
** RF Magic's Terms and Conditions of Sale or other negotiated and signed agreements.                          **
**                                                                                                             **
** No circuit licenses or patent licenses, express or implied, by estoppels or otherwise, to any intellectual  **
** property rights, is granted by this software or documentation.                                              **
**                                                                                                             **
*****************************************************************************************************************
****************************************************************************************************************/


/*NOTE: This is a "library" of functions that are used to communicate with the
 RF4000 device.  Each of the functions is described by comments associated with
 the function, however, there is a general scheme.  The functions all return an
 integer where the value of zero indicates an "OK" return; that is, one without
 errors.  If an error occurs, there will be a return value greater than zero,
 and the value will indicate the type of error. 

 Initial code by JW Huss - 2/09/06 */

#ifndef H_RF4KCCLIB
#define H_RF4KCCLIB


/* Exclude for MicroSoft Visual C++ compilation.*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/*#include <utility.h>*/
#include "string.h"

/* Required for STAPI.*/
/*#include "I2c.h"*/
#include "stddefs.h"
/*#include "gen_csts.h"*/

/* Required for RF4000 initial values + library function proto-types.*/
#include "RF4000Init.h"



/* this is a special proto-type that should be replaced by a STAPI
time delay function, when one becomes available.*/
/*void rf4kDelay(S32 t);*/


/* The following define is for the revision number.  The 
 double quotes are necessary.*/
#define VER_NUM		"1.0.0"


/*The following are general defines for the library.*/
#define NBR_WREG		62				/* R/W registers are from 15 to 76.*/
#define MAX_FREQ		860000000		/* Max frequency value used in table.*/
#define LOW_BAND		1				/* Represents the value for the register setting.*/
#define MID_BAND		2				/* Represents the value for the register setting.*/
#define HIGH_BAND		3				/* Represents the value for the register setting.*/
#define DEQ_CTRL_VAL	31				/* A value to use when the frequency is >= MAX_FREQ.*/
#define MIN_CHANNEL		150000000		/* The minimum allowed channel setting: inclusive.*/
#define MAX_CHANNEL		860000000		/* The maximum allowed channel setting: inclusive.*/



typedef struct {
	U32		freq;
	S32		def_f0ctrl;		 /*Default f0ctrl value.*/
	S32		act_f0ctrl;		 /*Actual f0ctrl value found after powerUpSequence.*/
} f0ctrlFreqs_td;

#define NBR_F0CTRL		16			/* Number of f0ctrl values to generate; ie, frequencies.*/
#define NBR_MIDBAND		 6			/* Number of "mid band" frequency values in the table.*/
#define NBR_HIBAND		10			/* Number of "high band" frequency values in the table.*/
#define MIDBAND			150			/* Starting frequency for mid band.*/
#define HIGHBAND		400			/* Starting frequency for high band.*/


	
/* A temporary save location for f0ctrl values for the "powerUpSequence".*/
U8	tmpF0ctrl;


 /*Structure contains the register settings and data for the
 "LO Divider, FBPS, LODV, and LCDS vs. Channel Frequency" table.
 The channel is always greater than or equal to the chanMin, and
 less than the chanMax.*/
typedef struct {
	U32	chanMin;
	U32	chanMax;
	U32	loDivider;
	U8	fbps;
	U8	lodv;
	U8	lcds;
} tableFLOCF_td;

#define tableFLOCF_LEN		5



/* Structure contains the settings for register 51, cprs bits based
 on the "N Range(CPRS vs. N Counter" table.  Min and Max values 
are inclusive.*/
typedef struct {
	U32 minN;
	U32 maxN;
	U8 cprs;
} tableNRVSNC_td;

#define tableNRVSNC_LEN		4




/* Structure contains the VCO starting values for the VCO frequency range.
 The Min and Max values are inclusive.*/
typedef struct {
	signed long int  minF;
	signed long int  maxF;
	U8 oscs;
} tableVCOSVFR_td;

#define TABLEVCOSVFR_LEN		15


/* Define the divide ratio for OKS1 as 2048.
Changed from 2048(0x2) to 512(0x1)*/
#define OKS1_DIV		0x1

/* define the disable state for the LO1 state machine.*/
#define OKS1_OFF		0x3


/*/ The following defines are used in the LNA Calibration sequence,
 and are for register values relative to their respective register
 bit start location.  The "CLR" contains the byte that may be 
 used to clear the desired bits by the logical "AND" technique.
 Some bit defines contain constant values to be used during cal sequence.*/
#define BCOR_BIT		0
#define BCOR_CLR		0x0
#define BCOR_REG		15
#define BCOR_VAL		0xff

#define VGAS_BIT		2
#define VGAS_CLR		0xfb
#define VGAS_REG		16
#define VGAS_VAL		0x1

#define VGAG_BIT		4
#define VGAG_CLR		0x0f
#define VGAG_REG		17
#define VGAG_VAL		0x8

#define BCON_BIT		0
#define BCON_CLR		0xfe
#define BCON_REG		20
#define BCON_OVR		0x1
#define BCON_STM		0x0

#define BDBW_BIT		0
#define BDBW_CLR		0xfc
#define BDBW_REG		21
#define BDBW_VAL		0x2

#define LCPL_BIT		6
#define LCPL_CLR		0xbf
#define LCPL_REG		62
#define LCPL_DWN		0x0
#define LCPL_UP			0x1

#define LCTW_BIT		0
#define LCTW_CLR		0xf8
#define LCTW_REG		63
#define LCTW_VAL		0x1

#define	LCTN_BIT		7
#define LCTN_CLR		0x7f
#define LCTN_REG		65
#define LCTN_VAL		0x0

#define LCGN_BIT		6
#define LCGN_CLR		0xbf
#define LCGN_REG		65
#define LCGN_VAL		0x0

#define LCAW_BIT		3
#define LCAW_CLR		0xc7
#define LCAW_REG		66
#define LCAW_VAL		0x3

#define DQCW_BIT		0
#define DQCW_CLR		0x80
#define DQCW_REG		67

#define LCFR_BIT		7
#define LCFR_CLR		0x7f
#define LCFR_REG		67
#define LCFR_WSW		0x0
#define LCFR_WSM		0x1

#define LNEN_BIT		0
#define LNEN_CLR		0xfc
#define LNEN_REG		68

#define LAEN_BIT		7
#define LAEN_CLR		0x7f
#define LAEN_REG		70
#define LAEN_VAL		0x0

#define LAHO_BIT		6
#define LAHO_CLR		0xbf
#define LAHO_REG		70
#define LAHO_VAL		0x1


/* The following structure is used to contain specific information
 about LNA calibration sequence control settings.  The search criteria
 is that the channel frequency is always greater than or equal to the
 chanMin, and less than the chanMax.*/
typedef struct {
	U32 chanMin;
	U32 chanMax;
	U8 lcpl;
	U8 lcfc;
	U8 dqcw;
	U8 lds1;
	U8 lnen;
	U8 lds2;
} tableLNACS_td;


/* this table has been modified for the Rev B chip;
 ie, 375 to 400, and 400 to 650.*/
#define TABLELNACS_LEN		6



/* These defines are for the baseband filter calibration input 
 parameter sequence, and contain register values
 relative to their respective register bit start location.
 The "CLR" contains the byte that may be used to clear the desired 
 bits by the logical "AND" technique.  Some bit defines contain 
 constant values to be used during the cal sequence.*/
#define RCTN_BIT		6
#define RCTN_CLR		0xbf
#define RCTN_REG		21
#define RCTN_DIS		0x0
#define RCTN_ENA		0x1

#define RCRN_BIT		5
#define RCRN_CLR		0xdf
#define RCRN_REG		21
#define RCRN_DIS		0x0

#define RCOT_BIT		4
#define RCOT_CLR		0xef
#define RCOT_REG		21
#define RCOT_DIS		0x0

#define RCTM_BIT		3
#define RCTM_CLR		0xf7
#define RCTM_REG		21
#define RCTM_DIS		0x0
#define RCTM_ENA		0x1

#define RCON_BIT		2
#define RCON_CLR		0xfb
#define RCON_REG		21
#define RCON_DIS		0x0

#define RCMD_BIT		4
#define RCMD_CLR		0x0f
#define RCMD_REG		22



typedef struct {
	int bw;
	U8 rcmd;
	U8 rcsp;
} tableWVCCFR_td;

#define TABLEWVCCFR_LEN		4




/* The following defines are for registers and bits that
effect the crystal oscillator, buffer selection, and
LO2 crystal frequency selections.*/
#define RCXS_BIT		7		/*Select crystal frequency of 22 or 24*/
#define RCXS_CLR		0x7f
#define RCXS_REG		19
#define RCXS_22M		0
#define RCXS_24M		1

#define XRGN_BIT		0		/* crystal oscillator on or off*/
#define XRGN_CLR		0xfe
#define XRGN_REG		49
#define XRGN_ENA		0
#define XRGN_DIS		1

#define XOBN_BIT		3		/* crystal output buffer on or off*/
#define XOBN_CLR		0xf7
#define XOBN_REG		49
#define XOBN_ENA		0
#define XOBN_DIS		1

 
/*The next define is to control the spectral inversion bit.*/
#define AINV_BIT		4
#define AINV_CLR		0xef
#define AINV_REG		25
#define AINV_NIV		0
#define AINV_INV		1


/*Add/Delete  error code **/
typedef enum
	{
	    FE_RF4K_NO_ERROR,
	    FE_RF4K_INVALID_HANDLE,
	    FE_RF4K_BAD_PARAMETER,
	    FE_RF4K_MISSING_PARAMETER,
	    FE_RF4K_ALREADY_INITIALIZED,
	    FE_RF4K_I2C_ERROR,
	    FE_RF4K_SEARCH_FAILED,
	    FE_RF4K_TRACKING_FAILED,
	    FE_RF4K_TERM_FAILED,
	    FE_RF4K_TERM_TIMEOUT
	} FE_RF4K_Error_t;





/* Prototype functions from the RF4KCLib.lib library.*/
	 ST_ErrorCode_t chipOn(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);

	 ST_ErrorCode_t setLO1(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 Fchan, S32 Fxtal, S32 stepSize);

	 ST_ErrorCode_t setLNACal(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 fchan) ;

	 ST_ErrorCode_t baseBandFltrCal(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, S32 ctrFreq, S32 thermalMode);

	 ST_ErrorCode_t powerUpSequence(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, S32 fxtal, S32 stepSize, 
													S32 crystalOut, S32 crystalOsc);

	ST_ErrorCode_t setChannel(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 fchan, S32 fxtal, S32 stepSize);


	 U32 getChannelFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle);

	 U32 RF4000_doInit(TUNTDRV_InstanceData_t *Instance);
	
	 S32 checkLockStatus(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle);



#endif


