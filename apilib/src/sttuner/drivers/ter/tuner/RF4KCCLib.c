/* RF44KCCLib.c */



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



/* NOTE: This is a "library" of functions that are used to communicate with the
 RF4000 device.  Each of the functions is described by comments associated with
 the function, however, there is a general scheme.  The functions all return an
 integer where the value of zero indicates an "OK" return; that is, one without
/ errors.  If an error occurs, there will be a return value greater than zero,
/ and the value may contain error type content.
/
/
/	Ver 1.0.0: JW Huss 02/09/06
/		This version has been created from version 1.5 of the "RF4kCCLib" library.
/	It has been modified specifically to operate in the "STAPI" environment of 
/	STMicroelectronics.*/

#include "stcommon.h"
#include "ioreg.h"			

#include "tuntdrv.h"    /* header for this file */	
#include "RF4KCCLib.h"
#include "chip.h"


#define rf4kDelay(X)     STOS_TaskDelayUs( X * 1000 )   

U16  RF4000_Address[RF4000_NBREGS]={
0x0,
0x1,
0x2,
0x3,
0x4,
0x5,
0x6,
0x7,
0x8,
0x9,
0xa,
0xb,
0xc,
0xd,
0xe,
0xf,
0x10,
0x11,
0x12,
0x13,
0x14,
0x15,
0x16,
0x17,
0x18,
0x19,
0x1a,
0x1b,
0x1c,
0x1d,
0x1e,
0x1f,
0x20,
0x21,
0x22,
0x23,
0x24,
0x25,
0x26,
0x27,
0x28,
0x29,
0x2a,
0x2b,
0x2c,
0x2d,
0x2e,
0x2f,
0x30,
0x31,
0x32,
0x33,
0x34,
0x35,
0x36,
0x37,
0x38,
0x39,
0x3a,
0x3b,
0x3c,
0x3d,
0x3e,
0x3f,
0x40,
0x41,
0x42,
0x43,
0x44,
0x45,
0x46,
0x47,
0x48,
0x49,
0x4a,
0x4b,
0x4c
};
/* The following array contains a set of frequency values that are
used to develop the table of f0ctrl values during the power-up.
 The frequencies follow the "FChannel" table in so much as the 
 "bands" change with frequency.  Thus, the end points of the band
 must appear, but be less than the actual value for the next band.
 this will insure that there will be a proper point available for
 for the linear interpolation that will take place between frequencies
 in order to determine the correct f0ctrl to use in the channel change.
 In addition, the table is now an array of structures
 that contain the frequency and a default f0ctrl value.  The
 default f0ctrl value is used in the event of an LNA Cal error,
 to allow a "soft landing" for the user.*/
 
f0ctrlFreqs_td f0ctrlFreqs[NBR_F0CTRL] = {
	{ 150000000, 58, 0 },
	{ 200000000, 43, 0 },
	{ 250000000, 31, 0 },
	{ 300000000, 20, 0 },
	{ 350000000, 11, 0 },
	{ 399999000,  3, 0 },
	{ 400000000, 29, 0 },
	{ 450000000, 25, 0 },
	{ 500000000, 22, 0 },
	{ 550000000, 18, 0 },
	{ 600000000, 15, 0 },
	{ 650000000, 12, 0 },
	{ 700000000, 10, 0 },
	{ 750000000,  8, 0 },
	{ 800000000,  5, 0 },
	{ MAX_FREQ,   3, 0 }
};

tableFLOCF_td tableFLOCF[tableFLOCF_LEN] = {
	{  26875000,  53750000, 64, 1, 7, 1 },
	{  53750000, 107500000, 32, 1, 6, 1 },
	{ 107500000, 215000000, 16, 1, 2, 1 },
	{ 215000000, 430000000,  8, 2, 3, 0 },
	{ 430000000, 860000000,  4, 4, 1, 0 }
};

/* The values in this table are slightly modified from
the spec so that there will be no "gaps": the min frequency
 is now placed into the max frequency of the previous entry.
The compare is now based as follows:  minN <= N < maxN, and
a separate test will be made for the last maxN.*/
tableNRVSNC_td tableNRVSNC[tableNRVSNC_LEN] = {
	{ 1456, 2433, 0 },
	{ 2433, 3441, 1 },
	{ 3441, 4866, 2 },
	{ 4866, 6880, 3 }
};

tableVCOSVFR_td tableVCOSVFR[TABLEVCOSVFR_LEN] = {
	{ 161700000, 174500000, 1 },
	{ 174500000, 187200000, 2 },
	{ 187200000, 202900000, 3 },
	{ 202900000, 211700000, 5 },
	{ 211700000, 226400000, 6 },
	{ 226400000, 242100000, 7 },
	{ 242100000, 250000000, 9 },
	{ 250000000, 264700000, 10 },
	{ 264700000, 280400000, 11 },
	{ 280400000, 289200000, 13 },
	{ 289200000, 302000000, 14 },
	{ 302000000, 316600000, 15 },
	{ 316600000, 324500000, 17 },
	{ 324500000, 338200000, 18 },
	{ 338200000, 352000000, 19 }
};


tableLNACS_td tableLNACS[TABLELNACS_LEN] = {
	{ 150000000, 300000000,  0, 0, 0, 1, 2, 1 },
	{ 300000000, 375000000,  1, 0, 0, 1, 2, 1 },
	{ 375000000, 400000000,  1, 0, 0, 1, 2, 1 },
	{ 400000000, 650000000,  0, 0, 0, 2, 3, 2 },
	{ 650000000, 750000000,  1, 0, 0, 2, 3, 2 },
	{ 750000000, 862000000,  1, 0, 0, 2, 3, 2 }
};


tableWVCCFR_td tableWVCCFR_22[tableFLOCF_LEN] = {
	{ 5, 0x8, 0x2 },	/* A narrow 6 for adjacent band interference (2.600).*/
	{ 6, 0x9, 0xd },	/* Set for 6 MHz width (2.794).*/
	{ 7, 0xa, 0x2 },	/*/ Set for 7 MHz width (3.257).*/
	{ 8, 0xc, 0x5 }		/* Set for 8 MHz width (3.737).*/
};

