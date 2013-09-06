#define  STTBX_REPORT

/*{{{    COMMENT Standard Header */
/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 1995

Source file name : nvm.c
Author :           S.R.Christian

Contains functions to access Non Volatile Memory storage in system

Date        Initials	Modification
----        --------	------------
15-Aug-95   SRC		Created
26-Dec-96   THT		Adapted for Weixing board
07-Aug-97   THT 		Modified to take care of the hard coded size limit
							between ST24E64 and ST24E32
25-MAY-98   GPV, KS
            CV       Adapted for Diversity board
31-AUG-99   LEHU  Corrected the bug of no nvm update after eeprom initialisation
30-OC-2003  zxg   change adapt to changhong dbase 
2005-02-    yxl   changed   形成一个NORMAL Release的数据库,将IPANEL HTML从BEIJING_DATABASE独立出来             
						  定义宏BEIJING_DBASE表示GeHua Release 的数据库  
				          否则，为NORMAL Release的数据库
************************************************************************/
/*}}}  */
#include <stdio.h>
#include <string.h>
#include "stcommon.h"
#include "stddefs.h"  /* standard definitions                    */

#include "sttbx.h"

#include "ste2p.h"
#include "..\report\report.h"
/*#include "..\symbol\symbol.h"*/
#include "..\dbase\st_nvm.h"
#include "..\dbase\vdbase.h"

#include "appltype.h"
#include "stdevice.h"
#include "stack.h"
#include "stlite.h"
#include "stddefs.h"
#include "stflash.h"
#include "stboot.h"
#include "..\main\initterm.h"
#include "channelbase.h"
#include "..\ch_flash\ch_flashmid.h"

#define MD_090817_ZXG
extern STFLASH_Handle_t FLASHHandle; /*yxl 2004-06-15 add this line*/


#ifdef STBID_FROM_E2PROM

boolean CH_STBIDIsFromE2P(void)    ;
void CH_ReadSTBIDFromE2P( U8 *uc_StbID ); 

#endif

#ifdef MD_090817_ZXG
boolean CH_CheckdataValid(void);
#endif
#define FLASH_BANK_0_BASE_ADDRESS	0xa0000000



/*#define  TEST_DEBUG*/
static   CH_NVM_BLOCK_HEADER      *pstNvmRootBlockHeader; /* actual root block data pointer (RAM copy) */
static   CH_NVM_ROOT_BLOCK_DATA   *pstNvmRootBlockData;   /* actual root block header pointer (RAM copy) */
U8       *aucNvmCopy =NULL;

#define NVM_MAGIC_NUMBER             0x7234

#define DBASEVERION  4

#define NVM_APPLICATION_MEM_SIZE  (8 * 1024 - 4) /*(32 * 1024 - 4)  in bytes - top 4 bytes for MFG id */
#define  NVM_APPLICATION_MEM_START     0x0000            /* physical addr in the device */
#define  NVM_APPLICATION_MEM_END       ( NVM_APPLICATION_MEM_START + NVM_APPLICATION_MEM_SIZE ) /* physical end address */

#define  NVM_DEVICE_NAME               "EEPROM"

static U16 usDbaseNum = (NVM_MAGIC_NUMBER + DBASEVERION*10);  


/* module private static variables */
extern STE2P_Handle_t          hE2PDevice; 

semaphore_t  *gp_semNvmAccessLock = NULL;
semaphore_t *gp_semFlashAccess = NULL;

static  BOOLEAN   hard_error; /* set if we get an I2C error */

#define  ST24E32       0xA0       /* device id for EEPROM */
#define  ST24E64       0xA0       /* device id for EEPROM */
#define  DEVICE_SIZE            /*  NVM_APPLICATION_MEM_START_DFA*/256 /*wz 重新规划了E2P 20101214 */

extern   partition_t             *SystemPartition ;

/*{{{ NVM_WriteRecord ()*/
static   BOOLEAN  CH_NVM_WriteRecord ( int iOffset, int iLength, BYTE *paucData )
{
   ST_ErrorCode_t       ReturnError;
   U32                  uiActualLen;
   U32					retry = 500  ;/*wufei changed 5 to 5000 2008-11-14*/
   while(--retry)
   {
	   ReturnError = STE2P_Write (   hE2PDevice
                                 , iOffset
                                 , paucData
                                 , iLength
                                 , &uiActualLen ) ;
	   if((ReturnError == ST_NO_ERROR) && (uiActualLen == iLength))   
		   return   FALSE;
	   else
		   task_delay(200);
   }
   return   TRUE;
}


/*{{{ NVMUpdate*/
BOOLEAN  CH_NVMUpdate ( opaque_t nvmid )
{
   BOOLEAN error = FALSE;
   CH_NVM_BLOCK_HEADER   *pstBlockHeader;
   
   UINT offset;
  clock_t clock1=time_now();
  clock_t clock2=0;
  clock_t clock3=0;
   pstBlockHeader = ( CH_NVM_BLOCK_HEADER * ) nvmid;

   if ( pstBlockHeader -> usMagicValue != usDbaseNum )
   {
#ifdef TEST_DEBUG
      do_report ( severity_error, "NVMUpdate Invalid block access ID [%X][%x]\n", nvmid ,pstBlockHeader -> usMagicValue);
#endif
      return  ( TRUE );
   }
   
   /* serialise access to data */
   semaphore_wait ( gp_semNvmAccessLock );

   offset = (int)((BYTE*)pstBlockHeader - (BYTE*)aucNvmCopy);

   error = CH_NVM_WriteRecord(offset, pstBlockHeader->usBlockLength  + CH_NVM_HEADER_LENGTH,
                           (BYTE*)pstBlockHeader);
                /* wait to aggregate updates MILLI_DELAY( 200 ); */
   /* release access and signal update */
   semaphore_signal ( gp_semNvmAccessLock );
   clock2=time_now();
   if(error==true)
   {
		do_report(0,"\nError File==%s Line==%d",__FILE__,__LINE__);
   }
   clock3=clock2-clock1;
   do_report(0,"  clock =%d\n",clock3/ST_GetClocksPerSecond());

				
   return(error);
}

