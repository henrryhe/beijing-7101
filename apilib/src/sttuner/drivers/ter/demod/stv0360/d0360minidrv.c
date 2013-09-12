
/*	standard ansi includes	*/
/*#include "math.h"*/

#include  <stdlib.h>
#include "chip.h"
#include "sti2c.h"

#include "360_map.h"
#include "360_init.h"
#include "360_echo.h"
#include "360_drv.h"
#include "stlite.h"                     /* Standard includes */ 
#include "stddefs.h"

/* STAPI */
#include "sttuner.h"
#include "sti2c.h"
#include "stcommon.h"
#include "sttbx.h"
#include "stevt.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */ 
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */ 


#include "ioarch.h"     /* I/O for this driver */ 
#include "ioreg.h"      /* I/O for this driver */

#include "d0360.h"      /* top level header for this driver */
#include "tuntdrv.h"

#ifdef STTUNER_MINIDRIVER
#include "iodirect.h"
#endif


/* Current LLA revision	
static ST_Revision_t Revision360  = " STV0360-LLA_REL_2.14E(GUI) ";
*/
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
#define M8_F_TRL_NOMRATE0       0x1/*0x0*/
#define M8_F_TRL_NOMRATE8_1     0xAA/*0xAC*/ 
#define M8_F_TRL_NOMRATE16_9    0x56


#ifdef ST_OS21
#define UTIL_Delay(micro_sec) task_delay((signed  int)(((micro_sec) + 999) / 1000) * ST_GetClocksPerSecond() / 1000)
#define Delay(x) task_delay((signed  int)(ST_GetClocksPerSecond()*x));
#else
#define UTIL_Delay(micro_sec) task_delay((unsigned int)(((micro_sec) + 999) / 1000) * ST_GetClocksPerSecond() / 1000)
#define Delay(x) task_delay((unsigned int)(ST_GetClocksPerSecond()*x));
#endif 

#define SystemWaitFor(x) UTIL_Delay((x*1000))
TUNTDRV_InstanceData_t *TUNTDRV_GetInstFromHandle(TUNER_Handle_t Handle);



#define CPQ_LIMIT 23

extern 	unsigned int dbg_num_trial;  

#define RX123 0x22
#define FX_L  0x2
#define FX_S  0x4


#ifdef STTUNER_MINIDRIVER
	extern D0360_InstanceData_t *DEMODInstance;
	static FE_360_InternalParams_t *Params;
