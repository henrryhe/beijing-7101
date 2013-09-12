/* ----------------------------------------------------------------------------
File Name: tunsdrv.c

Description: external tuner drivers.

Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 3-July-2001
version: 3.1.0
 author: GJP
comment: write for multi-instance.
    
   date: 17-August-2001
version: 3.1.1
 author: GJP
comment: update SubAddr to U16

Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */


/* C libs */
#ifndef STTUNER_MINIDRIVER
#ifndef ST_OSLINUX
   #include <string.h>
#endif
#include "stlite.h"     /* Standard includes */

/* STAPI */
#include "sttbx.h"
#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#include "sdrv.h"       /* utilities */
#include "chip.h"


#include "sioctl.h"     /* data structure typedefs for all the the sat ioctl functions */

#include "tunsdrv.h"       /* header for this file */
#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))

#define TUNSDRV_IO_TIMEOUT 10


#if defined(STTUNER_DRV_SAT_TUN_MAX2116)
static U32 MAX2116_LOOKUP[10][4]=	{                                       /* low           high	       vco            div/2 */
										{850000,	931600,		4,		0},    
										{931600,	1037000,	5,		0},    
										{1037000,	1126000,	6,		0},    
										{1126000,	1217000,	0,		1},    
										{1217000,	1356000,	1,		1},
										{1356000,	1513000,	2,		1},
										{1513000,	1671000,	3,		1},
										{1671000,	1864000,	4,		1},
										{1864000,	2072000,	5,		1},
										{2072000,	2247000,	6,		1}
									};
#endif


#ifdef STTUNER_DRV_SAT_TUN_IX2476
static U32 IX2476_LOOKUP[8][4]=		{/* low			high	  vco   div/2 */
										{950000,	1065000,	6,		1},    
										{1065000,	1170000,	7,		1},    
										{1170000,	1300000,	1,		0},
										{1300000,	1445000,	2,		0},
										{1445000,	1607000,	3,		0},
										{1607000,	1778000,	4,		0},
										{1778000,	1942000,	5,		0},
										{1942000,	2150000,	6,		0}
									};
#endif
#ifdef  STTUNER_DRV_SAT_TUN_VG0011
static U32 TUNERSAT_LOOKUP[4][5]=	{/* low			high		div4	rdiv	presc32 */
					{950000,	992000,		1,	1,		0},
					{992000,	1300000,	1,  	1,		1},
					{1300000,	1984000,	0,  	2,		0},
					{1984000,	2150000,	0,	2,		1}
					};
#endif

#ifdef STTUNER_DRV_SAT_TUN_IX2476
/* Default values for Sharp IX2476 tuner registers	*/
U8 DefIX2476Val[IX2476_NBREGS]=		{0x09,0x60,0xf5,0x2c,0x00};
U16 IX2476_AddressArray[HZ1184_NBREGS]={0x01,0x02,0x03,0x04,0x05};
#endif
#ifdef STTUNER_DRV_SAT_TUN_VG0011
/* Default values for tuner sat */
U8 DefTUNERSATVal[TUNERSAT_NBREGS]= {0x07,0xa3,0x92,0x37,0x05,0x00,0x00,0x05};
U16 TUNERSAT_AddressArray[TUNERSAT_NBREGS]   =       {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
#endif

#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
U8 DefTUNERMAX2116Val[MAX2116_NBREGS]= 	 {0x00,0x04,0x77,0x3e,0x4f,0x43,0x0c,0x14,0x4f};
U16 MAX2116_AddressArray[MAX2116_NBREGS]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
#endif

#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
U8 DefTUNERHZ1184Val[HZ1184_NBREGS]=   { 0x22,0x10,0x86,0x8a,0x00};
U16 HZ1184_AddressArray[HZ1184_NBREGS]={0x01,0x02,0x03,0x04,0x05};
#endif




#if defined(STTUNER_DRV_SAT_TUN_STB6000)
#ifndef STTUNER_DRV_SAT_TUN_STB6100
static U32 STB6K_LOOKUP[11][3]=		{                                         /* low         high	      osm */
										{950000,	1000000,	0xA},
										{1000000,	1075000,	0xC},  
										{1075000,	1200000,	0x0},  
										{1200000,	1300000,	0x1},
										{1300000,	1370000,	0x2},
										{1370000,	1470000,	0x4},
										{1470000,	1530000,	0x5},
										{1530000,	1650000,	0x6},
										{1650000,	1800000,	0x8},
										{1800000,	1950000,	0xA},
										{1950000,	2150000,	0xC}
									};
#endif
#endif



#if defined(STTUNER_DRV_SAT_TUN_STB6110)
 U8 DefSTB6110Val[STB6110_NBREGS]={ 0xdF,0x33,0xf2,0xC6,0x17,0x01,0x04,0x1E };   
U16 STB6110_AddressArray[STB6110_NBREGS] ={0x00,0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
#endif

#ifdef STTUNER_DRV_SAT_TUN_STB6000
#ifndef STTUNER_DRV_SAT_5188
	/* reference divider is set to 4 */ 
U8 DefSTB6000Val[STB6000_NBREGS]={0x01,0x7a,0x3c,0x6c,0x04,0x07,0x9e,0xdf,0xd0,0x50,0xfb,0x4f,0x81};
#else
	/* reference divider is set to 30 to keep 1Mhz tuner step */  
U8 DefSTB6000Val[STB6000_NBREGS]={0x01,0x7a,0x3c,0x6c,0x1e,0x07,0x9e,0xdf,0xd0,0x50,0xfb,0xcf,0x81};	
#endif
U16 STB6000_AddressArray[STB6000_NBREGS] ={0x00,0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,0x0c};
#endif



#ifdef  STTUNER_DRV_SAT_TUN_STB6100				
static U32 STB6K_LOOKUP[11][3]=		{                                         /* low         high	      osm */
										{950000,		1000000,	0xA,},
										{1000000,	1075000,	0xC,  },
										{1075000,	1200000,	0x0,  },
										{1200000,	1300000,	0x1,},
										{1300000,	1370000,	0x2,},
										{1370000,	1470000,	0x4,},
										{1470000,	1530000,	0x5,},
										{1530000,	1650000,	0x6,},
										{1650000,	1800000,	0x8,},
										{1800000,	1950000,	0xA,},
										{1950000,	2150000,	0xC}
									};
#endif




#ifdef STTUNER_DRV_SAT_TUN_STB6100
U16 STB6100_AddressArray[STB6100_NBREGS] ={0x00,0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
U8 STB6100_DefVal[STB6100_NBREGS] = {  0x81,0x66,0x39,0xd0,0x3c,0x3c,0xd4,/*0xdc*/0xcc,0x8f,0x0d,0xfb,0xde};
/*U32 STB6100_DefVal[STB6100_NBREGS] = {  0x81, 0x60, 0x58,  0xc7,  0x39,  0x3b, 0xcd, 0xdc,  0x8f,  0xd, 0xfb, 0xde };	*/
#endif


/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */


static BOOL Installed = FALSE;

	static BOOL Installed_VG1011  = FALSE;
	static BOOL Installed_S68G21  = FALSE;
	static BOOL Installed_TUA6100 = FALSE;
	static BOOL Installed_EVALMAX = FALSE;
	static BOOL Installed_VG0011  = FALSE;
	static BOOL Installed_HZ1184  = FALSE;
	static BOOL Installed_MAX2116 = FALSE;
	static BOOL Installed_DSF8910 = FALSE;
	static BOOL Installed_IX2476 = FALSE;

	static BOOL Installed_STB6000 = FALSE;
	static BOOL Installed_STB6110 = FALSE;

static BOOL Installed_STB6100 = FALSE;

/* instance chain, the default boot value is invalid, to catch errors */
static TUNSDRV_InstanceData_t *InstanceChainTop = (TUNSDRV_InstanceData_t *)0x7fffffff;
extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];

/* functions --------------------------------------------------------------- */
/* API */

ST_ErrorCode_t tuner_tunsdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams);
ST_ErrorCode_t tuner_tunsdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams);

ST_ErrorCode_t tuner_tunsdrv_Open(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Handle_t *Handle);

