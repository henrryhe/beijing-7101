/*
	(c) Copyright changhong
  Name:NECDecode.c
  Description:the soft decode program of NEC remote code
  Authors:yxl
  Date          Remark
  2005-04-01    Created

Modifiction:
Date:2006-01-05 by yxl
1.Add new static variable ConKeyCount=0,modify function  NEC_Decode(),
解决按住键不放时产生的连续键过快及过多的问题。

*/
#include <string.h>
#include "stblast.h"
#include "sttbx.h"
#include "NEC_keymap.h"
#include "..\report\report.h"
//#include "initterm.h"

#define DECODE_SUCCESS 0
#define WRONG_LEVEL 1
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
#if 1
#define con_up_arrow_key	0x53ac
#define con_down_arrow_key	0x4bb4
#define con_left_arrow_key	0x9966
#define con_right_arrow_key	0x837c

#define con_p_up_key		55335
#define con_p_down_key		63495
#define con_vol_up_key		51255
#define con_vol_down_key	10455
#define con_page_up_key		0xbb44
#define con_page_down_key	0x31ce

#define   EMAIL_KEY                 0xaa55
#define  	KEY_F3			0xaf5
#define  	KEY_F4			0x8A75 	
#define  	KEY_F5			0x4AB5 	
#define  	KEY_F6			0xCA35 	
#define   FAV_KEY                     0xe817   /*喜爱键*/
#else
#define con_up_arrow_key	0xd827
#define con_down_arrow_key	0xf807
#define con_left_arrow_key	0x7887
#define con_right_arrow_key	0x58a7

#define con_p_up_key		55335
#define con_p_down_key		63495
#define con_vol_up_key		51255
#define con_vol_down_key	10455
#define con_page_up_key		21165
#define con_page_down_key	53805
#endif


static U8 LastCodeNumber=0;
static U8 LastKeyCode[5]={0x0,0x0,0x0,0x0,0x0};
static U8 ConKeyCount=0;/*yxl 2006-01-05 add this variable*/

