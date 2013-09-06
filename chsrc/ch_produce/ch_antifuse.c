/*******************文件说明注释************************************/
/*公司版权说明：版权（2007）归长虹网络科技有限公司所有。           */
/*文件名：ch_antifuse.c                                            */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2007-07-20                                             */
/*模块功能:
  STi7109/710 安全功能设置                                         */
/*主要函数及功能:                                                  */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/*          20070720    zxg       从STi5107移植到STi7101平台       */
/*******************************************************************/

/**************** #include 部分*************************************/
#include <stdlib.h>
#include <stdio.h>
#include "stlite.h"
#include "stsectoolfuse.h"
#include "stsectoolnuid.h"
#include "ch_antifuse.h"
#include "..\ch_flash\ch_flashmid.h"
#include "..\ch_tkdma\sttkdma_m1.h"/* zq add 20090906*/
/******************************************************************/
/**************** #define 部分*************************************/

#define PAIRING_ADDR      0xA003FC00         /*配对KEY数据存放地址*/

/***************  全局变量定义部分 ********************************/

static boolean gb_Securinited=false;
/* 功能： 判断安全模块是否初始化                                 */
/* 值域： [fase,true]                                            */
/* 应用环境：1:     CH_Secure_Init（void）中调用                 */
/* 注意事项：无                                                  */


/***************************函数体定义*****************************/ 
/******************************************************************/
/*函数名:BOOL CH_Secure_Init( void )                              */
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:初始化STi7101/7109安全模块初始化                   */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：TRUE, 失败,FALSE,成功                               */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */      
BOOL CH_Secure_Init( void )      
{
	boolean b_result;
	if(gb_Securinited == false)
	{
     b_result = CH_Antifuse_Init();
     b_result += CH_NUID_Init();
	 gb_Securinited = true;
	}
	return b_result;
}
/* zq add 20090906*/
BOOL CH_Secure_Term(void)
{
	boolean b_result;
	if(gb_Securinited == true)
	{
		b_result = CH_Antifuse_Term();
                b_result += CH_NUID_Term();
		gb_Securinited = false;
	}
	return b_result;
}
/***********/
/******************************************************************/
/*函数名:BOOL CH_Antifuse_Init（void）                            */
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:初始化STi7101/7109 fuse模块                        */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：TRUE, 失败,FALSE,成功                               */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */     
BOOL CH_Antifuse_Init( void )
{
	ST_ErrorCode_t st_Error;
	st_Error = STSECTOOLFUSE_Init("SECTOOLFUSE", NULL, STSECTOOLFUSE_NOINT);
	if(st_Error == ST_NO_ERROR)
	{
       return false;
	}
	else
    	{
    		do_report(0,"\n<<<<<<<<<CH_Antifuse_Init  0x%x <<<<<<<<<\n",st_Error);
       		return true;
	}
}
/******************************************************************/
/*函数名:BOOL CH_Antifuse_WriteItem(STSECTOOLFUSE_ITEM_t st_item, U32 ui_value)*/
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:永久写对应数据到sti7101/7019 fuse                  */
/*函数原理说明:                                                   */
/*输入参数：STSECTOOLFUSE_ITEM_t st_item,需要写入的永久数据项
            U32 ui_value,需要写入的数据                           */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：TRUE, 失败,FALSE,成功                               */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */     