#ifdef  STTUNER_DRV_SAT_TUN_VG1011
ST_ErrorCode_t tuner_tunsdrv_Open_VG1011 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_SAT_TUN_S68G21
ST_ErrorCode_t tuner_tunsdrv_Open_S68G21 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_SAT_TUN_TUA6100
ST_ErrorCode_t tuner_tunsdrv_Open_TUA6100(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_SAT_TUN_EVALMAX
ST_ErrorCode_t tuner_tunsdrv_Open_EVALMAX(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef  STTUNER_DRV_SAT_TUN_VG0011
ST_ErrorCode_t tuner_tunsdrv_Open_VG0011(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams,  TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
ST_ErrorCode_t tuner_tunsdrv_Open_HZ1184(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
ST_ErrorCode_t tuner_tunsdrv_Open_MAX2116(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_SAT_TUN_DSF8910
ST_ErrorCode_t tuner_tunsdrv_Open_DSF8910(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_SAT_TUN_IX2476
ST_ErrorCode_t tuner_tunsdrv_Open_IX2476(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 

#ifdef STTUNER_DRV_SAT_TUN_STB6000
ST_ErrorCode_t tuner_tunsdrv_Open_STB6000(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_SAT_TUN_STB6110
ST_ErrorCode_t tuner_tunsdrv_Open_STB6110(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 

#ifdef STTUNER_DRV_SAT_TUN_STB6100
ST_ErrorCode_t tuner_tunsdrv_Open_STB6100(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 

ST_ErrorCode_t tuner_tunsdrv_Close(TUNER_Handle_t Handle, TUNER_CloseParams_t *CloseParams);

ST_ErrorCode_t tuner_tunsdrv_SetFrequency (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency);
ST_ErrorCode_t tuner_tunsdrv_GetStatus    (TUNER_Handle_t Handle, TUNER_Status_t *Status);
ST_ErrorCode_t tuner_tunsdrv_IsTunerLocked(TUNER_Handle_t Handle, BOOL *Locked);
ST_ErrorCode_t tuner_tunsdrv_SetBandWidth (TUNER_Handle_t Handle, U32 BandWidth, U32 *NewBandWidth);


/* I/O API */
ST_ErrorCode_t tuner_tunsdrv_ioaccess(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle,
    STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* access device specific low-level functions */
ST_ErrorCode_t tuner_tunsdrv_ioctl(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);


/* local functions --------------------------------------------------------- */

ST_ErrorCode_t STTUNER_DRV_TUNER_TUNSDRV_Install(  STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType);
ST_ErrorCode_t STTUNER_DRV_TUNER_TUNSDRV_UnInstall(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType);
BOOL Tuner_GetStatus(TUNSDRV_InstanceData_t *Instance);
ST_ErrorCode_t Tuner_Read(TUNSDRV_InstanceData_t *Instance);
int checkrange(long freqstart,long frequency,long freqend);

static __inline U32 CalculateSteps    (TUNSDRV_InstanceData_t *Instance);

static __inline U32 CalculateFrequency(TUNSDRV_InstanceData_t *Instance);

TUNSDRV_InstanceData_t *TUNSDRV_GetInstFromHandle(TUNER_Handle_t Handle);

#if defined (STTUNER_DRV_SAT_TUN_HZ1184)	|| defined(STTUNER_DRV_SAT_TUN_MAX2116)|| defined (STTUNER_DRV_SAT_TUN_IX2476)||defined (STTUNER_DRV_SAT_TUN_STB6110)|| defined(STTUNER_DRV_SAT_TUN_STB6100)	
/*****************************************************
**FUNCTION	::	TUNER_PowOf2
**ACTION	::	Compute  2^n (where n is an integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/
static U32 TUNER_PowOf2(U32 number)
{
	U32 i;
	U32 result=1;
	
	for(i=0;i<number;i++)
		result*=2;
		
	return result;
}
#endif

/*****************************************************
**FUNCTION	::	Tuner_GetStatus
**ACTION	::	To Get the Lock status of PLL.

*****************************************************/

BOOL Tuner_GetStatus(TUNSDRV_InstanceData_t *Instance)
{
    
    BOOL  locked = FALSE;
    #ifdef STTUNER_DRV_SAT_TUN_MAX2116
    U8 u8;
    #endif
 	
	if(&(Instance->DeviceMap))
	{
		Instance->DeviceMap.Error = Tuner_Read(Instance); 
		
		switch(Instance->TunerType)
		{
			#ifdef  STTUNER_DRV_SAT_TUN_VG0011
			case STTUNER_TUNER_VG0011:
				if(!(Instance->DeviceMap.Error))
					locked = ChipGetFieldImage(FTUNERSAT_LOCK,Instance->TunerRegVal);
			break;
			#endif			
			#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
			case STTUNER_TUNER_HZ1184:
				if(!(Instance->DeviceMap.Error))
					locked = ChipGetFieldImage(FHZ1184_LOCK,Instance->TunerRegVal);
			break;
			#endif			
			#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
			case STTUNER_TUNER_MAX2116:
				if(!Instance->DeviceMap.Error) 
				{
					u8 = ChipGetFieldImage(FMAX2116_ADC,Instance->TunerRegVal); 
					locked = ((u8>0) && (u8<7)) ? TRUE : FALSE;
				}
			break;
			#endif
			
			#ifdef STTUNER_DRV_SAT_TUN_IX2476
			case STTUNER_TUNER_IX2476:
				if(!Instance->DeviceMap.Error)
					locked = ChipGetFieldImage(FIX2476_FL,Instance->TunerRegVal);
			break;
			#endif
		
			#ifdef STTUNER_DRV_SAT_TUN_STB6000
			case STTUNER_TUNER_STB6000:
				if(!Instance->DeviceMap.Error)
					locked = ChipGetFieldImage(FSTB6000_LD,Instance->TunerRegVal);
			break;
			#endif
			
			#ifdef STTUNER_DRV_SAT_TUN_STB6110
			case STTUNER_TUNER_STB6110:
				if(!Instance->DeviceMap.Error)
					locked = ChipGetFieldImage(FSTB6110_LOCKPLL,Instance->TunerRegVal);
			break;
			#endif			
					
		
			#ifdef STTUNER_DRV_SAT_TUN_STB6100		
			case STTUNER_TUNER_STB6100:
				if(!Instance->DeviceMap.Error)
					locked = ChipGetFieldImage(FSTB6100_LD,Instance->TunerRegVal);
			break;
			#endif	
			
			default:
				/* nothing to do here */
			break;
		}
	}
	
	return locked;
}

/*****************************************************
**FUNCTION	::	Tuner_Read()
**ACTION	::	To read "read only" registers of tuner

*****************************************************/
ST_ErrorCode_t Tuner_Read(TUNSDRV_InstanceData_t *Instance)
{
	
	ST_ErrorCode_t Error = ST_NO_ERROR;
	
	if(&(Instance->DeviceMap))
	{
		switch(Instance->TunerType)
		{
			#ifdef  STTUNER_DRV_SAT_TUN_VG0011
			case STTUNER_TUNER_VG0011:
			#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
			if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
				{
					Error = ChipGetRegisters(Instance->IOHandle, RTUNERSAT_TUNING_LSB,8, Instance->TunerRegVal);
			    } 
			#endif
			#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
			   
			  
			if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
				{
				  Error = STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, RTUNERSAT_TUNING_LSB,8, Instance->TunerRegVal);
			    }	    
			#endif	
			break;
			#endif
			
			#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
			case STTUNER_TUNER_HZ1184:
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
					 Error = ChipGetRegisters(Instance->IOHandle, RHZ1184_STATUS, 1, Instance->TunerRegVal);
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
						Error = STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, RHZ1184_STATUS, 1, Instance->TunerRegVal);
				    }	    
				#endif			
			break;
			#endif
			
			#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
			case STTUNER_TUNER_MAX2116:
			    #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				    Error = ChipGetRegisters(Instance->IOHandle, RMAX2116_STATUS, 2, Instance->TunerRegVal);
				    } 
				 #endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, RMAX2116_STATUS, 2, Instance->TunerRegVal);
				    }	    
				#endif			
	
			break;
			#endif
			
			#ifdef STTUNER_DRV_SAT_TUN_IX2476
			case STTUNER_TUNER_IX2476:
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				    Error = ChipGetRegisters(Instance->IOHandle,RIX2476_STATUS,1,Instance->TunerRegVal);
				    } 
				 #endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle,RIX2476_STATUS,1,Instance->TunerRegVal);
				    }	    
				#endif	
	
			break;
			#endif
			
			#ifdef STTUNER_DRV_SAT_TUN_STB6000
			case STTUNER_TUNER_STB6000:
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				    Error = ChipGetRegisters(Instance->IOHandle, RSTB6000_LD, 1,Instance->TunerRegVal);
				    } 
				 #endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, RSTB6000_LD, 1,Instance->TunerRegVal);
				    }	    
				#endif	
			
				
			break;
			#endif
			
			#ifdef STTUNER_DRV_SAT_TUN_STB6110
			case STTUNER_TUNER_STB6110:
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				    Error = ChipGetRegisters(Instance->IOHandle, RSTB6110_CTRL1, 1,Instance->TunerRegVal);
				    } 
				 #endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, RSTB6110_CTRL1, 1,Instance->TunerRegVal);
				    }	    
				#endif	

			break;
			#endif
			
			
			
			#ifdef STTUNER_DRV_SAT_TUN_STB6100
			case STTUNER_TUNER_STB6100:
			   #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				    Error = ChipGetRegisters(Instance->IOHandle, RSTB6100_LD, 12,Instance->TunerRegVal);
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, RSTB6100_LD, 12,Instance->TunerRegVal);
				    }	    
				#endif					
			break;
			#endif
			
			default:
			break;
		}
	}
	
	return Error;
}
/*****************************************************
**FUNCTION	::	Tuner_GetFrequency()
**ACTION	::	It will return frequency at which PLL is locked. 

*****************************************************/
U32 Tuner_GetFrequency(TUNSDRV_InstanceData_t *Instance)
{
	
	U32 frequency = 0;
	#if defined (STTUNER_DRV_SAT_TUN_HZ1184  )||defined (STTUNER_DRV_SAT_TUN_STB6000)||defined(STTUNER_DRV_SAT_TUN_IX2476)||defined (STTUNER_DRV_SAT_TUN_MAX2116)
	U32 stepsize=0;
	#endif
	#if defined (STTUNER_DRV_SAT_TUN_HZ1184  )||defined (STTUNER_DRV_SAT_TUN_STB6110)||defined (STTUNER_DRV_SAT_TUN_STB6000)||defined(STTUNER_DRV_SAT_TUN_IX2476)||defined (STTUNER_DRV_SAT_TUN_MAX2116)
	U32 nbsteps=0;
	#endif
	#if defined (STTUNER_DRV_SAT_TUN_HZ1184  )|| defined (STTUNER_DRV_SAT_TUN_VG0011)||defined (STTUNER_DRV_SAT_TUN_STB6100)||defined (STTUNER_DRV_SAT_TUN_STB6000)||defined (STTUNER_DRV_SAT_TUN_STB6110)
	U32 divider = 0;
	#endif
	#if defined (STTUNER_DRV_SAT_TUN_HZ1184  )||defined (STTUNER_DRV_SAT_TUN_STB6100)||defined (STTUNER_DRV_SAT_TUN_STB6000)
	U32 swallow;
	#endif
	
	#if defined(STTUNER_DRV_SAT_TUN_STB6100)|| defined(STTUNER_DRV_SAT_TUN_STB6110)
	U32 psd2;
	#endif
	
	
	if(&(Instance->DeviceMap)	)
	{
		switch(Instance->TunerType)
		{
			#ifdef  STTUNER_DRV_SAT_TUN_VG0011
			case STTUNER_TUNER_VG0011:
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				    ChipGetRegisters(Instance->IOHandle,RTUNERSAT_TUNING_LSB,2, Instance->TunerRegVal);
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RTUNERSAT_TUNING_LSB,2, Instance->TunerRegVal);
				    }	
				 #endif
				
				divider = (ChipGetFieldImage(FTUNERSAT_N_MSB,Instance->TunerRegVal)<<8)+ChipGetFieldImage(FTUNERSAT_N_LSB,Instance->TunerRegVal);

				frequency = divider*1000;
				
			break;
			#endif
			#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
			case STTUNER_TUNER_HZ1184:
								#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				     divider =   (ChipGetField(Instance->IOHandle,FHZ1184_N_HSB)<<10)
				          + (ChipGetField(Instance->IOHandle,FHZ1184_N_MSB)<<3)
				          + ChipGetField(Instance->IOHandle,FHZ1184_N_LSB); 
				     swallow = ChipGetField(Instance->IOHandle,FHZ1184_A);
				     nbsteps = divider*32 + swallow;
				     stepsize = 2*(Instance->Status.Reference / (TUNER_PowOf2(ChipGetField(Instance->IOHandle,FHZ1184_R))));
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					 divider =   (STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FHZ1184_N_HSB)<<10)
				          + (STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FHZ1184_N_MSB)<<3)
				          + STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FHZ1184_N_LSB); 
				     swallow = STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FHZ1184_A);
				     nbsteps = divider*32 + swallow;
				     stepsize = 2*(Instance->Status.Reference / (TUNER_PowOf2(STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FHZ1184_R))));
				    }	    
				#endif		

				
				frequency = (nbsteps * stepsize)/1000 - Instance->Status.IntermediateFrequency;
			break;
			#endif
			#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
			case STTUNER_TUNER_MAX2116:
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				     nbsteps =   (ChipGetField(Instance->IOHandle,FMAX2116_N_MSB)<<8) 
						  + ChipGetField(Instance->IOHandle,FMAX2116_N_LSB);
				
				     stepsize = Instance->Status.Reference / (TUNER_PowOf2(ChipGetField(Instance->IOHandle,FMAX2116_R)+1));	
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
						nbsteps =   (STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FMAX2116_N_MSB)<<8) 
						  + STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FMAX2116_N_LSB);
				
				     stepsize = Instance->Status.Reference / (TUNER_PowOf2(STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FMAX2116_R)+1));	
				    }	    
				#endif		
				frequency = (nbsteps * stepsize)/1000 - Instance->Status.IntermediateFrequency;  
			break;
			#endif
						
			#ifdef STTUNER_DRV_SAT_TUN_IX2476
			case STTUNER_TUNER_IX2476:
			   #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				    nbsteps =  32 * ( (ChipGetField(Instance->IOHandle,FIX2476_N_MSB)<<3) 
						  + ChipGetField(Instance->IOHandle,FIX2476_N_LSB) ) 
						  + ChipGetField(Instance->IOHandle,FIX2476_A);
				
				   stepsize = Instance->Status.Reference / (TUNER_PowOf2(ChipGetField(Instance->IOHandle,FIX2476_REF)+2));
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					  nbsteps =  32 * ( (STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FIX2476_N_MSB)<<3) 
						  + STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FIX2476_N_LSB) ) 
						  + STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FIX2476_A);
				
				    stepsize = Instance->Status.Reference / (TUNER_PowOf2(STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FIX2476_REF)+2));
				    }	    
				#endif		
					
				frequency = (nbsteps * stepsize)/1000 - Instance->Status.IntermediateFrequency;  
			break;
			#endif	
			
			#ifdef STTUNER_DRV_SAT_TUN_STB6000
			case STTUNER_TUNER_STB6000:
				divider =	(ChipGetFieldImage(FSTB6000_N_MSB,Instance->TunerRegVal)<<1) 
							+ ChipGetFieldImage(FSTB6000_N_LSB,Instance->TunerRegVal);
							
				swallow = ChipGetFieldImage(FSTB6000_A,Instance->TunerRegVal);
				nbsteps = divider*16 + swallow;	/* N x P + A ; P=16*/			
				stepsize =  2*(Instance->Status.Reference / ChipGetFieldImage(FSTB6000_R,Instance->TunerRegVal)); /* 2 x Fr / R */
				
				/*frequency = (nbsteps * stepsize)/1000 - hTuner->IF;*/ /* 2 x Fr x (PxN + A)/R */
				frequency = (nbsteps * (stepsize>>(ChipGetFieldImage(FSTB6000_ODIV,Instance->TunerRegVal)+1)))/1000; /* 2 x Fr x (PxN + A)/R */
			break;
			#endif
			
			#ifdef STTUNER_DRV_SAT_TUN_STB6110
			case STTUNER_TUNER_STB6110:
				/****************/				
				divider =	(ChipGetFieldImage(FSTB6110_NDIV_MSB,Instance->TunerRegVal)<<1) 
							+ ChipGetFieldImage(FSTB6110_NDIV_LSB,Instance->TunerRegVal);
				nbsteps = ChipGetFieldImage(FSTB6110_RDIV,Instance->TunerRegVal);	
				psd2=ChipGetFieldImage(FSTB6110_DIV4SEL,Instance->TunerRegVal);
				
				/* N x P + A ; P=16*/	
				frequency = divider * (Instance->Status.Reference /1000);		
				frequency = frequency/(TUNER_PowOf2(nbsteps+psd2));
				frequency /= 4;
				/******************/
			break;
			#endif
						
			
			#ifdef STTUNER_DRV_SAT_TUN_STB6100
			case STTUNER_TUNER_STB6100:
			
				Tuner_Read(Instance);
				swallow=(ChipGetFieldImage(FSTB6100_NF_MSB,Instance->TunerRegVal)<<8)  /*Nf val*/
						+ChipGetFieldImage(FSTB6100_NF_LSB,Instance->TunerRegVal);
				
				/*Ni = floor (fVCO / (fXTAL * P))*/
				divider=ChipGetFieldImage(FSTB6100_NI,Instance->TunerRegVal); /*NI val*/
				psd2=ChipGetFieldImage(FSTB6100_PSD2,Instance->TunerRegVal);
				
				frequency=(((1+psd2)*(Instance->Status.Reference /1000)*swallow)/TUNER_PowOf2(9));
				frequency+=(Instance->Status.Reference /1000) * (divider)*(1+psd2);
				/*Flo=Fxtal*P*(Ni+Nf/2^9) . P=DIV2+1 */
				
				frequency=frequency/((ChipGetFieldImage(FSTB6100_ODIV,Instance->TunerRegVal)+1)*2);
			
			break;
			#endif	
				
			default:
			break;
		}
	}
	
	return frequency;
}
/*****************************************************
**FUNCTION	::	Tuner_GetBandwidth()
**ACTION	::	It will return BW at which tuner LPF filter is programmed. 

*****************************************************/
U32 Tuner_GetBandwidth(TUNSDRV_InstanceData_t *Instance)
{
	U32 bandwidth = 0;
	#if defined (STTUNER_DRV_SAT_TUN_HZ1184)||defined(STTUNER_DRV_SAT_TUN_IX2476)||defined(STTUNER_DRV_SAT_TUN_STB6000)||defined(STTUNER_DRV_SAT_TUN_STB6110)||defined(STTUNER_DRV_SAT_TUN_STB6100) 
	U8 u8 = 0;
	#endif
	if(&(Instance->DeviceMap))
	{
		switch(Instance->TunerType)
		{
			#ifdef  STTUNER_DRV_SAT_TUN_VG0011
			case STTUNER_TUNER_VG0011:
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				    bandwidth = (ChipGetField(Instance->IOHandle,FTUNERSAT_CF)+5)*2000000; /* x2 for ZIF tuner */  	
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					bandwidth = (STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FTUNERSAT_CF)+5)*2000000; /* x2 for ZIF tuner */  	
				    }	    
				#endif		
				
			break;
			#endif
			#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
			case STTUNER_TUNER_HZ1184:
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				     u8 = (ChipGetField(Instance->IOHandle,FHZ1184_PD2) << 3)  +
					(ChipGetField(Instance->IOHandle,FHZ1184_PD3) << 2)  +
					(ChipGetField(Instance->IOHandle,FHZ1184_PD4) << 1)  +
					ChipGetField(Instance->IOHandle,FHZ1184_PD5);
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					  u8 = (STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FHZ1184_PD2) << 3)  +
					(STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FHZ1184_PD3) << 2)  +
					(STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FHZ1184_PD4) << 1)  +
					STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FHZ1184_PD5);
				    }	    
				#endif		

				bandwidth = 2*(10000000 + (u8-2) * 2500000);
			break;
			#endif
			
			#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
			case STTUNER_TUNER_MAX2116:
				Tuner_Read(Instance);
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				     bandwidth = ((Instance->Status.Reference / ChipGetField(Instance->IOHandle,FMAX2116_M)) *
							((40000 + 1451 * ChipGetField(Instance->IOHandle,FMAX2116_F_IN))/10000)) * 2;
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					  bandwidth = ((Instance->Status.Reference / STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FMAX2116_M)) *
							((40000 + 1451 * STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FMAX2116_F_IN))/10000)) * 2;
				    }	    
				#endif		
				/* Below calculation come from max2116 data sheet, will return bandwidth for both I & Q components*/
				
			break;
			#endif
					
			#ifdef  STTUNER_DRV_SAT_TUN_IX2476
			case STTUNER_TUNER_IX2476:
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
						     u8 = (ChipGetField(Instance->IOHandle,FIX2476_PD2) << 3)  +
						(ChipGetField(Instance->IOHandle,FIX2476_PD3) << 2)  +
						(ChipGetField(Instance->IOHandle,FIX2476_PD4) << 1)  +
						ChipGetField(Instance->IOHandle,FIX2476_PD5);
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
							u8 = (STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FIX2476_PD2) << 3)  +
					(STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FIX2476_PD3) << 2)  +
					(STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FIX2476_PD4) << 1)  +
					STTUNER_IOREG_GetField(&(Instance->DeviceMap),Instance->IOHandle,FIX2476_PD5);
					
				    }	    
				#endif		

			bandwidth = 20000000 + (u8-3) * 4000000; /* x2 for ZIF tuner */
			break;
			#endif
		
			#ifdef STTUNER_DRV_SAT_TUN_STB6000
			case STTUNER_TUNER_STB6000:
				u8 = ChipGetFieldImage(FSTB6000_F,Instance->TunerRegVal);
				
				bandwidth = (u8+5)*2000000;	/* x2 for ZIF tuner */	
			break;
			#endif 
			#ifdef STTUNER_DRV_SAT_TUN_STB6110
			case STTUNER_TUNER_STB6110:
				u8 = ChipGetFieldImage(FSTB6110_CF,Instance->TunerRegVal);
				
				bandwidth = (u8+5)*2000000;	/* x2 for ZIF tuner */	
			break;
			#endif 
			
			
			#ifdef STTUNER_DRV_SAT_TUN_STB6100			
			case STTUNER_TUNER_STB6100:
				Tuner_Read(Instance);
				u8 = ChipGetFieldImage(FSTB6100_F,Instance->TunerRegVal);
				
				bandwidth = (u8+5)*2000000;	/* x2 for ZIF tuner BW/2=F+5 Mhz*/
			break;
			#endif	
								
			default:
			break;
		}
	}
	
	return bandwidth;
}


