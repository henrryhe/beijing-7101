/*	standard ansi includes	*/
/*#include "math.h"*/
#ifndef ST_OSLINUX
   #include <stdlib.h>
#endif
 /* Standard includes */
 #include "stlite.h"
 /*C libs */
 
#include "ioreg.h"



#include "stddefs.h"
#include "stcommon.h"


#include "360_map.h"

#include "360_echo.h"
#include "360_drv.h"


#ifndef HOST_PC

#define UTIL_Delay(micro_sec) STOS_TaskDelayUs(micro_sec)

#define  SystemWaitFor(x) UTIL_Delay((x*1000)) 
#endif

#define DELTA 2

S8 sign (int x)
{
 if (x>0) return 1;
 else if (x<0) return -1;
 else return 0;
}

void Echo_Init (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_OFDMEchoParams_t *pParams)
{

int i,data;
/****** here different epq reference value has been set in relation to FFT mode 
        modulation technique*********/
data=STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_CONST);
if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_MODE) ) 
{
switch(data)
{
   case 0 : /*QPSK*/
      pParams->best_EPQ_val=35;
      pParams->bad_EPQ_val=61;
      pParams->EPQ_ref=35;
      break;

   case 1 : /*16 QAM */
      pParams->best_EPQ_val=28;
      pParams->bad_EPQ_val=54;
      pParams->EPQ_ref=28;
      break;

   case 2 : /* 64 QAM */
      pParams->best_EPQ_val=24;
      pParams->bad_EPQ_val=50;
      pParams->EPQ_ref=24;
      break;

default :
      pParams->best_EPQ_val=24;
      pParams->bad_EPQ_val=50;
      pParams->EPQ_ref=24;
      break;
}/* end of switch */
}
else
{
	
switch(data)
{
   case 0 : /*QPSK*/
      pParams->best_EPQ_val=9;
      pParams->bad_EPQ_val=15;
      pParams->EPQ_ref=9;
      break;

   case 1 : /*16 QAM */
      pParams->best_EPQ_val=7;
      pParams->bad_EPQ_val=13;
      pParams->EPQ_ref=7;
      break;

   case 2 : /* 64 QAM */
      pParams->best_EPQ_val=6;
      pParams->bad_EPQ_val=12;
      pParams->EPQ_ref=6;
      break;

      default :
      pParams->best_EPQ_val=6;
      pParams->bad_EPQ_val=12;
      pParams->EPQ_ref=6;
      break;
}/* end of switch */

}

for (i=0;i<8;i++)
{
*(pParams->past_EPQ_val+i)=pParams->EPQ_ref;             
}

pParams->L1s2va3vp4 = 1;
pParams->I2CSpeed=SpeedInit(DeviceMap,IOHandle);

/* init for Error counter number 3 */
/* error rate 2^16 */
data = (0<<7)+(1<<4)+(1<<2)+(2<<0);
STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_ERRCTRL3,data);
STTUNER_IOREG_SetField(DeviceMap,IOHandle,RESET_CNTR3,0);
return;
}

int Get_EPQ(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U8 Speed)
{
int epq;
epq=STTUNER_IOREG_GetField(DeviceMap,IOHandle,EPQ);
return epq ;
}


/* ********************************************************************* */

/* ********************************************************************* */
void Echo_long_scan(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_OFDMEchoParams_t *pParams)
/* ********************************************************************* */

{
U8 loc_L1;
U8 LEP_step,LEP_hold,LEP_ave;
U16 epq;
U8 tempo;

U16 SNR_max,snr;
S8  SNR_max_pos=0;
S8 current_LEP, LEP_idx;
S8 LEP_sens,LEP_start,LEP_stop;

unsigned short int i;
unsigned short int  EPQ_val;
unsigned short int tmp_best_EPQ_val,best_EPQ_val,bad_EPQ_val;
short int best_EPQ_val_idx=0;


loc_L1 = pParams->L1s2va3vp4;
if ( (loc_L1!=1)  && (loc_L1!=2))
{
	 return;
}

#ifdef LOG_EPQ
DebugPrintf("\nlong echo ");
#endif

	
 
if (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)) return;
	






	
SystemWaitFor(10);
if (loc_L1==1) LEP_step=1;
else LEP_step=3;


/* current LEP val can be adjusted */

current_LEP=STTUNER_IOREG_GetField(DeviceMap,IOHandle,LONG_ECHO);
if (current_LEP >= 0)
	{
	LEP_sens=-1;
	LEP_start=7;
	LEP_stop=-8;
	}