/*{{{ NVMInit */
/***NVM记录类型个数***/
#define NVM_RECORD_NUM 1
/***记录名称***/
char *pacNvmBlockNames [ NVM_RECORD_NUM] ={						
							"nvm_box_db"
							};
/***每个NVM记录数据长度***/
USHORT ausNvmRecordLength  [NVM_RECORD_NUM] ={                        
								 sizeof ( BOX_INFO_STRUCT )
								};
/**每个NVM记录个数****/
USHORT  ausNvmNoOfRecords  [NVM_RECORD_NUM] =
								{								
                        			                     1
								};
/****记录初始化函数数组****/
PTR_2_HOOK_FUNCTION apfuncNvmHookFunctions[NVM_RECORD_NUM] =
{
	InitialiseNvmBoxInfo
};

int InitialiseNvmBoxInfo ( INT iParam1, INT iParam2 )
{
    int i ;
	
    if ( iParam1 == ( INT ) NULL )
	{
#ifdef TEST_DEBUG
			do_report(0, "Invalid Address for Box Info ...\n" );
#endif
		return TRUE;
	}
	pstBoxInfo = ( BOX_INFO_STRUCT * ) iParam1;
    
   
    pstBoxInfo->stLastWatchingProg.uiWord32=INVALID_LINK;
    pstBoxInfo->stLastWatchingProg.uiWord32=INVALID_LINK;
    pstBoxInfo->stLastWatchingRadioProg.uiWord32=INVALID_LINK;

    pstBoxInfo->iNoOfValidServices=0;

    pstBoxInfo->iNoOfValidXpdrs =0;  

	pstBoxInfo->iHeadServiceIndex=INVALID_LINK;
	pstBoxInfo->iTailServiceIndex =INVALID_LINK;

    pstBoxInfo->iHeadXpdrIndex  =INVALID_LINK;

    pstBoxInfo->iTailXpdrIndex =INVALID_LINK;

	pstBoxInfo->abiFlashValidity =FALSE;
				
	pstBoxInfo->abiBoxPoweredState = 0;
					

	pstBoxInfo->abiOutputDisplayFormat =0;
#ifdef CH_DVBT_PLATFORM
	pstBoxInfo->abiLanguageInUse =1;/*20050916 add for default English*/
#else
    pstBoxInfo->abiLanguageInUse =0;
#endif

	pstBoxInfo->abiTVColor =0;      /*自动输出*/
        pstBoxInfo->abiBoxLock=1;
	
	pstBoxInfo->abiTVSaturation =50;
	/*
	TV Saturation
	*/

	pstBoxInfo->abiTVBright=50;
	/*
	TV Bright
	*/

	pstBoxInfo->abiTVContrast=50;
	/*
	TV Contrast
	*/


	
	pstBoxInfo->GlobalVolume=0;

    /*20060225 add*/
	#ifdef NAFR_KERNEL /*yxl 2005-02-23 add this line*/

    pstBoxInfo->FirstPin=true;
	/**************/
	pstBoxInfo->PinResetIndex=-1;
		#endif


#if 1
	pstBoxInfo->SDVideoOutputType=TYPE_CVBS;
	pstBoxInfo->HDVideoOutputType=TYPE_YUV;
	pstBoxInfo->HDDigitalVideoOutputType=TYPE_HDMI;
	pstBoxInfo->SDVideoTimingMode=MODE_576I50;
	pstBoxInfo->HDVideoTimingMode=MODE_720P50;
#endif


	pstBoxInfo->SPDIFOUT=0;

	

	pstBoxInfo->serach_tag                  = 0;
	pstBoxInfo->ucSoftwareVerMajor=0;     /* software version major (8)*/
	pstBoxInfo->ucSoftwareVerMinor=0;     /* software version minor (4)*/
	pstBoxInfo->upgrade=0;     /* 记录升级后第一次升级*/

	return FALSE;
}
/*}}}  */


/*}}}  */
/* E2PROM record registe */
BOOLEAN CH_NVMAccess ( char *name, opaque_t *nvmid_ptr)
{
   BOOLEAN error = FALSE;
   CH_NVM_BLOCK_HEADER *pstBlockHeader;

   if ( strlen(name) > (CH_NVM_NAME_LENGTH -1))
   {
     do_report(0,"NVRAM record name too long\n" );
   }

   /* serialise access to data */
   semaphore_wait ( gp_semNvmAccessLock );
   *nvmid_ptr = ( opaque_t ) NULL;

   /* look for existing data */
   error = symbol_enquire_value ( ( char * )name, ( void ** ) &pstBlockHeader);

   /* release access and wait for lazy update to control block */
   semaphore_signal ( gp_semNvmAccessLock );

   if (!error)
   {
#ifdef TEST_DEBUG
	  	do_report(0,"NVM Access %s Successful!",/*name ,*/(char *)pstBlockHeader ->aucBlockName);
#endif
	  *nvmid_ptr = ( opaque_t ) pstBlockHeader;
   }
   return ( error );
}
/*}}}  */

