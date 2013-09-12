#include "d0899_util.h"
#include "d0899_init.h"
#include "drv0899.h"
#include "ioreg.h"
#include "sttuner.h"

#define MULT32X32(a,b) (((((long)((a)>>16))*((long)((b)>>16)))<<1) +((((long)((a)>>16))*((long)(b&0x0000ffff)))>>15) + ((((long)((b)>>16))*((long)((a)&0x0000ffff)))>>15))

static char l1d8[256] =	/* Lookup table to evaluate exponent*/
 {
         8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 
         3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };


/*****************************************************
**FUNCTION	::	FE_899_XtoPowerY
**ACTION	::	Compute  x^y (where x and y are integers) 
**PARAMS IN	::	Number -> x
**				Power -> y
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/
U32 FE_899_XtoPowerY(U32 Number, U32 Power)
{
	int i;
	long result = 1;
	
	for(i=0;i<Power;i++)
		result *= Number;
		
	return (U32)result;
}

/*****************************************************
**FUNCTION	::	RoundToNextHighestInteger
**ACTION	::	return Ceil(n.d)  
**PARAMS IN	::	Number -> integer part
**				Digits -> decimal part
**PARAMS OUT::	NONE
**RETURN	::	Ceil(n.d)
*****************************************************/
S32 FE_899_RoundToNextHighestInteger(S32 Number,U32 Digits)
{
	S32 result = 0,
		highest = 0;
	
	highest = FE_899_XtoPowerY(10,Digits);
	result = Number / highest;
	
	if((Number - (Number/highest)*highest) > 0)
		result += 1;
	
	return result;
}

/*****************************************************
**FUNCTION	::	FE_899_PowOf2
**ACTION	::	Compute  2^n (where n is an integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/
long FE_899_PowOf2(int number)
{
	int i;
	long result=1;
	
	for(i=0;i<number;i++)
		result*=2;
		
	return result;
}


/*****************************************************
**FUNCTION	::	FE_899_GetPowOf2
**ACTION	::	Compute  x according to y with y=2^x 
**PARAMS IN	::	number -> y
**PARAMS OUT::	NONE
**RETURN	::	x
*****************************************************/
long FE_899_GetPowOf2(int number)
{
	int i=0;
	
	while(FE_899_PowOf2(i)<number) i++;
		
	return i;
}


long Log2Int(int number)
{
	long i;

	i = 0;
	while((int)FE_899_PowOf2((int)i) <= ABS(number))
		i++;

	if (number == 0)
		i= 1;
    
    return i - 1;
}

short DSPnormalize(long *arg, short maxnorm)
{

	long input;
	int one, two, three, four;
	long  retval;
	input = (*arg<0)?~(*arg):*arg;

	one = ((unsigned char) (input >> 24));   
	two = ((unsigned char) (input >> 16));
	three = ((unsigned char)(input >> 8));
	four  = ((unsigned char)(input));
     
	retval = one ? l1d8[one]-1L  : two ? 7L+ l1d8[two] : three ? 15L + l1d8[three] : 23L + l1d8[four];
	return ((retval>maxnorm)? maxnorm:(short)(retval)); 


} /* end DSPnormalize*/

