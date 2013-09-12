/*******************************************************************************

File name   : memory_map.h

Description : Define the memory mapping for the different chips

COPYRIGHT (C) STMicroelectronics 2007.

Date               Modification                                     Name
----               ------------                                     ----
07 Mar 2007        Creation                                         LF
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __MEMORY_MAP_H
#define __MEMORY_MAP_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stdevice.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Memory Mapping ------------------------------------------------------------- */
/* Defines various constants for STAPIGAT/STAVMEM Partitions */
#if defined(ST_7100) || defined(ST_7109)
#define NCACHE_SH4_TO_STBUS_ADDRESS_OFFSET  0xA0000000

#define EMBX_SHARED_MEMORY_SIZE             (512*1024)
#endif
#if defined(ST_7200)
/* MEMORY MAP for MB519
 * We have 128 MBytes on LMI0 and 64 MBytes on LMI1 on the MB519 divided as followed :
 * LMI0 :
 * - SH4 segment           ( 16M) : All ST40 code, defined through board.mem
 * - STAVMEM partition     (100M) : for any driver
 * - NCACHE partition      (  2M) : For caching
 * - LX Audio segment      (  2M) : All audio ST231 code
 * - LX Audio segment      (  2M) : All audio ST231 code
 * - LX Video segment      (  3M) : All video ST231 code
 * - LX Video segment      (  3M) : All video ST231 code
 * LMI1 :
 * - STAVMEM partition      (64M) : for any driver
 *
 * STi7200 mapping
 *  0xA0000000  -------------------------------
 *              | EMI (64MB)                  |
 *  0xA8000000  -------------------------------
 *              | LX_VIDEO1_CODE_MEMORY (3MB) |
 *  0xA8300000  -------------------------------
 *              | LX_VIDEO2_CODE_MEMORY (3MB) |
 *  0xA8600000  -------------------------------
 *              | LX_AUDIO1_CODE_MEMORY (2MB) |
 *  0xA8800000  -------------------------------
 *              | LX_AUDIO2_CODE_MEMORY (2MB) |
 *  0xA8A00000  -------------------------------
 *              | NCACHE PARTITION      (2MB) |
 *  0xA8C00000  -------------------------------
 *              | SH4_CODE_MEMORY      (16MB) |
 *  0xA9C00000  -------------------------------
 *              | STAVMEM_SYS_MEMORY  (100MB) |
 *  0xAF000000  -------------------------------
 *              ...............................
 *  0xB8000000  -------------------------------
 *              | STAVMEM_VID_MEMORY   (64MB) |
 *  0xBC000000  -------------------------------
 */
/* STb7200 BASE ADDRESSES ----------------------------------------------------- */
#define EMI_BASE                            0xA0000000
#define LMI0_BASE                           0xA8000000
#define LMI1_BASE                           0xB8000000
#define PERIPH_BASE                         0xFD000000

/* Memory sizes */
#define LMI0_SIZE                           (128*1024*1024)
#define LMI1_SIZE                           (64*1024*1024)

/* Make partition stop 2 lines of MB (HD) before the end of memory, so that Deltaphi, when reading data from the frame buffer
 * does not request data outside of the memory. (Prediction block in Deltaphi, when reading Frame buffers)
 * Info: frame buffers are allocated on top of the memory
 * 1920 = nb pixels Luma, HD
 * 32 = number of lines of pixels in 2 lines of Luma MB
 */
#define LMI_FRAME_BUFFERS_MARGIN            (   1920 * 32)  /* Margin to avoid deltaphi/mu access outsie LMI */

/* Memory sections sizes */
#define LX_VIDEO1_CODE_MEMORY_SIZE          (  3*1024*1024)
#define LX_VIDEO2_CODE_MEMORY_SIZE          (  3*1024*1024)
#define LX_AUDIO1_CODE_MEMORY_SIZE          (  2*1024*1024)
#define LX_AUDIO2_CODE_MEMORY_SIZE          (  2*1024*1024)
#define NCACHE_PARTITION_MEMORY_SIZE        (  2*1024*1024)
#define SH4_CODE_MEMORY_SIZE                ( 16*1024*1024)
#define SH4_SYSTEM_PARTITION_MEMORY_SIZE    (  8*1024*1024) /* Included in SH4_CODE_MEMORY_SIZE */
#define AVMEM0_MEMORY_SIZE                  (LMI0_SIZE                      \
                                             - LX_VIDEO1_CODE_MEMORY_SIZE   \
                                             - LX_VIDEO2_CODE_MEMORY_SIZE   \
                                             - LX_AUDIO1_CODE_MEMORY_SIZE   \
                                             - LX_AUDIO2_CODE_MEMORY_SIZE   \
                                             - NCACHE_PARTITION_MEMORY_SIZE \
                                             - SH4_CODE_MEMORY_SIZE         \
                                             - LMI_FRAME_BUFFERS_MARGIN)

#define EMBX_SHARED_MEMORY_SIZE             (     128*1024) /* Included in STVID AvmemPartition */
#define AVMEM1_MEMORY_SIZE                  (LMI1_SIZE - LMI_FRAME_BUFFERS_MARGIN)