/*zxg add for 更新E2PROM中某段位置数据*/
BOOLEAN  CH_NVMUpdateByOffset ( opaque_t nvmid, S16 Offset,S16 Len )
{
	BOOLEAN error = FALSE;
	CH_NVM_BLOCK_HEADER   *pstBlockHeader;
	BYTE  *paucData;
	int   iRecordLength;
	int   iTemp;
	int   iOffset;
	
	
	/* serialise access to data */
    pstBlockHeader = ( CH_NVM_BLOCK_HEADER * ) nvmid ;
	
    if ( pstBlockHeader -> usMagicValue != usDbaseNum )
    {
		do_report ( severity_error, "NVMUpdateByID Invalid block access ID [%X][%x]\n", nvmid ,pstBlockHeader -> usMagicValue );
		return  ( TRUE );
    }
	

	
	/* serialise access to data */
	semaphore_wait ( gp_semNvmAccessLock );
		
	
	paucData = ( BYTE * ) nvmid;

	paucData += (CH_NVM_HEADER_LENGTH+Offset);
	
	iOffset = paucData - aucNvmCopy;
	
	error =  CH_NVM_WriteRecord ( iOffset, Len, paucData );
	/* release access and signal update */
	semaphore_signal ( gp_semNvmAccessLock );
	
   return(error);
}

/*{{{ NVMGetDataAddress */

void	*CH_NVMGetDataAddress ( opaque_t nvmid )
{
   CH_NVM_BLOCK_HEADER   *pstBlockHeader;
   BYTE  *paucData;

   pstBlockHeader = ( CH_NVM_BLOCK_HEADER * ) nvmid;
#if 1
   if ( pstBlockHeader ->usMagicValue != usDbaseNum )
   {
      do_report(0,"invalid block address\n");      
      return  ( ( void * ) NULL );
   }
#endif

   paucData = ( BYTE * ) nvmid;
   paucData += CH_NVM_HEADER_LENGTH;
	return ( paucData );
}


/*函数名: void  CH_FlashLock(void)
  *开发人员:zengxianggen
  *开发时间:2004/10/30
  *函数功能:访问FLASH信息互斥操作
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:
  *返回值说明:	
  *参数表
/*函数体定义*/

void  CH_FlashLock(void)
{
      semaphore_wait ( gp_semFlashAccess );
}
/*函数名: void  CH_FlashUnLock(void)
  *开发人员:zengxianggen
  *开发时间:2004/10/30
  *函数功能:访问FLASH信息互斥操作
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:
  *返回值说明:	
  *参数表
/*函数体定义*/
void  CH_FlashUnLock(void)
{
      semaphore_signal( gp_semFlashAccess );
}


/*函数名:CH_PUB_LockFlash(U32 Offset) */
/*开发人和开发时间:zengxianggen 2006-11-06             */
/*函数功能描述:设置FLASH写保护模式模式*/
/*函数原理说明:    */
/*使用的全局变量、表和结构：                          */
/*调用的主要函数列表：                                           */
/*返回值说明：无  */
void CH_PUB_LockFlash(U32 Offset)
{
	CH_LDR_BlockLock (Offset/0x20000);
}

/*函数名:void CH_PUB_LockFlash(U32 Offset)*/
/*开发人和开发时间:zengxianggen 2006-11-06             */
/*函数功能描述:设置FLASH写保护模式模式*/
/*函数原理说明:    */
/*使用的全局变量、表和结构：                          */
/*调用的主要函数列表：                                           */
/*返回值说明：无  */
void CH_PUB_UnLockFlash(U32 Offset)
{
      CH_LDR_BlockUnlock (Offset/0x20000);
}

