/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                     */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author huzh 2007/11/22                                                 */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_UPGRADE_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_UPGRADE_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct loaderinfo
{
	unsigned int Frequency;	// example: 2990000  
	unsigned int Symbolrate;// example: 69000
 	unsigned int Modulation;// example: "1"(16-QAM)  "2"(32-QAM)"3"(64-QAM)     "4"(128-QAM)  "5"(256-QAM)unsigned int Pid;
 	// example: 0x137
	//以下这些参数只有在通过后门升级时ipanel才会赋值并传给第三方
	unsigned int  TableId;			// 根据需要赋值，可能为0
	unsigned char Version;			// 将要升级的软件的版本(一般不传)     
	unsigned char Upgrade_Type;		// 提示更新:0 ; 强制更新:1
	unsigned char Upgrade_flag;		// currently no use,pass 1
	unsigned char Software_Type;	// 表明将要升级的软件类型，应用软件:0 ; 系统软件:1
}IPANEL_LOADER_INFO;

/*
将表中收到的升级描述符传给升级规则检测模块。
return：
-1 	– 错误，数据错误或不是本设备升级描叙符；
 0 	- 版本相等，无需升级;
 1 	- 有新版本，手动升级;
 2 	- 有新版本，强制升级。
*/
INT32_T ipanel_upgrade_check(CHAR_T *des, UINT32_T len);

/*通知升级执行模块执行升级操作。*/
INT32_T ipanel_upgrade_start(CHAR_T *des, UINT32_T len);

//ipanel提供给第三方使用的函数，第三方库使用该函数时必须在ipanel调用的ipanel_upgrade_start()函数调用
INT32_T ipanel_upgrade_getparams(UINT32_T onid, UINT32_T tsid, UINT32_T srvid, UINT32_T comp_tag, IPANEL_LOADER_INFO *info);

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_UPGRADE_API_FUNCTOTYPE_H_
