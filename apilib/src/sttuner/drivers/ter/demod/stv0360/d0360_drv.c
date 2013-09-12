
/*	standard ansi includes	*/
/*#include "math.h"*/
#ifndef ST_OSLINUX
  #include <stdlib.h> /* for abs() */
#endif
/* Standard includes */
   #include "stlite.h"
 /* STAPI */
   #include "sttbx.h"
 /*C libs */
   
#include "ioreg.h"


#include "360_map.h"

#include "360_echo.h"
#include "360_drv.h"
#include "stddefs.h"

/* STAPI */
#include "sttuner.h"
#include "sti2c.h"
#include "stcommon.h"
#include "stevt.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */ 
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */ 


#include "ioarch.h"     /* I/O for this driver */ 
#include "ioreg.h"      /* I/O for this driver */

#include "d0360.h"      /* top level header for this driver */
#include "tuntdrv.h"

/* Current LLA revision	*/
static ST_Revision_t Revision360  = " STV0360-LLA_REL_2.14E(GUI) ";

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

#define M8_F_GAIN_SRC_HI        0xC /*0xA*/
#define M8_F_GAIN_SRC_LO        0xE4 /*0xB4*/ /*27MHz*/ 
#define M8_F_TRL_NOMRATE0      0x1/*0x0*/
#define M8_F_TRL_NOMRATE8_1    0xAA/*0xAC*/ 
#define M8_F_TRL_NOMRATE16_9    0x56/*0x56*/


#ifdef ST_OS21
#define Delay(x) STOS_TaskDelayUs((signed  int)x);
#else
#define Delay(x) STOS_TaskDelayUs((unsigned int)x);
#endif 

#define UTIL_Delay(micro_sec) STOS_TaskDelayUs(micro_sec)

#define SystemWaitFor(x) UTIL_Delay((x*1000))
TUNTDRV_InstanceData_t *TUNTDRV_GetInstFromHandle(TUNER_Handle_t Handle);



#define CPQ_LIMIT 23

extern 	unsigned int dbg_num_trial;  
#ifdef ST_OSLINUX

   typedef struct div_s
   {
	   int quot;
	   int rem;
   }div_t;

   div_t div(int divisor, int divider)
   {
	   div_t ldiv;
	   ldiv.quot = divisor/divider;
	   ldiv.rem  = divisor%divider;
	   return ldiv;
   }
#endif


/***********************************************************
**FUNCTION	::	Drv0360_GetLLARevision
**ACTION	::	Returns the 360 LLA driver revision
**RETURN	::	Revision360
***********************************************************/
ST_Revision_t Drv0360_GetLLARevision(void)
{
	return (Revision360);
}



/*****************************************************
**FUNCTION	::	FE_360_PowOf2
**ACTION	::	Compute  2^n (where n is an integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/
int FE_360_PowOf2(int number)
{
	int i;
	int result=1;
	
	for(i=0;i<number;i++)
		result*=2;
		
	return result;
}


/*********************************************************
--FUNCTION	::	SpeedInit
--ACTION	::	calculate I2C speed (for SystemWait ...)
				
--PARAMS IN	::	Handle to the Chip
--PARAMS OUT::	NONE
--RETURN	::	#ms for an I2C reading access ..
--********************************************************/
int SpeedInit(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{									
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


}

/*********************************************************
--FUNCTION	::	CheckEPQ
--ACTION	::	calculate I2C speed (for SystemWait ...)
--PARAMS IN	::	Handle to the Chip
--PARAMS OUT::	NONE
--RETURN	::	
--********************************************************/
void CheckEPQ(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_OFDMEchoParams_t *Echo)
{													



unsigned short int epq=0,mode,epq_limit,data;
unsigned short int epq_avg , i;
/****** Here according to FFT Mode and Modualtion different EPQ values has been set****/
mode=STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_MODE);
data=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST);
for(i=0;i<4;i++)
{
   epq+=Get_EPQ(DeviceMap,IOHandle,0);
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
Ack_long_scan(DeviceMap,IOHandle);
}

return;


}


/*****************************************************
--FUNCTION	::	FE_360_TunerSet
--ACTION	::	Search for a valid transponder
--PARAMS IN	::	Handle	==>	Front End Handle
				pSearch ==> Search parameters
				pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/
FE_360_Error_t	FE_360_TunerSet(TUNER_Handle_t Handle, FE_360_InternalParams_t* pParams)

{

FE_360_Error_t error = FE_NO_ERROR;   
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
			if ((Frequency<470000) || (Frequency>862000)) error = FE_BAD_PARAMETER;
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_DTT7572)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_DTT7572,0xC0);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
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
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}

else if (Tuner==STTUNER_TUNER_DTT7578 )  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_DTT7578,0xC0);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_DTT7592)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_DTT7592,0xC0);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_ENG47402G1)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_ENG47402G1,0xC0);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_TDA6650)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDA6650,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_TDM1316)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDM1316,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_TDEB2)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDEB2,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
else if (Tuner==STTUNER_TUNER_TDED4)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDED4,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}

/***********New tuner added ED5058**************/		
else if (Tuner==STTUNER_TUNER_ED5058)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_ED5058,0xC2);
			if ((Frequency<430000) || (Frequency>858000)) /* PARAMETRES A REGLER*/
			{
			error = FE_BAD_PARAMETER;   
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
/************New tuner MIVAR*****************/
else if (Tuner==STTUNER_TUNER_MIVAR)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_MIVAR,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
/************New tuner DTT7102*****************/
else if (Tuner==STTUNER_TUNER_DTT7102)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_DTT7102,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
/************New tuner TECC2849PG*****************/
else if (Tuner==STTUNER_TUNER_TECC2849PG)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TECC2849PG,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
		
/************New tuner TDCC2345*****************/
else if (Tuner==STTUNER_TUNER_TDCC2345)  	
		{
		tuner_tuntdrv_Select(Handle,STTUNER_TUNER_TDCC2345,0xC2);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency(Handle,(int)Frequency,(U32*)&returned_freq);
		}
		

/* *Freq=returned_freq; */
(pParams->Frequency)=returned_freq;
return error; 
}


