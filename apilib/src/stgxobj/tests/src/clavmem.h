/*******************************************************************************

File name   : clavmem.h

Description : AVMEM configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
09 Apr 2002        Created                                          HSdLM
14 Oct 2002        Change code compiled for mb290                   HSdLM
28 Oct 2002        Hardware or software bug for mb290               BS
20 Nov 2002        mb290 board issue work around can be disabled    HSdLM
02 Apr 2003        Add support of db573, mb376 and 5528             HSdLM
17 Apr 2003        Multi-partitions support                         HSdLM
 *                 remove mb290 board issue work around
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLAVMEM_H
#define __CLAVMEM_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stavmem.h"
#include "memory_map.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STAVMEM_DEVICE_NAME       "AVMEM"

#if defined(mb231) || defined(mb282b) || defined(mb275) || defined(mb275_64)
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 0
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS
#define VIRTUAL_SIZE                       SDRAM_SIZE
#define VIRTUAL_WINDOW_SIZE                SDRAM_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            SDRAM_BASE_ADDRESS
#define MPEG_SDRAM_SIZE                    SDRAM_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   SDRAM_BASE_ADDRESS
#define PARTITION0_STOP                    (SDRAM_BASE_ADDRESS + SDRAM_SIZE - 1)

#elif defined(mb295) || defined(mb290)
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_WINDOW_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 0
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS
#define VIRTUAL_SIZE                       SDRAM_SIZE
#define VIRTUAL_WINDOW_SIZE                SDRAM_WINDOW_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            SDRAM_BASE_ADDRESS
#define MPEG_SDRAM_SIZE                    SDRAM_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   SDRAM_BASE_ADDRESS
#define PARTITION0_STOP                    (SDRAM_BASE_ADDRESS + SDRAM_SIZE - 1)

#elif ((defined(mb314) || defined(mb361) || defined(mb382)) && !defined(ST_7020)) /* not db573 */
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 0
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS
#ifdef UNIFIED_MEMORY /* be sure we won't work in CODE section ! */
#define VIRTUAL_SIZE                       0x00300000 /* see mb314_um.cfg and mb361_um.cfg */
#define VIRTUAL_WINDOW_SIZE                0x00300000
#define MPEG_SDRAM_SIZE                    0x00300000
#define PARTITION0_STOP                    (SDRAM_BASE_ADDRESS + 0x00300000 - 1)
#else
#define VIRTUAL_SIZE                       SDRAM_SIZE
#define VIRTUAL_WINDOW_SIZE                SDRAM_SIZE
#define MPEG_SDRAM_SIZE                    SDRAM_SIZE
#define PARTITION0_STOP                    (SDRAM_BASE_ADDRESS + SDRAM_SIZE - 1)
#endif /* #ifdef UNIFIED_MEMORY */
#define MPEG_SDRAM_BASE_ADDRESS            SDRAM_BASE_ADDRESS
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   SDRAM_BASE_ADDRESS

#elif defined(mb314) && defined(ST_7020)                                  /* db573 */
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_BASE_ADDRESS /* min(SDRAM_BASE_ADDRESS,MB314_SDRAM_BASE_ADDRESS) */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0 /* from 7020 */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 (S32)(SDRAM_BASE_ADDRESS - MB314_SDRAM_BASE_ADDRESS) /* from 55xx */
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       (MB314_SDRAM_BASE_ADDRESS + MB314_SDRAM_SIZE - SDRAM_BASE_ADDRESS)
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            MB314_SDRAM_BASE_ADDRESS
#define MPEG_SDRAM_SIZE                    MB314_SDRAM_SIZE
#define MAX_PARTITIONS                     2
#define PARTITION0_START                   SDRAM_BASE_ADDRESS
#define PARTITION0_STOP                    (SDRAM_BASE_ADDRESS + SDRAM_SIZE - 1)
#define PARTITION1_START                   MB314_SDRAM_BASE_ADDRESS
#define PARTITION1_STOP                    (MB314_SDRAM_BASE_ADDRESS + MB314_SDRAM_SIZE - 1)

#elif ((defined(mb376) && !defined(ST_7020)) || defined (espresso))
#ifdef ST_OSLINUX

#undef  AVMEM_BASE_ADDRESS
#undef  AVMEM_SIZE
#define AVMEM_BASE_ADDRESS                 0x0000000   /* Start Address bigphysarea (Not used)*/
#define AVMEM_SIZE                         0xE00000    /* Size of bigphysarea allocated */

