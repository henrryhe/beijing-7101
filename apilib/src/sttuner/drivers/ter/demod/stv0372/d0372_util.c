#include "d0372_init.h"
#include "d0372_map.h"
#include "chip.h"

		

/**  solution to div() function*/
typedef struct UTIL_372_div_s
{
	int quot;
	int rem;
}UTIL_372_div_t;

/*Assuming DIV Value*/
/*************/
UTIL_372_div_t UTIL_372_div(int divisor, int divider)
{
	UTIL_372_div_t ldiv;
	ldiv.quot = divisor/divider;
	ldiv.rem  = divisor%divider;
	return ldiv;
}


/*****************************************************
**FUNCTION	::	UTIL_372_PowOf2
**ACTION	::	Compute  2^n (where n is an integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/
unsigned long UTIL_372_PowOf2(int number)
{
	int i;
	unsigned long result=1;
	
	for(i=0;i<number;i++)
		result*=2;
		
	return result;
}

U32 UTIL_372_Calc_vcxoOffset(U32 SymbolRate,U32 clk)
{

	U32 result;
	U16 idx;

	result  = (U32)((SymbolRate * UTIL_372_PowOf2(8))/(U32)(clk/1000000));
	result =(U32)(result*UTIL_372_PowOf2(5));
	
	for (idx=1; idx<=6; idx++)
		{
			result/=5;
			result *=4;
		}
	
	return result;
}

U32 UTIL_372_Calc_NCOcnst(U32 IFfrequency,U32 clk)
{

	U32 result;

	result= (U32)(((U32)(IFfrequency/1000)) * UTIL_372_PowOf2(17));
	
	result/=clk/1000000;

	result= (U32)(result * UTIL_372_PowOf2(5));
	result/=500;
	return result;
}


void UTIL_372_Set_vcxoOffset_Regs(IOARCH_Handle_t IOHandle,int value)
{

	UTIL_372_div_t divi;   
	

	divi=UTIL_372_div(value,16777216); 
	ChipSetField(IOHandle,F0372_VCXOOFFSET4,divi.quot);
	
	divi=UTIL_372_div(divi.rem,65536);
	ChipSetField(IOHandle,F0372_VCXOOFFSET3,divi.quot);
	
	divi=UTIL_372_div(divi.rem,256);
	ChipSetField(IOHandle,F0372_VCXOOFFSET2,divi.quot);
	ChipSetField(IOHandle,F0372_VCXOOFFSET1,divi.rem);
	
					
}

void UTIL_372_Set_NCOcnst_Regs(IOARCH_Handle_t IOHandle,int value)
{


	UTIL_372_div_t divi;    
        
	divi=UTIL_372_div(value,65536);
	ChipSetField(IOHandle,F0372_NCOCNST_MMSB,divi.quot);
	
	divi=UTIL_372_div(divi.rem,256);
	ChipSetField(IOHandle,F0372_NCOCNST_MSB,divi.quot);
	ChipSetField(IOHandle,F0372_NCOCNST_LSB,divi.rem);
	
					
}


int UTIL_372_Get_NCOerr_Value(IOARCH_Handle_t IOHandle) 
{
int result;
result=  ChipGetField(IOHandle,F0372_NCOERR_LSB)+
		(ChipGetField(IOHandle,F0372_NCOERR_MSB)<<8)+
		(ChipGetField(IOHandle,F0372_NCOERR_MMSB)<<16);					
return result;
}

S32 UTIL_372_Get_FrequencyOffset(IOARCH_Handle_t IOHandle,U32 clk) /* return freq offset in khz */  
{

S32  NCOerr;
S32 frequencyoffset;
U16 idx;

NCOerr=	UTIL_372_Get_NCOerr_Value(IOHandle); 
if (NCOerr>=65536)
	{
		NCOerr= NCOerr-131072;
		frequencyoffset=(NCOerr*(clk/1000));
		for (idx=1; idx<=3; idx++)
			{
				frequencyoffset/=8;
				frequencyoffset *=5;
			}
		frequencyoffset/=(S32)(UTIL_372_PowOf2(11));
	}
else
	{
	        frequencyoffset=(NCOerr*(clk/1000));
	        for (idx=1; idx<=3; idx++)
			{
				frequencyoffset/=8;
				frequencyoffset *=5;
			}
	
		frequencyoffset =(S32)(frequencyoffset / (UTIL_372_PowOf2(11))); 
		}

return (int)frequencyoffset;
}






