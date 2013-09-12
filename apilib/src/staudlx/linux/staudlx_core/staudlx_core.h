/*****************************************************************************
 *
 *  Module      : staudlx_core.h
 *  Date        : 02-07-2005
 *  Author      : WALKERM
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005 
 *****************************************************************************/


#ifndef STAUDLX_CORE_H
#define STAUDLX_CORE_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */



/*** IOCtl defines ***/

#define STAUDLX_CORE_TYPE   0XFF     /* Unique module id - See 'ioctl-number.txt' */

#define STAUDLX_CORE_CMD0   _IO  (STAUDLX_CORE_TYPE, 0)         /* icoctl() - no argument */
#define STAUDLX_CORE_CMD1   _IOR (STAUDLX_CORE_TYPE, 1, int)    /* icoctl() - read an int argument */
#define STAUDLX_CORE_CMD2   _IOW (STAUDLX_CORE_TYPE, 2, long)   /* icoctl() - write a long argument */
#define STAUDLX_CORE_CMD3   _IOWR(STAUDLX_CORE_TYPE, 3, double) /* icoctl() - read/write a double argument */                                                                                                            



#endif
