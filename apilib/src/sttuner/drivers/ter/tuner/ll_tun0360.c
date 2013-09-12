#ifndef STTUNER_MINIDRIVER
#include "stcommon.h"   
#ifdef HOST_PC
 #include 	<utility.h>
 #include 	<ansi_c.h>   /*to eliminate as the debug code is removed from*/
#endif
					  /* TUnerget steps ... */

/*#include	"def.h"*/

 #ifndef ST_OSLINUX
  
 #include 	"string.h"
 #endif
  /* Standard includes */
   #include "stlite.h"
 #include        "sttbx.h"

#ifdef HOST_PC
 #include	"tun0360.h"
 #else
 #include 	"tuntdrv.h"
#endif
 
#include	"ll_tun0360.h"


#ifdef HOST_PC
#include 	"system.h"     
#include    "i2c.h"
#include    <math.h>
#include    "360_Usr.h"
#include 	"ApplMain.h"
#include 	"360_Map.h"  
#include 	"drivmain.h"   
#include 	"Userpar.h" 
#endif

#define 	WRITE 1
#define 	READ  0


#define WAIT_IN_MS(X)     STOS_TaskDelayUs( X * 1000)   
/*****************************************************
**FUNCTION	::	LL_TunerSetProperties
**ACTION	::	Set the properties of the tuner
**PARAMS IN	::	tnr	==> tuner properties
**PARAMS OUT::	NONE  
**RETURN	::	NONE
*****************************************************/
void LL_TunerSetProperties(TUNTDRV_InstanceData_t* Tuner,TUNTDRV_InstanceData_t tnr)
{
	int i;
	
	Tuner->Error = tnr.Error; 
	strcpy((char*)Tuner->DeviceName,(char*)tnr.DeviceName);
	Tuner->Address =	tnr.Address;
	
	Tuner->TunerType = tnr.TunerType;
	Tuner->PLLType =	tnr.PLLType; 
	Tuner->I_Q = tnr.I_Q;
	Tuner->FreqFactor = tnr.FreqFactor;
	Tuner->StepSize = tnr.StepSize;
	Tuner->IF = tnr.IF;
	Tuner->Repeator = tnr.Repeator;
	
	Tuner->SelectBW = tnr.SelectBW;
	i=0;
	while(i<(TUN_MAX_BW+1))
	{
		Tuner->BandWidth[i] = tnr.BandWidth[i];
		i++;
	}
	
	Tuner->WrSize = tnr.WrSize;
	
	for(i=0;i<tnr.WrSize;i++)
		Tuner->WrBuffer[i] = tnr.WrBuffer[i];

	Tuner->RdSize = tnr.RdSize;
}

/*****************************************************
**FUNCTION	::	LL_TunerGetProperties
**ACTION	::	Get the properties of the tuner
**PARAMS IN	::	NONE
**PARAMS OUT::	tnr	==> tuner properties   
**RETURN	::	NONE
*****************************************************/
void LL_TunerGetProperties(TUNTDRV_InstanceData_t *Tuner,TUNTDRV_InstanceData_t *tnr)
{
	int i;
	
	if(tnr != NULL)
	{
		tnr->TunerType = Tuner->TunerType;
		strcpy((char*)tnr->DeviceName,(char*)Tuner->DeviceName);
		tnr->Address =	Tuner->Address;
		tnr->PLLType =	Tuner->PLLType; 
		tnr->Error = Tuner->Error;

		tnr->I_Q = Tuner->I_Q;
		tnr->FreqFactor = Tuner->FreqFactor;
		tnr->StepSize = Tuner->StepSize;
		tnr->IF = Tuner->IF;
		tnr->Repeator = Tuner->Repeator;
	
		tnr->SelectBW = Tuner->SelectBW; 
		i=0;
		while(i<(TUN_MAX_BW+1))
		{
			tnr->BandWidth[i] = Tuner->BandWidth[i];
			i++;
		}
	
		tnr->WrSize = Tuner->WrSize;
	
		for(i=0;i<Tuner->WrSize;i++)
			tnr->WrBuffer[i] = Tuner->WrBuffer[i];

		tnr->RdSize = Tuner->RdSize;
	
		for(i=0;i<Tuner->RdSize;i++)
			tnr->RdBuffer[i] = Tuner->RdBuffer[i];
	}
}

/*****************************************************
**FUNCTION	::	LL_TunerResetError
**ACTION	::	reset the error state of the tuner 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE  
**RETURN	::	NONE
*****************************************************/
void LL_TunerResetError(TUNTDRV_InstanceData_t* Tuner)
{
	Tuner->Error = TNR_NO_ERR;
}

/*****************************************************
**FUNCTION	::	LL_TunerGetError
**ACTION	::	return the error state of the tuner 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE  
**RETURN	::	error
*****************************************************/
TUNER_ERROR LL_TunerGetError(TUNTDRV_InstanceData_t *Tuner)
{
	return Tuner->Error;
}