#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_BASE_ADDRESS /* defined in mb376.h */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 AVMEM_BASE_ADDRESS
#define VIRTUAL_BASE_ADDRESS               AVMEM_BASE_ADDRESS /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       AVMEM_SIZE         /* defined in mb376.h or espresso.h */
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            AVMEM_BASE_ADDRESS /* no MPEG block move on 5528 */
#define MPEG_SDRAM_SIZE                    AVMEM_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   AVMEM_BASE_ADDRESS
#define PARTITION0_STOP                    (AVMEM_BASE_ADDRESS + AVMEM_SIZE - 1)
#endif /* ST_OSLINUX */

#ifdef ST_OS21
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_ST40_BASE_ADDRESS /* defined in mb376.h */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 AVMEM_BASE_ADDRESS
#define VIRTUAL_BASE_ADDRESS               AVMEM_ST40_BASE_ADDRESS /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       AVMEM_SIZE         /* defined in mb376.h */
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            AVMEM_ST40_BASE_ADDRESS  /* no MPEG block move on 5528 */
#define MPEG_SDRAM_SIZE                    AVMEM_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   AVMEM_ST40_BASE_ADDRESS
#define PARTITION0_STOP                    (AVMEM_ST40_BASE_ADDRESS + AVMEM_SIZE - 1)
#endif  /* ST_OS21 */
#ifdef ST_OS20
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_BASE_ADDRESS /* defined in mb376.h */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 AVMEM_BASE_ADDRESS
#define VIRTUAL_BASE_ADDRESS               AVMEM_BASE_ADDRESS /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       AVMEM_SIZE         /* defined in mb376.h */
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            AVMEM_BASE_ADDRESS /* no MPEG block move on 5528 */
#define MPEG_SDRAM_SIZE                    AVMEM_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   AVMEM_BASE_ADDRESS
#define PARTITION0_STOP                    (AVMEM_BASE_ADDRESS + AVMEM_SIZE - 1)
#endif /* ST_OS20 */
#elif defined(mb376) && defined(ST_7020)
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_BASE_ADDRESS /* (STI7020_BASE_ADDRESS+0x02000000)  */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 AVMEM_BASE_ADDRESS /* from 55xx */
#define VIRTUAL_BASE_ADDRESS               AVMEM_BASE_ADDRESS /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       AVMEM_SIZE
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            AVMEM_BASE_ADDRESS /* no MPEG block move on 5528 */
#define MPEG_SDRAM_SIZE                    AVMEM_SIZE
#define MAX_PARTITIONS                     2
#define PARTITION0_START                   SDRAM_BASE_ADDRESS
#define PARTITION0_STOP                    (SDRAM_BASE_ADDRESS + SDRAM_SIZE - 1)
#define PARTITION1_START                   MB376_SDRAM_BASE_ADDRESS
#define PARTITION1_STOP                    (MB376_SDRAM_BASE_ADDRESS + MB376_SDRAM_SIZE - 1)

#elif defined(mb382) && defined(ST_7020)                                /* db573 */
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     SDRAM_BASE_ADDRESS /* min(SDRAM_BASE_ADDRESS,MB382_SDRAM_BASE_ADDRESS) */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  0 /* from 7020 */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 (S32)(SDRAM_BASE_ADDRESS - MB382_SDRAM_BASE_ADDRESS) /* from 55xx */
#define VIRTUAL_BASE_ADDRESS               SDRAM_BASE_ADDRESS /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       (MB382_SDRAM_BASE_ADDRESS + MB382_SDRAM_SIZE - SDRAM_BASE_ADDRESS)
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            MB382_SDRAM_BASE_ADDRESS
#define MPEG_SDRAM_SIZE                    MB382_SDRAM_SIZE
#define MAX_PARTITIONS                     2
#define PARTITION0_START                   SDRAM_BASE_ADDRESS
#define PARTITION0_STOP                    (SDRAM_BASE_ADDRESS + SDRAM_SIZE - 1)
#define PARTITION1_START                   MB382_SDRAM_BASE_ADDRESS
#define PARTITION1_STOP                    (MB382_SDRAM_BASE_ADDRESS + MB382_SDRAM_SIZE - 1)

#elif defined(mb317a) || defined(mb317b)
#define RAM_SIZE                           0x04000000  /* 64 MBytes */
#define AVMEM_BASE_ADDRESS                 (LMI_BASE_ADDRESS + (RAM_SIZE>>1))
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  (AVMEM_BASE_ADDRESS & 0x1fffffff)
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 0 /* not used */
#define VIRTUAL_BASE_ADDRESS               AVMEM_BASE_ADDRESS
#define VIRTUAL_SIZE                       (RAM_SIZE>>1)
#define VIRTUAL_WINDOW_SIZE                (RAM_SIZE>>1)
#define SDRAM_BASE_ADDRESS                 0
#define MPEG_SDRAM_BASE_ADDRESS            0
#define MPEG_SDRAM_SIZE                    0
#define VIDEO_BASE_ADDRESS                 0    /* no MPEG1D/2D copy method */
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   AVMEM_BASE_ADDRESS
#define PARTITION0_STOP                    (AVMEM_BASE_ADDRESS + (RAM_SIZE>>1) - 1)

