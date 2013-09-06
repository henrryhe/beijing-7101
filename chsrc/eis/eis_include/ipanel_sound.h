/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                     */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the SOUND Porting APIs needed by iPanel  */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author huzh 2007/11/22                                                 */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_SOUND_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_SOUND_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0 // move to ipanel_typedef.h
typedef enum
{
    IPANEL_XMEM_PCM = 1,
    IPANEL_XMEM_MP3,
    IPANEL_XMEM_TS,
    IPANEL_XMEM_ES
} IPANEL_XMEM_PAYLOAD_TYPE_e;

typedef struct
{
    IPANEL_XMEM_PAYLOAD_TYPE_e destype;  /* 1，表示PCM数据描叙符。见流式数据描叙符类型定义 */
    UINT32_T samplerate;                 /* 采样频率 HZ */
    UINT16_T channelnum;                 /* 通道个数，1单声道，2双声道 */
    UINT16_T bitspersample;              /* 采样精度，bpp */
    UINT16_T bsigned;                    /* 有符号还是无符号, 1有符号，0无符号 */
    UINT16_T bmsbf;                      /* 是否高位在前（most significant bit first）。1，高位在前，0低位在前 */
    UINT32_T samples;                    /* 采样个数 */
} IPANEL_PCMDES, *pIPANEL_PCMDES;

// 定义数据块释放函数指针
typedef VOID (*IPANEL_AUDIO_FREE_NOTIFY)(VOID *pblk);

typedef struct
{
    VOID *pdes;                          /* pbuf中的数据属性描述 */
    IPANEL_AUDIO_FREE_NOTIFY pfree;     /* IPANEL_XMEMBLK数据块释放函数指针 */
    UINT32_T *pbuf;                      /* 数据缓冲区地址 */
    UINT32_T len;                        /* 缓冲区长度 */
} IPANEL_XMEMBLK, *pIPANEL_XMEMBLK;

typedef enum
{
    IPANEL_DEV_USE_SHARED,               /* 和其他用户共享使用设备 */
    IPANEL_DEV_USE_EXCUSIVE              /* 独占使用设备 */
} IPANEL_DEV_USE_MODE;
#endif

typedef IPANEL_XMEM_FREE IPANEL_AUDIO_FREE_NOTIFY;

typedef enum
{
    IPANEL_AUDIO_DATA_CONSUMED,          /* 某个数据块中的数据已经处理完毕 */
    IPANEL_AUDIO_DATA_LACK               /* 所有的数据处理完毕，不能马上获得新的数据就会导致声音停止 */
} IPANEL_AUDIO_MIXER_EVENT;

typedef enum
{
    IPANEL_MIXER_SET_VOLUME   	= 1,
    IPANEL_MIXER_CLEAR_BUFFER 	= 2,
    IPANEL_MIXER_PAUSE 			= 3,
    IPANEL_MIXER_RESUME 		= 4
} IPANEL_MIXER_IOCTL_e;

typedef VOID (*IPANEL_AUDIO_MIXER_NOTIFY)(UINT32_T hmixer, IPANEL_AUDIO_MIXER_EVENT event, UINT32_T *param);

UINT32_T ipanel_porting_audio_mixer_open(IPANEL_DEV_USE_MODE mode, IPANEL_AUDIO_MIXER_NOTIFY func);
INT32_T ipanel_porting_audio_mixer_close(UINT32_T handle);
IPANEL_XMEMBLK *ipanel_porting_audio_mixer_memblk_get(UINT32_T handle, UINT32_T size);
INT32_T ipanel_porting_audio_mixer_memblk_send(UINT32_T handle, IPANEL_XMEMBLK *pcmblk);
INT32_T ipanel_porting_audio_mixer_ioctl(UINT32_T handle, IPANEL_MIXER_IOCTL_e op, VOID *arg);

#ifdef __cplusplus
}
#endif

#endif//_IPANEL_MIDDLEWARE_PORTING_SOUND_API_FUNCTOTYPE_H_
