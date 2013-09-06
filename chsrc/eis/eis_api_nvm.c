/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_nvm.c
  * 描述: 	实现永久存储相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-23
  * ===================================================================================
  */

#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"
#include "eis_api_nvm.h"
#include "../main/initterm.h"

#include "eis_include\Ipanel_nvram.h"
#ifdef SUMA_SECURITY
#include "..\ch_security\stb_tsm.h"
extern semaphore_t *p_SumaMemorySema; 
U8 SumaSecurity_write_flash ( U32 ru32_FlashAddr, U32 ru32_WriteSize, U8 *rp_Data );
#endif

#ifdef __EIS_API_DEBUG_ENABLE__
#define __EIS_API_NVM_DEBUG__
#endif


typedef enum eisflashcmd
{
	EIS_FLASH_ERASE,
	EIS_FLASH_WRITE,
#ifdef SUMA_SECURITY
	SUMA_FLASH_WRITE,
#endif
	EIS_FLASH_NONE
}EIS_FLASH_CMD;


typedef struct eisflash
{ 
    EIS_FLASH_CMD                       eis_cmd;
    U32                       i_flashAddr;
    U8                      *puc_EisFlashRamData;
    U32                      eis_flashsize;	
}CH_Eis_FlashCmd;

#define CH_FLASH_MIRROR
/*#define SKIN_FUNCTION*/

#ifdef CH_FLASH_MIRROR
static BYTE_T *mirror_basic=NULL;
static BYTE_T *mirror_appmgr=NULL;
static BYTE_T *mirror_third=NULL;
static BYTE_T *mirror_quick=NULL;
#endif

message_queue_t  *gp_EisCmd_Message = NULL;    


INT32_T eis_Nvm_buf_len;/* 记录IPANEL数据将要写入的长度*/
static U8 eis_flash_write_or_erase=EIS_FLASH_NONE;/* 记录IPANEL数据动作*/
	
static semaphore_t *gpsema_eis_nvm	= NULL;	/* 用于nvm内存互斥 */

static INT32_T eis_flash_state=IPANEL_NVRAM_UNKNOWN;/* 记录FLASH操作状态*/

static void Eis_NVM_Monitor(void *ptr);
#ifdef CH_FLASH_MIRROR
int EIS_Get_ActFALSHAddr(unsigned int mirrie_addr)
{
	if((unsigned int)mirror_basic == mirrie_addr)
	{
		eis_report("\n EIS_FLASH_OFFSET_BASIC");
		return (EIS_FLASH_OFFSET_BASIC);
	}
	else
	if((unsigned int)mirror_appmgr == mirrie_addr)
	{
		return (EIS_FLASH_OFFSET_APPMGR);
	}
	else
	if((unsigned int)mirror_third == mirrie_addr)
	{
		eis_report("\n EIS_FLASH_OFFSET_THIRD_PART");
		return (EIS_FLASH_OFFSET_THIRD_PART );
	}
	else
	if((unsigned int)mirror_quick == mirrie_addr)
	{
		eis_report("\n EIS_FLASH_OFFSET_QUICK");
		return (EIS_FLASH_OFFSET_QUICK);
	}
	else
		{
		eis_report("\n FLASH addr find failed");
		return 0;
		}
}
#endif

static void Eis_NVM_Monitor(void *ptr);

/*
  * Func: ipanel_porting_term_nvm
  * Desc: 销毁nvm用到的镜像内存
  */
void ipanel_porting_term_nvm ( void )
{
	if ( gpsema_eis_nvm )
	{
		semaphore_delete ( gpsema_eis_nvm );
		gpsema_eis_nvm = NULL;
	}
}

boolean ipanel_porting_init_nvm ( void )
{
	message_queue_t  *pstMessageQueue = NULL;	

	gpsema_eis_nvm = semaphore_create_fifo(1);

	eis_flash_write_or_erase=EIS_FLASH_NONE;
    /*创建消息队列*/
	if (( pstMessageQueue = message_create_queue_timeout ( 20,50 ) ) == NULL )
   	{
             return true;
	 }
   	else
   	{
		symbol_register (	"EisCmd_Message", (	void * ) pstMessageQueue );
   	}

	if ( symbol_enquire_value ( "EisCmd_Message", ( void ** ) &gp_EisCmd_Message ) )
	{
		STTBX_Print(("Cant find EisCmd_Message message queue\n" ));
		return true;
	}
	
	#ifdef CH_FLASH_MIRROR
	CH_FlashLock();
	mirror_basic=(BYTE_T *)memory_allocate(CHSysPartition,EIS_FLASH_BASIC_SIZE);
	if(mirror_basic!=NULL)
	{
		memcpy ( mirror_basic, (void*)(EIS_FLASH_OFFSET_BASIC+EIS_FLASH_BASE_ADDR), EIS_FLASH_BASIC_SIZE );
	}
	else
	{
		CH_FlashUnLock();
		return true;
	}
	
	mirror_appmgr=(BYTE_T *)memory_allocate(CHSysPartition,EIS_FLASH_APPMGR_SIZE);
	if(mirror_appmgr!=NULL)
	{
		memcpy ( mirror_appmgr, (void*)(EIS_FLASH_OFFSET_APPMGR+EIS_FLASH_BASE_ADDR), EIS_FLASH_APPMGR_SIZE );
	}
	else
	{
		CH_FlashUnLock();
		return true;
	}
	
	mirror_third=(BYTE_T *)memory_allocate(CHSysPartition,EIS_FLASH_THIRDPART_SIZE);
	if(mirror_third!=NULL)
	{
		memcpy ( mirror_third, (void*)(EIS_FLASH_OFFSET_THIRD_PART+EIS_FLASH_BASE_ADDR), EIS_FLASH_THIRDPART_SIZE );
	}
	else
	{
		CH_FlashUnLock();
		return true;
	}
	
	mirror_quick=(BYTE_T *)memory_allocate(CHSysPartition,EIS_FLASH_QUICK_SIZE);
	if(mirror_quick!=NULL)
	{
		memcpy ( mirror_quick, (void*)(EIS_FLASH_OFFSET_QUICK+EIS_FLASH_BASE_ADDR), EIS_FLASH_QUICK_SIZE );
	}
	else
	{
		CH_FlashUnLock();
		return true;
	}

	CH_FlashUnLock();
	
	#endif
	
	if ( ( Task_Create( Eis_NVM_Monitor, NULL,1024*20, 7,"Eis_DVM_Monitor", 0 ) ) == NULL )
	{
		eis_report ( "\n Eis_DVM_Monitor creat failed\n");
		return true;
	}
	
	return false;
}
/*
  * Func: ipanel_porting_erase_flash
  * Desc: 擦除FLASH
  * In:	ru32_FlashAddr 	起始偏移量
  		ru32_EraseSize	擦除空间的长度
  */
  /*返回值0:未等到信号量 ，1 成功 ，2失败*/