/*****************************************************
**FUNCTION	::	LL_TunerInit
**ACTION	::	Initialize the tuner according to its type
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE  
**RETURN	::	NONE
*****************************************************/
void LL_TunerInit(TUNTDRV_InstanceData_t* Tuner)
{
#ifdef HOST_PC
int step;
double If;
#endif
	if(!Tuner->Error)
	{
		Tuner->SelectBW = 0;
		Tuner->BandWidth[TUN_MAX_BW] = 0;	/* To avoid list without end	*/ 
/* dcdc		Tuner->Repeator=0x0;*/ /* no repeator by defaults */
		
		
		switch(Tuner->TunerType)
		{
							 
		
		#ifdef	 STTUNER_DRV_TER_TUN_TDLB7
		case  	STTUNER_TUNER_TDLB7:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"ALPS TDLB7");
				#endif
				Tuner->PLLType	=	TUNER_PLL_TDLB7;/* need to be verified */
			
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int) 166667L;
				Tuner->IF = (int) 36166L;
				Tuner->BandWidth[0] = (int) 7600L;
				Tuner->BandWidth[1] = (int) 0L;
		/* dcdc	Tuner->Repeator = 0; */
			
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x0B;
				Tuner->WrBuffer[1] = 0xF5;
				Tuner->WrBuffer[2] = 0x85;
				Tuner->WrBuffer[3] = 0x40;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
		break;		
		#endif
		
		#ifdef STTUNER_DRV_TER_TUN_TDEB2
		case  	STTUNER_TUNER_TDEB2:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"ALPS TDEB2");
				#endif
				Tuner->PLLType	=	TUNER_PLL_TDEB2;/* need to be verified */
			
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int) 166667L;
				Tuner->IF = (int) 36167L;
				Tuner->BandWidth[0] = (int) 7600L;
				Tuner->BandWidth[1] = (int) 0L;
		/* dcdc	Tuner->Repeator = 0; */
			
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x0B;
				Tuner->WrBuffer[1] = 0xF5;
				Tuner->WrBuffer[2] = 0x85;
				Tuner->WrBuffer[3] = 0x8;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
		break;		
		#endif
		
			
		#ifdef STTUNER_DRV_TER_TUN_DTT7572
		case  	STTUNER_TUNER_DTT7572:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"TMM DTT7572");
				#endif
				Tuner->PLLType	=	TUNER_PLL_DTT7572;/* need to be verified */
			
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int) 166667L;
				Tuner->IF = (int) 36000L;
				Tuner->BandWidth[0] = (int) 7600L;
				Tuner->BandWidth[1] = (int) 0L;
		/*dcdc		Tuner->Repeator = 0;*/
			
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x2;
				Tuner->WrBuffer[1] = 0x34;
				Tuner->WrBuffer[2] = 0x93;
				Tuner->WrBuffer[3] = 0x05;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
		break;		
		#endif
			
		#ifdef STTUNER_DRV_TER_TUN_EAL2780
		case  	STTUNER_TUNER_EAL2780:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"SIEL EAL2780");
				#endif
				Tuner->PLLType	=	TUNER_PLL_EAL2780;/* need to be verified */
			
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int) 166667L;
				Tuner->IF = (int) 36167L;
				Tuner->BandWidth[0] = (int) 7600L;
				Tuner->BandWidth[1] = (int) 0L;
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x2;
				Tuner->WrBuffer[1] = 0x34;
				Tuner->WrBuffer[2] = 0x93;
				Tuner->WrBuffer[3] = 0x05;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
		break;		
		#endif
			
			
		#if defined(STTUNER_DRV_TER_TUN_DTT7578) ||defined(STTUNER_DRV_TER_TUN_DTT7592)
		case  	STTUNER_TUNER_DTT7578 :
		case     STTUNER_TUNER_DTT7592 :
			
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"TMM DTT7576");
				#endif
				if(Tuner->TunerType == STTUNER_TUNER_DTT7578)
				{
				   Tuner->PLLType	=	TUNER_PLL_DTT7578;/* need to be verified */
				}
				else
				{
				   Tuner->PLLType	=	TUNER_PLL_DTT7592;/* need to be verified */
				}
				Tuner->SubAddress=0xC0;
				Tuner->WrSubSize=4;		
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int)166667L;
				Tuner->IF = (int)36000L;
				Tuner->BandWidth[0] = (int)7600L;
				Tuner->BandWidth[1] = (int)0L;
		/*dcdc		Tuner->Repeator = 0;*/
			
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x2;
				Tuner->WrBuffer[1] = 0x34;
				Tuner->WrBuffer[2] = 0x93;
				Tuner->WrBuffer[3] = 0x05;
				
				Tuner->WrSubBuffer[0] = 0x2;
				Tuner->WrSubBuffer[1] = 0x34;
				Tuner->WrSubBuffer[2] = 0x93;
				Tuner->WrSubBuffer[3] = 0x05;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
		break;		
		#endif
		#ifdef STTUNER_DRV_TER_TUN_TDA6650
		case  	STTUNER_TUNER_TDA6650:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"TDA 6650");
				#endif
				Tuner->PLLType	=	TUNER_PLL_TDA6650;/* need to be verified */
				Tuner->SubAddress=0xC2;
				Tuner->WrSubSize=5;		
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int)125000L;
				Tuner->IF = (int)36125L;
				Tuner->BandWidth[0] = (int)7600L;
				Tuner->BandWidth[1] = (int)0L;
		/*dcdc		Tuner->Repeator = 0;*/
				Tuner->WrSize = 5;
				Tuner->WrBuffer[0] = 0x19;
				Tuner->WrBuffer[1] = 0xF1;
				Tuner->WrBuffer[2] = 0xCC;
				Tuner->WrBuffer[3] = 0xC4;
				Tuner->WrBuffer[4] = 0x8B;
				
				Tuner->WrSubBuffer[0] = 0x19;
				Tuner->WrSubBuffer[1] = 0xF1;
				Tuner->WrSubBuffer[2] = 0xCC;
				Tuner->WrSubBuffer[3] = 0xC4;
				Tuner->WrSubBuffer[4] = 0x83;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
				#ifdef HOST_PC
				/*UsrWrInt("Step",Tuner->StepSize);*/
				/*UsrWrDouble("If",(double)(Tuner->IF/1000.0));*/
				#endif
		break;		
		#endif
		#ifdef STTUNER_DRV_TER_TUN_TDM1316
		case  	STTUNER_TUNER_TDM1316:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"TDM 1316");
				#endif
				Tuner->PLLType	=	TUNER_PLL_TDM1316;/* need to be verified */
				Tuner->SubAddress=0xC2;
				Tuner->WrSubSize=5;		
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize =(int)166667L;
				Tuner->IF = (int)36130L;
				Tuner->BandWidth[0] = (int)7600L;
				Tuner->BandWidth[1] = (int)0L;
		/*dcdc		Tuner->Repeator = 0;*/
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x19;
				Tuner->WrBuffer[1] = 0xF1;
				Tuner->WrBuffer[2] = 0xC8;
				Tuner->WrBuffer[3] = 0xAC;
				
				
				Tuner->WrSubBuffer[0] = 0x19;
				Tuner->WrSubBuffer[1] = 0xF1;
				Tuner->WrSubBuffer[2] = 0xC8;
				Tuner->WrSubBuffer[3] = 0xAC;
				
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
				#ifdef HOST_PC
				/*UsrWrInt("Step",Tuner->StepSize);*/
				/*UsrWrDouble("If",(double)(Tuner->IF/1000.0));*/
				#endif
		break;		
		#endif
			
		#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
		case  	STTUNER_TUNER_ENG47402G1:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"MACO ET-03DT");
				#endif
				Tuner->PLLType	=	TUNER_PLL_ENG47402G1;/* need to be verified */
				Tuner->SubAddress=0xCA;
				
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int)166667L;
				Tuner->IF = (int)36167L;
				Tuner->BandWidth[0] = (int)7600L;
				Tuner->BandWidth[1] = (int)0L;
		/*dcdc		Tuner->Repeator = 0;*/
			
				Tuner->WrSize = 4;
				Tuner->WrSubSize = 4;
				Tuner->WrBuffer[0] = 0x2;
				Tuner->WrBuffer[1] = 0x34;
				Tuner->WrBuffer[2] = 0x93;
				Tuner->WrBuffer[3] = 0x05;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
		break;		
		#endif
		#ifdef STTUNER_DRV_TER_TUN_ED5058
		case  	STTUNER_TUNER_ED5058:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"SHARP ED5058");
				#endif
				Tuner->PLLType	=	TUNER_PLL_ED5058; 
				Tuner->SubAddress=0xC2;  /* Avoir anvec davidee*/
			/*	Tuner->WrSubSize=4;		*/
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int)166667L;
				Tuner->IF = (int)36000L;
				Tuner->BandWidth[0] = (int)8000L;/* BS5 in CD2 is set to 1 */
				/* NEXT I WILL GIVE THE CHOICE TO SWITCH BETWEEN 7Mhz AND 8Mhz */
				
				Tuner->BandWidth[1] = (int)0L;
		/*dcdc		Tuner->Repeator = 0;*/
			
			
				/******************NE CHANGE RIEN***************/
				Tuner->WrSize = 5;
				
				
				Tuner->WrBuffer[0] = 0xB;
				Tuner->WrBuffer[1] = 0xF4;
				Tuner->WrBuffer[2] = 0xCA;
				Tuner->WrBuffer[3] = 0x94;
				Tuner->WrBuffer[4] = 0x80;
				
		/*		Tuner->WrSubBuffer[0] = 0x2;
				Tuner->WrSubBuffer[1] = 0x34;
				Tuner->WrSubBuffer[2] = 0x93;
				Tuner->WrSubBuffer[3] = 0x05;*/
			
				
				
				/**********************************************/
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
		break;		
		#endif
			/***********************************************************************************************/
			/******************MIVAR***************************************/
		#ifdef STTUNER_DRV_TER_TUN_MIVAR
		case  	STTUNER_TUNER_MIVAR:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"MIVAR");
				#endif
				Tuner->PLLType	= TUNER_PLL_MIVAR; 
				Tuner->SubAddress=0xC0;
				Tuner->WrSubSize=5;		
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int)125000L;
				Tuner->IF = (int)36125L;
				Tuner->BandWidth[0] = (int)7600L;
				Tuner->BandWidth[1] = (int)0L;
			/*	Tuner->Repeator = 0;*/
				Tuner->WrSize = 5;
				Tuner->WrBuffer[0] = 0x19;
				Tuner->WrBuffer[1] = 0xF1;
				Tuner->WrBuffer[2] = 0xCC;
				Tuner->WrBuffer[3] = 0xC4;
				Tuner->WrBuffer[4] = 0x8B;
				
				Tuner->WrSubBuffer[0] = 0x19;
				Tuner->WrSubBuffer[1] = 0xF1;
				Tuner->WrSubBuffer[2] = 0xCC;
				Tuner->WrSubBuffer[3] = 0xC4;
				Tuner->WrSubBuffer[4] = 0x83;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
				#ifdef HOST_PC
				/*UsrWrInt("Step",Tuner->StepSize);
				UsrWrDouble("If",(double)(Tuner->IF/1000.0));*/
				#endif
		break;		
		#endif
			/******************Tuner TDED4*******************/
		#ifdef STTUNER_DRV_TER_TUN_TDED4
		case  	STTUNER_TUNER_TDED4:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"ALPS TDED4");
				#endif
				Tuner->PLLType	= TUNER_PLL_TDED4; 
			
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int) 166667L;
				Tuner->IF = (int) 36167L;
				Tuner->BandWidth[0] = (int) 7600L;
				Tuner->BandWidth[1] = (int) 0L;
		/* dcdc	Tuner->Repeator = 0; */
			
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x0B;
				Tuner->WrBuffer[1] = 0xF5;
				Tuner->WrBuffer[2] = 0x85;
				Tuner->WrBuffer[3] = 0x8;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00;
		break;		 
		#endif
			/**************Tuner DTT7102*******************/
		#ifdef STTUNER_DRV_TER_TUN_DTT7102
		case  	STTUNER_TUNER_DTT7102:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"TMM DTT7102");
				#endif
				Tuner->PLLType	=	TUNER_PLL_DTT7102; 
				Tuner->SubAddress=0xC0;
				Tuner->WrSubSize=4;		
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int)166667L;
				Tuner->IF = (int)36000L;
				Tuner->BandWidth[0] = (int)7600L;
				Tuner->BandWidth[1] = (int)0L;
		  /*		Tuner->Repeator = 0; */
			
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x2;
				Tuner->WrBuffer[1] = 0x34;
				Tuner->WrBuffer[2] = 0x93;
				Tuner->WrBuffer[3] = 0x05;
				
				Tuner->WrSubBuffer[0] = 0x2;
				Tuner->WrSubBuffer[1] = 0x34;
				Tuner->WrSubBuffer[2] = 0x93;
				Tuner->WrSubBuffer[3] = 0x05;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
		break;		
			
		#endif
			/***************TECC2849PG***********************/
		#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
		case  	STTUNER_TUNER_TECC2849PG:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"SEM TECC2849PG ");
				#endif
				Tuner->PLLType	=	TUNER_PLL_TECC2849PG; 
				Tuner->SubAddress=0xC0;
				Tuner->WrSubSize=0;		
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int)166667L;
				Tuner->IF = (int)36125L;
				Tuner->BandWidth[0] = (int)7600L;
				Tuner->BandWidth[1] = (int)0L;
		/*		Tuner->Repeator = 0;*/
			
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x2;
				Tuner->WrBuffer[1] = 0x34;
				Tuner->WrBuffer[2] = 0x93;
				Tuner->WrBuffer[3] = 0x05;
				
				Tuner->WrSubBuffer[0] = 0x2;
				Tuner->WrSubBuffer[1] = 0x34;
				Tuner->WrSubBuffer[2] = 0x93;
				Tuner->WrSubBuffer[3] = 0x05;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
		break;		
			
		#endif
			
			
			/************TDCC2345*****************/
		#ifdef STTUNER_DRV_TER_TUN_TDCC2345
		case  	STTUNER_TUNER_TDCC2345:
				#ifdef HOST_PC
				strcpy((char*)Tuner->DeviceName,(char*)"SEM TDCC2345 ");
				#endif
				Tuner->PLLType	=	TUNER_PLL_TDCC2345; 
				Tuner->SubAddress=0xC0;
				Tuner->WrSubSize=0;		
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int)166667L;
				Tuner->IF = (int)36125L;
				Tuner->BandWidth[0] = (int)7600L;
				Tuner->BandWidth[1] = (int)0L;
		/*		Tuner->Repeator = 0;*/
			
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x2;
				Tuner->WrBuffer[1] = 0x34;
				Tuner->WrBuffer[2] = 0x93;
				Tuner->WrBuffer[3] = 0x05;
				
				Tuner->WrSubBuffer[0] = 0x2;
				Tuner->WrSubBuffer[1] = 0x34;
				Tuner->WrSubBuffer[2] = 0x93;
				Tuner->WrSubBuffer[3] = 0x05;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
		break;		
			
		#endif
			
			default:
				Tuner->Error = TNR_TYPE_ERR;
			break;
		}
