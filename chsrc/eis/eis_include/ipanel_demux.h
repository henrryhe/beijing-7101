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

#ifndef _IPANEL_MIDDLEWARE_PORTING_DEMUX_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_DEMUX_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* demux channel types */
typedef enum
{
	IPANEL_DMX_DATA_PSI_CHANNEL = 0,
	IPANEL_DMX_DATA_PES_CHANNEL	= 1,
	IPANEL_DMX_VIDEO_CHANNEL	= 2,
	IPANEL_DMX_AUDIO_CHANNEL	= 3,
	IPANEL_DMX_PCR_CHANNEL		= 4,
	IPANEL_DMX_CA_ECM_CHANNEL	= 5, //平台如非有特别特点, CA ECM type filter跟普通PSI filter并无二致.
	IPANEL_DMX_CA_EMM_CHANNEL	= 6, //平台如非有特别特点, CA EMM type filter跟普通PSI filter并无二致.
	IPANEL_DMX_FPOB_CHANNEL	    = 7,  //村村通项目针对FPOB数据另起了一个任务来接收数据,porting需要根据特定的CHANNEL type来决定数据
	                             //送给哪个任务，并且可以自己确定灵活的pool size.
								 //FPOB数据量大，混杂在DEMUX单个任务中会导致数据丢失。故而....
	IPANEL_DMX_DATA_TS_CHANNEL     = 8,
	IPANEL_DMX_IGNORE_CRC_CHANNEL = 9
} IPANEL_DMX_CHANNEL_TYPE_e;

typedef enum
{
    IPANEL_DEMUX_SET_CHANNEL_NOTIFY = 1,
    IPANEL_DEMUX_SET_SRC            = 2,
    IPANEL_DEMUX_SET_STREAM_TYPE    = 3,
    IPANEL_DEMUX_GET_BUFFER         = 4,
    IPANEL_DEMUX_PUSH_STREAM        = 5,
    IPANEL_DEMUX_STC_FETCH_MODE     = 6,
    IPANEL_DEMUX_STC_SET_TIMEBASE   = 7,
    IPANEL_DEMUX_GET_CURRENT_PCR 	= 8,
    IPANEL_DEMUX_GET_CURRENT_STC 	= 9
} IPANEL_DEMUX_IOCTL_e;

typedef enum 
{
    IPANEL_DEMUX_STC_ONCE_PCR 	= 1,
    IPANEL_DEMUX_STC_FOLLOW_PCR	= 2,
    IPANEL_DEMUX_STC_ONCE_APTS	= 3,
    IPANEL_DEMUX_STC_ONCE_VPTS	= 4,
    IPANEL_DEMUX_STC_FREE_RUN	= 5
} IPANEL_DEMUX_STC_FETCH_MODE_e;

typedef enum
{
	IPANEL_STREAM_TS	= 0,
	IPANEL_STREAM_ES	= 1,
	IPANEL_STREAM_PS	= 2
} IPANEL_STREAM_TYPE_e;

typedef enum 
{
	// 从芯片硬件接口输入数据
	IPANEL_DEMUX_SOURCE_DEVICE	= 0x10,
	// 系统软件推入的数据
	IPANEL_DEMUX_SOURCE_HOST	= 0x20
} IPANEL_DEMUX_SOURCE_TYPE_e;

typedef struct 
{
    UINT32_T timestamp_h; // Bits [9] ：是33位90K时钟的第33位，Bits [8：0]27M时钟的计数值。其他位未定义。
    UINT32_T timestamp_l; // Bits [31:0] ：是33位90K时钟的32位，与timestamp_h 中的Bit[9]结合成33位
}IPANEL_MEDIA_TIMESTAMP;

typedef  VOID (*IPANEL_DMX_NOTIFY_FUNC)(UINT32_T ch, UINT32_T filter, 
               UINT32_T DataStartAddr, INT32_T len);
INT32_T ipanel_porting_demux_set_notify(IPANEL_DMX_NOTIFY_FUNC func);
//
UINT32_T ipanel_porting_demux_create_channel(INT32_T poolsize, 
             IPANEL_DMX_CHANNEL_TYPE_e type);
INT32_T ipanel_porting_demux_set_channel_pid(UINT32_T channel, INT16_T pid);
INT32_T ipanel_porting_demux_destroy_channel(UINT32_T channel);
INT32_T ipanel_porting_demux_start_channel(UINT32_T channel);
INT32_T ipanel_porting_demux_stop_channel(UINT32_T channel);
//
UINT32_T ipanel_porting_demux_create_filter(UINT32_T channel);
INT32_T ipanel_porting_demux_set_filter(UINT32_T channel, UINT32_T filter,
            UINT32_T wide, BYTE_T coef[], BYTE_T mask[], BYTE_T excl[]);
INT32_T ipanel_porting_demux_destroy_filter(UINT32_T channel, UINT32_T filter);
INT32_T ipanel_porting_demux_enable_filter(UINT32_T channel, UINT32_T filter);
INT32_T ipanel_porting_demux_disable_filter(UINT32_T channel, UINT32_T filter);

INT32_T ipanel_porting_demux_ioctl(IPANEL_DEMUX_IOCTL_e op, VOID *arg);

#ifdef __cplusplus
}
#endif

#endif//_IPANEL_MIDDLEWARE_PORTING_DEMUX_API_FUNCTOTYPE_H_