U8 ipanel_porting_erase_flash ( U32 ru32_FlashAddr, U32 ru32_EraseSize )
{
	U32 	u32_ActOffset;
	int 	u32_ActLen;
	U8	u8_Status;
	int	i_OffsetTmp;
	int	i_Try;
	int	i_Error;
	/* 1. 参数检查 */
	/* 1.1 对其块数 */
	if ( NULL==gpsema_eis_nvm )
	{
		eis_report ( "\n++>eis ipanel_porting_nvram_read failed" );
		return  0;
	}

#ifdef CH_FLASH_MIRROR
#ifdef SKIN_FUNCTION
	if((ru32_FlashAddr >= (EIS_FLASH_OFFSET_SKIN+EIS_FLASH_BASE_ADDR)) && (ru32_FlashAddr < (EIS_FLASH_OFFSET_SKIN+EIS_FLASH_BASE_ADDR+EIS_FLASH_SKIN_SIZE) ) )
	{
		u32_ActOffset = ru32_FlashAddr -EIS_FLASH_BASE_ADDR;
	}
	else
#endif		
	u32_ActOffset =EIS_Get_ActFALSHAddr(ru32_FlashAddr);
#else
	u32_ActOffset = ru32_FlashAddr -EIS_FLASH_BASE_ADDR;
#endif

	if(u32_ActOffset==0)
	{
		return 0;
	}

	
	u32_ActLen	= ru32_EraseSize ;
	
	u8_Status = IPANEL_FLASH_STATUS_ERASING;

	i_OffsetTmp = u32_ActOffset;
	CH_FlashLock();
	while ( u32_ActLen > 0 )
	{
#ifndef SKIN_FUNCTION
       		if((i_OffsetTmp>=EIS_FLASH_OFFSET_BASIC)&&(i_OffsetTmp<EIS_FLASH_MAX_ADDR))
#endif
			{
		     i_Try = 1000;
		       do
		      {   
		        CH_PUB_UnLockFlash(i_OffsetTmp);
			i_Error = CHFLASH_Erase ( FLASHHandle, i_OffsetTmp, EIS_FLASH_BLOCK_SIZE );
				i_Try --;

		   	}        while ( i_Error != 0 &&  i_Try > 0);
		
			eis_report ( "\n++>eis erase1 [0x%x] <%s>", i_OffsetTmp, GetErrorText(i_Error) );
		
		
                    CH_PUB_LockFlash(i_OffsetTmp);
       		}
		
			if ( i_Try <= 0 )
			{
				break;
			}
		
		      i_OffsetTmp 	+= EIS_FLASH_BLOCK_SIZE;
		      u32_ActLen	-= EIS_FLASH_BLOCK_SIZE;
	}
	CH_FlashUnLock();

	if ( i_Try <= 0 )
	{
		u8_Status = IPANEL_FLASH_STATUS_FAIL;
		return 2;
	}
	else
	{
		u8_Status = IPANEL_FLASH_STATUS_OK;
		return 1;
	}
}

/*
  * Func: ipanel_porting_write_flash
  * Desc: 擦除FLASH
  * In:	ru32_FlashAddr 	起始偏移量
  		ru32_WriteSize	写入空间的长度
  		rp_Data			写入数据的缓冲指针
  */
  /*返回值0:未等到信号量 ，1 成功 ，2失败*/
