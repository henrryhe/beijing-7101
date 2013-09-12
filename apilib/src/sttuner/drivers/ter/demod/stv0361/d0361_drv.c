
/*	standard ansi includes	*/
/*#include "math.h"*/
#ifndef ST_OSLINUX
    
#include  <stdlib.h>
#endif
/*#include "chip.h"*/
#include "ioreg.h"
#ifdef HOST_PC
#include "I2C.h"
#else
#include "sti2c.h"
#endif
#ifdef HOST_PC
/*	generic includes	*/
#include <utility.h>
#include <ansi_c.h>
#include "gen_macros.h"
#include "gen_types.h"
#include "gen_csts.h" 
#include "system.h"
#include "360_util.h"
#endif

#include "361_map.h"

#include "361_echo.h"
#include "361_drv.h"

#ifdef HOST_PC
/*	360 includes	*/
#include "360_GUI.h"
#include "ApplMain.h"
#include "DRIVMAIN.h"
#include "Tun0360.h" 

FILE *fp;


#else
#ifndef ST_OSLINUX
#include "stlite.h"                     /* Standard includes */ 
#endif
#include "stddefs.h"

/* STAPI */
#include "sttuner.h"
#include "sti2c.h"
#include "stcommon.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "stevt.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */ 
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */ 


#include "ioarch.h"     /* I/O for this driver */ 
#include "ioreg.h"      /* I/O for this driver */

#include "d0361.h"      /* top level header for this driver */
#include "tuntdrv.h"


/* Current LLA revision	*/
static ST_Revision_t Revision361  = " STV0361-LLA_REL_3.0(GUI) ";

#define M6_F_GAIN_SRC_HI        0xE/*0x8*/
#define M6_F_GAIN_SRC_LO        0x10/*0x20*/ 
#define M6_F_TRL_NOMRATE0       0x0
#define M6_F_TRL_NOMRATE8_1     0x00/*0x01*/ 
#define M6_F_TRL_NOMRATE16_9    0x41

#define M7_F_GAIN_SRC_HI        0xA
#define M7_F_GAIN_SRC_LO        0xF0/*0x7B*/ 
#define M7_F_TRL_NOMRATE0       0x1/*0x0*/
#define M7_F_TRL_NOMRATE8_1     0xD5/*0xD7*/  
#define M7_F_TRL_NOMRATE16_9    0x4B/*0x4D*/ 

#define M8_F_GAIN_SRC_HI        0xD /*0xA*/
#define M8_F_GAIN_SRC_LO        0xAC /*0xB4*/ /*27MHz*/ 
#define M8_F_TRL_NOMRATE0       0x00 
#define M8_F_TRL_NOMRATE8_1     0xAC  
#define M8_F_TRL_NOMRATE16_9    0x56 

#if defined (ST_OS21) || defined (ST_OSLINUX)
#define Delay(x) STOS_TaskDelay((signed int)x);
#else
#define Delay(x) STOS_TaskDelay((unsigned int)x);
#endif
#define UTIL_Delay(micro_sec) STOS_TaskDelayUs(micro_sec)
#define SystemWaitFor(x) UTIL_Delay((x*1000))

TUNTDRV_InstanceData_t *TUNTDRV_GetInstFromHandle(TUNER_Handle_t Handle);

#endif



#define DELTA 666


/* dcdc #define CPQ_LIMIT 23*/
#define CPQ_LIMIT 23


/***********************************************************
**FUNCTION	::	Drv0361_GetLLARevision
**ACTION	::	Returns the 361 LLA driver revision
**RETURN	::	Revision361
***********************************************************/
ST_Revision_t Drv0361_GetLLARevision(void)
{
	return (Revision361);
}


/*****************************************************
**FUNCTION	::	FE_361_PowOf2
**ACTION	::	Compute  2^n (where n is an integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/
int FE_361_PowOf2(int number)
{
	int i;
	int result=1;
	
	for(i=0;i<number;i++)
		result*=2;
		
	return result;
}


/*********************************************************
--FUNCTION	::	SpeedInit_361
--ACTION	::	calculate I2C speed (for SystemWait ...)
				
--PARAMS IN	::	Handle to the Chip
--PARAMS OUT::	NONE
--RETURN	::	#ms for an I2C reading access ..
--********************************************************/
int SpeedInit_361(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{									
#ifdef HOST_PC
unsigned int i;
int tempo;

tempo= clock();
for (i=0;i<16;i++) STTUNER_IOREG_GetField(DeviceMap,IOHandle,PPM_CPC);

tempo=clock()-tempo;
tempo=(tempo*1000)/CLOCKS_PER_SEC;
tempo=(tempo+4)>>4;	/* 4 not 8 to avoid too rough rounding */

return tempo;
#else
unsigned int i,tempo;
#ifdef ST_OS21
osclock_t     time1,time2,time_diff;
#else
clock_t     time1,time2,time_diff;
#endif

time1 = STOS_time_now();
for (i=0;i<16;i++) STTUNER_IOREG_GetField(DeviceMap,IOHandle,PPM_CPC); time2 = STOS_time_now();
time_diff = STOS_time_minus(time2,time1);


tempo = (time_diff * 1000 ) / ST_GetClocksPerSecond() + 4; /* Duration in milliseconds, + 4 for rounded value */

tempo = tempo << 4;
return tempo;
#endif

}

/*********************************************************
--FUNCTION	::	CheckEPQ_361
--ACTION	::	calculate I2C speed (for SystemWait ...)
--PARAMS IN	::	Handle to the Chip
--PARAMS OUT::	NONE
--RETURN	::	
--********************************************************/
void CheckEPQ_361(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_361_OFDMEchoParams_t *Echo)
{													



unsigned short int epq=0,mode,epq_limit,data;
unsigned short int epq_avg , i;

U8 epq_auto= 0;
/********* To check the status of auto epq**********/
epq_auto=STTUNER_IOREG_GetField(DeviceMap,IOHandle,AUTO_LE_EN);

if(epq_auto==1)
{
   return;
}
/****** Here according to FFT Mode and Modualtion different EPQ values has been set****/
mode=STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_MODE);
data=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST);
for(i=0;i<4;i++)
{
   epq+=Get_EPQ_361(DeviceMap,IOHandle,0);
  /* SystemWaitFor(DELTA);*/
  if(mode == 0)
   {
      SystemWaitFor(25); /* wait for 25 msec for 2k mode to get the epq over a range of time so that the
                           average is better*/   
   }
   else
   {
      SystemWaitFor(100);/* wait for 100 msec for 8k mode to get the epq over a range of time so that the
                           average is better*/
   } 
   
}
epq_avg=epq/4;

