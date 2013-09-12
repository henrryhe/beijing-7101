/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embxlb_module.c                                           */
/*                                                                 */
/* Description:                                                    */
/*         Linux kernel module functionality                       */
/*                                                                 */
/*******************************************************************/

#include "embx_osinterface.h"
#include "embxlb.h"

#if defined(__LINUX__) && defined(__KERNEL__) && defined(MODULE)

/* Public API exports */

EXPORT_SYMBOL( EMBXLB_loopback_factory );

MODULE_DESCRIPTION("Extended EMBX Loopback Transport");
MODULE_AUTHOR("STMicroelectronics Ltd");
MODULE_LICENSE("(c) 2002 STMicroelectronics, All rights reserved");

int __init embx_init( void )
{
    return 0;
}

void __exit embx_deinit( void )
{
    return;
}

module_init(embx_init);
module_exit(embx_deinit);

#endif /* __LINUX__ */
