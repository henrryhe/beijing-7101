/*
	(c) Copyright changhong
  Name:RC6Decode.c
  Description:the soft decode program of RC6 remote code
  Authors:yxl
  Date          Remark
  2002-10-17    Created
  2005-01-22    modifyed 
*/

#include "stblast.h"
#include "sttbx.h"

#define T 444
#define V1TMin 0.6*T
#define V1TMax 1.4*T

#define V2TMin 1.6*T
#define V2TMax 2.4*T

#define V3TMin 2.6*T
#define V3TMax 3.4*T

#define V4TMin 3.6*T
#define V4TMax 4.4*T

#define V5TMin 4.6*T
#define V5TMax 5.4*T

#define V6TMin 5.6*T
#define V6TMax 6.4*T

#define DECODE_SUCCESS 0
#define WRONG_LEVEL 1,
#define DISTURB_SIGNAL 2

 /*#define RC6_DECODE_DEBUG yxl 2005-01-22 add this macro*/



BOOL CH_InvertedInputCompensate(STBLAST_Symbol_t *Buf,
                             const U32 ExpectedStartSymbolMark,
                             const U32 SymbolsAvailable)
{
    BOOL Okay = TRUE;
    int Index;

    /* first symbol, copy from user value */
    Index=0;
    Buf[Index].MarkPeriod   = ExpectedStartSymbolMark;
    Buf[Index].SymbolPeriod = Buf[Index].MarkPeriod + Buf[Index+1].MarkPeriod;
    Index++;
  
    while(Index < SymbolsAvailable)
    {
        Buf[Index].MarkPeriod   = Buf[Index].SymbolPeriod - Buf[Index].MarkPeriod;
        Buf[Index].SymbolPeriod = Buf[Index].MarkPeriod + Buf[Index+1].MarkPeriod;
        Index++;
    }
    return Okay;
}
void CH_InvertSymbol(STBLAST_Symbol_t *SymbolBuf_p,U32 *SymbolsAvailable)
{
       
        #define MID_6T   (U16)2666.664
	U32 SymbolsCount;
        STBLAST_Symbol_t *LastSymbol_p;
	SymbolsCount=*SymbolsAvailable;
        if (CH_InvertedInputCompensate(SymbolBuf_p,
                                (MID_6T),
                                SymbolsCount) == FALSE)
        {
       
            return ;
        }
        *SymbolsAvailable = SymbolsCount-1;
        LastSymbol_p = SymbolBuf_p + (SymbolsCount-1);
        LastSymbol_p->SymbolPeriod = 0xFFFF;
	do_report(0,"INvert success\n");
}