/*epq_limit=(6<<(2*mode));*/
if(mode==0)
{
switch(data)
{
	case 0 : /* QPSK */
      epq_limit=9;
      break;
   case 1 : /* 16 QAM */
      epq_limit=7;
      break;
   case 2 : /* 64 QAM */
      epq_limit=6;
      break;
   default :
      epq_limit=6;
      break;
}/* end of switch */
}
else
{
switch(data)
{
	case 0 : /* QPSK */
      epq_limit=35;
      break;
   case 1 : /* 16 QAM */
      epq_limit=28;
      break;
   case 2 : /* 64 QAM */
      epq_limit=24;
      break;
   default :
      epq_limit=24;
      break;
}/* end of switch */

}

if ( epq_avg > epq_limit )
{
Ack_long_scan_361(DeviceMap,IOHandle);
}

return;

















































































































}


/*****************************************************
--FUNCTION	::	FE_361_TunerSet
--ACTION	::	Search for a valid transponder
--PARAMS IN	::	Handle	==>	Front End Handle
				pSearch ==> Search parameters
				pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/
FE_361_Error_t	FE_361_TunerSet(TUNER_Handle_t Handle, FE_361_InternalParams_t* pParams)

{

FE_361_Error_t error = FE_361_NO_ERROR;   
U32 Frequency;
int returned_freq;
#ifndef HOST_PC
TUNTDRV_InstanceData_t *Instance;
#endif

STTUNER_TunerType_t      Tuner;
/*U32 Freq; */

Frequency=pParams->Frequency; 
Tuner= pParams->Tuner;

#ifndef HOST_PC
Instance = TUNTDRV_GetInstFromHandle(Handle);
Instance->ChannelBW=pParams->ChannelBW;
#endif
returned_freq=Frequency ;

if 		(Tuner==STTUNER_TUNER_TDLB7)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDLB7,0xC0);
			if ((Frequency<470000) || (Frequency>862000)) error = FE_361_BAD_PARAMETER;
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_DTT7572)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_DTT7572,0xC0);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else
			{
				 tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
			}
		}
else if (Tuner==STTUNER_TUNER_EAL2780)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_EAL2780,0xC0);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}

else if (Tuner==STTUNER_TUNER_DTT7578)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_DTT7578,0xC0);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_DTT7592)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_DTT7592,0xC0);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_ENG47402G1)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_ENG47402G1,0xC0);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_TDA6650)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDA6650,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_TDM1316)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDM1316,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_TDEB2)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDEB2,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_TDED4)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDED4,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}

/***********New tuner added ED5058**************/		
else if (Tuner==STTUNER_TUNER_ED5058)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_ED5058,0xC2);
			if ((Frequency<430000) || (Frequency>858000)) /* PARAMETRES A REGLER*/
			{
			error = FE_361_BAD_PARAMETER;   
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
/************New tuner MIVAR*****************/
else if (Tuner==STTUNER_TUNER_MIVAR)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_MIVAR,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
/************New tuner DTT7102*****************/
else if (Tuner==STTUNER_TUNER_DTT7102)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_DTT7102,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
/************New tuner TECC2849PG*****************/
else if (Tuner==STTUNER_TUNER_TECC2849PG)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TECC2849PG,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
		
/************New tuner TDCC2345*****************/
else if (Tuner==STTUNER_TUNER_TDCC2345)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDCC2345,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_361_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
		


/* *Freq=returned_freq; */
(pParams->Frequency)=returned_freq;
return error; 
}















































































































































































































