#endif

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
int SpeedInit(void)
{									
unsigned int i,tempo;
#ifdef ST_OS21
osclock_t     time1,time2,time_diff;
#else
clock_t     time1,time2,time_diff;
#endif

time1 = time_now();
for (i=0;i<16;i++) ChipDemodGetField(PPM_CPC); time2 = time_now();
time_diff = time_minus(time2,time1);

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
void CheckEPQ(FE_OFDMEchoParams_t *Echo)
{													



unsigned short int epq=0,mode,epq_limit,data;
unsigned short int epq_avg , i;
/****** Here according to FFT Mode and Modualtion different EPQ values has been set****/
mode=ChipDemodGetField(SYR_MODE);
data=ChipDemodGetField(TPS_CONST);
for(i=0;i<4;i++)
{
   epq+=Get_EPQ(0);
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
Ack_long_scan();
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


/*else if (Tuner==STTUNER_TUNER_DTT7592)  	*/
	
		tuner_tuntdrv_Select(STTUNER_TUNER_DTT7592,0xC0);
			if ((Frequency<51000) || (Frequency>896000)) 
			{
			error = FE_BAD_PARAMETER;
			}
			else tuner_tuntdrv_SetFrequency((int)Frequency,(U32*)&returned_freq);
		
/***********New tuner added ED5058**************/		

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
FE_360_SignalStatus_t FE_360_SearchRun(FE_360_InternalParams_t *pParams,STTUNER_tuner_instance_t *TunerInstance)
{


	int dbg_var;/* debug var */
	/*STCHIP_Handle_t hChip;*/
	FE_360_SignalStatus_t ret_flag;	
	short int wd,ii,tempo;
	unsigned short int ppm,try,u_var,mode,guard,seuil;
		

	try=0;
	dbg_var=0;
	do
	{
			ret_flag=LOCK_OK;     
	
			ChipDemodSetField(MODE,pParams->Mode);
			ChipDemodSetField(GUARD,pParams->Guard);		
			ChipDemodSetField( FORCE,pParams->Force);
	
			ChipDemodSetField(CORE_ACTIVE,0);
			SystemWaitFor(5);


			ChipDemodSetField(CORE_ACTIVE,1);
			u_var=ChipDemodGetField(AGC_LOCK);
			wd=70;
		
			while ((u_var==0) && (wd>0))
			{
				SystemWaitFor(10);	
				wd-=10;
				u_var=ChipDemodGetField(AGC_LOCK);
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
			
			
			u_var=ChipDemodGetField(SYR_LOCK);
	
			while ( (!u_var) && (wd>0))
			{
				SystemWaitFor(5);
				wd-=5;
				u_var=ChipDemodGetField(SYR_LOCK);
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
                        mode =ChipDemodGetField(SYR_MODE) ;
			guard=ChipDemodGetField(SYR_GUARD);			
			tempo=15<<(2*mode);
			SystemWaitFor(tempo);
		
			
			ppm=0;
			tempo=1<<(2*mode);
			for (ii=0;ii<4;ii++) 
			{
				ppm+=ChipDemodGetField(PPM_CPC);
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
		if(((ret_flag == BAD_CPQ)||(ret_flag == NOSYMBOL)) && (pParams->TrlNormRateTunning == FALSE) 
	            && (ChipDemodGetField(SYR_MODE)==STTUNER_MODE_8K))
		{
		   /*SystemWaitFor(200);*/
		   #ifdef STTUNER_DEBUG_LOCK_STAGES
		   STTBX_Print(("\n TRLNORMRATE ALGORITHM CALLED \n"));
		   #endif
		   ret_flag=FE_360_TRLNOMRATE_Tuning() ;
		  
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
	               #ifdef STTUNER_DEBUG_LOCK_STAGES
		       STTBX_Print(("\n GOOD CPQ FOUND \n"));
		       #endif
	               u_var=ChipDemodGetField(TPS_LOCK);
			
			
			wd=65<<(2*mode); 
			tempo=4<<(2*mode);			
			while ((!u_var) && (wd>0))
			{
			SystemWaitFor(tempo);
			wd-= tempo;
			u_var=ChipDemodGetField(TPS_LOCK);
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
			guard=ChipDemodGetField(SYR_GUARD);
			if (guard== (unsigned short int )STTUNER_GUARD_1_4) CheckEPQ(&pParams->Echo);
			wd=36<<(2*mode);		

			u_var=ChipDemodGetField(PRF);
			tempo=4<<(2*mode);
			while ( (!u_var) && (wd>=0.0))
			{
			
			SystemWaitFor(tempo);
			wd-=(tempo);
			u_var=ChipDemodGetField(PRF);
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
			u_var=ChipDemodGetField(LK);
			
			wd=75<<(2*mode);/* a revoir */   
			tempo=4<<(2*mode) ;
			while ((!u_var) && (wd>=0))
			{
			SystemWaitFor( tempo);
			wd-=tempo;
			u_var=ChipDemodGetField(LK);
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
			
	/* lock/secure the FEC for HP*/
	u_var=ChipDemodGetField(TPS_HPCODE);
	if (u_var==4 ) u_var=5;
	switch(u_var)
	{
	case 0:
	seuil=ChipDemodGetField(VTH0);
	
	if (seuil <60) ChipDemodSetField(VTH0,45);
	break;
	case 1:
	seuil=ChipDemodGetField(VTH1);
	if (seuil < 40) ChipDemodSetField(VTH1,30);
	break;
	case 2:
	seuil=ChipDemodGetField(VTH2);
	if (seuil < 30) ChipDemodSetField(VTH2,23);
	break;
	case 3:
	seuil=ChipDemodGetField(VTH3);
	if (seuil < 18) ChipDemodSetField(VTH3,14);
	break;
	case 5:
	seuil=ChipDemodGetField(VTH5);
	if (seuil < 10) ChipDemodSetField(VTH5,8);
	break;
	default:
	break;
	
						   
	}
	
	/* lock/secure the FEC for LP (if needed)*/
	if (ChipDemodGetField(TPS_HIERMODE)!=0)
	{
		u_var=ChipDemodGetField(TPS_LPCODE);
		if (u_var==4 ) u_var=5;
		switch(u_var)
		{
		case 0:
		seuil=ChipDemodGetField(VTH0);
	
		if (seuil <60) ChipDemodSetField(VTH0,45);
		break;
		case 1:
		seuil=ChipDemodGetField(VTH1);
		if (seuil < 40) ChipDemodSetField(VTH1,30);
		break;
		case 2:
		seuil=ChipDemodGetField(VTH2);
		if (seuil < 30) ChipDemodSetField(VTH2,23);
		break;
		case 3:
		seuil=ChipDemodGetField(VTH3);
		if (seuil < 18) ChipDemodSetField(VTH3,14);
		break;
		case 5:
		seuil=ChipDemodGetField(VTH5);
		if (seuil < 10) ChipDemodSetField(VTH5,8);
		break;
		default:
		break;
	
		}
	}	
	
	
	if (ChipDemodGetField(TPS_HIERMODE)==0)	
		{
		u_var=ChipDemodGetField(TPS_HPCODE);
		if (u_var==4 ) u_var=5;    
		u_var= (1<<u_var);
		ChipDemodSetField(R_PR,u_var);  
		/*ChipSetFieldImage(PR_AUTO,1);
		ChipSetFieldImage(PR_FREEZE,0);
		ChipSetRegisters(hChip,R_VSEARCH,1);*/ 
	    ChipDemodSetField (PR_AUTO_N_PR_FREEZE, 0x10);
	}
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
FE_360_Error_t FE_360_SearchTerm(FE_360_InternalParams_t *pParams,FE_360_SearchResult_t *pResult)
{
	FE_360_Error_t error = FE_NO_ERROR;
	U8 constell,counter;
	S8 step;
	U8 nsbuffer[6],bfr;
	S32 timing_offset;
	U32 trl_nomrate, Error;
	int offset=0;
        int offsetvar=0;
        char signvalue=0;
	
		
	
	if(pParams != NULL)
	{
		
		
		if(pResult != NULL)
		{
			
		/*	if ((pParams->State==LOCK_OK)|| (pParams->State==NOPRFOUND)) */
		
			if ((pParams->State==LOCK_OK))    
		
			{
				/*ChipGetRegisters(hChip,R_TPS_RCVD2,3);
				ChipGetRegisters(hChip,R_SYR_STAT,1);				
				pResult->Mode=ChipGetFieldImage(hChip,SYR_MODE);
				pResult->Guard=ChipGetFieldImage(hChip,SYR_GUARD);
				constell = ChipGetFieldImage(hChip,TPS_CONST);*/

                Error=STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R_TPS_RCVD2, 0,0, nsbuffer, 3, FALSE);
                bfr= ChipDemodGetField(R_SYR_STAT) ;
                pResult->Mode=bfr & 0x04;	/*SYR_MODE)*/
				pResult->Guard=bfr & 0x03;	/*SYR_GUARD*/
				constell = nsbuffer[0]&0x03 ; /*TPS_CONST*/

					
				if (constell == 0) pResult->Modulation = STTUNER_MOD_QPSK;
				else if (constell==1) pResult->Modulation= STTUNER_MOD_16QAM;
				else pResult->Modulation= STTUNER_MOD_64QAM;
				
				pResult->hier  = nsbuffer[0]&0x70;   /*HIERMODE*/
				pResult->Hprate= nsbuffer[1]&0x07 ;  /*TPS_HPCODE*/
				pResult->Lprate= nsbuffer[1]&0x70;  /*TPS_LPCODE*/ 
				constell = ChipDemodGetField(PR);
				if (constell==5)	constell = 4;
				pResult->pr = (FE_360_Rate_t) constell;

				pResult->Sense=ChipDemodGetField(INV_SPECTR);
				
				/* dcdc modifs per Gilles*/
				pResult->Tuner=pParams->Tuner;
				pParams->first_lock=1;
				/* modifs Tuner */
				
				/* ChipGetRegisters(hChip,R_AGC2MAX,6);*/
				 Error=STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R_AGC2MAX, 0,0, nsbuffer, 6, FALSE);/* checknab*/
				 /*pResult->Agc_val=	(ChipGetFieldImage(hChip,AGC1_VAL_LO)<<16) 	+  
									(ChipGetFieldImage(hChip,AGC1_VAL_HI)<<24) +
									ChipGetFieldImage(hChip,AGC2_VAL_LO) +  
									(ChipGetFieldImage(hChip,AGC2_VAL_HI)<<8);*/

				pResult->Agc_val=	(ChipDemodGetField(AGC1_VAL_LO)<<16) 	+  
									(ChipDemodGetField(AGC1_VAL_HI)<<24) +
									ChipDemodGetField(AGC2_VAL_LO) +  
									(ChipDemodGetField(AGC2_VAL_HI)<<8);
				
		      /************To get exact frequency offset******************/
		        signvalue=ChipDemodGetField(SEXT);
                        offset=ChipDemodGetField(R_CRL_FREQ3);
                        offset <<=16;
                        offsetvar=ChipDemodGetField(R_CRL_FREQ2);
                        offsetvar <<=8;
                        offset = offset | offsetvar;
                        offsetvar=0;
                        offsetvar=ChipDemodGetField(R_CRL_FREQ1);
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
			pResult->Echo_pos=ChipDemodGetField(LONG_ECHO);
			
			if(pParams->TrlNormRateFineTunning == FALSE) 
			{
			   /* fine tuning of timing offset if required 
			   ChipGetRegisters(hChip,R_TRL_CTL,5);*/

			   Error=STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R_TRL_CTL, 0,0, nsbuffer, 5, FALSE);
			   timing_offset=nsbuffer[3] /*TRL_TOFFSET_LO*/ + 256*nsbuffer[4];/*TRL_TOFFSET_HI*/
			   if (timing_offset>=32768) timing_offset-=65536;
		           /*	timing_offset=(timing_offset+10)/20; */ /* rounding */
		        
			   /*trl_nomrate=  (512*ChipGetFieldImage(hChip,TRL_NOMRATE_HI)+ChipGetFieldImage(hChip,TRL_NOMRATE_LO)*2 + ChipGetFieldImage(hChip,TRL_NOMRATE_LSB));*/
			   trl_nomrate=  (512*nsbuffer[2]/*TRL_NOMRATE_HI*/+nsbuffer[1]*2/*TRL_NOMRATE_LO*/ + (nsbuffer[0] &0x80)/*TRL_NOMRATE_LSB*/);
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
				/*	ChipSetFieldImage(hChip,TRL_NOMRATE_LSB,trl_nomrate%2);
					ChipSetFieldImage(hChip,TRL_NOMRATE_LO,trl_nomrate/2);
					ChipSetRegisters(hChip,R_TRL_CTL,2);*/

					ChipDemodSetField(TRL_NOMRATE_LSB,trl_nomrate%2);
					ChipDemodSetField(TRL_NOMRATE_LO,trl_nomrate/2);
					SystemWaitFor(1);
				}
			
			pParams->TrlNormRateFineTunning =TRUE;
		      
		      if(pParams->ChannelBWStatus == STTUNER_CHAN_BW_6M)
		      { 
		         pParams->Channel_6M_Trl_Done = TRUE; 
		         pParams->Channel_6M_Trl[0] = ChipDemodGetField(TRL_NOMRATE_LSB);
		         pParams->Channel_6M_Trl[1] = ChipDemodGetField(TRL_NOMRATE_LO);
		         pParams->Channel_6M_Trl[2] = ChipDemodGetField(TRL_NOMRATE_HI);
		        
		      }
		      
		      if(pParams->ChannelBWStatus == STTUNER_CHAN_BW_7M)
		      { 
		         pParams->Channel_7M_Trl_Done = TRUE;  
		         pParams->Channel_7M_Trl[0] = ChipDemodGetField(TRL_NOMRATE_LSB);
		         pParams->Channel_7M_Trl[1] = ChipDemodGetField(TRL_NOMRATE_LO);
		         pParams->Channel_7M_Trl[2] = ChipDemodGetField(TRL_NOMRATE_HI);
		         		        
		      }
		      
		      if(pParams->ChannelBWStatus == STTUNER_CHAN_BW_8M)
		      { 
		         pParams->Channel_8M_Trl_Done = TRUE;  
		         pParams->Channel_8M_Trl[0] = ChipDemodGetField(TRL_NOMRATE_LSB);
		         pParams->Channel_8M_Trl[1] = ChipDemodGetField(TRL_NOMRATE_LO);
		         pParams->Channel_8M_Trl[2] = ChipDemodGetField(TRL_NOMRATE_HI);
		         		        
		      }
			}/*** End of if(pParams->TrlNormRateTunning == FALSE) ***/
			
			
			/* 			end of fine tuning				*/
			}
		
				
			else
				error = FE_SEARCH_FAILED;
		}
		else
			error = FE_BAD_PARAMETER;
	}
	else
		error = FE_INVALID_HANDLE;
	
	pResult->SignalStatus = pParams->State;
	pResult->State  = ChipDemodGetField(CORE_STATE_STAT);
	ChipDemodSetField(FORCE, (pParams->Force & 2)>>1);
	return error;
}


/*****************************************************
--FUNCTION	::	FE_360_Init
--ACTION	::	Initialisation of the STV0360 chip
--PARAMS IN	::	pInit	==>	pointer to FE_360_InitParams_t structure
--PARAMS OUT::	NONE
--RETURN	::	Handle to STV0360
--***************************************************/


 FE_360_Handle_t	FE_360_Init(void)

{
	Params = memory_allocate_clear(DEMODInstance->MemoryPartition, 1, sizeof( FE_360_InternalParams_t ));	
	
 	
/*	pParams->Echo = EchoParams;*/			 /* POUR STAPI passer un multiinstance */
	if(Params != NULL)
	{
		/* Chip initialisation */
 /* in internal struct chip is stored */
		
		
/*		pParams->hChip = STV0360_Init(pInit->hDemod);*/
		
		/*if(pParams->hChip != NULL)
		{*/
			/*pParams->Tuner = (STTUNER_TunerType_t)pInit->Tuner;*/
			Params->I2Cspeed = SpeedInit();  
			Params->prev_lock_status = NO_LOCK;
			Params->first_lock=0;
			/****Initialize TRLNORMRATE TUNNING FLAG*****/
			Params->TrlNormRateTunning = FALSE ;
			Params->TrlNormRateFineTunning = FALSE ;
			/***Initialize channel bandwidth value with a common value as the particular channel
			    bandwidth still not known*****/
			Params->ChannelBWStatus = STTUNER_CHAN_BW_NONE ;
			/****Initialize the trl done parameter with false *****/
			Params->Channel_6M_Trl_Done = FALSE;
			Params->Channel_7M_Trl_Done = FALSE;
			Params->Channel_8M_Trl_Done = FALSE;
			/****Initial normrate setting for 6 Mhz bandwidth***/
			Params->Channel_6M_Trl[0]=M6_F_TRL_NOMRATE0;
			Params->Channel_6M_Trl[1]=M6_F_TRL_NOMRATE8_1;
			Params->Channel_6M_Trl[2]=M6_F_TRL_NOMRATE16_9;
			
			/****Initial normrate setting for 7 Mhz bandwidth***/
			Params->Channel_7M_Trl[0]=M7_F_TRL_NOMRATE0;
			Params->Channel_7M_Trl[1]=M7_F_TRL_NOMRATE8_1;
			Params->Channel_7M_Trl[2]=M7_F_TRL_NOMRATE16_9;
			
			/****Initial normrate setting for 8 Mhz bandwidth***/
			Params->Channel_8M_Trl[0]=M8_F_TRL_NOMRATE0;
			Params->Channel_8M_Trl[1]=M8_F_TRL_NOMRATE8_1;
			Params->Channel_8M_Trl[2]=M8_F_TRL_NOMRATE16_9;

		/*}
		else
		{
		
			#ifdef CHIP_STAPI
			memory_deallocate(pInit->hDemod->Chip->MemoryPartition, pParams);
			#endif

			pParams = NULL;
		}*/
	}
	
	return (FE_360_Handle_t) Params;
}

/*****************************************************
--FUNCTION	::	FE_360_SearchInit
--ACTION	::	Set Params fields that are used by the search algorithm   
--PARAMS IN	::	Frequency	=>	Frequency used to start search 
--PARAMS OUT::	NONE
--RETURN	::	NONE
--***************************************************/
FE_360_Error_t FE_360_SearchInit(FE_360_InternalParams_t  *pParams,FE_360_SearchParams_t	*pSearch,STTUNER_tuner_instance_t *TunerInstance )
{

FE_360_Error_t error = FE_NO_ERROR;	
U8 temp_image[3];
/*
do initialization of the chip, for  instance put core in std_by and so on ...
*/

if (pParams==NULL)
	{
	/* no parameters set. Search completely automatic  */
	/* set guessed parameters to most plausible values */
	}
else
	{
/* 	hChip = pParams->hChip;		get the handle */
	
	pParams->Frequency=pSearch->Frequency;
	/* debug code */

	/* end debug code */
	pParams->Mode = pSearch->Mode;
	pParams->Guard = pSearch->Guard;
	pParams->Inv= pSearch->Inv;
	
	pParams->Force=pSearch->Force + ChipDemodGetField(FORCE)*2;
	pParams->ChannelBW = pSearch->ChannelBW; 
	
	if(pParams->ChannelBW != pParams->ChannelBWStatus)
	{
	   pParams->ChannelBWStatus=pParams->ChannelBW;
	   pParams->TrlNormRateTunning=FALSE;  
	   pParams->TrlNormRateFineTunning=FALSE;  
	}
	#ifndef HOST_PC
	if (pParams->ChannelBW == STTUNER_CHAN_BW_6M) 
	{
	   ChipDemodSetField(GAIN_SRC_HI,M6_F_GAIN_SRC_HI); 
	   ChipDemodSetField(GAIN_SRC_LO,M6_F_GAIN_SRC_LO); 
	   /*ChipSetRegisters(hChip,R_GAIN_SRC1,2); */
	   if(pParams->Channel_6M_Trl_Done == FALSE)
           {
	      /*if(pParams->Tuner == STTUNER_TUNER_DTT7592)
	      {
	         ChipSetFieldImage(hChip,TRL_NOMRATE_LSB,0x00);
	         ChipSetFieldImage(hChip,TRL_NOMRATE_LO,0xFE); 
	         ChipSetFieldImage(hChip,TRL_NOMRATE_HI,0x40); */

              temp_image[0]= 0x00; temp_image[1]=0xFE; temp_image[2]=0x40;

              /*}
              else
              {
                 ChipSetFieldImage(hChip,TRL_NOMRATE_LSB,M6_F_TRL_NOMRATE0);
	         ChipSetFieldImage(hChip,TRL_NOMRATE_LO,M6_F_TRL_NOMRATE8_1); 
	         ChipSetFieldImage(hChip,TRL_NOMRATE_HI,M6_F_TRL_NOMRATE16_9); 
	      } */
	   }  /*** end of if(pParams->Channel_6M_Trl_Done == FALSE)**/
	   else
	   {
	      /*ChipSetFieldImage(hChip,TRL_NOMRATE_LSB,pParams->Channel_6M_Trl[0]);
	      ChipSetFieldImage(hChip,TRL_NOMRATE_LO,pParams->Channel_6M_Trl[1]); 
	      ChipSetFieldImage(hChip,TRL_NOMRATE_HI,pParams->Channel_6M_Trl[2]);*/
	     temp_image[0]= pParams->Channel_6M_Trl[0]; 
	     temp_image[1]= pParams->Channel_6M_Trl[1]; 
	     temp_image[2]= pParams->Channel_6M_Trl[2];
	    	   
	   }/*** end of ELSE of if(pParams->Channel_6M_Trl_Done == FALSE)*/
	 /*ChipSetRegisters(hChip,R_TRL_CTL,3);*/
 	}
	else if ((pParams->ChannelBW == STTUNER_CHAN_BW_7M)) 
	{
	   ChipDemodSetField(GAIN_SRC_HI,M7_F_GAIN_SRC_HI); 
	   ChipDemodSetField(GAIN_SRC_LO,M7_F_GAIN_SRC_LO); 
	   
	   if(pParams->Channel_7M_Trl_Done == FALSE)
           {
	     

             temp_image[0]= 0x00; temp_image[1]=0xD3; temp_image[2]=0x4B; 
			 
		
	   }/*** end of  if(pParams->Channel_7M_Trl_Done == FALSE) **/
	   else
	   {
	         temp_image[0]= pParams->Channel_7M_Trl[0]; 
		 temp_image[1]= pParams->Channel_7M_Trl[1]; 
		 temp_image[2]= pParams->Channel_7M_Trl[2];
	   } 
	   /*ChipSetRegisters(hChip,R_TRL_CTL,3);*/
	}
	else /* Channel Bandwidth = 8M) */
	{
	  /* ChipSetFieldImage(hChip,GAIN_SRC_HI,M8_F_GAIN_SRC_HI); 
	   ChipSetFieldImage(hChip,GAIN_SRC_LO,M8_F_GAIN_SRC_LO); 
	   ChipSetRegisters(hChip,R_GAIN_SRC1,2); */
	   ChipDemodSetField(GAIN_SRC_HI,M8_F_GAIN_SRC_HI); 
	   ChipDemodSetField(GAIN_SRC_LO,M8_F_GAIN_SRC_LO); 
	   
	   if(pParams->Channel_8M_Trl_Done == FALSE)
           {
	     
			 temp_image[0]= 0x00; temp_image[1]=0xA8; temp_image[2]=0x56; 
              	      
	   }/*** end of if(pParams->Channel_8M_Trl_Done == FALSE) **/
	   else
	   {
	     		 temp_image[0]= pParams->Channel_8M_Trl[0]; 
		 temp_image[1]= pParams->Channel_8M_Trl[1]; 
		 temp_image[2]= pParams->Channel_8M_Trl[2];
	            
	   }/*** end of ELSE of if(pParams->Channel_8M_Trl_Done == FALSE) **/
	   
	}
    	temp_image[0] = (temp_image[0] &0x80)|(ChipDemodGetField(R_TRL_CTL))&0x7f;
	error=STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R_TRL_CTL, 0,0, temp_image, 3, FALSE);
	
	#endif
	pParams->EchoPos   = pSearch->EchoPos;
	
	ChipDemodSetField(LONG_ECHO,pParams->EchoPos);
	/*   dcdc */

	#ifdef HOST_PC
	repeator = TunerGetRepeator();	
	TunerSetRepeator(1);	
	#endif
	/*FE_360_TunerSet(TunerInstance->DrvHandle,&pParams->Frequency,(STTUNER_TunerType_t)pParams->Tuner);   */

        /** error checking is done here for the fix of the bug GNBvd20315 **/
   error=FE_360_TunerSet(TunerInstance->DrvHandle,(FE_360_InternalParams_t*)pParams);   
	if(error != FE_NO_ERROR )
	{
	   return error;
	} 
	
	/*TunerSetRepeator(0);*/
	/*   dcdc */
	
	
	
	ChipDemodSetField(VTH0,0x1E);
	ChipDemodSetField(VTH1,0x14);
	ChipDemodSetField(VTH2,0xF);
	ChipDemodSetField(VTH3,0x9);
	ChipDemodSetField(VTH4,0x0);
	ChipDemodSetField(VTH5,0x5);

	ChipDemodSetField(R_PR,0xAF);
	ChipDemodSetField(R_VSEARCH,0x30);
	ChipDemodSetField(FORCE,(pParams->Force & 1) );
	ChipDemodSetField(FRAPTCR,1);
	

		if  (pParams->Inv == STTUNER_INVERSION_NONE)
		
					ChipDemodSetField(INV_SPECTR,1);					
		
		else if  (pParams->Inv == STTUNER_INVERSION) 									
		
						ChipDemodSetField(INV_SPECTR,0);					
					
		else if (pParams->Inv == STTUNER_INVERSION_AUTO) 									
				{
					if (pParams->Sense==1) ChipDemodSetField(INV_SPECTR,0);
					else ChipDemodSetField(INV_SPECTR,1); 
				}
		else if ( (pParams->Inv == STTUNER_INVERSION_UNK) && (!pParams->first_lock))
				{
					if (pParams->Sense==1) ChipDemodSetField(INV_SPECTR,0);
					else ChipDemodSetField(INV_SPECTR,1); 					
				}

	}
      
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
FE_360_Error_t	FE_360_Search(FE_360_InternalParams_t *pParams, FE_360_SearchParams_t	*pSearch, FE_360_SearchResult_t *pResult,STTUNER_tuner_instance_t *TunerInstance)

{

        
        FE_360_Error_t error = FE_NO_ERROR;
		
	if ((void*) pParams == NULL)  error =  FE_INVALID_HANDLE;
	else 
		{
		
		/** error checking is done here for the fix of the bug GNBvd20315 **/
		error=FE_360_SearchInit(pParams,pSearch,TunerInstance);        
	 	if(error != FE_NO_ERROR)
	 	{
	 	   return error ;
	 		
	        }  
	
		if ((pParams->Frequency == 0) || (pParams->Tuner == STTUNER_TUNER_NONE))
		{
			error = FE_MISSING_PARAMETER;
		}
		else  
			{
		
	  		/*dcdc	pParams->State = FE_360_SearchRun((FE_360_InternalParams_t *)Handle);*/
				
			    pParams->State = FE_360_SearchRun(pParams,TunerInstance);
								
	  		    FE_360_SearchTerm(pParams,pResult);

		         }
		
				}/** end of first else **/
	
	return error;
}


/*****************************************************
--FUNCTION	::	FE_360_Term
--ACTION	::	Terminate STV0360 chip connection
--PARAMS IN	::	Handle	==>	Front End Handle
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/
void	FE_360_Term(void)
{
	
	memory_deallocate(DEMODInstance->MemoryPartition, Params);

	
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

FE_360_Error_t	FE_360_LookFor(FE_360_InternalParams_t *pParams, FE_360_SearchParams_t	*pSearch, FE_360_SearchResult_t *pResult,STTUNER_tuner_instance_t *TunerInstance)

{
	
	FE_360_SearchParams_t	 pLook; 

	U8 trials[6];
	S8 num_trials,index;
	FE_360_Error_t error = FE_NO_ERROR;
	U8 flag_spec_inv;
	U8 flag_freq_off;
	U32 frequency;
	U8 flag,inv;
	S8 delta_freq;
		
	
	if ((void*)pParams == NULL)  error =  FE_INVALID_HANDLE;
	else 
		{
		
	/*	pParams=(FE_360_InternalParams_t *) Handle;*/
		frequency		= pSearch->Frequency;		
		
		pLook.Frequency	= pSearch->Frequency;
		pLook.Mode 		= pSearch->Mode;
		pLook.Guard		= pSearch->Guard;
		pLook.Inv		= pSearch->Inv;		
		pLook.Offset	= pSearch->Offset;
		pLook.Force     = pSearch->Force;
		pLook.ChannelBW	= pSearch->ChannelBW;
		pLook.EchoPos   = pSearch->EchoPos;
	
		pParams->Offset	= pSearch->Offset;
		pParams->Inv	= pSearch->Inv;		
		pParams->Delta	= 0;

		flag_spec_inv	= 0;		
		flag_freq_off	= 0;
		flag			= pSearch->Offset*2 + ((pSearch->Inv>>1)&1);		
					
			switch (flag)
			{
			
			case 0:
			trials[0]=NINV_NOFF;
			trials[1]= INV_NOFF;			
			if ( (pParams->Inv == STTUNER_INVERSION_NONE) || (pParams->Inv == STTUNER_INVERSION))
				{
					num_trials=1;
				}
				
			else
				num_trials=2;
			
			if ( (pParams->first_lock)	&& (pParams->Inv == STTUNER_INVERSION_UNK))
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
			if ((pParams->prev_lock_status)==(LOCK_NO_OFF))
				{
				trials[0] = NINV_NOFF;
				trials[1] = NINV_ROFF;
				trials[2] =  INV_NOFF;									
				trials[3] =  INV_ROFF;									
				if ( (pParams->Inv == STTUNER_INVERSION_NONE) || (pParams->Inv == STTUNER_INVERSION))
				{
					num_trials=2;
				}
				else	num_trials= 4;
			
				
				if ( (pParams->first_lock)	&& (pParams->Inv == STTUNER_INVERSION_UNK))
				{
					num_trials=2;	
				}
				
				
				}
			else if ((pParams->prev_lock_status)==(LOCK_RI_OFF))
				{
				
				trials[0] = NINV_ROFF;									
				trials[1] =  INV_ROFF;									
				if ( (pParams->Inv == STTUNER_INVERSION_NONE) || (pParams->Inv == STTUNER_INVERSION))
				{
					num_trials=1;
				}
				
				else	num_trials= 2;
		
				
				if ( (pParams->first_lock)	&& (pParams->Inv == STTUNER_INVERSION_UNK))
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
				if ( (pParams->Inv == STTUNER_INVERSION_NONE) || (pParams->Inv == STTUNER_INVERSION))
				{
					num_trials=3;
				}
				
				else	num_trials= 6;
				
				
				
				if ( (pParams->first_lock)	&& (pParams->Inv == STTUNER_INVERSION_UNK))
					{
						num_trials=3;	
					}
				}
			break;
			case 3:
			
			default:
			if ((pParams->prev_lock_status)==(LOCK_NO_OFF))
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
			else if ((pParams->prev_lock_status)==(LOCK_RI_OFF))
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
		
			
	
		pResult->SignalStatus=LOCK_KO;		
		index=0;
		pParams->prev_lock_status=NO_LOCK;
		
		while ( ( (index) < num_trials) && (pResult->SignalStatus!=LOCK_OK)) 		 
		{
			
			inv= trials[index]%2;
			
			if ((!pParams->first_lock)||(pParams->Inv == STTUNER_INVERSION_AUTO) || (pParams->Inv == STTUNER_INVERSION_UNK) ) 
				{
					pParams->Sense	=  inv; 
				}
			else
				{
					
				}
			
			delta_freq=(trials[index]/2)*2 - (trials[index]/4)*6; /*circular module */
			pLook.Frequency = frequency+delta_freq * STEP;
					
			/** error checking is done here for the fix of the bug GNBvd20315 **/
			error=FE_360_Search(pParams,&pLook,pResult,TunerInstance); 
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
						pParams->prev_lock_status=LOCK_NO_OFF;	
						break;
			
					case 2:
						pParams->prev_lock_status=LOCK_RI_OFF;
						break;
				
					case -2:
						pParams->prev_lock_status=LOCK_LE_OFF;
						break;
				
					default:
						pParams->prev_lock_status=LOCK_LE_OFF;
						break;
				}
				
				}
			
			index++;
		}
		
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
void FE_360_GetNoiseEstimator( U32 *pNoise, U32 *pBer)
{
short unsigned int  source,prf;
U32                 quoz,error;
U32 snr=0;

error  = ChipDemodGetField(ERROR_COUNT1_LO);
error += ChipDemodGetField(ERROR_COUNT1_HI) * 256; 
error  = error*100;

quoz=1;
if (!ChipDemodGetField(ERRMODE1))
{
source=ChipDemodGetField(ERR_SOURCE1); 
quoz=FE_360_PowOf2(12+2*(ChipDemodGetField(NUM_EVENT1)));

switch (source)
{
case 0:
quoz=quoz*8;
prf=ChipDemodGetField(PR);

switch(prf)
{
case 0:
error=(U32)(error*(1.0/2.0)); /*gbgbcast*/ 
break;

case 1:
error=(U32)(error*(2.0/3.0)); /*gbgbcast*/ 
break;

case 2:
error=(U32)(error*(3.0/4.0)); /*gbgbcast*/ 
break;

case 3:
error=(U32)(error*(5.0/6.0)); /*gbgbcast*/ 
break;

case 4:
error=(U32)(error*(6.0/7.0)); /*gbgbcast*/
break;

case 5:
error=(U32)(error*(7.0/8.0)); /*gbgbcast*/ 
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

snr=ChipDemodGetField(CHC_SNR);
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
void FE_360_Tracking(FE_360_InternalParams_t *pParams)
{
int change;
int locka,lockb,wd;
int pr,mod;
unsigned short int mode;
unsigned short int guard=3;

short int tempo;



			/*	hChip = pParams->hChip;*/
				tempo=900;
				change=0;
				
				mode=(pParams->Results).Mode;
				guard=(pParams->Results).Guard;
				pr=(pParams->Results).Hprate;
				mod=(pParams->Results).Modulation;
				
				if (!ChipDemodGetField(TPS_LOCK) || (!ChipDemodGetField(LK))  )      
			  	{
					ChipDemodSetField(OP2_VAL,0);	
					ChipDemodSetField(OP2_VAL,1);
					ChipDemodSetField(OP2_VAL,0);	
					SystemWaitFor(50); 
					ChipDemodSetField(MODE,ChipDemodGetField(SYR_MODE));
					ChipDemodSetField(GUARD,ChipDemodGetField(SYR_GUARD));				
					ChipDemodSetField(FORCE,1);
					pParams->Echo.L1s2va3vp4 = 4;    
					ChipDemodSetField(CORE_ACTIVE,0);
					SystemWaitFor(2);   
					ChipDemodSetField(CORE_ACTIVE,1);
					ChipDemodSetField(OP2_VAL,0);	
					ChipDemodSetField(OP2_VAL,1);
					SystemWaitFor(30);   
					ChipDemodSetField(OP2_VAL,0);
					wd=300;
					while ((wd>=0) && (!ChipDemodGetField(TPS_LOCK)) )
						{
							SystemWaitFor(24);
							wd-=24;
						}
			    
					locka=lockb=ChipDemodGetField(TPS_LOCK);
					change=0;
					if (locka)
						{
							if( (ChipDemodGetField(TPS_HPCODE)==pr) && (mod==ChipDemodGetField(TPS_CONST))   )
								{
									SystemWaitFor(100);
									tempo-=80;
									lockb=ChipDemodGetField(TPS_LOCK);
									change=0;
								}
							else 
									change = 1;
						}
					if ((!lockb) && (!change))
						{
							ChipDemodSetField(OP2_VAL,0);	
							ChipDemodSetField(OP2_VAL,1);
							SystemWaitFor(30);
							ChipDemodSetField(OP2_VAL,0);
							pParams->Echo.L1s2va3vp4 = 4; 														
							ChipDemodSetField(CORE_ACTIVE,0);
							SystemWaitFor(2); 

							ChipDemodSetField(CORE_ACTIVE,1);
							ChipDemodSetField(OP2_VAL,0);	
							ChipDemodSetField(OP2_VAL,1);
							SystemWaitFor(32);
							ChipDemodSetField(OP2_VAL,0);								
							wd=300;
							while ((wd>=0) && (!ChipDemodGetField(TPS_LOCK)) )
								{		   
									SystemWaitFor(32);
									wd-=32;
								}
							if (wd>=0)
								{
									if( (ChipDemodGetField(TPS_HPCODE)!=pr) || (mod!=ChipDemodGetField(TPS_CONST))	) change=1;
								}
						}   /*end if locka */
				} /* end if chipget.... */
							if ( (ChipDemodGetField(TPS_LOCK ))&&(!change))
								{
										if ( ChipDemodGetField(SYR_GUARD) == 3)
											{
									/*			pParams->L1s2va3vp4 = 4; */
									
									/* 	it replaces the call to echo_proc  */ 
												
												passive_monitoring(&(pParams->Echo));
												active_monitoring(&(pParams->Echo));
												Echo_long_scan(&(pParams->Echo));
									/*   ********************************** */			
											/*	eco_proc(hChip,&(pParams->Echo));*/
											
												
											}
								}
			if (change)
				{
					if (ChipDemodGetField(TPS_CONST)==0) 
						{	
					 		;				
						}
					SystemWaitFor(500);  
					if (ChipDemodGetField(TPS_CONST)==0) 
						{	
							SystemWaitFor(400); 			
						}
					ChipDemodSetField(MODE,ChipDemodGetField(SYR_MODE));
					ChipDemodSetField(GUARD,ChipDemodGetField(SYR_GUARD));				
					ChipDemodSetField(FORCE,1);
					ChipDemodSetField(OP2_VAL,0);	
					ChipDemodSetField(OP2_VAL,1);
					ChipDemodSetField(OP2_VAL,0);
					if (ChipDemodGetField(TPS_CONST)==0) 
					{	
					 ;				
					}
				ChipDemodSetField(CORE_ACTIVE,0);
				ChipDemodSetField(CORE_ACTIVE,1);
				
					if (ChipDemodGetField(TPS_CONST)==0) 
					{	
					 ;				
					}
							
				SystemWaitFor(400);
				}
			
			if (ChipDemodGetField(LK) )
				{
				pr=ChipDemodGetField(TPS_HPCODE);
				mod=ChipDemodGetField(TPS_CONST);				
				(pParams->Results).Hprate=pr;
				(pParams->Results).Modulation=mod;
				
				}
			ChipDemodSetField(FORCE,0);   	
			
	return;

}



/**************************************************************************************/
/***** SET TRLNOMRATE REGISTERS *******************************/

void SET_TRLNOMRATE_REGS(short unsigned int value)
{
div_t divi;   

divi=div(value,512); 
ChipDemodSetField(TRL_NOMRATE_HI,divi.quot);
ChipDemodSetField(TRL_NOMRATE_LO,(divi.rem/2));
ChipDemodSetField(TRL_NOMRATE_LSB,(divi.rem%2));

}
/*************************************************************/
short unsigned int GET_TRLNOMRATE_VALUE(void)
{
short unsigned int value;   
value=  ChipDemodGetField(TRL_NOMRATE_HI)*512+ChipDemodGetField(TRL_NOMRATE_LO)*2+ChipDemodGetField(TRL_NOMRATE_LSB);	

return value;
}
/**************************************************************/
signed int GET_TRL_OFFSET(void)
{
	unsigned int  u_var;
    signed int    s_var;
	signed int i_int;
 
	ChipDemodSetField(FREEZE,1);								  
	/*ChipGetRegisters(hChip,R_TRL_NOMRATE1,4);*/
	ChipDemodSetField(FREEZE,0);
	/*s_var= 256* ChipGetFieldImage(hChip,TRL_TOFFSET_HI)+ChipGetFieldImage(hChip,TRL_TOFFSET_LO);*/
	s_var= 256* ChipDemodGetField(TRL_TOFFSET_HI)+ChipDemodGetField(TRL_TOFFSET_LO);
	if (s_var > 32768 ) s_var = s_var - 65536;
	
	u_var=  (512*ChipDemodGetField(TRL_NOMRATE_HI)+ChipDemodGetField(TRL_NOMRATE_LO)*2 + ChipDemodGetField(TRL_NOMRATE_LSB));
	i_int=((signed)(1000000/u_var)*s_var)/2048;


	return i_int;
}

void FE_360_Core_Switch(void)
{
ChipDemodSetField(CORE_ACTIVE,0);
ChipDemodSetField(CORE_ACTIVE,1);
SystemWaitFor(350);
return;
}

FE_360_SignalStatus_t FE_360_TRLNOMRATE_Tuning(void)
{
short unsigned int TRL_center,TRL_value ;
signed int OFFSET1,OFFSET2;
int i;

int Count=0;
        if(ChipDemodGetField(TPS_LOCK)==1)
        {
	   return LOCK_OK ;
	}
	else /** first else**/
	{
	TRL_center= GET_TRLNOMRATE_VALUE();
	TRL_value=TRL_center;
	for(i=0;i<5;i++)
		{
			TRL_value+=5;
			SET_TRLNOMRATE_REGS(TRL_value);
			FE_360_Core_Switch();
			if(ChipDemodGetField(TPS_LOCK)==1)
			break;
		}
	if (ChipDemodGetField(TPS_LOCK)==1)
	return LOCK_OK;
	else /**second else**/
	{
	   TRL_value=TRL_center;
	   
	for(i=0;i<5;i++)
	{
		TRL_value-=5;
		SET_TRLNOMRATE_REGS(TRL_value);
		FE_360_Core_Switch();
		if(ChipDemodGetField(TPS_LOCK)==1)
		break;
	}
	if (ChipDemodGetField(TPS_LOCK)==1)
	{
	   OFFSET1=GET_TRL_OFFSET();
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
		   SET_TRLNOMRATE_REGS(GET_TRLNOMRATE_VALUE()+1);
		   FE_360_Core_Switch(); 
		   OFFSET1= GET_TRL_OFFSET();
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
	            SET_TRLNOMRATE_REGS(GET_TRLNOMRATE_VALUE()-1);
                    OFFSET2= GET_TRL_OFFSET();
	            Count+=1;
	          }while (((OFFSET1>5) || (OFFSET1<-5))&&(Count<10));
	        /*  printf(".........TRLNOMRATE VALUE HI ......%x\n",GET_TRLNOMRATE_VALUE(hChip)/512);
	         printf(".........TRLNOMRATE VALUE LOW ......%x\n",(GET_TRLNOMRATE_VALUE(hChip)%512)/2);
	          printf(".........TRLNOMRATE VALUE LOW ......%x\n",(GET_TRLNOMRATE_VALUE(hChip)%512)%2);*/
	          return LOCK_OK;
								    
	      }
	   }/* end of else of if (OFFSET1==0)*/
	}/* end of if (ChipDemodGetField(TPS_LOCK)==1)*/
	
	   return NOTPS;
	
        }/* end of second else */
        }/* end of first else*/
}



				















