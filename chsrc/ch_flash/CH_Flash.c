/*
  (c) Copyright changhong
  Name:CH_Flash.c
  Description:flash initialize section for changhong STI7100 platform
 
  Authors:yixiaoli
  Date          Remark
  2006-11-10    Created

  Modifiction:
Date:2007-03-07 
1.增加ST M29W640GST falsh 初始化
 
Date:2007-04-05
1.Add new function BOOL CHFLASH_Setup(void);
20070716 zxg 增加SPANISON FLASH 写保护操作
*/


/*include file*/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>   
#include "stddefs.h"
#include "stlite.h"
#include "stflash.h"
#include "..\main\initterm.h"
#include "lld.h"
#include "CH_FlashMid.h"

#if 0
#define YXL_FLASH_DOWNPIC_TEST
#endif

#if 0
#define YXL_WRITE_PRGFILE
#endif


void YxlA_FlashTest(void);
BOOL CHFLASH_Setup(void);

void YxlAA_FlashTest(void);





/*yxl 2007-03-07 add below section*/




/*yxl 2007-04-05 add below functions*/
BOOL CHFLASH_Setup(void)
{
	BOOL res;
	CHFLASH_InitParams_t InitParams;
	
	InitParams.BaseAddress=0xa0000000;
	InitParams.DriverPartition=SystemPartition;
	
	res=CHFLASH_Init(&InitParams,&FLASHHandle);
	
	
	/*YxlAA_FlashTest();*/
	return res;


}
/*end yxl 2007-04-05 add below functions*/


#if 0

U8* VIDD_LoadFileToMemory(U8 *FileName,U32* pFileSize)
{
	void                         *FILE_Handle;
	U32                           FILE_Size;
	U32                           SizeTemp;
	U8                           *FILE_Buffer;
	U8                           *FILE_Buffer_Aligned;
	U32                           FILE_BufferSize;
	STEVT_OpenParams_t            EVT_OpenParams;
	STEVT_DeviceSubscribeParams_t EVT_SubcribeParams; 
	ST_ErrorCode_t                ErrCode=ST_NO_ERROR;
	STVID_Freeze_t                VID_Freeze;
	
	*pFileSize=0;

	FILE_Handle=SYS_FOpen((char *)FileName,"rb");
	if (FILE_Handle==NULL) 
	{
		STTBX_Print(("YxlInfo:Can't open file %s\n",FileName));
		return NULL;
	}
	
	SYS_FSeek(FILE_Handle,0,SEEK_END);
	FILE_Size=SYS_FTell(FILE_Handle);
	SizeTemp=FILE_Size/65536;
	if((FILE_Size%65536)!=0) SizeTemp+=1;

	SYS_FSeek(FILE_Handle, 0, SEEK_SET );


	FILE_Buffer         = memory_allocate(SystemPartition,SizeTemp*65536);
	if (FILE_Buffer==NULL)
	{
		STTBX_Print(("YxlInfo:Can't allocate memory to load the file !!!\n"));
		SYS_FClose(FILE_Handle);

		return NULL;
	} 

	memset((void*)FILE_Buffer,0xff,SizeTemp*65536);

	SYS_FRead(FILE_Buffer,FILE_Size,1,FILE_Handle);
	SYS_FClose(FILE_Handle);
	FILE_Handle=0;
	*pFileSize=SizeTemp;/*FILE_Size;*/

	return FILE_Buffer;
}




