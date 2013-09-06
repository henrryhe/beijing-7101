/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_ca.c
  * 描述: 	实现CA 相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-23
  * ===================================================================================
  */

#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"
#include "eis_api_ca.h"

#include "eis_include\ipanel_smc.h"

#include "appltype.h"
#include "..\dbase\vdbase.h"

#ifdef __EIS_API_DEBUG_ENABLE__
#define __EIS_API_CA_DEBUG__
#endif

/*
功能说明：
	分配一个解扰器通道，并按照指定设定工作模式。
参数说明：
	输入参数：
		encryptmode：解扰模式，取值如下：
		typedef enum
		{
			IPANEL_DSM_TYPE_PES,
			IPANEL_DSM_TYPE_TS,
			IPANEL_DSM_TYPE_UNKNOWN
		} IPANEL_ENCRYPT_MODE_e;
	输出参数：
		无
	返 回：
		！＝ IPANEL_NULL： 成功， 返回分配的解扰器的句柄
		＝＝IPANEL_NULL： 失败。
*/
UINT32_T ipanel_porting_descrambler_allocate(IPANEL_ENCRYPT_MODE_e encryptmode)
{
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_descrambler_allocate 未完成" );
	#endif

	return IPANEL_NULL;
}

/*
功能说明：
	释放一个通过ipanel_porting_descrambler_allocate 分配的解扰器通道。
参数说明：
	输入参数：
		handle：解扰器句柄
	输出参数：
	无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_descrambler_free (UINT32_T handle)
{
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_descrambler_free 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	设置解扰器通道PID。
参数说明：
	输入参数：
		handle：解扰器句柄
		pid：需要解扰的流的PID
	输出参数：
		无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败
*/
INT32_T ipanel_porting_descrambler_set_pid(UINT32_T handle, UINT16_T pid)
{
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_descrambler_set_pid 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	设置奇控制字。
参数说明：
	输入参数：
		handle：解扰器句柄
		key：控制字的值
		len：控制字长度
	输出参数：
		无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_descrambler_set_oddkey(UINT32_T handle,BYTE_T *key, INT32_T len)
{
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_descrambler_set_oddkey 未完成" );
	#endif

	return 0;
}

/*
功能说明：
	设置偶控制字。
参数说明：
	输入参数：
		handle：解扰器句柄
		key：控制字的值
		len：控制字长度
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_descrambler_set_evenkey(UINT32_T handle, BYTE_T *key, INT32_T len)
{
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_descrambler_set_evenkey 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	打开需要操作的智能卡，注册智能卡回调函数。实现者可以在此函数中完成对相
	应卡的初始化操作，如使能卡的硬件接口，对卡做Active 操作等。当打开时探测到插
	槽中有卡时，必须对卡完成上电复位操作。
	其中回调函数用到的类型定义为：
	typedef INT32_T (*IPANEL_SC_STATUS_NOTIFY)(INT32_T cardno,
	IPANEL_SMARTCARD_STATUS_e status, BYTE_T *msgATR, IPANEL_SMARTCARD_STANDARD_e
	standard)
	其中：cardno - 智能卡序号，
	status - 智能卡状态，
	msgATR - ATR 应答字符串，
	standard – 卡使用的标准
	智能卡的状态和智能卡的标准枚举定义如下：
	typedef enum
	{
		IPANEL_CARD_IN,
		IPANEL_CARD_OUT,
		IPANEL_CARD_ERROR
	} IPANEL_SMARTCARD_STATUS_e;
	typedef enum
	{
		IPANEL_SMARTCARD_STANDARD_ISO7816,
		IPANEL_SMARTCARD_STANDARD_NDS,
		IPANEL_SMARTCARD_STANDARD_EMV96,
		IPANEL_SMARTCARD_STANDARD_EMV2000,
		IPANEL_SMARTCARD_STANDARD_ECHOCHAR_T,
		IPANEL_SMARTCARD_STANDARD_UNDEFINE
	} IPANEL_SMARTCARD_STANDARD_e;
参数说明：
	输入参数：
		cardno：智能卡序号
		func：智能卡状态变化通知回调函数的地址
	输出参数：无
	返 回：
		– >=0 成功，
		– 0，接口打开操作成功，插槽中有卡但复位失败
		– 1，接口打开操作成功，插槽中无卡
		– 2，接口打开操作成功，插槽中有卡并复位成功，使用T0 协议通讯
		– 3，接口打开操作成功，插槽中有卡并复位成功，使用T1 协议通讯
		– 4，接口打开操作成功，插槽中有卡并复位成功，使用T14 协议通讯
		– <0 失败。
*/
INT32_T ipanel_porting_smartcard_open(INT32_T cardno, IPANEL_SC_STATUS_NOTIFY func)
{
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_smartcard_open 未完成" );
	#endif

	return -1;
}

/*
功能说明：
	关闭正在操作的智能卡接口。实现者可以在此函数中完成对相应卡的释放操作，
	如停止卡的硬件接口，对卡做DeActive 操作等。
参数说明：
	输入参数：
		cardno：智能卡序号
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_smartcard_close(INT32_T cardno)
{
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_smartcard_open 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	对智能卡执行复位操作。
参数说明：
	输入参数：
		cardno：智能卡序号
	输出参数：
		msgATR： 返回一个ATR 串，调用者必须分配足够的空间保存ATR；也可以是
		空的，底层不返回ATR 串。
	返 回：
		>=0 成功，
		0，复位成功，使用T0 协议通讯
		1，复位成功，使用T1 协议通讯
		2，复位成功，使用T14 协议通讯
		小于0 失败。
		-1 无卡
		-2 不可识别卡
		-3 通讯错误
*/
INT32_T ipanel_porting_smartcard_reset(INT32_T cardno, BYTE_T *msgATR)
{
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_smartcard_open 未完成" );
	#endif

	return -1;
}

/*
功能说明：
	与智能卡通讯。驱动程序需要根据不同的协议类型决定如何处理reqdata 中的数
	据。
	T0 协议：
	写数据：
	reqdata = header + data to write
	读数据：
	reqdata = header
	repdata = data from smartcard
	T14 协议：
	写数据：
	reqdata = data to write
	读数据：
	repdata = data from smartcard
参数说明：
	输入参数：
		cardno：智能卡序号
		reqdata：命令字段
		reqlen：命令字段长度
		repdata：返回字段
		replen：返回字段长度
		statusword：保存本次操作智能卡返回的状态字的地址
		*statusword ＝ SW1 << 8 | SW2
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		< 0 ：失败。
		-1 无卡
		-2 不可识别卡
		-3 通讯错误
*/
INT32_T ipanel_porting_smartcard_transfer_data(INT32_T cardno,const BYTE_T *reqdata, INT32_T reqlen, BYTE_T*repdata, INT32_T *replen, UINT16_T *statusword)
{
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_smartcard_transfer_data 未完成" );
	#endif

	return IPANEL_OK;
}

/*
  * Func: eis_api_stop_ca
  * Desc: 停止CA接收
  */
void eis_api_stop_ca ( void )
{

 #ifdef NITCDT
	usifNotifyToNitandCdt(STOP_NIT_BUILDING);/*处理NIT信息*/
	usifNotifyToNitandCdt(STOP_CDT_BUILDING);/*处理CDT信息*/
#ifdef ADD_SDT_MONITOR
	usifNotifyToNitandCdt(STOP_SDT_BUILDING);
#endif
#endif
/*20071116 wlq modify */
#ifdef NDS_CA 
  CH_PMT_PutMessageToCAQue(-1,STOP_DATA_BASE_BUILDING);
	CH_CAT_PutMessageToCAQue(STOP_DATA_BASE_BUILDING);
#endif
	
	
#if 0/*20071116 wlq modify */
	CH_EPG_StopALLEIT();
	task_delay(3000);
	/**CH_EPG_FreeAllEITSource();***20060222 shb add handle free eit source******/
	CH_EIT_ClearAppMem();
#endif
}


/*--eof------------------------------------------------------------------------------------------*/