else
	{
	LEP_sens=1;
	LEP_start=-8;
	LEP_stop=7;
	}
LEP_ave=4;
LEP_hold=16;
LEP_hold=8; /* modif */ 

/* re_find  start position */
LEP_idx=current_LEP;
while (LEP_idx!=LEP_start)
{
	LEP_idx-=LEP_sens*LEP_step;

	if ( (LEP_idx*sign(LEP_start))> abs(LEP_start)) LEP_idx = LEP_start;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_idx);
	SystemWaitFor(LEP_hold); /* delay to be defined   */
}


/* EPQ measurements for different LEP_val*/
LEP_idx=LEP_start-(LEP_sens * LEP_step);
best_EPQ_val=255;
SNR_max=0;
bad_EPQ_val = 0;  
tempo= ( (LEP_hold-LEP_ave) * DELTA/2);
do
{

	LEP_idx+=(LEP_sens * LEP_step);

	if ( (LEP_idx*sign(LEP_stop))> abs(LEP_stop)) LEP_idx = LEP_stop;

	STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_idx);
	SystemWaitFor( tempo );
	EPQ_val=0;
	epq=0;
	snr=0;
	for(i=0;i<LEP_ave;i++)		  /* modifs */
		{
		epq+=Get_EPQ(DeviceMap,IOHandle,pParams->I2CSpeed);
		snr+=STTUNER_IOREG_GetField(DeviceMap,IOHandle,CHC_SNR);

		}
		EPQ_val= (epq+(LEP_ave>>1))/LEP_ave;
	*((pParams->EPQ_val+(LEP_idx+8)) ) = EPQ_val;/*( (EPQ_val+(LEP_ave>>1))/LEP_ave );*/
	
	if (snr>=SNR_max)
		{
			SNR_max		=	snr;
			SNR_max_pos	=	LEP_idx;
		}
	
/* search best LEP and its index */
	if (EPQ_val<=best_EPQ_val) 
			{
			pParams->best_EPQ_val=EPQ_val;
			pParams->best_EPQ_val_idx=LEP_idx;       
			best_EPQ_val=EPQ_val;
			best_EPQ_val_idx=LEP_idx;
			}
	if (EPQ_val > bad_EPQ_val)  bad_EPQ_val=EPQ_val;
} while (LEP_idx!=LEP_stop);


if ((best_EPQ_val<30) && ((bad_EPQ_val-best_EPQ_val) <=4))
	 {
	 	best_EPQ_val		= 	*((pParams->EPQ_val+(SNR_max_pos+8)) );
		best_EPQ_val_idx	=	SNR_max_pos;	 	
	 }

pParams->best_EPQ_val=best_EPQ_val;
pParams->best_EPQ_val_idx=best_EPQ_val_idx;       
		
if (bad_EPQ_val <= 3) bad_EPQ_val=3;/* to be detailled in future*/

/* re-find best longechoprotection position */
LEP_sens = -LEP_sens;
/* LEP is in LEP_stop following the previous loop */

for (LEP_idx=LEP_stop;LEP_idx!=best_EPQ_val_idx;LEP_idx+=(LEP_step*LEP_sens) ) 
	{
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_idx);
	SystemWaitFor(DELTA);	/*wait 500 symbols*/
	}
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_idx);
#ifdef LOG_ECHO	
fa=fopen("debug_echo.txt","a");
fprintf(fa,"ls  LEP %d   ",LEP_idx);	
fclose(fa);
#endif

#ifdef LOG_EPQ
DebugPrintf(" ls  LEP %d   ",LEP_idx);	
#endif

if (best_EPQ_val> (pParams->EPQ_ref+1))
{
pParams->L1s2va3vp4=1;
#ifdef LOG_ECHO
fa=fopen("debug_echo.txt","a");
fprintf(fa,"ls best_EPQ %d final L1... %d \n",best_EPQ_val,pParams->L1s2va3vp4);
fclose(fa);
#endif

#ifdef LOG_EPQ
/*DebugPrintf ("\n ref: best_EPQ %d   best_pos %d \n",best_EPQ_val,best_EPQ_val_idx);*/
DebugPrintf("ls best_EPQ %d best_pos final L1... %d \n",best_EPQ_val,best_EPQ_val_idx,pParams->L1s2va3vp4);
#endif
	    
return;
}

/* verify best_LEP_val */

