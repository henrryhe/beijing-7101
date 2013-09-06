/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_define.h
  * 描述: 	定义常用的宏
  * 作者:	蒋庆洲
  * 时间:	2008-10-22
  * ===================================================================================
  */

#ifndef __EIS_API_DEFINE__
#define __EIS_API_DEFINE__

#include "stddefs.h"
#include "eis_include\Ipanel_typedef.h"
#include "eis_include\Ipanel_porting_event.h"

#define 	INVALID_LINK	-1
#define	SLOT_ID        	short

#define IPANEL_NULL		0

/* 回调函数 */
typedef INT32_T (*IPANEL_PROTOCOL_EXT_CBK)(const CHAR_T *protocolurl);

#endif

/*--eof----------------------------------------------------------------------------------------------------*/