#ifdef HOST_PC
UsrRdInt("Step",&step);
Tuner->StepSize = (int)step;
UsrRdDouble("If",&If);
Tuner->IF=(int)(1000*If);
#endif
/*dcdc		TunerReadWrite(WRITE);*/
/*dcdc		TunerReadWrite(READ);*/
	}
}


/*****************************************************
**FUNCTION	::	LL_tuner_tuntdrv_Select
**ACTION	::	Select the type of tuner used
**PARAMS IN	::	type	==> type of the tuner
**				address	==> I2C address of the tuner
**PARAMS OUT::	NONE  
**RETURN	::	NONE
*****************************************************/

void LL_TunerSelect(TUNTDRV_InstanceData_t* Tuner,STTUNER_TunerType_t type,unsigned char address)

{
	Tuner->Error=TNR_NO_ERR;
	Tuner->TunerType = type;
	Tuner->Address = address;
	
	 LL_TunerInit(Tuner);
	
}

/*****************************************************
**FUNCTION	::	LL_TunerSetRepeator
**ACTION	::	Select the type of tuner used
**PARAMS IN	::	type	==> type of the tuner
**				address	==> I2C address of the tuner
**PARAMS OUT::	NONE  
**RETURN	::	NONE
*****************************************************/
void LL_TunerSetRepeator(TUNTDRV_InstanceData_t* Tuner, unsigned short int  repeator)
{

	Tuner->Repeator = repeator;
}

/*****************************************************
**FUNCTION	::	LL_TunerGetError
**ACTION	::	return the error state of the tuner 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE  
**RETURN	::	error
*****************************************************/
int LL_TunerGetRepeator(TUNTDRV_InstanceData_t* Tuner)
{
	return Tuner->Repeator;
}


/*****************************************************
**FUNCTION	::	LL_TunerSelectBandwidth
**ACTION	::	Select the bandwidth of the tuner (if this one not exist,
**				closest value bigger than bandwidth will be selected)
**PARAMS IN	::	bandwidth	==> bandwidth of the tuner 
**PARAMS OUT::	NONE  
**RETURN	::	Selected bandwidth, 0 if error
*****************************************************/
int LL_TunerSelectBandwidth(TUNTDRV_InstanceData_t* Tuner, int bandwidth)
{
	
	
	Tuner->SelectBW = 0;
	
	if(!Tuner->Error)
	{
		while(	(Tuner->BandWidth[Tuner->SelectBW + 1] != 0) && 
				(bandwidth > Tuner->BandWidth[Tuner->SelectBW]))
		{
			Tuner->SelectBW++;
		}
	
		
		
		return Tuner->BandWidth[Tuner->SelectBW];  
	}
	else return 0;
}


/*****************************************************
**FUNCTION	::	LL_TunerSetNbSteps
**ACTION	::	Set the number of steps of the tuner
**PARAMS IN	::	nbsteps ==> number of steps of the tuner
**PARAMS OUT::	NONE  
**RETURN	::	NONE
*****************************************************/
void LL_TunerSetNbSteps(TUNTDRV_InstanceData_t* Tuner,int nbsteps)
{
	if(!Tuner->Error)
	{
		switch(Tuner->PLLType)
		{
			#ifdef STTUNER_DRV_TER_TUN_TDLB7
			case TUNER_PLL_TDLB7:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
  				Tuner->WrBuffer[2] = (unsigned char) ( 0x85 | ((nbsteps >> 16) & 0x60) );  /* PE=0,R=0xc */
			break ;	
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TDEB2
			case TUNER_PLL_TDEB2:/* new ALPS */
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
  				Tuner->WrBuffer[2] = (unsigned char) ( 0x85 );
			break ;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_DTT7572
			case TUNER_PLL_DTT7572:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
  				Tuner->WrBuffer[2] = (unsigned char) ( 0x93);
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_EAL2780
			case TUNER_PLL_EAL2780:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TDA6650
			case TUNER_PLL_TDA6650:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
				
				Tuner->WrSubBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrSubBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TDM1316
			case TUNER_PLL_TDM1316:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
				
				Tuner->WrSubBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrSubBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
			break;
			#endif
			#if defined( STTUNER_DRV_TER_TUN_DTT7578)|| defined( STTUNER_DRV_TER_TUN_DTT7592)
			case TUNER_PLL_DTT7578:
			case TUNER_PLL_DTT7592:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
				Tuner->WrSubBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrSubBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
  			break;
			#endif
                        #ifdef STTUNER_DRV_TER_TUN_ENG47402G1
			case TUNER_PLL_ENG47402G1:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
				nbsteps = ((nbsteps * Tuner->StepSize + 500000)/1000000);
				Tuner->WrSubBuffer[0] = (unsigned char) ( (nbsteps >> 4) & 0xff );    
				Tuner->WrSubBuffer[1] = (unsigned char) ( ((nbsteps <<4) & 0xF0)|0x4) ;
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_ED5058
			case TUNER_PLL_ED5058:
			
				/**************** A RAJOUTER**************************************/
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
  			
  			/*	Tuner->WrSubBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );*/
			/*	Tuner->WrSubBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );*/
  				/******************************************************************/
  			#endif	
  			/************MIVAR*******************************/
  			#ifdef STTUNER_DRV_TER_TUN_MIVAR
  			case TUNER_PLL_MIVAR:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
				
				Tuner->WrSubBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrSubBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TDED4
			/************Tuner TDED4**************************/
			case TUNER_PLL_TDED4:/* new ALPS */
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
  				Tuner->WrBuffer[2] = (unsigned char) ( 0x85 );
			break ;
			#endif
			/*********Tuner DTT7102*************************/
			#ifdef STTUNER_DRV_TER_TUN_DTT7102
			case TUNER_PLL_DTT7102:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
  				Tuner->WrSubBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrSubBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
  			
			break;
			#endif
			/*********Tuner TECC2849PG************************/
			#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
			case TUNER_PLL_TECC2849PG:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
			break;
			#endif
			/*******Tuner TDCC2345**************************/
			#ifdef STTUNER_DRV_TER_TUN_TDCC2345
			case TUNER_PLL_TDCC2345:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
			break;
			#endif
			default:
				Tuner->Error = TNR_PLL_ERR;
			break ;
		}
		
		#ifdef HOST_PC
		TunerReadWrite( Tuner,WRITE);  
		#else
		
		TunerReadWrite((TUNER_Handle_t)Tuner,WRITE); 
		#endif
		
	}
}