void YxlAA_FlashTest(void)
{
#ifdef YXL_FLASH_DOWNPIC_TEST 
	BOOL res;

	int i;
	U8* pBufTemp=NULL;
	U32 FSizeTemp=0;
	int resres;
#if 0
	U8 Buf[20];
	U8 Buf1[20];
#else
	U8 Buf[65536];
	U8 Buf1[65536];
#endif
	U32 Num=0;
	#ifndef DU16
#pragma ST_device(DU16)
typedef volatile unsigned short DU16;
#endif
		DU16* AdrTemp=0xA0000000;
#ifdef YXL_WRITE_PRGFILE
	int Offset=0x0;
#else
#if 1
	int Offset=0x440000;/*0x40000,this is ok;pic*/
#else
	int Offset=0x560000;/*0x40000,this is ok;pic*/
#endif

#endif

	memset((void*)Buf,0x88,65536);
	memset((void*)Buf1,0x99,20);


#ifndef YXL_WRITE_PRGFILE
	pBufTemp=VIDD_LoadFileToMemory((U8*)"ballpic.bin",&FSizeTemp);
#else
	pBufTemp=VIDD_LoadFileToMemory((U8*)"flash.bin",&FSizeTemp);
#endif

	if(pBufTemp==NULL)
	{
		STTBX_Print(("\nYxlInfo:VID_LoadFileToMemory() isn't successful"));
		return ;
	}

	STTBX_Print(("\n YxlInfo:Value=%x %x %x %x \n",*(pBufTemp),
		*(pBufTemp+1),*(pBufTemp+2),*(pBufTemp+3)));


	for(i=0;i<FSizeTemp;i++)
	{
		res=CHFLASH_Erase(FLASHHandle,i*65536+Offset,65536/*0x8000,spasion ok*/);
		if(res==TRUE )
		{
			STTBX_Print(("\nCHFLASH_Erase() is wrong,Offset=%d\n",i*65536+Offset));	
			return ;	
		}	
		else STTBX_Print(("CHFLASH_Erase() is ok,Offset=%lx\n",i*65536+Offset));	

#if 0 /*yxl 2007-04-23 ,below is spasion*/
		res=CHFLASH_Erase(FLASHHandle,i*65536+Offset+32768,0x8000);
		if(res==TRUE )
		{
			STTBX_Print(("\nCHFLASH_Erase() is wrong,Offset=%d\n",i*65536+Offset));	
			return ;	
		}	
		else STTBX_Print(("CHFLASH_Erase() is ok,Offset=%d\n",i*65536+Offset));	
#endif 

		res=CHFLASH_Read(FLASHHandle,i*65536+Offset,Buf1,20,&Num);
		if(res==TRUE )
		{
			STTBX_Print(("\nCHFLASH_Read() is wrong+Offset\n"));	
			return ;	
		}	
		else STTBX_Print(("CHFLASH_Read() is ok\n"));
		
#if 0
		res=CHFLASH_Write(FLASHHandle,i*65536+Offset,(U8*)(pBufTemp+i*65536),65536,&Num);
		if(res==TRUE )
		{
			STTBX_Print(("\nCHFLASH_Write() is wrong,Offset=%d\n",i*65536+Offset));	
		/*	return TRUE;*/	
		}	
		else STTBX_Print(("CHFLASH_Write () is ok,Offset=%d\n",i*65536+Offset));	
#else

		memcpy((void*)Buf,(const void*)(pBufTemp+i*65536),65536);/**/

		/*yxl 2007-04-24 add below section*/
		if((i*65536+Offset)==0x4c0000)
		{

			STTBX_Print(("\nyxlTest() is ,Num=%d V1=%x %x %x %x V2=%x %x %x %x V3=%x",Num,
			Buf[0x05d70+0],Buf[0x05d70+1],Buf[0x05d70+2],Buf[0x05d70+3],
			*(pBufTemp+0x085d70+0),*(pBufTemp+0x085d70+1),*(pBufTemp+0x085d70+2),
			*(pBufTemp+0x085d70+3)));			
		}

		/*end yxl 2007-04-24 add below section*/

		res=CHFLASH_Write(FLASHHandle,i*65536+Offset,Buf,65536/*/2*/,&Num);
		if(res==TRUE )
		{
			STTBX_Print(("\nCHFLASH_Write() is wrong,Offset=%d\n",i*65536+Offset));	
		/*	return TRUE;*/	
		}	
		else STTBX_Print(("CHFLASH_Write () is ok,Offset=%lx Num=%d\n",i*65536+Offset,Num));	

		res=CHFLASH_Read(FLASHHandle,i*65536+Offset,Buf1,65536,&Num);
		if(res==TRUE )
		{
			STTBX_Print(("\nCHFLASH_Read() is wrong"));	
			return TRUE;	
		}	
		else 
		{
			STTBX_Print(("CHFLASH_Read() is ok,Num=%d V1=%x %x %x %x V2=%x %x %x %x V3=%x",Num,
			Buf1[0],Buf1[1],Buf1[2],Buf1[3],
			*(pBufTemp+i*65536+0),*(pBufTemp+i*65536+1),*(pBufTemp+i*65536+2),
			*(pBufTemp+i*65536+3),*(AdrTemp+i*65536+Offset)));	
		}

				/*yxl 2007-04-24 add below section*/
		if((i*65536+Offset)==0x4c0000)
		{

			STTBX_Print(("\nyxlTest() is ,Num=%d V1=%x %x %x %x V2=%x %x %x %x V3=%x",Num,
			Buf1[0x05d70+0],Buf1[0x05d70+1],Buf1[0x05d70+2],Buf1[0x05d70+3],
			*(pBufTemp+0x085d70+0),*(pBufTemp+0x085d70+1),*(pBufTemp+0x085d70+2),
			*(pBufTemp+0x085d70+3)));			
		}

		/*end yxl 2007-04-24 add below section*/

		
		resres=memcmp((const void*)(pBufTemp+i*65536),(const void*)Buf1,65536);

		STTBX_Print(("\nres %d",resres));


#endif
	}

	/*yxl 2007-04-24 add below section*/

	for(i=0;i<FSizeTemp;i++)
	{
		res=CHFLASH_Read(FLASHHandle,i*65536+Offset,Buf1,65536,&Num);
		if(res==TRUE )
		{
			STTBX_Print(("\nCHFLASH_Read() is wrong"));	
			return TRUE;	
		}	
		else 
		{
			STTBX_Print(("\nCHFLASH_Read() is ok,Offset=%lx Num=%d V1=%x %x %x %x V2=%x %x %x %x V3=%x",
			i*65536+Offset,	Num,
			Buf1[0],Buf1[1],Buf1[2],Buf1[3],
			*(pBufTemp+i*65536+0),*(pBufTemp+i*65536+1),*(pBufTemp+i*65536+2),
			*(pBufTemp+i*65536+3),*(AdrTemp+i*65536+Offset)));	
		}
		
		resres=memcmp((const void*)(pBufTemp+i*65536),(const void*)Buf1,65536);

		STTBX_Print((" res %d",resres));
	}

	/*end yxl 2007-04-24 add below section*/

	{
		U8 Temp;
	
		res=CHFLASH_Read(FLASHHandle,/*0x4c2eb8*/0x4C5d70,Buf1,6,&Num);
		if(res==TRUE )
		{
			STTBX_Print(("\nCHFLASH_Read() is wrong"));	
			return TRUE;	
		}	
		else 
		{
#if 1
			STTBX_Print(("\nCHFLASH_Read() is ok,Num=%d V1=%x %x %x %x V2=%x %x %x %x V3=%x",Num,
			Buf1[0],Buf1[1],Buf1[2],Buf1[3],
			*(pBufTemp+0x085d70+0),*(pBufTemp+0x085d70+1),*(pBufTemp+0x085d70+2),
			*(pBufTemp+0x085d70+3)));	
#else
			STTBX_Print(("\nCHFLASH_Read() is ok,Num=%d V1=%x %x %x %x V2=%x %x %x %x \n",Num,
			Buf1[0],Buf1[1],Buf1[2],Buf1[3]));	
#endif
		}

		res=CHFLASH_Read(FLASHHandle,0x4C0000,Buf1,65536,&Num);
		if(res==TRUE )
		{
			STTBX_Print(("\nCHFLASH_Read() is wrong"));	
			return TRUE;	
		}	
		else 
		{
#if 1
			STTBX_Print(("\nCHFLASH_Read() is ok,Num=%d V1=%x %x %x %x V2=%x %x %x %x V3=%x",Num,
			Buf1[0x05d70+0],Buf1[0x05d70+1],Buf1[0x05d70+2],Buf1[0x05d70+3],
			*(pBufTemp+0x085d70+0),*(pBufTemp+0x085d70+1),*(pBufTemp+0x085d70+2),
			*(pBufTemp+0x085d70+3)));	
#else
			STTBX_Print(("\nCHFLASH_Read() is ok,Num=%d V1=%x %x %x %x V2=%x %x %x %x \n",Num,
			Buf1[0],Buf1[1],Buf1[2],Buf1[3]));	
#endif
		}

		Temp=*(AdrTemp+0x485d70);/**/
		AdrTemp+=0x485d70;
		Temp=*(AdrTemp);/**/
		STTBX_Print(("\nres %d Temp=%x Adr=%lx",resres,Temp,AdrTemp));
		AdrTemp+=1;
		Temp=*(AdrTemp);/**/
		STTBX_Print(("\nres %d Temp=%x Adr=%lx",resres,Temp,AdrTemp));		
		/*AdrTemp=0x4090BAE0;
		Temp=*(AdrTemp);
		STTBX_Print(("\nres %d Temp=%x Adr=%lx",resres,Temp,AdrTemp));
		resres=memcmp((const void*)(pBufTemp+0x085d70),(const void*)Buf1,5);*/	
		STTBX_Print(("\nres %d Temp=%x",resres,Temp));

	}	
#endif
	return ;
	
}
#endif


/*end of file*/