/*****************************************************
--FUNCTION	::	FE_360_SearchRun
--ACTION	::	Search for Signal, Timing, Carrier and then data at a given Frequency, 
--				in a given range:

--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	Type of the founded signal (if any)

--REMARKS   ::  This function is supposed to replace FE_360_SearchRun according
--				to last findings on SYR block
--***************************************************/
FE_360_SignalStatus_t FE_360_SearchRun(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_360_InternalParams_t *pParams,STTUNER_tuner_instance_t *TunerInstance)
{

STTUNER_InstanceDbase_t     *Inst;
	int dbg_var;/* debug var */
	
	FE_360_SignalStatus_t ret_flag;	
	short int wd,ii,tempo;
	unsigned short int ppm,try,u_var,mode=0,guard,seuil;
		
	U8 v_search,Rvsearch;
	try=0;
	dbg_var=0;
	Inst = STTUNER_GetDrvInst();
	
	do
	{
			ret_flag=LOCK_OK;     
	
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,MODE,pParams->Mode);
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,GUARD,pParams->Guard);		
			STTUNER_IOREG_SetField(DeviceMap,IOHandle, FORCE,pParams->Force);
	
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,0);
			SystemWaitFor(5);


			STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,1);
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
				ret_flag=NOAGC;
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
				ret_flag=NOSYMBOL;
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
			tempo=15<<(2*mode);
			SystemWaitFor(tempo);
		
			
			ppm=0;
			tempo=1<<(2*mode);
			for (ii=0;ii<4;ii++) 
			{
				ppm+=STTUNER_IOREG_GetField(DeviceMap,IOHandle,PPM_CPC);
				SystemWaitFor(tempo);
			}
			
			if (ppm< (CPQ_LIMIT <<(2*mode + 2)) ) 
				{
				#ifdef STTUNER_DEBUG_LOCK_STAGES 
				STTBX_Print(("\n BAD CPQ \n"));
				#endif
				ret_flag=BAD_CPQ;
				
				}
			else
				{
				   try=try;
				}
			try++;
			
	} while ( (try<2) && (ret_flag!=LOCK_OK) );
	
	if ( ret_flag!=LOCK_OK) 
	{
		if (Inst->TRL_IOCTL_Set_Flag==FALSE)
		   {
		if(((ret_flag == BAD_CPQ)||(ret_flag == NOSYMBOL)) && (pParams->TrlNormRateTunning == FALSE) 
	            && (STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_MODE)==STTUNER_MODE_8K))
		{
		   /*SystemWaitFor(200);*/
		   
		   #ifdef STTUNER_DEBUG_LOCK_STAGES
		   STTBX_Print(("\n TRLNORMRATE ALGORITHM CALLED \n"));
		   #endif
		   ret_flag=FE_360_TRLNOMRATE_Tuning(DeviceMap,IOHandle) ;
		  
		   if(ret_flag == LOCK_OK)
		   {
		    
		      pParams->TrlNormRateTunning =TRUE;
		   }
		   else
		   {
		      return ret_flag;   
		   }
		  
		   
		   
	        }
	        else
	        {
		   return ret_flag;
		}
	}
	else	
	{
	   return ret_flag;
	}	
				
	}
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
			   #ifdef STTUNER_DEBUG_LOCK_STAGES
		           STTBX_Print(("\n TPS NOT FOUND \n"));
		           #endif
			   return NOTPS;
			}
		        #ifdef STTUNER_DEBUG_LOCK_STAGES
		        STTBX_Print(("\n TPS LOCKED \n"));
		        #endif
			
			tempo= (14<<(2*mode));
			SystemWaitFor(tempo); /*	dcdc wait for EPQ to be reliable*/
			guard=STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_GUARD);
			if (guard== (unsigned short int )STTUNER_GUARD_1_4) CheckEPQ(DeviceMap,IOHandle,&pParams->Echo);
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
			   #ifdef STTUNER_DEBUG_LOCK_STAGES
		           STTBX_Print(("\n NO PRF FOUND \n"));
		           #endif
			   return NOPRFOUND;
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
			   #ifdef STTUNER_DEBUG_LOCK_STAGES
		           STTBX_Print(("\n LK NOT LOCKED \n"));
		           #endif
			   return NOPRFOUND;
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
  }/** End of if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HIERMODE)!=0)***/
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
		
	v_search=((1<<7)|Rvsearch)&0xbf;
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_VSEARCH,v_search);
	
	/******************************************************************/		
	
	pParams->TrlNormRateTunning =TRUE;
	return	LOCK_OK;
}