tmp_best_EPQ_val=0;
tempo=(LEP_hold)*DELTA;
SystemWaitFor( tempo );
epq=0;
for (i=0;i<LEP_ave;i++)
{
		
	epq+=Get_EPQ(DeviceMap,IOHandle,pParams->I2CSpeed);
/*	if(epq>tmp_best_EPQ_val) tmp_best_EPQ_val = epq;*/
	SystemWaitFor(DELTA);	/* wait next symbol*/
	
}
tmp_best_EPQ_val=(epq+(LEP_ave>>1))/LEP_ave;

/* dcdc tmp_best_EPQ_val=(tmp_best_EPQ_val+(LEP_ave>>1))/LEP_ave; */

if (best_EPQ_val>=(tmp_best_EPQ_val-5) )	 /* value to be checked */
	{
	best_EPQ_val=tmp_best_EPQ_val;
	pParams->L1s2va3vp4=4;
	}
else
	{
	pParams->L1s2va3vp4=1;
	}

pParams->best_EPQ_val=best_EPQ_val;
pParams->best_EPQ_val_idx=best_EPQ_val_idx;
pParams->bad_EPQ_val=bad_EPQ_val;

#ifdef LOG_EPQ
DebugPrintf (" norm :best_EPQ %d   best_pos %d tmp_epq %d \n",best_EPQ_val,best_EPQ_val_idx,tmp_best_EPQ_val);
#endif

#ifdef LOG_ECHO
fa=fopen("debug_echo.txt","a");
fprintf(fa,"final L1... %d \n",pParams->L1s2va3vp4);
fclose(fa);
#endif










								
								
return;
}	




/* ********************************************************************* */
/* ********************************************************************* */
	void passive_monitoring(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_OFDMEchoParams_t *pParams)
/* ********************************************************************* */
/* ********************************************************************* */

{
U8 loc_L1,LEP_step,LEP_ave,LEP_hold;
U16 epq;
S8 LEP_sens,LEP_start,LEP_stop,LEP_val;
unsigned short int tmp_best_EPQ_val,i;
unsigned int error;
U8 tempo;

loc_L1 = pParams->L1s2va3vp4;
if (loc_L1==4) LEP_step = 1;

if (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK) )  return ;

/* programmation NON dynamique ou dynamique si Doppler */

if (loc_L1 !=4)
{
	 return;
}
SystemWaitFor(10);
LEP_val=STTUNER_IOREG_GetField(DeviceMap,IOHandle,LONG_ECHO);
if (LEP_val >=0)
	{
	LEP_sens = -1;
	LEP_start = 7;
	LEP_stop = -8;
	}
else
	{
	LEP_sens = +1;
	LEP_start = -8;
	LEP_stop = +7;
	}

/* EPQ measurements for current LEP */
LEP_hold=10;
LEP_ave=4;
SystemWaitFor(DELTA);
tmp_best_EPQ_val=0;                          
for (i=0;i<8;i++)
{
tmp_best_EPQ_val+=*(pParams->past_EPQ_val+i);
}
pParams->best_EPQ_val = (tmp_best_EPQ_val+4)/8;

/*scroll past epq values */
for (i=7;i>0;i--)
{
*(pParams->past_EPQ_val+i)=*(pParams->past_EPQ_val+i-1);
}

tmp_best_EPQ_val=0;
tempo=LEP_hold - LEP_ave;
SystemWaitFor(tempo);
epq=0;

error=STTUNER_IOREG_GetField(DeviceMap,IOHandle,LINE_OK);           
error=LEP_ave;
for (i=0;i<LEP_ave;i++)
{
epq+=Get_EPQ(DeviceMap,IOHandle,pParams->I2CSpeed);
/* if (epq>tmp_best_EPQ_val) tmp_best_EPQ_val = epq;*/
/*SystemWaitFor(DELTA*2);*/
error-=STTUNER_IOREG_GetField(DeviceMap,IOHandle,LINE_OK);
SystemWaitFor(DELTA);
}

/* dcdc cut 2.0 */
tmp_best_EPQ_val=(epq+ (LEP_ave>>1))/LEP_ave;


*(pParams->past_EPQ_val)=tmp_best_EPQ_val;

if (tmp_best_EPQ_val <=(pParams->EPQ_ref)) 
	{
	/*	if (tmp_best_EPQ_val <= pParams->best_EPQ_val) pParams->best_EPQ_val = tmp_best_EPQ_val;*/
		pParams->L1s2va3vp4 = 4;
		SystemWaitFor(10);
		
	}