int NEC_Decode(U32* piSymbolCount,void* pSymbolValue,U32* pKeyCodeNum,U8* pKeyData)
{

	STBLAST_Symbol_t 	SymbolBuffer[100];
	short 				i/*,j jqz051226 rmk */;
	short 				CodeCountTemp,BitCountTemp; 
	U32 					SymbolCountTemp;
	U32 					LeadCodeMin;
	U32 					LeadCodeMax;
	U8 					DataTemp;
	U8 					MaskData[8]={0xFE,0xFD,0xFB,0xF7,0xEF,0xDF,0xBF,0x7F};
	U8 					KeyCode[10];
	//int 					SpecKey;
	static boolean continueKey=false ;

	//U8					uMask[50], uPer[50];

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

	#if 0
	/* jqz070118 add */
	do_report ( 0, "\nKeyCount1=[%d]==========================\n", SymbolCountTemp );
	for ( i = 0; i < SymbolCountTemp; i ++ )
	{
		//if ( i %8 == 0 )
		//	do_report ( 0, "\n" );

		if ( i == 0 )
			uMask[i] = SymbolBuffer[i].MarkPeriod;
		else
			uMask[i] = SymbolBuffer[i].MarkPeriod / 1000;
		
		do_report ( 0, "%02d,", uMask[i] );
	}
	do_report ( 0, "\nKeyCount2=[%d]==========================\n", SymbolCountTemp );
	for ( i = 0; i < SymbolCountTemp; i ++ )
	{
		//if ( i %8 == 0 )
		//	do_report ( 0, "\n" );

		if ( i == 0 )
			uPer[i] = SymbolBuffer[i].SymbolPeriod;
		else
			uPer[i] = SymbolBuffer[i].SymbolPeriod / 1000;


		do_report ( 0, "%02d,", uPer[i] );
	}
	do_report ( 0, "\nEnd==============================\n" );
	/* end jqz07118 add */
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
				continueKey = false;
				return 1;
			}
			else
			{
#if 0 /*yxl 2006-01-05 modify this section*/
				SpecKey = LastKeyCode[2] * 256 + LastKeyCode[3];
	
				//do_report ( 0, "\n[SpecKey=0x%x]", SpecKey );
			 	if(continueKey ||  con_up_arrow_key == SpecKey || con_down_arrow_key == SpecKey || con_left_arrow_key == SpecKey || con_right_arrow_key == SpecKey || \
					con_p_up_key == SpecKey ||con_p_down_key == SpecKey ||con_vol_up_key == SpecKey ||con_vol_down_key == SpecKey )
			 	{
					*pKeyCodeNum=LastCodeNumber;
					memcpy((void*)pKeyData,(const void*)LastKeyCode,LastCodeNumber);
					continueKey = true;
					return 0;
			 	}
#else
				ConKeyCount++;
                                continueKey = true;
				if(ConKeyCount>=2)
				{
					ConKeyCount=0;
					*pKeyCodeNum=LastCodeNumber;
					memcpy((void*)pKeyData,(const void*)LastKeyCode,LastCodeNumber);
					return 0;				
				}
				else return 5;

#endif/*end yxl 2006-01-05 modify this section*/
			}

                        LastCodeNumber = 0;
				continueKey = false;				
				return 1;

		}
		else
		{
			return 1;
		}
	
	}
	else /*in new key status*/
	{

		ConKeyCount=0;/*yxl 2006-01-05 add this line*/	
		continueKey = false;		
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
	#if 0
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
	#else
	case 255:	return 0;
	case 32895:	return 1;
	case 16575:	return 2;
	case 49215:	return 3;
	case 8415:	return 4;
	case 41055:	return 5;
	case 24735:	return 6;
	case 57375:	return 7;
	case 4335:	return 8;
	case 36975:	return 9;
	case 53295:	return /*EPG_KEY*/0x827d;
	case 39015:	return /*UP_ARROW_KEY*/0xd827;
	case 47175:	return /*DOWN_ARROW_KEY*/0xf807;
	case 14535:	return /*LEFT_ARROW_KEY*/0x7887;
	case 20655:	return /*OK_KEY*/0xda25;
	case 25245:	return /*RED_KEY*/0x629d;
	case 17085:	return /*GREEN_KEY*/0x42bd;
	case 23205:	return /*BLUE_KEY*/0x5aa5;
	case 27285:	return /*YELLOW_KEY*/0x6a95;
	case 4845:	return /*LIST_KEY*/0X12ed;
	/*case 59415:	return *//*MENU_KEY*//*0X6897*/;
	/* jqz060227 add */
	#if 0
	case 0xc837:	return /* VOLUME_UP_KEY */0xfffe;
	case 0x28d7: return /* VOLUME_DOWN_KEY */0xfffd;
	#endif
	case 0x8877: return /* RETURN_KEY */ 0xa858;
	/* end jqz060227 add */
	#endif
	default:
		return KeyPressed;
	}
}

#if 1
#if 0	/* jqz070127 rmk */
typedef struct
{
	U8 mu_KeyMask[35];
	U8 mu_KeyPeriod[35];
}SW_Key_t;

