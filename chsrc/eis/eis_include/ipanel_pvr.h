/******************************************************************************/
/*    Copyright (c) 2009 iPanel Technologies, Ltd.							  */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author huanghm 2009/03/19											  */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_PVR_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_PVR_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* pvr file options */
typedef enum
{
	IPANEL_PVR_FILE_TIME = 0,
	IPANEL_PVR_FILE_SIZE
} IPANEL_PVR_FILE_e;

/* pvr ioctl options */
typedef enum
{
	IPANEL_PVR_SET_SERVICE_ID_PARAM, //设置IPANEL_PVRSERVICE_T信息
	IPANEL_PVR_SET_RECORD_DURATION,
	IPANEL_PVR_SET_RECORD_TIME,
	IPANEL_PVR_GET_RECORD_TIME
} IPANEL_PVR_IOCTL_e;

#define PVR_MAX_NUM_CA 8

typedef struct tagPVR_STREAM_PARAM
{
	BYTE_T stream_type; 
	UINT16_T pid;
	UINT16_T component_tag;
	UINT32_T numEcmPIDs;							
	UINT16_T ecmPID[PVR_MAX_NUM_CA];
	UINT16_T CASystemId[PVR_MAX_NUM_CA];
	
}PVR_STREAM_PARAM;

typedef struct tagPVR_SERVICE_PARAM
{
	UINT32_T service_id; 
	UINT16_T pcr_pid;
	
	UINT32_T num_stream; //pid数目
	PVR_STREAM_PARAM *list_stream;
}PVR_SERVICE_PARAM;

/* 回调函数原型 */
typedef VOID (*IPANEL_PVR_NOTIFY_FUNC)(UINT32_T fd, INT32_T event, void *param);

/* 设置回调函数 */
INT32_T ipanel_pvr_set_notify(IPANEL_PVR_NOTIFY_FUNC func);

/* 创建一个PVR通道, 返回设备句柄(IPANEL_NULL表示失败) */
UINT32_T ipanel_pvr_create_channel(IPANEL_ACC_TRANSPORT *param, UINT16_T service_id);

/* 销毁一个PVR通道 */
INT32_T ipanel_pvr_destroy_channel(UINT32_T fd);

/* 输入输出控制 */
INT32_T ipanel_pvr_ioctl(UINT32_T fd, IPANEL_PVR_IOCTL_e op, VOID *arg);


/* 开始录像，返回IPANEL_OK表示成功，其他为错误码 
    filename [input]: 录像输出文件名（包含路径）
	duration [input]: 录像时间长度(单位秒)，等于0xFFFFFFFF(-1)表示一直录像直到STOP，
				等于其他值表示超过这个时间就覆盖过时数据，时移要用
*/
INT32_T ipanel_pvr_start_record(UINT32_T fd, CHAR_T *filename);

/* 停止录像并关闭录像文件，返回IPANEL_OK表示成功，其他为错误码 */
INT32_T ipanel_pvr_stop_record(UINT32_T fd);

/* 清除录像数据 */
INT32_T ipanel_pvr_clean_record(UINT32_T fd);

/* 取得第一个录像文件名，返回过滤句柄，IPANEL_NULL表示失败 */
UINT32_T ipanel_pvr_get_first_file(CONST CHAR_T *match, CHAR_T *filename, INT32_T length);

/* 释放过滤句柄 */
INT32_T ipanel_pvr_release_file(UINT32_T filter);

/* 取得录像文件信息返回IPANEL_OK表示成功，其他为出错码 */
INT32_T ipanel_pvr_get_file_info(CONST CHAR_T *filename, IPANEL_PVR_FILE_e type, VOID *arg);

INT32_T ipanel_pvr_init(void);
INT32_T ipanel_pvr_exit(void);

#ifdef __cplusplus
}
#endif

#endif//_IPANEL_MIDDLEWARE_PORTING_PVR_API_FUNCTOTYPE_H_