/*****************************************************
--FUNCTION	::	FE_360_SearchTerm
--ACTION	::	
--PARAMS IN	::	
--PARAMS OUT::	NONE
--RETURN	::	NONE
--***************************************************/
FE_360_Error_t FE_360_SearchTerm(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_360_InternalParams_t *pParams,FE_360_SearchResult_t *pResult)
{
	FE_360_Error_t error = FE_NO_ERROR;
	U8 constell,counter;
	S8 step;
	
	S32 timing_offset;
	U32 trl_nomrate;
	int offset=0;
        int offsetvar=0;
        char signvalue=0;
	U8 tps_rcvd[3]={0},agc2_max[13]={0},trl_ctl[5]={0},trl_ctl1[2],syr_stat[1]={0},Rtrl_ctl;
	
	
	
	
	if(pParams != NULL)
	{
		
		
		if(pResult != NULL)
		{
			
		/*	if ((pParams->State==LOCK_OK)|| (pParams->State==NOPRFOUND)) */
		
			if ((pParams->State==LOCK_OK))    
		
			{
				pResult->Locked = TRUE;

				STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R_TPS_RCVD2,3,tps_rcvd);
				STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R_SYR_STAT,1,syr_stat);				
				pResult->Mode=(syr_stat[0]&0x04)>>2;
				
				(pParams->Results).Mode=pResult->Mode;
				pResult->Guard=syr_stat[0]&0x03;
				
				(pParams->Results).Guard=pResult->Guard;
				constell = tps_rcvd[0] & 0x03;
				if (constell == 0) pResult->Modulation = STTUNER_MOD_QPSK;
				else if (constell==1) pResult->Modulation= STTUNER_MOD_16QAM;
				else pResult->Modulation= STTUNER_MOD_64QAM;
				(pParams->Results).Modulation=pResult->Modulation;
				pResult->hier=pParams->Hierarchy;
				(pParams->Results).hier=pResult->hier;
				pResult->Hierarchy_Alpha  =(tps_rcvd[0]&0x70)>>4;
				pResult->Hprate=tps_rcvd[1] & 0x07;
				(pParams->Results).Hprate=pResult->Hprate;
				pResult->Lprate=(tps_rcvd[1] &0x70)>>4;
				(pParams->Results).Lprate=pResult->Lprate;
				constell = STTUNER_IOREG_GetField(DeviceMap,IOHandle,PR);
				if (constell==5)	constell = 4;
				pResult->pr = (FE_360_Rate_t) constell;
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
                         
            /*task_delay(time_ticks_per_sec());*/



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
			   
			   timing_offset=trl_ctl[3] + 256*trl_ctl[4];
			   
			   if (timing_offset>=32768) timing_offset-=65536;
		           /*	timing_offset=(timing_offset+10)/20; */ /* rounding */
		        
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
					trl_ctl1[0]=((trl_nomrate%2)<<7) | (Rtrl_ctl&0X7F) ;
					trl_ctl1[1]=trl_nomrate/2;
					STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,trl_ctl1,2);
					SystemWaitFor(1);
				}
			
			pParams->TrlNormRateFineTunning =TRUE;
		      
		      if(pParams->ChannelBWStatus == STTUNER_CHAN_BW_6M)
		      { 
		         pParams->Channel_6M_Trl_Done = TRUE; 
		         pParams->Channel_6M_Trl[0] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LSB);
		         pParams->Channel_6M_Trl[1] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LO);
		         pParams->Channel_6M_Trl[2] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_HI);
		        
		      }
		      
		      if(pParams->ChannelBWStatus == STTUNER_CHAN_BW_7M)
		      { 
		         pParams->Channel_7M_Trl_Done = TRUE;  
		         pParams->Channel_7M_Trl[0] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LSB);
		         pParams->Channel_7M_Trl[1] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LO);
		         pParams->Channel_7M_Trl[2] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_HI);
		         		        
		      }
		      
		      if(pParams->ChannelBWStatus == STTUNER_CHAN_BW_8M)
		      { 
		         pParams->Channel_8M_Trl_Done = TRUE;  
		         pParams->Channel_8M_Trl[0] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LSB);
		         pParams->Channel_8M_Trl[1] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LO);
		         pParams->Channel_8M_Trl[2] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_HI);
		         		        
		      }
			}/*** End of if(pParams->TrlNormRateTunning == FALSE) ***/
			
			
			/* 			end of fine tuning				*/
			}
		
				
			else
			 pResult->Locked = FALSE;
				error = FE_SEARCH_FAILED;
		}
		else
			error = FE_BAD_PARAMETER;
	}
	else
		error = FE_INVALID_HANDLE;
	
	pResult->SignalStatus = pParams->State;
	pResult->State  = STTUNER_IOREG_GetField(DeviceMap,IOHandle,CORE_STATE_STAT);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,FORCE, (pParams->Force & 2)>>1);
	return error;
}





























































































































