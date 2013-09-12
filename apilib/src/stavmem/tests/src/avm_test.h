/*****************************************************************************

File name   : AVM_TEST.H

Description : common data for different avmem test files

COPYRIGHT (C) ST-Microelectronics 2003.

Date                Modification                                     Name
----                ------------                                     ----
04 May 2001         Created                                          HSdLM
11 Jun 2001         Add GPDMA method tests.                          HSdLM
30 Oct 2001         Update for mb317b board.                         HSdLM
09 May 2003         Update for stem7020                              HSdLM
*****************************************************************************/

#ifndef __AVM_TEST_H
#define __AVM_TEST_H

/* Includes --------------------------------------------------------------- */

#include "stavmem.h"

/* Exported Macros -------------------------------------------------------- */

#define V2CPU(Addr)       STAVMEM_VirtualToCPU(Addr,VMap_p)
#define IFV2CPU(Addr)     STAVMEM_IfVirtualThenToCPU(Addr, VMap_p)
#define ISVIR(Addr)       STAVMEM_IsAddressVirtual(Addr, VMap_p)
#define ISINVW(Addr,Len)  STAVMEM_IsDataInVirtualWindow(Addr, Len, VMap_p)

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* If defined, will test more deeply, with verification of block lists, etc. */
/*#define DEBUG_CODE*/

#if defined (mb317) || defined (mb317b)
#define FLASH_BASE_ADDRESS      0xA0000000 /* 16MB */
#define SDRAM_CPU_BASE_ADDRESS  0xAA000000 /* 32MB */
#else
#define SRAM_BASE_ADDRESS   0x80000000 /* SRAM Base address in ST20 space */
#define DRAM_BASE_ADDRESS   0x40000000 /* DRAM Base address in STi5510 eval kit space */
/*#define DRAM_TEST*/   /* makes fill hanging on 5517 with FDMA: memory_allocate_clear hangs. */
#endif

/* Address of a range of at least 512Kbytes of cached SDRAM */
#define TEST_SPEED_SDRAMc   SDRAM_CPU_BASE_ADDRESS
#define STAVMEM_512K        0x00080000
#define STAVMEM_1M          0x00100000
#define STAVMEM_16M         0x01000000
#define STAVMEM_256M        0x10000000

#define OFFSET_7 7
#define OFFSET_6 6
#define OFFSET_5 5
#define OFFSET_4 4
#define OFFSET_3 3
#define OFFSET_0 0

#define DUMP_6_LINES 6
#define DUMP_5_LINES 5
#define DUMP_4_LINES 4
#define DUMP_3_LINES 3
#define DUMP_2_LINES 2
#define DUMP_1_LINE 1
#define MAX_DUMP_LINES 10

typedef enum
{
    COPY_1D,
    COPY_2D,
    FILL_1D,
    FILL_2D,
    C_MEM_MOVE,
    C_MEM_COPY,
#ifdef ST_OS20
    MOVE_2D_SAME_PITCH,
    MOVE_2D_DIF_PITCH,
#endif /*ST_OS20*/
    BLK_MOVE_DMA,
    BLK_MOVE_FDMA_1D,
    BLK_MOVE_FDMA_2D_SAME_PITCH,
    BLK_MOVE_FDMA_2D_DIF_PITCH,
    BLK_MOVE_GPDMA_1D,
    BLK_MOVE_GPDMA_2D_SAME_PITCH,
    BLK_MOVE_GPDMA_2D_DIF_PITCH,
    MPEG_2D_BLK_MOVE_SAME_PITCH,
    MPEG_2D_BLK_MOVE_DIF_PITCH,
    MPEG_1D_BLK_MOVE_1D,
    MPEG_1D_BLK_MOVE_SAME_PITCH,
    MPEG_1D_BLK_MOVE_DIF_PITCH

} CopyMethod_t;

#define BEYOND_DEST_END_CHECK_VALUE 0x55
#define BEYOND_DEST_END_CHECK_SIZE  0xFF

/* Exported Variables ----------------------------------------------------- */

extern STAVMEM_SharedMemoryVirtualMapping_t *VMap_p;
extern U32 SDRamBaseAddr[];
extern char * SDRamName[];

/* Exported Functions ----------------------------------------------------- */

void DumpMemory(void *BeginAddr_p, const int NumberOfLines);
void FillMemoryWithRandom(void *MemoryBaseAddr_p, U32 MemorySize, BOOL Verbose);
U32 GetRandomBetween1And(const U32 Max);
void BlockAndBip(void);
void Inv_memmove(void *Src, void *Dst, U32 Size);
void os20_main(void *ptr);


#endif /* #ifndef __AVM_TEST_H */