/*函数名: S16 CHCA_FlashWrite(unsigned long  offset,unsigned char*  pBuffer,unsigned long  Size)
  *开发人员:zengxianggen
  *开发时间:2006/12/13
  *函数功能:更新 FLASH中MG CA的数据
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:
  *返回值说明:	
  *参数表:
  unsigned long  offset,相对CA数据存贮FLASH的偏移位置
  unsigned char*  pBuffer,更新数据
  unsigned long  Size，更新数据大小
/*函数体定义*/
S16 CHCA_FlashWrite(unsigned long  offset,unsigned char*  pBuffer,unsigned long  Size)
{
	#define CA_FLASH_BLOCKNUM              2                  /*存贮CA数据的信息的FLASH块数*/
	U32 iCount,iSize;
	BOOL bError = FALSE,bUpdate = FALSE;
	ST_ErrorCode_t  ReturnError;
	U8* aucFlashCopyTemp;

	int i_CAFLASH_BlockNum;
	int i_CAFLASH_BlockSize;
	/*20070607 change to 8M FLASH设置*/
       U32 CA_Flash_Address[CA_FLASH_BLOCKNUM]=
	   	{
			  0x01F60000,0x01F60000+128*1024
		};
	  /* 	{
                  0x012E0000,0x012E0000+128*1024
		};WZ 添加SKIN后的调整011211*/
	 int   itemp;
	 /*是否需要更新FLASH数据*/
        boolean CA_Flash_Update[CA_FLASH_BLOCKNUM]=
        	{
                  false,false
        	};

       /*判断参数的有效性*/
	 if(offset>16*1024||pBuffer==NULL||(offset+Size)>16*1024)
	 	{
                  do_report(0,"ChCA_FlashWrite 参数 ERROR !" );
	 	}

           i_CAFLASH_BlockNum=1;
	    i_CAFLASH_BlockSize=128*1024;
     
 
	 CH_FlashLock();
	 
	aucFlashCopyTemp = (U8 *)memory_allocate ( CHSysPartition , i_CAFLASH_BlockSize );

	if ( aucFlashCopyTemp == NULL )
	{
		do_report(0,"ChCA_FlashWrite allocate ERROR !" );
		CH_FlashUnLock();
		return TRUE;
	}
	memset ( aucFlashCopyTemp,0, i_CAFLASH_BlockSize );
	/*读取现在的FLASH中的数据*/
	for ( iCount =0 ;iCount < i_CAFLASH_BlockNum; iCount++ )
	{
	
	ReturnError = CHFLASH_Read (FLASHHandle,
								     CA_Flash_Address[iCount],
								     (U8 *)(aucFlashCopyTemp+iCount*i_CAFLASH_BlockSize),
								     i_CAFLASH_BlockSize,
								     &iSize);



	       if ( ReturnError != ST_NO_ERROR )
	       {

	  	     do_report(0,"CHFLASH_Read [%d] ERROR !",ReturnError );
 
		     bError = TRUE;
		     break;
	       }
	}

           CA_Flash_Update[0]=true;
 
	/*更新数据到内存*/
         for(itemp=0;itemp<Size;itemp++)
              {
                   *(aucFlashCopyTemp+offset+itemp) = *(pBuffer+itemp); 
		}
 
     /*写最新的数据到FLASH中*/
	 for ( iCount =0 ;iCount < i_CAFLASH_BlockNum; iCount++ )
	 	{
	          if(CA_Flash_Update[iCount]==true)
	 	  {

		     CH_PUB_UnLockFlash(CA_Flash_Address[iCount]);/*去保护*/
		     do		  
		    {
		     ReturnError = CHFLASH_Erase( FLASHHandle,
		   							    CA_Flash_Address[iCount],
                	    	                                       i_CAFLASH_BlockSize);
		     }while ( ReturnError != ST_NO_ERROR );
			
		    do
		   {	
		       ReturnError = CHFLASH_Write( FLASHHandle,
			                				    CA_Flash_Address[iCount],
                	                                              (U8 *)(aucFlashCopyTemp+iCount*i_CAFLASH_BlockSize),
                    	                                              i_CAFLASH_BlockSize,
                        	                                       &iSize );
             	       }while ( ReturnError != ST_NO_ERROR );


              CH_PUB_LockFlash(CA_Flash_Address[iCount]);/*写保护*/
	
                    if ( ReturnError != ST_NO_ERROR )
		      {
		        bError = TRUE;
		       break;
		       }	
	 	    }		
           }

        memory_deallocate ( CHSysPartition, aucFlashCopyTemp );
	CH_FlashUnLock();
	return ( bError );
}


BOOL bNvmUpdate = FALSE;