static SW_Key_t sw_Key[39] = 
{
	{	/* 0. 待机键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00,01,01,01,00,01,01,01,01,00,00,00,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,01,02,02,02,01,02,02,02,02,01,01,01,02,01,01,65 }
	},

	{	/* 1. 静音键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00,01,01,01,00,00,01,01,01,00,00,00,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,01,02,02,02,01,01,02,02,02,01,01,01,02,02,01,65 }
	},

	{
		/* 2. 声道键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00,01,00,00,00,00,01,01,01,00,01,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,01,02,01,01,01,01,02,02,02,01,02,02,02,02,01,65 }
	},

	{
		/* 3. 电视/广播键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,00,00,01,01,00,01,01,00,01,01,00,00,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,01,01,02,02,01,02,02,01,02,02,01,01,02,01,01,65 }
	},

	{
		/* 4. 邮件键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,00,01,00,00,00,00,01,00,01,00,01,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,01,02,01,01,01,01,02,01,02,01,02,02,02,02,01,65 }
	},

	{	/* 5. 点播键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,01,00,00,00,00,01,01,00,00,01,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,02,01,01,01,01,02,02,01,01,02,02,02,02,01,65 }
	},

	{
		/* 6. 咨询键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00,00,00,01,00,01,01,01,01,01,01,00,01,00,00,00 },
		{ 01,05,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,01,01,01,02,01,02,02,02,02,02,02,01,02,01,01,65 }
	},

	{
		/* 7. 证券键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,01,01,00,00,00,01,01,00,00,00,01,01,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,02,02,01,01,01,02,02,01,01,01,02,02,02,01,01,65 }
	},

	{
		/* 8. 菜单键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,00,01,00,01,00,00,01,00,01,00,01,00,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,01,02,01,02,01,01,02,01,02,01,02,01,02,02,01,65 }
	},

	{
		/* 9. 上键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,00,01,00,00,01,01,01,00,01,00,01,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,01,02,01,01,02,02,02,01,02,01,02,02,01,01,65 }
	},

	{
		/* 10. 退出键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,00,01,00,00,00,01,01,00,01,00,01,01,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,01,02,01,01,01,02,02,01,02,01,02,02,02,01,01,65 }
	},

	{
		/* 11. 左键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,00,00,01,01,00,00,01,00,01,01,00,00,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,01,01,02,02,01,01,02,01,02,02,01,01,02,02,01,65 }
	},

	{
		/* 12. 确认键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,01,01,00,00,01,01,01,00,00,00,01,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,02,02,01,01,02,02,02,01,01,01,02,02,01,01,65 }
	},

	{
		/* 13. 右键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,00,00,00,00,00,01,01,00,01,01,01,01,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,01,01,01,01,01,02,02,01,02,02,02,02,02,01,01,65 }
	},

	{
		/* 14. 信息键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00,00,00,00,00,00,01,01,01,01,01,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,01,01,01,01,01,01,02,02,02,02,02,02,02,02,01,65 }
	},

	{
		/* 15. 下键 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,00,00,01,00,01,01,01,00,01,01,00,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,01,01,02,01,02,02,02,01,02,02,01,02,01,01,65 }
	},

	{
		/* 16. 指南 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,01,00,01,00,01,01,01,00,00,01,00,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,02,01,02,01,02,02,02,01,01,02,01,02,01,01,65 }
	},

	{
		/* 17. F3 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,00,01,01,00,00,01,01,00,01,00,00,01,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,01,02,02,01,01,02,02,01,02,01,01,02,02,01,01,65 }
	},

	{
		/* 18. F4 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,00,01,01,00,00,00,01,00,01,00,00,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,01,02,02,01,01,01,02,01,02,01,01,02,02,02,01,65 }
	},

	{
		/* 19. page_up_key */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,00,01,01,01,00,01,01,00,01,00,00,00,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,01,02,02,02,01,02,02,01,02,01,01,01,02,01,01,65 }
	},

	{
		/* 20. F5 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,65 }
	},

	{
		/* 21. F6 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,00,00,00,00,00,01,01,00,01,01,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,01,01,01,01,01,02,02,01,02,02,02,02,02,01,65 }
	},

	{
		/* 22. page_down_key */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00,01,01,00,00,00,01,01,01,00,00,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,01,02,02,01,01,01,02,02,02,01,01,02,02,02,01,65 }
	},

	{
		/* 23. F7 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,00,01,01,00,01,01,01,00,01,00,00,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,01,02,02,01,02,02,02,01,02,01,01,02,01,01,65 }
	},

	{
		/* 24. F8 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,01,00,00,00,00,00,01,00,00,01,01,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,02,01,01,01,01,01,02,01,01,02,02,02,02,02,01,65 }
	},

	{
		/* 25. F9 */
		{ 01,04,01,01,00,00,00,00,00,01,00,01,00,01,01,01,01,00,00,01,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01 },
		{ 01,04,02,02,01,01,01,01,01,02,01,02,01,02,02,02,02,01,01,02,01,01,01,01,01,01,02,01,02,02,02,02,02,02,65 }
	},

	{
		/* 26. F10 */
		{ 01,04,01,01,00,00,00,00,00,01,00,01,00,01,01,01,01,00,00,00,00,01,00,00,00,00,01,01,01,00,01,01,01,01,01 },
		{ 01,04,02,02,01,01,01,01,01,02,01,02,01,02,02,02,02,01,01,01,01,02,01,01,01,01,02,02,02,01,02,02,02,02,65 }
	},

	{
		/* 27. 1 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,00,00,01,00,00,01,01,00,01,01,00,01,01,00,00 },
		{ 01,05,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,01,01,02,01,01,02,02,01,02,02,01,02,02,01,65 }
	},

	{
		/* 28. 2 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,01,00,00,01,00,00,01,00,00,01,01,00,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,02,01,01,02,01,01,02,01,01,02,02,01,02,02,01,65 }
	},

	{
		/* 29. 3 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00,01,01,00,00,01,01,01,01,00,00,01,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,01,02,02,01,01,02,02,02,02,01,01,02,02,01,01,65 }
	},

	{
		/* 30. 4 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,01,01,00,00,00,01,01,00,00,00,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,02,02,01,01,01,02,02,01,01,01,02,02,02,01,65 }
	},

	{
		/* 31.5 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,01,01,01,00,00,00,01,00,00,00,00,01,01,01,00,00 },
		{ 01,05,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,02,02,02,01,01,01,02,01,01,01,01,02,02,02,01,65 }
	},

	{
		/* 32.6 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00,00,01,00,00,01,01,01,01,01,00,01,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,01,01,02,01,01,02,02,02,02,02,01,02,02,01,01,65 }
	},

	{
		/* 33.7 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,00,01,00,00,00,01,01,00,01,00,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,01,02,01,01,01,02,02,01,02,01,02,02,02,01,65 }
	},

	{
		/* 34.8 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,01,00,01,00,00,00,01,00,00,01,00,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,02,01,02,01,01,01,02,01,01,02,01,02,02,02,01,65 }
	},

	{
		/* 35.9 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00,01,00,00,00,01,01,01,01,00,01,01,01,00,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,01,02,01,01,01,02,02,02,02,01,02,02,02,01,01,65 }
	},

	{
		/* 36.喜爱 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,00,00,01,00,00,00,01,01,01,01,00,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,01,01,02,01,01,01,02,02,02,02,01,02,02,02,01,65 }
	},

	{
		/* 37.0 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,01,01,01,00,00,00,00,01,00,00,00,01,01,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,02,02,02,01,01,01,01,02,01,01,01,02,02,02,02,01,65 }
	},

	{
		/* 38.回看 */
		{ 01,04,01,00,00,00,00,00,00,00,01,00,01,01,01,01,01,01,00,01,01,00,01,00,00,01,01,00,00,01,00,01,01,00,00 },
		{ 01,04,02,01,01,01,01,01,01,01,02,01,02,02,02,02,02,02,01,02,02,01,02,01,01,02,02,01,01,02,01,02,02,01,65 }
	}
};