/*****************************************************
--FUNCTION	::	FE_360_Search
--ACTION	::	Search for a valid transponder
--PARAMS IN	::	Handle	==>	Front End Handle
				pSearch ==> Search parameters
				pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/
FE_360_Error_t	FE_360_Search(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_360_InternalParams_t *pParams,  FE_360_SearchResult_t *pResult,STTUNER_tuner_instance_t *TunerInstance)

{

        

        FE_360_Error_t error = FE_NO_ERROR;
	 U8 trl_ctl[3],gain_src[2];	
	U8 Rgain_src,Rtrl_ctl;	
	STTUNER_InstanceDbase_t     *Inst;
	Inst = STTUNER_GetDrvInst();
	#ifndef HOST_PC
	if (pParams->ChannelBW == STTUNER_CHAN_BW_6M) 
	{
	Rgain_src=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_GAIN_SRC1);
	gain_src[0]=(Rgain_src & 0xf0) | M6_F_GAIN_SRC_HI;
	gain_src[1]=M6_F_GAIN_SRC_LO;
	   STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_GAIN_SRC1,gain_src,2); 
	    if (Inst->TRL_IOCTL_Set_Flag==FALSE)
		   {
	   if(pParams->Channel_6M_Trl_Done == FALSE)
           {
	      if(pParams->Tuner == STTUNER_TUNER_DTT7592)
	      {
	         
                
	       
		 Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
		 trl_ctl[0]=(Rtrl_ctl & 0x7f) | (0x00<<7); 
		 trl_ctl[1]=0xfe;
		 trl_ctl[2]=0x40;
              }
              else
              {
                 
	         Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
                
	       
		 trl_ctl[0]=(Rtrl_ctl & 0x7f) | (M6_F_TRL_NOMRATE0<<7);
		 trl_ctl[1]=M6_F_TRL_NOMRATE8_1;
		 trl_ctl[2]=M6_F_TRL_NOMRATE16_9;
	         
	      }
	   }/*** end of if(pParams->Channel_6M_Trl_Done == FALSE)**/
	   else
	   {
	      
	     Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
	      	trl_ctl[0]=(Rtrl_ctl & 0x7f) | (pParams->Channel_6M_Trl[0]<<7);
		 trl_ctl[1]=pParams->Channel_6M_Trl[1];
		 trl_ctl[2]=pParams->Channel_6M_Trl[2];
	      
	   }/*** end of ELSE of if(pParams->Channel_6M_Trl_Done == FALSE)**/
	   STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,trl_ctl,3);
	}
	}
	else if ((pParams->ChannelBW == STTUNER_CHAN_BW_7M)) 
	{
	
	  
	   Rgain_src=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_GAIN_SRC1);
		gain_src[0]=(Rgain_src & 0xf0) | M7_F_GAIN_SRC_HI;
		gain_src[1]=M7_F_GAIN_SRC_LO;
	   STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_GAIN_SRC1,gain_src,2); 
	    if (Inst->TRL_IOCTL_Set_Flag==FALSE)
		   {
	   if(pParams->Channel_7M_Trl_Done == FALSE)
           {
	      if(pParams->Tuner == STTUNER_TUNER_DTT7592)
	      {
	       
	        Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
		 trl_ctl[0]=(Rtrl_ctl & 0x7f) | (0x00<<7); 
                 trl_ctl[1]=0xd3;
		 trl_ctl[2]=0x4b;
	       
              }
              else
              {
                 
	         Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
                trl_ctl[0]=(Rtrl_ctl & 0x7f) | (M7_F_TRL_NOMRATE0<<7);
		 trl_ctl[1]=M7_F_TRL_NOMRATE8_1;
		 trl_ctl[2]=M7_F_TRL_NOMRATE16_9;
	        
	      }
	   }/*** end of  if(pParams->Channel_7M_Trl_Done == FALSE) **/
	   else
	   {
	     
	      Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
	      	trl_ctl[0]=(Rtrl_ctl & 0x7f) | (pParams->Channel_7M_Trl[0]<<7);
		 trl_ctl[1]=pParams->Channel_7M_Trl[1];
		 trl_ctl[2]=pParams->Channel_7M_Trl[2];
	       
	            
	   }/*** end of ELSE if(pParams->Channel_7M_Trl_Done == FALSE) **/
	   STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,trl_ctl,3);
	}
	}
	else /* Channel Bandwidth = 8M) */
	{
	  
	   Rgain_src=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_GAIN_SRC1);
		gain_src[0]=(Rgain_src & 0xf0) | M8_F_GAIN_SRC_HI;
		gain_src[1]=M8_F_GAIN_SRC_LO;
	   STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_GAIN_SRC1,gain_src,2); 
	   
	   if (Inst->TRL_IOCTL_Set_Flag==FALSE)
		   {
	   if(pParams->Channel_8M_Trl_Done == FALSE)
           {
           	
	      if(pParams->Tuner == STTUNER_TUNER_DTT7592)
	      {
	        
	        Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
		 trl_ctl[0]=(Rtrl_ctl & 0x7f) | (0x00<<7);
		 trl_ctl[1]=0xa8;
		 trl_ctl[2]=0x56;
	       
	         
              }
              else
              {
           
	        
                  Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
		 trl_ctl[0]=(Rtrl_ctl & 0x7f) | (M8_F_TRL_NOMRATE0<<7);
		 trl_ctl[1]=M8_F_TRL_NOMRATE8_1;
		 trl_ctl[2]=M8_F_TRL_NOMRATE16_9;
           
              }
	    
	  
	      
	   }/*** end of if(pParams->Channel_8M_Trl_Done == FALSE) **/
	   else
	   {
	      
	       Rtrl_ctl=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R_TRL_CTL);
	      	trl_ctl[0]=(Rtrl_ctl & 0x7f) | (pParams->Channel_8M_Trl[0]<<7);
		 trl_ctl[1]=pParams->Channel_8M_Trl[1];
		 trl_ctl[2]=pParams->Channel_8M_Trl[2];
	            
	   }/*** end of ELSE of if(pParams->Channel_8M_Trl_Done == FALSE) **/
	  
	   STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,trl_ctl,3);
	}

}
	#endif
	
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,pParams->EchoPos);
	/*   dcdc */

	#ifdef HOST_PC
	repeator = TunerGetRepeator();	
	TunerSetRepeator(1);	
	#endif	
	/*FE_360_TunerSet(TunerInstance->DrvHandle,&pParams->Frequency,(STTUNER_TunerType_t)pParams->Tuner);   */

        /** error checking is done here for the fix of the bug GNBvd20315 **/
   error=FE_360_TunerSet(TunerInstance->DrvHandle,pParams);   
	if(error != FE_NO_ERROR )
	{
	   return error;
	} 
	
	/*TunerSetRepeator(0);*/
	/*   dcdc */
	
	
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH0,0x1E);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH1,0x14);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH2,0xF);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH3,0x9);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH4,0x0);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,VTH5,0x5);

	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_PR,0xAF);
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_VSEARCH,0x30);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,FORCE,(pParams->Force & 1) );
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,FRAPTCR,1);