#elif defined(mb390) || defined(mb391)

#if defined(ST_5100) || defined(ST_7710)
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_BASE_ADDRESS /* defined in mb390.h */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 AVMEM_BASE_ADDRESS
#define VIRTUAL_BASE_ADDRESS               AVMEM_BASE_ADDRESS /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       AVMEM_SIZE         /* defined in mb390.h */
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            AVMEM_BASE_ADDRESS /* no MPEG block move on 5100 */
#define MPEG_SDRAM_SIZE                    AVMEM_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   AVMEM_BASE_ADDRESS
#define PARTITION0_STOP                    (AVMEM_BASE_ADDRESS + AVMEM_SIZE - 1)
#define PARTITION1_START                   (AVMEM_BASE_ADDRESS + 0x400000)
#endif /*ST_5100||ST_7710*/

#if defined(ST_5301)
#define ST5301_AVMEM_BASE_ADDRESS          0xC0800000
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     ST5301_AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  ST5301_AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 ST5301_AVMEM_BASE_ADDRESS
#define VIRTUAL_BASE_ADDRESS               PHYSICAL_ADDRESS_SEEN_FROM_CPU /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       /*0x01000000*/(16*1024*1024)         /* 16 Mbyte */
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            PHYSICAL_ADDRESS_SEEN_FROM_CPU /* no MPEG block move on 5100 */
#define MPEG_SDRAM_SIZE                    VIRTUAL_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   ST5301_AVMEM_BASE_ADDRESS
#define PARTITION0_STOP                    (ST5301_AVMEM_BASE_ADDRESS + VIRTUAL_SIZE - 1)
#define PARTITION1_START                   (ST5301_AVMEM_BASE_ADDRESS + 0x400000)
#endif /*ST_5301*/

#elif defined(mb400) || defined(maly3s) || defined(mb436) || defined(DTT5107) || defined(SAT5107)
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 AVMEM_BASE_ADDRESS
#define VIRTUAL_BASE_ADDRESS               AVMEM_BASE_ADDRESS /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       AVMEM_SIZE
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            AVMEM_BASE_ADDRESS /* no MPEG block move on 5105 */
#define MPEG_SDRAM_SIZE                    AVMEM_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   AVMEM_BASE_ADDRESS
#define PARTITION0_STOP                    (AVMEM_BASE_ADDRESS + AVMEM_SIZE - 1)
#define PARTITION1_START                   (AVMEM_BASE_ADDRESS + 0x400000)

#elif defined(mb457)
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_BASE_ADDRESS /* defined in mb457.h */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 AVMEM_BASE_ADDRESS
#define VIRTUAL_BASE_ADDRESS               AVMEM_BASE_ADDRESS /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       AVMEM_SIZE         /* defined in mb457.h */
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            AVMEM_BASE_ADDRESS /* no MPEG block move on 5188 */
#define MPEG_SDRAM_SIZE                    AVMEM_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   AVMEM_BASE_ADDRESS
#define PARTITION0_STOP                    (AVMEM_BASE_ADDRESS + AVMEM_SIZE - 1)
#define PARTITION1_START                   (AVMEM_BASE_ADDRESS + 0x400000)

#elif defined(mb411)