/*****************************************************
--FUNCTION	::	FE_361_SearchRun
--ACTION	::	Search for Signal, Timing, Carrier and then data at a given Frequency, 
--				in a given range:

--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	Type of the founded signal (if any)

--REMARKS   ::  This function is supposed to replace FE_361_SearchRun according
--				to last findings on SYR block
--***************************************************/
FE_361_SignalStatus_t FE_361_SearchRun(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_361_InternalParams_t *pParams,STTUNER_tuner_instance_t *TunerInstance)
{

	U8 v_search,Rvsearch;
	int dbg_var;/* debug var */
	
	FE_361_SignalStatus_t ret_flag;	
	short int wd,ii,tempo;
	unsigned short int ppm,try,u_var,mode,guard,seuil;
	#ifdef HOST_PC
	FILE *fp,*fn;
	/* debug variables */
	static unsigned short int dbg_failure=0;
	#endif
	
	#ifdef LOG
	#ifdef HOST_PC
	fp=fopen("debug_driv.txt","a");
	#endif
	#endif
	try=0;
	if (abs(pParams->Frequency-858000)<=2000)
		{
		dbg_var=0;

		}	
	else
		dbg_var=0;
	do
	{
			ret_flag=LOCK_OK_361;     
	               	STTUNER_IOREG_SetField(DeviceMap,IOHandle,MODE,pParams->Mode);
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,GUARD,pParams->Guard);		
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,FORCE,pParams->Force);
	
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,0);
			if (dbg_var) STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,0);
			if (dbg_var) STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,0);
			SystemWaitFor(5);


			STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,1);
			if (dbg_var) STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,1);
			if (dbg_var) STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,1);

			u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,AGC_LOCK);
			wd=70;
		
			while ((u_var==0) && (wd>0))
			{
				SystemWaitFor(10);	
				wd-=10;
				u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,AGC_LOCK);
			}
			
			if(!u_var)
			{
				ret_flag=NOAGC_361;
				#ifdef HOST_PC
				if(dbg_var) printf("no AGC! \n");
				#endif
				#ifdef STTUNER_DEBUG_LOCK_STAGES
			        STTBX_Print(("\n AGC NOT LOCKED \n"));
			        #endif
				break;
			}
			#ifdef STTUNER_DEBUG_LOCK_STAGES
			STTBX_Print(("\n AGC LOCKED \n"));
			#endif	
						
			wd=120; /* dcdc modifs gbgb */
			
			
			u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_LOCK);
	
			while ( (!u_var) && (wd>0))
			{
				SystemWaitFor(5);
				wd-=5;
				u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_LOCK);
			}
			if(!u_var)
			{
				ret_flag=NOSYMBOL_361;
				#ifdef HOST_PC
				if(dbg_var) printf("no SYR! \n");
				#endif
				#ifdef STTUNER_DEBUG_LOCK_STAGES
                                STTBX_Print(("\n SYMBOL NOT LOCKED \n"));
                                #endif
				break;
			}
			#ifdef STTUNER_DEBUG_LOCK_STAGES
                        STTBX_Print(("\n SYMBOL LOCKED \n"));
                        #endif
                       
                        mode =STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_MODE) ;
			guard=STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_GUARD);
			if(mode == STTUNER_MODE_2K)
			{
			    /***For 2k mode the chp_taps reg. must
			                                             ***always set to 0x01***/
			    STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_CHP_TAPS,0x01);                                           
			}
			else
			{
			
			   STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_CHP_TAPS,0x03);/***For 8k mode the chp_taps reg. must
			                                               ***always set to 0x03***/
			   		  
			}			
			tempo=15<<(2*mode);
			SystemWaitFor(tempo);
		
			
			ppm=0;
			tempo=1<<(2*mode);
			if(mode == STTUNER_MODE_8K)
			{
			   tempo+=30;
			}
			
			for (ii=0;ii<4;ii++) 
			{
				ppm+=STTUNER_IOREG_GetField(DeviceMap,IOHandle,PPM_CPC);
				SystemWaitFor(tempo);
			}
			
			if (ppm< (CPQ_LIMIT <<(2*mode + 2)) ) 
				{				
				ret_flag=BAD_CPQ_361;
				#ifdef HOST_PC
				if(dbg_var) printf("BAD CPQ! \n");
				#endif
				#ifdef STTUNER_DEBUG_LOCK_STAGES 
				STTBX_Print(("\n BAD CPQ \n"));
				#endif
				}
			else
				{
				try=try;
			}
			
			try++;
			
	} while ( (try<2) && (ret_flag!=LOCK_OK_361) );
	if ( ret_flag!=LOCK_OK_361) 
	{
		#ifdef LOG
		#ifdef HOST_PC
		fprintf(fp,"no lock status %d mode %d guard %d ppm %d   num_trial %d\n",ret_flag, STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_MODE),STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_GUARD),ppm,dbg_num_trial);
		fclose(fp);
		#endif
		#endif
		#ifdef HOST_PC
		if(dbg_var) printf("no something! \n");
		#endif
		return ret_flag;
	}
	
	/* dcdc debug */
	if ((try==2) && (ret_flag==LOCK_OK_361)) 
	{
	#ifdef LOG
	#ifdef HOST_PC
	fprintf(fp,"failure %d trial %d\n",dbg_failure++,dbg_num_trial);
	#endif
	#endif
	}
	
	/*dcdc */
	/*
	#ifdef LOG
	fclose(fp);
	return ret_flag;
	
	#endif
	*/
	
	/* end of dcdc debug */
	
	               #ifdef STTUNER_DEBUG_LOCK_STAGES
		       STTBX_Print(("\n GOOD CPQ FOUND \n"));
		       #endif
		       
		       u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK);
			
			
			wd=65<<(2*mode); 
			tempo=4<<(2*mode);			
			while ((!u_var) && (wd>0))
			{
			SystemWaitFor(tempo);
			wd-= tempo;
			u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK);
			}
			
			if (!u_var) 
			{
			#ifdef LOG
			#ifdef HOST_PC
			fprintf(fp,"trial %d notps mode %d guard %d ppm %d\n",dbg_num_trial,mode,guard,ppm);
			fclose(fp);
			#endif
			#endif
			#ifdef HOST_PC
			if(dbg_var) printf("no TPS! \n");
			#endif
			#ifdef STTUNER_DEBUG_LOCK_STAGES
		        STTBX_Print(("\n TPS NOT FOUND \n"));
		        #endif
			return NOTPS_361;
			}
			#ifdef STTUNER_DEBUG_LOCK_STAGES
		        STTBX_Print(("\n TPS LOCKED \n"));
		        #endif
		        
			tempo= (14<<(2*mode));
			SystemWaitFor(tempo); /*	dcdc wait for EPQ to be reliable*/
			guard=STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_GUARD);
			if (guard== (unsigned short int )STTUNER_GUARD_1_4) CheckEPQ_361(DeviceMap,IOHandle,&pParams->Echo);
			wd=36<<(2*mode);		

			u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,PRF);
			tempo=4<<(2*mode);
			while ( (!u_var) && (wd>=0))
			{
			
			SystemWaitFor(tempo);
			wd-=(tempo);
			u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,PRF);
			}
			
			if(!u_var)
			{
			#ifdef LOG
			#ifdef HOST_PC
			fprintf(fp,"trial %d noprf1 \n",dbg_num_trial);
			fclose(fp);
			#endif
			#endif
			#ifdef STTUNER_DEBUG_LOCK_STAGES
		        STTBX_Print(("\n NO PRF FOUND \n"));
		        #endif
			return NOPRFOUND_361;
			}
			#ifdef STTUNER_DEBUG_LOCK_STAGES
		        STTBX_Print(("\n PRF FOUND \n"));
		        #endif
		        
			u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,LK);
			
			wd=75<<(2*mode);/* a revoir */   
			tempo=4<<(2*mode) ;
			while ((!u_var) && (wd>=0))
			{
			SystemWaitFor( tempo);
			wd-=tempo;
			u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,LK);
			}
			if(!u_var)
			{
			#ifdef LOG
			#ifdef HOST_PC
			fprintf(fp,"trial %d no lk \n",dbg_num_trial);
			fclose(fp);
			#endif
			#endif
			#ifdef STTUNER_DEBUG_LOCK_STAGES
		        STTBX_Print(("\n LK NOT LOCKED \n"));
		        #endif
			return NOPRFOUND_361;
		}
		#ifdef STTUNER_DEBUG_LOCK_STAGES
		STTBX_Print(("\n LK LOCKED \n"));
		#endif
		
	/*********Code Added For Hierarchical Modulation****************/
	if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HIERMODE)!=0)
	{
	if ((pParams->Hierarchy== STTUNER_HIER_PRIO_ANY) || (pParams->Hierarchy== STTUNER_HIER_HIGH_PRIO)|| (pParams->Hierarchy== STTUNER_HIER_NONE))
		{
		   /* lock/secure the FEC for HP*/
	     u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE);
	     pParams->Hierarchy=STTUNER_HIER_HIGH_PRIO;
	     
    }/** End of if ((pParams->Hierarchy== STTUNER_HIER_PRIO_ANY) || (pParams->Hierarchy== STTUNER_HIER_HIGH_PRIO)|| (pParams->Hierarchy== STTUNER_HIER_NONE))**/
    else
    {
       u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LPCODE); 
       pParams->Hierarchy=STTUNER_HIER_LOW_PRIO;  
       
    }
  }/***/
  else
  {
     u_var=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE); 
     pParams->Hierarchy=STTUNER_HIER_NONE;   
  }
  if (pParams->Hierarchy==STTUNER_HIER_LOW_PRIO)
  {
     STTUNER_IOREG_SetField(DeviceMap,IOHandle,BDI_LPSEL,0x01);	
  }
  else
  {
     STTUNER_IOREG_SetField(DeviceMap,IOHandle,BDI_LPSEL,0x00);
  }
  if (u_var==4 ) u_var=5;
	switch(u_var)
	{
	case 0:
	seuil=STTUNER_IOREG_GetField(DeviceMap,IOHandle,VTH0);
	
	if (seuil <60) STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH0,45);
	break;
	case 1:
	seuil=STTUNER_IOREG_GetField(DeviceMap,IOHandle,VTH1);
	if (seuil < 40) STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH1,30);
	break;
	case 2:
	seuil=STTUNER_IOREG_GetField(DeviceMap,IOHandle,VTH2);
	if (seuil < 30) STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH2,23);
	break;
	case 3:
	seuil=STTUNER_IOREG_GetField(DeviceMap,IOHandle,VTH3);
	if (seuil < 18) STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH3,14);
	break;
	case 5:
	seuil=STTUNER_IOREG_GetField(DeviceMap,IOHandle,VTH5);
	if (seuil < 10) STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH5,8);
	break;
	default:
	break;
	
						   
	}
	u_var= (1<<u_var);
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_PR,u_var);  
	
	Rvsearch=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_VSEARCH);
	v_search=((1<<7) | Rvsearch) & 0xbf; /*use generic function/will be good for readability*/
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_VSEARCH,v_search);

	return	LOCK_OK_361;
}