/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_TUNER_TUNSDRV_Install()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_TUNER_TUNSDRV_Install(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c STTUNER_DRV_TUNER_TUNSDRV_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* driver wide bits to init once only */
    if (Installed == FALSE)
    {
        InstanceChainTop = NULL;

        Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);

        
        Installed = TRUE;
    }

    switch(TunerType)
    {
	    #ifdef  STTUNER_DRV_SAT_TUN_VG1011
        case STTUNER_TUNER_VG1011:
		#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:VG1011...", identity));
		#endif
            if (Installed_VG1011 == TRUE)
            {
		#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
		#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_VG1011;
            Tuner->tuner_Open = tuner_tunsdrv_Open_VG1011;
            Installed_VG1011 = TRUE;
            break;
	#endif
	
#ifdef STTUNER_DRV_SAT_TUN_TUA6100
        case STTUNER_TUNER_TUA6100:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:TUA6100...", identity));
#endif
            if (Installed_TUA6100 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TUA6100;
            Tuner->tuner_Open = tuner_tunsdrv_Open_TUA6100;
            Installed_TUA6100 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_SAT_TUN_EVALMAX
        case STTUNER_TUNER_EVALMAX:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:EVALMAX...", identity));
#endif
            if (Installed_EVALMAX == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_EVALMAX;
            Tuner->tuner_Open = tuner_tunsdrv_Open_EVALMAX;
            Installed_EVALMAX = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_SAT_TUN_S68G21
        case STTUNER_TUNER_S68G21:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:S68G21...", identity));
#endif
            if (Installed_S68G21 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_S68G21;
            Tuner->tuner_Open = tuner_tunsdrv_Open_S68G21;
            Installed_S68G21 = TRUE;
            break;
#endif

#ifdef  STTUNER_DRV_SAT_TUN_VG0011
        case STTUNER_TUNER_VG0011:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:VG0011...", identity));
#endif
            if (Installed_VG0011 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_VG0011;
            Tuner->tuner_Open = tuner_tunsdrv_Open_VG0011;
            Installed_VG0011 = TRUE;
            break;
#endif

#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
       case STTUNER_TUNER_HZ1184:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:HZ1184...", identity));
#endif
            if (Installed_HZ1184 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_HZ1184;
            Tuner->tuner_Open = tuner_tunsdrv_Open_HZ1184;
            Installed_HZ1184 = TRUE;
            break;
   #endif
       
       #ifdef  STTUNER_DRV_SAT_TUN_MAX2116     
       case STTUNER_TUNER_MAX2116:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:MAX2116...", identity));
#endif
            if (Installed_MAX2116 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_MAX2116;
            Tuner->tuner_Open = tuner_tunsdrv_Open_MAX2116;
            Installed_MAX2116 = TRUE;
            break;
     #endif
       
       #ifdef STTUNER_DRV_SAT_TUN_DSF8910  
            case STTUNER_TUNER_DSF8910:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:DSF8910...", identity));
#endif
            if (Installed_DSF8910 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DSF8910;
            Tuner->tuner_Open = tuner_tunsdrv_Open_DSF8910;
            Installed_DSF8910 = TRUE;
            break;
      #endif
            
 #ifdef STTUNER_DRV_SAT_TUN_IX2476
            case STTUNER_TUNER_IX2476:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:IX2476...", identity));
#endif
            if (Installed_IX2476 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_IX2476;
            Tuner->tuner_Open = tuner_tunsdrv_Open_IX2476;
            Installed_IX2476 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_SAT_TUN_STB6000
            case STTUNER_TUNER_STB6000:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:STB6000...", identity));
#endif
            if (Installed_STB6000 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_STB6000;
            Tuner->tuner_Open = tuner_tunsdrv_Open_STB6000;
            Installed_STB6000 = TRUE;
            break;
#endif
#ifdef STTUNER_DRV_SAT_TUN_STB6110
            case STTUNER_TUNER_STB6110:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:STB6110...", identity));
#endif
            if (Installed_STB6110 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_STB6110;
            Tuner->tuner_Open = tuner_tunsdrv_Open_STB6110;
            Installed_STB6110 = TRUE;
            break;
#endif


#ifdef STTUNER_DRV_SAT_TUN_STB6100
	case STTUNER_TUNER_STB6100:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s installing sat:tuner:STB6100...", identity));
#endif
            if (Installed_STB6100 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_STB6100;
            Tuner->tuner_Open = tuner_tunsdrv_Open_STB6100;
            Installed_STB6100 = TRUE;
            break;
#endif

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s incorrect tuner index", identity));
#endif
            return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }

    /* map rest of API */
    Tuner->tuner_Init  = tuner_tunsdrv_Init;
    Tuner->tuner_Term  = tuner_tunsdrv_Term;
    Tuner->tuner_Close = tuner_tunsdrv_Close;

    Tuner->tuner_SetFrequency  = tuner_tunsdrv_SetFrequency; 
    Tuner->tuner_GetStatus     = tuner_tunsdrv_GetStatus;
    Tuner->tuner_IsTunerLocked = tuner_tunsdrv_IsTunerLocked;   
    Tuner->tuner_SetBandWidth  = tuner_tunsdrv_SetBandWidth;          

    Tuner->tuner_ioaccess = tuner_tunsdrv_ioaccess;
    Tuner->tuner_ioctl    = tuner_tunsdrv_ioctl;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("ok\n"));
#endif    
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_TUNER_TUNSDRV_UnInstall()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_TUNER_TUNSDRV_UnInstall(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c STTUNER_DRV_TUNER_TUNSDRV_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;


    switch(TunerType)
    {
    	#ifdef  STTUNER_DRV_SAT_TUN_VG1011
        case STTUNER_TUNER_VG1011:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:VG1011\n", identity));
#endif
            Installed_VG1011 = FALSE;
            break;
#endif

	#ifdef STTUNER_DRV_SAT_TUN_TUA6100
        case STTUNER_TUNER_TUA6100:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:TUA6100\n", identity));
#endif
            Installed_TUA6100 = FALSE;
            break;
#endif

	#ifdef STTUNER_DRV_SAT_TUN_EVALMAX
        case STTUNER_TUNER_EVALMAX:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:EVALMAX\n", identity));
#endif
            Installed_EVALMAX = FALSE;
            break;
#endif

	#ifdef STTUNER_DRV_SAT_TUN_S68G21
        case STTUNER_TUNER_S68G21:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:S68G21\n", identity));
#endif
            Installed_S68G21 = FALSE;
            break;
#endif

#ifdef  STTUNER_DRV_SAT_TUN_VG0011
        case STTUNER_TUNER_VG0011:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:VG0011\n", identity));
#endif
            Installed_VG0011 = FALSE;
            break;
#endif

#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
        case STTUNER_TUNER_HZ1184:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:HZ1184\n", identity));
#endif
            Installed_HZ1184 = FALSE;
            break;
#endif

	#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
        case STTUNER_TUNER_MAX2116:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:MAX2116\n", identity));
#endif
            Installed_MAX2116 = FALSE;
            break;
#endif
            #ifdef STTUNER_DRV_SAT_TUN_DSF8910
            case STTUNER_TUNER_DSF8910:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:DSF8910\n", identity));
#endif
            Installed_DSF8910 = FALSE;
            break;
#endif
     
#ifdef STTUNER_DRV_SAT_TUN_IX2476
             case STTUNER_TUNER_IX2476:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:IX2476\n", identity));
#endif
            Installed_IX2476 = FALSE;
            break;
#endif

 #ifdef STTUNER_DRV_SAT_TUN_STB6000
            case STTUNER_TUNER_STB6000:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:STB6000\n", identity));
#endif
            Installed_STB6000 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_SAT_TUN_STB6110
            case STTUNER_TUNER_STB6110:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:STB6110\n", identity));
#endif
            Installed_STB6110 = FALSE;
            break;
#endif



#ifdef STTUNER_DRV_SAT_TUN_STB6100
             case STTUNER_TUNER_STB6100:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s uninstalling sat:tuner:STB6100\n", identity));
#endif
            Installed_STB6100 = FALSE;
            break;
#endif

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s incorrect tuner index", identity));
#endif
            return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }

    /* unmap API */
    Tuner->ID = STTUNER_NO_DRIVER;

    Tuner->tuner_Init  = NULL;
    Tuner->tuner_Term  = NULL;
    Tuner->tuner_Close = NULL;

    Tuner->tuner_SetFrequency  = NULL; 
    Tuner->tuner_GetStatus     = NULL;
    Tuner->tuner_IsTunerLocked = NULL;   
    Tuner->tuner_SetBandWidth  = NULL;          

    Tuner->tuner_ioaccess = NULL;
    Tuner->tuner_ioctl    = NULL;


    if ( 
    	 (Installed_VG1011  == FALSE) &&
         (Installed_TUA6100 == FALSE) &&
         (Installed_EVALMAX == FALSE) &&
         (Installed_VG0011  == FALSE) &&
         (Installed_HZ1184  == FALSE) &&
         (Installed_MAX2116 == FALSE) &&
         (Installed_DSF8910 == FALSE) &&         
         (Installed_S68G21  == FALSE) &&
         (Installed_IX2476  == FALSE) &&
         (Installed_STB6000 == FALSE) &&
         (Installed_STB6110 == FALSE) &&
         (Installed_STB6100 == FALSE)
        )
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s <", identity));
#endif

        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print((">"));
#endif

        InstanceChainTop = (TUNSDRV_InstanceData_t *)0x7ffffffe;
        Installed        = FALSE;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("all tunsdrv drivers uninstalled\n"));
#endif    
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("ok\n"));
#endif    
    return(Error);
}



/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_VG1011_Install()
        STTUNER_DRV_TUNER_S68G21_Install()
        STTUNER_DRV_TUNER_TUA6100_Install()
        STTUNER_DRV_TUNER_EVALMAX_Install()
        STTUNER_DRV_TUNER_VG0011_Install()
        STTUNER_DRV_TUNER_HZ1184_Install()
        STTUNER_DRV_TUNER_MAX2116_Install()
        STTUNER_DRV_TUNER_DSF8910_Install()
        STTUNER_DRV_TUNER_STB6000_Install()
        STTUNER_DRV_TUNER_STB6110_Install()
        
Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
#ifdef  STTUNER_DRV_SAT_TUN_VG1011
ST_ErrorCode_t STTUNER_DRV_TUNER_VG1011_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_VG1011) );
}
#endif
#ifdef STTUNER_DRV_SAT_TUN_S68G21
ST_ErrorCode_t STTUNER_DRV_TUNER_S68G21_Install (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_S68G21) );
}
#endif
#ifdef STTUNER_DRV_SAT_TUN_TUA6100
ST_ErrorCode_t STTUNER_DRV_TUNER_TUA6100_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_TUA6100) );
}
#endif
#ifdef STTUNER_DRV_SAT_TUN_EVALMAX
ST_ErrorCode_t STTUNER_DRV_TUNER_EVALMAX_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_EVALMAX) );
}
#endif
#ifdef  STTUNER_DRV_SAT_TUN_VG0011
ST_ErrorCode_t STTUNER_DRV_TUNER_VG0011_Install (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_VG0011) );
}
#endif
#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
ST_ErrorCode_t STTUNER_DRV_TUNER_HZ1184_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_HZ1184) );
}
#endif
#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
ST_ErrorCode_t STTUNER_DRV_TUNER_MAX2116_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_MAX2116) );
}
#endif
#ifdef STTUNER_DRV_SAT_TUN_DSF8910
ST_ErrorCode_t STTUNER_DRV_TUNER_DSF8910_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_DSF8910) );
}
#endif

#ifdef STTUNER_DRV_SAT_TUN_IX2476
ST_ErrorCode_t STTUNER_DRV_TUNER_IX2476_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_IX2476) );
}
#endif

#ifdef STTUNER_DRV_SAT_TUN_STB6000
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6000_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_STB6000) );
}
#endif
#ifdef STTUNER_DRV_SAT_TUN_STB6110
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6110_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_STB6110) );
}
#endif



#ifdef STTUNER_DRV_SAT_TUN_STB6100
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6100_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_Install(Tuner, STTUNER_TUNER_STB6100) );
}
#endif

/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_VG1011_UnInstall()
        STTUNER_DRV_TUNER_S68G21_UnInstall()
        STTUNER_DRV_TUNER_TUA6100_UnInstall()
        STTUNER_DRV_TUNER_EVALMAX_UnInstall()
        STTUNER_DRV_TUNER_VG0011_UnInstall()
        STTUNER_DRV_TUNER_HZ1184_UnInstall()
        STTUNER_DRV_TUNER_MAX2116_UnInstall()
        STTUNER_DRV_TUNER_DSF8910_UnInstall()
        STTUNER_DRV_TUNER_STB6000_UnInstall()
        STTUNER_DRV_TUNER_STB6110_UnInstall()

Description:
    uninstall a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

#ifdef  STTUNER_DRV_SAT_TUN_VG1011
ST_ErrorCode_t STTUNER_DRV_TUNER_VG1011_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_VG1011) );
}
#endif
#ifdef STTUNER_DRV_SAT_TUN_S68G21
ST_ErrorCode_t STTUNER_DRV_TUNER_S68G21_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_S68G21) );
}
#endif
#ifdef STTUNER_DRV_SAT_TUN_TUA6100
ST_ErrorCode_t STTUNER_DRV_TUNER_TUA6100_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_TUA6100) );
}
#endif
#ifdef STTUNER_DRV_SAT_TUN_EVALMAX
ST_ErrorCode_t STTUNER_DRV_TUNER_EVALMAX_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_EVALMAX) );
}
#endif
#ifdef  STTUNER_DRV_SAT_TUN_VG0011
ST_ErrorCode_t STTUNER_DRV_TUNER_VG0011_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_VG0011) );
}
#endif
#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
ST_ErrorCode_t STTUNER_DRV_TUNER_HZ1184_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_HZ1184) );
}
#endif
#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
ST_ErrorCode_t STTUNER_DRV_TUNER_MAX2116_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_MAX2116) );
}
#endif
#ifdef STTUNER_DRV_SAT_TUN_DSF8910
ST_ErrorCode_t STTUNER_DRV_TUNER_DSF8910_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_DSF8910) );
}
#endif

#ifdef STTUNER_DRV_SAT_TUN_IX2476
ST_ErrorCode_t STTUNER_DRV_TUNER_IX2476_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_IX2476) );
}
#endif

#ifdef STTUNER_DRV_SAT_TUN_STB6000
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6000_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_STB6000) );
}
#endif
#ifdef STTUNER_DRV_SAT_TUN_STB6110
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6110_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_STB6110) );
}
#endif


#ifdef STTUNER_DRV_SAT_TUN_STB6100
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6100_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSDRV_UnInstall(Tuner, STTUNER_TUNER_STB6100) );
}
#endif

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


   
/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_Init()

Description: called for every perdriver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail no driver installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( TUNSDRV_InstanceData_t ));
    
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }

    /* slot into chain */
    if (InstanceChainTop == NULL)
    {
        InstanceNew->InstanceChainPrev = NULL; /* no previous instance */
        InstanceChainTop = InstanceNew;
    }
    else    /* tag onto last data block in chain */
    {
        Instance = InstanceChainTop;

        while(Instance->InstanceChainNext != NULL)
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }
        Instance->InstanceChainNext     = (void *)InstanceNew;
        InstanceNew->InstanceChainPrev  = (void *)Instance;
    }

    InstanceNew->DeviceName          = DeviceName;
    InstanceNew->TopLevelHandle      = STTUNER_MAX_HANDLES;
    InstanceNew->IOHandle            = InitParams->IOHandle;
    InstanceNew->MemoryPartition     = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
    InstanceNew->TunerType           = InitParams->TunerType;
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->ExternalClock = InitParams->ExternalClock;

    switch(InstanceNew->TunerType)
    {
        #ifdef  STTUNER_DRV_SAT_TUN_VG1011
        case STTUNER_TUNER_VG1011:
            InstanceNew->PLLType = TUNER_PLL_5655;
            break;
#endif 
	#ifdef STTUNER_DRV_SAT_TUN_TUA6100
        case STTUNER_TUNER_TUA6100:
            InstanceNew->PLLType = TUNER_PLL_TUA6100;
            break;
#endif 

#ifdef STTUNER_DRV_SAT_TUN_EVALMAX
        case STTUNER_TUNER_EVALMAX:
            InstanceNew->PLLType = TUNER_PLL_5655;
            break;
#endif 

#ifdef STTUNER_DRV_SAT_TUN_S68G21
        case STTUNER_TUNER_S68G21:
            InstanceNew->PLLType = TUNER_PLL_5522;
            break;
#endif 

#ifdef  STTUNER_DRV_SAT_TUN_VG0011
        case STTUNER_TUNER_VG0011:
            InstanceNew->PLLType = TUNER_PLL_ABSENT;
            break;
#endif 

#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
        case STTUNER_TUNER_HZ1184:
            InstanceNew->PLLType = TUNER_PLL_ABSENT;
            break;
#endif 

#ifdef STTUNER_DRV_SAT_TUN_DSF8910
        case STTUNER_TUNER_DSF8910:
            InstanceNew->PLLType = TUNER_PLL_5655;
            break;
#endif 

  #ifdef  STTUNER_DRV_SAT_TUN_MAX2116          
        case STTUNER_TUNER_MAX2116:
            InstanceNew->PLLType = TUNER_PLL_ABSENT;
            break;
#endif 

#ifdef STTUNER_DRV_SAT_TUN_IX2476
        case STTUNER_TUNER_IX2476:
            InstanceNew->PLLType = TUNER_PLL_ABSENT;
            break;
#endif 

 #ifdef STTUNER_DRV_SAT_TUN_STB6000      
        case STTUNER_TUNER_STB6000:
            InstanceNew->PLLType = TUNER_PLL_ABSENT;
            break;
#endif 
#ifdef STTUNER_DRV_SAT_TUN_STB6110      
        case STTUNER_TUNER_STB6110:
            InstanceNew->PLLType = TUNER_PLL_ABSENT;
            break;
#endif 



#ifdef STTUNER_DRV_SAT_TUN_STB6100
        case STTUNER_TUNER_STB6100:
            InstanceNew->PLLType = TUNER_PLL_ABSENT;
            break;
#endif 

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s incorrect tuner index", identity));
#endif
            return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes) for  tuner ID=%d\n", identity, InstanceNew->DeviceName, InstanceNew, sizeof( TUNSDRV_InstanceData_t ), InstanceNew->TunerType ));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(TermParams);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print(("Looking (%s)", Instance->DeviceName));
#endif
    while(1)
    {
        if ( strcmp((char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("]\n"));
#endif
            /* found so now xlink prev and next(if applicable) and deallocate memory */
            InstancePrev = Instance->InstanceChainPrev;
            InstanceNext = Instance->InstanceChainNext;

            /* if instance to delete is first in chain */
            if (Instance->InstanceChainPrev == NULL)
            {
                InstanceChainTop = InstanceNext;        /* which would be NULL if last block to be term'd */
                if(InstanceNext != NULL)
                {
                InstanceNext->InstanceChainPrev = NULL; /* now top of chain, no previous instance */
                }
            }
            else
            {   /* safe to set value for prev instaance (because there IS one) */
                InstancePrev->InstanceChainNext = InstanceNext;
            }

            /* if there is a next block in the chain */            
            if (InstanceNext != NULL)
            {
                InstanceNext->InstanceChainPrev = InstancePrev;
            }   
            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance->TunerRegVal );
            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);
            #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
             STOS_MemoryDeallocate(Instance->MemoryPartition, IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream);
            #endif
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL) 
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                STTBX_Print(("\n%s fail no free handle before end of list\n", identity));
#endif
                SEM_UNLOCK(Lock_InitTermOpenClose);
                return(STTUNER_ERROR_INITSTATE);
        }
        else
        {
            Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        }
        
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;

    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print((" found ok\n"));