/* addendum */
else if (tmp_best_EPQ_val>(pParams->EPQ_ref+4)) 
	{
		pParams->L1s2va3vp4 = 3;
		SystemWaitFor(10);
	}
/* end of */

else if (tmp_best_EPQ_val <=(pParams->best_EPQ_val + 5)) 
	{
	/*	if (tmp_best_EPQ_val <= pParams->best_EPQ_val) pParams->best_EPQ_val = tmp_best_EPQ_val;*/
		pParams->L1s2va3vp4 = 4;
		SystemWaitFor(10);
	}
else if (tmp_best_EPQ_val > (pParams->best_EPQ_val + 4)) 
	{
		pParams->L1s2va3vp4 = 3;
		SystemWaitFor(10);
		#ifdef LOG_EPQ
		DebugPrintf("\n pm tmp_best_EPQ_val %d  pParams->best_EPQ_val %d LEP_idx %d\n ", tmp_best_EPQ_val, pParams->best_EPQ_val,LEP_val);
		#endif
	}

if (error==LEP_ave)
{
	    pParams->L1s2va3vp4 = 2;
	    SystemWaitFor(10);
}
/*
#ifdef LOG_ECHO
fa= fopen("debug_echo.txt","a");
fprintf(fa,"pm tmp_EPQ %d  best_EPQ %d L1...%d LEP %d\n",tmp_best_EPQ_val,pParams->best_EPQ_val,pParams->L1s2va3vp4,LEP_val);
fclose(fa);
#endif
#ifdef LOG_EPQ
DebugPrintf("pm tmp_EPQ %d  best_EPQ %d L1...%d LEP %d\n",tmp_best_EPQ_val,pParams->best_EPQ_val,pParams->L1s2va3vp4,LEP_val);
#endif
*/
return;
}

/* ********************************************************************* */   
/* ********************************************************************* */
void active_monitoring(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_OFDMEchoParams_t *pParams)
/* ********************************************************************* */
/* ********************************************************************* */   

{

U8 loc_L1,LEP_step,LEP_ave,LEP_hold;
S8 LEP_sens,LEP_start,LEP_stop,LEP_val,LEP_idx;
unsigned short int tmp_best_EPQ_val,tmp_min_EPQ_val,i,try;
U16 epq;
U8 tempo;
unsigned short int last_epq;



loc_L1 = pParams->L1s2va3vp4;
if (loc_L1 !=3)
{
	 return;
}

	
if (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK)) return;

#ifdef LOG_EPQ
DebugPrintf("\n\nactive monitoring \n");
#endif

SystemWaitFor(10);
/* programmation core dynamique */

STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_CHC_CTL1,0xB1);
STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_CRL_CTL,0x4F);
STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_CAS_CTL,0x40);
STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_FREESTFE_1,0x03);
STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_AGC_CTL,0x18);


/* end prog core dynamique */

LEP_step =4;

LEP_val=STTUNER_IOREG_GetField(DeviceMap,IOHandle,LONG_ECHO);
/*LEP_val = 0; fine adjustment*/

if (LEP_val >=0)
	{
	LEP_sens = -1;
	LEP_start = 7;
	LEP_stop = -8;
	}
else
	{
	LEP_sens = +1;
	LEP_start = -8;
	LEP_stop = +7;
	}
	

/* EPQ measurements for different  LEP_val */
tmp_min_EPQ_val = 255;
LEP_idx = LEP_val; 
LEP_hold=8;
LEP_ave=4;
/* SystemWaitFor(500); */
try=0;
/*while (try <4)*/

#ifdef LOG_EPQ
if (!STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK))
{
wd=80;
}
#endif


#ifdef LOG_EPQ
DebugPrintf(" initial eqp %d \n",LEP_idx);
#endif	

last_epq=0;
tempo= (LEP_hold - LEP_ave) * DELTA;
while ( ( (try<2) || (last_epq==1) )  && (STTUNER_IOREG_GetField(DeviceMap,IOHandle,TPS_LOCK) ) )
{

STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_idx);

SystemWaitFor( tempo );
tmp_best_EPQ_val=0;
epq=0;
	for (i=0;i<LEP_ave;i++)
	{
	epq+=Get_EPQ(DeviceMap,IOHandle,pParams->I2CSpeed);
	SystemWaitFor(DELTA); 
	}