/* Table for 24 MHz crystal frequency ( IF = 36 ).*/
tableWVCCFR_td tableWVCCFR_24[tableFLOCF_LEN] = {
	{ 5, 0x7, 0x0 },	/* A narrow 6 for adjacent band interference (2.600).*/
	{ 6, 0x8, 0x6 },	/* Set for 6 MHz width (2.794).*/
	{ 7, 0x9, 0x8 },	/* Set for 7 MHz width (3.257).*/
	{ 8, 0xb, 0x5 }		/* Set for 8 MHz width (3.737).*/
};

/* DUT "Chip On" default register values.
These register values are to be written directly to the chip without
 the need for "or'ing" into current register values.
 Table values brought up to "reg map" ver 0.30 on 7/29/05.
 Changed reg 70, bit 7 -> 1, and reg 71, bit 5 -> 1 - Rev 1.5*/
/*U8 temporarily changed to U32*/
U8 DefRF4000Val[RF4000_NBREGS] = {
	
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x03, 0x84, 0x44, 0xc1, 0x12, 0x01, 0x70, 0x48, 0x00, 
		0x80, 0x7f, 0xff, 0x00, 0xcb, 0xa9, 0x33, 0x34, 0x7f, 0x48, 
		0x00, 0x7f, 0xff, 0x00, 0xca, 0x8f, 0x55, 0x7f, 0x39, 0x1c, 
		0x4c, 0x23, 0x00, 0xcd, 0x00, 0xaf, 0xc3, 0xc8, 0x00, 0x30, 
		0x0c, 0x50, 0x00, 0x10, 0x58, 0x03, 0xff, 0x00, 0x05, 0x30, 
		0x83, 0xdc, 0xe4, 0x9f, 0x4b, 0xd2, 0x66, 0x04, 0x64, 0xc2, 
		0x1f, 0x00
};



/* DUT "Chip Off" default register values.
These register values are to be written directly to the chip without
th need for "or'ing" into current register values.
 Table values brought up to "reg map" ver 0.30 on 7/29/05.
 Register 49 is modified to leave the crystal osc, crystal buffer on,
 and Loop Thru Amp on: 08/03/05.*/
U8 chipOffState[NBR_WREG] = {
		0x00, 0x00, 0x04, 0x44, 0x01, 0x00, 0x01, 0x70, 0x08, 0x20, 
		0x60, 0x7f, 0xff, 0x00, 0xcb, 0xa9, 0x33, 0x34, 0x7f ,0x08, 
		0x00, 0x7f, 0xff, 0x00, 0xca, 0x8f, 0x55, 0x7f, 0x39, 0x1c, 
		0x06, 0x23, 0x00, 0xc1, 0x00, 0x2e, 0xc2, 0xc8, 0x00, 0x30, 
		0x08, 0x50, 0x00, 0x10, 0x18, 0x03, 0x00, 0x00, 0x07, 0x30, 
		0x03, 0x5c, 0x64, 0x1c, 0x0b, 0x52, 0x16, 0x04, 0x64, 0xc2, 
		0x00, 0x00 };
		
		
extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];


ST_ErrorCode_t doOneTimeDecComp(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
ST_ErrorCode_t baseBandFltrCal(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, S32 filterBW, S32 thermalMode);


/* This function checks the lock status of LO1 and LO2.
 A return value of 0 indicates a valid lock on both VCOs.
 Otherwise, a 1 is return.*/
/*In sttuner 1 means locked/ 0 means not locked */
BOOL checkLockStatus(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle)
{
	S32 lo1 = 0;
	S32 lo2 = 0;
	

	
	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 

					lo2 = (U8)ChipGetField(IOHandle, FRF4000_DLCK2);
	              lo1 = (U8)ChipGetField(IOHandle, FRF4000_DLCK1); 

		#endif
	#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)

		 	   	   	lo2 = (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_DLCK2);
	              lo1 = (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_DLCK1); 

		 #endif

		
	if ((lo1 > 0) && (lo2 > 0))
	{
		/* The chip is fully locked on both VCOs.*/
		return TRUE;
	}
	else
	{
		/* One or more of the VCOs is not locked.*/
		return FALSE;
	}
}


/* The following function does the "chip on" procedure.  This sets specific
/ registers values, and attempts to lock the 2nd local oscillator, LO2.
/ Zero is returned for success, and a "1" is returned for a "no lock" error
/ condition.  */
ST_ErrorCode_t  chipOn(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U8 temp;
	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372)
	U8 index;
	#endif
	S32 j;
	S32 retVal = 0;		/* Initially set for an "OK" return.*/
	ST_ErrorCode_t Error = ST_NO_ERROR;

    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 

				     
	     	  for (index = 0; index < RF4000_NBREGS; index++)
            {	
                  Error = ChipSetOneRegister(IOHandle, RF4000_Address[index],  DefRF4000Val[index]);
                if (Error != ST_NO_ERROR)
                {
                    DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
                    return( Error );
                }

            }   
	     	   
	          Error |= ChipSetField(IOHandle, FRF4000_OKS2, 1);	
	          Error |= ChipSetField(IOHandle, FRF4000_VSS2, 1); /*changed according to new rfmagic library*/
		
	          rf4kDelay(1);

	          Error |= ChipSetField(IOHandle, FRF4000_VSS2, 0);/*chanegd according to new rfmagic lib*/
	

		#endif
	#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)

		 	   	   Error = STTUNER_IOREG_Reset(DeviceMap, IOHandle,DefRF4000Val,RF4000_Address);

	
	Error |= STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_OKS2, 1);	
	Error |= STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_VSS2, 1); /*changed according to new rfmagic library*/
		
	rf4kDelay(1);

	Error |= STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_VSS2, 0);/*chanegd according to new rfmagic lib*/

		 	 #endif
	
	
	for (j = 0; j < 80; j++)
	{
		#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
				temp = (U8)ChipGetField(IOHandle, FRF4000_DLCK2);     
		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
                temp = (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_DLCK2);
		 	  }
		#endif
		
		if (temp)
		{
			/* Got a lock, so exit.*/
			break;
		}
		else
			rf4kDelay(1);
	}

	
	if (j >= 80)
	{
		retVal = 1;
	}
    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
				Error |= ChipSetField(IOHandle, FRF4000_OKS2, 3);     
		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
                Error |= STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_OKS2, 3);
		 	  }
		#endif
	
	return Error;
}


