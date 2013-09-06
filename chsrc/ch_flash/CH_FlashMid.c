/*
  (c) Copyright changhong
  Name:CH_FlashMid.c
  Description: 实现长虹FALSH控制的中间接口函数，用于在应用和硬件FLASH具体实现方式间建立桥梁  

  Authors:Yixiaoli
  Date          Remark
  2007-03-25    Created

  Modifiction:

  Date:2007-04-24,2007-04-25 by Yixiaoli

  1.Modify fuction BOOL CHFLASH_Erase(),BOOL CHFLASH_Read(),
	BOOL CHFLASH_Write(),解决直接地址访问和16位FLASH访问的问题.

*/


/*include file*/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>   
#include "stddefs.h"
#include "stlite.h"
#include "stdevice.h"
#include "stflash.h"
#include "..\main\initterm.h"
#include "CH_FlashMid.h"
#include "lld.h"

#if 1
#define YXL_FLASH_WORD /*when this macro defined,SPANSION FLASH is accessed in 16 mode*/  
#endif
#define DEVICEID_SPANSION_GL064AS2 0x7e3e00
#define DEVICEID_ST_GL064AS2 8261635
	
static U32 stgManufactID=0;
static U32 stgBlockNum=0;
static CHFLASH_Block_t* stgpBlockData=NULL;
static U32 stgFlashSize=0; 
static CHFLASH_DeviceType_t stgFlashType=FLASH_UNKNOWN;


BOOL CHFLASH_Init(const CHFLASH_InitParams_t *InitParams,
				  CHFLASH_Handle_t           *Handle)
{
//beijing Spansion Flash:  S29GL256P10FFI010
//             numonyx(ST) :   PC28F256M29EWH 
        #define BLOCK_SIZE_128K                    128*1024
        #define SPANGL064AS2_BLOCK_NUM  256
        #define SPANGL064AS2_SISE               32*1024*1024

	CHFLASH_Block_t SPANGL064AS2_s[SPANGL064AS2_BLOCK_NUM]=
	{
		/*1*/
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		/*16*/
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		/*32*/
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		/*63*/
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },

		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },

		/*1*/
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		/*16*/
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		/*32*/
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		/*63*/
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K },
		{ BLOCK_SIZE_128K }        
	};

	unsigned int FDeviceId=0;
	U32 BlockNumTemp=0;
	FLASHDATA * BaseaddrTemp=(FLASHDATA *)(InitParams->BaseAddress);


	//numonyx :   PC28F256M29EWH  
	//Spansion :   GL256P10FFI01
	//两款Flash保护也兼容,Bootcode中使用下面两行代码。
         CH_LDR_DybUnPAllSectors();
         //CH_LDR_EnableFirst2BlockOTP();

        //Spansion和numonyx(ST) 使用同一Driver.
        stgFlashType=FLASH_SPANSION_GL064AS2;
        stgFlashSize=SPANGL064AS2_SISE;
        BlockNumTemp=SPANGL064AS2_BLOCK_NUM;
        stgpBlockData=Memory_Allocate(InitParams->DriverPartition,	BlockNumTemp*sizeof(U32));
        if(stgpBlockData==NULL)
        {
                STTBX_Print(("\nYxlInfo:no enough memory"));
                return TRUE;
        }
        memcpy((void*)stgpBlockData,(const void*)SPANGL064AS2_s,BlockNumTemp*sizeof(U32));

	*Handle=(CHFLASH_Handle_t)BaseaddrTemp;

	return FALSE;

}

