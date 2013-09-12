/*****************************************************************************
File Name   : fdmatst.h

Description : Main test harness header

History     :

Copyright (C) 2002 STMicroelectronics

Reference   :
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __FDMATST_H
#define __FDMATST_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"         /* Standard definitions */

#ifdef __cplusplus
extern "C" {
#endif

#if defined (ARCHITECTURE_ST40)
#if defined (ST_OS21)
    #define TRANSLATE_NODE_ADDRESS_TO_PHYS(addr) PERIPH_ADDR_TRANSLATE(addr)
    #define TRANSLATE_BUFFER_ADDRESS_TO_PHYS(addr) PERIPH_ADDR_TRANSLATE(addr)
	#define PERIPH_ADDR_TRANSLATE(addr) fdmatst_PeriphAddrTranslate(addr)
#if defined (ST_OSWINCE)
	#define ADDRESS_TO_P2(addr) (addr)
#else
    #define ADDRESS_TO_P2(addr) ADDRESS_IN_UNCACHED_MEMORY(addr)
#endif
    #define TRANSLATE_NODE_ADDRESS_TO_VIRT(addr) ST40_NOCACHE_NOTRANSLATE(addr)
    #define TRANSLATE_BUFFER_ADDRESS_TO_VIRT(addr) ST40_NOCACHE_NOTRANSLATE(addr)
#else  /*!ST_OS21*/
    #define TRANSLATE_NODE_ADDRESS_TO_PHYS(addr) (void*)fdmatst_GetNodePhysicalAddressFromVirtual((STFDMA_GenericNode_t*)addr)
    #define TRANSLATE_BUFFER_ADDRESS_TO_PHYS(addr) (void*)fdmatst_GetBufferPhysicalAddressFromVirtual(addr)
    #define TRANSLATE_NODE_ADDRESS_TO_VIRT(addr) (void*)fdmatst_GetNodeVirtualAddressFromPhysical((dma_addr_t)addr)
    #define TRANSLATE_BUFFER_ADDRESS_TO_VIRT(addr) (void*)fdmatst_GetBufferVirtualAddressFromPhysical((dma_addr_t)addr)
	#define PERIPH_ADDR_TRANSLATE(addr) fdmatst_PeriphAddrTranslate(addr)	
    #define ADDRESS_TO_P2(addr) (addr)
#endif
#else		/*!ARCHITECTURE_ST40*/
    #define TRANSLATE_NODE_ADDRESS_TO_PHYS(addr) (addr)
    #define TRANSLATE_BUFFER_ADDRESS_TO_PHYS(addr) (addr)
    #define PERIPH_ADDR_TRANSLATE(addr) (addr)
    #define ADDRESS_TO_P2(addr) (addr)
    #define ST40_NOCACHE_NOTRANSLATE(addr) (addr)
    #define TRANSLATE_NODE_ADDRESS_TO_VIRT(addr) (addr)
    #define TRANSLATE_BUFFER_ADDRESS_TO_VIRT(addr) (addr)
#endif

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11)
#define LOCK_FDMA_CHANNELS(x,y) STFDMA_LockFdmaChannel(x,y)
#else
#define LOCK_FDMA_CHANNELS(x,y) STFDMA_LockFdmaChannel(y)
#endif
#endif

/* Exported Constants ----------------------------------------------------- */

/* FEI constants*/

#define FEI_BASE_ADDR                   0x20E00000

#define SW_ID							0
#define CHAN_IDLE						1
#define OUT_T1REQ_FIFO_CTRL				2
#define OUT_T2REQ_FIFO_CTRL				3

#define DF_INT_ENB_BIT					1
#define DF_INT_ENB_MASK					(1 << DF_INT_ENB_BIT)

#define FEI_SOFT_RST					(FEI_BASE_ADDR + 0x00000000 )
#define FEI_ENABLE						(FEI_BASE_ADDR + 0x00000008 )
#define FEI_CONFIG						(FEI_BASE_ADDR + 0x00000010 )
#define FEI_STATUS						(FEI_BASE_ADDR + 0x00000018 )
#define FEI_STATUS_CLR					(FEI_BASE_ADDR + 0x00000020 )
#define FEI_OUT_DATA					(FEI_BASE_ADDR + 0x00000028 )
#define FEI_OUT_ERROR					(FEI_BASE_ADDR + 0x00000030 )
#define FEI_SECTOR_SIZE					(FEI_BASE_ADDR + 0x00000038 )
#define FEI_TVO_COUNTER_ADDRESS         (FEI_BASE_ADDR + 0x00000048 )