#endif

    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   


#ifdef  STTUNER_DRV_SAT_TUN_VG1011
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_VG1011()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_VG1011(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_VG1011()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;

    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        

    Instance = TUNSDRV_GetInstFromHandle(*Handle);
 
    if (Instance->TunerType != STTUNER_TUNER_VG1011)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_VG1011 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;

    Instance->FreqFactor = 1;

    Instance->Status.TunerStep = 125000;
    Instance->Status.IntermediateFrequency = 479500;
    Instance->Status.IQSense = 1;
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif
    
   	#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	 IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TUNSDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
	#endif
    Instance->BandWidth[0] = 36000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x34;
    Instance->IOBuffer[1] = 0x7C;
    Instance->IOBuffer[2] = 0x95;
    Instance->IOBuffer[3] = 0x80;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
             #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				     Error = ChipSetRegisters(Instance->IOHandle,0, Instance->IOBuffer, 4);
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					  Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TUNSDRV_IO_TIMEOUT);
				    }	    
				#endif	
    

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}   
#endif

#ifdef STTUNER_DRV_SAT_TUN_S68G21
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_S68G21()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_S68G21(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_S68G21()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;

    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        

    Instance = TUNSDRV_GetInstFromHandle(*Handle);
     
     #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	 IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TUNSDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
	#endif
    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;
        
    Instance->FreqFactor = 1;

    Instance->Status.TunerStep = 125000;
    Instance->Status.IntermediateFrequency = 479500;
    Instance->Status.IQSense = -1;
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif

    Instance->BandWidth[0] = 36000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x00;
    Instance->IOBuffer[2] = 0xCE;
    Instance->IOBuffer[3] = 0xA1;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

   /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
      subaddress 0, Tx 4 bytes
   */
   
            #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				     Error = ChipSetRegisters(Instance->IOHandle,0, Instance->IOBuffer, 4);
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					   Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TUNSDRV_IO_TIMEOUT);
				    }	    
				#endif	
    
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}   
#endif

#ifdef STTUNER_DRV_SAT_TUN_TUA6100
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_TUA6100()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_TUA6100(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_TUA6100()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;

    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        

    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;
    
    Instance = TUNSDRV_GetInstFromHandle(*Handle);
    #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	 IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TUNSDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
	#endif
    Instance->FreqFactor = 1;

    Instance->Status.TunerStep = 125000;
    Instance->Status.IntermediateFrequency = 0;
    Instance->Status.IQSense = 1;
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif

    Instance->BandWidth[0] = 60000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x00;
    Instance->IOBuffer[1] = 0x0B;
    #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		{
	     Error = ChipSetRegisters(Instance->IOHandle,0, Instance->IOBuffer, 2);
	    } 
	#endif
	#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
	   
	  
	if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
		{
		   Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 2, TUNSDRV_IO_TIMEOUT);
	    }	    
	#endif	
    

    Instance->IOBuffer[0] = 0x02;
    Instance->IOBuffer[1] = 0x1C;
    Instance->IOBuffer[2] = 0x20;
    #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		{
	     Error |= ChipSetRegisters(Instance->IOHandle,0, Instance->IOBuffer, 3);
	    } 
	#endif
	#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
	   
	  
	if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
		{
		    Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 3, TUNSDRV_IO_TIMEOUT);
	    }	    
	#endif	
   

    Instance->IOBuffer[0] = 0x01;
    Instance->IOBuffer[1] = 0x2C;
    Instance->IOBuffer[2] = 0x96;
    Instance->IOBuffer[3] = 0x00;
    #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		{
	     Error = ChipSetRegisters(Instance->IOHandle,0, Instance->IOBuffer, 4);
	    } 
	#endif
	#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
	   
	  
	if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
		{
		   Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TUNSDRV_IO_TIMEOUT);
	    }	    
	#endif	
   

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}   

#endif

#ifdef STTUNER_DRV_SAT_TUN_EVALMAX
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_EVALMAX()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_EVALMAX(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_EVALMAX()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;

    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        

     /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;
     
    Instance = TUNSDRV_GetInstFromHandle(*Handle);
    
        #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	 IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TUNSDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
	#endif

    Instance->FreqFactor = 1;
    Instance->Status.TunerStep = 500000;
    Instance->Status.IntermediateFrequency = 479500;
    Instance->Status.IQSense = 1;
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif

    Instance->BandWidth[0] = 16000;
    Instance->BandWidth[1] = 60000;
    Instance->BandWidth[2] = 0;

    Instance->IOBuffer[0] = 0x08;
    Instance->IOBuffer[1] = 0xC2;
    Instance->IOBuffer[2] = 0x82;
    Instance->IOBuffer[3] = 0x41;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    
     #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		{
	     Error = ChipSetRegisters(Instance->IOHandle,0, Instance->IOBuffer, 4);
	    } 
	#endif
	#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
	   
	  
	if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
		{
		   Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TUNSDRV_IO_TIMEOUT);
	    }	    
	#endif	
    

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}   
#endif

#ifdef  STTUNER_DRV_SAT_TUN_VG0011
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_VG0011()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_VG0011(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
	U32 i;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_VG0011()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;
    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSDRV_GetInstFromHandle(*Handle);
 
    if (Instance->TunerType != STTUNER_TUNER_VG0011)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_VG0011 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;
 
		     #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
						
				        IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = TUNERSAT_NBREGS;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = TUNERSAT_NBFIELDS;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = TUNERSAT_NBREGS -1 ;  /* write only registers */
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,TUNERSAT_NBREGS,sizeof(U8));
					if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
					     Error = ChipSetRegisters(Instance->IOHandle,0,DefTUNERSATVal,(TUNERSAT_NBREGS -1));
				    } 
				#endif
				
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				
					    Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
					    Instance->DeviceMap.Registers = TUNERSAT_NBREGS;
					    Instance->DeviceMap.Fields    = TUNERSAT_NBFIELDS;
					    Instance->DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
					    Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
					    Instance->DeviceMap.Registers = TUNERSAT_NBREGS -1 ;  /* write only registers */
					 if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					     Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, DefTUNERSATVal, TUNERSAT_AddressArray);
				    }	    
				#endif	
				
				
    for(i=0;i<TUNERSAT_NBREGS;i++)
      Instance->TunerRegVal[i] =DefTUNERSATVal[i]; 

   
   


    /* now safe to unlock semaphore */
    SEM_UNLOCK(Lock_InitTermOpenClose);

    Instance->FreqFactor = 1;

    Instance->Status.TunerStep             = 125000;
    Instance->Status.IntermediateFrequency = 0;
    Instance->Status.Reference = 16000000;
    Instance->Status.IQSense               = 1;
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif
    Instance->Status.Bandwidth             = 125000;    /* Select default bandwidth */   
    
    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}   
#endif


#ifdef  STTUNER_DRV_SAT_TUN_IX2476
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_VG0011()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_IX2476(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
	U32 i;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_IX2476()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;
    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSDRV_GetInstFromHandle(*Handle);
 
    if (Instance->TunerType != STTUNER_TUNER_IX2476)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_IX2476 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;
    
		     #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
						
				        IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = IX2476_NBREGS;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = IX2476_NBFIELDS;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart   = RIX2476_NHIGH;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart   = RIX2476_STATUS;     
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = TUNERSAT_NBREGS -1 ;  /* write only registers */
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
					    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,TUNERSAT_NBREGS,sizeof(U8));
						if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
						{
						     Error = ChipSetRegisters(Instance->IOHandle,0,DefIX2476Val,(TUNERSAT_NBREGS -1));
					    } 
				#endif
				
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				
					     Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
					    Instance->DeviceMap.Registers = IX2476_NBREGS;
					    Instance->DeviceMap.Fields    = IX2476_NBFIELDS;
					    Instance->DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
					    Instance->DeviceMap.WrStart   = RIX2476_NHIGH;
					    Instance->DeviceMap.RdStart   = RIX2476_STATUS;     
					    Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
					    Instance->DeviceMap.Registers = TUNERSAT_NBREGS -1 ;  /* write only registers */
						    
						 if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
						{
						     Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, DefIX2476Val, IX2476_AddressArray);
					    }	    
				#endif	
		  
  
    Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition, TUNERSAT_NBREGS, sizeof( U32 ));
   
    for(i=0;i<TUNERSAT_NBREGS;i++)
      Instance->TunerRegVal[i] =DefIX2476Val[i]; 

    
   
    /* now safe to unlock semaphore */
    SEM_UNLOCK(Lock_InitTermOpenClose);

    Instance->FreqFactor = 1;

    Instance->Status.TunerStep             = 125000;
    Instance->Status.IntermediateFrequency = 0;
    Instance->Status.Reference = 4000000;
    Instance->Status.IQSense               = 1;
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif
    Instance->Status.Bandwidth             = 125000;    /* Select default bandwidth */   
    
    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}   
#endif

#ifdef  STTUNER_DRV_SAT_TUN_HZ1184

/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_HZ1184()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_HZ1184(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
	int i;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_HZ1184()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;

    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        

    SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_HZ1184)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_HZ1184 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;
			    #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
									
				            IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = HZ1184_NBREGS ;        /* Write+read register */
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = HZ1184_NBFIELDS;
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = HZ1184_NBREGS - 1;     /* Write only register */
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
					     IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,HZ1184_NBREGS,sizeof(U8));
						if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
						{
						     Error = ChipSetRegisters(Instance->IOHandle,0,DefTUNERHZ1184Val,(HZ1184_NBREGS -1));
					    } 
				#endif
				
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				
					       Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
						    Instance->DeviceMap.Registers = HZ1184_NBREGS ;        /* Write+read register */
						    Instance->DeviceMap.Fields    = HZ1184_NBFIELDS;
						    Instance->DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
						    Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
						    Instance->DeviceMap.Registers = HZ1184_NBREGS - 1;     /* Write only register */
						     
												 if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
												{
												     Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, DefTUNERHZ1184Val, HZ1184_AddressArray);
											    }	    
				#endif


    Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition, HZ1184_NBREGS, sizeof( U32 ));
    
   
    for(i=0;i<HZ1184_NBREGS;i++)
      Instance->TunerRegVal[i] =DefTUNERHZ1184Val[i]; 

  
  

	/* now safe to unlock semaphore */
	SEM_UNLOCK(Lock_InitTermOpenClose);
	

    Instance->FreqFactor = 1;

    Instance->Status.TunerStep             = 125000;
    Instance->Status.IntermediateFrequency = 0;
    Instance->Status.Reference = 4000000;
    Instance->Status.IQSense               = 1;  /* For JCL */
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif
    Instance->Status.Bandwidth             = 125000;    /* Select default bandwidth */


    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}   
#endif

#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_MAX2116()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_MAX2116(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
	U32 i;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_MAX2116()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;

    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        
	SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSDRV_GetInstFromHandle(*Handle);
 
    if (Instance->TunerType != STTUNER_TUNER_MAX2116)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MAX2116 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 925000;
    Capability->FreqMax = 2175000;
    
               #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
									
				           IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = MAX2116_NBREGS; /* Write + Read registers */
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = MAX2116_NBFIELDS;
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = MAX2116_NBREGS - 2; /* Write only registers */
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition; 
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,MAX2116_NBREGS,sizeof(U8));
					    
						if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
						{
						     Error = ChipSetRegisters(Instance->IOHandle,0,DefTUNERMAX2116Val,(MAX2116_NBREGS -2));
					    } 
					    
				#endif
				
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				
					       Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
						    Instance->DeviceMap.Registers = MAX2116_NBREGS ;        /* Write+read register */
						    Instance->DeviceMap.Fields    = MAX2116_NBFIELDS;
						    Instance->DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
						    Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
						    Instance->DeviceMap.Registers = MAX2116_NBREGS - 2;     /* Write only register */
						    
						 if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
						{
						      Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, DefTUNERMAX2116Val, MAX2116_AddressArray);
					    }	    
				#endif
       

   Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition, MAX2116_NBREGS, sizeof( U32 ));
   
   
    for(i=0;i<MAX2116_NBREGS;i++)
      Instance->TunerRegVal[i] =DefTUNERMAX2116Val[i]; 





    /* now safe to unlock semaphore */
    SEM_UNLOCK(Lock_InitTermOpenClose);
    
    Instance->FreqFactor = 1;

    Instance->Status.TunerStep             = 125000;
    Instance->Status.IntermediateFrequency = 0;
    Instance->Status.Reference = 4000000;
    Instance->Status.IQSense               = 1; 
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif
    Instance->Status.Bandwidth             = 125000;    /* Select default bandwidth */

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}   
#endif

#ifdef STTUNER_DRV_SAT_TUN_DSF8910
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_DSF8910()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_DSF8910(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_DSF8910()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;

    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        

    Instance = TUNSDRV_GetInstFromHandle(*Handle);
 
    if (Instance->TunerType != STTUNER_TUNER_DSF8910)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DSF8910 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;
    
      #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	 IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TUNSDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
	#endif


    Instance->FreqFactor = 1;

    Instance->Status.TunerStep = 250000;
    Instance->Status.IntermediateFrequency = 0;
    Instance->Status.IQSense = 1;
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif

    Instance->BandWidth[0] = 36000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x34;
    Instance->IOBuffer[1] = 0x7C;
    Instance->IOBuffer[2] = 0x83;
    Instance->IOBuffer[3] = 0x00;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		{
	     Error = ChipSetRegisters(Instance->IOHandle,0, Instance->IOBuffer, 4);
	    } 
	#endif
	#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
	   
	  
	if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
		{
		   Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TUNSDRV_IO_TIMEOUT);
	    }	    
	#endif	
   

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}   
#endif


#ifdef STTUNER_DRV_SAT_TUN_STB6000
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_STB6000()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_STB6000(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_STB6000()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;
        
       
     int i=0;
        enum
{
	ExternalClock_4=4,
	ExternalClock_27=27,
	ExternalClock_30=30	
}ExternalClock_t; 

    STTUNER_InstanceDbase_t  *Inst;
  Inst = STTUNER_GetDrvInst();


    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        
    SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSDRV_GetInstFromHandle(*Handle);
 
    if (Instance->TunerType != STTUNER_TUNER_STB6000)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_STB6000 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;
               #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
									
				            IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = STB6000_NBREGS ;        /* Write+read register */
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = STB6000_NBFIELDS;
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = STB6000_NBREGS - 1;     /* Write only register */
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;	
						    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,STB6000_NBREGS,sizeof(U8));
				#endif
				
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				
					           Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
							    Instance->DeviceMap.Registers = STB6000_NBREGS ;       /* write only registers */
							    Instance->DeviceMap.Fields    = STB6000_NBFIELDS;
							    Instance->DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
							    Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));   
							    Instance->DeviceMap.Registers = STB6000_NBREGS -1 ;    /* write only registers */
				#endif

    
    Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition, STB6000_NBREGS, sizeof( U32 ));
   
    for(i=0;i<STB6000_NBREGS;i++)
      Instance->TunerRegVal[i] =DefSTB6000Val[i]; 


ExternalClock_t = (Instance->ExternalClock)/1000000;

switch(ExternalClock_t )
    {
   case ExternalClock_4:       
    	Instance->TunerRegVal[5] = 0x04;    	
    	Instance->TunerRegVal[11]= 0x4f; 
    	 #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
    		if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
						{
						     Error = ChipSetRegisters(Instance->IOHandle,0,Instance->TunerRegVal,(STB6000_NBREGS -1));
					    } 
		#endif
		#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
						{
					    Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, Instance->TunerRegVal, STB6000_AddressArray);
					      } 
		#endif
	Instance->Status.Reference = 4000000;
	break;
  case ExternalClock_27:     
    	Instance->TunerRegVal[5] = 0x1b;
    	Instance->TunerRegVal[11] = 0xcf; 
    	#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
    		if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
						{
						     Error = ChipSetRegisters(Instance->IOHandle,0,Instance->TunerRegVal,(STB6000_NBREGS -1));
					    } 
		#endif
		#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
			if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
						{
					    Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, Instance->TunerRegVal, STB6000_AddressArray);
					      } 
		#endif
    
   	Instance->Status.Reference = 27000000;
   	break;
   case ExternalClock_30:    
    	Instance->TunerRegVal[5]  = 0x1e;
    	Instance->TunerRegVal[11] = 0xcf; 
    	#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
    		if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
						{
						     Error = ChipSetRegisters(Instance->IOHandle,0,Instance->TunerRegVal,(STB6000_NBREGS -1));
					    } 
		#endif
		#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
					   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
						{
					    Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, Instance->TunerRegVal, STB6000_AddressArray);
					      } 
		#endif
    	
    	Instance->Status.Reference = 30000000;
    	break;
    }

  /*  now safe to unlock semaphore*/
    SEM_UNLOCK(Lock_InitTermOpenClose);
    Instance->FreqFactor = 1;

    Instance->Status.TunerStep             = 125000;
    Instance->Status.IntermediateFrequency = 0;
    
    Instance->Status.IQSense               = 1;
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif
    Instance->Status.Bandwidth             = 125000;    /* Select default bandwidth */
  
    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif
/**************************/
#ifdef STTUNER_DRV_SAT_TUN_STB6110
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_STB6110()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_STB6110(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_STB6110()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;
        
       
     int i=0;
   STTUNER_InstanceDbase_t  *Inst;
  Inst = STTUNER_GetDrvInst();

    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }        
    SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSDRV_GetInstFromHandle(*Handle);
 
    if (Instance->TunerType != STTUNER_TUNER_STB6110)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_STB6110 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;
    
               #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
									
				              IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
							    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = STB6110_NBREGS ;       /* write only registers */
							    IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = STB6110_NBFIELDS;
							    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_SUBADR_8;
							    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = STB6110_NBREGS -1 ;    /* write only registers */
							    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart = RSTB6110_CTRL1;
							    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RSTB6110_CTRL1;
							    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
							    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,STB6110_NBREGS,sizeof(U8));					    
						if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
						{
						     Error = ChipSetRegisters(Instance->IOHandle,0,DefSTB6110Val,(STB6110_NBREGS -1));
					    } 
				#endif
				
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				
					         Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
							    Instance->DeviceMap.Registers = STB6110_NBREGS ;       /* write only registers */
							    Instance->DeviceMap.Fields    = STB6110_NBFIELDS;
							    Instance->DeviceMap.Mode      = IOREG_MODE_SUBADR_8;
							    Instance->DeviceMap.Registers = STB6110_NBREGS -1 ;    /* write only registers */
							    Instance->DeviceMap.WrStart = RSTB6110_CTRL1;
							    Instance->DeviceMap.RdStart = RSTB6110_CTRL1;
							      Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));   
												 if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
												{
												      Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, DefSTB6110Val, STB6110_AddressArray);
											    }	    
				#endif
  

    
    Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition, STB6110_NBREGS, sizeof( U32 ));
   
    for(i=0;i<STB6110_NBREGS;i++)
      Instance->TunerRegVal[i] =DefSTB6110Val[i]; 

   
#if 0
ExternalClock_t = (Instance->ExternalClock)/1000000;

switch(ExternalClock_t )
    {
   case ExternalClock_4:       
    	Instance->TunerRegVal[5] = 0x04;    	
    	Instance->TunerRegVal[11]= 0x4f; 
    	Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, Instance->TunerRegVal, STB6000_AddressArray);
	Instance->Status.Reference = 4000000;
	break;
  case ExternalClock_27:     
    	Instance->TunerRegVal[5] = 0x1b;
    	Instance->TunerRegVal[11] = 0xcf; 
    	Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, Instance->TunerRegVal, STB6000_AddressArray);
   	Instance->Status.Reference = 27000000;
   	break;
   case ExternalClock_30:    
    	Instance->TunerRegVal[5]  = 0x1e;
    	Instance->TunerRegVal[11] = 0xcf; 
    	Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, Instance->TunerRegVal, STB6000_AddressArray);
    	Instance->Status.Reference = 30000000;
    	break;
    }
    #endif

  /*  now safe to unlock semaphore*/
    SEM_UNLOCK(Lock_InitTermOpenClose);
    Instance->FreqFactor = 1;
     Instance->Status.Reference = 27000000;
    Instance->Status.TunerStep             = 125000;
    Instance->Status.IntermediateFrequency = 0;
    
    Instance->Status.IQSense               = 1;
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif
    Instance->Status.Bandwidth             = 125000;    /* Select default bandwidth */
  
    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

/************************/

#ifdef STTUNER_DRV_SAT_TUN_STB6100
/* ----------------------------------------------------------------------------
Name:   tuner_tunsdrv_Open_STB6100()

Description:

Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open_STB6100(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open_STB6100()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;
    STTUNER_tuner_instance_t *TunerInstance;
    STTUNER_InstanceDbase_t  *Inst;
    int i=0;
    
    
    Inst = STTUNER_GetDrvInst();
    
    Error = tuner_tunsdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
    return(Error);
}
    SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSDRV_GetInstFromHandle(*Handle);
 
    if (Instance->TunerType != STTUNER_TUNER_STB6100)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_STB6100 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Tuner Frequency range capability -> GNBvd09094 */
    Capability->FreqMin = 950000;
    Capability->FreqMax = 2150000;
   
   Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition, STB6100_NBREGS, sizeof( U32 ));
   /*Fill the TunerRegVal with the default Values	*/
   for(i=0;i<STB6100_NBREGS;i++)
      Instance->TunerRegVal[i] =STB6100_DefVal[i]; 
      
  #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = STB6100_NBREGS-1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = STB6100_NBFIELDS;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NO_R_SUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition     = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,STB6100_NBREGS ,sizeof(U32));
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart = RSTB6100_VCO;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RSTB6100_LD;
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
					 Error = ChipSetRegisters(Instance->IOHandle,0x0001,&STB6100_DefVal[1],(STB6100_NBREGS-1) );
				    }	 
   
  #endif
  #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299)    || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299)
    Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    Instance->DeviceMap.Registers = STB6100_NBREGS ;       /* write only registers */
    Instance->DeviceMap.Fields    = STB6100_NBFIELDS;
    Instance->DeviceMap.Mode      = IOREG_MODE_NO_R_SUBADR;
    Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));   
    Instance->DeviceMap.Registers = STB6100_NBREGS -1 ;    /* write only registers */
    Instance->DeviceMap.WrStart = RSTB6100_VCO;
    Instance->DeviceMap.RdStart = RSTB6100_LD;
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle, STB6100_DefVal, STB6100_AddressArray);
				    }	 
     
  #endif
    
   
    /* now safe to unlock semaphore */
 
   
    SEM_UNLOCK(Lock_InitTermOpenClose);
    Instance->FreqFactor = 1;

    Instance->Status.TunerStep             = 125000;
    Instance->Status.IntermediateFrequency = 0;
    Instance->Status.Reference = 27000000;
    Instance->Status.IQSense               = 1;
    #ifdef STTUNER_IQ_WIRING
    Instance->Status.IQSense = STTUNER_IQ_WIRING;
    #endif
    Instance->Status.Bandwidth             = 36000;    /* Select default bandwidth */
    TunerInstance = &Inst[OpenParams->TopLevelHandle].Sat.Tuner;
    tuner_tunsdrv_SetBandWidth(TunerInstance->DrvHandle, Instance->Status.Bandwidth, &(Instance->Status.Bandwidth));
    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
  
    return(Error);
}
#endif
/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Close(TUNER_Handle_t Handle, TUNER_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;

    Instance = TUNSDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* switch which allows us to deallocate the memory for the chip allocate in new tuner opens */
    switch (Instance->TunerType)
    {
        case STTUNER_TUNER_VG1011:
        case STTUNER_TUNER_TUA6100:
        case STTUNER_TUNER_EVALMAX:
        case STTUNER_TUNER_S68G21:
        case STTUNER_TUNER_DSF8910:
        	break;
        
        case STTUNER_TUNER_VG0011:
          Instance->DeviceMap.Registers += 1;
        case STTUNER_TUNER_HZ1184:
          Instance->DeviceMap.Registers += 1;
        case STTUNER_TUNER_MAX2116:
          Instance->DeviceMap.Registers += 2;
        case STTUNER_TUNER_IX2476:
          Instance->DeviceMap.Registers += 1;
      
        case STTUNER_TUNER_STB6000:
          Instance->DeviceMap.Registers += 1;
             case STTUNER_TUNER_STB6110:
          Instance->DeviceMap.Registers += 1;
       
        case STTUNER_TUNER_STB6100:
          Instance->DeviceMap.Registers += 1;
          #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
							   
							  
           Error = STTUNER_IOREG_Close(&(Instance->DeviceMap));
          #endif
           break;
       
        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("WARNING - invalid tuner in close function\n"));
#endif
            break;
    }


    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);

}  

ST_ErrorCode_t tuner_tunsdrv_Utilfunction(TUNER_Handle_t Handle, TUNER_Utility_t Utility)
{
    TUNSDRV_InstanceData_t *Instance;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    Instance = TUNSDRV_GetInstFromHandle(Handle);

    if(Utility == VCO_SEARCH_OFF)
    	{
    		switch (Instance->TunerType)
			  {
			  	case STTUNER_TUNER_STB6100:
			  		  
							ChipSetFieldImage(FSTB6100_OSCH,0,Instance->TunerRegVal);	/* VCO search disabled */
							ChipSetFieldImage(FSTB6100_OCK,3,Instance->TunerRegVal);		/* VCO search clock off */
							ChipSetFieldImage(FSTB6100_FCCK,0,Instance->TunerRegVal);	/*LPF BW setting clock disabled */
							#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				            if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					           {
						          Error = ChipSetRegisters(Instance->IOHandle,IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart,&Instance->TunerRegVal[1],IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				               }
				           #endif
				           #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
							   
							  
							  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
								{
								Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, Instance->DeviceMap.WrStart, Instance->TunerRegVal,(Instance->DeviceMap).Registers);
							    }	       
			              #endif
				  break;
					default:
						break;
					}
			}
		return(Error);
}
 