BOOL CH_Antifuse_WriteItem(STSECTOOLFUSE_ITEM_t st_item, U32 ui_value)
{
	ST_ErrorCode_t st_Error;
	U32 ui_data;

	/* 开始 OTP 操作 */
	st_Error = STSECTOOLFUSE_StartOTP();
	if(st_Error != ST_NO_ERROR)
	{
		do_report(0,"StartOTP error\n");
		return true;
	}
	/* 读取原来的数据 */
	st_Error = STSECTOOLFUSE_ReadItem(st_item, &ui_data);
    if(st_Error != ST_NO_ERROR)
	{
		do_report(0,"ReadItem error\n");
		return true;
	}
	/* 允许写数据 */
	st_Error = STSECTOOLFUSE_PermanentWriteEnable(st_item);
	 if(st_Error != ST_NO_ERROR)
	{
		do_report(0,"PermanentWriteEnable error\n");
		return true;
	}
	/* 写入对应的数据 */
	st_Error = STSECTOOLFUSE_WriteItem(st_item, ui_value);
    if(st_Error != ST_NO_ERROR)
	{
		do_report(0,"WriteItem error\n");
		return true;
	}
	return false;
}
/**********************************************************************/
/*函数名:boolean CH_EnableBootAuthenticaion( void )                   */
/*开发人和开发时间:zengxianggen 2007-07-20                            */
/*函数功能描述:打开开机安全校验状态功能                               */
/*函数原理说明: 
crypt_sigchk_enable, 1 bit, read/write (0 to 1)                       */
/*输入参数：无                                                        */
/*输出参数: 无                                                        */
/*使用的全局变量、表和结构：                                          */
/*调用的主要函数列表：                                                */
/*返回值说明:TRUE,操作失败,FALSE,操作成功                             */   
/*调用注意事项:                                                       */
/*其他说明:                                                           */  
boolean CH_EnableBootAuthenticaion( void )
{
   /*如果已经打开该功能,不执行操作*/
   if(CH_BootAuthenticaionStatus() == true)
   {
         return true;
   }

   return CH_Antifuse_WriteItem(STSECTOOLFUSE_CRYPT_SIGCHK_ENABLE_OTP_ITEM,0x01 );
}
/**********************************************************************/
/*函数名:boolean CH_EnableJTAGLOCK( void )                            */
/*开发人和开发时间:zengxianggen 2007-07-20                            */
/*函数功能描述:打开JTAG保护功能                                       */
/*函数原理说明:                                                      
jtag_prot, 4 bits, read/write (0 to 1): 
Indicates JTAG protection state. 
00xx=JTAG not protected; 
01xx or 10xx=JTAG locked (password required); 
11xx=JTAG disabled. (Bits 0,1 not used)                               
/*输入参数：无                                                        */
/*输出参数: 无                                                        */
/*使用的全局变量、表和结构：                                          */
/*调用的主要函数列表：                                                */
/*返回值说明:TRUE,操作失败,FALSE,操作成功                             */   
/*调用注意事项:                                                       */
/*其他说明:                                                           */  
boolean CH_EnableJTAGLOCK( void )
{
    /*如果已经打开该功能,不执行操作*/
   if(CH_JTAGLOCKStatus() == true)
   {
         return true;
   }

   return CH_Antifuse_WriteItem(STSECTOOLFUSE_JTAG_PROT_OTP_ITEM,0x08 );
}
/**********************************************************************/
/*函数名:boolean CH_EnableOTPBootCode( void )                         */
/*开发人和开发时间:zengxianggen 2007-07-20                            */
/*函数功能描述:打开BOOTCODE OPT功能                                   */
/*函数原理说明:                                                       */
/*输入参数：无                                                        */
/*输出参数: 无                                                        */
/*使用的全局变量、表和结构：                                          */
/*调用的主要函数列表：                                                */
/*返回值说明:TRUE,操作失败,FALSE,操作成功                             */   
/*调用注意事项:                                                       */
/*其他说明:                                                           */  
boolean CH_EnableOTPBootCode( void )
{
#if 0
	/*return 0	;*/
	if(CH_FLASH_GetFLashType()==FLASH_ST_M28||CH_FLASH_GetFLashType()==FLASH_ST_M29)/*ST FLASH*/
	{
		;
	}
	else
	{
		CH_LDR_7101SpansionOTPLock();
	}
#else
       CH_LDR_EnableFirst2BlockOTP();
#endif	
}

/**********************************************************************/
/*函数名:boolean CH_JTAGLOCKStatus( void )                            */
/*开发人和开发时间:zengxianggen 2007-07-20                            */
/*函数功能描述:得到JTAG是否锁定                                       */
/*函数原理说明:                                                       */
/*输入参数：无                                                        */
/*输出参数: 无                                                        */
/*使用的全局变量、表和结构：                                          */
/*调用的主要函数列表：                                                */
/*返回值说明:TRUE,JTAG锁定,FALSE,JTAG没有锁定                         */   
/*调用注意事项:                                                       */
/*其他说明:                                                           */  
boolean CH_JTAGLOCKStatus( void )
{
	ST_ErrorCode_t st_Error;
	U32 ui_data;

	/* 开始 OTP 操作 */
	st_Error = STSECTOOLFUSE_StartOTP();
	if(st_Error != ST_NO_ERROR)
	{
		do_report(0,"StartOTP error\n");
		return true;
	}

   	/* 读取数据 */
	st_Error = STSECTOOLFUSE_ReadItem(STSECTOOLFUSE_JTAG_PROT_OTP_ITEM, &ui_data);
    if(st_Error != ST_NO_ERROR)
	{
		do_report(0,"ReadItem error\n");
		return false;
	}
	if(((ui_data>>2)&0x03) == 0x00)
	{
       return false;
	}
	else
	{
       return true;
	}	
}