U8 ipanel_porting_write_flash ( U32 ru32_FlashAddr, U32 ru32_WriteSize, U8 *rp_Data )
{
	U32 	u32_ActOffset;
	int 	u32_ActLen ;
	U8	u8_Status;
	int	i_OffsetTmp;
	int	i_Try;
	int	i_Error;
	int	i_ActWrite;
	U8   *puc_ActualBlockData= NULL;
	int   i_BlockWriteSize;
	eis_report ( "\n ipanel_porting_write_flash  " );
	/* 1. 参数检查 */
	/* 1.1 对其块数 */

#ifdef CH_FLASH_MIRROR
#ifdef SKIN_FUNCTION
	if((ru32_FlashAddr >= (EIS_FLASH_OFFSET_SKIN+EIS_FLASH_BASE_ADDR)) && (ru32_FlashAddr < (EIS_FLASH_OFFSET_SKIN+EIS_FLASH_BASE_ADDR+EIS_FLASH_SKIN_SIZE) ) )
	{
		u32_ActOffset = ru32_FlashAddr -EIS_FLASH_BASE_ADDR;
	}
	else
#endif	
	u32_ActOffset =EIS_Get_ActFALSHAddr(ru32_FlashAddr);
#else
	u32_ActOffset = ru32_FlashAddr -EIS_FLASH_BASE_ADDR;
#endif


	u32_ActLen	= ru32_WriteSize;

	if(u32_ActOffset==0)
	{
		return 0;
	}
	
	u8_Status= IPANEL_FLASH_STATUS_WRITING;

	i_OffsetTmp = u32_ActOffset;
	
	 CH_FlashLock();
	 
	while ( u32_ActLen > 0 )
	{
                boolean b_NeedUpadate  = false;
     
	           i_Try =1000;
          /*当前块写入的数据大小*/
		  i_BlockWriteSize = (u32_ActLen>=EIS_FLASH_BLOCK_SIZE)?EIS_FLASH_BLOCK_SIZE:u32_ActLen;
		  
	         /*首先比较原来的数据和希望写入的数据是否变化*/
                if(puc_ActualBlockData  == NULL)
                {
                   puc_ActualBlockData = memory_allocate(CHSysPartition, EIS_FLASH_BLOCK_SIZE);
		   
                }
	              if(puc_ActualBlockData != NULL)
		    	{
		    		#ifdef CH_FLASH_MIRROR
					memcpy(puc_ActualBlockData,(U8 *)(u32_ActOffset+EIS_FLASH_BASE_ADDR+(ru32_WriteSize-u32_ActLen)),EIS_FLASH_BLOCK_SIZE);
				#else
					 memcpy(puc_ActualBlockData,ru32_FlashAddr+(ru32_WriteSize-u32_ActLen),EIS_FLASH_BLOCK_SIZE);
				#endif
			   if(memcmp(puc_ActualBlockData,rp_Data+(ru32_WriteSize-u32_ActLen),i_BlockWriteSize) == 0)/* 相同的数据内容，不需要更新*/
			   {
					 eis_report ( "same flash address[%x],size[%d]\n ", ru32_FlashAddr,i_BlockWriteSize);
			   }
			  else
			  {
				        memcpy(puc_ActualBlockData,rp_Data+(ru32_WriteSize-u32_ActLen),i_BlockWriteSize);

					eis_report ( "Need update  flash address[%x],size[%d]\n ", ru32_FlashAddr,i_BlockWriteSize);
					b_NeedUpadate = true;
			  }
			   
		    	}
		  if(b_NeedUpadate == true)
		  {
#ifndef SKIN_FUNCTION
		  	 if((i_OffsetTmp>=EIS_FLASH_OFFSET_BASIC)&&(i_OffsetTmp<EIS_FLASH_MAX_ADDR))
#endif
			 {
		     i_Try =1000;
		      do
		    {
		        CH_PUB_UnLockFlash(i_OffsetTmp);
			i_Error = CHFLASH_Erase ( FLASHHandle, i_OffsetTmp, EIS_FLASH_BLOCK_SIZE );

			i_Try--;
				}while ( i_Error != 0 && i_Try > 0);
			
			i_Try =1000;
		      do		  
		      {
			i_Error = CHFLASH_Write ( FLASHHandle, i_OffsetTmp, 
									/*rp_Data+(ru32_WriteSize-u32_ActLen)*/puc_ActualBlockData, 
									EIS_FLASH_BLOCK_SIZE/*(u32_ActLen>=EIS_FLASH_BLOCK_SIZE)?EIS_FLASH_BLOCK_SIZE:u32_ActLen*/, 
									&i_ActWrite );
			i_Try--;
			 }while ( i_Error != 0 &&  i_Try > 0);
	
            	     CH_PUB_LockFlash(i_OffsetTmp);
			}
		      if ( i_Try <= 0 )
			{
				break;
			}
		  }
		i_OffsetTmp 	+= EIS_FLASH_BLOCK_SIZE;
		u32_ActLen	-= EIS_FLASH_BLOCK_SIZE;
	}
	
	CH_FlashUnLock();
       if(puc_ActualBlockData != NULL)
       {
               memory_deallocate(CHSysPartition, puc_ActualBlockData);
       }
	if ( i_Try <= 0 )
	{
		u8_Status = IPANEL_FLASH_STATUS_FAIL;
		return 2;
	}
	else
	{
		u8_Status= IPANEL_FLASH_STATUS_OK;
		return 1;
	}
}