/*****************************************************
**FUNCTION	::	LL_TunerGetNbSteps
**ACTION	::	Get the number of steps of the tuner
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE  
**RETURN	::	Number of steps, 0 if an error occur
*****************************************************/
int LL_TunerGetNbSteps(TUNTDRV_InstanceData_t* Tuner)
{
	int nbsteps = 0;
	if(!Tuner->Error)
	{
	
	        switch (Tuner->PLLType)
		{
			#ifdef STTUNER_DRV_TER_TUN_TDLB7
			case TUNER_PLL_TDLB7:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
				nbsteps |= ((int)Tuner->WrBuffer[2] & 0x00000060)<<16L;
			break ;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TDEB2
			case TUNER_PLL_TDEB2:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break ;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_DTT7572
			case TUNER_PLL_DTT7572:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TDA6650
			case TUNER_PLL_TDA6650:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TDM1316
			case TUNER_PLL_TDM1316:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_EAL2780
			case TUNER_PLL_EAL2780:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break;
			#endif
			#if defined(STTUNER_DRV_TER_TUN_DTT7578) || defined (STTUNER_DRV_TER_TUN_DTT7592)
			
			case TUNER_PLL_DTT7578:
			case TUNER_PLL_DTT7592:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
				
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
			
			case TUNER_PLL_ENG47402G1:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_ED5058
			
			case TUNER_PLL_ED5058:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
				nbsteps=nbsteps;
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_MIVAR
			case TUNER_PLL_MIVAR:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TDED4
			case TUNER_PLL_TDED4:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_DTT7102
			case TUNER_PLL_DTT7102:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
			case TUNER_PLL_TECC2849PG:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TDCC2345
			case TUNER_PLL_TDCC2345:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
			break;
			#endif
			default:
				Tuner->Error = TNR_PLL_ERR;
			break;
		}
	}
	else
	{
	/*	dbg_counter++;*/
		/*if (!(dbg_counter % 8 )) printf("NO ACK \n");*/
	}
		
	return nbsteps;
}


/*****************************************************
**FUNCTION	::	LL_TunerGetFrequency
**ACTION	::	Get the frequency of the tuner
**PARAMS IN	::	NONE  
**PARAMS OUT::	NONE  
**RETURN	::	frequency of the tuner (KHz), 0 if an error occur 
*****************************************************/
int LL_TunerGetFrequency(TUNTDRV_InstanceData_t* Tuner)
{
	int frequency = 0;
	
	if(Tuner->StepSize>0)
	{
		
		  frequency = ((LL_TunerGetNbSteps(Tuner)*(Tuner->StepSize/2)) - 500*Tuner->IF)/500	;
	          frequency *= Tuner->FreqFactor;
		
	}
	else
		Tuner->Error = TNR_STEP_ERR;
	
	return frequency; 
}


/*****************************************************
**FUNCTION	::	LL_TunerSetFrequency
**ACTION	::	Set the frequency of the tuner
**PARAMS IN	::	frequency ==> the frequency of the tuner (KHz) 
**PARAMS OUT::	NONE  
**RETURN	::	real tuner frequency, 0 if an error occur 
*****************************************************/

/*ST_ErrorCode_t	tuner_tuntdrv_SetFrequency(TUNER_Handle_t  Handle,U32 frequency,(U32*)&returned_freq); */
 ST_ErrorCode_t LL_TunerSetFrequency(TUNTDRV_InstanceData_t* Tuner,U32 frequency,U32* returned_freq)  