#if defined(ST_OS21) && !defined(ST_OSWINCE)
/* MEMORY MAP for MB411
 * We have 64 MBytes on each LMI on the MB411 divided as followed :
 * LMI_SYS :
 * - SH4 segment            (16M) : All ST40 code, defined through board.mem
 * - STAVMEM partition      (43M) : for any driver
 * - LX Audio segment       ( 2M) : All audio ST231 code
 * - LX Video segment       ( 2 or 3M) : 2M for 7100, 3M for 7109
 * LMI_VID :
 * - STAVMEM partition      (64M) : for any driver
 *
 * STi7100 mapping
 *  0xA0000000  ------------------------------
 *              | EMI (64MB)                 |
 *  0xA4000000  ------------------------------
 *              | LX_VIDEO_CODE_MEMORY (2MB) |
 *  0xA4200000  ------------------------------
 *              | LX_AUDIO_CODE_MEMORY (2MB) |
 *  0xA4400000  ------------------------------
 *              | SH4_CODE_MEMORY     (16MB) |
 *  0xA5400000  ------------------------------
 *              | NCACHE PARTITION   (0.5MB) |
 *  0xA5480000  ------------------------------
 *              | STAVMEM_SYS_MEMORY  (43.5MB) |
 *  0xA80000000  ------------------------------
 *              ..............................
 *  0xB0000000  ------------------------------
 *              | STAVMEM_VID_MEMORY (64MB)  |
 *  0xB4000000  ------------------------------
 *
 * STi7109 mapping
 *  0xA0000000  ------------------------------
 *              | EMI (64MB)                 |
 *  0xA4000000  ------------------------------
 *              | LX_VIDEO_CODE_MEMORY (3MB) |
 *  0xA4300000  ------------------------------
 *              | LX_AUDIO_CODE_MEMORY (2MB) |
 *  0xA4500000  ------------------------------
 *              | SH4_CODE_MEMORY     (16MB) |
 *  0xA5500000  ------------------------------
 *              | NCACHE PARTITION   (0.5MB) |
 *  0xA5580000  ------------------------------
 *              | STAVMEM_SYS_MEMORY  (42.5MB) |
 *  0xA8000000  ------------------------------
 *              ..............................
 *  0xB0000000  ------------------------------
 *              | STAVMEM_VID_MEMORY (64MB)  |
 *  0xB4000000  ------------------------------
 *
*/

    #define SH4_CODE_MEMORY_SIZE        (16*1024*1024)   /* 16 MBytes */
#define NCACHE_PARTITION_SIZE       (512*1024)       /* 0.5 MBytes */
#define LX_AUDIO_CODE_MEMORY_SIZE   ( 2*1024*1024)   /*  2 MBytes */

#if defined (ST_7109)
#define LX_VIDEO_CODE_MEMORY_SIZE_MB    3

#else
#define LX_VIDEO_CODE_MEMORY_SIZE_MB    2

#endif /* defined (ST_7109) */
#define LX_VIDEO_CODE_MEMORY_SIZE       ((LX_VIDEO_CODE_MEMORY_SIZE_MB)*1024*1024)
#define STAVMEM_SYS_MEMORY_SIZE         ((45-LX_VIDEO_CODE_MEMORY_SIZE_MB)*1024*1024+512*1024)

#define STAVMEM_VID_MEMORY_SIZE     (64*1024*1024)   /* 64 MBytes */

#define LMI_SYS_BASE                    tlm_to_virtual(0xA4000000)
#define LMI_VID_BASE                    tlm_to_virtual(0xB0000000)
#define LX_VIDEO_CODE_MEMORY_BASE   (LMI_SYS_BASE)
#define LX_AUDIO_CODE_MEMORY_BASE   (LMI_SYS_BASE + LX_VIDEO_CODE_MEMORY_SIZE)
#define NCACHE_PARTITION_BASE       (LMI_SYS_BASE + LX_VIDEO_CODE_MEMORY_SIZE + LX_AUDIO_CODE_MEMORY_SIZE + SH4_CODE_MEMORY_SIZE)

#define PHYSICAL_ADDRESS_SEEN_FROM_CPU      LMI_SYS_BASE
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE   (LMI_SYS_BASE - NCACHE_SH4_TO_STBUS_ADDRESS_OFFSET)
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2  (LMI_SYS_BASE - NCACHE_SH4_TO_STBUS_ADDRESS_OFFSET)

#define VIRTUAL_BASE_ADDRESS                PHYSICAL_ADDRESS_SEEN_FROM_CPU
#define VIRTUAL_SIZE                        0xffffffff
#define VIRTUAL_WINDOW_SIZE                 VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS             PHYSICAL_ADDRESS_SEEN_FROM_CPU
#define MPEG_SDRAM_SIZE                     VIRTUAL_SIZE

#if defined(DVD_SECURED_CHIP) && defined (ST_7109)
/* Secured STi7109 mapping
 *  0xA0000000  -------------------------------
 *              | EMI (64MB)                  |
 *  0xA4000000  -------------------------------
 *              | LX_VIDEO_CODE_MEMORY (2MB)  |
 *  0xA4200000  -------------------------------
 *              | LX_AUDIO_CODE_MEMORY (2MB)  |
 *  0xA4400000  -------------------------------
 *              | SH4_CODE_MEMORY     (16MB)  |
 *  0xA5400000  -------------------------------
 *              | NCACHE PARTITION   (0.5MB)  |
 *  0xA5480000  -------------------------------
 *              | FDMA_NODE_PARTITION (512B)  |
 *  0xA5480200  -------------------------------
 *              | STAVMEM_SYS_MEMORY (~43.5MB)|
 *  0xA8000000  -------------------------------
 *              ..............................
 *  0xB0000000  -------------------------------
 *              | STAVMEM_VID_MEMORY (64MB)   |
 *  0xB4000000  -------------------------------
 *
*/
#define MAX_PARTITIONS                      3
#define FDMA_NODE_PARTITION_SIZE            0x200
#define PARTITION0_START                   (NCACHE_PARTITION_BASE + NCACHE_PARTITION_SIZE + FDMA_NODE_PARTITION_SIZE)
#define PARTITION0_STOP                    (PARTITION0_START + STAVMEM_SYS_MEMORY_SIZE - FDMA_NODE_PARTITION_SIZE - 1)