/* EIS异步写FLASH进程*/
static void Eis_NVM_Monitor(void *ptr)
{
	int i;
	clock_t timeout;
	CH_Eis_FlashCmd *msg_p ;
	while(1)
	{
	       timeout = time_plus(time_now(), ST_GetClocksPerSecond()*5);		
		msg_p =	(CH_Eis_FlashCmd *)message_receive_timeout(gp_EisCmd_Message,&timeout);
	      if(msg_p != NULL)
	      	{
			      semaphore_wait(gpsema_eis_nvm);
	           
	                  if(msg_p->eis_cmd ==  EIS_FLASH_ERASE)
	                  	{
				      if(ipanel_porting_erase_flash(msg_p->i_flashAddr,msg_p->eis_flashsize)==1)
					{
						eis_flash_write_or_erase = EIS_FLASH_NONE;
						eis_flash_state = IPANEL_NVRAM_SUCCESS;
					} 	
					else
					{
						eis_flash_write_or_erase = EIS_FLASH_NONE;
						eis_flash_state = IPANEL_NVRAM_FAILED;
					}
	                  	}
	                  else if(msg_p->eis_cmd  == EIS_FLASH_WRITE)
	                  	{
					if(ipanel_porting_write_flash(msg_p->i_flashAddr,msg_p->eis_flashsize,msg_p->puc_EisFlashRamData)==1)
					{
						eis_flash_write_or_erase = EIS_FLASH_NONE;
						eis_flash_state = IPANEL_NVRAM_SUCCESS;
					}	
					else
					{
						eis_flash_write_or_erase = EIS_FLASH_NONE;
						eis_flash_state = IPANEL_NVRAM_FAILED;
					}
	                  	}
#ifdef SUMA_SECURITY
				   else if(msg_p->eis_cmd  == SUMA_FLASH_WRITE)
	                  	{
						if(SumaSecurity_write_flash(msg_p->i_flashAddr,msg_p->eis_flashsize,msg_p->puc_EisFlashRamData)==1)
							{
							if(msg_p->i_flashAddr == FLASH_SECURITY_ADDR)
							{
								CH_SetSecurityFlashStatus(0x8A);
							}
							else if(msg_p->i_flashAddr == FLASH_SECURITY_ADDR_SPARE)
							{
								CH_SetSpareSecurityFlashStatus(0x8A);

							}
						
							}	
						else
							{
							do_report(0,"SUMA Write flash error\n");

							}
	                  	}
#endif

				semaphore_signal(gpsema_eis_nvm);
				
			#ifndef CH_FLASH_MIRROR 
			if(msg_p->puc_EisFlashRamData != NULL)
			{
		              memory_deallocate(CHSysPartition, msg_p->puc_EisFlashRamData);
				msg_p->puc_EisFlashRamData  = NULL;
			}
			#endif
			
			message_release(gp_EisCmd_Message,msg_p);
	      	}
		else
		{
               
		}
	}
}

