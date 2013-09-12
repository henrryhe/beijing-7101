/************************************************************************

Source file name : statapi.h

Description:       Interface to ATA/ATAPI driver.

COPYRIGHT (C) STMicroelectronics 2000

************************************************************************/

#ifndef __STATAPI_HAL_H
#define __STATAPI_HAL_H

/* Includes ---------------------------------------------------------------- */

#include "stddefs.h"
#include "stcommon.h"

#if defined(ATAPI_FDMA)
#include "stfdma.h"
#endif

/* For RegisterMap def */
#include "statapi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define         STATAPI_NUM_REG_MASKS       12

/* masks to address the control registers - EMI lines */
#define         aCS1                0x00040000
#define         nCS1                0x00000000
#define         aCS0                0x00080000
#define         nCS0                0x00000000
#define         aDA2                0x00020000
#define         nDA2                0x00000000
#define         aDA1                0x00010000
#define         nDA1                0x00000000
#define         aDA0                0x00008000
#define         nDA0                0x00000000

/*---------------MASKS bits---------------------------*/
#define         BIT_0               0x0001
#define         BIT_1               0x0002
#define         BIT_2               0x0004
#define         BIT_3               0x0008
#define         BIT_4               0x0010
#define         BIT_5               0x0020
#define         BIT_6               0x0040
#define         BIT_7               0x0080
#define         BIT_8               0x0100
#define         BIT_9               0x0200
#define         BIT_10              0x0400
#define         BIT_11              0x0800
#define         BIT_12              0x1000
#define         BIT_13              0x2000
#define         BIT_14              0x4000
#define         BIT_15              0x8000

#define          DEVHEAD_DEV0       0x00
#define          DEVHEAD_DEV1       0x10

#define          BSY_BIT_MASK       0x80   /* Busy         */
#define          DRQ_BIT_MASK       0x08   /* Data Request */
#define          DRDY_BIT_MASK      0x40   /* Device Ready   */
#define          DF_BIT_MASK        0x20   /* Device Fault */
#define          ERR_BIT_MASK       0x01   /* Error        */

#define          nIEN_CLEARED       0x00
#define          nIEN_SET           0x02
#define          SRST_SET           0x04

/* Only used for LBA-48 - High Order Bit, see ATA-6 spec. */
#define         CONTROL_HOB_SET          0x80
#define         CONTROL_HOB_CLEARED      0x00

/*---------------HW specific ---------------------------*/

#define INT_TIMEOUT         (ST_GetClocksPerSecond() * 10)

/*Exported variables ---------------------------------------------------------*/

/*Exported macros    ---------------------------------------------------------*/

#define WriteReg(Base, Value)   STSYS_WriteRegDev32LE(Base, Value)
#define ReadReg(Base)           STSYS_ReadRegDev32LE(Base)

/*Exported constants ---------------------------------------------------------*/

/* Indexing into array held in hal_atapi.c */
typedef enum
{
    ATA_REG_ALTSTAT = 0,
    ATA_REG_DATA ,
    ATA_REG_ERROR ,
    ATA_REG_FEATURE ,
    ATA_REG_SECCOUNT ,
    ATA_REG_SECNUM ,
    ATA_REG_CYLLOW ,
    ATA_REG_CYLHIGH ,
    ATA_REG_DEVHEAD ,
    ATA_REG_STATUS ,
    ATA_REG_COMMAND ,
    ATA_REG_CONTROL
} ATA_Register_t;

typedef enum ATA_Direction_e
{
    ATA_DATA_NONE,
    ATA_DATA_IN,
    ATA_DATA_OUT
} ATA_Direction_t;


/* ----- 5514/28-specific details follow in this section ----- */

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)

/* Some relevant HDDI internal registers - see ADCS 7180863 for
   name meanings */
#define HDDI_MODE               0x080
#define HDDI_ATA_RESET          0x084
#define HDDI_DMA_ITS            0x0e0
#define HDDI_DMA_STA            0x0e4
#define HDDI_DMA_ITM            0x0e8

/* PIO timing registers */
#define HDDI_DPIO_TIMING_OFFSET 0x090

#define HDDI_DPIO_I             0x090
#define HDDI_DPIO_IORDY         0x094
#define HDDI_DPIO_WR            0x098
#define HDDI_DPIO_RD            0x09c
#define HDDI_DPIO_WREN          0x0a0
#define HDDI_DPIO_AH            0x0a4
#define HDDI_DPIO_WRRE          0x0a8
#define HDDI_DPIO_RDRE          0x0ac