/* The following function does the "Chip Off". The values that were set for
  the "crystal oscillator output buffer" and "crystal oscillator power" that
  were set during the call to "RF4000_doInit" function are maintained during
  the "chipOff" condition.
  The return value is a 0 indicating success, and 1 if there is an error
  from the Two-Wire-Bus write, however finish all the writes and only report
  the error upon the return.

  This function is used to set the first local oscillator, LO1.
  The function returns a 0 for a successful lock, a 1 for no lock,
  and a 2 for a channel frequency out of range. Allowed values for
  stepSize are: 1000000, 666667, 500000.*/
ST_ErrorCode_t setLO1(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 Fchan, S32 Fxtal, S32 stepSize) 
{
	S32 j,k;
	U32 loDiv	=	16;				/* default value*/
	U32 Fvco	=	0;
	U32 Np		=	0;
	U8 oscs	=	1;				/* default value*/
	U8 cprs	=	0;
	U8 fbps	=	1;				/* default value*/
	U8 lodv	=	2;				/* default value*/
	U8 lcds	=	1;				/* default value*/
	U8 B		=	0;
	U8 A		=	0;
	U8 R		=	0;
	U8 temp	=	 0;
        ST_ErrorCode_t Error = ST_NO_ERROR;
	
	if ((Fchan < (U32)MIN_CHANNEL) || (Fchan > (U32)MAX_CHANNEL))
	{
		
		return ST_ERROR_BAD_PARAMETER;
	}

	if (Fchan == tableFLOCF[tableFLOCF_LEN - 1].chanMax)
	{
		loDiv = tableFLOCF[tableFLOCF_LEN - 1].loDivider;
		fbps  = tableFLOCF[tableFLOCF_LEN - 1].fbps;
		lodv  = tableFLOCF[tableFLOCF_LEN - 1].lodv;
		lcds  = tableFLOCF[tableFLOCF_LEN - 1].lcds;
	}
	else
	{
		for (j = 0; j < tableFLOCF_LEN; j++)
		{
			if ( (tableFLOCF[j].chanMin <= Fchan) && (Fchan < tableFLOCF[j].chanMax) )
			{
				loDiv = tableFLOCF[j].loDivider;
				fbps  = tableFLOCF[j].fbps; 
				lodv  = tableFLOCF[j].lodv;
				lcds  = tableFLOCF[j].lcds;
				break;
			}
		}
	}

	
	Fvco = Fchan * loDiv;

	
	for (j = 0; j < TABLEVCOSVFR_LEN; j++)
	{
		if ( (tableVCOSVFR[j].minF*10 <= Fvco) && (Fvco <= tableVCOSVFR[j].maxF*10) )
		{
			
			oscs = tableVCOSVFR[j].oscs;

			
			for (k=0;k<2;k++)
			{
				oscs -= 1;
				if (oscs <= 0) { oscs = 1; break; }
				else if (oscs == 4)  { oscs = 3;  }
				else if (oscs == 8)  { oscs = 7;  }
				else if (oscs == 12) { oscs = 11; }
				else if (oscs == 16) { oscs = 15; }
			}

			break;
		}
	}

	
	if (Fxtal == 24)
	{
		
		if (stepSize == 1000000)
		{
			Np = Fvco / 1000000;

			
			R = 24; 
		}
		else if (stepSize == 666667)
		{
			Np = Fvco / 666667;

			
			R = 36; 
		}
		else
		{
			Np = Fvco / 500000;

			
			R = 48; 
		}
	}
	else
	{
		
		if (stepSize == 1000000)
		{
			Np = Fvco / 1000000;

			
			R = 22; 
		}
		else if (stepSize == 666667)
		{
			Np = Fvco / 666667;

			
			R = 33;
		}
		else
		{
			Np = Fvco / 500000;

			
			R = 44; 
		}
	}
	
	/* Determine the value of cprs based on the NRVSNC table.
	   A default value of 0 will be used if there
	   is no match found in the table.  First, check for a match
	   to the maximum value.*/
	if ( Np == tableNRVSNC[tableNRVSNC_LEN - 1].maxN )
	{
		/* Extract the value and shift it to the correct bit.*/
		cprs = tableNRVSNC[tableNRVSNC_LEN - 1].cprs;
	}
	else
	{
		for (j = 0; j < tableNRVSNC_LEN; j++)
		{
			if ( (tableNRVSNC[j].minN <= Np) && (Np < tableNRVSNC[j].maxN) )
			{
				/*Extract the value and shift it to the correct bit.*/
				cprs = tableNRVSNC[j].cprs;
				break;
			}
		}
	}

	
	B = (U8)(Np / 32);
	A = (U8)(Np % 32);

	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 

				    Error  = (S32)ChipSetField(IOHandle, FRF4000_OSCS, (S32)oscs);
					Error |= (S32)ChipSetField(IOHandle, FRF4000_CPRS, (S32)cprs);
					Error |= (S32)ChipSetField(IOHandle, FRF4000_FBPS, (S32)fbps);
					Error |= (S32)ChipSetField(IOHandle, FRF4000_LODV, (S32)lodv);
					Error |= (S32)ChipSetField(IOHandle, FRF4000_LCDS, (S32)lcds);
					Error |= (S32)ChipSetField(IOHandle, FRF4000_B,    (S32)B);
					Error |= (S32)ChipSetField(IOHandle, FRF4000_A,    (S32)A);
					Error |= (S32)ChipSetField(IOHandle, FRF4000_R,    (S32)R);
					Error |= (S32)ChipSetField(IOHandle, FRF4000_OKS1, (S32)OKS1_DIV);     
					Error |= ChipSetField(IOHandle, FRF4000_VSS1, 1);
		
	
	             rf4kDelay(1);

					/* Set the trigger bit back to 0.*/
				  Error |= ChipSetField(IOHandle, FRF4000_VSS1, 0) ;

		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)

                 Error  = (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_OSCS, (S32)oscs);
					Error |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_CPRS, (S32)cprs);
					Error |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_FBPS, (S32)fbps);
					Error |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LODV, (S32)lodv);
					Error |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCDS, (S32)lcds);
					Error |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_B,    (S32)B);
					Error |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_A,    (S32)A);
					Error |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_R,    (S32)R);
					Error |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_OKS1, (S32)OKS1_DIV);
				   Error |= STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_VSS1, 1);
		
	
	             rf4kDelay(1);

					/* Set the trigger bit back to 0.*/
				  Error |= STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_VSS1, 0) ;
	 

       #endif
  
	
	




	/*Now, poll register DLCK1, for 80 ms, or until
	/ the value = 1, which indicates a VCO lock.  Report an 
	/ error if a lock doesn't occur.
	/ Changed from analog to digital lock: 8/3/05.*/
	for (j = 0; j < 80; j++)
	{
		#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
				temp =  (U8)ChipGetField(IOHandle, FRF4000_DLCK1);      
		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
                temp =  (U8)STTUNER_IOREG_GetField(DeviceMap, IOHandle, FRF4000_DLCK1); 
		 	  }
		#endif
		
		if (temp == 1)
		{
			/* Got a lock, so exit the loop.*/
			break;
		}
		else 
			rf4kDelay(1);
	}

	/* Check for a "no lock" condition on LO1.  If
	/ found, then set the appropriate error code
	/ for the return.*/
	if (j >= 80)
	{
		
		Error = FE_RF4K_TERM_TIMEOUT;
	}
	
	rf4kDelay(13);

	   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
				Error |= ChipSetField(IOHandle, FRF4000_OKS1, OKS1_OFF) ;     
		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
                Error |= STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_OKS1, OKS1_OFF) ;
		 	  }
		#endif
	

	return Error;
}