/*****************************************************
**FUNCTION	::	Log10Int
**ACTION	::	Compute  log(n) (where n is an integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	log(n)
*****************************************************/
long Log10Int(long logarg)
{
 	long powval1,powval2,powval3,powval4,powval5,powval6,powval7,powval8;
	long coeff0, coeff1, coeff2;
	long powexp_l;
	short powexp;
	int SignFlag = 0;
	
	/*Initialize coeffs 0.31 format*/
	coeff0	= 1422945213;		/* This coeff in 0.31 format;*/
	coeff1	= 2143609158;
	coeff2	= 724179374;

        /*Normalize power measure */
	powexp = DSPnormalize(&logarg, 32);
	logarg<<=powexp;
	
	if(powexp&0x8000)
	{
		SignFlag = 1;
		powexp	= -powexp;
	}
	
	powexp = 31 - powexp;
	powexp_l = ((long)(powexp)<<24);
	powval1 = powexp_l;
	powval4 = MULT32X32(logarg,coeff1);
	powval5 = coeff0 - powval4;
	powval2 = MULT32X32(logarg,logarg);
	powval3 = MULT32X32(powval2,coeff2);
	powval6 = powval5 + powval3;
	powval7 = powval6>>5;
	powval8 = powval1 - powval7;

	if(SignFlag ==1)
		powval8 = -powval8;
	
	return(powval8);	
	
} 
/*****************************************************
**FUNCTION	::	SqrtInt
**ACTION	::	Compute  sqrt(n) (where n is an integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	sqrt(n)
*****************************************************/
int SqrtInt(int Sq)
{
	int an,
		increment,
		an1;

	increment=0;
	an=Sq;

	while(increment<15) 
	{
		increment=increment+1;
    		an1=(an+Sq/an)/2;
    		an=an1; 
    }
    return an;
}
/*****************************************************
**FUNCTION	::	FE_STB0899_SetStandard
**ACTION	::	Set a standrad DVBS1, DVBS2 or DSS
**PARAMS IN	::  hChip		==>	handle to the chip
**				Standard	==>	DVBS1, DVBS2 or DSS
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_STB0899_SetStandard(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, STTUNER_FECType_t Standard, STTUNER_FECMode_t FECMode)
{
	if(FECMode == STTUNER_FEC_MODE_DIRECTV)
	{
			STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_FECM_RESERVED0,FSTB0899_FECM_RESERVED0_INFO,0x1);
			STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_VITON,FSTB0899_VITON_INFO,1); /*FECM/viterbi_on*/

			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_RSULC,0xa1);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_TSULC,0x61);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_RSLLC,0x42);
			/*STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_TSLPL,0x12);*/
			
			STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_FRESLDPC,FSTB0899_FRESLDPC_INFO,1);
			
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_STOPCLK1,0x78);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_STOPCLK2,0x07);

			
	}
	else
	if(Standard == STTUNER_FEC_MODE_DVBS1)
	{	
			STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_FECM_RESERVED0,FSTB0899_FECM_RESERVED0_INFO,0x0);
			STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_VITON,FSTB0899_VITON_INFO,1); /*FECM/viterbi_on*/

			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_RSULC,0xb1);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_TSULC,0x40);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_RSLLC,0x42);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_TSLPL,0x12);
			
			STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_FRESLDPC,FSTB0899_FRESLDPC_INFO,1);
			
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_STOPCLK1,0x78);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_STOPCLK2,0x07);


	}
	else if(Standard == STTUNER_FEC_MODE_DVBS2)
	{
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_FECM_RESERVED0,FSTB0899_FECM_RESERVED0_INFO,0x0);
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_VITON,FSTB0899_VITON_INFO,0); /*FECM/viterbi_on*/ 

			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_RSULC,0xb1);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_TSULC,0x42);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_RSLLC,0x40);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_TSLPL,0x2);
			
			STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_FRESLDPC,FSTB0899_FRESLDPC_INFO,0);
			
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_STOPCLK1,0x20);
			STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_STOPCLK2,0x0);
		
	}
	
}
/*****************************************************
**FUNCTION	::	FE_STB0899_GetMclkFreq
**ACTION	::	Set the STB0899 master clock frequency
**PARAMS IN	::  hChip		==>	handle to the chip
**				ExtClk		==>	External clock frequency (Hz)
**PARAMS OUT::	NONE
**RETURN	::	Synthesizer frequency (Hz)
*****************************************************/
U32 FE_STB0899_GetMclkFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 ExtClk)
{
	U32 mclk = 90000000,
	div = 0;
		
	div=STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_MDIV, FSTB0899_MDIV_INFO);
	
	mclk=(div+1)*ExtClk/6;
	
	return mclk; 
}