/* DMA registers - control, start address, word count, etc. */
#define HDDI_DMA_CONTROL_OFFSET 0x0b0

#define HDDI_DMA_C              0x0b0
#define HDDI_DMA_SA             0x0b4
#define HDDI_DMA_WC             0x0b8
#define HDDI_DMA_SI             0x0bc
#define HDDI_DMA_CA             0x0d0
#define HDDI_DMA_CB             0x0d4
#define HDDI_DMA_CHUNK          0x0d8       /* 5528 and 8010 only */

/* DMA timing - multiword DMA */
#define HDDI_MWDMA_TIMING_OFFSET    0x0f0

#define HDDI_MWDMA_TD           0x0f0
#define HDDI_MWDMA_TH           0x0f4
#define HDDI_MWDMA_TJ           0x0f8
#define HDDI_MWDMA_TKR          0x0fc
#define HDDI_MWDMA_TKW          0x100
#define HDDI_MWDMA_TM           0x104
#define HDDI_MWDMA_TN           0x108

/* DMA timing - UltraDMA */
#define HDDI_UDMA_TIMING_OFFSET     0x0f0

#define HDDI_UDMA_ACK          0x0f4
#define HDDI_UDMA_ENV          0x0f8
#define HDDI_UDMA_RP           0x0f0
#define HDDI_UDMA_ML           0x104
#define HDDI_UDMA_TLI          0x0fc        /* 5514 only */
#define HDDI_UDMA_CVS          0x0fc        /* 5528 and 8010 only */
#define HDDI_UDMA_SS           0x100
#define HDDI_UDMA_RFS          0x108
#define HDDI_UDMA_DVS          0x10c
#define HDDI_UDMA_DVH          0x110

/* --- DMA configuration registers masks ( hardware specific ) -------- */
#define     HDDI_DMA_C_MASK             0x0000001f
#define     HDDI_DMA_SA_MASK            0xfffffff0
#define     HDDI_DMA_WC_MASK            0x00ffffff
#define     HDDI_DMA_SI_MASK            0x00000003
#define     HDDI_DMA_CA_MASK            0xfffffff0
#define     HDDI_DMA_CB_MASK            0x01ffffff

/* Encodes the meanings of bits in the HDDI registers */

/* --- HDDI_MODE register (hardware specific) ----------------------- */
#define     HDDI_MODE_PIOREG            0x00000000
#define     HDDI_MODE_MWDMA             0x00000002
#define     HDDI_MODE_UDMA              0x00000003

/* --- HDDI_ATARESET register ------------------- */
#define     HDDI_ATARESET_ASSERT        0x00000001
#define     HDDI_ATARESET_DEASSERT      0x00000000

/* --- HDDI_ATA_ASR register -------------------- */
#define     HDDI_ATA_ASR_ERR            0x00000001
#define     HDDI_ATA_ASR_DRQ            0x00000008
#define     HDDI_ATA_ASR_DRDY           0x00000040
#define     HDDI_ATA_ASR_BSY            0x00000080

/* --- HDDI_ATA_DCR register -------------------- */
#define     HDDI_ATA_DCR_nIEN           0x00000002
#define     HDDI_ATA_DCR_SRST           0x00000004

/* --- HDDI_ATA_ERR register -------------------- */
#define     HDDI_ATA_ERR_ABRT           0x00000004
#define     HDDI_ATA_ERR_ICRC           0x00000080 /* DMA CRC error */
#define     HDDI_ATA_ERR_IDNF           0x00000010
#define     HDDI_ATA_ERR_UNC            0x00000040 /*WA GNBvd50151 STSA*/

/* --- HDDI_ATA_DHR register -------------------- */
#define     HDDI_ATA_DHR_DEV0           0x00000000
#define     HDDI_ATA_DHR_DEV1           0x00000010
#define     HDDI_ATA_DHR_LBA            0x00000040

/* --- HDDI_ATA_SR register --------------------- */
#define     HDDI_ATA_SR_ERR             0x00000001
#define     HDDI_ATA_SR_DRQ             0x00000008
#define     HDDI_ATA_SR_DF              0x00000020
#define     HDDI_ATA_SR_DRDY            0x00000040

/* --- HDDI_DMA_C register ( hardware specific ) ---------------------- */
#define     HDDI_DMA_C_STARTBIT         0x00000001
#define     HDDI_DMA_C_STOPBIT          0x00000002
#define     HDDI_DMA_C_DMAENABLE        0x00000004
#define     HDDI_DMA_C_NOTINCADDRESS    0x00000008
#define     HDDI_DMA_C_READNOTWRITE     0x00000010

