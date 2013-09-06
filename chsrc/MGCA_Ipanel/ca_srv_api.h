/******************************************************************************/
//  Copyright (c) 2008 iPanel Technologies, Ltd.
//  All rights reserved. You are not allowed to copy or distribute
//  the code without permission.
//  History:
//     Date        Author        Changes
//     2008/01/16   huzh         Created  (Version 0.1)
/******************************************************************************/
#ifndef __IPANEL_CAM_SRV_API_2008_01_16__H___
#define __IPANEL_CAM_SRV_API_2008_01_16__H___

#include "ipanel_typedef.h"

#define IPANEL_SRV_NAME_LEN_MAX 128
#define IPANEL_CAM_SRV_MAX		8

typedef enum {
	IPANEL_CAM_ADD_SRV,
	IPANEL_CAM_DEL_SRV,
	IPANEL_CAM_UPDATE_SRV
}IPANEL_CAM_ECM_CTRL_e;

typedef struct
{
	UINT16_T original_network_id;
	UINT16_T transport_stream_id;
	UINT16_T service_id;
	BYTE_T	 servicename[IPANEL_SRV_NAME_LEN_MAX];
	
	UINT16_T video_pid;
	UINT16_T audio_pid;
	UINT16_T pcr_pid;

	/* 同密情况下, PMT common loop会有多个CA_descriptor */
	UINT16_T pmt_pid;	
	//同密情况
	UINT16_T ecmPids[IPANEL_CAM_SRV_MAX];
	UINT16_T ecmCaSysIDs[IPANEL_CAM_SRV_MAX];
	//不同密情况,跟同密情况互斥。
	UINT16_T audioEcmPids[IPANEL_CAM_SRV_MAX];
	UINT16_T audioCaSysIDs[IPANEL_CAM_SRV_MAX];
	UINT16_T videoEcmPids[IPANEL_CAM_SRV_MAX];
	UINT16_T videoCaSysIDs[IPANEL_CAM_SRV_MAX];
} IPANEL_CAM_SRV_INFO;

//API
INT32_T ipanel_cam_srv_ctrl(IPANEL_CAM_ECM_CTRL_e act, IPANEL_CAM_SRV_INFO *psrv);

#endif //__IPANEL_CAM_SRV_API_2008_01_16__H___
