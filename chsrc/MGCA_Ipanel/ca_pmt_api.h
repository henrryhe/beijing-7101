/********************************************************************************/
/*  Copyright (c) 2008 iPanel Technologies, Ltd.								*/
/*  All rights reserved. You are not allowed to copy or distribute				*/
/*  the code without permission.												*/
/*     2008/01/16   huzh         Created  (Version 0.1)							*/
/********************************************************************************/
#ifndef __IPANEL_CAM_PMT_API_2008_01_16__H___
#define __IPANEL_CAM_PMT_API_2008_01_16__H___

#include "ipanel_typedef.h"

typedef enum {
	IPANEL_CAM_SET_PMT		= 1,
	IPANEL_CAM_UPDATE_PMT	= 2
}IPANEL_CAM_PMT_CTRL_e;

//API
INT32_T ipanel_cam_pmt_ctrl(IPANEL_CAM_PMT_CTRL_e act, BYTE_T *data, INT32_T len);

#endif //__IPANEL_CAM_PMT_API_2008_01_16__H___
