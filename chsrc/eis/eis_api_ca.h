/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_ca.h
  * 描述: 	定义CA 相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-23
  * ===================================================================================
  */
#ifndef __EIS_API_CA__
#define __EIS_API_CA__

typedef enum
{
	IPANEL_DSM_TYPE_PES,
	IPANEL_DSM_TYPE_TS,
	IPANEL_DSM_TYPE_UNKNOWN
} IPANEL_ENCRYPT_MODE_e;

/*
  * Func: eis_api_stop_ca
  * Desc: 停止CA接收
  */
extern void eis_api_stop_ca ( void );

#endif

/*--eof--------------------------------------------------------------------------------------------*/

