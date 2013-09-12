/*******************************************************************************

File name   : avm_allo.h

Description : Audio and video memory module memory allocation header file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                             Name
----               ------------                             ----
17 jun 99           Created                                  HG
 6 Oct 99           Introduce partitions functions           HG
18 Apr 01           remove export of
 *                  stavmem_BlockHandleBelongsToSpace        HSdLM
01 Jun 01           Add stavmem_ before non API exported     HSdLM
 *                  symbols
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __AVMEM_ALLO_H
#define __AVMEM_ALLO_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "avm_init.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define AVMEM_INVALID_PARTITION_HANDLE ((STAVMEM_PartitionHandle_t) NULL)

/* STAVMEM_DRIVER_ID = 39d = 0x027 -> 0x02072070. See code templates */
#define AVMEM_VALID_PARTITION     0x02072070



/* Exported Types ----------------------------------------------------------- */

typedef struct
{
    stavmem_Device_t *Device_p;          /* Link to the device where the partition was created */
    U32              Validity;           /* Only the value AVMEM_VALID_PARTITION means the unit is valid */

    MemBlockDesc_t   *VeryTopBlock_p;    /* Block on the top of the list of blocks of the partition */
    MemBlockDesc_t   *VeryBottomBlock_p; /* Blocks at the bottom of the list of blocks of the partition */

    MemBlockDesc_t   *TopOfFreeBlocksList_p;
} stavmem_Partition_t;


/* Exported Variables ------------------------------------------------------- */

extern stavmem_Partition_t stavmem_PartitionArray[STAVMEM_MAX_MAX_PARTITION];


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

/*extern ST_ErrorCode_t stavmem_BlockHandleBelongsToPartition(STAVMEM_BlockHandle_t BlockHandle, STAVMEM_PartitionHandle_t PartitionHandle);*/


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __AVMEM_ALLO_H */

/* End of avm_allo.h */
