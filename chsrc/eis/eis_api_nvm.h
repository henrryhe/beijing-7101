/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_nvm.h
  * 描述: 	定义永久存储相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-23
  * ===================================================================================
  */
#ifndef __EIS_API_NVM__
#define __EIS_API_NVM__

#define EIS_FLASH_BLOCK_SIZE		0x20000							/* 128K */
#define EIS_FLASH_BASE_ADDR		0xa0000000

#define EIS_FLASH_SKIN_SIZE               0x600000
#define EIS_FLASH_OFFSET_SKIN           0x1800000

#define EIS_FLASH_BASIC_SIZE		(EIS_FLASH_BLOCK_SIZE)/* BASIC	在整个FLASH中的偏移地址 */
#define EIS_FLASH_APPMGR_SIZE	(EIS_FLASH_BLOCK_SIZE*8)/* APPMGR	在整个FLASH中的偏移地址 */
#define EIS_FLASH_THIRDPART_SIZE	(EIS_FLASH_BLOCK_SIZE)/* 第三方共用 */
#define EIS_FLASH_QUICK_SIZE	(EIS_FLASH_BLOCK_SIZE)/* 立即写入的NVRAM */

#define EIS_FLASH_OFFSET_BASIC		(0x1E00000)/* BASIC	在整个FLASH中的偏移地址 *//*(0x1800000) WZ 添加SKIN后的调整011211*/
#define EIS_FLASH_OFFSET_APPMGR	(EIS_FLASH_OFFSET_BASIC+EIS_FLASH_BASIC_SIZE)/* APPMGR	在整个FLASH中的偏移地址 */
#define EIS_FLASH_OFFSET_THIRD_PART	(EIS_FLASH_OFFSET_APPMGR+EIS_FLASH_APPMGR_SIZE)/* 第三方共用 */
#define EIS_FLASH_OFFSET_QUICK	(EIS_FLASH_OFFSET_THIRD_PART+EIS_FLASH_THIRDPART_SIZE)/* 立即写入的NVRAM */


#define EIS_FLASH_MAX_ADDR (EIS_FLASH_OFFSET_QUICK+EIS_FLASH_QUICK_SIZE)
typedef enum
{
	IPANEL_FLASH_STATUS_IDLE	= 0,		/* IDLE */
	IPANEL_FLASH_STATUS_ERASING,		/* 正在擦除 */
	IPANEL_FLASH_STATUS_WRITING,	/* 正在写入 */
	IPANEL_FLASH_STATUS_OK,			/* 擦除或写入成功 */
	IPANEL_FLASH_STATUS_FAIL			/* 擦除或写入失败 */
}IPANEL_FLASH_STATUS;

#endif

/*--eof----------------------------------------------------------------------------------------------------*/