/* --- HDDI_DMA_SI register ( hardware specific ) --------------------- */
#define     HDDI_DMA_SI_16BYTES         0x00000000
#define     HDDI_DMA_SI_32BYTES         0x00000001
#define     HDDI_DMA_SI_64BYTES         0x00000002
#define     HDDI_DMA_SI_UNDEFINED       0x00000003

/* --- HDDI_DMA_CHUNK register ( hardware specific ) ------------------ */
#define     HDDI_DMA_CHUNK_MESSAGE      0x00000000
#define     HDDI_DMA_CHUNK_1PACKET      0x00000001
#define     HDDI_DMA_CHUNK_2PACKETS     0x00000002
#define     HDDI_DMA_CHUNK_UNDEFINED    0x00000003

/* --- HDDI_DMA_ITS register ( hardware specific ) -------------------- */
#define     HDDI_DMA_ITS_DEND           0x00000001
#define     HDDI_DMA_ITS_IRQ            0x00000002
#define     HDDI_DMA_ITS_DEVTERMOK      0x00000004

/* --- HDDI_DMA_STA register ( hardware specific ) -------------------- */
#define     HDDI_DMA_STA_DEND           0x00000001
#define     HDDI_DMA_STA_IRQ            0x00000002
#define     HDDI_DMA_STA_DEVTERMOK      0x00000004
#define     HDDI_DMA_STA_ATADMASTATUS   0x00000700

/* --- HDDI_DMA_STA_ATADMASTATUS register ( hardware specific ) ------- */
#define     HDDI_DMA_STA_ATADMASTATUS_ATAIF_INACTIVE                0x00000000
#define     HDDI_DMA_STA_ATADMASTATUS_WAITING_FOR_DEV_RESPONSE      0x00000100
#define     HDDI_DMA_STA_ATADMASTATUS_DEV_INITIALISED               0x00000200
#define     HDDI_DMA_STA_ATADMASTATUS_DATA_BURST_INPROGRESS         0x00000300
#define     HDDI_DMA_STA_ATADMASTATUS_DEV_TERM_ATTEMPT_OR_PAUSE     0x00000400
#define     HDDI_DMA_STA_ATADMASTATUS_HOST_TERMINATE_ON_ERROR       0x00000500
#define     HDDI_DMA_STA_ATADMASTATUS_HOST_PAUSE_FIFO_EMPTY_OR_FULL 0x00000600
#define     HDDI_DMA_STA_ATADMASTATUS_DMA_COMPLETING_ABORT          0x00000700

/* --- HDDI_DMA_ITM register ( hardware specific ) -------------------- */
#define     HDDI_DMA_ITM_DEND           0x00000001
#define     HDDI_DMA_ITM_IRQ            0x00000002
#define     HDDI_DMA_ITM_DEVTERMOK      0x00000004

#endif /* ST_5514 */

/* 7100 SATA */
#if defined(ST_7100) || defined(ST_7109)

#define SATA_WRAPPER_BASE_ADDRESS             0x00
#define SATA_AHB2STBUS_STBUS_OPC 	         SATA_WRAPPER_BASE_ADDRESS + 0x00
#define SATA_AHB2STBUS_MESSAGE_SIZE_CONFIG   SATA_WRAPPER_BASE_ADDRESS + 0x04
#define SATA_AHB2STBUS_CHUNK_SIZE_CONFIG     SATA_WRAPPER_BASE_ADDRESS + 0x08
#define SATA_AHB2STBUS_SW_RESET              SATA_WRAPPER_BASE_ADDRESS + 0x0C
#define SATA_AHB2STBUS_PC_STATUS             SATA_WRAPPER_BASE_ADDRESS + 0x10
#define PC_GLUE_LOGIC                        SATA_WRAPPER_BASE_ADDRESS + 0x14
#define PC_GLUE_LOGICH                       SATA_WRAPPER_BASE_ADDRESS + 0x18

/*************************************/
/* SATA HOST CONTROLLER BASE ADDRESS */
/*************************************/
#define HOST_BASE_ADDRESS   0x800

/* SERIAL ATA REGISTERS */
#define SCR0         HOST_BASE_ADDRESS + 0x024
#define SCR1         HOST_BASE_ADDRESS + 0x028
#define SCR2         HOST_BASE_ADDRESS + 0x02C
#define SCR3         HOST_BASE_ADDRESS + 0x030
#define SCR4         HOST_BASE_ADDRESS + 0x034

