/******************************************************************************/
//  Copyright (c) 2008 iPanel Technologies, Ltd.
//  All rights reserved. You are not allowed to copy or distribute
//  the code without permission.
//  History:
//     Date        Author        Changes
//     2008/01/16   huzh         Created  (Version 0.1)
/******************************************************************************/
#ifndef __IPANEL_CAM_EMM_API_2008_01_16__H___
#define __IPANEL_CAM_EMM_API_2008_01_16__H___

#include "ipanel_typedef.h"

typedef enum {
	IPANEL_CAM_SET_EMM,
	IPANEL_CAM_STOP_EMM,
	IPANEL_CAM_UPDATE_EMM
}IPANEL_CAM_EMM_CTRL_e;

//API
INT32_T ipanel_cam_emm_ctrl(IPANEL_CAM_EMM_CTRL_e act, BYTE_T *data, INT32_T len);

#endif //__IPANEL_CAM_EMM_API_2008_01_16__H___