{
	#if defined (STTUNER_DRV_TER_TUN_TDLB7)||defined (STTUNER_DRV_TER_TUN_TDEB2)||defined (STTUNER_DRV_TER_TUN_DTT7572)||defined (STTUNER_DRV_TER_TUN_EAL2780)||defined(STTUNER_DRV_TER_TUN_DTT7578) || defined(STTUNER_DRV_TER_TUN_DTT7592)||defined (STTUNER_DRV_TER_TUN_TDA6650)||defined (STTUNER_DRV_TER_TUN_TDM1316)||defined (STTUNER_DRV_TER_TUN_ENG47402G1)||defined (STTUNER_DRV_TER_TUN_ED5058)||defined (STTUNER_DRV_TER_TUN_MIVAR)||defined (STTUNER_DRV_TER_TUN_TDED4)||defined (STTUNER_DRV_TER_TUN_DTT7102)||defined (STTUNER_DRV_TER_TUN_TECC2849PG)||defined (STTUNER_DRV_TER_TUN_TDCC2345)
	int 	chargepump = 0,		
		channel_size,
		Atc=0;
					
		#endif
		#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
		int	loc_CB;
		#endif
	/*Code added for stb4K*/
	
	int nbsteps = 0;
	
	
	
	
	
	
	#ifdef STTUNER_ATCBIT_TUNER_ENABLE
	int loop = 0;
	int FL = 0;
	#endif
	/*Code added for MT2060*/
	Tuner->Error=TNR_NO_ERR;
	
	if(Tuner->StepSize > 0)
	{
		
		switch(Tuner->TunerType)
		{
		#ifdef STTUNER_DRV_TER_TUN_TDLB7
		case  	STTUNER_TUNER_TDLB7:
  				chargepump = (unsigned char) ( (frequency < (782000 + Tuner->IF) ) ?
                0x40 :   /* C="01",RE=0,RTS=0,Pn=0 */
                0x80 );  /* C="10",RE=0,RTS=0,Pn=0 */
				Tuner->WrBuffer[3] = chargepump;  
		break;		
		#endif
			
			
		#ifdef STTUNER_DRV_TER_TUN_TDEB2
		case  	STTUNER_TUNER_TDEB2:
				channel_size=Tuner->ChannelBW;
				#ifdef HOST_PC
				UsrRdInt("CHANNEL_WIDTH",&channel_size); 			
				#endif
  				if 		(frequency <= 230000) 	chargepump=(unsigned char)0x02;
				else if (frequency <= 824000) 	chargepump=(unsigned char)0x08;
				else 							chargepump=(unsigned char)0x88;
				if (channel_size==8) (chargepump=chargepump|4); 				
				Tuner->WrBuffer[3] = chargepump;  
		break;		
				
		#endif

			
		#ifdef STTUNER_DRV_TER_TUN_DTT7572
		case  	STTUNER_TUNER_DTT7572:
				if (frequency <= 89000) chargepump=(unsigned char)0x05;
				else if (frequency <= 114000) chargepump=(unsigned char)0x45;
				else if (frequency <= 134000) chargepump=(unsigned char)0x85;
				else if (frequency <= 146000) chargepump=(unsigned char)0xC5;
				
				else if (frequency <= 264000) chargepump=(unsigned char)0x06;
				else if (frequency <= 344000) chargepump=(unsigned char)0x46;
				else if (frequency <= 404000) chargepump=(unsigned char)0x86;
				else if (frequency <= 430000) chargepump=(unsigned char)0xC6;
				
				else if (frequency <= 714000) chargepump=(unsigned char)0x43;
				else if (frequency <= 814000) chargepump=(unsigned char)0x83;
				else if (frequency <= 860000) chargepump=(unsigned char)0xC3;
				else  chargepump=(unsigned char)0x83;
				Tuner->WrBuffer[3] = (unsigned char) chargepump;  
		break;		
		#endif

			
		#ifdef STTUNER_DRV_TER_TUN_EAL2780
		case  	STTUNER_TUNER_EAL2780:
				chargepump=(unsigned char)0x8C;
				Tuner->WrBuffer[2] = (unsigned char) chargepump;  				
				Tuner->WrBuffer[3] = (unsigned char) (0x40);
		#endif

			
		#if defined(STTUNER_DRV_TER_TUN_DTT7578) || defined(STTUNER_DRV_TER_TUN_DTT7592)
		case  	STTUNER_TUNER_DTT7578 :
		case    STTUNER_TUNER_DTT7592 :
				if 		(frequency <= 230000) chargepump=(unsigned char)0xB4;/* modifs of the spec */
				else if (frequency <= 699000) chargepump=(unsigned char)0xBC;
				else if (frequency <= 799000) chargepump=(unsigned char)0xF4;
				else if (frequency <= 860000) chargepump=(unsigned char)0xFC;
				else  chargepump=(unsigned char)0xFC;
				switch(Tuner->StepSize)
					{
					case 50000:
		
					chargepump=(chargepump&0xC9);	
					
					break;
					case 31250:
					chargepump=(chargepump&0xC9);
					chargepump=chargepump|((0x1)<<1);
					break;
					
					case 166667:
					chargepump=(chargepump&0xF9);
					chargepump=chargepump|(2<<1);
					break;

					case 62500:
					chargepump=(chargepump&0xF9);
					chargepump=chargepump|(3<<1);					
					break;

					case 125000:
					chargepump=(chargepump&0xC9);
					chargepump=chargepump|(0x3<<4);					
					break;

					case 142860:
					chargepump=(chargepump&0xC9);
					chargepump=chargepump|(0x3<<4);					
					chargepump=chargepump|(0x1<<1);/* RSA RSB */
					break;
					}
				Tuner->WrBuffer[2] = (unsigned char) chargepump;  
	
				channel_size=Tuner->ChannelBW;
				#ifdef HOST_PC
				UsrRdInt("CHANNEL_WIDTH",&channel_size); 			
				#endif
				
				if (frequency<=68000) 	   Tuner->WrBuffer[3] = (unsigned char) 0x01; /* VHF   I */ 
				else if (frequency<=228000) Tuner->WrBuffer[3] = (unsigned char) 0x02; /* VHF III */
				else 					   Tuner->WrBuffer[3] = (unsigned char) 0x08; /*   UHF   */
  						
  				if (channel_size==7) Tuner->WrBuffer[3]= ( (Tuner->WrBuffer[3]) | (0x10));
  					
				/* Tuner->WrBuffer[4] = (unsigned char) Tuner->Address; */
				Tuner->WrSubBuffer[2]= 	((chargepump & 0xC7)|(3<<3));
				Atc=1;
				#ifdef HOST_PC
				UsrRdInt("Atc",&Atc); 			
				#endif
				Tuner->WrSubBuffer[3] = (unsigned char) (0x20 | (Atc<<7));
				
				/******* ATC Implementation*****/
				
				#ifdef STTUNER_ATCBIT_TUNER_ENABLE
				TunerReadWrite((TUNER_Handle_t)Tuner,WRITE); /* Write the initial tuner*/
				/*** Now making ATC bit = 1 ******/
				if (frequency < 735000)
				{
				   Tuner->WrBuffer[2]= 0x9C;/*** Enabling Auxillary Byte to
				                                 be written over BandSwitch
				                                 byte*****/
				}
				else
				{
				   Tuner->WrBuffer[2]= 0xDC;/*** Enabling Auxillary Byte to
				                                 be written over BandSwitch
				                                 byte*****/   
				}
				 
				Tuner->WrBuffer[3]= 0xA0;/*** Making ATC bit 1 to make AGC
				                              current high . This is recommendable
				                              during channel select and search
				                              mode *****/
			        #endif
			/*******************************************************/
			break;	
			#endif

		#ifdef STTUNER_DRV_TER_TUN_TDA6650
		case  	STTUNER_TUNER_TDA6650:
					#ifdef HOST_PC
					UsrRdInt("Atc",&Atc); 			
					#endif
				chargepump=(unsigned char)0xC8;
				switch(Tuner->StepSize)
					{
					case 50000:
		
					chargepump=(chargepump|0x3);	
					
					break;
					
					case 166667:
					
					chargepump=chargepump|(2);
					break;

					case 62500:
					chargepump=chargepump;
					break;

					case 125000:
					chargepump=chargepump|(0x4);					
					break;

					case 142860:
					chargepump=chargepump|(0x1);/* RSA RSB */
					break;
					}
				Tuner->WrBuffer[2]    = (unsigned char) chargepump;  
				Tuner->WrSubBuffer[2] = (unsigned char) chargepump;  	
				Tuner->WrBuffer[3]    = (unsigned char) 0xC4;  								
				if (frequency<=68000) 	    Tuner->WrSubBuffer[3] = (unsigned char) (0xC1); /* VHF   I */ 
				else if (frequency<=228000) Tuner->WrSubBuffer[3] = (unsigned char) (0xC2); /* VHF III */
				else 					    Tuner->WrSubBuffer[3] = (unsigned char) (0xC4); /*   UHF   */
				Tuner->WrBuffer[4]    = (unsigned char)((Tuner->WrBuffer[2] & 0x80)|(0x3)|(Atc<<3));
				Tuner->WrSubBuffer[4] = (unsigned char)( Tuner->WrBuffer[4]& (0xF7)); 		/*   UHF   */
  		break;	
		#endif

		#ifdef STTUNER_DRV_TER_TUN_TDM1316
		case  	STTUNER_TUNER_TDM1316:
					#ifdef HOST_PC
					UsrRdInt("Atc",&Atc); 			
					#endif
				chargepump=(unsigned char)0xC8;
				switch(Tuner->StepSize)
					{
					case 50000:
		
					chargepump=(chargepump|0x3);	
					
					break;
					
					case 166667:
					
					chargepump=chargepump|(2);
					break;

					case 62500:
					chargepump=chargepump;
					break;

					}
				Tuner->WrBuffer[2]    = (unsigned char) chargepump;  
				channel_size=Tuner->ChannelBW;
				Tuner->WrBuffer[3]    = (unsigned char) 0xAC; 
				if(Tuner->StepSize ==50000 || Tuner->StepSize ==62500)
				{
				   Tuner->WrBuffer[3]    = (unsigned char) 0x44;    
				   if (frequency<=68000)
				   {
				      Tuner->WrBuffer[3] = (unsigned char) (0x49); /* VHF   I */ 
				   }
				   else if (frequency<=228000)
				   {
				      Tuner->WrBuffer[3] = (unsigned char) (0x4A); /* VHF III */
				   }
				   else
				   {
				      Tuner->WrBuffer[3] = (unsigned char) (0x4C); /*   UHF   */
				   }
				   if(channel_size == 7)
				   {
				      Tuner->WrBuffer[3] &= 0xF7;
				   }
				   Tuner->WrSubBuffer[3] = Tuner->WrSubBuffer[3];
				}
				else
				{
				   if (frequency<=68000)
				   {
				      Tuner->WrBuffer[3] = (unsigned char) (0x69); /* VHF   I */ 
				   }
				   else if (frequency<=228000)
				   {
				      Tuner->WrBuffer[3] = (unsigned char) (0x6A); /* VHF III */
				   }
				   else if (frequency <=620000)
				   {
				      Tuner->WrBuffer[3] = (unsigned char) (0x6C); /*   UHF   */
				   }
				   else
				   {
				        Tuner->WrBuffer[3] = (unsigned char) (0xAC); /*   UHF   */
				   }
				   if(channel_size == 7)
				   {
				      Tuner->WrBuffer[3] &= 0xF7;
				   } 					    
				   Tuner->WrSubBuffer[3] = Tuner->WrSubBuffer[3];
				}
  		break;	
		#endif

			
		#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
		case  	STTUNER_TUNER_ENG47402G1:
				chargepump = 0x88;							
				Tuner->WrBuffer[2] = (unsigned char) chargepump;  
				loc_CB=0x0;
				if (frequency <= 230000) 
					{
					loc_CB = 0x62;
					}
				else if (frequency <= 518000)
					{
					loc_CB = 0x64;					
					}
				else if (frequency <= 758000)
					{
					loc_CB = 0xA4;					
					}
				else 
					{
					loc_CB = 0xE4;					
					}
				
			Tuner->WrBuffer[3]   = (unsigned char) loc_CB;  			
			Tuner->WrSubBuffer[2]= (unsigned char) 0xFA;
			Tuner->WrSubBuffer[3]= (unsigned char) 0x6C;
		break;	
		#endif

			
			/*************************NEW TUNER**********************************************************/
			
		#ifdef STTUNER_DRV_TER_TUN_ED5058
		case  	STTUNER_TUNER_ED5058:
				/*if 		(frequency <= 239900) chargepump=(unsigned char)0x32;
				  else if (frequency <= 309900) chargepump=(unsigned char)0x52;
				  else if (frequency <= 379900) chargepump=(unsigned char)0x72;
				  else if (frequency <= 429900) chargepump=(unsigned char)0x92;*/
				
				
				/***************** UHF BAND ***********************************/
				if      (frequency <= 577900) chargepump=(unsigned char)0x94;
				else if (frequency <= 649900) chargepump=(unsigned char)0xB4;
				else if (frequency <= 745900) chargepump=(unsigned char)0xD4;
				else  chargepump=(unsigned char)0xF4;
			
					
				
			
				Tuner->WrBuffer[3] = (unsigned char) chargepump;
				channel_size=Tuner->ChannelBW;
				#ifdef HOST_PC
				UsrRdInt("CHANNEL_WIDTH",&channel_size); 			
				#endif
	
				
				
				
  				if (channel_size==7) Tuner->WrBuffer[3]= ( (Tuner->WrBuffer[3]) | (0x10)); 
  	
  	
  	
  			/*	Tuner->WrSubBuffer[3]=Tuner->WrBuffer[3];*/
				/* Tuner->WrBuffer[4] = (unsigned char) Tuner->Address; */
				Tuner->WrBuffer[2]= 0xCA; /* Configuration en mode normal et avec freq step de 166,67 khz*/
				/* The auxilary byte replaces bandswitch byte */
				
				Atc=1;
				#ifdef HOST_PC
				UsrRdInt("Atc",&Atc); 			
				#endif
				
				Tuner->WrBuffer[4] = (unsigned char) (0x80 | (Atc<<3));
		break;		
		#endif

		/******************************************************************************************/
		
		/*************MIVAR********************************************/
		#ifdef STTUNER_DRV_TER_TUN_MIVAR
		case  	STTUNER_TUNER_MIVAR:
					/*#ifdef HOST_PC
					UsrRdInt("Atc",&Atc); 			
					#else
					Atc=1;
					#endif*/
					Atc=1;
					frequency+=Tuner->IF; 
			
				if (frequency <= 136000)      chargepump=(unsigned char)0x41;
				else if (frequency <= 160000) chargepump=(unsigned char)0x61;
				else if (frequency <= 164000) chargepump=(unsigned char)0x81;
				else if (frequency <= 176000) chargepump=(unsigned char)0xA1;
				else if (frequency <= 185000) chargepump=(unsigned char)0xC1;/*spec modified*/
				else if (frequency <= 196000) chargepump=(unsigned char)0x42;/*spec modified*/
				
				else if (frequency <= 284000) chargepump=(unsigned char)0x42;
				else if (frequency <= 324000) chargepump=(unsigned char)0x62;
				else if (frequency <= 364000) chargepump=(unsigned char)0x82;
				else if (frequency <= 408000) chargepump=(unsigned char)0xA2;
				else if (frequency <= 444000) chargepump=(unsigned char)0xC2;
				else if (frequency <= 484000) chargepump=(unsigned char)0xA2;/* Should be 0xA2 */
				
				else if (frequency <= 500000) chargepump=(unsigned char)0x44;
				else if (frequency <= 524000) chargepump=(unsigned char)0x64;
				else if (frequency <= 560000) chargepump=(unsigned char)0x84;
				else if (frequency <= 712000) chargepump=(unsigned char)0xA4;
				else if (frequency <= 868000) chargepump=(unsigned char)0xC4;
			 /* else if (frequency <= 878000) chargepump=(unsigned char)0xE4;*/ /*should be 0xE4 */
				else if (frequency <= 904000) chargepump=(unsigned char)0xA4;/*should be 0xE4 */

				else  chargepump=(unsigned char)0xC4;
				Tuner->WrSubBuffer[3] = (unsigned char) chargepump;  
				/*
				chargepump=(chargepump&(0x3F));
				chargepump|=(2<<5);
				*/
				Tuner->WrBuffer[3] = (unsigned char) chargepump;  
				Tuner->WrBuffer[2] = (unsigned char) 0x8B;
				Tuner->WrSubBuffer[2] = (unsigned char) 0x83;				
				
				
				chargepump=(unsigned char)0xC8;
				
				
				switch(Tuner->StepSize)
					{
					case 50000:
		
					chargepump=(chargepump|0x3);	
					
					break;
					
					case 166667:
					
					chargepump=chargepump|(2);
					break;

					case 62500:
					chargepump=chargepump;
					break;

					case 125000:
					chargepump=chargepump|(0x4);					
					break;

					case 142860:
					chargepump=chargepump|(0x1);/* RSA RSB */
					break;
					}
				
				
				
				Tuner->WrBuffer[4]    = (unsigned char)(chargepump);
				Tuner->WrSubBuffer[4] = (unsigned char)(chargepump ); 		/*   UHF   */
				frequency-=Tuner->IF;   
		break;					
		#endif
			
			/**************TDED4**************************/
		#ifdef STTUNER_DRV_TER_TUN_TDED4
		case  	STTUNER_TUNER_TDED4:
				channel_size=Tuner->ChannelBW;
				#ifdef HOST_PC
				UsrRdInt("CHANNEL_WIDTH",&channel_size); 			
				#endif
  				if 		(frequency <= 230000) 	chargepump=(unsigned char)0x02;
				else if (frequency <= 824000) 	chargepump=(unsigned char)0x08;
				else 							chargepump=(unsigned char)0x88;
		/*		if (channel_size==8) (chargepump=chargepump|4); 				*/
				Tuner->WrBuffer[3] = chargepump;  
				
		break;		
		#endif

			/************DTT7102*********************/
		#ifdef STTUNER_DRV_TER_TUN_DTT7102
		case  	STTUNER_TUNER_DTT7102:
				if 		(frequency <= 230000) chargepump=(unsigned char)0xB4;/* modifs of the spec */
				else if (frequency <= 699000) chargepump=(unsigned char)0xBC;
				else if (frequency <= 799000) chargepump=(unsigned char)0xF4;
				else if (frequency <= 860000) chargepump=(unsigned char)0xFC;
				else  chargepump=(unsigned char)0xFC;
				switch(Tuner->StepSize)
					{
					case 50000:
		
					chargepump=(chargepump&0xC9);	
					
					break;
					case 31250:
					chargepump=(chargepump&0xC9);
					chargepump=chargepump|((0x1)<<1);
					break;
					
					case 166667:
					chargepump=(chargepump&0xF9);
					chargepump=chargepump|(2<<1);
					break;

					case 62500:
					chargepump=(chargepump&0xF9);
					chargepump=chargepump|(3<<1);					
					break;

					case 125000:
					chargepump=(chargepump&0xC9);
					chargepump=chargepump|(0x3<<4);					
					break;

					case 142860:
					chargepump=(chargepump&0xC9);
					chargepump=chargepump|(0x3<<4);					
					chargepump=chargepump|(0x1<<1);/* RSA RSB */
					break;
					}
				Tuner->WrBuffer[2] = (unsigned char) chargepump;  
	
				channel_size=Tuner->ChannelBW;
				#ifdef HOST_PC
				UsrRdInt("CHANNEL_WIDTH",&channel_size); 			
				#endif
				
				if (frequency<=68000) 	   Tuner->WrBuffer[3] = (unsigned char) 0x01; /* VHF   I */ 
				else if (frequency<=228000) Tuner->WrBuffer[3] = (unsigned char) 0x02; /* VHF III */
				else 					   Tuner->WrBuffer[3] = (unsigned char) 0x08; /*   UHF   */
  						
  				if (channel_size==7) Tuner->WrBuffer[3]= ( (Tuner->WrBuffer[3]) | (0x10));
  					
				/* Tuner->WrBuffer[4] = (unsigned char) Tuner->Address; */
				Tuner->WrSubBuffer[2]= 	((chargepump & 0xC7)|(3<<3));
				Atc=1;
				#ifdef HOST_PC
				UsrRdInt("Atc",&Atc); 			
				#endif
				Tuner->WrSubBuffer[3] = (unsigned char) (0x20 | (Atc<<7));
		break;		
		#endif

			
			/***************TECC2849PG******************************/
		#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
		case  	STTUNER_TUNER_TECC2849PG:
				if 		(frequency <= 150000) chargepump=(unsigned char)0xB0;/* modifs of the spec */
				else if (frequency <= 174000) chargepump=(unsigned char)0xB8;
				else if (frequency <= 400000) chargepump=(unsigned char)0xB0;
				else if (frequency <= 466000) chargepump=(unsigned char)0xB8;
				else if (frequency <= 730000) chargepump=(unsigned char)0xB8;
				else if (frequency <= 860000) chargepump=(unsigned char)0xF4;
				else  chargepump=(unsigned char)0xB8;
				switch(Tuner->StepSize)
					{
								
					case 166667:

					chargepump=chargepump|(2<<1);
					break;
					
					case 50000:
					case 31250:
					case 62500:
					
					chargepump=chargepump|(3<<1);					
					break;

					case 125000:
					
					chargepump=chargepump|(0x0<<1);					
					break;

					case 142860:

                                        
					chargepump=chargepump|(0x1<<1);/* RSA RSB */
					break;
					}
				/*UsrRdInt("Atc",&Atc);*/
				  Atc=1;
				  chargepump=chargepump|(Atc);/* RSA RSB */
				
				Tuner->WrBuffer[2] = (unsigned char) chargepump;  
	
				channel_size=Tuner->ChannelBW;
				#ifdef HOST_PC
				UsrRdInt("CHANNEL_WIDTH",&channel_size); 			
				#endif
				
				if (frequency<=174000) 	   Tuner->WrBuffer[3] = (unsigned char) 0x01;  /* VHF LOW  */ 
				else if (frequency<474000) Tuner->WrBuffer[3] = (unsigned char) 0x02;  /* VHF HIGH */
				else 					   Tuner->WrBuffer[3] = (unsigned char) 0x08;  /*    UHF   */
  				
				#ifdef HOST_PC
				UsrRdInt("Atc",&Atc); 			
				#endif
		break;		
		#endif

			
		#ifdef STTUNER_DRV_TER_TUN_TDCC2345
		case  	STTUNER_TUNER_TDCC2345:
				
				if(frequency <= 150000) chargepump=(unsigned char)0xB8;/* modifs of the spec */
				else if (frequency <= 174000) chargepump=(unsigned char)0xF0;
				else if (frequency <= 374000) chargepump=(unsigned char)0xB8;
				else if (frequency <= 454000) chargepump=(unsigned char)0xF0;
				else if (frequency <= 470000) chargepump=(unsigned char)0xF8;
				else if (frequency <= 542000) chargepump=(unsigned char)0xB8;
				else if (frequency <= 830000) chargepump=(unsigned char)0xF0;
				else if (frequency <= 860000) chargepump=(unsigned char)0xF8;
				else  chargepump=(unsigned char)0xB8;
				switch(Tuner->StepSize)
					{
					
					case 166667:
					chargepump=chargepump|(2<<1);
					break;
					
					case 50000:
					case 31250:
					case 62500:
					chargepump=chargepump|(3<<1);					
					break;

					case 125000:
					
					chargepump=chargepump|(0xC<<1);					
					break;

					case 142860:
					chargepump=chargepump|(0xD<<1);/* RSA RSB */
					break;
					}
				/*UsrRdInt("Atc",&Atc); */
				Atc=1;
				chargepump=chargepump|(Atc); /* RSA RSB */
				
				Tuner->WrBuffer[2] = (unsigned char) chargepump;  
	
				channel_size=Tuner->ChannelBW;
				#ifdef HOST_PC
				UsrRdInt("CHANNEL_WIDTH",&channel_size); 			
				#endif
				
				if (frequency<=174000) 	   Tuner->WrBuffer[3] = (unsigned char) 0x01;  /* VHF LOW  */ 
				else if (frequency<474000) Tuner->WrBuffer[3] = (unsigned char) 0x02;  /* VHF HIGH */
				else 					   Tuner->WrBuffer[3] = (unsigned char) 0x08;  /*    UHF   */
  				
				#ifdef HOST_PC
				UsrRdInt("Atc",&Atc); 			
				#endif
		break;		
		#endif
		
			
			default:
			break;
			}
		
		frequency+=Tuner->IF; 
		
		nbsteps = (int) ( (frequency * 1000 + Tuner->StepSize/2)/Tuner->StepSize);
	       
		LL_TunerSetNbSteps(Tuner,nbsteps);

		
	}
	 /******* ATC Implementation*****/
		 #ifdef STTUNER_ATCBIT_TUNER_ENABLE
		 
		 if (Tuner->TunerType == STTUNER_TUNER_DTT7592 )
				{
			        /*** Wait for 50 msec before checking whether PLL locks or not ***/		
				WAIT_IN_MS(50);		
				loop = 20;
					
				while ((FL==0) && (loop>0))
					{
					   WAIT_IN_MS(1);	
					   loop-=1;
					   /***Reading for checking PLL lock*****/
					   TunerReadWrite((TUNER_Handle_t)Tuner,READ);
					   FL=Tuner->RdBuffer[0];
					   FL=(FL && 0x40); 
					}
				
				if (FL)
					{
					   Tuner->WrBuffer[3]    = (unsigned char) 0x20;/** Making AGC current low***/
					   TunerReadWrite((TUNER_Handle_t)Tuner,WRITE);  
					}
					else
					{
					   Tuner->Error = TNR_PLL_ERR ;
					  
					}
					
				}
		#endif
	
	/********************************************/
	
	*returned_freq=LL_TunerGetFrequency(Tuner);	
	return Tuner->Error;

}