#define SSTATUS_REG 	SCR0 /*read*/
#define SERROR_REG 	    SCR1 /* R/W*/
#define SATA_ATA_SR_ERR         		 0x00000001 /*error in status register*/
#define SATA_SERROR_INTERRUPT_ENABLE     0x00000008

#define SATA_AHB_ILLEGALACCESS_ERROR     0x00000800
#define SATA_DATA_INTEGRITY_ERROR        0x00000100
#define SATA_CRC_ERROR    			     0x00200000  /* DMA CRC error */
#define SATA_10b8b_DECODER_ERROR 	     0x00080000  /*10b to 8b Decoder error*/
#define SATA_DISPARITY_ERROR		     0x00100000
										 

/* IP SPECIFIC REGISTERS */
#define FPTAGR       HOST_BASE_ADDRESS + 0x064
#define FPBOR        HOST_BASE_ADDRESS + 0x068
#define FPTCR        HOST_BASE_ADDRESS + 0x06C
#define DMACR        HOST_BASE_ADDRESS + 0x070
#define DBTSR        HOST_BASE_ADDRESS + 0x074
#define INTPR        HOST_BASE_ADDRESS + 0x078
#define INTMR        HOST_BASE_ADDRESS + 0x07C
#define ERRMR        HOST_BASE_ADDRESS + 0x080
#define LLCR         HOST_BASE_ADDRESS + 0x084
#define PHYCR        HOST_BASE_ADDRESS + 0x088
#define PHYSR        HOST_BASE_ADDRESS + 0x08C
#define RXBISTPD     HOST_BASE_ADDRESS + 0x090
#define RXBISTD1     HOST_BASE_ADDRESS + 0x094
#define RXBISTD2     HOST_BASE_ADDRESS + 0x098
#define TXBISTPD     HOST_BASE_ADDRESS + 0x09C
#define TXBISTD1     HOST_BASE_ADDRESS + 0x0A0
#define TXBISTD2     HOST_BASE_ADDRESS + 0x0A4
#define BISTCR       HOST_BASE_ADDRESS + 0x0A8
#define BISTFCTR     HOST_BASE_ADDRESS + 0x0AC
#define BISTSR       HOST_BASE_ADDRESS + 0x0B0
#define TESTR        HOST_BASE_ADDRESS + 0x0F4
#define VERSIONR     HOST_BASE_ADDRESS + 0x0F8
#define IDR          HOST_BASE_ADDRESS + 0x0FC 


/************************************/
/* SATA DMA CONTROLLER BASE ADDRESS */
/************************************/
#define DMAC_BASE_ADDRESS   0x400

#define MAX_DMA_BLK_SIZE                    8192 /* 8k */
#define DMAC_CHANNEL0_STATUS_TFRMASK        0x01
#define DMA_CFG_REG_ENABLE    		        0x1
#define DMA_CFG_REG_DISABLE    		        0x0
#define DMA_READ_CONTROL_L     		        0x220d824
#define DMA_WRITE_CONTROL_L     	        0x90C825
#define DMA_READ_CONFIGURATION_L 	        0x00000000
#define DMA_WRITE_CONFIGURATION_L 	        0x00001800
#define DMA_CONFIGURATION_H		            0x0802
#define DMA_CHANNEL_ENABLE		            0x101
#define DMA_DBTSR			                0x100010
#define DMA_DMACR			                0x3
#define DMACR_DISABLE						0x00


/* CHANNEL 0 PARAMETERS */
#define SAR0			DMAC_BASE_ADDRESS + 0x000
#define DAR0 	    	DMAC_BASE_ADDRESS + 0x008
#define CTL0L  	 	    DMAC_BASE_ADDRESS + 0x018
#define CTL0H  	 	    DMAC_BASE_ADDRESS + 0x01C
#define CFG0L       	DMAC_BASE_ADDRESS + 0x040
#define CFG0H       	DMAC_BASE_ADDRESS + 0x044

/* INTERRUPT REGISTERS */
#define RAW_TFR        DMAC_BASE_ADDRESS + 0x2C0
#define RAW_BLOCK      DMAC_BASE_ADDRESS + 0x2C8
#define RAW_SRCTRAN    DMAC_BASE_ADDRESS + 0x2D0
#define RAW_DSTTRAN    DMAC_BASE_ADDRESS + 0x2D8
#define RAW_ERR        DMAC_BASE_ADDRESS + 0x2E0