/****/
     	STTUNER_IOREG_SetField(DeviceMap,IOHandle,BDI_LPSEL,0x00);	
SystemWaitFor(200);
 /***/
		if  (pParams->Inv == STTUNER_INVERSION_NONE)
		
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,1);					
		
		else if  (pParams->Inv == STTUNER_INVERSION) 									
		
						STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,0);					
					
		else if (pParams->Inv == STTUNER_INVERSION_AUTO) 									
				{
					if (pParams->Sense==1) STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,0);
					else STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,1); 
				}
		else if ( (pParams->Inv == STTUNER_INVERSION_UNK) && (!pParams->first_lock))
				{
					if (pParams->Sense==1) STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,0);
					else STTUNER_IOREG_SetField(DeviceMap,IOHandle,INV_SPECTR,1); 					
				}

	
      	
		
		
		





























	
		if ((pParams->Frequency == 0) || (pParams->Tuner == STTUNER_TUNER_NONE))
		{
			error = FE_MISSING_PARAMETER;
		}
		else  
			{
		
	  						
			    pParams->State = FE_360_SearchRun(DeviceMap, IOHandle,pParams,TunerInstance);
								
	  		    FE_360_SearchTerm(DeviceMap, IOHandle,pParams,pResult);

		         }
		
				/** end of first else **/
	
	return error;















































}

