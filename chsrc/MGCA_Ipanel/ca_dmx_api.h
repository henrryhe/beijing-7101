/******************************************************************************/
//  Copyright (c) 2008 iPanel Technologies, Ltd.
//  All rights reserved. You are not allowed to copy or distribute
//  the code without permission.
//  History:
//     Date        Author        Changes
//     2008/01/16   huzh         Created  (Version 0.1)
/******************************************************************************/
#ifndef __IPANEL_CAM_DMX_API_2008_01_16__H___
#define __IPANEL_CAM_DMX_API_2008_01_16__H___

#include "ipanel_typedef.h"

#define IPANEL_FILTER_DEPTH_MAX	16
typedef INT32_T (*IPANEL_FILTER_NOTIFY)(VOID* tag, BYTE_T* data, INT32_T len);

typedef struct
{
	UINT16_T	pid;
	BYTE_T		coef[IPANEL_FILTER_DEPTH_MAX];
	BYTE_T		mask[IPANEL_FILTER_DEPTH_MAX];
	BYTE_T		excl[IPANEL_FILTER_DEPTH_MAX];
	BYTE_T		depth;
	IPANEL_FILTER_NOTIFY func;
	VOID		*tag;/*用户自定义字段*/
}IPANEL_FILTER_INFO;

//API 
INT32_T ipanel_add_filter(IPANEL_FILTER_INFO *pfilter);
INT32_T ipanel_remove_filter(INT32_T filter);
INT32_T ipanel_modify_filter(INT32_T filter, IPANEL_FILTER_INFO *pfilter);

#endif //__IPANEL_CAM_DMX_API_2008_01_16__H___