#endif /***** #ifndef STTUNER_MINIDRIVER *******/

#ifdef STTUNER_MINIDRIVER
#include	"chip.h"
#include "stcommon.h"   
#include 	"string.h"
#include 	"tuntdrv.h"
#include	"ll_tun0360.h"
#include        "sttbx.h"
#define 	WRITE 1
#define 	READ  0

#ifdef ST_OS21
#define WAIT_IN_MS(X)     task_delay( (signed int)(X * (ST_GetClocksPerSecond() / 1000)) )   /*task_delay(X)*/
#else
#define WAIT_IN_MS(X)     task_delay( (unsigned int)(X * (ST_GetClocksPerSecond() / 1000)) )   /*task_delay(X)*/
#endif


/*****************************************************
**FUNCTION	::	LL_TunerInit
**ACTION	::	Initialize the tuner according to its type
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE  
**RETURN	::	NONE
*****************************************************/
void LL_TunerInit(TUNTDRV_InstanceData_t* Tuner)
{
#ifdef HOST_PC
int step;
double If;
#endif
	
		Tuner->SelectBW = 0;
		Tuner->BandWidth[TUN_MAX_BW] = 0;	/* To avoid list without end	*/ 
/* dcdc		Tuner->Repeator=0x0;*/ /* no repeator by defaults */
		switch(Tuner->TunerType)
		{
			
			#if defined(STTUNER_DRV_TER_TUN_DTT7578)||defined(STTUNER_DRV_TER_TUN_DTT7592) 				 
			case STTUNER_TUNER_DTT7578:
			case STTUNER_TUNER_DTT7592:
				
				if(Tuner->TunerType == STTUNER_TUNER_DTT7578)
				{
				   Tuner->PLLType	=	TUNER_PLL_DTT7578;/* need to be verified */
				}
				else
				{
				   Tuner->PLLType	=	TUNER_PLL_DTT7592;/* need to be verified */
				}
				Tuner->SubAddress=0xC0;
				Tuner->WrSubSize=4;		
				Tuner->I_Q = 1;
				Tuner->FreqFactor = 1;
				Tuner->StepSize = (int)166667L;
				Tuner->IF = (int)36000L;
				Tuner->BandWidth[0] = (int)7600L;
				Tuner->BandWidth[1] = (int)0L;
		/*dcdc		Tuner->Repeator = 0;*/
			
				Tuner->WrSize = 4;
				Tuner->WrBuffer[0] = 0x2;
				Tuner->WrBuffer[1] = 0x34;
				Tuner->WrBuffer[2] = 0x93;
				Tuner->WrBuffer[3] = 0x05;
				
				Tuner->WrSubBuffer[0] = 0x2;
				Tuner->WrSubBuffer[1] = 0x34;
				Tuner->WrSubBuffer[2] = 0x93;
				Tuner->WrSubBuffer[3] = 0x05;
			
				Tuner->RdSize = 1;
				Tuner->RdBuffer[0] = 0x00; 
			break;
			#endif
			
			default:
				Tuner->Error = TNR_TYPE_ERR;
			break;
		}
}

