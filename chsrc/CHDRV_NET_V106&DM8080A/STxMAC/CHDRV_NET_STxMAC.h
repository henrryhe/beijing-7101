/*******************头文件说明注释**********************************/
/*公司版权说明：版权（2007）归长虹网络科技有限公司所有。           */
/*文件名：CHDRV_NET_MAC110.h                                              */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2007-08-10                                             */
/*头文件内容概要描述（功能/变量等）：                              */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/*                                                                 */
/*******************************************************************/
/*****************条件编译部分**************************************/
#ifndef __CHDRV_NET_MAC110__
#define __CHDRV_NET_MAC110__
/*******************************************************************/
/**************** #include 部分*************************************/
#include "stddef.h"
#include "stddefs.h"

#include "stcommon.h"
#include <osplus.h>
#include <osplus/device.h>
#include <osplus/registry.h>
#include <osplus/stmac110.h>
#include <osplus/dev.h>
#include <osplus/ethernet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(ARCHITECTURE_ST40)
#define ARCHITECTURE_ST40
#endif

#define ETHER_DEVICE_NAME       "eth0"                       /*以太网设备驱动名称,必须保持一致*/

/*******************************************************************/
/***************  外部函数申明部分 *********************************/

 static void CH_ReadPacketCallBack0(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack1(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack2(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack3(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack4(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack5(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack6(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack7(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack8(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack9(struct ethernet_async_s * r_syn_info);

/*******************************************************************/
#endif