/*****************************************************
--FUNCTION	::	FE_361_SearchTerm
--ACTION	::	
--PARAMS IN	::	
--PARAMS OUT::	NONE
--RETURN	::	NONE
--***************************************************/
FE_361_Error_t FE_361_SearchTerm(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_361_InternalParams_t *pParams,FE_361_SearchResult_t *pResult)
{
	FE_361_Error_t error = FE_361_NO_ERROR;
	U8 constell,counter;
	S8 step;
	
	S32 timing_offset;
	U32 trl_nomrate;
	int offset=0;
        int offsetvar=0;
        char signvalue=0;
	
	U8 trl_ctl1[2],tps_rcvd[3],trl_ctl[5]={0},syr_stat[1],agc2_max[13]={0},Rtrl_ctl;

	
	
	if(pParams != NULL)
	{
		
		
		if(pResult != NULL)
		{
			
		/*	if ((pParams->State==LOCK_OK)|| (pParams->State==NOPRFOUND)) */
		
			if ((pParams->State==LOCK_OK_361))    
		
			{
				pResult->Locked = TRUE;
				
				STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R_TPS_RCVD2,3,tps_rcvd);
				
				STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R_SYR_STAT,1,syr_stat);			
				
				/*pResult->Mode=STTUNER_IOREG_FieldGetVal(DeviceMap,SYR_MODE);*/
				pResult->Mode=(syr_stat[0]&0x04)>>2;
				(pParams->Results).Mode=pResult->Mode;
				/*pResult->Guard=STTUNER_IOREG_FieldGetVal(DeviceMap,SYR_GUARD);*/
				pResult->Guard=syr_stat[0]&0x03;
				(pParams->Results).Guard=pResult->Guard;
				constell = tps_rcvd[0] & 0x03;/*STTUNER_IOREG_FieldGetVal(DeviceMap,TPS_CONST);*/
				if (constell == 0) pResult->Modulation = STTUNER_MOD_QPSK;
				else if (constell==1) pResult->Modulation= STTUNER_MOD_16QAM;
				else pResult->Modulation= STTUNER_MOD_64QAM;
				(pParams->Results).Modulation=pResult->Modulation;
				
        pResult->hier=pParams->Hierarchy;
				(pParams->Results).hier=pResult->hier;
				pResult->Hierarchy_Alpha  =tps_rcvd[0];/*STTUNER_IOREG_FieldGetVal(DeviceMap,TPS_HIERMODE);*/

   			
   			
				pResult->Hprate= tps_rcvd[1] & 0x07;/*STTUNER_IOREG_FieldGetVal(DeviceMap,TPS_HPCODE);*/
				
				(pParams->Results).Hprate=pResult->Hprate;
				pResult->Lprate=(tps_rcvd[1] &0x70)>>4;/*STTUNER_IOREG_FieldGetVal(DeviceMap,TPS_LPCODE);*/
				(pParams->Results).Lprate=pResult->Lprate;
				constell = STTUNER_IOREG_GetField(DeviceMap,IOHandle,PR);;
				if (constell==5)	constell = 4;
				pResult->pr = (FE_361_Rate_t) constell;

        (pParams->Results).pr=pResult->pr;
				pResult->Sense=STTUNER_IOREG_GetField(DeviceMap,IOHandle,INV_SPECTR);
				
				/* dcdc modifs per Gilles*/
				pResult->Tuner=pParams->Tuner;
				pParams->first_lock=1;
				/* modifs Tuner */
				
				
				STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R_AGC2MAX,13,agc2_max);
				
				
				pResult->Agc_val=	(agc2_max[9]<<16) 	+  
									((agc2_max[10]&0x0f)<<24) +
									agc2_max[11] +  
									((agc2_max[12]&0x0f)<<8);
		      /************To get exact frequency offset******************/
		        signvalue=STTUNER_IOREG_GetField(DeviceMap,IOHandle,SEXT);
                        offset=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_CRL_FREQ3);
                        offset <<=16;
                        offsetvar=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_CRL_FREQ2);
                        offsetvar <<=8;
                        offset = offset | offsetvar;
                        offsetvar=0;
                        offsetvar=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_CRL_FREQ1);
                        offset =offset | offsetvar ;
                        if(signvalue==1)
                        {
                           offset |=0xff000000;/*** Here sign extension made for the negative number**/
                        }
                        offset =offset /16384;/***offset calculation done according to data sheet of STV0360***/
                        if(pResult->Mode==0)
                        {
                           /*******it is 2k mode***************/
                           offset=((offset*4464)/1000);/*** 1 FFT BIN=4.464khz***/
                        }
						  
                        else
                        {
                            /*******it is 8k mode***************/
                           
                            offset=((offset*11)/10);/*** 1 FFT BIN=1.1khz***/
                        }
                        offset= offset*2; /*** In 361 actual offset shown is half***/
                         /****This block of code ensures the frequency to be corrected only when***
                         *** the offset is in range between 140khz to 180khz both positive and negative***
                         ****direction . This takes care of 166khz offset which sometimes needed******
                         ****to get far away from some interfering analog carrier in some European countries.*****/
                        
                       /* if(((offset>=140) && (offset<=180)) ||((offset<=-140) && (offset>=-180)))
                        {
                            pParams->Frequency-=offset;
                                     
                         }*/
                         /*** Here +/-166 khz offset taken into consideration and the excess
                              offset lying between +/- 140 to 180 khz will be returned to 
                              user for fine tunning ****/
                         
                         if((offset>=140) && (offset<=180))
                         {
                         	 if(pResult->Sense==0)
                         	 {
                         	    pParams->Frequency +=166;
                         	 }
                         	 else
                         	 {
                               pParams->Frequency -=166;
                            }   
                         }
                         else if((offset<=-140) && (offset>=-180))
                         {
                         	 if(pResult->Sense==0)
                         	 {
                         	    pParams->Frequency -=166;
                         	 }
                         	 else
                         	 {
                               pParams->Frequency +=166;
                            }
                         }
                         /***pResult data structure retains the recent data after tuner locks***/
                         pResult->Frequency=pParams->Frequency;
                     /************************************************************************/   
			pResult->Echo_pos=STTUNER_IOREG_GetField(DeviceMap,IOHandle,LONG_ECHO);
						
			if(pParams->TrlNormRateFineTunning == FALSE) 
			{
			 
			 /* fine tuning of timing offset if required */
			
			STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,5,trl_ctl);
			
			
			timing_offset=trl_ctl[3]+ 256*trl_ctl[4];
			  
			if (timing_offset>=32768) timing_offset-=65536;
		
			trl_nomrate=  (512*trl_ctl[2]+trl_ctl[1]*2 + ((trl_ctl[0]&0x80)>>7));

			timing_offset=((signed)(1000000/trl_nomrate)*timing_offset)/2048;
			 
			if (timing_offset<=0) 
				{
				timing_offset=(timing_offset-11)/22;
				step=-1;
				}
			else
				{
				timing_offset=(timing_offset+11)/22;
				step=1;
				}
				
			for (counter=0;counter<abs(timing_offset);counter++)
				{
					
					trl_nomrate+=step;
					
					Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
					trl_ctl1[0]=((trl_nomrate%2)<<7) | (Rtrl_ctl & 0x7f) ;
					trl_ctl1[1]=trl_nomrate/2;
					
					STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,trl_ctl1,2);
					
					SystemWaitFor(1);
				}
		      pParams->TrlNormRateFineTunning =TRUE;
		      
		      if(pParams->ChannelBWStatus == STTUNER_CHAN_BW_6M)
		      { 
		         pParams->Channel_6M_Trl_Done = TRUE; 
		         pParams->Channel_6M_Trl[0] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LSB);
		         pParams->Channel_6M_Trl[1] =  STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LO);
		         pParams->Channel_6M_Trl[2] =  STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_HI);
		        
		      }
		      
		      if(pParams->ChannelBWStatus == STTUNER_CHAN_BW_7M)
		      { 
		         pParams->Channel_7M_Trl_Done = TRUE;  
		         pParams->Channel_7M_Trl[0] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LSB);
		         pParams->Channel_7M_Trl[1] =  STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LO);
		         pParams->Channel_7M_Trl[2] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_HI);
		         
		      }
		      
		      if(pParams->ChannelBWStatus == STTUNER_CHAN_BW_8M)
		      { 
		         pParams->Channel_8M_Trl_Done = TRUE;  
		         pParams->Channel_8M_Trl[0] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LSB);
		         pParams->Channel_8M_Trl[1] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LO);
		         pParams->Channel_8M_Trl[2] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_HI);
		         		        
		      }
			
			/* 			end of fine tuning				*/
		        }/*** End of if(pParams->TrlNormRateFineTunning == FALSE) ***/
			
			}	
			else
				 pResult->Locked = FALSE;
				error = FE_361_SEARCH_FAILED;
		}
		else
			error = FE_361_BAD_PARAMETER;
	}
	else
		error = FE_361_INVALID_HANDLE;
	
	pResult->SignalStatus = pParams->State;
	pResult->State  = STTUNER_IOREG_GetField(DeviceMap,IOHandle,CORE_STATE_STAT);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,FORCE, (pParams->Force & 2)>>1);
	return error;
}













































































































































