/*****************************************************************************

File name   : Dumpreg.h

Description : Register test

COPYRIGHT (C) ST-Microelectronics 1999.

Date               	Modification                 				Name
----               	------------                 				----
23-March 2001       Valid. software modified                    BD

*****************************************************************************/

#ifndef __DUMP_REG_H
#define __DUMP_REG_H

/* Includes --------------------------------------------------------------- */
#include <stddefs.h>
#include <stsys.h>
#include <testtool.h>

#ifdef ST_OSLINUX
#include "compat.h"
/*#else */
#endif /* ST_OSLINUX */

#include <sttbx.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

typedef struct
{
     const char   *Name;                      /* Name */
     U32    Base;                       /* Base address of the Array */
     U32    Offset;                     /* Offset from Base address */
     U32    RdMask;                     /* Mask for read operations */
     U32    WrMask;                     /* Mask for write operations */
     U32    RstVal;                     /* Reset state */
     U32    Type;                       /* Register characteristics */
     U32    Size;                   	/* Circular or Array register size */
} Register_t;



/* Exported Constants ----------------------------------------------------- */

#define REG_8B          		0x001            /* 8 bits register */
#define REG_16B         		0x002            /* 16 bits register */
#define REG_24B         		0x004            /* 24 bits register */
#define REG_32B         		0x008            /* 32 bits register */
#define REG_SIDE_READ      		0x010            /* Reading register has side-effects */
#define REG_CIRCULAR    		0x020            /* Circular register */
#define REG_ARRAY       		0x040            /* Array of registers */
#define REG_SPECIAL     		0x080            /* Special register */
#define REG_NEED_CLK    		0x100            /* Register needs clock */
#define REG_EXCL_FROM_TEST		0x200            /* Exclude from test (temporarily) */
#define REG_SIDE_WRITE     		0x800            /* Writing register has side-effects */

#define DEFAULT_MASK	(~(0x0))		/* Default mask: all 1 */
#define DEFAULT_TYPE	REG_32B			/* Default register size */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t DUMPREG_Init(void);
BOOL DUMPREG_RegisterCmd(void);

#ifdef __cplusplus
}
#endif

#endif /* __DUMP_REG_H */
/* ------------------------------- End of file ---------------------------- */