#define PARTITION1_START                   (LMI_VID_BASE)
#define PARTITION1_STOP                    (PARTITION1_START + (STAVMEM_VID_MEMORY_SIZE - 1 - 1920 * 32))

/* Reserve a small partition to allocate FDMA node structures in non-secured memory zone */
#define PARTITION2_START                   (NCACHE_PARTITION_BASE + NCACHE_PARTITION_SIZE)
#define PARTITION2_STOP                    (NCACHE_PARTITION_BASE + NCACHE_PARTITION_SIZE + FDMA_NODE_PARTITION_SIZE - 1)

#else /* Not Secured 7109 */
#define MAX_PARTITIONS                      2
#define PARTITION0_START                   (LMI_SYS_BASE + LX_VIDEO_CODE_MEMORY_SIZE + LX_AUDIO_CODE_MEMORY_SIZE + SH4_CODE_MEMORY_SIZE + NCACHE_PARTITION_SIZE)
#define PARTITION0_STOP                    (PARTITION0_START + STAVMEM_SYS_MEMORY_SIZE)

#define PARTITION1_START                   (LMI_VID_BASE)
/* Make partition stop 2 lines of MB (HD) before the end of memory, so that Deltaphi, when reading data from the frame buffer
 * does not request data outside of the memory. (Prediction block in Deltaphi, when reading Frame buffers)
 * Info: frame buffers are allocated on top of the memory
 * 1920 = nb pixels Luma, HD
 * 32 = number of lines of pixels in 2 lines of Luma MB
 */
#define PARTITION1_STOP                    (PARTITION1_START + (STAVMEM_VID_MEMORY_SIZE - 1 - 1920 * 32))
#endif  /* DVD_SECURED_CHIP && ST_7109 */

#elif defined(ST_OSWINCE)
/* MEMORY MAP for MB411
 * We have 64 MBytes on each LMI on the MB411 divided as followed :
 * LMI_SYS :
 * - SH4 segment            (16M) : All ST40 code, defined through board.mem
 * - STAVMEM partition      (44M) : for any driver
 * - LX Audio segment       ( 2M) : All audio ST231 code
 * - LX Video segment       ( 1 or 2M) : 1M for 7100, 2M for 7109
 * LMI_VID :
 * - STAVMEM partition      (64M) : for any driver
 *
 * STi7100 mapping
 *  0xA0000000  ------------------------------
 *              | EMI (64MB)                 |
 *  0xA4000000  ------------------------------
 *              | LX_VIDEO_CODE_MEMORY (1MB) |
 *  0xA4100000  ------------------------------
 *              | LX_AUDIO_CODE_MEMORY (2MB) |
 *  0xA4300000  ------------------------------
 *              | SH4_CODE_MEMORY   (31,5MB) |
 *  0xA6280000  ------------------------------
 *              | NCACHE PARTITION   (0.0MB) |
 *  0xA6280000  ------------------------------
 *              | STAVMEM_SYS_MEMORY(23.5MB) |
 *  0xA7A00000  ------------------------------
 *              | SHARED_AUDIO_WINCE   (6MB) |
 *  0xA8000000  ------------------------------
 *              ..............................
 *  0xB0000000  ------------------------------
 *              | STAVMEM_VID_MEMORY (64MB)  |
 *  0xB4000000  ------------------------------
 *
 * STi7109 mapping
 *  0xA0000000  ------------------------------
 *              | EMI (64MB)                 |
 *  0xA4000000  ------------------------------
 *              | LX_VIDEO_CODE_MEMORY (2MB) |
 *  0xA4200000  ------------------------------
 *              | LX_AUDIO_CODE_MEMORY (2MB) |
 *  0xA4400000  ------------------------------
 *              | SH4_CODE_MEMORY   (31.5MB) |
 *  0xA6380000  ------------------------------
 *              | NCACHE PARTITION   (0.0MB) |
 *  0xA6380000  ------------------------------
 *              | STAVMEM_SYS_MEMORY(22.5MB) |
 *  0xA7A00000  ------------------------------
 *              | SHARED_AUDIO_WINCE   (6MB) |
 *  0xA8000000  ------------------------------
 *              ..............................
 *  0xB0000000  ------------------------------
 *              | STAVMEM_VID_MEMORY (64MB)  |
 *  0xB4000000  ------------------------------
 *
*/