#define RS_INTRS 			            100

/* FEI constants*/

#if defined (ST_5525)
#define FDMA_INTERRUPT_NUMBER FDMA_0_INTERRUPT
#define FDMA2_INTERRUPT_NUMBER FDMA_1_INTERRUPT
#define FDMA_BASE_ADDRESS 0x1A100000
#define FDMA2_BASE_ADDRESS 0x1A000000
#else
#define FDMA_INTERRUPT_NUMBER FDMA_INTERRUPT
#endif

#define FDMATEST_DEVICENAME "TESTING_FDMA_1"

#define FDMATST_DIM_2D          2
#define FDMATST_DIM_1D          1
#define FDMATST_DIM_0D          0
#define FDMATST_Y_INC           0
#define FDMATST_Y_DEC           1
#define FDMATST_Y_STATIC        2
#define FDMATST_OVERLAP_NONE    0


#ifndef FDMA_INTERRUPT_LEVEL
#define FDMA_INTERRUPT_LEVEL  14
#endif

#define SPDIF_BURST_SIZE     128


#if defined(ST_5162)

#define FDMA_TEST_ACTIVE_DREQ    STFDMA_REQUEST_SIGNAL_DREQ0_HI
#define FDMA_TEST_INACTIVE_DREQ  STFDMA_REQUEST_SIGNAL_DREQ1_HI
#define FDMA_TEST_DEVICE         STFDMA_DEVICE_FDMA_2
#define POST_FDMA_1(x,y) x
#define NUM_CHANNELS  (16 -5)   /* Default channels */

#elif defined(ST_5517)

#define FDMA_TEST_ACTIVE_DREQ    STFDMA_REQUEST_SIGNAL_DREQ0_HI
#define FDMA_TEST_INACTIVE_DREQ  STFDMA_REQUEST_SIGNAL_DREQ1_HI
#define FDMA_TEST_DEVICE         STFDMA_DEVICE_FDMA_1
#define POST_FDMA_1(x,y) y

#define NUM_CHANNELS  6

#else

#if defined (ST_5105) || defined (ST_5188) || defined (ST_5107)
#define FDMA_TEST_ACTIVE_DREQ    STFDMA_REQUEST_SIGNAL_CD_EXT0

#elif defined(ST_5525)
#define FDMA_TEST_ACTIVE_DREQ    STFDMA_REQUEST_SIGNAL_SSC1_TX

#else
#define FDMA_TEST_ACTIVE_DREQ    STFDMA_REQUEST_SIGNAL_CD_EXT1
#define FDMA_TEST_INACTIVE_DREQ  STFDMA_REQUEST_SIGNAL_CD_EXT0
#endif
#define FDMA_TEST_DEVICE STFDMA_DEVICE_FDMA_2
#define POST_FDMA_1(x,y) x

#if defined (ST_5188)
#define NUM_CHANNELS  (8-5)   /* Default channels */
#elif defined (ST_7200)
#define NUM_CHANNELS  (16-2)   /* Default channels */
#else
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
#define NUM_CHANNELS  (11-5)   /* Default channels */
#else
#define NUM_CHANNELS  (16-5)
#endif
#endif

#endif

/* Exported Macros -------------------------------------------------------- */

#ifdef STFDMA_TEST_VERBOSE
    #define VERBOSE_PRINT(Text) STTBX_Print((Text))
    #define VERBOSE_PRINT_DATA(Text,Data) STTBX_Print((Text,Data))
#else
    #define VERBOSE_PRINT(Text)
    #define VERBOSE_PRINT_DATA(Text,Data)
#endif

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* FEI */

typedef struct
{
  unsigned int	    n_intrs;                      /* total number of interrupts   */
  unsigned long     n_status[RS_INTRS];           /* status register value*/
  STOS_Clock_t      time_intrs[RS_INTRS];         /* time of interrupt occurance  */
} rsisr_t;

/* Exported Functions ----------------------------------------------------- */

/* FEI */

void STFEI_Init(void);