/*****************************************************
--FUNCTION	::	FE_361_Search
--ACTION	::	Search for a valid transponder
--PARAMS IN	::	Handle	==>	Front End Handle
				pSearch ==> Search parameters
				pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/
FE_361_Error_t	FE_361_Search(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_361_InternalParams_t *pParams, FE_361_SearchResult_t *pResult,STTUNER_tuner_instance_t *TunerInstance)

{

        U8 trl_ctl[3],gain_src[2];
        FE_361_Error_t error = FE_361_NO_ERROR;
	U8 Rgain_src,Rtrl_ctl;	
	
		
		
	
	#ifndef HOST_PC
	if (pParams->ChannelBW == STTUNER_CHAN_BW_6M) 
	{
			
	Rgain_src=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_GAIN_SRC1);
	gain_src[0]=(Rgain_src & 0xf0) | M6_F_GAIN_SRC_HI;
	gain_src[1]=M6_F_GAIN_SRC_LO;
	
	STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_GAIN_SRC1,gain_src,2);

	if(pParams->Channel_6M_Trl_Done == FALSE)
           {
	      Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
                
	       
		 trl_ctl[0]=(Rtrl_ctl & 0x7f) | (M6_F_TRL_NOMRATE0<<7);
		 trl_ctl[1]=M6_F_TRL_NOMRATE8_1;
		 trl_ctl[2]=M6_F_TRL_NOMRATE16_9;
	   }/*** end of if(pParams.Channel_6M_Trl_Done == FALSE)**/
	   else
	   {
	    
	    
		Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
	      	trl_ctl[0]=(Rtrl_ctl & 0x7f) | (pParams->Channel_6M_Trl[0]<<7);
		 trl_ctl[1]=pParams->Channel_6M_Trl[1];
		 trl_ctl[2]=pParams->Channel_6M_Trl[2];
	      
	   }/*** end of ELSE of if(pParams->Channel_6M_Trl_Done == FALSE)**/
	   
	   STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,trl_ctl,3);
	 
	
	}/*** end of STTUNER_CHAN_BW_6M ***/
	else if (pParams->ChannelBW == STTUNER_CHAN_BW_7M) 
	{
	
		Rgain_src=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_GAIN_SRC1);
		gain_src[0]=(Rgain_src & 0xf0) | M7_F_GAIN_SRC_HI;
		gain_src[1]=M7_F_GAIN_SRC_LO;
	 STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_GAIN_SRC1,gain_src,2);
	if(pParams->Channel_7M_Trl_Done == FALSE)
           {
	      Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
                trl_ctl[0]=(Rtrl_ctl & 0x7f) | (M7_F_TRL_NOMRATE0<<7);
		 trl_ctl[1]=M7_F_TRL_NOMRATE8_1;
		 trl_ctl[2]=M7_F_TRL_NOMRATE16_9;
	       
	      
	   }/*** end of  if(pParams->Channel_7M_Trl_Done == FALSE) **/
	   else
	   {
	     Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
	      	trl_ctl[0]=(Rtrl_ctl & 0x7f) | (pParams->Channel_7M_Trl[0]<<7);
		 trl_ctl[1]=pParams->Channel_7M_Trl[1];
		 trl_ctl[2]=pParams->Channel_7M_Trl[2];
	       
	     
	      
	   }/*** end of ELSE if(pParams->Channel_7M_Trl_Done == FALSE) **/
	  
	   STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,trl_ctl,3);
	
	}/**** End of STTUNER_CHAN_BW_7M ***/
	else /* Channel Bandwidth = 8M) */
	{
	
        
       		Rgain_src=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_GAIN_SRC1);
		gain_src[0]=(Rgain_src & 0xf0) |M8_F_GAIN_SRC_HI;
		gain_src[1]=M8_F_GAIN_SRC_LO;
	
	STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_GAIN_SRC1,gain_src,2);
	
	
	
	if(pParams->Channel_8M_Trl_Done == FALSE)
           {
	         
                
		 Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
		
		 trl_ctl[0]=(Rtrl_ctl & 0x7f)| (M8_F_TRL_NOMRATE0<<7);
		 trl_ctl[1]=M8_F_TRL_NOMRATE8_1;
		 trl_ctl[2]=M8_F_TRL_NOMRATE16_9;
		 
		 
			  

           }/*** end of if(pParams->Channel_8M_Trl_Done == FALSE) **/
	   else
	   {
	     
		 Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
	      	trl_ctl[0]=(Rtrl_ctl & 0x7f) | (pParams->Channel_8M_Trl[0]<<7);
		 trl_ctl[1]=pParams->Channel_8M_Trl[1];
		 trl_ctl[2]=pParams->Channel_8M_Trl[2];
	            
	   }/*** end of ELSE of if(pParams->Channel_8M_Trl_Done == FALSE) **/
	    
	   STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,trl_ctl,3);
	      


	   

	}/*** end of STTUNER_CHAN_BW_8M ***/

	
	#endif
	
	
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,pParams->EchoPos) ;
	/*   dcdc */

	#ifdef HOST_PC
	repeator = TunerGetRepeator();	
	TunerSetRepeator(1);	
	#endif

	
        /** error checking is done here for the fix of the bug GNBvd20315 **/
        
   error=FE_361_TunerSet(TunerInstance->DrvHandle,pParams);   
 
	if(error != FE_361_NO_ERROR )
	{
	   return error;
	} 
	#ifdef HOST_PC
	TunerSetRepeator(repeator);
	#endif
	/*TunerSetRepeator(0);*/
	/*   dcdc */
	
	
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH0,0x1E) ;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH1,0x14) ;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH2,0xF) ;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH3,0x9) ;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH4,0x0) ;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH5,0x5) ;

	
	 STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_PR,0xAF);  
	  STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_VSEARCH,0x30);  
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,FORCE,(pParams->Force & 1)) ;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,FRAPTCR,1) ;

		if  (pParams->Inv == STTUNER_INVERSION_NONE)
		
					
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,1) ;					
		
		else if  (pParams->Inv == STTUNER_INVERSION) 									
		
						
						STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,0) ;					
					
		else if (pParams->Inv == STTUNER_INVERSION_AUTO) 									
				{
					if (pParams->Sense==1) STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,0) ;
					else STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,1); 
				}
		else if ( (pParams->Inv == STTUNER_INVERSION_UNK) && (!pParams->first_lock))
				{
					if (pParams->Sense==1) STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,0) ;
					else STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,1) ; 					
				}
	
	
	
	/***********************/
	
	





























	
		if ((pParams->Frequency == 0) || (pParams->Tuner == STTUNER_TUNER_NONE))
		{
			error = FE_361_MISSING_PARAMETER;
		}
		else  
			{
		
	  		
				
			    pParams->State = FE_361_SearchRun(DeviceMap, IOHandle,pParams,TunerInstance);
			   		    	
	  		  FE_361_SearchTerm(DeviceMap,IOHandle,pParams,pResult);
 	
		         }
		
				/** end of first else **/
	
	return error;
}



























































