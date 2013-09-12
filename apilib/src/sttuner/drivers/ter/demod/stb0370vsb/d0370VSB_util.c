#include "ioreg.h"
#include "370_VSB_map.h"

#ifdef HOST_PC
	#include "gen_macros.h"
#endif

#define MAX_MEAN 512 
static int  RegExtClk; 
 
			

/**  solution to div() function*/
typedef struct div_s
{
	int quot;
	int rem;
}div_t;


/*Assuming DIV Value*/
/*************/
div_t div(int divisor, int divider)
{
	div_t ldiv;
	ldiv.quot = divisor/divider;
	ldiv.rem  = divisor%divider;
	return ldiv;
}
/***************/


/*****************************************************
**FUNCTION	::	PowOf2
**ACTION	::	Compute  2^n (where n is an integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/
unsigned long PowOf2(int number)
{
	int i;
	unsigned long result=1;
	
	for(i=0;i<number;i++)
		result*=2;
		
	return result;
}

/*****************************************************
**FUNCTION	::	LongSqrt
**ACTION	::	Compute  the square root of n (where n is a long integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	sqrt(n)
*****************************************************/
long  LongSqrt(long Value)
{
	/* this routine performs a classical root extraction
	** on long integers */
	long Factor=1;
	long Root=1;
	long R;
	long Ltmp;
	
	if(Value<=1)
	  return (Value);
	
	Ltmp=Value;
	
	while (Ltmp>3)	
	{
		Factor<<=2;
		Ltmp>>=2;
	}
	
	R=Value-Factor; /*	Ratio	*/ 
	Factor>>=2;
	
	while(Factor>0)
	{
		Root<<=1;
		Ltmp=Root<<1;
		Ltmp*=Factor;
		Ltmp+=Factor;
		Factor>>=2;
		
		if(R-Ltmp>=0)
		{
			R=R-Ltmp;
			Root+=1;
		}
	}
	
	return (Root) ;
}



/*****************************************************
**FUNCTION	::	RegSetExtClk
**ACTION	::	Set the value of the external clock variable
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/
void  RegSetExtClk(S32 _Value)
{
	RegExtClk = _Value;
}
  
/*****************************************************
**FUNCTION	::	RegGetExtClk
**ACTION	::	Get the external clock value
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	External clock value
*****************************************************/
long  RegGetExtClk(void)
{
	return (RegExtClk);
}


unsigned long int GetBERPeriod(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle)
{

U32 berperiod;
berperiod=(U32)(STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370_BER_PERIOD_LLSB)+
			(STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370_BER_PERIOD_LSB)<<8)+
			(STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370_BER_PERIOD_MSB)<<16)+
			(STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370_BER_PERIOD_MMSB)<<24)); 
return berperiod; 
}

U32 Calc_vcxoOffset(U32 SymbolRate,U32 clk)
{
U16 idx;
	U32 result;

		
	result  = (U32)((SymbolRate * PowOf2(8))/(U32)(clk/1000000));
	result =(U32)(result * PowOf2(5));
	
	for (idx=1; idx<=6; idx++)
		{
			result/=5;
			result *=4;
		}

	
	return result;
}

U32 Calc_NCOcnst(U32 IFfrequency,U32 clk)
{

	U32 result;

	/*another way*/	
	/*IFfrequency=IFfrequency/1000;
	result=IFfrequency * PowOf2(17);
	clk=clk/100000;
	
	result=(((result/clk)* PowOf2(6))/100)+1;*/
	
	result= (U32)((U32)(IFfrequency/1000) * PowOf2(17));
	
	result/=clk/1000000;

	result= (U32)(result * PowOf2(5));
	result/=500;
	return result;
}



void Set_vcxoOffset_Regs(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle,int value)
{

	div_t divi;   
	

	divi=div(value,16777216); 
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370_VCXOOFFSET4,divi.quot);
	
	divi=div(divi.rem,65536);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370_VCXOOFFSET3,divi.quot);
	
	divi=div(divi.rem,256);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370_VCXOOFFSET2,divi.quot);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370_VCXOOFFSET1,divi.rem);
	
					
}

void Set_NCOcnst_Regs(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle,int value)
{


	div_t divi;    
        
	divi=div(value,65536);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370_NCOCNST_MMSB,divi.quot);
	
	divi=div(divi.rem,256);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370_NCOCNST_MSB,divi.quot);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F0370_NCOCNST_LSB,divi.rem);
	
					
}

 /* commented, may be used in future*/
/*
int Get_NCOcnst_Value(STCHIP_Handle_t hChip) 
{
int result;
result=  ChipGetField(hChip,F0370_NCOCNST_LSB)+
		(ChipGetField(hChip,F0370_NCOCNST_MSB)<<8)+
		(ChipGetField(hChip,F0370_NCOCNST_MMSB)<<16);					
return result;
}
*/

int Get_NCOerr_Value(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle) 
{
int result;
result=  STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370_NCOERR_LSB)+
		(STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370_NCOERR_MSB)<<8)+
		(STTUNER_IOREG_GetField(DeviceMap, IOHandle,F0370_NCOERR_MMSB)<<16);					
return result;
}

S32 Get_FrequencyOffset(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle,U32 clk) /* return freq offset in khz */  
{

S32 NCOerr;
S32 frequencyoffset;
U16 idx;
NCOerr=	Get_NCOerr_Value(DeviceMap,IOHandle); 

if (NCOerr>=65536)
	{
		NCOerr= NCOerr-131072;
		frequencyoffset=(NCOerr*(clk/1000));
		
		for (idx=1; idx<=3; idx++)
			{
				frequencyoffset/=8;
				frequencyoffset *=5;
			}
		
		frequencyoffset/=(S32)(PowOf2(11));
	}
else
	{
	
		frequencyoffset=(NCOerr*(clk/1000));
		
		for (idx=1; idx<=3; idx++)
			{
				frequencyoffset/=8;
				frequencyoffset *=5;
			}
		
		frequencyoffset= (S32)(frequencyoffset / (PowOf2(11)));

	}

return (int)frequencyoffset;
}

/*temporary commented*/
/*int Get_vcxoOffset_Value(STCHIP_Handle_t hChip)
{
int result;
	result=ChipGetField(hChip,VCXOOFFSET1)+(ChipGetField(hChip,VCXOOFFSET2)<<8)+
		  (ChipGetField(hChip,VCXOOFFSET3)<<16)+(ChipGetField(hChip,VCXOOFFSET4)<<24);					
return result;
}*/