/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_SetFrequency()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_SetFrequency (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_SetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 Steps;
    U32 Prescaler, NCnt, ACnt, F;
    TUNSDRV_InstanceData_t *Instance;
    #if defined(STTUNER_DRV_SAT_TUN_STB6100)||defined (STTUNER_DRV_SAT_TUN_STB6000)||defined (STTUNER_DRV_SAT_TUN_MAX2116)||defined(STTUNER_DRV_SAT_TUN_IX2476) || defined(STTUNER_DRV_SAT_TUN_HZ1184  )
    U32 frequency;
	U32 stepsize;
	#endif
	#if defined (STTUNER_DRV_SAT_TUN_HZ1184  )||defined (STTUNER_DRV_SAT_TUN_STB6100)||defined (STTUNER_DRV_SAT_TUN_STB6000)||defined(STTUNER_DRV_SAT_TUN_IX2476)
	U32 nbsteps;
	#endif
	#if defined (STTUNER_DRV_SAT_TUN_HZ1184  )|| defined (STTUNER_DRV_SAT_TUN_VG0011)||defined (STTUNER_DRV_SAT_TUN_STB6100)||defined (STTUNER_DRV_SAT_TUN_STB6000)||defined (STTUNER_DRV_SAT_TUN_MAX2116)||defined (STTUNER_DRV_SAT_TUN_STB6110)||defined(STTUNER_DRV_SAT_TUN_IX2476)
	U32 divider;
	#endif
	 #if defined (STTUNER_DRV_SAT_TUN_HZ1184  )|| defined (STTUNER_DRV_SAT_TUN_IX2476)||defined (STTUNER_DRV_SAT_TUN_STB6100)||defined (STTUNER_DRV_SAT_TUN_STB6000)
	U32 swallow;
	#endif
		#if defined (STTUNER_DRV_SAT_TUN_VG0011)||defined (STTUNER_DRV_SAT_TUN_MAX2116  )||defined (STTUNER_DRV_SAT_TUN_STB6100)||defined (STTUNER_DRV_SAT_TUN_STB6000)||defined(STTUNER_DRV_SAT_TUN_IX2476)
		U8 u8;
	#endif
	 #if defined (STTUNER_DRV_SAT_TUN_MAX2116  )|| defined (STTUNER_DRV_SAT_TUN_IX2476)||defined (STTUNER_DRV_SAT_TUN_STB6100)
	U8 vco;
	#endif
	#ifdef STTUNER_DRV_SAT_TUN_STB6110
	U32	p,
		Presc,
		m,
		rDiv,
		r,
		i;
	#endif
	
		#if defined(STTUNER_DRV_SAT_TUN_MAX2116)
		
		U8 index;
		#endif
	
    Instance = TUNSDRV_GetInstFromHandle(Handle);

    switch (Instance->TunerType)
    {
        case STTUNER_TUNER_VG1011:
        case STTUNER_TUNER_TUA6100:
        case STTUNER_TUNER_EVALMAX:
        case STTUNER_TUNER_S68G21:
        case STTUNER_TUNER_DSF8910:
        switch (Instance->TunerType)
        {
        	          case STTUNER_TUNER_DSF8910:
                if (Frequency < 1650000)
		{
   			Instance->IOBuffer[ 3 ] = 0x00;
		}
		else
		{
			if (Frequency < 2000000)
			{
   				Instance->IOBuffer[ 3 ] = 0x40;
			}
			else
   				Instance->IOBuffer[ 3 ] = 0x80;
		}
		#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
		               STTBX_Print(("%s IOBuffer[ 3 ] = 0x%2X \n", identity, Instance->IOBuffer[ 3 ]));
		#endif
	    break;
          default:
	     break;
        }

        	/* Calculate number of steps for nominated frequency */
    		Steps = ((Frequency * 500) / Instance->FreqFactor);

    		Steps = (Steps + (500 * Instance->Status.IntermediateFrequency)) /
            		(Instance->Status.TunerStep / 2);


    		/* Communications depends on PLL type */
    		switch (Instance->PLLType)
    		{
       		case TUNER_PLL_5522:
        		case TUNER_PLL_5655:
            			Instance->IOBuffer[0] = (U8)((Steps & 0x00007F00) >> 8);
            			Instance->IOBuffer[1] = (U8) (Steps & 0x000000FF);
        		break;

        		case TUNER_PLL_5659:
            			Instance->IOBuffer[0] = (U8)((Steps & 0x00007F00) >> 8);
            			Instance->IOBuffer[1] = (U8)(Steps & 0x000000FF);
            			Instance->IOBuffer[2] = (U8)( (((U32)Instance->IOBuffer[2]) & 0xFF9F) | (Steps >> 10) );
        		break;

        		case TUNER_PLL_TUA6100:
            			Prescaler = (Instance->IOBuffer[1] & 0x40) ? 64 : 32;
            			NCnt = Steps / Prescaler;
            			ACnt = Steps - (NCnt * Prescaler);
            
            			Instance->IOBuffer[1] = Instance->IOBuffer[1] | ((U8)((NCnt & 0x600) >> 9));
            			Instance->IOBuffer[2] = (U8)((NCnt & 0x1FE) >> 1);
            			Instance->IOBuffer[3] = (U8)(((NCnt & 0x01) << 7) | (ACnt & 0x7F));

            			F = CalculateFrequency(Instance);

            			Instance->IOBuffer[1] = (Instance->IOBuffer[1] & 0x7F) | ((F>1525000) ? 0x80 : 0x00);
            			Instance->IOBuffer[1] = (Instance->IOBuffer[1] & 0xCF) | ((F>1435000) ? 0x80 : 0x20);
        		break;

        		default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            			STTBX_Print(("%s WARNING - no PLL specified\n", identity));
#endif
            		break;
    		}

 #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
	if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		{
	     Error = ChipSetRegisters(Instance->IOHandle,0, Instance->IOBuffer, 4);
	    } 
	#endif
	#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
	   
	  
	if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
		{	   
			/* Update device with new frequency */
		    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TUNSDRV_IO_TIMEOUT);
	    }	    
	#endif	
      		
       	   

      		 /* Set new frequency of device */
    		*NewFrequency = Instance->Status.Frequency = CalculateFrequency(Instance);
      		    
        break;
#ifdef  STTUNER_DRV_SAT_TUN_VG0011
        case STTUNER_TUNER_VG0011:
        			   		
    		u8=0;
				
		if(Frequency>=950000)
		while(!INRANGE(TUNERSAT_LOOKUP[u8][0],Frequency,TUNERSAT_LOOKUP[u8][1]) && (u8<4)) u8++;
			
		ChipSetFieldImage(FTUNERSAT_DIV4SEL,TUNERSAT_LOOKUP[u8][2],Instance->TunerRegVal);
		ChipSetFieldImage(FTUNERSAT_RDIV,TUNERSAT_LOOKUP[u8][3],Instance->TunerRegVal);
		ChipSetFieldImage(FTUNERSAT_PRESC32ON,TUNERSAT_LOOKUP[u8][4],Instance->TunerRegVal);
		
		divider = Frequency/1000;
		ChipSetFieldImage(FTUNERSAT_N_MSB,(divider>>8) & 0x0F,Instance->TunerRegVal); 
		ChipSetFieldImage(FTUNERSAT_N_LSB,divider & 0xFF,Instance->TunerRegVal);
							#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				            if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					           {
						          ChipSetRegisters(Instance->IOHandle,RTUNERSAT_TUNING_LSB,Instance->TunerRegVal,3);
						          ChipSetField(Instance->IOHandle, FTUNERSAT_CALVCOSTRT,1);
				               }
				           #endif
				           #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
							   
							  
							  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
								{
								STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle,RTUNERSAT_TUNING_LSB,Instance->TunerRegVal,3);
		
		                     STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, FTUNERSAT_CALVCOSTRT,1);
							    }	       
			              #endif
		
    		*NewFrequency = Tuner_GetFrequency( Instance);
    		Instance->Status.Frequency = *NewFrequency;
    		
	break;
    #endif
  #ifdef  STTUNER_DRV_SAT_TUN_HZ1184      
        case STTUNER_TUNER_HZ1184:
        	ChipSetFieldImage(FHZ1184_PD1,Frequency < 1450000,Instance->TunerRegVal);
		
		frequency = Frequency + Instance->Status.IntermediateFrequency;
		stepsize = 2*(Instance->Status.Reference/(TUNER_PowOf2(ChipFieldExtractVal(*(Instance->TunerRegVal+2),FHZ1184_R))));
		nbsteps = (frequency * 100) / (stepsize / 10);
		divider = nbsteps / 32;
		swallow = nbsteps % 32;
	
		ChipSetFieldImage(FHZ1184_N_HSB,((divider >> 10) & 0x03),Instance->TunerRegVal);
		ChipSetFieldImage(FHZ1184_N_MSB,((divider >> 3) & 0x7F),Instance->TunerRegVal);
		ChipSetFieldImage(FHZ1184_N_LSB,(divider & 0x07),Instance->TunerRegVal);
		ChipSetFieldImage(FHZ1184_A,swallow,Instance->TunerRegVal);
			               #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				            if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					           {
						          ChipSetRegisters(Instance->IOHandle,0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				               }
				           #endif
				           #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
							   
							  
							  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
								{
                              Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, 0,Instance->TunerRegVal,(Instance->DeviceMap).Registers);		
							    }	       
			              #endif
		
		
		*NewFrequency = Tuner_GetFrequency(Instance);
    		Instance->Status.Frequency = *NewFrequency;
	break;
      #endif
         
    
     #ifdef  STTUNER_DRV_SAT_TUN_MAX2116   
         case STTUNER_TUNER_MAX2116:
         
		frequency =  Frequency + Instance->Status.IntermediateFrequency;  
		stepsize = Instance->Status.Reference / (TUNER_PowOf2(ChipFieldExtractVal(*(Instance->TunerRegVal+2),FMAX2116_R)+1));
		
		divider = (frequency * 100) / (stepsize / 10);
					
		ChipSetFieldImage(FMAX2116_N_MSB,(divider >> 8) & 0x7F,Instance->TunerRegVal); 
		ChipSetFieldImage(FMAX2116_N_LSB,divider & 0xFF,Instance->TunerRegVal);
	    	u8=0;
		
		while(!checkrange(MAX2116_LOOKUP[u8][0],Frequency,MAX2116_LOOKUP[u8][1]) && (u8<9)) u8++;
		
		ChipSetFieldImage(FMAX2116_DIV2,MAX2116_LOOKUP[u8][3],Instance->TunerRegVal);
		vco = MAX2116_LOOKUP[u8][2];
		ChipSetFieldImage(FMAX2116_VCO,vco,Instance->TunerRegVal);
		#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
        if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
         {
          ChipSetRegisters(Instance->IOHandle,0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
           }
       #endif
       #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
	   
	  
	  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
		{
                  Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, *(Instance->TunerRegVal), 0,(Instance->DeviceMap).Registers);
	    }	       
        #endif
		
		
		STOS_TaskDelayUs(10000);
		index = 0;
		
		while((!Tuner_GetStatus(Instance)) && (index < 4))
		{
			u8 = (U8)ChipFieldExtractVal(*(Instance->TunerRegVal+6),FMAX2116_ADC);   
			
			if(u8 == 0)
			{
				if(vco == 0)
				{
					vco = 7;
					ChipSetFieldImage(FMAX2116_DIV2,0,Instance->TunerRegVal);
				}
				else
				{
					vco--;
				}
			}
			else if(u8 == 7)
			{
				if(vco == 7)
				{
					vco = 0;
					ChipSetFieldImage(FMAX2116_DIV2,1,Instance->TunerRegVal);
				}
				else
				{
					vco++;
				}
			}
			
			ChipSetFieldImage(FMAX2116_VCO,vco,Instance->TunerRegVal);
			#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
		      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		       {
		        ChipSetRegisters(Instance->IOHandle,0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
		         }
		     #endif
		     #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
				               Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, 0, Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	       
        #endif
			
			STOS_TaskDelayUs(10000);
			index++;
		}
		
		*NewFrequency = Tuner_GetFrequency( Instance );
		Instance->Status.Frequency = *NewFrequency;
		
	  break;
	#endif
	#ifdef  STTUNER_DRV_SAT_TUN_IX2476 
	
        case STTUNER_TUNER_IX2476:

		frequency =  Frequency + Instance->Status.IntermediateFrequency;  
		stepsize = Instance->Status.Reference / (TUNER_PowOf2(ChipFieldExtractVal(*(Instance->TunerRegVal+2),FIX2476_REF)+2));

		nbsteps = (frequency * 100) / (stepsize / 10); 
		swallow = nbsteps % 32;
		divider = nbsteps / 32;
		
		ChipSetFieldImage(FIX2476_N_MSB,(divider >> 3) & 0x1F,Instance->TunerRegVal); 
		ChipSetFieldImage(FIX2476_N_LSB,divider & 0x07,Instance->TunerRegVal);
		ChipSetFieldImage(FIX2476_A, swallow,Instance->TunerRegVal);
		
		u8=0;
		
		while(!INRANGE(IX2476_LOOKUP[u8][0],Frequency,IX2476_LOOKUP[u8][1]) && (u8<10)) u8++;
		
		ChipSetFieldImage(FIX2476_DIV,IX2476_LOOKUP[u8][3],Instance->TunerRegVal);
		vco = IX2476_LOOKUP[u8][2];
		ChipSetFieldImage(FIX2476_VCO,vco,Instance->TunerRegVal);
		
		ChipSetFieldImage( FIX2476_TM, 0,Instance->TunerRegVal);
		#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
		      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		       {
		        ChipSetRegisters(Instance->IOHandle,RIX2476_NHIGH,Instance->TunerRegVal,4);
		         }
		     #endif
		     #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
				               Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle,RIX2476_NHIGH,Instance->TunerRegVal,4);
				    }	       
        #endif
		
		STOS_TaskDelayUs(1000);

		ChipSetFieldImage( FIX2476_TM, 1,Instance->TunerRegVal);
		#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
		      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		       {
		        ChipSetRegisters(Instance->IOHandle,RIX2476_NHIGH,Instance->TunerRegVal,4);
		         }
		     #endif
		     #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
				               Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle,RIX2476_NHIGH,Instance->TunerRegVal,4);
				    }	       
        #endif
		
		STOS_TaskDelayUs(6000);
		
		*NewFrequency = Tuner_GetFrequency( Instance );
		Instance->Status.Frequency = *NewFrequency;
		
	break;
	
	#endif
	
	#ifdef STTUNER_DRV_SAT_TUN_STB6000
        case STTUNER_TUNER_STB6000:
         {         	
         	if(Instance->SymbolRate <= 10000000)
			ChipSetFieldImage(FSTB6000_G, 10,Instance->TunerRegVal);
			else if(Instance->SymbolRate <= 20000000)
			ChipSetFieldImage(FSTB6000_G, 9,Instance->TunerRegVal);
		
        	ChipSetFieldImage(FSTB6000_ODIV,Frequency <= 1075000,Instance->TunerRegVal);
			u8=0;
		
		while(!checkrange(STB6K_LOOKUP[u8][0],Frequency,STB6K_LOOKUP[u8][1]) && (u8<10)) u8++;
		ChipSetFieldImage(FSTB6000_OSM,STB6K_LOOKUP[u8][2],Instance->TunerRegVal);
		
		/*frequency = Frequency + hTuner->IF;      */
		frequency = Frequency<<(ChipGetFieldImage(FSTB6000_ODIV,Instance->TunerRegVal)+1);
		stepsize =  2*(Instance->Status.Reference / ChipGetFieldImage(FSTB6000_R,Instance->TunerRegVal)); /* 2 x Fr / R */
		nbsteps = (frequency * 100) / (stepsize / 10);
		divider = nbsteps / 16; /* 32 */
		swallow = nbsteps % 16; /* 32 */
	
		ChipSetFieldImage(FSTB6000_N_MSB,(divider>>1) & 0xFF,Instance->TunerRegVal); 
		ChipSetFieldImage(FSTB6000_N_LSB,divider & 0x01,Instance->TunerRegVal);
		ChipSetFieldImage(FSTB6000_A,swallow,Instance->TunerRegVal);
		
		ChipSetFieldImage(FSTB6000_LPEN,0,Instance->TunerRegVal);	/* PLL loop disabled */
		ChipSetFieldImage(FSTB6000_OSCH,1,Instance->TunerRegVal);	/* VCO search enabled */
		ChipSetFieldImage(FSTB6000_OCK,1,Instance->TunerRegVal);		/* VCO search clock off */ 
		#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
		      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		       {
		        ChipSetRegisters(Instance->IOHandle,0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
		         }
		     #endif
		     #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
				         Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle,0, Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	       
        #endif
		
		ChipSetFieldImage(FSTB6000_LPEN,1,Instance->TunerRegVal);	/* PLL loop enabled */
			#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
		      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		       {
		        ChipSetRegisters(Instance->IOHandle,0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
		         }
		     #endif
		     #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
				         Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle,0, Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	       
        #endif
		
			
		STOS_TaskDelayUs(15000);
	
		ChipSetFieldImage(FSTB6000_OSCH,0,Instance->TunerRegVal);	/* VCO search disabled */
		ChipSetFieldImage(FSTB6000_OCK,3,Instance->TunerRegVal);		/* VCO search clock off */ 
			#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
		      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		       {
		        ChipSetRegisters(Instance->IOHandle,0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
		         }
		     #endif
		     #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
				         Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle,0, Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	       
        #endif     	
           	
        	*NewFrequency = Tuner_GetFrequency( Instance );
    		Instance->Status.Frequency = *NewFrequency;
    		
    		
    		
        }
        break;
   #endif     
   /*************************************************************/
   #ifdef STTUNER_DRV_SAT_TUN_STB6110
        case STTUNER_TUNER_STB6110:
         {         	
         	/*if(Instance->SymbolRate <= 10000000)STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, 
			ChipSetFieldImage(FSTB6000_G, 10,Instance->TunerRegVal);
			else if(Instance->SymbolRate <= 20000000)
			ChipSetFieldImage(FSTB6000_G, 9,Instance->TunerRegVal);*/
			ChipSetFieldImage(FSTB6110_K,((Instance->Status.Reference/1000000)-16),Instance->TunerRegVal); /*K= quartz-16MHz*/
				if (Frequency<=1023000) 
				{
					p=1;
					Presc=0;
				}
				else if (Frequency<=1300000)
				{
					p=1;
					Presc=1;
				}
				else if (Frequency<=2046000)
				{
					p=0;
					Presc=0;
				}
				else 
				{
					p=0;
					Presc=1;
				}
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
		      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		       {
		        ChipSetField(Instance->IOHandle,FSTB6110_DIV4SEL,p);
				ChipSetField(Instance->IOHandle, FSTB6110_PRESC32ON,Presc);
		         }
		     #endif
		     #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
				         STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,FSTB6110_DIV4SEL,p);
				        STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, FSTB6110_PRESC32ON,Presc);
				    }	       
          #endif   
				
				
				m = (Instance->Status.Reference/1000000)/(TUNER_PowOf2(p+1));
					
					i = 0;
					while(TUNER_PowOf2(i) <= ABS(m))
						i++;
										
					if (m == 0)
					i= 1;
				    
				    rDiv =m - 1;				

				r=TUNER_PowOf2(rDiv+1);
				divider= (Frequency * r * TUNER_PowOf2(p+1)*10)/(Instance->Status.Reference/1000);  
				divider=(divider+5)/10;
				
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
		      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		       {
		               ChipSetField( Instance->IOHandle, FSTB6110_RDIV,rDiv);
				       ChipSetField( Instance->IOHandle, FSTB6110_NDIV_MSB,MSB(divider));
				        ChipSetField( Instance->IOHandle, FSTB6110_NDIV_LSB,LSB(divider));
 
	          			ChipSetField(Instance->IOHandle, FSTB6110_CALVCOSTRT,1);		/* VCO Auto Calibration */
	          			i=0;
								while((i<10) && (ChipGetField(Instance->IOHandle, FSTB6110_CALVCOSTRT)!=0))
								{
									STOS_TaskDelayUs(1);	/* wait for VCO auto calibration */
									i++;
								}
		         }
		     #endif
		     #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
				        STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, FSTB6110_RDIV,rDiv);
				       STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, FSTB6110_NDIV_MSB,MSB(divider));
				        STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, FSTB6110_NDIV_LSB,LSB(divider));
 
	          			STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, FSTB6110_CALVCOSTRT,1);		/* VCO Auto Calibration */
	          				i=0;
								while((i<10) && (STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle, FSTB6110_CALVCOSTRT)!=0))
								{
									STOS_TaskDelayUs(1);	/* wait for VCO auto calibration */
									i++;
								}

	          			
				    }	       
              #endif   

			
		
		  	*NewFrequency = Tuner_GetFrequency( Instance );
    		Instance->Status.Frequency = *NewFrequency;
		
		
	/******************************/	    		
        }
        break;
   #endif      
   
   /*************************************************************/
	
	
	#ifdef STTUNER_DRV_SAT_TUN_STB6100    
        case STTUNER_TUNER_STB6100:
         {
         		if(Instance->SymbolRate >= 15000000)
		ChipSetFieldImage(FSTB6100_G, 11,Instance->TunerRegVal);
		else if(Instance->SymbolRate >= 5000000)
		ChipSetFieldImage(FSTB6100_G, 13,Instance->TunerRegVal);
		else
		ChipSetFieldImage(FSTB6100_G, 14,Instance->TunerRegVal);
		
	      	ChipSetFieldImage(FSTB6100_ODIV,Frequency <= 1075000,Instance->TunerRegVal);
		ChipSetFieldImage(FSTB6100_PSD2,!INRANGE (1075000,Frequency,1326000),Instance->TunerRegVal);
			
			u8=0;
			while(!checkrange(STB6K_LOOKUP[u8][0],Frequency,STB6K_LOOKUP[u8][1])&& (u8<10)) u8++;
				vco=STB6K_LOOKUP[u8][2];
				ChipSetFieldImage(FSTB6100_OSM,vco,Instance->TunerRegVal);

				frequency=2*Frequency*(ChipGetFieldImage(FSTB6100_ODIV,Instance->TunerRegVal)+1); /*Flo=2*Ftuner*(ODIV+1)*/
				

				Instance->Status.Reference/=1000; /*Refrence in Khz*/
				
				divider=(frequency/Instance->Status.Reference)/(ChipGetFieldImage(FSTB6100_PSD2,Instance->TunerRegVal)+1);
			
				stepsize=frequency-divider*(ChipGetFieldImage(FSTB6100_PSD2,Instance->TunerRegVal)+1)*(Instance->Status.Reference); /**/
				nbsteps=(stepsize*TUNER_PowOf2(9))/((ChipGetFieldImage(FSTB6100_PSD2,Instance->TunerRegVal)+1)*(Instance->Status.Reference));					  
				swallow=nbsteps; 
				
				ChipSetFieldImage(FSTB6100_NI,divider,Instance->TunerRegVal);
				ChipSetFieldImage(FSTB6100_NF_LSB,(swallow&0xFF),Instance->TunerRegVal);
				ChipSetFieldImage(FSTB6100_NF_MSB,(swallow>>8),Instance->TunerRegVal);
				Instance->Status.Reference*=1000; 
				ChipSetFieldImage(FSTB6100_LPEN,0,Instance->TunerRegVal);	/* PLL loop disabled */
				ChipSetFieldImage(FSTB6100_OSCH,1,Instance->TunerRegVal);	/* VCO search enabled */
				ChipSetFieldImage(FSTB6100_OCK,0,Instance->TunerRegVal);		/* VCO fast search*/
				#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
						Error = ChipSetRegisters(Instance->IOHandle,IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart,&Instance->TunerRegVal[1],IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				    }
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, Instance->DeviceMap.WrStart, Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	   
			    
			   #endif
				
				
				ChipSetFieldImage(FSTB6100_LPEN,1,Instance->TunerRegVal);	/* PLL loop enabled */
              #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
						Error = ChipSetRegisters(Instance->IOHandle,IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart,&Instance->TunerRegVal[1],IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				    }
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, Instance->DeviceMap.WrStart, Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	   
			    
			   #endif
				
				*NewFrequency = Tuner_GetFrequency( Instance );
    		    		Instance->Status.Frequency = *NewFrequency;

        }
        break;
     #endif
     
        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s WARNING - no TUNER specified\n", identity));