#define BOOTARGS_SIZE               (4*1024)                    // 4KB.
#define NK_SIZE                     (15*1024*1024 + 1020*1024)  //Around 16 MB.
#define RAM_SIZE                    (15*1024*1024 + 512*1024)   // 15.5 

#define SH4_CODE_MEMORY_SIZE        BOOTARGS_SIZE+NK_SIZE+RAM_SIZE   /* 31.5 MBytes */
#define NCACHE_PARTITION_SIZE       0  /*(512*1024)*/       /* 0.0 MBytes */
#define LX_AUDIO_CODE_MEMORY_SIZE   (2*1024*1024)   /*  2 MBytes */
#if defined (ST_7109)
#define STAVMEM_SYS_MEMORY_SIZE     (22*1024*1024 + 512*1024)   /* 28.5 MBytes */
#define LX_VIDEO_CODE_MEMORY_SIZE   (2*1024*1024)   /*  2 MBytes */
#else
#define STAVMEM_SYS_MEMORY_SIZE     (23*1024*1024 + 512*1024)   /* 29.5 MBytes */
#define LX_VIDEO_CODE_MEMORY_SIZE   (1*1024*1024)   /*  1 MBytes */
#endif /* defined (ST_7109) */
#define STAVMEM_VID_MEMORY_SIZE     (64*1024*1024)   /* 64 MBytes */
#define SHARED_AUDIO_WINCE			    (6*1024*1024)   //(16*1024*1024) /* 16 MBytes shared memory for audio */
#define LMI_SYS_BASE                0xA4000000
#define LMI_VID_BASE                0xB0000000

#define LX_VIDEO_CODE_MEMORY_BASE   (LMI_SYS_BASE)
#define LX_AUDIO_CODE_MEMORY_BASE   (LMI_SYS_BASE + LX_VIDEO_CODE_MEMORY_SIZE)

#define PHYSICAL_ADDRESS_SEEN_FROM_CPU      0xA4000000
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE   0x04000000
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2  0x04000000
#define VIRTUAL_BASE_ADDRESS                PHYSICAL_ADDRESS_SEEN_FROM_CPU
#define VIRTUAL_SIZE                        0xffffffff
#define VIRTUAL_WINDOW_SIZE                 VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS             PHYSICAL_ADDRESS_SEEN_FROM_CPU
#define MPEG_SDRAM_SIZE                     VIRTUAL_SIZE

#if defined(DVD_SECURED_CHIP) && defined (ST_7109)
/* Secured STi7109 mapping
 *  0xA0000000  -------------------------------
 *              | EMI (64MB)                  |
 *  0xA4000000  -------------------------------
 *              | LX_VIDEO_CODE_MEMORY (2MB)  |
 *  0xA4200000  -------------------------------
 *              | LX_AUDIO_CODE_MEMORY (2MB)  |
 *  0xA4400000  -------------------------------
 *              | SH4_CODE_MEMORY     (16MB)  |
 *  0xA5400000  -------------------------------
 *              | NCACHE PARTITION   (0.5MB)  |
 *  0xA5480000  -------------------------------
 *              | FDMA_NODE_PARTITION (512B)  |
 *  0xA5480200  -------------------------------
 *              | STAVMEM_SYS_MEMORY (~43.5MB)|
 *  0xA8000000  -------------------------------
 *              ..............................
 *  0xB0000000  -------------------------------
 *              | STAVMEM_VID_MEMORY (64MB)   |
 *  0xB4000000  -------------------------------
 *
 *
 *  0xA0000000  ------------------------------
 *              | EMI (64MB)                 |
 *  0xA4000000  ------------------------------
 *              | LX_VIDEO_CODE_MEMORY (2MB) |
 *  0xA4200000  ------------------------------
 *              | LX_AUDIO_CODE_MEMORY (2MB) |
 *  0xA4400000  ------------------------------
 *              | SH4_CODE_MEMORY   (31.5MB) |
 *  0xA6380000  ------------------------------
 *              | NCACHE PARTITION   (0.0MB) |
 *  0xA6380000  -------------------------------
 *              | FDMA_NODE_PARTITION (512B)  |
 *  0xA6380200  ------------------------------
 *              | STAVMEM_SYS_MEMORY(22.4MB) |
 *  0xA7A00000  ------------------------------
 *              | SHARED_AUDIO_WINCE   (6MB) |
 *  0xA8000000  ------------------------------
 *              ..............................
 *  0xB0000000  ------------------------------
 *              | STAVMEM_VID_MEMORY (64MB)  |
 *  0xB4000000  ------------------------------
 *
*/