static U32 gi_sw_skey[38] = {
/* 0. 待机键 */		0x17e7788,
/* 1. 静音键 */		0x17e738c,
/* 2. 声道键 */		0x17e43bc,
/* 3. 电视/广播键 */0x17f36c8,
/* 4. 邮件键 */		0x17f42bc,
/* 5. 点播键 */		0x17ec33c,
/* 6. 咨询键 */		0x17e17e8,
/* 7. 证券键 */		0x17fc638,
/* 8. 菜单键 */		0x17f52ac,
/* 9. 上键 */			0x17ea758,
/* 10. 退出键 */		0x17f46b8,
/* 11. 左键 */			0x17f32cc,
/* 12. 确认键 */		0x17ee718,
/* 13. 右键 */			0x17f06f8,
/* 14. 信息键 */		0x17e03fc,
/* 15. 下键 */			0x17e9768,
/* 16. 指南 */			0x17ed728,
/* 17. F3 */				0x17f6698,
/* 18. F4 */				0x17f629c,
/* 19. page_up_key */		0x17f7688,
/* 20. F5 */				0x17f02fc,
/* 21. F6 */				0x17e837c,
/* 22. page_down_key */	0x17e639c
/* 23. F7 */				0x17eb748,
/* 24. F8 */				0x17f827c,
/* 25. F9 */				0x82bc817f,
/* 26. F10 */				0x82bc21df,
/* 27. 1 */				0x17e936c,
/* 28. 2 */				0x17f926c
/* 29. 3 */				0x17e6798,
/* 30. 4 */				0x17ee31c,
/* 31.5 */				0x17fe21c,
/* 32.6 */				0x17e27d8,
/* 33.7 */				0x17ea35c,
/* 34.8 */				0x17fa25c,
/* 35.9 */				0x17e47b8,
/* 36.喜爱 */			0x17e23dc,
/* 37.0 */				0x17fc23c,
/* 38.回看 */			0x17ed32c,
};
#endif