tmp_best_EPQ_val=(epq+(LEP_ave>>1))/LEP_ave;
		
			#ifdef LOG_EPQ
			DebugPrintf(" tmp_best_eqp %d",tmp_best_EPQ_val);
			DebugPrintf(" LEP_idx %d    break \n",LEP_idx);			
			#endif	
			
/* dcdc cut 2.0 tmp_best_EPQ_val=(tmp_best_EPQ_val+(LEP_ave>>1))/LEP_ave;*/
/*DebugPrintf("\n LEP %d  EPQ_val = %d\n",LEP_idx,tmp_best_EPQ_val);*/

	/* if ((tmp_best_EPQ_val- pParams->best_EPQ_val)<=4)*/ /*dcdc*/ 
	
	if ( ((tmp_best_EPQ_val- pParams->best_EPQ_val)<=5) && (tmp_best_EPQ_val <=(pParams->EPQ_ref+1)) )/*dcdc*/ 
		{
		pParams->best_EPQ_val = tmp_best_EPQ_val;
		pParams->best_EPQ_val_idx=LEP_idx;
		pParams->L1s2va3vp4 = 4;
		try=0;
		
		break;
		}
	
	else
		{
			
		if (tmp_best_EPQ_val <=(tmp_min_EPQ_val)) 
			{
			tmp_min_EPQ_val = tmp_best_EPQ_val;
			}

			LEP_idx= LEP_idx + LEP_sens*LEP_step;
			
			if (LEP_idx >= 7)
				{
				LEP_idx = 7;
				LEP_sens = -1;
				LEP_step=4;
				try++;
			



				}
			else if (LEP_idx <=-8)
				{
				LEP_idx = -8;
				LEP_sens = 1;
				LEP_step=4;
				}

		if ( (try==2) && (LEP_idx==7)) last_epq=1;
		else last_epq = 0;
		
		if (tmp_best_EPQ_val > (pParams->best_EPQ_val + 2)) 
			{
			pParams->L1s2va3vp4 = 3;
			}

		}
} /* end of while */

	#ifdef LOG_EPQ
	DebugPrintf("try %d  ",try);
	DebugPrintf ("%d  %d \n",pParams->best_EPQ_val,pParams->best_EPQ_val_idx);
	#endif

if (try>1) 	
{
pParams->L1s2va3vp4 = 1;
}
else				
{
pParams->L1s2va3vp4 = 4;
}
#ifdef LOG_ECHO
fa= fopen("debug_echo.txt","a");
fprintf(fa,"am %d  tmp_EPQ %d trial %d LEP %d L1...%d \n",loc_L1,tmp_best_EPQ_val,try,LEP_idx,pParams->L1s2va3vp4);
fclose(fa);
#endif


return;
}



/* ********************************************************************* */
void Ack_long_scan(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
/* ********************************************************************* */

{

U8 LEP_step,LEP_hold,LEP_ave;
U16 epq;
U8 tempo;

S8 current_LEP, LEP_idx;
S8 LEP_sens,LEP_start,LEP_stop;

unsigned short int i;
unsigned short int EPQ_val;
unsigned short int best_EPQ_val,bad_EPQ_val;
short int best_EPQ_val_idx=0;


LEP_step=3;

/* end prog core dynamique */
current_LEP=STTUNER_IOREG_GetField(DeviceMap,IOHandle,LONG_ECHO);
if (current_LEP >= 0)
	{
	LEP_sens=-1;
	LEP_start=7;
	LEP_stop=-8;
	}
else
	{
	LEP_sens=1;
	LEP_start=-8;
	LEP_stop=7;
	}
LEP_ave=4;
LEP_hold=8;


/* re_find  start position */
LEP_idx=current_LEP;
while (LEP_idx!=LEP_start)
{
LEP_idx-=LEP_sens*LEP_step;
if ( (LEP_idx*sign(LEP_start))> abs(LEP_start)) LEP_idx = LEP_start;
STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_idx);
SystemWaitFor(DELTA); /* delay to be defined*/
}


/* EPQ measurements for different LEP_val*/
LEP_idx=LEP_start-(LEP_sens * LEP_step);
best_EPQ_val=255;
tempo=(LEP_hold-LEP_ave) * DELTA;
do
{

LEP_idx+=(LEP_sens * LEP_step);

if ( (LEP_idx*sign(LEP_stop))> abs(LEP_stop)) LEP_idx = LEP_stop;

STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_idx);
SystemWaitFor( tempo );
	EPQ_val=0;

	bad_EPQ_val = 0;
	epq=0;
	for(i=0;i<LEP_ave;i++)
		{
		epq+=Get_EPQ(DeviceMap,IOHandle,0);
/*		if (epq>EPQ_val) EPQ_val=epq;  */
		SystemWaitFor(DELTA);/*wait 1(DELTA for debug)  symbol(s)*/
		}
	
	EPQ_val=(epq+(LEP_ave>>1))/LEP_ave;

/* search best LEP and its index */
	if (EPQ_val<=best_EPQ_val) 
			{
			best_EPQ_val=EPQ_val;
			best_EPQ_val_idx=LEP_idx;
			}
} while (LEP_idx!=LEP_stop);


