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

#ifndef _IPANEL_MIDDLEWARE_PORTING_MIC_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_MIC_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*IPANEL_MIC_NOTIFY)( UINT32_T handle, INT32_T event, VOID *param);
UINT32_T ipanel_porting_mic_open(IPANEL_MIC_NOTIFY func);
INT32_T ipanel_porting_mic_close(UINT32_T handle);
INT32_T ipanel_porting_mic_read(UINT32_T handle, BYTE_T *buf, UINT32_T len);
/*op*/
typedef enum
{
    IPANEL_MIC_START        = 1,
    IPANEL_MIC_STOP         = 2,
    IPANEL_MIC_CLEAR_BUFFER = 3,
    IPANEL_MIC_SET_PARAM    = 4
} IPANEL_MIC_IOCTL_e;
INT32_T ipanel_porting_mic_ioctl(UINT32_T handle, IPANEL_MIC_IOCTL_e op, VOID *arg);

#ifdef __cplusplus
}
#endif

#endif // _IPANEL_MIDDLEWARE_PORTING_MIC_API_FUNCTOTYPE_H_