BOOL CHFLASH_Erase(CHFLASH_Handle_t Handle,                  /* Flash bank identifier */
                   U32              Address,                 /* offset from BaseAddress */
                   U32              NumberToErase ) 
{

	U32 AddressTemp=Address;
	switch(stgFlashType)
	{
	case FLASH_SPANSION_GL064AS2:
		{
			DEVSTATUS res;			
			FLASHDATA * BaseaddrTemp=(FLASHDATA *)Handle;
#ifdef YXL_FLASH_WORD
			AddressTemp=AddressTemp/2;/*yxl 2007-04-24 add this line*/
#endif
			if(AddressTemp>(stgFlashSize-1)) 
			{
				STTBX_Print(("\n YxlInfo:Address is out of range"));
				return TRUE;
			}
			res=lld_SectorEraseOp(BaseaddrTemp,AddressTemp);
			
			if(res!=DEV_NOT_BUSY)
			{
				STTBX_Print(("\n YxlInfo:lld_SectorEraseOp()=%d",res));
				return TRUE;
			}

		}
		break;

	case FLASH_ST_M28:
		{
			ST_ErrorCode_t res;
			res=STFLASH_Erase(FLASHHandle,AddressTemp,NumberToErase);
			
			if(res!=ST_NO_ERROR)
			{
				STTBX_Print(("\n YxlInfo:STFLASH_Erase()=%s",GetErrorText(res)));
				return TRUE;
			}

		}
		break;

	default:
		{

		}
		break;

	}

	return FALSE; 
}


BOOL CHFLASH_Read(CHFLASH_Handle_t Handle,                   /* Flash bank identifier */
                  U32              Address,                  /* offset from BaseAddress */
                  U8               *Buffer,                  /* base of receipt buffer */
                  U32              NumberToRead,             /* in bytes */
                  U32              *NumberActuallyRead )
{

	int i;
	U32 NumToReadTemp=NumberToRead;
	U32 NumReadTemp=0;
	FLASHDATA Datatemp;/*yxl 2007-04-23 add this line*/
	
	U32 AddressTemp=Address;/*yxl 2007-04-24 add this line*/

	*NumberActuallyRead=NumReadTemp;

	switch(stgFlashType)
	{
	case FLASH_SPANSION_GL064AS2:
		{
			DEVSTATUS res;
			
			FLASHDATA * BaseaddrTemp=(FLASHDATA *)Handle;
#ifdef YXL_FLASH_WORD /*yxl 2007-04-25 add below section*/				
			NumToReadTemp=NumToReadTemp/2;
			AddressTemp=AddressTemp/2;
#endif /*end yxl 2007-04-25 add below section*/	
			if(AddressTemp>(stgFlashSize-1)) 
			{
				STTBX_Print(("\n YxlInfo:Address is out of range"));
				return TRUE;
			}

			if((AddressTemp+NumToReadTemp)>(stgFlashSize-1))
			{
				STTBX_Print(("\n YxlInfo:Warnning Read address is out of range"));

				NumToReadTemp=stgFlashSize-AddressTemp;
			}
#ifndef YXL_FLASH_WORD	 /*yxl 2007-04-25 modify below section*/
			for(i=0;i<NumToReadTemp;i++)
			{
				*(Buffer+i)=(U8)lld_ReadOp(BaseaddrTemp,AddressTemp+i);

				NumReadTemp++;
			}

#else
			for(i=0;i<NumToReadTemp;i++)
			{
				Datatemp=lld_ReadOp(BaseaddrTemp,AddressTemp+i);

				*(Buffer+2*i+1)=(U8)(Datatemp/256);
				*(Buffer+2*i)=(U8)(Datatemp%256);
				NumReadTemp+=2;
			}

#endif/*end yxl 2007-04-25 modify below section*/

			*NumberActuallyRead=NumReadTemp;

		}
		break;
	case FLASH_ST_M28:
		{
			ST_ErrorCode_t res;
			res=STFLASH_Read(FLASHHandle,AddressTemp,Buffer,  
				NumberToRead,NumberActuallyRead);
			
			if(res!=ST_NO_ERROR)
			{
				STTBX_Print(("\n YxlInfo:STFLASH_Read()=%s",GetErrorText(res)));
				return TRUE;
			}

		}
		break;


	default:
		{

		}
		break;

	}

	return FALSE; 
}