/* STATUS REGISTERS*/
#define STATUS_TFR     DMAC_BASE_ADDRESS + 0x2E8
#define STATUS_BLOCK   DMAC_BASE_ADDRESS + 0x2F0
#define STATUS_SRCTRAN DMAC_BASE_ADDRESS + 0x2F8
#define STATUS_DSTTRAN DMAC_BASE_ADDRESS + 0x300
#define STATUS_ERR     DMAC_BASE_ADDRESS + 0x308

/* MASK REGISTERS*/
#define MASK_TFR       DMAC_BASE_ADDRESS + 0x310
#define MASK_BLOCK     DMAC_BASE_ADDRESS + 0x318
#define MASK_SRCTRAN   DMAC_BASE_ADDRESS + 0x320
#define MASK_DSTTRAN   DMAC_BASE_ADDRESS + 0x328
#define MASK_ERR       DMAC_BASE_ADDRESS + 0x330

/* CLEAR INTERRUPT AND STATUS REGISTERS*/
#define CLEAR_TFR      DMAC_BASE_ADDRESS + 0x338
#define CLEAR_BLOCK    DMAC_BASE_ADDRESS + 0x340
#define CLEAR_SRCTRAN  DMAC_BASE_ADDRESS + 0x348
#define CLEAR_DSTTRAN  DMAC_BASE_ADDRESS + 0x350
#define CLEAR_ERR      DMAC_BASE_ADDRESS + 0x358

/* LOGICAL OR BETWEEN ALL INTERRUPT REGISTER (= INTERRUPT REG. IN OUR CASE) */
#define STATUS_INT     DMAC_BASE_ADDRESS + 0x360


/* SOFTWARE HANDSHAKING REGISTERS */
#define REQ_SRC        DMAC_BASE_ADDRESS + 0x368
#define REQ_DST        DMAC_BASE_ADDRESS + 0x370
#define SGL_REQ_SRC    DMAC_BASE_ADDRESS + 0x378
#define SGL_REQ_DEST   DMAC_BASE_ADDRESS + 0x380
#define LST_SRC        DMAC_BASE_ADDRESS + 0x388
#define LST_DST        DMAC_BASE_ADDRESS + 0x390

/* MISCELLEANOUS REGISTERS */
#define DMA_CFG_REG         DMAC_BASE_ADDRESS + 0x398
#define CH_EN_REG           DMAC_BASE_ADDRESS + 0x3A0
#define DMA_ID_REG          DMAC_BASE_ADDRESS + 0x3A8
#define DMA_TEST_REG        DMAC_BASE_ADDRESS + 0x3B0
#define DMA_COMP_VERSION    DMAC_BASE_ADDRESS + 0x3FC
#define DMA_COMP_TYPE       DMAC_BASE_ADDRESS + 0x3F8
#define CHANNEL_ENABLED     0x01

#endif/*7100*/

/* Exported Types ---------------------------------------------------------*/

typedef struct {
    semaphore_t         *InterruptSemaphore_p;
    partition_t         *DriverPartition;
    volatile U32        *BaseAddress;
    volatile U16        *HWResetAddress;
    U32                 InterruptNumber;
    U32                 InterruptLevel;
#ifdef STATAPI_SET_EMIREGISTER_MAP    
    const STATAPI_RegisterMap_t *RegMap;
#endif    
#ifdef STATAPI_HOSTCERROR_INTERRUPT_ENABLE
	U32 				SATAError;
#endif
    STATAPI_Device_t    DeviceType;
    ATA_Direction_t     DataDirection;
    U8                  Cmd_Type;
#ifdef ATAPI_FDMA
    partition_t         *NCachePartition;
    STFDMA_Node_t       *Node_p;                /* 32-byte aligned */
    STFDMA_Node_t       *OriginalNode_p;        /* STOS_MemoryAllocate */
#endif
#ifdef BMDMA_ENABLE
    semaphore_t         *BMDMA_IntSemaphore_p;
#endif
    BOOL                DmaTransfer;
    BOOL                DmaAborted;
    U32                 StoredByteCount;
} hal_Handle_t;

#pragma ST_device(DU8)
typedef volatile U8 DU8;

#pragma ST_device(DU16)
typedef volatile U16 DU16;

#pragma ST_device(DU32)
typedef volatile U32 DU32;

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)

/*** HDDI structures, ideally matching register layout ***/