/**********************************************************************/
/*函数名:boolean CH_BootAuthenticaionStatus(void)                     */
/*开发人和开发时间:zengxianggen 2007-07-20                            */
/*函数功能描述:得到开机安全校验状态                                   */
/*函数原理说明:                                                       */
/*输入参数：无                                                        */
/*输出参数: 无                                                        */
/*使用的全局变量、表和结构：                                          */
/*调用的主要函数列表：                                                */
/*返回值说明:TRUE,,开机需要安全校验状态,FALSE,开机不需要安全校验状态  */   
/*调用注意事项:                                                       */
/*其他说明:                                                           */                                             
boolean CH_BootAuthenticaionStatus(void)
{
	ST_ErrorCode_t st_Error;
	U32 ui_data;

	/* 开始 OTP 操作 */
	st_Error = STSECTOOLFUSE_StartOTP();
	if(st_Error != ST_NO_ERROR)
	{
		do_report(0,"StartOTP error\n");
		return true;
	}
   	/* 读取数据 */
	st_Error = STSECTOOLFUSE_ReadItem(STSECTOOLFUSE_CRYPT_SIGCHK_ENABLE_OTP_ITEM, &ui_data);
    if(st_Error != ST_NO_ERROR)
	{
		do_report(0,"ReadItem error\n");
		return false;
	}
	
	if(ui_data == 0x00)
	{
		return false;
	}
	else
	{
		return true;
	}
}
/**********************************************************************/
/*函数名:boolean CH_OTPStatus( void )                                 */
/*开发人和开发时间:zengxianggen 2007-07-20                            */
/*函数功能描述:得到BOOTCODE区域是否OTP                                */
/*函数原理说明:                                                       */
/*输入参数：无                                                        */
/*输出参数: 无                                                        */
/*使用的全局变量、表和结构：                                          */
/*调用的主要函数列表：                                                */
/*返回值说明:TRUE,BOOTCODE已经OTP,FALSE,BOOTCODE没有OTP               */   
/*调用注意事项:                                                       */
/*其他说明:                                                           */   
boolean CH_OTPStatus( void )
{
#if 0
   /*return 0;*/
	 if(CH_FLASH_GetFLashType()==FLASH_ST_M28||CH_FLASH_GetFLashType()==FLASH_ST_M29)/*ST FLASH*/
	 {

	 }
     else/*SPANSION FLASH*/
	 {
		 return CH_LDR_7101SpansionOTPLockStatus();
	 }
#else
return CH_LDR_GetFirst2BlockOTPStatus();
#endif	 
}
/******************************************************************/
/*函数名:BOOL CH_NUID_Init（void）                                */
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:初始化STi7101/7109 NUID                            */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：TRUE, 失败,FALSE,成功                               */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */ 
BOOL CH_NUID_Init( void )
{	
	ST_ErrorCode_t st_Error=ST_NO_ERROR;

	st_Error = STSECTOOLNUID_Init("NUID_Init",NULL,-1);
	if(st_Error == ST_NO_ERROR)
	{
		return false;
	}
	else
    {
    		do_report(0,"\n<<<<<<<<<CH_NUID_Init  0x%x <<<<<<<<<\n",st_Error);
		return true;
	}	
}

BOOL CH_NUID_Term(void)
{
    boolean b_result;

    b_result = STSECTOOLNUID_Term("NUID_Init",NULL);
    return b_result;
}
/******************************************************************/
/*函数名:boolean CH_GetNUID(U8* puc_NuIDKey,U32 *pui_CRC)            */
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:得到STi7101/7109 NUID  CRC信息和NUID Key           */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: U8* puc_NuIDKey,NUID KEY,
            U32 *pui_CRC,CRC数据指针                              */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：无                                                  */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */ 