/*
功能说明：
	获得提供给上层的NVRAM(通常就是flash)内存区的起始地址，块数和每块的大小。
注意：IPANEL_NVRAM_DATA_BASIC和IPANEL_NVRAM_DATA_QUICK类数据的存储空间是
	必须提供的。其他两种数据存储空间如果ipanel调用该函数没有获取到，相应的功能就
	不会实现。
参数说明：
	输入参数：
		flag: 要获取的数据类型
		typedef enum
		{
			IPANEL_NVRAM_DATA_BASIC, / 基本NVRAM /
			IPANEL_NVRAM_DATA_SKIN, / Skin专用 /
			IPANEL_NVRAM_DATA_THIRD_PART, / 第三方共用 /
			IPANEL_NVRAM_DATA_QUICK, / 立即写入的NVRAM /
			IPANEL_NVRAM_DATA_UNKNOWN
		} IPANEL_NVRAM_DATA_TYPE_e;
		IPANEL_NVRAM_DATA_BASIC - 表示返回ipanel存储通用数据的NVRAM的起始
		地址、块个数和每个块大小。
		IPANEL_NVRAM_DATA_SKIN - 表示返回供skin文件存储的NVRAM的起始地址、
		块个数和每个块的大小。
		IPANEL_NVRAM_DATA_THIRD_PART - 表示返回供第三方存储的NVRAM的起始地
		址、块个数和每个块的大小。
		IPANEL_NVRAM_DATA_QUICK - 表示返回供ipanel存储少量但是变化频率很高
		的数据的NVRAM的起始地址、块个数和每个块的大小。
	输出参数：
		**addr：FLASH空间的起始地址；
		*sect_num：FLASH空间的块数
		*sect_size：FLASH的每块大小。
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */
INT32_T ipanel_porting_nvram_info(BYTE_T **addr, INT32_T *sect_num, INT32_T *sect_size, INT32_T flag)
{

#ifdef EIS_API_NVM_DEBUG
	eis_report ( "++>eis ipanel_porting_nvram_info flag=%d", flag );
#endif

	switch ( flag )
	{
	case IPANEL_NVRAM_DATA_BASIC: 			/*基本VRAM */
		*sect_num 	= EIS_FLASH_BASIC_SIZE/EIS_FLASH_BLOCK_SIZE;
		*sect_size	= EIS_FLASH_BLOCK_SIZE;
		
		#ifdef CH_FLASH_MIRROR
		if(mirror_basic==NULL)
		{
			return IPANEL_ERR;
		}
		else
		{
			*addr = mirror_basic;
		}
		#else
		*addr		= (BYTE_T *)(EIS_FLASH_OFFSET_BASIC+EIS_FLASH_BASE_ADDR)/*gpsta_eis_BasicNvmMirror*/;
		#endif
		
		eis_report ( "++>addr=0x%x,sect_size=%d ,sect_num=%d", *addr, *sect_size,*sect_num);	
		break;
	case IPANEL_NVRAM_DATA_APPMGR: 
		*sect_num 	= EIS_FLASH_APPMGR_SIZE/EIS_FLASH_BLOCK_SIZE;
		*sect_size	= EIS_FLASH_BLOCK_SIZE;
		
		#ifdef CH_FLASH_MIRROR
		if(mirror_appmgr==NULL)
		{
			return IPANEL_ERR;
		}
		else
		{
			*addr = mirror_appmgr;
		}
		#else
			*addr= (U8*)(EIS_FLASH_OFFSET_APPMGR+EIS_FLASH_BASE_ADDR);
		#endif

		eis_report ( "++>addr=0x%x,sect_size=%d sect_num=%d", *addr, *sect_size,*sect_num);
		break;
		
	case IPANEL_NVRAM_DATA_THIRD_PART:/* 第三方共用 */
		*sect_num 	= EIS_FLASH_THIRDPART_SIZE/EIS_FLASH_BLOCK_SIZE;
		*sect_size	= EIS_FLASH_BLOCK_SIZE;
		#ifdef CH_FLASH_MIRROR
		if(mirror_third==NULL)
		{
			return IPANEL_ERR;
		}
		else
		{
			*addr = mirror_third;
		}
		#else
		*addr		= (U8*)(EIS_FLASH_OFFSET_THIRD_PART+EIS_FLASH_BASE_ADDR)/*gpsta_eis_APPMGRNvmMirror*/;
		#endif

		eis_report ( "++>addr=0x%x,sect_size=%d sect_num=%d", *addr, *sect_size,*sect_num);
		break;
	case IPANEL_NVRAM_DATA_AUX:/* 立即写入的NVRAM */
		*sect_num 	= EIS_FLASH_QUICK_SIZE/EIS_FLASH_BLOCK_SIZE;
		*sect_size	= EIS_FLASH_BLOCK_SIZE;
		#ifdef CH_FLASH_MIRROR
		if(mirror_quick==NULL)
		{
			return IPANEL_ERR;
		}
		else
		{
			*addr = mirror_quick;
		}
		#else
		*addr = (U8*)(EIS_FLASH_OFFSET_QUICK+EIS_FLASH_BASE_ADDR)/*gpsta_eis_APPMGRNvmMirror*/;
		#endif

		eis_report ( "++>addr=0x%x,sect_size=%d sect_num=%d", *addr, *sect_size,*sect_num);
		break;
#ifdef SKIN_FUNCTION
		case IPANEL_NVRAM_DATA_SKIN:/* 立即写入的NVRAM */
			*sect_num 	= EIS_FLASH_SKIN_SIZE/EIS_FLASH_BLOCK_SIZE;
			*sect_size	= EIS_FLASH_BLOCK_SIZE;

			*addr = (U8*)(EIS_FLASH_OFFSET_SKIN+EIS_FLASH_BASE_ADDR)/*gpsta_eis_APPMGRNvmMirror*/;

			eis_report ( "++>addr=0x%x,sect_size=%d sect_num=%d", *addr, *sect_size,*sect_num);
		break;
#endif		
	default:
		{
		*sect_num 	= 0;
		*sect_size	= 0;
		*addr		= NULL;	
		return IPANEL_ERR;
			}
		break;
	}
#ifdef EIS_API_NVM_DEBUG
	eis_report ( "++>eis ipanel_porting_nvram_info end" );
#endif

	return IPANEL_OK;
}