#if 1	/* jqz070713 add */
int gi_StdKey[40] = {
#else
int gi_StdKey[39] = {
#endif
		POWER_KEY,
		MUTE_KEY,
		AUDIO_KEY,
		TV_RADIO_KEY,
		EMAIL_KEY,
		NVOD_KEY,
		HTML_KEY,
		STOCK_KEY,
		MENU_KEY,
		UP_ARROW_KEY,
		EXIT_KEY,
		LEFT_ARROW_KEY,
		OK_KEY,
		RIGHT_ARROW_KEY,
		HELP_KEY,
		DOWN_ARROW_KEY,
		EPG_KEY,
		KEY_F3,
		KEY_F4,
		PAGE_UP_KEY,
		KEY_F5,
		KEY_F6,
		PAGE_DOWN_KEY,
		RED_KEY,
		GREEN_KEY,
		YELLOW_KEY,
		BLUE_KEY,
		KEY_DIGIT1,
		KEY_DIGIT2,
		KEY_DIGIT3,
		KEY_DIGIT4,
		KEY_DIGIT5,
		KEY_DIGIT6,
		KEY_DIGIT7,
		KEY_DIGIT8,
		KEY_DIGIT9,
		FAV_KEY,
		KEY_DIGIT0,
		RETURN_KEY
		, GAME_KEY	/* jqz070713 add */
	};

/*
  * Func: JL_ParseKey
  * Desc: 针对吉林遥控器的解码程序
  * In:	采样值
  * Out:	-1		没有键值
  		其他	键值
  */
int JL_ParseKey ( U32* piSymbolCount, void* pSymbolValue, U32* pKeyCodeNum, U8* pKeyData )
{
	U8					uMask[50];//, uPer[50];
	int					i;
	STBLAST_Symbol_t 	SymbolBuffer[100];
	U32 					SymbolCountTemp;
	//int					iCmp1, iCmp2;
	static int				si_PrevKey = -1;
	clock_t				t_TimeNow;
	static clock_t			t_TimeWait1, t_TimeWait2;
	U32					u_KeyValue;
	int					i_KeyRead;
	static boolean			b_FirstContinus 	= false;
	static U8				u_Continuse		= 0;

	SymbolCountTemp = *piSymbolCount;

	#if 1
	/* 处理连续键 */
	if ( 3 == SymbolCountTemp )
	{
		if ( -1 == si_PrevKey )
			return -1;
	
		t_TimeNow = time_now();

		if ( !( UP_ARROW_KEY == si_PrevKey || DOWN_ARROW_KEY == si_PrevKey || 
					LEFT_ARROW_KEY == si_PrevKey || RIGHT_ARROW_KEY == si_PrevKey 
				) )
		{
			si_PrevKey = -1;
			return -1;
		}
		else if ( time_after ( t_TimeNow, t_TimeWait2 ) == 1 )	/* 1/4秒后的连续键示为无效 */
		{
			si_PrevKey = -1;
			return -1;
		}
		else if ( time_after ( t_TimeNow, t_TimeWait1 ) != 1 )	/* 1/16秒内的连续键示为无效 */
		{
			//t_TimeWait1 = time_plus ( time_now(), ST_GetClocksPerSecond() / 8 );
			//do_report ( 0, "\nCKey TimeOut!" );
			return -1;
		}

		if ( true == b_FirstContinus )
		{
			if ( u_Continuse > 0 )
			{
				b_FirstContinus = false;
				return -1;
			}
			else
			{
				u_Continuse ++;
				return -1;
			}
		}

		/* 左右键要求慢一点 */
		t_TimeWait1 = time_plus ( time_now(), ST_GetClocksPerSecondLow() / 32 );

		t_TimeWait2 = time_plus ( time_now(), ST_GetClocksPerSecondLow() * 2 );

		//do_report ( 0, "\nGet Continues Key...!!" );
		
		return si_PrevKey;
	}
	#endif

	if ( 35 != SymbolCountTemp )
		return -1;

	memcpy((void*)SymbolBuffer,(const void*)pSymbolValue,sizeof(STBLAST_Symbol_t)*SymbolCountTemp);

	for ( i = 0; i < SymbolCountTemp; i ++ )
	{
		if ( i == 0 )
			uMask[i] = SymbolBuffer[i].MarkPeriod;
		else
			uMask[i] = SymbolBuffer[i].MarkPeriod / 1000;
	}

	for ( i = 3; i < SymbolCountTemp; i ++ )
	{
		u_KeyValue = ( u_KeyValue << 1 ) | uMask[i];
	}

	//do_report ( 0, "\nKey=0x%x", u_KeyValue );

	switch ( u_KeyValue )
	{
	case 0x17e7788:	/* 0. 待机键 */
	case 0xbe916f:
		i_KeyRead = gi_StdKey[0];
		break;
	case 0x17e738c:	/* 1. 静音键 */
	case 0xbe11ef:
		i_KeyRead = gi_StdKey[1];
		break;
	case 0x17e43bc:	/* 2. 声道键 */
	case 0xbf34cb:
		i_KeyRead = gi_StdKey[2];
		break;
	case 0x17f36c8:	/* 3. 电视/广播键 */
	case 0xbf24db:
		i_KeyRead = gi_StdKey[3];
		break;
	case 0x17f42bc:	/* 4. 邮件键 */
	case 0xbf54ab:
		i_KeyRead = gi_StdKey[4];
		break;
	case 0x17ec33c:	/* 5. 点播键 */
	case 0xbee51b:
		i_KeyRead = gi_StdKey[5];
		break;
	case 0x17e17e8:	/* 6. 咨询键 */
	case 0xbf649b:
		i_KeyRead = gi_StdKey[6];
		break;
	case 0x17fc638:	/* 7. 证券键 */
	case 0xbe45bb:
		i_KeyRead = gi_StdKey[7];
		break;
	case 0x17f52ac:	/* 8. 菜单键 */
	case 0xbed12f:
		i_KeyRead = gi_StdKey[8];
		break;
	case 0x17ea758:	/* 9. 上键 */
	case 0xbfb04f:
		i_KeyRead = gi_StdKey[9];
		break;
	case 0x17f46b8:	/* 10. 退出键 */
	case 0xbf50af:
		i_KeyRead = gi_StdKey[10];
		break;
	case 0x17f32cc:	/* 11. 左键 */
	case 0xbef10f:
		i_KeyRead = gi_StdKey[11];
		break;
	case 0x17ee718:	/* 12. 确认键 */
	case 0xbfb44b:
		i_KeyRead = gi_StdKey[12];
		break;
	case 0x17f06f8:	/* 13. 右键 */
	case 0xbeb14f:
		i_KeyRead = gi_StdKey[13];
		break;
	case 0x17e03fc:	/* 14. 信息键 */
	case 0xbe05fb:
		i_KeyRead = gi_StdKey[14];
		break;
	case 0x17e9768:	/* 15. 下键 */
	case 0xbff00f:
		i_KeyRead = gi_StdKey[15];
		break;
	case 0x17ed728:	/* 16. 指南 */
	case 0xbf04fb:
		i_KeyRead = gi_StdKey[16];
		break;
	case 0x17f6698:	/* 17. F3 */
		i_KeyRead = gi_StdKey[17];
		break;
	case 0x17f629c:	/* 18. F4 */
		i_KeyRead = gi_StdKey[18];
		break;
	case 0x17f7688:	/* 19. page_up_key */
	case 0xbea55b:
		i_KeyRead = gi_StdKey[19];
		break;
	case 0x17f02fc:	/* 20. F5 */
		i_KeyRead = gi_StdKey[20];
		break;
	case 0x17e837c:	/* 21. F6 */
	case 0xbf946b:	/* jqz070713 */
		i_KeyRead = gi_StdKey[21];
		break;
	case 0x17e639c:	/* 22. page_down_key */
	case 0xbfa45b:
		i_KeyRead = gi_StdKey[22];
		break;
	case 0x17eb748:	/* 23. F7 */
	case 0xbec53b:
		i_KeyRead = gi_StdKey[23];
		break;
	case 0x17f827c:	/* 24. F8 */
	case 0xbe857b:
		i_KeyRead = gi_StdKey[24];
		break;
	case 0x82bc817f:	/* 25. F9 */
	case 0xbed52b:
		i_KeyRead = gi_StdKey[25];
		break;
	case 0x82bc21df:	/* 26. F10 */
	case 0xbeb54b:
		i_KeyRead = gi_StdKey[26];
		break;
	case 0x17e936c:	/* 27. 1 */
	case 0xbf00ff:
		i_KeyRead = gi_StdKey[27];
		break;
	case 0x17f926c:	/* 28. 2 */
	case 0xbe817f:
		i_KeyRead = gi_StdKey[28];
		break;
	case 0x17e6798:	/* 29. 3 */
	case 0xbf807f:
		i_KeyRead = gi_StdKey[29];
		break;
	case 0x17ee31c:	/* 30. 4 */
	case 0xbe41bf:
		i_KeyRead = gi_StdKey[30];
		break;
	case 0x17fe21c:	/* 31.5 */
	case 0xbf40bf:
		i_KeyRead = gi_StdKey[31];
		break;
	case 0x17e27d8:	/* 32.6 */
	case 0xbec13f:
		i_KeyRead = gi_StdKey[32];
		break;
	case 0x17ea35c:	/* 33.7 */
	case 0xbfc03f:
		i_KeyRead = gi_StdKey[33];
		break;
	case 0x17fa25c:	/* 34.8 */
	case 0xbe21df:
		i_KeyRead = gi_StdKey[34];
		break;
	case 0x17e47b8:	/* 35.9 */
	case 0xbf20df:
		i_KeyRead = gi_StdKey[35];
		break;
	case 0x17e23dc:	/* 36.喜爱 */
		i_KeyRead = gi_StdKey[36];
		break;
	case 0x17fc23c:	/* 37.0 */
	case 0xbe01ff:
		i_KeyRead = gi_StdKey[37];
		break;
	case 0x17ed32c:	/* 38.回看 */
	//case 0xbf946b:	/* jqz070713 rmk */
		i_KeyRead = gi_StdKey[38];
		break;
		/* jqz070713 add */
	case 0xbe659b:	/* 39.生产工装键 */
		i_KeyRead = gi_StdKey[39];
		break;
		/* end jqz070713 add */
	default:
		i_KeyRead = -1;
		break;
	}


	if ( -1 != i_KeyRead )
	{
		si_PrevKey = i_KeyRead;

		/* 左右键要求慢一点 */
		t_TimeWait1 = time_plus ( time_now(), ST_GetClocksPerSecondLow() / 32 );
		t_TimeWait2 = time_plus ( time_now(), ST_GetClocksPerSecondLow() );

		b_FirstContinus = true;
		u_Continuse	  = 0;
	}

	return i_KeyRead;
	
	#if 0
	for ( i = 0; i < SymbolCountTemp; i ++ )
	{
		if ( i == 0 )
			uPer[i] = SymbolBuffer[i].SymbolPeriod;
		else
			uPer[i] = SymbolBuffer[i].SymbolPeriod / 1000;
	}
	for ( i = 0; i < 50; i ++ )
	{
		iCmp1 = memcmp ( &uMask[2], &(sw_Key[i].mu_KeyMask[2]), 33 );
		iCmp2 = memcmp ( &uPer[2], &(sw_Key[i].mu_KeyPeriod[2]), 33 );

		if ( 0 == iCmp1 && 0 == iCmp2 )
		{
			//do_report ( 0, "\n[%d][KEY]=0x%x", i, gi_StdKey[i] );
			si_PrevKey = gi_StdKey[i];

			if ( LEFT_ARROW_KEY == si_PrevKey || RIGHT_ARROW_KEY == si_PrevKey )
			{
				/* 左右键要求慢一点 */
				t_TimeWait1 = time_plus ( time_now(), ST_GetClocksPerSecond() / 2 );
			}
			else
			{
				t_TimeWait1 = time_plus ( time_now(), ST_GetClocksPerSecond() / 4 );
			}

			t_TimeWait2 = time_plus ( time_now(), ST_GetClocksPerSecond() );

			return gi_StdKey[i];
		}
	}
	#endif

	do_report ( 0, "\nINVALID_KEY!" );
	si_PrevKey = -1;
	return -1;
}
#endif


