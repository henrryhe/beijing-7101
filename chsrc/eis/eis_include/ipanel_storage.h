/******************************************************************************/
/*    Copyright (c) 2008 iPanel Technologies, Ltd.                     */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is interface definition for storage device                          */
/*                                                                            */
/*    $author tujz 2008/10/31                                           */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_STORAGE_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_STORAGE_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    IPANEL_STORAGE_GET_DEV_NUM 		= 1,
    IPANEL_STORAGE_GET_LOGICDEV_NUM = 2,
    IPANEL_STORAGE_GET_DEV_INFO 	= 3,
    IPNAEL_STORAGE_FORMAT_DEV 		= 4,
    IPANEL_STORAGE_REMOVE_DEV 		= 5,
    IPANEL_STORAGE_CREATE_LOGICDEV	= 6,
    IPANEL_STORAGE_DELETE_LOGICDEV	= 7
} IPANEL_STORAGE_IOCTL_e;

typedef enum
{
    IPANEL_STORAGE_DRIVE_UNKNOWN 	= 0,	/* The drive type cannot be determined.  */
	IPANEL_STORAGE_DRIVE_REMOVABLE	= 1,	/* The disk can be removed from the drive. */
	IPANEL_STORAGE_DRIVE_FIXED		= 2,	/* The disk cannot be removed from the drive. */
	IPANEL_STORAGE_DRIVE_REMOTE		= 3,	/* The drive is a remote (network) drive. */
	IPANEL_STORAGE_DRIVE_CDROM		= 4,	/* The drive is a CD-ROM drive. */
	IPANEL_STORAGE_DRIVE_RAMDISK	= 5		/* The drive is a RAM disk. */
} IPANEL_STORAGE_DRIVE_TYPE_e;

typedef struct
{
	INT32_T index;						/*设备索引号,标志一个物理设备*/
	INT32_T subidx;						/*逻辑设备索引号,标志一个物理设备上的逻辑，等于0表示是物理设备本身，大于0，表示查询物理设备上的逻辑设备*/
	CHAR_T logic;						/*逻辑设备号，一个物理设备上可以有多个逻辑设备*/
	CHAR_T name[32];					/*逻辑设备名称*/
	IPANEL_STORAGE_DRIVE_TYPE_e type;	/*物理设备类型*/
	UINT32_T start;						/*逻辑设备空间起始位置，以KB为单位*/
	UINT32_T size;						/*物理或逻辑设备存储空间大小，以KB为单位*/
	UINT32_T free;						/*物理或逻辑设备空闲空间大小，以KB为单位*/
}IPANEL_STORAGE_DEV_INFO;

INT32_T ipanel_porting_storage_ioctl(IPANEL_STORAGE_IOCTL_e cmd, VOID *arg);

#ifdef __cplusplus
}
#endif

#endif//_IPANEL_MIDDLEWARE_PORTING_STORAGE_API_FUNCTOTYPE_H_