ST_ErrorCode_t setLNACal(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 fchan) 
{
	S32					j;
	S32					retVal	= 0;			/* Initially set for an "OK" return.*/
	S32					f0ctrl	= 0;
	U8		lcpl	= 0;
	U8		lcfc	= 0;
	U8		dqcw	= 0;
	U8		lds1	= 0;
	U8		lnen	= 0;
	U8		lds2	= 0;
	U8		tmpUChar;

	
	U8		varBCOR;
	U8		varVGAS;
	U8		varVGAG;
	U8		varBCON;
	U8		varBDBW;
	U8		varLAEN;
	U8		varLAHO;
	ST_ErrorCode_t Error = ST_NO_ERROR;

	
	doOneTimeDecComp(DeviceMap,IOHandle);

	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 

				   varBCOR =  (U8)ChipGetField(IOHandle, FRF4000_BCOR);
					varVGAS =  (U8)ChipGetField(IOHandle, FRF4000_VGAS);
					varVGAG =  (U8)ChipGetField(IOHandle, FRF4000_VGAG);
					varBCON =  (U8)ChipGetField(IOHandle, FRF4000_BCON);
					varBDBW =  (U8)ChipGetField(IOHandle, FRF4000_BDBW);
					varLAEN =  (U8)ChipGetField(IOHandle, FRF4000_LAEN);
					varLAHO =  (U8)ChipGetField(IOHandle, FRF4000_LAHO);
					
					Error   = (S32)ChipSetField(IOHandle, FRF4000_BCOR, (S32)BCOR_VAL);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_VGAS, (S32)VGAS_VAL);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_VGAG, (S32)VGAG_VAL);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_BCON, (S32)BCON_OVR);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_BDBW, (S32)BDBW_VAL);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_LCTW, (S32)LCTW_VAL);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_LCTN, (S32)LCTN_VAL);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_LCGN, (S32)LCGN_VAL);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_LCAW, (S32)LCAW_VAL);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_LAEN, (S32)LAEN_VAL);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_LAHO, (S32)LAHO_VAL);     

		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)

                varBCOR =  (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_BCOR);
					varVGAS =  (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_VGAS);
					varVGAG =  (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_VGAG);
					varBCON =  (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_BCON);
					varBDBW =  (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_BDBW);
					varLAEN =  (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_LAEN);
					varLAHO =  (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_LAHO);
					
					Error   = (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_BCOR, (S32)BCOR_VAL);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_VGAS, (S32)VGAS_VAL);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_VGAG, (S32)VGAG_VAL);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_BCON, (S32)BCON_OVR);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_BDBW, (S32)BDBW_VAL);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCTW, (S32)LCTW_VAL);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCTN, (S32)LCTN_VAL);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCGN, (S32)LCGN_VAL);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCAW, (S32)LCAW_VAL);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LAEN, (S32)LAEN_VAL);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LAHO, (S32)LAHO_VAL);

		#endif
	

	
	if (Error != ST_NO_ERROR )
	{
		return Error;
	}

	
	for (j = 0; j < TABLELNACS_LEN; j++)
	{
		if ( (tableLNACS[j].chanMin <= fchan) && (fchan < tableLNACS[j].chanMax) )
		{
			lcpl = tableLNACS[j].lcpl;
			lcfc = tableLNACS[j].lcfc;
			dqcw = tableLNACS[j].dqcw;
			lds1 = tableLNACS[j].lds1;
			lnen = tableLNACS[j].lnen; 
			lds2 = tableLNACS[j].lds2; 
			break;
		}
	}

	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
				Error   = (S32)ChipSetField(IOHandle, FRF4000_LCPL, (S32)lcpl);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_LCFC, (S32)lcfc);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_DQCW, (S32)dqcw);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_LDS1, (S32)lds1);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_LNEN, (S32)lnen);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_LDS2, (S32)lds2);  
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_LCFR, (S32)LCFR_WSM);
	
	
					if (Error != ST_NO_ERROR)
					{
						return Error;
					}
				
					
					
					if ( ChipSetField(IOHandle, FRF4000_LCAL, 1) != ST_NO_ERROR)
					{
						
						return Error;
					}
				
					
					 rf4kDelay(1);
				
					
					if ( ChipSetField(IOHandle, FRF4000_LCAL, 0) != ST_NO_ERROR)
					{
						
						return Error;
					}   
		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
               Error   = (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCPL, (S32)lcpl);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCFC, (S32)lcfc);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_DQCW, (S32)dqcw);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LDS1, (S32)lds1);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LNEN, (S32)lnen);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LDS2, (S32)lds2); 
				
			    Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCFR, (S32)LCFR_WSM);
	
	
					if (Error != ST_NO_ERROR)
					{
						return Error;
					}
				
					
					
					if ( STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCAL, 1) != ST_NO_ERROR)
					{
						
						return Error;
					}
				
					
					 rf4kDelay(1);
				
					
					if ( STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCAL, 0) != ST_NO_ERROR)
					{
						
						return Error;
					}
		 	  }
		#endif
	

	


	
	for (j = 0; j < 100; j++)
	{
		#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
				tmpUChar = (U8)ChipGetField(IOHandle, FRF4000_LCOM);    
		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
               tmpUChar = (U8)STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_LCOM);
		 	  }
		#endif
		
		if (tmpUChar > 0)
		{
			
			break;
		}
		else
			rf4kDelay(1);
	}

	   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
				f0ctrl = ChipGetField(IOHandle, FRF4000_LCFCR); 
	
	
				tmpF0ctrl = (U8)f0ctrl;
			
				
				retVal   = (S32)ChipSetField(IOHandle, FRF4000_BCOR, (S32)varBCOR);
				retVal  += (S32)ChipSetField(IOHandle, FRF4000_VGAS, (S32)varVGAS);
				retVal  += (S32)ChipSetField(IOHandle, FRF4000_VGAG, (S32)varVGAG);
				retVal  += (S32)ChipSetField(IOHandle, FRF4000_BCON, (S32)varBCON);
				retVal  += (S32)ChipSetField(IOHandle, FRF4000_BDBW, (S32)varBDBW);
				retVal  += (S32)ChipSetField(IOHandle, FRF4000_LAEN, (S32)varLAEN);
				retVal  += (S32)ChipSetField(IOHandle, FRF4000_LAHO, (S32)varLAHO);    
		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
               f0ctrl = STTUNER_IOREG_GetField(DeviceMap,IOHandle, FRF4000_LCFCR); 
	
	
				tmpF0ctrl = (U8)f0ctrl;
			
				
				retVal   = (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_BCOR, (S32)varBCOR);
				retVal  += (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_VGAS, (S32)varVGAS);
				retVal  += (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_VGAG, (S32)varVGAG);
				retVal  += (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_BCON, (S32)varBCON);
				retVal  += (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_BDBW, (S32)varBDBW);
				retVal  += (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LAEN, (S32)varLAEN);
				retVal  += (S32)STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LAHO, (S32)varLAHO);
		 	  }
	   #endif
	

	if (Error != ST_NO_ERROR)
	{
		return Error;
	}

	
	if ( (j < 100) && (tmpUChar == (U8)3) )
	{
		
		Error = ST_NO_ERROR;
	}
	else
	{
		
		if (tmpUChar == (U8)2)
		{
			
			retVal = 2;
		}
		else if (tmpUChar == (U8)1)
		{
			
			retVal = 1;
		}
		else
		{
			
			retVal = 3;
		}
	}

	return Error;
}



