/******************************************************************************

File name   : net_ioctl.h

COPYRIGHT (C) 2007 STMicroelectronics

******************************************************************************/
#ifndef __NET_IOCTL_H
#define __NET_IOCTL_H


ST_ErrorCode_t STNETi_ioctl_Init(const ST_DeviceName_t DeviceName,
		const STNET_InitParams_t *InitParams);
ST_ErrorCode_t STNETi_ioctl_Term(const ST_DeviceName_t DeviceName,
		const STNET_TermParams_t *TermParams);

ST_ErrorCode_t STNETi_ioctl_Open(const ST_DeviceName_t DeviceName,
		const STNET_OpenParams_t *OpenParams, STNET_Handle_t *Handle);
ST_ErrorCode_t STNETi_ioctl_Close(STNET_Handle_t Handle);

ST_ErrorCode_t STNETi_ioctl_Start(STNET_Handle_t Handle,
		const STNET_StartParams_t *StartParams);
ST_ErrorCode_t STNETi_ioctl_Stop(STNET_Handle_t Handle);

ST_ErrorCode_t STNETi_ioctl_SetReceiverConfig(STNET_Handle_t Handle,
		STNET_ReceiverConfigParams_t *ConfigParams);

#endif