BOOLEAN  CH_NVMCheckUpdate ( BOOL *bNvmUpdate )
{
	U32         len;
	int        iCount;

	ST_ErrorCode_t          ReturnError;

    CH_NVM_BLOCK_HEADER   *pstBlockHeader;
    int   iNoOfBytesPassed;

	/* first create the instance of STE2P from physical offset 0 to DEVICE_SIZE */
	/* select the application managed segment map */
	/* allocate private data and initialise */
	
	if ( ( aucNvmCopy = ( U8 * ) memory_allocate ( SystemPartition , DEVICE_SIZE ) ) == NULL )
	{
		do_report ( severity_info,"<%s><%d>MALLOC=> Failed to get free memory for NVM_data image\n"__FILE__ ,__LINE__ ) ;
		hard_error = TRUE;
		return TRUE;
	}

	iCount = 3; 

	semaphore_wait ( gp_semNvmAccessLock );
	while(1)
	{
		/* initialise (for now) data storage from mass file */		
		if ((( ReturnError = STE2P_Read ( hE2PDevice,
			0,
			( BYTE * ) aucNvmCopy,
			DEVICE_SIZE,
			&len ) ) == ST_NO_ERROR) && (len == DEVICE_SIZE) )
		{
			break;
		}
		else
		{
			if(--iCount)
			{
				task_delay(1000);
			}
			else
			{
				do_report ( severity_info,"Failed read EEPROM!Size Req[%d] Act[%d]\n",
					DEVICE_SIZE,len );
				hard_error = TRUE;
				semaphore_signal ( gp_semNvmAccessLock );
				return TRUE;
			}
		}
	}	
	semaphore_signal ( gp_semNvmAccessLock );
	
	hard_error = FALSE;
	*bNvmUpdate = FALSE;

	pstNvmRootBlockHeader = ( CH_NVM_BLOCK_HEADER * ) aucNvmCopy;
	pstNvmRootBlockData = ( CH_NVM_ROOT_BLOCK_DATA * ) ( aucNvmCopy + CH_NVM_HEADER_LENGTH );

	/* check for integrity of control block */
	if ( ( pstNvmRootBlockHeader -> usMagicValue != usDbaseNum ) ||
		( pstNvmRootBlockHeader -> usBlockLength != CH_NVM_ROOT_BLOCK_LENGTH ) ||
		( strcmp ( ( char *)pstNvmRootBlockHeader -> aucBlockName, "NVMRoot" ) != 0 ) ||
		( pstNvmRootBlockData -> iNoOfBlocksAvailable != NVM_RECORD_NUM )
		)

	{
		*bNvmUpdate = TRUE;
		/* re-initialise */
		do_report( severity_info,
			"<%s><%d>>>>Reinitialising whole EEPROM! (%d, %x, %d, %d)\n",
			__FILE__ ,__LINE__ ,hard_error,
			pstNvmRootBlockHeader -> usMagicValue,
			pstNvmRootBlockHeader -> usBlockLength,
			DEVICE_SIZE ) ;

			/*
			* clear the DRAM contents and write them back to EEPROM
		*/
	}

	/*   	symbol_register ( (char *)pstNvmRootBlockHeader ->aucBlockName , ( void * ) aucNvmCopy );*/
	pstBlockHeader = (CH_NVM_BLOCK_HEADER *)((BYTE*)aucNvmCopy + CH_NVM_HEADER_LENGTH + CH_NVM_ROOT_BLOCK_LENGTH);
	iNoOfBytesPassed = CH_NVM_HEADER_LENGTH + CH_NVM_ROOT_BLOCK_LENGTH;

	for ( iCount = 0; iCount < NVM_RECORD_NUM ; iCount++ )
	{

		USHORT   usTemp;

		/*
		* calculate the block length temporarily
		*/
		usTemp = ausNvmNoOfRecords [ iCount ] * ausNvmRecordLength [ iCount ];

		/*
		* position the block header
		*/
		pstBlockHeader = ( CH_NVM_BLOCK_HEADER * ) ( aucNvmCopy + iNoOfBytesPassed );

		if ( ( pstBlockHeader -> usMagicValue != usDbaseNum ) ||
			( pstBlockHeader -> usNoOfRecords != ausNvmNoOfRecords [ iCount ] ) ||
			( pstBlockHeader -> usRecordLength != ausNvmRecordLength [ iCount ] ) ||
			( pstBlockHeader -> usBlockLength != usTemp ) ||
			( strcmp ( ( char * ) pstBlockHeader -> aucBlockName, pacNvmBlockNames [ iCount ] ) != 0 )
			)
		{

			*bNvmUpdate = TRUE;
			/* re-initialise the block */
			do_report ( severity_info, "<%s><%d>>>> Reinitialising Block [%s]!(%d) (%d * %d = %d)\n",
				__FILE__,__LINE__,pacNvmBlockNames [ iCount ],pstBlockHeader -> usMagicValue,
				ausNvmNoOfRecords [ iCount ],
				ausNvmRecordLength [ iCount ],
				usTemp ) ;
		}
		symbol_register ( ( const char * )pacNvmBlockNames [ iCount ], ( void * ) pstBlockHeader );
		iNoOfBytesPassed += ( usTemp  + CH_NVM_HEADER_LENGTH );
	}
	return ( FALSE );
}

BOOL CH_NVMInit(void)
{
	BOOL bError ;
	bError = CH_NVM_WriteRecord(0,DEVICE_SIZE/*原来的有问题，复位E2P数据wz*/, (U8 *)aucNvmCopy);
	return 	bError;
}


BOOL CH_DBASEMGRINIT (void)
{
	BOOL bError=FALSE ;

    if( CH_NVMCheckUpdate(&bNvmUpdate) )
    {
        do_report( severity_info,"<%s><%d>Nvm or Flash check error!\n",__FILE__,__LINE__);
		return TRUE;
    }
#if 1
    if ( bNvmUpdate)
	{
	     Reset();
	}	
#endif 	

	bError |= CH_NVMAccess ( "nvm_box_db",&idNvmBoxInfoId);
	
	pstBoxInfo = ( BOX_INFO_STRUCT * ) CH_NVMGetDataAddress ( idNvmBoxInfoId );

 
#ifdef MD_090817_ZXG
   if(CH_CheckdataValid() == false)
   	{
   	   Reset(); /* */
   	}

   #endif
 
    return FALSE;
}

BOOL DBASEDATAINIT ( )
{
    CH_NVM_BLOCK_HEADER   *pstNvmBlockHeader;
    int   iCount ,iNoOfBytesPassed;
	
    /* NVM RESET  */
	
    memset ( aucNvmCopy, 0, DEVICE_SIZE ); 
    strcpy ( ( char * ) pstNvmRootBlockHeader -> aucBlockName, "NVMRoot" );
	
    pstNvmRootBlockHeader -> usMagicValue     = usDbaseNum;
    pstNvmRootBlockHeader -> usBlockLength    = CH_NVM_ROOT_BLOCK_LENGTH;
    pstNvmRootBlockHeader -> usRecordLength   = CH_NVM_ROOT_BLOCK_LENGTH;
    pstNvmRootBlockHeader -> usNoOfRecords    = 1;
	
    pstNvmRootBlockData -> iNoOfBlocksAvailable  = NVM_RECORD_NUM;
    pstNvmRootBlockData -> biFlashDataValidity   = FALSE;
	
    iNoOfBytesPassed = CH_NVM_HEADER_LENGTH + CH_NVM_ROOT_BLOCK_LENGTH;
	
    for ( iCount = 0; iCount < NVM_RECORD_NUM ; iCount++ )
    {
	/*
	* position the block header
		*/
		pstNvmBlockHeader = ( CH_NVM_BLOCK_HEADER * ) ( aucNvmCopy + iNoOfBytesPassed );
		
		strcpy ( ( char * ) pstNvmBlockHeader -> aucBlockName, pacNvmBlockNames [ iCount ] );
		pstNvmBlockHeader -> usMagicValue     = usDbaseNum;
		pstNvmBlockHeader -> usNoOfRecords    = ausNvmNoOfRecords [ iCount ];
		pstNvmBlockHeader -> usRecordLength   = ausNvmRecordLength [ iCount ];
		pstNvmBlockHeader -> usBlockLength    = ausNvmNoOfRecords [ iCount ] * ausNvmRecordLength [ iCount ];
		
		/*
        * call the application module to initialise its block by itself
		*/
		apfuncNvmHookFunctions [ iCount ] (  ( int ) ( aucNvmCopy + iNoOfBytesPassed + CH_NVM_HEADER_LENGTH ), ( int ) NULL );
		
		iNoOfBytesPassed += ( pstNvmBlockHeader -> usBlockLength + CH_NVM_HEADER_LENGTH );
    }
	
		
		
    do_report ( severity_info,"nvm init !\n");
		
   }