/* re-find best longechoprotection position */
LEP_sens = -LEP_sens;
/* LEP is in LEP_stop following the previous loop */

for (LEP_idx=LEP_stop;LEP_idx!=best_EPQ_val_idx;LEP_idx+=(LEP_step*LEP_sens) ) 
	{
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_idx);
	SystemWaitFor(DELTA);	/*wait 500 symbols*/
	}
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_idx);

return;
}	


/* ********************************************************************* */
void Dlong_scan(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
/* perform a along scan based on BER values								 */
/* ********************************************************************* */

{

S8  lep;
S8  i,ii;
U8  LEP_hold;
S8  LEP_stop,LEP_sense,LEP_start;
U8  mode,mode_BER;
U32 error;
U16 SNR_min;
S8  SNR_min_pos=0;
S8  best_EPQ_pos;
U8  tempo;



mode_BER=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle, R_ERRCTRL3);
STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R_ERRCTRL3,0x93);

lep= STTUNER_IOREG_GetField(DeviceMap,IOHandle, LONG_ECHO);
mode=STTUNER_IOREG_GetField(DeviceMap,IOHandle,SYR_MODE);
SNR_min=0;
LEP_hold=8;
if (lep >=0)
	{
		LEP_stop=7;
		LEP_sense=1;
		LEP_start=-8;
	}
else
	{
		LEP_stop=-8;
		LEP_sense=-1;
		LEP_start=7;
	}
	
/* find start position */	
tempo=	LEP_hold>>(2 - 2*mode);
for (i=lep;i!=LEP_stop;i+=LEP_sense)
	{
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,i);
		SystemWaitFor(tempo);
	}
	
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_stop);
		SystemWaitFor(tempo);

/* compute values for LEP */

for (i=LEP_stop;i!=LEP_start;i-=LEP_sense*3)
	{
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,i);
	error=0;
	for (ii=0;ii<8;ii++) error += STTUNER_IOREG_GetField(DeviceMap,IOHandle,CHC_SNR);
	
	if (error	> SNR_min)
		{
			SNR_min=error;
			SNR_min_pos=i;
		}
	}
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,LEP_start);

	error=0;
	for (ii=0;ii<8;ii++) error += STTUNER_IOREG_GetField(DeviceMap,IOHandle,CHC_SNR);
	
	if (error	> SNR_min)
		{
			SNR_min=error;
			SNR_min_pos=i;
		}
		
	best_EPQ_pos=SNR_min_pos;
	tempo=1<<(2*mode);
	for (i=LEP_start;i!=best_EPQ_pos;i+=LEP_sense)
		{
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,i);
		SystemWaitFor(tempo);
		}
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,LONG_ECHO,best_EPQ_pos);		
	
}


/* ********************************************************************* */   
/* ********************************************************************* */
void test_ber(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_OFDMEchoParams_t *pParams)
/* ********************************************************************* */
/* ********************************************************************* */
{
short int wd,perr,i,le;

/* latest BER evaluation */
/*if ( (pParams->best_EPQ_val * 125) < (pParams->bad_EPQ_val * 100) ) return ;*/
wd=40;
perr=0;

STTUNER_IOREG_GetField(DeviceMap,IOHandle,LINE_OK);
for (i=0;i<=11;i++)
	{
	SystemWaitFor(4);
	if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,LINE_OK)==0) 
		{
			perr++;
		}
	}
if (perr>=10)	
	{
			/* pParams->L1s2va3vp4 = 1;*/
			/* debug code */
			le=STTUNER_IOREG_GetField(DeviceMap,IOHandle,LONG_ECHO);
			le=le;
/*			Dlong_scan(hChip);*/
/*			pParams->L1s2va3vp4 = 4;			*/

			/* end of debug code */
			
	}	
return;
}