#define MAX_PARTITIONS                      3
#define FDMA_NODE_PARTITION_SIZE            0x200
#define PARTITION0_START                   (LMI_SYS_BASE + LX_VIDEO_CODE_MEMORY_SIZE + LX_AUDIO_CODE_MEMORY_SIZE + SH4_CODE_MEMORY_SIZE + NCACHE_PARTITION_SIZE+FDMA_NODE_PARTITION_SIZE)
#define PARTITION0_STOP                    (PARTITION0_START + STAVMEM_SYS_MEMORY_SIZE - FDMA_NODE_PARTITION_SIZE - 1)

#define PARTITION1_START                   (LMI_VID_BASE)
#define PARTITION1_STOP                    (PARTITION1_START + (STAVMEM_VID_MEMORY_SIZE - 1 - 1920 * 32))

/* Reserve a small partition to allocate FDMA node structures in non-secured memory zone */
#define PARTITION2_START                   (LMI_SYS_BASE + LX_VIDEO_CODE_MEMORY_SIZE + LX_AUDIO_CODE_MEMORY_SIZE + SH4_CODE_MEMORY_SIZE + NCACHE_PARTITION_SIZE)
#define PARTITION2_STOP                    (PARTITION2_START + FDMA_NODE_PARTITION_SIZE - 1)

#else /* Not Secured 7109 */
#define MAX_PARTITIONS                      2

#define PARTITION0_START                   (LMI_SYS_BASE + LX_VIDEO_CODE_MEMORY_SIZE + LX_AUDIO_CODE_MEMORY_SIZE + SH4_CODE_MEMORY_SIZE + NCACHE_PARTITION_SIZE)
#define PARTITION0_STOP                    (PARTITION0_START + STAVMEM_SYS_MEMORY_SIZE)

#define PARTITION1_START                   (LMI_VID_BASE)
/* Make partition stop 2 lines of MB (HD) before the end of memory, so that Deltaphi, when reading data from the frame buffer
 * does not request data outside of the memory. (Prediction block in Deltaphi, when reading Frame buffers)
 * Info: frame buffers are allocated on top of the memory
 * 1920 = nb pixels Luma, HD
 * 32 = number of lines of pixels in 2 lines of Luma MB
 */
#define PARTITION1_STOP                    (PARTITION1_START + (STAVMEM_VID_MEMORY_SIZE - 1 - 1920 * 32))
#endif /* !Not Secured 7109 */

#elif ST_OSLINUX

#undef  AVMEM_BASE_ADDRESS
#undef  AVMEM_SIZE
#define AVMEM_BASE_ADDRESS                 0x0000000   /* Start Address bigphysarea (Not used)*/
#define AVMEM_SIZE                         0x1500000    /* Size of bigphysarea allocated */

#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_BASE_ADDRESS /* defined in mb376.h */
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 AVMEM_BASE_ADDRESS
#define VIRTUAL_BASE_ADDRESS               AVMEM_BASE_ADDRESS /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       AVMEM_SIZE         /* defined in mb376.h or espresso.h */
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            AVMEM_BASE_ADDRESS /* no MPEG block move on 5528 */
#define MPEG_SDRAM_SIZE                    AVMEM_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   AVMEM_BASE_ADDRESS
#define PARTITION0_STOP                    (AVMEM_BASE_ADDRESS + AVMEM_SIZE - 1)
#define PARTITION1_START                   0
#define PARTITION1_STOP                    0

#else /* ST_OSLINUX */
    #error ST20 not supported for STb7100 and STb7109
#endif /* not ST_OS21 */
#elif defined(mb428)
#define AVMEM_BASE_ADDRESS                 0x81000000
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU     AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE  AVMEM_BASE_ADDRESS
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2 AVMEM_BASE_ADDRESS
#define VIRTUAL_BASE_ADDRESS               PHYSICAL_ADDRESS_SEEN_FROM_CPU /* must = PHYSICAL_ADDRESS_SEEN_FROM_CPU */
#define VIRTUAL_SIZE                       /*0x01000000*/(16*1024*1024)         /* 16 Mbyte */
#define VIRTUAL_WINDOW_SIZE                VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS            PHYSICAL_ADDRESS_SEEN_FROM_CPU /* no MPEG block move on 5100 */
#define MPEG_SDRAM_SIZE                    VIRTUAL_SIZE
#define MAX_PARTITIONS                     1
#define PARTITION0_START                   AVMEM_BASE_ADDRESS
#define PARTITION0_STOP                    (AVMEM_BASE_ADDRESS + VIRTUAL_SIZE - 1)
#define PARTITION1_START                   (AVMEM_BASE_ADDRESS + 0x400000)