/*****************************************************
--FUNCTION	::	FE_360_LookFor
--ACTION	::	Intermediate layer before launching Search
--PARAMS IN	::	Handle	==>	Front End Handle
				pSearch ==> Search parameters
				pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/

FE_360_Error_t	FE_360_LookFor(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_360_SearchParams_t	*pSearch, FE_360_SearchResult_t *pResult,STTUNER_tuner_instance_t *TunerInstance)

{
	
	 

	U8 trials[6];
	S8 num_trials,index;
	FE_360_Error_t error = FE_NO_ERROR;
	U8 flag_spec_inv;
	U8 flag_freq_off;
	U32 frequency;
	U8 flag,inv;
	S8 delta_freq;
	FE_360_InternalParams_t pParams;	
	
		





		frequency		= pSearch->Frequency;		
		
		pParams.I2Cspeed = SpeedInit(DeviceMap,IOHandle);  
			pParams.prev_lock_status = NO_LOCK;
			pParams.first_lock=0;










			/****Initialize TRLNORMRATE TUNNING FLAG*****/
			pParams.TrlNormRateFineTunning = FALSE ;
			/***Initialize channel bandwidth value with a common value as the particular channel
			    bandwidth still not known*****/
			pParams.ChannelBWStatus = STTUNER_CHAN_BW_NONE ;
		
		/************************/
		
		
		pParams.Frequency=pSearch->Frequency;
		pParams.Frequency	= pSearch->Frequency;
		pParams.Mode 		= pSearch->Mode;
		pParams.Guard		= pSearch->Guard;
		pParams.Inv		= pSearch->Inv;		
		pParams.Offset	= pSearch->Offset;
		pParams.Force     = pSearch->Force;
		pParams.ChannelBW	= pSearch->ChannelBW;
		pParams.EchoPos   = pSearch->EchoPos;
		pParams.Hierarchy = pSearch->Hierarchy;
		pParams.Tuner=TunerInstance->Driver->ID;
		pParams.Quartz=0;
		pParams.TrlNormRateTunning=FALSE;
	
	
	
	if(pParams.ChannelBW != pParams.ChannelBWStatus)
	{
	   pParams.ChannelBWStatus=pParams.ChannelBW;
	   pParams.TrlNormRateFineTunning=FALSE;   
	}
	
	
	
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
	
	
	pParams.State	=NODATA;
	pParams.EchoPos   = pSearch->EchoPos;	
	
	
	
	
	
	
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
				if (pParams.first_lock) num_trials=2;
				else num_trials= 4;
				*/
				num_trials= 4;
				}
			else if ((pParams.prev_lock_status)==(LOCK_RI_OFF))
				{
				
				trials[0] = NINV_ROFF;									
				trials[1] =  INV_ROFF;									
				/*
				if (pParams.first_lock) num_trials=1;
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
				if (pParams.first_lock) num_trials=3;
				else num_trials= 6;
				*/
				num_trials= 6;
				}
			
			break;
			
			}
		
			
	
		pResult->SignalStatus=LOCK_KO;		
		index=0;
		pParams.prev_lock_status=NO_LOCK;
		
		while ( ( (index) < num_trials) && (pResult->SignalStatus!=LOCK_OK)) 		 
		{
			
			inv= trials[index]%2;
			
			if ((!pParams.first_lock)||(pParams.Inv == STTUNER_INVERSION_AUTO) || (pParams.Inv == STTUNER_INVERSION_UNK) ) 
				{
					pParams.Sense	=  inv; 
				}
			else
				{
					
				}
			
			delta_freq=(trials[index]/2)*2 - (trials[index]/4)*6; /*circular module */
			pParams.Frequency = frequency+delta_freq * STEP;
					
			/** error checking is done here for the fix of the bug GNBvd20315 **/
			error=FE_360_Search(DeviceMap,IOHandle,&pParams,pResult,TunerInstance); 
			if(error != FE_NO_ERROR)
			{
			   return error;
			}
			
			
			if ((pResult->SignalStatus == NOAGC) || (pResult->SignalStatus == NOSYMBOL) ) break;
		
			if ((pResult->SignalStatus==LOCK_OK))
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
void FE_360_GetNoiseEstimator(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 *pNoise, U32 *pBer)
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
quoz=FE_360_PowOf2(12+2*(STTUNER_IOREG_GetField(DeviceMap,IOHandle,NUM_EVENT1)));

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
/**  fix done here for the bug GNBvd20972 where pNoise is calculated in right percentage **/
*pNoise=((snr/8)*100)/32;
*pBer   = error*(100000000/quoz);

return;
}



/*********************************************************
--FUNCTION	::	FE_360_Tracking
--ACTION	::	Once the chip is locked tracks its state
--PARAMS IN	::	period/duration of tracking in seconds
			::	Handle to Chip
--PARAMS OUT::	NONE
--RETURN	::	NONE
--********************************************************/
void FE_360_Tracking(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_360_SearchResult_t  *Result)
{

int change;
int locka,lockb,wd;
int pr,mod,pr_temp;
unsigned short int mode;
unsigned short int guard=3;

short int tempo;



				
				tempo=900;
				change=0;
				
				mode=Result->Mode;
				guard=Result->Guard;
				if (Result->hier==STTUNER_HIER_LOW_PRIO)
			  {
			     pr=Result->Lprate;
			  }
			  else
			  {
				pr=Result->Hprate;
			  }
				mod=Result->Modulation;
				
				if (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK) || (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,LK))  )      
			  	{
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0);	
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,1);
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0);	
					SystemWaitFor(50); 
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,MODE,STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_MODE));
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,GUARD,STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_GUARD));				
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,FORCE,1);
					Result->Echo.L1s2va3vp4 = 4;    
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,0);
					SystemWaitFor(2);   
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,1);
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0);	
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,1);
					SystemWaitFor(30);   
					STTUNER_IOREG_SetField(DeviceMap,IOHandle,OP2_VAL,0);
					wd=300;
					while ((wd>=0) && (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)) )
						{
							SystemWaitFor(24);
							wd-=24;
						}
			    
					locka=lockb=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK);
					change=0;
					if (locka)
						{
							if(Result->hier==STTUNER_HIER_LOW_PRIO)
							{
							   pr_temp=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LPCODE);
							}
							else
							{
							   pr_temp=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE);
							}
							if( (/*(STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE)*/pr_temp==pr) && (mod==STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST))   )
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
									if(Result->hier==STTUNER_HIER_LOW_PRIO)
							     {
							        pr_temp=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LPCODE);
							     }
							     else
							     {
							        pr_temp=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE);
							     }
									if( (/*STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE)*/pr_temp!=pr) || (mod!=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST))	) change=1;
								}
						}   /*end if locka */
				} /* end if chipget.... */
							if ( (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK ))&&(!change))
								{
										if ( STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_GUARD) == 3)
											{
									/*			pParams->L1s2va3vp4 = 4; */
									
									/* 	it replaces the call to echo_proc  */ 
												
												passive_monitoring(DeviceMap,IOHandle,&(Result->Echo));
												active_monitoring(DeviceMap,IOHandle,&(Result->Echo));
												Echo_long_scan(DeviceMap,IOHandle,&(Result->Echo));
									/*   ********************************** */			
											/*	eco_proc(DeviceMap,IOHandle,&(pParams->Echo));*/
											
												
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
				   if(Result->hier==STTUNER_HIER_LOW_PRIO)
					 {
					    Result->Lprate=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LPCODE);
					 }
					 else
					 {
					    Result->Hprate=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE);
					 }
				  /* pr=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_HPCODE);*/
				mod=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST);				
				  /*(pParams->Results).Hprate=pr;*/
				Result->Modulation=mod;
				
				}
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,FORCE,0);   	
			
	return;

}