ST_ErrorCode_t baseBandFltrCal(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, S32 filterBW, S32 thermalMode)
{
	S32					j;
	S32					retVal		= 0;	/* Initially set for an "OK" return.*/
	S32					xtalFreq	= 0;	/* Zero equals 22 MHz, 1 equals 24 MHz.*/
	U8		rcmd		= 0;
	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372)
	U8 i=0;
	#endif
	U8		rcsp		= 0;
	U8		tmpUChar=0;
	ST_ErrorCode_t Error = ST_NO_ERROR;
	
	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 

			/* Determine the operating crystal/IF frequency.  Get this from
				/ the default register.*/
				for (i=0;i < RF4000_NBREGS;i++ )
				    {
				     if (RF4000_Address [i] == RRF4000_REGISTER19)
				      break;
				    }
				    if (i == RF4000_NBREGS )
					    {
					    i = RF4000_NBREGS-1;
				        }
			    xtalFreq = (S32)(((U8)DefRF4000Val[i] & ~(U8)RCXS_CLR) >> (S32)RCXS_BIT );
				 	
				Error   = (S32)ChipSetField( IOHandle,FRF4000_BCON, (S32)BCON_STM);
				Error  |= (S32)ChipSetField( IOHandle, FRF4000_RCTN, (S32)RCTN_DIS);
				Error  |= (S32)ChipSetField( IOHandle, FRF4000_RCRN, (S32)RCRN_DIS);
				Error  |= (S32)ChipSetField( IOHandle, FRF4000_RCOT, (S32)RCOT_DIS);
				Error  |= (S32)ChipSetField( IOHandle, FRF4000_RCTM, (S32)RCTM_DIS);
				Error  |= (S32)ChipSetField( IOHandle, FRF4000_RCON, (S32)RCON_DIS);


		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)

               /* Determine the operating crystal/IF frequency.  Get this from
				/ the default register.*/
				xtalFreq = (S32)(((U8)STTUNER_IOREG_RegGetDefaultVal(DeviceMap,DefRF4000Val,RF4000_Address,RF4000_NBREGS,RRF4000_REGISTER19) & ~(U8)RCXS_CLR) >> (S32)RCXS_BIT);
				
				 	
				Error   = (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle,FRF4000_BCON, (S32)BCON_STM);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCTN, (S32)RCTN_DIS);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCRN, (S32)RCRN_DIS);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCOT, (S32)RCOT_DIS);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCTM, (S32)RCTM_DIS);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCON, (S32)RCON_DIS);


	   #endif
	
	
	
	if (retVal != ST_NO_ERROR)
	{
		return Error;
	}

	
	for (j = 0; j < TABLEWVCCFR_LEN; j++)
	{
		if (xtalFreq == 1)
		{
			
			if (filterBW == tableWVCCFR_24[j].bw)
			{
				rcmd = tableWVCCFR_24[j].rcmd;
				rcsp = tableWVCCFR_24[j].rcsp;
				break;
			}
		}
		else
		{
			if (filterBW == tableWVCCFR_22[j].bw)
			{
				rcmd = tableWVCCFR_22[j].rcmd;
				rcsp = tableWVCCFR_22[j].rcsp; 
				break;
			}
		}
	}
	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
			
					Error   = (S32)ChipSetField(IOHandle, FRF4000_RCMD, (S32)rcmd);
					Error  |= (S32)ChipSetField(IOHandle, FRF4000_RCSP, (S32)rcsp);
						
					if (retVal != ST_NO_ERROR)
					{
						return Error;
					}
				
					
					if (  ChipSetField(IOHandle, FRF4000_RCOT, 1) != ST_NO_ERROR)
					{
						
						return Error;
					}
				
					
					 rf4kDelay(1);
				
					
					if (  ChipSetField(IOHandle, FRF4000_RCOT, 0) != ST_NO_ERROR)
					{
						
						return Error;
					}

		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
	
					Error   = (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCMD, (S32)rcmd);
					Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCSP, (S32)rcsp);
					
						
					if (retVal != ST_NO_ERROR)
					{
						return Error;
					}
				
					
					if (  STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCOT, 1) != ST_NO_ERROR)
					{
						
						return Error;
					}
				
					
					 rf4kDelay(1);
				
					
					if (  STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCOT, 0) != ST_NO_ERROR)
					{
						
						return Error;
					}

		 	  }
	   #endif




	
	for (j = 0; j < 100; j++)
	{
		#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
             	tmpUChar =  (U8)ChipGetField(IOHandle, FRF4000_RCCND);

		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
                 	tmpUChar =  (U8)STTUNER_IOREG_GetField(DeviceMap, IOHandle, FRF4000_RCCND);
		 	  }
	   #endif
	
		if ( tmpUChar )
		{
			
			rf4kDelay(1);
		}
		else
		{
			
			break;
		}
	}

	
	if (j >= 100)
	{
		
		retVal = 1;
	}
	
	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
		{  
             tmpUChar =  (U8)ChipGetField(IOHandle, FRF4000_RCCAL);
             if ( ChipSetField(IOHandle, FRF4000_BCOR, tmpUChar) != ST_NO_ERROR)
				{
					
					return Error;
				}
				if (thermalMode == 1)
				{
					
					Error   = (U8)ChipSetField(IOHandle, FRF4000_RCTN, 1);
					Error  += (U8)ChipSetField(IOHandle, FRF4000_RCTM, 1);
					if (Error !=  ST_NO_ERROR)
					{
						return Error;
					}
				}
				 
				
					

		   }
		#endif
		
		
		
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		   {
                 	tmpUChar =  (U8)STTUNER_IOREG_GetField(DeviceMap, IOHandle, FRF4000_RCCAL);
                

			 	  if ( STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_BCOR, tmpUChar) != ST_NO_ERROR)
					{
						
						return Error;
					}
				
					
					if (thermalMode == 1)
					{
						
						Error   = (U8)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCTN, 1);
						Error  += (U8)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_RCTM, 1);
						if (Error !=  ST_NO_ERROR)
						{
							return Error;
						}
					}
				
					
	      }
	   #endif
	   