int RC6_Decode(U32* piSymbolCount,U32* pKeyDataNum,U32* pRC6Mode,void* pSymbolValue,char* KeyData)
{

	STBLAST_Symbol_t SymbolBuffer[100];
	char cWave[150];
	char cKeyValue[20];/*char cKeyValue[50];*/
	char cDataByte[30];
	U16 i=0;
	U16 j=0;
	int iTempValue;
	/*char cTempLevel;
	char cTempValue;*/
	char cTempValue;
	char cTempLevel;
	int iCount=0;
	/*	U8 h=0;U8 Mode=0; U16 h=0;U16 Mode=0;*/
	U32 h=0;U32 Mode=0;
  
	U8 tc=8;
	
	char cTemp;
	U32 iSymbolCount;
	iSymbolCount=*piSymbolCount;
	
	/*strncpy zxg 20060721 fix this bug*/memcpy(SymbolBuffer,(const char*)pSymbolValue,sizeof(STBLAST_Symbol_t)*iSymbolCount);

#ifdef RC6_DECODE_DEBUG
	do_report(0,"\n YxlInfo():SymbolCount=%d \n",iSymbolCount);
	for(i=0;i<iSymbolCount;i++)
	{
		do_report(0,"\ni=%d H=%d L=%d T=%d",i,SymbolBuffer[i].MarkPeriod,
			SymbolBuffer[i].SymbolPeriod-SymbolBuffer[i].MarkPeriod,
			SymbolBuffer[i].SymbolPeriod);
		do_report(0,"\n");
	}	
#endif

    for(i=0;i<30;i++) cDataByte[i]=0x00;	
	for(i=0;i<iSymbolCount;i++)
	{
		for(j=0;j<2;j++)
		{

			if(j==0) 
			{
				iTempValue=SymbolBuffer[i].MarkPeriod;
				cTempLevel='H';
			}
            else 
			{
				iTempValue=SymbolBuffer[i].SymbolPeriod-SymbolBuffer[i].MarkPeriod;
				cTempLevel='L';
			}
			if(iTempValue>V1TMin&&iTempValue<V1TMax)
			{
				cWave[iCount]=0x1;
				iCount++;
				cWave[iCount]=cTempLevel;
				iCount++;
				continue;
			}

			if(iTempValue>V2TMin&&iTempValue<V2TMax)
			{
				cWave[iCount]=0x2;/*cWave[iCount]='1'cWave[iCount]='2';*/
				iCount++;
				cWave[iCount]=cTempLevel;
				iCount++;
				continue;
			}
			
			if(iTempValue>V3TMin&&iTempValue<V3TMax)
			{
				cWave[iCount]=0x3;/*cWave[iCount]='3';*/
				iCount++;
				cWave[iCount]=cTempLevel;
				iCount++;
				continue;
			}
			if(iTempValue>V4TMin&&iTempValue<V4TMax)
			{
				cWave[iCount]=0x4;/*cWave[iCount]='4';*/
				iCount++;
				cWave[iCount]=cTempLevel;
				iCount++;
				continue;
			}

			if(iTempValue>V5TMin&&iTempValue<V5TMax)
			{
				cWave[iCount]=0x5;/*cWave[iCount]='5';*/
				iCount++;
				cWave[iCount]=cTempLevel;
				iCount++;
				continue;
			}

			if(iTempValue>V6TMin&&iTempValue<V6TMax)
			{
				cWave[iCount]=0x6;/*cWave[iCount]='6';*/
				iCount++;
				cWave[iCount]=cTempLevel;
				iCount++;
				continue;
			}

			if(iTempValue<V1TMin) 
			{/*65535-2.4*T*/
				/*STTBX_Print(("\n266 status:i=%d j=%d %d count=%d \n",i,j,iTempValue,iSymbolCount));*/
				return DISTURB_SIGNAL;/**/
			}
			
			if(iTempValue<64470) 
			{/*65535-2.4*T*/
				/*STTBX_Print(("\n64470 status:i=%d j=%d %d count=%d \n",i,j,iTempValue,iSymbolCount));*/
				return 1;
			}
			else 
			{
				cWave[iCount]='E';/*end sign*/
				cWave[iCount+1]='\0';
			    	}
			}
		}
   /* STTBX_Print(("Wave picture is:%d %s\n",iCount,cWave));*/
	j=0;
	for(i=0;i<iCount;i++) 
	{
		cTempValue=cWave[i];
		cTempLevel=cWave[i+1];
		/*STTBX_Print((" loop:total=%d i=%d value=%c level=%c\n",iCount,i,cTempValue,cTempLevel));*/
		switch(cTempValue)
		{
		case 6:
			if(cTempLevel=='H') 
			{
				cTempValue=cWave[i+2];
				if(cTempValue==2) 
				{
					cKeyValue[j]='L';
					i+=3;
					j++;
					continue;
				}
				if(cTempValue<2) 
				{
					return 1;/*error*/
				}
				if(cTempValue>2)
				{
					cWave[i+2]-=2;
					cKeyValue[j]='L';
					j++;
					i+=1;
					continue;
				}
			}
			STTBX_Print((" 6:status:i=%d tempValue=%c TempLevel=%c  %s\n",i,cTempValue,cTempLevel,cKeyValue));
	
			return 1;
		case 1:
			if(cTempLevel=='H')
			{
				cTempValue=cWave[i+2];
				if(cTempValue=='E') 
				{
					/*cKeyValue[j]=1;*/
					cTemp=0x1;
					i+=3;
					j++;
					break;/*continue;*/
				}
				if(cTempValue==1) 
				{
					/*cKeyValue[j]=1;*/
					cTemp=0x1;
					i+=3;
					j++;
					break;/*continue;*/
				}
				if(cTempValue>1) {
					/*cKeyValue[j]=1;*/
					cTemp=0x1;
					cWave[i+2]-=1;
					i+=1;
					j++;
					

					break;/*continue;*/
				}
			}
			else
			{
				cTempValue=cWave[i+2];
				if(cTempValue==1) 
				{
					/*cKeyValue[j]=0;cKeyValue[j]='0';*/
					cTemp=0x0;
					i+=3;
					j++;
					break;/*continue;*/
				}
				if(cTempValue>1) 
				{
					/*cKeyValue[j]=0;cKeyValue[j]='0';*/
					cTemp=0x0;
					cWave[i+2]-=1;
					i+=1;
					j++;
					
				break;/*continue;*/
				}
			}
			break;
		case 2:
			if(cTempLevel=='H')
			{
				cTempValue=cWave[i+2];
				if(cTempValue==2) 
				{
					cKeyValue[j]='T';
					j++;
					cKeyValue[j]='1';
					j++;
					i+=3;
					continue;
				}
				if(cTempValue>2) 
				{
					cKeyValue[j]='T';
					j++;
					cKeyValue[j]='1';
					cWave[i+2]-=2;
					j++;
					i+=1;
					
					continue;
				}
				if(cTempValue<2) 
				{
					/*STTBX_Print((" 2H:status:i=%d tempValue=%c TempLevel=%c %s\n",i,cTempValue,cTempLevel,cKeyValue));*/
	
					return 1;
				}
			}
			else
			{
				cTempValue=cWave[i+2];
				if(cTempValue==2) 
				{
					cKeyValue[j]='T';
					j++;
					cKeyValue[j]='0';
					j++;
					i+=3;
					continue;
				}
				if(cTempValue>2) 
				{
					cKeyValue[j]='T';
					j++;
					cKeyValue[j]='0';
					cWave[i+2]-=2;
					j++;
					i+=1;
					
					continue;
				}
				if(cTempValue<2) 
				{
					/*STTBX_Print((" 2l:status:i=%d tempValue=%c TempLevel=%c  %s\n",i,cTempValue,cTempLevel,cKeyValue));*/
	
					return 1;
				}
			}
			break;
		default:
/*			STTBX_Print((" default:status:i=%d tempValue=%c TempLevel=%c  %s\n",i,cTempValue,cTempLevel,cKeyValue));*/
	
			return 1;
			}

			if(j>7)
			{
				
				tc--;
				cTemp=cTemp<<tc;
				cDataByte[h]=cDataByte[h]|cTemp;
				/*STTBX_Print(("j=%d tc=%d  ctemp=%x  cDataByte[%d]=%x\n ",
					j,tc,cTemp,h,cDataByte[h]));*/
				if(tc==0) 
				{
					h++;
					tc=8;
				}
			}
			else
			{
				cKeyValue[j-1]=cTemp;
			}
			
	}
	
	/*STTBX_Print(("Wave Value is:%d %s  h=%d %x %x %x %x  tc=%d j=%d\n",j,cKeyValue,h,
		cDataByte[0],cDataByte[1],cDataByte[2],cDataByte[3],tc,j));*/
	cKeyValue[j]='\0';
    /**pMode=22;*/
	*pKeyDataNum=h;
	
    if(tc!=8) return 1;/* data not */
	
	if(j<7) return 1;

	/*for(i=0;i<7;i++) {
		STTBX_Print((" i=%d %c ",i,cKeyValue[i]));
	}*/

	
    if(cKeyValue[0]!='L'||cKeyValue[1]!=0x1||cKeyValue[5]!='T')
	{
	
		return 1;
	}
	else 
	{
		if(cKeyValue[6]!='0'&&cKeyValue[6]!='1') 
		{
			
			return 1;
		}
	}
	
	
	for(i=0;i<3;i++) 
	{
		cTemp=cKeyValue[i+2];		
		cTemp=cTemp<<(2-i);
		Mode=Mode|cTemp;
	}
	/*STTBX_Print(("Mode=%d\n",Mode));*/
	*pRC6Mode=Mode;
	/*STTBX_Print(("Mode=%d %d \n",Mode,*pRC6Mode));*/
	    
	strncpy(KeyData,(const char*)cDataByte,h);
	

#if 0  /*yxl 2004-07-12 add this section for key check*/
	
	STTBX_Print(("\nYxl Info():KeyData=%x %x %x %x",KeyData[0],KeyData[1],KeyData[2],KeyData[3]));
#endif/*end yxl 2004-07-12 add this section for */
	
	/*yxl 2004-05-26 add for customer code check,for RC6 Mode A:Costomer is 0x80,0x10*/
	if(KeyData[0]==/*zxg 20070427 add强制类型转换*/(char)0x80&&KeyData[1]==0x10)
	{
		return DECODE_SUCCESS;		
	}
	else return 6;/*6 stand for customer code isn't right*/

}