/**************************************************************************************/
/***** SET TRLNOMRATE REGISTERS *******************************/

void SET_TRLNOMRATE_REGS(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,short unsigned int value)
{
div_t divi;   

divi=div(value,512); 
STTUNER_IOREG_SetField(DeviceMap,IOHandle,TRL_NOMRATE_HI,divi.quot);
STTUNER_IOREG_SetField(DeviceMap,IOHandle,TRL_NOMRATE_LO,(divi.rem/2));
STTUNER_IOREG_SetField(DeviceMap,IOHandle,TRL_NOMRATE_LSB,(divi.rem%2));

}
/*************************************************************/
short unsigned int GET_TRLNOMRATE_VALUE(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
short unsigned int value;   
value=  STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_HI)*512+STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LO)*2+STTUNER_IOREG_GetField(DeviceMap,IOHandle,TRL_NOMRATE_LSB);	

return value;
}
/**************************************************************/
signed int GET_TRL_OFFSET(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	unsigned int  u_var;
    signed int    s_var;
	signed int i_int;
 	U8 trl_ctl1[5]={0};
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,FREEZE,1);								  
	STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R_TRL_CTL,5,trl_ctl1);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,FREEZE,0);
	
	s_var= 256* trl_ctl1[4]+trl_ctl1[3];
	if (s_var > 32768 ) s_var = s_var - 65536;
	
	u_var=  ((512*trl_ctl1[2])+(trl_ctl1[1]*2) + ((trl_ctl1[0]&0x80)>>7));
	i_int=((signed)(1000000/u_var)*s_var)/2048;


	return i_int;
}

void FE_360_Core_Switch(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,0);
STTUNER_IOREG_SetField(DeviceMap,IOHandle,CORE_ACTIVE,1);
SystemWaitFor(350);
return;
}

FE_360_SignalStatus_t FE_360_TRLNOMRATE_Tuning(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
short unsigned int TRL_center,TRL_value ;
signed int OFFSET1,OFFSET2;
int i;

int Count=0;
        if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)==1)
        {
	   return LOCK_OK ;
	}
	else /** first else**/
	{
	TRL_center= GET_TRLNOMRATE_VALUE(DeviceMap,IOHandle);
	TRL_value=TRL_center;
	for(i=0;i<5;i++)
		{
			TRL_value+=5;
			SET_TRLNOMRATE_REGS(DeviceMap,IOHandle,TRL_value);
			FE_360_Core_Switch(DeviceMap,IOHandle);
			if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)==1)
			break;
		}
	if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)==1)
	return LOCK_OK;
	else /**second else**/
	{
	   TRL_value=TRL_center;
	   
	for(i=0;i<5;i++)
	{
		TRL_value-=5;
		SET_TRLNOMRATE_REGS(DeviceMap,IOHandle,TRL_value);
		FE_360_Core_Switch(DeviceMap,IOHandle);
		if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)==1)
		break;
	}
	if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)==1)
	{
	   OFFSET1=GET_TRL_OFFSET(DeviceMap,IOHandle);
	   if (OFFSET1 >=-5 && OFFSET1 <= 5)
	   {
	      return LOCK_OK;
	   }
	   else
	   {
	      if (OFFSET1>5  )
	      {
	      	do 
	        {
		   OFFSET2=OFFSET1; 
		   SET_TRLNOMRATE_REGS(DeviceMap,IOHandle,GET_TRLNOMRATE_VALUE(DeviceMap,IOHandle)+1);
		   FE_360_Core_Switch(DeviceMap,IOHandle); 
		   OFFSET1= GET_TRL_OFFSET(DeviceMap,IOHandle);
		   Count+=1;
		  
	         }while (((OFFSET1>5) || (OFFSET1<-5))&&(Count<10));
	            return LOCK_OK;
									    
	      }
	      else if( OFFSET1<-5 )
	      {
	         OFFSET2= OFFSET1;
	         do 
	         {
	            OFFSET1=OFFSET2; 
	            SET_TRLNOMRATE_REGS(DeviceMap,IOHandle,GET_TRLNOMRATE_VALUE(DeviceMap,IOHandle)-1);
                    OFFSET2= GET_TRL_OFFSET(DeviceMap,IOHandle);
	            Count+=1;
	          }while (((OFFSET1>5) || (OFFSET1<-5))&&(Count<10));
	        /*  STTBX_Print(".........TRLNOMRATE VALUE HI ......%x\n",GET_TRLNOMRATE_VALUE(hChip)/512);
	         STTBX_Print(".........TRLNOMRATE VALUE LOW ......%x\n",(GET_TRLNOMRATE_VALUE(hChip)%512)/2);
	          STTBX_Print(".........TRLNOMRATE VALUE LOW ......%x\n",(GET_TRLNOMRATE_VALUE(hChip)%512)%2);*/
	          return LOCK_OK;
								    
	      }
	   }/* end of else of if (OFFSET1==0)*/
	}/* end of if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)==1)*/
	
	   return NOTPS;
	
        }/* end of second else */
        }/* end of first else*/
}