return Error;	   
}


/* 	This function is used to power-up the chip.  It calls the "chip on"
/ sequence, but differs since it creates a table of f0ctrl values by
/ repeatedly setting LO1 and doing an LNA Cal for a list of frequencies.
/ 	A return value of 0 indicates a successful result.  Return values 
/ greater than 0 indicate an error, where the error type is encoded
/ in the return value as follows: the 1st 5 bits <4:0> contain an offset
/ into the "f0ctrlFreqs" array, which will provide the frequency at which
/ the failure took place.  The last 2 bits <7:6> contain the error type,
/ where 1 = LO2 lock failure during chip on: 2 = LO1 lock failure at 
/ one of the frequencies in the "f0ctrlFreqs" array: 3 = LNA Cal failure
/ at one of the frequencies in the "f0ctrlFreqs" array: 4 = not assigned.
/ In the case of failures 2 and 3, the frequency of failure is encoded
/ in bits 0 thru 4.  This return value represents the last failure that
/ occurred.  There may be other failures.  See the operational note below.
/ 	OPERATIONAL NOTE: The array "f0ctrlFreqs" is an array of structures where
/ the 1st is the frequency, and the 2nd is a default value of f0ctrl 
/ The 3rd value represents the actual f0ctrl found during this routine.
/ In the case of a type 2 or 3 failure, this default is placed in the
/ "actual" location so that the chip may continue to operate, although not
/ necessarily at the optimum level.*/
ST_ErrorCode_t powerUpSequence(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,S32 fxtal, S32 stepSize, 
											S32 crystalOut, S32 crystalOsc)
{
	S32 j;
	ST_ErrorCode_t Error = ST_NO_ERROR;
	
	for (j=0; j<NBR_F0CTRL; j++)
	{
		f0ctrlFreqs[j].act_f0ctrl = f0ctrlFreqs[j].def_f0ctrl;
	}

	
	Error = chipOn(DeviceMap,IOHandle);

	
	if (Error != ST_NO_ERROR )
	{
		return Error;
	}

	for (j = (NBR_F0CTRL - 1); j >= 0; j--) 
	{
		
		Error = setLO1(DeviceMap,IOHandle, f0ctrlFreqs[j].freq, fxtal, stepSize);

		
		if (Error != ST_NO_ERROR)
		{			
			continue;
		}
		
		
		Error  = setLNACal(DeviceMap,IOHandle, f0ctrlFreqs[j].freq);
		
		
		if (Error != ST_NO_ERROR)
		{			
			Error = setLNACal(DeviceMap,IOHandle, f0ctrlFreqs[j].freq);
		}
		if (Error != ST_NO_ERROR)
		{
			 Error = setLNACal(DeviceMap,IOHandle, f0ctrlFreqs[j].freq);
		}
		if (Error != ST_NO_ERROR)
		{       
			Error = setLNACal(DeviceMap,IOHandle, f0ctrlFreqs[j].freq);
		}
		if (Error != ST_NO_ERROR)
		{
			Error  = setLNACal(DeviceMap,IOHandle, f0ctrlFreqs[j].freq);
		}					   

		
		if (Error != ST_NO_ERROR)
		{
			continue;
		}

		
		f0ctrlFreqs[j].act_f0ctrl = tmpF0ctrl;
	}

	
	return Error;
}