/* 恢复出厂设置,数据库清空 */
S8 Reset(void)
{

    boolean error;

	DBASEDATAINIT ( );
	error = CH_NVMInit();		

	DisplayLED( 0 );

    return error;
}
/***read E2PROM Data****/
int ReadNvmData(U32 addr,U16 number , void* buf)
{
	U32 	retval= ST_NO_ERROR;
	U32 ret = 0;
	
	semaphore_wait ( gp_semNvmAccessLock );
	retval = STE2P_Read (hE2PDevice,addr,buf,number,&ret);
	semaphore_signal ( gp_semNvmAccessLock );
	
	if(ret != number ||retval != ST_NO_ERROR)
	{
		do_report(0," STE2P_Read ERR!!addr %d  len %d  buf  0x%x ret = %d retval = %d\n",addr,number,buf,ret,retval);
		return 1;
	}
	return 0;
}
/***Write data to E2PROM ****/
int WriteNvmData(U32 addr,U16 number , void* buf)
{
	U32 	retval= ST_NO_ERROR;
	U32 ret = 0;
	
	semaphore_wait ( gp_semNvmAccessLock );
	retval = STE2P_Write (hE2PDevice,addr,buf,number,&ret);
	semaphore_signal ( gp_semNvmAccessLock );

	if(ret != number ||retval != ST_NO_ERROR)
	{
		do_report(0,"STE2P_Write ERR!!addr %d  len %d  buf  0x%x  ret = %d retval = %d \n",addr,number,buf,ret,retval);
		return 1;
	}
	
	return 0;
}



/************************函数说明***********************************/
/*函数名:void CH_CAReadSTBID(U8*  iStbId)                                           */
/*开发人和开发时间:zengxianggen 2006-11-19                         */
/*函数功能描述:得到本机机号信息                        */
/*函数原理说明:                                                                          */
/*使用的全局变量、表和结构：                                     */
/*调用的主要函数列表：                                                     */
/*返回值说明： 返回卡的控制状态             */
void CH_CAReadSTBID(U8*  iStbId)
{
	 U8 STB_ID[8];
	 int i;
#ifdef STBID_FROM_E2PROM
         if(CH_STBIDIsFromE2P() == true)
         	{        
                    CH_ReadSTBIDFromE2P(STB_ID);
		    	 for (i=0;i<8;i++)
		       {
		          iStbId[i] = STB_ID[i];
		       }
		    return;
         	}
#endif	 
#if 0
        /*读取备用机号信息*/   
	 CH_PUB_SetFlashRead();
        memcpy((U8 *)STB_ID,(U8 *)AUXILIARY_STB_ID_ADDRES,8);
#else
      CH_ReadSTBID(STB_ID);
#endif
	 for (i=0;i<8;i++)
       {
          iStbId[i] = STB_ID[7-i];
       }
/*0x01,19,5f,00,00,01,00,01 NAGRA测试机号*/
#ifndef NAGRA_PRODUCE_VERSION/*如果为测试版本，是测试机号也认为有效*/
        if( 
	   iStbId[0]==0x01                    
	   &&iStbId[1]==0x19                    
	   &&iStbId[2]==0x5f
	    &&iStbId[3]==0x00
	     &&iStbId[4]==0x00
	      &&iStbId[5]==0x01
	       &&iStbId[6]==0x00

	        &&iStbId[7]==0x01
	)
    	{
          return;
    	}
      
#endif

}



/*end yxl 2005-09-04 add below section*/
/*zxg 20060303 add for 更新E2PROM 中某个字段数据
函数名称:BOOLEAN CH_NVMUpdateByAddress( opaque_t nvmid, U8 *Address,S16 Len )
函数说明: 更新E2PROM 中某个字段数据*/
BOOLEAN CH_NVMUpdateByAddress( opaque_t nvmid, U8 *Address,S16 Len )
{
	/* serialise access to data */

    CH_NVM_BLOCK_HEADER   *pstBlockHeader;
    BYTE  *paucData;

    S16 Offset;

   pstBlockHeader = ( CH_NVM_BLOCK_HEADER * ) nvmid ;
    if ( pstBlockHeader -> usMagicValue != usDbaseNum )
    {
		do_report ( severity_error, "NVMUpdateByID Invalid block access ID [%X][%x]\n", nvmid ,pstBlockHeader -> usMagicValue );
		return  ( TRUE );
    }
    
   paucData = CH_NVMGetDataAddress(nvmid);
  Offset=/*20060828 change*/(unsigned int)Address-(unsigned int)paucData;  
  /*20060828 add*/
   if(Offset<0)/* wz 20061113 change <=to <*/
   {
   	do_report(0,"Error E2P update address\n");
   	return false;
   }
   /***********************************************/
 return  CH_NVMUpdateByOffset(nvmid,Offset,Len);

}