/*****************************************************
--FUNCTION	::	FE_361_LookFor
--ACTION	::	Intermediate layer before launching Search
--PARAMS IN	::	Handle	==>	Front End Handle
				pSearch ==> Search parameters
				pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/

FE_361_Error_t	FE_361_LookFor(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_361_SearchParams_t	*pSearch, FE_361_SearchResult_t *pResult,STTUNER_tuner_instance_t *TunerInstance)

{
	

	
	U8 trials[6];
	S8 num_trials,index;
	FE_361_Error_t error = FE_361_NO_ERROR;
	U8 flag_spec_inv;
	U8 flag_freq_off;
	U32 frequency;
	U8 flag,inv;
	S8 delta_freq;
	FE_361_InternalParams_t pParams;	
	
	





		frequency		= pSearch->Frequency;		
		
			












		
			pParams.I2Cspeed = SpeedInit_361(DeviceMap,IOHandle);  
			pParams.prev_lock_status = NO_LOCK;
			pParams.first_lock=0;
			/****Initialize TRLNORMRATE TUNNING FLAG*****/
			pParams.TrlNormRateFineTunning = FALSE ;
			/***Initialize channel bandwidth value with a common value as the particular channel
			    bandwidth still not known*****/
			pParams.ChannelBWStatus = STTUNER_CHAN_BW_NONE ;
		
		/************************/
		
		
	pParams.Frequency=pSearch->Frequency;
	/* debug code */

	/* end debug code */
	pParams.Mode = pSearch->Mode;
	pParams.Guard = pSearch->Guard;
	pParams.Inv= pSearch->Inv;
	pParams.Hierarchy=pSearch->Hierarchy;
	
	pParams.Force=pSearch->Force + STTUNER_IOREG_GetField(DeviceMap,IOHandle,FORCE)*2;
	pParams.ChannelBW = pSearch->ChannelBW; 
	pParams.Tuner=TunerInstance->Driver->ID;
	pParams.Quartz=0;
	if(pParams.ChannelBW != pParams.ChannelBWStatus)
	{
	   pParams.ChannelBWStatus=pParams.ChannelBW;
	   pParams.TrlNormRateFineTunning=FALSE;   
	}
	
	/****Initialize the trl done parameter with false *****/
			pParams.Channel_6M_Trl_Done = FALSE;
			pParams.Channel_7M_Trl_Done = FALSE;
			pParams.Channel_8M_Trl_Done = FALSE;
			/****Initial normrate setting for 6 Mhz bandwidth***/
			pParams.Channel_6M_Trl[0]=M6_F_TRL_NOMRATE0;
			pParams.Channel_6M_Trl[1]=M6_F_TRL_NOMRATE8_1;
			pParams.Channel_6M_Trl[2]=M6_F_TRL_NOMRATE16_9;
			
			/****Initial normrate setting for 7 Mhz bandwidth***/
			pParams.Channel_7M_Trl[0]=M7_F_TRL_NOMRATE0;
			pParams.Channel_7M_Trl[1]=M7_F_TRL_NOMRATE8_1;
			pParams.Channel_7M_Trl[2]=M7_F_TRL_NOMRATE16_9;
			
			/****Initial normrate setting for 8 Mhz bandwidth***/
			pParams.Channel_8M_Trl[0]=M8_F_TRL_NOMRATE0;
			pParams.Channel_8M_Trl[1]=M8_F_TRL_NOMRATE8_1;
			pParams.Channel_8M_Trl[2]=M8_F_TRL_NOMRATE16_9;

	
	pParams.State	=NODATA_361;
	pParams.EchoPos   = pSearch->EchoPos;	
		/**************************/
	
		pParams.Offset	= pSearch->Offset;
		pParams.Inv	= pSearch->Inv;		
		pParams.Delta	= 0;

		flag_spec_inv	= 0;		
		flag_freq_off	= 0;
		flag			= pSearch->Offset*2 + ((pSearch->Inv>>1)&1);		
					
			switch (flag)
			{
			
			case 0:
			trials[0]=NINV_NOFF;
			trials[1]= INV_NOFF;			
			if ( (pParams.Inv == STTUNER_INVERSION_NONE) || (pParams.Inv == STTUNER_INVERSION))
				{
					num_trials=1;
				}
				
			else
				num_trials=2;
			
			if ( (pParams.first_lock)	&& (pParams.Inv == STTUNER_INVERSION_UNK))
				{
					num_trials=1;	
				}
			break;
			
			case 1:
			trials[0]= NINV_NOFF;
			trials[1]=  INV_NOFF;
			num_trials=2;
			break;
			
			case 2:
			if ((pParams.prev_lock_status)==(LOCK_NO_OFF))
				{
				trials[0] = NINV_NOFF;
				trials[1] = NINV_ROFF;
				trials[2] =  INV_NOFF;									
				trials[3] =  INV_ROFF;									
				if ( (pParams.Inv == STTUNER_INVERSION_NONE) || (pParams.Inv == STTUNER_INVERSION))
				{
					num_trials=2;
				}
				else	num_trials= 4;
			
				
				if ( (pParams.first_lock)	&& (pParams.Inv == STTUNER_INVERSION_UNK))
				{
					num_trials=2;	
				}
				
				
				}
			else if ((pParams.prev_lock_status)==(LOCK_RI_OFF))
				{
				
				trials[0] = NINV_ROFF;									
				trials[1] =  INV_ROFF;									
				if ( (pParams.Inv == STTUNER_INVERSION_NONE) || (pParams.Inv == STTUNER_INVERSION))
				{
					num_trials=1;
				}
				
				else	num_trials= 2;
		
				
				if ( (pParams.first_lock)	&& (pParams.Inv == STTUNER_INVERSION_UNK))
					{
						num_trials=1;	
					}
				}
			else
				{
				trials[0] = NINV_NOFF;
				trials[1] = NINV_LOFF;									
				trials[2] = NINV_ROFF;
				trials[3] =  INV_NOFF;
				trials[4] =  INV_LOFF;									
				trials[5] =  INV_ROFF;
				if ( (pParams.Inv == STTUNER_INVERSION_NONE) || (pParams.Inv == STTUNER_INVERSION))
				{
					num_trials=3;
				}
				
				else	num_trials= 6;
				
				
				
				if ( (pParams.first_lock)	&& (pParams.Inv == STTUNER_INVERSION_UNK))
					{
						num_trials=3;	
					}
				}
			break;
			case 3:
			
			default:
			if ((pParams.prev_lock_status)==(LOCK_NO_OFF))
				{
				trials[0] = NINV_NOFF;
				trials[1] = NINV_ROFF;									
				trials[2] =  INV_NOFF;
				trials[3] =  INV_ROFF;									
				/*
				if (pParams->first_lock) num_trials=2;
				else num_trials= 4;
				*/
				num_trials= 4;
				}
			else if ((pParams.prev_lock_status)==(LOCK_RI_OFF))
				{
				
				trials[0] = NINV_ROFF;									
				trials[1] =  INV_ROFF;									
				/*
				if (pParams->first_lock) num_trials=1;
				else num_trials= 2;
				*/
				num_trials= 2;
				}
			else
				{
				trials[0] = NINV_NOFF;
				trials[1] = NINV_LOFF;									
				trials[2] = NINV_ROFF;
				trials[3] =  INV_NOFF;
				trials[4] =  INV_LOFF;									
				trials[5] =  INV_ROFF;
				/*
				if (pParams->first_lock) num_trials=3;
				else num_trials= 6;
				*/
				num_trials= 6;
				}
			
			break;
			
			}
		
			
	
		pResult->SignalStatus=LOCK_KO_361;		
		index=0;
		pParams.prev_lock_status=NO_LOCK;
		
		while ( ( (index) < num_trials) && (pResult->SignalStatus!=LOCK_OK_361)) 		 
		{
			
			inv= trials[index]%2;
			
			if ((!pParams.first_lock)||(pParams.Inv == STTUNER_INVERSION_AUTO) || (pParams.Inv == STTUNER_INVERSION_UNK) ) 
				{
					pParams.Sense	=  inv; 
				}
			else
				{
					
				}
			
			/*
			inv= trials[index]%2;
			
			if ((!pParams->first_lock)||(pSearch->Inv)) 
				{
					pLook.Inv		=  inv;
					pParams->Sense	=  inv; 
				}
			else
				{
					pLook.Inv		=  pParams->Sense;
				}
			*/			
			
			delta_freq=(trials[index]/2)*2 - (trials[index]/4)*6; /*circular module */
			
			pParams.Frequency = frequency+delta_freq * STEP;
			
			
			/** error checking is done here for the fix of the bug GNBvd20315 **/
			error=FE_361_Search(DeviceMap,IOHandle,&pParams,pResult,TunerInstance); 
			if(error != FE_361_NO_ERROR)
			{
			   return error;
			}
			
			
			if ((pResult->SignalStatus == NOAGC_361) || (pResult->SignalStatus == NOSYMBOL_361) ) break;
		
			if ((pResult->SignalStatus==LOCK_OK_361))
			
				{
				
			   	switch(delta_freq)
					{
					case 0:
						pParams.prev_lock_status=LOCK_NO_OFF;	
						break;
			
					case 2:
						pParams.prev_lock_status=LOCK_RI_OFF;
						break;
				
					case -2:
						pParams.prev_lock_status=LOCK_LE_OFF;
						break;
				
					default:
						pParams.prev_lock_status=LOCK_LE_OFF;
						break;
				}
				
				}
			
			index++;
		}
		
		
		return  error;

}