/* this function performs a full channel change.  It utilizes other, but required library
/functions to make the change.  In addition, it performs a "short cut" technique for the
/ LNA Cal sequence: a f0ctrl table look-up, and direct write to the registers of the value.
/ The return value is zero for "OK", and an integer "1" to indicate a no lock condition on
/ the LO1 VCO.  If there is an error in finding the frequency value in the table, then
/ the return value is "2".  If the requested channel frequency is out of range, then
/ the return value is "3".  If there is a Two Wire Bus error, the return is 4.*/
ST_ErrorCode_t setChannel(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 fchan, S32 fxtal, S32 stepSize)
{
	S32 j			= 0;
	S32 retVal		= 1;	/* Initially set for an error return.*/
	S32 band		= 2;	/* Indicates mid or high band, initially set to mid band.*/
	S32 minF0ctrl	= 0;
	S32 maxF0ctrl	= 0;
	U32 tmpInt		= 0;
	U32	minFreq		= 0;
	U32	maxFreq		= 0;
	U8	fctrl		= 0;
	U8	lcpl		= 0;
	U8	lcfc		= 0;
	U8	dqcw		= 0;
	U8	lds1		= 0;
	U8	lnen		= 0;
	U8	lds2		= 0;
        ST_ErrorCode_t Error = ST_NO_ERROR;
	
	if ((fchan < (U32)MIN_CHANNEL) || (fchan > (U32)MAX_CHANNEL))
	{
		return ST_ERROR_BAD_PARAMETER;
	}

	
	Error = setLO1(DeviceMap,IOHandle, fchan, fxtal, stepSize);

	
	if (Error != ST_NO_ERROR)
	{
		
		return Error;
	}

	
	retVal = 2;

	
	for (j = 0; j < TABLELNACS_LEN; j++)
	{
		if ( (tableLNACS[j].chanMin <= fchan) && (fchan < tableLNACS[j].chanMax) )
		{
			lcpl = tableLNACS[j].lcpl;
			lcfc = tableLNACS[j].lcfc;
			dqcw = tableLNACS[j].dqcw;
			lds1 = tableLNACS[j].lds1;
			lnen = tableLNACS[j].lnen;
			lds2 = tableLNACS[j].lds2;
			band = tableLNACS[j].lnen;
			break;
		}
	}

	
	if (fchan > MAX_FREQ)
	{
		
		fchan = MAX_FREQ;
		dqcw = (U8)DEQ_CTRL_VAL;
	}

	
	if (band == MID_BAND)
	{
		
		for (j=0; j < (NBR_MIDBAND - 1); j++)
		{
			if ( (f0ctrlFreqs[j].freq <= fchan) && (fchan <= f0ctrlFreqs[j + 1].freq) )
			{
				
				maxF0ctrl = f0ctrlFreqs[j].act_f0ctrl * 10;
				minF0ctrl = f0ctrlFreqs[j + 1].act_f0ctrl * 10;
				minFreq = f0ctrlFreqs[j].freq;
				maxFreq = f0ctrlFreqs[j + 1].freq;
				retVal = 0;	
				break;
			}
		}
	}
	else
	{
		
		for (j=(NBR_MIDBAND); j < (NBR_F0CTRL - 1); j++)
		{
			if ( (f0ctrlFreqs[j].freq <= fchan) && (fchan <= f0ctrlFreqs[j + 1].freq) )
			{
				
				maxF0ctrl = f0ctrlFreqs[j].act_f0ctrl * 10;
				minF0ctrl = f0ctrlFreqs[j + 1].act_f0ctrl * 10;
				minFreq = f0ctrlFreqs[j].freq;
				maxFreq = f0ctrlFreqs[j + 1].freq;
				retVal = 0;	
				break;
			}
		}
	}

	
	if (retVal > 0)
	{
		
		return retVal;
	}

	
	tmpInt = ((fchan - minFreq) / ((maxFreq - minFreq) / 100));
	fctrl = (U8)((maxF0ctrl - ((tmpInt * (maxF0ctrl - minF0ctrl)) / 100) + 5) / 10);
	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
 
				Error   = (S32)ChipSetField(IOHandle, FRF4000_LCPL, (S32)LCPL_DWN);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_LCFC, (S32)fctrl);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_DQCW, (S32)dqcw);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_LCFR, (S32)LCFR_WSW);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_LDS1, (S32)lds1);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_LNEN, (S32)lnen);
				Error  |= (S32)ChipSetField(IOHandle, FRF4000_LDS2, (S32)lds2);  

		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)

               Error   = (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_LCPL, (S32)LCPL_DWN);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_LCFC, (S32)fctrl);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_DQCW, (S32)dqcw);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_LCFR, (S32)LCFR_WSW);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_LDS1, (S32)lds1);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_LNEN, (S32)lnen);
				Error  |= (S32)STTUNER_IOREG_SetField(DeviceMap, IOHandle, FRF4000_LDS2, (S32)lds2);

		#endif
	

	if (Error != ST_NO_ERROR)
	{
		return Error;
	}

	
	return 0;
}



U32 getChannelFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle)
{
	S32	B		= 0;
	S32	A		= 0;
	S32	R		= 0;
	U32	tmpInt	= 0;
	U32	fcmp	= 0;
	U32	lodv	= 0;
		
	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
 
             	R = ChipGetField(IOHandle,FRF4000_R);/*previously getfieldval*/

		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)

                 	R = STTUNER_IOREG_GetField(DeviceMap,IOHandle,FRF4000_R);/*previously getfieldval*/

		#endif


	switch(R)
	{
		case 24:				/* crystal = 24 MHz.*/
		case 22:				/* crystal = 22 MHz.*/
		default:				/* Just in case...*/
			fcmp = 1000000;
			break;

		case 36:				/*crystal = 24 MHz.*/
		case 33:				/* crystal = 22 MHz.*/
			fcmp = 666667;
			break;

		case 48:				/*crystal = 24 MHz.*/
		case 44:				/* crystal = 22 MHz*/
			fcmp = 500000;
			break;
	}

	  	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
             		B = ChipGetField(IOHandle,FRF4000_B);/*previously getfieldval*/
		
	
					A = ChipGetField(IOHandle,FRF4000_A);/*previously getfieldval*/
					
					
					tmpInt =ChipGetField(IOHandle,FRF4000_LODV);/*previously getfieldval*/
		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
                 	B = STTUNER_IOREG_GetField(DeviceMap,IOHandle,FRF4000_B);/*previously getfieldval*/
		
	
					A = STTUNER_IOREG_GetField(DeviceMap,IOHandle,FRF4000_A);/*previously getfieldval*/
					
					
					tmpInt =STTUNER_IOREG_GetField(DeviceMap,IOHandle,FRF4000_LODV);/*previously getfieldval*/
		 	  }
		#endif
	

	switch(tmpInt)
	{
		case 0:
		case 1:
		case 4:
		case 5:
			lodv = 4;
			break;

		case 3:
			lodv = 8;
			break;

		case 2:
			lodv = 16;
			break;
		
		case 6:
			lodv = 32;
			break;

		case 7:
		default:
			lodv = 64;
			break;
	}
	
	if ((fcmp == 1000000) || (fcmp == 500000))
	{
		return (fcmp / lodv) * ((32 * B) + A);
	}
	else
	{
		
		fcmp = (((fcmp * 10) / lodv) + 5) / 10;
		return fcmp * ((32 * B) + A);
	}
}