/* DMA setup registers */
typedef struct {
    DU32 Control;
    DU32 SourceAddress;
    DU32 WordCount;
    DU32 DMABlockSize;
    DU32 CurrentAddress;        /* Read only */
    DU32 CurrentByteCount;      /* Read only */
    DU32 DMAChunkSize;          /* 5528 only */
} HDDI_DMASetup_t;

/* PIO timing registers */
typedef struct {
    DU32 I;
    DU32 IORDY;
    DU32 WR;
    DU32 RD;
    DU32 WREN;
    DU32 AH;
    DU32 WRRE;
    DU32 RDRE;
} HDDI_DPIOTiming_t;

/* MWDMA timing registers */
typedef struct {
    DU32 TD;
    DU32 TH;
    DU32 TJ;
    DU32 TKR;
    DU32 TKW;
    DU32 TM;
    DU32 TN;
} HDDI_MWDMATiming_t;

/* UDMA timing registers. Not listed in document in offset order, which is
  why this struct contains the elements in a slightly different order */
typedef struct {
    DU32 RP;
    DU32 ACK;
    DU32 ENV;
    DU32 TLI;
    DU32 CVS;                   /* This replaces TLI on 5528 */
    DU32 SS;
    DU32 ML;
    DU32 RFS;
    DU32 DVS;
    DU32 DVH;
} HDDI_UDMATiming_t;
#endif

/*Exported functions---------------------------------------------------------*/

BOOL hal_Init (const ST_DeviceName_t DeviceName,const STATAPI_InitParams_t *params_p,
															 hal_Handle_t **HalHndl_p);
BOOL hal_Term(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p);
BOOL hal_GetCapabilities(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl,
											STATAPI_Capability_t *Capabilities);
BOOL hal_HardReset(hal_Handle_t *HalHndl);
BOOL hal_SoftReset(hal_Handle_t *HalHndl_p);

void hal_RegOutByte(hal_Handle_t *HalHndl,ATA_Register_t regNo,U8 data);
U8   hal_RegInByte (hal_Handle_t *HalHndl,ATA_Register_t regNo);
void  hal_RegOutWord(hal_Handle_t *HalHndl,U16 data);
U16   hal_RegInWord(hal_Handle_t *HalHndl);
void ATA_BMDMA(void *Source, void *Destination, U32 Size);

void hal_RegOutBlock(hal_Handle_t *HalHndl_p, U32 *data,
                               U32 Size, BOOL UseDMA,BOOL EnableCrypt);
void hal_RegInBlock(hal_Handle_t *HalHndl_p, U32 *Data,
                               U32 Size, BOOL UseDMA,BOOL EnableCrypt);

void hal_EnableInts(hal_Handle_t *HalHndl);
void hal_DisableInts(hal_Handle_t *HalHndl);

BOOL hal_AwaitInt(hal_Handle_t *HalHndl,U32 timeout);

BOOL hal_ClearInterrupt (hal_Handle_t *HalHndl);

BOOL hal_GetDmaTiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p,
													STATAPI_DmaTiming_t *Time);
BOOL hal_GetPioTiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p,
													STATAPI_PioTiming_t *Time);

BOOL hal_SetDmaMode(hal_Handle_t *HalHndl,STATAPI_DmaMode_t mode);
BOOL hal_SetPioMode(hal_Handle_t *HalHndl,STATAPI_PioMode_t mode);
BOOL hal_SetDmaTiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p,
													STATAPI_DmaTiming_t *Time);
BOOL hal_SetPioTiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p,
													STATAPI_PioTiming_t *Time);

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)

void hal_SetMWDMATiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p,
													 STATAPI_MwDmaTiming_t *Timing);
void hal_SetUDMATiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p,
                       STATAPI_UltraDmaTiming_t *Timing);
#endif

BOOL hal_DmaDataBlock(hal_Handle_t *HalHndl_p, U8 DevCtrl, U8 DevHead,
                      U16 *StartAddress, U32 WordCount, U32 BufferSize,
                      U32 *BytesRW, BOOL Read, BOOL *CrcError);
void hal_AfterDma (hal_Handle_t *HalHndl_p);

void hal_DmaPause(hal_Handle_t *HalHndl);
void hal_DmaResume(hal_Handle_t *HalHndl);
BOOL hal_DmaAbort(hal_Handle_t *HalHndl);
void hal_ResetInterface(void);
void hal_SetRegisterMap(const STATAPI_RegisterMap_t *RegMap);
void hal_GetRegisterMap(STATAPI_RegisterMap_t *RegMap);

#ifdef __cplusplus
}
#endif

#endif /* _STATAPI_HAL_H */