/*----------------------------------------------------
FUNCTION      GetNoiseEstimator (18Oct01) 
ACTION
PARAMS IN
PARAMS OUT
RETURN
------------------------------------------------------*/
void FE_361_GetNoiseEstimator(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 *pNoise, U32 *pBer)
{
short unsigned int  source,prf;
U32                 quoz,error;
U32 snr=0;

error  = STTUNER_IOREG_GetField(DeviceMap,IOHandle,ERROR_COUNT1_LO);
error += STTUNER_IOREG_GetField(DeviceMap,IOHandle,ERROR_COUNT1_HI) * 256; 
error  = error*100;

quoz=1;
if (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,ERRMODE1))
{
source=STTUNER_IOREG_GetField(DeviceMap,IOHandle,ERR_SOURCE1); 

quoz=FE_361_PowOf2(12+2*(STTUNER_IOREG_GetField(DeviceMap,IOHandle,NUM_EVENT1)));

switch (source)
{
case 0:
quoz=quoz*8;
prf=STTUNER_IOREG_GetField(DeviceMap,IOHandle,PR);

switch(prf)
{
case 0:
error=(U32)(error*(1/2)); /*gbgbcast*/ 
break;

case 1:
error=(U32)(error*(2/3)); /*gbgbcast*/ 
break;

case 2:
error=(U32)(error*(3/4)); /*gbgbcast*/ 
break;

case 3:
error=(U32)(error*(5/6)); /*gbgbcast*/ 
break;

case 4:
error=(U32)(error*(6/7)); /*gbgbcast*/
break;

case 5:
error=(U32)(error*(7/8)); /*gbgbcast*/ 
break;

default:
error=error;
break;

}
break;

case 1:
quoz=quoz*8;
break;

case 2:
break;

case 3:
error=error*188;
break;

default:
error=error;
break;
}
}