ST_ErrorCode_t doOneTimeDecComp(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U8 reg	= 0;
         ST_ErrorCode_t Error = ST_NO_ERROR;

	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 

             		if ( ChipSetField(IOHandle, FRF4000_LCFP, 0) != ST_NO_ERROR)
						{
							
							return Error;
						}

	
					if (ChipSetField(IOHandle, FRF4000_LCSC, 1) != ST_NO_ERROR)
					{
						
						return Error;
					}


		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)

                 	if ( STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCFP, 0) != ST_NO_ERROR)
						{
							
							return Error;
						}

	
					if (STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCSC, 1) != ST_NO_ERROR)
					{
						
						return Error;
					}

		#endif
	
	
	if (reg == 0)
	{
		
		reg = 1;
	}
	else
	{
		reg = 0;
	}

		#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[IOHandle].RepeaterFn != NULL)
			{  
             		if (ChipSetField(IOHandle, FRF4000_LCFP, reg) != ST_NO_ERROR)
					{
						
						return Error;
					}
					
					if (ChipSetField(IOHandle, FRF4000_LCSC, 0) != ST_NO_ERROR)
					{
						
						return Error;
					}

		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[IOHandle].TargetFunction != NULL)
		 	   {
                 	if (ChipSetField(IOHandle, FRF4000_LCFP, reg) != ST_NO_ERROR)
					{
						
						return Error;
					}
					
					if (STTUNER_IOREG_SetField(DeviceMap,IOHandle, FRF4000_LCSC, 0) != ST_NO_ERROR)
					{
						
						return Error;
					}
		 	  }
		#endif
	
	
	return Error;
}


S32 getF0ctrlValue(S32 offset, S32* freqVal)
{
	S32 retVal = 0;
	
	
	if ((offset < 0) || (offset >= (S32)NBR_F0CTRL))
	{
		
		retVal = 1;
	}
	else
	{
		
		*freqVal = (U32)f0ctrlFreqs[offset].act_f0ctrl;
	}
	
	return retVal;
}


/* This is the initialization function.  It defines and sets all
 the chips registers.  It must be called after "open".
 The argument crystalOut, and crystalOsc set the initial state of the
 crystal output buffer and the crystal oscillator, respectively.  For either
 argument, a value of 0 sets the condition to on, and a value of 1 sets the
 condition to off.
 Zero is returned for success, and a "1" is returned for a "no lock" error
 condition on LO2.  */
ST_ErrorCode_t RF4000_doInit(TUNTDRV_InstanceData_t *Instance)
{
	U8	tmpU8	= 0;
	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372)
	U8 i=0;
	#endif
	U32	stepSize = 0;	
	U8 *DefVal;
	ST_ErrorCode_t Error= ST_NO_ERROR;
	stepSize = Instance->StepSize;
	DefVal = DefRF4000Val;
	
	#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
			{  
			     for (i=0;i < RF4000_NBREGS;i++ )
				    {
				     if (RF4000_Address [i] == RRF4000_REGISTER49 )
				      break;
				    }
			    if (i == RF4000_NBREGS )
				    {
				    i = RF4000_NBREGS-1;
				   }
			    tmpU8 = DefRF4000Val[i];
		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
		 	   {
              tmpU8 = (U8)STTUNER_IOREG_RegGetDefaultVal(&(Instance->DeviceMap),DefRF4000Val,RF4000_Address,RF4000_NBREGS,RRF4000_REGISTER49);
		 	  }
		#endif
		
	tmpU8 &= (U8)XRGN_CLR;
	tmpU8 &= (U8)XOBN_CLR;
	
	
	if ((U32)RF4000_CRYSTALBUF == 0)
	{
		
		tmpU8 |= (U8)XOBN_ENA << (S32)XOBN_BIT;
	}
	else
	{
		
		tmpU8 |= (U8)XOBN_DIS << (S32)XOBN_BIT;
	}

	
	if ((U32)RF4000_CRYSTALOSC == 0)
	{
		
		tmpU8 |= (U8)XRGN_ENA << (S32)XRGN_BIT;
	}
	else
	{
		
		tmpU8 |= (U8)XRGN_DIS << (S32)XRGN_BIT;
	}

		#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
			{  
              for (i=0;i < RF4000_NBREGS;i++ )
				    {
				     if (RF4000_Address [i] == RRF4000_REGISTER49 )
				      break;
				    }
			    if (i == RF4000_NBREGS )
				    {
				    i = RF4000_NBREGS-1;
				   }
			    DefRF4000Val[i] = tmpU8;
		   }
		#endif
	   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
		 if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
		 	   {
             STTUNER_IOREG_RegSetDefaultVal(&(Instance->DeviceMap),DefRF4000Val,RF4000_Address,RF4000_NBREGS,RRF4000_REGISTER49, tmpU8);
		 	  }
		#endif
 	
	
	
	
	
	
	if ( powerUpSequence(&(Instance->DeviceMap), Instance->IOHandle,RF4000_FXTAL, stepSize,
								RF4000_CRYSTALBUF, RF4000_CRYSTALOSC)!= ST_NO_ERROR)
	{
		return Error;
	}

	
	if ( baseBandFltrCal(&(Instance->DeviceMap), Instance->IOHandle, Instance->ChannelBW, RF4000_THERM_ON) != ST_NO_ERROR)
	{
		return Error;
	}
	

	
	return 0;
}

