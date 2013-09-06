/******************************************************************************/
/*    Copyright (c) 2008 iPanel Technologies, Ltd.                            */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author huzh 2008/01/17                                                 */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_AV_MANAGER_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_AV_MANAGER_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"
#include "ipanel_adec.h"
#include "ipanel_vdec.h"
#include "ipanel_vout.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPANEL_CA_MAX_NUM   8
typedef struct
{
#if !defined(USE_PORTING_GUIDE_315)
    UINT16_T original_network_id;
    UINT16_T transport_stream_id;
#endif
    UINT16_T service_id;
    BYTE_T   servicename[128];

    UINT16_T video_pid;
    UINT16_T audio_pid;
    UINT16_T pcr_pid;

    /* 同密情况下, PMT common loop会有多个CA_descriptor */
    UINT16_T pmt_pid;

    UINT16_T ecmPids[IPANEL_CA_MAX_NUM];
    UINT16_T ecmCaSysIDs[IPANEL_CA_MAX_NUM];

    UINT16_T audioEcmPids[IPANEL_CA_MAX_NUM];
    UINT16_T audioCaSysIDs[IPANEL_CA_MAX_NUM];
    UINT16_T videoEcmPids[IPANEL_CA_MAX_NUM];
    UINT16_T videoCaSysIDs[IPANEL_CA_MAX_NUM];
} IPANEL_SERVICE_INFO;

/*
typedef enum // ipanel_adec.h
{
    IPANEL_AUDIO_MODE_STEREO            = 0,   // 立体声
    IPANEL_AUDIO_MODE_LEFT_MONO         = 1,   // 左声道
    IPANEL_AUDIO_MODE_RIGHT_MONO        = 2,   // 右声道
    IPANEL_AUDIO_MODE_MIX_MONO          = 3,   // 左右声道混合
    IPANEL_AUDIO_MODE_STEREO_REVERSE    = 4    // 立体声，左右声道反转
} IPANEL_ADEC_CHANNEL_OUT_MODE_e;

typedef struct // ipanel_vdec.h
{
    INT32_T len;
    BYTE_T  *data;
}IPANEL_IOCTL_DATA;

typedef enum // ipanel_vdec.h
{
    IPANEL_VIDEO_STREAM_MPEG1,
    IPANEL_VIDEO_STREAM_MPEG2,
    IPANEL_VIDEO_STREAM_H264,
    IPANEL_VIDEO_STREAM_AVS,
    IPANEL_VIDEO_STREAM_WMV,
    IPANEL_VIDEO_STREAM_MPEG4
}IPANEL_VIDEO_STREAM_TYPE_e;

typedef enum // ipanel_vout.h
{
    IPANEL_VIDEO_OUTPUT_CVBS    = 1<<0,
    IPANEL_VIDEO_OUTPUT_SVIDEO  = 1<<1,
    IPANEL_VIDEO_OUTPUT_RGB     = 1<<2,
    IPANEL_VIDEO_OUTPUT_YPBPR   = 1<<3,
    IPANEL_VIDEO_OUTPUT_HDMI    = 1<<4
}IPANEL_VDIS_VIDEO_OUTPUT_e;

typedef enum // ipanel_vout.h
{
    IPANEL_DIS_TVENC_NTSC,
    IPANEL_DIS_TVENC_PAL,
    IPANEL_DIS_TVENC_SECAM,
    IPANEL_DIS_TVENC_AUTO
} IPANEL_DIS_TVENC_MODE_e;

typedef enum // ipanel_vout.h
{
    IPANEL_DIS_AR_FULL_SCREEN,
    IPANEL_DIS_AR_PILLARBOX,
    IPANEL_DIS_AR_LETTERBOX,
    IPANEL_DIS_AR_PAN_SCAN
} IPANEL_DIS_AR_MODE_e;

typedef struct // ipanel_vout.h
{
    UINT32_T x;
    UINT32_T y;
    UINT32_T w;
    UINT32_T h;
} IPANEL_RECT;
*/

typedef enum
{
    IPANEL_AVM_SET_CHANNEL_MODE		= 0,
    IPANEL_AVM_SET_MUTE				= 1,
    IPANEL_AVM_SET_PASS_THROUGH		= 2,
    IPANEL_AVM_SET_VOLUME			= 3,
    IPANEL_AVM_SELECT_DEV			= 4,
    IPANEL_AVM_ENABLE_DEV			= 5,
    IPANEL_AVM_SET_TVMODE			= 6,
    IPANEL_AVM_SET_VISABLE			= 7,
    IPANEL_AVM_SET_ASPECT_RATIO		= 8,
    IPANEL_AVM_SET_WIN_LOCATION		= 9,
    IPANEL_AVM_SET_WIN_TRANSPARENT	= 10,
    IPANEL_AVM_SET_CONTRAST			= 11,
    IPANEL_AVM_SET_HUE				= 12,
    IPANEL_AVM_SET_BRIGHTNESS		= 13,
    IPANEL_AVM_SET_SATURATION		= 14,
    IPANEL_AVM_SET_SHARPNESS		= 15,
    IPANEL_AVM_PLAY_I_FRAME			= 16,       
    IPANEL_AVM_STOP_I_FRAME			= 17,       
    IPANEL_AVM_SET_STREAM_TYPE		= 18,
    IPANEL_AVM_START_AUDIO			= 19,
    IPANEL_AVM_STOP_AUDIO			= 20,
	IPANEL_AVM_SET_HD_RES			= 21,
	IPANEL_AVM_GET_VOLUME			= 22,
	IPANEL_AVM_GET_MUTE				= 23
} IPANEL_AVM_IOCTL_e;

/*AV Manager*/
INT32_T ipanel_avm_play_srv(IPANEL_SERVICE_INFO *srv);

INT32_T ipanel_avm_stop_srv(VOID);

INT32_T ipanel_avm_ioctl(IPANEL_AVM_IOCTL_e op, VOID *arg);

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_AV_MANAGER_API_FUNCTOTYPE_H_