#endif
            break;
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print(("%s Frequency=%u NewFrequency=%u\n", identity, Frequency, *NewFrequency));
#endif

    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail, error=%d\n", identity, Error));
#endif
        
        return(Error);
    }
    
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_GetStatus()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_GetStatus(TUNER_Handle_t Handle, TUNER_Status_t *Status)
{
    TUNSDRV_InstanceData_t *Instance;
    ST_ErrorCode_t Error=ST_NO_ERROR;

    Instance = TUNSDRV_GetInstFromHandle(Handle);

    switch (Instance->TunerType)
    {
      
        case STTUNER_TUNER_VG1011:
        case STTUNER_TUNER_TUA6100:
        case STTUNER_TUNER_EVALMAX:
        case STTUNER_TUNER_S68G21:
        case STTUNER_TUNER_VG0011:
        case STTUNER_TUNER_HZ1184:
        case STTUNER_TUNER_MAX2116:
        case STTUNER_TUNER_DSF8910:
        case STTUNER_TUNER_IX2476:
      
        case STTUNER_TUNER_STB6000:
        case STTUNER_TUNER_STB6110:
     
        case STTUNER_TUNER_STB6100:
 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
        break;

        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("WARNING - no TUNER specified\n"));
#endif
        break;
    }
    return(Error);
        
}

/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_IsTunerLocked()

Description:
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_IsTunerLocked(TUNER_Handle_t Handle, BOOL *Locked)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_IsTunerLocked()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_IOARCH_Operation_t OpType = STTUNER_IO_READ;/*For removing warning of uninitialized OpType in LINUX*/
    U8 RxStatus;
    U8 SubAddr=0; /* I2C address 8 bits wide */
    U8 BitMask=0;
    TUNSDRV_InstanceData_t *Instance;

    Instance = TUNSDRV_GetInstFromHandle(Handle);

    switch(Instance->TunerType)
    {
        case STTUNER_TUNER_VG0011:
        case STTUNER_TUNER_HZ1184:
        case STTUNER_TUNER_MAX2116:
        case STTUNER_TUNER_IX2476:
     
        case STTUNER_TUNER_STB6000:
        case STTUNER_TUNER_STB6110:
     case STTUNER_TUNER_STB6100:
	 	*Locked = Tuner_GetStatus( Instance);
	 	 Error = Instance->DeviceMap.Error ;
	 	 break; 
	 	 
        case STTUNER_TUNER_VG1011:
        case STTUNER_TUNER_TUA6100:
        case STTUNER_TUNER_EVALMAX:
        case STTUNER_TUNER_S68G21:
        case STTUNER_TUNER_DSF8910:
        	{
    			*Locked = FALSE;    /* Assume not locked */

    			switch (Instance->PLLType)
    			{
        			case TUNER_PLL_5522:
        			case TUNER_PLL_5655:
        			case TUNER_PLL_5659:
            				BitMask = 0x40;
            				SubAddr = 0x00;
            				OpType  = STTUNER_IO_READ;
            			break;

       			        case TUNER_PLL_TUA6100:
            				BitMask = 0x80;
            				SubAddr = 0x80;
            				OpType  = STTUNER_IO_SA_READ;
            			break;

        		    default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
                        STTBX_Print(("%s WARNING - no PLL specified\n", identity));
#endif
            	 	break;
    			}

        		/* Read 'locked' status no idea whats going on here, missing a read? */
        		#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				     Error = ChipGetRegisters(Instance->IOHandle,(U16)SubAddr,1,&RxStatus);
				    } 
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, OpType, (U16)SubAddr, &RxStatus, 1, TUNSDRV_IO_TIMEOUT); 
				    }	    
				#endif	
			
			    			/* Check status bits */
    			if ( (RxStatus & BitMask) != 0) *Locked = TRUE;
        	}
        break;
        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
          		STTBX_Print(("%s WARNING - bad tuner specified\n", identity));
#endif
        break;
    }
        
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print(("%s Locked: ", identity));
    if (*Locked == TRUE)
    {
        STTBX_Print(("Yes\n"));
    }
    else
    {
        STTBX_Print(("no\n"));
    }
#endif

    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_SetBandWidth()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_SetBandWidth(TUNER_Handle_t Handle, U32 BandWidth, U32 *NewBandWidth)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_SetBandWidth()";
#endif

    U8 index = 1;

    #if defined(STTUNER_DRV_SAT_TUN_STB6100)||defined (STTUNER_DRV_SAT_TUN_STB6000)||defined (STTUNER_DRV_SAT_TUN_MAX2116)||defined (STTUNER_DRV_SAT_TUN_STB6110)||defined(STTUNER_DRV_SAT_TUN_IX2476) || defined(STTUNER_DRV_SAT_TUN_HZ1184  ) || defined(STTUNER_DRV_SAT_TUN_VG0011  )
    U8 u8;
    #endif
    #if defined(STTUNER_DRV_SAT_TUN_STB6100)||defined (STTUNER_DRV_SAT_TUN_STB6000)||defined (STTUNER_DRV_SAT_TUN_MAX2116)||defined(STTUNER_DRV_SAT_TUN_IX2476) || defined(STTUNER_DRV_SAT_TUN_HZ1184  )
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #endif
    

     #ifdef STTUNER_DRV_SAT_TUN_STB6110
    U8 i;
    #endif
    #ifdef STTUNER_DRV_SAT_TUN_MAX2116
    S32 filter;
    #endif
    U32 Bandwidth;
	
    TUNSDRV_InstanceData_t *Instance;

    Instance = TUNSDRV_GetInstFromHandle(Handle);

    Bandwidth = BandWidth * 1000;
   
    switch(Instance->TunerType)
    {  
    	#ifdef  STTUNER_DRV_SAT_TUN_VG0011
        case STTUNER_TUNER_VG0011:
	        /* No settable filter */
	        
	        u8=((Bandwidth-5000000)/2000000) & 0x1f;
		ChipSetFieldImage(FTUNERSAT_CF,u8,Instance->TunerRegVal);
		ChipSetFieldImage(FTUNERSAT_CALRC_STRT,1,Instance->TunerRegVal);
			
	        *NewBandWidth = Tuner_GetBandwidth( Instance ) / 1000;
	    	Instance->Status.Bandwidth = *NewBandWidth;
	break;
	#endif
	
	#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
        case STTUNER_TUNER_HZ1184:
        if(Bandwidth < 20000000)
		u8 = 2;
		else if(Bandwidth >= 60000000)
		u8 = 10;
		else
		u8 = (Bandwidth - 10000000)/5000000 + 1;
		
		ChipSetFieldImage(FHZ1184_PD2,0x01 & (u8>>3),Instance->TunerRegVal);
		ChipSetFieldImage(FHZ1184_PD3,0x01 & (u8>>2),Instance->TunerRegVal);
		ChipSetFieldImage(FHZ1184_PD4,0x01 & (u8>>1),Instance->TunerRegVal);
		ChipSetFieldImage(FHZ1184_PD5,0x01 & u8,Instance->TunerRegVal);
		
		      #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
						Error = ChipSetRegisters(Instance->IOHandle, 0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				    }
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, 0,Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	   
			   #endif
				
		
		
	        /* read back the bandwidth */
	        *NewBandWidth = Tuner_GetBandwidth( Instance ) / 1000;
    		Instance->Status.Bandwidth = *NewBandWidth;
	
	break;
	#endif
	
	#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
	case STTUNER_TUNER_MAX2116:
	
		u8 = ChipFieldExtractVal(*(Instance->TunerRegVal+4),FMAX2116_M); 
		
				if(u8)
				{
					filter = ((((S32)Bandwidth /2)/((S32)Instance->Status.Reference / (1000*u8))) * 1000 - 4000000)/145100 + 1;
					
					if(filter < 0)
						filter = 0;
					else if(filter > 0x7F)
						filter = 0x7F;
				}
				else
					filter = 0x00;
					
							
				ChipSetFieldImage(FMAX2116_F_OUT,filter,Instance->TunerRegVal);
					#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
						Error = ChipSetRegisters(Instance->IOHandle, 0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				    }
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, 0,Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	   
			   #endif
				
				
				/* read back the bandwidth */
				*NewBandWidth = Tuner_GetBandwidth( Instance ) / 1000;
    		    		Instance->Status.Bandwidth = *NewBandWidth;
	break;
	#endif			


        case STTUNER_TUNER_VG1011:
        case STTUNER_TUNER_TUA6100:
        case STTUNER_TUNER_EVALMAX:
        case STTUNER_TUNER_S68G21:
        case STTUNER_TUNER_DSF8910:
        {

    		/* Find closest (largest) bandwidth to requested value */
    		while( (Instance->BandWidth[index] != 0) && (BandWidth > Instance->BandWidth[index]) )
    		{
        		index++;
    		}
		#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    			STTBX_Print(("%s index=%u\n", identity, index));
		#endif
    
    		/* Check whether or not we found a valid band (i.e. no zero) */
    		if (Instance->BandWidth[index] == 0)
    		{
      			index = 0;  /* Use default band */
    		}
  
    		/* For EVALMAX we must update the command sent to the device for selecting narrow or wide band filtering */
    		if (Instance->TunerType == STTUNER_TUNER_EVALMAX)
    		{
        		/* For i=0 - narrow, i=1 - wide (odd/even band indexes) */
       		Instance->IOBuffer[3] = (Instance->IOBuffer[3] & 0xFE) | index;
    		}

    		/* Update status */
    		Instance->Status.Bandwidth = Instance->BandWidth[index];
    		*NewBandWidth              = Instance->BandWidth[index];
        }
        break;
        #ifdef  STTUNER_DRV_SAT_TUN_IX2476
	case STTUNER_TUNER_IX2476:
		if(Bandwidth < 20000000)
			u8 = 3;
		else if(Bandwidth >= 60000000)
			u8 = 13;
		else
			u8 = (Bandwidth - 20000000)/4000000 + 3;
		
		ChipSetFieldImage(FIX2476_PD2,0x01 & (u8>>3),Instance->TunerRegVal);
		ChipSetFieldImage(FIX2476_PD3,0x01 & (u8>>2),Instance->TunerRegVal);
		ChipSetFieldImage(FIX2476_PD4,0x01 & (u8>>1),Instance->TunerRegVal);
		ChipSetFieldImage(FIX2476_PD5,0x01 & u8,Instance->TunerRegVal);
		#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
						Error = ChipSetRegisters(Instance->IOHandle, 0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				    }
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, 0,Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	   
			   #endif
	break;
	#endif
	
#ifdef STTUNER_DRV_SAT_TUN_STB6000
        case STTUNER_TUNER_STB6000:
        {
        if(Bandwidth == 0)
		{
			ChipSetFieldImage(FSTB6000_FCL,0,Instance->TunerRegVal);
		
		}
		else
		{
			if((Bandwidth/2) > 36000000)		/* BW/2 > max filter bandwidth (36 Mhz) ==> force to max */
					u8 = 31;
				else if((Bandwidth/2) < 5000000)	/* BW/2 < min filter bandwidth (5 Mhz)  ==> force to min */
					u8 = 0;
				else							 	/* filter = BW/2 */
					u8 = (Bandwidth/2)/1000000 - 5;
		
			ChipSetFieldImage(FSTB6000_FCL,0,Instance->TunerRegVal);
			#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
						Error = ChipSetRegisters(Instance->IOHandle, 0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				    }
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, 0,Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	   
			   #endif
			ChipSetFieldImage(FSTB6000_F,u8,Instance->TunerRegVal);
			#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
						Error = ChipSetRegisters(Instance->IOHandle, 0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				    }
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, 0,Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	   
			   #endif
			STOS_TaskDelayUs(10000);
			ChipSetFieldImage(FSTB6000_FCL,7,Instance->TunerRegVal);
		}
			#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
						Error = ChipSetRegisters(Instance->IOHandle, 0,Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				    }
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, 0,Instance->TunerRegVal,(Instance->DeviceMap).Registers);
				    }	   
			   #endif
		
		
		    /* read back the bandwidth */
        	    *NewBandWidth = Tuner_GetBandwidth( Instance ) / 1000;
    		    Instance->Status.Bandwidth = *NewBandWidth;
    	 }
	 break;
	 #endif 
	 /*********************/
	 #ifdef STTUNER_DRV_SAT_TUN_STB6110
	 case STTUNER_TUNER_STB6110:
        {
        if(Bandwidth == 0)
		{
			ChipSetFieldImage(FSTB6000_FCL,0,Instance->TunerRegVal);
		
		}
		else
		{
			if((Bandwidth/2) > 36000000)		/* BW/2 > max filter bandwidth (36 Mhz) ==> force to max */
					u8 = 31;
				else if((Bandwidth/2) < 5000000)	/* BW/2 < min filter bandwidth (5 Mhz)  ==> force to min */
					u8 = 0;
				else							 	/* filter = BW/2 */
					u8 = (Bandwidth/2)/1000000 - 5;
		
			STOS_TaskDelayUs(10000);
			#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
		      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
		       {
		              ChipSetField( Instance->IOHandle,FSTB6110_CF,u8);		 /* Set the LPF value */	
					ChipSetField( Instance->IOHandle,FSTB6110_CALRCSTRT,1); /* Start LPF auto calibration*/
					
					i=0;
						while((i<10) &&(ChipGetField (Instance->IOHandle,FSTB6110_CALRCSTRT)!=0))
						{
							STOS_TaskDelayUs(10000);/* wait for LPF auto calibration */
							i++;
				         }
		      }
		     #endif
		     #if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
		            STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,FSTB6110_CF,u8);		 /* Set the LPF value */	
					STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,FSTB6110_CALRCSTRT,1); /* Start LPF auto calibration*/
					
					i=0;
						while((i<10) && (STTUNER_IOREG_GetField(&(Instance->DeviceMap), Instance->IOHandle,FSTB6110_CALRCSTRT)!=0))
						{
							STOS_TaskDelayUs(10000);/* wait for LPF auto calibration */
							i++;
						}
				    }	       
              #endif   
			}
			/*Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle,0, Instance->TunerRegVal,(Instance->DeviceMap).Registers);*/
		
		
		    /* read back the bandwidth */
        	    *NewBandWidth = Tuner_GetBandwidth( Instance ) / 1000;
    		    Instance->Status.Bandwidth = *NewBandWidth;
    	 }
	 break;
	 #endif 
	 
	 /**********************/ 
	 
	
	#ifdef STTUNER_DRV_SAT_TUN_STB6100
	case STTUNER_TUNER_STB6100:
        {
        	if((Bandwidth/2) > 36000000)   /*F[4:0] BW/2 max =31+5=36 mhz for F=31*/
					u8 = 31;
				else if((Bandwidth/2) < 5000000) /* BW/2 min = 5Mhz for F=0 */
					u8 = 0;
				else							 /*if 5 < BW/2 < 36*/
					u8 = (Bandwidth/2)/1000000 - 5;
				
				ChipSetFieldImage(FSTB6100_FCCK,(Instance->Status.Reference/1000000), Instance->TunerRegVal); /* FCL_Clk=FXTAL/FCL=1Mhz */
			   #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				Error = ChipSetRegisters(Instance->IOHandle,IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart,&Instance->TunerRegVal[1],IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
				    }
				#endif
				#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, Instance->DeviceMap.WrStart, Instance->TunerRegVal,(Instance->DeviceMap).Registers); 
				    }	   
			   #endif
	
				ChipSetFieldImage(FSTB6100_F, u8, Instance->TunerRegVal);
				/*Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, Instance->DeviceMap.WrStart, Instance->TunerRegVal,(Instance->DeviceMap).Registers); */
				
		    /* read back the bandwidth */
        	    *NewBandWidth = Tuner_GetBandwidth( Instance ) / 1000;
    		    Instance->Status.Bandwidth = *NewBandWidth;
    	 }
	 break; 
	 #endif 
	
        default:
        	#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
          		STTBX_Print(("%s WARNING - bad tuner specified\n", identity));
		#endif
        break;
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print(("%s BandWidth %u NewBandWidth %u\n", identity, BandWidth, *NewBandWidth));
#endif
    return(Instance->DeviceMap.Error);

}   