void STFEI_Start(void);

/* FEI */

void fmdatst_TestCaseSummarise(void);

void fdmatst_ResetSuccessState(void);

void fdmatst_SetSuccessState(BOOL Value);

BOOL fdmatst_IsTestSuccessful(void);

void fdmatst_ErrorReport(char *text, ST_ErrorCode_t ErrorGiven,ST_ErrorCode_t ExpectedErr);

partition_t *fdmatst_GetSystemPartition(void);

partition_t *fdmatst_GetInternalPartition(void);

partition_t *fdmatst_GetNCachePartition(void);

partition_t *fdmatst_GetSMIPartition(void);


ST_ErrorCode_t fdmatst_CreateNode(U8 SrcDim, U8 SrcYDir, BOOL SrcAligned,
                                  U8 DestDim, U8  DestYDir, BOOL DestAligned,
                                  BOOL NodeCompleteNotify, BOOL NodeCompletePause,
                                  U32 PaceSignal, STFDMA_Node_t **FirstNode_p);

ST_ErrorCode_t fdmatst_CreateFEINode(U8 SrcDim,                 U8 SrcYDir,             BOOL SrcAligned,
                                     U8 DestDim,                U8  DestYDir,           BOOL DestAligned,
                                     BOOL NodeCompleteNotify,   BOOL NodeCompletePause, U32 NumPackets,
                                     STFDMA_GenericNode_t **FirstNode_p);

ST_ErrorCode_t fdmatst_CreatePESNode(U8 *SrcBuff_p, U32 Size,
                                     STFDMA_ContextId_t Context,
                                     STFDMA_GenericNode_t **FirstNode_p);

#if !defined (STFDMA_NO_PACED_TESTS)
ST_ErrorCode_t fdmatst_CreateSPDIFNode(U8  *SrcBuff_p,  U32 SrcSize,
                                       U8 **DestBuff_p, BOOL End, BOOL Valid,
                                       STFDMA_GenericNode_t **FirstNode_p);
#endif

ST_ErrorCode_t fdmatst_CreateESBuffer(U32 Size,
                                      U8  **Buffer_p);

ST_ErrorCode_t fdmatst_SetupCircularTransfer(BOOL NodeCompleteNotify, BOOL NodeCompletePause,
                                             BOOL SrcAligned, BOOL DestAligned,
                                             BOOL Paced, STFDMA_Node_t **FirstNode_p);


U32 fdmatst_DeallocateNodes(void);

U32 fdmatst_CheckData(STFDMA_Node_t **FirstNode_p, char TestCaseId[5]);

U32* fdmatst_GetBaseAddress(void);

U32 fdmatst_GetClocksPerSec(void);

#if !defined (ST_OSLINUX)
void fdmatst_DisplayPartitionStatus(partition_status_t *Partition);
#endif

#if !defined (STFDMA_NO_PACED_TESTS)
ST_ErrorCode_t fdmatst_InitPaceRequestGenerator(STFDMA_Node_t *Node_p, U32 BytesPerSec);

U32 fdmatst_StartPaceRequestGenerator(void);

U32 fdmatst_StopPaceRequestGenerator(void);

U32 fdmatst_TermPaceRequestGenerator(void);
#endif

#if defined (ARCHITECTURE_ST40)
void fdmatst_CovertNodeAddressDataToPeripheral(STFDMA_Node_t* pFirstNode);
void * fdmatst_PeriphAddrTranslate(void * Addr);
void   fdmatst_ResetPeriphAddrTranslate(void);
#else
#define fdmatst_CovertNodeAddressDataToPeripheral(pNode)
#endif

#if defined (ST_OSLINUX)
int fdmatst_main(void);
dma_addr_t fdmatst_GetNodePhysicalAddressFromVirtual(STFDMA_GenericNode_t* pNode);
dma_addr_t fdmatst_GetBufferPhysicalAddressFromVirtual(void* buffer_addr);
STFDMA_GenericNode_t* fdmatst_GetNodeVirtualAddressFromPhysical(dma_addr_t pNode);
void* fdmatst_GetBufferVirtualAddressFromPhysical(dma_addr_t buffer_addr);
#endif


#ifdef __cplusplus
}
#endif

#endif /* __FDMATST_H */

/*eof*/