/*
功能说明：
	从NVRAM (通常就是flash)中读取指定的字节数到数据缓存区中，实现时注意操作
	空间越界。如果读取的起始地址超出了有效地址范围，返回IPANEL_ERR；如果读取的
	内容地址空间范围超过有效空间，则buffer中返回有效空间的数据，函数返回值指示
	实际读取的数字长度。
参数说明：
	输入参数：
		flash_addr：想要读取的目标Flash Memory的起始地址；
		buf_addr：保存读取数据的缓冲区。
		len：想要读取的字节数。
	输出参数：
		buf_addr：读取的数据。
	返 回：
		>＝ 0:实际读取的数据长度;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_nvram_read ( UINT32_T flash_addr, BYTE_T *buf_addr, INT32_T len )
{
#ifdef EIS_API_NVM_DEBUG
	eis_report ( "\n++>eis ipanel_porting_nvram_read flash_addr=%x, buf_addr=%x, len=%d" ,flash_addr,buf_addr,len);
#endif
	if((0==flash_addr)||(NULL==buf_addr)||(len==0))
		return IPANEL_ERR;
	
	if ( NULL==gpsema_eis_nvm )
	{
		eis_report ( "\n++>eis ipanel_porting_nvram_read failed" );
		return IPANEL_ERR;
	}
       semaphore_wait(gpsema_eis_nvm);

	memcpy ( buf_addr, (void*)(flash_addr), len );
	
	semaphore_signal ( gpsema_eis_nvm );
#ifdef EIS_API_NVM_DEBUG
	eis_report ( "++>eis ipanel_porting_nvram_read end" );
#endif

	return len;
}

/*
功能说明：
	将数据写入NVRAM(通常就是flash)中，有两种不同的操作模式：后台写
	（IPANEL_NVRAM_BURN_DELAY）和即时写（IPANEL_NVRAM_BURN_NOW）模
	式。
	实现时注意操作空间越界问题，对于后台写入模式，建议使用新的线程来同步，并
	且该函数必须立即返回，不能阻塞，在实际处理时，通常会建立一个镜像内存区，先
	将数据写到内存镜像去，立即返回，然后后台进程/线程将镜像内存区中的数据写到
	NVRAM中。后台写入的flash空间的擦除操作由底层负责。
	对于即时写入模式，需要在接口中立即完成写入操作，不能异步实现。及时写入模
	式的flash空间的擦除操作由ipanel负责。
注意：此函数操作的地址和数据长度是以字节为单位的，对于字模式操作的flash，
		请注意保护数据整性。
		1、 如果起始地址是奇地址，第一个字节必须前补0xFF后形成字再写入flash。
		2、 如果结束地址是奇地址，最后一个字节必须后补0xFF形成字在写入flash。
		3、 为了优化执行效率，用户可以重新开辟数据区（对少量数据合适）或者将首尾
			奇地址字节单独处理（对大量数据合适）
参数说明：
	输入参数：
		flash_addr：想要写入的目标起始地址；
		buf_addr：写入数据块的起始地址。
		len：想要写入的字节数。
		mode：写入操作模式
		typedef enum
		{
			IPANEL_NVRAM_BURN_DELAY,
			IPANEL_NVRAM_BURN_NOW
		}IPANEL_NVRAM_BURN_MODE_e;
	输出参数：无
	返 回：
		>＝ 0:实际写入的数据长度;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_nvram_burn(UINT32_T flash_addr, const CHAR_T *buf_addr, INT32_T len,IPANEL_NVRAM_BURN_MODE_e mode)
{
#ifdef EIS_API_NVM_DEBUG
	eis_report ( "\n++>eis ipanel_porting_nvram_burn flash_addr=%x, buf_addr=%x, len=%d" ,flash_addr,buf_addr,len);
#endif
      clock_t timeout;
      CH_Eis_FlashCmd *msg_p;

	 if((0==flash_addr)||(NULL==buf_addr)||(len==0))
		return IPANEL_ERR;
#ifndef SKIN_FUNCTION
	if(len>(EIS_FLASH_BLOCK_SIZE*8))
		return IPANEL_ERR;

#else	
	if(IPANEL_NVRAM_BURN_NOW == mode)
	{
		if( ipanel_porting_write_flash(flash_addr,len,buf_addr) == 1)
		{
			return len;
		}
		else
		{
			return 0;
		}
	}
#endif	
	if ( NULL==gpsema_eis_nvm )
	{
		eis_report ( "\n++>eis ipanel_porting_nvram_burn failed" );
		return IPANEL_ERR;
	}
	
       semaphore_wait(gpsema_eis_nvm);
#ifdef CH_FLASHWRITE_SYN
	eis_flash_state = IPANEL_NVRAM_BURNING;
	if( ipanel_porting_write_flash(flash_addr,len,buf_addr) == 1)
	{
		eis_flash_state = IPANEL_NVRAM_SUCCESS;
	}
	else
	{
		eis_flash_state = IPANEL_NVRAM_FAILED;
	}
#else
	eis_flash_write_or_erase = EIS_FLASH_WRITE;/* 记录操作类型*/

	  
      timeout = time_plus(time_now(), ST_GetClocksPerSecond()*3);		
      msg_p =	(CH_Eis_FlashCmd *)message_claim_timeout(gp_EisCmd_Message,&timeout);
      if(msg_p!=NULL)
	{
		msg_p->eis_cmd = EIS_FLASH_WRITE;
		msg_p->eis_flashsize = len;
		msg_p->i_flashAddr = flash_addr;

#ifdef CH_FLASH_MIRROR
		msg_p->puc_EisFlashRamData=(U8 *)flash_addr;
#else
		msg_p->puc_EisFlashRamData = memory_allocate(CHSysPartition,len);
#endif
		if(msg_p->puc_EisFlashRamData != NULL)
		{
			eis_flash_state = IPANEL_NVRAM_BURNING;

			memcpy(msg_p->puc_EisFlashRamData,buf_addr,len);
			message_send(gp_EisCmd_Message,msg_p);
		}
	} 
#endif
	semaphore_signal ( gpsema_eis_nvm );
#ifdef EIS_API_NVM_DEBUG
	eis_report ( "++>eis ipanel_porting_nvram_burn end" );
#endif
	
	return IPANEL_OK;
}

