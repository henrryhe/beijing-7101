/*****************************************************************************
 *
 *  Module      : fdmatest
 *  Date        : 05-04-2005
 *  Author      : WALKERM
 *  Description :
 *
 *****************************************************************************/


#ifndef FDMATEST_H
#define FDMATEST_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */
#include "compat.h"

#include "stfdma.h"

/*** IOCtl defines ***/

#define FDMATEST_TYPE   0XF0     /* Unique module id - See 'ioctl-number.txt' */

#define IO_STFDMA_StartTest            _IO  (FDMATEST_TYPE, 0)

#endif