boolean CH_GetNUID(U8* puc_NuIDKey,U32 *pui_CRC)
{
    int i_STRes = 0;
    U8  *puc_Buffer = NULL;
	U8  *puc_AlineBuffer = NULL;
	U8  uc_NUIDBuf[16];
    U32 DevPubID;


	task_delay(1000);
	puc_Buffer = memory_allocate(NcachePartition,512);
	if(puc_Buffer != NULL)
	{
	  puc_AlineBuffer = (U8 *)((unsigned int)(puc_Buffer)/32*32+32);/*32 BYTE对齐*/
	  i_STRes = STSECTOOLNUID_GetId((U8 *)uc_NUIDBuf,pui_CRC,(U8* )puc_AlineBuffer);
	}
	else 
	  return false;

	task_delay(1000);

	DevPubID = CH_GetNUIDKey();
	//task_delay(ST_GetClocksPerSecond()/2);
#if 0	/*20070816 change*/
    puc_NuIDKey[12] = DevPubID&0xFF;
    puc_NuIDKey[13] = (DevPubID>>8)&0xFF;

	puc_NuIDKey[14] = (DevPubID>>16)&0xFF;
    puc_NuIDKey[15] = (DevPubID>>24)&0xFF;
#else

    puc_NuIDKey[15] = DevPubID&0xFF;
    puc_NuIDKey[14] = (DevPubID>>8)&0xFF;

	puc_NuIDKey[13] = (DevPubID>>16)&0xFF;
    puc_NuIDKey[12] = (DevPubID>>24)&0xFF;
#endif
	
	if(puc_Buffer != NULL)
	{
         	memory_deallocate(NcachePartition,puc_Buffer);
	}
	if(i_STRes != ST_NO_ERROR)
	{
		switch(i_STRes)
		{
		case ST_ERROR_BAD_PARAMETER:
			do_report(0,"STSECTOOLNUID_GetId ST_ERROR_BAD_PARAMETER\n");
			
			break;
		case STSECTOOLNUID_ERROR_OPTION_NOT_SUPPORTED:
			do_report(0,"STSECTOOLNUID_GetId STSECTOOLNUID_ERROR_OPTION_NOT_SUPPORTED\n");
			
			break;
		case STSECTOOLNUID_ERROR_COMMAND_FAILED:
			do_report(0,"STSECTOOLNUID_GetId STSECTOOLNUID_ERROR_COMMAND_FAILED\n");
			
			break;
        default:
			do_report(0,"STSECTOOLNUID_GetId UNKOW ERROR\n");
			break;
		}
		return false;
	}
     else
     	{
              return true;
     	}
	
}

ST_ErrorCode_t CH_GetNUIDFormTKDMA(U8* NUID)
{
	ST_ErrorCode_t  st_ErrorCode;
	STTKDMA_Key_t Key;
    
	st_ErrorCode = STTKDMA_ReadPublicID(&Key);

          memcpy(NUID,Key.key_data,16);

          return st_ErrorCode;
}

/******************************************************************/
/*函数名:U32 CH_GetNUIDKey( void )                                   */
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:得到STi7101/7109 unencrypted Nagra Unique ID.      */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：返回unencrypted Nagra Unique ID                     */   
/*调用注意事项:                                                   */
/*其他说明:无                                                     */ 
U32 CH_GetNUIDKey( void )
{
	return STSECTOOLNUID_GetNuid();
}

/*函数名:void CH_GetPairCASerial(U8 *CASerial)*/
/*开发人和开发时间:zengxianggen 2007-01-18                       */
/*函数功能描述:得到CASerial信息                                  */
/*函数原理说明:                                                  */
/*使用的全局变量、表和结构：                                     */
/*输入参数：无                                                   */
/*输出参数: U8 *CASerial,CA序列号数据                            */
/*使用的全局变量、表和结构：                                     */
/*调用的主要函数列表：                                           */
/*返回值说明：无                                                 */
/*其他说明:无                                                    */ 
void CH_GetPairCASerial(U8 *CASerial)
{
    memcpy(CASerial,(U8 *)(PAIRING_ADDR +0x04),4);
}


/*----------------------------------CRC32------------------------*/
unsigned long crc_table_ISO_3309[256];
/* Make the table for a fast CRC. */
void make_crc_table_ISO_3309(void)
{
	unsigned long c;
	int n, k;
	for (n = 0; n < 256; n++) {
		c = (unsigned long) n;
		for (k = 0; k < 8; k++) {
			if (c & 1) {
				c = 0xedb88320L ^ (c >> 1);
			} else {
				c = c >> 1;
			}
		}
		crc_table_ISO_3309[n] = c;
	}
}
unsigned long crc_ISO_3309( unsigned char *buf, int len)
{
	unsigned long c = 0 ^ 0xffffffffL;
	int n;
	make_crc_table_ISO_3309();
	for (n = 0; n < len; n++) {
		c = crc_table_ISO_3309[(c ^ buf[n]) & 0xff] ^ (c >> 8);
	}
	return c ^ 0xffffffffL;
}

/******************************************************************/
/*函数名:BOOL CH_Antifuse_Term（void）                            */
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:关闭STi7101/7109 fuse模块                        */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：TRUE, 失败,FALSE,成功                               */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */     
BOOL CH_Antifuse_Term( void )
{
	ST_ErrorCode_t st_Error;
	st_Error = STSECTOOLFUSE_Term("SECTOOLFUSE",NULL);
	if(st_Error == ST_NO_ERROR)
	{
       return false;
	}
	else
    {
       return true;
	}
}