/*****************************************************
**FUNCTION	::	LL_tuner_tuntdrv_Select
**ACTION	::	Select the type of tuner used
**PARAMS IN	::	type	==> type of the tuner
**				address	==> I2C address of the tuner
**PARAMS OUT::	NONE  
**RETURN	::	NONE
*****************************************************/

void LL_TunerSelect(TUNTDRV_InstanceData_t* Tuner,STTUNER_TunerType_t type,unsigned char address)

{
	Tuner->Error=TNR_NO_ERR;
	Tuner->TunerType = type;
	Tuner->Address = address;
	
}

/*****************************************************
**FUNCTION	::	LL_TunerGetNbSteps
**ACTION	::	Get the number of steps of the tuner
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE  
**RETURN	::	Number of steps, 0 if an error occur
*****************************************************/
int LL_TunerGetNbSteps(TUNTDRV_InstanceData_t* Tuner)
{
	int nbsteps = 0;
		
	        switch (Tuner->PLLType)
		{
			case TUNER_PLL_DTT7578:
			case TUNER_PLL_DTT7592:
				nbsteps	 = ((int)Tuner->WrBuffer[0] & 0x0000007F)<<8L;
				nbsteps	|= ((int)Tuner->WrBuffer[1] & 0x000000FF);
				
			break;
			default:
				Tuner->Error = TNR_PLL_ERR;
			break;
		}
	
	
		
	return nbsteps;
}