/************** Get TPS Cell ID********************/
ST_ErrorCode_t FE_360_GETCELLID(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,Cell_Id_Value * cell_id )
{
		BOOL 	 CELL_ID_FOUND_LSB,
			 CELL_ID_FOUND_MSB;
			 
		ST_ErrorCode_t Error = ST_NO_ERROR;	 
		int FrameId , Cell_1, Cell_2,Cell_id; 
		int Counter=0;
                int Tempbuf_MSB[2] = {0};
                int Tempbuf_LSB[2] = {0}; 
                int Counter_MSB =0 , Counter_LSB =0 , Tempbuf_MSB_Fill_Flag =0 ,Tempbuf_LSB_Fill_Flag=0;   
		U8 tps_rcvd1[7]={0};
		CELL_ID_FOUND_LSB=FALSE;
		CELL_ID_FOUND_MSB=FALSE;
		cell_id->lsb=0;
		cell_id->msb=0;
	        STOS_TaskDelayUs(200000);
		
      for(Counter =0 ;Counter <= 600 ; Counter++)
      {
      	 #ifndef ST_OSLINUX
         STOS_TaskLock();
        
          #endif
         STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R_TPS_RCVD1,7,tps_rcvd1);
         FrameId = ((tps_rcvd1[6]&0x03)&0x03);
         Cell_1=tps_rcvd1[4];
         Cell_2=tps_rcvd1[5];
         #ifndef ST_OSLINUX
         STOS_TaskUnlock();
         #endif
         STOS_TaskDelayUs(6000);
         Cell_id= (Cell_1>>6)+(Cell_2<<2);
               
        
      if (CELL_ID_FOUND_MSB ==FALSE)
      {
         
         if((FrameId==1) ||(FrameId==3))
         {
            if ( Counter_MSB == 0)
            {
               if(Tempbuf_MSB_Fill_Flag ==0)
               {
                  Tempbuf_MSB[0]= Cell_id;
                  Tempbuf_MSB_Fill_Flag ++;
             
               }
               else
               {
                  Tempbuf_MSB[1] = Cell_id;
                  if (Tempbuf_MSB[0] == Tempbuf_MSB[1] )
                  {
                     Counter_MSB ++;
                  
                  }
                  else
                  {
                     Counter_MSB = 0;
                     Tempbuf_MSB_Fill_Flag = 0 ;
                
                  }
              
               }
         
         }/* end of Counter_MSB == 0 */
         else
         {
            Tempbuf_MSB[1] = Cell_id;
            if (Tempbuf_MSB[0] == Tempbuf_MSB[1] )
            {
               Counter_MSB ++;
               if ( Counter_MSB > 3)
               {
                  CELL_ID_FOUND_MSB = TRUE ;
                  cell_id->msb = Tempbuf_MSB[1];
               }
                  
           }
           else
           {
              Counter_MSB = 0;
              Tempbuf_MSB_Fill_Flag = 0 ;
           }
               
         }
         
      }/* end of frameid if */
   }
   if (CELL_ID_FOUND_LSB ==FALSE)
   {
      if((FrameId==0) || (FrameId==2))
      {
         if ( Counter_LSB == 0)
         {
            if(Tempbuf_LSB_Fill_Flag ==0)
            {
               Tempbuf_LSB[0]= Cell_id;
               Tempbuf_LSB_Fill_Flag ++;
            }
            else
            {
               Tempbuf_LSB[1] = Cell_id;
               if (Tempbuf_LSB[0] == Tempbuf_LSB[1] )
               {
                  Counter_LSB ++;
                  
               }
               else
               {
                  Counter_LSB = 0;
                  Tempbuf_LSB_Fill_Flag = 0 ;
               }
            
            }
         
         }/* end of Counter_LSB == 0 */
         else
         {
            Tempbuf_LSB[1] = Cell_id;
            if (Tempbuf_LSB[0] == Tempbuf_LSB[1] )
            {
               Counter_LSB ++;
               if ( Counter_LSB > 3 )
               {
                  CELL_ID_FOUND_LSB = TRUE ;
                  cell_id->lsb = Tempbuf_LSB[1]; 
               }
            }
            else
            {
               Counter_LSB = 0;
               Tempbuf_LSB_Fill_Flag = 0 ;
            }
               
         }/* end of else of if Counter_LSB == 0 */
      }/* end of if((FrameId==0) || (FrameId==2)) */
   } /* end of if (CELL_ID_FOUND_LSB ==FALSE) */
   task_delay((ST_GetClocksPerSecond()/1000)*0);
   if ((CELL_ID_FOUND_MSB ==TRUE ) && (CELL_ID_FOUND_LSB ==TRUE )) 
   {
      break;
   }
         
   }/* for loop */		
   if ((CELL_ID_FOUND_MSB ==FALSE ) && (CELL_ID_FOUND_LSB ==FALSE )) 
   {
      Error = ST_ERROR_TIMEOUT ;
   }
   return Error;
}

				















