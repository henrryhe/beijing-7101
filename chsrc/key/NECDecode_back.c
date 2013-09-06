/*
	(c) Copyright changhong
  Name:NECDecode.c
  Description:the soft decode program of NEC remote code
  Authors:yxl
  Date          Remark
  2005-04-01    Created
*/

#include "stblast.h"
#include "sttbx.h"

#define DECODE_SUCCESS 0
#define WRONG_LEVEL 1,
#define DISTURB_SIGNAL 2


#define NEW_KEY_LEADING_MAX 5500 
#define NEW_KEY_LEADING_MIN 4500

#define CONTINUOUS_KEY_LEADING_MAX 3000 
#define CONTINUOUS_KEY_LEADING_MIN 2500

#define LOGICAL_0_MAX 1200 
#define LOGICAL_0_MIN 900

#define LOGICAL_1_MAX 2500 
#define LOGICAL_1_MIN 2000

#define NEW_KEY_SYMBOL_COUNT 35 
#define CONTINUOUS_KEY_SYMBOL_COUNT 3

/*#define NEC_DECODE_DEBUG */


static U8 LastCodeNumber=0;
static U8 LastKeyCode[5]={0x0,0x0,0x0,0x0,0x0};


int NEC_Decode(U32* piSymbolCount,void* pSymbolValue,U32* pKeyCodeNum,U8* pKeyData)
{

	STBLAST_Symbol_t SymbolBuffer[100];
	short i,j;
	short CodeCountTemp,BitCountTemp; 
	U32 SymbolCountTemp;
	U32 LeadCodeMin;
	U32 LeadCodeMax;
	U8 DataTemp;
	U8 MaskData[8]={0xFE,0xFD,0xFB,0xF7,0xEF,0xDF,0xBF,0x7F};
	U8 KeyCode[10];

	SymbolCountTemp=*piSymbolCount;
#if 0 /*yxl 2005-09-21 modify this section*/
	if(SymbolCountTemp==0) return 1;/*no key read*/
#else
	if(SymbolCountTemp==0)/*no key read*/
	{
		LastCodeNumber=0;
		return 1;
	}

#endif/*end yxl 2005-09-21 modify this section*/


	if(SymbolCountTemp==NEW_KEY_SYMBOL_COUNT)
	{
		LeadCodeMin=NEW_KEY_LEADING_MIN;
		LeadCodeMax=NEW_KEY_LEADING_MAX;
	}
	else if(SymbolCountTemp==CONTINUOUS_KEY_SYMBOL_COUNT)
	{
		LeadCodeMin=CONTINUOUS_KEY_LEADING_MIN;
		LeadCodeMax=CONTINUOUS_KEY_LEADING_MAX;		
	}
#if 0 /*yxl 2005-09-23 cancel this section*/
	else if(SymbolCountTemp==0) 
		return 1;/*no key read*/

#endif/*end yxl 2005-09-23 cancel this section*/
	else return 2;

	memcpy((void*)SymbolBuffer,(const void*)pSymbolValue,sizeof(STBLAST_Symbol_t)*SymbolCountTemp);


#ifdef NEC_DECODE_DEBUG
	STTBX_Print(("\n YxlInfo():SymbolCount=%d \n",SymbolCountTemp));
	if(SymbolCountTemp==CONTINUOUS_KEY_SYMBOL_COUNT)
	{
		for(i=0;i<SymbolCountTemp;i++)
		{
			STTBX_Print(("\ni=%d H=%d L=%d T=%d",i,SymbolBuffer[i].MarkPeriod,
				SymbolBuffer[i].SymbolPeriod-SymbolBuffer[i].MarkPeriod,
				SymbolBuffer[i].SymbolPeriod));
			STTBX_Print(("\n"));
		}
	}
#endif

	/*judge leading code*/
	if(SymbolBuffer[1].SymbolPeriod<LeadCodeMin||SymbolBuffer[1].SymbolPeriod>LeadCodeMax)
		return 2;

	/*judge data code*/

	if(SymbolCountTemp==CONTINUOUS_KEY_SYMBOL_COUNT)/*in continuous key status*/
	{
	
		if(SymbolBuffer[2].SymbolPeriod==65535) 
		{
			if(LastCodeNumber==0) 
			{
				memset((void*)LastKeyCode,0,sizeof(U8)*5);
				return 1;
			}
			else
			{
				*pKeyCodeNum=LastCodeNumber;
				memcpy((void*)pKeyData,(const void*)LastKeyCode,LastCodeNumber);
			/*	STTBX_Print(("\n YxlInfo:Contiuous key send out,Number=%d Value=%x %x %x %x ",
					LastCodeNumber,LastKeyCode[0],LastKeyCode[1],LastKeyCode[2],LastKeyCode[3]));*/
				return 0;
			}
		}
		else
		{
			return 1;
		}
	
	}
	else /*in new key status*/
	{

		
		CodeCountTemp=0;
		BitCountTemp=8;

		memset((void*)KeyCode,0,sizeof(U8)*10);
		for(i=2;i<SymbolCountTemp-1;i++)
		{
			BitCountTemp--;
			if(SymbolBuffer[i].SymbolPeriod>=LOGICAL_0_MIN&&
				SymbolBuffer[i].SymbolPeriod<=LOGICAL_0_MAX)
			{
				DataTemp=MaskData[BitCountTemp];
				KeyCode[CodeCountTemp]=KeyCode[CodeCountTemp]&DataTemp;
				
			}
			else if(SymbolBuffer[i].SymbolPeriod>=LOGICAL_1_MIN&&
				SymbolBuffer[i].SymbolPeriod<=LOGICAL_1_MAX)
			{
				DataTemp=~MaskData[BitCountTemp];
				KeyCode[CodeCountTemp]=KeyCode[CodeCountTemp]|DataTemp;				
			}
			else
				 return 3;
			if(BitCountTemp<=0)
			{
				BitCountTemp=8;
				CodeCountTemp++;
			}


		}

	}
	*pKeyCodeNum=CodeCountTemp;


	memcpy((void*)pKeyData,(const void*)KeyCode,CodeCountTemp);
 
	LastCodeNumber=CodeCountTemp;
	memcpy((void*)LastKeyCode,(const void*)KeyCode,CodeCountTemp);

	return 0;
}


int NEC_DigitalKeyConvert(int KeyPressed)
{
	switch(KeyPressed)
	{
	case 0x00ff:
		return 0;
	case 0x807f:
		return 1;
	case 0x40bf:
		return 2;
	case 0xc03f:
		return 3;
	case 0x20df:
		return 4;
	case 0xa05f:
		return 5;
	case 0x609f:
		return 6;
	case 0xe01f:
		return 7;
	case 0x10ef:
		return 8;
	case 0x906f:
		return 9;
	default:
		return KeyPressed;
	}
}