/*
功能说明：
	擦除指定地址的内容，同步实现擦除操作。
参数说明：
	输入参数：
		flash_addr：擦除块的起始地址；
		len：擦除空间的长度。
	输出参数：
		无
	返 回：
		IPANEL_OK:擦除成功;
		IPANEL_ERR:擦除失败。
*/
INT32_T ipanel_porting_nvram_erase(UINT32_T flash_addr, INT32_T len)
{
      clock_t timeout;
      CH_Eis_FlashCmd *msg_p;

#ifdef EIS_API_NVM_DEBUG
	eis_report ( "\n++>eis ipanel_porting_nvram_erase flash_addr=%x" ,flash_addr);
#endif
	if((EIS_FLASH_OFFSET_THIRD_PART+EIS_FLASH_BASE_ADDR)==flash_addr)
	{
		return IPANEL_OK;
	}
	
	if((0==flash_addr)||(len==0))
		return IPANEL_ERR;
	
#ifndef SKIN_FUNCTION
	if(len>(EIS_FLASH_BLOCK_SIZE*8))
		return IPANEL_ERR;
#else
	 if( ipanel_porting_erase_flash(flash_addr,len) == 1)
	 {
		return IPANEL_OK;
	 }
	 else
	 {
		return IPANEL_ERR;
	 }
#endif
	
	
	if ( NULL==gpsema_eis_nvm )
	{
		eis_report ( "\n++>eis ipanel_porting_nvram_erase failed" );
		return IPANEL_ERR;
	}

       semaphore_wait(gpsema_eis_nvm);
#ifdef CH_FLASH_MIRROR
	memset((U8 *)flash_addr,0,len);
#endif

 #ifdef CH_FLASHWRITE_SYN
	eis_flash_state = IPANEL_NVRAM_BURNING;
	if( ipanel_porting_erase_flash(flash_addr,len) == 1)
		eis_flash_state = IPANEL_NVRAM_SUCCESS;
	else
		eis_flash_state = IPANEL_NVRAM_FAILED;
	
#else
	eis_flash_write_or_erase=EIS_FLASH_ERASE;/* 记录操作类型*/

      timeout = time_plus(time_now(), ST_GetClocksPerSecond()*3);		
      msg_p =	(CH_Eis_FlashCmd *)message_claim_timeout(gp_EisCmd_Message,&timeout);
      if(msg_p!=NULL)
	{
		msg_p->eis_cmd = EIS_FLASH_ERASE;
		msg_p->eis_flashsize = len;
		msg_p->i_flashAddr = flash_addr;
		msg_p->puc_EisFlashRamData = NULL;
		eis_flash_state = IPANEL_NVRAM_BURNING;
		message_send(gp_EisCmd_Message,msg_p);

	} 
    #endif	  
	semaphore_signal ( gpsema_eis_nvm );
#ifdef EIS_API_NVM_DEBUG
	eis_report ( "++>eis ipanel_porting_nvram_erase end" );
#endif

	return IPANEL_OK;
}

/*
功能说明：
	在后台写入模式下，调用此函数判断真正的NVRAM(通常就是flash)的写入状态。
参数说明：
	输入参数：
		flash_addr – burn 的起始地址；
		len - 写入的字节数
	输出参数：无
	返 回：
		IPANEL_NVRAM_BURNING：正在写；
		IPANEL_NVRAM_SUCCESS：已经写成功；
		IPANEL_NVRAM_FAILED：写失败。
*/
INT32_T ipanel_porting_nvram_status(UINT32_T flash_addr, INT32_T len)
{
    int   i_status;
#ifdef EIS_API_NVM_DEBUG
	eis_report ( "\n ipanel_porting_nvram_status  " );
#endif
  	if ( NULL==gpsema_eis_nvm )
	{
		eis_report ( "\n++>eis ipanel_porting_nvram_status failed" );
		return IPANEL_ERR;
	}	
  semaphore_wait(gpsema_eis_nvm);
    i_status =  eis_flash_state;
    semaphore_signal ( gpsema_eis_nvm );
		
    return i_status;
}


#ifdef SUMA_SECURITY


/*
  * Func: ipanel_porting_write_flash
  * Desc: 擦除FLASH
  * In:	ru32_FlashAddr 	起始偏移量
  		ru32_WriteSize	写入空间的长度
  		rp_Data			写入数据的缓冲指针
  */
  /*返回值0:未等到信号量 ，1 成功 ，2失败*/