snr=STTUNER_IOREG_GetField(DeviceMap,IOHandle,CHC_SNR);



/**pNoise = (snr*10) >> 3;*/
/**  fix done here for the bug GNBvd20972 where pNoise is calculated in right percentage **/
*pNoise=((snr/8)*100)/32;
*pBer   = error*(100000000/quoz); /**** (error/quoz) gives my actual BER . We multiply it with
                                        10*e+8 now and 10*e+2 before for scaling purpose so that we can send a integer 
                                        value to application user . So total scaling factor is 10*e+10*****/  
return;
}



/*********************************************************
--FUNCTION	::	FE_361_Tracking
--ACTION	::	Once the chip is locked tracks its state
--PARAMS IN	::	period/duration of tracking in seconds
			::	Handle to Chip
--PARAMS OUT::	NONE
--RETURN	::	NONE
--********************************************************/
void FE_361_Tracking(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_361_SearchResult_t  *Result)
{

int change;
int locka,lockb,wd;
int pr,mod,pr_temp;
unsigned short int mode;
unsigned short int guard=3;
/*FE_361_InternalParams_ts *pParams;*/
short int tempo;
/*D0361_InstanceData_t    *Instance;*/
/*pParams = (FE_361_InternalParams_t *)Instance->FE_361_Handle;*/
                                
				

				tempo=900;
				change=0;
                                
                               	mode=Result->Mode;
				guard=Result->Guard;
				if (Result->Hierarchy==STTUNER_HIER_LOW_PRIO)
			  {
			     pr=Result->Lprate;
			  }
			  else
			  {
				pr=Result->Hprate;
			  }

			/*	pr=(pParams->Results).Hprate;*/
				mod=Result->Modulation;
				
				if (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)  || (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,LK) )  )      
			  	{
			  		
			  		STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0) ;
					
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,1) ;
					
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0) ;
					SystemWaitFor(50); 
					
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,MODE,(STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_MODE))) ;
					
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,GUARD,(STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_GUARD))) ;			
					
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,FORCE,1) ;
					Result->Echo.L1s2va3vp4 = 4;    
					
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,0) ;
					SystemWaitFor(2);   
					
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,1) ;
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0) ;
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,1) ;
					
					SystemWaitFor(30);   
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0) ;
					
					wd=300;
					while ((wd>=0) && (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK) ) )
						{
							SystemWaitFor(24);
							wd-=24;
						}
			    
					locka=lockb=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK);
					change=0;
					if (locka)
						{
							if(Result->Hierarchy==STTUNER_HIER_LOW_PRIO)
							{
							   pr_temp=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LPCODE);
							}
							else
							{
							   pr_temp=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE);
							}
							if( (pr_temp==pr) && (mod==STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST))   )
								{
									SystemWaitFor(100);
									tempo-=80;
									lockb=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK);
									change=0;
								}
							else 
									change = 1;
						}
					if ((!lockb) && (!change))
						{
							
							STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0);
							STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,1);
							SystemWaitFor(30);
							
							STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0);
							Result->Echo.L1s2va3vp4 = 4; 														
							
							STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,0);
							SystemWaitFor(2); 

							
							STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,1);
							STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0);
							STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,1);
							SystemWaitFor(32);
								
							STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0);							
							wd=300;
							while ((wd>=0) && (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)) )
								{		   
									SystemWaitFor(32);
									wd-=32;
								}
							if (wd>=0)
								{
									if(Result->Hierarchy==STTUNER_HIER_LOW_PRIO)
							     {
							        pr_temp=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LPCODE);
							     }
							     else
							     {
							        pr_temp=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE);
							     }

									if( (pr_temp!=pr) || (mod!=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST))	) change=1;
								}
						}   /*end if locka */
				} /* end if chipget.... */
							if ( (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK))&&(!change))
								{
										if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_GUARD) == 3)
											{
									/*			pParams->L1s2va3vp4 = 4; */
									
									/* 	it replaces the call to echo_proc  */ 
												
												passive_monitoring_361(DeviceMap,IOHandle,&(Result->Echo));
												active_monitoring_361(DeviceMap,IOHandle,&(Result->Echo));
												Echo_long_scan_361(DeviceMap,IOHandle,&(Result->Echo));
												
									/*   ********************************** */			
											
											
												




											}
								}
			if (change)
				{
					if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST)==0) 
						{	
					 		;				
						}
					SystemWaitFor(500);  
					if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST)==0) 
						{	
							SystemWaitFor(400); 			
						}
					
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,MODE,STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_MODE));	
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,GUARD,STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_GUARD));	
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,FORCE,1);	
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0);	
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,1);	
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0);	
					
					if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST)==0) 
					{	
					 ;				
					}
				
				STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,0);
				STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,1);
				
					if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST)==0) 
					{	
					 ;				
					}
							
				SystemWaitFor(400);
				}
			
			if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,LK) )
				{
				 if(Result->Hierarchy==STTUNER_HIER_LOW_PRIO)
					 {
					    Result->Lprate=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LPCODE);
					 }
					 else
					 {
					    Result->Hprate=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE);
					 }
				  
				mod=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST);				
				  
				Result->Modulation=mod;
				
				}
			
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,FORCE,0);
			
			
			
	return;

}				















