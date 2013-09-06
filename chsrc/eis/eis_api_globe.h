/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_globe.h
  * 描述: 	定义全局变量的外部定义
  * 作者:	蒋庆洲
  * 时间:	2008-10-22
  * ===================================================================================
  */

#ifndef __EIS_API_GLOBE__
#define __EIS_API_GLOBE__

#include "stddefs.h"
#include "graphicbase.h"
#include "stflash.h"

typedef enum
{
	Payment,
	SMS,
	Edu,
	WebTV,
	RevTV,
	KaraOke,
	VOD,
	MAIL,
	Lottery,
	TVBank,
	Gameon,
	Stock,
}IPANEL_IP_URI_TYPE;

extern ST_Partition_t   *gp_appl2_partition;			/* 茁壮需要用到的内存分区 */
extern ST_Partition_t   *appl_partition;				/* 茁壮需要用到的内存分区 */
extern ST_Partition_t   *SystemPartition;				/* 系统内存分区 */
extern ST_Partition_t   *CHSysPartition;				/* 绘图内存分区 */

extern clock_t 		geis_reboot_timeout;		/* 重启判断 */
extern CH_ViewPort_t 	CH6VPOSD;

extern boolean 	log_out_state;					/* 退出标志 */

extern STFLASH_Handle_t FLASHHandle;
extern void* 			pEisHandle;					/* ipanel浏览器句柄 */
extern semaphore_t *gp_EisSema ;
extern void  CH_FlashLock(void);
extern void  CH_FlashUnLock(void);

extern void CH_PUB_UnLockFlash(U32 Offset);
extern void CH_PUB_LockFlash(U32 Offset);

#endif
/*--eof-----------------------------------------------------------------------------------------------------*/