#elif defined(mb519)
#ifdef ST_OS21
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
 *  0xA8000000  -------------------------------
 *              | EMI (64MB)                  |
 *  0xAC000000  -------------------------------
 *              | LX_VIDEO1_CODE_MEMORY (3MB) |
 *  0xAC300000  -------------------------------
 *              | LX_VIDEO2_CODE_MEMORY (3MB) |
 *  0xAC600000  -------------------------------
 *              | LX_AUDIO1_CODE_MEMORY (2MB) |
 *  0xAC800000  -------------------------------
 *              | LX_AUDIO2_CODE_MEMORY (2MB) |
 *  0xACA00000  -------------------------------
 *              | NCACHE PARTITION      (2MB) |
 *  0xACC00000  -------------------------------
 *              | SH4_CODE_MEMORY      (16MB) |
 *  0xADC00000  -------------------------------
 *              | STAVMEM_SYS_MEMORY  (100MB) |
 *  0xAF000000  -------------------------------
 *              ...............................
 *  0xB0000000  -------------------------------
 *              | STAVMEM_VID_MEMORY   (64MB) |
 *  0xB4000000  -------------------------------
 */
#ifdef NATIVE_CORE
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU      tlm_to_virtual(0x08000000)
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE   0x08000000
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2  0x08000000
#else /* NATIVE_CORE */
#define PHYSICAL_ADDRESS_SEEN_FROM_CPU      LMI0_BASE
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE   (LMI0_BASE - NCACHE_SH4_TO_STBUS_ADDRESS_OFFSET)
#define PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2  (LMI0_BASE - NCACHE_SH4_TO_STBUS_ADDRESS_OFFSET)
#endif /* NATIVE_CORE */


#define VIRTUAL_BASE_ADDRESS                PHYSICAL_ADDRESS_SEEN_FROM_CPU
#define VIRTUAL_SIZE                        AVMEM1_MEMORY_SIZE
#define VIRTUAL_WINDOW_SIZE                 VIRTUAL_SIZE
#define MPEG_SDRAM_BASE_ADDRESS             PHYSICAL_ADDRESS_SEEN_FROM_CPU
#define MPEG_SDRAM_SIZE                     VIRTUAL_SIZE

#define MAX_PARTITIONS                      2

#ifdef NATIVE_CORE
#define STAVMEM_SYS_MEMORY_SIZE     (43*1024*1024+512*1024)   /* 43.5 MBytes */
#define STAVMEM_VID_MEMORY_SIZE     (64*1024*1024)   /* 64 MBytes */

#define PARTITION0_START     (PHYSICAL_ADDRESS_SEEN_FROM_CPU)
#define PARTITION0_STOP      (PARTITION0_START + STAVMEM_SYS_MEMORY_SIZE)
#define PARTITION1_START     (PHYSICAL_ADDRESS_SEEN_FROM_CPU+0x3000000)
#define PARTITION1_STOP      (PARTITION1_START + (STAVMEM_VID_MEMORY_SIZE - 1 - 1920 * 32))
#else /* NATIVE_CORE */
#define PARTITION0_START                   (AVMEM0_ADDRESS)
#define PARTITION0_STOP                    (AVMEM0_END_ADDRESS)
#define PARTITION1_START                   (AVMEM1_ADDRESS)
#define PARTITION1_STOP                    (AVMEM1_END_ADDRESS)
#endif /* NATIVE_CORE */

#else /* ST_OS21 */
    #error LINUX not yet supporter and ST20 not supported for STb7200
#endif /* not ST_OS21 */
#endif

#if defined(ST_5516)||defined(ST_5517)||defined(ST_5528)||defined(ST_GX1)||defined(ST_5100)||defined(ST_5105)||defined(ST_7710)\
 || defined(ST_7100)||defined(ST_5301)||defined(ST_7109)||defined(ST_5525)|| defined(ST_5188)|| defined(ST_5107)
#define BM_BASE_ADDRESS     0 /* no Block Move DMA on 5516/17/28/GX1/5100/5301 */
#endif

#define SDRAM_CPU_BASE_ADDRESS PHYSICAL_ADDRESS_SEEN_FROM_CPU

/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */

extern STAVMEM_PartitionHandle_t AvmemPartitionHandle[MAX_PARTITIONS+1];
extern STAVMEM_MemoryRange_t     SecuredSysMemoryRange;

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

BOOL AVMEM_Init(void);
BOOL AVMEM_Command(void);
void AVMEM_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLAVMEM_H */


