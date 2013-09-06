/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                     */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author Zouxianyun 2005/04/28                                           */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_SMC_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_SMC_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* redefinitions for old usage */
#define EIS_CARD_IN		IPANEL_CARD_IN
#define EIS_CARD_OUT	IPANEL_CARD_OUT
#define EIS_CARD_ERROR	IPANEL_CARD_ERROR

/* smart card */
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

/*******************************************************************************
打开智能卡时的回调函数
其中：
    cardno -　智能卡序号
    status -　智能卡状态
    msgATR -  ATR应答字符串
    standard – 卡使用的标准
*******************************************************************************/
typedef INT32_T (*IPANEL_SC_STATUS_NOTIFY)(INT32_T cardno, 
										 IPANEL_SMARTCARD_STATUS_e status, 
										 BYTE_T *msgATR, 
										 IPANEL_SMARTCARD_STANDARD_e standard);

/*******************************************************************************
对智能卡执行复位操作。
input:	cardno:智能卡序号
	    msgATR:保存卡返回的ATR串的buffer地址，使用时必须检查地址是否是空指针。
output:	msgATR: 返回ATR串，可为空。
return:	>=0 成功
					0,复位成功,使用T0协议通讯
					1,复位成功,使用T1协议通讯
					2,复位成功,使用T14协议通讯
				小于0 失败
					-1,无卡
					-2,不可识别卡
					-3,通讯错误
*******************************************************************************/
INT32_T ipanel_porting_smartcard_reset(INT32_T cardno, BYTE_T *msgATR);

/*******************************************************************************
打开需要操作的智能卡，注册智能卡回调函数。实现者可以在此函数中完成对相应卡的
初始化操作，如使能卡的硬件接口，对卡做Active操作等。当打开时探测到插槽中有卡时，
必须对卡完成上电复位操作。
input:	cardno 智能卡序号
				sc_notify 智能卡状态变化通知回调函数的地址。
output:	None
return:	>=0 成功
					0，接口打开操作成功，插槽中有卡但复位失败
					1，接口打开操作成功，插槽中无卡
					2，接口打开操作成功，插槽中有卡并复位成功，使用T0协议通讯
					3，接口打开操作成功，插槽中有卡并复位成功，使用T1协议通讯
					4，接口打开操作成功，插槽中有卡并复位成功，使用T14协议通讯
				小于0 失败
*******************************************************************************/
INT32_T ipanel_porting_smartcard_open(INT32_T cardno,
                                      IPANEL_SC_STATUS_NOTIFY func);

/*******************************************************************************
关闭正在操作的智能卡接口。实现者可以在此函数中完成对相应卡的释放操作，如停止卡的
硬件接口，对卡做DeActive操作等。
input:	cardno 智能卡序号
output:	None
rerurn:	IPANEL_OK 成功; IPANEL_ERR 失败
*******************************************************************************/
INT32_T ipanel_porting_smartcard_close(INT32_T cardno);

/*******************************************************************************
与智能卡通讯。驱动程序需要根据不同的协议类型决定如何处理reqdata中的数据
T0协议：
	写数据：
	reqdata ＝ header + data to write
	读数据：
	reqdata ＝ header
	repdata ＝ data from smartcard
T14协议：
	写数据：
	reqdata ＝ data to write
	读数据：
	repdata ＝ data from smartcard

input:	cardno:智能卡序号
				reqdata:命令字段
				reqlen:命令字段长度
				repdata:返回字段
				replen:返回字段长度
				StatusWord:保存本次操作智能卡返回的状态字的地址
					*StatusWordS ＝ SW1 << 8 | SW2
output:	None
return:	IPANEL_OK 成功
				小于0 失败
					-1:无卡
					-2:不可识别卡
					-3:通讯错误
*******************************************************************************/
INT32_T ipanel_porting_smartcard_transfer_data(INT32_T cardno,
                                               CONST BYTE_T *reqdata,
                                               INT32_T reqlen, 
											   BYTE_T *repdata, 
                                               INT32_T *replen, 
                                               UINT16_T *statusword);



#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_SMC_API_FUNCTOTYPE_H_