/************************函数说明***********************************/
/*函数名:void FLSH_DRV_Write(U8 *Data,int FlashAddress,int Len)                                             */
/*开发人和开发时间:zengxianggen 2007-01-18                            */
/*函数功能描述:写数据到指定FLASH位置,主要用于写备用机号和PARRING DATA,不支持跨FLASH块写和8K BLOCK      */
/*函数原理说明:                                                                          */
/*使用的全局变量、表和结构：                                     */
/*调用的主要函数列表：                                                     */
/*返回值说明：          */
boolean FLSH_DRV_Write(U8 *Data,U32 FlashAddress,int Len)
{
	U32 FLashBlock1;
	U32 FLashBlock2;
	U32 FlashOffset;
	U8* aucFlashCopyTemp;
	U32 iSize;
	int ErrorTime=0;
       
	ST_ErrorCode_t  ReturnError;
	/*首先判断数据的有效性*/
	if(FlashAddress<0xA0000000||(FlashAddress+Len)>0xA0800000)
	{
		return true;
	}
 	/*计算需要写入数据的FLASH位置*/
	FLashBlock1=(FlashAddress-0xA0000000)/(128*1024);
	FLashBlock2=(FlashAddress+Len-0xA0000000)/(128*1024);
	
	if(FLashBlock1!=FLashBlock2||FLashBlock1>=128||FLashBlock2>=128)/*需要写入的数据跨越FLASH块*/
	{
		return true;
	}
	FlashOffset=FLashBlock1/*20070726 del *64*1024*/;
	
	aucFlashCopyTemp = (U8 *)memory_allocate ( CHSysPartition , 128*1024);
	
	if ( aucFlashCopyTemp == NULL )
	{
		do_report(0,"FLSH_DRV_Write allocate ERROR !" );
		return true;
	}
	CH_FlashLock();
	/*读取原来的数据*/
	ReturnError = CHFLASH_Read (FLASHHandle,
		FlashOffset*128*1024,
		aucFlashCopyTemp,
		128*1024,
		&iSize);
	/*得到新的数据*/
       memcpy(aucFlashCopyTemp+(FlashAddress-(0xA0000000+FlashOffset*128*1024)),Data,Len);
	CH_PUB_UnLockFlash(FlashOffset*128*1024);/*去保护*/
	/*把新的数据写入数据*/
	ErrorTime=0;
	do	  
	{
		ReturnError = CHFLASH_Erase( 
			FLASHHandle,
			FlashOffset*128*1024,
			128*1024);
		ErrorTime++;
		if(ErrorTime>1000)
		{
			CH_PUB_LockFlash(FlashOffset*128*1024);/*加保护*/
			CH_FlashUnLock();
			return true;
		}
	}while ( ReturnError != ST_NO_ERROR );
	ErrorTime=0;
	do
	{	
		ReturnError = CHFLASH_Write(
			FLASHHandle,
			FlashOffset*128*1024,
			(U8 *)(aucFlashCopyTemp),
			128*1024,
			&iSize );
		ErrorTime++;
		if(ErrorTime>1000)
		{
			CH_PUB_LockFlash(FlashOffset*128*1024);/*加保护*/
			CH_FlashUnLock();
			return true;
		}
		
	}while ( ReturnError != ST_NO_ERROR );
	CH_PUB_LockFlash(FlashOffset*128*1024);/*加保护*/
	CH_FlashUnLock();
      
	return false;
}


#ifdef STBID_FROM_E2PROM
/************************函数说明***********************************/
/*函数名:boolean CH_STBIDIsFromE2P(void)                                               */
/*开发人和开发时间:zengxianggen 2008-03-03                             */
/*函数功能描述:判断机号信息是否从E2PROM中获取*/
/*函数原理说明:                                                                              */
/*使用的全局变量、表和结构：                                         */
/*调用的主要函数列表：                                                         */
/*返回值说明： TRUE，机号信息从E2PROM中获取 ，
                                      FALSE,   机号信息从FLASH 中获取*/
                                      
boolean CH_STBIDIsFromE2P(void)    
{
    U8 uc_Tag;
    U8 uc_Sum1;
    U8 uc_Sum2;
    U8 uc_StbID[8];
    U8 uc_iStbID[8];
    	
    int i;
    /*首先判断标志位*/
     ReadNvmData(START_E2PROM_STBID,1,&uc_Tag);
    if(uc_Tag !=0xA5)
    	{
           return false;
    	}
    /*判断机号效验和*/
    ReadNvmData(START_E2PROM_STBID+1,8,uc_StbID);  
    uc_Sum1 = 0;
    for(i =0;i < 8;i++)
    	{
         uc_Sum1 +=  uc_StbID[i];
    	}	
    ReadNvmData(START_E2PROM_STBID+9,1,&uc_Sum2);   
    if(uc_Sum1 != uc_Sum2)
    	{
            return false;
    	}
	
   /*判断数据有效性*/

	 for (i=0;i<8;i++)
       {
          uc_iStbID[i] = uc_StbID[i];
       }

       /*判断数据的有效性*/
      if(  uc_iStbID[7]==0x12       /*厂商ID changhong 0x12(CA厂家分配)*/
	   &&uc_iStbID[6]==0x04/*fj 20080312 mod 3-->7*/  /*机顶盒型号0x00~0x03(changhong定义)DVB-C2800B=0x03*/
	   &&uc_iStbID[0]==0x00                     /*歌华运营商ID(CA厂家分配)*/
	)
    	{
          return true;
    	}
	else
	{
            return false;
	}

}