/*****************************************************
**FUNCTION	::	LL_TunerGetFrequency
**ACTION	::	Get the frequency of the tuner
**PARAMS IN	::	NONE  
**PARAMS OUT::	NONE  
**RETURN	::	frequency of the tuner (KHz), 0 if an error occur 
*****************************************************/
int LL_TunerGetFrequency(TUNTDRV_InstanceData_t* Tuner)
{
	int frequency = 0;
	
	if(Tuner->StepSize>0)
	{
				 
		  frequency = ((LL_TunerGetNbSteps(Tuner)*(Tuner->StepSize/2)) - 500*Tuner->IF)/500	;
		  frequency *= Tuner->FreqFactor;
		
	}
	else
		Tuner->Error = TNR_STEP_ERR;
	
	return frequency; 
}

/*****************************************************
**FUNCTION	::	LL_TunerSetNbSteps
**ACTION	::	Set the number of steps of the tuner
**PARAMS IN	::	nbsteps ==> number of steps of the tuner
**PARAMS OUT::	NONE  
**RETURN	::	NONE
*****************************************************/
void LL_TunerSetNbSteps(TUNTDRV_InstanceData_t* Tuner,int nbsteps)
{
	
		switch(Tuner->PLLType)
		{

			
			case TUNER_PLL_DTT7578:
			case TUNER_PLL_DTT7592:
				Tuner->WrBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
				Tuner->WrSubBuffer[0] = (unsigned char) ( (nbsteps >> 8) & 0x7f );
				Tuner->WrSubBuffer[1] = (unsigned char) ( (nbsteps >> 0) & 0xff );
  			break;
                        default:
				Tuner->Error = TNR_PLL_ERR;
			break ;
		}
		
		TunerReadWrite(WRITE); 
		
		
	
}


/*****************************************************
**FUNCTION	::	LL_TunerSetFrequency
**ACTION	::	Set the frequency of the tuner
**PARAMS IN	::	frequency ==> the frequency of the tuner (KHz) 
**PARAMS OUT::	NONE  
**RETURN	::	real tuner frequency, 0 if an error occur 
*****************************************************/

/*ST_ErrorCode_t	tuner_tuntdrv_SetFrequency(TUNER_Handle_t  Handle,U32 frequency,(U32*)&returned_freq); */
 ST_ErrorCode_t LL_TunerSetFrequency(TUNTDRV_InstanceData_t* Tuner,U32 frequency,U32* returned_freq)  

{
	int nbsteps = 0,     
		chargepump = 0,
		channel_size,
		Atc;
	
	
	#ifdef STTUNER_ATCBIT_TUNER_ENABLE
	int loop = 0;
	int FL = 0;
	#endif
	/*Code added for MT2060*/
	Tuner->Error=TNR_NO_ERR;
	
	if(Tuner->StepSize > 0)
	{
		
		switch(Tuner->TunerType)
		{
	
			case STTUNER_TUNER_DTT7578:
			case STTUNER_TUNER_DTT7592:
				if 		(frequency <= 230000) chargepump=(unsigned char)0xB4;/* modifs of the spec */
				else if (frequency <= 699000) chargepump=(unsigned char)0xBC;
				else if (frequency <= 799000) chargepump=(unsigned char)0xF4;
				else if (frequency <= 860000) chargepump=(unsigned char)0xFC;
				else  chargepump=(unsigned char)0xFC;
				switch(Tuner->StepSize)
					{
					case 50000:
		
					chargepump=(chargepump&0xC9);	
					
					break;
					case 31250:
					chargepump=(chargepump&0xC9);
					chargepump=chargepump|((0x1)<<1);
					break;
					
					case 166667:
					chargepump=(chargepump&0xF9);
					chargepump=chargepump|(2<<1);
					break;

					case 62500:
					chargepump=(chargepump&0xF9);
					chargepump=chargepump|(3<<1);					
					break;

					case 125000:
					chargepump=(chargepump&0xC9);
					chargepump=chargepump|(0x3<<4);					
					break;

					case 142860:
					chargepump=(chargepump&0xC9);
					chargepump=chargepump|(0x3<<4);					
					chargepump=chargepump|(0x1<<1);/* RSA RSB */
					break;
					}
				Tuner->WrBuffer[2] = (unsigned char) chargepump;  
	
				channel_size=Tuner->ChannelBW;
				#ifdef HOST_PC
				UsrRdInt("CHANNEL_WIDTH",&channel_size); 			
				#endif
				
				if (frequency<=68000) 	   Tuner->WrBuffer[3] = (unsigned char) 0x01; /* VHF   I */ 
				else if (frequency<=228000) Tuner->WrBuffer[3] = (unsigned char) 0x02; /* VHF III */
				else 					   Tuner->WrBuffer[3] = (unsigned char) 0x08; /*   UHF   */
  						
  				if (channel_size==7) Tuner->WrBuffer[3]= ( (Tuner->WrBuffer[3]) | (0x10));
  					
				/* Tuner->WrBuffer[4] = (unsigned char) Tuner->Address; */
				Tuner->WrSubBuffer[2]= 	((chargepump & 0xC7)|(3<<3));
				Atc=1;
				#ifdef HOST_PC
				UsrRdInt("Atc",&Atc); 			
				#endif
				Tuner->WrSubBuffer[3] = (unsigned char) (0x20 | (Atc<<7));
				
				/******* ATC Implementation*****/
				
				#ifdef STTUNER_ATCBIT_TUNER_ENABLE
				TunerReadWrite(WRITE); /* Write the initial tuner*/
				/*** Now making ATC bit = 1 ******/
				if (frequency < 735000)
				{
				   Tuner->WrBuffer[2]= 0x9C;/*** Enabling Auxillary Byte to
				                                 be written over BandSwitch
				                                 byte*****/
				}
				else
				{
				   Tuner->WrBuffer[2]= 0xDC;/*** Enabling Auxillary Byte to
				                                 be written over BandSwitch
				                                 byte*****/   
				}
				 
				Tuner->WrBuffer[3]= 0xA0;/*** Making ATC bit 1 to make AGC
				                              current high . This is recommendable
				                              during channel select and search
				                              mode *****/
			        #endif
			/*******************************************************/
				
			break;
			
			default:
			break;
		}
		
		frequency+=Tuner->IF; 
		nbsteps = (int) ( (frequency * 1000 + (Tuner->StepSize/2))/Tuner->StepSize);
		LL_TunerSetNbSteps(Tuner,nbsteps);
		
	}
	 /******* ATC Implementation*****/
	 #ifdef STTUNER_ATCBIT_TUNER_ENABLE
	 if (Tuner->TunerType == STTUNER_TUNER_DTT7592 )
			{
		        /*** Wait for 50 msec before checking whether PLL locks or not ***/		
			WAIT_IN_MS(50);		
			loop = 20;
				
			while ((FL==0) && (loop>0))
				{
				   WAIT_IN_MS(1);	
				   loop-=1;
				   /***Reading for checking PLL lock*****/
				   TunerReadWrite(READ);
				   FL=Tuner->RdBuffer[0];
				   FL=(FL && 0x40); 
				}
			
			if (FL)
				{
				   Tuner->WrBuffer[3]    = (unsigned char) 0x20;/** Making AGC current low***/
				   TunerReadWrite(WRITE);  
				}
				else
				{
				   Tuner->Error = TNR_PLL_ERR ;
				  
				}
				
			}
	#endif
	
	/********************************************/
	
	*returned_freq=LL_TunerGetFrequency(Tuner);	
	return Tuner->Error;

}
#endif /****************#ifdef STTUNER_MINIDRIVER******************/