BOOL CHFLASH_Write(CHFLASH_Handle_t Handle,                  /* Flash bank identifier */
                   U32              Address,                 /* offset from BaseAddress */
                   U8               *Buffer,                 /* base of source buffer */
                   U32              NumberToWrite,           /* in bytes */
                   U32              *NumberActuallyWritten )
{
	int i=0;
	int j=0;
	int ProgCountTemp=0;
	int ProgNumLastTemp=0;
	int ProgNumTemp=16;

	U32 AddressTemp=Address;
	U32 NumToWriteTemp=NumberToWrite;
	U32 NumWrittenTemp=0;
	U8* pBufferTemp= Buffer;

	FLASHDATA BufTemp[16];

	*NumberActuallyWritten=NumWrittenTemp;
	switch(stgFlashType)
	{
	case FLASH_SPANSION_GL064AS2:
		{
			DEVSTATUS res;
			WORDCOUNT CountTemp;
			
			FLASHDATA * BaseaddrTemp=(FLASHDATA *)Handle;
#ifdef YXL_FLASH_WORD	/*yxl 2007-04-25 add below section*/		
			NumToWriteTemp=NumToWriteTemp/2;
			AddressTemp=AddressTemp/2;
#endif /*end yxl 2007-04-25 add below section*/
			if(AddressTemp>(stgFlashSize-1)) 
			{
				STTBX_Print(("\n YxlInfo:Address is out of range"));
				return TRUE;
			}

			if((AddressTemp+NumToWriteTemp)>(stgFlashSize-1))
			{
				STTBX_Print(("\n YxlInfo:Warnning Read address is out of range"));

				NumToWriteTemp=stgFlashSize-AddressTemp;
			}

			ProgCountTemp=NumToWriteTemp/16;
			ProgNumLastTemp=NumToWriteTemp%16;

			
			for(i=0;i<ProgCountTemp+1;i++)
			{
				if(i==ProgCountTemp) CountTemp=(WORDCOUNT)ProgNumLastTemp;
				else CountTemp=(WORDCOUNT)ProgNumTemp;

#if 0	 /*yxl 2007-04-25 modify below section*/
				for(j=0;j<CountTemp;j++)
				{
					BufTemp[j]=(FLASHDATA)(*(pBufferTemp+j));
				}

				res=lld_WriteBufferProgramOp(BaseaddrTemp,AddressTemp,
					CountTemp,(FLASHDATA*)BufTemp);
				if(res==DEV_NOT_BUSY)
				{
					
					NumWrittenTemp+=(U32)CountTemp;
					AddressTemp+=(U32)CountTemp;
					pBufferTemp+=(U8)CountTemp;
				}
				else
				{
					STTBX_Print(("\n YxlInfo:lld_WriteBufferProgramOp() is wrong"));
					return TRUE;

				}

#else
				for(j=0;j<CountTemp;j++)
				{
					BufTemp[j]=(FLASHDATA)(*(pBufferTemp+2*j+1)*256+*(pBufferTemp+2*j));
				}


				res=lld_WriteBufferProgramOp(BaseaddrTemp,AddressTemp,
					CountTemp,(FLASHDATA*)BufTemp);
				if(res==DEV_NOT_BUSY)
				{
					
				/*	NumWrittenTemp+=(U32)CountTemp;yxl 2007-04-25*/
					NumWrittenTemp=NumWrittenTemp+2*(U32)CountTemp;
					AddressTemp+=(U32)CountTemp;
					pBufferTemp=pBufferTemp+2*(U8)CountTemp;
				}
				else
				{
					STTBX_Print(("\n YxlInfo:lld_WriteBufferProgramOp() is wrong"));
					return TRUE;

				}
			}

#endif/*end yxl 2007-04-25 modify below section*/

			*NumberActuallyWritten=NumWrittenTemp;



		}
		break;
	case FLASH_ST_M28:
		{
			ST_ErrorCode_t res;
			res=STFLASH_Write(FLASHHandle,Address,Buffer,  
				NumberToWrite,NumberActuallyWritten);
			
			if(res!=ST_NO_ERROR)
			{
				STTBX_Print(("\n YxlInfo:STFLASH_Write()=%s",GetErrorText(res)));
				return TRUE;
			}

		}
		break;

	default:
		{

		}
		break;

	}

	return FALSE; 
}

#if 0                              
ST_ErrorCode_t STFLASH_BlockLock( STFLASH_Handle_t Handle,    /* Flash bank identifier */
                                  U32              Offset ); /* offset from BaseAddress */
                              
ST_ErrorCode_t STFLASH_BlockUnlock( STFLASH_Handle_t Handle,     /* Flash bank identifier */
                                    U32              Offset );  /* offset from BaseAddress */

#endif



/*end of file*/