/************************函数说明***********************************/
/*函数名:void CH_EanbleSTBIDFromE2P( void )                                             */
/*开发人和开发时间:zengxianggen 2008-03-03                            */
/*函数功能描述:设置STBID从E2PROM中获取                       */
/*函数原理说明:                                                                             */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                       */
/*返回值说明：无                                                                       */

void CH_EanbleSTBIDFromE2P( void )  
{
	U8 uc_EnbaleTag = 0xA5;
	WriteNvmData(START_E2PROM_STBID, 1,&uc_EnbaleTag);
}


/************************函数说明***********************************/
/*函数名:void CH_DisableSTBIDFromE2P( void )                                         */
/*开发人和开发时间:zengxianggen 2008-03-03                           */
/*函数功能描述:设置STBID不从E2PROM中获取                 */
/*函数原理说明:                                                                            */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                       */
/*返回值说明：无                                                                        */

void CH_DisableSTBIDFromE2P( void )  
{
	U8 uc_DisbaleTag = 0x00;
	WriteNvmData(START_E2PROM_STBID, 1,&uc_DisbaleTag);
}


/************************函数说明***********************************/
/*函数名:void CH_ReadSTBIDFromE2P( U8 *uc_StbID )                              */
/*开发人和开发时间:zengxianggen 2008-03-03                           */
/*函数功能描述:从E2PROM中读取STBID                                 */
/*函数原理说明:                                                                            */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                       */
/*返回值说明：无                                                                        */
void CH_ReadSTBIDFromE2P( U8 *uc_StbID ) 
{
       ReadNvmData(START_E2PROM_STBID+1,8,uc_StbID);  
 
}

/************************函数说明***********************************/
/*函数名:void CH_WriteSTBIDToE2P( U8 *uc_StbID )                              */
/*开发人和开发时间:zengxianggen 2008-03-03                           */
/*函数功能描述:从E2PROM中读取STBID                                 */
/*函数原理说明:                                                                            */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                       */
/*返回值说明：无                                                                        */
void CH_WriteSTBIDToE2P( U8 *uc_StbID ) 
{
       U8 uc_Sum = 0;
	int i;
	for(i =0 ;i < 8 ;i++)
		{
                 uc_Sum += uc_StbID[i];
		}
       WriteNvmData(START_E2PROM_STBID+1,8,uc_StbID);
	WriteNvmData(START_E2PROM_STBID+9,1,&uc_Sum);
	
}



#endif

boolean CH_CheckdataValid(void)
{
    if(pstBoxInfo->SDVideoOutputType != TYPE_CVBS||
	pstBoxInfo->HDVideoOutputType  != TYPE_YUV ||
	pstBoxInfo->HDDigitalVideoOutputType != TYPE_HDMI||
	pstBoxInfo->SDVideoTimingMode != MODE_576I50)
    	{
                 return false;
    	}
	else
	     return true;
}
#ifdef SUMA_SECURITY


boolean CH_ReadSecurityData(U32 adr,U32 len,void* buf)
{
	int i =10;
	int ret = -1;

	
	while(i>0)
	{
		if(ReadNvmData(adr,len,buf) == 0)
		{
			ret = 0;
			break;
		}
		else
		{
			i--;
			task_delay(1000);
		}

	}

	if(ret == 0)
	{
		return FALSE;	
	}
	else
	{
		do_report(0," ERR!!!ReadNvmData ERR\n");
		return TRUE;
	
	}
	

}
boolean CH_WriteSecurityData(U32 adr,U32 len,void* buf)
{
	int i =10;
	int ret = -1;


	while(i>0)
	{
		if(WriteNvmData(adr,len,buf) == 0)
		{
			ret = 0;
			break;
		}
		else
		{
			i--;
			task_delay(1000);
		}

	}
	
	if(ret == 0)
	{
		return FALSE;
		
	}
	else
	{
		do_report(0,"ERR!!!CH_WriteSecurityData err\n");
		return TRUE;
	}

}
U8 CH_GetSecurityFlashStatus(void)
{
	U8 a;
	ReadNvmData(START_E2PROM_SECURITYFLASHDATA,1,&a);  
	return a;

}
void CH_SetSecurityFlashStatus( U8 val)
{
	int  a = val;
	WriteNvmData(START_E2PROM_SECURITYFLASHDATA,1,&a);
}
U8 CH_GetSpareSecurityFlashStatus(void)
{
	U8 a;
	ReadNvmData(START_E2PROM_SECURITYFLASHDATA+1,1,&a);  
	return a;

}

void CH_SetSpareSecurityFlashStatus( U8 val)
{
	int  a = val;
	WriteNvmData(START_E2PROM_SECURITYFLASHDATA+1,1,&a);
}


#endif