#define NCACHE_SH4_TO_STBUS_ADDRESS_OFFSET  0xA0000000

/* Memory sections offsets */
#define LX_VIDEO1_CODE_OFFSET               (0x00000000)
#define LX_VIDEO1_CODE_END_OFFSET           (LX_VIDEO1_CODE_OFFSET + LX_VIDEO1_CODE_MEMORY_SIZE)

#define LX_VIDEO2_CODE_OFFSET               (LX_VIDEO1_CODE_END_OFFSET)
#define LX_VIDEO2_CODE_END_OFFSET           (LX_VIDEO2_CODE_OFFSET + LX_VIDEO2_CODE_MEMORY_SIZE)

#define LX_AUDIO1_CODE_OFFSET               (LX_VIDEO2_CODE_END_OFFSET)
#define LX_AUDIO1_CODE_END_OFFSET           (LX_AUDIO1_CODE_OFFSET + LX_AUDIO1_CODE_MEMORY_SIZE)

#define LX_AUDIO2_CODE_OFFSET               (LX_AUDIO1_CODE_END_OFFSET)
#define LX_AUDIO2_CODE_END_OFFSET           (LX_AUDIO2_CODE_OFFSET + LX_AUDIO2_CODE_MEMORY_SIZE)

#define NCACHE_PARTITION_OFFSET             (LX_AUDIO2_CODE_END_OFFSET)
#define NCACHE_PARTITION_END_OFFSET         (NCACHE_PARTITION_OFFSET + NCACHE_PARTITION_MEMORY_SIZE)

#define SH4_CODE_OFFSET                     (NCACHE_PARTITION_END_OFFSET)
#define SH4_CODE_END_OFFSET                 (SH4_CODE_OFFSET + SH4_CODE_MEMORY_SIZE)

#define AVMEM_SYS_OFFSET                    (SH4_CODE_END_OFFSET)
#define AVMEM_SYS_END_OFFSET                (AVMEM_SYS_OFFSET + AVMEM_SYS_MEMORY_SIZE)

#define AVMEM_VID_OFFSET                    (0x00000000)
#define AVMEM_VID_END_OFFSET                (AVMEM_VID_OFFSET + AVMEM_VID_MEMORY_SIZE)

#define AVMEM0_OFFSET                       (SH4_CODE_END_OFFSET)
#define AVMEM0_END_OFFSET                   (AVMEM0_OFFSET + AVMEM0_MEMORY_SIZE)

#define AVMEM1_OFFSET                       (0x00000000)
#define AVMEM1_END_OFFSET                   (AVMEM1_OFFSET + AVMEM1_MEMORY_SIZE)

/* Memory sections addresses */
#define LX_VIDEO1_CODE_ADDRESS              (LMI0_BASE + LX_VIDEO1_CODE_OFFSET)
#define LX_VIDEO1_CODE_END_ADDRESS          (LMI0_BASE + LX_VIDEO1_CODE_END_OFFSET)

#define LX_VIDEO2_CODE_ADDRESS              (LMI0_BASE + LX_VIDEO2_CODE_OFFSET)
#define LX_VIDEO2_CODE_END_ADDRESS          (LMI0_BASE + LX_VIDEO2_CODE_END_OFFSET)

#define LX_AUDIO1_CODE_ADDRESS              (LMI0_BASE + LX_AUDIO1_CODE_OFFSET)
#define LX_AUDIO1_CODE_END_ADDRESS          (LMI0_BASE + LX_AUDIO1_CODE_END_OFFSET)

#define LX_AUDIO2_CODE_ADDRESS              (LMI0_BASE + LX_AUDIO2_CODE_OFFSET)
#define LX_AUDIO2_CODE_END_ADDRESS          (LMI0_BASE + LX_AUDIO2_CODE_END_OFFSET)

#define NCACHE_PARTITION_ADDRESS            (LMI0_BASE + NCACHE_PARTITION_OFFSET)
#define NCACHE_PARTITION_END_ADDRESS        (LMI0_BASE + NCACHE_PARTITION_END_OFFSET)

#define SH4_CODE_ADDRESS                    (LMI0_BASE + SH4_CODE_OFFSET)
#define SH4_CODE_END_ADDRESS                (LMI0_BASE + SH4_CODE_END_OFFSET)

#define AVMEM0_ADDRESS                      (LMI0_BASE + AVMEM0_OFFSET)
#define AVMEM0_END_ADDRESS                  (LMI0_BASE + AVMEM0_END_OFFSET)

#define AVMEM1_ADDRESS                      (LMI1_BASE + AVMEM1_OFFSET)
#define AVMEM1_END_ADDRESS                  (LMI1_BASE + AVMEM1_END_OFFSET)

/*#define SDRAM_BASE_ADDRESS              0*/
/*#define SDRAM_SIZE                      0xffffffff*/
#define BM_BASE_ADDRESS  0
#define BM_BASE	BM_BASE_ADDRESS

#endif /* #if defined(ST_7200) ... */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MEMORY_MAP_H */