U8 SumaSecurity_write_flash ( U32 ru32_FlashAddr, U32 ru32_WriteSize, U8 *rp_Data )
{
	U32 	u32_ActOffset;
	int 	u32_ActLen;
	U8	u8_Status;
	int	i_OffsetTmp;
	int	i_Try =1000;
	int	i_Error;
	int	i_ActWrite;
	U8   *puc_ActualBlockData= NULL;
	int   i_BlockWriteSize;
	
	eis_report ( "\n ipanel_porting_write_flash  " );
	/* 1. 参数检查 */
	/* 1.1 对其块数 */

	u32_ActOffset = ru32_FlashAddr -EIS_FLASH_BASE_ADDR;
	
	u32_ActLen	= ru32_WriteSize;

	if(u32_ActOffset==0)
	{
		return 0;
	}
	
	u8_Status= IPANEL_FLASH_STATUS_WRITING;

	i_OffsetTmp = u32_ActOffset;
	
	semaphore_wait(p_SumaMemorySema);
	
	CH_FlashLock();
	
	while ( u32_ActLen > 0 )
	{
                boolean b_NeedUpadate  = false;
     
	           i_Try =1000;
          	/*当前块写入的数据大小*/
		  i_BlockWriteSize = (u32_ActLen>=EIS_FLASH_BLOCK_SIZE)?EIS_FLASH_BLOCK_SIZE:u32_ActLen;
		  
	         /*首先比较原来的数据和希望写入的数据是否变化*/
                if(puc_ActualBlockData  == NULL)
                {
                   	puc_ActualBlockData = memory_allocate(CHSysPartition, EIS_FLASH_BLOCK_SIZE);   
                }
	              if(puc_ActualBlockData != NULL)
		    {
#ifdef CH_FLASH_MIRROR
				memcpy(puc_ActualBlockData,(U8 *)(u32_ActOffset+EIS_FLASH_BASE_ADDR+(ru32_WriteSize-u32_ActLen)),EIS_FLASH_BLOCK_SIZE);
#else
				memcpy(puc_ActualBlockData,ru32_FlashAddr+(ru32_WriteSize-u32_ActLen),EIS_FLASH_BLOCK_SIZE);
#endif
			   if(memcmp(puc_ActualBlockData,rp_Data+(ru32_WriteSize-u32_ActLen),i_BlockWriteSize) == 0)/* 相同的数据内容，不需要更新*/
			   {
				eis_report ( "same flash address[%x],size[%d]\n ", ru32_FlashAddr,i_BlockWriteSize);						
			   }
			  else
			  {
				        memcpy(puc_ActualBlockData,rp_Data+(ru32_WriteSize-u32_ActLen),i_BlockWriteSize);

					eis_report ( "Need update  flash address[%x],size[%d]\n ", ru32_FlashAddr,i_BlockWriteSize);
					b_NeedUpadate = true;
			  }
			   
		    	}
		  if(b_NeedUpadate == true)
		  {
				 CH_PUB_UnLockFlash(i_OffsetTmp);
				 i_Try =1000;
		      		do
			    	{
					     
						i_Error = CHFLASH_Erase ( FLASHHandle, i_OffsetTmp, EIS_FLASH_BLOCK_SIZE );

						i_Try--;
				}while ( i_Error != 0 && i_Try > 0);
			
				i_Try =1000;
			      do		  
			      {
					i_Error = CHFLASH_Write ( FLASHHandle, i_OffsetTmp, 
											puc_ActualBlockData, 
											EIS_FLASH_BLOCK_SIZE, 
											&i_ActWrite );
					i_Try--;
				 }while ( i_Error != 0 &&  i_Try > 0);
				
            	 		CH_PUB_LockFlash(i_OffsetTmp);
			      if ( i_Try <= 0 )
				{
					break;
				}
		  }
		i_OffsetTmp 	+= EIS_FLASH_BLOCK_SIZE;
		u32_ActLen	-= EIS_FLASH_BLOCK_SIZE;
	}
	
	CH_FlashUnLock();
	
       if(puc_ActualBlockData != NULL)
       {
               memory_deallocate(CHSysPartition, puc_ActualBlockData);
       }
	  semaphore_signal(p_SumaMemorySema);
	if ( i_Try <= 0 )
	{
		u8_Status = IPANEL_FLASH_STATUS_FAIL;
		return 2;
	}
	else
	{
		u8_Status= IPANEL_FLASH_STATUS_OK;
		return 1;
	}
}


boolean  CH_Security_Flash_burn(UINT32_T flash_addr, const CHAR_T *buf_addr, INT32_T len)
{
	clock_t timeout;
	CH_Eis_FlashCmd *msg_p;

	if (  buf_addr == NULL || len <= 0)
	{
		return TRUE;
	}
	  
      timeout = time_plus(time_now(), ST_GetClocksPerSecond()*3);		
      msg_p =	(CH_Eis_FlashCmd *)message_claim_timeout(gp_EisCmd_Message,&timeout);
      if(msg_p!=NULL)
     	{
		msg_p->eis_cmd = SUMA_FLASH_WRITE;
		msg_p->eis_flashsize = len;
		msg_p->i_flashAddr = flash_addr;
#ifdef CH_FLASH_MIRROR
		msg_p->puc_EisFlashRamData=(U8 *)buf_addr;
#else
		msg_p->puc_EisFlashRamData = memory_allocate(CHSysPartition,len);
#endif
		if(msg_p->puc_EisFlashRamData != NULL)
		{
			memcpy(msg_p->puc_EisFlashRamData,buf_addr,len);
			message_send(gp_EisCmd_Message,msg_p);
		}
     	} 

#ifdef EIS_API_NVM_DEBUG
	eis_report ( "++>eis CH_Security_Flash_burn end" );
#endif
	
	return FALSE;
}


void CH_CheckSecurityDataStatus(void)
{
	char * tempbuf = NULL;
	if(CH_GetSecurityFlashStatus() != 0X8A)
	{
		if(CH_GetSpareSecurityFlashStatus() == 0x8A)/*备份数据有效*/
		{

			tempbuf = (char *)memory_allocate ( CHSysPartition , FLASH_SECURITYDATA_LEN);
			if(tempbuf != NULL)
			{
				memset(tempbuf,0,FLASH_SECURITYDATA_LEN);

				ipanel_porting_nvram_read(FLASH_SECURITY_ADDR_SPARE,tempbuf,FLASH_SECURITYDATA_LEN);
				
				CH_SetSecurityFlashStatus(0x8b);
				if(CH_Security_Flash_burn(FLASH_SECURITY_ADDR,tempbuf,FLASH_SECURITYDATA_LEN)== false)
				{
					int time = 3000;
				
					while(time >0)
					{
						if(CH_GetSecurityFlashStatus()==0x8a)
						{
							break;
						}
						else
						{
							task_delay(ST_GetClocksPerSecond()/1000);
							time--;
						}
						
					}
					
				}
			

				memory_deallocate(CHSysPartition,tempbuf);
				tempbuf == NULL;
			}
			else
			{
				return;
			}



		}

	}


}


#endif



/*--eof----------------------------------------------------------------------------------------------------*/