/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_ioctl(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
   
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_ioctl()";
#endif
    ST_ErrorCode_t Error=ST_NO_ERROR;
    TUNSDRV_InstanceData_t *Instance;
    #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
     U8 Buffer[30];
    U32 Size;
    U8  SubAddress;
    #endif

    Instance = TUNSDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s tuner driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* raw I/O to device */
        	#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
        	 SubAddress = (U8)(((SATIOCTL_IOARCH_Params_t *)InParams)->SubAddr & 0xFF);
					      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
					     { 
 					       	switch(((SATIOCTL_IOARCH_Params_t *)InParams)->Operation)
 					  	       {
							  	   case STTUNER_IO_SA_READ_NS:
							            Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
				                       Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((SATIOCTL_IOARCH_Params_t *)InParams)->Data,((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							
							     case STTUNER_IO_SA_READ:
							           Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size); /* fix for cable (297 chip) */
							            /* fallthrough (no break;) */
							     case STTUNER_IO_READ:
							           Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((SATIOCTL_IOARCH_Params_t *)InParams)->Data,((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
				                case STTUNER_IO_SA_WRITE_NS:
							            Buffer[0] = SubAddress;
							            memcpy( (Buffer + 1), ((SATIOCTL_IOARCH_Params_t *)InParams)->Data, ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
							            Error = STI2C_WriteNoStop(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize+1, ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     case STTUNER_IO_SA_WRITE:				         
							           Buffer[0] = SubAddress;
								        memcpy( (Buffer + 1), ((SATIOCTL_IOARCH_Params_t *)InParams)->Data, ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
								        Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize+1, ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     case STTUNER_IO_WRITE:
							            Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((SATIOCTL_IOARCH_Params_t *)InParams)->Data, ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     default:
							            Error = STTUNER_ERROR_ID;
							            break;
 				   
 				               }
				        }
            #endif
            		#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					 Error =  STTUNER_IOARCH_ReadWrite( Instance->IOHandle, 
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Operation,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->SubAddr,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Data,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout );
				    }	               
            #endif
            break;

       default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }



#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print(("%s function %d called\n", identity, Function));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_ioaccess()

Description:
    no passthru for a tuner.
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_ioaccess(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle,STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
   
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    IOARCH_Handle_t MyIOHandle;
    TUNSDRV_InstanceData_t *Instance;
    #if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
     U8 Buffer[30];
    U32 Size;
    U8  SubAddress;
    #endif

    Instance = TUNSDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    MyIOHandle = Instance->IOHandle;

        /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */ 
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
         	#if defined(STTUNER_DRV_SAT_STB0900) || defined(STTUNER_DRV_SAT_STX0288)
        	             SubAddress = (U8)(SubAddr & 0xFF);
					      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
					     { 
 					       	switch(Operation)
 					  	       {
							  	   case STTUNER_IO_SA_READ_NS:
							            Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, Timeout, &Size);
				                       Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data,TransferSize, Timeout, &Size);
							            break;
							
							     case STTUNER_IO_SA_READ:
							           Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, Timeout, &Size); /* fix for cable (297 chip) */
							            /* fallthrough (no break;) */
							     case STTUNER_IO_READ:
							           Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data,TransferSize, Timeout, &Size);
							            break;
				                case STTUNER_IO_SA_WRITE_NS:
							            Buffer[0] = SubAddress;
							            memcpy( (Buffer + 1), Data, TransferSize);
							            Error = STI2C_WriteNoStop(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, TransferSize+1, Timeout, &Size);
							            break;
							     case STTUNER_IO_SA_WRITE:				         
							           Buffer[0] = SubAddress;
								        memcpy( (Buffer + 1), Data, TransferSize);
								        Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, TransferSize+1, Timeout, &Size);
							            break;
							     case STTUNER_IO_WRITE:
							            Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data, TransferSize, Timeout, &Size);
							            break;
							     default:
							            Error = STTUNER_ERROR_ID;
							            break;
 				   
 				               }
				        }
            #endif
            		#if defined(STTUNER_DRV_SAT_STV0299)  || defined(STTUNER_DRV_SAT_STB0289) || defined(STTUNER_DRV_SAT_STBADV1) || defined(STTUNER_DRV_SAT_STB0899) || defined(STTUNER_DRV_SAT_STBSTC1) || defined(STTUNER_DRV_SAT_STV0399E) || defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STEM) || defined(STTUNER_DRV_SAT_DUAL_STEM_299) 
				   
				  
				if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					{
					Error = STTUNER_IOARCH_ReadWrite(MyIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
				    }	   
              
            #endif
    }
    else    /* repeater */
    {
        Error = ST_ERROR_FEATURE_NOT_SUPPORTED; /* not supported for this tuner */
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
    STTBX_Print(("%s called\n", identity));
#endif
    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

TUNSDRV_InstanceData_t *TUNSDRV_GetInstFromHandle(TUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV_HANDLES
   const char *identity = "STTUNER tunsdrv.c TUNSDRV_GetInstFromHandle()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Handle));
#endif

    return( (TUNSDRV_InstanceData_t *)Handle );
}


static __inline U32 CalculateSteps(TUNSDRV_InstanceData_t *Instance)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c CalculateSteps()";
#endif
    U32 Steps=0, NCnt, ACnt, Prescaler;

    switch (Instance->PLLType)
    {
        case TUNER_PLL_5522:
        case TUNER_PLL_5655:
            Steps  = ((U32)Instance->IOBuffer[0] & 0x0000007F) << 8;
            Steps |=  (U32)Instance->IOBuffer[1] & 0x000000FF;
            break;

        case TUNER_PLL_5659:
            Steps  = ( (U32)Instance->IOBuffer[0] & 0x0000007F) << 8;
            Steps |= ( (U32)Instance->IOBuffer[1] & 0x000000FF);
            Steps |= (((U32)Instance->IOBuffer[2] & 0x00000060) << 10);
            break;

        case TUNER_PLL_TUA6100:
            Prescaler = (Instance->IOBuffer[1] & 0x40) ? 64 : 32;   /* Prescaler = 64 if high 32 if low */

            NCnt = (( (U32)Instance->IOBuffer[1] & 0x03) << 9) |
                   (( (U32)Instance->IOBuffer[2] & 0xFF) << 1) |
                   (( (U32)Instance->IOBuffer[3] & 0x80) >> 7);     /* N counter value */

            ACnt  =   (U32)Instance->IOBuffer[3] & 0x7F;            /* A counter value */
            Steps = (Prescaler * NCnt) + ACnt; 
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
            STTBX_Print(("%s WARNING - no PLL specified\n", identity));
#endif
            break;
    }

    return(Steps);
}

int checkrange(long freqstart,long frequency,long freqend)
{
	if((frequency>=freqstart) && (frequency<=freqend)) 
	return 1;
	else
	return 0;
}

static __inline U32 CalculateFrequency(TUNSDRV_InstanceData_t *Instance)
{
    U32 F;

    F = ((CalculateSteps(Instance) * (Instance->Status.TunerStep / 2)) - 
            500 * Instance->Status.IntermediateFrequency) / 500;

    F = F * Instance->FreqFactor;

    return(F);
}
#endif   

#ifdef STTUNER_MINIDRIVER
#ifdef ST_OSLINUX
   #include "stos.h"
#else
	  
#include <string.h>                     
#include "stcommon.h"
#include "stlite.h"     /* Standard includes */

/* STAPI */
#include "sttbx.h"
#endif

#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */

#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "iodirect.h"       /* utilities */

#include "tunsdrv.h"       /* header for this file */

#define TUNSDRV_IO_TIMEOUT 10
									
static U32 STB6K_LOOKUP[11][3]=		{                                         /* low         high	      osm */
										950000,		1000000,	0xA,
										1000000,	1076000,	0xC,  
										1076000,	1200000,	0x0,  
										1200000,	1300000,	0x1,
										1300000,	1370000,	0x2,
										1370000,	1470000,	0x4,
										1470000,	1530000,	0x5,
										1530000,	1650000,	0x6,
										1650000,	1800000,	0x8,
										1800000,	1950000,	0xA,
										1950000,	2150000,	0xC
									};

U8 Tuner_Default_Reg[] = {
0x01,0xc1,0x3a,0xe3,0x04,0x07,0x9f,0xdf,0xd0,0x50,0xeb,0x4f
};



/* private variables ------------------------------------------------------- */
#if defined(ST_OS21) || defined(ST_OSLINUX)
static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */
#else
static semaphore_t Lock_InitTermOpenClose; /* guard calls to the functions */
#endif
static TUNSDRV_InstanceData_t *TUNERInstance;

ST_ErrorCode_t tuner_tunsdrv_SetFrequency (U32 Frequency, U32 *ReturnFrequency, U32 SymbolRate);
ST_ErrorCode_t tuner_tunsdrv_SetBandWidth (U32 BandWidth);
BOOL Tuner_GetStatus(void);
int checkrange(long freqstart,long frequency,long freqend);	
			

/*****************************************************
**FUNCTION	::	Tuner_GetStatus
**ACTION	::	To Get the Lock status of PLL.

*****************************************************/

BOOL Tuner_GetStatus(void)
{
    
    BOOL  locked = FALSE;
    U8 u8;
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, 0x00, 0x00, 0, &u8, 1, TRUE);
    if((u8 & 0x01) == 0x01)
    {
    	locked = TRUE;
    }
    else
    {
    	locked = FALSE;
    	
    }
		
    return locked;
}

/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_Init()

Description: called for every perdriver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#if defined(ST_OS21) || defined(ST_OSLINUX)   
    Lock_InitTermOpenClose = semaphore_create_fifo(1);
#else
    semaphore_init_fifo(&Lock_InitTermOpenClose, 1);
#endif  
   
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    TUNERInstance = memory_allocate_clear(InitParams->MemoryPartition, 1, sizeof( TUNSDRV_InstanceData_t ));
    TUNERInstance->MemoryPartition = InitParams->MemoryPartition;
    if (TUNERInstance == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }

    
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    memory_deallocate(TUNERInstance->MemoryPartition, TUNERInstance);
    SEM_UNLOCK(Lock_InitTermOpenClose);
      return(Error);
}


/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Open(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t  *TUNER_Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
   
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* now got pointer to free (and valid) data block */
    *Handle = (U32)TUNERInstance;
     TUNER_Capability->FreqMin = 950000;
     TUNER_Capability->FreqMax = 2150000;
     STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00, 0x00, 0, Tuner_Default_Reg, 0x0C, TRUE);
    /* Write Default tuner setting */
    /***********************************/
    /***********************************/
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   


/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_Close(TUNER_Handle_t Handle, TUNER_CloseParams_t *CloseParams)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
   
    return(Error);

}   



/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_SetFrequency()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_SetFrequency (U32 Frequency, U32 *ReturnFrequency, U32 SymbolRate)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_SetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
   
    U32 frequency;
	U32 stepsize;
	U32 nbsteps;
	U32 divider;
	U32 swallow;
	U8 u8;
	U8 Data;
   
        	u8=0;
        	/******************Gain setting w.r.t SymbolRate***********************/
        	
        	if(SymbolRate >= 20000000)
        	Tuner_Default_Reg[5] = (Tuner_Default_Reg[5] & 0xF0) | 0x7;
		/*ChipSetFieldImage(FSTB6000_G, 10);*/
		else if(SymbolRate >= 10000000)
		Tuner_Default_Reg[5] = (Tuner_Default_Reg[5] & 0xF0) | 0x9;
		else
		Tuner_Default_Reg[5] = (Tuner_Default_Reg[5] & 0xF0) | 0xA;
		
		/*ChipSetFieldImage(FSTB6000_G, 9);*/
        	
        	/**************Selection of ODIV****/
		if(Frequency <= 1075000)
		Tuner_Default_Reg[1] = (Tuner_Default_Reg[1] & 0xEF) | 0x10;
		/***********************************/
		
		while(!checkrange(STB6K_LOOKUP[u8][0],Frequency,STB6K_LOOKUP[u8][1]) && (u8<10)) u8++;
		Tuner_Default_Reg[1] = (Tuner_Default_Reg[1] & 0xF0) | STB6K_LOOKUP[u8][2];
						
		/*frequency = Frequency + hTuner->IF;      */
		if(Frequency <= 1075000)
		frequency = Frequency<<2;
		else
		frequency = Frequency<<1;
		/* Read FSTB6000_R */
		Data = Tuner_Default_Reg[4] & 0x3F;
		stepsize =  2*(4000000 / Data); /* 2 x Fr / R */
		nbsteps = (frequency * 100) / (stepsize / 10);
		divider = nbsteps / 16; /* 32 */
		swallow = nbsteps % 16; /* 32 */
	        
	        Tuner_Default_Reg[2] = (divider>>1) & 0xFF;
	        Tuner_Default_Reg[3] = (Tuner_Default_Reg[3] & 0x7F) | ((divider & 0x01)<<7);
	        Tuner_Default_Reg[3] = (Tuner_Default_Reg[3] & 0xE0) | swallow;
	        /****VCO SEARCH ENABLED***************************/
	        Tuner_Default_Reg[0x1] = (Tuner_Default_Reg[0x1] & 0x7F) | 0x80;
	        /************************************************/
	        /****************VCO SEARCH CLOCK IN FAST MODE*********************/
	        Tuner_Default_Reg[0x1] = (Tuner_Default_Reg[0x1] & 0x9F)/* | 0x20*/;
	        /*************************************************************/
	        
	        /******************PLL LOOP DISABLE*********************/
	        Tuner_Default_Reg[0xA] = (Tuner_Default_Reg[0xA] & 0xEF)/* | 0x10*/;
		/*******************************************************/
		/* Write All Register Together */
		
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00, 0x00, 0, Tuner_Default_Reg, 0x0C, TRUE);
		/*********************PLL LOOP ENABLED************************/
		Tuner_Default_Reg[0xA] = (Tuner_Default_Reg[0xA] & 0xEF) | 0x10;
		/**************************************************************/
		/* Write All Register Together */
		
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00, 0x00, 0, Tuner_Default_Reg, 0x0C, TRUE);
		STOS_TaskDelayUs(20000);
		
		/****************VCO SEARCH DISABLED*****************/
		Tuner_Default_Reg[0x1] = (Tuner_Default_Reg[0x1] & 0x7F);
		/****************************************************/
		/*******************CVO SEARCH CLOCK is TURNED OFF*****************/
		Tuner_Default_Reg[0x1] = (Tuner_Default_Reg[0x1] & 0x9F) | 0x60;
		/******************************************************************/
		/* Write All Register Together */
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00, 0x00, 0, Tuner_Default_Reg, 0x0C, TRUE);
           	
           	/* REad TUner Frequency */
           	divider =	((Tuner_Default_Reg[2]<<1) 
							+ ((Tuner_Default_Reg[3]&0x80)>>7));
							
				swallow = Tuner_Default_Reg[3] & 0x1F;
				nbsteps = divider*16 + swallow;	/* N x P + A ; P=16*/			
				stepsize =  2*(4000000 / (Tuner_Default_Reg[4]&0x3F)); /* 2 x Fr / R */
				
				/*frequency = (nbsteps * stepsize)/1000 - hTuner->IF;*/ /* 2 x Fr x (PxN + A)/R */
				*ReturnFrequency = (nbsteps * (stepsize>>(((Tuner_Default_Reg[1]&0x10)>>4)+1)))/1000; /* 2 x Fr x (PxN + A)/R */
           	
        	   		        
    		return(Error);
}   




/* ----------------------------------------------------------------------------
Name: tuner_tunsdrv_SetBandWidth()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunsdrv_SetBandWidth(U32 Bandwidth)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_SetBandWidth()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
   
    U8 u8;

            	if(Bandwidth == 0)
		{
			Tuner_Default_Reg[0x7] = (Tuner_Default_Reg[0x7] & 0xF8) | 0x00;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00, 0x00, 0, Tuner_Default_Reg, 0x0C, TRUE);
			/* Write ALL Register Together */
		}
		else
		{
			if((Bandwidth/2) > 36000000) 
				u8 = 31;
			
			else
				u8 = (Bandwidth/2)/1000000; 
		
			Tuner_Default_Reg[0x7] = (Tuner_Default_Reg[0x7] & 0xF8) | 0x00;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00, 0x00, 0, Tuner_Default_Reg, 0x0C, TRUE);
			/*STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00,FSTB6000_FCL,1, 0, 1, 20, TRUE);*/
			/* Write ALL Register Together */
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00, 0x00, 0, Tuner_Default_Reg, 0x0C, TRUE);
			Tuner_Default_Reg[0x6] = (Tuner_Default_Reg[0x6] & 0xE0) | (u8 & 0x1F);
			/*STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00,FSTB6000_F,1, u8, 1, 20, TRUE);*/
			/* Write ALL Register Together */
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00, 0x00, 0, Tuner_Default_Reg, 0x0C, TRUE);
			/*STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00,FSTB6000_FCL,1, 7, 1, 20, TRUE);*/
			/* Write ALL Register Together */
			Tuner_Default_Reg[0x7] = (Tuner_Default_Reg[0x7] & 0xF8) | 0x07;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00, 0x00, 0, Tuner_Default_Reg, 0x0C, TRUE);
			
		}		
    return(Error);
}
int checkrange(long freqstart,long frequency,long freqend)
{
	if((frequency>=freqstart) && (frequency<=freqend)) 
	return 1;
	else
	return 0;
}



#endif

/* End of tunsdrv.c */

