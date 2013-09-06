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
#include "nec_keymap.h"
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
static U8 ConKeyCount=0;/*yxl 2006-01-05 add this variable*/

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
#if 0 /*yxl 2006-01-05 modify this section*/
				*pKeyCodeNum=LastCodeNumber;
				memcpy((void*)pKeyData,(const void*)LastKeyCode,LastCodeNumber);
			/*	STTBX_Print(("\n YxlInfo:Contiuous key send out,Number=%d Value=%x %x %x %x ",
					LastCodeNumber,LastKeyCode[0],LastKeyCode[1],LastKeyCode[2],LastKeyCode[3]));*/
				return 0;
#else
				ConKeyCount++;
				if(ConKeyCount>=5)
				{
					ConKeyCount=0;
					*pKeyCodeNum=LastCodeNumber;
					memcpy((void*)pKeyData,(const void*)LastKeyCode,LastCodeNumber);
					return 0;				
				}
				else return 5;

#endif/*end yxl 2006-01-05 modify this section*/
			}
		}
		else
		{
			return 1;
		}
	
	}
	else /*in new key status*/
	{

		ConKeyCount=0;/*yxl 2006-01-05 add this line*/	
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

int gi_StdKey[50] = {
		POWER_KEY,	//0
		MUTE_KEY,	// 1
		KEY_DIGIT1,	// 2
		KEY_DIGIT2,	// 3
		KEY_DIGIT3,	// 4
		KEY_DIGIT4,	// 5
		KEY_DIGIT5,	// 6
		KEY_DIGIT6,	// 7
		KEY_DIGIT7,	// 8
		KEY_DIGIT8,	// 9
		KEY_DIGIT9,	// 10
		KEY_DIGIT0,	// 11
		0,	// 12
		TV_RADIO_KEY,	// 13
		MENU_KEY,		// 14
		EPG_KEY,		// 15
		0,		// 16
		EXIT_KEY,		// 17
		AUDIO_KEY,		// 18
		UP_ARROW_KEY,	// 19
		DOWN_ARROW_KEY,	// 20
		LEFT_ARROW_KEY,		// 21
		RIGHT_ARROW_KEY,	// 22
		OK_KEY,				// 23
		VOLUME_UP_KEY,		// 24
		VOLUME_DOWN_KEY,	// 25
		PAGE_UP_KEY,		// 26
		PAGE_DOWN_KEY,		// 27
		RED_KEY,			// 28
		GREEN_KEY,			// 29
		YELLOW_KEY,			// 30
		BLUE_KEY,			// 31
		FAV_KEY,				// 32
		0,//SUBTITLE_KEY,	// 33
		0,			// 34
		0,			// 35
		RETURN_KEY,			// 36
		0,			// 37
		0,				// 38
		PROGM_SEARCH_KEY,			// 39 
		F1_KEY,				//40
		F2_KEY,				//41
		F3_KEY,				//42
		F4_KEY,				//43
	};

U32 YW_GetLogicKey ( U32 RawKey )
{
	switch ( RawKey / 100 )
	{
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		return 10/5;
	case 13:
	case 14:
	case 15:
	case 16:
		return 15/5;
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
		return 20/5;
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
		return 25/5;
	case 28:
	case 29:
	case 30:
	case 31:
	case 32:
		return 30/5;
	case 33:
	case 34:
	case 35:
	case 36:
		return 35/5;
	case 37:
	case 38:
	case 39:
	case 40:
	case 41:
	case 42:
		return 40/5;
	case 43:
	case 44:
	case 45:
	case 46:
		return 45/5;
	default:
		return RawKey;
	}
}

static U8 YW_StdMark[18] 		= { 1, 6, 5, 3, 3, 5, 5, 5, 5, 3, 5, 5, 5, 3, 5, 5, 5, 3 };
static U8 YW_StdSymbol[18]	= { 1, 7, 6, 4, 4, 6, 6, 6, 6, 4, 6, 6, 6, 4, 6, 6, 6, 4 };

extern U32 guTestSpeedTime;
/*
  * Func: JL_ParseKey
  * Desc: 针对吉林遥控器的解码程序
  * In:	采样值
  * Out:	-1		没有键值
  		其他	键值
  */
 #if 0
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

	int					i_YWCmpMark, i_YWCmpSymbol;
	int					i_YWMarkTmp[50], i_YWSymbolTmp[50];
	U32					u32_YWMark, u32_YWSymbol, u32_YWKey;

	SymbolCountTemp = *piSymbolCount;

	if ( 0 == SymbolCountTemp )
	{
		return -1;
	}

	if ( 28 == SymbolCountTemp )
	{
		memcpy((void*)SymbolBuffer,(const void*)pSymbolValue,sizeof(STBLAST_Symbol_t)*SymbolCountTemp);
		//do_report ( 0, "\nKeylen = %d", SymbolCountTemp );
		if ( !(1 == SymbolBuffer[0].MarkPeriod && 1 == SymbolBuffer[0].SymbolPeriod && 65535 == SymbolBuffer[27].SymbolPeriod) )
		{
			return -1;
		}

		SymbolBuffer[27].SymbolPeriod = 6;
		//do_report ( 0, "\n--MarkPeriod--\n{" );
		for ( i = 0; i < SymbolCountTemp; i ++ )
		{
			//do_report ( 0, "%d, ", YW_GetLogicKey (SymbolBuffer[i].MarkPeriod) );
			i_YWMarkTmp[i] = YW_GetLogicKey (SymbolBuffer[i].MarkPeriod);
		}
		//do_report ( 0, "}\n--end--\n" );

		//do_report ( 0, "\n--SymbolPeriod--\n{" );
		for ( i = 0; i < SymbolCountTemp; i ++ )
		{
			//do_report ( 0, "%d, ", YW_GetLogicKey ( SymbolBuffer[i].SymbolPeriod ) );
			i_YWSymbolTmp[i] = YW_GetLogicKey ( SymbolBuffer[i].SymbolPeriod );
		}
		//do_report ( 0, "}\n--end--\n" );

		#if 0
		i_YWCmpMark 	= memcmp ( i_YWMarkTmp, YW_StdMark, 18 );
		i_YWCmpSymbol	= memcmp ( i_YWSymbolTmp, YW_StdSymbol, 18 );
		#else
		i_YWCmpMark = 0;
		for ( i = 0; i < 18; i ++ )
		{
			if ( i_YWMarkTmp[i] != YW_StdMark[i] )
			{
				//do_report ( 0, "\nMark不同[%d], <%d, %d>", i, i_YWMarkTmp[i], YW_StdMark[i] );
				i_YWCmpMark = 1;
			}
		}

		i_YWCmpSymbol = 0;
		for ( i = 0; i < 18; i ++ )
		{
			if ( i_YWSymbolTmp[i] != YW_StdSymbol[i] )
			{
				//do_report ( 0, "\nSymbol不同[%d], <%d, %d>", i, i_YWSymbolTmp[i], YW_StdSymbol[i] );
				i_YWCmpSymbol = 1;
			}
		}
		#endif

		if ( i_YWCmpMark || i_YWCmpSymbol )
		{
			#if 0
			do_report ( 0, "\n--MarkPeriod--\n{" );
			for ( i = 0; i < SymbolCountTemp; i ++ )
			{
				do_report ( 0, "%d, ", i_YWMarkTmp[i] );
				//i_YWMarkTmp[i] = YW_GetLogicKey (SymbolBuffer[i].MarkPeriod);
			}
			do_report ( 0, "}\n--end--\n" );

			do_report ( 0, "\n--SymbolPeriod--\n{" );
			for ( i = 0; i < SymbolCountTemp; i ++ )
			{
				do_report ( 0, "%d, ", i_YWSymbolTmp[i] );
				//i_YWSymbolTmp[i] = YW_GetLogicKey ( SymbolBuffer[i].SymbolPeriod );
			}
			do_report ( 0, "}\n--end--\n" );
			#endif
			do_report ( 0, "\n用户码不对!<%d,%d>", i_YWCmpMark, i_YWCmpSymbol );
			return -1;
		}

		#if 0
		u32_YWMark 		= ( i_YWMarkTmp[18] << 28 ) | ( i_YWMarkTmp[19] << 24 ) | ( i_YWMarkTmp[20] << 20 ) | ( i_YWMarkTmp[21] << 16 ) |
						( i_YWMarkTmp[22] << 12 ) | ( i_YWMarkTmp[23] << 8 ) | ( i_YWMarkTmp[24] << 4 ) | ( i_YWMarkTmp[25] )	;
		u32_YWSymbol 	= ( i_YWSymbolTmp[18] << 28 ) | ( i_YWSymbolTmp[19] << 24 ) | ( i_YWSymbolTmp[20] << 20 ) | ( i_YWSymbolTmp[21] << 16 ) |
						( i_YWSymbolTmp[22] << 12 ) | ( i_YWSymbolTmp[23] << 8 ) | ( i_YWSymbolTmp[24] << 4 ) | ( i_YWSymbolTmp[25] );
		#else
		/* mark 中3 为0，5为1 */
		for ( i = 18; i <= 25; i ++ )
		{
			i_YWMarkTmp[i] = ( 3 == i_YWMarkTmp[i] ) ? 0 : 1;
		}

		/* Symbol中4为0，6为1 */		
		for ( i = 18; i <= 25; i ++ )
		{
			i_YWSymbolTmp[i] = ( 4 == i_YWSymbolTmp[i] ) ? 0 : 1;
		}

		u32_YWMark = 0;
		for ( i = 0; i < 8; i ++ )
		{
			u32_YWMark = ( u32_YWMark << 1) | i_YWMarkTmp[i+18];
		}
		u32_YWSymbol = 0;
		for ( i = 0; i < 8; i ++ )
		{
			u32_YWSymbol = ( u32_YWSymbol << 1) | i_YWSymbolTmp[i+18];
		}

		u32_YWKey = ( u32_YWMark << 8 ) | u32_YWSymbol;
		#endif

		//do_report ( 0, "\nywKey=0x%x", u32_YWKey );

		switch ( u32_YWKey )
		{
		case 0x5050:	/* POWER_KEY */
			i_KeyRead = POWER_KEY;
			break;
		case 0x8080:	/* 1 */
			i_KeyRead = KEY_DIGIT1;
			break;
		case 0x4040:	/* 2 */
			i_KeyRead = KEY_DIGIT2;
			break;
		case 0xc0c0:	/* 3 */
			i_KeyRead = KEY_DIGIT3;
			break;
		case 0x2020:	/* 4 */
			i_KeyRead = KEY_DIGIT4;
			break;
		case 0xa0a0:	/* 5 */
			i_KeyRead = KEY_DIGIT5;
			break;
		case 0x6060:	/* 6 */
			i_KeyRead = KEY_DIGIT6;
			break;
		case 0xe0e0:	/* 7 */
			i_KeyRead = KEY_DIGIT7;
			break;
		case 0x1010:	/* 8 */
			i_KeyRead = KEY_DIGIT8;
			break;
		case 0x9090:	/* 9 */
			i_KeyRead = KEY_DIGIT9;
			break;			
		case 0x0:	/* 0 */
			i_KeyRead = KEY_DIGIT0;
			break;			
		case 0x7070:	/* FAV_KEY */
			i_KeyRead = FAV_KEY;
			break;			
		case 0x6868:	/* RETURN_KEY */
			i_KeyRead = RETURN_KEY;
			break;			
		case 0x1818:	/* MENU_KEY */
			i_KeyRead = MENU_KEY;
			break;			
		case 0xe8e8:	/* EXIT_KEY */
			i_KeyRead = EXIT_KEY;
			break;			
		case 0x0808:	/* UP_ARROW_KEY */
			i_KeyRead = UP_ARROW_KEY;
			break;			
		case 0x8888:	/* DOWN_ARROW_KEY */
			i_KeyRead = DOWN_ARROW_KEY;
			break;			
		case 0x4848:	/* LEFT_ARROW_KEY */
			i_KeyRead = LEFT_ARROW_KEY;
			break;			
		case 0xc8c8:	/* RIGHT_ARROW_KEY */
			i_KeyRead = RIGHT_ARROW_KEY;
			break;			
		case 0xf0f0:	/* OK_KEY */
			i_KeyRead = OK_KEY;
			break;			
		case 0x2828:	/* EPG_KEY */
			i_KeyRead = EPG_KEY;
			break;			
		case 0xa8a8:	/* HELP_KEY */	/* 注意添加此处*/
			i_KeyRead = -1;//gi_StdKey[15];
			break;			
		case 0x9898:	/* PAGE_UP_KEY */
			i_KeyRead = PAGE_UP_KEY;
			break;			
		case 0x5858:	/* PAGE_DOWN_KEY */
			i_KeyRead = PAGE_DOWN_KEY;
			break;			
		case 0x0404:	/* RED_KEY */
			i_KeyRead = RED_KEY;
			break;			
		case 0x8484:	/* YELLOW_KEY */
			i_KeyRead = YELLOW_KEY;
			break;			
		case 0x4444:	/* BLUE_KEY */
			i_KeyRead = BLUE_KEY;
			break;			
		case 0xc4c4:	/* GREEN_KEY */
			i_KeyRead = GREEN_KEY;
			break;			
		case 0xd8d8:	/* VOLUME_UP_KEY */
			i_KeyRead = VOLUME_UP_KEY;
			break;			
		case 0xb0b0:	/* RADIO_KEY */
			i_KeyRead = RADIO_KEY;
			break;			
		case 0x3030:	/* HTML_KEY */	// 注意此处需要添加
			i_KeyRead = -1;//gi_StdKey[13];
			break;
		case 0x7878:	/* AUDIO_KEY */
			i_KeyRead = AUDIO_KEY;
			break;
		case 0x3838:	/* VOLUME_DOWN_KEY */
			i_KeyRead = VOLUME_DOWN_KEY;
			break;		
		case 0xd0d0:	/* NVOD_KEY */	// 注意此处需要添加
			i_KeyRead = -1;//gi_StdKey[24];
			break;		
		case 0xf8f8:	/* MAIL_KEY */	// 注意此处需要添加
			i_KeyRead = -1;//gi_StdKey[24];
			break;
		case 0xb8b8:	/* MUTE_KEY */	// 注意此处需要添加
			i_KeyRead = MUTE_KEY;
			break;
		}

		//do_report ( 0, "\nprev=%x, curr=%x", si_PrevKey, i_KeyRead );
		if ( si_PrevKey == i_KeyRead )
		{
			if ( !( VOLUME_UP_KEY == i_KeyRead ||VOLUME_DOWN_KEY == i_KeyRead || UP_ARROW_KEY == i_KeyRead || 
				DOWN_ARROW_KEY == i_KeyRead || LEFT_ARROW_KEY == i_KeyRead || RIGHT_ARROW_KEY == i_KeyRead ) )
			{
				if ( time_after ( time_now (), t_TimeWait2 ) == 0 )
				{
					do_report ( 0, "打回1" );
					return -1;
				}
			}
			else 
			{
				if ( time_after ( time_now(), t_TimeWait1 ) == 0 )	/* 1/4秒后的连续键示为无效 */
				{
					do_report ( 0, "打回2" );
					return -1;
				}
			}
		}

		t_TimeWait1 = time_plus ( time_now(), ST_GetClocksPerSecondLow () / 4 );
		t_TimeWait2 = time_plus ( time_now(), ST_GetClocksPerSecondLow () * 2 );

		si_PrevKey = i_KeyRead;

		do_report ( 0, "\nkey=%x", i_KeyRead );
		return i_KeyRead;
	}

	#if 1
	/* 处理连续键 */
	if ( 3 == SymbolCountTemp )
	{
		if ( -1 == si_PrevKey )
			return -1;
	
		t_TimeNow = time_now();

		if ( !( UP_ARROW_KEY == si_PrevKey || DOWN_ARROW_KEY == si_PrevKey || 
					LEFT_ARROW_KEY == si_PrevKey || RIGHT_ARROW_KEY == si_PrevKey||
					VOLUME_UP_KEY == si_PrevKey || VOLUME_DOWN_KEY == si_PrevKey
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
		t_TimeWait1 = time_plus ( time_now(), ST_GetClocksPerSecondLow () / 32 );

		t_TimeWait2 = time_plus ( time_now(), ST_GetClocksPerSecondLow () * 2 );

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

#if 0
	do_report ( 0, "\nReadKey=0x%x", u_KeyValue );
#endif

	switch ( u_KeyValue )
	{
	case 0xf6916f:	/* 0. 待机键 */
		i_KeyRead = gi_StdKey[0];
		break;
	case 0xbe11ef:	/* 1. 静音键 */
		i_KeyRead = gi_StdKey[1];
		break;
	case 0xf700ff:	/* 2. 数字1 */
	case 0xbf00ff:
		i_KeyRead = gi_StdKey[2];
		break;
	case 0xf6817f:	/* 3. 数字2 */
	case 0xbe817f:
		i_KeyRead = gi_StdKey[3];
		break;
	case 0xf7807f:	/* 4. 数字3 */
	case 0xbf807f:
		i_KeyRead = gi_StdKey[4];
		break;
	case 0xf641bf:	/* 5. 数字4 */
	case 0xbe41bf:
		i_KeyRead = gi_StdKey[5];
		break;
	case 0xf740bf:	/* 6. 数字5 */
	case 0xbf40bf:
		i_KeyRead = gi_StdKey[6];
		break;
	case 0xf6c13f:	/* 7. 数字6 */
	case 0xbec13f:
		i_KeyRead = gi_StdKey[7];
		break;
	case 0xf7c03f:	/* 8. 数字7 */
	case 0xbfc03f:
		i_KeyRead = gi_StdKey[8];
		break;
	case 0xf621df:	/* 9. 数字8 */
	case 0xbe21df:
		i_KeyRead = gi_StdKey[9];
		break;
	case 0xf720df:	/* 10. 数字9 */
	case 0xbf20df:
		i_KeyRead = gi_StdKey[10];
		break;
	case 0xf601ff:	/* 11. 数字0 */
	case 0xbe01ff:
		i_KeyRead = gi_StdKey[11];
		break;
	case 0xf651af:	/* 12. SATE_KEY */
		i_KeyRead = gi_StdKey[12];
		break;
	case 0xf724db:	/* 13. TV_RADIO_KEY */
	case 0xbf24db:
		i_KeyRead = gi_StdKey[13];
		break;
	case 0xf6d12f:	/* 14. MENU_KEY */
	case 0xbed12f:
		i_KeyRead = gi_StdKey[14];
		break;
	case 0xf704fb:	/* 15. EPG_KEY */
	case 0xbf04fb:
		i_KeyRead = gi_StdKey[15];
		break;
	case 0xf605fb:	/* 16. INFO_KEY */
	case 0xbe05fb:
		i_KeyRead = gi_StdKey[16];
		break;
	case 0xf750af:	/* 17. EXIT_KEY */
	case 0xbf50af: 
		i_KeyRead = gi_StdKey[17];
		break;
	case 0xf734cb:	/* 18. AUDIO_KEY */
	case 0xbf34cb:
		i_KeyRead = gi_StdKey[18];
		break;
	case 0xf7b04f:	/* 19. 上 */
	case 0xbfb04f:
		i_KeyRead = gi_StdKey[19];
		break;
	case 0xf7f00f:	/* 20. 下 */
	case 0xbff00f:
		i_KeyRead = gi_StdKey[20];
		break;
	case 0xf6f10f:	/* 21. 左 */
	case 0xbef10f:
		i_KeyRead = gi_StdKey[21];
		break;
	case 0xf6b14f:	/* 22. 右 */
	case 0xbeb14f:
		i_KeyRead = gi_StdKey[22];
		break;
	case 0xf7b44b:	/* 23. 确定 */
	case 0xbfb44b:
		i_KeyRead = gi_StdKey[23];
		break;
	case 0xf6718f:	/* 24. VOLUME_UP_KEY */
	case 0xf7906f:
		i_KeyRead = gi_StdKey[24];
		break;
	case 0xf7708f:	/* 25. VOLUME_DOWN_KEY */
	case 0xf631cf:
		i_KeyRead = gi_StdKey[25];
		break;
	case 0xf6a55b:		
	case 0xbea55b:
		i_KeyRead = gi_StdKey[26];
		break;
	case 0xf730cf:	/* 27. PAGE_DOWN_KEY */
	case 0xf7a45b:	/* 26. PAGE_UP_KEY */		
	case 0xbfa45b:
		i_KeyRead = gi_StdKey[27];
		break;
	case 0xf6c53b:	/* 28. RED_KEY */
	case 0xbec53b:
		i_KeyRead = gi_StdKey[28];
		break;
	case 0xf6857b:	/* 29. GREEN_KEY */
	case 0xbe857b:
		i_KeyRead = gi_StdKey[29];
		break;
	case 0xf6d52b:	/* 30. YELLOW_KEY */
	case 0xbed52b:
		i_KeyRead = gi_StdKey[30];
		break;
	case 0xf6b54b:	/* 31. BLUE_KEY */
	case 0xbeb54b:
		i_KeyRead = gi_StdKey[31];
		break;
	case 0xf7a05f:	/* 32. FAV_KEY */
		i_KeyRead = gi_StdKey[32];
		break;
	case 0xf754ab:	/* 33. SUBTITLE_KEY */
		i_KeyRead = gi_StdKey[33];
		break;
	case 0xf744bb:	/* 34. TIMER_KEY */
		i_KeyRead = gi_StdKey[34];
		break;
	case 0xf615eb:	/* 35. ZOOM_KEY */
		i_KeyRead = gi_StdKey[35];
		break;
	case 0xf714eb:	/* 36. RETURN_KEY */
		i_KeyRead = gi_StdKey[36];
		break;
	case 0xf6956b:	/* 37. PAUSE_KEY */
		i_KeyRead = gi_StdKey[37];
		break;
	case 0xf7946b:	/* 38. TTX_KEY */
		i_KeyRead = gi_StdKey[38];
		break;
		/* jqz070713 add */
	case 0xbf54ab:	/* 39.搜索键 */
		i_KeyRead = gi_StdKey[39];
		break;
		/* end jqz070713 add */
	case 0xf6758b:	/* 40. F1_KEY */
	case 0xbe15eb:
		i_KeyRead = gi_StdKey[40];
		break;
	case 0xf7748b:	/* 40. F2_KEY */
		i_KeyRead = gi_StdKey[41];
		break;
	case 0xf6f50b:	/* 40. F3_KEY */
		i_KeyRead = gi_StdKey[42];
		break;
	case 0xf7f40b:	/* 40. F4_KEY */
		i_KeyRead = gi_StdKey[43];
		break;	
	default:
		i_KeyRead = -1;
		break;
	}

	if ( -1 != i_KeyRead )
	{
		si_PrevKey = i_KeyRead;

		/* 左右键要求慢一点 */
		t_TimeWait1 = time_plus ( time_now(), ST_GetClocksPerSecondLow ()  / 32 );
		t_TimeWait2 = time_plus ( time_now(), ST_GetClocksPerSecondLow ()  );

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

